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
| 23 | 2026-03-07 | 移除 Core 单元测试目标与测试文件 | ✅ 已修复 |
| 24 | 2026-03-07 | 外挂字幕加载入口（SRT）接入主流程 | ✅ 已修复 |
| 25 | 2026-03-07 | 字幕渲染叠加与播放时序同步接入 | ✅ 已修复 |
| 26 | 2026-03-08 | 字幕开关控制与字幕加载异常处理完善 | ✅ 已修复 |
| 27 | 2026-03-08 | 快捷键配置持久化接入（hotkey.*） | ✅ 已修复 |
| 28 | 2026-03-08 | 快捷键冲突检测与恢复默认能力 | ✅ 已修复 |
| 29 | 2026-03-08 | M1 验收 1.4.1：SRT seek 同步自检命令落地 | ✅ 已修复 |
| 30 | 2026-03-08 | M1 验收 1.4.2：播放列表连续播放 5 文件自检通过 | ✅ 已修复 |
| 31 | 2026-03-08 | M1 验收 1.4.3：设置重启恢复自检通过 | ✅ 已修复 |
| 32 | 2026-03-08 | M2 2.1.2：容器矩阵补齐 mov/avi/m2ts 并回归通过 | ✅ 已修复 |
| 33 | 2026-03-08 | M2 2.1.3：视频编码矩阵补齐 MPEG-2 并回归通过 | ✅ 已修复 |
| 34 | 2026-03-08 | M2 2.1.4：音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM 并回归通过 | ✅ 已修复 |

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

---

## 问题 23: 移除 Core 单元测试目标与测试文件

**日期**: 2026-03-07

### 问题描述
- 当前仓库保留了两个 Core 相关测试目标与测试文件，但本次需求要求移除这两项测试内容并删除文件。
- 若仅删除测试源码而不清理构建脚本，会导致构建配置存在悬挂路径风险。

### 解决方案
- 从 `CMakeLists.txt` 移除：
  - `BUILD_CORE_TESTS` 选项；
  - `core_frame_queue_tests`、`core_clock_tests` 两个测试目标；
  - `core_tests` 聚合目标。
- 删除测试文件：
  - `tests/core_frame_queue_tests.cpp`
  - `tests/core_clock_tests.cpp`
- 同步更新变更文档，确保记录与当前仓库状态一致。

### 修改文件
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp（删除）
- tests/core_clock_tests.cpp（删除）
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 24: 外挂字幕加载入口（SRT）接入主流程

**日期**: 2026-03-07

### 问题描述
- 任务清单 `1.1.1` 要求支持外挂字幕加载入口，但当前主流程只有视频/音频播放链路，未提供外部字幕文件入口。
- 项目已存在 `subtitle::SrtParser`，但未接入 `VideoPlayer` 与命令行参数。

### 解决方案
- 在 `VideoPlayer` 增加外挂字幕加载接口：
  - `loadExternalSubtitle()` / `clearExternalSubtitle()`；
  - 支持 `.srt` 文件解析与容错日志；
  - 暴露已加载字幕路径与条目数量，便于后续渲染接入。
- 在 `main` 增加命令行入口：
  - 新增 `--subtitle <file.srt>`；
  - 保持现有播放列表参数逻辑；
  - 未显式传参时，自动尝试加载与媒体同名的 `.srt`。
- 更新任务清单，标记 `1.1.1 外挂字幕加载入口` 已完成。

### 修改文件
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 25: 字幕渲染叠加与播放时序同步接入

**日期**: 2026-03-07

### 问题描述
- 任务清单 `1.1.2` 要求字幕可渲染叠加到画面，但现有渲染接口没有字幕文本通道。
- 任务清单 `1.1.3` 要求字幕与播放/暂停/seek 同步，但主播放时钟链路没有字幕时间轴更新逻辑。

### 解决方案
- 扩展渲染抽象：
  - 在 `IVideoRenderer` 增加 `setSubtitleText()`；
  - SDL 渲染器转发字幕文本到 `Display`；
  - D3D11/OpenGL 先提供兼容桩实现，保持接口一致。
- 在 `Display` 增加字幕叠加层：
  - 新增字幕状态存储与线程安全更新；
  - 在视频帧渲染后、控制条渲染前绘制字幕面板；
  - 支持多行字幕、超长截断与基础可读性样式（阴影+半透明底板）。
  - 当前使用轻量字模渲染，非 ASCII 字符会降级显示。
