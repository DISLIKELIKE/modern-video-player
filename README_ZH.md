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
- **音视频库**: FFmpeg 8.0.1
- **显示/音频库**: SDL2 2.30.11
- **日志库**: Quill 6.0.0
- **构建工具**: CMake 3.15+
- **编译器**: 
  - Windows: MSVC 2017+ / MinGW-w64
  - Linux: GCC 9+
  - macOS: Clang 10+

## 项目结构

```
modern-video-player/
├── CMakeLists.txt                    # CMake 构建配置
├── README.md                         # 英文项目说明
├── README_ZH.md                      # 中文项目说明
├── LICENSE                           # MIT 许可证
├── include/                          # 对外头文件
│   ├── core/                         # PlayerCore / Scheduler / Clock / FrameQueue
│   ├── decoder/                      # 解码器工厂与后端选择
│   ├── filters/                      # 音视频滤镜接口与注册表
│   ├── input/                        # 快捷键处理
│   ├── playlist/                     # 播放列表管理
│   ├── render/                       # SDL / D3D11 / OpenGL 渲染接口
│   ├── subtitle/                     # 字幕管线与加载器
│   ├── audio_player.h                # 音频输出
│   ├── demuxer.h                     # 解封装入口
│   ├── display.h                     # 窗口与呈现桥接
│   └── video_player.h                # 播放器 Facade
├── src/                              # 源文件
│   ├── core/                         # 核心调度与播放状态机
│   ├── decoder/                      # 解码实现
│   ├── filters/                      # 内置滤镜
│   ├── input/                        # 快捷键实现
│   ├── playlist/                     # 播放列表实现
│   ├── render/                       # 渲染后端
│   ├── subtitle/                     # 字幕实现
│   ├── main.cpp                      # 主程序入口
│   ├── demuxer.cpp                   # 解封装实现
│   ├── display.cpp                   # 呈现桥接实现
│   └── video_player.cpp              # Facade 实现
└── docs/                             # 文档
    ├── README.md                     # 文档索引
    ├── guides/                       # 环境配置 / 使用 / 入门
    ├── design/                       # 架构 / 设计说明 / 草案
    ├── analysis/                     # 差距评估 / 巡检 / 问题分析
    ├── reference/                    # 外部参考 / 学习资料
    ├── plans/                        # 路线图 / 任务清单 / TODO
    ├── workflows/                    # 操作手册 / 清单 / 工作流
    ├── records/                      # 版本 / 修复 / 开发记录
    └── reports/                      # 本地验收报告
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
| Enter / Alt+Enter / F | 全屏切换 |
| ESC | 退出全屏（窗口态时退出播放器） |
| Q | 退出播放器 |
| 左 / 右 | 快退 / 快进 5 秒 |
| Ctrl+左 / Ctrl+右 | 快退 / 快进 30 秒 |
| 上 / 下 / +/- | 音量 +5 / -5 |
| M | 静音 / 取消静音 |
| [ / ] / R | 降速 / 升速 / 恢复 1.0x |
| PageUp / PageDown | 上一项 / 下一项 |
| Home / End | 上一章 / 下一章 |
| A / B / C | 设置 A 点 / 设置 B 点 / 清除 A-B Repeat |
| S | 保存截图 |
| , / . | 暂停态单帧后退 / 前进 |
| J / K | 字幕延迟 -/+100ms |
| Ctrl+J / Ctrl+K | 音轨延迟 -/+100ms |
| 1..9 | 跳转到 10%..90% 位置 |
| V | 字幕开关 |

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
│        VideoPlayer（Facade）          │
│  ┌─────────────────────────────────┐    │
│  │      PlayerCore                 │    │
│  │  - 播放状态机                  │    │
│  │  - 命令 / seek / 截图           │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │      Scheduler                  │    │
│  │  - 解码 / 渲染线程              │    │
│  │  - 音视频同步与背压控制         │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │ Demuxer + DecoderFactory        │    │
│  │  - 流选择                       │    │
│  │  - 软解 / D3D11VA 后端协商      │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │ Display / Render Backends       │    │
│  │ SDL / D3D11 / OpenGL            │    │
│  └─────────────────────────────────┘    │
│  ┌─────────────────────────────────┐    │
│  │ 播放列表 / 字幕 / 滤镜 / 设置   │    │
│  │ 快捷键 / 报告 / 配置            │    │
│  └─────────────────────────────────┘    │
└─────────────────────────────────────────┘
```

- 当前主链为 `VideoPlayer -> PlayerCore -> Scheduler -> core/*`。
- 旧版 decoder / thread 架构仅作为历史说明保留在旧文档中；当前实现请以 `docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md` 为准。

## 文档

- [文档索引](docs/README.md) - 按分类浏览所有文档的统一入口
- [实现步骤指南](docs/guides/IMPLEMENTATION.md) - 早期原型的从零实现教程
- [架构设计文档](docs/design/ARCHITECTURE.md) - 历史架构基线与设计背景
- [架构重构说明](docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md) - 当前主链重构说明
- [Windows 配置指南](docs/guides/WINDOWS_SETUP.md) - Windows 环境配置详情
- [版本记录](docs/records/VERSION.md) - 项目版本和依赖信息

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

**解决方案**:
- 如果使用 `vcpkg`，请在配置时传入正确的 toolchain：
```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]/scripts/buildsystems/vcpkg.cmake
```
- 如果使用仓库内手动依赖布局，请将 FFmpeg 放在 `external/ffmpeg/` 下，让当前 `CMakeLists.txt` 自动探测。

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
