# 项目版本记录

本文档记录项目的版本变更历史和当前状态。

## 当前版本信息

### 第三方库版本

| 组件 | 版本 | 说明 |
|------|------|------|
| FFmpeg | 8.0.1 | 多媒体处理框架 |
| SDL2 | 2.30.11 | 多媒体库（显示和音频） |
| Quill | v6.x | 异步日志库 |
| CMake | 3.15+ | 构建系统 |
| C++ 标准 | C++17 | 编译标准 |

**注意**: Quill 版本为 v6.x 系列，API 与 6.0.0 有差异。

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
- [x] Quill 6.0.0 日志集成
- [x] 视频解码模块
- [x] 音频解码模块
- [x] 视频显示模块
- [x] 音频播放模块
- [x] 主播放器逻辑
- [x] 版本兼容性修复

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

#### Quill 6.0.0 API 更新
- 更新 `quill::start()` 调用方式
- 日志宏格式变更：`LOG_INFO(msg)` → `LOG_INFO("{}", msg)`
- `quill::flush()` → `quill::flush_log()`
- Handler 创建方式更新
- 修改文件：
  - `src/logger.cpp`

### 已知问题
- 音视频同步使用简单的时间计算，高速或低速播放时可能有同步问题
- 仅支持 YUV420P 格式的视频
- seek 功能需要进一步完善

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

#### Quill 6.0.0
```powershell
# 使用 vcpkg
vcpkg install quill:x64-windows

# 或手动下载
# 访问 https://github.com/odygrd/quill/releases
# 下载 quill-6.0.0.zip
```

### Linux

```bash
# 编译 FFmpeg 8.0.1
./configure --prefix=/usr/local
make -j$(nproc)
sudo make install

# SDL2 通常通过包管理器安装
sudo apt install libsdl2-dev

# Quill 从源码编译
git clone https://github.com/odygrd/quill.git
cd quill && mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### macOS

```bash
# 使用 Homebrew
brew install ffmpeg
brew install sdl2
brew install quill
```

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