- 在 `PlayerCore` 增加字幕时间轴驱动：
  - 新增外挂字幕轨道状态与索引缓存；
  - 渲染帧路径与空闲事件路径均调用 `updateSubtitleOverlay()`；
  - 基于当前播放时间选择活跃字幕，覆盖播放、暂停与 seek 场景；
  - 修复锁内调用渲染接口的问题，避免在字幕互斥锁内触发渲染回调。
- 调整 `VideoPlayer::open()` 字幕状态处理，消除“先清空再判断加载”的矛盾逻辑。
- 更新任务清单，标记 `1.1.2`、`1.1.3` 已完成。

### 修改文件
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/display.h
- src/display.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 26: 字幕开关控制与字幕加载异常处理完善

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.1.4` 要求字幕开关与异常处理，但当前字幕仅支持“加载后显示”，缺少运行时开关。
- 外挂字幕加载路径在文件系统异常场景下容错不足，影响稳定性预期。

### 解决方案
- 增加字幕开关控制链路（按键 `V`）：
  - `Display` 新增字幕开关请求；
  - `Renderer` 抽象新增 `consumeToggleSubtitleRequest()`；
  - `PlayerCore` 新增字幕显示状态管理与切换接口；
  - 关闭字幕时立即清空叠加层，开启时按当前播放时间恢复同步。
- 增强外挂字幕异常处理：
  - `VideoPlayer::loadExternalSubtitle()` 改为使用 `std::error_code` 路径检查；
  - 捕获解析器异常并降级为告警日志，不中断播放主流程；
  - 保持“加载失败自动清空旧字幕”的状态一致性。
- 更新帮助信息，补充 `V - Toggle subtitles on/off`。
- 更新任务清单，标记 `1.1.4` 已完成。

### 修改文件
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 27: 快捷键配置持久化接入（hotkey.*）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.3.2` 要求支持键位配置持久化，但当前快捷键逻辑固定写死在 `Display` 事件分支中。
- `HotkeyManager` 仅有骨架实现，未接入主播放链，无法通过配置文件保持自定义键位。

### 解决方案
- 扩展 `HotkeyManager`：
  - 对齐当前首版快捷键默认映射（播放、seek、音量、静音、变速、切集、字幕开关、全屏、退出）；
  - 增加配置序列化能力：`actionConfigKey`、`keyCodeToToken`、`keyCodeFromToken`。
- 将快捷键映射接入渲染输入链：
  - `Display` 事件处理改为由 `HotkeyManager` 驱动；
  - 保留 `Esc` 与 `Enter` 的兼容行为；
  - `Renderer`/`PlayerCore`/`VideoPlayer` 增加热键管理透传接口。
- 在 `main` 的设置加载/保存流程中接入 `hotkey.*`：
  - 启动读取并应用 `player_settings.ini` 的 `hotkey.*`；
  - 非法键位配置降级为默认并记录告警；
  - 退出时将当前键位回写配置，实现持久化。
- 更新默认配置样例 `config/player_settings.ini`，补齐全部 `hotkey.*` 项。
- 更新任务清单，标记 `1.3.2` 已完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 28: 快捷键冲突检测与恢复默认能力

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.3.3` 要求支持键位冲突检测与恢复默认。
- 现有 `hotkey.*` 持久化已可工作，但重复键位配置会产生动作冲突，且缺少“一键回到默认”能力。

### 解决方案
- 扩展 `HotkeyManager`：
  - 新增 `findConflicts()` / `hasConflicts()`，用于检测重复键位绑定；
  - 新增 `resetToDefaults()`，统一恢复默认键位映射。
- 在热键配置加载流程中加入冲突治理：
  - 启动时先应用配置到候选映射，再执行冲突检测；
  - 若发现冲突，记录冲突动作与键位日志，自动回退默认键位；
  - 对非法 token 保留默认并输出告警。
- 新增恢复默认开关：
  - 在 `player_settings.ini` 增加 `hotkey.restore_defaults`；
  - 设置为 `true` 后下次启动自动恢复默认并回写为 `false`。
- 更新帮助输出，补充恢复默认说明。
- 更新任务清单，标记 `1.3.3` 已完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 29: M1 验收 1.4.1（SRT seek 同步自检命令落地）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.1` 要求“`SRT 字幕可用且 seek 后同步`”，现有实现缺少可重复执行的验收入口。
- 若只依赖人工播放观察，回归成本高且难以稳定复现。

