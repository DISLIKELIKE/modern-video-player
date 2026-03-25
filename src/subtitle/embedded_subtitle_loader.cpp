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

bool isSupportedBitmapSubtitleCodec(AVCodecID codec_id) {
    switch (codec_id) {
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
    case AV_CODEC_ID_DVD_SUBTITLE:
        return true;
    default:
        return false;
    }
}

bool isSupportedSubtitleCodec(AVCodecID codec_id) {
    return isSupportedTextSubtitleCodec(codec_id) || isSupportedBitmapSubtitleCodec(codec_id);
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
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
        return 190;
    case AV_CODEC_ID_DVD_SUBTITLE:
        return 170;
    default:
        return 0;
    }
}

int subtitleTrackPreferenceScore(const AVStream* stream) {
    if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        return std::numeric_limits<int>::min();
    }
    if (!isSupportedSubtitleCodec(stream->codecpar->codec_id)) {
        return std::numeric_limits<int>::min();
    }

    int score = subtitleCodecPreference(stream->codecpar->codec_id);
    if ((stream->disposition & AV_DISPOSITION_DEFAULT) != 0) {
        score += 10000;
    }
    if ((stream->disposition & AV_DISPOSITION_FORCED) != 0) {
        score += 2000;
    }
    score -= static_cast<int>(stream->index);
    return score;
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

EmbeddedSubtitleTrackInfo buildTrackInfo(const AVStream* stream) {
    EmbeddedSubtitleTrackInfo info;
    if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        return info;
    }
    info.stream_index = stream->index;
    info.codec_name = codecName(stream->codecpar->codec_id);
    info.language = metadataValue(stream->metadata, "language");
    info.title = metadataValue(stream->metadata, "title");
    info.is_default = (stream->disposition & AV_DISPOSITION_DEFAULT) != 0;
    info.is_forced = (stream->disposition & AV_DISPOSITION_FORCED) != 0;
    info.is_hearing_impaired = (stream->disposition & AV_DISPOSITION_HEARING_IMPAIRED) != 0;
    info.supported_text_codec = isSupportedTextSubtitleCodec(stream->codecpar->codec_id);
    info.supported_bitmap_codec = isSupportedBitmapSubtitleCodec(stream->codecpar->codec_id);
    info.supported_codec = info.supported_text_codec || info.supported_bitmap_codec;
    info.preference_score = subtitleTrackPreferenceScore(stream);
    return info;
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

bool decodeBitmapSubtitleRect(const AVSubtitleRect* rect, SubtitleBitmap& out_bitmap) {
    if (!rect || rect->w <= 0 || rect->h <= 0) {
        return false;
    }
    if (!rect->data[0] || !rect->data[1] || rect->linesize[0] <= 0) {
        return false;
    }

    const int width = rect->w;
    const int height = rect->h;
    const int stride = rect->linesize[0];
    if (stride < width) {
        return false;
    }

    const uint8_t* indices = rect->data[0];
    const auto* palette = reinterpret_cast<const uint32_t*>(rect->data[1]);

    out_bitmap = SubtitleBitmap{};
    out_bitmap.x = rect->x;
    out_bitmap.y = rect->y;
    out_bitmap.width = width;
    out_bitmap.height = height;
    out_bitmap.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);

    for (int y = 0; y < height; ++y) {
        const uint8_t* src_row = indices + static_cast<size_t>(y) * static_cast<size_t>(stride);
        uint8_t* dst_row = out_bitmap.rgba.data() + static_cast<size_t>(y) * static_cast<size_t>(width) * 4u;
        for (int x = 0; x < width; ++x) {
            const uint32_t color = palette[src_row[x]];
            const uint8_t b = static_cast<uint8_t>(color & 0xffu);
            const uint8_t g = static_cast<uint8_t>((color >> 8u) & 0xffu);
            const uint8_t r = static_cast<uint8_t>((color >> 16u) & 0xffu);
            const uint8_t a = static_cast<uint8_t>((color >> 24u) & 0xffu);

            uint8_t* pixel = dst_row + static_cast<size_t>(x) * 4u;
            pixel[0] = r;
            pixel[1] = g;
            pixel[2] = b;
            pixel[3] = a;
        }
    }

    return true;
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
    result.bitmap_codec = false;
    result.bitmap_item_count = 0;

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
    result.bitmap_item_count = 0;
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
    result.bitmap_codec = false;
    result.bitmap_item_count = 0;

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
    result.bitmap_item_count = 0;
    return result;
}

