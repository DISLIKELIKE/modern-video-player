#include "filters/builtin_filters.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace vp::filters::builtin {

namespace {
inline uint8_t clampByte(int value) {
    return static_cast<uint8_t>(std::max(0, std::min(255, value)));
}
}  // namespace

/// 直接修改 Y 平面，整体抬升或压低画面亮度。
void BrightnessFilter::process(core::VideoFrame& frame) {
    if (!enabled_ || !frame.frame || !frame.frame->data[0]) {
        return;
    }
    const int width = frame.frame->width;
    const int height = frame.frame->height;
    const int stride = frame.frame->linesize[0];
    const int delta = static_cast<int>(std::round(brightness_ * 255.0 / 100.0));

    for (int y = 0; y < height; ++y) {
        uint8_t* row = frame.frame->data[0] + y * stride;
        for (int x = 0; x < width; ++x) {
            row[x] = clampByte(static_cast<int>(row[x]) + delta);
        }
    }
}

/// 设置亮度参数；范围限制在 [-100, 100]。
void BrightnessFilter::setParameter(const std::string& name, double value) {
    if (name != "brightness") {
        throw std::invalid_argument("unknown brightness parameter");
    }
    brightness_ = std::max(-100.0, std::min(100.0, value));
}

/// 返回指定参数的当前值；未知参数统一回退为 0。
double BrightnessFilter::getParameter(const std::string& name) const {
    if (name == "brightness") {
        return brightness_;
    }
    return 0.0;
}

/// 返回亮度滤镜支持的参数名集合。
std::vector<std::string> BrightnessFilter::getParameterNames() const {
    return {"brightness"};
}

}  // namespace vp::filters::builtin

