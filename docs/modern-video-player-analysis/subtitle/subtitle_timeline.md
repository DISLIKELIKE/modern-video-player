# subtitle_timeline 字幕时间线

源码: `include/subtitle/subtitle_timeline.h`, `src/subtitle/subtitle_timeline.cpp`

## 角色

字幕活跃区间查询工具。根据当前播放时间查找活跃字幕索引，供 `PlayerCore` 更新 overlay 使用。

## 接口

| 接口 | 用途 |
|---|---|
| `isWithinSubtitleRange(item, position_seconds)` | 判断时间是否在字幕条目区间内 |
| `resolveActiveSubtitleIndex(items, position_seconds, active_index_hint)` | 根据当前位置和上一索引查找当前活跃字幕 |
| `collectActiveSubtitleIndices(items, position_seconds)` | 收集同一时间点全部活跃字幕 |

## 数据流

```mermaid
flowchart LR
    CLOCK[播放时间] --> TL[subtitle_timeline]
    ITEMS[SubtitleItem[]] --> TL
    TL --> ACTIVE[活跃字幕索引]
    ACTIVE --> OVERLAY[渲染器 overlay]
```

## 关键约束

- 字幕时间以秒为单位，与 `PlayerCore` 的播放时钟一致。
- `active_index_hint` 用于减少连续播放时的重复扫描成本。

## 注意点

- ASS 同屏多行、bitmap 多矩形等场景应使用 `collectActiveSubtitleIndices` 保留多条活跃字幕。
- seek 后需要重新按当前时间计算活跃字幕，不能信任旧 hint。
