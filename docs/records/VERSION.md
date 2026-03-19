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


### 2026-03-19 更新：SoftwareSDL 拷贝链路量化、Scheduler 重启预算与 renderer override

- `Display::copyFrameData()` 现已具备 `frames / bytes / time_us_total / time_us_max` 统计，并通过 `RendererDiagnostics + DiagnosticsSnapshot + --performance-log-check` 输出软件显示链的真实成本。
- `Scheduler` worker 重启策略已从固定次数改成“30s 窗口内最多 4 次 + 100ms 冷却”，并新增 `scheduler_*_restart_limit_hits` 诊断。
- `RendererFactory` 新增 `MVP_RENDERER_BACKEND` override，并在 Windows 下支持 `MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`，`--renderer-fallback-check` 当前已恢复通过。
- 本机 4K60 强制 `SoftwareSDL` 采样显示：`video_copy_back_ratio_percent=30.1018`、`video_swscale_ratio_percent=18.6623`、`display_copy_ratio_percent=21.8407`；说明软件回退链已经是 copy-back、swscale、display memcpy 的叠加热点。
- 默认 `D3D11 + D3D11VA` 主链重新验证后仍保持 `video_copy_back_ratio_percent=0`、`video_swscale_ratio_percent=0`、`display_copy_ratio_percent=0`，zero-copy 结论不变。
### 2026-03-19 更新：高码率/4K 队列容量、自适应节流与 copy-back 诊断增强

- `FrameQueue` 已新增 `peak_size / push_timeout_count` 统计，`DiagnosticsSnapshot` 与 `--performance-log-check` 会同步输出 frame queue 的 `capacity / peak / timeout`。
- `PlayerCore::open()` 现在按媒体属性配置视频/音频 frame queue 容量，并在 `D3D11VA` 打开前设置 `extra_hw_frames`，避免 4K native path 打爆 FFmpeg 的硬件帧池。
- `Scheduler` 已把 video/audio 背压改为迟滞阈值，并新增 `video/audio_backpressure_wait_ms` 统计；`pumpRenderOnce()` 同时修正了 `Video` master 的 wall-clock pacing 和 late-frame catch-up。
- 最新 4K 性能采样显示：`renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`video_native_output_frames=101`、`video_copy_back_frames=0`、`video_swscale_frames=0`，说明当前主链仍以 native zero-copy 为主，不是 copy-back 热点。
- 已重新验证：`MSBuild`、`--performance-log-check`、`--high-bitrate-check`、`--4k-playback-check`、`--long-playback-check` 当前均通过。
### 2026-03-19 更新：4K backend session 子进程退出路径修复

- `--windows-backend-session-check` 已从“复用常规播放器退出收尾”改为“一次性 probe 子进程”路径：打印结构化结果后显式 flush，并在 Windows 下直接 `TerminateProcess(GetCurrentProcess(), code)` 退出。
- 这次修复针对的是回归 harness，不是主播放链运行时逻辑；目标是消除 `hard` 模式打印 PASS 后超时、`soft` 模式打印 PASS 后异常退出的残留失败。
- 已重新验证：`hard/soft --windows-backend-session-check` 退出码均为 `0`，`--windows-backend-check` 与 `--4k-playback-check` 当前均已恢复通过。
### 2026-03-19 更新：音频设备失败时的视频-only降级与回归门禁纠偏

- `PlayerCore::open()` 现在把音频设备失败分成两类：视频文件会降级为 `video-only` 继续播放，音频-only 文件仍然直接失败，避免“打开成功但没有任何可播放输出”的伪成功。
- 无音频输出但有视频流时，主时钟从 `System` 改为 `Video`，让位置推进跟随实际渲染 PTS，而不是系统时钟估算。
- `DiagnosticsSnapshot` 新增 `audio_output_initialized / video_only_fallback / clock_source`，`--performance-log-check`、`--1080p60-check`、`--4k-playback-check`、`--high-bitrate-check`、`--long-playback-check` 会同步输出这些状态。
- `1080p60/high-bitrate/long-playback` 三个回归门禁现已改为只看 `demux_queue_drop_packets`，不再把音频禁用场景下的 `demux_ignored_packets` 误判成高码率回压失败。
- 已重新执行：
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  以及 `--1080p60-check`、`--high-bitrate-check`、`--long-playback-check`、`--performance-log-check`，当前均通过；`--4k-playback-check` 仍只剩 `fallback_ok` 子进程路径待继续排查。

### 2026-03-19 更新：播放链诊断分层与 decoder drain / scheduler 容错补强

- `decodeVideoFrame()` / `decodeAudioFrame()` 已改为持续 `receive -> send -> receive` 状态机，并在 packet queue EOF 后对 codec 发送 `nullptr` drain，避免把“暂时无输出”和“真正失败”混在一起。
- `DiagnosticsSnapshot` 现已区分 `demux_ignored_packets / demux_queue_drop_packets`，并新增 decoder `send_packet(EAGAIN)`、drain 次数、`native/copy-back/swscale/filter-blocked` 视频路径计数。
- `Scheduler` 已新增 video/audio 背压事件与 video/audio/render restart 统计，render thread 也纳入受保护 worker；`--performance-log-check` 会同步输出这些新指标。

### 2026-03-19 更新：PlayerCore 停播收口与运行时设计债修复

- `PlayerCore` 已补上 deferred stop / worker reap 路径，EOF 自动停播、Next/Previous、Quit 等路径不再只改状态而遗留 demux/audio/scheduler 脏线程。
- `PacketQueue` 已从原始 `AVPacket*` 切到 RAII `unique_ptr` 所有权模型，seek / stop / close 过程中的剩余压缩包会随队列清理自动释放。
- `Scheduler` 已支持异步停机并在重启前回收已退出 worker；`Clock` 已修复 system-clock 的 pause/speed 时间基准连续性；`Demuxer::open()` 也已去掉锁内重入 `close()` 的死锁风险。
- 已重新执行整工程验证命令
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`
  当前结果为 0 个警告 / 0 个错误。

