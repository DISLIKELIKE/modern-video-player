# Filters 使用说明

## 状态说明（2026-03-08）
- 当前生效的滤镜主链是 `PlayerCore` 内的 `FilterPipeline`。
- `VideoFilterChain` / `AudioFilterChain` 已实现，但当前仓库中仍主要作为可复用组件预留，未替代 `FilterPipeline` 成为顶层主流程入口。
- 本文档记录的是现有代码级接入点与内置滤镜范围；独立 UI / 设置入口以后续文档和代码状态为准。

## 架构位置
- `include/filters/*`：滤镜接口和注册/管道定义。
- `src/filters/*`：注册中心、处理管道、内置滤镜实现。

## 核心接口
- `IVideoFilter`：处理 `core::VideoFrame`。
- `IAudioFilter`：处理 PCM 音频缓冲。
- `FilterRegistry`：注册和创建滤镜工厂。
- `FilterPipeline`：串联执行多个音视频滤镜。
- `VideoFilterChain` / `AudioFilterChain`：更细粒度的音视频滤镜链封装，当前未作为主流程默认入口。

## 内置滤镜
- `brightness`：亮度，范围 `-100 ~ +100`。
- `contrast`：对比度，范围 `0.5 ~ 2.0`。
- `saturation`：饱和度，范围 `0.0 ~ 2.0`。
- `volume_balance`：音量 / 声像，`volume` 范围 `0.0 ~ 4.0`，`balance` 范围 `-1.0 ~ +1.0`。

## 集成点
- `PlayerCore` 在音频消费与视频渲染前调用 `FilterPipeline`。
- 内置滤镜在 `PlayerCore` 构造时通过 `registerBuiltinFilters()` 注册。
- 当前内置视频滤镜面向 `YUV420P` 帧，音频滤镜面向 `S16` PCM 输出。

