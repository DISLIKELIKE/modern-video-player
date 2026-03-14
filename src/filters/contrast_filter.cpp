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

/// 围绕中性灰度调整 Y 平面对比度。
void ContrastFilter::process(core::VideoFrame& frame) {
    if (!enabled_ || !frame.frame || !frame.frame->data[0]) {
        return;
    }
    const int width = frame.frame->width;
    const int height = frame.frame->height;
    const int stride = frame.frame->linesize[0];

    for (int y = 0; y < height; ++y) {
        uint8_t* row = frame.frame->data[0] + y * stride;
        for (int x = 0; x < width; ++x) {
            const int centered = static_cast<int>(row[x]) - 128;
            row[x] = clampByte(static_cast<int>(std::round(centered * contrast_ + 128.0)));
        }
    }
}

/// 设置对比度参数；范围限制在 [0.5, 2.0]。
void ContrastFilter::setParameter(const std::string& name, double value) {
    if (name != "contrast") {
        throw std::invalid_argument("unknown contrast parameter");
    }
    contrast_ = std::max(0.5, std::min(2.0, value));
}

/// 返回指定参数的当前值；未知参数统一回退为 0。
double ContrastFilter::getParameter(const std::string& name) const {
    if (name == "contrast") {
        return contrast_;
    }
    return 0.0;
}

/// 返回对比度滤镜支持的参数名集合。
std::vector<std::string> ContrastFilter::getParameterNames() const {
    return {"contrast"};
}

}  // namespace vp::filters::builtin

