# 问题修复记录

本文档记录开发过程中遇到的问题及其解决方案。

---

## 问题列表

| # | 日期 | 问题 | 状态 |
|---|------|------|------|
| 1 | 2026-02-17 | FFmpeg 8.0 兼容性问题 | ✅ 已修复 |
| 2 | 2026-02-24 | 视频流索引不匹配 | ✅ 已修复 |
| 3 | 2026-02-24 | 音频流索引不匹配 | ✅ 已修复 |
| 4 | 2026-02-24 | YUV 数据渲染错误 | ✅ 已修复 |
| 5 | 2026-02-24 | 企业级 Quill 日志通道 | ✅ 已修复 |
| 6 | 2026-02-25 | 多线程播放架构重构 | ✅ 已完成 |
| 7 | 2026-02-25 | 音频播放架构修复 | ✅ 已修复 |
| 9 | 2026-02-25 | VideoFrame/AudioFrame 移动语义缺陷 | ✅ 已修复 |
| 10 | 2026-02-25 | 多解码器实例竞争读取导致解码错误 | ✅ 已修复 |
| 11 | 2026-02-27 | 并发读取 AVFormatContext 导致崩溃 | ✅ 已修复 |
| 12 | 2026-02-27 | 企业级多线程架构重构 | ✅ 已完成 |
| 15 | 2026-03-06 | 小屏窗口过大且拖拽缩放不稳定 | ✅ 已修复 |
| 18 | 2026-03-07 | DASH 解析编译失败与格式能力矩阵缺失 | ✅ 已修复 |
| 19 | 2026-03-07 | D3D11VA 硬解最小闭环与软解回退 | ✅ 已修复 |
| 20 | 2026-03-07 | 探测入口与格式回归脚本落地 | ✅ 已修复 |
| 21 | 2026-03-07 | GitHub Actions 自动格式回归接入 | ✅ 已修复 |
| 22 | 2026-03-07 | 播放列表主链路、设置持久化与快捷键首版接入 | ✅ 已修复 |

---

## 问题 12: 企业级多线程架构重构

**日期**: 2026-02-27

### 问题描述

原有架构存在以下问题：
1. 组件职责不清晰，VideoPlayer 承担过多职责
2. 线程模型复杂，难以维护
3. 内存管理容易出错，导致双重释放等 bug

### 解决方案

重构为企业级多线程架构：

