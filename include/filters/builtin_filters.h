#pragma once

#include "filters/audio_filter.h"
#include "filters/video_filter.h"

namespace vp::filters::builtin {

class BrightnessFilter : public IVideoFilter {
public:
    std::string getName() const override { return "brightness"; }
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override { return {FilterPin{"in", "yuv420p"}}; }
    std::vector<FilterPin> getOutputPins() const override { return {FilterPin{"out", "yuv420p"}}; }
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
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override { return {FilterPin{"in", "yuv420p"}}; }
    std::vector<FilterPin> getOutputPins() const override { return {FilterPin{"out", "yuv420p"}}; }
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
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override { return {FilterPin{"in", "yuv420p"}}; }
    std::vector<FilterPin> getOutputPins() const override { return {FilterPin{"out", "yuv420p"}}; }
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

class VolumeBalanceFilter : public IAudioFilter {
public:
    std::string getName() const override { return "volume_balance"; }
    FilterType getType() const override { return FilterType::Audio; }
    std::vector<FilterPin> getInputPins() const override { return {FilterPin{"in", "s16"}}; }
    std::vector<FilterPin> getOutputPins() const override { return {FilterPin{"out", "s16"}}; }
    void process(uint8_t* samples, size_t sample_count, int channels) override;
    void setParameter(const std::string& name, double value) override;
    double getParameter(const std::string& name) const override;
    std::vector<std::string> getParameterNames() const override;
    void enable(bool enabled) override { enabled_ = enabled; }
    bool isEnabled() const override { return enabled_; }

private:
    double volume_{1.0};
    double balance_{0.0};
    bool enabled_{true};
};

void registerBuiltinFilters();

}  // namespace vp::filters::builtin
