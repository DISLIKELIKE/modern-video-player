# 2026-03-06 架构重构说明

## 重构目标

- 统一到企业级播放器常见分层：Facade -> Core API -> Scheduler -> Queue -> Output -> Filters。
- 去除旧单体播放链路（旧 decoder/thread/sync/packet 模块），减少并发竞态点。
- 让音视频解码线程与渲染线程解耦，避免“音频正常、视频卡住几帧后停滞”的结构性问题。

## 新架构（当前唯一实现）

- `VideoPlayer`：轻量 Facade，仅负责向外暴露稳定 API。
- `core::PlayerCore`：核心控制面，管理状态机、事件回调、seek/stop 生命周期。
- `core::Scheduler`：3 线程模型（视频解码、音频解码、渲染），含背压控制（80%/50%）。
- `core::FrameQueue`：线程安全帧队列，作为生产者/消费者边界。
- `core::Clock`：主时钟与同步决策。
- `filters::*`：插件化滤镜管道与内置亮度/对比度/饱和度滤镜。

## 删除的旧模块

- `video_decoder/audio_decoder`
- `video_decode_thread/audio_decode_thread`
- `sync_manager`
- `packet_reader`
- `decoder_worker`
- 旧 `clock` 和旧 `frame_queue`

## 当前保留文件清单

### include/

- `include/video_player.h`
- `include/logger.h`
- `include/demuxer.h`
- `include/display.h`
- `include/audio_player.h`
- `include/thread_safe_queue.h`
- `include/core/clock.h`
- `include/core/command.h`
- `include/core/frame.h`
- `include/core/frame_queue.h`
- `include/core/player_core.h`
- `include/core/scheduler.h`
- `include/filters/audio_filter.h`
- `include/filters/video_filter.h`
- `include/filters/filter_registry.h`
- `include/filters/filter_pipeline.h`
- `include/filters/builtin_filters.h`

### src/

- `src/main.cpp`
- `src/video_player.cpp`
- `src/logger.cpp`
- `src/demuxer.cpp`
- `src/display.cpp`
- `src/audio_player.cpp`
- `src/core/frame.cpp`
- `src/core/clock.cpp`
- `src/core/scheduler.cpp`
- `src/core/player_core.cpp`
- `src/filters/filter_registry.cpp`
- `src/filters/filter_pipeline.cpp`
- `src/filters/brightness_filter.cpp`
- `src/filters/contrast_filter.cpp`
- `src/filters/saturation_filter.cpp`
- `src/filters/builtin_filters.cpp`

## 构建策略

- `CMakeLists.txt` 已改为仅编译新架构代码，不再保留旧路径宏切换。

