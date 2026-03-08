# 视频流索引不匹配问题分析

## 状态说明（2026-03-08）

- 本文档归档的是 `2026-02-24` 早期原型阶段的视频流索引问题分析。
- 文中的 `playLoop`、`src/video_decoder.cpp` 等路径和日志，反映的是当时的单体/旧解码器实现，不代表当前仓库主链结构。
- 当前播放器主链已收敛到 `VideoPlayer -> PlayerCore -> Scheduler -> core/*`；本文件应作为历史问题分析样本阅读。

## 问题描述

### 现象

运行视频播放器时，程序初始化成功但无法正常播放视频，日志显示：

```
[DEBUG] [LOOP] playLoop started, stopped=0, display=valid, shouldQuit=0
[DEBUG] [LOOP] Loop iteration 0, shouldQuit=0
[DEBUG] [VIDEO] Calling decodeFrame...
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=0, expected=1
[DEBUG] [VIDEO] decodeFrame: packet stream mismatch, skipping
[DEBUG] [VIDEO] decodeFrame failed or frame invalid
[DEBUG] [LOOP] Loop iteration 1, shouldQuit=0
[DEBUG] [VIDEO] Calling decodeFrame...
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=0, expected=1
[DEBUG] [VIDEO] decodeFrame: packet stream mismatch, skipping
... (重复数十次)
[DEBUG] [LOOP] Loop iteration 48, shouldQuit=0
[DEBUG] [VIDEO] Calling decodeFrame...
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=1, expected=1
[DEBUG] [VIDEO] decodeFrame: success, pts=0
```

### 初始化日志

```
Video decoder opened: 1920x1080, format: yuv420p
Display initialized: 1920x1080
Audio decoder opened: 48000Hz, 2 channels, fltp
Audio player initialized: 48000Hz, 2 channels
Opened file: .\a.mp4
Duration: 1492.7 seconds
Video: 1920x1080
Audio: 48000Hz, 2 channels
```

## 问题分析

### 根本原因

MP4 文件的流顺序是：**音频流(索引 0) 在前，视频流(索引 1) 在后**。

当调用 `av_read_frame()` 读取数据包时：

1. 第一次读取返回的是音频包（stream_index=0）
2. 视频解码器期望的是视频流（expected=1）
3. 原代码检查到流索引不匹配后，直接返回 `false`，导致解码失败
4. 循环需要跳过所有音频包（约 48 个）才能读到第一个视频包

### 代码分析

原始代码在 `src/video_decoder.cpp` 中：

```cpp
bool VideoDecoder::decodeFrame(VideoFrame& frame) {
    // ...
    
    int ret = av_read_frame(format_ctx_, packet);
    
    if (ret < 0) {
        av_packet_free(&packet);
        return false;
    }
    
    LOG_TRACE_VIDEO("decodeFrame: read packet, stream_index=" << packet->stream_index << ", expected=" << stream_idx_);
    
    if (packet->stream_index != stream_idx_) {
        LOG_TRACE_VIDEO("decodeFrame: packet stream mismatch, skipping");
        av_packet_unref(packet);
        av_packet_free(&packet);
        return false;  // ← 问题：直接返回，导致无法继续读取视频包
    }
    
    // ... 解码逻辑
}
```

### 问题影响

1. **播放卡顿**：需要循环数十次才能解码一帧
2. **资源浪费**：每次循环都分配和释放 AVPacket
3. **音频包堆积**：音频包被跳过但未被正确处理
4. **潜在闪退**：长时间无法解码可能导致程序异常

## 解决方案

### 修复方案

将遇到不匹配流时直接返回，改为循环读取直到找到正确的流：

```cpp
bool VideoDecoder::decodeFrame(VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!codec_ctx_ || !format_ctx_) {
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return false;
    }
    
    int ret;
    
    // 循环读取直到找到正确流索引的包
    while (true) {
        ret = av_read_frame(format_ctx_, packet);
        
        if (ret < 0) {
            LOG_TRACE_VIDEO("decodeFrame: av_read_frame failed, ret=" << ret);
            av_packet_free(&packet);
            return false;
        }
        
        LOG_TRACE_VIDEO("decodeFrame: read packet, stream_index=" << packet->stream_index << ", expected=" << stream_idx_);
        
        // 不匹配的流：跳过并继续读取
        if (packet->stream_index != stream_idx_) {
            LOG_TRACE_VIDEO("decodeFrame: packet stream mismatch, skipping");
            av_packet_unref(packet);
            continue;  // ← 修复：继续循环读取
        }
        
        break;  // 找到正确的流
    }
    
    ret = avcodec_send_packet(codec_ctx_, packet);
    av_packet_unref(packet);
    av_packet_free(&packet);
    
    // ... 后续解码逻辑
}
```

### 修改文件

- `src/video_decoder.cpp`

## 调试方法

### 启用调试日志

```bash
cmake -B build -DDEBUG_MODE=ON
cmake --build build
```

### 运行程序

```bash
.\build\Release\modern-video-player.exe your_video.mp4
```

### 分析日志

根据日志输出判断问题：

| 日志信息 | 问题原因 |
|---------|---------|
| `stream_index=0, expected=1` | 音频包被错误跳过 |
| `decodeFrame: success` | 解码成功 |
| `packet stream mismatch` | 流索引不匹配 |

### 常见问题排查

1. **问题：所有包都被跳过**
   - 检查视频流索引是否正确
   - 确认 format_ctx 有效性

2. **问题：解码成功但画面静止**
   - 检查 SDL 渲染逻辑
   - 确认纹理格式匹配

3. **问题：程序闪退**
   - 检查 shouldQuit() 状态
   - 确认事件处理逻辑

## 预防措施

### 1. 添加更详细的日志

在关键位置添加日志，便于定位问题：

```cpp
LOG_TRACE_VIDEO("decodeFrame: packet stream_index=" << packet->stream_index 
                << ", expected=" << stream_idx_);
```

### 2. 流类型检查

在打开解码器时验证流类型：

```cpp
AVStream* stream = fmt_ctx->streams[stream_idx];
if (stream->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
    LOG_ERROR("Stream " << stream_idx << " is not video!");
    return false;
}
```

### 3. 初始化事件处理

在播放循环开始前清空事件队列：

```cpp
void VideoPlayer::playLoop() {
    if (display_) {
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_QUIT);
        SDL_FlushEvent(SDL_WINDOWEVENT);
    }
    // ... 原有代码
}
```

## 相关文档

- [播放循环问题调试](./playback-loop-exit-issue.md)
- [日志系统说明](./LOGGING.md)
- [ARCHITECTURE.md](./ARCHITECTURE.md)
