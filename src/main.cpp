#include "video_player.h"
#include "config/settings_manager.h"
#include "demuxer.h"
#include "media/format_support.h"
#include "playlist/playlist_manager.h"
#include "logger.h"
#include "subtitle/srt_parser.h"
#include "subtitle/subtitle_timeline.h"

extern "C" {
#include <libavutil/log.h>
}

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <system_error>
#include <thread>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

using namespace vp;

static std::unique_ptr<VideoPlayer> g_player;

namespace {

std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

std::string extensionFromPath(const std::string& path) {
    std::filesystem::path fs_path(path);
    std::string ext = fs_path.extension().string();
    if (!ext.empty() && ext.front() == '.') {
        ext.erase(ext.begin());
    }
    return toLower(ext);
}

std::string joinTopN(const std::vector<std::string>& values, size_t limit = 16) {
    std::ostringstream oss;
    const size_t count = std::min(limit, values.size());
    for (size_t i = 0; i < count; ++i) {
        if (i > 0) {
            oss << ", ";
        }
        oss << values[i];
    }
    if (values.size() > limit) {
        oss << ", ...";
    }
    return oss.str();
}

std::string coverageLevel(size_t hit, size_t total) {
    if (total == 0 || hit == 0) {
        return "FAIL";
    }
    if (hit == total) {
        return "PASS";
    }
    return "PARTIAL";
}

void printCoverageLine(const std::string& category, size_t hit, size_t total, const std::vector<std::string>& missing) {
    std::cout << std::left << std::setw(12) << category
              << " " << std::setw(8) << coverageLevel(hit, total)
              << " (" << hit << "/" << total << ")";
    if (!missing.empty()) {
        std::cout << " missing: " << joinTopN(missing, 8);
    }
    std::cout << std::endl;
}

void printCapabilityMatrix() {
    const auto report = media::FormatSupport::queryRuntimeCapabilities();

    const std::vector<std::string> required_containers = {
        "mp4", "mkv", "mov", "avi", "webm", "flv", "ts", "m2ts"
    };
    const std::vector<std::string> required_video = {
        "h264", "hevc", "vp9", "av1", "mpeg2video"
    };
    const std::vector<std::string> required_audio = {
        "aac", "mp3", "ac3", "eac3", "dts", "flac", "opus", "vorbis", "pcm_s16le"
    };

    auto collect_missing = [](const std::vector<std::string>& required,
                              const std::function<bool(const std::string&)>& checker,
                              size_t& hit_out) {
        std::vector<std::string> missing;
        hit_out = 0;
        for (const auto& item : required) {
            if (checker(item)) {
                ++hit_out;
            } else {
                missing.push_back(item);
            }
        }
        return missing;
    };

    size_t container_hit = 0;
    const auto missing_containers = collect_missing(
        required_containers,
        [](const std::string& item) { return media::FormatSupport::isContainerSupported(item); },
        container_hit);
    size_t video_hit = 0;
    const auto missing_video = collect_missing(
        required_video,
        [](const std::string& item) { return media::FormatSupport::isVideoCodecLikelySupported(item); },
        video_hit);
    size_t audio_hit = 0;
    const auto missing_audio = collect_missing(
        required_audio,
        [](const std::string& item) { return media::FormatSupport::isAudioCodecLikelySupported(item); },
        audio_hit);

    std::cout << "=== Runtime Capability Report ===" << std::endl;
    std::cout << "Containers detected: " << report.containers.size() << std::endl;
    std::cout << "Video decoders detected: " << report.video_codecs.size() << std::endl;
    std::cout << "Audio decoders detected: " << report.audio_codecs.size() << std::endl;
    std::cout << std::endl;

    std::cout << "Top containers : " << joinTopN(report.containers) << std::endl;
    std::cout << "Top vcodecs    : " << joinTopN(report.video_codecs) << std::endl;
    std::cout << "Top acodecs    : " << joinTopN(report.audio_codecs) << std::endl;
    std::cout << std::endl;

    std::cout << "=== Mainstream Coverage Matrix ===" << std::endl;
    printCoverageLine("Container", container_hit, required_containers.size(), missing_containers);
    printCoverageLine("VideoCodec", video_hit, required_video.size(), missing_video);
    printCoverageLine("AudioCodec", audio_hit, required_audio.size(), missing_audio);
}

bool tryParseInt(const char* text, int& value) {
    if (!text) {
        return false;
    }
    char* end = nullptr;
    const long parsed = std::strtol(text, &end, 10);
    if (end == text || *end != '\0') {
        return false;
    }
    value = static_cast<int>(parsed);
    return true;
}

bool tryParseDouble(const char* text, double& value) {
    if (!text) {
        return false;
    }
    char* end = nullptr;
    const double parsed = std::strtod(text, &end);
    if (end == text || *end != '\0') {
        return false;
    }
    value = parsed;
    return true;
}

void printTargetDecision(const media::PlaybackCapabilityTarget& target) {
    const auto decision = media::FormatSupport::evaluatePlaybackTarget(target);
    std::cout << "=== Playback Target Evaluation ===" << std::endl;
    std::cout << "Target: " << target.width << "x" << target.height
              << " @" << target.fps << "fps, "
              << target.audio_channels << "ch, "
              << (target.video_bitrate / 1000000ULL) << "Mbps" << std::endl;
    std::cout << "suitable_for_realtime: " << (decision.suitable_for_realtime ? "true" : "false") << std::endl;
    std::cout << "recommends_hardware_decode: " << (decision.recommends_hardware_decode ? "true" : "false") << std::endl;
    std::cout << "recommends_d3d11_renderer: " << (decision.recommends_d3d11_renderer ? "true" : "false") << std::endl;
    std::cout << "reason: " << decision.reason << std::endl;
}

struct ScopedAvLogLevel {
    explicit ScopedAvLogLevel(int level)
        : previous_level(av_log_get_level()) {
        av_log_set_level(level);
    }

    ~ScopedAvLogLevel() {
        av_log_set_level(previous_level);
    }

    int previous_level;
};

struct ProbeReport {
    std::string path;
    std::string open = "FAIL";
    std::string overall = "FAIL";
    std::string container_ext;
    std::string container_status = "FAIL";
    int video_stream = -1;
    std::string video_codec = "none";
    std::string video_status = "FAIL";
    int audio_stream = -1;
    std::string audio_codec = "none";
    std::string audio_status = "FAIL";
    int width = 0;
    int height = 0;
    double fps = 0.0;
    int audio_channels = 0;
    double duration = 0.0;
    bool realtime = false;
    bool recommend_hw = false;
    bool recommend_d3d11 = false;
    std::string reason = "probe not executed";
    int exit_code = 3;
};

std::string jsonEscape(const std::string& input) {
    std::ostringstream escaped;
    for (char ch : input) {
        switch (ch) {
            case '"': escaped << "\\\""; break;
            case '\\': escaped << "\\\\"; break;
            case '\b': escaped << "\\b"; break;
            case '\f': escaped << "\\f"; break;
            case '\n': escaped << "\\n"; break;
            case '\r': escaped << "\\r"; break;
            case '\t': escaped << "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    escaped << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                            << static_cast<int>(static_cast<unsigned char>(ch)) << std::dec;
                } else {
                    escaped << ch;
                }
                break;
        }
    }
    return escaped.str();
}

void detectProbeStreams(AVFormatContext* fmt_ctx, int& video_stream_idx, int& audio_stream_idx) {
    video_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    audio_stream_idx = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, video_stream_idx, nullptr, 0);

    for (unsigned int i = 0; i < fmt_ctx->nb_streams; ++i) {
        AVStream* stream = fmt_ctx->streams[i];
        if (!stream || !stream->codecpar) {
            continue;
        }
        if (video_stream_idx < 0 && stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_idx = static_cast<int>(i);
        }
        if (audio_stream_idx < 0 && stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = static_cast<int>(i);
        }
    }
}

ProbeReport collectFileProbeReport(const std::string& path) {
    ProbeReport report;
    report.path = path;

    ScopedAvLogLevel quiet_logs(AV_LOG_QUIET);

    AVFormatContext* fmt_ctx = nullptr;
    AVDictionary* options = nullptr;
    av_dict_set(&options, "probesize", "104857600", 0);
    av_dict_set(&options, "analyzeduration", "10000000", 0);
    av_dict_set(&options, "fflags", "+genpts", 0);

    const int open_ret = avformat_open_input(&fmt_ctx, path.c_str(), nullptr, &options);
    av_dict_free(&options);
    if (open_ret != 0 || !fmt_ctx) {
        report.reason = "failed to open input";
        return report;
    }

    report.open = "PASS";

    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {
        report.reason = "failed to read stream info";
        avformat_close_input(&fmt_ctx);
        return report;
    }

    detectProbeStreams(fmt_ctx, report.video_stream, report.audio_stream);

    report.container_ext = extensionFromPath(path);
    const bool container_supported = media::FormatSupport::isContainerSupported(report.container_ext);
    report.container_status = container_supported ? "PASS" : "PARTIAL";

    bool video_status_ok = false;
    bool audio_status_ok = false;
    uint64_t video_bitrate = 0;

    if (report.video_stream >= 0 &&
        report.video_stream < static_cast<int>(fmt_ctx->nb_streams)) {
        AVStream* video_stream = fmt_ctx->streams[report.video_stream];
        report.video_codec = avcodec_get_name(video_stream->codecpar->codec_id);
        video_status_ok = media::FormatSupport::isVideoCodecLikelySupported(report.video_codec);
        report.video_status = video_status_ok ? "PASS" : "PARTIAL";
        report.width = video_stream->codecpar->width;
        report.height = video_stream->codecpar->height;
        const AVRational fps_rate = video_stream->avg_frame_rate.num > 0
            ? video_stream->avg_frame_rate
            : video_stream->r_frame_rate;
        if (fps_rate.num > 0 && fps_rate.den > 0) {
            report.fps = av_q2d(fps_rate);
        }
        if (video_stream->codecpar->bit_rate > 0) {
            video_bitrate = static_cast<uint64_t>(video_stream->codecpar->bit_rate);
        }
    } else {
        report.video_status = "PARTIAL";
        report.video_codec = "none";
    }

    if (report.audio_stream >= 0 &&
        report.audio_stream < static_cast<int>(fmt_ctx->nb_streams)) {
        AVStream* audio_stream = fmt_ctx->streams[report.audio_stream];
        report.audio_codec = avcodec_get_name(audio_stream->codecpar->codec_id);
        audio_status_ok = media::FormatSupport::isAudioCodecLikelySupported(report.audio_codec);
        report.audio_status = audio_status_ok ? "PASS" : "PARTIAL";
        report.audio_channels = std::max(1, audio_stream->codecpar->ch_layout.nb_channels);
    } else {
        report.audio_status = "PARTIAL";
        report.audio_codec = "none";
        report.audio_channels = 0;
    }

    if (fmt_ctx->duration > 0) {
        report.duration = fmt_ctx->duration / static_cast<double>(AV_TIME_BASE);
    }

    media::PlaybackCapabilityTarget target{};
    target.width = report.width;
    target.height = report.height;
    target.fps = report.fps;
    target.audio_channels = std::max(1, report.audio_channels);
    target.video_bitrate = video_bitrate;
    const auto decision = media::FormatSupport::evaluatePlaybackTarget(target);
    report.realtime = decision.suitable_for_realtime;
    report.recommend_hw = decision.recommends_hardware_decode;
    report.recommend_d3d11 = decision.recommends_d3d11_renderer;
    report.reason = decision.reason;

    const bool partial = !container_supported || !video_status_ok || !audio_status_ok;
    report.overall = partial ? "PARTIAL" : "PASS";
    report.exit_code = partial ? 2 : 0;

    avformat_close_input(&fmt_ctx);
    return report;
}

