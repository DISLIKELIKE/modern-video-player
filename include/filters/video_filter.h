#pragma once

#include <string>
#include <vector>

#include "core/frame.h"

namespace vp::filters {

class IVideoFilter {
public:
    virtual ~IVideoFilter() = default;
    virtual std::string getName() const = 0;
    virtual void process(core::VideoFrame& frame) = 0;
    virtual void setParameter(const std::string& name, double value) = 0;
    virtual double getParameter(const std::string& name) const = 0;
    virtual std::vector<std::string> getParameterNames() const = 0;
    virtual void enable(bool enabled) = 0;
    virtual bool isEnabled() const = 0;
};

}  // namespace vp::filters