### 2026-03-18 更新：MSVC warning debt 分层清理

- Windows MSVC 目标已启用 `/utf-8 /external:anglebrackets /external:W0`，本地 UTF-8 源文件不再触发 `C4819`，第三方 angle-bracket 头文件 warning 也被隔离到外部层。
- 本地 `logger.cpp` 的 `getenv`、字幕解析器的 `sscanf`、`main.cpp` 的未使用参数，以及 `player_core.cpp` 的条件赋值 warning 均已清理。
- 已重新执行整工程验证命令
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`
  当前结果为 `0 个警告 / 0 个错误`。
- 这轮变更不扩展播放器功能面，目标仅为恢复 Windows CI 的低噪声构建基线。

### 2026-03-18 更新：ASS 标签解析与 UTF-16 范围修正

- `ASS/SSA` override 解析已修复 `\fnArial`、`\rDefault` 等紧凑写法的标签识别错误，常见字体切换和样式重置现在会按预期进入原生 D3D11 字幕链。
- `SubtitleTextRun.start/length` 与 `D3D11VideoRenderer` 的 DirectWrite `DWRITE_TEXT_RANGE` 已统一为 UTF-16 code unit 语义，emoji、扩展字符和其他非 BMP 文本的样式范围不再错位。
- 已重新执行整工程验证命令
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`
  当前结果为 `167 个警告 / 0 个错误`；warning 主要来自第三方头文件（FFmpeg / Quill）、项目内多处源码的 C4819 编码告警，以及少量既有的 C4996 / C4100 提示。
- 本轮剩余补丁不再扩展功能面，只收敛原生 D3D11 字幕链的语义正确性与交付记录。
### 2026-03-18 更新：D3D11 原生渲染链与 ASS/SSA 字幕链收口

