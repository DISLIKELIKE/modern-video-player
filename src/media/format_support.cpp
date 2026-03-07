#include "media/format_support.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavcodec/codec_desc.h>
#include <libavformat/avformat.h>
}

namespace vp::media {

namespace {

std::string normalizeToken(const std::string& text) {
    std::string normalized;
    normalized.reserve(text.size());
    for (char ch : text) {
        if (ch == '.' || ch == '-' || ch == ' ') {
            continue;
        }
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
    }
    return normalized;
}

void addCsvTokensToSet(std::set<std::string>& output, const char* csv) {
    if (!csv || *csv == '\0') {
        return;
    }
    std::stringstream ss(csv);
    std::string token;
    while (std::getline(ss, token, ',')) {
        token = normalizeToken(token);
        if (!token.empty()) {
            output.insert(token);
        }
    }
}

std::vector<std::string> toVector(const std::set<std::string>& values) {
    return std::vector<std::string>(values.begin(), values.end());
}

const std::vector<std::string>& extensions() {
    static const std::vector<std::string> kExtensions = {
        "3gp", "aac", "avi", "flac", "flv", "m2ts", "m4a", "m4v", "mkv", "mov",
        "mp3", "mp4", "mpeg", "mpg", "ogg", "ts", "wav", "webm", "wmv"
    };
    return kExtensions;
}

const std::vector<std::string>& videoCodecs() {
    static const std::vector<std::string> kVideoCodecs = {
        "av1", "h264", "h265", "hevc", "mpeg2video", "vp8", "vp9"
    };
    return kVideoCodecs;
}

const std::vector<std::string>& audioCodecs() {
    static const std::vector<std::string> kAudioCodecs = {
        "aac", "ac3", "dca", "dts", "eac3", "flac", "mp3", "opus", "pcm",
        "pcm_s16le", "vorbis"
    };
    return kAudioCodecs;
}

bool containsLikelyCodec(const std::vector<std::string>& codecs, const std::string& codec_name) {
    const std::string normalized = normalizeToken(codec_name);
    if (normalized.empty()) {
        return false;
    }

    if (std::find(codecs.begin(), codecs.end(), normalized) != codecs.end()) {
        return true;
    }
    if (normalized == "hevc") {
        return std::find(codecs.begin(), codecs.end(), "h265") != codecs.end();
    }
    if (normalized == "h265") {
        return std::find(codecs.begin(), codecs.end(), "hevc") != codecs.end();
    }
    if (normalized == "dts") {
        return std::find(codecs.begin(), codecs.end(), "dca") != codecs.end();
    }
    if (normalized == "dca") {
        return std::find(codecs.begin(), codecs.end(), "dts") != codecs.end();
    }
    return false;
}

bool hasRuntimeDecoder(const std::string& codec_name, AVMediaType media_type) {
    const std::string normalized = normalizeToken(codec_name);
    if (normalized.empty()) {
        return false;
    }

    if (const AVCodec* by_name = avcodec_find_decoder_by_name(normalized.c_str())) {
        return by_name->type == media_type;
    }

    if (const AVCodecDescriptor* descriptor = avcodec_descriptor_get_by_name(normalized.c_str())) {
        if (const AVCodec* by_id = avcodec_find_decoder(descriptor->id)) {
            return by_id->type == media_type;
        }
    }
    return false;
}

PlaybackCapabilityDecision evaluateTargetHeuristics(const PlaybackCapabilityTarget& target) {
    PlaybackCapabilityDecision decision{};

    if (target.width <= 0 || target.height <= 0) {
        decision.suitable_for_realtime = false;
        decision.reason = "invalid resolution target";
        return decision;
    }

    const uint64_t pixel_count = static_cast<uint64_t>(target.width) * static_cast<uint64_t>(target.height);
    const bool is_4k_or_above = pixel_count >= (3840ull * 2160ull);
    const bool is_8k_or_above = pixel_count >= (7680ull * 4320ull);
    const bool high_fps = target.fps >= 60.0;
    const bool very_high_fps = target.fps > 120.0;
    const bool high_bitrate = target.video_bitrate >= 50ull * 1000ull * 1000ull;
    const bool very_high_bitrate = target.video_bitrate >= 120ull * 1000ull * 1000ull;

    decision.recommends_hardware_decode = is_4k_or_above || high_fps || high_bitrate;
    decision.recommends_d3d11_renderer = is_4k_or_above || high_fps;

    if ((is_8k_or_above && high_fps) || (is_4k_or_above && very_high_fps) || (is_8k_or_above && very_high_bitrate)) {
        decision.suitable_for_realtime = false;
    }

    std::ostringstream reason;
    if (decision.suitable_for_realtime) {
        reason << "target is feasible";
    } else {
        reason << "target may exceed realtime budget";
    }

    if (decision.recommends_hardware_decode) {
        reason << ", prefer hardware decode";
    }
    if (decision.recommends_d3d11_renderer) {
        reason << ", prefer D3D11 renderer";
    }
    if (target.audio_channels > 2) {
        reason << ", verify " << target.audio_channels << "ch output path";
    }

    decision.reason = reason.str();
    return decision;
}

}  // namespace

bool FormatSupport::isContainerSupported(const std::string& extension) {
    std::string normalized = extension;
    if (!normalized.empty() && normalized.front() == '.') {
        normalized.erase(normalized.begin());
    }
    normalized = normalizeToken(normalized);

    const auto& ext = extensions();
    return std::find(ext.begin(), ext.end(), normalized) != ext.end();
}

bool FormatSupport::isVideoCodecLikelySupported(const std::string& codec_name) {
    return containsLikelyCodec(videoCodecs(), codec_name) ||
           hasRuntimeDecoder(codec_name, AVMEDIA_TYPE_VIDEO);
}

bool FormatSupport::isAudioCodecLikelySupported(const std::string& codec_name) {
    return containsLikelyCodec(audioCodecs(), codec_name) ||
           hasRuntimeDecoder(codec_name, AVMEDIA_TYPE_AUDIO);
}

std::vector<std::string> FormatSupport::supportedContainers() {
    return extensions();
}

std::vector<std::string> FormatSupport::supportedVideoCodecs() {
    return videoCodecs();
}

std::vector<std::string> FormatSupport::supportedAudioCodecs() {
    return audioCodecs();
}

bool FormatSupport::supportsMkvChapters() {
    return true;
}

bool FormatSupport::supportsMp4MoovPreload() {
    return true;
}

FormatCapabilityReport FormatSupport::queryRuntimeCapabilities() {
    FormatCapabilityReport report;
    std::set<std::string> containers;
    std::set<std::string> video_codecs;
    std::set<std::string> audio_codecs;

#if LIBAVFORMAT_VERSION_MAJOR >= 58
    void* demuxer_opaque = nullptr;
    const AVInputFormat* demuxer = nullptr;
    while ((demuxer = av_demuxer_iterate(&demuxer_opaque)) != nullptr) {
        addCsvTokensToSet(containers, demuxer->extensions);
        addCsvTokensToSet(containers, demuxer->name);
    }
#else
    const AVInputFormat* demuxer = nullptr;
    while ((demuxer = av_iformat_next(demuxer)) != nullptr) {
        addCsvTokensToSet(containers, demuxer->extensions);
        addCsvTokensToSet(containers, demuxer->name);
    }
#endif

#if LIBAVCODEC_VERSION_MAJOR >= 58
    void* codec_opaque = nullptr;
    const AVCodec* codec = nullptr;
    while ((codec = av_codec_iterate(&codec_opaque)) != nullptr) {
        if (!av_codec_is_decoder(codec) || !codec->name) {
            continue;
        }
        const std::string codec_name = normalizeToken(codec->name);
        if (codec_name.empty()) {
            continue;
        }
        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            video_codecs.insert(codec_name);
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            audio_codecs.insert(codec_name);
        }
    }
#else
    const AVCodec* codec = nullptr;
    while ((codec = av_codec_next(codec)) != nullptr) {
        if (!av_codec_is_decoder(codec) || !codec->name) {
            continue;
        }
        const std::string codec_name = normalizeToken(codec->name);
        if (codec_name.empty()) {
            continue;
        }
        if (codec->type == AVMEDIA_TYPE_VIDEO) {
            video_codecs.insert(codec_name);
        } else if (codec->type == AVMEDIA_TYPE_AUDIO) {
            audio_codecs.insert(codec_name);
        }
    }
#endif

    if (containers.empty()) {
        const auto fallback = supportedContainers();
        containers.insert(fallback.begin(), fallback.end());
    }
    if (video_codecs.empty()) {
        const auto fallback = supportedVideoCodecs();
        video_codecs.insert(fallback.begin(), fallback.end());
    }
    if (audio_codecs.empty()) {
        const auto fallback = supportedAudioCodecs();
        audio_codecs.insert(fallback.begin(), fallback.end());
    }

    report.containers = toVector(containers);
    report.video_codecs = toVector(video_codecs);
    report.audio_codecs = toVector(audio_codecs);
    return report;
}

PlaybackCapabilityDecision FormatSupport::evaluatePlaybackTarget(const PlaybackCapabilityTarget& target) {
    return evaluateTargetHeuristics(target);
}

}  // namespace vp::media
