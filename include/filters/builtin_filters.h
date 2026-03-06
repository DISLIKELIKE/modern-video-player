#pragma once

#include "filters/video_filter.h"

namespace vp::filters::builtin {

class BrightnessFilter : public IVideoFilter {
public:
    std::string getName() const override { return "brightness"; }
    void process(core::VideoFrame& frame) override;
    void setParameter(const std::string& name, double value) override;
    double getParameter(const std::string& name) const override;
    std::vector<std::string> getParameterNames() const override;
    void enable(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

private:
    double brightness_{0.0};
    bool enabled_{true};
};

class ContrastFilter : public IVideoFilter {
public:
    std::string getName() const override { return "contrast"; }
    void process(core::VideoFrame& frame) override;
    void setParameter(const std::string& name, double value) override;
    double getParameter(const std::string& name) const override;
    std::vector<std::string> getParameterNames() const override;
    void enable(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

private:
    double contrast_{1.0};
    bool enabled_{true};
};

class SaturationFilter : public IVideoFilter {
public:
    std::string getName() const override { return "saturation"; }
    void process(core::VideoFrame& frame) override;
    void setParameter(const std::string& name, double value) override;
    double getParameter(const std::string& name) const override;
    std::vector<std::string> getParameterNames() const override;
    void enable(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

private:
    double saturation_{1.0};
    bool enabled_{true};
};

void registerBuiltinFilters();

}  // namespace vp::filters::builtin

