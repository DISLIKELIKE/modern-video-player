#include "filters/builtin_filters.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>

namespace vp::filters::builtin {

namespace {

int16_t clampSample(int sample) {
    return static_cast<int16_t>(std::max(-32768, std::min(32767, sample)));
}

}  // namespace

/// 对 PCM 声道应用音量和左右平衡系数。
void VolumeBalanceFilter::process(uint8_t* samples, size_t sample_count, int channels) {
    if (!enabled_ || !samples || sample_count == 0 || channels <= 0) {
        return;
    }

    int16_t* pcm = reinterpret_cast<int16_t*>(samples);
    const size_t frame_count = sample_count / static_cast<size_t>(channels);
    const double left_gain = volume_ * (balance_ <= 0.0 ? 1.0 : 1.0 - balance_);
    const double right_gain = volume_ * (balance_ >= 0.0 ? 1.0 : 1.0 + balance_);

    for (size_t i = 0; i < frame_count; ++i) {
        if (channels == 1) {
            const int scaled = static_cast<int>(std::lround(static_cast<double>(pcm[i]) * volume_));
            pcm[i] = clampSample(scaled);
            continue;
        }

        int16_t* frame = pcm + i * static_cast<size_t>(channels);
        frame[0] = clampSample(static_cast<int>(std::lround(static_cast<double>(frame[0]) * left_gain)));
        frame[1] = clampSample(static_cast<int>(std::lround(static_cast<double>(frame[1]) * right_gain)));

        for (int ch = 2; ch < channels; ++ch) {
            const int scaled = static_cast<int>(std::lround(static_cast<double>(frame[ch]) * volume_));
            frame[ch] = clampSample(scaled);
        }
    }
}

/// 设置音量或平衡参数；非法参数名会抛异常。
void VolumeBalanceFilter::setParameter(const std::string& name, double value) {
    if (name == "volume") {
        volume_ = std::max(0.0, std::min(4.0, value));
        return;
    }
    if (name == "balance") {
        balance_ = std::max(-1.0, std::min(1.0, value));
        return;
    }
    throw std::invalid_argument("unknown volume/balance parameter");
}

/// 返回指定参数的当前值；未知参数统一回退为 0。
double VolumeBalanceFilter::getParameter(const std::string& name) const {
    if (name == "volume") {
        return volume_;
    }
    if (name == "balance") {
        return balance_;
    }
    return 0.0;
}

/// 返回音量/平衡滤镜支持的参数名集合。
std::vector<std::string> VolumeBalanceFilter::getParameterNames() const {
    return {"volume", "balance"};
}

}  // namespace vp::filters::builtin

