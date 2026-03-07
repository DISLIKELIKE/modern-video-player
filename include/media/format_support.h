#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vp::media {

struct FormatCapabilityReport {
    std::vector<std::string> containers;
    std::vector<std::string> video_codecs;
    std::vector<std::string> audio_codecs;
};

struct PlaybackCapabilityTarget {
    int width{0};
    int height{0};
    double fps{0.0};
    int audio_channels{2};
    uint64_t video_bitrate{0};
};

struct PlaybackCapabilityDecision {
    bool suitable_for_realtime{true};
    bool recommends_hardware_decode{false};
    bool recommends_d3d11_renderer{false};
    std::string reason;
};

class FormatSupport {
public:
    static bool isContainerSupported(const std::string& extension);
    static bool isVideoCodecLikelySupported(const std::string& codec_name);
    static bool isAudioCodecLikelySupported(const std::string& codec_name);
    static std::vector<std::string> supportedContainers();
    static std::vector<std::string> supportedVideoCodecs();
    static std::vector<std::string> supportedAudioCodecs();
    static bool supportsMkvChapters();
    static bool supportsMp4MoovPreload();

    static FormatCapabilityReport queryRuntimeCapabilities();
    static PlaybackCapabilityDecision evaluatePlaybackTarget(const PlaybackCapabilityTarget& target);
};

}  // namespace vp::media
