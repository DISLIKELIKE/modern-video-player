#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace vp::audio {

class AudioEqualizer {
public:
    static constexpr size_t kBandCount = 10;

    AudioEqualizer();

    void setBandGain(size_t index, float gain_db);
    float getBandGain(size_t index) const;

    void setMasterGain(float gain);
    float getMasterGain() const;

    void apply(int16_t* samples, size_t sample_count, int channels) const;

private:
    std::array<float, kBandCount> band_gains_db_{};
    float master_gain_{1.0f};
};

}  // namespace vp::audio
