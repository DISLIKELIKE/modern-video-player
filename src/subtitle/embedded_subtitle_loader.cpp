#include "subtitle/embedded_subtitle_loader.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}

#include "logger.h"
#include "subtitle/ass_parser.h"
#include "subtitle/srt_parser.h"

namespace vp::subtitle {

namespace {

struct SubtitleDecoderDeleter {
    void operator()(AVCodecContext* codec_ctx) const noexcept {
        if (codec_ctx) {
            avcodec_free_context(&codec_ctx);
        }
    }
};

using SubtitleDecoderPtr = std::unique_ptr<AVCodecContext, SubtitleDecoderDeleter>;

struct InputContextDeleter {
    void operator()(AVFormatContext* format_ctx) const noexcept {
        if (format_ctx) {
            avformat_close_input(&format_ctx);
        }
    }
};

using InputContextPtr = std::unique_ptr<AVFormatContext, InputContextDeleter>;

struct SubtitlePacketEntry {
    double start_seconds{0.0};
    double end_seconds{0.0};
    std::string payload;
};

std::string metadataValue(const AVDictionary* metadata, const char* key) {
    if (!metadata || !key || key[0] == '\0') {
        return {};
    }
    const AVDictionaryEntry* entry = av_dict_get(metadata, key, nullptr, 0);
    return (entry && entry->value) ? std::string(entry->value) : std::string{};
}

std::string codecName(AVCodecID codec_id) {
    const AVCodecDescriptor* descriptor = avcodec_descriptor_get(codec_id);
    return descriptor && descriptor->name ? std::string(descriptor->name) : std::string("unknown");
}

std::string toLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool isSupportedTextSubtitleCodec(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_ASS:
    case AV_CODEC_ID_SSA:
    case AV_CODEC_ID_SUBRIP:
    case AV_CODEC_ID_TEXT:
    case AV_CODEC_ID_MOV_TEXT:
    case AV_CODEC_ID_WEBVTT:
        return true;
    default:
        return false;
    }
}

int subtitleCodecPreference(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_ASS:
        return 500;
    case AV_CODEC_ID_SSA:
        return 450;
    case AV_CODEC_ID_SUBRIP:
        return 300;
    case AV_CODEC_ID_WEBVTT:
        return 250;
    case AV_CODEC_ID_MOV_TEXT:
        return 220;
    case AV_CODEC_ID_TEXT:
        return 200;
    default:
        return 0;
    }
}

double timestampToSeconds(int64_t timestamp, AVRational time_base) {
    if (timestamp == AV_NOPTS_VALUE || time_base.den == 0) {
        return 0.0;
    }
    return static_cast<double>(timestamp) * av_q2d(time_base);
}

double packetStartSeconds(const AVPacket& packet, const AVStream* stream) {
    if (!stream) {
        return 0.0;
    }
    if (packet.pts != AV_NOPTS_VALUE) {
        return timestampToSeconds(packet.pts, stream->time_base);
    }
    if (packet.dts != AV_NOPTS_VALUE) {
        return timestampToSeconds(packet.dts, stream->time_base);
    }
    return 0.0;
}

double packetDurationSeconds(const AVPacket& packet, const AVStream* stream) {
    if (!stream || packet.duration <= 0 || stream->time_base.den == 0) {
        return 0.0;
    }
    return static_cast<double>(packet.duration) * av_q2d(stream->time_base);
}

void finalizeSubtitlePacketDurations(std::vector<SubtitlePacketEntry>& entries, double fallback_duration_seconds = 3.0) {
    for (size_t i = 0; i < entries.size(); ++i) {
        if (entries[i].end_seconds > entries[i].start_seconds + 0.001) {
            continue;
        }
        if (i + 1 < entries.size() && entries[i + 1].start_seconds > entries[i].start_seconds + 0.001) {
            entries[i].end_seconds = entries[i + 1].start_seconds;
        } else {
            entries[i].end_seconds = entries[i].start_seconds + fallback_duration_seconds;
        }
    }
}