```
┌─────────────────────────────────────────────────────────────┐
│                    VideoPlayer (主控制器)                    │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Demuxer      │  │ DecoderWorker│  │ Display      │      │
│  │ (解封装器)    │  │ (解码工作线程)│  │ (渲染器)     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         │                 │                 │               │
│         ▼                 ▼                 ▼               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │PacketQueue   │  │ Clock        │  │ AudioPlayer  │      │
│  │ (包队列)     │  │ (时钟同步)   │  │ (音频播放)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### 新增组件

1. **Demuxer (解封装器)**
   - 封装 AVFormatContext 的读取操作
   - 提供统一的 packet 读取接口
   - 支持 seek 操作

2. **DecoderWorker (解码工作线程)**
   - 封装单个流的解码逻辑
   - 从 PacketQueue 获取 packet，解码后通过回调输出
   - 支持暂停/恢复/flush

3. **ThreadSafeQueue (线程安全队列)**
   - 通用的线程安全队列模板
   - 支持阻塞和非阻塞操作
   - 支持 EOF 信号传递

4. **Clock (时钟同步)**
   - 管理主时钟
   - 计算音视频同步延迟
   - 支持多种同步模式

### 修改文件

- 新增 `include/demuxer.h`, `src/demuxer.cpp`
- 新增 `include/decoder_worker.h`, `src/decoder_worker.cpp`
- 新增 `include/thread_safe_queue.h`
- 新增 `include/clock.h`, `src/clock.cpp`
- 重构 `include/video_player.h`, `src/video_player.cpp`
- 修改 `CMakeLists.txt`
- 修复 `src/packet_reader.cpp` 双重释放 bug

---

## 问题 11: 并发读取 AVFormatContext 导致崩溃

**日期**: 2026-02-27

### 问题描述

播放视频时出现大量 FFmpeg 解码错误和访问冲突崩溃：
```
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
0xC0000005: 写入位置 0x... 时发生访问冲突
```

### 原因分析

视频解码线程 (`VideoDecodeThread`) 和音频解码线程 (`AudioDecodeThread`) 各自拥有独立的解码器实例，但共享同一个 `AVFormatContext`。两个线程并发调用 `av_read_frame(format_ctx_, packet)` 导致数据竞争，读取到的 packet 数据错乱，引发 H264 解码错误和内存访问冲突。

### 解决方案

引入统一的 `PacketReaderThread` 作为唯一的 packet 读取入口：

1. **新增 PacketReaderThread 类**
   - 作为唯一的 `av_read_frame()` 调用点
   - 根据 stream_index 将 packet 分发到对应的 PacketQueue

2. **新增 PacketRef 和 PacketQueue 类**
   - `PacketRef`: 包装 AVPacket 的智能结构体，支持移动语义
   - `PacketQueue`: 线程安全的 packet 队列，支持阻塞等待

3. **修改解码器接口**
   - `VideoDecoder` 和 `AudioDecoder` 新增 `decodePacket()` 方法
   - 接收外部传入的 packet，而非内部读取

4. **重构解码线程**
   - `VideoDecodeThread` 和 `AudioDecodeThread` 从 `PacketQueue` 获取 packet
   - 不再直接调用 `av_read_frame()`

### 修改文件

- 新增 `include/packet_reader.h`
- 新增 `src/packet_reader.cpp`
- 修改 `include/video_decoder.h`
- 修改 `src/video_decoder.cpp`
- 修改 `include/audio_decoder.h`
- 修改 `src/audio_decoder.cpp`
- 修改 `include/video_decode_thread.h`
- 修改 `src/video_decode_thread.cpp`
- 修改 `include/audio_decode_thread.h`
- 修改 `src/audio_decode_thread.cpp`
- 修改 `include/video_player.h`
- 修改 `src/video_player.cpp`
- 修改 `CMakeLists.txt`

---

## 问题 10: 多解码器实例竞争读取导致解码错误

## 问题 1: FFmpeg 8.0 兼容性问题

**日期**: 2026-02-17

### 问题描述

编译时报错，`codec_ctx_->avctx->priv_data` 在 FFmpeg 8.0 中不可用。

### 原因

FFmpeg 8.0 更改了 API，移除了对 `avctx->priv_data` 的直接访问。

### 解决方案

修改 `video_decoder.cpp` 和 `audio_decoder.cpp`：
- 在解码器类中添加 `format_ctx_` 成员变量
- 直接使用传入的 format context 而非从 codec context 获取

### 修改文件

- `include/video_decoder.h`
- `include/audio_decoder.h`
- `src/video_decoder.cpp`
- `src/audio_decoder.cpp`

---

## 问题 2: 视频流索引不匹配

**日期**: 2026-02-24

### 问题描述

播放 mp4 文件时，视频无法正常显示。日志显示：
```
decodeFrame: read packet, stream_index=0, expected=1
decodeFrame: packet stream mismatch, skipping
```
程序循环 48 次才能读到正确的视频帧。

### 原因

MP4 文件的流顺序是：音频流(索引 0) 在前，视频流(索引 1) 在后。`av_read_frame()` 返回的包可能是任意流的（通常是第一个流 - 音频流）。原代码遇到不匹配的流时直接返回 false，导致视频帧无法解码。

### 解决方案

修改 `src/video_decoder.cpp` 的 `decodeFrame()` 方法：
- 将遇到不匹配流时返回 false，改为 continue 跳过该包
- 循环读取直到找到正确流索引的包

### 修改文件

- `src/video_decoder.cpp`

### 代码变更

```cpp
// 修改前
if (packet->stream_index != stream_idx_) {
    av_packet_unref(packet);
    av_packet_free(&packet);
    return false;
}