EmbeddedSubtitleLoadResult loadBitmapSubtitleTrack(AVFormatContext* format_ctx,
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
    result.bitmap_codec = true;
    result.bitmap_item_count = 0;

    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return result;
    }

    std::vector<SubtitleItem> items;
    int item_index = 0;
    const int video_width = bestVideoWidth(format_ctx);
    const int video_height = bestVideoHeight(format_ctx);

    while (av_read_frame(format_ctx, packet) >= 0) {
        if (packet->stream_index == stream_index) {
            AVSubtitle subtitle{};
            int got_subtitle = 0;
            const int decode_ret = avcodec_decode_subtitle2(codec_ctx, &subtitle, &got_subtitle, packet);
            if (decode_ret >= 0 && got_subtitle != 0) {
                const double packet_start = packetStartSeconds(*packet, format_ctx->streams[stream_index]);
                double start_seconds = packet_start + (static_cast<double>(subtitle.start_display_time) / 1000.0);
                double end_seconds = packet_start + (static_cast<double>(subtitle.end_display_time) / 1000.0);
                if (end_seconds <= start_seconds + 0.001) {
                    end_seconds = start_seconds + std::max(0.5, packetDurationSeconds(*packet, format_ctx->streams[stream_index]));
                }

                for (unsigned int i = 0; i < subtitle.num_rects; ++i) {
                    const AVSubtitleRect* rect = subtitle.rects[i];
                    if (!rect || rect->type != SUBTITLE_BITMAP) {
                        continue;
                    }

                    SubtitleBitmap bitmap;
                    if (!decodeBitmapSubtitleRect(rect, bitmap)) {
                        continue;
                    }

                    SubtitleItem item;
                    item.index = item_index++;
                    item.layer = static_cast<int>(i);
                    item.start_seconds = start_seconds;
                    item.end_seconds = end_seconds;
                    item.source_path = result.source_label;
                    item.text = "[bitmap subtitle]";
                    item.raw_text = item.text;
                    item.play_res_x = video_width;
                    item.play_res_y = video_height;
                    item.is_bitmap = true;
                    item.bitmap = std::move(bitmap);
                    items.push_back(std::move(item));
                }
                avsubtitle_free(&subtitle);
            }
        }
        av_packet_unref(packet);
    }
    av_packet_free(&packet);

    if (!items.empty()) {
        std::sort(items.begin(), items.end(), [](const SubtitleItem& lhs, const SubtitleItem& rhs) {
            if (lhs.start_seconds != rhs.start_seconds) {
                return lhs.start_seconds < rhs.start_seconds;
            }
            if (lhs.layer != rhs.layer) {
                return lhs.layer < rhs.layer;
            }
            if (lhs.end_seconds != rhs.end_seconds) {
                return lhs.end_seconds < rhs.end_seconds;
            }
            return lhs.index < rhs.index;
        });
    }

    result.items = std::move(items);
    result.bitmap_item_count = result.items.size();
    result.loaded = !result.items.empty();
    return result;
}

}  // namespace

std::vector<EmbeddedSubtitleTrackInfo> listEmbeddedSubtitleTracks(const std::string& media_source_path) {
    std::vector<EmbeddedSubtitleTrackInfo> tracks;
    InputContextPtr format_ctx = openInputContext(media_source_path);
    if (!format_ctx) {
        LOG_WARNING("Embedded subtitle track listing failed to open media: " << media_source_path);
        return tracks;
    }

    tracks.reserve(format_ctx->nb_streams);
    for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
        const AVStream* stream = format_ctx->streams[i];
        if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
            continue;
        }
        tracks.push_back(buildTrackInfo(stream));
    }

    LOG_INFO("Embedded subtitle track listing: media=" << media_source_path
             << ", subtitle_tracks=" << tracks.size());
    return tracks;
}

int selectBestEmbeddedSubtitleStream(const std::vector<EmbeddedSubtitleTrackInfo>& tracks) {
    int best_stream_index = -1;
    int best_score = std::numeric_limits<int>::min();
    for (const EmbeddedSubtitleTrackInfo& track : tracks) {
        if (!track.supported_codec) {
            continue;
        }
        if (track.preference_score > best_score) {
            best_score = track.preference_score;
            best_stream_index = track.stream_index;
        }
    }
    return best_stream_index;
}

