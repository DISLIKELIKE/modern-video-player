#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/video_filter.h"

namespace vp::filters {

class VideoFilterChain {
public:
    void addFilter(std::unique_ptr<IVideoFilter> filter);
    void removeFilter(const std::string& name);
    void process(core::VideoFrame& frame);

private:
    std::vector<std::unique_ptr<IVideoFilter>> filters_;
    std::mutex mutex_;
};

}  // namespace vp::filters