std::string formatAssTimecode(double seconds) {
    const int64_t centiseconds = std::max<int64_t>(0, std::llround(seconds * 100.0));
    const int64_t total_seconds = centiseconds / 100;
    const int64_t hours = total_seconds / 3600;
    const int64_t minutes = (total_seconds / 60) % 60;
    const int64_t secs = total_seconds % 60;
    const int64_t centis = centiseconds % 100;

    std::ostringstream oss;
    oss << hours << ':'
        << std::setw(2) << std::setfill('0') << minutes << ':'
        << std::setw(2) << std::setfill('0') << secs << '.'
        << std::setw(2) << std::setfill('0') << centis;
    return oss.str();
}

std::string formatSrtTimecode(double seconds) {
    const int64_t milliseconds = std::max<int64_t>(0, std::llround(seconds * 1000.0));
    const int64_t total_seconds = milliseconds / 1000;
    const int64_t hours = total_seconds / 3600;
    const int64_t minutes = (total_seconds / 60) % 60;
    const int64_t secs = total_seconds % 60;
    const int64_t millis = milliseconds % 1000;

    std::ostringstream oss;
    oss << std::setw(2) << std::setfill('0') << hours << ':'
        << std::setw(2) << std::setfill('0') << minutes << ':'
        << std::setw(2) << std::setfill('0') << secs << ','
        << std::setw(3) << std::setfill('0') << millis;
    return oss.str();
}

int bestVideoWidth(const AVFormatContext* format_ctx) {
    if (!format_ctx) {
        return 0;
    }
    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        const AVStream* stream = format_ctx->streams[i];
        if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        if (stream->codecpar->width > 0) {
            return stream->codecpar->width;
        }
    }
    return 0;
}

int bestVideoHeight(const AVFormatContext* format_ctx) {
    if (!format_ctx) {
        return 0;
    }
    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        const AVStream* stream = format_ctx->streams[i];
        if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;
        }
        if (stream->codecpar->height > 0) {
            return stream->codecpar->height;
        }
    }
    return 0;
}

std::string buildFallbackAssHeader(const AVFormatContext* format_ctx) {
    std::ostringstream oss;
    oss << "[Script Info]\n";
    oss << "ScriptType: v4.00+\n";
    const int width = bestVideoWidth(format_ctx);
    const int height = bestVideoHeight(format_ctx);
    if (width > 0) {
        oss << "PlayResX: " << width << '\n';
    }
    if (height > 0) {
        oss << "PlayResY: " << height << '\n';
    }
    oss << '\n';
    return oss.str();
}

std::string buildAssHeader(const AVCodecContext* codec_ctx, const AVFormatContext* format_ctx) {
    if (codec_ctx && codec_ctx->subtitle_header && codec_ctx->subtitle_header_size > 0) {
        return std::string(reinterpret_cast<const char*>(codec_ctx->subtitle_header),
                           static_cast<size_t>(codec_ctx->subtitle_header_size));
    }
    if (codec_ctx && codec_ctx->extradata && codec_ctx->extradata_size > 0) {
        return std::string(reinterpret_cast<const char*>(codec_ctx->extradata),
                           static_cast<size_t>(codec_ctx->extradata_size));
    }
    return buildFallbackAssHeader(format_ctx);
}

std::string buildEmbeddedSubtitleSourceLabel(const std::string& media_source_path,
                                             int stream_index,
                                             const std::string& codec_name,
                                             const std::string& language) {
    std::ostringstream oss;
    oss << "embedded:" << media_source_path << "#stream=" << stream_index;
    if (!codec_name.empty()) {
        oss << ":codec=" << codec_name;
    }
    if (!language.empty()) {
        oss << ":lang=" << language;
    }
    return oss.str();
}

bool splitAssPayloadPrefix(const std::string& payload,
                           std::string& read_order,
                           std::string& layer,
                           std::string& rest) {
    const size_t first_comma = payload.find(',');
    if (first_comma == std::string::npos) {
        return false;
    }
    const size_t second_comma = payload.find(',', first_comma + 1);
    if (second_comma == std::string::npos) {
        return false;
    }
    read_order = payload.substr(0, first_comma);
    layer = payload.substr(first_comma + 1, second_comma - first_comma - 1);
    rest = payload.substr(second_comma + 1);
    return true;
}

