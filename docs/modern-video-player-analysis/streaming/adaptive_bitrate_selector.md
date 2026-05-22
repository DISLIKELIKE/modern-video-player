# AdaptiveBitrateSelector 自适应码率选择

源码: `include/streaming/adaptive_bitrate_selector.h`, `src/streaming/adaptive_bitrate_selector.cpp`

## 角色

根据估计带宽和 headroom 选择 HLS/DASH variant。它不下载媒体，只输出当前应选择的 variant 索引和原因。

## 接口

| 接口 | 用途 |
|---|---|
| `chooseVariant(variants, estimated_bandwidth_bps, headroom_ratio)` | 选择码率 variant |

## 数据

| 数据 | 说明 |
|---|---|
| `AdaptiveStreamVariant` | 码率、分辨率、URI |
| `AdaptiveBitrateDecision` | 是否选中、选中索引、原因 |

## 数据流

```mermaid
flowchart LR
    HLS[HLS variants] --> NORM[AdaptiveStreamVariant[]]
    DASH[DASH representations] --> NORM
    BW[estimated bandwidth] --> SELECT[chooseVariant]
    NORM --> SELECT
    SELECT --> DECISION[AdaptiveBitrateDecision]
```

## 关键约束

- `headroom_ratio` 默认 0.85，用于避免选择过于贴近带宽上限的码率。
- 没有可用 variant 时返回未选中决策。

## 注意点

- 该模块是策略层，不负责真实 buffer 或网络吞吐统计。
- 修改策略会影响 `--adaptive-bitrate-check` 输出。
