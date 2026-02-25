# 项目版本记录

本文档记录项目的版本变更历史和当前状态。

## 当前版本信息

### 第三方库版本

| 组件 | 版本 | 说明 |
|------|------|------|
| FFmpeg | 8.0.1 | 多媒体处理框架 |
| SDL2 | 2.30.11 | 多媒体库（显示和音频） |
| Quill | 6.0.0 | 异步 Console + Rotating 日志 |
| CMake | 3.15+ | 构建系统 |
| C++ 标准 | C++17 | 编译标准 |

**注意**: Quill 由 `external/quill` 提供，或通过设置 `QUILL_ROOT` 指向系统安装的 v6.0.0+ 版本。

### 项目版本

- **项目版本**: 1.0.0
- **构建类型**: Release / Debug
- **支持平台**: Windows, Linux, macOS

---

## 阶段一：基础播放器 (当前阶段)

### 开始日期
2026-02-17

### 阶段目标
实现基础的视频播放功能，包括：
- 视频解码和显示
- 音频解码和播放
- 基本播放控制

### 完成状态
- [x] 项目结构搭建
- [x] FFmpeg 8.0.1 集成
- [x] SDL2 2.30.11 集成
- [x] 日志系统（Quill 双通道 + Fallback）
- [x] 视频解码模块
- [x] 音频解码模块
- [x] 视频显示模块
- [x] 音频播放模块
- [x] 主播放器逻辑
- [x] 版本兼容性修复
- [x] 视频流索引不匹配问题修复

### 版本兼容性修改

#### FFmpeg 8.0.1 兼容性
- 修复 `codec_ctx_->avctx->priv_data` 在 FFmpeg 8.0 中不可用的问题
- 修改 `video_decoder.cpp` 和 `audio_decoder.cpp`：
  - 在解码器类中添加 `format_ctx_` 成员变量
  - 直接使用传入的 format context 而非从 codec context 获取
- 修改文件：
  - `include/video_decoder.h`
  - `include/audio_decoder.h`
  - `src/video_decoder.cpp`
  - `src/audio_decoder.cpp`

#### 日志系统更新（企业级 Quill 管道）
- 重新启用 Quill，构建 ConsoleSink + RotatingFileSink 异步日志通道，并保持 stdout/stderr 备援。
- `config/logging.conf` 与 `MVP_LOG_*` 环境变量支持运行时调整 `log_dir`、轮转大小/数量及日志等级。
- 初始化失败或目录不可写时自动告警并降级；宏接口与 `DEBUG_MODE` 行为保持兼容。
- 修改文件：`include/logger.h`、`src/logger.cpp`、`config/logging.conf`、`docs/LOGGING.md`、`docs/CHANGELOG.md`、`docs/VERSION.md`
- 编译输出示例：`Quill: enabled`，运行期日志写入 `logs/modern-video-player.log`

#### 多线程播放架构重构 (2026-02-25)
- 实现独立视频解码线程（VideoDecodeThread）
- 实现独立音频解码线程（AudioDecodeThread）
- 实现线程安全帧队列（FrameQueue），支持条件变量等待
- 实现音视频同步管理器（SyncManager），支持 AudioMaster/VideoMaster/Free 三种模式
- 重构 VideoPlayer，从单线程 playLoop 改为多线程 renderLoop
- 新增文件：
  - `include/frame_queue.h`
  - `include/video_decode_thread.h`
  - `include/audio_decode_thread.h`
  - `include/sync_manager.h`
  - `src/video_decode_thread.cpp`
  - `src/audio_decode_thread.cpp`
  - `src/sync_manager.cpp`
- 修改文件：
  - `include/video_player.h`
  - `include/audio_decoder.h` (添加 setConvertedData 方法)
  - `src/video_player.cpp`
  - `CMakeLists.txt`

#### 音频播放架构修复 (2026-02-25)
- 修复音频播放断断续续的问题
- AudioDecodeThread 解码后直接调用 AudioPlayer::play() 将数据放入 SDL 队列
- 确保 SDL 音频回调能持续获取数据
- 修改文件：
  - `include/audio_decode_thread.h`
  - `src/audio_decode_thread.cpp`
  - `src/video_player.cpp`

#### Frame 移动语义修复 (2026-02-25)
- 修复 VideoFrame 和 AudioFrame 类缺少移动语义导致的崩溃问题
- 为 VideoFrame 添加移动构造函数和移动赋值运算符
- 为 AudioFrame 添加移动构造函数和移动赋值运算符
- 显式删除拷贝构造函数和拷贝赋值运算符，防止浅拷贝
- 修改文件：
  - `include/video_decoder.h`
  - `src/video_decoder.cpp`
  - `include/audio_decoder.h`
  - `src/audio_decoder.cpp`

#### 多解码器实例竞争读取修复 (2026-02-25)
- 修复播放时多个解码器竞争读取同一 AVFormatContext 导致的解码错误
- 在 play() 前重置 video_decoder_ 和 audio_decoder_，避免竞争
- 修改文件：
  - `src/video_player.cpp`