EmbeddedSubtitleLoadResult loadEmbeddedSubtitleTrack(const std::string& media_source_path, int stream_index) {
    EmbeddedSubtitleLoadResult result;
    InputContextPtr format_ctx = openInputContext(media_source_path);
    if (!format_ctx) {
        LOG_WARNING("Embedded subtitle loader could not open media: " << media_source_path);
        return result;
    }

    if (stream_index < 0 || stream_index >= static_cast<int>(format_ctx->nb_streams)) {
        bool any_subtitle_stream_found = false;
        bool any_supported_stream_found = false;
        for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
            const AVStream* stream = format_ctx->streams[i];
            if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                continue;
            }
            any_subtitle_stream_found = true;
            if (isSupportedSubtitleCodec(stream->codecpar->codec_id)) {
                any_supported_stream_found = true;
            }
        }
        result.subtitle_stream_found = any_subtitle_stream_found;
        result.supported_stream_found = any_supported_stream_found;
        return result;
    }

    AVStream* stream = format_ctx->streams[stream_index];
    if (!stream || !stream->codecpar || stream->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
        bool any_supported_stream_found = false;
        for (unsigned int i = 0; i < format_ctx->nb_streams; ++i) {
            const AVStream* candidate = format_ctx->streams[i];
            if (!candidate || !candidate->codecpar || candidate->codecpar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                continue;
            }
            if (isSupportedSubtitleCodec(candidate->codecpar->codec_id)) {
                any_supported_stream_found = true;
                break;
            }
        }
        result.subtitle_stream_found = false;
        result.supported_stream_found = any_supported_stream_found;
        return result;
    }

    const std::string language = metadataValue(stream->metadata, "language");
    const std::string title = metadataValue(stream->metadata, "title");
    result.subtitle_stream_found = true;
    result.supported_stream_found = isSupportedSubtitleCodec(stream->codecpar->codec_id);
    result.stream_index = stream_index;
    result.codec_name = codecName(stream->codecpar->codec_id);
    result.language = language;
    result.title = title;
    result.source_label = buildEmbeddedSubtitleSourceLabel(media_source_path, stream_index, result.codec_name, language);
    result.bitmap_codec = isSupportedBitmapSubtitleCodec(stream->codecpar->codec_id);
    result.bitmap_item_count = 0;

    if (!result.supported_stream_found) {
        return result;
    }

    SubtitleDecoderPtr codec_ctx = openSubtitleDecoder(format_ctx.get(), stream_index);
    if (!codec_ctx) {
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
    case AV_CODEC_ID_HDMV_PGS_SUBTITLE:
    case AV_CODEC_ID_DVD_SUBTITLE:
        result = loadBitmapSubtitleTrack(format_ctx.get(),
                                         codec_ctx.get(),
                                         stream_index,
                                         media_source_path,
                                         language,
                                         title);
        break;
    default:
        break;
    }

    LOG_INFO("Embedded subtitle track load: media=" << media_source_path
             << ", stream_index=" << result.stream_index
             << ", codec=" << result.codec_name
             << ", language=" << (result.language.empty() ? std::string("und") : result.language)
             << ", title=" << (result.title.empty() ? std::string("none") : result.title)
             << ", loaded=" << (result.loaded ? "true" : "false")
             << ", items=" << result.items.size());

    return result;
}

EmbeddedSubtitleLoadResult loadBestEmbeddedSubtitleTrack(const std::string& media_source_path) {
    EmbeddedSubtitleLoadResult result;
    const std::vector<EmbeddedSubtitleTrackInfo> tracks = listEmbeddedSubtitleTracks(media_source_path);
    result.subtitle_stream_found = !tracks.empty();
    result.supported_stream_found =
        std::any_of(tracks.begin(), tracks.end(), [](const EmbeddedSubtitleTrackInfo& track) {
            return track.supported_codec;
        });
    if (!result.supported_stream_found) {
        return result;
    }

    const int stream_index = selectBestEmbeddedSubtitleStream(tracks);
    if (stream_index < 0) {
        return result;
    }

    result = loadEmbeddedSubtitleTrack(media_source_path, stream_index);

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
