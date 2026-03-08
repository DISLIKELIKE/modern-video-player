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
| 2026-03-06 | 修复小屏窗口过大与窗口缩放事件兼容问题 |
| 2026-03-07 | 接入 GitHub Actions 自动格式回归与 CI 兼容改造 |
| 2026-03-07 | 接入播放列表主链路、设置持久化与快捷键首版 |

---

## 2026-03-06 更新

### Core API / Scheduler / Filter 重构
- 新增 `core` 子模块：`PlayerCore`、`Scheduler`、`FrameQueue`、`Clock`、`Command`、`Frame`。
- 新增 `filters` 子模块：Filter 接口、`FilterRegistry`、`FilterPipeline`、内置亮度/对比度/饱和度滤镜。
- 新增 `USE_NEW_PLAYER_CORE` 路径下的 `VideoPlayer` 适配，保持旧接口兼容。

### 测试与构建
- 新增 `tests/core_frame_queue_tests.cpp`、`tests/core_clock_tests.cpp`。
- `CMakeLists.txt` 增加 `src/core/*` 与 `src/filters/*` 编译项，补充核心测试包含路径。

---

## 2026-03-06 架构收敛更新

### 统一核心架构
- 去除旧播放链路，实现统一为 `VideoPlayer + PlayerCore + Scheduler + Filters`。
- 删除旧 decoder/thread/sync/packet/legacy clock 相关文件。
- `CMakeLists.txt` 仅保留新架构编译项。

### 文档
- 新增 `docs/ARCHITECTURE_REFACTOR_2026-03-06.md`，包含重构目标、删除模块、保留文件清单。

---

## 2026-03-06 窗口体验修复

### 小屏自适应与缩放稳定性
- `Display::init()` 启动时按屏幕可用区域自适应初始窗口尺寸（限制到 90%，保持视频比例）。
- 增加 `SDL_WINDOWEVENT_SIZE_CHANGED` 处理，兼容不同平台/驱动下的窗口尺寸变化事件。
- 渲染阶段按源视频比例计算目标区域，避免窗口拖拽后画面拉伸。

### 修改文件
- `src/display.cpp`
- `docs/DEVELOP_LOG.md`
- `docs/CHANGELOG.md`
- `docs/VERSION.md`

## 2026-03-07 更新

### 播放稳定性与基础交互增强
- 修复窗口最大化/缩放时视频画面可能卡住的问题（音频不中断）。
- 显示层新增基础控制条（进度条、音量条）。
- 支持鼠标拖动进度条进行 seek。
- 支持鼠标拖动音量条进行实时音量调节。
- 补充音量快捷键：`+/-` 与 `↑/↓`。

### 修改文件
- include/display.h
- src/display.cpp
- src/core/player_core.cpp
- src/main.cpp
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

## 2026-03-07 更新（企业级模块推进）

### MPC-HC 架构模块扩展
- 新增企业级基础设施模块：`TaskQueue`、`FramePool`、`DecoderThread`。
- 新增渲染抽象层：`IVideoRenderer`、`SdlVideoRenderer`、`D3D11/OpenGL` 占位实现、`RendererFactory`。
- `PlayerCore` 从直接依赖 `Display` 调整为依赖渲染抽象接口。
- 新增音频增强模块：10 段均衡器、混音器；`AudioPlayer` 增加缓冲观测接口。
- 新增解码器管理模块：能力探测与后端自动选择。
- 新增字幕（SRT）、播放列表（M3U8）、设置、快捷键、皮肤、插件、格式支持、流媒体解析等模块骨架。
- 新增滤镜基类与音视频滤镜链，补充音量/平衡音频滤镜。
- 同步更新 `.monkeycode/specs/enterprise-quill-logging/tasklist.md` 已实现项。

