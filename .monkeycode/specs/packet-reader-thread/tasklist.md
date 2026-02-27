# 需求实施计划: 统一 Packet 读取器解决并发解码冲突

## 问题背景

当前视频播放器在播放时出现大量解码错误：
- `Invalid NAL unit size`
- `missing picture in access unit`
- `Error splitting the input into NAL units`
- 最终导致访问冲突崩溃

根本原因：视频解码线程和音频解码线程各自拥有独立的解码器实例，但共享同一个 `AVFormatContext`，并发调用 `av_read_frame()` 导致数据竞争。

---

- [ ] 1. 创建 Packet 结构体和 PacketQueue 类
  - [ ] 1.1 定义 PacketRef 结构体包装 AVPacket
    - 在 `include/` 目录下创建 `packet_reader.h`
    - 定义 `PacketRef` 结构体，包含 `AVPacket*` 指针和流索引
    - 实现移动语义，确保 AVPacket 资源正确转移

  - [ ] 1.2 实现 PacketQueue 队列类
    - 在 `packet_reader.h` 中定义 `PacketQueue` 模板类
    - 实现线程安全的 push/pop 操作
    - 支持带超时的等待机制

- [ ] 2. 实现 PacketReaderThread 类
  - [ ] 2.1 定义 PacketReaderThread 类接口
    - 在 `include/packet_reader.h` 中定义类接口
    - 包含视频和音频两个 PacketQueue 输出队列
    - 包含 start/stop/pause/resume/flush 控制方法

  - [ ] 2.2 实现 PacketReaderThread 核心逻辑
    - 创建 `src/packet_reader.cpp`
    - 实现统一的 `av_read_frame` 调用点
    - 根据 stream_index 将 packet 分发到对应队列
    - 处理 EOF 和错误情况

- [ ] 3. 修改 VideoDecoder 类
  - [ ] 3.1 移除 VideoDecoder 中的 av_read_frame 调用
    - 修改 `decodeFrame()` 方法签名，接受 `AVPacket*` 参数
    - 从外部传入 packet，而不是内部读取

  - [ ] 3.2 更新 VideoDecoder 接口
    - 修改 `include/video_decoder.h` 中的方法声明
    - 移除 `format_ctx_` 成员变量（解码器不再需要读取文件）

- [ ] 4. 修改 AudioDecoder 类
  - [ ] 4.1 移除 AudioDecoder 中的 av_read_frame 调用
    - 修改 `decodeFrame()` 方法签名，接受 `AVPacket*` 参数
    - 从外部传入 packet，而不是内部读取

  - [ ] 4.2 更新 AudioDecoder 接口
    - 修改 `include/audio_decoder.h` 中的方法声明
    - 移除 `format_ctx_` 成员变量

- [ ] 5. 重构 VideoDecodeThread
  - [ ] 5.1 修改 VideoDecodeThread 使用 PacketQueue
    - 修改 `include/video_decode_thread.h`，添加 `PacketQueue*` 参数
    - 修改 `start()` 方法签名，接受 `PacketQueue` 输入

  - [ ] 5.2 更新 VideoDecodeThread 解码循环
    - 修改 `src/video_decode_thread.cpp` 的 `decodeLoop()`
    - 从 PacketQueue 获取 packet，传递给 VideoDecoder

- [ ] 6. 重构 AudioDecodeThread
  - [ ] 6.1 修改 AudioDecodeThread 使用 PacketQueue
    - 修改 `include/audio_decode_thread.h`，添加 `PacketQueue*` 参数
    - 修改 `start()` 方法签名

  - [ ] 6.2 更新 AudioDecodeThread 解码循环
    - 修改 `src/audio_decode_thread.cpp` 的 `decodeLoop()`
    - 从 PacketQueue 获取 packet，传递给 AudioDecoder

- [ ] 7. 重构 VideoPlayer 整合所有组件
  - [ ] 7.1 添加 PacketReaderThread 成员
    - 修改 `include/video_player.h`，添加 `PacketReaderThread` 成员
    - 添加视频和音频 `PacketQueue` 成员

  - [ ] 7.2 修改 play() 方法启动顺序
    - 修改 `src/video_player.cpp` 的 `play()` 方法
    - 先启动 PacketReaderThread
    - 再启动 VideoDecodeThread 和 AudioDecodeThread
    - 确保 PacketQueue 正确传递给解码线程

  - [ ] 7.3 修改 stop() 和 close() 方法
    - 确保 PacketReaderThread 正确停止
    - 确保所有 PacketQueue 正确清理

- [ ] 8. 检查点 - 确保核心功能正常
  - 编译项目确保无错误
  - 测试播放视频文件，验证音频和视频正常播放
  - 确认不再出现 "Invalid NAL unit size" 错误

- [ ] 9. 更新文档
  - [ ] 9.1 更新 ARCHITECTURE.md
    - 添加 PacketReaderThread 架构说明
    - 更新线程模型图

  - [ ] 9.2 更新 CHANGELOG.md
    - 添加问题 11: 统一 Packet 读取解决并发冲突

  - [ ] 9.3 更新 DEVELOP_LOG.md
    - 记录问题分析和解决过程
