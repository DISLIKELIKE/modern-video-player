#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vp::audio {

/// 多路音频混音器；按样本位置叠加多路 PCM 输入。
class AudioMixer {
public:
    /// 单路输入缓冲描述。
    struct InputBuffer {
        /// 输入样本首地址；不转移所有权。
        const int16_t* samples{nullptr};
        /// 可读样本数。
        size_t sample_count{0};
        /// 该路输入的线性增益。
        float gain{1.0f};
    };

    /// 将多路输入混合为固定长度输出；不足部分按静音处理。
    static std::vector<int16_t> mix(const std::vector<InputBuffer>& inputs, size_t output_samples);
};

}  // namespace vp::audio