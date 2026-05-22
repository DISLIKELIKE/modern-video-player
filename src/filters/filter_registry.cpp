#include "filters/filter_registry.h"

namespace vp::filters {

/// 返回进程内全局滤镜注册表单例。
FilterRegistry& FilterRegistry::instance() {
    static FilterRegistry registry;
    return registry;
}

/// 注册视频滤镜工厂；同名注册会被新的工厂实现覆盖。
void FilterRegistry::registerVideoFilter(const std::string& name, VideoFilterFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    video_factories_[name] = std::move(factory);
}

/// 注册音频滤镜工厂；同名注册会被新的工厂实现覆盖。
void FilterRegistry::registerAudioFilter(const std::string& name, AudioFilterFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_factories_[name] = std::move(factory);
}

/// 注销视频滤镜工厂；返回是否真正删除了注册项。
bool FilterRegistry::unregisterVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return video_factories_.erase(name) > 0;
}

/// 注销音频滤镜工厂；返回是否真正删除了注册项。
bool FilterRegistry::unregisterAudioFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return audio_factories_.erase(name) > 0;
}

/// 按名称创建视频滤镜实例；未注册时返回空。
std::unique_ptr<IVideoFilter> FilterRegistry::createVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = video_factories_.find(name);
    if (it == video_factories_.end()) {
        return nullptr;
    }
    return it->second();
}

/// 按名称创建音频滤镜实例；未注册时返回空。
std::unique_ptr<IAudioFilter> FilterRegistry::createAudioFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    const auto it = audio_factories_.find(name);
    if (it == audio_factories_.end()) {
        return nullptr;
    }
    return it->second();
}

/// 返回当前已注册的视频滤镜名称快照。
std::vector<std::string> FilterRegistry::getVideoFilterNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(video_factories_.size());
    for (const auto& item : video_factories_) {
        names.push_back(item.first);
    }
    return names;
}

/// 返回当前已注册的音频滤镜名称快照。
std::vector<std::string> FilterRegistry::getAudioFilterNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(audio_factories_.size());
    for (const auto& item : audio_factories_) {
        names.push_back(item.first);
    }
    return names;
}

}  // namespace vp::filters