void printFileProbeTextReport(const ProbeReport& report) {
    std::cout << "probe.path=" << report.path << std::endl;
    std::cout << "probe.open=" << report.open << std::endl;
    std::cout << "probe.overall=" << report.overall << std::endl;
    std::cout << "probe.container_ext=" << report.container_ext << std::endl;
    std::cout << "probe.container_status=" << report.container_status << std::endl;
    std::cout << "probe.video_stream=" << report.video_stream << std::endl;
    std::cout << "probe.video_codec=" << report.video_codec << std::endl;
    std::cout << "probe.video_status=" << report.video_status << std::endl;
    std::cout << "probe.audio_stream=" << report.audio_stream << std::endl;
    std::cout << "probe.audio_codec=" << report.audio_codec << std::endl;
    std::cout << "probe.audio_status=" << report.audio_status << std::endl;
    std::cout << "probe.width=" << report.width << std::endl;
    std::cout << "probe.height=" << report.height << std::endl;
    std::cout << "probe.fps=" << report.fps << std::endl;
    std::cout << "probe.audio_channels=" << report.audio_channels << std::endl;
    std::cout << "probe.duration=" << report.duration << std::endl;
    std::cout << "probe.realtime=" << (report.realtime ? "true" : "false") << std::endl;
    std::cout << "probe.recommend_hw=" << (report.recommend_hw ? "true" : "false") << std::endl;
    std::cout << "probe.recommend_d3d11=" << (report.recommend_d3d11 ? "true" : "false") << std::endl;
    std::cout << "probe.reason=" << report.reason << std::endl;
}

void printFileProbeJsonReport(const ProbeReport& report) {
    std::ostringstream json;
    json << "{\n";
    json << "  \"path\": \"" << jsonEscape(report.path) << "\",\n";
    json << "  \"open\": \"" << report.open << "\",\n";
    json << "  \"overall\": \"" << report.overall << "\",\n";
    json << "  \"container\": {\n";
    json << "    \"ext\": \"" << jsonEscape(report.container_ext) << "\",\n";
    json << "    \"status\": \"" << report.container_status << "\"\n";
    json << "  },\n";
    json << "  \"video\": {\n";
    json << "    \"stream\": " << report.video_stream << ",\n";
    json << "    \"codec\": \"" << jsonEscape(report.video_codec) << "\",\n";
    json << "    \"status\": \"" << report.video_status << "\",\n";
    json << "    \"width\": " << report.width << ",\n";
    json << "    \"height\": " << report.height << ",\n";
    json << "    \"fps\": " << std::fixed << std::setprecision(3) << report.fps << "\n";
    json << "  },\n";
    json << "  \"audio\": {\n";
    json << "    \"stream\": " << report.audio_stream << ",\n";
    json << "    \"codec\": \"" << jsonEscape(report.audio_codec) << "\",\n";
    json << "    \"status\": \"" << report.audio_status << "\",\n";
    json << "    \"channels\": " << report.audio_channels << "\n";
    json << "  },\n";
    json << "  \"recommendation\": {\n";
    json << "    \"realtime\": " << (report.realtime ? "true" : "false") << ",\n";
    json << "    \"hardware_decode\": " << (report.recommend_hw ? "true" : "false") << ",\n";
    json << "    \"d3d11_renderer\": " << (report.recommend_d3d11 ? "true" : "false") << ",\n";
    json << "    \"reason\": \"" << jsonEscape(report.reason) << "\"\n";
    json << "  },\n";
    json << "  \"duration\": " << std::fixed << std::setprecision(3) << report.duration << ",\n";
    json << "  \"exit_code\": " << report.exit_code << "\n";
    json << "}\n";
    std::cout << json.str();
}

playlist::PlaylistManager buildPlaylistFromInputs(const std::vector<std::string>& media_inputs);

std::vector<subtitle::SubtitleItem> sortSubtitleItems(std::vector<subtitle::SubtitleItem> items) {
    std::sort(items.begin(), items.end(), [](const subtitle::SubtitleItem& lhs, const subtitle::SubtitleItem& rhs) {
        if (lhs.start_seconds == rhs.start_seconds) {
            return lhs.end_seconds < rhs.end_seconds;
        }
        return lhs.start_seconds < rhs.start_seconds;
    });
    return items;
}

int resolveSubtitleIndexByScan(const std::vector<subtitle::SubtitleItem>& items, double position_seconds) {
    int resolved_index = -1;
    for (size_t i = 0; i < items.size(); ++i) {
        const subtitle::SubtitleItem& item = items[i];
        if (position_seconds >= item.start_seconds && position_seconds <= item.end_seconds) {
            resolved_index = static_cast<int>(i);
        }
    }
    return resolved_index;
}

std::vector<double> buildSubtitleValidationTimeline(const std::vector<subtitle::SubtitleItem>& items) {
    std::vector<double> timeline;
    timeline.reserve(items.size() * 7 + 64);
    timeline.push_back(0.0);

    constexpr double kEdgeEpsilon = 0.001;
    for (const auto& item : items) {
        timeline.push_back(std::max(0.0, item.start_seconds - kEdgeEpsilon));
        timeline.push_back(item.start_seconds);
        timeline.push_back(item.start_seconds + kEdgeEpsilon);
        timeline.push_back((item.start_seconds + item.end_seconds) * 0.5);
        if (item.end_seconds > kEdgeEpsilon) {
            timeline.push_back(item.end_seconds - kEdgeEpsilon);
        }
        timeline.push_back(item.end_seconds);
        timeline.push_back(item.end_seconds + kEdgeEpsilon);
    }

    const double max_end_seconds = items.empty() ? 0.0 : std::max(0.0, items.back().end_seconds);
    constexpr double kStepSeconds = 0.25;
    for (double t = 0.0; t <= max_end_seconds + 1.0; t += kStepSeconds) {
        timeline.push_back(t);
    }

    std::sort(timeline.begin(), timeline.end());
    timeline.erase(std::unique(timeline.begin(), timeline.end(), [](double lhs, double rhs) {
        return std::abs(lhs - rhs) < 1e-7;
    }), timeline.end());
    return timeline;
}

std::vector<double> buildSeekValidationTimeline(const std::vector<double>& ordered_timeline) {
    std::vector<double> seek_timeline;
    if (ordered_timeline.empty()) {
        return seek_timeline;
    }

    const size_t total = ordered_timeline.size();
    const size_t max_checks = std::min<size_t>(total, 120);
    seek_timeline.reserve(max_checks + 4);

    size_t index = total / 2;
    const size_t base_step = (total > 1) ? (total / 3 + 1) : 1;
    for (size_t i = 0; i < max_checks; ++i) {
        index = (index + base_step + (i % 7)) % total;
        seek_timeline.push_back(ordered_timeline[index]);
    }

    seek_timeline.push_back(ordered_timeline.front());
    seek_timeline.push_back(ordered_timeline.back());
    seek_timeline.push_back(ordered_timeline[total / 3]);
    seek_timeline.push_back(ordered_timeline[(total * 2) / 3]);
    return seek_timeline;
}

bool runSubtitleSyncCheck(const std::string& subtitle_path) {
    subtitle::SrtParser parser;
    if (!parser.parseFile(subtitle_path)) {
        std::cerr << "subtitle-sync-check: failed to parse subtitle file: " << subtitle_path << std::endl;
        return false;
    }

    std::vector<subtitle::SubtitleItem> items = sortSubtitleItems(parser.items());
    if (items.empty()) {
        std::cerr << "subtitle-sync-check: no subtitle entries found: " << subtitle_path << std::endl;
        return false;
    }

    for (size_t i = 0; i < items.size(); ++i) {
        if (items[i].end_seconds < items[i].start_seconds) {
            std::cerr << "subtitle-sync-check: invalid subtitle range at index " << i
                      << " start=" << items[i].start_seconds
                      << " end=" << items[i].end_seconds << std::endl;
            return false;
        }
    }

    const auto ordered_timeline = buildSubtitleValidationTimeline(items);
    const auto seek_timeline = buildSeekValidationTimeline(ordered_timeline);

    size_t ordered_checks = 0;
    size_t seek_checks = 0;
    size_t mismatch_count = 0;
    int active_index_hint = -1;

    auto validate_at = [&](const char* phase, double position_seconds) {
        const int expected = resolveSubtitleIndexByScan(items, position_seconds);
        const int actual = subtitle::resolveActiveSubtitleIndex(items, position_seconds, active_index_hint);
        if (actual != expected) {
            ++mismatch_count;
            if (mismatch_count <= 8) {
                std::cout << std::fixed << std::setprecision(3)
                          << "subtitle-sync-check mismatch [" << phase << "]"
                          << " t=" << position_seconds
                          << " expected=" << expected
                          << " actual=" << actual
                          << " previous_hint=" << active_index_hint
                          << std::endl;
            }
        }
        active_index_hint = actual;
    };

    for (double position_seconds : ordered_timeline) {
        ++ordered_checks;
        validate_at("ordered", position_seconds);
    }

    for (double position_seconds : seek_timeline) {
        ++seek_checks;
        validate_at("seek", position_seconds);
    }

    std::cout << "subtitle-sync-check.path=" << subtitle_path << std::endl;
    std::cout << "subtitle-sync-check.entries=" << items.size() << std::endl;
    std::cout << "subtitle-sync-check.ordered_checks=" << ordered_checks << std::endl;
    std::cout << "subtitle-sync-check.seek_checks=" << seek_checks << std::endl;
    std::cout << "subtitle-sync-check.mismatches=" << mismatch_count << std::endl;
    std::cout << "subtitle-sync-check.result=" << (mismatch_count == 0 ? "PASS" : "FAIL") << std::endl;
    return mismatch_count == 0;
}

std::string joinIndexList(const std::vector<size_t>& indices) {
    std::ostringstream oss;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << indices[i];
    }
    return oss.str();
}

