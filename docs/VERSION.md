# 项目版本记录

本文档记录项目的版本变更历史和当前状态。

## 当前版本信息

### 第三方库版本

| 组件 | 版本 | 说明 |
|------|------|------|
| FFmpeg | 8.0.1 | 多媒体处理框架 |
| SDL2 | 2.30.11 | 多媒体库（显示和音频） |
| Quill | 已禁用 | 使用 std::cout 日志替代 |
| CMake | 3.15+ | 构建系统 |
| C++ 标准 | C++17 | 编译标准 |

**注意**: Quill 日志库已被禁用，改用标准 std::cout 进行日志输出。

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
- [x] 日志系统（使用 std::cout）
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

#### 日志系统更新 (Quill 已被禁用)
- 由于 Quill v6.x API 不兼容，项目暂时禁用 Quill
- 使用 `Logger` 类替代，内部实现为标准 std::cout/cerr
- 修改文件：
  - `src/logger.cpp`
- 编译时会输出：`Quill found: disabled (using std::cout)`

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

**注意**: 不需要安装 Quill 日志库，项目使用 std::cout 替代。

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

**注意**: 不需要安装 Quill 日志库。

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
| 2026-02-24 | 更新 Quill 禁用状态，记录视频流索引问题修复 |
