# builtin_filters 内置滤镜

源码: `include/filters/builtin_filters.h`, `src/filters/*.cpp`

## 角色

内置音视频滤镜集合。提供亮度、对比度、饱和度和音量平衡滤镜，并通过 `registerBuiltinFilters()` 注册到 `FilterRegistry`。

## 组件

| 滤镜 | 类型 | 说明 |
|---|---|---|
| `BrightnessFilter` | `IVideoFilter` | 视频亮度调整 |
| `ContrastFilter` | `IVideoFilter` | 视频对比度调整 |
| `SaturationFilter` | `IVideoFilter` | 视频饱和度调整 |
| `VolumeBalanceFilter` | `IAudioFilter` | 音频音量/声道平衡 |

## 公共接口

| 接口 | 用途 |
|---|---|
| `getName()` | 返回滤镜名称 |
| `getType()` | 返回音频或视频类型 |
| `enable(bool)` / `isEnabled()` | 启用状态 |
| `getInputPins()` / `getOutputPins()` | 声明输入输出 pin |
| `process(...)` | 在各自 cpp 中实现实际处理 |

## 数据流

```mermaid
flowchart LR
    INIT[registerBuiltinFilters] --> REG[FilterRegistry]
    REG --> CREATE[create filter]
    CREATE --> PIPE[FilterPipeline]
```

## 关键约束

- 内置滤镜都实现统一 `IFilter` pin 和启用接口。
- 视频滤镜处理 `core::VideoFrame`，音频滤镜处理 PCM sample buffer。

## 注意点

- 新增内置滤镜时，要补注册函数、头文件声明和对应 cpp。
- 滤镜处理逻辑应保持输入输出格式约束清晰，避免破坏渲染器支持的帧格式。