bool runPlaylistFlowCheck(const std::vector<std::string>& media_inputs, size_t required_items = 5) {
    const playlist::PlaylistManager playlist_manager = buildPlaylistFromInputs(media_inputs);
    const size_t total_entries = playlist_manager.size();
    if (total_entries < required_items) {
        std::cerr << "playlist-flow-check: requires at least " << required_items
                  << " playlist entries, got " << total_entries << std::endl;
        return false;
    }

    const size_t checked_entries = required_items;
    size_t open_passes = 0;
    std::vector<size_t> failed_open_indices;
    failed_open_indices.reserve(checked_entries);
    for (size_t i = 0; i < checked_entries; ++i) {
        const ProbeReport report = collectFileProbeReport(playlist_manager.items()[i].uri);
        const bool open_ok = report.open == "PASS";
        if (open_ok) {
            ++open_passes;
        } else {
            failed_open_indices.push_back(i);
        }
    }

    std::vector<size_t> visited_indices;
    visited_indices.reserve(checked_entries);
    size_t current_index = 0;
    while (current_index < checked_entries) {
        visited_indices.push_back(current_index);
        if (current_index + 1 < checked_entries) {
            ++current_index;
            continue;
        }
        break;
    }

    bool ordered_sequence_ok = visited_indices.size() == checked_entries;
    for (size_t i = 0; ordered_sequence_ok && i < visited_indices.size(); ++i) {
        if (visited_indices[i] != i) {
            ordered_sequence_ok = false;
        }
    }

    const bool open_phase_ok = open_passes == checked_entries;
    const bool result = open_phase_ok && ordered_sequence_ok;

    std::cout << "playlist-flow-check.required_entries=" << required_items << std::endl;
    std::cout << "playlist-flow-check.total_entries=" << total_entries << std::endl;
    std::cout << "playlist-flow-check.checked_entries=" << checked_entries << std::endl;
    std::cout << "playlist-flow-check.open_passes=" << open_passes << std::endl;
    std::cout << "playlist-flow-check.open_failed_indices="
              << (failed_open_indices.empty() ? "-" : joinIndexList(failed_open_indices)) << std::endl;
    std::cout << "playlist-flow-check.sequence=" << joinIndexList(visited_indices) << std::endl;
    std::cout << "playlist-flow-check.sequence_ok=" << (ordered_sequence_ok ? "true" : "false") << std::endl;
    std::cout << "playlist-flow-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

struct AppSettings {
    float volume{1.0f};
    double playback_speed{1.0};
    double audio_delay_seconds{0.0};
    double subtitle_delay_seconds{0.0};
    bool prefer_hardware_decode{true};
    bool resume_last_playlist{true};
    int last_playlist_index{0};
    input::HotkeyManager hotkey_manager{};
};

struct PlaybackCliArgs {
    std::vector<std::string> media_inputs;
    std::string subtitle_file;
};

std::string normalizePathForTitle(const std::string& uri) {
    std::filesystem::path path(uri);
    const std::string title = path.filename().string();
    return title.empty() ? uri : title;
}

bool isM3U8File(const std::string& value) {
    return toLower(extensionFromPath(value)) == "m3u8";
}

bool parsePlaybackCliArgs(int argc, char* argv[], PlaybackCliArgs& out, std::string& error) {
    out = PlaybackCliArgs{};
    error.clear();
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--subtitle") {
            if (!out.subtitle_file.empty()) {
                error = "duplicate --subtitle option";
                return false;
            }
            if (i + 1 >= argc) {
                error = "missing subtitle file path after --subtitle";
                return false;
            }
            out.subtitle_file = argv[++i];
            continue;
        }

        if (!arg.empty() && arg[0] == '-') {
            error = "unknown option in playback mode: " + arg;
            return false;
        }

        out.media_inputs.push_back(arg);
    }

    if (out.media_inputs.empty()) {
        error = "no media input provided";
        return false;
    }
    return true;
}

playlist::PlaylistManager buildPlaylistFromInputs(const std::vector<std::string>& media_inputs) {
    playlist::PlaylistManager manager;

    if (media_inputs.size() == 1) {
        const std::string& single_arg = media_inputs.front();
        if (isM3U8File(single_arg) && manager.loadM3U8(single_arg)) {
            return manager;
        }
    }

    for (const std::string& media_input : media_inputs) {
        playlist::PlaylistItem item{};
        item.uri = media_input;
        item.title = normalizePathForTitle(item.uri);
        manager.addItem(std::move(item));
    }
    return manager;
}

std::string detectAutoSubtitlePath(const std::string& media_uri) {
    if (media_uri.find("://") != std::string::npos) {
        return {};
    }

    std::error_code ec;
    std::filesystem::path candidate(media_uri);
    candidate.replace_extension(".srt");
    if (std::filesystem::exists(candidate, ec) && !ec && std::filesystem::is_regular_file(candidate, ec) && !ec) {
        return candidate.string();
    }
    return {};
}

std::string hotkeySettingKey(input::PlayerAction action) {
    return "hotkey." + input::HotkeyManager::actionConfigKey(action);
}

void loadHotkeySettings(config::SettingsManager& settings_manager, input::HotkeyManager& hotkey_manager) {
    const bool restore_defaults_requested = settings_manager.getBool("hotkey.restore_defaults").value_or(false);
    if (restore_defaults_requested) {
        hotkey_manager.resetToDefaults();
        settings_manager.setBool("hotkey.restore_defaults", false);
        LOG_INFO("Hotkey restore requested, applied default bindings");
        return;
    }

    input::HotkeyManager candidate = hotkey_manager;
    bool invalid_hotkey_found = false;
    for (const input::PlayerAction action : input::HotkeyManager::allActions()) {
        const std::string key = hotkeySettingKey(action);
        const auto token = settings_manager.getString(key);
        if (!token) {
            continue;
        }

        const auto key_code = input::HotkeyManager::keyCodeFromToken(*token);
        if (!key_code) {
            LOG_WARNING("Invalid hotkey token for " << key << ": " << *token << ", keeping default binding");
            invalid_hotkey_found = true;
            continue;
        }
        candidate.bind(action, *key_code);
    }

    const auto conflicts = candidate.findConflicts();
    if (!conflicts.empty()) {
        for (const auto& [first_action, second_action] : conflicts) {
            const auto key_code = candidate.keyForAction(first_action);
            const std::string key_token = key_code ? input::HotkeyManager::keyCodeToToken(*key_code) : "UNKNOWN";
            LOG_WARNING("Hotkey conflict detected on key " << key_token
                        << ": " << input::HotkeyManager::actionConfigKey(first_action)
                        << " <-> " << input::HotkeyManager::actionConfigKey(second_action));
        }
        hotkey_manager.resetToDefaults();
        LOG_WARNING("Hotkey conflicts detected, restored default bindings");
        settings_manager.setBool("hotkey.restore_defaults", false);
        return;
    }

    hotkey_manager = candidate;
    if (invalid_hotkey_found) {
        LOG_WARNING("Some hotkey settings are invalid and were ignored");
    }
}

void syncHotkeySettings(config::SettingsManager& settings_manager, const input::HotkeyManager& hotkey_manager) {
    for (const input::PlayerAction action : input::HotkeyManager::allActions()) {
        const auto key_code = hotkey_manager.keyForAction(action);
        if (!key_code) {
            continue;
        }
        settings_manager.setString(hotkeySettingKey(action), input::HotkeyManager::keyCodeToToken(*key_code));
    }
}

AppSettings loadAppSettings(config::SettingsManager& settings_manager, const std::string& settings_path) {
    AppSettings settings{};

    const bool loaded = settings_manager.loadIni(settings_path);
    if (!loaded) {
        settings_manager.setInt("player.volume_percent", 100);
        settings_manager.setDouble("player.playback_speed", 1.0);
        settings_manager.setInt("player.audio_delay_ms", 0);
        settings_manager.setInt("player.subtitle_delay_ms", 0);
        settings_manager.setBool("decoder.prefer_hardware_decode", true);
        settings_manager.setBool("player.resume_last_playlist", true);
        settings_manager.setInt("player.last_playlist_index", 0);
        settings_manager.setBool("hotkey.restore_defaults", false);
        syncHotkeySettings(settings_manager, settings.hotkey_manager);
        return settings;
    }

    if (const auto volume_percent = settings_manager.getInt("player.volume_percent")) {
        settings.volume = std::max(0.0f, std::min(1.0f, static_cast<float>(*volume_percent) / 100.0f));
    }

    if (const auto speed = settings_manager.getDouble("player.playback_speed")) {
        settings.playback_speed = std::max(0.5, std::min(2.0, *speed));
    }

    if (const auto audio_delay_ms = settings_manager.getInt("player.audio_delay_ms")) {
        settings.audio_delay_seconds = std::max(-5.0, std::min(5.0, static_cast<double>(*audio_delay_ms) / 1000.0));
    }

    if (const auto subtitle_delay_ms = settings_manager.getInt("player.subtitle_delay_ms")) {
        settings.subtitle_delay_seconds = std::max(-5.0, std::min(5.0, static_cast<double>(*subtitle_delay_ms) / 1000.0));
    }

    if (const auto prefer_hardware = settings_manager.getBool("decoder.prefer_hardware_decode")) {
        settings.prefer_hardware_decode = *prefer_hardware;
    }

    if (const auto resume_last = settings_manager.getBool("player.resume_last_playlist")) {
        settings.resume_last_playlist = *resume_last;
    }

    if (const auto index = settings_manager.getInt("player.last_playlist_index")) {
        settings.last_playlist_index = std::max(0, *index);
    }
    loadHotkeySettings(settings_manager, settings.hotkey_manager);

    settings_manager.setInt("player.volume_percent", static_cast<int>(std::lround(settings.volume * 100.0f)));
    settings_manager.setDouble("player.playback_speed", settings.playback_speed);
    settings_manager.setInt("player.audio_delay_ms", static_cast<int>(std::lround(settings.audio_delay_seconds * 1000.0)));
    settings_manager.setInt("player.subtitle_delay_ms", static_cast<int>(std::lround(settings.subtitle_delay_seconds * 1000.0)));
    settings_manager.setBool("decoder.prefer_hardware_decode", settings.prefer_hardware_decode);
    settings_manager.setBool("player.resume_last_playlist", settings.resume_last_playlist);
    settings_manager.setInt("player.last_playlist_index", settings.last_playlist_index);
    settings_manager.setBool("hotkey.restore_defaults", false);
    syncHotkeySettings(settings_manager, settings.hotkey_manager);

    return settings;
}

void saveAppSettings(config::SettingsManager& settings_manager,
                     const std::string& settings_path,
                     float volume,
                     double playback_speed,
                     double audio_delay_seconds,
                     double subtitle_delay_seconds,
                     bool prefer_hardware_decode,
                     bool resume_last_playlist,
                     int last_playlist_index,
                     const input::HotkeyManager& hotkey_manager) {
    const float clamped_volume = std::max(0.0f, std::min(1.0f, volume));
    const double clamped_speed = std::max(0.5, std::min(2.0, playback_speed));
    const double clamped_audio_delay = std::max(-5.0, std::min(5.0, audio_delay_seconds));
    const double clamped_subtitle_delay = std::max(-5.0, std::min(5.0, subtitle_delay_seconds));

    settings_manager.setInt("player.volume_percent", static_cast<int>(std::lround(clamped_volume * 100.0f)));
    settings_manager.setDouble("player.playback_speed", clamped_speed);
    settings_manager.setInt("player.audio_delay_ms", static_cast<int>(std::lround(clamped_audio_delay * 1000.0)));
    settings_manager.setInt("player.subtitle_delay_ms", static_cast<int>(std::lround(clamped_subtitle_delay * 1000.0)));
    settings_manager.setBool("decoder.prefer_hardware_decode", prefer_hardware_decode);
    settings_manager.setBool("player.resume_last_playlist", resume_last_playlist);
    settings_manager.setInt("player.last_playlist_index", std::max(0, last_playlist_index));
    syncHotkeySettings(settings_manager, hotkey_manager);

    if (!settings_manager.saveIni(settings_path)) {
        LOG_WARNING("Failed to save settings file: " << settings_path);
    }
}

