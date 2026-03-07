#include "video_player.h"
#include "demuxer.h"
#include "media/format_support.h"
#include "logger.h"

#include <algorithm>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
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

int printFileProbeReport(const std::string& path) {
    Demuxer demuxer;
    const bool open_ok = demuxer.open(path);

    std::cout << "probe.path=" << path << std::endl;
    std::cout << "probe.open=" << (open_ok ? "PASS" : "FAIL") << std::endl;
    if (!open_ok) {
        std::cout << "probe.overall=FAIL" << std::endl;
        return 3;
    }

    const MediaInfo& info = demuxer.getMediaInfo();
    AVFormatContext* fmt_ctx = demuxer.getFormatContext();
    const std::string ext = extensionFromPath(path);
    const bool container_supported = media::FormatSupport::isContainerSupported(ext);

    std::string video_codec_name = "none";
    std::string audio_codec_name = "none";
    bool video_status_ok = true;
    bool audio_status_ok = true;
    uint64_t video_bitrate = 0;

    if (fmt_ctx && info.video_stream_idx >= 0 &&
        info.video_stream_idx < static_cast<int>(fmt_ctx->nb_streams)) {
        AVStream* vs = fmt_ctx->streams[info.video_stream_idx];
        video_codec_name = avcodec_get_name(vs->codecpar->codec_id);
        video_status_ok = media::FormatSupport::isVideoCodecLikelySupported(video_codec_name);
        if (vs->codecpar->bit_rate > 0) {
            video_bitrate = static_cast<uint64_t>(vs->codecpar->bit_rate);
        }
    }

    if (fmt_ctx && info.audio_stream_idx >= 0 &&
        info.audio_stream_idx < static_cast<int>(fmt_ctx->nb_streams)) {
        AVStream* as = fmt_ctx->streams[info.audio_stream_idx];
        audio_codec_name = avcodec_get_name(as->codecpar->codec_id);
        audio_status_ok = media::FormatSupport::isAudioCodecLikelySupported(audio_codec_name);
    }

    media::PlaybackCapabilityTarget target{};
    target.width = info.width;
    target.height = info.height;
    target.fps = info.fps;
    target.audio_channels = std::max(1, info.channels);
    target.video_bitrate = video_bitrate;
    const auto decision = media::FormatSupport::evaluatePlaybackTarget(target);

    const bool partial = !container_supported || !video_status_ok || !audio_status_ok;
    const std::string overall = partial ? "PARTIAL" : "PASS";

    std::cout << "probe.overall=" << overall << std::endl;
    std::cout << "probe.container_ext=" << ext << std::endl;
    std::cout << "probe.container_status=" << (container_supported ? "PASS" : "PARTIAL") << std::endl;
    std::cout << "probe.video_stream=" << info.video_stream_idx << std::endl;
    std::cout << "probe.video_codec=" << video_codec_name << std::endl;
    std::cout << "probe.video_status=" << (video_status_ok ? "PASS" : "PARTIAL") << std::endl;
    std::cout << "probe.audio_stream=" << info.audio_stream_idx << std::endl;
    std::cout << "probe.audio_codec=" << audio_codec_name << std::endl;
    std::cout << "probe.audio_status=" << (audio_status_ok ? "PASS" : "PARTIAL") << std::endl;
    std::cout << "probe.width=" << info.width << std::endl;
    std::cout << "probe.height=" << info.height << std::endl;
    std::cout << "probe.fps=" << info.fps << std::endl;
    std::cout << "probe.audio_channels=" << info.channels << std::endl;
    std::cout << "probe.duration=" << info.duration << std::endl;
    std::cout << "probe.realtime=" << (decision.suitable_for_realtime ? "true" : "false") << std::endl;
    std::cout << "probe.recommend_hw=" << (decision.recommends_hardware_decode ? "true" : "false") << std::endl;
    std::cout << "probe.recommend_d3d11=" << (decision.recommends_d3d11_renderer ? "true" : "false") << std::endl;
    std::cout << "probe.reason=" << decision.reason << std::endl;

    return partial ? 2 : 0;
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
    std::cout << "Usage: " << program_name << " <video_file>" << std::endl;
    std::cout << "       " << program_name << " --capabilities" << std::endl;
    std::cout << "       " << program_name << " --probe-file <media_file>" << std::endl;
    std::cout << "       " << program_name
              << " --evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>" << std::endl;
    std::cout << std::endl;
    std::cout << "Controls:" << std::endl;
    std::cout << "  SPACE - Play/Pause" << std::endl;
    std::cout << "  Mouse drag progress bar - Seek" << std::endl;
    std::cout << "  Mouse drag volume bar - Volume" << std::endl;
    std::cout << "  +/- or Up/Down - Adjust volume" << std::endl;
    std::cout << "  Q/ESC - Quit" << std::endl;
    std::cout << "  F - Toggle Fullscreen" << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[]) {
    Logger::init();

    if (argc >= 2 && std::string(argv[1]) == "--capabilities") {
        printCapabilityMatrix();
        Logger::shutdown();
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--evaluate-target") {
        if (argc < 7) {
            printUsage(argv[0]);
            Logger::shutdown();
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
            Logger::shutdown();
            return 1;
        }

        media::PlaybackCapabilityTarget target{};
        target.width = width;
        target.height = height;
        target.fps = fps;
        target.audio_channels = channels;
        target.video_bitrate = static_cast<uint64_t>(std::max(0.0, bitrate_mbps) * 1000000.0);
        printTargetDecision(target);
        Logger::shutdown();
        return 0;
    }

    if (argc >= 2 && std::string(argv[1]) == "--probe-file") {
        if (argc < 3) {
            printUsage(argv[0]);
            Logger::shutdown();
            return 1;
        }
        const int result = printFileProbeReport(argv[2]);
        Logger::shutdown();
        return result;
    }
    
    if (argc < 2) {
        printUsage(argv[0]);
        Logger::shutdown();
        return 1;
    }
    
    std::cout << "========================================" << std::endl;
    std::cout << "  Video Player - FFmpeg + SDL2 + C++17" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;
    
    Logger::info("Starting Modern Video Player");
    
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    g_player = std::make_unique<VideoPlayer>();
    
    std::string filename(argv[1]);
    Logger::info("Opening file: " + filename);
    
    if (!g_player->open(filename)) {
        Logger::error("Could not open video file");
        Logger::shutdown();
        return 1;
    }
    
    Logger::info("Starting playback...");
    std::cout << "Playing video..." << std::endl;
    std::cout << std::endl;
    
    g_player->play();
    
    while (g_player->isPlaying() || g_player->isPaused()) {
        g_player->pumpEvents();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    Logger::info("Playback finished");
    g_player->stop();
    g_player->close();
    
    Logger::shutdown();
    
    std::cout << std::endl;
    std::cout << "Playback finished." << std::endl;
    
    return 0;
}
