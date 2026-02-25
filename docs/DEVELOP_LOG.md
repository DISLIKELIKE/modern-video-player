# 开发日志

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
