#include "filters/filter_pipeline.h"

#include <algorithm>
#include <exception>

#include "logger.h"

namespace vp::filters {

void FilterPipeline::addVideoFilter(std::unique_ptr<IVideoFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(video_mutex_);
    video_filters_.push_back(std::move(filter));
}

void FilterPipeline::addAudioFilter(std::unique_ptr<IAudioFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_filters_.push_back(std::move(filter));
}

void FilterPipeline::removeVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(video_mutex_);
    video_filters_.erase(
        std::remove_if(video_filters_.begin(), video_filters_.end(),
                       [&name](const auto& filter) { return filter->getName() == name; }),
        video_filters_.end());
}

void FilterPipeline::removeAudioFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_filters_.erase(
        std::remove_if(audio_filters_.begin(), audio_filters_.end(),
                       [&name](const auto& filter) { return filter->getName() == name; }),
        audio_filters_.end());
}

/// 执行视频滤镜管线；异常会被记录但不会向上传播。
void FilterPipeline::processVideo(core::VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(video_mutex_);
    for (const auto& filter : video_filters_) {
        if (!filter || !filter->isEnabled()) {
            continue;
        }
        try {
            filter->process(frame);
        } catch (const std::exception& ex) {
            LOG_ERROR("Video filter failure: " << ex.what());
        } catch (...) {
            LOG_ERROR("Video filter failure: unknown error");
        }
    }
}

/// 执行音频滤镜管线；异常会被记录但不会向上传播。
void FilterPipeline::processAudio(uint8_t* samples, size_t sample_count, int channels) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    for (const auto& filter : audio_filters_) {
        if (!filter || !filter->isEnabled()) {
            continue;
        }
        try {
            filter->process(samples, sample_count, channels);
        } catch (const std::exception& ex) {
            LOG_ERROR("Audio filter failure: " << ex.what());
        } catch (...) {
            LOG_ERROR("Audio filter failure: unknown error");
        }
    }
}

IVideoFilter* FilterPipeline::getVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(video_mutex_);
    for (const auto& filter : video_filters_) {
        if (filter->getName() == name) {
            return filter.get();
        }
    }
    return nullptr;
}

IAudioFilter* FilterPipeline::getAudioFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    for (const auto& filter : audio_filters_) {
        if (filter->getName() == name) {
            return filter.get();
        }
    }
    return nullptr;
}

}  // namespace vp::filters