std::string buildSyntheticAssDocument(const AVCodecContext* codec_ctx,
                                      const AVFormatContext* format_ctx,
                                      const std::vector<SubtitlePacketEntry>& entries) {
    std::string document = buildAssHeader(codec_ctx, format_ctx);
    if (!document.empty() && document.back() != '\n' && document.back() != '\r') {
        document.push_back('\n');
    }

    document += "\n[Events]\n";
    document += "Format: ReadOrder, Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text\n";

    for (const SubtitlePacketEntry& entry : entries) {
        std::string read_order;
        std::string layer;
        std::string rest;
        if (splitAssPayloadPrefix(entry.payload, read_order, layer, rest)) {
            document += "Dialogue: " + read_order + "," + layer + "," +
                        formatAssTimecode(entry.start_seconds) + "," +
                        formatAssTimecode(entry.end_seconds) + "," + rest + "\n";
        } else {
            document += "Dialogue: 0,0," + formatAssTimecode(entry.start_seconds) + "," +
                        formatAssTimecode(entry.end_seconds) + ",Default,,0,0,0,," + entry.payload + "\n";
        }
    }

    return document;
}

std::string stripAssMarkupToPlainText(const std::string& text) {
    std::string result;
    result.reserve(text.size());
    bool inside_override = false;
    for (size_t i = 0; i < text.size(); ++i) {
        const char ch = text[i];
        if (inside_override) {
            if (ch == '}') {
                inside_override = false;
            }
            continue;
        }
        if (ch == '{') {
            inside_override = true;
            continue;
        }
        if (ch == '\\' && i + 1 < text.size()) {
            const char next = text[i + 1];
            if (next == 'N' || next == 'n') {
                result.push_back('\n');
                ++i;
                continue;
            }
            if (next == 'h') {
                result.push_back(' ');
                ++i;
                continue;
            }
        }
        result.push_back(ch);
    }
    return result;
}

std::string extractPlainTextFromAssRectPayload(const std::string& payload) {
    size_t comma_count = 0;
    for (size_t i = 0; i < payload.size(); ++i) {
        if (payload[i] == ',') {
            ++comma_count;
            if (comma_count == 8) {
                return stripAssMarkupToPlainText(payload.substr(i + 1));
            }
        }
    }
    return stripAssMarkupToPlainText(payload);
}

std::string extractDecodedSubtitleText(const AVSubtitle& subtitle) {
    std::ostringstream oss;
    bool first = true;
    for (unsigned int i = 0; i < subtitle.num_rects; ++i) {
        const AVSubtitleRect* rect = subtitle.rects[i];
        if (!rect) {
            continue;
        }

        std::string text;
        if (rect->text) {
            text = rect->text;
        } else if (rect->ass) {
            text = extractPlainTextFromAssRectPayload(rect->ass);
        }

        if (text.empty()) {
            continue;
        }
        if (!first) {
            oss << '\n';
        }
        first = false;
        oss << text;
    }
    return oss.str();
}

SubtitleDecoderPtr openSubtitleDecoder(AVFormatContext* format_ctx, int stream_index) {
    if (!format_ctx || stream_index < 0 || stream_index >= static_cast<int>(format_ctx->nb_streams)) {
        return {};
    }

    AVStream* stream = format_ctx->streams[stream_index];
    if (!stream || !stream->codecpar) {
        return {};
    }

    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        return {};
    }

    SubtitleDecoderPtr codec_ctx(avcodec_alloc_context3(codec));
    if (!codec_ctx) {
        return {};
    }
    if (avcodec_parameters_to_context(codec_ctx.get(), stream->codecpar) < 0) {
        return {};
    }
    if (avcodec_open2(codec_ctx.get(), codec, nullptr) < 0) {
        return {};
    }
    return codec_ctx;
}

InputContextPtr openInputContext(const std::string& media_source_path) {
    AVFormatContext* raw_ctx = nullptr;
    if (avformat_open_input(&raw_ctx, media_source_path.c_str(), nullptr, nullptr) != 0) {
        return {};
    }
    InputContextPtr format_ctx(raw_ctx);
    if (avformat_find_stream_info(format_ctx.get(), nullptr) < 0) {
        return {};
    }
    return format_ctx;
}

