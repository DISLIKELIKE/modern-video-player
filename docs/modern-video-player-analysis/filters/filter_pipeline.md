# FilterPipeline 滤镜流水线

源码: `include/filters/filter_pipeline.h`, `src/filters/filter_pipeline.cpp`

## 角色

视频和音频滤镜流水线。播放器核心通过它统一管理启用的 `IVideoFilter` 和 `IAudioFilter`，在渲染前或音频输出前处理数据。

## 接口

| 接口 | 用途 |
|---|---|
| `addVideoFilter` / `removeVideoFilter` | 增删视频滤镜 |
| `addAudioFilter` / `removeAudioFilter` | 增删音频滤镜 |
| `processVideo(frame)` | 处理视频帧 |
| `processAudio(samples, sample_count, channels)` | 处理音频样本 |
| `hasEnabledVideoFilters()` | 判断是否存在启用的视频滤镜 |
| `getVideoFilter` / `getAudioFilter` | 按名称查询滤镜 |

## 数据流

```mermaid
flowchart LR
    FRAME[VideoFrame] --> VP[processVideo]
    VP --> VF[IVideoFilter[]]
    VF --> RENDER[Renderer]
    PCM[PCM samples] --> AP[processAudio]
    AP --> AF[IAudioFilter[]]
    AF --> AUDIO[AudioPlayer]
```

## 关键约束

- 视频和音频滤镜分别用独立 mutex 保护。
- `PlayerCore` 当前只对非硬件帧执行视频滤镜处理。

## 注意点

- 滤镜名称是删除和查询的关键，不应重复。
- 新增滤镜时应同步 `FilterRegistry` 和 `registerBuiltinFilters()`。
