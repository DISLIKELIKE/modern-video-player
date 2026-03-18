#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vp::audio {

class AudioMixer {
public:
    struct InputBuffer {
        const int16_t* samples{nullptr};
        size_t sample_count{0};
        float gain{1.0f};
    };

    static std::vector<int16_t> mix(const std::vector<InputBuffer>& inputs, size_t output_samples);
};

}  // namespace vp::audio
