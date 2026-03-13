#include "audio/audio_equalizer.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace vp::audio {

namespace {

float clampDb(float gain_db) {
    return std::max(-24.0f, std::min(24.0f, gain_db));
}

float dbToLinear(float gain_db) {
    return std::pow(10.0f, gain_db / 20.0f);
}

int16_t clampSample(float sample) {
    const float min_sample = static_cast<float>(std::numeric_limits<int16_t>::min());
    const float max_sample = static_cast<float>(std::numeric_limits<int16_t>::max());
    const float clamped = std::max(min_sample, std::min(max_sample, sample));
    return static_cast<int16_t>(clamped);
}

}  // namespace

/// 初始化均衡器参数；默认所有频段为 0dB。
AudioEqualizer::AudioEqualizer() {
    band_gains_db_.fill(0.0f);
}

void AudioEqualizer::setBandGain(size_t index, float gain_db) {
    if (index >= kBandCount) {
        return;
    }
    band_gains_db_[index] = clampDb(gain_db);
}

float AudioEqualizer::getBandGain(size_t index) const {
    if (index >= kBandCount) {
        return 0.0f;
    }
    return band_gains_db_[index];
}

void AudioEqualizer::setMasterGain(float gain) {
    master_gain_ = std::max(0.0f, std::min(4.0f, gain));
}

float AudioEqualizer::getMasterGain() const {
    return master_gain_;
}

/// 原地处理 PCM 缓冲；当前按平均频段增益与总增益统一缩放。
void AudioEqualizer::apply(int16_t* samples, size_t sample_count, int channels) const {
    (void)channels;
    if (!samples || sample_count == 0) {
        return;
    }

    float sum_db = 0.0f;
    for (float band_db : band_gains_db_) {
        sum_db += band_db;
    }
    const float avg_band_gain = dbToLinear(sum_db / static_cast<float>(kBandCount));
    const float total_gain = avg_band_gain * master_gain_;

    for (size_t i = 0; i < sample_count; ++i) {
        const float scaled = static_cast<float>(samples[i]) * total_gain;
        samples[i] = clampSample(scaled);
    }
}

}  // namespace vp::audio

