# 开发日志

## 问题 12: 企业级多线程架构重构

**日期**: 2026-02-27
**状态**: 已完成

### 问题描述
原有架构存在以下问题：
1. 组件职责不清晰，VideoPlayer 承担过多职责
2. 线程模型复杂，难以维护
3. 内存管理容易出错，导致双重释放等 bug

### 解决方案
重构为企业级多线程架构，引入以下新组件：

1. **Demuxer** - 解封装器，封装 AVFormatContext 的读取操作
2. **DecoderWorker** - 解码工作线程，封装单个流的解码逻辑
3. **ThreadSafeQueue** - 线程安全队列模板
4. **Clock** - 时钟同步器，管理音视频同步

### 架构图
```
VideoPlayer (主控制器)
    ├── Demuxer (解封装器) → PacketQueue
    ├── DecoderWorker (视频解码线程)
    ├── DecoderWorker (音频解码线程)
    ├── Display (渲染器)
    ├── AudioPlayer (音频播放)
    └── Clock (时钟同步)
```

---

## 问题 11: 并发读取 AVFormatContext 导致崩溃

**日期**: 2026-02-27
**状态**: 已解决

### 问题描述
播放视频时出现大量 FFmpeg 解码错误和访问冲突崩溃：
```
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
[aac @ ...] Number of scalefactor bands in group (53) exceeds limit (49).
0xC0000005: 写入位置 0x0000022947799000 时发生访问冲突
```
调用堆栈显示错误在 `av_read_frame(format_ctx_, packet)` 处。

### 日志输出
```
Video decoder opened: 1920x1080, format: yuv420p
Audio decoder opened: 48000Hz, 2 channels, fltp
Video decode thread started
Audio decode thread started
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
... (大量类似错误)
```

### 分析记录
1. 问题出现在视频解码线程和音频解码线程同时运行时
2. 两个线程各自拥有独立的 VideoDecoder/AudioDecoder 实例
3. 两个实例共享同一个 `AVFormatContext* format_ctx_`
4. 在 `decodeFrame()` 中都调用了 `av_read_frame(format_ctx_, packet)`
5. 并发读取导致 packet 数据错乱，H264 解码器读取到错误的 NAL 单元

### 根本原因
**并发调用 `av_read_frame()` 导致数据竞争**。`AVFormatContext` 不是线程安全的，两个线程同时读取会导致：
- 读取位置错乱
- Packet 数据被覆盖
- 解码器收到损坏的数据

### 解决方案
引入统一的 `PacketReaderThread` 作为唯一的 packet 读取入口：
1. `PacketReaderThread` 是唯一调用 `av_read_frame()` 的地方
2. 读取到的 packet 根据 stream_index 分发到 `PacketQueue`
3. `VideoDecodeThread` 和 `AudioDecodeThread` 从各自的 `PacketQueue` 获取 packet
4. 解码器新增 `decodePacket()` 方法接收外部传入的 packet

---

## 问题 9: VideoFrame/AudioFrame 移动语义缺陷导致崩溃

**日期**: 2026-02-25
**状态**: 已解决

### 问题描述
程序启动播放后立即崩溃，错误信息：
```
modern-video-player.exe - 应用程序错误
0x00007FFF7A80DA4C 指令引用了 0xFFFFFFFFFFFFFFFF 内存。该内存不能为 read
```

### 日志输出
```
Video decoder opened: 1920x1080, format: yuv420p
Audio decoder opened: 48000Hz, 2 channels, fltp
Playing video...
Video decode thread started
Audio decode thread started
(程序崩溃)
```

### 分析记录
1. 错误地址 0xFFFFFFFFFFFFFFFF = -1，表示空指针/无效指针
2. 崩溃发生在视频解码线程启动后不久
3. 代码审查发现 `VideoFrame` 和 `AudioFrame` 类缺少正确的移动语义实现

### 根本原因
`FrameQueue::pop()` 使用 `std::move` 将帧移动出队列：
```cpp
frame = std::move(queue_.front());
queue_.pop();
```