### 原因分析
- 字幕时间轴匹配逻辑仅存在于 `PlayerCore` 内部，无法单独验证“顺序播放 + seek 跳转”两类场景。

### 解决方案
- 提取公共时间轴函数 `subtitle::resolveActiveSubtitleIndex(...)`，并由 `PlayerCore` 复用。
- 在 `main` 增加 `--subtitle-sync-check <subtitle.srt>` 命令：
  - 顺序时间轴检查（ordered）；
  - 非顺序 seek 跳转检查（seek）；
  - 输出 `mismatches` 与 `PASS/FAIL`。
- 新增样例字幕 `samples/subtitle/subtitle_seek_sync_sample.srt` 和本地报告 `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`。
- 任务清单 `1.4.1` 标记完成。

### 修改文件
- include/subtitle/subtitle_timeline.h
- src/subtitle/subtitle_timeline.cpp
- src/core/player_core.cpp
- src/main.cpp
- CMakeLists.txt
- samples/subtitle/subtitle_seek_sync_sample.srt
- samples/README.md
- docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 30: M1 验收 1.4.2（播放列表连续播放 5 文件自检）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.2` 要求“播放列表连续播放 5 文件通过”，但缺少可重复执行的验收命令。
- 仅靠手工点击验证，回归效率低且结果不稳定。

### 原因分析
- 当前主流程具备 EOF 自动切换逻辑，但没有结构化输出可用于快速验收。

### 解决方案
- 在 `main` 新增 `--playlist-flow-check` 命令：
  - 读取输入并构建播放列表；
  - 校验至少 5 条目；
  - 对前 5 条执行可打开探测（`--probe-file` 同源逻辑）；
  - 模拟 EOF 自动切换顺序，验证 `0,1,2,3,4` 连续覆盖；
  - 输出 `PASS/FAIL` 与失败索引。
- 新增本地报告 `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`。
- 更新任务清单，标记 `1.4.2` 完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md
- samples/README.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 31: M1 验收 1.4.3（设置重启恢复自检）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.3` 要求“设置重启后可恢复”，但缺少可重复执行的验收入口。
- 仅手工重启验证难以覆盖关键字段（音量、速度、恢复标志、播放列表索引、快捷键）。

### 原因分析
- 主流程已有 `loadAppSettings/saveAppSettings`，但没有独立命令行输出用于回归验收。

### 解决方案
- 在 `main` 新增 `--settings-persistence-check [settings_file]` 命令：
  - 写入一组测试设置；
  - 重新加载并逐项校验 volume/speed/resume/index/hotkey；
  - 输出 `settings-persistence-check.result=PASS/FAIL`。
- 默认使用系统临时目录进行检查，不污染项目内 `config/player_settings.ini`。
- 新增本地报告 `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`。
- 更新任务清单，标记 `1.4.3` 完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 32: M2 2.1.2（容器矩阵补齐 mov/avi/m2ts）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.2` 目标要求容器覆盖 `mp4/mkv/mov/avi/webm/flv/ts/m2ts`。
- 现有格式回归样本仅覆盖了 `mp4/mkv/webm/flv/ts`，缺少 `mov/avi/m2ts` 实测闭环。

### 原因分析
- 回归样本列表与自动生成脚本未包含 `mov/avi/m2ts` 输出，导致容器矩阵覆盖不完整。

### 解决方案
- 扩展样本矩阵 `format_samples.csv`，新增：
  - `mov`（h264 + aac）
  - `avi`（h264 + mp3）
  - `m2ts`（h264 + ac3）
- 扩展 `tools/download_test_samples.ps1`：
  - 新增 `samples/mov`、`samples/avi`、`samples/m2ts` 目录生成；
  - 新增三类容器样本生成命令。
- 更新样本目录文档与忽略规则，补齐 `.gitkeep`。
- 执行本地回归并更新报告：`docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`。
- 任务清单 `2.1.2` 标记完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- samples/.gitignore
- samples/mov/.gitkeep
- samples/avi/.gitkeep
- samples/m2ts/.gitkeep
- samples/README.md
- docs/FORMAT_REGRESSION.md
- docs/REGRESSION_OPERATION_PLAYBOOK.md
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 33: M2 2.1.3（视频编码矩阵补齐 MPEG-2）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.3` 目标为视频编码支持 `H.264/H.265/VP9/AV1/MPEG-2`。
- 现有回归样本已覆盖前四项，但缺少 `MPEG-2` 视频编码样本，验收闭环不完整。

