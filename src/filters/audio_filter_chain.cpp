#include "filters/audio_filter_chain.h"

#include <algorithm>

#include "logger.h"

namespace vp::filters {

/// 将音频滤镜追加到链尾；执行顺序与插入顺序一致。
void AudioFilterChain::addFilter(std::unique_ptr<IAudioFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.push_back(std::move(filter));
}

/// 删除全部同名音频滤镜，便于运行时替换滤镜配置。
void AudioFilterChain::removeFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.erase(std::remove_if(filters_.begin(), filters_.end(),
                                  [&name](const std::unique_ptr<IAudioFilter>& filter) {
                                      return filter && filter->getName() == name;
                                  }),
                   filters_.end());
}

/// 顺序执行全部已启用音频滤镜；单个滤镜失败不会中断整条链。
void AudioFilterChain::process(uint8_t* samples, size_t sample_count, int channels) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::unique_ptr<IAudioFilter>& filter : filters_) {
        if (!filter || !filter->isEnabled()) {
            continue;
        }
        try {
            filter->process(samples, sample_count, channels);
        } catch (...) {
            LOG_ERROR("AudioFilterChain: filter execution failed");
        }
    }
}

}  // namespace vp::filters

