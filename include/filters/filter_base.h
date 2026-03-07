#pragma once

#include <string>
#include <vector>

namespace vp::filters {

enum class FilterType {
    Video,
    Audio
};

struct FilterPin {
    std::string name;
    std::string media_format;
};

class IFilter {
public:
    virtual ~IFilter() = default;

    virtual std::string getName() const = 0;
    virtual FilterType getType() const = 0;
    virtual void enable(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
    virtual std::vector<FilterPin> getInputPins() const = 0;
    virtual std::vector<FilterPin> getOutputPins() const = 0;
};

}  // namespace vp::filters

