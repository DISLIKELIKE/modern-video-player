# 开发日志

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
