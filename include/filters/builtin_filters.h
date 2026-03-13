#pragma once

#include "filters/audio_filter.h"
#include "filters/video_filter.h"

namespace vp::filters::builtin {

/// 亮度滤镜；直接调整 Y 平面亮度。
class BrightnessFilter : public IVideoFilter {
public:
    std::string getName() const override { return "brightness"; }
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override {
        return {FilterPin{"in", "yuv420p"}};
    }
    std::vector<FilterPin> getOutputPins() const override {
        return {FilterPin{"out", "yuv420p"}};
    }
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

/// 对比度滤镜；围绕中性灰度重映射 Y 平面。
class ContrastFilter : public IVideoFilter {
public:
    std::string getName() const override { return "contrast"; }
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override {
        return {FilterPin{"in", "yuv420p"}};
    }
    std::vector<FilterPin> getOutputPins() const override {
        return {FilterPin{"out", "yuv420p"}};
    }
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

/// 饱和度滤镜；调整 U/V 色度平面偏移量。
class SaturationFilter : public IVideoFilter {
public:
    std::string getName() const override { return "saturation"; }
    FilterType getType() const override { return FilterType::Video; }
    std::vector<FilterPin> getInputPins() const override {
        return {FilterPin{"in", "yuv420p"}};
    }
    std::vector<FilterPin> getOutputPins() const override {
        return {FilterPin{"out", "yuv420p"}};
    }
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

/// 音量/声像滤镜；对 PCM 声道执行线性缩放。
class VolumeBalanceFilter : public IAudioFilter {
public:
    std::string getName() const override { return "volume_balance"; }
    FilterType getType() const override { return FilterType::Audio; }
    std::vector<FilterPin> getInputPins() const override {
        return {FilterPin{"in", "s16"}};
    }
    std::vector<FilterPin> getOutputPins() const override {
        return {FilterPin{"out", "s16"}};
    }
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

/// 把内建滤镜注册到全局 `FilterRegistry`。
void registerBuiltinFilters();

}  // namespace vp::filters::builtin