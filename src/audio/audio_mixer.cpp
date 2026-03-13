#include "audio/audio_mixer.h"

#include <algorithm>
#include <limits>

namespace vp::audio {

/// 对多路 PCM 输入做逐样本叠加，并对结果执行饱和裁剪。
std::vector<int16_t> AudioMixer::mix(const std::vector<InputBuffer>& inputs, size_t output_samples) {
    std::vector<int16_t> output(output_samples, 0);
    if (inputs.empty() || output_samples == 0) {
        return output;
    }

    const float min_sample = static_cast<float>(std::numeric_limits<int16_t>::min());
    const float max_sample = static_cast<float>(std::numeric_limits<int16_t>::max());

    for (size_t i = 0; i < output_samples; ++i) {
        float accumulator = 0.0f;
        for (const InputBuffer& input : inputs) {
            if (!input.samples || input.sample_count == 0 || i >= input.sample_count) {
                continue;
            }
            const float gain = std::max(0.0f, std::min(4.0f, input.gain));
            accumulator += static_cast<float>(input.samples[i]) * gain;
        }
        accumulator = std::max(min_sample, std::min(max_sample, accumulator));
        output[i] = static_cast<int16_t>(accumulator);
    }

    return output;
}

}  // namespace vp::audio