int findBestSupportedSubtitleStream(const AVFormatContext* format_ctx,
                                    bool& any_subtitle_stream_found,
                                    bool& any_supported_stream_found) {
    any_subtitle_stream_found = false;
    any_supported_stream_found = false;

    int best_stream_index = -1;
    int best_score = std::numeric_limits<int>::min();

    if (!format_ctx) {
        return best_stream_index;
    }

    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        const AVStream* stream = format_ctx->streams[i];
        if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            continue;
        }

        any_subtitle_stream_found = true;
        if (!isSupportedTextSubtitleCodec(stream->codecpar->codec_id)) {
            continue;
        }

        any_supported_stream_found = true;
        int score = subtitleCodecPreference(stream->codecpar->codec_id);
        if ((stream->disposition & AV_DISPOSITION_DEFAULT) != 0) {
            score += 10000;
        }
        if ((stream->disposition & AV_DISPOSITION_FORCED) != 0) {
            score += 2000;
        }
        score -= static_cast<int>(i);

        if (score > best_score) {
            best_score = score;
            best_stream_index = static_cast<int>(i);
        }
    }

    return best_stream_index;
}

EmbeddedSubtitleLoadResult loadAssSubtitleTrack(AVFormatContext* format_ctx,
                                                AVCodecContext* codec_ctx,
                                                int stream_index,
                                                const std::string& media_source_path,
                                                const std::string& language,
                                                const std::string& title) {
    EmbeddedSubtitleLoadResult result;
    result.subtitle_stream_found = true;
    result.supported_stream_found = true;
    result.stream_index = stream_index;
    result.codec_name = codecName(format_ctx->streams[stream_index]->codecpar->codec_id);
    result.language = language;
    result.title = title;
    result.source_label = buildEmbeddedSubtitleSourceLabel(media_source_path, stream_index, result.codec_name, language);

    std::vector<SubtitlePacketEntry> entries;
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return result;
    }

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == stream_index && packet->data && packet->size > 0) {
            SubtitlePacketEntry entry;
            entry.start_seconds = packetStartSeconds(*packet, format_ctx->streams[stream_index]);
            entry.end_seconds = entry.start_seconds + packetDurationSeconds(*packet, format_ctx->streams[stream_index]);
            entry.payload.assign(reinterpret_cast<const char*>(packet->data),
                                 static_cast<size_t>(packet->size));
            entries.push_back(std::move(entry));
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    if (entries.empty()) {
        return result;
    }

    finalizeSubtitlePacketDurations(entries);
    const std::string ass_document = buildSyntheticAssDocument(codec_ctx, format_ctx, entries);

    AssParser parser;
    if (!parser.parseText(ass_document, media_source_path)) {
        return result;
    }

    result.items = parser.items();
    result.loaded = !result.items.empty();
    return result;
}

EmbeddedSubtitleLoadResult loadPlainTextSubtitleTrack(AVFormatContext* format_ctx,
                                                      AVCodecContext* codec_ctx,
                                                      int stream_index,
                                                      const std::string& media_source_path,
                                                      const std::string& language,
                                                      const std::string& title) {
    EmbeddedSubtitleLoadResult result;
    result.subtitle_stream_found = true;
    result.supported_stream_found = true;
    result.stream_index = stream_index;
    result.codec_name = codecName(format_ctx->streams[stream_index]->codecpar->codec_id);
    result.language = language;
    result.title = title;
    result.source_label = buildEmbeddedSubtitleSourceLabel(media_source_path, stream_index, result.codec_name, language);

    std::vector<SubtitlePacketEntry> entries;
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return result;
    }

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == stream_index) {
            AVSubtitle subtitle{};
            int got_subtitle = 0;
            const int ret = avcodec_decode_subtitle2(codec_ctx, &subtitle, &got_subtitle, packet);
            if (ret >= 0 && got_subtitle != 0) {
                std::string text = extractDecodedSubtitleText(subtitle);
                if (!text.empty()) {
                    SubtitlePacketEntry entry;
                    const double packet_start = packetStartSeconds(*packet, format_ctx->streams[stream_index]);
                    entry.start_seconds =
                        packet_start + (static_cast<double>(subtitle.start_display_time) / 1000.0);
                    if (subtitle.end_display_time > subtitle.start_display_time) {
                        entry.end_seconds =
                            packet_start + (static_cast<double>(subtitle.end_display_time) / 1000.0);
                    } else {
                        entry.end_seconds = entry.start_seconds + packetDurationSeconds(*packet, format_ctx->streams[stream_index]);
                    }
                    entry.payload = std::move(text);
                    entries.push_back(std::move(entry));
                }
                avsubtitle_free(&subtitle);
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    if (entries.empty()) {
        return result;
    }

    finalizeSubtitlePacketDurations(entries);

    std::ostringstream srt_document;
    for (size_t i = 0; i < entries.size(); ++i) {
        srt_document << (i + 1) << "\n"
                     << formatSrtTimecode(entries[i].start_seconds) << " --> "
                     << formatSrtTimecode(entries[i].end_seconds) << "\n"
                     << entries[i].payload << "\n\n";
    }

    SrtParser parser;
    if (!parser.parseText(srt_document.str(), media_source_path)) {
        return result;
    }

    result.items = parser.items();
    result.loaded = !result.items.empty();
    return result;
}

}  // namespace