// 修改后
while (true) {
    ret = av_read_frame(format_ctx_, packet);
    // ...
    if (packet->stream_index != stream_idx_) {
        av_packet_unref(packet);
        continue;  // 继续循环读取
    }
    break;  // 找到正确的流
}
```

---

## 问题 3: 音频流索引不匹配

**日期**: 2026-02-24

### 问题描述

与视频流索引相同的问题，但出现在音频解码器中。

### 原因

同样的问题：音频包可能不是第一个被读取的流。

### 解决方案

修改 `src/audio_decoder.cpp` 的 `decodeFrame()` 方法，应用与视频解码器相同的修复。

### 修改文件

- `src/audio_decoder.cpp`

---

## 问题 4: YUV 数据渲染错误

**日期**: 2026-02-24

### 问题描述

解码成功后程序立即退出，没有画面显示。

### 原因

`renderFrame` 函数使用错误的 YUV 数据：
- 原来传递的是 `frame->data[0]`（只是 Y 平面指针）
- 然后错误地假设 Y/U/V 是连续存储的

实际上 AVFrame 中 Y/U/V 是分开存储的，使用 `linesize` 来计算每行的步长。

### 解决方案

1. 传递整个 AVFrame 指针而不是 `data[0]`
2. 正确使用 Y/U/V 平面的数据和行大小

### 修改文件

- `src/display.cpp`
- `src/video_player.cpp`

### 代码变更

```cpp
// video_player.cpp - 修改前
display_->renderFrame(frame->data[0], frame->width, frame->height);

// video_player.cpp - 修改后
display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);

// display.cpp - renderFrame 函数
// 修改前
int ret = SDL_UpdateYUVTexture(
    texture_, nullptr,
    data, width,
    data + width * height, width / 2,
    data + width * height * 5 / 4, width / 2
);

