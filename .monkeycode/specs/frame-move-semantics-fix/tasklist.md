# Frame 移动语义修复实施计划

## 问题描述

VideoFrame 和 AudioFrame 类缺少正确的移动语义实现，导致在 FrameQueue 中移动帧时出现 use-after-free 崩溃。

## 修复方案

为 VideoFrame 和 AudioFrame 类实现正确的移动语义，确保在移动后原对象的 frame_ 指针被置空，避免析构时释放仍被使用的内存。

## 实施状态：已完成

---

- [x] 1. 修复 VideoFrame 移动语义
  - [x] 1.1 在 video_decoder.h 中添加移动构造函数声明
    - 添加 `VideoFrame(VideoFrame&& other) noexcept`
    - 将参数的 `frame_` 指针转移给新对象
    - 将参数的 `frame_` 设为 nullptr

  - [x] 1.2 在 video_decoder.h 中添加移动赋值运算符声明
    - 添加 `VideoFrame& operator=(VideoFrame&& other) noexcept`
    - 如果当前对象有 frame_，先释放
    - 将参数的 `frame_` 指针转移给当前对象
    - 将参数的 `frame_` 设为 nullptr
    - 返回 *this

  - [x] 1.3 在 video_decoder.cpp 中实现移动构造函数
    - 拷贝 other.frame_ 和 other.pts_
    - 将 other.frame_ 设为 nullptr

  - [x] 1.4 在 video_decoder.cpp 中实现移动赋值运算符
    - 先判断是否为自赋值
    - 如果 frame_ 已有值，调用 av_frame_free 释放
    - 拷贝 other.frame_ 和 other.pts_
    - 将 other.frame_ 设为 nullptr
    - 返回 *this

- [x] 2. 修复 AudioFrame 移动语义
  - [x] 2.1 在 audio_decoder.h 中添加移动构造函数声明
    - 添加 `AudioFrame(AudioFrame&& other) noexcept`
    - 转移 frame_、pts_、converted_data_ 等成员

  - [x] 2.2 在 audio_decoder.h 中添加移动赋值运算符声明
    - 添加 `AudioFrame& operator=(AudioFrame&& other) noexcept`
    - 正确转移所有权并释放原有资源

  - [x] 2.3 在 audio_decoder.cpp 中实现移动构造函数和赋值运算符
    - 实现完整的移动语义，包括 converted_data_

- [ ] 3. 编译验证
  - [ ] 3.1 编译项目确保无错误
    - Windows: cmake -B build -G "Visual Studio 17 2022" && cmake --build build
    - 验证编译输出无错误

- [ ] 4. 测试验证
  - [ ] 4.1 运行程序测试播放功能
    - 执行 .\modern-video-player.exe .\a.mp4
    - 验证程序不再崩溃
    - 验证视频和音频正常播放

---

## 额外审查

已审查项目中其他类是否存在类似的移动语义问题：
- VideoPlayer - 使用 unique_ptr 管理，无需修改
- SyncManager - 只包含原子变量，无需修改
- VideoDecodeThread / AudioDecodeThread - 使用 unique_ptr
- AudioPlayer / Display - 只使用原始指针和基本类型

---

## 检查点

- [x] 确保所有 VideoFrame 和 AudioFrame 的移动语义正确实现
- [ ] 确保编译无错误（需用户在本地执行）
- [ ] 确保程序运行不崩溃（需用户在本地执行）
