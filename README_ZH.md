# Modern Video Player

一个使用 FFmpeg 和 SDL2 实现的现代化视频播放器，采用 C++17 标准，支持 Windows、Linux 和 macOS。

## 功能特性

- 视频解码和显示
- 音频解码和播放
- 音视频同步
- 播放控制（播放/暂停/停止）
- 全屏切换
- 音量控制
- 播放速度调节

## 技术栈

- **编程语言**: C++17
- **音视频库**: FFmpeg 6.x / 7.x
- **显示/音频库**: SDL2
- **日志库**: Quill (可选）
- **构建工具**: CMake 3.15+
- **编译器**: 
  - Windows: MSVC 2017+ / MinGW-w64
  - Linux: GCC 9+
  - macOS: Clang 10+

## 项目结构

```
modern-video-player/
├── CMakeLists.txt          # CMake 构建配置
├── README.md               # 英文项目说明
├── README_ZH.md            # 中文项目说明
├── LICENSE                 # MIT 许可证
├── include/                # 头文件
│   ├── video_player.h     # 主播放器类
│   ├── video_decoder.h    # 视频解码器
│   ├── audio_decoder.h    # 音频解码器
│   ├── display.h          # 显示窗口
│   ├── audio_player.h     # 音频播放器
│   └── logger.h           # 日志系统
├── src/                    # 源文件
│   ├── main.cpp           # 主程序入口
│   ├── video_player.cpp   # 播放器实现
│   ├── video_decoder.cpp  # 视频解码实现
│   ├── audio_decoder.cpp  # 音频解码实现
│   ├── display.cpp        # 显示实现
│   ├── audio_player.cpp   # 音频播放实现
│   └── logger.cpp         # 日志实现
└── docs/                   # 文档
    ├── WINDOWS_SETUP.md    # Windows 配置指南
    ├── IMPLEMENTATION.md   # 实现步骤文档
    └── ARCHITECTURE.md     # 架构设计文档
```

## 安装

### Windows

#### 方法一：使用 vcpkg (推荐）

1. 安装 [vcpkg](https://github.com/microsoft/vcpkg)
2. 安装依赖：
```powershell
vcpkg install sdl2 ffmpeg quill:x64-windows
```

3. 配置和编译：
```powershell
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg root]/scripts/buildsystems/vcpkg.cmake
cmake --build . --config Release
```

#### 方法二：手动安装

1. 下载并安装 FFmpeg:
   - 从 [gyan.dev](https://www.gyan.dev/ffmpeg/builds/) 下载
   - 解压到 `external/ffmpeg/`

2. 下载并安装 SDL2:
   - 从 [SDL2 Releases](https://github.com/libsdl-org/SDL/releases) 下载
   - 解压到 `external/SDL2/`

3. 下载并安装 Quill (可选）:
   - 从 [Quill Releases](https://github.com/odygrd/quill/releases) 下载
   - 解压到 `external/quill/`

4. 编译：
```powershell
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Linux

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libavformat-dev \
    libavcodec-dev \
    libswscale-dev \
    libswresample-dev \
    libavutil-dev \
    libsdl2-dev

# Quill 可选
git clone https://github.com/odygrd/quill.git external/quill
cd external/quill
mkdir build && cd build
cmake .. && make && sudo make install

cd ../../
mkdir build && cd build
cmake ..
make
```

### macOS

```bash
brew install ffmpeg sdl2 cmake pkg-config

# Quill 可选
brew install quill

mkdir build && cd build
cmake ..
make
```

## 使用方法

```bash
# Windows
build\Release\modern-video-player.exe your_video.mp4

# Linux/macOS
./build/modern-video-player your_video.mp4
```

## 快捷键

| 按键 | 功能 |
|------|------|
| 空格 | 播放/暂停 |
| Q / ESC | 退出 |
| F | 全屏切换 |

## 技术亮点

### C++17 新特性

- `std::unique_ptr` - 自动资源管理
- `std::atomic` - 线程安全的原子操作
- `std::thread` - 多线程支持
- `std::mutex` - 互斥锁保护共享资源
- `std::condition_variable` - 线程同步

### FFmpeg 核心概念

1. **AVFormatContext**: 媒体文件上下文，包含所有流信息
2. **AVCodecContext**: 编解码器上下文，包含解码器配置
3. **AVPacket**: 压缩的数据包
4. **AVFrame**: 解码后的原始数据（视频 YUV，音频 PCM）
5. **SwsContext**: 视频格式转换上下文
6. **SwrContext**: 音频格式转换上下文

### SDL2 核心概念

1. **SDL_Window**: 窗口对象，用于显示视频
2. **SDL_Renderer**: 渲染器，用于绘制到窗口
3. **SDL_Texture**: 纹理，用于存储视频帧数据
4. **SDL_AudioDevice**: 音频设备，用于播放音频

### 日志系统

项目集成了 Quill 日志库，提供：

- 高性能异步日志
- 文件和控制台输出
- 日志级别控制（DEBUG, INFO, WARNING, ERROR）
- 日志文件自动轮转
- 线程安全

如果未安装 Quill，会自动降级使用 `std::cout/std::cerr`。

## 架构

```
┌─────────────────────────────────────────┐
│           VideoPlayer                   │
│  ┌─────────────────────────────────┐    │
│  │      VideoDecoder              │    │
│  │  - 解码视频帧                  │    │
│  │  - 格式转换 (YUV)              │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      AudioDecoder              │    │
│  │  - 解码音频帧                  │    │
│  │  - 格式转换 (S16)              │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Display                    │    │
│  │  - SDL 窗口管理                │    │
│  │  - YUV 纹理渲染                │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      AudioPlayer                │    │
│  │  - SDL 音频播放                │    │
│  │  - 音量控制                     │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Logger                     │    │
│  │  - Quill 日志系统              │    │
│  │  - 异步日志记录                │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Play Thread                │    │
│  │  - 音视频同步                  │    │
│  │  - 帧率控制                     │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

## 文档

- [实现步骤指南](docs/IMPLEMENTATION.md) - 详细的分步实现指南
- [架构设计文档](docs/ARCHITECTURE.md) - 系统架构和设计模式
- [Windows 配置指南](docs/WINDOWS_SETUP.md) - Windows 环境配置详情

## 学习路线

### 基础知识
1. C++17 新特性
2. 多线程编程
3. FFmpeg 基础
4. SDL2 基础
5. 日志系统设计

### 进阶内容
1. 音视频同步算法
2. 播放器架构设计
3. 性能优化
4. 错误处理
5. 异步日志

### 扩展方向
1. 播放列表支持
2. 字幕支持
3. 倍速播放
4. 播放历史记录
5. 硬件加速

## 故障排除

### Windows

**问题**: SDL2.dll 未找到

**解决方案**: 确保 SDL2.dll 在可执行文件同一目录中。

**问题**: FFmpeg 库未找到

**解决方案**: 设置 `FFMPEG_DIR` 指向您的 FFmpeg 安装目录：
```bash
cmake .. -DFFMPEG_DIR=path/to/ffmpeg
```

### Linux

**问题**: pkg-config 未找到

**解决方案**:
```bash
sudo apt-get install pkg-config
```

## 性能优化

### 1. Release 编译

```powershell
cmake --build . --config Release
```

### 2. 日志级别控制

在运行时设置日志级别：
```cpp
Logger::init();
// 调试信息会输出到控制台和文件
```

## 贡献

欢迎贡献！请随时提交 Issue 和 Pull Request。

## 许可证

MIT License

## 致谢

- [FFmpeg](https://ffmpeg.org/) - 完整的跨平台音视频解决方案
- [SDL2](https://wiki.libsdl.org/) - 简单的 DirectMedia 层
- [Quill](https://github.com/odygrd/quill) - 高性能 C++ 异步日志库

## 作者

作为一个现代化的 C++17 视频播放器实现，用于学习和教育目的。