// 修改后
AVFrame* frame = (AVFrame*)data;
int ret = SDL_UpdateYUVTexture(
    texture_, nullptr,
    frame->data[0], frame->linesize[0],
    frame->data[1], frame->linesize[1],
    frame->data[2], frame->linesize[2]
);
```

---

## 问题 5: 企业级 Quill 日志通道

**日期**: 2026-02-24

### 问题描述

- 旧日志系统只使用 `std::cout/std::cerr`，无法满足企业记录、异步写盘与轮转需求。
- 无运行时配置通道，无法根据环境调整日志目录、文件大小与等级阈值。
- 缺乏健壮性：目录不可写或 Quill 初始化失败时没有明确告警与降级逻辑。

### 原因分析

- 为规避 Quill v6.x API 变更曾临时禁用 Quill，引起功能倒退。
- Logger 逻辑集中在头文件宏内，扩展点有限，新增配置与降级路径困难。

### 解决方案

- 重新启用 Quill，构建异步 Backend + ConsoleSink + RotatingFileSink 双通道；日志按照 `[time][level][thread][logger][category] message` 统一格式输出。
- 新增 `LoggingConfigLoader`，解析 `config/logging.conf` 及 `MVP_LOG_*` 环境变量，非法值自动纠正并输出 `LOG_WARNING`。
- 当 `USE_QUILL_LOGGING` 未定义、目录不可写或 Quill 抛出异常时，自动降级到 stdout/stderr，并保留旧宏行为。
- 同步更新文档（LOGGING.md、VERSION.md、CHANGELOG.md）并提供默认配置文件。

### 修改文件

- `include/logger.h`
- `src/logger.cpp`
- `config/logging.conf`
- `docs/LOGGING.md`
- `docs/CHANGELOG.md`
- `docs/VERSION.md`

---

## 问题 6: 多线程播放架构重构

**日期**: 2026-02-25

### 问题描述

- 原有架构为单线程 playLoop，解码和渲染在同一线程
- 视频解码会阻塞渲染线程，导致画面卡顿
- 音视频同步实现困难
- 队列满时 CPU 忙轮询导致占用过高

### 解决方案

1. **新增 FrameQueue 模板类**
   - 实现线程安全的帧队列
   - 使用 condition_variable 实现阻塞等待，避免 CPU 忙轮询
   - 支持 push/pop/clear/stop 操作

2. **新增 VideoDecodeThread 和 AudioDecodeThread**
   - 独立的视频/音频解码线程
   - 解码后的帧通过 FrameQueue 传递给渲染线程
   - 支持 pause/resume/flush 控制

3. **新增 SyncManager 同步管理器**
   - 支持 AudioMaster/VideoMaster/Free 三种同步模式
   - 实现帧延迟计算
   - 实现跳帧/重复帧策略

4. **重构 VideoPlayer**
   - 从单线程 playLoop 改为多线程 renderLoop
   - 添加 setSyncMode 方法支持同步模式切换
   - 修复 AudioPlayer::play 签名不匹配问题

### 修改文件

- 新增 `include/frame_queue.h`
- 新增 `include/video_decode_thread.h`
- 新增 `include/audio_decode_thread.h`
- 新增 `include/sync_manager.h`
- 新增 `src/video_decode_thread.cpp`
- 新增 `src/audio_decode_thread.cpp`
- 新增 `src/sync_manager.cpp`
- 修改 `include/video_player.h`
- 修改 `include/audio_decoder.h`
- 修改 `src/video_player.cpp`
- 修改 `CMakeLists.txt`

---

## 问题 7: 音频播放架构修复

**日期**: 2026-02-25

### 问题描述

- AudioDecodeThread 解码后的音频通过 FrameQueue 传递给 renderLoop
- renderLoop 逐帧调用 AudioPlayer::play()，导致音频断断续续
- SDL 回调机制需要持续的数据流，当前实现无法满足

### 解决方案

1. 修改 AudioDecodeThread::start() 方法，增加 AudioPlayer* 参数
2. 解码线程解码完成后，直接调用 audio_player_->play() 将数据放入 SDL 队列
3. 移除 renderLoop 中的音频播放代码，由解码线程直接处理

### 修改文件

- `include/audio_decode_thread.h`
- `src/audio_decode_thread.cpp`
- `src/video_player.cpp`

---

## 待解决的问题

### 问题 8: 硬件加速解码支持

**状态**: 待实现

需要添加 CUDA/D3D11VA 硬件加速解码支持，提升解码性能。

---

## 问题 9: VideoFrame/AudioFrame 移动语义缺陷导致崩溃

**日期**: 2026-02-25

### 问题描述

程序启动播放后立即崩溃，错误信息：
```
modern-video-player.exe - 应用程序错误
0x00007FFF7A80DA4C 指令引用了 0xFFFFFFFFFFFFFFFF 内存。该内存不能为 read
```

### 原因分析

`VideoFrame` 和 `AudioFrame` 类缺少正确的移动语义实现。

在 `FrameQueue::pop()` 中使用 `std::move` 将帧移动出队列：
```cpp
frame = std::move(queue_.front());
queue_.pop();
```

由于没有定义移动构造函数和移动赋值运算符，默认的移动操作只是浅拷贝 `frame_` 指针。当原对象析构时调用 `av_frame_free(&frame_)` 释放内存，目标对象的 `frame_` 变成悬空指针。渲染循环访问此悬空指针时导致崩溃。

### 解决方案

1. 为 `VideoFrame` 类添加移动构造函数和移动赋值运算符
2. 为 `AudioFrame` 类添加移动构造函数和移动赋值运算符
3. 显式删除拷贝构造函数和拷贝赋值运算符
4. 移动时将原对象的 `frame_` 指针置为 nullptr，防止析构时释放仍被使用的内存

### 修改文件

- `include/video_decoder.h`
- `src/video_decoder.cpp`
- `include/audio_decoder.h`
- `src/audio_decoder.cpp`

### 代码变更

```cpp
// video_decoder.h - 添加移动语义声明
VideoFrame(VideoFrame&& other) noexcept;
VideoFrame& operator=(VideoFrame&& other) noexcept;
VideoFrame(const VideoFrame&) = delete;
VideoFrame& operator=(const VideoFrame&) = delete;