`VideoFrame` 类没有定义移动构造函数和移动赋值运算符：
- 默认的移动操作只是浅拷贝 `frame_` 指针
- 原对象析构时调用 `av_frame_free(&frame_)` 释放内存
- 目标对象的 `frame_` 变成悬空指针
- 渲染循环访问悬空指针导致崩溃

### 解决方案
为 `VideoFrame` 和 `AudioFrame` 类实现正确的移动语义：
1. 添加移动构造函数，将 `frame_` 指针转移
2. 添加移动赋值运算符，将 `frame_` 指针转移
3. 将原对象的 `frame_` 设为 nullptr，防止析构时释放内存

---

## 问题 13: Core API + Scheduler + Filter 多线程重构落地

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 现有播放器主流程仍以旧架构为中心，缺少规格要求的 `Core API / Scheduler / Filter` 模块化分层。
- 音视频解码线程需要在新核心中独立调度，并通过队列实现无阻塞解耦。

### 分析记录
- 新增 `core` 层：`frame/frame_queue/clock/command/scheduler/player_core`。
- 新增 `filters` 层：`video_filter/audio_filter/filter_registry/filter_pipeline/builtin filters`。
- `VideoPlayer` 增加 `USE_NEW_PLAYER_CORE` 迁移开关路径，保持旧接口不变。

### 修改文件
- include/core/frame.h
- include/core/frame_queue.h
- include/core/clock.h
- include/core/command.h
- include/core/scheduler.h
- include/core/player_core.h
- src/core/frame.cpp
- src/core/clock.cpp
- src/core/scheduler.cpp
- src/core/player_core.cpp
- include/filters/video_filter.h
- include/filters/audio_filter.h
- include/filters/filter_registry.h
- include/filters/filter_pipeline.h
- include/filters/builtin_filters.h
- src/filters/filter_registry.cpp
- src/filters/filter_pipeline.cpp
- src/filters/brightness_filter.cpp
- src/filters/contrast_filter.cpp
- src/filters/saturation_filter.cpp
- src/filters/builtin_filters.cpp
- include/video_player.h
- src/video_player.cpp
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp
- tests/core_clock_tests.cpp

---

## 问题 14: 播放器架构收敛为 Core 单路径

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 项目同时保留旧播放链路和新核心链路，存在维护分叉与并发设计风险。

### 解决方案
- `VideoPlayer` 仅保留 `PlayerCore` 包装实现，删除旧链路分支。
- CMake 删除旧模块编译入口，统一到 `core/*` + `filters/*`。
- 清理旧头源文件，保留必要基础设施和输出模块。
- 新增架构重构文档：`docs/ARCHITECTURE_REFACTOR_2026-03-06.md`。

### 修改文件
- CMakeLists.txt
- include/video_player.h
- src/video_player.cpp
- docs/ARCHITECTURE_REFACTOR_2026-03-06.md
- 删除旧模块文件（decoder/thread/sync/packet/legacy clock/frame_queue）

---

## 问题 15: 小屏窗口过大且拖拽缩放不稳定

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 播放器在小屏设备上按视频原始分辨率（如 1920x1080）直接开窗，窗口初始显示过大。
- 用户拖拽窗口后，部分环境下渲染区域未稳定跟随，表现为“窗口不能正常调整”。

### 日志输出
```
Display initialized: window 1306x734 (source 1920x1080)
```

### 分析记录
- `PlayerCore::open()` 直接把视频分辨率传给 `Display::init()`，没有根据屏幕可用区做首帧窗口缩放。
- 事件处理只监听 `SDL_WINDOWEVENT_RESIZED`，未覆盖常见的 `SDL_WINDOWEVENT_SIZE_CHANGED`。
- 渲染目标矩形直接铺满窗口，缺少按源视频比例计算的目标区域。

### 解决方案
- 在 `Display::init()` 增加基于 `SDL_GetDisplayUsableBounds()` 的初始窗口尺寸计算，保持原始宽高比并限制到屏幕可用区 90%。
- 同时处理 `SDL_WINDOWEVENT_RESIZED` 和 `SDL_WINDOWEVENT_SIZE_CHANGED`，确保拖拽时窗口尺寸状态实时更新。
- 渲染时按视频宽高比计算 `dst_rect`，避免窗口变化时拉伸失真。

### 修改文件
- src/display.cpp
