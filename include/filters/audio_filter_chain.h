#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "filters/audio_filter.h"

namespace vp::filters {

/// 音频滤镜链；按顺序执行多个音频滤镜。
class AudioFilterChain {
public:
    /// 追加一个音频滤镜。
    void addFilter(std::unique_ptr<IAudioFilter> filter);
    /// 按名称移除音频滤镜。
    void removeFilter(const std::string& name);
    /// 依次执行已启用的音频滤镜。
    void process(uint8_t* samples, size_t sample_count, int channels);

private:
    std::vector<std::unique_ptr<IAudioFilter>> filters_;
    std::mutex mutex_;
};

}  // namespace vp::filters