// video_decoder.cpp - 实现移动构造函数
VideoFrame::VideoFrame(VideoFrame&& other) noexcept
    : frame_(other.frame_)
    , pts_(other.pts_) {
    other.frame_ = nullptr;
}

// audio_decoder.h/cpp - 类似实现
```

---

## 问题 10: 多解码器实例竞争读取导致解码错误

**日期**: 2026-02-25

### 问题描述

播放视频时出现大量 FFmpeg 解码错误：
```
[h264 @ ...] Invalid NAL unit size (0 > 1045).
[h264 @ ...] missing picture in access unit with size 1049
[aac @ ...] channel element 0.0 duplicate
[mov,mp4,m4a,3gp,3g2,mj2 @ ...] DTS 2625751 < 2632415 out of order
```

### 原因分析

在 `VideoPlayer::open()` 中创建了 `video_decoder_` 和 `audio_decoder_` 解码器实例用于获取视频信息。然后在 `play()` -> `initDecodeThreads()` 中又创建了 `VideoDecodeThread` 和 `AudioDecodeThread`，这两个类内部又各自创建了新的解码器实例。

两个解码器同时从同一个 `AVFormatContext` 读取 packet，造成数据竞争，导致解码错误和数据损坏。

### 解决方案

在 `play()` 方法中，调用 `initDecodeThreads()` 之前，先关闭并释放 `video_decoder_` 和 `audio_decoder_`：

```cpp
void VideoPlayer::play() {
    // ...
    playing_.store(true);
    
    video_decoder_.reset();
    audio_decoder_.reset();
    
    if (!initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_)) {
        // ...
    }
}
```

### 修改文件

- `src/video_player.cpp`

---

## 相关文档

- [VERSION.md](./VERSION.md) - 版本记录
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 架构设计
- [WINDOWS_SETUP.md](./WINDOWS_SETUP.md) - Windows 配置指南
- [LOGGING.md](./LOGGING.md) - 日志系统说明

---

## 问题 13: Core API + Scheduler + Filter 多线程重构落地

**日期**: 2026-03-06

### 问题描述
- 需要按规格引入 Core API、Scheduler 和 Filter 插件框架，并保持 `VideoPlayer` 外部接口稳定。

### 原因分析
- 旧架构以 `VideoPlayer` 聚合大部分职责，缺少可演进的核心层与调度层。
- 缺少 `USE_NEW_PLAYER_CORE` 受控迁移路径下的新核心实现。

### 解决方案
- 新增 `core` 模块：`PlayerCore`、`Scheduler`、`FrameQueue`、`Clock`、`Command`、`Frame`。
- 新增 `filters` 模块：过滤器接口、注册中心、处理管道、亮度/对比度/饱和度内置滤镜。
- `VideoPlayer` 改造为双路径：`USE_NEW_PLAYER_CORE=ON` 走新核心，OFF 保持旧实现。
- 新增 `tests/core_frame_queue_tests.cpp`、`tests/core_clock_tests.cpp`。

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

## 问题 14: 架构收敛为 Core 单路径

**日期**: 2026-03-06

### 问题描述
- 历史上并存的新旧播放链路增加了维护成本和行为不一致风险。

### 原因分析
- 旧链路和新链路共存导致排障路径复杂，且旧链路存在结构性并发隐患。

### 解决方案
- 删除旧链路模块，统一到 `VideoPlayer -> PlayerCore -> Scheduler -> Queue -> Output`。
- 构建系统改为仅编译新核心模块。
- 新增重构文档说明保留文件与职责边界。

### 修改文件
- CMakeLists.txt
- include/video_player.h
- src/video_player.cpp
- docs/ARCHITECTURE_REFACTOR_2026-03-06.md
- 旧模块头源文件删除（见 DEVELOP_LOG 问题 14）

---

## 问题 15: 小屏窗口过大且拖拽缩放不稳定

**日期**: 2026-03-06

### 问题描述
- 小屏设备播放高分辨率视频时，窗口初始尺寸过大，影响操作。
- 窗口拖拽后部分场景下渲染区域未及时更新，用户感知为“窗口不能调整”。

### 原因分析
- `Display::init()` 直接使用视频原始分辨率创建窗口，未按屏幕可用区域做首屏缩放。
- 事件处理仅监听 `SDL_WINDOWEVENT_RESIZED`，未覆盖 `SDL_WINDOWEVENT_SIZE_CHANGED`。
- 渲染目标区域直接使用窗口宽高，缺少按视频比例计算的目标矩形。

### 解决方案
- 启动时通过 `SDL_GetDisplayUsableBounds()` 计算可用屏幕区域，将初始窗口限制在可用区 90% 内并保持视频比例。
- 同时处理 `SDL_WINDOWEVENT_RESIZED` 与 `SDL_WINDOWEVENT_SIZE_CHANGED`，确保窗口尺寸变化实时生效。
- 按源视频比例计算 `SDL_RenderCopy` 的目标矩形，避免拖拽后画面拉伸。

### 修改文件
- src/display.cpp
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

## 问题 16: 最大化/缩放时画面卡住，并补充基础交互控制

**日期**: 2026-03-07

### 问题描述
- 播放时最大化窗口或拖动缩放窗口，视频画面可能卡住，音频继续播放。
- 缺少进度条、音量调节、拖动进度条等基础交互能力。

### 原因分析
- SDL 窗口事件处理与渲染调用分散在不同线程路径，窗口尺寸变化时更容易触发画面刷新停滞。
- 显示层没有控制条与鼠标交互请求上报能力。

### 解决方案
- 调整事件处理路径：在渲染/空闲渲染路径中轮询 SDL 事件，主线程仅消费交互请求。
- 为 Display 添加控制层绘制：进度条、音量条、暂停状态提示。
- 新增鼠标交互：拖动进度条触发 seek，拖动音量条调节音量。
- PlayerCore 增加对 seek/音量请求的消费执行。

### 修改文件
- include/display.h
- src/display.cpp
- src/core/player_core.cpp
- src/main.cpp
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

## 问题 17: 企业级 MPC-HC 模块骨架落地（阶段二/三推进）

**日期**: 2026-03-07

### 问题描述
- 企业级模块规划已定义，但多数模块缺少代码入口，无法继续并行开发。

### 原因分析
- 旧实现以核心播放链路为主，模块边界不完整，难以分工推进。

### 解决方案
- 新增并接入企业级基础设施和模块骨架：任务队列、帧池、解码线程基类。
- 引入渲染抽象层（`IVideoRenderer` + `RendererFactory`），并让 `PlayerCore` 切换到抽象接口。
- 增加音频均衡器/混音器、解码器工厂、字幕 SRT 解析、播放列表、设置/快捷键、皮肤、插件、格式与流媒体解析模块。
- 完善滤镜基类与音视频滤镜链，补齐音量平衡滤镜。
- 同步更新 tasklist 对应已实现项。

### 修改文件
- CMakeLists.txt
- include/core/task_queue.h
- include/core/frame_pool.h
- src/core/frame_pool.cpp
- include/core/decoder_thread.h
- src/core/decoder_thread.cpp
- include/render/*
- src/render/*
- include/audio/*
- src/audio/*
- include/decoder/*
- src/decoder/*
- include/subtitle/*
- src/subtitle/*
- include/playlist/*
- src/playlist/*
- include/config/*
- src/config/*
- include/input/*
- src/input/*
- include/media/*
- src/media/*
- include/streaming/*
- src/streaming/*
- include/ui/*
- src/ui/*
- include/plugin/*
- src/plugin/*
- include/filters/filter_base.h
- include/filters/video_filter_chain.h
- src/filters/video_filter_chain.cpp
- include/filters/audio_filter_chain.h
- src/filters/audio_filter_chain.cpp
- include/filters/audio_filter.h
- include/filters/video_filter.h
- include/filters/builtin_filters.h
- src/filters/volume_balance_filter.cpp
- src/filters/builtin_filters.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/audio_player.h
- src/audio_player.cpp
- .monkeycode/specs/enterprise-quill-logging/tasklist.md
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

---

## 问题 18: DASH 解析编译失败与格式能力矩阵缺失

**日期**: 2026-03-07

### 问题描述
- `src/streaming/dash_manifest_parser.cpp` 在 MSVC 下编译失败，阻塞全量构建。
- 缺少一个可直接复用的“运行时格式能力矩阵”入口，不利于单人迭代中快速验证格式覆盖。

### 原因分析
- 原始字符串正则使用了默认分隔符，表达式中出现 `)"` 触发提前终止，导致语法错误。
- 现有格式支持模块虽有基础接口，但缺少统一 CLI 检查入口与主力格式覆盖输出。