- Windows 默认 `D3D11` renderer 现在具备独立的 `window + device + swap chain + native video + native subtitle overlay` 主链。
- 在未启用视频滤镜且格式受支持时，`PlayerCore` 会保留 `AV_PIX_FMT_D3D11` 原生硬件帧直通到 renderer，并同时向渲染器推送多条当前激活字幕对象。
- 外挂字幕链已从“仅 SRT / 纯文本”推进到“.ass/.ssa/.srt + 结构化字幕对象 + 原生 D3D11 样式绘制”；`main.cpp` 自动探测顺序为 `.ass -> .ssa -> .srt`。
- `D3D11VideoRenderer` 现在在同一块 swap chain backbuffer 上完成 ASS/SSA 常用样式字幕绘制，覆盖填充、描边、阴影、背景框、对齐、定位和 run 级字体样式；非 D3D11 渲染器默认退化为纯文本显示。
- 已清理全局构建阻塞文件中的编码误读问题，完整解决方案验证命令
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m`
  当前结果为 `0 个警告 / 0 个错误`。
- 当前限制仍然存在：这不是完整的 libass 等价实现，尚未补充真实 `.ass` 样本的运行时视觉验收；详见 `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md` 与 `docs/records/CHANGELOG.md` 的问题 66。
---
## 阶段一：基础播放器（历史起点）

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

**说明**:
- 本节记录的是 2026-02-17 ~ 2026-02-25 的早期实现基线。
- 当时使用的是旧版 decoder / playLoop 架构；相关文件已在 2026-03-06 架构收敛后移除或并入 `PlayerCore + Scheduler + core/*` 主链。
- 为避免误导，下述历史说明保留“能力与问题”本身，不再把已删除文件当作当前仓库结构说明。

### 版本兼容性修改

#### FFmpeg 8.0.1 兼容性
- 修复 `codec_ctx_->avctx->priv_data` 在 FFmpeg 8.0 中不可用的问题
- 该修复发生在旧版独立视频/音频解码器实现中：
  - 在解码器对象内显式保存 `format context`；
  - 不再依赖从 `codec context` 反向读取内部字段。
- 相关实现后来已并入现行解封装/解码主链，旧文件名已不再保留。
#### 日志系统更新（企业级 Quill 管道）
- 重新启用 Quill，构建 ConsoleSink + RotatingFileSink 异步日志通道，并保持 stdout/stderr 备援。
- `config/logging.conf` 与 `MVP_LOG_*` 环境变量支持运行时调整 `log_dir`、轮转大小/数量及日志等级。
- 初始化失败或目录不可写时自动告警并降级；宏接口与 `DEBUG_MODE` 行为保持兼容。
- 修改文件：`include/logger.h`、`src/logger.cpp`、`config/logging.conf`、`docs/design/LOGGING.md`、`docs/records/CHANGELOG.md`、`docs/records/VERSION.md`
- 编译输出示例：`Quill: enabled`，运行期日志写入 `logs/modern-video-player.log`

#### 多线程播放架构重构 (2026-02-25)
- 实现独立视频解码线程（VideoDecodeThread）
- 实现独立音频解码线程（AudioDecodeThread）
- 实现线程安全帧队列（FrameQueue），支持条件变量等待
- 实现音视频同步管理器（SyncManager），支持 AudioMaster/VideoMaster/Free 三种模式
- 重构 VideoPlayer，从单线程 playLoop 改为多线程 renderLoop
- 这些能力后来沉淀为当前仓库中的 `core` 模块：
  - `include/core/frame_queue.h`
  - `include/core/decoder_thread.h`
  - `include/core/clock.h`
  - `include/core/scheduler.h`
  - `include/core/player_core.h`
- 旧版线程/同步文件名仅代表早期实现阶段，现已不再作为当前文件结构存在。
#### 音频播放架构修复 (2026-02-25)
- 修复音频播放断断续续的问题
- AudioDecodeThread 解码后直接调用 AudioPlayer::play() 将数据放入 SDL 队列
- 确保 SDL 音频回调能持续获取数据
- 相关逻辑后来已并入当前音频消费线程、`AudioPlayer` 与 `PlayerCore` 主链，不再对应独立旧文件名。
#### Frame 移动语义修复 (2026-02-25)
- 修复 VideoFrame 和 AudioFrame 类缺少移动语义导致的崩溃问题
- 为 VideoFrame 添加移动构造函数和移动赋值运算符
- 为 AudioFrame 添加移动构造函数和移动赋值运算符
- 显式删除拷贝构造函数和拷贝赋值运算符，防止浅拷贝
- 当前等价能力位于：
  - `include/core/frame.h`
  - `src/core/frame.cpp`
#### 多解码器实例竞争读取修复 (2026-02-25)
- 修复播放时多个解码器竞争读取同一 AVFormatContext 导致的解码错误
- 在早期实现中通过重置旧版解码器状态避免竞争；后续已改为统一 `Demuxer + PlayerCore + Scheduler` 主链
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
- 修改旧版视频解码循环的读包逻辑
- 将遇到不匹配流时返回 false，改为 continue 跳过该包
- 循环读取直到找到正确流索引的包

**修改文件**:
- 旧版视频解码器实现（该文件后续已在架构收敛中移除）

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
- 修改旧版音频解码循环的读包逻辑
- 应用与视频解码器相同的修复

**修改文件**:
- 旧版音频解码器实现（该文件后续已在架构收敛中移除）

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

### 阶段后续目标（历史记录）
- 后续目标已在 2026-03-06 之后的章节中逐步落地，包括：
  - 统一 `PlayerCore + Scheduler + Filters` 架构；
  - 外挂字幕、播放列表、设置与快捷键接入；
  - D3D11VA / D3D11 最小可用链路；
  - 章节导航、A-B Repeat、截图。

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
| 2026-03-08 | 清理历史章节中的旧路径描述，避免将已移除文件误写为当前仓库结构 |
| 2026-03-18 | 同步 D3D11 原生渲染链最终状态，记录 ASS/SSA 原生字幕链收口与全局构建阻塞清理 |

---

## 2026-03-06 更新

### Core API / Scheduler / Filter 重构
- 新增 `core` 子模块：`PlayerCore`、`Scheduler`、`FrameQueue`、`Clock`、`Command`、`Frame`。
- 新增 `filters` 子模块：Filter 接口、`FilterRegistry`、`FilterPipeline`、内置亮度/对比度/饱和度滤镜。
- 初期引入 `VideoPlayer -> PlayerCore` 适配层，随后在同日完成架构收敛并移除旧路径分叉。

### 测试与构建
- 当时曾引入临时核心测试文件；这些测试文件已在 2026-03-07 清理。
- `CMakeLists.txt` 在该阶段开始纳入 `src/core/*` 与 `src/filters/*` 主编译项。

---

## 2026-03-06 架构收敛更新

### 统一核心架构
- 去除旧播放链路，实现统一为 `VideoPlayer + PlayerCore + Scheduler + Filters`。
- 删除旧 decoder/thread/sync/packet/legacy clock 相关文件。
- `CMakeLists.txt` 仅保留新架构编译项。

### 文档
- 新增 `docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`，包含重构目标、删除模块、保留文件清单。

---

## 2026-03-06 窗口体验修复

### 小屏自适应与缩放稳定性
- `Display::init()` 启动时按屏幕可用区域自适应初始窗口尺寸（限制到 90%，保持视频比例）。
- 增加 `SDL_WINDOWEVENT_SIZE_CHANGED` 处理，兼容不同平台/驱动下的窗口尺寸变化事件。
- 渲染阶段按源视频比例计算目标区域，避免窗口拖拽后画面拉伸。

### 修改文件
- `src/display.cpp`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`

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
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

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
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

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
- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md
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
- 新增 `docs/workflows/FORMAT_REGRESSION.md`：记录脚本参数、输出路径和使用方式。
- 报告输出：`docs/reports/FORMAT_REGRESSION_*.md`。
- 返回码语义：`0=全部PASS`，`1=存在PARTIAL`，`2=存在FAIL`。

### 修改文件
- src/main.cpp
- tools/format_regression/run_format_regression.ps1
- tools/format_regression/format_samples.csv
- docs/workflows/FORMAT_REGRESSION.md
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
- docs/workflows/FORMAT_REGRESSION.md
- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/workflows/FORMAT_REGRESSION.md
- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

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
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-08 更新（截图）

### 交互能力增强
- 新增稳定可用的截图能力：
  - `S` 触发截图；
  - 输出文件写入 `screenshots/` 目录；
  - 命名格式为 `screenshot_YYYYMMDD_HHMMSS_mmm.ppm`。

### 截图链路收敛
- `PlayerCore` 新增最近渲染帧缓存，用于在暂停态直接保存当前画面。
- `requestScreenshot()` 区分两种路径：
  - 播放中：异步等待当前/下一帧落盘；
  - 暂停态：直接从缓存帧落盘。
- `main` 中的 `--screenshot-check` 改为覆盖暂停态截图场景。

### 本地验证
- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`：`PASS`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.3 截图` 标记完成。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- README.md
- README_ZH.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SCREENSHOT_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-08 更新（帧步进）

### 交互能力增强
- 新增暂停态帧步进：
  - `,` 后退一帧；
  - `.` 前进一帧；
  - 步进后仍保持暂停状态。

### 主链实现
- `PlayerCore` 新增 `stepFrameBackward()` / `stepFrameForward()`，采用“暂停态 seek + 首帧刷新”收敛单帧步进。
- 步进时复用最近渲染帧时长与媒体 FPS 估算步长，并在 seek 后主动渲染目标时间点的首个视频帧。
- 音频消费线程在暂停态不再用旧 `playback_pts` 回写当前位置，避免帧步进后位置被音频时钟回拉。
- `main` 新增 `--frame-step-check <media_file>` 自检命令。

### 本地验证
- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.4 帧步进（暂停态）` 标记完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
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
- src/main.cpp
- README.md
- README_ZH.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/FRAME_STEP_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-08 更新（差距评估文档对齐）

### 文档基线刷新
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 更新为截至 `2026-03-08` 的实现状态。
- 评估依据从“只看接口/骨架”升级为“代码入口 + 本地验收报告”联合判断。

### 结论修正
- 将以下模块从旧版的“骨架/未接入”纠正为“部分实现”：
  - 字幕系统；
  - 播放列表；
  - 解码器管理；
  - 快捷键系统；
  - 设置系统。
- 模块统计更新为：
  - `达到 MPC-HC 等级: 0 / 14`
  - `部分实现: 11 / 14`
  - `骨架/未接入: 3 / 14`

### 剩余重点更新
- `P0` 聚焦转为：
  - `1080p60 / 4K / 高码率` 稳定性与性能日志；
  - 多音轨/字幕轨/时延调节；
  - `OpenGL` 后端策略收敛；
  - 倍速音频策略继续完善。
- `P1` 聚焦：全量快捷键、画幅/缩放/旋转、滤镜用户入口。
- `P2` 聚焦：插件、流媒体、皮肤系统产品化。

### 修改文件
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-08 更新（版本文档历史段落清理）

### 历史章节去歧义
- 将“阶段一：基础播放器（当前阶段）”调整为“阶段一：基础播放器（历史起点）”，明确该段记录的是 `2026-02-17 ~ 2026-02-25` 的早期实现基线。
- 为阶段一补充说明：旧版 `decoder / playLoop` 路径已在 `2026-03-06` 架构收敛后并入 `PlayerCore + Scheduler + core/*` 主链。

### 旧路径表述清洗
- 将早期 `video_decoder` / `audio_decoder` 以及 `VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 的文件级表述改写为能力级历史记录。
- 将“下一步计划”改写为“阶段后续目标（历史记录）”，避免与当前迭代进度章节混淆。
- 将 `USE_NEW_PLAYER_CORE` 和临时 `tests/core_*` 的历史描述改写为“架构收敛 / 后续清理”口径。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

## 2026-03-08 ?????/???????

### ????
- ?? `J / K` ???????`- / +100ms`??
- ?? `Ctrl+J / Ctrl+K` ???????`- / +100ms`??
- ????/??????????
  - `player.audio_delay_ms`
  - `player.subtitle_delay_ms`

### ?????
- ?????????`--delay-adjust-check <media_file> <subtitle.srt>`?
- ???????
  - `build/Debug/modern-video-player.exe --settings-persistence-check`
  - `build/Debug/modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt`
- ?????`docs/reports/DELAY_ADJUST_LOCAL_CHECK.md`

### ??????
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.5 ????/??????` ?????

### ????
- include/core/player_core.h
- include/video_player.h
- include/display.h
- include/render/video_renderer.h
- include/render/sdl_video_renderer.h
- include/render/d3d11_video_renderer.h
- include/render/opengl_video_renderer.h
- include/input/hotkey_manager.h
- src/core/player_core.cpp
- src/video_player.cpp
- src/display.cpp
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- src/input/hotkey_manager.cpp
- src/main.cpp
- config/player_settings.ini
- README.md
- README_ZH.md
- docs/README.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/reports/DELAY_ADJUST_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-08 ??????????

### ????
- ?????? `1..9`???????? `10%..90%` ???
- ????????????`seek_to_10_percent` ~ `seek_to_90_percent`?
- `Display` ????????? seek ????????????????

### ?????
- ?????????`--numeric-seek-check <media_file>`?
- ???????
  - `build/Debug/modern-video-player.exe --numeric-seek-check .\juren-30s.mp4`
- ?????`docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md`

### ??????
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `4.6 ???????A/B/C/S,./J/K/1..9?` ?????

### ????
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/display.cpp
- src/main.cpp
- README.md
- README_ZH.md
- docs/README.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（播放性能日志验收）

### 可观测性补齐
- 新增 `core::DiagnosticsSnapshot`，统一导出解封装包计数、重试/丢包、解码帧数、渲染帧数、音频提交帧数、调度器掉帧与队列长度。
- `VideoPlayer` 新增 `getInfo()` / `getDiagnosticsSnapshot()` 透传接口，供命令行验收和后续诊断复用。
- `main` 新增 `--performance-log-check <media_file> [sample_ms]`，输出：
  - renderer / decoder backend（作为当前 GPU 路径标识）；
  - 进程平均 CPU 占用与逻辑核心数；
  - demux / decode / render / scheduler / queue 关键指标。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`：`PASS`
- 验收报告：`docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `2.2.4 输出性能日志（掉帧/队列/CPU/GPU）` 标记完成。

### 修改文件
- include/core/player_core.h
- include/video_player.h
- src/core/player_core.cpp
- src/video_player.cpp
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（1080p60 稳定播放验收）

### 稳定性门禁补齐
- `main` 新增 `--1080p60-check <media_file> [sample_ms]`，复用 `probe + diagnostics` 口径验证：
  - `1920x1080 @ 60fps` 样本是否匹配目标；
  - 连续播放窗口内时间是否持续推进；
  - 调度器是否出现 `late_drop`；
  - 解封装是否发生丢包。
- 新增 `samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4` 的样本生成脚本入口，用于本地稳定性回归。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`：`PASS`
- 验收报告：`docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `2.2.1 1080p60 长时稳定` 标记完成。
  - `2.3.2 1080p60 稳定播放达标` 标记完成。

### 修改文件
- src/main.cpp
- tools/download_test_samples.ps1
- samples/README.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/1080P60_STABILITY_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（4K 播放与降级验收）

### 4K 门禁补齐
- `main` 新增 `--4k-playback-check <media_file> [sample_ms]`，联合验证：
  - `4K` 样本在连续播放窗口内是否持续推进；
  - 当前播放链路是否无 `late_drop`；
  - hard / soft 两个子进程会话是否均可进入播放，证明硬解失败时可降级。
- 验收逻辑复用了已有 `runBackendSessionSubprocess()`，避免重复实现 Windows 后端回退校验。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`：`PASS`
- 验收报告：`docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `2.2.2 4K 可播放（优先稳定，再提性能）` 标记完成。
  - `2.3.3 4K 播放可用并可降级` 标记完成。

### 修改文件
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（高码率样本验收）

### 高码率门禁补齐
- `main` 新增 `--high-bitrate-check <media_file> [sample_ms]`，前置读取格式码率并要求 `>= 80Mbps`。
- 连续播放窗口同时检查：时间推进、`late_drop`、demux 丢包与实际 decoder / renderer backend。
- `tools/download_test_samples.ps1` 新增 `stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 生成入口，用于高码率回归。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`：`PASS`
- 验收报告：`docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `2.2.3 大码率样本（>80Mbps）可播放` 标记完成。

### 修改文件
- src/main.cpp
- tools/download_test_samples.ps1
- samples/README.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/HIGH_BITRATE_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（长时播放稳定性验收）

- `main` 新增 `--long-playback-check <media_file> [sample_ms]`，要求采样窗口至少 `5000ms`，并输出 `probe`、backend、时间推进与掉帧/丢包门禁字段。
- 连续播放窗口同时检查：`open_ok`、进入播放循环、窗口结束后仍处于播放态、`advance_ratio` 达标、`scheduler_late_drops=0` 与 `demux_dropped_packets=0`。
- 新增 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`，并同步发布门禁 `6.1 ~ 6.6` 为已完成。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`：`PASS`
- 验收报告：`docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `6.1 功能：M1/M2 必须全部通过` 标记完成。
  - `6.2 格式：主力格式矩阵有结果且可解释` 标记完成。
  - `6.3 分辨率：1080p60 稳定，4K 可用并可降级` 标记完成。
  - `6.4 交互：默认快捷键完整可用` 标记完成。
  - `6.5 稳定性：长时播放无 crash` 标记完成。
  - `6.6 可观测：关键日志完整` 标记完成。

### 修改文件
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-08 更新（插件系统）

- 新增 `include/plugin/plugin_api.h` 与重写 `PluginManager`，补齐 `DLL` 动态加载、`API` 版本校验、`load/unload` 生命周期与异常保护。
- `PluginManager` 现可作为插件宿主注册外部视频/音频滤镜工厂；`FilterRegistry` 同步补充注销接口，支持卸载时清理扩展点。
- 新增 `sample_logger_plugin` 示例 `DLL` 与 `--plugin-check [plugin_dir_or_file]` 验收命令，验证插件加载、滤镜注册与卸载清理闭环。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --plugin-check`：`PASS`
- 验收报告：`docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `7.1 插件系统` 标记完成。

### 修改文件
- CMakeLists.txt
- include/plugin/plugin_api.h
- include/plugin/plugin_manager.h
- include/filters/filter_registry.h
- src/plugin/plugin_manager.cpp
- src/plugin/sample_logger_plugin.cpp
- src/filters/filter_registry.cpp
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-08 更新（流媒体 HTTP 分片与缓冲）

- 重写 `HttpStreamDownloader`：基于 FFmpeg `avio` 支持真实 HTTP 打开、分块读取、内部缓冲、EOF 状态与错误透传。
- `main` 新增 `--streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]`，可下载 HLS 媒体清单、解析相对分片 URL，并验证分片缓冲闭环。
- 新增本地夹具 `samples/streaming/hls_local/*` 与 `tools/start_streaming_fixture_server.ps1`，用于在本机启动 HTTP 静态服务并复现实验。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`
- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`：`PASS`
- 验收报告：`docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `7.2 流媒体（真实 HTTP 分片与缓冲）` 标记完成。

### 修改文件
- include/streaming/http_stream_downloader.h
- src/streaming/http_stream_downloader.cpp
- src/main.cpp
- tools/start_streaming_fixture_server.ps1
- samples/README.md
- samples/streaming/hls_local/sample.m3u8
- samples/streaming/hls_local/segment000.ts
- samples/streaming/hls_local/segment001.ts
- samples/streaming/hls_local/segment002.ts
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 2026-03-08 更新（HLS/DASH 自适应码率）

- 扩展 `HlsManifestParser`：识别 `#EXT-X-STREAM-INF` master playlist、多码率 variant 与媒体播放列表。
- 扩展 `DashManifestParser`：解析 `Representation` 带宽、`BaseURL`、`Initialization` 与 `SegmentURL` 明细。
- 新增 `AdaptiveBitrateSelector`，按估算带宽选择最合适码率，并在 `main` 中提供 `--adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]` 本地验收入口。
- 新增 `samples/streaming/abr_local/{hls,dash}` 夹具，并扩展 `tools/start_streaming_fixture_server.ps1` 支持 `mpd/m4s/mp4` 内容类型。

### 本地验证
- `cmake --build build --config Debug`：通过。
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`
- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`：`PASS`
- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`：`PASS`
- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`：`PASS`
- 验收报告：`docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `7.3 HLS/DASH 自适应码率` 标记完成。

### 修改文件
- include/streaming/hls_manifest_parser.h
- src/streaming/hls_manifest_parser.cpp
- include/streaming/dash_manifest_parser.h
- src/streaming/dash_manifest_parser.cpp
- include/streaming/adaptive_bitrate_selector.h
- src/streaming/adaptive_bitrate_selector.cpp
- src/main.cpp
- CMakeLists.txt
- tools/start_streaming_fixture_server.ps1
- samples/README.md
- samples/streaming/abr_local/hls/master.m3u8
- samples/streaming/abr_local/hls/low/index.m3u8
- samples/streaming/abr_local/hls/low/segment000.ts
- samples/streaming/abr_local/hls/low/segment001.ts
- samples/streaming/abr_local/hls/medium/index.m3u8
- samples/streaming/abr_local/hls/medium/segment000.ts
- samples/streaming/abr_local/hls/medium/segment001.ts
- samples/streaming/abr_local/hls/high/index.m3u8
- samples/streaming/abr_local/hls/high/segment000.ts
- samples/streaming/abr_local/hls/high/segment001.ts
- samples/streaming/abr_local/dash/sample.mpd
- samples/streaming/abr_local/dash/low/init.mp4
- samples/streaming/abr_local/dash/low/segment000.m4s
- samples/streaming/abr_local/dash/low/segment001.m4s
- samples/streaming/abr_local/dash/medium/init.mp4
- samples/streaming/abr_local/dash/medium/segment000.m4s
- samples/streaming/abr_local/dash/medium/segment001.m4s
- samples/streaming/abr_local/dash/high/init.mp4
- samples/streaming/abr_local/dash/high/segment000.m4s
- samples/streaming/abr_local/dash/high/segment001.m4s
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
## 2026-03-08 更新（建立里程碑标签）

- 完成任务清单 `0.4`，建立里程碑标签 `v0.2.0-rc1` 与 `v0.2.0`。
- `v0.2.0-rc1` 用于冻结发布门禁已收口的候选快照；`v0.2.0` 用于标记当前主线里程碑完成点。
- 同步刷新差距评估与任务清单状态，避免文档仍保留“仅差标签操作”的旧口径。
- 由于里程碑已具备可追溯的 `RC` 标签，任务清单中的 `5.3 每个里程碑结束前必须可打 RC 标签` 也同步满足。

### 标签说明
- `v0.2.0-rc1`：基于发布门禁闭环后的稳定快照建立。
- `v0.2.0`：基于当前 `main` 的里程碑收口快照建立。

### 本地验证
- `git tag --list`：确认标签已存在。
- `git show v0.2.0-rc1 --no-patch --stat`
- `git show v0.2.0 --no-patch --stat`

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `0.4 建立里程碑标签（v0.2.0-rc1 / v0.2.0）` 标记完成。
  - `5.3 每个里程碑结束前必须可打 RC 标签` 标记完成。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
## 2026-03-08 更新（执行守则口径同步）

- 结合本轮提交与任务推进顺序，`5.1 WIP 限制：同时进行任务不超过 2 个` 可判定为满足，并同步勾选。
- `5.2 每周五只做收敛（修复、回归、文档）` 属于按周执行节奏约束，当前仓库中尚缺持续性的周维度证据，暂保持未勾选。

### 口径说明
- `5.1` 的判断依据是本轮任务按单主线串行推进：发布门禁、插件系统、流媒体缓冲、ABR、里程碑标签与守则口径依次收口。
- `5.2` 需要跨周、重复执行的过程证据，不适合仅凭当前一次收口结果直接判定完成。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `5.1 WIP 限制：同时进行任务不超过 2 个` 标记完成。
  - `5.2 每周五只做收敛（修复、回归、文档）` 保持待完成。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 问题 63: 落地 5.2 周五收敛日执行手册

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 守则项 `5.2 每周五只做收敛（修复、回归、文档）` 目前只有原则描述，缺少可重复执行的流程、边界和输出物定义。

### 分析记录
1. `docs/plans/MPC_HC_ITERATION_PLAN.md` 只给出“每周五固定做收敛日”的节奏建议，但没有落实成逐步操作手册。
2. `docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md` 已覆盖“如何做回归”，但还没有回答“周五当天允许做什么、禁止做什么、什么时候可以得出 RC 结论”。
3. 因此当前最合适的动作是先把 `5.2` 落成流程文档，再等待后续跨周执行证据决定是否勾选。

### 解决方案
- 新增 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`，明确周节奏、周五允许/禁止事项、执行顺序、推荐命令、输出物与勾选口径。
- 更新 `docs/README.md` 入口，确保该流程文档可直接检索。
- 同步版本/变更/开发记录，明确 `5.2` 已有执行手册，但任务清单仍保持待完成。

### 本地验收结果
- 新文档已覆盖“周一至周四开发、周五只做收敛”的节奏说明。
- 新文档已明确“允许事项 / 禁止事项 / 输出物 / RC 准备结论”的执行边界。
- 任务清单 `5.2` 本次仍未勾选，待后续形成跨周执行证据后再回写。

### 修改文件
- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md
- docs/README.md
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md


## 问题 64: 补齐 5.2 留痕模板（daily_board / 周报）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `5.2` 已有流程手册，但还缺少可直接填写的“日看板记录卡 + 周报模板”，不利于后续沉淀跨周执行证据。

### 分析记录
1. 仅有 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`，还不足以支撑每周实际执行时的低成本留痕。
2. `daily_board.md` 当前只有任务安排，没有为两个周五预留固定填写位置。
3. 因此需要同时补齐“周五当日记录卡”和“每周汇总模板”两个载体。

### 解决方案
- 更新 `.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`，为 Day 5 / Day 10 增加收敛日记录卡。
- 新增 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`，作为 `5.2` 的周报留痕模板。
- 同步 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`、`docs/README.md` 与相关记录文档，明确这两个模板的入口和用途。

### 本地验收结果
- `daily_board.md` 已可直接填写周五的范围冻结、回归命令、blocker 结论、文档同步和阶段结论。
- `weekly_report_template.md` 已可用于复制生成每周周报实例，并沉淀 `5.2` 的跨周证据。
- 任务清单 `5.2` 本次仍未勾选，仅补齐留痕模板。

### 修改文件
- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md
- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md
- docs/README.md
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md


## 问题 65: 汇总当前功能、使用方式与验证入口

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 当前仓库已经具备较多播放器能力与验收命令，但功能清单、使用方式和验证入口分散在 `README`、`reports`、`tasklist` 与 `main.cpp` 帮助输出中，缺少一份统一总览。

### 分析记录
1. 用户可直接使用的功能，与开发/验收向命令混杂在多个文档和源码帮助输出中，不利于快速理解“程序现在到底能做什么”。
2. 根 `README.md` 只保留了较早期的功能与使用示例，无法完整覆盖当前主线能力。
3. 因此需要新增一份总览文档，把“功能、用法、验证”集中整理，并同步入口索引。

### 解决方案
- 新增 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`，系统整理当前功能、使用方式、配置方式、验证命令与报告映射。
- 更新 `docs/README.md`，为该总览文档增加入口，并在更新历史中记账。
- 更新根 `README.md`，补充到当前总览文档的入口，避免使用说明继续停留在旧口径。

### 本地验收结果
- 新文档已覆盖“用户可直接使用功能 / 开发验收能力 / 使用方式 / 验证流程 / 当前边界”。
- 文档中已给出 `--capabilities`、`--probe-file`、`--evaluate-target` 以及专项验收命令的统一入口。
- 当前仓库没有新增代码改动，本次只补齐功能与使用说明文档。

### 修改文件
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/README.md
- README.md
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md










