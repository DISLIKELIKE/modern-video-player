#include "filters/video_filter_chain.h"

#include <algorithm>

#include "logger.h"

namespace vp::filters {

void VideoFilterChain::addFilter(std::unique_ptr<IVideoFilter> filter) {
    if (!filter) {
        return;
    }
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.push_back(std::move(filter));
}

void VideoFilterChain::removeFilter(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    filters_.erase(std::remove_if(filters_.begin(), filters_.end(),
                                  [&name](const std::unique_ptr<IVideoFilter>& filter) {
                                      return filter && filter->getName() == name;
                                  }),
                   filters_.end());
}

/// 顺序执行全部已启用视频滤镜；单个滤镜失败不会中断整条链。
void VideoFilterChain::process(core::VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const std::unique_ptr<IVideoFilter>& filter : filters_) {
        if (!filter || !filter->isEnabled()) {
            continue;
        }
        try {
            filter->process(frame);
        } catch (...) {
            LOG_ERROR("VideoFilterChain: filter execution failed");
        }
    }
}

}  // namespace vp::filters

