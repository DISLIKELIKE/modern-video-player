# audio_tools 音频工具

源码: `include/audio/audio_equalizer.h`, `include/audio/audio_mixer.h`, `src/audio/*.cpp`

## 角色

音频处理辅助模块。当前包含 10 段均衡器和多输入 PCM 混音器，可作为后续音频滤镜或播放增强能力的基础。

## 组件

| 组件 | 用途 |
|---|---|
| `AudioEqualizer` | 10 段增益和 master gain，处理 `int16_t` PCM |
| `AudioMixer` | 将多个 `InputBuffer` 混合到一个输出 buffer |

## 接口

| 接口 | 用途 |
|---|---|
| `AudioEqualizer::setBandGain` / `getBandGain` | 设置/查询单频段增益 |
| `AudioEqualizer::setMasterGain` / `getMasterGain` | 设置/查询总增益 |
| `AudioEqualizer::apply(samples, sample_count, channels)` | 应用均衡处理 |
| `AudioMixer::mix(inputs, output_samples)` | 混合多个输入缓冲 |

## 数据流

```mermaid
flowchart LR
    PCM[PCM int16] --> EQ[AudioEqualizer]
    INPUTS[InputBuffer[]] --> MIX[AudioMixer]
    EQ --> OUT[processed PCM]
    MIX --> OUT
```

## 关键约束

- `AudioEqualizer::kBandCount` 固定为 10。
- `AudioMixer::InputBuffer` 持有外部 sample 指针，调用方负责保证输入生命周期覆盖 mix 调用。

## 注意点

- 当前主播放音频输出由 `AudioPlayer` 负责，这些工具属于辅助处理能力。
- 接入实时播放链路时需要明确采样格式、声道数和溢出裁剪策略。
