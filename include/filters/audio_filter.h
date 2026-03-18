#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "filters/filter_base.h"

namespace vp::filters {

class IAudioFilter : public IFilter {
public:
    virtual ~IAudioFilter() = default;

    virtual void process(uint8_t* samples, size_t sample_count, int channels) = 0;
    virtual void setParameter(const std::string& name, double value) = 0;
    virtual double getParameter(const std::string& name) const = 0;
    virtual std::vector<std::string> getParameterNames() const = 0;
};

}  // namespace vp::filters