EmbeddedSubtitleLoadResult loadBestEmbeddedSubtitleTrack(const std::string& media_source_path) {
    EmbeddedSubtitleLoadResult result;
    InputContextPtr format_ctx = openInputContext(media_source_path);
    if (!format_ctx) {
        LOG_WARNING("Embedded subtitle loader could not open media: " << media_source_path);
        return result;
    }

    bool any_subtitle_stream_found = false;
    bool any_supported_stream_found = false;
    const int stream_index =
        findBestSupportedSubtitleStream(format_ctx.get(), any_subtitle_stream_found, any_supported_stream_found);
    result.subtitle_stream_found = any_subtitle_stream_found;
    result.supported_stream_found = any_supported_stream_found;
    if (stream_index < 0) {
        return result;
    }

    AVStream* stream = format_ctx->streams[stream_index];
    if (!stream || !stream->codecpar) {
        return result;
    }

    const std::string language = metadataValue(stream->metadata, "language");
    const std::string title = metadataValue(stream->metadata, "title");
    SubtitleDecoderPtr codec_ctx = openSubtitleDecoder(format_ctx.get(), stream_index);
    if (!codec_ctx) {
        result.stream_index = stream_index;
        result.codec_name = codecName(stream->codecpar->codec_id);
        result.language = language;
        result.title = title;
        result.source_label = buildEmbeddedSubtitleSourceLabel(media_source_path, stream_index, result.codec_name, language);
        return result;
    }

    switch (stream->codecpar->codec_id) {
    case AV_CODEC_ID_ASS:
    case AV_CODEC_ID_SSA:
        result = loadAssSubtitleTrack(format_ctx.get(),
                                      codec_ctx.get(),
                                      stream_index,
                                      media_source_path,
                                      language,
                                      title);
        break;
    case AV_CODEC_ID_SUBRIP:
    case AV_CODEC_ID_TEXT:
    case AV_CODEC_ID_MOV_TEXT:
    case AV_CODEC_ID_WEBVTT:
        result = loadPlainTextSubtitleTrack(format_ctx.get(),
                                            codec_ctx.get(),
                                            stream_index,
                                            media_source_path,
                                            language,
                                            title);
        break;
    default:
        result.stream_index = stream_index;
        result.codec_name = codecName(stream->codecpar->codec_id);
        result.language = language;
        result.title = title;
        result.source_label = buildEmbeddedSubtitleSourceLabel(media_source_path, stream_index, result.codec_name, language);
        break;
    }

    LOG_INFO("Embedded subtitle probe: media=" << media_source_path
             << ", subtitle_stream_found=" << (result.subtitle_stream_found ? "true" : "false")
             << ", supported_stream_found=" << (result.supported_stream_found ? "true" : "false")
             << ", loaded=" << (result.loaded ? "true" : "false")
             << ", stream_index=" << result.stream_index
             << ", codec=" << result.codec_name
             << ", language=" << (result.language.empty() ? std::string("und") : result.language)
             << ", title=" << (result.title.empty() ? std::string("none") : result.title)
             << ", items=" << result.items.size());

    return result;
}

}  // namespace vp::subtitle
