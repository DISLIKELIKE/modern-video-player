#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "filters/audio_filter.h"

namespace vp::filters {

class AudioFilterChain {
public:
    void addFilter(std::unique_ptr<IAudioFilter> filter);
    void removeFilter(const std::string& name);
    void process(uint8_t* samples, size_t sample_count, int channels);

private:
    std::vector<std::unique_ptr<IAudioFilter>> filters_;
    std::mutex mutex_;
};

}  // namespace vp::filters

