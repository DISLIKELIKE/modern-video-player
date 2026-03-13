#pragma once

#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/video_filter.h"

namespace vp::filters {

/// 视频滤镜链；按顺序执行多个视频滤镜。
class VideoFilterChain {
public:
    /// 追加一个视频滤镜。
    void addFilter(std::unique_ptr<IVideoFilter> filter);
    /// 按名称移除视频滤镜。
    void removeFilter(const std::string& name);
    /// 依次执行已启用的视频滤镜。
    void process(core::VideoFrame& frame);

private:
    std::vector<std::unique_ptr<IVideoFilter>> filters_;
    std::mutex mutex_;
};

}  // namespace vp::filters