### 已知问题
- 音视频同步使用简单的时间计算，高速或低速播放时可能有同步问题
- 仅支持 YUV420P 格式的视频
- seek 功能需要进一步完善

### 问题修复记录

#### 视频流索引不匹配问题 (2026-02-24)

**问题描述**:
- 播放 mp4 文件时，视频无法正常显示
- 日志显示 `stream_index=0, expected=1` 反复出现
- 程序循环 48 次才能读到正确的视频帧

**问题原因**:
- MP4 文件的流顺序是音频流(索引0)在视频流(索引1)之前
- `av_read_frame()` 返回的包可能是任意流的（通常是第一个流 - 音频流）
- 原代码遇到不匹配的流时直接返回 false，导致视频帧无法解码

**解决方案**:
- 修改 `src/video_decoder.cpp` 的 `decodeFrame()` 方法
- 将遇到不匹配流时返回 false，改为 continue 跳过该包
- 循环读取直到找到正确流索引的包

**修改文件**:
- `src/video_decoder.cpp`

**日志示例**:
```
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=0, expected=1
[DEBUG] [VIDEO] decodeFrame: packet stream mismatch, skipping
... (重复 48 次)
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=1, expected=1
[DEBUG] [VIDEO] decodeFrame: success, pts=0
```

#### 音频流索引不匹配问题 (2026-02-24)

**问题描述**:
- 与视频流索引相同的问题，音频无法正常解码

**问题原因**:
- 同样的问题：音频包可能不是第一个被读取的流

**解决方案**:
- 修改 `src/audio_decoder.cpp` 的 `decodeFrame()` 方法
- 应用与视频解码器相同的修复

**修改文件**:
- `src/audio_decoder.cpp`

#### YUV 数据渲染错误 (2026-02-24)

**问题描述**:
- 解码成功后程序立即退出，没有画面显示

**问题原因**:
- `renderFrame` 函数使用错误的 YUV 数据
- 原来传递的是 `frame->data[0]`（只是 Y 平面指针）
- 然后错误地假设 Y/U/V 是连续存储的

**解决方案**:
1. 传递整个 AVFrame 指针而不是 `data[0]`
2. 正确使用 Y/U/V 平面的数据和行大小

**修改文件**:
- `src/display.cpp`
- `src/video_player.cpp`

**代码变更**:
```cpp
// video_player.cpp
display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);

// display.cpp
AVFrame* frame = (AVFrame*)data;
SDL_UpdateYUVTexture(
    texture_, nullptr,
    frame->data[0], frame->linesize[0],
    frame->data[1], frame->linesize[1],
    frame->data[2], frame->linesize[2]
);
```

### 下一步计划
- [ ] 完善音视频同步机制
- [ ] 添加更多的播放控制（快进、快退）
- [ ] 支持更多的视频格式
- [ ] 添加字幕支持
- [ ] 优化性能

---

## 版本历史

### v1.0.0 (第一阶段)
- 初始版本
- 基于 FFmpeg + SDL2 + Quill 构建
- 支持基本的视频播放功能

---

## 依赖库安装说明

### Windows

#### FFmpeg 8.0.1
```powershell
# 使用 vcpkg
vcpkg install ffmpeg:x64-windows

# 或手动下载
# 访问 https://www.gyan.dev/ffmpeg/builds/
# 下载 ffmpeg-8.0.1-full_build.7z
```

#### SDL2 2.30.11
```powershell
# 使用 vcpkg
vcpkg install sdl2:x64-windows

# 或手动下载
# 访问 https://github.com/libsdl-org/SDL/releases
# 下载 SDL2-devel-2.30.11-VC.zip
```

**注意**: 若未单独安装 Quill，可将源码解压到 `external/quill`；也可以通过包管理器安装后设置 `QUILL_ROOT`。

### Linux

```bash
# 编译 FFmpeg 8.0.1
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install

# SDL2 通常通过包管理器安装
sudo apt install libsdl2-dev
```

### macOS

```bash
# 使用 Homebrew
brew install ffmpeg
brew install sdl2
```

**注意**: Quill 为 header-only 库，可通过 `brew` 安装或直接下载源码后配置 `QUILL_ROOT`。

---

## 编译命令

### Windows (Visual Studio)
```powershell
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
cmake --build . --config Release
```

### Linux / macOS
```bash
mkdir build
cd build
cmake ..
make -j$(nproc)
```

---

## 文档更新日志

| 日期 | 更新内容 |
|------|----------|
| 2026-02-17 | 创建版本记录文档，记录第一阶段完成情况 |
| 2026-02-24 | 更新日志系统为企业级 Quill 管道，记录视频流索引问题修复 |
| 2026-02-24 | 记录音频流索引问题和 YUV 渲染问题修复 |
| 2026-02-24 | 创建 CHANGELOG.md 问题修复记录文档 |
| 2026-02-25 | 记录多线程播放架构重构和音频播放架构修复 |
| 2026-02-25 | 记录 Frame 移动语义修复 |
| 2026-02-25 | 记录多解码器实例竞争读取修复 |
