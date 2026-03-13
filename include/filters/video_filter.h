#pragma once

#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/filter_base.h"

namespace vp::filters {

/// 视频滤镜抽象接口；对 `VideoFrame` 做原地处理。
class IVideoFilter : public IFilter {
public:
    virtual ~IVideoFilter() = default;
    /// 处理一帧视频数据。
    virtual void process(core::VideoFrame& frame) = 0;
    /// 设置命名参数值。
    virtual void setParameter(const std::string& name, double value) = 0;
    /// 读取命名参数值。
    virtual double getParameter(const std::string& name) const = 0;
    /// 返回支持的参数名列表。
    virtual std::vector<std::string> getParameterNames() const = 0;
};

}  // namespace vp::filters