### 解决方案
- 修复 DASH 正则：改为自定义 raw-string 分隔符，恢复 MSVC 编译通过。
- 扩展 `FormatSupport`：
  - 增加运行时容器/编解码器枚举（`av_demuxer_iterate` / `av_codec_iterate`）
  - 增加播放目标评估（高分辨率/高帧率/多音道）
- 改造 `main`，新增命令：
  - `--capabilities`
  - `--evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>`
- 增强 `Demuxer` 与 `PlayerCore` 音频链路稳健性（多音道输出参数对齐、重采样器复用等）。

### 修改文件
- src/streaming/dash_manifest_parser.cpp
- include/media/format_support.h
- src/media/format_support.cpp
- include/demuxer.h
- src/demuxer.cpp
- include/audio_player.h
- src/audio_player.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md
- docs/README.md

---

## 问题 19: D3D11VA 硬解最小闭环与软解回退

**日期**: 2026-03-07

### 问题描述
- 需要在 Windows 下优先利用 D3D11VA 硬解高分辨率/高帧率视频，并确保失败时可自动回退软解。
- 硬件解码输出通常是 GPU 帧或 `NV12`，现有 SDL 渲染链路要求 `YUV420P`，存在格式不匹配风险。

### 解决方案
- `PlayerCore` 增加 D3D11VA 尝试逻辑：
  - 检测 codec 的 D3D11VA HW config；
  - 创建 `AV_HWDEVICE_TYPE_D3D11VA` 设备上下文；
  - 绑定 `get_format` 回调选择硬件像素格式。
