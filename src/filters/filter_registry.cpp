#include "filters/filter_registry.h"

namespace vp::filters {

/// 返回进程内全局滤镜注册表单例。
FilterRegistry& FilterRegistry::instance() {
    static FilterRegistry registry;
    return registry;
}

void FilterRegistry::registerVideoFilter(const std::string& name, VideoFilterFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    video_factories_[name] = std::move(factory);
}

void FilterRegistry::registerAudioFilter(const std::string& name, AudioFilterFactory factory) {
    std::lock_guard<std::mutex> lock(mutex_);
    audio_factories_[name] = std::move(factory);
}

bool FilterRegistry::unregisterVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return video_factories_.erase(name) > 0;
}

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

std::vector<std::string> FilterRegistry::getVideoFilterNames() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> names;
    names.reserve(video_factories_.size());
    for (const auto& item : video_factories_) {
        names.push_back(item.first);
    }
    return names;
}

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

