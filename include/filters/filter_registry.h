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

using VideoFilterFactory = std::function<std::unique_ptr<IVideoFilter>()>;
using AudioFilterFactory = std::function<std::unique_ptr<IAudioFilter>()>;

class FilterRegistry {
public:
    static FilterRegistry& instance();

    void registerVideoFilter(const std::string& name, VideoFilterFactory factory);
    void registerAudioFilter(const std::string& name, AudioFilterFactory factory);

    std::unique_ptr<IVideoFilter> createVideoFilter(const std::string& name);
    std::unique_ptr<IAudioFilter> createAudioFilter(const std::string& name);

    std::vector<std::string> getVideoFilterNames() const;
    std::vector<std::string> getAudioFilterNames() const;

private:
    std::unordered_map<std::string, VideoFilterFactory> video_factories_;
    std::unordered_map<std::string, AudioFilterFactory> audio_factories_;
    mutable std::mutex mutex_;
};

}  // namespace vp::filters

