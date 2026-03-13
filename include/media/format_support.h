#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace vp::media {

/// 运行时格式能力快照。
struct FormatCapabilityReport {
    /// 当前可识别的容器扩展名或名称。
    std::vector<std::string> containers;
    /// 当前可识别的视频解码器名称。
    std::vector<std::string> video_codecs;
    /// 当前可识别的音频解码器名称。
    std::vector<std::string> audio_codecs;
};

/// 用于评估播放压力的目标媒体参数。
struct PlaybackCapabilityTarget {
    int width{0};
    int height{0};
    double fps{0.0};
    int audio_channels{2};
    uint64_t video_bitrate{0};
};

/// 播放能力评估结果。
struct PlaybackCapabilityDecision {
    /// 是否认为该目标适合实时播放。
    bool suitable_for_realtime{true};
    /// 是否建议优先启用硬解。
    bool recommends_hardware_decode{false};
    /// 是否建议优先使用 D3D11 渲染路径。
    bool recommends_d3d11_renderer{false};
    /// 评估原因说明。
    std::string reason;
};

/// 格式能力辅助类；提供容器/编码支持判断与播放压力评估。
class FormatSupport {
public:
    /// 判断给定容器扩展名是否属于已知支持范围。
    static bool isContainerSupported(const std::string& extension);
    /// 判断视频编码名是否大概率可播放。
    static bool isVideoCodecLikelySupported(const std::string& codec_name);
    /// 判断音频编码名是否大概率可播放。
    static bool isAudioCodecLikelySupported(const std::string& codec_name);
    /// 返回内置支持容器列表。
    static std::vector<std::string> supportedContainers();
    /// 返回内置支持视频编码列表。
    static std::vector<std::string> supportedVideoCodecs();
    /// 返回内置支持音频编码列表。
    static std::vector<std::string> supportedAudioCodecs();
    /// 返回 MKV 章节能力是否被视为支持。
    static bool supportsMkvChapters();
    /// 返回 MP4 `moov` 预加载能力是否被视为支持。
    static bool supportsMp4MoovPreload();

    /// 查询 FFmpeg 运行时解复用与解码能力。
    static FormatCapabilityReport queryRuntimeCapabilities();
    /// 根据目标媒体参数评估实时播放压力与建议配置。
    static PlaybackCapabilityDecision evaluatePlaybackTarget(const PlaybackCapabilityTarget& target);
};

}  // namespace vp::media