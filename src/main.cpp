#include "video_player.h"

#include "config/settings_manager.h"

#include "demuxer.h"

#include "filters/filter_registry.h"

#include "media/format_support.h"
#include "media/ffmpeg_channel_layout_compat.h"

#include "playlist/playlist_manager.h"

#include "plugin/plugin_manager.h"

#include "logger.h"
#include "input/playback_input_source.h"

#include "mvp_version.h"

#include "render/renderer_factory.h"

#include "streaming/adaptive_bitrate_selector.h"

#include "streaming/dash_manifest_parser.h"

#include "streaming/hls_manifest_parser.h"

#include "streaming/http_stream_downloader.h"

#include "subtitle/subtitle_font_registry.h"

#include "subtitle/embedded_subtitle_loader.h"
#include "subtitle/libass_probe.h"

#include "subtitle/subtitle_parser.h"

#include "subtitle/subtitle_timeline.h"

#include "thread_safe_queue.h"

#if defined(_WIN32) && defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
#include "render/d3d11_video_renderer.h"
#endif
#if defined(_WIN32) && defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
#include "render/opengl_video_renderer.h"
#endif

#ifndef MVP_VULKAN_DEPENDENCY_SOURCE
#define MVP_VULKAN_DEPENDENCY_SOURCE "unknown"
#endif

#if __has_include(<SDL2/SDL.h>)
#include <SDL2/SDL.h>
#elif __has_include(<SDL.h>)
#include <SDL.h>
#else
#error "SDL2 headers not found"
#endif



extern "C" {

#include <libavutil/error.h>

#include <libavutil/log.h>

}



#include <atomic>

#include <algorithm>

#include <chrono>

#include <csignal>

#include <cctype>

#include <cmath>

#include <cstdio>

#include <cstdlib>

#include <ctime>

#include <filesystem>

#include <fstream>

#include <functional>

#include <iomanip>

#include <iostream>

#include <limits>

#include <memory>

#include <set>

#include <sstream>

#include <system_error>

#include <thread>

#include <vector>



#if defined(_WIN32)

#ifndef NOMINMAX

#define NOMINMAX

#endif

#include <windows.h>
#include <dwrite.h>
#include <wrl/client.h>

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



std::string avErrorToString(int error_code) {

    if (error_code == std::numeric_limits<int>::min()) {

        return "not-set";

    }

    char buffer[AV_ERROR_MAX_STRING_SIZE] = {};

    if (av_strerror(error_code, buffer, sizeof(buffer)) == 0) {

        return std::string(buffer);

    }

    return "unknown";

}

void printD3D11RuntimeHdrState(const std::string& prefix, const core::DiagnosticsSnapshot& diag);



const char* clockSourceName(core::ClockSource source) {

    switch (source) {

    case core::ClockSource::Audio:

        return "Audio";

    case core::ClockSource::Video:

        return "Video";

    case core::ClockSource::System:

        return "System";

    default:

        return "Unknown";

    }

}



const char* endedReasonName(core::EndedReason reason) {

    switch (reason) {

    case core::EndedReason::None:

        return "None";

    case core::EndedReason::Eof:

        return "Eof";

    default:

        return "Unknown";

    }

}

const char* schedulerClockPolicyName(core::SchedulerClockPolicy policy) {

    switch (policy) {

    case core::SchedulerClockPolicy::UseClockSource:

        return "UseClockSource";

    case core::SchedulerClockPolicy::AudioMaster:

        return "AudioMaster";

    case core::SchedulerClockPolicy::VideoMaster:

        return "VideoMaster";

    case core::SchedulerClockPolicy::SystemMonotonic:

        return "SystemMonotonic";

    default:

        return "Unknown";

    }

}

const char* schedulerAudioMasterPolicyName(core::SchedulerAudioMasterPolicy policy) {

    switch (policy) {

    case core::SchedulerAudioMasterPolicy::Disabled:

        return "Disabled";

    case core::SchedulerAudioMasterPolicy::SoftWhenAudioReady:

        return "SoftWhenAudioReady";

    case core::SchedulerAudioMasterPolicy::RequireBufferedAudio:

        return "RequireBufferedAudio";

    default:

        return "Unknown";

    }

}

const char* schedulerEndedPolicyName(core::SchedulerEndedPolicy policy) {

    switch (policy) {

    case core::SchedulerEndedPolicy::StopOutput:

        return "StopOutput";

    case core::SchedulerEndedPolicy::HoldLastFrame:

        return "HoldLastFrame";

    case core::SchedulerEndedPolicy::HoldLastFrameNoClockSync:

        return "HoldLastFrameNoClockSync";

    default:

        return "Unknown";

    }

}



bool startsWithInsensitive(const std::string& value, const std::string& prefix) {

    if (value.size() < prefix.size()) {

        return false;

    }

    return toLower(value.substr(0, prefix.size())) == toLower(prefix);

}



bool containsString(const std::vector<std::string>& values, const std::string& expected) {

    return std::find(values.begin(), values.end(), expected) != values.end();

}



std::string resolveUrl(const std::string& base_url, const std::string& relative_url) {

    if (relative_url.empty()) {

        return base_url;

    }

    if (startsWithInsensitive(relative_url, "http://") || startsWithInsensitive(relative_url, "https://")) {

        return relative_url;

    }



    const size_t scheme_pos = base_url.find("://");

    if (scheme_pos == std::string::npos) {

        return relative_url;

    }

    const size_t authority_start = scheme_pos + 3;

    const size_t path_start = base_url.find('/', authority_start);

    const std::string origin = (path_start == std::string::npos) ? base_url : base_url.substr(0, path_start);



    if (!relative_url.empty() && relative_url.front() == '/') {

        return origin + relative_url;

    }



    const size_t last_slash = base_url.find_last_of('/');

    if (last_slash == std::string::npos || last_slash < authority_start) {

        return origin + "/" + relative_url;

    }

    return base_url.substr(0, last_slash + 1) + relative_url;

}



/// 读取整个远端文本资源；供 HLS/DASH 清单和远端播放列表检查复用。

