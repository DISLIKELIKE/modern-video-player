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

/// 调整 U/V 色度平面，改变画面饱和度。
void SaturationFilter::process(core::VideoFrame& frame) {
    if (!enabled_ || !frame.frame || !frame.frame->data[1] || !frame.frame->data[2]) {
        return;
    }
    const int chroma_width = frame.frame->width / 2;
    const int chroma_height = frame.frame->height / 2;
    const int u_stride = frame.frame->linesize[1];
    const int v_stride = frame.frame->linesize[2];

    for (int y = 0; y < chroma_height; ++y) {
        uint8_t* u = frame.frame->data[1] + y * u_stride;
        uint8_t* v = frame.frame->data[2] + y * v_stride;
        for (int x = 0; x < chroma_width; ++x) {
            const int u_centered = static_cast<int>(u[x]) - 128;
            const int v_centered = static_cast<int>(v[x]) - 128;
            u[x] = clampByte(static_cast<int>(std::round(u_centered * saturation_ + 128.0)));
            v[x] = clampByte(static_cast<int>(std::round(v_centered * saturation_ + 128.0)));
        }
    }
}

/// 设置饱和度参数；范围限制在 [0.0, 2.0]。
void SaturationFilter::setParameter(const std::string& name, double value) {
    if (name != "saturation") {
        throw std::invalid_argument("unknown saturation parameter");
    }
    saturation_ = std::max(0.0, std::min(2.0, value));
}

double SaturationFilter::getParameter(const std::string& name) const {
    if (name == "saturation") {
        return saturation_;
    }
    return 0.0;
}

std::vector<std::string> SaturationFilter::getParameterNames() const {
    return {"saturation"};
}

}  // namespace vp::filters::builtin

