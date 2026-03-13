#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "filters/filter_base.h"

namespace vp::filters {

/// 音频滤镜抽象接口；对原始 PCM 数据做原地处理。
class IAudioFilter : public IFilter {
public:
    virtual ~IAudioFilter() = default;
    /// 处理音频样本缓冲。
    virtual void process(uint8_t* samples, size_t sample_count, int channels) = 0;
    /// 设置命名参数值。
    virtual void setParameter(const std::string& name, double value) = 0;
    /// 读取命名参数值。
    virtual double getParameter(const std::string& name) const = 0;
    /// 返回支持的参数名列表。
    virtual std::vector<std::string> getParameterNames() const = 0;
};

}  // namespace vp::filters