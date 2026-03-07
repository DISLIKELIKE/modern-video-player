#include "filters/audio_filter_chain.h"

#include <algorithm>

#include "logger.h"

namespace vp::filters {

void AudioFilterChain::addFilter(std::unique_ptr<IAudioFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.push_back(std::move(filter));
}

void AudioFilterChain::removeFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.erase(std::remove_if(filters_.begin(), filters_.end(),
                                  [&name](const std::unique_ptr<IAudioFilter>& filter) {
                                      return filter && filter->getName() == name;
                                  }),
                   filters_.end());
}

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