### 原因分析
- `format_samples.csv` 与 `download_test_samples.ps1` 未包含 `mpeg2video` 样本。

### 解决方案
- 在回归样本矩阵中新增 `mpeg2video` 条目：
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`
- 在样本生成脚本中新增 MPEG-2 样本生成流程：
  - 视频编码 `mpeg2video`；
  - 音频编码 `ac3`；
  - 容器 `mpegts`。
- 运行本地格式回归并更新报告。
- 任务清单 `2.1.3` 标记完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 34: M2 2.1.4（音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.4` 目标要求音频编码覆盖 `AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`。
- 现有回归样本缺少 `E-AC3/DTS/Vorbis/PCM` 实测，验收闭环不完整。

### 原因分析
- 回归样本清单和自动样本生成脚本未覆盖上述四类音频编码。

### 解决方案
- 扩展 `format_samples.csv` 新增四条样本：
  - `h264 + eac3 (mkv)`
  - `h264 + dts (mkv)`
  - `vp9 + vorbis (webm)`
  - `h264 + pcm_s16le (mov)`
- 扩展 `download_test_samples.ps1` 生成流程并修复 DTS 编码：
  - `dca` 编码使用 `-strict -2` 通过实验特性限制。
- 扩展回归脚本兼容等价编码名：
  - `dts` <-> `dca`
  - `hevc` <-> `h265`
  - `pcm` <-> `pcm_*`
- 运行本地回归并更新报告。
- 任务清单 `2.1.4` 标记完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- tools/format_regression/run_format_regression.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 35: M3 3.1.1（DecoderFactory 接入真实初始化流程）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.1.1` 要求 `DecoderFactory` 接入真实解码初始化流程。
- 现有链路中，`DecoderFactory` 未形成统一的“候选后端 -> 逐个尝试 -> 失败回退”主流程。

### 原因分析
- `DecoderFactory` 仅提供“最佳后端”选择，缺少候选序列接口。
- `PlayerCore::initDecoders` 的初始化与回退逻辑耦合在局部条件分支中，不利于统一扩展。

### 解决方案
- `DecoderFactory` 新增 `selectBackendOrder(codec_name, prefer_hardware)`，输出按优先级排序的后端候选序列，并保留软件解码兜底。
- `PlayerCore::initDecoders` 改为按候选序列逐个尝试初始化：
  - 对每个候选后端重建并配置 `AVCodecContext`；
  - 后端配置失败或 `avcodec_open2` 失败时自动切换下一个候选；
  - 成功后统一记录最终解码后端。
- `tryConfigureD3D11HardwareDecode` 调整为纯 D3D11 配置职责，不再在函数内做后端策略决策。
- 任务清单 `3.1.1` 标记完成。

### 修改文件
- include/decoder/decoder_factory.h
- src/decoder/decoder_factory.cpp
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 36: M3 3.1.2（D3D11VA 初始化失败回退软解兜底）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.1.2` 要求 D3D11VA 初始化失败时可靠回退软解。
- 现有逻辑在像素格式协商失败场景下仅日志提示，未显式更新后端状态，存在状态不一致风险。

### 原因分析
- `selectVideoPixelFormat` 在协商不到 D3D11VA 格式时会返回软件格式；
- 但此前没有同步切换 `video_decoder_backend_` 与硬件像素格式状态。

### 解决方案
- 在 `PlayerCore::selectVideoPixelFormat` 中补充显式软解降级：
  - `video_hw_pixel_fmt_ = AV_PIX_FMT_NONE`；
  - `video_decoder_backend_ = Software`。
- 在 `initDecoders` 后端尝试链路中补充“D3D11VA 协商阶段降级软解”日志。
- 更新任务清单，标记 `3.1.2` 完成。

### 修改文件
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
