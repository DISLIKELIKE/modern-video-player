#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/audio_filter.h"
#include "filters/video_filter.h"

namespace vp::filters {

class FilterPipeline {
public:
    void addVideoFilter(std::unique_ptr<IVideoFilter> filter);
    void addAudioFilter(std::unique_ptr<IAudioFilter> filter);
    void removeVideoFilter(const std::string& name);
    void removeAudioFilter(const std::string& name);

    void processVideo(core::VideoFrame& frame);
    bool hasEnabledVideoFilters() const;
    void processAudio(uint8_t* samples, size_t sample_count, int channels);

    IVideoFilter* getVideoFilter(const std::string& name);
    IAudioFilter* getAudioFilter(const std::string& name);

private:
    std::vector<std::unique_ptr<IVideoFilter>> video_filters_;
    std::vector<std::unique_ptr<IAudioFilter>> audio_filters_;
    mutable std::mutex video_mutex_;
    mutable std::mutex audio_mutex_;
};

}  // namespace vp::filters
