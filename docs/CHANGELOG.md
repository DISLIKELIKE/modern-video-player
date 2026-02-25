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

---

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

## 相关文档

- [VERSION.md](./VERSION.md) - 版本记录
- [ARCHITECTURE.md](./ARCHITECTURE.md) - 架构设计
- [WINDOWS_SETUP.md](./WINDOWS_SETUP.md) - Windows 配置指南
- [LOGGING.md](./LOGGING.md) - 日志系统说明
