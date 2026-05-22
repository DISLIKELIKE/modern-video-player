# libass_probe libass 探测

源码: `include/subtitle/libass_probe.h`, `src/subtitle/libass_probe.cpp`

## 角色

libass shaping 能力探测工具。用于检查 ASS/SSA 字幕在指定画面尺寸和采样时间范围内的 shaping/render 路径是否可用。

## 接口

| 接口 | 用途 |
|---|---|
| `probeLibassShaping(subtitle_path, frame_width, frame_height, sample_duration_ms, sample_step_ms)` | 探测 libass shaping |
| `isSupportedLibassInputExtension(subtitle_path)` | 判断输入扩展名是否适合 libass 探测 |

## 数据流

```mermaid
flowchart LR
    ASS[ASS/SSA 字幕] --> PROBE[probeLibassShaping]
    PROBE --> TS[采样时间点]
    TS --> LIBASS[libass render]
    LIBASS --> SUMMARY[LibassShapingProbeSummary]
```

## 关键约束

- 只对 libass 支持的字幕扩展名执行探测。
- 探测通过采样时间点驱动，不等同于完整播放。

## 注意点

- 该模块用于诊断和回归检查，不是播放器主渲染链路的唯一字幕实现。
- 修改采样逻辑时需要同步 `--libass-shaping-check` 输出预期。