- 若 `avcodec_open2` 在硬解路径失败，自动重建解码上下文并回退到软解。
- 新增视频帧输出规整链路：
  - 硬件帧先 `av_hwframe_transfer_data` 转到系统内存；
  - 非 `YUV420P` 帧统一经 `sws_scale` 转为 `YUV420P` 再进入渲染。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp

---

## 问题 20: 探测入口与格式回归脚本落地

**日期**: 2026-03-07

### 问题描述
- 需要把格式覆盖验证从“手工打开视频观察”升级为“可重复的命令行回归”。
- 现有能力入口只有总体能力评估，缺少单文件探测和批量样本报告。

### 解决方案
- 在 `main` 中新增 `--probe-file <media_file>`：输出 `probe.*` 机器可读字段，包含容器/视频/音频状态、分辨率、帧率、声道与建议信息。
- 新增 `tools/format_regression/run_format_regression.ps1`：
  - 读取 `tools/format_regression/format_samples.csv`；
  - 逐个调用 `--probe-file`；
  - 生成 `docs/reports/FORMAT_REGRESSION_*.md` 报告；
  - 返回码语义：`0=全部PASS`，`1=存在PARTIAL`，`2=存在FAIL`。
- 补充 `docs/FORMAT_REGRESSION.md` 与文档索引，便于在 VS2022/PowerShell 下直接执行。