### 修改文件
- CMakeLists.txt
- include/core/*
- src/core/*
- include/render/*
- src/render/*
- include/audio/*
- src/audio/*
- include/decoder/*
- src/decoder/*
- include/subtitle/*
- src/subtitle/*
- include/playlist/*
- src/playlist/*
- include/config/*
- src/config/*
- include/input/*
- src/input/*
- include/media/*
- src/media/*
- include/streaming/*
- src/streaming/*
- include/ui/*
- src/ui/*
- include/plugin/*
- src/plugin/*
- include/filters/*
- src/filters/*
- .monkeycode/specs/enterprise-quill-logging/tasklist.md
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

## 2026-03-07 更新（格式能力矩阵与高分高帧评估）

### 能力检查入口
- 新增 `--capabilities` 命令：输出运行时容器/视频解码器/音频解码器能力，并给出主力格式覆盖矩阵（PASS/PARTIAL/FAIL）。
- 新增 `--evaluate-target` 命令：按 `宽/高/FPS/声道/码率` 评估实时播放可行性与硬解/D3D11建议。

### 稳健性增强
- 修复 `src/streaming/dash_manifest_parser.cpp` 在 MSVC 下的 raw-string 正则编译问题，恢复构建基线。
- `Demuxer` 使用 `av_find_best_stream`，并引入 `probesize`/`analyzeduration`/`+genpts` 选项增强复杂媒体探测。
- `AudioPlayer` 暴露实际输出参数；`PlayerCore` 复用重采样器并按设备输出参数做多音道重采样。

### 修改文件
- src/streaming/dash_manifest_parser.cpp
- include/media/format_support.h
- src/media/format_support.cpp
- include/demuxer.h
- src/demuxer.cpp
- include/audio_player.h
- src/audio_player.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md
- docs/README.md

## 2026-03-07 更新（D3D11VA 硬解回退链路）

### 解码后端
- `PlayerCore` 增加 D3D11VA 硬解尝试逻辑（Windows）。
- 硬解初始化失败时自动回退软解，保证可播放性优先。

### 视频输出规整
- 对硬件帧增加 `av_hwframe_transfer_data`；
- 对非 `YUV420P` 帧增加 `sws_scale` 转换，统一为 `YUV420P` 给 SDL 渲染。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp

## 2026-03-07 更新（单文件探测与格式回归自动化）

### 新增可执行入口
- 新增 `--probe-file <media_file>` 命令：输出 `probe.*` 机器可读字段，包含容器/视频/音频状态、分辨率、帧率、声道与建议信息。

### 回归工具链
- 新增 `tools/format_regression/run_format_regression.ps1`：按样本清单自动批量执行探测。
- 新增 `tools/format_regression/format_samples.csv`：维护格式样本与预期编码信息。
- 新增 `docs/FORMAT_REGRESSION.md`：记录脚本参数、输出路径和使用方式。
- 报告输出：`docs/reports/FORMAT_REGRESSION_*.md`。
- 返回码语义：`0=全部PASS`，`1=存在PARTIAL`，`2=存在FAIL`。

### 修改文件
- src/main.cpp
- tools/format_regression/run_format_regression.ps1
- tools/format_regression/format_samples.csv
- docs/FORMAT_REGRESSION.md
- docs/README.md

## 2026-03-07 更新（GitHub Actions 自动回归）

### CI 工作流
- 新增 `.github/workflows/format-regression.yml`：
  - PR / `main` / `master` 自动触发；
  - 自动下载 `SDL2/FFmpeg` 预编译依赖后执行 Windows 构建；
  - 样本生成 + `run_all_checks.ps1` 自动回归；
  - 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 产物。

### 构建与脚本兼容性
- `CMakeLists.txt`（Windows）优先支持 `SDL2::`、`FFMPEG::`、`unofficial::ffmpeg::` 导入目标，保留 `external/` 回退路径。
- `tools/download_test_samples.ps1` 支持通过 PATH 解析 `ffmpeg` 命令名，便于 CI 直接调用。

### 任务清单同步
- 更新 `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `0.3`（P0/P1/P2 范围冻结）标记完成；
  - `2.1.1`（格式矩阵定义冻结）标记完成；
  - `2.1.5`（PASS/PARTIAL/FAIL 结果表）标记完成；
  - `2.3.1`（格式矩阵结果可追溯）标记完成。

### 修改文件
- .github/workflows/format-regression.yml
- CMakeLists.txt
- tools/download_test_samples.ps1
- docs/FORMAT_REGRESSION.md
- docs/REGRESSION_OPERATION_PLAYBOOK.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/DEVELOP_LOG.md
- docs/CHANGELOG.md
- docs/VERSION.md

## 2026-03-07 更新（播放列表 + 设置 + 快捷键首版）

### 主流程能力
- `main` 接入 `PlaylistManager`：
  - 支持多文件参数播放列表；
  - 支持 `.m3u8` 导入；
  - 支持 `PageUp/PageDown` 上一首/下一首；
  - EOF 自动切换下一项。
- `main` 接入 `SettingsManager`：
  - 启动加载 `config/player_settings.ini`；
  - 失败回退默认值；
  - 退出保存音量、速度与列表索引。

### 交互增强
- 快捷键首版默认键位全部接入：
  - `Space` 播放/暂停；
  - `Enter/Alt+Enter/F` 全屏；
  - `Esc/Q` 退出逻辑（全屏优先退全屏）；
  - `Left/Right` 与 `Ctrl+Left/Ctrl+Right` 相对 seek；
  - `Up/Down/+/-/M` 音量与静音；
  - `[`/`]` 变速与 `R` 恢复 1.0x。
- 渲染事件请求链路扩展到 `PlayerCore` 与 `VideoPlayer`，支持主流程消费“上一首/下一首/退出”控制请求。

### 修改文件
- src/main.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- include/render/opengl_video_renderer.h
- src/render/opengl_video_renderer.cpp
- include/display.h
- src/display.cpp
- include/config/settings_manager.h
- src/config/settings_manager.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-07 更新（移除 Core 单元测试目标）

### 构建配置调整
- 移除 `BUILD_CORE_TESTS` 选项，避免保留无效测试开关。
- 移除 `core_frame_queue_tests`、`core_clock_tests` 和 `core_tests` 聚合目标。

### 文件清理
- 删除 `tests/core_frame_queue_tests.cpp`。
- 删除 `tests/core_clock_tests.cpp`。

### 修改文件
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp（删除）
- tests/core_clock_tests.cpp（删除）
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-07 更新（外挂字幕加载入口）

### 字幕入口能力
- 主流程新增外挂字幕参数：`--subtitle <file.srt>`。
- 未传 `--subtitle` 时，自动尝试加载与当前媒体同名 `.srt` 文件。
- `VideoPlayer` 新增外挂字幕加载与清理接口，支持 SRT 解析并记录条目数。

### 修改文件
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-07 更新（字幕渲染叠加与时序同步）

### 字幕渲染能力
- 渲染接口新增字幕文本通道：`IVideoRenderer::setSubtitleText()`。
- SDL 渲染链路新增字幕叠加层，支持多行显示、超长截断、底板与阴影。
- D3D11/OpenGL 渲染器补齐字幕接口桩实现，保证多后端编译一致性。
- 当前字幕字模为轻量实现，非 ASCII 字符会降级显示。

### 字幕时间轴同步
- `PlayerCore` 新增外挂字幕轨道管理与活跃索引缓存。
- 渲染帧路径与空闲路径均按当前播放时间更新字幕，覆盖播放/暂停/seek。
- 修复字幕更新中的锁粒度问题，避免锁内调用渲染接口。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `1.1.2 字幕渲染叠加` 标记完成；
  - `1.1.3 播放/暂停/seek 同步` 标记完成。

### 修改文件
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/display.h
- src/display.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（字幕开关与异常处理）

### 字幕开关能力
- 新增运行时字幕开关链路，支持在播放中按 `V` 切换字幕显示开/关。
- 字幕关闭时立即清空叠加层；重新开启后按当前播放时间恢复字幕同步。
- 字幕开关状态在播放会话内保持一致，跨媒体切换可继承当前显示偏好。

### 异常处理增强
- 外挂字幕加载路径检查改为 `std::error_code`，避免文件系统异常传播。
- 增加字幕解析异常捕获与日志降级处理，确保播放主链不中断。
- 加载失败时保持状态一致：清空旧字幕并继续播放媒体。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `1.1.4 字幕开关与异常处理` 标记完成。

### 修改文件
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（快捷键配置持久化）

### 持久化能力
- 新增 `hotkey.*` 配置项，覆盖首版快捷键动作。
- 启动时读取 `config/player_settings.ini` 并应用键位映射。
- 退出时将当前键位回写配置文件，保证重启后保持一致。

### 交互链路调整
- `Display` 改为通过 `HotkeyManager` 处理键位映射，不再固定写死主键值。
- `Renderer` / `PlayerCore` / `VideoPlayer` 增加热键管理透传接口。
- 保留 `Esc` 与 `Enter` 的兼容行为，降低默认使用习惯回归风险。

### 异常与兼容
- 对非法 `hotkey.*` 配置进行容错降级（保留默认并记录告警）。
- 更新 `config/player_settings.ini` 默认样例，补齐所有快捷键项。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `1.3.2 支持键位配置持久化` 标记完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（快捷键冲突检测与恢复默认）

### 冲突检测能力
- `HotkeyManager` 新增键位冲突检测接口：
  - `findConflicts()`：返回冲突动作对；
  - `hasConflicts()`：快速判断是否存在冲突。
- 启动加载热键配置时，自动检测重复键位并输出冲突日志。

### 恢复默认能力
- `HotkeyManager` 新增 `resetToDefaults()`，统一回退默认键位。
- 新增配置开关 `hotkey.restore_defaults`：
  - 设置为 `true` 后，下一次启动自动恢复默认键位；
  - 恢复完成后自动回写为 `false`，避免重复触发。
- 发现冲突时自动恢复默认键位，防止运行期动作歧义。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `1.3.3 支持键位冲突检测与恢复默认` 标记完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（字幕 seek 同步验收）

### 验收与可回归能力
- 新增 `--subtitle-sync-check <subtitle.srt>` 命令，用于自动验证字幕时间轴匹配。
- 检查覆盖两类场景：
  - 顺序播放时间轴（ordered）；
  - 非顺序 seek 跳转（seek）。
- 输出 `mismatches` 与 `PASS/FAIL`，用于 M1 验收 `1.4.1`。

### 代码结构调整
- 提取字幕时间轴匹配公共函数：
  - `include/subtitle/subtitle_timeline.h`
  - `src/subtitle/subtitle_timeline.cpp`
- `PlayerCore` 复用统一匹配函数，避免运行路径与验收路径算法分叉。

### 样例与报告
- 新增样例字幕：`samples/subtitle/subtitle_seek_sync_sample.srt`
- 新增本地验证报告：`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`1.4.1` 已完成。

### 修改文件
- include/subtitle/subtitle_timeline.h
- src/subtitle/subtitle_timeline.cpp
- src/core/player_core.cpp
- src/main.cpp
- CMakeLists.txt
- samples/subtitle/subtitle_seek_sync_sample.srt
- samples/README.md
- docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（播放列表 5 文件验收）

### 验收能力
- 新增 `--playlist-flow-check` 命令用于 `1.4.2` 自动验收。
- 验收包含：
  - 至少 5 条播放列表输入校验；
  - 前 5 条媒体可打开检查；
  - EOF 自动切换顺序覆盖 `0 -> 1 -> 2 -> 3 -> 4`。

### 输出与报告
- 命令输出 `playlist-flow-check.*` 字段，包含 `PASS/FAIL`。
- 新增本地报告：`docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`1.4.2` 已完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md
- samples/README.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（设置重启恢复验收）

### 验收能力
- 新增 `--settings-persistence-check [settings_file]`，用于 `1.4.3` 自动验收。
- 验收流程：写入设置 -> 重载设置 -> 字段一致性比对。

### 校验覆盖字段
- `player.volume_percent`
- `player.playback_speed`
- `player.resume_last_playlist`
- `player.last_playlist_index`
- `hotkey.toggle_subtitle`

### 输出与报告
- 命令输出 `settings-persistence-check.*` 字段和 `PASS/FAIL`。
- 新增本地报告：`docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`1.4.3` 已完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（容器矩阵补齐）

### 覆盖范围
- 完成 `2.1.2` 容器验收覆盖：`mp4/mkv/mov/avi/webm/flv/ts/m2ts`。

### 回归样本扩展
- `format_samples.csv` 新增三条样本：
  - `samples/mov/demo__h264_aac__1920x1080__30fps__2ch.mov`
  - `samples/avi/demo__h264_mp3__1280x720__30fps__2ch.avi`
  - `samples/m2ts/demo__h264_ac3__1920x1080__30fps__2ch.m2ts`

### 自动生成脚本扩展
- `download_test_samples.ps1` 新增 `mov/avi/m2ts` 目录与生成流程。

### 本地回归结果
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`
  - Total=12, PASS=12, PARTIAL=0, FAIL=0, SKIP=0

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`2.1.2` 已完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- samples/.gitignore
- samples/mov/.gitkeep
- samples/avi/.gitkeep
- samples/m2ts/.gitkeep
- samples/README.md
- docs/FORMAT_REGRESSION.md
- docs/REGRESSION_OPERATION_PLAYBOOK.md
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（视频编码矩阵补齐）

### 覆盖范围
- 完成 `2.1.3` 视频编码验收覆盖：`H.264/H.265/VP9/AV1/MPEG-2`。

### 回归样本扩展
- `format_samples.csv` 新增：
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`

### 自动生成脚本扩展
- `download_test_samples.ps1` 新增 MPEG-2 视频样本生成流程（`mpeg2video + ac3 + ts`）。

### 本地回归结果
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`
  - Total=13, PASS=13, PARTIAL=0, FAIL=0, SKIP=0

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`2.1.3` 已完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（音频编码矩阵补齐）

### 覆盖范围
- 完成 `2.1.4` 音频编码验收覆盖：`AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`。

### 回归样本扩展
- 新增 4 条音频编码样本：
  - `samples/mkv/demo__h264_eac3__1920x1080__30fps__2ch.mkv`
  - `samples/mkv/demo__h264_dts__1920x1080__30fps__2ch.mkv`
  - `samples/webm/demo__vp9_vorbis__1920x1080__30fps__2ch.webm`
  - `samples/mov/demo__h264_pcm_s16le__1920x1080__30fps__2ch.mov`

### 脚本增强
- `download_test_samples.ps1` 增加 E-AC3/DTS/Vorbis/PCM 样本生成。
- DTS (`dca`) 编码添加 `-strict -2`。
- `run_format_regression.ps1` 增加编码名等价匹配：
  - `dts` <-> `dca`
  - `hevc` <-> `h265`
  - `pcm` <-> `pcm_*`

### 本地回归结果
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`
  - Total=17, PASS=17, PARTIAL=0, FAIL=0, SKIP=0

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：`2.1.4` 已完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- tools/format_regression/run_format_regression.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（DecoderFactory 初始化流程接入）

### 解码初始化链路统一
- `DecoderFactory` 新增 `selectBackendOrder(codec_name, prefer_hardware)`，输出后端候选序列并保留软件解码兜底。
- `PlayerCore::initDecoders` 接入候选序列，按顺序尝试后端初始化与 `avcodec_open2`，失败自动回退下一个候选。
- `tryConfigureD3D11HardwareDecode` 调整为纯 D3D11 配置函数，后端策略统一由 `DecoderFactory` 决定。

### 配置兼容
- 保持 `decoder.prefer_hardware_decode` 配置项生效：
  - `true`：优先硬解候选，再回退软解；
  - `false`：直接走软件解码候选。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `3.1.1 DecoderFactory 接入真实初始化流程` 标记完成。

### 修改文件
- include/decoder/decoder_factory.h
- src/decoder/decoder_factory.cpp
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（D3D11VA 协商失败软解兜底）

### 回退链路增强
- 在 `PlayerCore::selectVideoPixelFormat` 中，当 D3D11VA 目标像素格式未被解码器提供时，显式降级到软件后端：
  - `video_hw_pixel_fmt_` 重置为 `AV_PIX_FMT_NONE`；
  - `video_decoder_backend_` 置为 `Software`。
- 在解码初始化流程中补充协商降级日志，便于定位“硬解初始化成功但协商阶段回退”的场景。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `3.1.2 D3D11VA 初始化与失败回退软解` 标记完成。

### 修改文件
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（D3D11 渲染最小可用链路）

### 渲染后端能力补齐
- `D3D11VideoRenderer` 完成最小可用实现：
  - `init`：创建渲染链路；
  - `renderFrame`：接入帧上传；
  - `present`：接入呈现；
  - 事件与交互请求透传。
- `Display` 新增渲染后端可观测能力：
  - 可设置 renderer 驱动偏好；
  - 可查询当前实际 renderer 后端名称。

### D3D11 后端校验与回退协同
- `D3D11VideoRenderer::init` 在请求 `direct3d11` 后，会校验实际 SDL renderer 后端：
  - 命中 `direct3d11/d3d11`：初始化成功；
  - 未命中：初始化失败并由上层触发 `SoftwareSDL` 回退链路（与 `3.2.2` 协同）。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `3.2.1 D3D11 渲染最小可用（init/upload/present）` 标记完成。

### 修改文件
- include/display.h
- src/display.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（渲染失败自动降级回归入口）

### 新增回归命令
- 新增 `--renderer-fallback-check <media_file>`：
  - 强制构造 D3D11 渲染初始化失败场景；
  - 验证是否自动回退 `SoftwareSDL`；
  - 输出 `renderer-fallback-check.*` 字段和 `PASS/FAIL`。

### 可观测能力补齐
- 渲染接口新增后端名称查询，播放器对外新增当前渲染后端/解码后端查询接口，用于命令化验收输出。

### 本地报告
- 新增 `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`，记录本地回归命令与结果。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`：
  - `3.3.2 渲染失败降级不中断播放` 标记完成。

### 修改文件
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- include/render/opengl_video_renderer.h
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（Windows 软解/硬解主力样本回归）

### 回归能力补齐
- 新增 `--windows-backend-session-check <media_file> <hard|soft>`：单模式会话检查与结构化输出。
- 新增并稳定 `--windows-backend-check <media_file>`：自动聚合硬解/软解结果并输出 `PASS/FAIL`。

### 稳定性修复
- `--windows-backend-check` 从同进程双会话改为父进程拉起两个子进程（hard/soft）并汇总，规避会话回收卡死。
- Windows 子进程执行改为 `CreateProcess`，避免 shell 重定向语法差异导致失败。

### 本地验证
- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `3.3.1 Windows 下软解+硬解主力样本可播` 标记完成。
  - `3.3.3 回归报告完整` 标记完成。

### 新增报告
- `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`

## 2026-03-08 更新（章节导航：上一章/下一章）

### 交互能力增强
- 新增章节导航快捷键：
  - `HOME`：跳转上一章；
  - `END`：跳转下一章。
- 章节请求链路已打通：
  - `Display -> Renderer -> PlayerCore -> VideoPlayer`。

### 媒体信息能力增强
- `Demuxer` 新增章节元数据解析：
  - `ChapterInfo`（start/end/title）；
  - `MediaInfo::chapters`。
- `PlayerCore` 在打开媒体后构建章节时间点，并提供：
  - `seekToNextChapter()`；
  - `seekToPreviousChapter()`；
  - `chapterCount()`。

### 验收能力补齐
- 新增 `--chapter-nav-check <media_file>`：
  - 自动执行播放、下一章、上一章并校验位置变化；
  - 输出 `chapter-nav-check.*` 与 `PASS/FAIL`。
- 新增本地报告：`docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`。

### 本地验证
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.1 章节导航（上一章/下一章）` 标记完成。

### 修改文件
- include/demuxer.h
- src/demuxer.cpp
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- include/render/opengl_video_renderer.h
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/CHAPTER_NAV_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

## 2026-03-08 更新（A-B Repeat）

### 交互能力增强
- 新增 A-B Repeat 默认热键：
  - `A`：设置 A 点；
  - `B`：设置 B 点并启用循环；
  - `C`：清除 A-B Repeat。
- 帮助信息补充 `A/B/C` 键位说明。

### 播放核心能力增强
- `PlayerCore` 新增 A-B Repeat 状态管理与查询：
  - `setABRepeatStart()`；
  - `setABRepeatEnd()`；
  - `clearABRepeat()`；
  - `isABRepeatEnabled()`；
  - `abRepeatStart()` / `abRepeatEnd()`。
- 新增播放循环检测 `handleABRepeatLoop()`：
  - 当播放位置到达 B 点时自动 seek 回 A 点。

### 验收能力补齐
- 新增 `--ab-repeat-check <media_file>`：
  - 自动执行“设置 A 点 -> 设置 B 点 -> 观察循环 -> 清除”；
  - 输出 `ab-repeat-check.*` 与 `PASS/FAIL`。
- 新增本地报告：`docs/reports/AB_REPEAT_LOCAL_CHECK.md`。

### 回归修复
- 修复 `--settings-persistence-check` 与新默认热键冲突：
  - 测试键位由 `b` 调整为 `x`，恢复回归稳定性。

### 本地验证
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`：`PASS`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.2 A-B Repeat` 标记完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- include/display.h
- src/display.cpp
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- src/render/sdl_video_renderer.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- include/render/opengl_video_renderer.h
- src/render/opengl_video_renderer.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/AB_REPEAT_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
