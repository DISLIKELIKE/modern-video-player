#include "filters/filter_pipeline.h"

#include <algorithm>
#include <exception>

#include "logger.h"

namespace vp::filters {

/// 灏嗚棰戞护闀滆拷鍔犲埌绠＄嚎灏鹃儴锛涘悗缁鐞嗘寜鍔犲叆椤哄簭鎵ц銆?
void FilterPipeline::addVideoFilter(std::unique_ptr<IVideoFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(video_mutex_);
    video_filters_.push_back(std::move(filter));
}

/// 灏嗛煶棰戞护闀滆拷鍔犲埌绠＄嚎灏鹃儴锛涘悗缁鐞嗘寜鍔犲叆椤哄簭鎵ц銆?
void FilterPipeline::addAudioFilter(std::unique_ptr<IAudioFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_filters_.push_back(std::move(filter));
}

/// 鎸夊悕绉扮Щ闄ゅ叏閮ㄥ悓鍚嶈棰戞护闀滃疄渚嬨€?
void FilterPipeline::removeVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(video_mutex_);
    video_filters_.erase(
        std::remove_if(video_filters_.begin(), video_filters_.end(),
                       [&name](const auto& filter) { return filter->getName() == name; }),
        video_filters_.end());
}

/// 鎸夊悕绉扮Щ闄ゅ叏閮ㄥ悓鍚嶉煶棰戞护闀滃疄渚嬨€?
void FilterPipeline::removeAudioFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(audio_mutex_);
    audio_filters_.erase(
        std::remove_if(audio_filters_.begin(), audio_filters_.end(),
                       [&name](const auto& filter) { return filter->getName() == name; }),
        audio_filters_.end());
}

/// 鎵ц瑙嗛婊ら暅绠＄嚎锛涘紓甯镐細琚褰曚絾涓嶄細鍚戜笂浼犳挱銆?
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

bool FilterPipeline::hasEnabledVideoFilters() const {
    std::lock_guard<std::mutex> lock(video_mutex_);
    for (const auto& filter : video_filters_) {
        if (filter && filter->isEnabled()) {
            return true;
        }
    }
    return false;
}
/// 鎵ц闊抽婊ら暅绠＄嚎锛涘紓甯镐細琚褰曚絾涓嶄細鍚戜笂浼犳挱銆?
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

/// 鎸夊悕绉版煡鎵惧凡鎸傝浇鐨勮棰戞护闀滐紝渚夸簬杩愯鏃惰皟鍙傘€?
IVideoFilter* FilterPipeline::getVideoFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(video_mutex_);
    for (const auto& filter : video_filters_) {
        if (filter->getName() == name) {
            return filter.get();
        }
    }
    return nullptr;
}

/// 鎸夊悕绉版煡鎵惧凡鎸傝浇鐨勯煶棰戞护闀滐紝渚夸簬杩愯鏃惰皟鍙傘€?
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