### 修改文件
- src/main.cpp
- tools/format_regression/run_format_regression.ps1
- tools/format_regression/format_samples.csv
- docs/FORMAT_REGRESSION.md
- docs/README.md

---

## 问题 21: GitHub Actions 自动格式回归接入

**日期**: 2026-03-07

### 问题描述
- 回归链路虽可在本地运行，但缺少 PR/主分支自动执行，无法在提交阶段及时拦截格式退化。
- Windows CI 环境与本地依赖发现方式不一致，需补齐构建与脚本兼容性。

### 解决方案
- 新增工作流 `.github/workflows/format-regression.yml`：
  - 在 `windows-latest` 下载 `SDL2/FFmpeg` 预编译包并构建 `Debug`；
  - 执行 `download_test_samples.ps1` 与 `run_all_checks.ps1`；
  - 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 报告产物。
- 调整 `CMakeLists.txt`：
  - Windows 下优先识别 `SDL2::`、`FFMPEG::` 与 `unofficial::ffmpeg::` 导入目标；
  - 保留 `external/` 目录回退逻辑，兼容本地既有构建。
- 调整 `download_test_samples.ps1`：
  - `-FfmpegPath` 支持 PATH 命令名（如 `ffmpeg`），便于 CI 直接调用。
- 更新回归文档与任务清单状态，补齐自动回归入口说明。

### 修改文件
- .github/workflows/format-regression.yml
- CMakeLists.txt
- tools/download_test_samples.ps1
- docs/FORMAT_REGRESSION.md
- docs/REGRESSION_OPERATION_PLAYBOOK.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 22: 播放列表主链路、设置持久化与快捷键首版接入

**日期**: 2026-03-07

### 问题描述
- 播放器主流程仅支持单文件，不支持上一首/下一首与 EOF 自动切换。
- 设置模块未接入运行链路，启动/退出时无法恢复音量和播放速度。
- 默认快捷键缺失关键能力（相对 seek、变速、静音、播放列表切换）。

### 解决方案
- 主链路接入 `PlaylistManager`：
  - 支持命令行传入多个媒体文件；
  - 支持传入 `.m3u8` 作为播放列表；
  - 支持 `PageUp/PageDown` 上一首/下一首；
  - EOF 自动切换下一项。
- 主链路接入 `SettingsManager`：
  - 启动时加载 `config/player_settings.ini`；
  - 缺失或解析失败时回退默认值（音量 100%、速度 1.0x、恢复上次索引）；
  - 退出时保存当前音量、播放速度和播放列表索引。
- 扩展 SDL 事件到播放器控制链路：
  - `Left/Right` seek ±5 秒；
  - `Ctrl+Left/Ctrl+Right` seek ±30 秒；
  - `[`/`]` 调速，`R` 恢复 1.0x；
  - `M` 静音/恢复；
  - `Enter/Alt+Enter/F` 全屏切换；
  - `Esc` 全屏态退全屏，窗口态退出。

### 修改文件
- src/main.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- include/render/opengl_video_renderer.h
- src/render/opengl_video_renderer.cpp
- include/display.h
- src/display.cpp
- include/config/settings_manager.h
- src/config/settings_manager.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
