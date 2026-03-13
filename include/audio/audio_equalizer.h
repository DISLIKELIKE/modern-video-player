#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace vp::audio {

/// 轻量级均衡器参数封装；对外提供 10 段增益和总增益控制。
class AudioEqualizer {
public:
    /// 支持的频段数。
    static constexpr size_t kBandCount = 10;

    /// 创建默认均衡器；所有频段初始为 0dB。
    AudioEqualizer();

    /// 设置指定频段增益，单位 dB；索引越界时忽略。
    void setBandGain(size_t index, float gain_db);
    /// 返回指定频段增益，索引越界时返回 0dB。
    float getBandGain(size_t index) const;

    /// 设置总增益倍数。
    void setMasterGain(float gain);
    /// 返回当前总增益倍数。
    float getMasterGain() const;

    /// 原地处理 PCM 采样数据。
    /// @note 当前实现对全部样本应用统一缩放，不做真实频域分离。
    void apply(int16_t* samples, size_t sample_count, int channels) const;

private:
    std::array<float, kBandCount> band_gains_db_{};
    float master_gain_{1.0f};
};

}  // namespace vp::audio