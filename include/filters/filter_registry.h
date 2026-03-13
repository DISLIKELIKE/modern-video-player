#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "filters/audio_filter.h"
#include "filters/video_filter.h"

namespace vp::filters {

/// 视频滤镜工厂函数类型。
using VideoFilterFactory = std::function<std::unique_ptr<IVideoFilter>()>;
/// 音频滤镜工厂函数类型。
using AudioFilterFactory = std::function<std::unique_ptr<IAudioFilter>()>;

/// 滤镜注册表；负责按名称注册、注销和实例化滤镜。
class FilterRegistry {
public:
    /// 返回全局单例注册表。
    static FilterRegistry& instance();

    /// 注册视频滤镜工厂。
    void registerVideoFilter(const std::string& name, VideoFilterFactory factory);
    /// 注册音频滤镜工厂。
    void registerAudioFilter(const std::string& name, AudioFilterFactory factory);
    /// 注销视频滤镜工厂。
    bool unregisterVideoFilter(const std::string& name);
    /// 注销音频滤镜工厂。
    bool unregisterAudioFilter(const std::string& name);

    /// 创建指定名称的视频滤镜实例。
    std::unique_ptr<IVideoFilter> createVideoFilter(const std::string& name);
    /// 创建指定名称的音频滤镜实例。
    std::unique_ptr<IAudioFilter> createAudioFilter(const std::string& name);

    /// 返回已注册视频滤镜名称列表。
    std::vector<std::string> getVideoFilterNames() const;
    /// 返回已注册音频滤镜名称列表。
    std::vector<std::string> getAudioFilterNames() const;

private:
    std::unordered_map<std::string, VideoFilterFactory> video_factories_;
    std::unordered_map<std::string, AudioFilterFactory> audio_factories_;
    mutable std::mutex mutex_;
};

}  // namespace vp::filters