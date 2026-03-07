#include "video_player.h"
#include "config/settings_manager.h"
#include "demuxer.h"
#include "media/format_support.h"
#include "playlist/playlist_manager.h"
#include "logger.h"

extern "C" {
#include <libavutil/log.h>
}

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <system_error>
#include <thread>
#include <vector>

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

struct AppSettings {
    float volume{1.0f};
    double playback_speed{1.0};
    bool resume_last_playlist{true};
    int last_playlist_index{0};
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

AppSettings loadAppSettings(config::SettingsManager& settings_manager, const std::string& settings_path) {
    AppSettings settings{};

    const bool loaded = settings_manager.loadIni(settings_path);
    if (!loaded) {
        settings_manager.setInt("player.volume_percent", 100);
        settings_manager.setDouble("player.playback_speed", 1.0);
        settings_manager.setBool("player.resume_last_playlist", true);
        settings_manager.setInt("player.last_playlist_index", 0);
        return settings;
    }

    if (const auto volume_percent = settings_manager.getInt("player.volume_percent")) {
        settings.volume = std::max(0.0f, std::min(1.0f, static_cast<float>(*volume_percent) / 100.0f));
    }

    if (const auto speed = settings_manager.getDouble("player.playback_speed")) {
        settings.playback_speed = std::max(0.5, std::min(2.0, *speed));
    }

    if (const auto resume_last = settings_manager.getBool("player.resume_last_playlist")) {
        settings.resume_last_playlist = *resume_last;
    }

    if (const auto index = settings_manager.getInt("player.last_playlist_index")) {
        settings.last_playlist_index = std::max(0, *index);
    }

    settings_manager.setInt("player.volume_percent", static_cast<int>(std::lround(settings.volume * 100.0f)));
    settings_manager.setDouble("player.playback_speed", settings.playback_speed);
    settings_manager.setBool("player.resume_last_playlist", settings.resume_last_playlist);
    settings_manager.setInt("player.last_playlist_index", settings.last_playlist_index);

    return settings;
}

void saveAppSettings(config::SettingsManager& settings_manager,
                     const std::string& settings_path,
                     float volume,
                     double playback_speed,
                     bool resume_last_playlist,
                     int last_playlist_index) {
    const float clamped_volume = std::max(0.0f, std::min(1.0f, volume));
    const double clamped_speed = std::max(0.5, std::min(2.0, playback_speed));

    settings_manager.setInt("player.volume_percent", static_cast<int>(std::lround(clamped_volume * 100.0f)));
    settings_manager.setDouble("player.playback_speed", clamped_speed);
    settings_manager.setBool("player.resume_last_playlist", resume_last_playlist);
    settings_manager.setInt("player.last_playlist_index", std::max(0, last_playlist_index));

    if (!settings_manager.saveIni(settings_path)) {
        LOG_WARNING("Failed to save settings file: " << settings_path);
    }
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
    std::cout << "  [/ ] / R - Speed down/up/reset" << std::endl;
    std::cout << "  V - Toggle subtitles on/off" << std::endl;
    std::cout << "  M - Mute/Unmute" << std::endl;
    std::cout << "  Mouse drag progress bar - Seek" << std::endl;
    std::cout << "  Mouse drag volume bar - Volume" << std::endl;
    std::cout << "  +/- or Up/Down - Adjust volume" << std::endl;
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
    saveAppSettings(settings_manager,
                    settings_path,
                    final_volume,
                    final_speed,
                    app_settings.resume_last_playlist,
                    static_cast<int>(current_index));

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