bool runSettingsPersistenceCheck(const std::string& settings_path_override) {
    std::error_code ec;
    std::filesystem::path check_path;
    if (!settings_path_override.empty()) {
        check_path = std::filesystem::path(settings_path_override);
    } else {
        check_path = std::filesystem::temp_directory_path(ec) / "modern_video_player_settings_check.ini";
        if (ec || check_path.empty()) {
            check_path = std::filesystem::path("build") / "modern_video_player_settings_check.ini";
        }
    }

    const std::string check_path_text = check_path.string();
    if (check_path.has_parent_path()) {
        std::filesystem::create_directories(check_path.parent_path(), ec);
    }
    std::filesystem::remove(check_path, ec);
    ec.clear();

    constexpr float expected_volume = 0.37f;
    constexpr double expected_speed = 1.25;
    constexpr double expected_audio_delay = 0.2;
    constexpr double expected_subtitle_delay = -0.3;
    constexpr bool expected_prefer_hardware_decode = false;
    constexpr bool expected_resume_last_playlist = false;
    constexpr int expected_playlist_index = 3;
    constexpr int expected_subtitle_key = 'x';

    config::SettingsManager writer_manager;
    AppSettings initial = loadAppSettings(writer_manager, check_path_text);
    input::HotkeyManager expected_hotkeys = initial.hotkey_manager;
    expected_hotkeys.bind(input::PlayerAction::ToggleSubtitle, expected_subtitle_key);
    saveAppSettings(writer_manager,
                    check_path_text,
                    expected_volume,
                    expected_speed,
                    expected_audio_delay,
                    expected_subtitle_delay,
                    expected_prefer_hardware_decode,
                    expected_resume_last_playlist,
                    expected_playlist_index,
                    expected_hotkeys);

    config::SettingsManager reader_manager;
    AppSettings restored = loadAppSettings(reader_manager, check_path_text);

    const bool volume_ok = std::abs(restored.volume - expected_volume) < 1e-6f;
    const bool speed_ok = std::abs(restored.playback_speed - expected_speed) < 1e-9;
    const bool audio_delay_ok = std::abs(restored.audio_delay_seconds - expected_audio_delay) < 1e-9;
    const bool subtitle_delay_ok = std::abs(restored.subtitle_delay_seconds - expected_subtitle_delay) < 1e-9;
    const bool decode_pref_ok = restored.prefer_hardware_decode == expected_prefer_hardware_decode;
    const bool resume_ok = restored.resume_last_playlist == expected_resume_last_playlist;
    const bool index_ok = restored.last_playlist_index == expected_playlist_index;

    const auto subtitle_key = restored.hotkey_manager.keyForAction(input::PlayerAction::ToggleSubtitle);
    const bool hotkey_ok = subtitle_key.has_value() && *subtitle_key == expected_subtitle_key;

    const bool result = volume_ok && speed_ok && audio_delay_ok && subtitle_delay_ok && decode_pref_ok && resume_ok && index_ok && hotkey_ok;

    std::cout << "settings-persistence-check.path=" << check_path_text << std::endl;
    std::cout << "settings-persistence-check.volume_ok=" << (volume_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.speed_ok=" << (speed_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.audio_delay_ok=" << (audio_delay_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.subtitle_delay_ok=" << (subtitle_delay_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.decode_pref_ok=" << (decode_pref_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.resume_ok=" << (resume_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.index_ok=" << (index_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.hotkey_ok=" << (hotkey_ok ? "true" : "false") << std::endl;
    std::cout << "settings-persistence-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    std::filesystem::remove(check_path, ec);
    return result;
}

class ScopedEnvOverride {
public:
    ScopedEnvOverride(const char* key, const std::string& value) : key_(key ? key : "") {
        if (key_.empty()) {
            return;
        }
        const std::string previous = get(key_.c_str());
        if (!previous.empty()) {
            had_previous_ = true;
            previous_value_ = previous;
        }
        set(value);
    }

    ~ScopedEnvOverride() {
        if (key_.empty()) {
            return;
        }
        if (had_previous_) {
            set(previous_value_);
        } else {
            clear();
        }
    }

private:
    std::string get(const char* key) const {
        if (!key || key[0] == '\0') {
            return {};
        }
#if defined(_WIN32)
        char* value = nullptr;
        size_t len = 0;
        if (_dupenv_s(&value, &len, key) != 0 || !value) {
            return {};
        }
        std::string result(value);
        std::free(value);
        return result;
#else
        const char* value = std::getenv(key);
        return value ? std::string(value) : std::string{};
#endif
    }

    void set(const std::string& value) {
#if defined(_WIN32)
        _putenv_s(key_.c_str(), value.c_str());
#else
        setenv(key_.c_str(), value.c_str(), 1);
#endif
    }

    void clear() {
#if defined(_WIN32)
        _putenv_s(key_.c_str(), "");
#else
        unsetenv(key_.c_str());
#endif
    }

    std::string key_;
    std::string previous_value_;
    bool had_previous_{false};
};

bool runRendererFallbackCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "renderer-fallback-check.path=" << media_file << std::endl;
        std::cout << "renderer-fallback-check.open_ok=false" << std::endl;
        std::cout << "renderer-fallback-check.renderer_backend=None" << std::endl;
        std::cout << "renderer-fallback-check.decoder_backend=Unknown" << std::endl;
        std::cout << "renderer-fallback-check.entered_playback_loop=false" << std::endl;
        std::cout << "renderer-fallback-check.fallback_to_sdl=false" << std::endl;
        std::cout << "renderer-fallback-check.result=FAIL" << std::endl;
        return false;
    }

    ScopedEnvOverride force_non_d3d11_driver("MVP_D3D11_DRIVER_HINT", "software");

    auto run_session = [&](bool prefer_hardware_decode) {
        struct SessionResult {
            bool open_ok{false};
            bool entered_playback_loop{false};
            std::string renderer_backend{"None"};
            std::string decoder_backend{"Unknown"};
        } result;

        VideoPlayer player;
        player.setPreferHardwareDecode(prefer_hardware_decode);
        result.open_ok = player.open(media_file);
        result.renderer_backend = player.videoRendererBackendName();
        result.decoder_backend = player.videoDecoderBackendName();

        if (result.open_ok) {
            player.play();
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
            while (std::chrono::steady_clock::now() < deadline &&
                   (player.isPlaying() || player.isPaused())) {
                result.entered_playback_loop = true;
                player.pumpEvents();
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            player.stop();
            player.close();
        }
        return result;
    };

    const auto session = run_session(true);
    const bool fallback_to_sdl = session.renderer_backend == "SoftwareSDL";
    const bool result = session.open_ok && fallback_to_sdl;

    std::cout << "renderer-fallback-check.path=" << media_file << std::endl;
    std::cout << "renderer-fallback-check.open_ok=" << (session.open_ok ? "true" : "false") << std::endl;
    std::cout << "renderer-fallback-check.renderer_backend=" << session.renderer_backend << std::endl;
    std::cout << "renderer-fallback-check.decoder_backend=" << session.decoder_backend << std::endl;
    std::cout << "renderer-fallback-check.entered_playback_loop="
              << (session.entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "renderer-fallback-check.fallback_to_sdl=" << (fallback_to_sdl ? "true" : "false") << std::endl;
    std::cout << "renderer-fallback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

struct BackendSessionResult {
    bool open_ok{false};
    bool entered_playback_loop{false};
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    bool mode_ok{false};
    int exit_code{3};
    bool parsed{false};
};

std::string trimWhitespace(std::string value) {
    auto is_space = [](unsigned char ch) {
        return std::isspace(ch) != 0;
    };

    while (!value.empty() && is_space(static_cast<unsigned char>(value.front()))) {
        value.erase(value.begin());
    }
    while (!value.empty() && is_space(static_cast<unsigned char>(value.back()))) {
        value.pop_back();
    }
    return value;
}

std::string stripWrappingQuotes(std::string value) {
    value = trimWhitespace(std::move(value));
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
        value = value.substr(1, value.size() - 2);
    }
    return value;
}

bool parseBoolText(const std::string& text) {
    const std::string lowered = toLower(trimWhitespace(text));
    return lowered == "1" || lowered == "true" || lowered == "yes" || lowered == "on";
}

std::string quoteCommandArg(const std::string& arg) {
    std::string quoted;
    quoted.reserve(arg.size() + 2);
    quoted.push_back('"');
    for (char ch : arg) {
        if (ch == '"') {
            quoted += "\"\"";
        } else {
            quoted.push_back(ch);
        }
    }
    quoted.push_back('"');
    return quoted;
}

BackendSessionResult parseBackendSessionOutput(const std::filesystem::path& output_path,
                                               const std::string& mode) {
    BackendSessionResult result;

    std::ifstream input(output_path);
    if (!input.is_open()) {
        return result;
    }

    const std::string prefix = "windows-backend-session-check." + mode + ".";
    std::string line;
    while (std::getline(input, line)) {
        line = trimWhitespace(line);
        if (line.rfind(prefix, 0) != 0) {
            continue;
        }

        const size_t split_pos = line.find('=');
        if (split_pos == std::string::npos || split_pos <= prefix.size()) {
            continue;
        }
        result.parsed = true;
        const std::string key = line.substr(prefix.size(), split_pos - prefix.size());
        const std::string value = line.substr(split_pos + 1);

        if (key == "open_ok") {
            result.open_ok = parseBoolText(value);
            continue;
        }
        if (key == "entered_playback_loop") {
            result.entered_playback_loop = parseBoolText(value);
            continue;
        }
        if (key == "renderer_backend") {
            result.renderer_backend = trimWhitespace(value);
            continue;
        }
        if (key == "decoder_backend") {
            result.decoder_backend = trimWhitespace(value);
            continue;
        }
        if (key == "mode_ok") {
            result.mode_ok = parseBoolText(value);
            continue;
        }
    }

    return result;
}

BackendSessionResult runBackendSessionSubprocess(const std::string& program_path,
                                                 const std::string& media_file,
                                                 const std::string& mode) {
    BackendSessionResult result;
    const std::string clean_program_path = stripWrappingQuotes(program_path);
    const std::string clean_media_file = stripWrappingQuotes(media_file);
    if (clean_program_path.empty() || clean_media_file.empty()) {
        return result;
    }
    const auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch())
                            .count();

    std::error_code ec;
    const std::filesystem::path output_path = std::filesystem::temp_directory_path(ec) /
        ("mvp_windows_backend_" + mode + "_" + std::to_string(now_ms) + ".log");
    if (ec) {
        return result;
    }

    const std::string command =
        quoteCommandArg(clean_program_path) + " --windows-backend-session-check " +
        quoteCommandArg(clean_media_file) + " " + mode +
        " > " + quoteCommandArg(output_path.string()) + " 2>&1";
#if defined(_WIN32)
    (void)command;
    SECURITY_ATTRIBUTES sa{};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE output_handle = CreateFileA(output_path.string().c_str(),
                                       GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE,
                                       &sa,
                                       CREATE_ALWAYS,
                                       FILE_ATTRIBUTE_NORMAL,
                                       nullptr);
    if (output_handle == INVALID_HANDLE_VALUE) {
        return result;
    }

    STARTUPINFOA si{};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    si.hStdOutput = output_handle;
    si.hStdError = output_handle;

    PROCESS_INFORMATION pi{};
    std::string child_command = quoteCommandArg(clean_program_path) +
                                " --windows-backend-session-check " +
                                quoteCommandArg(clean_media_file) +
                                " " + mode;
    std::vector<char> mutable_command(child_command.begin(), child_command.end());
    mutable_command.push_back('\0');

    const BOOL created = CreateProcessA(nullptr,
                                        mutable_command.data(),
                                        nullptr,
                                        nullptr,
                                        TRUE,
                                        CREATE_NO_WINDOW,
                                        nullptr,
                                        nullptr,
                                        &si,
                                        &pi);
    CloseHandle(output_handle);
    if (!created) {
        return result;
    }

    const DWORD wait_result = WaitForSingleObject(pi.hProcess, 120000);
    if (wait_result == WAIT_TIMEOUT) {
        TerminateProcess(pi.hProcess, 124);
        result.exit_code = 124;
    } else {
        DWORD child_exit_code = 3;
        if (GetExitCodeProcess(pi.hProcess, &child_exit_code)) {
            result.exit_code = static_cast<int>(child_exit_code);
        }
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
#else
    result.exit_code = std::system(command.c_str());
#endif
    BackendSessionResult parsed = parseBackendSessionOutput(output_path, mode);
    if (parsed.parsed) {
        parsed.exit_code = result.exit_code;
        result = parsed;
    } else {
        result.mode_ok = (result.exit_code == 0);
    }

    std::filesystem::remove(output_path, ec);
    return result;
}

bool runWindowsBackendSessionCheck(const std::string& media_file, const std::string& mode) {
    const bool hard_mode = mode == "hard";
    const bool soft_mode = mode == "soft";
    if (!hard_mode && !soft_mode) {
        std::cout << "windows-backend-session-check.mode=" << mode << std::endl;
        std::cout << "windows-backend-session-check.result=FAIL" << std::endl;
        return false;
    }

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "windows-backend-session-check.path=" << media_file << std::endl;
        std::cout << "windows-backend-session-check.mode=" << mode << std::endl;
        std::cout << "windows-backend-session-check.result=FAIL" << std::endl;
        return false;
    }

    const bool prefer_hardware_decode = hard_mode;
    const std::string expected_decoder = prefer_hardware_decode ? "D3D11VA" : "Software";
    ScopedEnvOverride force_d3d11_driver_hint("MVP_D3D11_DRIVER_HINT", "direct3d11");

    // Keep this session self-contained and avoid stop/close teardown deadlocks by
    // leaving cleanup to process exit in the parent command path.
    auto* player = new VideoPlayer();
    BackendSessionResult session{};
    player->setPreferHardwareDecode(prefer_hardware_decode);
    session.open_ok = player->open(media_file);
    session.renderer_backend = player->videoRendererBackendName();
    session.decoder_backend = player->videoDecoderBackendName();

    if (session.open_ok) {
        player->play();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
        while (std::chrono::steady_clock::now() < deadline &&
               (player->isPlaying() || player->isPaused())) {
            session.entered_playback_loop = true;
            player->pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    session.mode_ok = session.open_ok && session.entered_playback_loop &&
                      session.decoder_backend == expected_decoder;

    std::cout << "windows-backend-session-check.path=" << media_file << std::endl;
    std::cout << "windows-backend-session-check.mode=" << mode << std::endl;
    std::cout << "windows-backend-session-check." << mode
              << ".open_ok=" << (session.open_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-session-check." << mode
              << ".entered_playback_loop=" << (session.entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "windows-backend-session-check." << mode
              << ".renderer_backend=" << session.renderer_backend << std::endl;
    std::cout << "windows-backend-session-check." << mode
              << ".decoder_backend=" << session.decoder_backend << std::endl;
    std::cout << "windows-backend-session-check." << mode
              << ".mode_ok=" << (session.mode_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-session-check.result=" << (session.mode_ok ? "PASS" : "FAIL") << std::endl;

    return session.mode_ok;
}

bool runWindowsBackendPlaybackCheck(const std::string& program_path, const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "windows-backend-check.path=" << media_file << std::endl;
        std::cout << "windows-backend-check.result=FAIL" << std::endl;
        return false;
    }

    const BackendSessionResult hard_session = runBackendSessionSubprocess(program_path, media_file, "hard");
    const BackendSessionResult soft_session = runBackendSessionSubprocess(program_path, media_file, "soft");
    const bool result = hard_session.mode_ok && soft_session.mode_ok &&
                        hard_session.exit_code == 0 && soft_session.exit_code == 0;

    std::cout << "windows-backend-check.path=" << media_file << std::endl;
    std::cout << "windows-backend-check.hard.open_ok=" << (hard_session.open_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.hard.entered_playback_loop="
              << (hard_session.entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.hard.renderer_backend=" << hard_session.renderer_backend << std::endl;
    std::cout << "windows-backend-check.hard.decoder_backend=" << hard_session.decoder_backend << std::endl;
    std::cout << "windows-backend-check.hard.mode_ok=" << (hard_session.mode_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.hard.exit_code=" << hard_session.exit_code << std::endl;

    std::cout << "windows-backend-check.soft.open_ok=" << (soft_session.open_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.soft.entered_playback_loop="
              << (soft_session.entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.soft.renderer_backend=" << soft_session.renderer_backend << std::endl;
    std::cout << "windows-backend-check.soft.decoder_backend=" << soft_session.decoder_backend << std::endl;
    std::cout << "windows-backend-check.soft.mode_ok=" << (soft_session.mode_ok ? "true" : "false") << std::endl;
    std::cout << "windows-backend-check.soft.exit_code=" << soft_session.exit_code << std::endl;

    std::cout << "windows-backend-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runChapterNavigationCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "chapter-nav-check.path=" << media_file << std::endl;
        std::cout << "chapter-nav-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    const size_t chapter_count = player.chapterCount();

    bool entered_playback_loop = false;
    bool next_invoked = false;
    bool prev_invoked = false;
    double before = 0.0;
    double after_next = 0.0;
    double after_prev = 0.0;

    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(500));
        before = player.getCurrentTime();
        next_invoked = player.seekToNextChapter();
        pump_for(player, std::chrono::milliseconds(500));
        after_next = player.getCurrentTime();
        prev_invoked = player.seekToPreviousChapter();
        pump_for(player, std::chrono::milliseconds(500));
        after_prev = player.getCurrentTime();
        player.stop();
        player.close();
    }

    const bool moved_forward = after_next > before + 0.5;
    const bool moved_backward = after_prev + 0.5 < after_next;
    const bool result = open_ok &&
                        chapter_count > 0 &&
                        entered_playback_loop &&
                        next_invoked &&
                        prev_invoked &&
                        moved_forward &&
                        moved_backward;

    std::cout << "chapter-nav-check.path=" << media_file << std::endl;
    std::cout << "chapter-nav-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.chapter_count=" << chapter_count << std::endl;
    std::cout << "chapter-nav-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.next_invoked=" << (next_invoked ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.prev_invoked=" << (prev_invoked ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.before=" << before << std::endl;
    std::cout << "chapter-nav-check.after_next=" << after_next << std::endl;
    std::cout << "chapter-nav-check.after_prev=" << after_prev << std::endl;
    std::cout << "chapter-nav-check.moved_forward=" << (moved_forward ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.moved_backward=" << (moved_backward ? "true" : "false") << std::endl;
    std::cout << "chapter-nav-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runABRepeatCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "ab-repeat-check.path=" << media_file << std::endl;
        std::cout << "ab-repeat-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    const double duration = player.getDuration();

    bool entered_playback_loop = false;
    bool a_set = false;
    bool b_set = false;
    bool enabled_after_set = false;
    bool loop_observed = false;
    bool cleared = false;
    double point_a = -1.0;
    double point_b = -1.0;
    double loop_before = -1.0;
    double loop_after = -1.0;

    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(600));

        a_set = player.setABRepeatStart();
        point_a = player.abRepeatStart();

        if (a_set) {
            double b_target = point_a + 2.0;
            if (duration > 0.0) {
                const double max_target = std::max(point_a + 0.3, duration - 0.3);
                b_target = std::min(b_target, max_target);
            }
            player.seek(b_target);
            pump_for(player, std::chrono::milliseconds(600));
            b_set = player.setABRepeatEnd();
            point_b = player.abRepeatEnd();
            enabled_after_set = player.isABRepeatEnabled();
        }

        if (enabled_after_set) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(6);
            double previous = player.getCurrentTime();
            while (std::chrono::steady_clock::now() < deadline &&
                   (player.isPlaying() || player.isPaused())) {
                player.pumpEvents();
                const double current = player.getCurrentTime();
                if (previous >= point_b - 0.15 && current <= point_a + 0.5) {
                    loop_observed = true;
                    loop_before = previous;
                    loop_after = current;
                    break;
                }
                previous = current;
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        player.clearABRepeat();
        cleared = !player.isABRepeatEnabled();
        player.stop();
        player.close();
    }

    const bool result = open_ok &&
                        entered_playback_loop &&
                        a_set &&
                        b_set &&
                        enabled_after_set &&
                        loop_observed &&
                        cleared;

    std::cout << "ab-repeat-check.path=" << media_file << std::endl;
    std::cout << "ab-repeat-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.a_set=" << (a_set ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.b_set=" << (b_set ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.enabled_after_set=" << (enabled_after_set ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.cleared=" << (cleared ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.point_a=" << point_a << std::endl;
    std::cout << "ab-repeat-check.point_b=" << point_b << std::endl;
    std::cout << "ab-repeat-check.loop_before=" << loop_before << std::endl;
    std::cout << "ab-repeat-check.loop_after=" << loop_after << std::endl;
    std::cout << "ab-repeat-check.loop_observed=" << (loop_observed ? "true" : "false") << std::endl;
    std::cout << "ab-repeat-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runFrameStepCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "frame-step-check.path=" << media_file << std::endl;
        std::cout << "frame-step-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    bool paused_before_step = false;
    bool forward_invoked = false;
    bool backward_invoked = false;
    bool paused_after_forward = false;
    bool paused_after_backward = false;
    double before = 0.0;
    double after_forward = 0.0;
    double after_backward = 0.0;

    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(800));
        if (entered_playback_loop) {
            player.pause();
            paused_before_step = player.isPaused();
            pump_for(player, std::chrono::milliseconds(120));
            before = player.getCurrentTime();
            forward_invoked = player.stepFrameForward();
            paused_after_forward = player.isPaused();
            pump_for(player, std::chrono::milliseconds(120));
            after_forward = player.getCurrentTime();
            backward_invoked = player.stepFrameBackward();
            paused_after_backward = player.isPaused();
            pump_for(player, std::chrono::milliseconds(120));
            after_backward = player.getCurrentTime();
        }
        player.stop();
        player.close();
    }

    const bool moved_forward = after_forward > before + 0.005;
    const bool moved_backward = after_backward + 0.005 < after_forward;
    const bool result = open_ok &&
                        entered_playback_loop &&
                        paused_before_step &&
                        forward_invoked &&
                        backward_invoked &&
                        paused_after_forward &&
                        paused_after_backward &&
                        moved_forward &&
                        moved_backward;

    std::cout << "frame-step-check.path=" << media_file << std::endl;
    std::cout << "frame-step-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.paused_before_step=" << (paused_before_step ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.forward_invoked=" << (forward_invoked ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.backward_invoked=" << (backward_invoked ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.paused_after_forward=" << (paused_after_forward ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.paused_after_backward=" << (paused_after_backward ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.before=" << before << std::endl;
    std::cout << "frame-step-check.after_forward=" << after_forward << std::endl;
    std::cout << "frame-step-check.after_backward=" << after_backward << std::endl;
    std::cout << "frame-step-check.moved_forward=" << (moved_forward ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.moved_backward=" << (moved_backward ? "true" : "false") << std::endl;
    std::cout << "frame-step-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runDelayAdjustCheck(const std::string& media_file, const std::string& subtitle_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    const std::filesystem::path subtitle_path(subtitle_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        !std::filesystem::exists(subtitle_path, ec) || ec ||
        !std::filesystem::is_regular_file(subtitle_path, ec) || ec) {
        std::cout << "delay-adjust-check.path=" << media_file << std::endl;
        std::cout << "delay-adjust-check.subtitle=" << subtitle_file << std::endl;
        std::cout << "delay-adjust-check.result=FAIL" << std::endl;
        return false;
    }

    constexpr double kDelayCheckStepSeconds = 0.1;
    constexpr double kProbeOffsetSeconds = 0.05;
    subtitle::SrtParser parser;
    const bool subtitle_parsed = parser.parseFile(subtitle_file);
    const auto& items = parser.items();
    bool probe_found = false;
    bool baseline_before_clear = false;
    bool baseline_inside_visible = false;
    bool subtitle_early_ok = false;
    bool subtitle_late_ok = false;
    size_t probe_index = 0;
    double probe_before = 0.0;
    double probe_inside = 0.0;

    if (subtitle_parsed) {
        for (size_t i = 0; i < items.size(); ++i) {
            const auto& item = items[i];
            const double gap_before = (i == 0) ? item.start_seconds : (item.start_seconds - items[i - 1].end_seconds);
            const double duration = item.end_seconds - item.start_seconds;
            if (item.start_seconds < kProbeOffsetSeconds || gap_before < kProbeOffsetSeconds || duration < kProbeOffsetSeconds) {
                continue;
            }

            probe_found = true;
            probe_index = i;
            probe_before = item.start_seconds - kProbeOffsetSeconds;
            probe_inside = item.start_seconds + kProbeOffsetSeconds;

            const int baseline_before_index = subtitle::resolveActiveSubtitleIndex(items, probe_before, -1);
            const int baseline_inside_index = subtitle::resolveActiveSubtitleIndex(items, probe_inside, -1);
            const int early_index = subtitle::resolveActiveSubtitleIndex(items, probe_before + kDelayCheckStepSeconds, -1);
            const int late_index = subtitle::resolveActiveSubtitleIndex(items, probe_inside - kDelayCheckStepSeconds, -1);

            baseline_before_clear = baseline_before_index != static_cast<int>(i);
            baseline_inside_visible = baseline_inside_index == static_cast<int>(i);
            subtitle_early_ok = early_index == static_cast<int>(i);
            subtitle_late_ok = late_index != static_cast<int>(i);
            break;
        }
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool subtitle_loaded = false;
    bool entered_playback_loop = false;
    bool paused_before_adjust = false;
    bool audio_roundtrip_ok = false;
    bool subtitle_roundtrip_ok = false;
    double final_audio_delay = 0.0;
    double final_subtitle_delay = 0.0;

    if (open_ok) {
        subtitle_loaded = player.loadExternalSubtitle(subtitle_file);
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(800));
        if (entered_playback_loop) {
            player.pause();
            paused_before_adjust = player.isPaused();
            pump_for(player, std::chrono::milliseconds(120));

            player.setAudioDelay(kDelayCheckStepSeconds);
            player.setSubtitleDelay(-kDelayCheckStepSeconds);
            audio_roundtrip_ok = std::abs(player.getAudioDelay() - kDelayCheckStepSeconds) < 1e-9;
            subtitle_roundtrip_ok = std::abs(player.getSubtitleDelay() + kDelayCheckStepSeconds) < 1e-9;

            player.play();
            pump_for(player, std::chrono::milliseconds(250));
            audio_roundtrip_ok = audio_roundtrip_ok && std::abs(player.getAudioDelay() - kDelayCheckStepSeconds) < 1e-9;
            subtitle_roundtrip_ok = subtitle_roundtrip_ok && std::abs(player.getSubtitleDelay() + kDelayCheckStepSeconds) < 1e-9;
            final_audio_delay = player.getAudioDelay();
            final_subtitle_delay = player.getSubtitleDelay();
        }
        player.stop();
        player.close();
    }

    const bool subtitle_math_ok = probe_found && baseline_before_clear && baseline_inside_visible && subtitle_early_ok && subtitle_late_ok;
    const bool result = open_ok &&
                        subtitle_loaded &&
                        entered_playback_loop &&
                        paused_before_adjust &&
                        audio_roundtrip_ok &&
                        subtitle_roundtrip_ok &&
                        subtitle_math_ok;

    std::cout << "delay-adjust-check.path=" << media_file << std::endl;
    std::cout << "delay-adjust-check.subtitle=" << subtitle_file << std::endl;
    std::cout << "delay-adjust-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.subtitle_loaded=" << (subtitle_loaded ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.subtitle_entries=" << items.size() << std::endl;
    std::cout << "delay-adjust-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.paused_before_adjust=" << (paused_before_adjust ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.probe_found=" << (probe_found ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.probe_index=" << probe_index << std::endl;
    std::cout << "delay-adjust-check.probe_before=" << probe_before << std::endl;
    std::cout << "delay-adjust-check.probe_inside=" << probe_inside << std::endl;
    std::cout << "delay-adjust-check.baseline_before_clear=" << (baseline_before_clear ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.baseline_inside_visible=" << (baseline_inside_visible ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.subtitle_early_ok=" << (subtitle_early_ok ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.subtitle_late_ok=" << (subtitle_late_ok ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.audio_roundtrip_ok=" << (audio_roundtrip_ok ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.subtitle_roundtrip_ok=" << (subtitle_roundtrip_ok ? "true" : "false") << std::endl;
    std::cout << "delay-adjust-check.final_audio_delay_ms=" << std::lround(final_audio_delay * 1000.0) << std::endl;
    std::cout << "delay-adjust-check.final_subtitle_delay_ms=" << std::lround(final_subtitle_delay * 1000.0) << std::endl;
    std::cout << "delay-adjust-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runNumericSeekCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "numeric-seek-check.path=" << media_file << std::endl;
        std::cout << "numeric-seek-check.result=FAIL" << std::endl;
        return false;
    }

    input::HotkeyManager hotkeys;
    auto binding_is = [&hotkeys](input::PlayerAction action, int expected_key) {
        const auto key = hotkeys.keyForAction(action);
        return key.has_value() && *key == expected_key;
    };

    const bool bindings_ok =
        binding_is(input::PlayerAction::SeekTo10Percent, '1') &&
        binding_is(input::PlayerAction::SeekTo20Percent, '2') &&
        binding_is(input::PlayerAction::SeekTo30Percent, '3') &&
        binding_is(input::PlayerAction::SeekTo40Percent, '4') &&
        binding_is(input::PlayerAction::SeekTo50Percent, '5') &&
        binding_is(input::PlayerAction::SeekTo60Percent, '6') &&
        binding_is(input::PlayerAction::SeekTo70Percent, '7') &&
        binding_is(input::PlayerAction::SeekTo80Percent, '8') &&
        binding_is(input::PlayerAction::SeekTo90Percent, '9');

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    bool paused_before_seek = false;
    double duration = 0.0;
    double target_10 = 0.0;
    double target_90 = 0.0;
    double after_10 = 0.0;
    double after_90 = 0.0;
    bool seek_10_ok = false;
    bool seek_90_ok = false;

    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(800));
        duration = player.getDuration();
        if (entered_playback_loop && duration > 1.0) {
            player.pause();
            paused_before_seek = player.isPaused();
            pump_for(player, std::chrono::milliseconds(120));

            target_10 = duration * 0.1;
            target_90 = duration * 0.9;
            const double tolerance = 0.75;

            player.seek(target_10);
            pump_for(player, std::chrono::milliseconds(180));
            after_10 = player.getCurrentTime();
            seek_10_ok = std::abs(after_10 - target_10) <= tolerance;

            player.seek(target_90);
            pump_for(player, std::chrono::milliseconds(180));
            after_90 = player.getCurrentTime();
            seek_90_ok = std::abs(after_90 - target_90) <= tolerance;
        }
        player.stop();
        player.close();
    }

    const bool result = open_ok && bindings_ok && entered_playback_loop && paused_before_seek && seek_10_ok && seek_90_ok;

    std::cout << "numeric-seek-check.path=" << media_file << std::endl;
    std::cout << "numeric-seek-check.bindings_ok=" << (bindings_ok ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.paused_before_seek=" << (paused_before_seek ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.duration=" << duration << std::endl;
    std::cout << "numeric-seek-check.target_10=" << target_10 << std::endl;
    std::cout << "numeric-seek-check.after_10=" << after_10 << std::endl;
    std::cout << "numeric-seek-check.target_90=" << target_90 << std::endl;
    std::cout << "numeric-seek-check.after_90=" << after_90 << std::endl;
    std::cout << "numeric-seek-check.seek_10_ok=" << (seek_10_ok ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.seek_90_ok=" << (seek_90_ok ? "true" : "false") << std::endl;
    std::cout << "numeric-seek-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runPerformanceLogCheck(const std::string& media_file, int sample_ms = 1500) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 500) {
        std::cout << "performance-log-check.path=" << media_file << std::endl;
        std::cout << "performance-log-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "performance-log-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    double cpu_avg_percent = 0.0;
    unsigned int logical_cores = std::max(1u, std::thread::hardware_concurrency());
    core::PlaybackInfo info{};
    core::DiagnosticsSnapshot diag{};
    std::string renderer_backend = "None";
    std::string decoder_backend = "Unknown";

    if (open_ok) {
        player.play();
        const auto wall_start = std::chrono::steady_clock::now();
        const std::clock_t cpu_start = std::clock();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        const auto wall_end = std::chrono::steady_clock::now();
        const std::clock_t cpu_end = std::clock();

        const double wall_seconds = std::max(0.001,
            std::chrono::duration_cast<std::chrono::duration<double>>(wall_end - wall_start).count());
        const double cpu_seconds = static_cast<double>(cpu_end - cpu_start) / static_cast<double>(CLOCKS_PER_SEC);
        cpu_avg_percent = (cpu_seconds / wall_seconds) * 100.0;

        info = player.getInfo();
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const bool result = open_ok && entered_playback_loop && diag.render_frames > 0;

    std::cout << "performance-log-check.path=" << media_file << std::endl;
    std::cout << "performance-log-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "performance-log-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.video_width=" << info.video_width << std::endl;
    std::cout << "performance-log-check.video_height=" << info.video_height << std::endl;
    std::cout << "performance-log-check.audio_channels=" << info.audio_channels << std::endl;
    std::cout << "performance-log-check.audio_sample_rate=" << info.audio_sample_rate << std::endl;
    std::cout << "performance-log-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "performance-log-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "performance-log-check.cpu_avg_percent=" << cpu_avg_percent << std::endl;
    std::cout << "performance-log-check.cpu_logical_cores=" << logical_cores << std::endl;
    std::cout << "performance-log-check.demux_video_packets=" << diag.demux_video_packets << std::endl;
    std::cout << "performance-log-check.demux_audio_packets=" << diag.demux_audio_packets << std::endl;
    std::cout << "performance-log-check.demux_push_retries=" << diag.demux_push_retries << std::endl;
    std::cout << "performance-log-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;
    std::cout << "performance-log-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "performance-log-check.decode_audio_ok=" << diag.decode_audio_ok << std::endl;
    std::cout << "performance-log-check.audio_submitted_frames=" << diag.audio_submitted_frames << std::endl;
    std::cout << "performance-log-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "performance-log-check.scheduler_video_decoded_frames=" << diag.scheduler_video_decoded_frames << std::endl;
    std::cout << "performance-log-check.scheduler_audio_decoded_frames=" << diag.scheduler_audio_decoded_frames << std::endl;
    std::cout << "performance-log-check.scheduler_late_drops=" << diag.scheduler_late_drops << std::endl;
    std::cout << "performance-log-check.video_packet_queue_size=" << diag.video_packet_queue_size << std::endl;
    std::cout << "performance-log-check.audio_packet_queue_size=" << diag.audio_packet_queue_size << std::endl;
    std::cout << "performance-log-check.video_frame_queue_size=" << diag.video_frame_queue_size << std::endl;
    std::cout << "performance-log-check.audio_frame_queue_size=" << diag.audio_frame_queue_size << std::endl;
    std::cout << "performance-log-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool run1080p60Check(const std::string& media_file, int sample_ms = 5000) {
    const ProbeReport probe = collectFileProbeReport(media_file);
    if (sample_ms < 2000) {
        std::cout << "1080p60-check.path=" << media_file << std::endl;
        std::cout << "1080p60-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "1080p60-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    const double sample_seconds = static_cast<double>(sample_ms) / 1000.0;
    const bool probe_ok = probe.overall == "PASS" && probe.width >= 1920 && probe.height >= 1080 && probe.fps >= 59.0;
    const bool duration_ok = probe.duration <= 0.0 || probe.duration >= sample_seconds + 1.0;

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    bool still_playing_after_window = false;
    double start_position = 0.0;
    double end_position = 0.0;
    double advanced_seconds = 0.0;
    double advance_ratio = 0.0;
    core::DiagnosticsSnapshot diag{};
    std::string renderer_backend = "None";
    std::string decoder_backend = "Unknown";

    if (open_ok) {
        player.play();
        start_position = player.getCurrentTime();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        end_position = player.getCurrentTime();
        advanced_seconds = std::max(0.0, end_position - start_position);
        advance_ratio = sample_seconds > 0.0 ? advanced_seconds / sample_seconds : 0.0;
        still_playing_after_window = player.isPlaying();
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const double min_advance_seconds = std::max(1.0, sample_seconds * 0.8);
    const bool progress_ok = advanced_seconds >= min_advance_seconds;
    const bool late_drops_ok = diag.scheduler_late_drops == 0;
    const bool demux_drops_ok = diag.demux_dropped_packets == 0;
    const bool result = probe_ok && duration_ok && open_ok && entered_playback_loop && still_playing_after_window &&
                        progress_ok && late_drops_ok && demux_drops_ok;

    std::cout << "1080p60-check.path=" << media_file << std::endl;
    std::cout << "1080p60-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "1080p60-check.probe_overall=" << probe.overall << std::endl;
    std::cout << "1080p60-check.probe_width=" << probe.width << std::endl;
    std::cout << "1080p60-check.probe_height=" << probe.height << std::endl;
    std::cout << "1080p60-check.probe_fps=" << probe.fps << std::endl;
    std::cout << "1080p60-check.probe_duration=" << probe.duration << std::endl;
    std::cout << "1080p60-check.probe_recommend_hw=" << (probe.recommend_hw ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.probe_recommend_d3d11=" << (probe.recommend_d3d11 ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.still_playing_after_window=" << (still_playing_after_window ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "1080p60-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "1080p60-check.start_position=" << start_position << std::endl;
    std::cout << "1080p60-check.end_position=" << end_position << std::endl;
    std::cout << "1080p60-check.advanced_seconds=" << advanced_seconds << std::endl;
    std::cout << "1080p60-check.advance_ratio=" << advance_ratio << std::endl;
    std::cout << "1080p60-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.late_drops=" << diag.scheduler_late_drops << std::endl;
    std::cout << "1080p60-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;
    std::cout << "1080p60-check.demux_drops_ok=" << (demux_drops_ok ? "true" : "false") << std::endl;
    std::cout << "1080p60-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "1080p60-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "1080p60-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool run4kPlaybackCheck(const std::string& program_path, const std::string& media_file, int sample_ms = 2000) {
    const ProbeReport probe = collectFileProbeReport(media_file);
    if (sample_ms < 1000) {
        std::cout << "4k-playback-check.path=" << media_file << std::endl;
        std::cout << "4k-playback-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "4k-playback-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    const double sample_seconds = static_cast<double>(sample_ms) / 1000.0;
    const bool probe_ok = probe.overall == "PASS" && probe.width >= 3840 && probe.height >= 2160;
    const bool duration_ok = probe.duration <= 0.0 || probe.duration >= sample_seconds + 1.0;

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    bool still_playing_after_window = false;
    double start_position = 0.0;
    double end_position = 0.0;
    double advanced_seconds = 0.0;
    double advance_ratio = 0.0;
    core::DiagnosticsSnapshot diag{};
    std::string renderer_backend = "None";
    std::string decoder_backend = "Unknown";

    if (open_ok) {
        player.play();
        start_position = player.getCurrentTime();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        end_position = player.getCurrentTime();
        advanced_seconds = std::max(0.0, end_position - start_position);
        advance_ratio = sample_seconds > 0.0 ? advanced_seconds / sample_seconds : 0.0;
        still_playing_after_window = player.isPlaying();
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const double min_advance_seconds = std::max(0.8, sample_seconds * 0.75);
    const bool progress_ok = advanced_seconds >= min_advance_seconds;
    const bool late_drops_ok = diag.scheduler_late_drops == 0;

    const BackendSessionResult hard_session = runBackendSessionSubprocess(program_path, media_file, "hard");
    const BackendSessionResult soft_session = runBackendSessionSubprocess(program_path, media_file, "soft");
    const bool fallback_ok = hard_session.mode_ok && soft_session.mode_ok &&
                             hard_session.exit_code == 0 && soft_session.exit_code == 0;

    const bool result = probe_ok && duration_ok && open_ok && entered_playback_loop && still_playing_after_window &&
                        progress_ok && late_drops_ok && fallback_ok;

    std::cout << "4k-playback-check.path=" << media_file << std::endl;
    std::cout << "4k-playback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "4k-playback-check.probe_overall=" << probe.overall << std::endl;
    std::cout << "4k-playback-check.probe_width=" << probe.width << std::endl;
    std::cout << "4k-playback-check.probe_height=" << probe.height << std::endl;
    std::cout << "4k-playback-check.probe_fps=" << probe.fps << std::endl;
    std::cout << "4k-playback-check.probe_duration=" << probe.duration << std::endl;
    std::cout << "4k-playback-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.still_playing_after_window=" << (still_playing_after_window ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "4k-playback-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "4k-playback-check.start_position=" << start_position << std::endl;
    std::cout << "4k-playback-check.end_position=" << end_position << std::endl;
    std::cout << "4k-playback-check.advanced_seconds=" << advanced_seconds << std::endl;
    std::cout << "4k-playback-check.advance_ratio=" << advance_ratio << std::endl;
    std::cout << "4k-playback-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.late_drops=" << diag.scheduler_late_drops << std::endl;
    std::cout << "4k-playback-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;
    std::cout << "4k-playback-check.hard.mode_ok=" << (hard_session.mode_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.hard.renderer_backend=" << hard_session.renderer_backend << std::endl;
    std::cout << "4k-playback-check.hard.decoder_backend=" << hard_session.decoder_backend << std::endl;
    std::cout << "4k-playback-check.hard.exit_code=" << hard_session.exit_code << std::endl;
    std::cout << "4k-playback-check.soft.mode_ok=" << (soft_session.mode_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.soft.renderer_backend=" << soft_session.renderer_backend << std::endl;
    std::cout << "4k-playback-check.soft.decoder_backend=" << soft_session.decoder_backend << std::endl;
    std::cout << "4k-playback-check.soft.exit_code=" << soft_session.exit_code << std::endl;
    std::cout << "4k-playback-check.fallback_ok=" << (fallback_ok ? "true" : "false") << std::endl;
    std::cout << "4k-playback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runScreenshotCheck(const std::string& media_file) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec) {
        std::cout << "screenshot-check.path=" << media_file << std::endl;
        std::cout << "screenshot-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered_playback_loop = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered_playback_loop;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    bool paused_before_request = false;
    bool request_ok = false;
    bool captured = false;
    bool file_exists = false;
    std::string screenshot_path;

    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(800));
        if (entered_playback_loop) {
            player.pause();
            paused_before_request = player.isPaused();
        }
        request_ok = player.requestScreenshot();
        if (request_ok) {
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
            while (std::chrono::steady_clock::now() < deadline &&
                   (player.isPlaying() || player.isPaused())) {
                player.pumpEvents();
                if (player.consumeLastScreenshotPath(screenshot_path)) {
                    captured = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }
        player.stop();
        player.close();
    }

    if (!screenshot_path.empty()) {
        const std::filesystem::path path(screenshot_path);
        file_exists = std::filesystem::exists(path, ec) && !ec && std::filesystem::is_regular_file(path, ec) && !ec;
    }

    const bool result = open_ok &&
                         entered_playback_loop &&
                         paused_before_request &&
                         request_ok &&
                         captured &&
                         file_exists;

    std::cout << "screenshot-check.path=" << media_file << std::endl;
    std::cout << "screenshot-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.paused_before_request=" << (paused_before_request ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.request_ok=" << (request_ok ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.captured=" << (captured ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.file_exists=" << (file_exists ? "true" : "false") << std::endl;
    std::cout << "screenshot-check.output=" << screenshot_path << std::endl;
    std::cout << "screenshot-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

}  // namespace

void signalHandler(int signal) {
    if (g_player) {
        g_player->stop();
    }
    Logger::shutdown();
    exit(0);
}

void printUsage(const char* program_name) {
    std::cout << "Usage: " << program_name << " <video_file> [more_video_files...]" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --subtitle <subtitle.srt>" << std::endl;
    std::cout << "       " << program_name << " <playlist.m3u8>" << std::endl;
    std::cout << "       " << program_name << " --capabilities" << std::endl;
    std::cout << "       " << program_name << " --probe-file <media_file> [--json]" << std::endl;
    std::cout << "       " << program_name << " --subtitle-sync-check <subtitle.srt>" << std::endl;
    std::cout << "       " << program_name << " --playlist-flow-check <media1> <media2> <media3> <media4> <media5> [more...]" << std::endl;
    std::cout << "       " << program_name << " --settings-persistence-check [settings_file]" << std::endl;
    std::cout << "       " << program_name << " --renderer-fallback-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --windows-backend-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --chapter-nav-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --ab-repeat-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --frame-step-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --delay-adjust-check <media_file> <subtitle.srt>" << std::endl;
    std::cout << "       " << program_name << " --numeric-seek-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --performance-log-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --1080p60-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --4k-playback-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --screenshot-check <media_file>" << std::endl;
    std::cout << "       " << program_name
              << " --evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  ENTER/F or ALT+ENTER - Toggle Fullscreen" << std::endl;
    std::cout << "  ESC - Exit Fullscreen (or Quit when windowed)" << std::endl;
    std::cout << "  Q - Quit" << std::endl;
    std::cout << "  LEFT/RIGHT - Seek -/+5s" << std::endl;
    std::cout << "  CTRL+LEFT/CTRL+RIGHT - Seek -/+30s" << std::endl;
    std::cout << "  PAGEUP/PAGEDOWN - Previous/Next media in playlist" << std::endl;
    std::cout << "  HOME/END - Previous/Next chapter" << std::endl;
    std::cout << "  A/B/C - Set A point / Set B point / Clear A-B repeat" << std::endl;
    std::cout << "  S - Save screenshot" << std::endl;
    std::cout << "  , / . - Step backward / forward one frame when paused" << std::endl;
    std::cout << "  J / K - Subtitle delay -/+100ms" << std::endl;
    std::cout << "  CTRL+J / CTRL+K - Audio delay -/+100ms" << std::endl;
    std::cout << "  1..9 - Jump to 10%..90% of media" << std::endl;
    std::cout << "  [/ ] / R - Speed down/up/reset" << std::endl;
    std::cout << "  V - Toggle subtitles on/off" << std::endl;
    std::cout << "  M - Mute/Unmute" << std::endl;
    std::cout << "  Mouse drag progress bar - Seek" << std::endl;
    std::cout << "  Mouse drag volume bar - Volume" << std::endl;
    std::cout << "  +/- or Up/Down - Adjust volume" << std::endl;
    std::cout << "  Custom hotkeys: config/player_settings.ini (hotkey.*)" << std::endl;
    std::cout << "  Restore defaults: set hotkey.restore_defaults=true then restart" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    if (argc >= 2 && std::string(argv[1]) == "--capabilities") {
        printCapabilityMatrix();
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--evaluate-target") {
        if (argc < 7) {
            printUsage(argv[0]);
            return 1;
        }

        int width = 0;
        int height = 0;
        int channels = 0;
        double fps = 0.0;
        double bitrate_mbps = 0.0;
        if (!tryParseInt(argv[2], width) ||
            !tryParseInt(argv[3], height) ||
            !tryParseDouble(argv[4], fps) ||
            !tryParseInt(argv[5], channels) ||
            !tryParseDouble(argv[6], bitrate_mbps)) {
            printUsage(argv[0]);
            return 1;
        }

        media::PlaybackCapabilityTarget target{};
        target.width = width;
        target.height = height;
        target.fps = fps;
        target.audio_channels = channels;
        target.video_bitrate = static_cast<uint64_t>(std::max(0.0, bitrate_mbps) * 1000000.0);
        printTargetDecision(target);
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--probe-file") {
        std::string media_path;
        bool output_json = false;

        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "--json") {
                output_json = true;
                continue;
            }
            if (media_path.empty()) {
                media_path = arg;
                continue;
            }
            printUsage(argv[0]);
            return 1;
        }

        if (media_path.empty()) {
            printUsage(argv[0]);
            return 1;
        }

        const ProbeReport report = collectFileProbeReport(media_path);
        if (output_json) {
            printFileProbeJsonReport(report);
        } else {
            printFileProbeTextReport(report);
        }
        return report.exit_code;
    }

    if (argc >= 2 && std::string(argv[1]) == "--subtitle-sync-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runSubtitleSyncCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--playlist-flow-check") {
        if (argc < 3) {
            printUsage(argv[0]);
            return 1;
        }
        std::vector<std::string> media_inputs;
        media_inputs.reserve(static_cast<size_t>(argc - 2));
        for (int i = 2; i < argc; ++i) {
            media_inputs.push_back(argv[i]);
        }
        return runPlaylistFlowCheck(media_inputs) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--settings-persistence-check") {
        if (argc > 3) {
            printUsage(argv[0]);
            return 1;
        }
        const std::string override_path = (argc == 3) ? argv[2] : std::string{};
        return runSettingsPersistenceCheck(override_path) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--renderer-fallback-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runRendererFallbackCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--windows-backend-session-check") {
        if (argc != 4) {
            printUsage(argv[0]);
            return 1;
        }
        return runWindowsBackendSessionCheck(argv[2], argv[3]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--windows-backend-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runWindowsBackendPlaybackCheck(argv[0], argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--chapter-nav-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runChapterNavigationCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--ab-repeat-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runABRepeatCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--frame-step-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runFrameStepCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--delay-adjust-check") {
        if (argc != 4) {
            printUsage(argv[0]);
            return 1;
        }
        return runDelayAdjustCheck(argv[2], argv[3]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--numeric-seek-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runNumericSeekCheck(argv[2]) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--performance-log-check") {
        if (argc != 3 && argc != 4) {
            printUsage(argv[0]);
            return 1;
        }
        int sample_ms = 1500;
        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {
            printUsage(argv[0]);
            return 1;
        }
        return runPerformanceLogCheck(argv[2], sample_ms) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--1080p60-check") {
        if (argc != 3 && argc != 4) {
            printUsage(argv[0]);
            return 1;
        }
        int sample_ms = 5000;
        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {
            printUsage(argv[0]);
            return 1;
        }
        return run1080p60Check(argv[2], sample_ms) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--4k-playback-check") {
        if (argc != 3 && argc != 4) {
            printUsage(argv[0]);
            return 1;
        }
        int sample_ms = 2000;
        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {
            printUsage(argv[0]);
            return 1;
        }
        return run4kPlaybackCheck(argv[0], argv[2], sample_ms) ? 0 : 2;
    }

    if (argc >= 2 && std::string(argv[1]) == "--screenshot-check") {
        if (argc != 3) {
            printUsage(argv[0]);
            return 1;
        }
        return runScreenshotCheck(argv[2]) ? 0 : 2;
    }
    
    if (argc < 2) {
        printUsage(argv[0]);
        return 1;
    }

    PlaybackCliArgs cli_args;
    std::string cli_error;
    if (!parsePlaybackCliArgs(argc, argv, cli_args, cli_error)) {
        std::cerr << "Argument error: " << cli_error << std::endl;
        printUsage(argv[0]);
        return 1;
    }

    Logger::init();
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Video Player - FFmpeg + SDL2 + C++17" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    Logger::info("Starting Modern Video Player");
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    const std::string settings_path = "config/player_settings.ini";
    config::SettingsManager settings_manager;
    const AppSettings app_settings = loadAppSettings(settings_manager, settings_path);

    playlist::PlaylistManager playlist_manager = buildPlaylistFromInputs(cli_args.media_inputs);
    if (playlist_manager.empty()) {
        Logger::error("No playable media found from input arguments");
        Logger::shutdown();
        return 1;
    }

    size_t current_index = 0;
    if (app_settings.resume_last_playlist && app_settings.last_playlist_index >= 0) {
        const size_t saved_index = static_cast<size_t>(app_settings.last_playlist_index);
        if (saved_index < playlist_manager.size()) {
            current_index = saved_index;
        }
    }

    g_player = std::make_unique<VideoPlayer>();
    g_player->setVolume(app_settings.volume);
    g_player->setPlaybackSpeed(app_settings.playback_speed);
    g_player->setAudioDelay(app_settings.audio_delay_seconds);
    g_player->setSubtitleDelay(app_settings.subtitle_delay_seconds);
    g_player->setPreferHardwareDecode(app_settings.prefer_hardware_decode);
    g_player->setHotkeyManager(app_settings.hotkey_manager);

    bool quit_requested = false;
    bool opened_any_media = false;

    while (!quit_requested && current_index < playlist_manager.size()) {
        const playlist::PlaylistItem& item = playlist_manager.items()[current_index];
        Logger::info("Opening file: " + item.uri);

        if (!g_player->open(item.uri)) {
            Logger::error("Could not open media file: " + item.uri);
            if (current_index + 1 < playlist_manager.size()) {
                ++current_index;
                continue;
            }
            break;
        }

        opened_any_media = true;
        Logger::info("Starting playback...");
        std::cout << "Playing: " << item.uri << std::endl;

        const std::string subtitle_path =
            !cli_args.subtitle_file.empty() ? cli_args.subtitle_file : detectAutoSubtitlePath(item.uri);
        if (!subtitle_path.empty()) {
            if (g_player->loadExternalSubtitle(subtitle_path)) {
                Logger::info("Loaded external subtitle: " + subtitle_path +
                             " entries=" + std::to_string(g_player->externalSubtitleCount()));
            } else {
                Logger::warning("Failed to load external subtitle: " + subtitle_path);
            }
        }

        g_player->play();

        bool next_requested = false;
        bool previous_requested = false;
        while (g_player->isPlaying() || g_player->isPaused()) {
            g_player->pumpEvents();
            if (g_player->consumeQuitRequest()) {
                quit_requested = true;
                break;
            }
            if (g_player->consumeNextItemRequest()) {
                next_requested = true;
                break;
            }
            if (g_player->consumePreviousItemRequest()) {
                previous_requested = true;
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        g_player->stop();
        g_player->close();

        if (quit_requested) {
            break;
        }

        if (previous_requested) {
            if (current_index > 0) {
                --current_index;
            }
            continue;
        }

        if (next_requested) {
            if (current_index + 1 < playlist_manager.size()) {
                ++current_index;
                continue;
            }
            break;
        }

        // Auto-next on EOF.
        if (current_index + 1 < playlist_manager.size()) {
            ++current_index;
            continue;
        }
        break;
    }

    const float final_volume = g_player ? g_player->getVolume() : app_settings.volume;
    const double final_speed = g_player ? g_player->getPlaybackSpeed() : app_settings.playback_speed;
    const double final_audio_delay = g_player ? g_player->getAudioDelay() : app_settings.audio_delay_seconds;
    const double final_subtitle_delay = g_player ? g_player->getSubtitleDelay() : app_settings.subtitle_delay_seconds;
    const input::HotkeyManager& final_hotkeys = g_player ? g_player->hotkeyManager() : app_settings.hotkey_manager;
    saveAppSettings(settings_manager,
                    settings_path,
                    final_volume,
                    final_speed,
                    final_audio_delay,
                    final_subtitle_delay,
                    app_settings.prefer_hardware_decode,
                    app_settings.resume_last_playlist,
                    static_cast<int>(current_index),
                    final_hotkeys);

    if (!opened_any_media) {
        Logger::error("No media could be opened");
        Logger::shutdown();
        return 1;
    }

    Logger::info("Playback finished");
    Logger::shutdown();
    
    std::cout << std::endl;
    std::cout << "Playback finished." << std::endl;
    
    return 0;
}