bool readUrlText(const std::string& url, std::string& text_out, std::string& error_out) {

    streaming::HttpStreamDownloader downloader;

    if (!downloader.open(url)) {

        error_out = downloader.lastError();

        return false;

    }



    constexpr size_t kChunkSize = 32 * 1024;

    text_out.clear();

    while (!downloader.eof() || downloader.bufferedBytes() > 0) {

        if (!downloader.prefetch(kChunkSize, kChunkSize)) {

            error_out = downloader.lastError();

            downloader.close();

            return false;

        }

        const auto chunk = downloader.consumeBuffered(kChunkSize);

        if (chunk.empty()) {

            continue;

        }

        text_out.append(reinterpret_cast<const char*>(chunk.data()), chunk.size());

    }



    downloader.close();

    error_out.clear();

    return true;

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



/// 打印运行时容器/编码能力与主流格式覆盖矩阵。

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



/// 单文件探测报告；汇总容器、音视频流与播放建议。

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



/// 对单个媒体文件做静态探测，生成可打印的能力与风险报告。

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

        report.audio_channels = std::max(1, media::ffmpeg_compat::codecParametersChannels(audio_stream->codecpar, 2));

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



int64_t collectFormatBitrateBitsPerSecond(const std::string& path) {

    ScopedAvLogLevel quiet_logs(AV_LOG_ERROR);

    AVFormatContext* fmt_ctx = nullptr;

    if (avformat_open_input(&fmt_ctx, path.c_str(), nullptr, nullptr) < 0 || !fmt_ctx) {

        return 0;

    }



    if (avformat_find_stream_info(fmt_ctx, nullptr) < 0) {

        const int64_t bitrate = std::max<int64_t>(0, fmt_ctx->bit_rate);

        avformat_close_input(&fmt_ctx);

        return bitrate;

    }



    const int64_t bitrate = std::max<int64_t>(0, fmt_ctx->bit_rate);

    avformat_close_input(&fmt_ctx);

    return bitrate;

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



/// 验证字幕时间线解析是否稳定，覆盖顺序播放和跳转场景。

std::vector<int> resolveActiveSubtitleIndicesByScan(const std::vector<subtitle::SubtitleItem>& items,
                                                    double position_seconds) {

    std::vector<int> active_indices;
    for (size_t i = 0; i < items.size(); ++i) {
        const subtitle::SubtitleItem& item = items[i];
        if (position_seconds >= item.start_seconds && position_seconds <= item.end_seconds) {
            active_indices.push_back(static_cast<int>(i));
        }
    }
    return active_indices;

}

uint64_t hashBitmapSubtitleForValidation(const subtitle::SubtitleBitmap& bitmap) {

    constexpr uint64_t kFnvOffset = 1469598103934665603ull;
    constexpr uint64_t kFnvPrime = 1099511628211ull;

    auto hash_bytes = [kFnvPrime](uint64_t seed, const uint8_t* data, size_t size) {
        uint64_t hash = seed;
        for (size_t i = 0; i < size; ++i) {
            hash ^= static_cast<uint64_t>(data[i]);
            hash *= kFnvPrime;
        }
        return hash;
    };

    uint64_t hash = kFnvOffset;
    const int width = bitmap.width;
    const int height = bitmap.height;
    hash = hash_bytes(hash, reinterpret_cast<const uint8_t*>(&width), sizeof(width));
    hash = hash_bytes(hash, reinterpret_cast<const uint8_t*>(&height), sizeof(height));
    return hash_bytes(hash, bitmap.rgba.data(), bitmap.rgba.size());

}

struct BitmapSubtitleValidationSummary {
    size_t item_count{0};
    size_t rect_count{0};
    size_t multi_rect_item_count{0};
    size_t max_rects_per_item{0};
    size_t unique_bitmap_signature_count{0};
    size_t cache_reuse_candidate_count{0};
};

BitmapSubtitleValidationSummary summarizeBitmapSubtitleItems(const std::vector<subtitle::SubtitleItem>& items) {

    BitmapSubtitleValidationSummary summary;
    summary.item_count = items.size();
    std::set<uint64_t> unique_signatures;

    for (const subtitle::SubtitleItem& item : items) {
        if (!item.is_bitmap) {
            continue;
        }

        const size_t rect_count = item.bitmap_rects.size();
        summary.rect_count += rect_count;
        if (rect_count > 1) {
            ++summary.multi_rect_item_count;
        }
        summary.max_rects_per_item = std::max(summary.max_rects_per_item, rect_count);

        for (const subtitle::SubtitleBitmap& bitmap : item.bitmap_rects) {
            unique_signatures.insert(hashBitmapSubtitleForValidation(bitmap));
        }
    }

    summary.unique_bitmap_signature_count = unique_signatures.size();
    if (summary.rect_count >= summary.unique_bitmap_signature_count) {
        summary.cache_reuse_candidate_count = summary.rect_count - summary.unique_bitmap_signature_count;
    }
    return summary;

}

size_t countActiveBitmapRects(const std::vector<subtitle::SubtitleItem>& items,
                              const std::vector<int>& active_indices) {

    size_t rect_count = 0;
    for (int index : active_indices) {
        if (index < 0 || index >= static_cast<int>(items.size())) {
            continue;
        }
        rect_count += items[static_cast<size_t>(index)].bitmap_rects.size();
    }
    return rect_count;

}

int selectBestBitmapSubtitleStream(const std::vector<subtitle::EmbeddedSubtitleTrackInfo>& tracks) {

    int best_stream_index = -1;
    int best_score = std::numeric_limits<int>::min();
    for (const subtitle::EmbeddedSubtitleTrackInfo& track : tracks) {
        if (!track.supported_bitmap_codec) {
            continue;
        }
        if (track.preference_score > best_score) {
            best_score = track.preference_score;
            best_stream_index = track.stream_index;
        }
    }
    return best_stream_index;

}

bool runSubtitleSyncCheck(const std::string& subtitle_path) {

    auto parser = subtitle::createParserForPath(subtitle_path);

    if (!parser || !parser->parseFile(subtitle_path)) {

        std::cerr << "subtitle-sync-check: failed to parse subtitle file: " << subtitle_path << std::endl;

        return false;

    }



    std::vector<subtitle::SubtitleItem> items = sortSubtitleItems(parser->items());

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

const char* subtitleFormatName(subtitle::SubtitleFormat format) {

    switch (format) {

    case subtitle::SubtitleFormat::Srt:

        return "srt";

    case subtitle::SubtitleFormat::Ass:

        return "ass";

    case subtitle::SubtitleFormat::Ssa:

        return "ssa";

    default:

        return "unknown";

    }

}

std::string formatSubtitleColor(const subtitle::SubtitleColor& color) {

    std::ostringstream oss;

    oss << std::hex << std::uppercase << std::setfill('0')

        << std::setw(2) << static_cast<unsigned int>(color.r)

        << std::setw(2) << static_cast<unsigned int>(color.g)

        << std::setw(2) << static_cast<unsigned int>(color.b)

        << std::setw(2) << static_cast<unsigned int>(color.a);

    return oss.str();

}

std::string formatSubtitleTransitionMask(uint64_t mask) {
    struct NamedBit {
        uint64_t value;
        const char* name;
    };
    static const NamedBit kNamedBits[] = {
        {subtitle::kSubtitleTransitionFontSize, "font_size"},
        {subtitle::kSubtitleTransitionPrimaryColor, "primary_color"},
        {subtitle::kSubtitleTransitionSecondaryColor, "secondary_color"},
        {subtitle::kSubtitleTransitionOutlineColor, "outline_color"},
        {subtitle::kSubtitleTransitionBackgroundColor, "background_color"},
        {subtitle::kSubtitleTransitionOutlineX, "outline_x"},
        {subtitle::kSubtitleTransitionOutlineY, "outline_y"},
        {subtitle::kSubtitleTransitionShadowX, "shadow_x"},
        {subtitle::kSubtitleTransitionShadowY, "shadow_y"},
        {subtitle::kSubtitleTransitionSpacing, "spacing"},
        {subtitle::kSubtitleTransitionScaleX, "scale_x"},
        {subtitle::kSubtitleTransitionScaleY, "scale_y"},
        {subtitle::kSubtitleTransitionRotationZ, "rotation_z"},
        {subtitle::kSubtitleTransitionRotationX, "rotation_x"},
        {subtitle::kSubtitleTransitionRotationY, "rotation_y"},
        {subtitle::kSubtitleTransitionShearX, "shear_x"},
        {subtitle::kSubtitleTransitionShearY, "shear_y"},
        {subtitle::kSubtitleTransitionRotationOrigin, "rotation_origin"},
    };

    std::ostringstream oss;
    bool first = true;
    for (const NamedBit& named_bit : kNamedBits) {
        if ((mask & named_bit.value) == 0) {
            continue;
        }
        if (!first) {
            oss << ",";
        }
        first = false;
        oss << named_bit.name;
    }
    return oss.str();
}

const char* subtitleKaraokeModeName(subtitle::SubtitleKaraokeMode mode) {
    switch (mode) {
    case subtitle::SubtitleKaraokeMode::None:
        return "none";
    case subtitle::SubtitleKaraokeMode::Instant:
        return "instant";
    case subtitle::SubtitleKaraokeMode::Sweep:
        return "sweep";
    default:
        return "unknown";
    }
}

const char* subtitleFadeModeName(subtitle::SubtitleFadeMode mode) {
    switch (mode) {
    case subtitle::SubtitleFadeMode::None:
        return "none";
    case subtitle::SubtitleFadeMode::Simple:
        return "simple";
    case subtitle::SubtitleFadeMode::Complex:
        return "complex";
    default:
        return "unknown";
    }
}

void printSubtitleStyleFields(const std::string& prefix, const subtitle::SubtitleStyle& style) {

    std::cout << prefix << ".style_name=" << std::quoted(style.style_name) << std::endl;

    std::cout << prefix << ".font_family=" << std::quoted(style.font_family) << std::endl;

    std::cout << prefix << ".font_size=" << style.font_size << std::endl;

    std::cout << prefix << ".bold=" << (style.bold ? "true" : "false") << std::endl;

    std::cout << prefix << ".italic=" << (style.italic ? "true" : "false") << std::endl;

    std::cout << prefix << ".underline=" << (style.underline ? "true" : "false") << std::endl;

    std::cout << prefix << ".strikeout=" << (style.strikeout ? "true" : "false") << std::endl;

    std::cout << prefix << ".primary_rgba=" << formatSubtitleColor(style.primary_color) << std::endl;

    std::cout << prefix << ".secondary_rgba=" << formatSubtitleColor(style.secondary_color) << std::endl;

    std::cout << prefix << ".outline_rgba=" << formatSubtitleColor(style.outline_color) << std::endl;

    std::cout << prefix << ".background_rgba=" << formatSubtitleColor(style.background_color) << std::endl;

    std::cout << prefix << ".alignment=" << style.alignment << std::endl;

    std::cout << prefix << ".margin_l=" << style.margin_l << std::endl;

    std::cout << prefix << ".margin_r=" << style.margin_r << std::endl;

    std::cout << prefix << ".margin_v=" << style.margin_v << std::endl;

    std::cout << prefix << ".border_style=" << style.border_style << std::endl;

    std::cout << prefix << ".outline=" << style.outline << std::endl;

    std::cout << prefix << ".shadow=" << style.shadow << std::endl;

    std::cout << prefix << ".outline_x=" << style.outline_x << std::endl;

    std::cout << prefix << ".outline_y=" << style.outline_y << std::endl;

    std::cout << prefix << ".shadow_x=" << style.shadow_x << std::endl;

    std::cout << prefix << ".shadow_y=" << style.shadow_y << std::endl;

    std::cout << prefix << ".wrap_style=" << style.wrap_style << std::endl;

    std::cout << prefix << ".spacing=" << style.spacing << std::endl;

    std::cout << prefix << ".scale_x_percent=" << style.scale_x_percent << std::endl;

    std::cout << prefix << ".scale_y_percent=" << style.scale_y_percent << std::endl;

    std::cout << prefix << ".rotation_degrees=" << style.rotation_degrees << std::endl;

    std::cout << prefix << ".rotation_x_degrees=" << style.rotation_x_degrees << std::endl;

    std::cout << prefix << ".rotation_y_degrees=" << style.rotation_y_degrees << std::endl;

    std::cout << prefix << ".shear_x=" << style.shear_x << std::endl;

    std::cout << prefix << ".shear_y=" << style.shear_y << std::endl;

    std::cout << prefix << ".has_rotation_origin=" << (style.has_rotation_origin ? "true" : "false") << std::endl;

    std::cout << prefix << ".rotation_origin_x=" << style.rotation_origin_x << std::endl;

    std::cout << prefix << ".rotation_origin_y=" << style.rotation_origin_y << std::endl;

    std::cout << prefix << ".has_clip=" << (style.has_clip ? "true" : "false") << std::endl;

    std::cout << prefix << ".inverse_clip=" << (style.inverse_clip ? "true" : "false") << std::endl;

    std::cout << prefix << ".clip_x1=" << style.clip_x1 << std::endl;

    std::cout << prefix << ".clip_y1=" << style.clip_y1 << std::endl;

    std::cout << prefix << ".clip_x2=" << style.clip_x2 << std::endl;

    std::cout << prefix << ".clip_y2=" << style.clip_y2 << std::endl;

    std::cout << prefix << ".has_vector_clip=" << (style.has_vector_clip ? "true" : "false") << std::endl;

    std::cout << prefix << ".vector_clip_scale=" << style.vector_clip_scale << std::endl;

    std::cout << prefix << ".vector_clip_commands=" << std::quoted(style.vector_clip_commands) << std::endl;

    std::cout << prefix << ".has_position=" << (style.has_position ? "true" : "false") << std::endl;

    std::cout << prefix << ".position_x=" << style.position_x << std::endl;

    std::cout << prefix << ".position_y=" << style.position_y << std::endl;

}

void printSubtitleAnimationFields(const std::string& prefix, const subtitle::SubtitleStyleAnimation& animation) {
    std::cout << prefix << ".has_move=" << (animation.has_move ? "true" : "false") << std::endl;
    std::cout << prefix << ".move_has_timing=" << (animation.move_has_timing ? "true" : "false") << std::endl;
    std::cout << prefix << ".move_x1=" << animation.move_x1 << std::endl;
    std::cout << prefix << ".move_y1=" << animation.move_y1 << std::endl;
    std::cout << prefix << ".move_x2=" << animation.move_x2 << std::endl;
    std::cout << prefix << ".move_y2=" << animation.move_y2 << std::endl;
    std::cout << prefix << ".move_t1_seconds=" << animation.move_t1_seconds << std::endl;
    std::cout << prefix << ".move_t2_seconds=" << animation.move_t2_seconds << std::endl;
    std::cout << prefix << ".fade_mode=" << subtitleFadeModeName(animation.fade_mode) << std::endl;
    std::cout << prefix << ".fade_in_seconds=" << animation.fade_in_seconds << std::endl;
    std::cout << prefix << ".fade_out_seconds=" << animation.fade_out_seconds << std::endl;
    std::cout << prefix << ".fade_alpha1=" << static_cast<unsigned int>(animation.fade_alpha1) << std::endl;
    std::cout << prefix << ".fade_alpha2=" << static_cast<unsigned int>(animation.fade_alpha2) << std::endl;
    std::cout << prefix << ".fade_alpha3=" << static_cast<unsigned int>(animation.fade_alpha3) << std::endl;
    std::cout << prefix << ".fade_t1_seconds=" << animation.fade_t1_seconds << std::endl;
    std::cout << prefix << ".fade_t2_seconds=" << animation.fade_t2_seconds << std::endl;
    std::cout << prefix << ".fade_t3_seconds=" << animation.fade_t3_seconds << std::endl;
    std::cout << prefix << ".fade_t4_seconds=" << animation.fade_t4_seconds << std::endl;
    std::cout << prefix << ".transition_count=" << animation.style_transitions.size() << std::endl;
    for (size_t transition_index = 0; transition_index < animation.style_transitions.size(); ++transition_index) {
        const subtitle::SubtitleStyleTransition& transition = animation.style_transitions[transition_index];
        const std::string transition_prefix = prefix + ".transition" + std::to_string(transition_index);
        std::cout << transition_prefix << ".has_timing=" << (transition.has_timing ? "true" : "false") << std::endl;
        std::cout << transition_prefix << ".start_seconds=" << transition.start_seconds << std::endl;
        std::cout << transition_prefix << ".end_seconds=" << transition.end_seconds << std::endl;
        std::cout << transition_prefix << ".accel=" << transition.accel << std::endl;
        std::cout << transition_prefix << ".property_mask=" << transition.property_mask << std::endl;
        std::cout << transition_prefix << ".property_names=" << std::quoted(formatSubtitleTransitionMask(transition.property_mask)) << std::endl;
        printSubtitleStyleFields(transition_prefix + ".target", transition.target_style);
    }
}

bool runSubtitleStyleCheck(const std::string& subtitle_path) {

    auto parser = subtitle::createParserForPath(subtitle_path);

    if (!parser || !parser->parseFile(subtitle_path)) {

        std::cerr << "subtitle-style-check: failed to parse subtitle file: " << subtitle_path << std::endl;

        return false;

    }

    const auto& parsed_items = parser->items();

    std::cout << "subtitle-style-check.path=" << subtitle_path << std::endl;

    std::cout << "subtitle-style-check.format=" << subtitleFormatName(parser->format()) << std::endl;

    std::cout << "subtitle-style-check.entries=" << parsed_items.size() << std::endl;

    for (size_t item_index = 0; item_index < parsed_items.size(); ++item_index) {

        const subtitle::SubtitleItem& item = parsed_items[item_index];

        const std::string item_prefix = "subtitle-style-check.item" + std::to_string(item_index);

        std::cout << item_prefix << ".index=" << item.index << std::endl;

        std::cout << item_prefix << ".layer=" << item.layer << std::endl;

        std::cout << item_prefix << ".start=" << item.start_seconds << std::endl;

        std::cout << item_prefix << ".end=" << item.end_seconds << std::endl;

        std::cout << item_prefix << ".play_res_x=" << item.play_res_x << std::endl;

        std::cout << item_prefix << ".play_res_y=" << item.play_res_y << std::endl;

        std::cout << item_prefix << ".source_path=" << std::quoted(item.source_path) << std::endl;

        std::cout << item_prefix << ".is_vector_drawing=" << (item.is_vector_drawing ? "true" : "false") << std::endl;

        std::cout << item_prefix << ".drawing_scale=" << item.drawing_scale << std::endl;

        std::cout << item_prefix << ".drawing_commands=" << std::quoted(item.drawing_commands) << std::endl;

        std::cout << item_prefix << ".text=" << std::quoted(item.text) << std::endl;

        std::cout << item_prefix << ".raw_text=" << std::quoted(item.raw_text) << std::endl;

        std::cout << item_prefix << ".run_count=" << item.runs.size() << std::endl;

        printSubtitleStyleFields(item_prefix + ".style", item.style);
        printSubtitleAnimationFields(item_prefix + ".animation", item.animation);

        for (size_t run_index = 0; run_index < item.runs.size(); ++run_index) {

            const subtitle::SubtitleTextRun& run = item.runs[run_index];

            const std::string run_prefix = item_prefix + ".run" + std::to_string(run_index);

            std::cout << run_prefix << ".start=" << run.start << std::endl;

            std::cout << run_prefix << ".length=" << run.length << std::endl;

            printSubtitleStyleFields(run_prefix + ".style", run.style);

            std::cout << run_prefix << ".karaoke_mode="
                      << subtitleKaraokeModeName(run.karaoke_mode) << std::endl;

            std::cout << run_prefix << ".karaoke_start_cs="
                      << run.karaoke_start_centiseconds << std::endl;

            std::cout << run_prefix << ".karaoke_end_cs="
                      << run.karaoke_end_centiseconds << std::endl;

        }

    }

    std::cout << "subtitle-style-check.result=PASS" << std::endl;

    return true;

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



/// 验证播放列表的装载、遍历和基础探测流程。

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



/// 应用级持久化设置快照。

struct AppSettings {

    float volume{1.0f};

    double playback_speed{1.0};

    double audio_delay_seconds{0.0};

    double subtitle_delay_seconds{0.0};

    bool prefer_hardware_decode{true};

    bool resume_last_playlist{true};

    int last_playlist_index{0};

    subtitle::EmbeddedSubtitleSelectionPolicy embedded_subtitle_selection_policy{};

    input::HotkeyManager hotkey_manager{};

};



/// 播放模式下的 CLI 参数解析结果。

struct PlaybackCliArgs {

    std::vector<std::string> media_inputs;

    std::string subtitle_file;

    int preferred_embedded_subtitle_stream_index{-1};

    bool has_preferred_embedded_subtitle_stream_index{false};

    std::vector<std::string> preferred_subtitle_languages;

    bool has_preferred_subtitle_languages{false};

    subtitle::EmbeddedSubtitleForcedPolicy subtitle_forced_policy{subtitle::EmbeddedSubtitleForcedPolicy::Auto};

    bool has_subtitle_forced_policy{false};

    subtitle::EmbeddedSubtitleSdhPolicy subtitle_sdh_policy{subtitle::EmbeddedSubtitleSdhPolicy::Auto};

    bool has_subtitle_sdh_policy{false};

    std::string opengl_hdr_output_mode;

    bool has_opengl_hdr_output_mode{false};

    std::string d3d11_hdr_output_mode;

    bool has_d3d11_hdr_output_mode{false};

    std::string opengl_3dlut_file;

    std::string opengl_icc_profile_file;

    bool enable_opengl_auto_icc{false};

};

std::string normalizeSubtitleLanguageTag(std::string value) {

    std::string normalized = toLower(std::move(value));
    std::replace(normalized.begin(), normalized.end(), '_', '-');
    normalized.erase(std::remove_if(normalized.begin(),
                                    normalized.end(),
                                    [](unsigned char ch) {
                                        return !(std::isalnum(ch) != 0 || ch == '-');
                                    }),
                     normalized.end());
    return normalized;

}

bool parsePreferredSubtitleLanguages(const std::string& raw_text, std::vector<std::string>& out_languages) {

    out_languages.clear();
    std::string normalized_text = toLower(raw_text);
    normalized_text.erase(std::remove_if(normalized_text.begin(),
                                         normalized_text.end(),
                                         [](unsigned char ch) { return std::isspace(ch) != 0; }),
                          normalized_text.end());
    if (normalized_text.empty() || normalized_text == "none" || normalized_text == "-" || normalized_text == "off") {
        return true;
    }

    std::string token;
    std::istringstream token_stream(normalized_text);
    while (std::getline(token_stream, token, ',')) {
        const std::string normalized_token = normalizeSubtitleLanguageTag(token);
        if (normalized_token.empty()) {
            return false;
        }
        if (std::find(out_languages.begin(), out_languages.end(), normalized_token) == out_languages.end()) {
            out_languages.push_back(normalized_token);
        }
    }
    return !out_languages.empty();

}

std::string preferredSubtitleLanguagesToConfigValue(const std::vector<std::string>& languages) {

    if (languages.empty()) {
        return "none";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < languages.size(); ++i) {
        if (i > 0) {
            oss << ",";
        }
        oss << languages[i];
    }
    return oss.str();

}

const char* embeddedSubtitleForcedPolicyName(subtitle::EmbeddedSubtitleForcedPolicy policy) {

    switch (policy) {
    case subtitle::EmbeddedSubtitleForcedPolicy::Auto:
        return "auto";
    case subtitle::EmbeddedSubtitleForcedPolicy::PreferForced:
        return "prefer";
    case subtitle::EmbeddedSubtitleForcedPolicy::AvoidForced:
        return "avoid";
    case subtitle::EmbeddedSubtitleForcedPolicy::OnlyForced:
        return "only";
    default:
        return "auto";
    }

}

const char* embeddedSubtitleSdhPolicyName(subtitle::EmbeddedSubtitleSdhPolicy policy) {

    switch (policy) {
    case subtitle::EmbeddedSubtitleSdhPolicy::Auto:
        return "auto";
    case subtitle::EmbeddedSubtitleSdhPolicy::PreferSdh:
        return "prefer";
    case subtitle::EmbeddedSubtitleSdhPolicy::AvoidSdh:
        return "avoid";
    case subtitle::EmbeddedSubtitleSdhPolicy::OnlySdh:
        return "only";
    default:
        return "auto";
    }

}

bool parseEmbeddedSubtitleForcedPolicy(const std::string& raw_text,
                                       subtitle::EmbeddedSubtitleForcedPolicy& out_policy) {

    const std::string normalized = toLower(raw_text);
    if (normalized == "auto") {
        out_policy = subtitle::EmbeddedSubtitleForcedPolicy::Auto;
        return true;
    }
    if (normalized == "prefer") {
        out_policy = subtitle::EmbeddedSubtitleForcedPolicy::PreferForced;
        return true;
    }
    if (normalized == "avoid") {
        out_policy = subtitle::EmbeddedSubtitleForcedPolicy::AvoidForced;
        return true;
    }
    if (normalized == "only") {
        out_policy = subtitle::EmbeddedSubtitleForcedPolicy::OnlyForced;
        return true;
    }
    return false;

}

bool parseEmbeddedSubtitleSdhPolicy(const std::string& raw_text,
                                    subtitle::EmbeddedSubtitleSdhPolicy& out_policy) {

    const std::string normalized = toLower(raw_text);
    if (normalized == "auto") {
        out_policy = subtitle::EmbeddedSubtitleSdhPolicy::Auto;
        return true;
    }
    if (normalized == "prefer") {
        out_policy = subtitle::EmbeddedSubtitleSdhPolicy::PreferSdh;
        return true;
    }
    if (normalized == "avoid") {
        out_policy = subtitle::EmbeddedSubtitleSdhPolicy::AvoidSdh;
        return true;
    }
    if (normalized == "only") {
        out_policy = subtitle::EmbeddedSubtitleSdhPolicy::OnlySdh;
        return true;
    }
    return false;

}



std::string normalizePathForTitle(const std::string& uri) {

    std::filesystem::path path(uri);

    const std::string title = path.filename().string();

    return title.empty() ? uri : title;

}



bool isM3U8File(const std::string& value) {

    return toLower(extensionFromPath(value)) == "m3u8";

}



/// 解析普通播放模式参数，并分离媒体输入与外挂字幕参数。

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

        if (arg == "--subtitle-track") {

            if (out.has_preferred_embedded_subtitle_stream_index) {

                error = "duplicate --subtitle-track option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing stream index after --subtitle-track";

                return false;

            }

            int stream_index = -1;
            if (!tryParseInt(argv[++i], stream_index)) {

                error = "invalid stream index for --subtitle-track";

                return false;

            }

            out.preferred_embedded_subtitle_stream_index = stream_index;
            out.has_preferred_embedded_subtitle_stream_index = true;
            continue;

        }

        if (arg == "--subtitle-language") {

            if (out.has_preferred_subtitle_languages) {

                error = "duplicate --subtitle-language option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing language list after --subtitle-language";

                return false;

            }

            std::vector<std::string> languages;
            if (!parsePreferredSubtitleLanguages(argv[++i], languages)) {

                error = "invalid language list for --subtitle-language (example: zh,zh-cn,en)";

                return false;

            }

            out.preferred_subtitle_languages = std::move(languages);
            out.has_preferred_subtitle_languages = true;
            continue;

        }

        if (arg == "--subtitle-forced") {

            if (out.has_subtitle_forced_policy) {

                error = "duplicate --subtitle-forced option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing policy after --subtitle-forced";

                return false;

            }

            subtitle::EmbeddedSubtitleForcedPolicy policy = subtitle::EmbeddedSubtitleForcedPolicy::Auto;
            if (!parseEmbeddedSubtitleForcedPolicy(argv[++i], policy)) {

                error = "invalid policy for --subtitle-forced (expected auto|prefer|avoid|only)";

                return false;

            }

            out.subtitle_forced_policy = policy;
            out.has_subtitle_forced_policy = true;
            continue;

        }

        if (arg == "--subtitle-sdh") {

            if (out.has_subtitle_sdh_policy) {

                error = "duplicate --subtitle-sdh option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing policy after --subtitle-sdh";

                return false;

            }

            subtitle::EmbeddedSubtitleSdhPolicy policy = subtitle::EmbeddedSubtitleSdhPolicy::Auto;
            if (!parseEmbeddedSubtitleSdhPolicy(argv[++i], policy)) {

                error = "invalid policy for --subtitle-sdh (expected auto|prefer|avoid|only)";

                return false;

            }

            out.subtitle_sdh_policy = policy;
            out.has_subtitle_sdh_policy = true;
            continue;

        }

        if (arg == "--opengl-hdr-output-mode") {

            if (out.has_opengl_hdr_output_mode) {

                error = "duplicate --opengl-hdr-output-mode option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing mode after --opengl-hdr-output-mode";

                return false;

            }

            const std::string mode = toLower(argv[++i]);
            if (mode != "auto" && mode != "off" && mode != "force") {

                error = "invalid mode for --opengl-hdr-output-mode (expected auto|off|force)";

                return false;

            }

            out.opengl_hdr_output_mode = mode;
            out.has_opengl_hdr_output_mode = true;
            continue;

        }

        if (arg == "--d3d11-hdr-output-mode") {

            if (out.has_d3d11_hdr_output_mode) {

                error = "duplicate --d3d11-hdr-output-mode option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing mode after --d3d11-hdr-output-mode";

                return false;

            }

            const std::string mode = toLower(argv[++i]);
            if (mode != "auto" && mode != "off" && mode != "force") {

                error = "invalid mode for --d3d11-hdr-output-mode (expected auto|off|force)";

                return false;

            }

            out.d3d11_hdr_output_mode = mode;
            out.has_d3d11_hdr_output_mode = true;
            continue;

        }

        if (arg == "--opengl-3dlut") {

            if (!out.opengl_3dlut_file.empty()) {

                error = "duplicate --opengl-3dlut option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing file path after --opengl-3dlut";

                return false;

            }

            out.opengl_3dlut_file = argv[++i];
            continue;

        }

        if (arg == "--opengl-icc-profile") {

            if (!out.opengl_icc_profile_file.empty()) {

                error = "duplicate --opengl-icc-profile option";

                return false;

            }

            if (i + 1 >= argc) {

                error = "missing file path after --opengl-icc-profile";

                return false;

            }

            out.opengl_icc_profile_file = argv[++i];
            continue;

        }

        if (arg == "--opengl-auto-icc") {

            if (out.enable_opengl_auto_icc) {

                error = "duplicate --opengl-auto-icc option";

                return false;

            }

            out.enable_opengl_auto_icc = true;
            continue;

        }



        if (!arg.empty() && arg[0] == '-') {

            error = "unknown option in playback mode: " + arg;

            return false;

        }



        out.media_inputs.push_back(arg);

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

const char* rendererTypeName(render::VideoRendererType type) {

    switch (type) {

    case render::VideoRendererType::SoftwareSDL:

        return "SoftwareSDL";

    case render::VideoRendererType::D3D11:

        return "D3D11";

    case render::VideoRendererType::OpenGL:

        return "OpenGL";

    case render::VideoRendererType::Vulkan:

        return "Vulkan";

    case render::VideoRendererType::Auto:

    default:

        return "Auto";

    }

}

std::string rendererCandidateChainToString(const std::vector<render::VideoRendererType>& candidates) {
    if (candidates.empty()) {
        return "none";
    }
    std::ostringstream oss;
    for (size_t i = 0; i < candidates.size(); ++i) {
        if (i > 0) {
            oss << " -> ";
        }
        oss << render::RendererFactory::rendererName(candidates[i]);
    }
    return oss.str();
}

bool tryResolveInteractiveMediaInput(const std::string& raw_input,

                                    playlist::PlaylistManager& playlist_manager,

                                    std::string& normalized_input,

                                    std::string& error) {

    playlist_manager = playlist::PlaylistManager{};

    normalized_input.clear();

    error.clear();



    if (raw_input.empty()) {

        error = "empty input";

        return false;

    }



    normalized_input = raw_input;

    if (raw_input.find("://") == std::string::npos) {

        std::error_code ec;

        std::filesystem::path fs_path(raw_input);

        if (!std::filesystem::exists(fs_path, ec) || ec) {

            error = "path does not exist";

            return false;

        }



        ec.clear();

        if (!std::filesystem::is_regular_file(fs_path, ec) || ec) {

            error = "path is not a regular file";

            return false;

        }



        ec.clear();

        const std::filesystem::path canonical_path = std::filesystem::weakly_canonical(fs_path, ec);

        normalized_input = (!ec ? canonical_path : fs_path.lexically_normal()).string();

    }



    playlist_manager = buildPlaylistFromInputs({normalized_input});

    if (playlist_manager.empty()) {

        error = "no playable items built from input";

        return false;

    }



    return true;

}

render::VideoRendererPtr createIdleRendererWindow(const input::HotkeyManager& hotkey_manager) {

    render::VideoRendererConfig renderer_config{};

    renderer_config.width = 1280;

    renderer_config.height = 720;

    renderer_config.title = "Modern Video Player - Drop Media Here";



    core::PlaybackOpenRequest idle_request{};
    idle_request.media_info.video_stream_idx = 0;
    idle_request.preferences.prefer_hardware_decode = true;
    idle_request.platform_capabilities = platform::PlatformCapabilitiesProbe::detect();
    const core::PlaybackOpenPlan idle_plan = core::PlaybackStrategy::buildOpenPlan(idle_request);

    std::vector<render::VideoRendererType> renderer_candidates = idle_plan.renderer_candidates;
    if (renderer_candidates.empty()) {
        renderer_candidates.push_back(render::VideoRendererType::SoftwareSDL);
    }

    const auto init_renderer = [&](render::VideoRendererType type) -> render::VideoRendererPtr {

        render::VideoRendererPtr renderer = render::RendererFactory::create(type);

        if (!renderer) {

            return nullptr;

        }

        if (!renderer->init(renderer_config)) {

            Logger::warning(std::string("Idle renderer init failed: ") + rendererTypeName(type));

            return nullptr;

        }

        input::IPlaybackInputSource* input_source =
            dynamic_cast<input::IPlaybackInputSource*>(renderer.get());
        if (!input_source) {
            Logger::warning(std::string("Idle renderer missing input source interface: ") + rendererTypeName(type));
            renderer->close();
            return nullptr;
        }
        input_source->setHotkeyManager(hotkey_manager);

        renderer->clear();

        renderer->present();

        Logger::info(std::string("Idle renderer initialized: ") + rendererTypeName(type));

        return renderer;

    };



    render::VideoRendererPtr renderer;
    for (render::VideoRendererType type : renderer_candidates) {
        renderer = init_renderer(type);
        if (renderer) {
            break;
        }
        Logger::warning(std::string("Idle renderer candidate failed: ") + rendererTypeName(type));
    }



    return renderer;

}

bool waitForIdlePlaylistSelection(const input::HotkeyManager& hotkey_manager,

                                 playlist::PlaylistManager& playlist_manager,

                                 std::string& normalized_input,

                                 bool& quit_requested) {

    normalized_input.clear();



    render::VideoRendererPtr idle_renderer = createIdleRendererWindow(hotkey_manager);

    if (!idle_renderer) {

        Logger::error("Failed to create idle renderer window");

        quit_requested = true;

        return false;

    }



    Logger::info("Idle window ready; waiting for drag-and-drop media input");
    input::IPlaybackInputSource* idle_input_source =
        dynamic_cast<input::IPlaybackInputSource*>(idle_renderer.get());
    if (!idle_input_source) {
        Logger::error("Idle renderer does not expose playback input source interface");
        quit_requested = true;
        idle_renderer->close();
        return false;
    }



    while (!quit_requested && playlist_manager.empty()) {

        idle_input_source->handleEvents();



        if (idle_input_source->shouldQuit()) {

            quit_requested = true;

            break;

        }



        std::string dropped_path;

        if (idle_input_source->consumeOpenFileRequest(dropped_path)) {

            std::string error;

            if (!tryResolveInteractiveMediaInput(dropped_path, playlist_manager, normalized_input, error)) {

                Logger::warning("Ignoring dropped input: " + dropped_path + " reason=" + error);

                continue;

            }



            Logger::info("Accepted dropped media: " + normalized_input);

            break;

        }



        std::this_thread::sleep_for(std::chrono::milliseconds(10));

    }



    idle_renderer->close();

    return !quit_requested && !playlist_manager.empty();

}

std::string detectAutoSubtitlePath(const std::string& media_uri) {

    if (media_uri.find("://") != std::string::npos) {

        return {};

    }



    std::error_code ec;

    std::filesystem::path candidate(media_uri);

    for (const char* extension : {".ass", ".ssa", ".srt"}) {

        std::filesystem::path subtitle_candidate = candidate;

        subtitle_candidate.replace_extension(extension);

        if (std::filesystem::exists(subtitle_candidate, ec) && !ec &&

            std::filesystem::is_regular_file(subtitle_candidate, ec) && !ec) {

            return subtitle_candidate.string();

        }

        ec.clear();

    }

    return {};

}



std::string hotkeySettingKey(input::PlayerAction action) {

    return "hotkey." + input::HotkeyManager::actionConfigKey(action);

}



/// 从配置文件恢复热键绑定，并在冲突或非法配置时回退默认值。

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

        settings_manager.setString("subtitle.preferred_languages", "none");

        settings_manager.setString("subtitle.forced_policy", "auto");

        settings_manager.setString("subtitle.sdh_policy", "auto");

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

    if (const auto preferred_languages = settings_manager.getString("subtitle.preferred_languages")) {

        std::vector<std::string> parsed_languages;
        if (parsePreferredSubtitleLanguages(*preferred_languages, parsed_languages)) {
            settings.embedded_subtitle_selection_policy.preferred_languages = std::move(parsed_languages);
        }

    }

    if (const auto forced_policy = settings_manager.getString("subtitle.forced_policy")) {

        subtitle::EmbeddedSubtitleForcedPolicy parsed_forced_policy{};
        if (parseEmbeddedSubtitleForcedPolicy(*forced_policy, parsed_forced_policy)) {
            settings.embedded_subtitle_selection_policy.forced_policy = parsed_forced_policy;
        }

    }

    if (const auto sdh_policy = settings_manager.getString("subtitle.sdh_policy")) {

        subtitle::EmbeddedSubtitleSdhPolicy parsed_sdh_policy{};
        if (parseEmbeddedSubtitleSdhPolicy(*sdh_policy, parsed_sdh_policy)) {
            settings.embedded_subtitle_selection_policy.sdh_policy = parsed_sdh_policy;
        }

    }

    loadHotkeySettings(settings_manager, settings.hotkey_manager);



    settings_manager.setInt("player.volume_percent", static_cast<int>(std::lround(settings.volume * 100.0f)));

    settings_manager.setDouble("player.playback_speed", settings.playback_speed);

    settings_manager.setInt("player.audio_delay_ms", static_cast<int>(std::lround(settings.audio_delay_seconds * 1000.0)));

    settings_manager.setInt("player.subtitle_delay_ms", static_cast<int>(std::lround(settings.subtitle_delay_seconds * 1000.0)));

    settings_manager.setBool("decoder.prefer_hardware_decode", settings.prefer_hardware_decode);

    settings_manager.setBool("player.resume_last_playlist", settings.resume_last_playlist);

    settings_manager.setInt("player.last_playlist_index", settings.last_playlist_index);

    settings_manager.setString("subtitle.preferred_languages",
                               preferredSubtitleLanguagesToConfigValue(
                                   settings.embedded_subtitle_selection_policy.preferred_languages));

    settings_manager.setString("subtitle.forced_policy",
                               embeddedSubtitleForcedPolicyName(settings.embedded_subtitle_selection_policy.forced_policy));

    settings_manager.setString("subtitle.sdh_policy",
                               embeddedSubtitleSdhPolicyName(settings.embedded_subtitle_selection_policy.sdh_policy));

    settings_manager.setBool("hotkey.restore_defaults", false);

    syncHotkeySettings(settings_manager, settings.hotkey_manager);



    return settings;

}



/// 将当前播放偏好和热键配置写回设置文件。

void saveAppSettings(config::SettingsManager& settings_manager,

                     const std::string& settings_path,

                     float volume,

                     double playback_speed,

                     double audio_delay_seconds,

                     double subtitle_delay_seconds,

                     bool prefer_hardware_decode,

                     bool resume_last_playlist,

                     int last_playlist_index,

                     const subtitle::EmbeddedSubtitleSelectionPolicy& embedded_subtitle_selection_policy,

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

    settings_manager.setString("subtitle.preferred_languages",
                               preferredSubtitleLanguagesToConfigValue(
                                   embedded_subtitle_selection_policy.preferred_languages));

    settings_manager.setString("subtitle.forced_policy",
                               embeddedSubtitleForcedPolicyName(embedded_subtitle_selection_policy.forced_policy));

    settings_manager.setString("subtitle.sdh_policy",
                               embeddedSubtitleSdhPolicyName(embedded_subtitle_selection_policy.sdh_policy));

    syncHotkeySettings(settings_manager, hotkey_manager);



    if (!settings_manager.saveIni(settings_path)) {

        LOG_WARNING("Failed to save settings file: " << settings_path);

    }

}



/// 验证播放器设置的保存与重新加载流程。

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
    const std::vector<std::string> expected_subtitle_languages = {"zh-cn", "en"};
    constexpr subtitle::EmbeddedSubtitleForcedPolicy expected_subtitle_forced_policy =
        subtitle::EmbeddedSubtitleForcedPolicy::PreferForced;
    constexpr subtitle::EmbeddedSubtitleSdhPolicy expected_subtitle_sdh_policy =
        subtitle::EmbeddedSubtitleSdhPolicy::AvoidSdh;



    config::SettingsManager writer_manager;

    AppSettings initial = loadAppSettings(writer_manager, check_path_text);

    input::HotkeyManager expected_hotkeys = initial.hotkey_manager;

    expected_hotkeys.bind(input::PlayerAction::ToggleSubtitle, expected_subtitle_key);
    subtitle::EmbeddedSubtitleSelectionPolicy expected_subtitle_policy{};
    expected_subtitle_policy.preferred_languages = expected_subtitle_languages;
    expected_subtitle_policy.forced_policy = expected_subtitle_forced_policy;
    expected_subtitle_policy.sdh_policy = expected_subtitle_sdh_policy;

    saveAppSettings(writer_manager,

                    check_path_text,

                    expected_volume,

                    expected_speed,

                    expected_audio_delay,

                    expected_subtitle_delay,

                    expected_prefer_hardware_decode,

                    expected_resume_last_playlist,

                    expected_playlist_index,

                    expected_subtitle_policy,

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
    const bool subtitle_languages_ok =
        restored.embedded_subtitle_selection_policy.preferred_languages == expected_subtitle_languages;
    const bool subtitle_forced_policy_ok =
        restored.embedded_subtitle_selection_policy.forced_policy == expected_subtitle_forced_policy;
    const bool subtitle_sdh_policy_ok =
        restored.embedded_subtitle_selection_policy.sdh_policy == expected_subtitle_sdh_policy;



    const auto subtitle_key = restored.hotkey_manager.keyForAction(input::PlayerAction::ToggleSubtitle);

    const bool hotkey_ok = subtitle_key.has_value() && *subtitle_key == expected_subtitle_key;



    const bool result = volume_ok && speed_ok && audio_delay_ok && subtitle_delay_ok &&
                        decode_pref_ok && resume_ok && index_ok &&
                        subtitle_languages_ok && subtitle_forced_policy_ok && subtitle_sdh_policy_ok &&
                        hotkey_ok;



    std::cout << "settings-persistence-check.path=" << check_path_text << std::endl;

    std::cout << "settings-persistence-check.volume_ok=" << (volume_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.speed_ok=" << (speed_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.audio_delay_ok=" << (audio_delay_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.subtitle_delay_ok=" << (subtitle_delay_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.decode_pref_ok=" << (decode_pref_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.resume_ok=" << (resume_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.index_ok=" << (index_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.subtitle_languages_ok="
              << (subtitle_languages_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.subtitle_forced_policy_ok="
              << (subtitle_forced_policy_ok ? "true" : "false") << std::endl;

    std::cout << "settings-persistence-check.subtitle_sdh_policy_ok="
              << (subtitle_sdh_policy_ok ? "true" : "false") << std::endl;

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



/// 验证渲染后端初始化失败时是否能正确回退到可用路径。

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



/// Windows 渲染后端会话检查结果。

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



/// 在 Windows 上验证指定渲染后端模式能否稳定启动会话，并直接以结果码退出子进程。

[[noreturn]] void runWindowsBackendSessionCheckAndExit(const std::string& media_file, const std::string& mode) {

    auto flush_and_exit = [](int code) -> void {

        std::cout.flush();

        std::cerr.flush();

        std::fflush(nullptr);

#if defined(_WIN32)

        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));

#else

        std::_Exit(code);

#endif

        std::abort();

    };



    const bool hard_mode = mode == "hard";

    const bool soft_mode = mode == "soft";

    if (!hard_mode && !soft_mode) {

        std::cout << "windows-backend-session-check.mode=" << mode << std::endl;

        std::cout << "windows-backend-session-check.result=FAIL" << std::endl;

        flush_and_exit(2);

    }



    std::error_code ec;

    const std::filesystem::path media_path(media_file);

    if (!std::filesystem::exists(media_path, ec) || ec ||

        !std::filesystem::is_regular_file(media_path, ec) || ec) {

        std::cout << "windows-backend-session-check.path=" << media_file << std::endl;

        std::cout << "windows-backend-session-check.mode=" << mode << std::endl;

        std::cout << "windows-backend-session-check.result=FAIL" << std::endl;

        flush_and_exit(2);

    }



    const bool prefer_hardware_decode = hard_mode;

    const std::string expected_decoder = prefer_hardware_decode ? "D3D11VA" : "Software";

    ScopedEnvOverride force_d3d11_driver_hint("MVP_D3D11_DRIVER_HINT", "direct3d11");



    BackendSessionResult session{};

    VideoPlayer player;

    player.setPreferHardwareDecode(prefer_hardware_decode);

    session.open_ok = player.open(media_file);

    session.renderer_backend = player.videoRendererBackendName();

    session.decoder_backend = player.videoDecoderBackendName();



    if (session.open_ok) {

        player.play();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);

        while (std::chrono::steady_clock::now() < deadline &&

               (player.isPlaying() || player.isPaused())) {

            session.entered_playback_loop = true;

            player.pumpEvents();

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



    flush_and_exit(session.mode_ok ? 0 : 2);

}



/// Windows 播放回归入口；串联多种后端模式检查。

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



/// 验证章节信息读取与前后章节跳转行为。

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



/// 验证 A-B Repeat 的起止点设置、循环与清除行为。

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



/// 验证暂停态下逐帧前进/后退逻辑。

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



/// 验证音频延迟和字幕延迟调节的状态持有与恢复行为。

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

    auto parser = subtitle::createParserForPath(subtitle_file);

    const bool subtitle_parsed = parser && parser->parseFile(subtitle_file);

    std::vector<subtitle::SubtitleItem> items = subtitle_parsed ? parser->items() : std::vector<subtitle::SubtitleItem>{};

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



/// 验证按比例数字跳转与位置同步行为。

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



uint64_t counterDelta(uint64_t after, uint64_t before) {

    return after >= before ? (after - before) : 0;

}



uint64_t staleDropTotal(const core::DiagnosticsSnapshot& diag) {

    return diag.stale_video_packets_dropped +

           diag.stale_audio_packets_dropped +

           diag.stale_video_frames_dropped +

           diag.stale_audio_frames_dropped +

           diag.stale_audio_submit_frames_dropped +

           diag.stale_render_frames_dropped +

           diag.stale_paused_render_frames_dropped;

}



uint64_t illegalTransitionTotal(const core::DiagnosticsSnapshot& diag) {

    return diag.illegal_session_transitions +

           diag.illegal_run_transitions +

           diag.illegal_pipeline_transitions;

}



bool runSeekBurstSerialCheck(const std::string& media_file, int seek_count = 6) {

    std::error_code ec;

    const std::filesystem::path media_path(media_file);

    if (!std::filesystem::exists(media_path, ec) || ec ||

        !std::filesystem::is_regular_file(media_path, ec) || ec ||

        seek_count < 2 || seek_count > 24) {

        std::cout << "seek-burst-serial-check.path=" << media_file << std::endl;

        std::cout << "seek-burst-serial-check.seek_count=" << seek_count << std::endl;

        std::cout << "seek-burst-serial-check.result=FAIL" << std::endl;

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



    const std::vector<double> seek_ratios{0.12, 0.58, 0.28, 0.66, 0.22, 0.6, 0.35, 0.7};
    constexpr double kSeekToleranceSeconds = 1.2;
    constexpr uint64_t kSettleStaleDeltaMax = 2;



    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    double duration = 0.0;
    int seek_ops_executed = 0;
    int seek_position_ok_count = 0;
    int serial_advance_count = 0;
    double last_seek_target = 0.0;
    double last_seek_position = 0.0;

    uint64_t boundary_stale_total_delta = 0;
    uint64_t boundary_stale_render_delta = 0;
    uint64_t boundary_stale_audio_submit_delta = 0;
    uint64_t fail_session_events = 0;

    core::DiagnosticsSnapshot baseline_diag{};
    core::DiagnosticsSnapshot last_seek_diag{};
    core::DiagnosticsSnapshot settled_diag{};



    if (open_ok) {

        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(900));
        duration = player.getDuration();
        baseline_diag = player.getDiagnosticsSnapshot();
        last_seek_diag = baseline_diag;
        settled_diag = baseline_diag;

        if (entered_playback_loop && duration > 2.0) {

            for (int i = 0; i < seek_count; ++i) {

                const core::DiagnosticsSnapshot before_seek = player.getDiagnosticsSnapshot();
                const double ratio = seek_ratios[static_cast<size_t>(i) % seek_ratios.size()];
                const double target = std::clamp(duration * ratio, 0.0, std::max(0.0, duration - 0.05));
                player.seek(target);
                pump_for(player, std::chrono::milliseconds(260));
                const core::DiagnosticsSnapshot after_seek = player.getDiagnosticsSnapshot();

                ++seek_ops_executed;
                last_seek_target = target;
                last_seek_position = player.getCurrentTime();

                if (std::abs(last_seek_position - target) <= kSeekToleranceSeconds) {
                    ++seek_position_ok_count;
                }
                if (after_seek.timeline_serial > before_seek.timeline_serial) {
                    ++serial_advance_count;
                }

                boundary_stale_total_delta += counterDelta(staleDropTotal(after_seek), staleDropTotal(before_seek));
                boundary_stale_render_delta += counterDelta(after_seek.stale_render_frames_dropped,
                                                           before_seek.stale_render_frames_dropped);
                boundary_stale_audio_submit_delta += counterDelta(after_seek.stale_audio_submit_frames_dropped,
                                                                 before_seek.stale_audio_submit_frames_dropped);
                fail_session_events += counterDelta(after_seek.runtime_failure_fail_sessions,
                                                    before_seek.runtime_failure_fail_sessions);
                last_seek_diag = after_seek;
            }

            pump_for(player, std::chrono::milliseconds(700));
            settled_diag = player.getDiagnosticsSnapshot();
        }

        player.stop();
        player.close();
    }



    const uint64_t settled_stale_total_delta =
        counterDelta(staleDropTotal(settled_diag), staleDropTotal(last_seek_diag));
    const uint64_t settled_stale_render_delta =
        counterDelta(settled_diag.stale_render_frames_dropped, last_seek_diag.stale_render_frames_dropped);
    const uint64_t settled_stale_audio_submit_delta =
        counterDelta(settled_diag.stale_audio_submit_frames_dropped, last_seek_diag.stale_audio_submit_frames_dropped);
    const uint64_t illegal_transition_total = illegalTransitionTotal(settled_diag);
    const bool seek_position_ok = seek_ops_executed > 0 && (seek_position_ok_count + 1) >= seek_ops_executed;
    const bool serial_advance_ok = seek_ops_executed > 0 && serial_advance_count == seek_ops_executed;
    const bool stale_settle_ok = settled_stale_total_delta <= kSettleStaleDeltaMax;
    const bool old_output_settle_ok = settled_stale_render_delta == 0 && settled_stale_audio_submit_delta == 0;
    const bool fail_session_transition_ok =
        settled_diag.runtime_failure_fail_sessions == 0 || illegal_transition_total == 0;
    const bool result = open_ok &&
                        entered_playback_loop &&
                        seek_ops_executed == seek_count &&
                        seek_position_ok &&
                        serial_advance_ok &&
                        stale_settle_ok &&
                        old_output_settle_ok &&
                        fail_session_transition_ok &&
                        illegal_transition_total == 0;



    std::cout << "seek-burst-serial-check.path=" << media_file << std::endl;
    std::cout << "seek-burst-serial-check.seek_count=" << seek_count << std::endl;
    std::cout << "seek-burst-serial-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "seek-burst-serial-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "seek-burst-serial-check.duration=" << duration << std::endl;
    std::cout << "seek-burst-serial-check.seek_ops_executed=" << seek_ops_executed << std::endl;
    std::cout << "seek-burst-serial-check.seek_position_ok_count=" << seek_position_ok_count << std::endl;
    std::cout << "seek-burst-serial-check.serial_advance_count=" << serial_advance_count << std::endl;
    std::cout << "seek-burst-serial-check.last_seek_target=" << last_seek_target << std::endl;
    std::cout << "seek-burst-serial-check.last_seek_position=" << last_seek_position << std::endl;
    std::cout << "seek-burst-serial-check.baseline_timeline_serial=" << baseline_diag.timeline_serial << std::endl;
    std::cout << "seek-burst-serial-check.last_seek_timeline_serial=" << last_seek_diag.timeline_serial << std::endl;
    std::cout << "seek-burst-serial-check.settled_timeline_serial=" << settled_diag.timeline_serial << std::endl;
    std::cout << "seek-burst-serial-check.boundary_stale_total_delta=" << boundary_stale_total_delta << std::endl;
    std::cout << "seek-burst-serial-check.boundary_stale_render_delta=" << boundary_stale_render_delta << std::endl;
    std::cout << "seek-burst-serial-check.boundary_stale_audio_submit_delta="
              << boundary_stale_audio_submit_delta << std::endl;
    std::cout << "seek-burst-serial-check.settled_stale_total_delta=" << settled_stale_total_delta << std::endl;
    std::cout << "seek-burst-serial-check.settled_stale_render_delta=" << settled_stale_render_delta << std::endl;
    std::cout << "seek-burst-serial-check.settled_stale_audio_submit_delta="
              << settled_stale_audio_submit_delta << std::endl;
    std::cout << "seek-burst-serial-check.runtime_failure_stop_requests="
              << settled_diag.runtime_failure_stop_requests << std::endl;
    std::cout << "seek-burst-serial-check.runtime_failure_fail_sessions="
              << settled_diag.runtime_failure_fail_sessions << std::endl;
    std::cout << "seek-burst-serial-check.fail_session_events=" << fail_session_events << std::endl;
    std::cout << "seek-burst-serial-check.illegal_session_transitions="
              << settled_diag.illegal_session_transitions << std::endl;
    std::cout << "seek-burst-serial-check.illegal_run_transitions="
              << settled_diag.illegal_run_transitions << std::endl;
    std::cout << "seek-burst-serial-check.illegal_pipeline_transitions="
              << settled_diag.illegal_pipeline_transitions << std::endl;
    std::cout << "seek-burst-serial-check.illegal_transition_total=" << illegal_transition_total << std::endl;
    std::cout << "seek-burst-serial-check.seek_position_ok=" << (seek_position_ok ? "true" : "false") << std::endl;
    std::cout << "seek-burst-serial-check.serial_advance_ok=" << (serial_advance_ok ? "true" : "false")
              << std::endl;
    std::cout << "seek-burst-serial-check.stale_settle_ok=" << (stale_settle_ok ? "true" : "false") << std::endl;
    std::cout << "seek-burst-serial-check.old_output_settle_ok=" << (old_output_settle_ok ? "true" : "false")
              << std::endl;
    std::cout << "seek-burst-serial-check.fail_session_transition_ok="
              << (fail_session_transition_ok ? "true" : "false") << std::endl;
    std::cout << "seek-burst-serial-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



bool runPausedSeekSerialCheck(const std::string& media_file, int seek_count = 4) {

    std::error_code ec;

    const std::filesystem::path media_path(media_file);

    if (!std::filesystem::exists(media_path, ec) || ec ||

        !std::filesystem::is_regular_file(media_path, ec) || ec ||

        seek_count < 2 || seek_count > 16) {

        std::cout << "paused-seek-serial-check.path=" << media_file << std::endl;

        std::cout << "paused-seek-serial-check.seek_count=" << seek_count << std::endl;

        std::cout << "paused-seek-serial-check.result=FAIL" << std::endl;

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



    const std::vector<double> seek_ratios{0.2, 0.72, 0.35, 0.62, 0.5, 0.1};
    constexpr double kSeekToleranceSeconds = 0.95;
    constexpr uint64_t kSettleStaleDeltaMax = 1;



    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    bool pause_ok = false;
    bool paused_retained = true;
    double duration = 0.0;
    int seek_ops_executed = 0;
    int seek_position_ok_count = 0;
    int serial_advance_count = 0;
    double last_seek_target = 0.0;
    double last_seek_position = 0.0;

    uint64_t boundary_stale_total_delta = 0;
    uint64_t boundary_stale_paused_render_delta = 0;
    uint64_t boundary_stale_audio_submit_delta = 0;

    core::DiagnosticsSnapshot baseline_diag{};
    core::DiagnosticsSnapshot last_seek_diag{};
    core::DiagnosticsSnapshot settled_diag{};



    if (open_ok) {

        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(900));
        duration = player.getDuration();
        baseline_diag = player.getDiagnosticsSnapshot();
        last_seek_diag = baseline_diag;
        settled_diag = baseline_diag;

        if (entered_playback_loop && duration > 2.0) {

            player.pause();
            pause_ok = player.isPaused();
            paused_retained = pause_ok;
            pump_for(player, std::chrono::milliseconds(160));

            for (int i = 0; i < seek_count; ++i) {

                const core::DiagnosticsSnapshot before_seek = player.getDiagnosticsSnapshot();
                const double ratio = seek_ratios[static_cast<size_t>(i) % seek_ratios.size()];
                const double target = std::clamp(duration * ratio, 0.0, std::max(0.0, duration - 0.05));
                player.seek(target);
                pump_for(player, std::chrono::milliseconds(240));
                const core::DiagnosticsSnapshot after_seek = player.getDiagnosticsSnapshot();

                ++seek_ops_executed;
                paused_retained = paused_retained && player.isPaused();
                last_seek_target = target;
                last_seek_position = player.getCurrentTime();

                if (std::abs(last_seek_position - target) <= kSeekToleranceSeconds) {
                    ++seek_position_ok_count;
                }
                if (after_seek.timeline_serial > before_seek.timeline_serial) {
                    ++serial_advance_count;
                }

                boundary_stale_total_delta += counterDelta(staleDropTotal(after_seek), staleDropTotal(before_seek));
                boundary_stale_paused_render_delta += counterDelta(after_seek.stale_paused_render_frames_dropped,
                                                                  before_seek.stale_paused_render_frames_dropped);
                boundary_stale_audio_submit_delta += counterDelta(after_seek.stale_audio_submit_frames_dropped,
                                                                 before_seek.stale_audio_submit_frames_dropped);
                last_seek_diag = after_seek;
            }

            pump_for(player, std::chrono::milliseconds(500));
            settled_diag = player.getDiagnosticsSnapshot();
        }

        player.stop();
        player.close();
    }



    const uint64_t settled_stale_total_delta =
        counterDelta(staleDropTotal(settled_diag), staleDropTotal(last_seek_diag));
    const uint64_t settled_stale_paused_render_delta =
        counterDelta(settled_diag.stale_paused_render_frames_dropped, last_seek_diag.stale_paused_render_frames_dropped);
    const uint64_t settled_stale_audio_submit_delta =
        counterDelta(settled_diag.stale_audio_submit_frames_dropped, last_seek_diag.stale_audio_submit_frames_dropped);
    const uint64_t illegal_transition_total = illegalTransitionTotal(settled_diag);
    const bool seek_position_ok = seek_ops_executed > 0 && seek_position_ok_count == seek_ops_executed;
    const bool serial_advance_ok = seek_ops_executed > 0 && serial_advance_count == seek_ops_executed;
    const bool stale_settle_ok = settled_stale_total_delta <= kSettleStaleDeltaMax;
    const bool old_output_settle_ok = settled_stale_audio_submit_delta == 0;
    const bool fail_session_transition_ok =
        settled_diag.runtime_failure_fail_sessions == 0 || illegal_transition_total == 0;
    const bool result = open_ok &&
                        entered_playback_loop &&
                        pause_ok &&
                        paused_retained &&
                        seek_ops_executed == seek_count &&
                        seek_position_ok &&
                        serial_advance_ok &&
                        stale_settle_ok &&
                        old_output_settle_ok &&
                        fail_session_transition_ok &&
                        illegal_transition_total == 0;



    std::cout << "paused-seek-serial-check.path=" << media_file << std::endl;
    std::cout << "paused-seek-serial-check.seek_count=" << seek_count << std::endl;
    std::cout << "paused-seek-serial-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "paused-seek-serial-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "paused-seek-serial-check.pause_ok=" << (pause_ok ? "true" : "false") << std::endl;
    std::cout << "paused-seek-serial-check.paused_retained=" << (paused_retained ? "true" : "false") << std::endl;
    std::cout << "paused-seek-serial-check.duration=" << duration << std::endl;
    std::cout << "paused-seek-serial-check.seek_ops_executed=" << seek_ops_executed << std::endl;
    std::cout << "paused-seek-serial-check.seek_position_ok_count=" << seek_position_ok_count << std::endl;
    std::cout << "paused-seek-serial-check.serial_advance_count=" << serial_advance_count << std::endl;
    std::cout << "paused-seek-serial-check.last_seek_target=" << last_seek_target << std::endl;
    std::cout << "paused-seek-serial-check.last_seek_position=" << last_seek_position << std::endl;
    std::cout << "paused-seek-serial-check.baseline_timeline_serial=" << baseline_diag.timeline_serial << std::endl;
    std::cout << "paused-seek-serial-check.last_seek_timeline_serial=" << last_seek_diag.timeline_serial << std::endl;
    std::cout << "paused-seek-serial-check.settled_timeline_serial=" << settled_diag.timeline_serial << std::endl;
    std::cout << "paused-seek-serial-check.boundary_stale_total_delta=" << boundary_stale_total_delta << std::endl;
    std::cout << "paused-seek-serial-check.boundary_stale_paused_render_delta="
              << boundary_stale_paused_render_delta << std::endl;
    std::cout << "paused-seek-serial-check.boundary_stale_audio_submit_delta="
              << boundary_stale_audio_submit_delta << std::endl;
    std::cout << "paused-seek-serial-check.settled_stale_total_delta=" << settled_stale_total_delta << std::endl;
    std::cout << "paused-seek-serial-check.settled_stale_paused_render_delta="
              << settled_stale_paused_render_delta << std::endl;
    std::cout << "paused-seek-serial-check.settled_stale_audio_submit_delta="
              << settled_stale_audio_submit_delta << std::endl;
    std::cout << "paused-seek-serial-check.runtime_failure_stop_requests="
              << settled_diag.runtime_failure_stop_requests << std::endl;
    std::cout << "paused-seek-serial-check.runtime_failure_fail_sessions="
              << settled_diag.runtime_failure_fail_sessions << std::endl;
    std::cout << "paused-seek-serial-check.illegal_session_transitions="
              << settled_diag.illegal_session_transitions << std::endl;
    std::cout << "paused-seek-serial-check.illegal_run_transitions="
              << settled_diag.illegal_run_transitions << std::endl;
    std::cout << "paused-seek-serial-check.illegal_pipeline_transitions="
              << settled_diag.illegal_pipeline_transitions << std::endl;
    std::cout << "paused-seek-serial-check.illegal_transition_total=" << illegal_transition_total << std::endl;
    std::cout << "paused-seek-serial-check.seek_position_ok=" << (seek_position_ok ? "true" : "false")
              << std::endl;
    std::cout << "paused-seek-serial-check.serial_advance_ok=" << (serial_advance_ok ? "true" : "false")
              << std::endl;
    std::cout << "paused-seek-serial-check.stale_settle_ok=" << (stale_settle_ok ? "true" : "false")
              << std::endl;
    std::cout << "paused-seek-serial-check.old_output_settle_ok=" << (old_output_settle_ok ? "true" : "false")
              << std::endl;
    std::cout << "paused-seek-serial-check.fail_session_transition_ok="
              << (fail_session_transition_ok ? "true" : "false") << std::endl;
    std::cout << "paused-seek-serial-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



bool runCloseReopenSerialCheck(const std::string& media_file, int sample_ms = 900) {

    std::error_code ec;

    const std::filesystem::path media_path(media_file);

    if (!std::filesystem::exists(media_path, ec) || ec ||

        !std::filesystem::is_regular_file(media_path, ec) || ec ||

        sample_ms < 400) {

        std::cout << "close-reopen-serial-check.path=" << media_file << std::endl;

        std::cout << "close-reopen-serial-check.sample_ms=" << sample_ms << std::endl;

        std::cout << "close-reopen-serial-check.result=FAIL" << std::endl;

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
    const bool first_open_ok = player.open(media_file);

    bool first_entered_playback_loop = false;
    bool first_close_ok = false;
    bool second_open_ok = false;
    bool second_entered_playback_loop = false;
    bool second_rendered_ok = false;
    double duration = 0.0;

    core::DiagnosticsSnapshot first_session_diag{};
    core::DiagnosticsSnapshot second_session_diag{};



    if (first_open_ok) {

        player.play();
        first_entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        duration = player.getDuration();

        if (first_entered_playback_loop && duration > 2.0) {
            player.seek(duration * 0.55);
            pump_for(player, std::chrono::milliseconds(220));
            player.seek(duration * 0.25);
            pump_for(player, std::chrono::milliseconds(220));
        }

        first_session_diag = player.getDiagnosticsSnapshot();
        player.stop();
        player.close();
        first_close_ok = true;

        second_open_ok = player.open(media_file);
        if (second_open_ok) {
            player.play();
            second_entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
            second_session_diag = player.getDiagnosticsSnapshot();
            second_rendered_ok = second_session_diag.render_frames > 0;
            player.stop();
            player.close();
        }
    }



    const uint64_t first_session_stale_total = staleDropTotal(first_session_diag);
    const uint64_t second_session_stale_total = staleDropTotal(second_session_diag);
    const uint64_t first_illegal_transition_total = illegalTransitionTotal(first_session_diag);
    const uint64_t second_illegal_transition_total = illegalTransitionTotal(second_session_diag);
    const bool reopen_stale_reset_ok = second_session_stale_total <= 2 &&
                                       second_session_diag.stale_render_frames_dropped == 0 &&
                                       second_session_diag.stale_audio_submit_frames_dropped == 0;
    const bool reopen_failure_reset_ok = second_session_diag.runtime_failure_stop_requests == 0 &&
                                         second_session_diag.runtime_failure_fail_sessions == 0;
    const bool fail_session_transition_ok =
        second_session_diag.runtime_failure_fail_sessions == 0 || second_illegal_transition_total == 0;
    const bool result = first_open_ok &&
                        first_entered_playback_loop &&
                        first_close_ok &&
                        second_open_ok &&
                        second_entered_playback_loop &&
                        second_rendered_ok &&
                        reopen_stale_reset_ok &&
                        reopen_failure_reset_ok &&
                        fail_session_transition_ok &&
                        first_illegal_transition_total == 0 &&
                        second_illegal_transition_total == 0;



    std::cout << "close-reopen-serial-check.path=" << media_file << std::endl;
    std::cout << "close-reopen-serial-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "close-reopen-serial-check.first_open_ok=" << (first_open_ok ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.first_entered_playback_loop="
              << (first_entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.first_close_ok=" << (first_close_ok ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.second_open_ok=" << (second_open_ok ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.second_entered_playback_loop="
              << (second_entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.second_rendered_ok=" << (second_rendered_ok ? "true" : "false")
              << std::endl;
    std::cout << "close-reopen-serial-check.duration=" << duration << std::endl;
    std::cout << "close-reopen-serial-check.first_session_timeline_serial="
              << first_session_diag.timeline_serial << std::endl;
    std::cout << "close-reopen-serial-check.second_session_timeline_serial="
              << second_session_diag.timeline_serial << std::endl;
    std::cout << "close-reopen-serial-check.first_session_stale_total=" << first_session_stale_total << std::endl;
    std::cout << "close-reopen-serial-check.first_session_illegal_transition_total="
              << first_illegal_transition_total << std::endl;
    std::cout << "close-reopen-serial-check.second_session_stale_total=" << second_session_stale_total << std::endl;
    std::cout << "close-reopen-serial-check.second_session_stale_render_frames_dropped="
              << second_session_diag.stale_render_frames_dropped << std::endl;
    std::cout << "close-reopen-serial-check.second_session_stale_audio_submit_frames_dropped="
              << second_session_diag.stale_audio_submit_frames_dropped << std::endl;
    std::cout << "close-reopen-serial-check.second_session_runtime_failure_stop_requests="
              << second_session_diag.runtime_failure_stop_requests << std::endl;
    std::cout << "close-reopen-serial-check.second_session_runtime_failure_fail_sessions="
              << second_session_diag.runtime_failure_fail_sessions << std::endl;
    std::cout << "close-reopen-serial-check.second_session_illegal_session_transitions="
              << second_session_diag.illegal_session_transitions << std::endl;
    std::cout << "close-reopen-serial-check.second_session_illegal_run_transitions="
              << second_session_diag.illegal_run_transitions << std::endl;
    std::cout << "close-reopen-serial-check.second_session_illegal_pipeline_transitions="
              << second_session_diag.illegal_pipeline_transitions << std::endl;
    std::cout << "close-reopen-serial-check.second_session_illegal_transition_total="
              << second_illegal_transition_total << std::endl;
    std::cout << "close-reopen-serial-check.reopen_stale_reset_ok=" << (reopen_stale_reset_ok ? "true" : "false")
              << std::endl;
    std::cout << "close-reopen-serial-check.reopen_failure_reset_ok=" << (reopen_failure_reset_ok ? "true" : "false")
              << std::endl;
    std::cout << "close-reopen-serial-check.fail_session_transition_ok="
              << (fail_session_transition_ok ? "true" : "false") << std::endl;
    std::cout << "close-reopen-serial-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 在短播放窗口内采集诊断计数，检查性能日志链路是否工作。

bool runSerialFailSessionRegressionCheck(const std::string& media_file,
                                         int seek_burst_count = 6,
                                         int paused_seek_count = 4,
                                         int close_reopen_sample_ms = 900) {
    const bool seek_burst_ok = runSeekBurstSerialCheck(media_file, seek_burst_count);
    const bool paused_seek_ok = runPausedSeekSerialCheck(media_file, paused_seek_count);
    const bool close_reopen_ok = runCloseReopenSerialCheck(media_file, close_reopen_sample_ms);
    const int pass_count = (seek_burst_ok ? 1 : 0) + (paused_seek_ok ? 1 : 0) + (close_reopen_ok ? 1 : 0);
    const bool result = seek_burst_ok && paused_seek_ok && close_reopen_ok;

    std::cout << "serial-failsession-regression-check.path=" << media_file << std::endl;
    std::cout << "serial-failsession-regression-check.seek_burst_count=" << seek_burst_count << std::endl;
    std::cout << "serial-failsession-regression-check.paused_seek_count=" << paused_seek_count << std::endl;
    std::cout << "serial-failsession-regression-check.close_reopen_sample_ms=" << close_reopen_sample_ms
              << std::endl;
    std::cout << "serial-failsession-regression-check.seek_burst_ok=" << (seek_burst_ok ? "true" : "false")
              << std::endl;
    std::cout << "serial-failsession-regression-check.paused_seek_ok=" << (paused_seek_ok ? "true" : "false")
              << std::endl;
    std::cout << "serial-failsession-regression-check.close_reopen_ok=" << (close_reopen_ok ? "true" : "false")
              << std::endl;
    std::cout << "serial-failsession-regression-check.pass_count=" << pass_count << std::endl;
    std::cout << "serial-failsession-regression-check.total_count=3" << std::endl;
    std::cout << "serial-failsession-regression-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runForcedFailSessionCheck(const std::string& media_file, int sample_ms = 1500) {

    std::error_code ec;

    const std::filesystem::path media_path(media_file);

    if (!std::filesystem::exists(media_path, ec) || ec ||

        !std::filesystem::is_regular_file(media_path, ec) || ec ||

        sample_ms < 500) {

        std::cout << "forced-failsession-check.path=" << media_file << std::endl;

        std::cout << "forced-failsession-check.sample_ms=" << sample_ms << std::endl;

        std::cout << "forced-failsession-check.result=FAIL" << std::endl;

        return false;

    }



    ScopedEnvOverride force_fail_session("MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE", "1");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);

    bool entered_playback_loop = false;
    bool fail_session_observed = false;
    bool stopped_after_failure = false;
    int pump_iterations = 0;

    core::DiagnosticsSnapshot first_failure_diag{};
    core::DiagnosticsSnapshot settled_diag{};

    if (open_ok) {
        player.play();
        entered_playback_loop = player.isPlaying() || player.isPaused();
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(sample_ms);
        while (std::chrono::steady_clock::now() < deadline) {
            player.pumpEvents();
            ++pump_iterations;

            const bool active = player.isPlaying() || player.isPaused();
            entered_playback_loop = entered_playback_loop || active;
            settled_diag = player.getDiagnosticsSnapshot();

            if (!fail_session_observed && settled_diag.runtime_failure_fail_sessions > 0) {
                fail_session_observed = true;
                first_failure_diag = settled_diag;
                entered_playback_loop = true;
            }

            if (fail_session_observed && !active) {
                stopped_after_failure = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        if (fail_session_observed && !stopped_after_failure) {
            const auto settle_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
            while (std::chrono::steady_clock::now() < settle_deadline) {
                player.pumpEvents();
                ++pump_iterations;
                settled_diag = player.getDiagnosticsSnapshot();
                if (!player.isPlaying() && !player.isPaused()) {
                    stopped_after_failure = true;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        }

        if (!fail_session_observed) {
            first_failure_diag = settled_diag;
        }

        player.stop();
        player.close();
    }

    const uint64_t illegal_transition_total = illegalTransitionTotal(settled_diag);
    const bool fail_session_count_ok = settled_diag.runtime_failure_fail_sessions >= 1;
    const bool fail_stop_count_ok = settled_diag.runtime_failure_stop_requests >= 1;
    const bool fail_session_transition_ok = !fail_session_count_ok || illegal_transition_total == 0;
    const bool result = open_ok &&
                        entered_playback_loop &&
                        fail_session_observed &&
                        fail_session_count_ok &&
                        fail_stop_count_ok &&
                        stopped_after_failure &&
                        fail_session_transition_ok;

    std::cout << "forced-failsession-check.path=" << media_file << std::endl;
    std::cout << "forced-failsession-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "forced-failsession-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "forced-failsession-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false")
              << std::endl;
    std::cout << "forced-failsession-check.fail_session_observed=" << (fail_session_observed ? "true" : "false")
              << std::endl;
    std::cout << "forced-failsession-check.stopped_after_failure=" << (stopped_after_failure ? "true" : "false")
              << std::endl;
    std::cout << "forced-failsession-check.pump_iterations=" << pump_iterations << std::endl;
    std::cout << "forced-failsession-check.first_failure_timeline_serial=" << first_failure_diag.timeline_serial
              << std::endl;
    std::cout << "forced-failsession-check.settled_timeline_serial=" << settled_diag.timeline_serial << std::endl;
    std::cout << "forced-failsession-check.runtime_failure_stop_requests="
              << settled_diag.runtime_failure_stop_requests << std::endl;
    std::cout << "forced-failsession-check.runtime_failure_fail_sessions="
              << settled_diag.runtime_failure_fail_sessions << std::endl;
    std::cout << "forced-failsession-check.illegal_session_transitions="
              << settled_diag.illegal_session_transitions << std::endl;
    std::cout << "forced-failsession-check.illegal_run_transitions="
              << settled_diag.illegal_run_transitions << std::endl;
    std::cout << "forced-failsession-check.illegal_pipeline_transitions="
              << settled_diag.illegal_pipeline_transitions << std::endl;
    std::cout << "forced-failsession-check.illegal_transition_total=" << illegal_transition_total << std::endl;
    std::cout << "forced-failsession-check.fail_session_count_ok=" << (fail_session_count_ok ? "true" : "false")
              << std::endl;
    std::cout << "forced-failsession-check.fail_stop_count_ok=" << (fail_stop_count_ok ? "true" : "false")
              << std::endl;
    std::cout << "forced-failsession-check.fail_session_transition_ok="
              << (fail_session_transition_ok ? "true" : "false") << std::endl;
    std::cout << "forced-failsession-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

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

    double wall_seconds = 0.0;

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



        wall_seconds = std::max(0.001,

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

    const double copy_back_time_ms = static_cast<double>(diag.video_copy_back_time_us_total) / 1000.0;

    const double swscale_time_ms = static_cast<double>(diag.video_swscale_time_us_total) / 1000.0;

    const double copy_back_ratio_percent =

        wall_seconds > 0.0 ? (static_cast<double>(diag.video_copy_back_time_us_total) / (wall_seconds * 1000000.0)) * 100.0

                           : 0.0;

    const double swscale_ratio_percent =

        wall_seconds > 0.0 ? (static_cast<double>(diag.video_swscale_time_us_total) / (wall_seconds * 1000000.0)) * 100.0

                           : 0.0;

    const double copy_back_avg_ms =

        diag.video_copy_back_frames > 0 ? copy_back_time_ms / static_cast<double>(diag.video_copy_back_frames) : 0.0;

    const double swscale_avg_ms =

        diag.video_swscale_frames > 0 ? swscale_time_ms / static_cast<double>(diag.video_swscale_frames) : 0.0;

    const double display_copy_time_ms = static_cast<double>(diag.display_copy_time_us_total) / 1000.0;

    const double display_copy_ratio_percent =

        sample_ms > 0 ? (display_copy_time_ms * 100.0 / static_cast<double>(sample_ms)) : 0.0;

    const double display_copy_avg_ms =

        diag.display_copy_frames > 0 ? display_copy_time_ms / static_cast<double>(diag.display_copy_frames) : 0.0;



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

    std::cout << "performance-log-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "performance-log-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "performance-log-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "performance-log-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "performance-log-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "performance-log-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "performance-log-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "performance-log-check.startup_platform=" << diag.startup_platform << std::endl;

    std::cout << "performance-log-check.startup_renderer_capabilities="
              << diag.startup_renderer_capabilities << std::endl;

    std::cout << "performance-log-check.startup_decoder_capabilities="
              << diag.startup_decoder_capabilities << std::endl;
    std::cout << "performance-log-check.startup_renderer_compiled_set="
              << diag.startup_renderer_compiled_set << std::endl;
    std::cout << "performance-log-check.startup_renderer_runtime_set="
              << diag.startup_renderer_runtime_set << std::endl;
    std::cout << "performance-log-check.startup_decoder_compiled_set="
              << diag.startup_decoder_compiled_set << std::endl;
    std::cout << "performance-log-check.startup_decoder_runtime_set="
              << diag.startup_decoder_runtime_set << std::endl;

    std::cout << "performance-log-check.startup_renderer_candidates="
              << diag.startup_renderer_candidates << std::endl;

    std::cout << "performance-log-check.startup_decoder_candidates="
              << diag.startup_decoder_candidates << std::endl;

    std::cout << "performance-log-check.startup_renderer_plan_reason="
              << diag.startup_renderer_plan_reason << std::endl;

    std::cout << "performance-log-check.startup_decoder_plan_reason="
              << diag.startup_decoder_plan_reason << std::endl;

    std::cout << "performance-log-check.startup_selected_renderer="
              << diag.startup_selected_renderer << std::endl;

    std::cout << "performance-log-check.startup_selected_decoder="
              << diag.startup_selected_decoder << std::endl;

    std::cout << "performance-log-check.startup_renderer_fallback_reason="
              << diag.startup_renderer_fallback_reason << std::endl;

    std::cout << "performance-log-check.startup_decoder_fallback_reason="
              << diag.startup_decoder_fallback_reason << std::endl;

    std::cout << "performance-log-check.scheduler_clock_policy="
              << schedulerClockPolicyName(diag.scheduler_clock_policy) << std::endl;

    std::cout << "performance-log-check.scheduler_audio_master_policy="
              << schedulerAudioMasterPolicyName(diag.scheduler_audio_master_policy) << std::endl;

    std::cout << "performance-log-check.scheduler_ended_policy="
              << schedulerEndedPolicyName(diag.scheduler_ended_policy) << std::endl;

    std::cout << "performance-log-check.scheduler_audio_buffered_seconds="
              << diag.scheduler_audio_buffered_seconds << std::endl;

    std::cout << "performance-log-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "performance-log-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "performance-log-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "performance-log-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "performance-log-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "performance-log-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "performance-log-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "performance-log-check.cpu_avg_percent=" << cpu_avg_percent << std::endl;

    std::cout << "performance-log-check.cpu_logical_cores=" << logical_cores << std::endl;

    std::cout << "performance-log-check.demux_video_packets=" << diag.demux_video_packets << std::endl;

    std::cout << "performance-log-check.demux_audio_packets=" << diag.demux_audio_packets << std::endl;

    std::cout << "performance-log-check.demux_push_retries=" << diag.demux_push_retries << std::endl;

    std::cout << "performance-log-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;

    std::cout << "performance-log-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "performance-log-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

    std::cout << "performance-log-check.decode_video_ok=" << diag.decode_video_ok << std::endl;

    std::cout << "performance-log-check.decode_audio_ok=" << diag.decode_audio_ok << std::endl;

    std::cout << "performance-log-check.stale_video_packets_dropped=" << diag.stale_video_packets_dropped << std::endl;

    std::cout << "performance-log-check.stale_audio_packets_dropped=" << diag.stale_audio_packets_dropped << std::endl;

    std::cout << "performance-log-check.stale_video_frames_dropped=" << diag.stale_video_frames_dropped << std::endl;

    std::cout << "performance-log-check.stale_audio_frames_dropped=" << diag.stale_audio_frames_dropped << std::endl;

    std::cout << "performance-log-check.stale_audio_submit_frames_dropped="
              << diag.stale_audio_submit_frames_dropped << std::endl;

    std::cout << "performance-log-check.stale_render_frames_dropped=" << diag.stale_render_frames_dropped << std::endl;

    std::cout << "performance-log-check.stale_paused_render_frames_dropped="
              << diag.stale_paused_render_frames_dropped << std::endl;

    std::cout << "performance-log-check.video_packet_dequeue_count=" << diag.video_packet_dequeue_count << std::endl;

    std::cout << "performance-log-check.video_send_packet_ok=" << diag.video_send_packet_ok << std::endl;

    std::cout << "performance-log-check.video_send_packet_last_ret=" << diag.video_send_packet_last_ret << std::endl;

    std::cout << "performance-log-check.decode_video_send_eagain=" << diag.decode_video_send_eagain << std::endl;

    std::cout << "performance-log-check.decode_audio_send_eagain=" << diag.decode_audio_send_eagain << std::endl;

    std::cout << "performance-log-check.video_decoder_drain_signals=" << diag.video_decoder_drain_signals << std::endl;

    std::cout << "performance-log-check.audio_decoder_drain_signals=" << diag.audio_decoder_drain_signals << std::endl;

    std::cout << "performance-log-check.video_native_output_frames=" << diag.video_native_output_frames << std::endl;

    std::cout << "performance-log-check.video_copy_back_frames=" << diag.video_copy_back_frames << std::endl;

    std::cout << "performance-log-check.video_copy_back_time_ms=" << copy_back_time_ms << std::endl;

    std::cout << "performance-log-check.video_copy_back_ratio_percent=" << copy_back_ratio_percent << std::endl;

    std::cout << "performance-log-check.video_copy_back_avg_ms=" << copy_back_avg_ms << std::endl;

    std::cout << "performance-log-check.video_copy_back_max_us=" << diag.video_copy_back_time_us_max << std::endl;

    std::cout << "performance-log-check.video_swscale_frames=" << diag.video_swscale_frames << std::endl;

    std::cout << "performance-log-check.video_swscale_time_ms=" << swscale_time_ms << std::endl;

    std::cout << "performance-log-check.video_swscale_ratio_percent=" << swscale_ratio_percent << std::endl;

    std::cout << "performance-log-check.video_swscale_avg_ms=" << swscale_avg_ms << std::endl;

    std::cout << "performance-log-check.video_swscale_max_us=" << diag.video_swscale_time_us_max << std::endl;

    std::cout << "performance-log-check.display_copy_frames=" << diag.display_copy_frames << std::endl;

    std::cout << "performance-log-check.display_copy_bytes=" << diag.display_copy_bytes << std::endl;

    std::cout << "performance-log-check.display_copy_time_ms=" << display_copy_time_ms << std::endl;

    std::cout << "performance-log-check.display_copy_ratio_percent=" << display_copy_ratio_percent << std::endl;

    std::cout << "performance-log-check.display_copy_avg_ms=" << display_copy_avg_ms << std::endl;

    std::cout << "performance-log-check.display_copy_max_us=" << diag.display_copy_time_us_max << std::endl;

    std::cout << "performance-log-check.renderer_opengl_native_interop_active="
              << (diag.renderer_opengl_native_interop_active ? "true" : "false") << std::endl;

    std::cout << "performance-log-check.renderer_opengl_native_interop_startup_disabled="
              << (diag.renderer_opengl_native_interop_startup_disabled ? "true" : "false") << std::endl;

    std::cout << "performance-log-check.renderer_opengl_native_interop_disable_rule="
              << diag.renderer_opengl_native_interop_disable_rule << std::endl;

    std::cout << "performance-log-check.renderer_opengl_native_interop_frames="
              << diag.renderer_opengl_native_interop_frames << std::endl;

    std::cout << "performance-log-check.renderer_opengl_native_interop_disable_events="
              << diag.renderer_opengl_native_interop_disable_events << std::endl;

    std::cout << "performance-log-check.renderer_opengl_present_wait_timeouts="
              << diag.renderer_opengl_present_wait_timeouts << std::endl;
    std::cout << "performance-log-check.renderer_opengl_present_mode_requested="
              << diag.renderer_opengl_present_mode_requested << std::endl;
    std::cout << "performance-log-check.renderer_opengl_present_mode_active="
              << diag.renderer_opengl_present_mode_active << std::endl;
    std::cout << "performance-log-check.renderer_opengl_hdr_bridge_requested="
              << (diag.renderer_opengl_hdr_bridge_requested ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.renderer_opengl_hdr_bridge_active="
              << (diag.renderer_opengl_hdr_bridge_active ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.renderer_opengl_hdr_bridge_mode="
              << diag.renderer_opengl_hdr_bridge_mode << std::endl;
    std::cout << "performance-log-check.renderer_opengl_hdr_bridge_decision="
              << diag.renderer_opengl_hdr_bridge_decision << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_configured="
              << (diag.renderer_opengl_output_lut_configured ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_active="
              << (diag.renderer_opengl_output_lut_active ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_size="
              << diag.renderer_opengl_output_lut_size << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_path="
              << diag.renderer_opengl_output_lut_path << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_error="
              << diag.renderer_opengl_output_lut_error << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_source="
              << diag.renderer_opengl_output_lut_source << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_lut_reload_count="
              << diag.renderer_opengl_output_lut_reload_count << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_display_index="
              << diag.renderer_opengl_output_display_index << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_display_name="
              << diag.renderer_opengl_output_display_name << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_display_device_name="
              << diag.renderer_opengl_output_display_device_name << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_icc_profile_available="
              << (diag.renderer_opengl_output_icc_profile_available ? "true" : "false") << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_icc_profile_source="
              << diag.renderer_opengl_output_icc_profile_source << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_icc_profile_path="
              << diag.renderer_opengl_output_icc_profile_path << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_icc_profile_description="
              << diag.renderer_opengl_output_icc_profile_description << std::endl;
    std::cout << "performance-log-check.renderer_opengl_output_binding_error="
              << diag.renderer_opengl_output_binding_error << std::endl;

    std::cout << "performance-log-check.video_filter_blocked_native_frames=" << diag.video_filter_blocked_native_frames << std::endl;

    std::cout << "performance-log-check.audio_submitted_frames=" << diag.audio_submitted_frames << std::endl;

    std::cout << "performance-log-check.render_frames=" << diag.render_frames << std::endl;

    std::cout << "performance-log-check.scheduler_video_decoded_frames=" << diag.scheduler_video_decoded_frames << std::endl;

    std::cout << "performance-log-check.scheduler_audio_decoded_frames=" << diag.scheduler_audio_decoded_frames << std::endl;

    std::cout << "performance-log-check.scheduler_late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "performance-log-check.scheduler_wait_events=" << diag.scheduler_wait_events << std::endl;

    std::cout << "performance-log-check.scheduler_render_wait_ms=" << diag.scheduler_render_wait_ms << std::endl;

    std::cout << "performance-log-check.scheduler_video_backpressure_events=" << diag.scheduler_video_backpressure_events << std::endl;

    std::cout << "performance-log-check.scheduler_audio_backpressure_events=" << diag.scheduler_audio_backpressure_events << std::endl;

    std::cout << "performance-log-check.scheduler_video_backpressure_wait_ms=" << diag.scheduler_video_backpressure_wait_ms << std::endl;

    std::cout << "performance-log-check.scheduler_audio_backpressure_wait_ms=" << diag.scheduler_audio_backpressure_wait_ms << std::endl;

    std::cout << "performance-log-check.scheduler_video_restart_attempts=" << diag.scheduler_video_restart_attempts << std::endl;

    std::cout << "performance-log-check.scheduler_audio_restart_attempts=" << diag.scheduler_audio_restart_attempts << std::endl;

    std::cout << "performance-log-check.scheduler_render_restart_attempts=" << diag.scheduler_render_restart_attempts << std::endl;

    std::cout << "performance-log-check.scheduler_video_restart_limit_hits=" << diag.scheduler_video_restart_limit_hits << std::endl;

    std::cout << "performance-log-check.scheduler_audio_restart_limit_hits=" << diag.scheduler_audio_restart_limit_hits << std::endl;

    std::cout << "performance-log-check.scheduler_render_restart_limit_hits=" << diag.scheduler_render_restart_limit_hits << std::endl;

    std::cout << "performance-log-check.runtime_failure_stop_requests="
              << diag.runtime_failure_stop_requests << std::endl;

    std::cout << "performance-log-check.runtime_failure_fail_sessions="
              << diag.runtime_failure_fail_sessions << std::endl;

    std::cout << "performance-log-check.illegal_session_transitions="
              << diag.illegal_session_transitions << std::endl;

    std::cout << "performance-log-check.illegal_run_transitions="
              << diag.illegal_run_transitions << std::endl;

    std::cout << "performance-log-check.illegal_pipeline_transitions="
              << diag.illegal_pipeline_transitions << std::endl;

    std::cout << "performance-log-check.runtime_drop_total=" << diag.runtime_drop_total << std::endl;

    std::cout << "performance-log-check.runtime_drop_summary=" << diag.runtime_drop_summary << std::endl;

    printD3D11RuntimeHdrState("performance-log-check.renderer_d3d11", diag);

    std::cout << "performance-log-check.video_packet_queue_size=" << diag.video_packet_queue_size << std::endl;

    std::cout << "performance-log-check.audio_packet_queue_size=" << diag.audio_packet_queue_size << std::endl;

    std::cout << "performance-log-check.video_frame_queue_size=" << diag.video_frame_queue_size << std::endl;

    std::cout << "performance-log-check.audio_frame_queue_size=" << diag.audio_frame_queue_size << std::endl;

    std::cout << "performance-log-check.video_frame_queue_capacity=" << diag.video_frame_queue_capacity << std::endl;

    std::cout << "performance-log-check.audio_frame_queue_capacity=" << diag.audio_frame_queue_capacity << std::endl;

    std::cout << "performance-log-check.video_frame_queue_peak_size=" << diag.video_frame_queue_peak_size << std::endl;

    std::cout << "performance-log-check.audio_frame_queue_peak_size=" << diag.audio_frame_queue_peak_size << std::endl;

    std::cout << "performance-log-check.video_frame_queue_push_timeouts=" << diag.video_frame_queue_push_timeouts << std::endl;

    std::cout << "performance-log-check.audio_frame_queue_push_timeouts=" << diag.audio_frame_queue_push_timeouts << std::endl;

    std::cout << "performance-log-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}

bool runInteractionFreezeCheck(const std::string& media_file, int sample_ms = 1800) {

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 800) {
        std::cout << "interaction-freeze-check.path=" << media_file << std::endl;
        std::cout << "interaction-freeze-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "interaction-freeze-check.result=FAIL" << std::endl;
        return false;
    }

    auto push_event = [](const SDL_Event& source_event) {
        SDL_Event event = source_event;
        return SDL_PushEvent(&event) == 1;
    };
    auto push_mouse_motion = [&](int x, int y) {
        SDL_Event event{};
        event.type = SDL_MOUSEMOTION;
        event.motion.type = SDL_MOUSEMOTION;
        event.motion.x = x;
        event.motion.y = y;
        return push_event(event);
    };
    auto push_mouse_click = [&](int x, int y) {
        SDL_Event down{};
        down.type = SDL_MOUSEBUTTONDOWN;
        down.button.type = SDL_MOUSEBUTTONDOWN;
        down.button.button = SDL_BUTTON_LEFT;
        down.button.state = SDL_PRESSED;
        down.button.x = x;
        down.button.y = y;

        SDL_Event up{};
        up.type = SDL_MOUSEBUTTONUP;
        up.button.type = SDL_MOUSEBUTTONUP;
        up.button.button = SDL_BUTTON_LEFT;
        up.button.state = SDL_RELEASED;
        up.button.x = x;
        up.button.y = y;
        return push_event(down) && push_event(up);
    };
    auto push_key_tap = [&](SDL_Keycode key_code) {
        SDL_Event down{};
        down.type = SDL_KEYDOWN;
        down.key.type = SDL_KEYDOWN;
        down.key.state = SDL_PRESSED;
        down.key.repeat = 0;
        down.key.keysym.sym = key_code;

        SDL_Event up{};
        up.type = SDL_KEYUP;
        up.key.type = SDL_KEYUP;
        up.key.state = SDL_RELEASED;
        up.key.repeat = 0;
        up.key.keysym.sym = key_code;
        return push_event(down) && push_event(up);
    };
    auto push_window_event = [&](Uint8 window_event, int data1 = 0, int data2 = 0) {
        SDL_Event event{};
        event.type = SDL_WINDOWEVENT;
        event.window.type = SDL_WINDOWEVENT;
        event.window.event = window_event;
        event.window.data1 = data1;
        event.window.data2 = data2;
        return push_event(event);
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    uint64_t pump_iterations = 0;
    uint64_t settle_iterations = 0;
    uint64_t injected_mouse_move = 0;
    uint64_t injected_mouse_click = 0;
    uint64_t injected_hotkey = 0;
    uint64_t injected_window_event = 0;
    core::DiagnosticsSnapshot before_diag{};
    core::DiagnosticsSnapshot after_diag{};
    std::string renderer_backend = "None";
    std::string decoder_backend = "Unknown";

    if (open_ok) {
        player.play();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        before_diag = player.getDiagnosticsSnapshot();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(sample_ms);
        while (std::chrono::steady_clock::now() < deadline && (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            const int phase = static_cast<int>(pump_iterations % 24);
            const int mouse_x = 80 + static_cast<int>((pump_iterations % 20) * 36);
            const int progress_y = 700;

            if (push_mouse_motion(mouse_x, progress_y)) {
                ++injected_mouse_move;
            }
            if ((phase == 2 || phase == 14) && push_mouse_click(240 + phase * 8, progress_y)) {
                ++injected_mouse_click;
            }
            if ((phase == 4 || phase == 16) && push_mouse_click(1120 - phase * 6, progress_y)) {
                ++injected_mouse_click;
            }
            if ((phase == 6 || phase == 18) && push_key_tap(SDLK_EQUALS)) {
                ++injected_hotkey;
            }
            if ((phase == 7 || phase == 19) && push_key_tap(SDLK_MINUS)) {
                ++injected_hotkey;
            }
            if ((phase == 8 || phase == 20) && push_key_tap(SDLK_RETURN)) {
                ++injected_hotkey;
            }
            if (phase == 10 && push_window_event(SDL_WINDOWEVENT_MINIMIZED)) {
                ++injected_window_event;
            }
            if (phase == 11 && push_window_event(SDL_WINDOWEVENT_RESTORED)) {
                ++injected_window_event;
            }
            if (phase == 12 && push_window_event(SDL_WINDOWEVENT_MAXIMIZED)) {
                ++injected_window_event;
            }
            if (phase == 13 && push_window_event(SDL_WINDOWEVENT_SIZE_CHANGED, 1280, 720)) {
                ++injected_window_event;
            }

            player.pumpEvents();
            ++pump_iterations;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        const auto settle_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(600);
        while (std::chrono::steady_clock::now() < settle_deadline && (player.isPlaying() || player.isPaused())) {
            player.pumpEvents();
            ++settle_iterations;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        after_diag = player.getDiagnosticsSnapshot();
        player.stop();
        player.close();
    }

    const uint64_t injected_events_total =
        injected_mouse_move + injected_mouse_click + injected_hotkey + injected_window_event;
    const bool injected_events_ok = injected_events_total > 0;
    const bool render_progress_ok = after_diag.render_frames > before_diag.render_frames;
    const bool fail_session_ok = after_diag.runtime_failure_fail_sessions <= before_diag.runtime_failure_fail_sessions;
    const bool illegal_transition_ok =
        after_diag.illegal_session_transitions <= before_diag.illegal_session_transitions &&
        after_diag.illegal_run_transitions <= before_diag.illegal_run_transitions &&
        after_diag.illegal_pipeline_transitions <= before_diag.illegal_pipeline_transitions;

    const bool result = open_ok &&
                        entered_playback_loop &&
                        injected_events_ok &&
                        render_progress_ok &&
                        fail_session_ok &&
                        illegal_transition_ok;

    std::cout << "interaction-freeze-check.path=" << media_file << std::endl;
    std::cout << "interaction-freeze-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "interaction-freeze-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "interaction-freeze-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false")
              << std::endl;
    std::cout << "interaction-freeze-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "interaction-freeze-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "interaction-freeze-check.pump_iterations=" << pump_iterations << std::endl;
    std::cout << "interaction-freeze-check.settle_iterations=" << settle_iterations << std::endl;
    std::cout << "interaction-freeze-check.injected_mouse_move=" << injected_mouse_move << std::endl;
    std::cout << "interaction-freeze-check.injected_mouse_click=" << injected_mouse_click << std::endl;
    std::cout << "interaction-freeze-check.injected_hotkey=" << injected_hotkey << std::endl;
    std::cout << "interaction-freeze-check.injected_window_event=" << injected_window_event << std::endl;
    std::cout << "interaction-freeze-check.injected_events_total=" << injected_events_total << std::endl;
    std::cout << "interaction-freeze-check.render_frames_before=" << before_diag.render_frames << std::endl;
    std::cout << "interaction-freeze-check.render_frames_after=" << after_diag.render_frames << std::endl;
    std::cout << "interaction-freeze-check.render_progress_ok=" << (render_progress_ok ? "true" : "false")
              << std::endl;
    std::cout << "interaction-freeze-check.fail_session_ok=" << (fail_session_ok ? "true" : "false")
              << std::endl;
    std::cout << "interaction-freeze-check.illegal_transition_ok=" << (illegal_transition_ok ? "true" : "false")
              << std::endl;
    std::cout << "interaction-freeze-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 强制 `SoftwareSDL + Software decode` 并验证软解链是否真实产帧；命令本身以 probe 方式硬退出，避免被 blocker 卡死。

bool pumpPlayerForDuration(VideoPlayer& player,
                           std::chrono::milliseconds duration,
                           uint64_t* pump_iterations = nullptr) {
    bool entered_playback_loop = false;
    uint64_t iterations = 0;
    const auto deadline = std::chrono::steady_clock::now() + duration;
    while (std::chrono::steady_clock::now() < deadline &&
           (player.isPlaying() || player.isPaused())) {
        entered_playback_loop = true;
        player.pumpEvents();
        ++iterations;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    if (pump_iterations) {
        *pump_iterations = iterations;
    }
    return entered_playback_loop;
}

bool equalsIgnoreCase(std::string lhs, std::string rhs) {
    return toLower(std::move(lhs)) == toLower(std::move(rhs));
}

std::vector<std::string> collectSdlAudioDrivers() {
    std::vector<std::string> drivers;
    const int driver_count = SDL_GetNumAudioDrivers();
    for (int index = 0; index < driver_count; ++index) {
        if (const char* driver = SDL_GetAudioDriver(index)) {
            drivers.emplace_back(driver);
        }
    }
    return drivers;
}

bool runLinuxSoftwareAudioCheck(const std::string& media_file, int sample_ms = 1800) {
#if !defined(__linux__)
    std::cout << "linux-software-audio-check.path=" << media_file << std::endl;
    std::cout << "linux-software-audio-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-software-audio-check.platform=NonLinux" << std::endl;
    std::cout << "linux-software-audio-check.platform_ok=false" << std::endl;
    std::cout << "linux-software-audio-check.result=FAIL" << std::endl;
    return false;
#endif

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "linux-software-audio-check.path=" << media_file << std::endl;
        std::cout << "linux-software-audio-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "linux-software-audio-check.result=FAIL" << std::endl;
        return false;
    }

    ScopedEnvOverride force_software_renderer("MVP_RENDERER_BACKEND", "software");
    ScopedEnvOverride force_audio_enabled("MVP_DISABLE_AUDIO_OUTPUT", "0");

    VideoPlayer player;
    player.setPreferHardwareDecode(false);
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    std::string active_audio_driver{"none"};
    core::DiagnosticsSnapshot diag{};

    if (open_ok) {
        player.play();
        entered_playback_loop = pumpPlayerForDuration(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        if (const char* current_driver = SDL_GetCurrentAudioDriver()) {
            active_audio_driver = current_driver;
        }
        player.stop();
        player.close();
    }

    const bool linux_platform = diag.startup_platform == "Linux";
    const bool renderer_ok = renderer_backend == "SoftwareSDL";
    const bool decoder_ok = decoder_backend == "Software";
    const bool audio_output_ok =
        diag.audio_output_initialized &&
        !diag.video_only_fallback &&
        diag.audio_init_strategy == "sdl-open-ok";
    const bool decode_ok = diag.decode_video_ok > 0 && diag.decode_audio_ok > 0;
    const bool output_ok = diag.render_frames > 0 && diag.audio_submitted_frames > 0;
    const bool result = linux_platform &&
                        open_ok &&
                        entered_playback_loop &&
                        renderer_ok &&
                        decoder_ok &&
                        audio_output_ok &&
                        decode_ok &&
                        output_ok;

    std::cout << "linux-software-audio-check.path=" << media_file << std::endl;
    std::cout << "linux-software-audio-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-software-audio-check.platform=" << diag.startup_platform << std::endl;
    std::cout << "linux-software-audio-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "linux-software-audio-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "linux-software-audio-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "linux-software-audio-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "linux-software-audio-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "linux-software-audio-check.active_audio_driver=" << active_audio_driver << std::endl;
    std::cout << "linux-software-audio-check.audio_output_initialized="
              << (diag.audio_output_initialized ? "true" : "false") << std::endl;
    std::cout << "linux-software-audio-check.video_only_fallback="
              << (diag.video_only_fallback ? "true" : "false") << std::endl;
    std::cout << "linux-software-audio-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;
    std::cout << "linux-software-audio-check.audio_init_detail=" << diag.audio_init_detail << std::endl;
    std::cout << "linux-software-audio-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "linux-software-audio-check.decode_audio_ok=" << diag.decode_audio_ok << std::endl;
    std::cout << "linux-software-audio-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "linux-software-audio-check.audio_submitted_frames=" << diag.audio_submitted_frames << std::endl;
    std::cout << "linux-software-audio-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

bool runLinuxOpenGLPlaybackCheck(const std::string& media_file, int sample_ms = 1800) {
#if !defined(__linux__)
    std::cout << "linux-opengl-playback-check.path=" << media_file << std::endl;
    std::cout << "linux-opengl-playback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-opengl-playback-check.platform=NonLinux" << std::endl;
    std::cout << "linux-opengl-playback-check.platform_ok=false" << std::endl;
    std::cout << "linux-opengl-playback-check.result=FAIL" << std::endl;
    return false;
#endif

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "linux-opengl-playback-check.path=" << media_file << std::endl;
        std::cout << "linux-opengl-playback-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "linux-opengl-playback-check.result=FAIL" << std::endl;
        return false;
    }

    ScopedEnvOverride force_opengl_renderer("MVP_RENDERER_BACKEND", "opengl");
    ScopedEnvOverride clear_opengl_fail_injection("MVP_OPENGL_FORCE_INIT_FAIL", "0");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    core::DiagnosticsSnapshot diag{};

    if (open_ok) {
        player.play();
        entered_playback_loop = pumpPlayerForDuration(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const bool linux_platform = diag.startup_platform == "Linux";
    const bool renderer_ok = renderer_backend == "OpenGL";
    const bool decode_ok = diag.decode_video_ok > 0;
    const bool render_ok = diag.render_frames > 0;
    const bool runtime_ok =
        diag.runtime_failure_fail_sessions == 0 &&
        diag.illegal_session_transitions == 0 &&
        diag.illegal_run_transitions == 0 &&
        diag.illegal_pipeline_transitions == 0;
    const bool result = linux_platform &&
                        open_ok &&
                        entered_playback_loop &&
                        renderer_ok &&
                        decode_ok &&
                        render_ok &&
                        runtime_ok;

    std::cout << "linux-opengl-playback-check.path=" << media_file << std::endl;
    std::cout << "linux-opengl-playback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-opengl-playback-check.platform=" << diag.startup_platform << std::endl;
    std::cout << "linux-opengl-playback-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-playback-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-playback-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-playback-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "linux-opengl-playback-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "linux-opengl-playback-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "linux-opengl-playback-check.decode_audio_ok=" << diag.decode_audio_ok << std::endl;
    std::cout << "linux-opengl-playback-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "linux-opengl-playback-check.runtime_failure_fail_sessions="
              << diag.runtime_failure_fail_sessions << std::endl;
    std::cout << "linux-opengl-playback-check.illegal_session_transitions="
              << diag.illegal_session_transitions << std::endl;
    std::cout << "linux-opengl-playback-check.illegal_run_transitions="
              << diag.illegal_run_transitions << std::endl;
    std::cout << "linux-opengl-playback-check.illegal_pipeline_transitions="
              << diag.illegal_pipeline_transitions << std::endl;
    std::cout << "linux-opengl-playback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

bool runLinuxOpenGLFallbackCheck(const std::string& media_file, int sample_ms = 1800) {
#if !defined(__linux__)
    std::cout << "linux-opengl-fallback-check.path=" << media_file << std::endl;
    std::cout << "linux-opengl-fallback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-opengl-fallback-check.platform=NonLinux" << std::endl;
    std::cout << "linux-opengl-fallback-check.platform_ok=false" << std::endl;
    std::cout << "linux-opengl-fallback-check.result=FAIL" << std::endl;
    return false;
#endif

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "linux-opengl-fallback-check.path=" << media_file << std::endl;
        std::cout << "linux-opengl-fallback-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "linux-opengl-fallback-check.result=FAIL" << std::endl;
        return false;
    }

    ScopedEnvOverride force_opengl_renderer("MVP_RENDERER_BACKEND", "opengl");
    ScopedEnvOverride force_opengl_init_fail("MVP_OPENGL_FORCE_INIT_FAIL", "1");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    core::DiagnosticsSnapshot diag{};

    if (open_ok) {
        player.play();
        entered_playback_loop = pumpPlayerForDuration(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const bool linux_platform = diag.startup_platform == "Linux";
    const bool fallback_to_software = renderer_backend == "SoftwareSDL";
    const bool fallback_selected_non_opengl = renderer_backend != "OpenGL" && renderer_backend != "None";
    const bool fallback_reason_recorded =
        !diag.startup_renderer_fallback_reason.empty() &&
        diag.startup_renderer_fallback_reason != "none";
    const bool fallback_chain_listed =
        diag.startup_renderer_candidates.find("OpenGL") != std::string::npos &&
        diag.startup_renderer_candidates.find("SoftwareSDL") != std::string::npos;
    const bool decode_ok = diag.decode_video_ok > 0;
    const bool render_ok = diag.render_frames > 0;
    const bool result = linux_platform &&
                        open_ok &&
                        entered_playback_loop &&
                        fallback_selected_non_opengl &&
                        fallback_reason_recorded &&
                        fallback_chain_listed &&
                        decode_ok &&
                        render_ok;

    std::cout << "linux-opengl-fallback-check.path=" << media_file << std::endl;
    std::cout << "linux-opengl-fallback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-opengl-fallback-check.platform=" << diag.startup_platform << std::endl;
    std::cout << "linux-opengl-fallback-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "linux-opengl-fallback-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "linux-opengl-fallback-check.startup_renderer_candidates="
              << diag.startup_renderer_candidates << std::endl;
    std::cout << "linux-opengl-fallback-check.startup_selected_renderer="
              << diag.startup_selected_renderer << std::endl;
    std::cout << "linux-opengl-fallback-check.startup_renderer_fallback_reason="
              << diag.startup_renderer_fallback_reason << std::endl;
    std::cout << "linux-opengl-fallback-check.fallback_to_software="
              << (fallback_to_software ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.fallback_selected_non_opengl="
              << (fallback_selected_non_opengl ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.fallback_chain_listed="
              << (fallback_chain_listed ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.fallback_reason_recorded="
              << (fallback_reason_recorded ? "true" : "false") << std::endl;
    std::cout << "linux-opengl-fallback-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "linux-opengl-fallback-check.decode_audio_ok=" << diag.decode_audio_ok << std::endl;
    std::cout << "linux-opengl-fallback-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "linux-opengl-fallback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

bool runLinuxVaapiFallbackCheck(const std::string& media_file, int sample_ms = 1800) {
#if !defined(__linux__)
    std::cout << "linux-vaapi-fallback-check.path=" << media_file << std::endl;
    std::cout << "linux-vaapi-fallback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-vaapi-fallback-check.platform=NonLinux" << std::endl;
    std::cout << "linux-vaapi-fallback-check.platform_ok=false" << std::endl;
    std::cout << "linux-vaapi-fallback-check.result=FAIL" << std::endl;
    return false;
#endif

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "linux-vaapi-fallback-check.path=" << media_file << std::endl;
        std::cout << "linux-vaapi-fallback-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "linux-vaapi-fallback-check.result=FAIL" << std::endl;
        return false;
    }

    ScopedEnvOverride force_opengl_renderer("MVP_RENDERER_BACKEND", "opengl");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    core::DiagnosticsSnapshot diag{};

    if (open_ok) {
        player.play();
        entered_playback_loop = pumpPlayerForDuration(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        player.stop();
        player.close();
    }

    const bool linux_platform = diag.startup_platform == "Linux";
    const bool renderer_ok = renderer_backend == "OpenGL";
    const bool decode_ok = diag.decode_video_ok > 0;
    const bool render_ok = diag.render_frames > 0;
    const bool runtime_ok =
        diag.runtime_failure_fail_sessions == 0 &&
        diag.illegal_session_transitions == 0 &&
        diag.illegal_run_transitions == 0 &&
        diag.illegal_pipeline_transitions == 0;
    const bool capability_probe_completed =
        diag.startup_decoder_capabilities.find("VAAPI(compiled=true") != std::string::npos;
    const bool capability_available =
        diag.startup_decoder_capabilities.find("VAAPI(compiled=true,runtime=true") != std::string::npos;
    const bool fallback_chain_listed =
        diag.startup_decoder_candidates.find("VAAPI") != std::string::npos &&
        diag.startup_decoder_candidates.find("Software") != std::string::npos;
    const bool selected_vaapi = decoder_backend == "VAAPI";
    const bool selected_software = decoder_backend == "Software";
    const bool copy_back_ok = diag.video_copy_back_frames > 0;
    const bool fallback_reason_recorded =
        !diag.startup_decoder_fallback_reason.empty() &&
        diag.startup_decoder_fallback_reason != "none";
    const bool path_ok =
        capability_available
            ? ((selected_vaapi && copy_back_ok) || (selected_software && fallback_reason_recorded))
            : selected_software;
    const bool result = linux_platform &&
                        open_ok &&
                        entered_playback_loop &&
                        renderer_ok &&
                        decode_ok &&
                        render_ok &&
                        runtime_ok &&
                        capability_probe_completed &&
                        path_ok &&
                        (!capability_available || fallback_chain_listed);

    std::cout << "linux-vaapi-fallback-check.path=" << media_file << std::endl;
    std::cout << "linux-vaapi-fallback-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-vaapi-fallback-check.platform=" << diag.startup_platform << std::endl;
    std::cout << "linux-vaapi-fallback-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "linux-vaapi-fallback-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "linux-vaapi-fallback-check.startup_decoder_capabilities="
              << diag.startup_decoder_capabilities << std::endl;
    std::cout << "linux-vaapi-fallback-check.startup_decoder_candidates="
              << diag.startup_decoder_candidates << std::endl;
    std::cout << "linux-vaapi-fallback-check.startup_selected_decoder="
              << diag.startup_selected_decoder << std::endl;
    std::cout << "linux-vaapi-fallback-check.startup_decoder_fallback_reason="
              << diag.startup_decoder_fallback_reason << std::endl;
    std::cout << "linux-vaapi-fallback-check.capability_probe_completed="
              << (capability_probe_completed ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.capability_available="
              << (capability_available ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.fallback_chain_listed="
              << (fallback_chain_listed ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.selected_vaapi="
              << (selected_vaapi ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.selected_software="
              << (selected_software ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.copy_back_frames=" << diag.video_copy_back_frames << std::endl;
    std::cout << "linux-vaapi-fallback-check.copy_back_ok="
              << (copy_back_ok ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.fallback_reason_recorded="
              << (fallback_reason_recorded ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.decode_video_ok=" << diag.decode_video_ok << std::endl;
    std::cout << "linux-vaapi-fallback-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "linux-vaapi-fallback-check.runtime_ok=" << (runtime_ok ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.path_ok=" << (path_ok ? "true" : "false") << std::endl;
    std::cout << "linux-vaapi-fallback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runLibassShapingCheck(const std::string& subtitle_file) {
    const subtitle::LibassShapingProbeSummary summary = subtitle::probeLibassShaping(subtitle_file);
    const bool linux_platform = summary.supported_platform;
    const bool result = summary.supported_platform &&
                        summary.file_accessible &&
                        summary.parser_supported_extension &&
                        summary.library_initialized &&
                        summary.renderer_initialized &&
                        summary.track_loaded &&
                        summary.events_loaded &&
                        summary.rendered_output;

    std::cout << "libass-shaping-check.path=" << subtitle_file << std::endl;
    std::cout << "libass-shaping-check.platform=" << (linux_platform ? "Linux" : "NonLinux") << std::endl;
    std::cout << "libass-shaping-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.file_accessible=" << (summary.file_accessible ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.parser_supported_extension="
              << (summary.parser_supported_extension ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.library_initialized="
              << (summary.library_initialized ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.renderer_initialized="
              << (summary.renderer_initialized ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.track_loaded=" << (summary.track_loaded ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.events_loaded=" << (summary.events_loaded ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.sampled_timestamps=" << summary.sampled_timestamps << std::endl;
    std::cout << "libass-shaping-check.changed_frames=" << summary.changed_frames << std::endl;
    std::cout << "libass-shaping-check.rendered_images=" << summary.rendered_images << std::endl;
    std::cout << "libass-shaping-check.max_image_width=" << summary.max_image_width << std::endl;
    std::cout << "libass-shaping-check.max_image_height=" << summary.max_image_height << std::endl;
    std::cout << "libass-shaping-check.rendered_output="
              << (summary.rendered_output ? "true" : "false") << std::endl;
    std::cout << "libass-shaping-check.error="
              << (summary.error.empty() ? std::string("none") : summary.error) << std::endl;
    std::cout << "libass-shaping-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runEmbeddedSubtitleLivePacketCheck(const std::string& media_file,
                                        int stream_index = -1,
                                        int max_packets = 120) {
    const size_t safe_max_packets =
        max_packets > 0 ? static_cast<size_t>(max_packets) : static_cast<size_t>(120);
    const subtitle::EmbeddedSubtitleLivePacketProbeResult summary =
        subtitle::probeEmbeddedSubtitleLivePacketPath(media_file, stream_index, safe_max_packets);

    const bool result = summary.input_opened &&
                        summary.subtitle_stream_found &&
                        summary.supported_stream_found &&
                        summary.decoder_opened &&
                        summary.subtitle_packets_read > 0 &&
                        summary.monotonic_timestamps &&
                        summary.produced_output;

    std::cout << "embedded-subtitle-live-packet-check.path=" << media_file << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.requested_stream_index="
              << stream_index << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.max_packets=" << summary.max_packets << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.input_opened="
              << (summary.input_opened ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.subtitle_stream_found="
              << (summary.subtitle_stream_found ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.supported_stream_found="
              << (summary.supported_stream_found ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.decoder_opened="
              << (summary.decoder_opened ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.stream_index=" << summary.stream_index << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.codec=" << summary.codec_name << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.packets_read=" << summary.packets_read << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.subtitle_packets_read="
              << summary.subtitle_packets_read << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.decoded_text_events="
              << summary.decoded_text_events << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.decoded_bitmap_rects="
              << summary.decoded_bitmap_rects << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.monotonic_timestamps="
              << (summary.monotonic_timestamps ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.produced_output="
              << (summary.produced_output ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.eof_reached="
              << (summary.eof_reached ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-live-packet-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runLinuxAudioBackendSmoke(const std::string& media_file, int sample_ms = 1800) {
#if !defined(__linux__)
    std::cout << "linux-audio-backend-smoke.path=" << media_file << std::endl;
    std::cout << "linux-audio-backend-smoke.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-audio-backend-smoke.platform=NonLinux" << std::endl;
    std::cout << "linux-audio-backend-smoke.platform_ok=false" << std::endl;
    std::cout << "linux-audio-backend-smoke.available_target_count=0" << std::endl;
    std::cout << "linux-audio-backend-smoke.pass_target_count=0" << std::endl;
    std::cout << "linux-audio-backend-smoke.result=FAIL" << std::endl;
    return false;
#endif

    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "linux-audio-backend-smoke.path=" << media_file << std::endl;
        std::cout << "linux-audio-backend-smoke.sample_ms=" << sample_ms << std::endl;
        std::cout << "linux-audio-backend-smoke.result=FAIL" << std::endl;
        return false;
    }

    struct DriverTarget {
        std::string key;
        std::vector<std::string> aliases;
    };
    struct DriverSmokeSession {
        std::string key;
        bool available{false};
        std::string requested_driver{"none"};
        bool open_ok{false};
        bool entered_playback_loop{false};
        std::string renderer_backend{"None"};
        std::string decoder_backend{"Unknown"};
        std::string active_audio_driver{"none"};
        bool driver_match{false};
        bool audio_output_initialized{false};
        bool video_only_fallback{false};
        std::string audio_init_strategy{"not-run"};
        std::string audio_init_detail{};
        uint64_t decode_video_ok{0};
        uint64_t decode_audio_ok{0};
        uint64_t render_frames{0};
        uint64_t audio_submitted_frames{0};
        bool result{false};
    };

    const std::vector<DriverTarget> targets = {
        {"pulseaudio", {"pulseaudio", "pulse"}},
        {"alsa", {"alsa"}},
        {"pipewire", {"pipewire"}},
        // Keep a deterministic software-audio fallback for headless CI runners.
        {"dummy", {"dummy"}},
    };
    const std::vector<std::string> available_drivers = collectSdlAudioDrivers();

    const auto find_available_driver = [&](const std::vector<std::string>& aliases) -> std::string {
        for (const auto& alias : aliases) {
            const auto it = std::find_if(available_drivers.begin(),
                                         available_drivers.end(),
                                         [&](const std::string& driver) {
                                             return equalsIgnoreCase(driver, alias);
                                         });
            if (it != available_drivers.end()) {
                return *it;
            }
        }
        return {};
    };
    const auto driver_name_matches = [&](const std::string& requested, const std::string& active) {
        if (requested.empty() || active.empty() || active == "none") {
            return false;
        }
        if (equalsIgnoreCase(requested, active)) {
            return true;
        }
        return (equalsIgnoreCase(requested, "pulseaudio") && equalsIgnoreCase(active, "pulse")) ||
               (equalsIgnoreCase(requested, "pulse") && equalsIgnoreCase(active, "pulseaudio"));
    };

    bool linux_platform = false;
    std::vector<DriverSmokeSession> sessions;
    sessions.reserve(targets.size());

    for (const auto& target : targets) {
        DriverSmokeSession session;
        session.key = target.key;
        session.requested_driver = find_available_driver(target.aliases);
        session.available = !session.requested_driver.empty();

        if (!session.available) {
            sessions.push_back(std::move(session));
            continue;
        }

        ScopedEnvOverride force_software_renderer("MVP_RENDERER_BACKEND", "software");
        ScopedEnvOverride force_audio_enabled("MVP_DISABLE_AUDIO_OUTPUT", "0");
        ScopedEnvOverride force_audio_driver("SDL_AUDIODRIVER", session.requested_driver);

        VideoPlayer player;
        player.setPreferHardwareDecode(false);
        session.open_ok = player.open(media_file);
        if (session.open_ok) {
            player.play();
            session.entered_playback_loop = pumpPlayerForDuration(player, std::chrono::milliseconds(sample_ms));

            const core::DiagnosticsSnapshot diag = player.getDiagnosticsSnapshot();
            linux_platform = linux_platform || (diag.startup_platform == "Linux");
            session.renderer_backend = player.videoRendererBackendName();
            session.decoder_backend = player.videoDecoderBackendName();
            session.audio_output_initialized = diag.audio_output_initialized;
            session.video_only_fallback = diag.video_only_fallback;
            session.audio_init_strategy = diag.audio_init_strategy;
            session.audio_init_detail = diag.audio_init_detail;
            session.decode_video_ok = diag.decode_video_ok;
            session.decode_audio_ok = diag.decode_audio_ok;
            session.render_frames = diag.render_frames;
            session.audio_submitted_frames = diag.audio_submitted_frames;
            if (const char* current_driver = SDL_GetCurrentAudioDriver()) {
                session.active_audio_driver = current_driver;
            }
            player.stop();
            player.close();
        }

        session.driver_match = driver_name_matches(session.requested_driver, session.active_audio_driver);
        const bool renderer_ok = session.renderer_backend == "SoftwareSDL";
        const bool decoder_ok = session.decoder_backend == "Software";
        const bool audio_ok = session.audio_output_initialized &&
                              !session.video_only_fallback &&
                              session.audio_init_strategy == "sdl-open-ok";
        const bool decode_ok = session.decode_video_ok > 0 && session.decode_audio_ok > 0;
        const bool output_ok = session.render_frames > 0 && session.audio_submitted_frames > 0;
        session.result = session.open_ok &&
                         session.entered_playback_loop &&
                         renderer_ok &&
                         decoder_ok &&
                         audio_ok &&
                         decode_ok &&
                         output_ok &&
                         session.driver_match;
        sessions.push_back(std::move(session));
    }

    size_t available_count = 0;
    size_t pass_count = 0;
    for (const auto& session : sessions) {
        if (session.available) {
            ++available_count;
            if (session.result) {
                ++pass_count;
            }
        }
    }
    // Gate contract requires at least one Linux audio backend smoke path to pass.
    const bool result = linux_platform && available_count > 0 && pass_count > 0;

    std::cout << "linux-audio-backend-smoke.path=" << media_file << std::endl;
    std::cout << "linux-audio-backend-smoke.sample_ms=" << sample_ms << std::endl;
    std::cout << "linux-audio-backend-smoke.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "linux-audio-backend-smoke.available_drivers="
              << (available_drivers.empty() ? "none" : joinTopN(available_drivers)) << std::endl;
    std::cout << "linux-audio-backend-smoke.target_count=" << sessions.size() << std::endl;
    std::cout << "linux-audio-backend-smoke.available_target_count=" << available_count << std::endl;
    std::cout << "linux-audio-backend-smoke.pass_target_count=" << pass_count << std::endl;

    for (const auto& session : sessions) {
        std::cout << "linux-audio-backend-smoke." << session.key << ".available="
                  << (session.available ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".requested_driver="
                  << session.requested_driver << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".active_audio_driver="
                  << session.active_audio_driver << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".open_ok="
                  << (session.open_ok ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".entered_playback_loop="
                  << (session.entered_playback_loop ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".renderer_backend="
                  << session.renderer_backend << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".decoder_backend="
                  << session.decoder_backend << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".audio_output_initialized="
                  << (session.audio_output_initialized ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".video_only_fallback="
                  << (session.video_only_fallback ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".audio_init_strategy="
                  << session.audio_init_strategy << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".audio_init_detail="
                  << session.audio_init_detail << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".decode_video_ok="
                  << session.decode_video_ok << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".decode_audio_ok="
                  << session.decode_audio_ok << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".render_frames="
                  << session.render_frames << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".audio_submitted_frames="
                  << session.audio_submitted_frames << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".driver_match="
                  << (session.driver_match ? "true" : "false") << std::endl;
        std::cout << "linux-audio-backend-smoke." << session.key << ".result="
                  << (session.available ? (session.result ? "PASS" : "FAIL") : "SKIP") << std::endl;
    }
    std::cout << "linux-audio-backend-smoke.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

bool runCorePlaybackBehaviorCheck(const std::string& media_file, int sample_ms = 1800) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1200) {
        std::cout << "core-playback-behavior-check.path=" << media_file << std::endl;
        std::cout << "core-playback-behavior-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "core-playback-behavior-check.result=FAIL" << std::endl;
        return false;
    }

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    bool pause_state_ok = false;
    bool seek_position_changed = false;
    bool resume_state_ok = false;
    bool stopped_state_ok = false;
    bool play_called = false;
    bool pause_called = false;
    bool seek_called = false;
    bool resume_called = false;
    bool stop_called = false;
    uint64_t play_pump_iterations = 0;
    uint64_t settle_pump_iterations = 0;
    uint64_t resume_pump_iterations = 0;
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    double duration = 0.0;
    double seek_target = 0.0;
    double position_before_seek = 0.0;
    double position_after_seek = 0.0;
    core::DiagnosticsSnapshot before_diag{};
    core::DiagnosticsSnapshot after_diag{};

    if (open_ok) {
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();
        before_diag = player.getDiagnosticsSnapshot();

        play_called = true;
        player.play();
        const int play_window_ms = std::max(600, sample_ms / 2);
        entered_playback_loop = pumpPlayerForDuration(player,
                                                      std::chrono::milliseconds(play_window_ms),
                                                      &play_pump_iterations);

        pause_called = true;
        player.pause();
        for (int i = 0; i < 60; ++i) {
            player.pumpEvents();
            ++settle_pump_iterations;
            if (player.isPaused()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        pause_state_ok = player.isPaused();

        duration = player.getDuration();
        position_before_seek = player.getCurrentTime();
        const double max_seekable =
            duration > 1.0 ? std::max(0.0, duration - 0.2) : std::max(2.0, position_before_seek + 2.0);
        seek_target = std::max(0.0, std::min(max_seekable, position_before_seek + 1.0));
        seek_called = true;
        player.seek(seek_target);

        for (int i = 0; i < 80; ++i) {
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        position_after_seek = player.getCurrentTime();
        const double seek_delta = std::fabs(position_after_seek - position_before_seek);
        seek_position_changed = seek_delta > 0.25 || std::fabs(position_after_seek - seek_target) < 1.5;

        resume_called = true;
        player.play();
        const int resume_window_ms = std::max(600, sample_ms / 2);
        const bool resume_entered = pumpPlayerForDuration(player,
                                                          std::chrono::milliseconds(resume_window_ms),
                                                          &resume_pump_iterations);
        resume_state_ok = resume_entered && (player.isPlaying() || player.isPaused());

        stop_called = true;
        player.stop();
        for (int i = 0; i < 60; ++i) {
            player.pumpEvents();
            if (!player.isPlaying() && !player.isPaused()) {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        stopped_state_ok = !player.isPlaying() && !player.isPaused();

        after_diag = player.getDiagnosticsSnapshot();
        player.close();
    }

    const bool linux_platform = after_diag.startup_platform == "Linux";
    const bool progress_ok =
        after_diag.decode_video_ok > before_diag.decode_video_ok &&
        after_diag.render_frames > before_diag.render_frames;
    const bool runtime_ok =
        after_diag.runtime_failure_fail_sessions <= before_diag.runtime_failure_fail_sessions &&
        after_diag.illegal_session_transitions <= before_diag.illegal_session_transitions &&
        after_diag.illegal_run_transitions <= before_diag.illegal_run_transitions &&
        after_diag.illegal_pipeline_transitions <= before_diag.illegal_pipeline_transitions;
    const bool result = linux_platform &&
                        open_ok &&
                        play_called &&
                        pause_called &&
                        seek_called &&
                        resume_called &&
                        stop_called &&
                        entered_playback_loop &&
                        pause_state_ok &&
                        seek_position_changed &&
                        resume_state_ok &&
                        stopped_state_ok &&
                        progress_ok &&
                        runtime_ok;

    std::cout << "core-playback-behavior-check.path=" << media_file << std::endl;
    std::cout << "core-playback-behavior-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "core-playback-behavior-check.platform=" << after_diag.startup_platform << std::endl;
    std::cout << "core-playback-behavior-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "core-playback-behavior-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "core-playback-behavior-check.play_called=" << (play_called ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.pause_called=" << (pause_called ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.seek_called=" << (seek_called ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.resume_called=" << (resume_called ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.stop_called=" << (stop_called ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.pause_state_ok="
              << (pause_state_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.seek_position_changed="
              << (seek_position_changed ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.resume_state_ok="
              << (resume_state_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.stopped_state_ok="
              << (stopped_state_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.duration=" << duration << std::endl;
    std::cout << "core-playback-behavior-check.seek_target=" << seek_target << std::endl;
    std::cout << "core-playback-behavior-check.position_before_seek=" << position_before_seek << std::endl;
    std::cout << "core-playback-behavior-check.position_after_seek=" << position_after_seek << std::endl;
    std::cout << "core-playback-behavior-check.play_pump_iterations=" << play_pump_iterations << std::endl;
    std::cout << "core-playback-behavior-check.settle_pump_iterations=" << settle_pump_iterations << std::endl;
    std::cout << "core-playback-behavior-check.resume_pump_iterations=" << resume_pump_iterations << std::endl;
    std::cout << "core-playback-behavior-check.decode_video_ok.before=" << before_diag.decode_video_ok << std::endl;
    std::cout << "core-playback-behavior-check.decode_video_ok.after=" << after_diag.decode_video_ok << std::endl;
    std::cout << "core-playback-behavior-check.render_frames.before=" << before_diag.render_frames << std::endl;
    std::cout << "core-playback-behavior-check.render_frames.after=" << after_diag.render_frames << std::endl;
    std::cout << "core-playback-behavior-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.runtime_ok=" << (runtime_ok ? "true" : "false") << std::endl;
    std::cout << "core-playback-behavior-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

bool runUiInteractionCheck(const std::string& media_file, int sample_ms = 1800) {
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 1000) {
        std::cout << "ui-interaction-check.path=" << media_file << std::endl;
        std::cout << "ui-interaction-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "ui-interaction-check.result=FAIL" << std::endl;
        return false;
    }

    auto push_event = [](const SDL_Event& source_event) {
        SDL_Event event = source_event;
        return SDL_PushEvent(&event) == 1;
    };
    auto push_key_tap = [&](SDL_Keycode key_code) {
        SDL_Event down{};
        down.type = SDL_KEYDOWN;
        down.key.type = SDL_KEYDOWN;
        down.key.state = SDL_PRESSED;
        down.key.repeat = 0;
        down.key.keysym.sym = key_code;

        SDL_Event up{};
        up.type = SDL_KEYUP;
        up.key.type = SDL_KEYUP;
        up.key.state = SDL_RELEASED;
        up.key.repeat = 0;
        up.key.keysym.sym = key_code;
        return push_event(down) && push_event(up);
    };
    auto push_window_event = [&](Uint8 window_event, int data1 = 0, int data2 = 0) {
        SDL_Event event{};
        event.type = SDL_WINDOWEVENT;
        event.window.type = SDL_WINDOWEVENT;
        event.window.event = window_event;
        event.window.data1 = data1;
        event.window.data2 = data2;
        return push_event(event);
    };
    auto push_drop_file_event = [&](const std::string& path) {
        SDL_Event event{};
        event.type = SDL_DROPFILE;
        event.drop.type = SDL_DROPFILE;
        event.drop.file = SDL_strdup(path.c_str());
        if (!event.drop.file) {
            return false;
        }
        if (SDL_PushEvent(&event) != 1) {
            SDL_free(event.drop.file);
            return false;
        }
        return true;
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    bool drop_event_injected = false;
    bool drop_request_consumed = false;
    bool dropped_path_match = false;
    uint64_t pump_iterations = 0;
    uint64_t injected_fullscreen_toggles = 0;
    uint64_t injected_resize_events = 0;
    std::string dropped_path{};
    std::string renderer_backend{"None"};
    std::string decoder_backend{"Unknown"};
    core::DiagnosticsSnapshot before_diag{};
    core::DiagnosticsSnapshot after_diag{};

    if (open_ok) {
        player.play();
        before_diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        decoder_backend = player.videoDecoderBackendName();

        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(sample_ms);
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered_playback_loop = true;
            const int phase = static_cast<int>(pump_iterations % 20);
            if ((phase == 2 || phase == 12) && push_key_tap(SDLK_RETURN)) {
                ++injected_fullscreen_toggles;
            }
            if ((phase == 4 || phase == 14) && push_window_event(SDL_WINDOWEVENT_SIZE_CHANGED, 1280, 720)) {
                ++injected_resize_events;
            }
            if ((phase == 6 || phase == 16) && push_window_event(SDL_WINDOWEVENT_SIZE_CHANGED, 960, 540)) {
                ++injected_resize_events;
            }
            if (!drop_event_injected && phase == 9) {
                drop_event_injected = push_drop_file_event(media_file);
            }

            player.pumpEvents();
            ++pump_iterations;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        const auto settle_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
        while (std::chrono::steady_clock::now() < settle_deadline &&
               (player.isPlaying() || player.isPaused())) {
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        drop_request_consumed = player.consumeOpenFileRequest(dropped_path);
        auto normalize_path = [](const std::string& value) {
            std::error_code path_ec;
            std::filesystem::path normalized = std::filesystem::weakly_canonical(value, path_ec);
            if (path_ec) {
                path_ec.clear();
                normalized = std::filesystem::absolute(value, path_ec);
            }
            std::string normalized_text = normalized.generic_string();
#if defined(_WIN32)
            normalized_text = toLower(std::move(normalized_text));
#endif
            return normalized_text;
        };
        dropped_path_match = drop_request_consumed &&
                            !dropped_path.empty() &&
                            normalize_path(dropped_path) == normalize_path(media_file);

        after_diag = player.getDiagnosticsSnapshot();
        player.stop();
        player.close();
    }

    const bool linux_platform = after_diag.startup_platform == "Linux";
    const bool fullscreen_ok = injected_fullscreen_toggles > 0;
    const bool resize_ok = injected_resize_events > 0;
    const bool drag_drop_ok = drop_event_injected &&
                              drop_request_consumed &&
                              dropped_path_match;
    const bool render_progress_ok = after_diag.render_frames > before_diag.render_frames;
    const bool stability_ok =
        after_diag.runtime_failure_fail_sessions <= before_diag.runtime_failure_fail_sessions &&
        after_diag.illegal_session_transitions <= before_diag.illegal_session_transitions &&
        after_diag.illegal_run_transitions <= before_diag.illegal_run_transitions &&
        after_diag.illegal_pipeline_transitions <= before_diag.illegal_pipeline_transitions;
    const bool result = linux_platform &&
                        open_ok &&
                        entered_playback_loop &&
                        fullscreen_ok &&
                        resize_ok &&
                        drag_drop_ok &&
                        render_progress_ok &&
                        stability_ok;

    std::cout << "ui-interaction-check.path=" << media_file << std::endl;
    std::cout << "ui-interaction-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "ui-interaction-check.platform=" << after_diag.startup_platform << std::endl;
    std::cout << "ui-interaction-check.platform_ok=" << (linux_platform ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "ui-interaction-check.decoder_backend=" << decoder_backend << std::endl;
    std::cout << "ui-interaction-check.injected_fullscreen_toggles=" << injected_fullscreen_toggles << std::endl;
    std::cout << "ui-interaction-check.injected_resize_events=" << injected_resize_events << std::endl;
    std::cout << "ui-interaction-check.drop_event_injected="
              << (drop_event_injected ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.drop_request_consumed="
              << (drop_request_consumed ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.dropped_path=" << dropped_path << std::endl;
    std::cout << "ui-interaction-check.dropped_path_match="
              << (dropped_path_match ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.render_frames_before=" << before_diag.render_frames << std::endl;
    std::cout << "ui-interaction-check.render_frames_after=" << after_diag.render_frames << std::endl;
    std::cout << "ui-interaction-check.render_progress_ok="
              << (render_progress_ok ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.stability_ok=" << (stability_ok ? "true" : "false") << std::endl;
    std::cout << "ui-interaction-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;
}

[[noreturn]] void runSoftwareVideoDecodeCheckAndExit(const std::string& media_file, int sample_ms = 2000) {

    auto flush_and_exit = [](int code) -> void {

        std::cout.flush();

        std::cerr.flush();

        std::fflush(nullptr);

#if defined(_WIN32)

        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));

#else

        std::_Exit(code);

#endif

        std::abort();

    };



    const auto envFlagEnabled = [](const char* key) {

        if (!key || key[0] == '\0') {

            return false;

        }

#if defined(_WIN32)

        char* raw_value = nullptr;

        size_t raw_len = 0;

        if (_dupenv_s(&raw_value, &raw_len, key) != 0 || !raw_value) {

            return false;

        }

        std::string normalized(raw_value);

        std::free(raw_value);

#else

        const char* value = std::getenv(key);

        if (!value) {

            return false;

        }

        std::string normalized(value);

#endif

        normalized = toLower(std::move(normalized));

        normalized.erase(

            std::remove_if(normalized.begin(), normalized.end(), [](unsigned char ch) { return std::isspace(ch) != 0; }),

            normalized.end());

        return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";

    };



    const bool disable_audio_output = envFlagEnabled("MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO");

    const char* audio_probe_mode = disable_audio_output ? "disabled" : "dummy";

    const ProbeReport probe = collectFileProbeReport(media_file);

    if (sample_ms < 1000) {

        std::cout << "software-video-decode-check.path=" << media_file << std::endl;

        std::cout << "software-video-decode-check.sample_ms=" << sample_ms << std::endl;

        std::cout << "software-video-decode-check.audio_probe_mode=" << audio_probe_mode << std::endl;

        std::cout << "software-video-decode-check.result=FAIL" << std::endl;

        flush_and_exit(2);

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

    const bool probe_ok = probe.overall == "PASS" && probe.width > 0 && probe.height > 0;

    const bool duration_ok = probe.duration <= 0.0 || probe.duration >= sample_seconds + 1.0;



    ScopedEnvOverride force_software_renderer("MVP_RENDERER_BACKEND", "software");

    std::unique_ptr<ScopedEnvOverride> force_audio_driver;

    std::unique_ptr<ScopedEnvOverride> force_disable_audio_output;

    if (disable_audio_output) {

        force_disable_audio_output = std::make_unique<ScopedEnvOverride>("MVP_DISABLE_AUDIO_OUTPUT", "1");

    } else {

        force_audio_driver = std::make_unique<ScopedEnvOverride>("SDL_AUDIODRIVER", "dummy");

    }



    VideoPlayer player;

    player.setPreferHardwareDecode(false);

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

    }



    const bool renderer_ok = renderer_backend == "SoftwareSDL";

    const bool decoder_ok = decoder_backend == "Software";

    const bool decoded_frames_ok = diag.decode_video_ok > 0 && diag.scheduler_video_decoded_frames > 0;

    const bool frame_queue_ok = diag.video_frame_queue_peak_size > 0;

    const bool rendered_ok = diag.render_frames > 0;

    const bool copy_back_free_ok = diag.video_copy_back_frames == 0;

    const bool position_advanced = advanced_seconds > 0.1;

    const bool real_frame_output_ok = decoded_frames_ok && frame_queue_ok && rendered_ok;

    const bool result = probe_ok && duration_ok && open_ok && entered_playback_loop &&

                        renderer_ok && decoder_ok && real_frame_output_ok && copy_back_free_ok;



    std::cout << "software-video-decode-check.path=" << media_file << std::endl;

    std::cout << "software-video-decode-check.sample_ms=" << sample_ms << std::endl;

    std::cout << "software-video-decode-check.audio_probe_mode=" << audio_probe_mode << std::endl;

    std::cout << "software-video-decode-check.probe_overall=" << probe.overall << std::endl;

    std::cout << "software-video-decode-check.probe_width=" << probe.width << std::endl;

    std::cout << "software-video-decode-check.probe_height=" << probe.height << std::endl;

    std::cout << "software-video-decode-check.probe_fps=" << probe.fps << std::endl;

    std::cout << "software-video-decode-check.probe_duration=" << probe.duration << std::endl;

    std::cout << "software-video-decode-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.still_playing_after_window=" << (still_playing_after_window ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.renderer_backend=" << renderer_backend << std::endl;

    std::cout << "software-video-decode-check.decoder_backend=" << decoder_backend << std::endl;

    std::cout << "software-video-decode-check.renderer_ok=" << (renderer_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.decoder_ok=" << (decoder_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "software-video-decode-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "software-video-decode-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "software-video-decode-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "software-video-decode-check.scheduler_clock_policy="
              << schedulerClockPolicyName(diag.scheduler_clock_policy) << std::endl;

    std::cout << "software-video-decode-check.scheduler_audio_master_policy="
              << schedulerAudioMasterPolicyName(diag.scheduler_audio_master_policy) << std::endl;

    std::cout << "software-video-decode-check.scheduler_ended_policy="
              << schedulerEndedPolicyName(diag.scheduler_ended_policy) << std::endl;

    std::cout << "software-video-decode-check.scheduler_audio_buffered_seconds="
              << diag.scheduler_audio_buffered_seconds << std::endl;

    std::cout << "software-video-decode-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "software-video-decode-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "software-video-decode-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "software-video-decode-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "software-video-decode-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "software-video-decode-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "software-video-decode-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "software-video-decode-check.start_position=" << start_position << std::endl;

    std::cout << "software-video-decode-check.end_position=" << end_position << std::endl;

    std::cout << "software-video-decode-check.advanced_seconds=" << advanced_seconds << std::endl;

    std::cout << "software-video-decode-check.advance_ratio=" << advance_ratio << std::endl;

    std::cout << "software-video-decode-check.position_advanced=" << (position_advanced ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.demux_video_packets=" << diag.demux_video_packets << std::endl;

    std::cout << "software-video-decode-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "software-video-decode-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

    std::cout << "software-video-decode-check.decode_video_ok=" << diag.decode_video_ok << std::endl;

    std::cout << "software-video-decode-check.stale_video_packets_dropped=" << diag.stale_video_packets_dropped << std::endl;

    std::cout << "software-video-decode-check.stale_video_frames_dropped=" << diag.stale_video_frames_dropped << std::endl;

    std::cout << "software-video-decode-check.stale_render_frames_dropped=" << diag.stale_render_frames_dropped << std::endl;

    std::cout << "software-video-decode-check.video_packet_dequeue_count=" << diag.video_packet_dequeue_count << std::endl;

    std::cout << "software-video-decode-check.video_send_packet_ok=" << diag.video_send_packet_ok << std::endl;

    std::cout << "software-video-decode-check.video_send_packet_last_ret=" << diag.video_send_packet_last_ret << std::endl;

    std::cout << "software-video-decode-check.decode_video_send_eagain=" << diag.decode_video_send_eagain << std::endl;

    std::cout << "software-video-decode-check.video_decoder_drain_signals=" << diag.video_decoder_drain_signals << std::endl;

    std::cout << "software-video-decode-check.scheduler_video_decoded_frames=" << diag.scheduler_video_decoded_frames << std::endl;

    std::cout << "software-video-decode-check.render_frames=" << diag.render_frames << std::endl;

    std::cout << "software-video-decode-check.video_frame_queue_peak_size=" << diag.video_frame_queue_peak_size << std::endl;

    std::cout << "software-video-decode-check.video_frame_queue_capacity=" << diag.video_frame_queue_capacity << std::endl;

    std::cout << "software-video-decode-check.video_frame_queue_push_timeouts=" << diag.video_frame_queue_push_timeouts << std::endl;

    std::cout << "software-video-decode-check.video_copy_back_frames=" << diag.video_copy_back_frames << std::endl;

    std::cout << "software-video-decode-check.video_swscale_frames=" << diag.video_swscale_frames << std::endl;

    std::cout << "software-video-decode-check.scheduler_late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "software-video-decode-check.scheduler_wait_events=" << diag.scheduler_wait_events << std::endl;

    std::cout << "software-video-decode-check.runtime_failure_stop_requests="
              << diag.runtime_failure_stop_requests << std::endl;

    std::cout << "software-video-decode-check.runtime_failure_fail_sessions="
              << diag.runtime_failure_fail_sessions << std::endl;

    std::cout << "software-video-decode-check.illegal_session_transitions="
              << diag.illegal_session_transitions << std::endl;

    std::cout << "software-video-decode-check.illegal_run_transitions="
              << diag.illegal_run_transitions << std::endl;

    std::cout << "software-video-decode-check.illegal_pipeline_transitions="
              << diag.illegal_pipeline_transitions << std::endl;

    std::cout << "software-video-decode-check.decoded_frames_ok=" << (decoded_frames_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.frame_queue_ok=" << (frame_queue_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.rendered_ok=" << (rendered_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.copy_back_free_ok=" << (copy_back_free_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.real_frame_output_ok=" << (real_frame_output_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-decode-check.result=" << (result ? "PASS" : "FAIL") << std::endl;



    flush_and_exit(result ? 0 : 2);

}



/// 绕开 PlayerCore/Scheduler/Renderer，只验证 software video decode 首个 `send_packet` 是否能按时返回。

[[noreturn]] void runSoftwareVideoSendProbeAndExit(const std::string& media_file, int send_timeout_ms = 1500) {

    struct ProbeAvPacketDeleter {

        void operator()(AVPacket* packet) const noexcept {

            if (packet) {

                av_packet_free(&packet);

            }

        }

    };

    using ProbePacketPtr = std::unique_ptr<AVPacket, ProbeAvPacketDeleter>;



    auto flush_and_exit = [](int code) -> void {

        std::cout.flush();

        std::cerr.flush();

        std::fflush(nullptr);

#if defined(_WIN32)

        TerminateProcess(GetCurrentProcess(), static_cast<UINT>(code));

#else

        std::_Exit(code);

#endif

        std::abort();

    };



    const ProbeReport probe = collectFileProbeReport(media_file);

    if (send_timeout_ms < 100) {

        std::cout << "software-video-send-probe.path=" << media_file << std::endl;

        std::cout << "software-video-send-probe.send_timeout_ms=" << send_timeout_ms << std::endl;

        std::cout << "software-video-send-probe.result=FAIL" << std::endl;

        flush_and_exit(2);

    }



    Demuxer demuxer;

    const bool open_ok = demuxer.open(media_file);

    const MediaInfo& info = demuxer.getMediaInfo();

    const bool has_video_stream = open_ok && info.video_stream_idx >= 0;

    AVFormatContext* fmt_ctx = open_ok ? demuxer.getFormatContext() : nullptr;

    AVStream* video_stream = (has_video_stream && fmt_ctx) ? fmt_ctx->streams[info.video_stream_idx] : nullptr;

    const AVCodec* codec =

        (video_stream && video_stream->codecpar) ? avcodec_find_decoder(video_stream->codecpar->codec_id) : nullptr;



    AVCodecContext* codec_ctx = nullptr;

    bool codec_ctx_alloc_ok = false;

    bool codec_ctx_config_ok = false;

    bool codec_open_ok = false;

    int open_ret = std::numeric_limits<int>::min();



    if (codec) {

        codec_ctx = avcodec_alloc_context3(codec);

        codec_ctx_alloc_ok = codec_ctx != nullptr;

        if (codec_ctx_alloc_ok &&

            avcodec_parameters_to_context(codec_ctx, video_stream->codecpar) >= 0) {

            codec_ctx->thread_count = 1;

            codec_ctx->thread_type = 0;

            codec_ctx->pkt_timebase = video_stream->time_base;

            codec_ctx->flags2 |= AV_CODEC_FLAG2_FAST;

            codec_ctx_config_ok = true;

            open_ret = avcodec_open2(codec_ctx, codec, nullptr);

            codec_open_ok = open_ret >= 0;

        }

    }



    AVPacket* packet = av_packet_alloc();

    bool packet_alloc_ok = packet != nullptr;

    bool packet_found = false;

    bool packet_queue_push_ok = false;

    bool packet_queue_pop_ok = false;

    uint64_t packets_read_total = 0;

    uint64_t video_packets_seen = 0;



    if (packet_alloc_ok && codec_open_ok) {

        while (demuxer.readPacket(packet)) {

            ++packets_read_total;

            if (packet->stream_index != info.video_stream_idx) {

                av_packet_unref(packet);

                continue;

            }

            ++video_packets_seen;

            packet_found = true;

            break;

        }

    }



    if (packet_found) {

        ThreadSafeQueue<ProbePacketPtr> packet_queue(1);

        ProbePacketPtr queued_packet(packet);

        packet = nullptr;

        packet_queue_push_ok = packet_queue.push(std::move(queued_packet), 20);

        if (packet_queue_push_ok) {

            ProbePacketPtr popped_packet;

            packet_queue_pop_ok = packet_queue.pop(popped_packet, 20) && popped_packet != nullptr;

            if (packet_queue_pop_ok) {

                packet = popped_packet.release();

            }

        }

        if (!packet_queue_pop_ok) {

            packet_found = false;

        }

    }



    int send_ret = std::numeric_limits<int>::min();

    int receive_ret = std::numeric_limits<int>::min();

    bool receive_got_frame = false;

    int pre_send_receive_ret = std::numeric_limits<int>::min();

    std::atomic<uint64_t> read_ahead_packets{0};

    std::atomic<bool> read_ahead_done{false};

    std::atomic<bool> send_started{false};

    std::atomic<bool> send_completed{false};

    std::atomic<int> send_ret_atomic{std::numeric_limits<int>::min()};

    std::atomic<int64_t> send_enter_ms{0};

    std::atomic<int64_t> send_exit_ms{0};

    bool send_timed_out = false;



    std::thread read_ahead_thread;

    std::thread send_thread;

    if (packet_found) {

        read_ahead_thread = std::thread([&] {

            AVPacket* read_ahead_packet = av_packet_alloc();

            if (!read_ahead_packet) {

                read_ahead_done.store(true);

                return;

            }



            while (read_ahead_packets.load() < 512 && demuxer.readPacket(read_ahead_packet)) {

                read_ahead_packets.fetch_add(1);

                av_packet_unref(read_ahead_packet);

            }



            av_packet_free(&read_ahead_packet);

            read_ahead_done.store(true);

        });



        const auto read_ahead_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(100);

        while (std::chrono::steady_clock::now() < read_ahead_deadline &&

               read_ahead_packets.load() < 64 &&

               !read_ahead_done.load()) {

            std::this_thread::sleep_for(std::chrono::milliseconds(2));

        }



        AVFrame* pre_send_frame = av_frame_alloc();

        if (pre_send_frame) {

            pre_send_receive_ret = avcodec_receive_frame(codec_ctx, pre_send_frame);

            av_frame_free(&pre_send_frame);

        }



        send_thread = std::thread([&] {

            send_started.store(true);

            send_enter_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(

                                    std::chrono::steady_clock::now().time_since_epoch())

                                    .count());

            const int ret = avcodec_send_packet(codec_ctx, packet);

            send_ret_atomic.store(ret);

            send_exit_ms.store(std::chrono::duration_cast<std::chrono::milliseconds>(

                                   std::chrono::steady_clock::now().time_since_epoch())

                                   .count());

            send_completed.store(true);

        });



        const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(send_timeout_ms);

        while (std::chrono::steady_clock::now() < deadline && !send_completed.load()) {

            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        }



        if (send_completed.load()) {

            send_thread.join();

            send_ret = send_ret_atomic.load();



            AVFrame* frame = av_frame_alloc();

            if (frame) {

                receive_ret = avcodec_receive_frame(codec_ctx, frame);

                receive_got_frame = receive_ret >= 0;

                av_frame_free(&frame);

            }

        } else {

            send_timed_out = true;

            send_thread.detach();

        }

    }



    if (read_ahead_thread.joinable()) {

        read_ahead_thread.join();

    }



    const int64_t send_elapsed_ms =

        send_completed.load() && send_enter_ms.load() > 0 && send_exit_ms.load() >= send_enter_ms.load()

            ? (send_exit_ms.load() - send_enter_ms.load())

            : -1;

    const bool result = probe.overall == "PASS" && open_ok && has_video_stream && codec && codec_open_ok &&

                        packet_found && send_completed.load() && send_ret >= 0;



    std::cout << "software-video-send-probe.path=" << media_file << std::endl;

    std::cout << "software-video-send-probe.send_timeout_ms=" << send_timeout_ms << std::endl;

    std::cout << "software-video-send-probe.probe_overall=" << probe.overall << std::endl;

    std::cout << "software-video-send-probe.open_ok=" << (open_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-send-probe.has_video_stream=" << (has_video_stream ? "true" : "false") << std::endl;

    std::cout << "software-video-send-probe.video_stream_index=" << info.video_stream_idx << std::endl;

    std::cout << "software-video-send-probe.codec_name=" << (codec && codec->name ? codec->name : "none")

              << std::endl;

    std::cout << "software-video-send-probe.codec_ctx_alloc_ok=" << (codec_ctx_alloc_ok ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.codec_ctx_config_ok=" << (codec_ctx_config_ok ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.codec_open_ok=" << (codec_open_ok ? "true" : "false") << std::endl;

    std::cout << "software-video-send-probe.codec_open_ret=" << open_ret << std::endl;

    std::cout << "software-video-send-probe.codec_open_message=" << avErrorToString(open_ret) << std::endl;

    std::cout << "software-video-send-probe.thread_count=" << (codec_ctx ? codec_ctx->thread_count : -1)

              << std::endl;

    std::cout << "software-video-send-probe.thread_type=" << (codec_ctx ? codec_ctx->thread_type : -1)

              << std::endl;

    std::cout << "software-video-send-probe.packets_read_total=" << packets_read_total << std::endl;

    std::cout << "software-video-send-probe.video_packets_seen=" << video_packets_seen << std::endl;

    std::cout << "software-video-send-probe.packet_found=" << (packet_found ? "true" : "false") << std::endl;

    std::cout << "software-video-send-probe.packet_queue_push_ok=" << (packet_queue_push_ok ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.packet_queue_pop_ok=" << (packet_queue_pop_ok ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.read_ahead_packets=" << read_ahead_packets.load() << std::endl;

    std::cout << "software-video-send-probe.read_ahead_done=" << (read_ahead_done.load() ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.packet_size=" << (packet_found ? packet->size : -1) << std::endl;

    std::cout << "software-video-send-probe.packet_pts=" << (packet_found ? packet->pts : AV_NOPTS_VALUE)

              << std::endl;

    std::cout << "software-video-send-probe.packet_dts=" << (packet_found ? packet->dts : AV_NOPTS_VALUE)

              << std::endl;

    std::cout << "software-video-send-probe.pre_send_receive_ret=" << pre_send_receive_ret << std::endl;

    std::cout << "software-video-send-probe.pre_send_receive_message="

              << avErrorToString(pre_send_receive_ret) << std::endl;

    std::cout << "software-video-send-probe.send_started=" << (send_started.load() ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.send_completed=" << (send_completed.load() ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.send_timed_out=" << (send_timed_out ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.send_elapsed_ms=" << send_elapsed_ms << std::endl;

    std::cout << "software-video-send-probe.send_ret="

              << (send_completed.load() ? send_ret_atomic.load() : std::numeric_limits<int>::min()) << std::endl;

    std::cout << "software-video-send-probe.send_message="

              << avErrorToString(send_completed.load() ? send_ret_atomic.load() : std::numeric_limits<int>::min())

              << std::endl;

    std::cout << "software-video-send-probe.receive_ret=" << receive_ret << std::endl;

    std::cout << "software-video-send-probe.receive_message=" << avErrorToString(receive_ret) << std::endl;

    std::cout << "software-video-send-probe.receive_got_frame=" << (receive_got_frame ? "true" : "false")

              << std::endl;

    std::cout << "software-video-send-probe.result=" << (result ? "PASS" : "FAIL") << std::endl;



    if (!send_timed_out) {

        if (packet) {

            av_packet_free(&packet);

        }

        if (codec_ctx) {

            avcodec_free_context(&codec_ctx);

        }

    }



    flush_and_exit(result ? 0 : 2);

}



/// 面向 1080p60 场景的实时播放回归检查。

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

    const bool demux_drops_ok = diag.demux_queue_drop_packets == 0;

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

    std::cout << "1080p60-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "1080p60-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "1080p60-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "1080p60-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "1080p60-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "1080p60-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "1080p60-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "1080p60-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "1080p60-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "1080p60-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "1080p60-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "1080p60-check.start_position=" << start_position << std::endl;

    std::cout << "1080p60-check.end_position=" << end_position << std::endl;

    std::cout << "1080p60-check.advanced_seconds=" << advanced_seconds << std::endl;

    std::cout << "1080p60-check.advance_ratio=" << advance_ratio << std::endl;

    std::cout << "1080p60-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "1080p60-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;

    std::cout << "1080p60-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "1080p60-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

    std::cout << "1080p60-check.demux_drops_ok=" << (demux_drops_ok ? "true" : "false") << std::endl;

    std::cout << "1080p60-check.decode_video_ok=" << diag.decode_video_ok << std::endl;

    std::cout << "1080p60-check.render_frames=" << diag.render_frames << std::endl;

    std::cout << "1080p60-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 面向 4K 场景的播放压力与后端可用性检查。

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

    std::cout << "4k-playback-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "4k-playback-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "4k-playback-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "4k-playback-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "4k-playback-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "4k-playback-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "4k-playback-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "4k-playback-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "4k-playback-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "4k-playback-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "4k-playback-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "4k-playback-check.start_position=" << start_position << std::endl;

    std::cout << "4k-playback-check.end_position=" << end_position << std::endl;

    std::cout << "4k-playback-check.advanced_seconds=" << advanced_seconds << std::endl;

    std::cout << "4k-playback-check.advance_ratio=" << advance_ratio << std::endl;

    std::cout << "4k-playback-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "4k-playback-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;

    std::cout << "4k-playback-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;

    std::cout << "4k-playback-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "4k-playback-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

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



/// 面向高码率媒体的稳定性检查。

bool runHighBitrateCheck(const std::string& media_file, int sample_ms = 3000) {

    const ProbeReport probe = collectFileProbeReport(media_file);

    const int64_t format_bitrate_bps = collectFormatBitrateBitsPerSecond(media_file);

    if (sample_ms < 1500) {

        std::cout << "high-bitrate-check.path=" << media_file << std::endl;

        std::cout << "high-bitrate-check.sample_ms=" << sample_ms << std::endl;

        std::cout << "high-bitrate-check.result=FAIL" << std::endl;

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

    const bool probe_ok = probe.overall == "PASS";

    const bool bitrate_ok = format_bitrate_bps >= 80000000LL;

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

    const bool demux_drops_ok = diag.demux_queue_drop_packets == 0;

    const bool result = probe_ok && bitrate_ok && duration_ok && open_ok && entered_playback_loop &&

                        still_playing_after_window && progress_ok && late_drops_ok && demux_drops_ok;



    std::cout << "high-bitrate-check.path=" << media_file << std::endl;

    std::cout << "high-bitrate-check.sample_ms=" << sample_ms << std::endl;

    std::cout << "high-bitrate-check.probe_overall=" << probe.overall << std::endl;

    std::cout << "high-bitrate-check.probe_width=" << probe.width << std::endl;

    std::cout << "high-bitrate-check.probe_height=" << probe.height << std::endl;

    std::cout << "high-bitrate-check.probe_fps=" << probe.fps << std::endl;

    std::cout << "high-bitrate-check.probe_duration=" << probe.duration << std::endl;

    std::cout << "high-bitrate-check.format_bitrate_bps=" << format_bitrate_bps << std::endl;

    std::cout << "high-bitrate-check.bitrate_ok=" << (bitrate_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.still_playing_after_window=" << (still_playing_after_window ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.renderer_backend=" << renderer_backend << std::endl;

    std::cout << "high-bitrate-check.decoder_backend=" << decoder_backend << std::endl;

    std::cout << "high-bitrate-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "high-bitrate-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "high-bitrate-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "high-bitrate-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "high-bitrate-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "high-bitrate-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "high-bitrate-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "high-bitrate-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "high-bitrate-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "high-bitrate-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "high-bitrate-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "high-bitrate-check.start_position=" << start_position << std::endl;

    std::cout << "high-bitrate-check.end_position=" << end_position << std::endl;

    std::cout << "high-bitrate-check.advanced_seconds=" << advanced_seconds << std::endl;

    std::cout << "high-bitrate-check.advance_ratio=" << advance_ratio << std::endl;

    std::cout << "high-bitrate-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "high-bitrate-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;

    std::cout << "high-bitrate-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "high-bitrate-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

    std::cout << "high-bitrate-check.demux_drops_ok=" << (demux_drops_ok ? "true" : "false") << std::endl;

    std::cout << "high-bitrate-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 长时间播放窗口检查，用于验证持续推进和丢帧控制。

bool runLongPlaybackCheck(const std::string& media_file, int sample_ms = 10000) {

    const ProbeReport probe = collectFileProbeReport(media_file);

    if (sample_ms < 5000) {

        std::cout << "long-playback-check.path=" << media_file << std::endl;

        std::cout << "long-playback-check.sample_ms=" << sample_ms << std::endl;

        std::cout << "long-playback-check.result=FAIL" << std::endl;

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

    const bool probe_ok = probe.overall == "PASS";

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



    const double min_advance_seconds = std::max(2.0, sample_seconds * 0.85);

    const bool progress_ok = advanced_seconds >= min_advance_seconds;

    const bool late_drops_ok = diag.scheduler_late_drops == 0;

    const bool demux_drops_ok = diag.demux_queue_drop_packets == 0;

    const bool result = probe_ok && duration_ok && open_ok && entered_playback_loop && still_playing_after_window &&

                        progress_ok && late_drops_ok && demux_drops_ok;



    std::cout << "long-playback-check.path=" << media_file << std::endl;

    std::cout << "long-playback-check.sample_ms=" << sample_ms << std::endl;

    std::cout << "long-playback-check.probe_overall=" << probe.overall << std::endl;

    std::cout << "long-playback-check.probe_duration=" << probe.duration << std::endl;

    std::cout << "long-playback-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.still_playing_after_window=" << (still_playing_after_window ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.renderer_backend=" << renderer_backend << std::endl;

    std::cout << "long-playback-check.decoder_backend=" << decoder_backend << std::endl;

    std::cout << "long-playback-check.audio_output_initialized="

              << (diag.audio_output_initialized ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.video_only_fallback="

              << (diag.video_only_fallback ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.clock_source=" << clockSourceName(diag.clock_source) << std::endl;

    std::cout << "long-playback-check.audio_device_open_attempted="
              << (diag.audio_device_open_attempted ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.audio_init_latency_ms=" << diag.audio_init_latency_ms << std::endl;

    std::cout << "long-playback-check.audio_init_strategy=" << diag.audio_init_strategy << std::endl;

    std::cout << "long-playback-check.audio_init_detail=" << diag.audio_init_detail << std::endl;

    std::cout << "long-playback-check.timeline_serial=" << diag.timeline_serial << std::endl;

    std::cout << "long-playback-check.pending_seek_serial=" << diag.pending_seek_serial << std::endl;

    std::cout << "long-playback-check.ended_reason=" << endedReasonName(diag.ended_reason) << std::endl;

    std::cout << "long-playback-check.video_packet_queue_generation=" << diag.video_packet_queue_generation << std::endl;

    std::cout << "long-playback-check.audio_packet_queue_generation=" << diag.audio_packet_queue_generation << std::endl;

    std::cout << "long-playback-check.video_frame_queue_generation=" << diag.video_frame_queue_generation << std::endl;

    std::cout << "long-playback-check.audio_frame_queue_generation=" << diag.audio_frame_queue_generation << std::endl;

    std::cout << "long-playback-check.start_position=" << start_position << std::endl;

    std::cout << "long-playback-check.end_position=" << end_position << std::endl;

    std::cout << "long-playback-check.advanced_seconds=" << advanced_seconds << std::endl;

    std::cout << "long-playback-check.advance_ratio=" << advance_ratio << std::endl;

    std::cout << "long-playback-check.progress_ok=" << (progress_ok ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.duration_ok=" << (duration_ok ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.late_drops=" << diag.scheduler_late_drops << std::endl;

    std::cout << "long-playback-check.late_drops_ok=" << (late_drops_ok ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.demux_dropped_packets=" << diag.demux_dropped_packets << std::endl;

    std::cout << "long-playback-check.demux_ignored_packets=" << diag.demux_ignored_packets << std::endl;

    std::cout << "long-playback-check.demux_queue_drop_packets=" << diag.demux_queue_drop_packets << std::endl;

    std::cout << "long-playback-check.demux_drops_ok=" << (demux_drops_ok ? "true" : "false") << std::endl;

    std::cout << "long-playback-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 验证插件加载、滤镜注册和卸载清理流程。

bool runPluginCheck(const char* program_name, const std::string& plugin_input = std::string{}) {

    const std::filesystem::path default_plugin_dir = [&]() {

        if (program_name && *program_name) {

            const std::filesystem::path executable_path(program_name);

            if (executable_path.has_parent_path()) {

                return executable_path.parent_path() / "plugins";

            }

        }

        return std::filesystem::current_path() / "plugins";

    }();



    const std::filesystem::path plugin_path = plugin_input.empty() ? default_plugin_dir : std::filesystem::path(plugin_input);

    auto& registry = filters::FilterRegistry::instance();

    const auto video_filters_before = registry.getVideoFilterNames();

    const auto audio_filters_before = registry.getAudioFilterNames();



    plugin::PluginManager manager;

    std::vector<std::string> errors;

    size_t loaded_count = 0;

    std::error_code ec;

    if (std::filesystem::is_regular_file(plugin_path, ec)) {

        std::string error_message;

        if (manager.loadPlugin(plugin_path, &error_message)) {

            loaded_count = 1;

        } else if (!error_message.empty()) {

            errors.push_back(error_message);

        }

    } else {

        loaded_count = manager.loadPluginsFromDirectory(plugin_path, &errors);

    }



    const auto loaded_plugins = manager.listPlugins();

    const auto video_filters_loaded = registry.getVideoFilterNames();

    const auto audio_filters_loaded = registry.getAudioFilterNames();



    const bool sample_plugin_loaded = std::any_of(loaded_plugins.begin(), loaded_plugins.end(), [](const plugin::PluginDescriptor& descriptor) {

        return descriptor.id == "sample_logger_plugin";

    });

    const bool sample_video_filter_registered =

        !containsString(video_filters_before, "sample_identity") && containsString(video_filters_loaded, "sample_identity");



    manager.unloadAll(&errors);



    const auto video_filters_unloaded = registry.getVideoFilterNames();

    const auto audio_filters_unloaded = registry.getAudioFilterNames();

    const bool sample_video_filter_unloaded = !containsString(video_filters_unloaded, "sample_identity");



    const bool result = loaded_count > 0 && sample_plugin_loaded && sample_video_filter_registered && sample_video_filter_unloaded;



    std::vector<std::string> plugin_ids;

    plugin_ids.reserve(loaded_plugins.size());

    for (const auto& descriptor : loaded_plugins) {

        plugin_ids.push_back(descriptor.id + "@" + descriptor.version);

    }



    std::cout << "plugin-check.path=" << plugin_path.generic_string() << std::endl;

    std::cout << "plugin-check.loaded_count=" << loaded_count << std::endl;

    std::cout << "plugin-check.plugin_ids=" << (plugin_ids.empty() ? std::string("none") : joinTopN(plugin_ids, 16)) << std::endl;

    std::cout << "plugin-check.video_filters_before=" << video_filters_before.size() << std::endl;

    std::cout << "plugin-check.video_filters_loaded=" << video_filters_loaded.size() << std::endl;

    std::cout << "plugin-check.video_filters_unloaded=" << video_filters_unloaded.size() << std::endl;

    std::cout << "plugin-check.audio_filters_before=" << audio_filters_before.size() << std::endl;

    std::cout << "plugin-check.audio_filters_loaded=" << audio_filters_loaded.size() << std::endl;

    std::cout << "plugin-check.audio_filters_unloaded=" << audio_filters_unloaded.size() << std::endl;

    std::cout << "plugin-check.sample_plugin_loaded=" << (sample_plugin_loaded ? "true" : "false") << std::endl;

    std::cout << "plugin-check.sample_video_filter_registered=" << (sample_video_filter_registered ? "true" : "false") << std::endl;

    std::cout << "plugin-check.sample_video_filter_unloaded=" << (sample_video_filter_unloaded ? "true" : "false") << std::endl;

    std::cout << "plugin-check.errors=" << (errors.empty() ? std::string("none") : joinTopN(errors, 8)) << std::endl;

    std::cout << "plugin-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 流媒体清单类型。

enum class StreamingManifestKind {

    Unknown,

    Hls,

    Dash,

};



/// 流媒体码率候选项；统一承载 HLS/DASH 变体下载信息。

struct StreamingVariantCandidate {

    std::string id;

    int bandwidth{0};

    std::string descriptor_url;

    std::string initialization_url;

    std::vector<std::string> segment_urls;

};



std::string joinIntegers(const std::vector<int>& values, size_t limit = 16) {

    std::ostringstream oss;

    const size_t count = std::min(limit, values.size());

    for (size_t index = 0; index < count; ++index) {

        if (index > 0) {

            oss << ", ";

        }

        oss << values[index];

    }

    if (values.size() > limit) {

        oss << ", ...";

    }

    return oss.str();

}



bool parseCsvIntegers(const std::string& text, std::vector<int>& values) {

    values.clear();

    std::stringstream input(text);

    std::string token;

    while (std::getline(input, token, ',')) {

        const size_t begin = token.find_first_not_of(" \t\r\n");

        if (begin == std::string::npos) {

            continue;

        }

        const size_t end = token.find_last_not_of(" \t\r\n");

        const std::string trimmed = token.substr(begin, end - begin + 1);

        char* parse_end = nullptr;

        const long parsed = std::strtol(trimmed.c_str(), &parse_end, 10);

        if (parse_end == trimmed.c_str() || *parse_end != '\0') {

            values.clear();

            return false;

        }

        values.push_back(static_cast<int>(parsed));

    }

    return !values.empty();

}



std::string urlExtension(const std::string& url) {

    std::string cleaned = url;

    const size_t query_pos = cleaned.find_first_of("?#");

    if (query_pos != std::string::npos) {

        cleaned = cleaned.substr(0, query_pos);

    }

    const size_t slash_pos = cleaned.find_last_of('/');

    const size_t dot_pos = cleaned.find_last_of('.');

    if (dot_pos == std::string::npos || (slash_pos != std::string::npos && dot_pos < slash_pos)) {

        return {};

    }

    return toLower(cleaned.substr(dot_pos + 1));

}



StreamingManifestKind detectManifestKind(const std::string& manifest_url, const std::string& manifest_text) {

    if (manifest_text.find("#EXTM3U") != std::string::npos) {

        return StreamingManifestKind::Hls;

    }

    if (manifest_text.find("<MPD") != std::string::npos || manifest_text.find("<mpd") != std::string::npos) {

        return StreamingManifestKind::Dash;

    }



    const std::string extension = urlExtension(manifest_url);

    if (extension == "m3u8") {

        return StreamingManifestKind::Hls;

    }

    if (extension == "mpd") {

        return StreamingManifestKind::Dash;

    }

    return StreamingManifestKind::Unknown;

}



std::string manifestKindName(StreamingManifestKind kind) {

    switch (kind) {

    case StreamingManifestKind::Hls:

        return "HLS";

    case StreamingManifestKind::Dash:

        return "DASH";

    default:

        return "UNKNOWN";

    }

}



/// 下载一个 URL，并记录缓冲峰值、读取块数等观测指标。

bool downloadBufferedUrl(

    const std::string& url,

    size_t target_buffer_bytes,

    size_t& downloaded_bytes,

    size_t& max_buffered_bytes,

    size_t& chunk_reads,

    std::string& error_out) {

    downloaded_bytes = 0;

    max_buffered_bytes = 0;

    chunk_reads = 0;

    error_out.clear();



    streaming::HttpStreamDownloader downloader;

    if (!downloader.open(url)) {

        error_out = downloader.lastError();

        return false;

    }



    while (!downloader.eof() || downloader.bufferedBytes() > 0) {

        if (!downloader.prefetch(target_buffer_bytes, target_buffer_bytes)) {

            error_out = downloader.lastError();

            downloader.close();

            return false;

        }

        max_buffered_bytes = std::max(max_buffered_bytes, downloader.bufferedBytes());

        const auto chunk = downloader.consumeBuffered(target_buffer_bytes);

        if (chunk.empty()) {

            continue;

        }

        downloaded_bytes += chunk.size();

        ++chunk_reads;

    }



    downloader.close();

    if (downloaded_bytes == 0) {

        error_out = "download produced no bytes";

        return false;

    }

    return true;

}



/// 从 HLS 主/媒体清单构建可下载的变体候选列表。

bool buildHlsVariantCandidates(

    const std::string& manifest_url,

    const std::string& manifest_text,

    std::vector<StreamingVariantCandidate>& variants_out,

    std::string& error_out) {

    variants_out.clear();

    error_out.clear();



    streaming::HlsManifest manifest{};

    if (!streaming::HlsManifestParser::parse(manifest_text, manifest)) {

        error_out = "failed to parse hls manifest";

        return false;

    }



    if (!manifest.variants.empty()) {

        for (size_t index = 0; index < manifest.variants.size(); ++index) {

            const auto& variant = manifest.variants[index];

            const std::string playlist_url = resolveUrl(manifest_url, variant.uri);



            std::string playlist_text;

            if (!readUrlText(playlist_url, playlist_text, error_out)) {

                return false;

            }



            streaming::HlsManifest media_manifest{};

            if (!streaming::HlsManifestParser::parse(playlist_text, media_manifest) || media_manifest.is_master ||

                media_manifest.segment_urls.empty()) {

                error_out = "failed to parse hls media playlist";

                return false;

            }



            StreamingVariantCandidate candidate{};

            candidate.id = !variant.resolution.empty()

                               ? variant.resolution

                               : (variant.bandwidth > 0 ? ("hls_" + std::to_string(variant.bandwidth))

                                                        : ("hls_variant_" + std::to_string(index + 1)));

            candidate.bandwidth = variant.bandwidth;

            candidate.descriptor_url = playlist_url;

            for (const auto& segment_url : media_manifest.segment_urls) {

                candidate.segment_urls.push_back(resolveUrl(playlist_url, segment_url));

            }

            variants_out.push_back(std::move(candidate));

        }

        return !variants_out.empty();

    }



    if (manifest.segment_urls.empty()) {

        error_out = "hls manifest did not contain variants or segments";

        return false;

    }



    StreamingVariantCandidate direct_candidate{};

    direct_candidate.id = "hls_single_variant";

    direct_candidate.descriptor_url = manifest_url;

    for (const auto& segment_url : manifest.segment_urls) {

        direct_candidate.segment_urls.push_back(resolveUrl(manifest_url, segment_url));

    }

    variants_out.push_back(std::move(direct_candidate));

    return true;

}



/// 从 DASH MPD 清单构建可下载的表示候选列表。

bool buildDashVariantCandidates(

    const std::string& manifest_url,

    const std::string& manifest_text,

    std::vector<StreamingVariantCandidate>& variants_out,

    std::string& error_out) {

    variants_out.clear();

    error_out.clear();



    streaming::DashManifest manifest{};

    if (!streaming::DashManifestParser::parse(manifest_text, manifest)) {

        error_out = "failed to parse dash manifest";

        return false;

    }



    for (size_t index = 0; index < manifest.representations.size(); ++index) {

        const auto& representation = manifest.representations[index];

        StreamingVariantCandidate candidate{};

        candidate.id = !representation.id.empty()

                           ? representation.id

                           : (representation.bandwidth > 0 ? ("dash_" + std::to_string(representation.bandwidth))

                                                           : ("dash_representation_" + std::to_string(index + 1)));

        candidate.bandwidth = representation.bandwidth;

        candidate.descriptor_url = representation.base_url.empty() ? manifest_url : resolveUrl(manifest_url, representation.base_url);

        if (!representation.initialization_url.empty()) {

            candidate.initialization_url = resolveUrl(candidate.descriptor_url, representation.initialization_url);

        }

        for (const auto& segment_url : representation.segment_urls) {

            candidate.segment_urls.push_back(resolveUrl(candidate.descriptor_url, segment_url));

        }

        variants_out.push_back(std::move(candidate));

    }



    if (variants_out.empty()) {

        error_out = "dash manifest did not contain representations";

        return false;

    }

    return true;

}



/// 下载选定变体的初始化段和若干媒体分片。

bool downloadVariantSelection(

    const StreamingVariantCandidate& variant,

    int segment_limit,

    size_t target_buffer_bytes,

    std::set<std::string>& initialized_urls,

    size_t& initialization_count,

    size_t& segment_count,

    size_t& buffered_bytes,

    size_t& max_buffered_bytes,

    size_t& chunk_reads,

    std::vector<std::string>& downloaded_urls,

    std::string& error_out) {

    if (!variant.initialization_url.empty() && initialized_urls.insert(variant.initialization_url).second) {

        size_t init_bytes = 0;

        size_t init_max_buffer = 0;

        size_t init_reads = 0;

        if (!downloadBufferedUrl(variant.initialization_url, target_buffer_bytes, init_bytes, init_max_buffer, init_reads, error_out)) {

            return false;

        }

        ++initialization_count;

        buffered_bytes += init_bytes;

        max_buffered_bytes = std::max(max_buffered_bytes, init_max_buffer);

        chunk_reads += init_reads;

        downloaded_urls.push_back(variant.initialization_url);

    }



    const size_t segments_to_download = std::min(static_cast<size_t>(segment_limit), variant.segment_urls.size());

    if (segments_to_download == 0) {

        error_out = "variant contains no segments";

        return false;

    }



    for (size_t index = 0; index < segments_to_download; ++index) {

        const std::string& segment_url = variant.segment_urls[index];

        size_t segment_bytes = 0;

        size_t segment_max_buffer = 0;

        size_t segment_reads = 0;

        if (!downloadBufferedUrl(segment_url, target_buffer_bytes, segment_bytes, segment_max_buffer, segment_reads, error_out)) {

            return false;

        }

        ++segment_count;

        buffered_bytes += segment_bytes;

        max_buffered_bytes = std::max(max_buffered_bytes, segment_max_buffer);

        chunk_reads += segment_reads;

        downloaded_urls.push_back(segment_url);

    }



    return true;

}



/// 验证流媒体清单解析、远端读取与基础缓冲行为。

bool runStreamingBufferCheck(const std::string& playlist_url, int segment_limit = 3, int target_buffer_bytes = 256) {

    if (segment_limit <= 0 || target_buffer_bytes <= 0) {

        std::cout << "streaming-buffer-check.playlist_url=" << playlist_url << std::endl;

        std::cout << "streaming-buffer-check.result=FAIL" << std::endl;

        return false;

    }



    std::string manifest_text;

    std::string manifest_error;

    const bool manifest_download_ok = readUrlText(playlist_url, manifest_text, manifest_error);



    streaming::HlsManifest manifest{};

    const bool manifest_parse_ok = manifest_download_ok && streaming::HlsManifestParser::parse(manifest_text, manifest);



    size_t requested_segments = 0;

    size_t segments_downloaded = 0;

    size_t buffered_bytes = 0;

    size_t chunk_reads = 0;

    size_t max_segment_buffer = 0;

    std::vector<std::string> resolved_segments;

    std::string segment_error;



    if (manifest_parse_ok) {

        requested_segments = std::min(static_cast<size_t>(segment_limit), manifest.segment_urls.size());

        for (size_t index = 0; index < requested_segments; ++index) {

            const std::string segment_url = resolveUrl(playlist_url, manifest.segment_urls[index]);

            resolved_segments.push_back(segment_url);



            streaming::HttpStreamDownloader downloader;

            if (!downloader.open(segment_url)) {

                segment_error = downloader.lastError();

                break;

            }



            std::vector<uint8_t> segment_buffer;

            while (!downloader.eof() || downloader.bufferedBytes() > 0) {

                if (!downloader.prefetch(static_cast<size_t>(target_buffer_bytes), static_cast<size_t>(target_buffer_bytes))) {

                    segment_error = downloader.lastError();

                    break;

                }

                max_segment_buffer = std::max(max_segment_buffer, downloader.bufferedBytes());

                const auto chunk = downloader.consumeBuffered(static_cast<size_t>(target_buffer_bytes));

                if (chunk.empty()) {

                    continue;

                }

                segment_buffer.insert(segment_buffer.end(), chunk.begin(), chunk.end());

                ++chunk_reads;

            }



            downloader.close();

            if (!segment_error.empty()) {

                break;

            }

            if (segment_buffer.empty()) {

                segment_error = "segment download produced no bytes";

                break;

            }



            buffered_bytes += segment_buffer.size();

            ++segments_downloaded;

        }

    }



    const bool buffer_ok = buffered_bytes >= static_cast<size_t>(target_buffer_bytes) && max_segment_buffer > 0;

    const bool result = manifest_download_ok && manifest_parse_ok && segment_error.empty() &&

                        requested_segments > 0 && segments_downloaded == requested_segments && buffer_ok;



    std::cout << "streaming-buffer-check.playlist_url=" << playlist_url << std::endl;

    std::cout << "streaming-buffer-check.segment_limit=" << segment_limit << std::endl;

    std::cout << "streaming-buffer-check.target_buffer_bytes=" << target_buffer_bytes << std::endl;

    std::cout << "streaming-buffer-check.manifest_download_ok=" << (manifest_download_ok ? "true" : "false") << std::endl;

    std::cout << "streaming-buffer-check.manifest_parse_ok=" << (manifest_parse_ok ? "true" : "false") << std::endl;

    std::cout << "streaming-buffer-check.manifest_segments=" << manifest.segment_urls.size() << std::endl;

    std::cout << "streaming-buffer-check.requested_segments=" << requested_segments << std::endl;

    std::cout << "streaming-buffer-check.segments_downloaded=" << segments_downloaded << std::endl;

    std::cout << "streaming-buffer-check.buffered_bytes=" << buffered_bytes << std::endl;

    std::cout << "streaming-buffer-check.max_segment_buffer=" << max_segment_buffer << std::endl;

    std::cout << "streaming-buffer-check.chunk_reads=" << chunk_reads << std::endl;

    std::cout << "streaming-buffer-check.buffer_ok=" << (buffer_ok ? "true" : "false") << std::endl;

    std::cout << "streaming-buffer-check.resolved_segments=" << (resolved_segments.empty() ? std::string("none") : joinTopN(resolved_segments, 8)) << std::endl;

    std::cout << "streaming-buffer-check.error="

              << (segment_error.empty() ? (manifest_error.empty() ? std::string("none") : manifest_error) : segment_error)

              << std::endl;

    std::cout << "streaming-buffer-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 验证自适应码率选择逻辑与各变体下载可达性。

bool runAdaptiveBitrateCheck(

    const std::string& manifest_url,

    const std::string& bandwidth_samples_csv,

    int segment_limit = 2,

    int target_buffer_bytes = 256) {

    if (segment_limit <= 0 || target_buffer_bytes <= 0) {

        std::cout << "adaptive-bitrate-check.manifest_url=" << manifest_url << std::endl;

        std::cout << "adaptive-bitrate-check.result=FAIL" << std::endl;

        return false;

    }



    std::vector<int> bandwidth_samples;

    const bool bandwidth_samples_ok = parseCsvIntegers(bandwidth_samples_csv, bandwidth_samples);



    std::string manifest_text;

    std::string manifest_error;

    const bool manifest_download_ok = readUrlText(manifest_url, manifest_text, manifest_error);

    const StreamingManifestKind manifest_kind =

        manifest_download_ok ? detectManifestKind(manifest_url, manifest_text) : StreamingManifestKind::Unknown;



    std::vector<StreamingVariantCandidate> variants;

    std::string parse_error;

    bool manifest_parse_ok = false;

    if (manifest_download_ok) {

        if (manifest_kind == StreamingManifestKind::Hls) {

            manifest_parse_ok = buildHlsVariantCandidates(manifest_url, manifest_text, variants, parse_error);

        } else if (manifest_kind == StreamingManifestKind::Dash) {

            manifest_parse_ok = buildDashVariantCandidates(manifest_url, manifest_text, variants, parse_error);

        } else {

            parse_error = "unsupported manifest type";

        }

    }



    std::vector<streaming::AdaptiveStreamVariant> selector_variants;

    selector_variants.reserve(variants.size());

    for (const auto& variant : variants) {

        selector_variants.push_back({variant.id, variant.bandwidth});

    }



    std::vector<std::string> selected_variants;

    std::vector<int> selected_bandwidths;

    std::vector<std::string> downloaded_urls;

    std::set<std::string> initialized_urls;

    size_t initialization_count = 0;

    size_t segments_downloaded = 0;

    size_t buffered_bytes = 0;

    size_t max_buffered_bytes = 0;

    size_t chunk_reads = 0;

    size_t switch_count = 0;

    size_t upswitch_count = 0;

    size_t downswitch_count = 0;

    size_t fallback_count = 0;

    std::string download_error;



    if (manifest_parse_ok && bandwidth_samples_ok && !selector_variants.empty()) {

        size_t previous_variant_index = std::numeric_limits<size_t>::max();

        int previous_bandwidth = -1;



        for (int sample : bandwidth_samples) {

            const auto decision = streaming::AdaptiveBitrateSelector::chooseVariant(selector_variants, sample);

            const auto& variant = variants[decision.variant_index];

            selected_variants.push_back(variant.id);

            selected_bandwidths.push_back(variant.bandwidth);

            if (decision.fallback_to_lowest) {

                ++fallback_count;

            }

            if (previous_variant_index != std::numeric_limits<size_t>::max() && decision.variant_index != previous_variant_index) {

                ++switch_count;

                if (variant.bandwidth > previous_bandwidth) {

                    ++upswitch_count;

                } else if (variant.bandwidth < previous_bandwidth) {

                    ++downswitch_count;

                }

            }

            previous_variant_index = decision.variant_index;

            previous_bandwidth = variant.bandwidth;



            if (!downloadVariantSelection(

                    variant,

                    segment_limit,

                    static_cast<size_t>(target_buffer_bytes),

                    initialized_urls,

                    initialization_count,

                    segments_downloaded,

                    buffered_bytes,

                    max_buffered_bytes,

                    chunk_reads,

                    downloaded_urls,

                    download_error)) {

                break;

            }

        }

    }



    const bool variant_count_ok = variants.size() >= 2;

    const bool downloads_ok = download_error.empty() && segments_downloaded > 0;

    const bool abr_ok = selected_variants.size() == bandwidth_samples.size() && switch_count > 0;

    const bool result = manifest_download_ok && manifest_parse_ok && bandwidth_samples_ok && variant_count_ok && downloads_ok && abr_ok;



    std::cout << "adaptive-bitrate-check.manifest_url=" << manifest_url << std::endl;

    std::cout << "adaptive-bitrate-check.protocol=" << manifestKindName(manifest_kind) << std::endl;

    std::cout << "adaptive-bitrate-check.segment_limit=" << segment_limit << std::endl;

    std::cout << "adaptive-bitrate-check.target_buffer_bytes=" << target_buffer_bytes << std::endl;

    std::cout << "adaptive-bitrate-check.manifest_download_ok=" << (manifest_download_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.manifest_parse_ok=" << (manifest_parse_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.bandwidth_samples_ok=" << (bandwidth_samples_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.bandwidth_samples="

              << (bandwidth_samples.empty() ? std::string("none") : joinIntegers(bandwidth_samples, 16)) << std::endl;

    std::cout << "adaptive-bitrate-check.variant_count=" << variants.size() << std::endl;

    std::cout << "adaptive-bitrate-check.selected_variants="

              << (selected_variants.empty() ? std::string("none") : joinTopN(selected_variants, 16)) << std::endl;

    std::cout << "adaptive-bitrate-check.selected_bandwidths="

              << (selected_bandwidths.empty() ? std::string("none") : joinIntegers(selected_bandwidths, 16)) << std::endl;

    std::cout << "adaptive-bitrate-check.switch_count=" << switch_count << std::endl;

    std::cout << "adaptive-bitrate-check.upswitch_count=" << upswitch_count << std::endl;

    std::cout << "adaptive-bitrate-check.downswitch_count=" << downswitch_count << std::endl;

    std::cout << "adaptive-bitrate-check.fallback_count=" << fallback_count << std::endl;

    std::cout << "adaptive-bitrate-check.initializations_downloaded=" << initialization_count << std::endl;

    std::cout << "adaptive-bitrate-check.segments_downloaded=" << segments_downloaded << std::endl;

    std::cout << "adaptive-bitrate-check.buffered_bytes=" << buffered_bytes << std::endl;

    std::cout << "adaptive-bitrate-check.max_buffered_bytes=" << max_buffered_bytes << std::endl;

    std::cout << "adaptive-bitrate-check.chunk_reads=" << chunk_reads << std::endl;

    std::cout << "adaptive-bitrate-check.variant_count_ok=" << (variant_count_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.abr_ok=" << (abr_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.downloads_ok=" << (downloads_ok ? "true" : "false") << std::endl;

    std::cout << "adaptive-bitrate-check.downloaded_urls="

              << (downloaded_urls.empty() ? std::string("none") : joinTopN(downloaded_urls, 12)) << std::endl;

    std::cout << "adaptive-bitrate-check.error="

              << (download_error.empty() ? (parse_error.empty() ? (manifest_error.empty() ? std::string("none") : manifest_error)

                                                               : parse_error)

                                        : download_error)

              << std::endl;

    std::cout << "adaptive-bitrate-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    return result;

}



/// 验证暂停态截图请求、落盘和路径回传链路。

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


#if defined(_WIN32) && defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
std::string formatHexUint32(uint32_t value) {
    std::ostringstream oss;
    oss << "0x" << std::hex << std::uppercase << value;
    return oss.str();
}

void printD3D11FormatSupport(const std::string& prefix, const render::D3D11FormatSupportSnapshot& support) {
    std::cout << prefix << ".check_succeeded=" << (support.check_succeeded ? "true" : "false") << std::endl;
    std::cout << prefix << ".check_hr=" << support.check_hr << std::endl;
    std::cout << prefix << ".raw_support=" << formatHexUint32(support.raw_support) << std::endl;
    std::cout << prefix << ".texture2d=" << (support.texture2d ? "true" : "false") << std::endl;
    std::cout << prefix << ".shader_sample=" << (support.shader_sample ? "true" : "false") << std::endl;
    std::cout << prefix << ".shader_load=" << (support.shader_load ? "true" : "false") << std::endl;
    std::cout << prefix << ".decoder_output=" << (support.decoder_output ? "true" : "false") << std::endl;
}

void printD3D11HdrOutputSnapshot(const std::string& prefix, const render::D3D11HdrOutputSnapshot& hdr_output) {
    std::cout << prefix << ".probe_succeeded=" << (hdr_output.probe_succeeded ? "true" : "false") << std::endl;
    std::cout << prefix << ".probe_error=" << hdr_output.probe_error << std::endl;
    std::cout << prefix << ".adapter_matched=" << (hdr_output.adapter_matched ? "true" : "false") << std::endl;
    std::cout << prefix << ".adapter_index=" << hdr_output.adapter_index << std::endl;
    std::cout << prefix << ".output_found=" << (hdr_output.output_found ? "true" : "false") << std::endl;
    std::cout << prefix << ".output_index=" << hdr_output.output_index << std::endl;
    std::cout << prefix << ".output_name=" << hdr_output.output_name << std::endl;
    std::cout << prefix << ".output_device_name=" << hdr_output.output_device_name << std::endl;
    std::cout << prefix << ".output_attached_to_desktop="
              << (hdr_output.output_attached_to_desktop ? "true" : "false") << std::endl;
    std::cout << prefix << ".has_output6=" << (hdr_output.has_output6 ? "true" : "false") << std::endl;
    std::cout << prefix << ".color_space=" << hdr_output.color_space << std::endl;
    std::cout << prefix << ".advanced_color_active="
              << (hdr_output.advanced_color_active ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_active=" << (hdr_output.hdr_active ? "true" : "false") << std::endl;
    std::cout << prefix << ".bits_per_color=" << hdr_output.bits_per_color << std::endl;
    std::cout << prefix << ".min_luminance_nits=" << hdr_output.min_luminance_nits << std::endl;
    std::cout << prefix << ".max_luminance_nits=" << hdr_output.max_luminance_nits << std::endl;
    std::cout << prefix << ".max_full_frame_luminance_nits="
              << hdr_output.max_full_frame_luminance_nits << std::endl;
}

void printD3D11RuntimeHdrState(const std::string& prefix, const core::DiagnosticsSnapshot& diag) {
    const double present_time_ms = static_cast<double>(diag.renderer_d3d11_present_time_us_total) / 1000.0;
    const double present_avg_ms =
        diag.renderer_d3d11_present_count > 0
            ? present_time_ms / static_cast<double>(diag.renderer_d3d11_present_count)
            : 0.0;

    std::cout << prefix << ".hdr_present_requested="
              << (diag.renderer_d3d11_hdr_present_requested ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_present_active="
              << (diag.renderer_d3d11_hdr_present_active ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_content_detected="
              << (diag.renderer_d3d11_hdr_content_detected ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_present_mode=" << diag.renderer_d3d11_hdr_present_mode << std::endl;
    std::cout << prefix << ".hdr_present_decision=" << diag.renderer_d3d11_hdr_present_decision << std::endl;
    std::cout << prefix << ".hdr_swapchain_format=" << diag.renderer_d3d11_hdr_swapchain_format << std::endl;
    std::cout << prefix << ".hdr_output_color_space=" << diag.renderer_d3d11_hdr_output_color_space << std::endl;
    std::cout << prefix << ".hdr_output_display_index=" << diag.renderer_d3d11_hdr_output_display_index << std::endl;
    std::cout << prefix << ".hdr_output_display_name=" << diag.renderer_d3d11_hdr_output_display_name << std::endl;
    std::cout << prefix << ".hdr_output_device_name=" << diag.renderer_d3d11_hdr_output_device_name << std::endl;
    std::cout << prefix << ".hdr_output_advanced_color_active="
              << (diag.renderer_d3d11_hdr_output_advanced_color_active ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_output_hdr_active="
              << (diag.renderer_d3d11_hdr_output_hdr_active ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_metadata_available="
              << (diag.renderer_d3d11_hdr_metadata_available ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_metadata_applied="
              << (diag.renderer_d3d11_hdr_metadata_applied ? "true" : "false") << std::endl;
    std::cout << prefix << ".hdr_metadata_source=" << diag.renderer_d3d11_hdr_metadata_source << std::endl;
    std::cout << prefix << ".hdr_state_reload_count=" << diag.renderer_d3d11_hdr_state_reload_count << std::endl;
    std::cout << prefix << ".present_count=" << diag.renderer_d3d11_present_count << std::endl;
    std::cout << prefix << ".present_failures=" << diag.renderer_d3d11_present_failures << std::endl;
    std::cout << prefix << ".present_time_ms=" << present_time_ms << std::endl;
    std::cout << prefix << ".present_avg_ms=" << present_avg_ms << std::endl;
    std::cout << prefix << ".present_time_us_max=" << diag.renderer_d3d11_present_time_us_max << std::endl;
    std::cout << prefix << ".hdr_error=" << diag.renderer_d3d11_hdr_error << std::endl;
}
#else
void printD3D11RuntimeHdrState(const std::string&, const core::DiagnosticsSnapshot&) {}
#endif

bool runD3D11Diagnostics() {
#if defined(_WIN32) && defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    const render::D3D11DiagnosticsSnapshot diag = render::D3D11VideoRenderer::probeSystemDiagnostics();
    const bool h264_any = diag.decoder_profiles.h264_vld_nofgt || diag.decoder_profiles.h264_vld_fgt;
    const bool hevc_any = diag.decoder_profiles.hevc_main || diag.decoder_profiles.hevc_main10;
    const bool vp9_any = diag.decoder_profiles.vp9_profile0 || diag.decoder_profiles.vp9_profile2_10bit;
    const bool av1_any = diag.decoder_profiles.av1_profile0 ||
                         diag.decoder_profiles.av1_profile1 ||
                         diag.decoder_profiles.av1_profile2 ||
                         diag.decoder_profiles.av1_profile2_12bit ||
                         diag.decoder_profiles.av1_profile2_12bit_420;

    std::cout << "d3d11-diagnostics.supported_platform=true" << std::endl;
    std::cout << "d3d11-diagnostics.probe_succeeded=" << (diag.probe_succeeded ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.create_device_hr=" << diag.create_device_hr << std::endl;
    std::cout << "d3d11-diagnostics.adapter_name=" << diag.adapter_name << std::endl;
    std::cout << "d3d11-diagnostics.vendor_id=" << formatHexUint32(diag.vendor_id) << std::endl;
    std::cout << "d3d11-diagnostics.device_id=" << formatHexUint32(diag.device_id) << std::endl;
    std::cout << "d3d11-diagnostics.subsystem_id=" << formatHexUint32(diag.subsystem_id) << std::endl;
    std::cout << "d3d11-diagnostics.revision=" << diag.revision << std::endl;
    std::cout << "d3d11-diagnostics.driver_version=" << diag.driver_version << std::endl;
    std::cout << "d3d11-diagnostics.software_adapter=" << (diag.software_adapter ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.dedicated_video_mib=" << diag.dedicated_video_mib << std::endl;
    std::cout << "d3d11-diagnostics.dedicated_system_mib=" << diag.dedicated_system_mib << std::endl;
    std::cout << "d3d11-diagnostics.shared_system_mib=" << diag.shared_system_mib << std::endl;
    std::cout << "d3d11-diagnostics.feature_level=" << diag.feature_level << std::endl;
    std::cout << "d3d11-diagnostics.debug_layer_enabled=" << (diag.debug_layer_enabled ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.multithread_protected=" << (diag.multithread_protected ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.has_device3=" << (diag.has_device3 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.has_video_device=" << (diag.has_video_device ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.has_video_context=" << (diag.has_video_context ? "true" : "false") << std::endl;

    printD3D11FormatSupport("d3d11-diagnostics.format.nv12", diag.nv12_support);
    printD3D11FormatSupport("d3d11-diagnostics.format.p010", diag.p010_support);
    printD3D11FormatSupport("d3d11-diagnostics.format.p016", diag.p016_support);

    std::cout << "d3d11-diagnostics.decoder_profiles.enumeration_succeeded="
              << (diag.decoder_profiles.enumeration_succeeded ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.enumerated_profile_count="
              << diag.decoder_profiles.enumerated_profile_count << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.h264_any=" << (h264_any ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.h264_vld_nofgt="
              << (diag.decoder_profiles.h264_vld_nofgt ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.h264_vld_fgt="
              << (diag.decoder_profiles.h264_vld_fgt ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.hevc_any=" << (hevc_any ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.hevc_main="
              << (diag.decoder_profiles.hevc_main ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.hevc_main10="
              << (diag.decoder_profiles.hevc_main10 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.vp9_any=" << (vp9_any ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.vp9_profile0="
              << (diag.decoder_profiles.vp9_profile0 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.vp9_profile2_10bit="
              << (diag.decoder_profiles.vp9_profile2_10bit ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_any=" << (av1_any ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_profile0="
              << (diag.decoder_profiles.av1_profile0 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_profile1="
              << (diag.decoder_profiles.av1_profile1 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_profile2="
              << (diag.decoder_profiles.av1_profile2 ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_profile2_12bit="
              << (diag.decoder_profiles.av1_profile2_12bit ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.decoder_profiles.av1_profile2_12bit_420="
              << (diag.decoder_profiles.av1_profile2_12bit_420 ? "true" : "false") << std::endl;
    printD3D11HdrOutputSnapshot("d3d11-diagnostics.hdr_output", diag.hdr_output);

    std::cout << "d3d11-diagnostics.native_direct.allowed=" << (diag.native_direct_allowed ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.native_direct.startup_disabled="
              << (diag.native_direct_startup_disabled ? "true" : "false") << std::endl;
    std::cout << "d3d11-diagnostics.native_direct.disable_rule=" << diag.native_direct_disable_rule << std::endl;
    std::cout << "d3d11-diagnostics.native_direct.disable_reason=" << diag.native_direct_disable_reason << std::endl;
    std::cout << "d3d11-diagnostics.result=" << (diag.probe_succeeded ? "PASS" : "FAIL") << std::endl;
    return diag.probe_succeeded;
#else
    std::cout << "d3d11-diagnostics.supported_platform=false" << std::endl;
    std::cout << "d3d11-diagnostics.probe_succeeded=false" << std::endl;
    std::cout << "d3d11-diagnostics.native_direct.allowed=false" << std::endl;
    std::cout << "d3d11-diagnostics.result=FAIL" << std::endl;
    return false;
#endif
}

bool runD3D11HdrOutputCheck(const std::string& media_file, int sample_ms = 1200) {
#if defined(_WIN32) && defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    std::error_code ec;
    const std::filesystem::path media_path(media_file);
    if (!std::filesystem::exists(media_path, ec) || ec ||
        !std::filesystem::is_regular_file(media_path, ec) || ec ||
        sample_ms < 500) {
        std::cout << "d3d11-hdr-output-check.path=" << media_file << std::endl;
        std::cout << "d3d11-hdr-output-check.sample_ms=" << sample_ms << std::endl;
        std::cout << "d3d11-hdr-output-check.result=FAIL" << std::endl;
        return false;
    }

    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered;
    };

    ScopedEnvOverride force_d3d11("MVP_RENDERER_BACKEND", "d3d11");
    ScopedEnvOverride force_hdr_mode("MVP_D3D11_HDR_OUTPUT_MODE", "auto");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    core::DiagnosticsSnapshot diag{};
    std::string renderer_backend = "None";
    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        player.stop();
        player.close();
    }

    const bool backend_ok = renderer_backend == "D3D11";
    const bool decision_evaluated = diag.renderer_d3d11_hdr_present_decision != "not-evaluated";
    const bool output_hdr_ready =
        diag.renderer_d3d11_hdr_output_advanced_color_active || diag.renderer_d3d11_hdr_output_hdr_active;
    const bool hdr_runtime_ok = (output_hdr_ready && diag.renderer_d3d11_hdr_content_detected)
        ? diag.renderer_d3d11_hdr_present_active
        : (!diag.renderer_d3d11_hdr_present_active && decision_evaluated);
    const bool result = open_ok &&
                        entered_playback_loop &&
                        backend_ok &&
                        diag.render_frames > 0 &&
                        decision_evaluated &&
                        hdr_runtime_ok;

    std::cout << "d3d11-hdr-output-check.path=" << media_file << std::endl;
    std::cout << "d3d11-hdr-output-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "d3d11-hdr-output-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "d3d11-hdr-output-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "d3d11-hdr-output-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "d3d11-hdr-output-check.render_frames=" << diag.render_frames << std::endl;
    std::cout << "d3d11-hdr-output-check.decision_evaluated=" << (decision_evaluated ? "true" : "false")
              << std::endl;
    std::cout << "d3d11-hdr-output-check.output_hdr_ready=" << (output_hdr_ready ? "true" : "false")
              << std::endl;
    std::cout << "d3d11-hdr-output-check.hdr_runtime_ok=" << (hdr_runtime_ok ? "true" : "false")
              << std::endl;
    printD3D11RuntimeHdrState("d3d11-hdr-output-check", diag);
    std::cout << "d3d11-hdr-output-check.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
#else
    (void)media_file;
    (void)sample_ms;
    std::cout << "d3d11-hdr-output-check.supported_platform=false" << std::endl;
    std::cout << "d3d11-hdr-output-check.result=FAIL" << std::endl;
    return false;
#endif
}

bool runVulkanDiagnostics() {
    const platform::PlatformCapabilities capabilities = platform::PlatformCapabilitiesProbe::detect();
    const bool supported_platform =
        capabilities.platform == platform::PlatformKind::Linux ||
        capabilities.platform == platform::PlatformKind::Windows;

    const auto read_env = [](const char* key) -> std::string {
        if (!key || key[0] == '\0') {
            return {};
        }
#if defined(_WIN32)
        char* value = nullptr;
        size_t length = 0;
        if (_dupenv_s(&value, &length, key) != 0 || !value) {
            return {};
        }
        std::string copy(value);
        std::free(value);
        return copy;
#else
        const char* value = std::getenv(key);
        return value ? std::string(value) : std::string{};
#endif
    };

    bool compiled_in = false;
    bool runtime_available = false;
    for (const platform::RendererSupport& support : capabilities.renderer_support) {
        if (support.type == render::VideoRendererType::Vulkan) {
            compiled_in = support.compiled_in;
            runtime_available = support.runtime_available;
            break;
        }
    }

    std::string requested_renderer_override = "none";
    const std::string raw_override = read_env("MVP_RENDERER_BACKEND");
    if (!raw_override.empty()) {
        requested_renderer_override = toLower(raw_override);
    }

    core::PlaybackOpenRequest open_request{};
    open_request.media_info.video_stream_idx = 0;
    open_request.preferences.prefer_hardware_decode = true;
    open_request.platform_capabilities = capabilities;
    const core::PlaybackOpenPlan open_plan = core::PlaybackStrategy::buildOpenPlan(open_request);

    std::string selected_renderer = "None";
    for (render::VideoRendererType candidate : open_plan.renderer_candidates) {
        if (!render::RendererFactory::isSupported(candidate, capabilities)) {
            continue;
        }
        selected_renderer = render::RendererFactory::rendererName(candidate);
        break;
    }

    std::string fallback_target = "none";
    const auto vulkan_it = std::find(open_plan.renderer_candidates.begin(),
                                     open_plan.renderer_candidates.end(),
                                     render::VideoRendererType::Vulkan);
    if (vulkan_it != open_plan.renderer_candidates.end()) {
        for (auto it = vulkan_it; it != open_plan.renderer_candidates.end();) {
            ++it;
            if (it == open_plan.renderer_candidates.end()) {
                break;
            }
            if (render::RendererFactory::isSupported(*it, capabilities)) {
                fallback_target = render::RendererFactory::rendererName(*it);
                break;
            }
        }
    }

    const bool result = supported_platform && compiled_in && runtime_available;

    std::cout << "vulkan-diagnostics.platform=" << platform::platformKindName(capabilities.platform) << std::endl;
    std::cout << "vulkan-diagnostics.supported_platform=" << (supported_platform ? "true" : "false") << std::endl;
    std::cout << "vulkan-diagnostics.compiled_in=" << (compiled_in ? "true" : "false") << std::endl;
    std::cout << "vulkan-diagnostics.runtime_available=" << (runtime_available ? "true" : "false") << std::endl;
    std::cout << "vulkan-diagnostics.dependency_source=" << MVP_VULKAN_DEPENDENCY_SOURCE << std::endl;
    std::cout << "vulkan-diagnostics.requested_renderer_override=" << requested_renderer_override << std::endl;
    std::cout << "vulkan-diagnostics.startup_renderer_candidates="
              << rendererCandidateChainToString(open_plan.renderer_candidates) << std::endl;
    std::cout << "vulkan-diagnostics.startup_renderer_plan_reason="
              << open_plan.renderer_plan_reason << std::endl;
    std::cout << "vulkan-diagnostics.selected_renderer=" << selected_renderer << std::endl;
    std::cout << "vulkan-diagnostics.fallback_target=" << fallback_target << std::endl;
    std::cout << "vulkan-diagnostics.result=" << (result ? "PASS" : "FAIL") << std::endl;
    return result;
}

bool runOpenGLDiagnostics() {
#if defined(_WIN32) && defined(MVP_HAVE_OPENGL_RENDERER) && MVP_HAVE_OPENGL_RENDERER
    const render::OpenGLDiagnosticsSnapshot diag = render::OpenGLVideoRenderer::probeSystemDiagnostics();

    std::cout << "opengl-diagnostics.supported_platform=true" << std::endl;
    std::cout << "opengl-diagnostics.probe_succeeded=" << (diag.probe_succeeded ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.gl_context_created=" << (diag.gl_context_created ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.create_context_error=" << diag.create_context_error << std::endl;
    std::cout << "opengl-diagnostics.gl_vendor=" << diag.gl_vendor << std::endl;
    std::cout << "opengl-diagnostics.gl_renderer=" << diag.gl_renderer << std::endl;
    std::cout << "opengl-diagnostics.gl_version=" << diag.gl_version << std::endl;
    std::cout << "opengl-diagnostics.has_wgl_dx_interop=" << (diag.has_wgl_dx_interop ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.native_interop.env_override=" << diag.native_interop_env_override << std::endl;
    std::cout << "opengl-diagnostics.native_interop.allowed=" << (diag.native_direct_allowed ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.native_interop.startup_disabled=" << (diag.native_direct_startup_disabled ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.native_interop.disable_rule=" << diag.native_direct_disable_rule << std::endl;
    std::cout << "opengl-diagnostics.native_interop.disable_reason=" << diag.native_direct_disable_reason << std::endl;
    std::cout << "opengl-diagnostics.native_interop.hard_blocker_matched=" << (diag.hard_blocker_matched ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.native_interop.hard_blocker_rule=" << diag.hard_blocker_rule << std::endl;
    std::cout << "opengl-diagnostics.native_interop.hard_blocker_reason=" << diag.hard_blocker_reason << std::endl;
    std::cout << "opengl-diagnostics.native_interop.quirk_rule_matched=" << (diag.quirk_rule_matched ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.native_interop.quirk_rule_name=" << diag.quirk_rule_name << std::endl;
    std::cout << "opengl-diagnostics.native_interop.quirk_rule_reason=" << diag.quirk_rule_reason << std::endl;
    std::cout << "opengl-diagnostics.native_interop.env_force_overrode_quirk=" << (diag.env_force_overrode_quirk ? "true" : "false") << std::endl;

#if defined(MVP_HAVE_D3D11_RENDERER) && MVP_HAVE_D3D11_RENDERER
    const bool h264_any = diag.d3d11.decoder_profiles.h264_vld_nofgt || diag.d3d11.decoder_profiles.h264_vld_fgt;
    const bool hevc_any = diag.d3d11.decoder_profiles.hevc_main || diag.d3d11.decoder_profiles.hevc_main10;
    const bool vp9_any = diag.d3d11.decoder_profiles.vp9_profile0 || diag.d3d11.decoder_profiles.vp9_profile2_10bit;
    const bool av1_any = diag.d3d11.decoder_profiles.av1_profile0 ||
                         diag.d3d11.decoder_profiles.av1_profile1 ||
                         diag.d3d11.decoder_profiles.av1_profile2 ||
                         diag.d3d11.decoder_profiles.av1_profile2_12bit ||
                         diag.d3d11.decoder_profiles.av1_profile2_12bit_420;

    std::cout << "opengl-diagnostics.d3d11.adapter_name=" << diag.d3d11.adapter_name << std::endl;
    std::cout << "opengl-diagnostics.d3d11.vendor_id=" << formatHexUint32(diag.d3d11.vendor_id) << std::endl;
    std::cout << "opengl-diagnostics.d3d11.device_id=" << formatHexUint32(diag.d3d11.device_id) << std::endl;
    std::cout << "opengl-diagnostics.d3d11.driver_version=" << diag.d3d11.driver_version << std::endl;
    std::cout << "opengl-diagnostics.d3d11.feature_level=" << diag.d3d11.feature_level << std::endl;
    std::cout << "opengl-diagnostics.d3d11.software_adapter=" << (diag.d3d11.software_adapter ? "true" : "false") << std::endl;
    printD3D11FormatSupport("opengl-diagnostics.d3d11.format.nv12", diag.d3d11.nv12_support);
    std::cout << "opengl-diagnostics.d3d11.decoder_profiles.h264_any=" << (h264_any ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.d3d11.decoder_profiles.hevc_any=" << (hevc_any ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.d3d11.decoder_profiles.vp9_any=" << (vp9_any ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.d3d11.decoder_profiles.av1_any=" << (av1_any ? "true" : "false") << std::endl;
#else
    std::cout << "opengl-diagnostics.d3d11.compiled=false" << std::endl;
#endif
    std::cout << "opengl-diagnostics.hdr_output.probe_succeeded=" << (diag.hdr_output.probe_succeeded ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.probe_error=" << diag.hdr_output.probe_error << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.adapter_matched=" << (diag.hdr_output.adapter_matched ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.adapter_index=" << diag.hdr_output.adapter_index << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.output_found=" << (diag.hdr_output.output_found ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.output_index=" << diag.hdr_output.output_index << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.output_name=" << diag.hdr_output.output_name << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.output_device_name=" << diag.hdr_output.output_device_name << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.output_attached_to_desktop="
              << (diag.hdr_output.output_attached_to_desktop ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.has_output6=" << (diag.hdr_output.has_output6 ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.color_space=" << diag.hdr_output.color_space << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.advanced_color_active="
              << (diag.hdr_output.advanced_color_active ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.hdr_active="
              << (diag.hdr_output.hdr_active ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.bits_per_color=" << diag.hdr_output.bits_per_color << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.min_luminance_nits=" << diag.hdr_output.min_luminance_nits << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.max_luminance_nits=" << diag.hdr_output.max_luminance_nits << std::endl;
    std::cout << "opengl-diagnostics.hdr_output.max_full_frame_luminance_nits="
              << diag.hdr_output.max_full_frame_luminance_nits << std::endl;
    std::cout << "opengl-diagnostics.output_display.binding_succeeded="
              << (diag.output_display.binding_succeeded ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.output_display.sdl_display_index="
              << diag.output_display.sdl_display_index << std::endl;
    std::cout << "opengl-diagnostics.output_display.sdl_display_name="
              << diag.output_display.sdl_display_name << std::endl;
    std::cout << "opengl-diagnostics.output_display.output_device_name="
              << diag.output_display.output_device_name << std::endl;
    std::cout << "opengl-diagnostics.output_display.icc_profile_available="
              << (diag.output_display.icc_profile_available ? "true" : "false") << std::endl;
    std::cout << "opengl-diagnostics.output_display.icc_profile_source="
              << diag.output_display.icc_profile_source << std::endl;
    std::cout << "opengl-diagnostics.output_display.icc_profile_path="
              << diag.output_display.icc_profile_path << std::endl;
    std::cout << "opengl-diagnostics.output_display.icc_profile_description="
              << diag.output_display.icc_profile_description << std::endl;
    std::cout << "opengl-diagnostics.output_display.binding_error="
              << diag.output_display.binding_error << std::endl;
    std::cout << "opengl-diagnostics.result=" << (diag.probe_succeeded ? "PASS" : "FAIL") << std::endl;
    return diag.probe_succeeded;
#else
    std::cout << "opengl-diagnostics.supported_platform=false" << std::endl;
    std::cout << "opengl-diagnostics.probe_succeeded=false" << std::endl;
    std::cout << "opengl-diagnostics.native_interop.allowed=false" << std::endl;
    std::cout << "opengl-diagnostics.result=FAIL" << std::endl;
    return false;
#endif
}

bool runOpenGLOutputColorCheck(const std::string& media_file,
                               const std::string& lut_file,
                               int sample_ms = 1200) {
    ScopedEnvOverride force_opengl("MVP_RENDERER_BACKEND", "opengl");
    ScopedEnvOverride force_lut("MVP_OPENGL_3DLUT_FILE", lut_file);
    ScopedEnvOverride clear_icc("MVP_OPENGL_ICC_PROFILE_FILE", "");
    ScopedEnvOverride disable_auto_icc("MVP_OPENGL_AUTO_ICC", "0");
    ScopedEnvOverride force_hdr_mode("MVP_OPENGL_HDR_OUTPUT_MODE", "auto");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend = "None";
    core::DiagnosticsSnapshot diag{};
    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered;
    };
    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        player.stop();
        player.close();
    }

    const bool pass = open_ok &&
                      entered_playback_loop &&
                      diag.render_frames > 0 &&
                      diag.renderer_opengl_output_lut_configured &&
                      diag.renderer_opengl_output_lut_source == "cube";

    std::cout << "opengl-output-color-check.path=" << media_file << std::endl;
    std::cout << "opengl-output-color-check.lut_file=" << lut_file << std::endl;
    std::cout << "opengl-output-color-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "opengl-output-color-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.entered_playback_loop=" << (entered_playback_loop ? "true" : "false")
              << std::endl;
    std::cout << "opengl-output-color-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "opengl-output-color-check.hdr_bridge_requested="
              << (diag.renderer_opengl_hdr_bridge_requested ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.hdr_bridge_active="
              << (diag.renderer_opengl_hdr_bridge_active ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.hdr_bridge_mode="
              << diag.renderer_opengl_hdr_bridge_mode << std::endl;
    std::cout << "opengl-output-color-check.hdr_bridge_decision="
              << diag.renderer_opengl_hdr_bridge_decision << std::endl;
    std::cout << "opengl-output-color-check.output_lut_configured="
              << (diag.renderer_opengl_output_lut_configured ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.output_lut_active="
              << (diag.renderer_opengl_output_lut_active ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.output_lut_size="
              << diag.renderer_opengl_output_lut_size << std::endl;
    std::cout << "opengl-output-color-check.output_lut_path="
              << diag.renderer_opengl_output_lut_path << std::endl;
    std::cout << "opengl-output-color-check.output_lut_error="
              << diag.renderer_opengl_output_lut_error << std::endl;
    std::cout << "opengl-output-color-check.output_lut_source="
              << diag.renderer_opengl_output_lut_source << std::endl;
    std::cout << "opengl-output-color-check.output_lut_reload_count="
              << diag.renderer_opengl_output_lut_reload_count << std::endl;
    std::cout << "opengl-output-color-check.output_display_index="
              << diag.renderer_opengl_output_display_index << std::endl;
    std::cout << "opengl-output-color-check.output_display_name="
              << diag.renderer_opengl_output_display_name << std::endl;
    std::cout << "opengl-output-color-check.output_display_device_name="
              << diag.renderer_opengl_output_display_device_name << std::endl;
    std::cout << "opengl-output-color-check.output_icc_profile_available="
              << (diag.renderer_opengl_output_icc_profile_available ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-check.output_icc_profile_source="
              << diag.renderer_opengl_output_icc_profile_source << std::endl;
    std::cout << "opengl-output-color-check.output_icc_profile_path="
              << diag.renderer_opengl_output_icc_profile_path << std::endl;
    std::cout << "opengl-output-color-check.output_icc_profile_description="
              << diag.renderer_opengl_output_icc_profile_description << std::endl;
    std::cout << "opengl-output-color-check.output_binding_error="
              << diag.renderer_opengl_output_binding_error << std::endl;
    std::cout << "opengl-output-color-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runOpenGLOutputColorIccCheck(const std::string& media_file,
                                  int sample_ms = 1200) {
    ScopedEnvOverride force_opengl("MVP_RENDERER_BACKEND", "opengl");
    ScopedEnvOverride clear_lut("MVP_OPENGL_3DLUT_FILE", "");
    ScopedEnvOverride clear_icc("MVP_OPENGL_ICC_PROFILE_FILE", "");
    ScopedEnvOverride enable_auto_icc("MVP_OPENGL_AUTO_ICC", "1");
    ScopedEnvOverride force_hdr_mode("MVP_OPENGL_HDR_OUTPUT_MODE", "auto");

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    bool entered_playback_loop = false;
    std::string renderer_backend = "None";
    core::DiagnosticsSnapshot diag{};
    auto pump_for = [](VideoPlayer& player, std::chrono::milliseconds duration) {
        bool entered = false;
        const auto deadline = std::chrono::steady_clock::now() + duration;
        while (std::chrono::steady_clock::now() < deadline &&
               (player.isPlaying() || player.isPaused())) {
            entered = true;
            player.pumpEvents();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        return entered;
    };
    if (open_ok) {
        player.play();
        entered_playback_loop = pump_for(player, std::chrono::milliseconds(sample_ms));
        diag = player.getDiagnosticsSnapshot();
        renderer_backend = player.videoRendererBackendName();
        player.stop();
        player.close();
    }

    const bool icc_lut_selected = diag.renderer_opengl_output_lut_source.rfind("icc", 0) == 0;
    const bool pass = open_ok &&
                      entered_playback_loop &&
                      diag.render_frames > 0 &&
                      diag.renderer_opengl_output_lut_configured &&
                      icc_lut_selected;

    std::cout << "opengl-output-color-icc-check.path=" << media_file << std::endl;
    std::cout << "opengl-output-color-icc-check.sample_ms=" << sample_ms << std::endl;
    std::cout << "opengl-output-color-icc-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-icc-check.entered_playback_loop="
              << (entered_playback_loop ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-icc-check.renderer_backend=" << renderer_backend << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_configured="
              << (diag.renderer_opengl_output_lut_configured ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_active="
              << (diag.renderer_opengl_output_lut_active ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_size="
              << diag.renderer_opengl_output_lut_size << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_path="
              << diag.renderer_opengl_output_lut_path << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_error="
              << diag.renderer_opengl_output_lut_error << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_source="
              << diag.renderer_opengl_output_lut_source << std::endl;
    std::cout << "opengl-output-color-icc-check.output_lut_reload_count="
              << diag.renderer_opengl_output_lut_reload_count << std::endl;
    std::cout << "opengl-output-color-icc-check.output_display_index="
              << diag.renderer_opengl_output_display_index << std::endl;
    std::cout << "opengl-output-color-icc-check.output_display_name="
              << diag.renderer_opengl_output_display_name << std::endl;
    std::cout << "opengl-output-color-icc-check.output_display_device_name="
              << diag.renderer_opengl_output_display_device_name << std::endl;
    std::cout << "opengl-output-color-icc-check.output_icc_profile_available="
              << (diag.renderer_opengl_output_icc_profile_available ? "true" : "false") << std::endl;
    std::cout << "opengl-output-color-icc-check.output_icc_profile_source="
              << diag.renderer_opengl_output_icc_profile_source << std::endl;
    std::cout << "opengl-output-color-icc-check.output_icc_profile_path="
              << diag.renderer_opengl_output_icc_profile_path << std::endl;
    std::cout << "opengl-output-color-icc-check.output_icc_profile_description="
              << diag.renderer_opengl_output_icc_profile_description << std::endl;
    std::cout << "opengl-output-color-icc-check.output_binding_error="
              << diag.renderer_opengl_output_binding_error << std::endl;
    std::cout << "opengl-output-color-icc-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runAttachmentFontCheck(const std::string& media_file) {
    Demuxer demuxer;
    const bool open_ok = demuxer.open(media_file);
    const AVFormatContext* format_ctx = open_ok ? demuxer.getFormatContext() : nullptr;
    const subtitle::SubtitleFontRegistrationSummary summary =
        subtitle::ensureMediaAttachmentFontsRegistered(media_file, format_ctx);

    const bool has_font_attachments = summary.discovered_attachment_stream_count > 0;
    const bool extraction_ok = has_font_attachments &&
                               summary.extracted_file_count == summary.discovered_attachment_stream_count &&
                               summary.invalid_attachment_stream_count == 0;
    const bool registration_ok = has_font_attachments &&
                                 summary.registered_file_count == summary.extracted_file_count;
    const bool result = open_ok && extraction_ok && registration_ok;

    std::cout << "attachment-font-check.path=" << media_file << std::endl;
    std::cout << "attachment-font-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "attachment-font-check.attachment_streams=" << summary.discovered_attachment_stream_count << std::endl;
    std::cout << "attachment-font-check.extracted_file_count=" << summary.extracted_file_count << std::endl;
    std::cout << "attachment-font-check.registered_file_count=" << summary.registered_file_count << std::endl;
    std::cout << "attachment-font-check.invalid_attachment_stream_count="
              << summary.invalid_attachment_stream_count << std::endl;
    std::cout << "attachment-font-check.cache_path="
              << (summary.extraction_cache_path.empty() ? "none" : summary.extraction_cache_path) << std::endl;
    std::cout << "attachment-font-check.extracted_font_files="
              << (summary.extracted_font_files.empty() ? "none" : joinTopN(summary.extracted_font_files)) << std::endl;
    std::cout << "attachment-font-check.registered_font_files="
              << (summary.registered_font_files.empty() ? "none" : joinTopN(summary.registered_font_files)) << std::endl;
    std::cout << "attachment-font-check.result=" << (result ? "PASS" : "FAIL") << std::endl;

    subtitle::releaseMediaAttachmentFonts(media_file);
    demuxer.close();
    return result;
}

bool runDirectWriteFontCollectionCheck(const std::string& media_file) {
#if defined(_WIN32)
    Demuxer demuxer;
    const bool open_ok = demuxer.open(media_file);
    const AVFormatContext* format_ctx = open_ok ? demuxer.getFormatContext() : nullptr;
    const subtitle::SubtitleFontRegistrationSummary sidecar_summary =
        subtitle::ensureSubtitleFontsRegistered(media_file);
    const subtitle::SubtitleFontRegistrationSummary attachment_summary =
        subtitle::ensureMediaAttachmentFontsRegistered(media_file, format_ctx);

    Microsoft::WRL::ComPtr<IDWriteFactory> dwrite_factory;
    const HRESULT factory_hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED,
                                                   __uuidof(IDWriteFactory),
                                                   reinterpret_cast<IUnknown**>(dwrite_factory.GetAddressOf()));
    const bool factory_ok = SUCCEEDED(factory_hr) && dwrite_factory != nullptr;

    Microsoft::WRL::ComPtr<IDWriteFontCollection> custom_collection;
    subtitle::SubtitleDirectWriteCollectionSummary collection_summary{};
    if (factory_ok) {
        collection_summary = subtitle::buildDirectWriteSubtitleFontCollection(
            dwrite_factory.Get(),
            custom_collection.GetAddressOf());
    } else {
        collection_summary.attempted = true;
        collection_summary.error = "DWriteCreateFactory failed";
    }

    const bool has_registered_fonts =
        collection_summary.registered_font_file_count > 0 ||
        sidecar_summary.registered_file_count > 0 ||
        attachment_summary.registered_file_count > 0;
    const bool pass = open_ok &&
                      factory_ok &&
                      (!has_registered_fonts || collection_summary.collection_created);

    std::cout << "directwrite-font-collection-check.path=" << media_file << std::endl;
    std::cout << "directwrite-font-collection-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "directwrite-font-collection-check.factory_ok=" << (factory_ok ? "true" : "false") << std::endl;
    std::cout << "directwrite-font-collection-check.factory_hr=" << static_cast<long>(factory_hr) << std::endl;
    std::cout << "directwrite-font-collection-check.sidecar_registered_file_count="
              << sidecar_summary.registered_file_count << std::endl;
    std::cout << "directwrite-font-collection-check.attachment_registered_file_count="
              << attachment_summary.registered_file_count << std::endl;
    std::cout << "directwrite-font-collection-check.registered_font_file_count="
              << collection_summary.registered_font_file_count << std::endl;
    std::cout << "directwrite-font-collection-check.added_font_file_count="
              << collection_summary.added_font_file_count << std::endl;
    std::cout << "directwrite-font-collection-check.factory3_available="
              << (collection_summary.factory3_available ? "true" : "false") << std::endl;
    std::cout << "directwrite-font-collection-check.collection_created="
              << (collection_summary.collection_created ? "true" : "false") << std::endl;
    std::cout << "directwrite-font-collection-check.error="
              << (collection_summary.error.empty() ? "none" : collection_summary.error) << std::endl;
    std::cout << "directwrite-font-collection-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;

    subtitle::releaseMediaAttachmentFonts(media_file);
    demuxer.close();
    return pass;
#else
    (void)media_file;
    std::cout << "directwrite-font-collection-check.supported_platform=false" << std::endl;
    std::cout << "directwrite-font-collection-check.result=FAIL" << std::endl;
    return false;
#endif
}

bool runBitmapSubtitleCheck(const std::string& media_file, int requested_stream_index = -1) {
    const std::vector<subtitle::EmbeddedSubtitleTrackInfo> tracks = subtitle::listEmbeddedSubtitleTracks(media_file);
    const size_t bitmap_track_count = static_cast<size_t>(std::count_if(
        tracks.begin(), tracks.end(), [](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.supported_bitmap_codec;
        }));

    const int selected_stream_index = requested_stream_index >= 0
        ? requested_stream_index
        : selectBestBitmapSubtitleStream(tracks);
    const auto selected_track_it = std::find_if(
        tracks.begin(),
        tracks.end(),
        [selected_stream_index](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.stream_index == selected_stream_index;
        });
    const bool stream_listed = selected_track_it != tracks.end();
    const bool stream_bitmap_supported = stream_listed && selected_track_it->supported_bitmap_codec;
    const subtitle::EmbeddedSubtitleLoadResult result =
        stream_bitmap_supported ? subtitle::loadEmbeddedSubtitleTrack(media_file, selected_stream_index)
                                : subtitle::EmbeddedSubtitleLoadResult{};

    std::vector<subtitle::SubtitleItem> items = sortSubtitleItems(result.items);
    const BitmapSubtitleValidationSummary summary = summarizeBitmapSubtitleItems(items);
    const auto ordered_timeline = buildSubtitleValidationTimeline(items);
    const auto seek_timeline = buildSeekValidationTimeline(ordered_timeline);

    size_t ordered_checks = 0;
    size_t seek_checks = 0;
    size_t mismatch_count = 0;
    size_t max_active_item_count = 0;
    size_t max_active_rect_count = 0;

    auto validate_at = [&](double position_seconds) {
        const std::vector<int> expected = resolveActiveSubtitleIndicesByScan(items, position_seconds);
        const std::vector<int> actual = subtitle::collectActiveSubtitleIndices(items, position_seconds);
        if (expected != actual) {
            ++mismatch_count;
        }
        max_active_item_count = std::max(max_active_item_count, actual.size());
        max_active_rect_count = std::max(max_active_rect_count, countActiveBitmapRects(items, actual));
    };

    for (double position_seconds : ordered_timeline) {
        ++ordered_checks;
        validate_at(position_seconds);
    }
    for (double position_seconds : seek_timeline) {
        ++seek_checks;
        validate_at(position_seconds);
    }

    const bool stats_match = summary.item_count == result.bitmap_item_count &&
                             summary.rect_count == result.bitmap_rect_count &&
                             summary.multi_rect_item_count == result.bitmap_multi_rect_item_count &&
                             summary.max_rects_per_item == result.bitmap_max_rects_per_item;
    const bool pass = stream_bitmap_supported &&
                      result.loaded &&
                      result.bitmap_codec &&
                      summary.item_count > 0 &&
                      summary.rect_count >= summary.item_count &&
                      mismatch_count == 0 &&
                      stats_match;

    std::cout << "bitmap-subtitle-check.path=" << media_file << std::endl;
    std::cout << "bitmap-subtitle-check.requested_stream_index=" << requested_stream_index << std::endl;
    std::cout << "bitmap-subtitle-check.selected_stream_index=" << selected_stream_index << std::endl;
    std::cout << "bitmap-subtitle-check.track_count=" << tracks.size() << std::endl;
    std::cout << "bitmap-subtitle-check.bitmap_track_count=" << bitmap_track_count << std::endl;
    std::cout << "bitmap-subtitle-check.stream_listed=" << (stream_listed ? "true" : "false") << std::endl;
    std::cout << "bitmap-subtitle-check.stream_bitmap_supported="
              << (stream_bitmap_supported ? "true" : "false") << std::endl;
    std::cout << "bitmap-subtitle-check.loaded=" << (result.loaded ? "true" : "false") << std::endl;
    std::cout << "bitmap-subtitle-check.codec_name="
              << (result.codec_name.empty() ? "unknown" : result.codec_name) << std::endl;
    std::cout << "bitmap-subtitle-check.bitmap_item_count=" << result.bitmap_item_count << std::endl;
    std::cout << "bitmap-subtitle-check.bitmap_rect_count=" << result.bitmap_rect_count << std::endl;
    std::cout << "bitmap-subtitle-check.bitmap_multi_rect_item_count="
              << result.bitmap_multi_rect_item_count << std::endl;
    std::cout << "bitmap-subtitle-check.bitmap_max_rects_per_item="
              << result.bitmap_max_rects_per_item << std::endl;
    std::cout << "bitmap-subtitle-check.unique_bitmap_signature_count="
              << summary.unique_bitmap_signature_count << std::endl;
    std::cout << "bitmap-subtitle-check.cache_reuse_candidate_count="
              << summary.cache_reuse_candidate_count << std::endl;
    std::cout << "bitmap-subtitle-check.ordered_checks=" << ordered_checks << std::endl;
    std::cout << "bitmap-subtitle-check.seek_checks=" << seek_checks << std::endl;
    std::cout << "bitmap-subtitle-check.max_active_item_count=" << max_active_item_count << std::endl;
    std::cout << "bitmap-subtitle-check.max_active_rect_count=" << max_active_rect_count << std::endl;
    std::cout << "bitmap-subtitle-check.mismatches=" << mismatch_count << std::endl;
    std::cout << "bitmap-subtitle-check.stats_match=" << (stats_match ? "true" : "false") << std::endl;
    std::cout << "bitmap-subtitle-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runBitmapSubtitleStressCheck() {
    auto make_bitmap = [](int x, int y, int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        subtitle::SubtitleBitmap bitmap{};
        bitmap.x = x;
        bitmap.y = y;
        bitmap.width = width;
        bitmap.height = height;
        bitmap.rgba.resize(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u, 0u);
        for (size_t i = 0; i < static_cast<size_t>(width) * static_cast<size_t>(height); ++i) {
            bitmap.rgba[i * 4u + 0u] = r;
            bitmap.rgba[i * 4u + 1u] = g;
            bitmap.rgba[i * 4u + 2u] = b;
            bitmap.rgba[i * 4u + 3u] = a;
        }
        return bitmap;
    };

    std::vector<subtitle::SubtitleItem> items;
    {
        subtitle::SubtitleItem item{};
        item.index = 0;
        item.start_seconds = 0.4;
        item.end_seconds = 2.4;
        item.is_bitmap = true;
        item.text = "[bitmap subtitle]";
        item.raw_text = item.text;
        item.source_path = "synthetic:bitmap-stress";
        item.play_res_x = 640;
        item.play_res_y = 360;
        item.bitmap_rects.push_back(make_bitmap(24, 220, 32, 16, 255, 240, 32, 220));
        item.bitmap_rects.push_back(make_bitmap(80, 220, 28, 16, 16, 180, 255, 200));
        items.push_back(std::move(item));
    }
    {
        subtitle::SubtitleItem item{};
        item.index = 1;
        item.start_seconds = 1.2;
        item.end_seconds = 3.0;
        item.is_bitmap = true;
        item.text = "[bitmap subtitle]";
        item.raw_text = item.text;
        item.source_path = "synthetic:bitmap-stress";
        item.play_res_x = 640;
        item.play_res_y = 360;
        item.bitmap_rects.push_back(make_bitmap(240, 220, 32, 16, 255, 240, 32, 220));
        items.push_back(std::move(item));
    }
    {
        subtitle::SubtitleItem item{};
        item.index = 2;
        item.start_seconds = 2.0;
        item.end_seconds = 4.5;
        item.is_bitmap = true;
        item.text = "[bitmap subtitle]";
        item.raw_text = item.text;
        item.source_path = "synthetic:bitmap-stress";
        item.play_res_x = 640;
        item.play_res_y = 360;
        item.bitmap_rects.push_back(make_bitmap(320, 220, 28, 16, 16, 180, 255, 200));
        item.bitmap_rects.push_back(make_bitmap(372, 220, 24, 16, 255, 96, 96, 210));
        items.push_back(std::move(item));
    }

    items = sortSubtitleItems(std::move(items));
    const BitmapSubtitleValidationSummary summary = summarizeBitmapSubtitleItems(items);
    const auto ordered_timeline = buildSubtitleValidationTimeline(items);
    const auto seek_timeline = buildSeekValidationTimeline(ordered_timeline);

    size_t ordered_checks = 0;
    size_t seek_checks = 0;
    size_t mismatch_count = 0;
    size_t max_active_item_count = 0;
    size_t max_active_rect_count = 0;

    auto validate_at = [&](double position_seconds) {
        const std::vector<int> expected = resolveActiveSubtitleIndicesByScan(items, position_seconds);
        const std::vector<int> actual = subtitle::collectActiveSubtitleIndices(items, position_seconds);
        if (expected != actual) {
            ++mismatch_count;
        }
        max_active_item_count = std::max(max_active_item_count, actual.size());
        max_active_rect_count = std::max(max_active_rect_count, countActiveBitmapRects(items, actual));
    };

    for (double position_seconds : ordered_timeline) {
        ++ordered_checks;
        validate_at(position_seconds);
    }
    for (double position_seconds : seek_timeline) {
        ++seek_checks;
        validate_at(position_seconds);
    }

    const bool pass = mismatch_count == 0 &&
                      summary.item_count == 3 &&
                      summary.rect_count == 5 &&
                      summary.multi_rect_item_count == 2 &&
                      summary.max_rects_per_item == 2 &&
                      summary.cache_reuse_candidate_count == 2 &&
                      max_active_item_count == 3 &&
                      max_active_rect_count == 5;

    std::cout << "bitmap-subtitle-stress-check.item_count=" << summary.item_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.rect_count=" << summary.rect_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.multi_rect_item_count="
              << summary.multi_rect_item_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.max_rects_per_item="
              << summary.max_rects_per_item << std::endl;
    std::cout << "bitmap-subtitle-stress-check.unique_bitmap_signature_count="
              << summary.unique_bitmap_signature_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.cache_reuse_candidate_count="
              << summary.cache_reuse_candidate_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.ordered_checks=" << ordered_checks << std::endl;
    std::cout << "bitmap-subtitle-stress-check.seek_checks=" << seek_checks << std::endl;
    std::cout << "bitmap-subtitle-stress-check.max_active_item_count=" << max_active_item_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.max_active_rect_count=" << max_active_rect_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.mismatches=" << mismatch_count << std::endl;
    std::cout << "bitmap-subtitle-stress-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runEmbeddedSubtitleCheck(const std::string& media_file) {
    const subtitle::EmbeddedSubtitleLoadResult result = subtitle::loadBestEmbeddedSubtitleTrack(media_file);
    const bool pass = result.loaded && !result.items.empty();
    std::string first_text = result.items.empty() ? std::string("none") : result.items.front().text;
    std::replace(first_text.begin(), first_text.end(), '\r', ' ');
    std::replace(first_text.begin(), first_text.end(), '\n', ' ');

    std::cout << "embedded-subtitle-check.path=" << media_file << std::endl;
    std::cout << "embedded-subtitle-check.subtitle_stream_found="
              << (result.subtitle_stream_found ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-check.supported_stream_found="
              << (result.supported_stream_found ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-check.loaded=" << (result.loaded ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-check.stream_index=" << result.stream_index << std::endl;
    std::cout << "embedded-subtitle-check.codec_name="
              << (result.codec_name.empty() ? "unknown" : result.codec_name) << std::endl;
    std::cout << "embedded-subtitle-check.language="
              << (result.language.empty() ? "und" : result.language) << std::endl;
    std::cout << "embedded-subtitle-check.title="
              << (result.title.empty() ? "none" : result.title) << std::endl;
    std::cout << "embedded-subtitle-check.source_label="
              << (result.source_label.empty() ? "none" : result.source_label) << std::endl;
    std::cout << "embedded-subtitle-check.item_count=" << result.items.size() << std::endl;
    std::cout << "embedded-subtitle-check.bitmap_codec="
              << (result.bitmap_codec ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-check.bitmap_item_count=" << result.bitmap_item_count << std::endl;
    std::cout << "embedded-subtitle-check.bitmap_rect_count=" << result.bitmap_rect_count << std::endl;
    std::cout << "embedded-subtitle-check.bitmap_multi_rect_item_count="
              << result.bitmap_multi_rect_item_count << std::endl;
    std::cout << "embedded-subtitle-check.bitmap_max_rects_per_item="
              << result.bitmap_max_rects_per_item << std::endl;
    std::cout << "embedded-subtitle-check.first_text=" << first_text << std::endl;
    std::cout << "embedded-subtitle-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runEmbeddedSubtitleListCheck(const std::string& media_file) {
    Demuxer demuxer;
    const bool open_ok = demuxer.open(media_file);
    const std::vector<subtitle::EmbeddedSubtitleTrackInfo> tracks =
        open_ok ? subtitle::listEmbeddedSubtitleTracks(media_file) : std::vector<subtitle::EmbeddedSubtitleTrackInfo>{};
    const int best_stream_index = subtitle::selectBestEmbeddedSubtitleStream(tracks);
    const size_t supported_track_count = static_cast<size_t>(std::count_if(
        tracks.begin(), tracks.end(), [](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.supported_codec;
        }));
    const size_t supported_text_track_count = static_cast<size_t>(std::count_if(
        tracks.begin(), tracks.end(), [](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.supported_text_codec;
        }));
    const size_t supported_bitmap_track_count = static_cast<size_t>(std::count_if(
        tracks.begin(), tracks.end(), [](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.supported_bitmap_codec;
        }));

    std::cout << "embedded-subtitle-list.path=" << media_file << std::endl;
    std::cout << "embedded-subtitle-list.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-list.track_count=" << tracks.size() << std::endl;
    std::cout << "embedded-subtitle-list.supported_track_count=" << supported_track_count << std::endl;
    std::cout << "embedded-subtitle-list.supported_text_track_count=" << supported_text_track_count << std::endl;
    std::cout << "embedded-subtitle-list.supported_bitmap_track_count=" << supported_bitmap_track_count << std::endl;
    std::cout << "embedded-subtitle-list.best_stream_index=" << best_stream_index << std::endl;
    for (size_t i = 0; i < tracks.size(); ++i) {
        const subtitle::EmbeddedSubtitleTrackInfo& track = tracks[i];
        const std::string prefix = "embedded-subtitle-list.track" + std::to_string(i);
        std::cout << prefix << ".stream_index=" << track.stream_index << std::endl;
        std::cout << prefix << ".codec_name="
                  << (track.codec_name.empty() ? "unknown" : track.codec_name) << std::endl;
        std::cout << prefix << ".language="
                  << (track.language.empty() ? "und" : track.language) << std::endl;
        std::cout << prefix << ".title="
                  << (track.title.empty() ? "none" : track.title) << std::endl;
        std::cout << prefix << ".supported="
                  << (track.supported_codec ? "true" : "false") << std::endl;
        std::cout << prefix << ".supported_text="
                  << (track.supported_text_codec ? "true" : "false") << std::endl;
        std::cout << prefix << ".supported_bitmap="
                  << (track.supported_bitmap_codec ? "true" : "false") << std::endl;
        std::cout << prefix << ".is_default="
                  << (track.is_default ? "true" : "false") << std::endl;
        std::cout << prefix << ".is_forced="
                  << (track.is_forced ? "true" : "false") << std::endl;
        std::cout << prefix << ".is_hearing_impaired="
                  << (track.is_hearing_impaired ? "true" : "false") << std::endl;
    }

    const bool pass = open_ok;
    std::cout << "embedded-subtitle-list.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    demuxer.close();
    return pass;
}

bool runEmbeddedSubtitleSelectCheck(const std::string& media_file, int stream_index) {
    const std::vector<subtitle::EmbeddedSubtitleTrackInfo> tracks = subtitle::listEmbeddedSubtitleTracks(media_file);
    auto selected_track_it = std::find_if(
        tracks.begin(),
        tracks.end(),
        [stream_index](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.stream_index == stream_index;
        });

    const bool stream_listed = selected_track_it != tracks.end();
    const bool stream_supported = stream_listed && selected_track_it->supported_codec;
    const subtitle::EmbeddedSubtitleLoadResult result =
        subtitle::loadEmbeddedSubtitleTrack(media_file, stream_index);
    const bool pass = stream_listed && stream_supported &&
                      result.loaded && !result.items.empty() &&
                      result.stream_index == stream_index;

    std::string first_text = result.items.empty() ? std::string("none") : result.items.front().text;
    std::replace(first_text.begin(), first_text.end(), '\r', ' ');
    std::replace(first_text.begin(), first_text.end(), '\n', ' ');

    std::cout << "embedded-subtitle-select-check.path=" << media_file << std::endl;
    std::cout << "embedded-subtitle-select-check.requested_stream_index=" << stream_index << std::endl;
    std::cout << "embedded-subtitle-select-check.track_count=" << tracks.size() << std::endl;
    std::cout << "embedded-subtitle-select-check.stream_listed="
              << (stream_listed ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-select-check.stream_supported="
              << (stream_supported ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-select-check.loaded=" << (result.loaded ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-select-check.stream_index=" << result.stream_index << std::endl;
    std::cout << "embedded-subtitle-select-check.codec_name="
              << (result.codec_name.empty() ? "unknown" : result.codec_name) << std::endl;
    std::cout << "embedded-subtitle-select-check.language="
              << (result.language.empty() ? "und" : result.language) << std::endl;
    std::cout << "embedded-subtitle-select-check.title="
              << (result.title.empty() ? "none" : result.title) << std::endl;
    std::cout << "embedded-subtitle-select-check.item_count=" << result.items.size() << std::endl;
    std::cout << "embedded-subtitle-select-check.bitmap_codec="
              << (result.bitmap_codec ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-select-check.bitmap_item_count=" << result.bitmap_item_count << std::endl;
    std::cout << "embedded-subtitle-select-check.bitmap_rect_count=" << result.bitmap_rect_count << std::endl;
    std::cout << "embedded-subtitle-select-check.bitmap_multi_rect_item_count="
              << result.bitmap_multi_rect_item_count << std::endl;
    std::cout << "embedded-subtitle-select-check.bitmap_max_rects_per_item="
              << result.bitmap_max_rects_per_item << std::endl;
    std::cout << "embedded-subtitle-select-check.first_text=" << first_text << std::endl;
    std::cout << "embedded-subtitle-select-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

bool runEmbeddedSubtitlePolicyCheck(const std::string& media_file,
                                    const subtitle::EmbeddedSubtitleSelectionPolicy& policy) {
    Demuxer demuxer;
    const bool open_ok = demuxer.open(media_file);
    const std::vector<subtitle::EmbeddedSubtitleTrackInfo> tracks =
        open_ok ? subtitle::listEmbeddedSubtitleTracks(media_file) : std::vector<subtitle::EmbeddedSubtitleTrackInfo>{};
    const size_t supported_track_count = static_cast<size_t>(std::count_if(
        tracks.begin(), tracks.end(), [](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            return track.supported_codec;
        }));
    const size_t policy_candidate_count = static_cast<size_t>(std::count_if(
        tracks.begin(),
        tracks.end(),
        [&policy](const subtitle::EmbeddedSubtitleTrackInfo& track) {
            if (!track.supported_codec) {
                return false;
            }
            if (policy.forced_policy == subtitle::EmbeddedSubtitleForcedPolicy::OnlyForced && !track.is_forced) {
                return false;
            }
            if (policy.sdh_policy == subtitle::EmbeddedSubtitleSdhPolicy::OnlySdh && !track.is_hearing_impaired) {
                return false;
            }
            return true;
        }));
    const int selected_stream_index = subtitle::selectBestEmbeddedSubtitleStream(tracks, policy);
    const subtitle::EmbeddedSubtitleLoadResult result =
        selected_stream_index >= 0 ? subtitle::loadEmbeddedSubtitleTrack(media_file, selected_stream_index)
                                   : subtitle::EmbeddedSubtitleLoadResult{};

    const bool selected_stream_loaded = selected_stream_index >= 0 && result.loaded;
    const bool pass = open_ok && ((policy_candidate_count == 0 && selected_stream_index < 0) ||
                                  (policy_candidate_count > 0 && selected_stream_loaded));

    std::cout << "embedded-subtitle-policy-check.path=" << media_file << std::endl;
    std::cout << "embedded-subtitle-policy-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "embedded-subtitle-policy-check.preferred_languages="
              << preferredSubtitleLanguagesToConfigValue(policy.preferred_languages) << std::endl;
    std::cout << "embedded-subtitle-policy-check.forced_policy="
              << embeddedSubtitleForcedPolicyName(policy.forced_policy) << std::endl;
    std::cout << "embedded-subtitle-policy-check.sdh_policy="
              << embeddedSubtitleSdhPolicyName(policy.sdh_policy) << std::endl;
    std::cout << "embedded-subtitle-policy-check.track_count=" << tracks.size() << std::endl;
    std::cout << "embedded-subtitle-policy-check.supported_track_count=" << supported_track_count << std::endl;
    std::cout << "embedded-subtitle-policy-check.policy_candidate_count=" << policy_candidate_count << std::endl;
    std::cout << "embedded-subtitle-policy-check.selected_stream_index=" << selected_stream_index << std::endl;
    std::cout << "embedded-subtitle-policy-check.selected_loaded=" << (selected_stream_loaded ? "true" : "false")
              << std::endl;
    std::cout << "embedded-subtitle-policy-check.selected_language="
              << (result.language.empty() ? "und" : result.language) << std::endl;
    std::cout << "embedded-subtitle-policy-check.selected_title="
              << (result.title.empty() ? "none" : result.title) << std::endl;
    std::cout << "embedded-subtitle-policy-check.selected_item_count=" << result.items.size() << std::endl;
    std::cout << "embedded-subtitle-policy-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;

    demuxer.close();
    return pass;
}

bool runSubtitleOwnershipCheck(const std::string& media_file) {
    const subtitle::EmbeddedSubtitleLoadResult embedded_probe = subtitle::loadBestEmbeddedSubtitleTrack(media_file);

    std::error_code ec;
    std::filesystem::path temp_dir = std::filesystem::temp_directory_path(ec);
    if (ec || temp_dir.empty()) {
        temp_dir = std::filesystem::path("build") / "tmp";
    }
    std::filesystem::create_directories(temp_dir, ec);
    const uint64_t nonce = static_cast<uint64_t>(std::chrono::steady_clock::now().time_since_epoch().count());
    const std::filesystem::path external_subtitle_path =
        temp_dir / ("subtitle-ownership-check-" + std::to_string(nonce) + ".srt");

    bool temp_subtitle_written = false;
    {
        std::ofstream output(external_subtitle_path.string(), std::ios::trunc);
        if (output.good()) {
            output << "1\n";
            output << "00:00:00,100 --> 00:00:01,400\n";
            output << "OWNERSHIP_EXTERNAL_1\n\n";
            output << "2\n";
            output << "00:00:01,600 --> 00:00:02,800\n";
            output << "OWNERSHIP_EXTERNAL_2\n";
            temp_subtitle_written = output.good();
        }
    }

    auto normalizePathForCompare = [](const std::string& path_value) {
        if (path_value.empty()) {
            return std::string();
        }
        std::error_code path_ec;
        const std::filesystem::path raw = std::filesystem::path(path_value).lexically_normal();
        const std::filesystem::path canonical = std::filesystem::weakly_canonical(raw, path_ec);
        const std::filesystem::path effective = path_ec ? raw : canonical;
        return toLower(effective.u8string());
    };

    VideoPlayer player;
    const bool open_ok = player.open(media_file);
    const std::string active_source_before_external = open_ok ? player.activeSubtitleSourcePath() : std::string();
    const size_t active_count_before_external = open_ok ? player.activeSubtitleItemCount() : 0U;
    const bool embedded_active_before_external =
        active_count_before_external > 0 && active_source_before_external.rfind("embedded:", 0) == 0;

    const bool external_loaded =
        open_ok && temp_subtitle_written && player.loadExternalSubtitle(external_subtitle_path.string());
    const std::string active_source_with_external = open_ok ? player.activeSubtitleSourcePath() : std::string();
    const size_t active_count_with_external = open_ok ? player.activeSubtitleItemCount() : 0U;
    const bool external_precedence_ok =
        external_loaded && player.hasExternalSubtitle() &&
        normalizePathForCompare(active_source_with_external) ==
            normalizePathForCompare(external_subtitle_path.string()) &&
        active_count_with_external > 0;

    if (open_ok) {
        player.clearExternalSubtitle();
    }
    const std::string active_source_after_clear = open_ok ? player.activeSubtitleSourcePath() : std::string();
    const size_t active_count_after_clear = open_ok ? player.activeSubtitleItemCount() : 0U;
    const bool fallback_to_embedded_ok =
        open_ok && !player.hasExternalSubtitle() && active_source_after_clear.rfind("embedded:", 0) == 0 &&
        active_count_after_clear == active_count_before_external && active_count_after_clear > 0;

    player.close();
    std::filesystem::remove(external_subtitle_path, ec);

    const bool pass = open_ok && embedded_probe.loaded && temp_subtitle_written &&
                      embedded_active_before_external && external_precedence_ok && fallback_to_embedded_ok;

    std::cout << "subtitle-ownership-check.path=" << media_file << std::endl;
    std::cout << "subtitle-ownership-check.open_ok=" << (open_ok ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.embedded_probe_loaded=" << (embedded_probe.loaded ? "true" : "false")
              << std::endl;
    std::cout << "subtitle-ownership-check.temp_subtitle_written="
              << (temp_subtitle_written ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.active_source_before_external="
              << (active_source_before_external.empty() ? "none" : active_source_before_external) << std::endl;
    std::cout << "subtitle-ownership-check.active_count_before_external=" << active_count_before_external << std::endl;
    std::cout << "subtitle-ownership-check.embedded_active_before_external="
              << (embedded_active_before_external ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.external_loaded=" << (external_loaded ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.external_precedence_ok="
              << (external_precedence_ok ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.active_source_after_clear="
              << (active_source_after_clear.empty() ? "none" : active_source_after_clear) << std::endl;
    std::cout << "subtitle-ownership-check.active_count_after_clear=" << active_count_after_clear << std::endl;
    std::cout << "subtitle-ownership-check.fallback_to_embedded_ok="
              << (fallback_to_embedded_ok ? "true" : "false") << std::endl;
    std::cout << "subtitle-ownership-check.result=" << (pass ? "PASS" : "FAIL") << std::endl;
    return pass;
}

}  // namespace



/// 进程信号处理入口；优先停止播放并关闭日志系统。

void signalHandler([[maybe_unused]] int signal) {

    if (g_player) {

        g_player->stop();

    }

    Logger::shutdown();

    exit(0);

}



/// 打印 CLI 用法、验证命令和播放器交互快捷键说明。

void printVersion() {

    std::cout << vp::build::kProductName << ' ' << vp::build::kVersion << std::endl;

}

void printUsage(const char* program_name) {

    std::cout << vp::build::kProductName << ' ' << vp::build::kVersion << std::endl;

    std::cout << std::endl;

    std::cout << "Usage: " << program_name << " [video_file] [more_video_files...]" << std::endl;

    std::cout << "       " << program_name << " [media_files...] --subtitle <subtitle.(srt|ass|ssa)>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --subtitle-track <stream_index>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --subtitle-language <lang[,lang2...]|none>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --subtitle-forced <auto|prefer|avoid|only> --subtitle-sdh <auto|prefer|avoid|only>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --opengl-hdr-output-mode <auto|off|force>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --d3d11-hdr-output-mode <auto|off|force>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --opengl-3dlut <cube_lut_file>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --opengl-icc-profile <icc_profile_file>" << std::endl;
    std::cout << "       " << program_name << " [media_files...] --opengl-auto-icc" << std::endl;

    std::cout << "       " << program_name << " <playlist.m3u8>" << std::endl;

    std::cout << "       " << program_name << " --capabilities" << std::endl;

    std::cout << "Launch without media to open an idle window, then drag files into it to play." << std::endl;

    std::cout << "       " << program_name << " --probe-file <media_file> [--json]" << std::endl;

    std::cout << "       " << program_name << " --subtitle-sync-check <subtitle.(srt|ass|ssa)>" << std::endl;

    std::cout << "       " << program_name << " --subtitle-style-check <subtitle.(srt|ass|ssa)>" << std::endl;

    std::cout << "       " << program_name << " --embedded-subtitle-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --bitmap-subtitle-check <media_file> [stream_index]" << std::endl;
    std::cout << "       " << program_name << " --bitmap-subtitle-stress-check" << std::endl;
    std::cout << "       " << program_name << " --embedded-subtitle-list <media_file>" << std::endl;
    std::cout << "       " << program_name << " --embedded-subtitle-select-check <media_file> <stream_index>" << std::endl;
    std::cout << "       " << program_name << " --embedded-subtitle-policy-check <media_file> [lang_list] [forced_policy] [sdh_policy]" << std::endl;
    std::cout << "       " << program_name << " --subtitle-ownership-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --attachment-font-check <media_file>" << std::endl;
    std::cout << "       " << program_name << " --directwrite-font-collection-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --playlist-flow-check <media1> <media2> <media3> <media4> <media5> [more...]" << std::endl;

    std::cout << "       " << program_name << " --settings-persistence-check [settings_file]" << std::endl;

    std::cout << "       " << program_name << " --renderer-fallback-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --windows-backend-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --chapter-nav-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --ab-repeat-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --frame-step-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --delay-adjust-check <media_file> <subtitle.(srt|ass|ssa)>" << std::endl;

    std::cout << "       " << program_name << " --numeric-seek-check <media_file>" << std::endl;

    std::cout << "       " << program_name << " --seek-burst-serial-check <media_file> [seek_count]" << std::endl;

    std::cout << "       " << program_name << " --paused-seek-serial-check <media_file> [seek_count]" << std::endl;

    std::cout << "       " << program_name << " --close-reopen-serial-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name
              << " --serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]"
              << std::endl;

    std::cout << "       " << program_name << " --forced-failsession-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --d3d11-diagnostics" << std::endl;
    std::cout << "       " << program_name << " --d3d11-hdr-output-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --opengl-diagnostics" << std::endl;
    std::cout << "       " << program_name << " --vulkan-diagnostics" << std::endl;
    std::cout << "       " << program_name << " --opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --opengl-output-color-icc-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --version" << std::endl;

    std::cout << "       " << program_name << " --performance-log-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --interaction-freeze-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --linux-software-audio-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --linux-opengl-playback-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --linux-opengl-fallback-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --linux-vaapi-fallback-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --libass-shaping-check <subtitle.(ass|ssa)>" << std::endl;
    std::cout << "       " << program_name
              << " --embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]" << std::endl;
    std::cout << "       " << program_name << " --linux-audio-backend-smoke <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --core-playback-behavior-check <media_file> [sample_ms]" << std::endl;
    std::cout << "       " << program_name << " --ui-interaction-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --software-video-decode-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --software-video-send-probe <media_file> [send_timeout_ms]" << std::endl;

    std::cout << "       " << program_name << " --1080p60-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --4k-playback-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --high-bitrate-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --long-playback-check <media_file> [sample_ms]" << std::endl;

    std::cout << "       " << program_name << " --plugin-check [plugin_dir_or_file]" << std::endl;

    std::cout << "       " << program_name << " --streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]" << std::endl;

    std::cout << "       " << program_name << " --adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]" << std::endl;

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



/// 程序主入口；分发 CLI 检查命令或进入正常播放模式。

int main(int argc, char* argv[]) {

    if (argc >= 2 && std::string(argv[1]) == "--version") {

        printVersion();

        return 0;

    }

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

    if (argc >= 2 && std::string(argv[1]) == "--subtitle-style-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runSubtitleStyleCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--embedded-subtitle-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runEmbeddedSubtitleCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--bitmap-subtitle-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int stream_index = -1;
        if (argc == 4 && !tryParseInt(argv[3], stream_index)) {

            printUsage(argv[0]);

            return 1;

        }

        return runBitmapSubtitleCheck(argv[2], stream_index) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--bitmap-subtitle-stress-check") {

        if (argc != 2) {

            printUsage(argv[0]);

            return 1;

        }

        return runBitmapSubtitleStressCheck() ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--embedded-subtitle-list") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runEmbeddedSubtitleListCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--embedded-subtitle-select-check") {

        if (argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int stream_index = -1;
        if (!tryParseInt(argv[3], stream_index)) {

            printUsage(argv[0]);

            return 1;

        }

        return runEmbeddedSubtitleSelectCheck(argv[2], stream_index) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--embedded-subtitle-policy-check") {

        if (argc < 3 || argc > 6) {

            printUsage(argv[0]);

            return 1;

        }

        subtitle::EmbeddedSubtitleSelectionPolicy policy{};
        if (argc >= 4 && !parsePreferredSubtitleLanguages(argv[3], policy.preferred_languages)) {

            printUsage(argv[0]);

            return 1;

        }
        if (argc >= 5 && !parseEmbeddedSubtitleForcedPolicy(argv[4], policy.forced_policy)) {

            printUsage(argv[0]);

            return 1;

        }
        if (argc >= 6 && !parseEmbeddedSubtitleSdhPolicy(argv[5], policy.sdh_policy)) {

            printUsage(argv[0]);

            return 1;

        }

        return runEmbeddedSubtitlePolicyCheck(argv[2], policy) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--subtitle-ownership-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runSubtitleOwnershipCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--attachment-font-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runAttachmentFontCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--directwrite-font-collection-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runDirectWriteFontCollectionCheck(argv[2]) ? 0 : 2;

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

        runWindowsBackendSessionCheckAndExit(argv[2], argv[3]);

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



    if (argc >= 2 && std::string(argv[1]) == "--seek-burst-serial-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int seek_count = 6;

        if (argc == 4 && !tryParseInt(argv[3], seek_count)) {

            printUsage(argv[0]);

            return 1;

        }

        return runSeekBurstSerialCheck(argv[2], seek_count) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--paused-seek-serial-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int seek_count = 4;

        if (argc == 4 && !tryParseInt(argv[3], seek_count)) {

            printUsage(argv[0]);

            return 1;

        }

        return runPausedSeekSerialCheck(argv[2], seek_count) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--close-reopen-serial-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 900;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runCloseReopenSerialCheck(argv[2], sample_ms) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--serial-failsession-regression-check") {

        if (argc < 3 || argc > 6) {

            printUsage(argv[0]);

            return 1;

        }

        int seek_count = 6;
        int paused_seek_count = 4;
        int sample_ms = 900;

        if (argc >= 4 && !tryParseInt(argv[3], seek_count)) {

            printUsage(argv[0]);

            return 1;

        }

        if (argc >= 5 && !tryParseInt(argv[4], paused_seek_count)) {

            printUsage(argv[0]);

            return 1;

        }

        if (argc >= 6 && !tryParseInt(argv[5], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runSerialFailSessionRegressionCheck(argv[2], seek_count, paused_seek_count, sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--forced-failsession-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1500;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runForcedFailSessionCheck(argv[2], sample_ms) ? 0 : 2;

    }


    if (argc >= 2 && std::string(argv[1]) == "--d3d11-diagnostics") {

        if (argc != 2) {

            printUsage(argv[0]);

            return 1;

        }

        return runD3D11Diagnostics() ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--d3d11-hdr-output-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1200;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runD3D11HdrOutputCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--opengl-diagnostics") {

        if (argc != 2) {

            printUsage(argv[0]);

            return 1;

        }

        return runOpenGLDiagnostics() ? 0 : 2;

    }
    if (argc >= 2 && std::string(argv[1]) == "--vulkan-diagnostics") {

        if (argc != 2) {

            printUsage(argv[0]);

            return 1;

        }

        return runVulkanDiagnostics() ? 0 : 2;

    }
    if (argc >= 2 && std::string(argv[1]) == "--opengl-output-color-check") {

        if (argc != 4 && argc != 5) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1200;

        if (argc == 5 && !tryParseInt(argv[4], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runOpenGLOutputColorCheck(argv[2], argv[3], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--opengl-output-color-icc-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1200;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runOpenGLOutputColorIccCheck(argv[2], sample_ms) ? 0 : 2;

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

    if (argc >= 2 && std::string(argv[1]) == "--interaction-freeze-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runInteractionFreezeCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--linux-software-audio-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLinuxSoftwareAudioCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--linux-opengl-playback-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLinuxOpenGLPlaybackCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--linux-opengl-fallback-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLinuxOpenGLFallbackCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--linux-vaapi-fallback-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLinuxVaapiFallbackCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--libass-shaping-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runLibassShapingCheck(argv[2]) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--embedded-subtitle-live-packet-check") {

        if (argc != 3 && argc != 4 && argc != 5) {

            printUsage(argv[0]);

            return 1;

        }

        int stream_index = -1;
        int max_packets = 120;
        if (argc >= 4 && !tryParseInt(argv[3], stream_index)) {

            printUsage(argv[0]);

            return 1;

        }
        if (argc == 5 && !tryParseInt(argv[4], max_packets)) {

            printUsage(argv[0]);

            return 1;

        }

        return runEmbeddedSubtitleLivePacketCheck(argv[2], stream_index, max_packets) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--linux-audio-backend-smoke") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLinuxAudioBackendSmoke(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--core-playback-behavior-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runCorePlaybackBehaviorCheck(argv[2], sample_ms) ? 0 : 2;

    }

    if (argc >= 2 && std::string(argv[1]) == "--ui-interaction-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 1800;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runUiInteractionCheck(argv[2], sample_ms) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--software-video-decode-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 2000;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        runSoftwareVideoDecodeCheckAndExit(argv[2], sample_ms);

    }

    if (argc >= 2 && std::string(argv[1]) == "--software-video-send-probe") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int send_timeout_ms = 1500;

        if (argc == 4 && !tryParseInt(argv[3], send_timeout_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        runSoftwareVideoSendProbeAndExit(argv[2], send_timeout_ms);

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



    if (argc >= 2 && std::string(argv[1]) == "--high-bitrate-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 3000;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runHighBitrateCheck(argv[2], sample_ms) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--long-playback-check") {

        if (argc != 3 && argc != 4) {

            printUsage(argv[0]);

            return 1;

        }

        int sample_ms = 10000;

        if (argc == 4 && !tryParseInt(argv[3], sample_ms)) {

            printUsage(argv[0]);

            return 1;

        }

        return runLongPlaybackCheck(argv[2], sample_ms) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--plugin-check") {

        if (argc > 3) {

            printUsage(argv[0]);

            return 1;

        }

        const std::string plugin_input = (argc == 3) ? argv[2] : std::string{};

        return runPluginCheck(argv[0], plugin_input) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--streaming-buffer-check") {

        if (argc != 3 && argc != 4 && argc != 5) {

            printUsage(argv[0]);

            return 1;

        }

        int segment_limit = 3;

        int target_buffer_bytes = 256;

        if (argc >= 4 && !tryParseInt(argv[3], segment_limit)) {

            printUsage(argv[0]);

            return 1;

        }

        if (argc == 5 && !tryParseInt(argv[4], target_buffer_bytes)) {

            printUsage(argv[0]);

            return 1;

        }

        return runStreamingBufferCheck(argv[2], segment_limit, target_buffer_bytes) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--adaptive-bitrate-check") {

        if (argc != 4 && argc != 5 && argc != 6) {

            printUsage(argv[0]);

            return 1;

        }

        int segment_limit = 2;

        int target_buffer_bytes = 256;

        if (argc >= 5 && !tryParseInt(argv[4], segment_limit)) {

            printUsage(argv[0]);

            return 1;

        }

        if (argc == 6 && !tryParseInt(argv[5], target_buffer_bytes)) {

            printUsage(argv[0]);

            return 1;

        }

        return runAdaptiveBitrateCheck(argv[2], argv[3], segment_limit, target_buffer_bytes) ? 0 : 2;

    }



    if (argc >= 2 && std::string(argv[1]) == "--screenshot-check") {

        if (argc != 3) {

            printUsage(argv[0]);

            return 1;

        }

        return runScreenshotCheck(argv[2]) ? 0 : 2;

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

    const bool interactive_idle_mode = playlist_manager.empty();

    size_t current_index = 0;

    if (!interactive_idle_mode &&
        app_settings.resume_last_playlist &&
        app_settings.last_playlist_index >= 0) {

        const size_t saved_index = static_cast<size_t>(app_settings.last_playlist_index);

        if (saved_index < playlist_manager.size()) {

            current_index = saved_index;

        }

    }

    std::optional<ScopedEnvOverride> opengl_hdr_output_mode_override;
    if (cli_args.has_opengl_hdr_output_mode) {
        opengl_hdr_output_mode_override.emplace("MVP_OPENGL_HDR_OUTPUT_MODE", cli_args.opengl_hdr_output_mode);
        Logger::info("OpenGL HDR output mode from CLI: " + cli_args.opengl_hdr_output_mode);
    }

    std::optional<ScopedEnvOverride> d3d11_hdr_output_mode_override;
    if (cli_args.has_d3d11_hdr_output_mode) {
        d3d11_hdr_output_mode_override.emplace("MVP_D3D11_HDR_OUTPUT_MODE", cli_args.d3d11_hdr_output_mode);
        Logger::info("D3D11 HDR output mode from CLI: " + cli_args.d3d11_hdr_output_mode);
    }

    std::optional<ScopedEnvOverride> opengl_3dlut_override;
    if (!cli_args.opengl_3dlut_file.empty()) {
        opengl_3dlut_override.emplace("MVP_OPENGL_3DLUT_FILE", cli_args.opengl_3dlut_file);
        Logger::info("OpenGL 3D LUT from CLI: " + cli_args.opengl_3dlut_file);
    }

    std::optional<ScopedEnvOverride> opengl_icc_profile_override;
    if (!cli_args.opengl_icc_profile_file.empty()) {
        opengl_icc_profile_override.emplace("MVP_OPENGL_ICC_PROFILE_FILE", cli_args.opengl_icc_profile_file);
        Logger::info("OpenGL ICC profile from CLI: " + cli_args.opengl_icc_profile_file);
    }

    std::optional<ScopedEnvOverride> opengl_auto_icc_override;
    if (cli_args.enable_opengl_auto_icc) {
        opengl_auto_icc_override.emplace("MVP_OPENGL_AUTO_ICC", "1");
        Logger::info("OpenGL auto ICC enabled from CLI");
    }



    g_player = std::make_unique<VideoPlayer>();

    g_player->setVolume(app_settings.volume);

    g_player->setPlaybackSpeed(app_settings.playback_speed);

    g_player->setAudioDelay(app_settings.audio_delay_seconds);

    g_player->setSubtitleDelay(app_settings.subtitle_delay_seconds);

    g_player->setPreferHardwareDecode(app_settings.prefer_hardware_decode);

    g_player->setHotkeyManager(app_settings.hotkey_manager);
    subtitle::EmbeddedSubtitleSelectionPolicy subtitle_selection_policy =
        app_settings.embedded_subtitle_selection_policy;
    if (cli_args.has_preferred_subtitle_languages) {
        subtitle_selection_policy.preferred_languages = cli_args.preferred_subtitle_languages;
    }
    if (cli_args.has_subtitle_forced_policy) {
        subtitle_selection_policy.forced_policy = cli_args.subtitle_forced_policy;
    }
    if (cli_args.has_subtitle_sdh_policy) {
        subtitle_selection_policy.sdh_policy = cli_args.subtitle_sdh_policy;
    }
    g_player->setEmbeddedSubtitleSelectionPolicy(subtitle_selection_policy);
    Logger::info("Embedded subtitle selection policy: languages=" +
                 preferredSubtitleLanguagesToConfigValue(subtitle_selection_policy.preferred_languages) +
                 ", forced=" + embeddedSubtitleForcedPolicyName(subtitle_selection_policy.forced_policy) +
                 ", sdh=" + embeddedSubtitleSdhPolicyName(subtitle_selection_policy.sdh_policy));

    if (cli_args.has_preferred_embedded_subtitle_stream_index) {
        g_player->setPreferredEmbeddedSubtitleStreamIndex(cli_args.preferred_embedded_subtitle_stream_index);
        Logger::info("Preferred embedded subtitle stream index from CLI: " +
                     std::to_string(cli_args.preferred_embedded_subtitle_stream_index));
    }



    bool quit_requested = false;

    bool opened_any_media = false;

    std::string session_subtitle_override = cli_args.subtitle_file;



    while (!quit_requested) {

        if (playlist_manager.empty()) {

            if (!interactive_idle_mode) {

                break;

            }



            std::string accepted_input;

            if (!waitForIdlePlaylistSelection(g_player->hotkeyManager(),

                                             playlist_manager,

                                             accepted_input,

                                             quit_requested)) {

                if (quit_requested) {

                    break;

                }

                continue;

            }



            current_index = 0;

            Logger::info("Opening dropped media from idle window: " + accepted_input);

        }



        bool replacement_requested = false;

        std::string replacement_input;

        playlist::PlaylistManager replacement_playlist;



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

                !session_subtitle_override.empty() ? session_subtitle_override : detectAutoSubtitlePath(item.uri);

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



                std::string dropped_path;

                if (g_player->consumeOpenFileRequest(dropped_path)) {

                    std::string error;

                    if (tryResolveInteractiveMediaInput(dropped_path, replacement_playlist, replacement_input, error)) {

                        replacement_requested = true;

                        Logger::info("Dropped media will replace current playback: " + replacement_input);

                        g_player->stop();

                        break;

                    }



                    Logger::warning("Ignoring dropped input during playback: " + dropped_path + " reason=" + error);

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



            if (quit_requested || replacement_requested) {

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



            if (current_index + 1 < playlist_manager.size()) {

                ++current_index;

                continue;

            }

            break;

        }



        if (quit_requested) {

            break;

        }



        if (replacement_requested) {

            playlist_manager = std::move(replacement_playlist);

            current_index = 0;

            session_subtitle_override.clear();

            Logger::info("Switching to dropped media playlist: " + replacement_input);

            continue;

        }



        if (interactive_idle_mode) {

            playlist_manager = playlist::PlaylistManager{};

            current_index = 0;

            continue;

        }



        break;

    }



    const float final_volume = g_player ? g_player->getVolume() : app_settings.volume;

    const double final_speed = g_player ? g_player->getPlaybackSpeed() : app_settings.playback_speed;

    const double final_audio_delay = g_player ? g_player->getAudioDelay() : app_settings.audio_delay_seconds;

    const double final_subtitle_delay = g_player ? g_player->getSubtitleDelay() : app_settings.subtitle_delay_seconds;

    const subtitle::EmbeddedSubtitleSelectionPolicy final_subtitle_policy =
        g_player ? g_player->embeddedSubtitleSelectionPolicy() : app_settings.embedded_subtitle_selection_policy;

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

                    final_subtitle_policy,

                    final_hotkeys);



    if (!opened_any_media) {

        if (!interactive_idle_mode) {

            Logger::error("No media could be opened");

            Logger::shutdown();

            return 1;

        }



        Logger::info("Player closed without opening media");

        Logger::shutdown();

        return 0;

    }



    Logger::info("Playback finished");

    Logger::shutdown();

    

    std::cout << std::endl;

    std::cout << "Playback finished." << std::endl;

    

    return 0;

}
