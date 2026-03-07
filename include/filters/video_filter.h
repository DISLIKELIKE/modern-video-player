#pragma once

#include <string>
#include <vector>

#include "core/frame.h"
#include "filters/filter_base.h"

namespace vp::filters {

class IVideoFilter : public IFilter {
public:
    virtual ~IVideoFilter() = default;
    virtual void process(core::VideoFrame& frame) = 0;
    virtual void setParameter(const std::string& name, double value) = 0;
    virtual double getParameter(const std::string& name) const = 0;
    virtual std::vector<std::string> getParameterNames() const = 0;
};

}  // namespace vp::filters

