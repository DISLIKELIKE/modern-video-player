#pragma once

#include <string>
#include <vector>

namespace vp::filters {

/// 滤镜媒体类型。
enum class FilterType {
    Video,
    Audio
};

/// 滤镜输入/输出引脚描述。
struct FilterPin {
    std::string name;
    std::string media_format;
};

/// 滤镜抽象基类；定义名称、启用状态与引脚元信息。
class IFilter {
public:
    virtual ~IFilter() = default;

    /// 返回滤镜名称。
    virtual std::string getName() const = 0;
    /// 返回滤镜处理的媒体类型。
    virtual FilterType getType() const = 0;
    /// 启用或禁用滤镜。
    virtual void enable(bool enabled) = 0;
    /// 返回滤镜是否启用。
    virtual bool isEnabled() const = 0;
    /// 返回输入引脚描述列表。
    virtual std::vector<FilterPin> getInputPins() const = 0;
    /// 返回输出引脚描述列表。
    virtual std::vector<FilterPin> getOutputPins() const = 0;
};

}  // namespace vp::filters