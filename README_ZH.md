# Modern Video Player

基于 FFmpeg、SDL2 和 C++17 的桌面视频播放器。当前源码主线已经不是早期教学 demo，而是带有跨平台策略、多个渲染后端、硬解回退、字幕、回归 CLI 和打包脚本的播放器工程。

当前版本配置见 `CMakeLists.txt`：项目版本 `1.0.0`，默认预发布后缀为 `rc2`。

## 当前能力

- 本地媒体播放：打开、播放、暂停、停止、seek、音量、静音、倍速。
- 播放交互：全屏、章节跳转、A-B Repeat、帧步进、截图、数字键百分比跳转。
- 播放列表：多文件参数、本地 `m3u8` 列表、上一项/下一项、EOF 自动下一项、上次索引恢复。
- 字幕：外挂 SRT/ASS/SSA、同名字幕探测、内嵌字幕轨列表/选择、语言/forced/SDH 策略、PGS/DVD bitmap subtitle、attachment font 注册与清理。
- 渲染后端：Software SDL、D3D11、OpenGL、Vulkan。是否编译由 CMake feature switch 和平台依赖共同决定。
- 解码后端：Software、D3D11VA、VAAPI、VideoToolbox 等策略枚举；实际可用性取决于平台、构建开关和运行时能力。
- 平台策略：`platform_capabilities` + `playback_strategy` 生成 renderer/decoder 候选链，`PlayerCore` 只消费策略并执行 fallback。
- 诊断和回归：媒体 probe、性能日志、D3D11/OpenGL/Vulkan diagnostics、Linux gate、Windows gate、格式回归、字幕/截图/插件/流媒体基础检查。
- 扩展基础：滤镜管线、插件 API、HTTP/HLS/DASH/ABR 基础设施、设置持久化、热键管理。

## 当前边界

- Windows 和 Linux 是当前主要维护目标。
- macOS 只保留构建/策略预留，尚未作为完整交付平台收口。
- 流媒体模块已经有下载、manifest 和 ABR 验证，但还不是完整在线播放主链。
- 插件系统已有加载和示例插件，但没有用户级插件管理 UI。
- 皮肤系统仍是轻量主题骨架。

## 构建依赖

| 平台 | 主要依赖 |
| --- | --- |
| Windows | FFmpeg、SDL2、可选 Quill、可选 Vulkan SDK |
| Linux | FFmpeg、SDL2、OpenGL、libass、fontconfig、freetype、可选 Vulkan/VAAPI |
| macOS | FFmpeg、SDL2、OpenGL/VideoToolbox 预留，未完成交付闭环 |

常用 CMake 开关：

```text
ENABLE_D3D11_RENDERER
ENABLE_OPENGL_RENDERER
ENABLE_SDL_RENDERER
ENABLE_VULKAN_RENDERER
ENABLE_D3D11VA
ENABLE_DXVA2
ENABLE_VAAPI
ENABLE_VIDEOTOOLBOX
```

## 构建

Windows：

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target modern-video-player sample_logger_plugin
```

Linux：

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Windows 打包：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release
```

Linux 打包：

```bash
./tools/package_linux.sh
```

## 使用

播放文件：

```powershell
.\build\Release\modern-video-player.exe .\juren-30s.mp4
```

外挂字幕：

```powershell
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle .\movie.ass
```

指定内嵌字幕策略：

```powershell
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle-language zh,en --subtitle-forced auto --subtitle-sdh avoid
```

## 常用快捷键

| 按键 | 功能 |
| --- | --- |
| Space | 播放/暂停 |
| Enter / Alt+Enter / F | 全屏切换 |
| Esc / Q | 退出全屏或退出播放器 |
| Left / Right | 快退/快进 5 秒 |
| Ctrl+Left / Ctrl+Right | 快退/快进 30 秒 |
| Up / Down / +/- | 音量调整 |
| M | 静音 |
| [ / ] / R | 降速/升速/恢复 1.0x |
| PageUp / PageDown | 上一项/下一项 |
| Home / End | 上一章/下一章 |
| A / B / C | 设置/清除 A-B Repeat |
| S | 截图 |
| , / . | 暂停态帧步进 |
| J / K | 字幕延迟调整 |
| Ctrl+J / Ctrl+K | 音频延迟调整 |
| 1..9 | 跳转到 10%..90% |
| V | 字幕开关 |

## 诊断与验证

```powershell
.\build\Release\modern-video-player.exe --version
.\build\Release\modern-video-player.exe --capabilities
.\build\Release\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```

完整功能和回归入口见 [docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md](docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md)。

## 源码结构

```text
include/ and src/
  core/        PlayerCore、Scheduler、Clock、FrameQueue、WorkerThread、PlaybackStrategy
  platform/    平台能力探测、硬件设备创建
  decoder/     解码后端能力与候选排序
  render/      SDL、D3D11、OpenGL、Vulkan renderer
  subtitle/    SRT/ASS、内嵌字幕、bitmap subtitle、字体注册
  audio/       mixer、equalizer，SDL audio 输出在 audio_player
  filters/     音视频滤镜接口、链路和内置滤镜
  streaming/   HTTP、HLS、DASH、ABR 基础设施
  plugin/      插件 API、加载器、示例插件
  playlist/    播放列表
  config/      设置持久化
  input/       热键和输入抽象
  ui/          轻量 skin/theme 骨架
```

## 文档入口

- 当前架构：[docs/design/ARCHITECTURE.md](docs/design/ARCHITECTURE.md)
- 当前功能与验证：[docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md](docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md)
- 当前路线与 TODO：[docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md](docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md)
- 本地验收报告索引：[docs/reports/README.md](docs/reports/README.md)
- 版本记录：[docs/records/VERSION.md](docs/records/VERSION.md)
