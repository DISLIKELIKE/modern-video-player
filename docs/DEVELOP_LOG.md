# 开发日志

## 问题 12: 企业级多线程架构重构

**日期**: 2026-02-27
**状态**: 已完成

### 问题描述
原有架构存在以下问题：
1. 组件职责不清晰，VideoPlayer 承担过多职责
2. 线程模型复杂，难以维护
3. 内存管理容易出错，导致双重释放等 bug

### 解决方案
重构为企业级多线程架构，引入以下新组件：

1. **Demuxer** - 解封装器，封装 AVFormatContext 的读取操作
2. **DecoderWorker** - 解码工作线程，封装单个流的解码逻辑
3. **ThreadSafeQueue** - 线程安全队列模板
4. **Clock** - 时钟同步器，管理音视频同步

### 架构图
```
VideoPlayer (主控制器)
    ├── Demuxer (解封装器) → PacketQueue
    ├── DecoderWorker (视频解码线程)
    ├── DecoderWorker (音频解码线程)
    ├── Display (渲染器)
    ├── AudioPlayer (音频播放)
    └── Clock (时钟同步)
```

---

## 问题 11: 并发读取 AVFormatContext 导致崩溃

**日期**: 2026-02-27
**状态**: 已解决

### 问题描述
播放视频时出现大量 FFmpeg 解码错误和访问冲突崩溃：
```
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
[aac @ ...] Number of scalefactor bands in group (53) exceeds limit (49).
0xC0000005: 写入位置 0x0000022947799000 时发生访问冲突
```
调用堆栈显示错误在 `av_read_frame(format_ctx_, packet)` 处。

### 日志输出
```
Video decoder opened: 1920x1080, format: yuv420p
Audio decoder opened: 48000Hz, 2 channels, fltp
Video decode thread started
Audio decode thread started
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
... (大量类似错误)
```

### 分析记录
1. 问题出现在视频解码线程和音频解码线程同时运行时
2. 两个线程各自拥有独立的 VideoDecoder/AudioDecoder 实例
3. 两个实例共享同一个 `AVFormatContext* format_ctx_`
4. 在 `decodeFrame()` 中都调用了 `av_read_frame(format_ctx_, packet)`
5. 并发读取导致 packet 数据错乱，H264 解码器读取到错误的 NAL 单元

### 根本原因
**并发调用 `av_read_frame()` 导致数据竞争**。`AVFormatContext` 不是线程安全的，两个线程同时读取会导致：
- 读取位置错乱
- Packet 数据被覆盖
- 解码器收到损坏的数据

### 解决方案
引入统一的 `PacketReaderThread` 作为唯一的 packet 读取入口：
1. `PacketReaderThread` 是唯一调用 `av_read_frame()` 的地方
2. 读取到的 packet 根据 stream_index 分发到 `PacketQueue`
3. `VideoDecodeThread` 和 `AudioDecodeThread` 从各自的 `PacketQueue` 获取 packet
4. 解码器新增 `decodePacket()` 方法接收外部传入的 packet

---

## 问题 9: VideoFrame/AudioFrame 移动语义缺陷导致崩溃

**日期**: 2026-02-25
**状态**: 已解决

### 问题描述
程序启动播放后立即崩溃，错误信息：
```
modern-video-player.exe - 应用程序错误
0x00007FFF7A80DA4C 指令引用了 0xFFFFFFFFFFFFFFFF 内存。该内存不能为 read
```

### 日志输出
```
Video decoder opened: 1920x1080, format: yuv420p
Audio decoder opened: 48000Hz, 2 channels, fltp
Playing video...
Video decode thread started
Audio decode thread started
(程序崩溃)
```

### 分析记录
1. 错误地址 0xFFFFFFFFFFFFFFFF = -1，表示空指针/无效指针
2. 崩溃发生在视频解码线程启动后不久
3. 代码审查发现 `VideoFrame` 和 `AudioFrame` 类缺少正确的移动语义实现

### 根本原因
`FrameQueue::pop()` 使用 `std::move` 将帧移动出队列：
```cpp
frame = std::move(queue_.front());
queue_.pop();
```

`VideoFrame` 类没有定义移动构造函数和移动赋值运算符：
- 默认的移动操作只是浅拷贝 `frame_` 指针
- 原对象析构时调用 `av_frame_free(&frame_)` 释放内存
- 目标对象的 `frame_` 变成悬空指针
- 渲染循环访问悬空指针导致崩溃

### 解决方案
为 `VideoFrame` 和 `AudioFrame` 类实现正确的移动语义：
1. 添加移动构造函数，将 `frame_` 指针转移
2. 添加移动赋值运算符，将 `frame_` 指针转移
3. 将原对象的 `frame_` 设为 nullptr，防止析构时释放内存

---

## 问题 13: Core API + Scheduler + Filter 多线程重构落地

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 现有播放器主流程仍以旧架构为中心，缺少规格要求的 `Core API / Scheduler / Filter` 模块化分层。
- 音视频解码线程需要在新核心中独立调度，并通过队列实现无阻塞解耦。

### 分析记录
- 新增 `core` 层：`frame/frame_queue/clock/command/scheduler/player_core`。
- 新增 `filters` 层：`video_filter/audio_filter/filter_registry/filter_pipeline/builtin filters`。
- `VideoPlayer` 增加 `USE_NEW_PLAYER_CORE` 迁移开关路径，保持旧接口不变。

### 修改文件
- include/core/frame.h
- include/core/frame_queue.h
- include/core/clock.h
- include/core/command.h
- include/core/scheduler.h
- include/core/player_core.h
- src/core/frame.cpp
- src/core/clock.cpp
- src/core/scheduler.cpp
- src/core/player_core.cpp
- include/filters/video_filter.h
- include/filters/audio_filter.h
- include/filters/filter_registry.h
- include/filters/filter_pipeline.h
- include/filters/builtin_filters.h
- src/filters/filter_registry.cpp
- src/filters/filter_pipeline.cpp
- src/filters/brightness_filter.cpp
- src/filters/contrast_filter.cpp
- src/filters/saturation_filter.cpp
- src/filters/builtin_filters.cpp
- include/video_player.h
- src/video_player.cpp
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp
- tests/core_clock_tests.cpp

---

## 问题 14: 播放器架构收敛为 Core 单路径

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 项目同时保留旧播放链路和新核心链路，存在维护分叉与并发设计风险。

### 解决方案
- `VideoPlayer` 仅保留 `PlayerCore` 包装实现，删除旧链路分支。
- CMake 删除旧模块编译入口，统一到 `core/*` + `filters/*`。
- 清理旧头源文件，保留必要基础设施和输出模块。
- 新增架构重构文档：`docs/ARCHITECTURE_REFACTOR_2026-03-06.md`。

### 修改文件
- CMakeLists.txt
- include/video_player.h
- src/video_player.cpp
- docs/ARCHITECTURE_REFACTOR_2026-03-06.md
- 删除旧模块文件（decoder/thread/sync/packet/legacy clock/frame_queue）

---

## 问题 15: 小屏窗口过大且拖拽缩放不稳定

**日期**: 2026-03-06
**状态**: 已解决

### 问题描述
- 播放器在小屏设备上按视频原始分辨率（如 1920x1080）直接开窗，窗口初始显示过大。
- 用户拖拽窗口后，部分环境下渲染区域未稳定跟随，表现为“窗口不能正常调整”。

### 日志输出
```
Display initialized: window 1306x734 (source 1920x1080)
```

### 分析记录
- `PlayerCore::open()` 直接把视频分辨率传给 `Display::init()`，没有根据屏幕可用区做首帧窗口缩放。
- 事件处理只监听 `SDL_WINDOWEVENT_RESIZED`，未覆盖常见的 `SDL_WINDOWEVENT_SIZE_CHANGED`。
- 渲染目标矩形直接铺满窗口，缺少按源视频比例计算的目标区域。

### 解决方案
- 在 `Display::init()` 增加基于 `SDL_GetDisplayUsableBounds()` 的初始窗口尺寸计算，保持原始宽高比并限制到屏幕可用区 90%。
- 同时处理 `SDL_WINDOWEVENT_RESIZED` 和 `SDL_WINDOWEVENT_SIZE_CHANGED`，确保拖拽时窗口尺寸状态实时更新。
- 渲染时按视频宽高比计算 `dst_rect`，避免窗口变化时拉伸失真。

### 修改文件
- src/display.cpp

## 问题 16: 窗口最大化/缩放时视频画面卡住，缺少基础控制条

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 窗口最大化或拖动缩放时，视频画面容易卡住，音频仍继续播放。
- 播放器缺少进度条、音量调节和拖动进度的基础能力。

### 日志输出
```text
现象复现：窗口最大化/缩放时视频画面停止刷新，音频继续。
```

### 分析记录
- 运行期 SDL 事件处理与渲染分布在不同线程，窗口变化事件与渲染调用并发时容易出现渲染停滞。
- 播放器 UI 仅支持键盘暂停/退出/全屏，缺少基础交互控件。

### 解决方案
- 将窗口事件处理收敛到渲染路径（`Display::renderFrame`/`PlayerCore::onRenderIdle`）侧，降低缩放与最大化时的渲染阻塞风险。
- `Display` 新增控制层绘制：进度条 + 音量条。
- 新增鼠标交互：拖动进度条发起 seek、拖动音量条实时调节音量。
- `PlayerCore::pumpEvents` 新增对 seek/volume 请求的消费与执行。

### 修改文件
- include/display.h
- src/display.cpp
- src/core/player_core.cpp
- src/main.cpp

## 问题 17: 企业级 MPC-HC 架构剩余模块缺少可落地代码骨架

**日期**: 2026-03-07
**状态**: 已解决（阶段性）

### 问题描述
- `enterprise-quill-logging/tasklist.md` 中模块 02-14 仍有大量未完成项，缺少统一接口与代码落地点。
- 现有播放器主链路未抽象渲染后端，难以扩展 D3D11/OpenGL 等企业级渲染模块。

### 分析记录
- 当前工程已具备 Core + Scheduler + Filter 基础，但缺少跨模块边界定义。
- 需要先补高优先模块骨架并接入构建，再逐步填充完整能力。

### 解决方案
- 新增基础设施：`TaskQueue`、`FramePool`、`DecoderThread`。
- 新增渲染模块：`IVideoRenderer`、SDL 适配器、D3D11/OpenGL 占位实现、`RendererFactory`，并接入 `PlayerCore`。
- 新增音频增强：10 段均衡器、多流混音器，扩展 `AudioPlayer` 缓冲观测接口。
- 新增解码器管理：`IDecoder`、`DecoderCapability`、`DecoderFactory` 自动选择逻辑。
- 新增字幕/播放列表/设置/快捷键/皮肤/插件/格式支持/流媒体等企业级模块骨架。
- 新增滤镜基类与音视频滤镜链，并补充音量平衡滤镜。
- 同步更新 `tasklist.md` 勾选状态（仅勾选已落地代码项）。

### 修改文件
- CMakeLists.txt
- include/core/task_queue.h
- include/core/frame_pool.h
- src/core/frame_pool.cpp
- include/core/decoder_thread.h
- src/core/decoder_thread.cpp
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
- include/filters/filter_base.h
- include/filters/video_filter_chain.h
- src/filters/video_filter_chain.cpp
- include/filters/audio_filter_chain.h
- src/filters/audio_filter_chain.cpp
- include/filters/audio_filter.h
- include/filters/video_filter.h
- include/filters/builtin_filters.h
- src/filters/volume_balance_filter.cpp
- src/filters/builtin_filters.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- include/audio_player.h
- src/audio_player.cpp
- .monkeycode/specs/enterprise-quill-logging/tasklist.md

---

## 问题 18: 编译基线恢复与格式能力矩阵入口

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- `dash_manifest_parser.cpp` 在 VS2022/MSVC 编译失败，阻塞本地持续开发。
- 缺少可直接运行的能力检查入口，不利于快速验证“主力格式 + 高分高帧 + 多音道”目标。

### 解决方案
- 修复 DASH 正则 raw-string 分隔符，恢复工程可编译。
- 新增命令行能力入口：
  - `--capabilities`：输出运行时容器/编解码器能力与主力格式覆盖矩阵。
  - `--evaluate-target`：评估指定分辨率/帧率/声道/码率目标是否建议硬解与 D3D11 渲染。
- 增强播放链路基础能力：
  - `Demuxer` 使用 `av_find_best_stream` + probe 参数增强；
  - `AudioPlayer` 暴露实际输出参数；
  - `PlayerCore` 复用 `SwrContext`，按设备输出参数进行重采样。

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

---

## 问题 19: D3D11VA 硬解最小闭环（失败回退软解）

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 需要在 Windows 下优先使用 D3D11VA 解码高负载视频，同时避免硬解失败导致无法播放。
- 硬件输出帧格式与 SDL 渲染链路不一致（GPU/NV12 vs YUV420P），需要统一输出格式。

### 解决方案
- 在 `PlayerCore::initDecoders` 中加入 D3D11VA 配置与选择逻辑。
- 当硬解 `avcodec_open2` 失败时，自动重建解码器并回退软解继续播放。
- 在视频解码路径补充硬件帧转存与像素格式规整：
  - `av_hwframe_transfer_data`（硬件帧 -> 系统内存帧）
  - `sws_scale`（非 YUV420P -> YUV420P）

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp

---

## 问题 20: `--probe-file` 与格式回归脚本

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 缺少单文件可脚本化探测入口，不便于快速定位“某个样本为什么不达标”。
- 缺少批量回归脚本，不利于单人开发迭代中持续追踪格式兼容性退化。

### 解决方案
- 新增命令：`modern-video-player.exe --probe-file <media_file>`，输出 `probe.*` 键值结果，便于脚本解析。
- 新增 `tools/format_regression/run_format_regression.ps1`：按 CSV 样本清单批量探测并输出 Markdown 报告。
- 报告默认路径：`docs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`。
- 返回码约定：`0=PASS`，`1=PARTIAL`，`2=FAIL`。

### 验证记录
- `.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4` 返回 `probe.overall=PASS`。
- `.\tools\format_regression\run_format_regression.ps1` 成功生成报告。
- `-OutputFile` 自定义路径验证通过。

### 修改文件
- src/main.cpp
- tools/format_regression/run_format_regression.ps1
- tools/format_regression/format_samples.csv
- docs/FORMAT_REGRESSION.md
- docs/README.md

---

## 问题 21: GitHub Actions 自动回归接入

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 当前格式回归仅在本地手动执行，PR 缺少自动化门禁。
- Windows CI 中依赖发现方式与本地 `external/` 目录方案不同，存在构建不一致风险。

### 解决方案
- 新增 `.github/workflows/format-regression.yml`：
  - 自动下载 `SDL2/FFmpeg` 预编译依赖并构建 `build/Debug/modern-video-player.exe`；
  - 调用 `tools/download_test_samples.ps1` 生成样本；
  - 调用 `tools/run_all_checks.ps1` 执行单文件探测 + 批量回归；
  - 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 产物。
- 调整 `CMakeLists.txt`（Windows）：
  - 优先支持 `SDL2::`、`FFMPEG::`、`unofficial::ffmpeg::` 导入目标链接；
  - 继续保留 `external/SDL2` 与 `external/ffmpeg` 回退路径。
- 调整 `tools/download_test_samples.ps1`：
  - 新增 PATH 可执行解析逻辑，支持 `-FfmpegPath ffmpeg`。
- 同步更新回归文档与任务清单已完成项。

### 修改文件
- .github/workflows/format-regression.yml
- CMakeLists.txt
- tools/download_test_samples.ps1
- docs/FORMAT_REGRESSION.md
- docs/REGRESSION_OPERATION_PLAYBOOK.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 22: M1 主链路推进（播放列表 + 设置 + 快捷键首版）

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 当前主流程只有单文件播放，`PlaylistManager`/`SettingsManager` 虽有模块但未接入。
- 关键快捷键未覆盖任务清单目标，用户交互效率不足。

### 解决方案
- 在 `main` 主流程接入播放列表：
  - 支持多文件参数构建播放列表；
  - 支持 `.m3u8` 加载；
  - 支持上一首/下一首与 EOF 自动下一首。
- 在 `main` 接入设置持久化：
  - 启动加载 `config/player_settings.ini`；
  - 设置失败回退默认；
  - 退出保存音量、速度、索引。
- 扩展渲染事件请求链路（Display -> Renderer -> PlayerCore -> VideoPlayer -> main）：
  - seek 增量请求、速度调整请求、列表切换请求、退出请求；
  - 实现任务清单首版默认键位集（`Space/Enter/Esc/Q/Left/Right/Ctrl+Left/Ctrl+Right/Up/Down/M/PageUp/PageDown/[ ]/R`）。

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

---

## 问题 23: 移除 Core 单元测试目标与测试文件

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 需求要求移除两个 Core 测试及相关文件。
- `CMakeLists.txt` 中仍存在测试目标与开关，直接删除文件会留下构建残留。

### 分析记录
1. 已确认待移除测试文件为 `tests/core_frame_queue_tests.cpp` 与 `tests/core_clock_tests.cpp`。
2. 已确认构建脚本中存在 `BUILD_CORE_TESTS`、`core_frame_queue_tests`、`core_clock_tests` 与 `core_tests` 聚合目标引用。
3. 需要同步清理 CMake 配置与文档记录，保证仓库状态一致。

### 解决方案
- 清理 `CMakeLists.txt` 中的两个测试目标、聚合目标与测试开关。
- 删除两个测试文件。
- 同步更新 `CHANGELOG.md`、`VERSION.md`、`DEVELOP_LOG.md`。

### 修改文件
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp（删除）
- tests/core_clock_tests.cpp（删除）
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 24: 外挂字幕加载入口（SRT）接入主流程

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 任务清单 `1.1.1` 需要提供外挂字幕加载入口。
- 现有代码虽然有 `subtitle::SrtParser`，但主流程没有任何参数入口或播放器加载接口。

### 分析记录
1. 字幕解析模块已具备（`include/subtitle/srt_parser.h` + `src/subtitle/srt_parser.cpp`）。
2. `main` 参数解析仅支持媒体输入、能力查询和 probe，缺失字幕入口。
3. `VideoPlayer` 不持有外挂字幕状态，无法在播放会话中追踪已加载字幕。

### 解决方案
- `VideoPlayer` 新增外挂字幕加载能力（仅 SRT，含容错）：
  - `loadExternalSubtitle()` / `clearExternalSubtitle()`；
  - 记录已加载字幕路径和条目数。
- `main` 新增 `--subtitle <file.srt>` 入口，并保留播放列表参数能力。
- 若未显式传 `--subtitle`，自动尝试加载同名 `.srt` 文件。
- 任务清单 `1.1.1` 标记完成。

### 修改文件
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 25: 字幕渲染叠加与播放时序同步接入

**日期**: 2026-03-07
**状态**: 已解决

### 问题描述
- 任务清单 `1.1.2` 要求字幕渲染叠加，但当前渲染接口只有画面与控制层，没有字幕通道。
- 任务清单 `1.1.3` 要求字幕与播放/暂停/seek 同步，但播放核心未按时钟驱动字幕更新。

### 分析记录
1. `VideoPlayer` 已具备外挂字幕加载能力（问题 24），但字幕数据未进入渲染链路。
2. 渲染层抽象缺失字幕接口，SDL/D3D11/OpenGL 后端无法统一接收字幕文本。
3. 播放核心需要在渲染帧与空闲事件都更新字幕，才能覆盖暂停态和 seek 后静止画面。
4. 字幕更新实现中存在锁内调用渲染接口风险，需要调整锁粒度。

### 解决方案
- 渲染抽象扩展：
  - `IVideoRenderer` 增加 `setSubtitleText()`；
  - SDL 后端转发到 `Display`；
  - D3D11/OpenGL 增加兼容桩实现。
- `Display` 新增字幕叠加层：
  - 新增字幕状态与互斥保护；
  - 在每帧渲染中叠加字幕面板与文本；
  - 支持多行与超长文本处理。
  - 当前字模为轻量实现，非 ASCII 字符降级显示。
- `PlayerCore` 新增字幕时间轴逻辑：
  - 引入外挂字幕轨道状态管理；
  - 在 `renderFrame()` 与 `onRenderIdle()` 中调用 `updateSubtitleOverlay()`；
  - 依据当前播放时间选择活跃字幕，支持播放/暂停/seek 同步。
- 修复并发细节：
  - 将渲染调用移出字幕互斥锁，避免锁内回调渲染层。
- 更新任务清单：
  - `1.1.2`、`1.1.3` 标记完成。

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

---

## 问题 26: 字幕开关控制与字幕加载异常处理完善

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.1.4` 要求“字幕开关与异常处理”，但当前缺少运行时字幕开关能力。
- 字幕加载流程在文件系统异常场景下缺少专门容错，稳定性不足。

### 分析记录
1. 现有输入事件链路（`Display -> Renderer -> PlayerCore`）可扩展请求型控制，适合加入字幕开关。
2. 字幕时间轴更新已在 `PlayerCore` 内闭环，新增“开关状态”即可复用现有同步逻辑。
3. `VideoPlayer::loadExternalSubtitle()` 使用默认 `filesystem` 检查方式，存在异常传播风险。

### 解决方案
- 新增字幕开关控制：
  - 在 `Display` 中新增字幕开关请求并绑定 `V` 键；
  - 在渲染器接口新增 `consumeToggleSubtitleRequest()`；
  - 在 `PlayerCore` 中新增 `setSubtitleEnabled()/toggleSubtitleEnabled()/isSubtitleEnabled()`；
  - 关闭字幕时主动清空渲染文本，开启字幕后按当前时间重新选取字幕。
- 加强异常处理：
  - 字幕路径检查改用 `std::error_code`；
  - 捕获字幕解析异常并写告警日志；
  - 加载失败保持主流程可播放，不保留脏字幕状态。
- 同步任务状态：
  - `1.1.4` 标记完成。

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

---

## 问题 27: 快捷键配置持久化接入（hotkey.*）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.3.2` 需要支持键位配置持久化。
- 当前快捷键逻辑直接写在 `Display` 的 `SDL_KEYDOWN` 分支，无法从配置加载并在重启后保持。

### 分析记录
1. 项目已有 `HotkeyManager`，但默认键位与主流程实际键位不一致，且未接入播放链。
2. `SettingsManager` 已能读写 `player_settings.ini`，可复用为 `hotkey.*` 持久化通道。
3. 需要打通 `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 的热键映射透传链路。

### 解决方案
- 扩展 `HotkeyManager`：
  - 对齐首版默认动作与键位；
  - 新增动作键名和键码序列化/反序列化能力。
- 事件链路接入：
  - `Display` 改为通过 `HotkeyManager` 解析键位动作；
  - 渲染器接口新增 `setHotkeyManager()`；
  - `PlayerCore` 与 `VideoPlayer` 增加热键管理 API 并透传到 SDL 渲染器。
- 持久化接入：
  - `main` 新增 `hotkey.*` 配置加载与回写；
  - 非法配置自动降级默认并记录告警；
  - 更新 `config/player_settings.ini` 默认样例。
- 任务状态同步：
  - `1.3.2` 标记完成。

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

---

## 问题 28: 快捷键冲突检测与恢复默认能力

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.3.3` 需要支持“键位冲突检测与恢复默认”。
- 当前已具备 `hotkey.*` 持久化，但缺少冲突治理，重复键位会导致动作语义不清晰。

### 分析记录
1. `HotkeyManager` 已有动作与键位映射，适合扩展冲突扫描接口。
2. 配置加载流程在 `main` 中集中处理，适合统一加入冲突检测与恢复策略。
3. 需要一个可显式触发恢复默认的配置通道，便于用户自救。

### 解决方案
- 在 `HotkeyManager` 增加：
  - `resetToDefaults()`；
  - `findConflicts()` / `hasConflicts()`。
- 在热键加载流程增加冲突治理：
  - 加载配置后先构建候选映射；
  - 检测到冲突则记录详细日志并自动回退默认键位；
  - 非法 token 保留默认并给出告警。
- 增加恢复默认配置开关：
  - `hotkey.restore_defaults=true` 时启动自动恢复默认；
  - 恢复后自动置回 `false`。
- 任务清单同步：
  - `1.3.3` 标记完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 29: M1 验收 1.4.1（SRT seek 同步自检命令落地）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.4.1` 需要验证 SRT 字幕在 seek 后仍与播放时间同步。
- 当前缺少可重复的本地验收命令，无法稳定做回归。

### 分析记录
1. `PlayerCore::updateSubtitleOverlay()` 已具备缓存索引 + 二分定位逻辑，但无法脱离播放 UI 独立验证。
2. 需要把字幕时间轴匹配算法提取为可复用函数，并提供命令行自检入口。

### 解决方案
- 新增 `subtitle::resolveActiveSubtitleIndex(...)` 并由 `PlayerCore` 统一复用。
- 新增 `--subtitle-sync-check <subtitle.srt>`：
  - 有序时间轴检查；
  - seek 跳转检查；
  - 输出 mismatch 统计与 PASS/FAIL。
- 新增样例文件 `samples/subtitle/subtitle_seek_sync_sample.srt` 与本地报告 `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`。
- 更新任务清单，勾选 `1.4.1`。

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

---

## 问题 30: M1 验收 1.4.2（播放列表连续播放 5 文件自检）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.4.2` 需要验证播放列表连续播放 5 文件。
- 缺少可重复执行的自动化验收入口。

### 分析记录
1. 主流程已支持 EOF 自动切换下一条目，但运行结果未结构化输出。
2. 现有 `--probe-file` 已具备稳定的媒体可打开检测能力，可复用为验收前置检查。

### 解决方案
- 新增 `--playlist-flow-check`：
  - 构建播放列表并要求至少 5 条目；
  - 检查前 5 条的可打开状态；
  - 按 EOF 自动切换逻辑验证顺序覆盖 `0,1,2,3,4`；
  - 输出 `playlist-flow-check.result=PASS/FAIL`。
- 新增本地报告：`docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`。
- 任务清单 `1.4.2` 标记完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md
- samples/README.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 31: M1 验收 1.4.3（设置重启恢复自检）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `1.4.3` 需要验证设置在重启后可恢复。
- 当前缺少可重复的命令行验收入口。

### 分析记录
1. `loadAppSettings/saveAppSettings` 已覆盖播放器核心设置和热键持久化。
2. 需要新增独立命令将“写入->重载->比对”过程结构化输出。

### 解决方案
- 新增 `--settings-persistence-check [settings_file]`。
- 使用测试配置执行 round-trip：
  - `volume`
  - `playback_speed`
  - `resume_last_playlist`
  - `last_playlist_index`
  - `toggle_subtitle` 热键
- 输出每个字段的 `*_ok` 结果与总结果。
- 默认使用临时路径，避免污染实际配置。
- 任务清单 `1.4.3` 标记完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 32: M2 2.1.2（容器矩阵补齐 mov/avi/m2ts）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `2.1.2` 需要覆盖 `mp4/mkv/mov/avi/webm/flv/ts/m2ts` 容器。
- 当前回归样本缺失 `mov/avi/m2ts`。

### 分析记录
1. `FormatSupport` 已声明容器支持，但缺少对应回归样本无法形成验收闭环。
2. 样本生成脚本仅生成 5 类容器，需同步扩展。

### 解决方案
- 扩展 `format_samples.csv` 新增 `mov/avi/m2ts` 三条样本。
- 扩展 `download_test_samples.ps1` 生成三类新样本。
- 更新样本目录规则与文档。
- 运行 `run_format_regression.ps1` 产出最新本地报告。
- 任务清单 `2.1.2` 标记完成。

### 本地验收结果
- `FORMAT_REGRESSION_LOCAL_CHECK.md`：
  - Total=12
  - PASS=12
  - PARTIAL=0
  - FAIL=0
  - SKIP=0

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

---

## 问题 33: M2 2.1.3（视频编码矩阵补齐 MPEG-2）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `2.1.3` 需要覆盖 `H.264/H.265/VP9/AV1/MPEG-2` 视频编码。
- 回归样本尚缺 `MPEG-2`，无法完成完整验收。

### 分析记录
1. 现有矩阵包含 h264/hevc/vp9/av1。
2. `FormatSupport` 已包含 `mpeg2video`，缺口在样本和回归链路。

### 解决方案
- 新增 `mpeg2video` 回归样本条目（ts 容器，ac3 音频）。
- 扩展 `download_test_samples.ps1` 自动生成 MPEG-2 样本。
- 运行回归并更新本地报告。
- 任务清单 `2.1.3` 标记完成。

### 本地验收结果
- `FORMAT_REGRESSION_LOCAL_CHECK.md`：
  - Total=13
  - PASS=13
  - PARTIAL=0
  - FAIL=0
  - SKIP=0
- 新增样本结果：
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts` -> `PASS (mpeg2video)`

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 34: M2 2.1.4（音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `2.1.4` 需要覆盖 `AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`。
- 样本矩阵缺少 `E-AC3/DTS/Vorbis/PCM`。

### 分析记录
1. 现有样本已覆盖 aac/mp3/ac3/flac/opus。
2. DTS 编码器 `dca` 在当前 FFmpeg 中属于实验特性，需要 `-strict -2`。
3. 兼容性比对需处理 `dts/dca`、`hevc/h265`、`pcm/pcm_*` 等价关系。

### 解决方案
- 扩展回归样本与生成脚本，新增四类音频样本。
- 在 `download_test_samples.ps1` 为 DTS 命令增加 `-strict -2`。
- 在 `run_format_regression.ps1` 增加等价编码名比较函数。
- 更新本地回归报告并确认全量通过。
- 任务清单 `2.1.4` 标记完成。

### 本地验收结果
- `FORMAT_REGRESSION_LOCAL_CHECK.md`：
  - Total=17
  - PASS=17
  - PARTIAL=0
  - FAIL=0
  - SKIP=0

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- tools/format_regression/run_format_regression.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 35: M3 3.1.1（DecoderFactory 接入真实初始化流程）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `3.1.1` 要求 `DecoderFactory` 接入真实解码初始化流程。
- 现状是 `PlayerCore` 主要走内嵌分支逻辑，`DecoderFactory` 只在硬解配置局部被动参与，未形成统一候选链路。

### 分析记录
1. `DecoderFactory` 已有能力探测与优先级，但缺少“后端候选序列”接口。
2. `PlayerCore::initDecoders` 需要按候选序列逐个尝试，以实现统一初始化与回退。
3. `3.1.3` 已提供“是否偏好硬解”配置，需要保留并复用。

### 解决方案
- `DecoderFactory` 新增 `selectBackendOrder(codec_name, prefer_hardware)`：
  - 生成按优先级排序的解码后端候选序列；
  - 保证软件解码兜底在候选链路中。
- `PlayerCore::initDecoders` 接入候选序列：
  - 逐个后端尝试初始化与 `avcodec_open2`；
  - 失败自动切换下一个候选；
  - 成功后统一记录最终后端日志。
- `tryConfigureD3D11HardwareDecode` 去除内部后端选择判断，改为纯 D3D11 配置职责，选择策略由 `DecoderFactory` 统一决定。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

### 修改文件
- include/decoder/decoder_factory.h
- src/decoder/decoder_factory.cpp
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 36: M3 3.1.2（D3D11VA 初始化失败回退软解兜底）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `3.1.2` 要求 D3D11VA 初始化失败时必须可靠回退到软解。
- 现有链路在像素格式协商回退场景下仅打印日志，后端状态未显式切换为软解，存在状态不一致风险。

### 分析记录
1. `PlayerCore::selectVideoPixelFormat` 在未命中 D3D11VA 像素格式时会返回软件像素格式。
2. 该路径此前未同步更新 `video_decoder_backend_`，可能导致后续硬件绑定清理条件判断不准确。

### 解决方案
- 在 `selectVideoPixelFormat` 中补充显式降级：
  - 将 `video_hw_pixel_fmt_` 重置为 `AV_PIX_FMT_NONE`；
  - 将 `video_decoder_backend_` 设置为 `Software`。
- 在 `initDecoders` 的后端尝试链路中新增降级日志：
  - 当 D3D11VA 初始化过程中被协商为软件路径时，记录“降级为软解”提示。
- 同步更新任务清单 `3.1.2` 为完成。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --capabilities` 通过（主力矩阵 `PASS`）。

### 修改文件
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 37: M3 3.2.1（D3D11 渲染最小可用链路）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `3.2.1` 要求 D3D11 渲染最小可用（`init/upload/present`）。
- 现有 `D3D11VideoRenderer` 为桩实现，`init` 固定失败，无法形成可用渲染后端。

### 分析记录
1. 现有 `Display` 已具备稳定的 owner-thread 渲染与帧上传链路，可复用于 D3D11 最小闭环。
2. 需要可观测当前 SDL renderer 后端，以判断是否真正启用了 `direct3d11`。
3. 若实际后端不是 D3D11，应在 `D3D11VideoRenderer::init` 失败并交给上层触发 `3.2.2` 自动回退。

### 解决方案
- `Display` 新增渲染驱动控制与观测接口：
  - `setPreferredRendererDriver()`：允许在创建 renderer 前设置偏好驱动；
  - `currentRendererDriver()` / `isUsingRendererDriver()`：返回当前实际 renderer 后端。
- `D3D11VideoRenderer` 改为最小可用实现：
  - `init`：创建 `Display`，指定 `direct3d11` 偏好，完成窗口/渲染初始化；
  - `renderFrame/present/clear`：接通帧上传与呈现；
  - 事件与控制请求接口统一透传 `Display`。
- `init` 阶段增加后端校验：
  - 若实际 renderer 后端非 `direct3d11/d3d11`，主动返回失败并记录日志，触发上层 SDL 回退。
- 任务清单 `3.2.1` 标记完成。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

### 修改文件
- include/display.h
- src/display.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 38: M3 3.3.2（渲染失败降级不中断播放）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `3.3.2` 要求渲染失败时可自动降级且不中断播放。
- 需要一个可重复执行的本地验收入口来稳定验证 D3D11 失败后的 SDL 回退链路。

### 分析记录
1. `3.2.1/3.2.2` 已具备 D3D11 初始化与 SDL 回退机制，但缺少自动化验收命令。
2. 需要在不依赖人工操作的前提下，强制制造 D3D11 renderer 初始化失败场景。

### 解决方案
- 新增渲染/解码后端可观测接口（renderer backend / decoder backend）。
- 新增命令 `--renderer-fallback-check <media_file>`：
  - 临时注入 `MVP_D3D11_DRIVER_HINT=software`，强制 D3D11 渲染初始化失败；
  - 通过播放器主链路验证是否自动回退到 `SoftwareSDL`；
  - 输出结构化字段与 `PASS/FAIL`。
- 新增本地报告：`docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`。
- 更新任务清单，标记 `3.3.2` 完成。

### 本地验收结果
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`
  - `open_ok=true`
  - `renderer_backend=SoftwareSDL`
  - `entered_playback_loop=true`
  - `fallback_to_sdl=true`
  - `result=PASS`

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

---

## 问题 39: M3 3.3.1（Windows 软解+硬解主力样本可播）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `3.3.1` 要求在 Windows 下验证硬解（D3D11VA）和软解（Software）主力样本均可播。
- 原聚合命令在同进程顺序执行双会话时，出现停止阶段卡住，导致命令超时。

### 日志输出
```text
windows-backend-check.command=... 
... The filename, directory name, or volume label syntax is incorrect.
windows-backend-check.result=FAIL
```

### 分析记录
1. 同进程连续跑 hard/soft 会话时，第二轮存在回收链路不稳定。
2. `std::system` + 重定向在当前路径组合下存在 shell 解析不稳定。
3. 需要将会话隔离并改为更稳定的子进程创建方式。

### 解决方案
- 新增 `--windows-backend-session-check <media_file> <hard|soft>`，每次只验证一个会话并输出结构化字段。
- 将 `--windows-backend-check` 改为父进程拉起两个子进程（hard、soft）并汇总结果。
- Windows 下用 `CreateProcess` + 文件句柄重定向采集子进程输出，避免 shell 语法问题。
- 增加本地报告 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 hard` 通过（PASS）。
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 soft` 通过（PASS）。
- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4` 通过（PASS）。
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4` 通过（PASS）。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（PASS）。

### 任务清单同步
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
  - `3.3.1` 标记完成。
  - `3.3.3` 标记完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 40: M4 4.1（章节导航：上一章/下一章）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `4.1` 要求提供章节导航能力（上一章/下一章）。
- 当前播放主链路没有章节请求动作和跳章入口，无法从键盘直接跳章。

### 分析记录
1. 章节元数据来自 `AVFormatContext::chapters`，需要在 `Demuxer` 开档阶段提取并保存在媒体信息中。
2. 输入事件链路需新增章节动作透传：`Display -> Renderer -> PlayerCore -> VideoPlayer`。
3. 需要新增命令行自检入口，避免章节导航仅依赖手工播放验证。

### 解决方案
- `Demuxer` 新增章节模型：
  - 增加 `ChapterInfo` 结构与 `MediaInfo::chapters`；
  - 解析 `AVChapter` 的起止时间和标题，并按起始时间排序。
- 快捷键与请求链路扩展：
  - `HotkeyManager` 新增 `PreviousChapter` / `NextChapter`；
  - 默认键位绑定 `HOME` / `END`；
  - `Display`、`IVideoRenderer` 及各渲染器实现新增章节请求消费接口。
- `PlayerCore` 增加章节跳转逻辑：
  - `rebuildChapterPoints()` 在 `open()` 后构建章节时间点；
  - 新增 `seekToNextChapter()` / `seekToPreviousChapter()`；
  - `pumpEvents()` 消费章节请求并触发 seek。
- `VideoPlayer` 增加章节 API 包装：
  - `seekToNextChapter()` / `seekToPreviousChapter()` / `chapterCount()`。
- `main` 新增 `--chapter-nav-check <media_file>`：
  - 自动执行播放、下一章、上一章流程；
  - 输出 `chapter-nav-check.*` 字段与 `PASS/FAIL`。
- 任务清单同步：
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.1` 完成。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

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

---

## 问题 41: M4 4.2（A-B Repeat）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `4.2` 要求支持 A-B Repeat。
- 当前主流程缺少 A/B/C 区间循环控制，无法设置 A 点、B 点并重复播放区间。

### 分析记录
1. 热键与事件请求链路已具备可扩展模式，可复用为 A-B Repeat 请求透传。
2. `PlayerCore` 已有 seek 与播放位置状态，适合新增区间状态并在播放中触发回跳。
3. 需要新增命令行自检入口，确保区间循环可稳定回归。

### 解决方案
- 新增热键动作与默认绑定：
  - `SetABRepeatStart`（`A`）；
  - `SetABRepeatEnd`（`B`）；
  - `ClearABRepeat`（`C`）。
- 扩展链路：
  - `Display` 增加 A-B Repeat 请求标记/消费；
  - `Renderer` 抽象与 SDL/D3D11/OpenGL 实现新增对应透传接口。
- `PlayerCore` 增加 A-B Repeat 控制能力：
  - `setABRepeatStart()`、`setABRepeatEnd()`、`clearABRepeat()`；
  - `isABRepeatEnabled()`、`abRepeatStart()`、`abRepeatEnd()`；
  - `handleABRepeatLoop()` 在播放中检测到达 B 点后自动 seek 回 A 点。
- `VideoPlayer` 增加 A-B Repeat API 包装。
- `main` 新增 `--ab-repeat-check <media_file>` 验收命令与帮助信息。
- 修复回归检查冲突：
  - `--settings-persistence-check` 测试键位由 `b` 改为 `x`，避免与新默认 `B` 热键冲突。
- 任务清单同步：
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.2` 完成。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 通过（`PASS`）。

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

---

## 问题 42: M4 4.3（截图）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `4.3` 需要支持截图。
- 当前截图链路仍处于 WIP：热键、主流程与 `--screenshot-check` 已接入，但暂停态请求缺少“最后显示帧”缓存，导致暂停时无法稳定保存当前画面。

### 分析记录
1. `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 的请求链路已经具备，只差播放器核心对“最近一帧”的保留能力。
2. 现有实现只在渲染线程处理截图请求；一旦进入暂停态，调度器停止继续送帧，截图请求就拿不到新的图像数据。
3. 要让截图成为可用功能，除了补齐缓存外，还需要把自检改成覆盖“暂停态截图”，否则无法证明根因已修复。

### 解决方案
- `PlayerCore` 新增最近一次渲染帧缓存：
  - `updateLastRenderedFrame()` 在每次呈现后缓存当前帧；
  - `captureScreenshotFromCachedFrame()` 支持在暂停态直接截图；
  - `clearLastRenderedFrame()` 在重新打开/关闭媒体时清理缓存。
- `requestScreenshot()` 调整为：
  - 播放中：维持异步请求；
  - 非播放态（重点是暂停态）：直接从缓存帧落盘。
- `--screenshot-check` 调整为“先播放 -> 暂停 -> 请求截图 -> 校验输出文件”，确保本次修复有针对性验证。
- 文档与任务清单同步：
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.3` 完成；
  - 新增本地报告 `docs/reports/SCREENSHOT_LOCAL_CHECK.md`；
  - 更新 `README.md` / `README_ZH.md` 的快捷键说明。

### 本地验收结果
- `cmake --build build --config Debug --target modern-video-player` 通过。
- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 通过（`PASS`）。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- README.md
- README_ZH.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SCREENSHOT_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 43: `MPC_HC_GAP_ANALYSIS` 评估结论过期

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `docs/MPC_HC_GAP_ANALYSIS.md` 仍停留在 2026-03-07 之前的评估口径。
- 文档把字幕、快捷键、设置、播放列表、解码器管理等能力写成“未接入/骨架”，与当前代码和本地验收报告不一致。

### 分析记录
1. `M1`、`M3` 与 `M4 4.1/4.2/4.3` 已陆续落地，但差距评估文档没有同步刷新。
2. 继续沿用旧结论会误导后续优先级判断，特别是会把已完成项继续列入 `P0/P1`。
3. 差距评估文档需要同时参考代码入口和 `docs/reports/*` 验收报告，而不是只看类/接口是否存在。

### 解决方案
- 将 `docs/MPC_HC_GAP_ANALYSIS.md` 全文更新到 2026-03-08 版本。
- 重写模块总览、模块统计、剩余差距清单与建议里程碑。
- 把字幕、播放列表、设置、快捷键、解码器管理、截图等已落地能力从“骨架/未接入”纠正为“部分实现”。
- 新增“验收与报告证据”章节，明确评估依据包含本地回归报告。
- 更新文档索引，补一条本次差距评估对齐说明。

### 本地校对结果
- `docs/MPC_HC_GAP_ANALYSIS.md` 标题日期已更新为 `2026-03-08`。
- 模块统计更新为：`部分实现 11 / 14`、`骨架/未接入 3 / 14`。
- `P0/P1/P2` 剩余项已与当前任务清单和验收报告保持一致。

### 修改文件
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## 问题 44: `docs/VERSION.md` 历史路径描述过期

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `docs/VERSION.md` 的早期阶段描述仍保留旧版 decoder/thread/test 文件级表述，看起来像当前仓库仍在使用这些路径。
- 这会影响按文档遍历仓库时的进度判断，也容易把历史实现和当前主链结构混为一谈。

### 分析记录
1. `2026-03-06` 架构收敛后，播放器主链已经切到 `PlayerCore + Scheduler + core/*`，但版本文档的历史章节没有同步去歧义。
2. “阶段一：基础播放器 (当前阶段)” 与“下一步计划”等措辞已经不再符合 `2026-03-08` 的实际状态。
3. 历史章节应该保留能力演进和问题修复记录，但不应再把已删除文件当作当前仓库结构说明。

### 解决方案
- 将阶段一改写为“历史起点”，并补充旧版 `decoder / playLoop` 架构已并入当前主链的说明。
- 将 `video_decoder` / `audio_decoder`、`VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 等旧路径改写为能力级历史表述。
- 将“下一步计划”、`USE_NEW_PLAYER_CORE`、临时 `tests/core_*` 的相关描述调整为历史记录口径。
- 在 `docs/VERSION.md` 的文档更新日志中补记本次清理。

### 本地校对结果
- `docs/VERSION.md` 不再出现“当前阶段”“下一步计划”等易误导当前状态的旧口径。
- 早期章节对旧版 decoder/thread 的表述已改为历史说明，并指向当前 `core/*` 主链。
- 本次变更限定在文档层，未扩散到代码或构建配置。

### 修改文件
- docs/VERSION.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 45: README 与架构文档仍混用旧主链表述

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 根目录 `README.md` / `README_ZH.md` 仍把 `video_decoder`、`audio_decoder` 等旧文件写成项目当前结构。
- `docs/ARCHITECTURE.md` 同时混用了“当前实现”和旧模块命名，读者容易误判现行主链仍依赖这些历史模块。

### 分析记录
1. `2026-03-06` 架构收敛后，当前播放器主链已经稳定在 `VideoPlayer + PlayerCore + Scheduler + core/*`。
2. `README` 更偏向“快速理解仓库”的入口文档，因此目录树和架构示意应优先反映现状，而不是保留已删除路径。
3. `docs/ARCHITECTURE.md` 作为背景文档可以保留历史内容，但必须显式标注哪些章节属于历史实现。

### 解决方案
- 重写 `README.md` / `README_ZH.md` 的项目结构树和架构示意，改为当前主链口径。
- 在 `docs/ARCHITECTURE.md` 顶部增加状态说明，并将旧 decoder/thread/sync 章节统一标成“历史实现”。
- 将文档中的日志示例改为当前 `Quill` 宏接口。
- 更新 `docs/README.md` 的架构文档索引说明，区分历史基线和当前重构说明。

### 本地校对结果
- `README.md` / `README_ZH.md` 已不再把 `video_decoder` / `audio_decoder` 写为当前目录结构。
- `docs/ARCHITECTURE.md` 已明确声明历史章节边界，并不再把旧多线程链路标为“当前实现”。
- 本次改动仍限定在文档层，未涉及代码和构建配置。

### 修改文件
- README.md
- README_ZH.md
- docs/ARCHITECTURE.md
- docs/README.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 46: 实现教程与迭代计划缺少历史/当前边界说明

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `docs/IMPLEMENTATION.md` 保留的是早期原型的从零实现教程，但没有明确声明其中 `video_decoder`、`audio_decoder`、单体 `playLoop` 等内容属于历史实现。
- `docs/MPC_HC_ITERATION_PLAN.md` 记录的是 `2026-03-07` 的计划基线，但没有提示其中部分能力已在后续提交中落地。

### 分析记录
1. 这两份文档都还有使用价值：前者偏教程，后者偏规划；问题在于缺少和“当前代码状态”的显式分界。
2. 经过前几轮清理后，`README`、`VERSION`、`ARCHITECTURE` 已经区分历史与当前，如果这两份文档不跟进，读者仍会在入口层产生混淆。
3. 最合适的做法不是重写全文，而是在文档顶部补充状态说明，并把索引描述统一到同一口径。

### 解决方案
- 为 `docs/IMPLEMENTATION.md` 增加“状态说明（2026-03-08）”，明确其为早期原型教程，并指向当前主链参考文档。
- 为 `docs/MPC_HC_ITERATION_PLAN.md` 增加“状态说明（2026-03-08）”，明确其为 `2026-03-07` 的计划快照，并列出当前进度参考入口。
- 更新 `docs/README.md`、`README.md`、`README_ZH.md` 的文档说明，使历史教程、计划快照、当前主链说明三者边界一致。

### 本地校对结果
- `docs/IMPLEMENTATION.md` 已不再被表述为当前仓库的逐文件实施清单。
- `docs/MPC_HC_ITERATION_PLAN.md` 已明确为计划快照，并指向当前进度来源。
- 本次改动仍限定在文档层，未涉及代码、构建和任务清单状态修改。

### 修改文件
- README.md
- README_ZH.md
- docs/IMPLEMENTATION.md
- docs/MPC_HC_ITERATION_PLAN.md
- docs/README.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 47: 辅助说明文档仍缺少当前入口与状态边界

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- `docs/FILTERS.md`、`docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`、`docs/WINDOWS_SETUP.md` 仍偏向“静态说明”，缺少与当前代码主链、依赖探测方式和文档适用范围的衔接说明。
- `docs/WINDOWS_SETUP.md` 还保留了旧的 `SDL2_DIR` / `FFMPEG_DIR` 配置示例，与当前 `CMakeLists.txt` 的手动依赖回退路径不完全一致。

### 分析记录
1. 经过前几轮清理后，入口文档和核心设计文档已经区分了“历史基线”和“当前主链”，但辅助说明文档还没有完全跟上。
2. `FILTERS.md` 的主要问题不是错误，而是缺少“当前默认主流程入口是 `FilterPipeline`”这一层解释。
3. `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 更像专题参考笔记，需要提醒读者不要把其中的目标项直接等同于当前未完成项。
4. `WINDOWS_SETUP.md` 则需要与现有 `CMakeLists.txt` 的依赖探测顺序保持一致，否则会影响实际搭建体验。

### 解决方案
- 为 `docs/FILTERS.md` 增加状态说明，并补齐当前内置音频滤镜 `volume_balance` 与链路说明。
- 为 `docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 增加状态说明，明确其是专题参考而不是全量进度面板。
- 为 `docs/WINDOWS_SETUP.md` 增加状态说明，修正 Quill 说明、手动安装示例和共享库使用说明，使之与当前 `CMakeLists.txt` 对齐。
- 更新 `docs/README.md` 索引，将 `FILTERS.md` 纳入可发现入口，并追加本轮文档整理记录。

### 本地校对结果
- `docs/FILTERS.md` 已明确当前生效的滤镜主链与预留组件边界。
- `docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 已明确为专题参考文档，并指向当前总进度来源。
- `docs/WINDOWS_SETUP.md` 已不再建议使用与当前仓库不一致的 `SDL2_DIR` / `FFMPEG_DIR` 传参示例。
- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。

### 修改文件
- docs/FILTERS.md
- docs/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md
- docs/WINDOWS_SETUP.md
- docs/README.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 48: 根 README 故障排除与历史问题归档仍有旧口径

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 根目录 `README.md` / `README_ZH.md` 的 Windows 故障排除仍提示使用 `FFMPEG_DIR` 传参，这与当前 `CMakeLists.txt` 的自动探测 / `external/ffmpeg` 回退逻辑不一致。
- `docs/video-stream-index-fix.md` 记录的是早期原型阶段的问题，但缺少“历史归档”标识，文中的 `playLoop` 与 `src/video_decoder.cpp` 易被误读为现行实现。

### 分析记录
1. 前几轮已经清理了 `WINDOWS_SETUP.md` 的构建入口，但根 README 的简版故障排除还残留旧说明。
2. `video-stream-index-fix.md` 作为问题分析样本仍然有价值，适合保留；重点是让读者明确它属于早期阶段。
3. 这两类问题都属于“入口文档和历史归档之间边界不清”，适合一起收尾。

### 解决方案
- 将根 README 的 FFmpeg 故障排除改为当前 `vcpkg toolchain / external/ffmpeg` 口径。
- 为 `docs/video-stream-index-fix.md` 增加状态说明，标记其为 `2026-02-24` 早期原型问题分析归档。
- 在 `docs/README.md` 中为该历史文档补充索引，并记录本轮更新。

### 本地校对结果
- `README.md` / `README_ZH.md` 不再建议使用与当前主链不一致的 `FFMPEG_DIR` 传参方式。
- `docs/video-stream-index-fix.md` 已明确为历史归档，不再暗示 `playLoop` / `video_decoder.cpp` 属于当前仓库结构。
- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。

### 修改文件
- README.md
- README_ZH.md
- docs/video-stream-index-fix.md
- docs/README.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 49: 缺少独立的文档巡检总表

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 本轮已经连续完成多批文档清理，但结果仍然分散在多个提交和日志条目里。
- 如果后续需要继续维护文档口径，缺少一份“一眼看懂本轮清理范围和结论”的总表。

### 分析记录
1. `CHANGELOG.md` 和 `DEVELOP_LOG.md` 能记录过程，但对后续维护者来说不够聚合。
2. `docs/README.md` 提供了索引入口，但没有一份专门解释“本轮文档巡检结论”的汇总报告。
3. 增加单独报告文档，可以把“当前主链基线、已收敛文档、保留历史内容、后续建议”固定下来，减少重复解释成本。

### 解决方案
- 新增 `docs/DOC_AUDIT_2026-03-08.md`，作为本轮文档巡检总表。
- 报告中集中记录：巡检范围、关键词方法、当前主链基线、已完成对齐项、保留但合理的历史内容、后续维护建议、关联提交。
- 在 `docs/README.md` 中将该报告纳入索引，并追加更新历史说明。

### 本地校对结果
- `docs/DOC_AUDIT_2026-03-08.md` 已可独立说明本轮文档清理工作的范围与结论。
- `docs/README.md` 已可直接定位到该巡检报告。
- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。

### 修改文件
- docs/DOC_AUDIT_2026-03-08.md
- docs/README.md
- docs/CHANGELOG.md
- docs/DEVELOP_LOG.md

---

## 问题 50: M4 4.4（帧步进）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `4.4` 要求支持暂停态逐帧查看画面。
- 当前仓库缺少暂停态帧步进入口，无法像 MPC-HC 一样在暂停后逐帧前后检查画面。

### 分析记录
1. 输入层当前只有播放、seek、音量、章节、A-B Repeat、截图等动作，没有帧步进动作和默认键位。
2. `PlayerCore` 暂停时会冻结调度器，因此不能直接复用常规渲染循环，需要一条暂停态的定向刷新路径。
3. 初版实现联调时发现，音频消费线程在暂停态仍会用旧 `playback_pts` 回写 `position_`，会把步进结果覆盖掉，需要同步收口。

### 解决方案
- 新增 `step_frame_backward` / `step_frame_forward` 热键动作，默认绑定 `,` / `.`。
- 在 `Display`、渲染器接口、`PlayerCore`、`VideoPlayer` 之间打通帧步进请求与 API。
- 采用“暂停态 seek + 首帧刷新”的方式实现前后单帧步进，并在步进后维持暂停状态。
- 调整音频消费线程：仅在 `PlaybackState::Playing` 时才用音频播放位置回写主时间轴。
- 新增 `--frame-step-check .\\juren-30s.mp4` 自检命令，并记录本地验收结果。

### 本地校对结果
- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`：`PASS`
- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`
- 默认热键文档已补充 `, / .` 的暂停态帧步进说明。

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
- docs/MPC_HC_GAP_ANALYSIS.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/FRAME_STEP_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md

---

## ?? 51: M4 4.5???/???????

**??**: 2026-03-08
**??**: ???

### ????
- ???? `4.5` ???????? / ???????
- ?????? `J/K` ? `Ctrl+J/K` ?????????????? MPC-HC ??????????

### ????
1. ??????????????????????????????????????????
2. ??????????????????????????????????????????????????
3. ???????????????????????????????????????????????

### ????
- ?? `SubtitleDelayDown` / `SubtitleDelayUp` ??????? `J / K`??? `Ctrl` ???????????
- ? `PlayerCore` ????/??????? getter/setter?
  - ???? `updateSubtitleOverlay` ???????????????
  - ??????? PTS???????????????????????????
- ?? `AppSettings`?`config/player_settings.ini` ? `--settings-persistence-check`?????????/???
- ?? `--delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt` ???????? README / ???? / ???? / ?????

### ??????
- `build/Debug/modern-video-player.exe --settings-persistence-check`?`PASS`
- `build/Debug/modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt`?`PASS`
- ????????? `J / K` ? `Ctrl+J / Ctrl+K` ????????

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
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/reports/DELAY_ADJUST_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## ?? 52: M4 4.6????? `1..9` ???

**??**: 2026-03-08
**??**: ???

### ????
- `4.6` ????????? `1..9` ???????
- ????A-B Repeat???????????????????????? MPC-HC ???????????????????????

### ????
1. ??????????????????????????????????? `1..9` ????????????????????
2. `Display` ? `PlayerCore` ?????????????? seek ???????????????? `seek_ratio_` ?????????
3. ???????????????????????? 10% / 90% ???????????????????

### ????
- ?? `SeekTo10Percent` ~ `SeekTo90Percent` ????????? `1..9`?
- ? `Display` ????????????? `0.1` ~ `0.9` ???? seek ???
- ?? `--numeric-seek-check .\juren-30s.mp4` ???????? README / ???? / ???? / ???

### ??????
- `build/Debug/modern-video-player.exe --numeric-seek-check .\juren-30s.mp4`?`PASS`
- ????????? `1..9` ?? `10%..90%` ????

### ????
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/display.cpp
- src/main.cpp
- README.md
- README_ZH.md
- docs/README.md
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 53: M2 2.2.4：输出播放性能日志（掉帧/队列/CPU/GPU）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `2.2.4` 需要一个统一的性能日志入口，用于观察高分辨率/高码率样本的掉帧、队列和资源占用情况。
- 当前播放器主链已经具备较多内部统计，但外部没有稳定的结构化验收输出，无法直接沉淀为本地报告。

### 分析记录
1. `PlayerCore` 已经维护了 demux、decode、render 等多组原子计数，`Scheduler` 也有掉帧与解码统计，但缺少统一快照接口。
2. 现有命令行自检更偏向功能验证，不适合做 `1080p60 / 4K / 高码率` 样本的性能对比留档。
3. GPU 直接占用率采样跨平台成本较高，因此本轮先输出当前激活的解码 backend / 渲染 backend，作为 GPU 路径观测信息。

### 解决方案
- 新增 `core::DiagnosticsSnapshot`，在 `PlayerCore` 中收敛 demux / decode / render / scheduler / queue 指标。
- 在 `VideoPlayer` 中增加 `getInfo()` / `getDiagnosticsSnapshot()` 透传接口。
- 在 `main` 中新增 `--performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200` 自检命令，输出 CPU 平均占用、逻辑核心数、backend、掉帧与队列指标。
- 新增 `docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`，并同步任务清单、差距评估、版本文档与变更记录。

### 本地校对结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`：`PASS`
- 当前样本输出包含 `renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`cpu_avg_percent≈100%`、`scheduler_late_drops=0` 等关键指标。

### 修改文件
- include/core/player_core.h
- include/video_player.h
- src/core/player_core.cpp
- src/video_player.cpp
- src/main.cpp
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 54: M2 2.2.1 / 2.3.2：1080p60 稳定播放验收

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 需要为 `1080p60` 稳定播放建立明确的本地验收入口，避免只凭性能日志或人工观察判断。
- 当前样本包没有显式的 `1080p60` 稳定性样本生成说明，复现实验路径不够清晰。

### 分析记录
1. `collectFileProbeReport()` 已经能给出宽高、FPS、推荐 backend 等元信息，适合做稳定性验收前置条件。
2. `DiagnosticsSnapshot` 已可提供 `scheduler_late_drops` / `demux_dropped_packets` 等关键稳定性指标。
3. 还需要补一个“连续播放窗口内时间推进”的判断，才能把 `1080p60` 验收从观测日志提升为明确门禁。

### 解决方案
- 新增 `--1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`，检查 `probe`、时间推进、late drop 与 demux drop。
- 在 `tools/download_test_samples.ps1` 增加 `1080p60 AAC 2ch` 样本生成路径，并在 `samples/README.md` 标记其用于 `2.2.1 / 2.3.2`。
- 新增 `docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。

### 本地校对结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`：`PASS`
- 当前样本输出包含 `advance_ratio=0.994133`、`late_drops=0`、`demux_dropped_packets=0`、`decoder_backend=D3D11VA`。

### 修改文件
- src/main.cpp
- tools/download_test_samples.ps1
- samples/README.md
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/1080P60_STABILITY_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 55: M2 2.2.2 / 2.3.3：4K 播放与降级验收

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 需要把 `4K` 样本播放与“失败时可降级”收敛成一个统一的本地验收入口。
- 现有能力分散在性能日志与 Windows 后端回退检查中，不利于直接对应任务清单 `2.2.2 / 2.3.3`。

### 分析记录
1. `collectFileProbeReport()` 已能确认样本是否为 `3840x2160`，适合做 4K 门禁前置判断。
2. `runBackendSessionSubprocess()` 已能稳定验证 hard / soft 两个后端模式，无需重新实现降级流程。
3. 还需要一个主进程连续播放窗口，确保 `4K` 样本不是“只打开不推进”，而是真正进入播放状态。

### 解决方案
- 新增 `--4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`。
- 主进程检查 `probe`、时间推进、`late_drop` 与当前 backend；子进程检查 hard / soft 模式都能进入播放。
- 新增 `docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。

### 本地校对结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`：`PASS`
- 当前样本输出包含 `advance_ratio=0.968167`、`late_drops=0`、`hard.decoder_backend=D3D11VA`、`soft.decoder_backend=Software`。

### 修改文件
- src/main.cpp
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 56: M2 2.2.3：>80Mbps 高码率样本验收

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 需要一个真正超过 `80Mbps` 的回归样本和对应验收入口，才能完成 `2.2.3`。
- 当前样本集合虽然覆盖分辨率与格式，但码率普遍偏低，无法作为高码率门禁。

### 分析记录
1. 现有 `collectFileProbeReport()` 没有直接输出码率，但 FFmpeg 的 `AVFormatContext::bit_rate` 可直接复用。
2. 高码率任务的核心不在分辨率，而在于样本真实码率、连续播放窗口推进和掉帧/丢包情况。
3. 基于当前 `D3D11VA + D3D11` 主链，`100Mbps` 级别的 `1080p60` H.264 样本已能稳定进入播放链路，适合作为本地回归基线。

### 解决方案
- 新增 `collectFormatBitrateBitsPerSecond()` 与 `--high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`。
- 在 `tools/download_test_samples.ps1` 增加 `100Mbps` 样本生成路径，并在 `samples/README.md` 标注用途。
- 新增 `docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。

### 本地校对结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`：`PASS`
- 当前样本输出包含 `format_bitrate_bps=102829290`、`advance_ratio=0.988444`、`late_drops=0`、`demux_dropped_packets=0`。

### 修改文件
- src/main.cpp
- tools/download_test_samples.ps1
- samples/README.md
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/HIGH_BITRATE_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 57: 发布门禁 6.5（长时播放稳定性）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `6.5` 需要一个可重复执行的本地入口，验证长时播放窗口内无 crash 且仍能持续推进。

### 分析记录
1. 现有 `1080p60` / `4K` / `>80Mbps` 验收命令都以短窗口为主，不能直接作为“长时播放无 crash”的证明。
2. `VideoPlayer` 已提供 `play()` / `isPlaying()` / `getCurrentTime()` / `getDiagnosticsSnapshot()`，可以复用为 smoke 验收闭环。
3. 只要同时验证播放态保持、时间推进与 `late_drop` / demux drop，即可形成一个足够轻量的发布门禁。

### 解决方案
- 新增 `--long-playback-check .\juren-30s.mp4 10000`，要求采样窗口不少于 `5000ms`。
- 命令输出 `probe_duration`、`renderer_backend`、`decoder_backend`、`advance_ratio`、`late_drops`、`demux_dropped_packets` 等结构化字段。
- 新增 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`，并同步任务清单、差距评估、版本记录与变更记录。

### 本地验收结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`：`PASS`
- 当前输出包含 `probe_duration=30.03`、`entered_playback_loop=true`、`still_playing_after_window=true`、`advance_ratio=0.996267`、`late_drops=0`、`demux_dropped_packets=0`、`renderer_backend=D3D11`、`decoder_backend=D3D11VA`。

### 修改文件
- src/main.cpp
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 58: 7.1 插件系统（动态加载与生命周期闭环）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `7.1` 需要一个真正可运行的插件宿主，而不只是内存中的元数据占位。

### 分析记录
1. 现有 `PluginManager` 只能做静态描述符注册与启停标记，无法覆盖 `DLL` 动态加载、版本兼容和卸载清理场景。
2. 代码库已经有 `FilterRegistry` 这类天然扩展点，适合作为首个插件闭环的宿主能力。
3. 如果没有示例 `DLL` 和命令行验收入口，就无法证明插件系统已从“骨架”进入“可运行”。

### 解决方案
- 新增 `include/plugin/plugin_api.h`，定义插件宿主接口与导出符号。
- 重写 `PluginManager`，支持按路径加载插件、校验 `API` 版本、执行 `initialize/shutdown` 生命周期，并跟踪插件注册的滤镜工厂用于卸载清理。
- 新增 `sample_logger_plugin` 示例插件与 `--plugin-check` 验收命令；插件会注册 `sample_identity` 视频滤镜，供宿主验证注册/卸载闭环。

### 本地验收结果
- `cmake --build build --config Debug`：通过。
- `build/Debug/modern-video-player.exe --plugin-check`：`PASS`
- 当前输出包含 `loaded_count=1`、`plugin_ids=sample_logger_plugin@0.1.0`、`sample_video_filter_registered=true`、`sample_video_filter_unloaded=true`、`errors=none`。

### 修改文件
- CMakeLists.txt
- include/plugin/plugin_api.h
- include/plugin/plugin_manager.h
- include/filters/filter_registry.h
- src/plugin/plugin_manager.cpp
- src/plugin/sample_logger_plugin.cpp
- src/filters/filter_registry.cpp
- src/main.cpp
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 59: 7.2 流媒体（真实 HTTP 分片与缓冲）

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `7.2` 需要一个真正可跑的 HTTP 分片下载与缓冲入口，而不是停留在清单解析骨架。

### 分析记录
1. `HttpStreamDownloader` 之前没有真实读流实现，`readChunk()` 永远返回空数组。
2. 当前代码已经具备 HLS 清单解析器，适合以 HLS 媒体清单作为首个流媒体 smoke 入口。
3. 在受限网络环境下，最稳定的验收方式是在本机起一个小型 HTTP 夹具服务，避免依赖外部站点。

### 解决方案
- 重写 `HttpStreamDownloader`，基于 FFmpeg `avio` 支持真实 HTTP 打开、分块读取、内部缓冲、EOF 状态与错误透传。
- 新增 `--streaming-buffer-check`，下载 HLS 清单、解析相对 URL、抓取前 3 个分片并验证缓冲字节数。
- 新增 `tools/start_streaming_fixture_server.ps1` 与 `samples/streaming/hls_local/*` 夹具，用于本机 HTTP 回归。

### 本地验收结果
- `cmake --build build --config Debug`：通过。
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`
- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`：`PASS`
- 当前输出包含 `manifest_download_ok=true`、`manifest_parse_ok=true`、`segments_downloaded=3`、`buffered_bytes=621`、`buffer_ok=true`、`error=none`。

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
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


## 问题 60: 7.3 HLS/DASH 自适应码率

**日期**: 2026-03-08
**状态**: 已解决

### 问题描述
- 任务清单 `7.3` 需要一个可重复执行的 HLS/DASH 多码率解析与自适应码率本地验收入口。

### 分析记录
1. 现有 `HlsManifestParser` 只能读取媒体播放列表，无法处理 `master playlist`。
2. 现有 `DashManifestParser` 只知道 `Representation` 带宽，无法拿到初始化分片和媒体分片 URL。
3. 最稳妥的验收方式仍是在本机 HTTP 夹具下，分别验证 HLS/DASH 的档位选择与升降档路径。

### 解决方案
- 扩展 HLS/DASH 解析器，补齐 variant / representation / `BaseURL` / 初始化分片 / 媒体分片明细。
- 新增 `AdaptiveBitrateSelector`，并在 `main` 中提供 `--adaptive-bitrate-check` 命令。
- 新增 `samples/streaming/abr_local/{hls,dash}` 样本与报告，验证 HLS/DASH 在给定带宽序列下的升码率与降码率切换。

### 本地验收结果
- `cmake --build build --config Debug`：通过。
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`
- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`：`PASS`
- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`：`PASS`
- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`：`PASS`
- 当前输出包含 `switch_count=2`、`upswitch_count=1`、`downswitch_count=1`，说明 HLS/DASH 两条路径都完成了一次升档和一次降档。

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
- docs/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md
- docs/CHANGELOG.md
- docs/VERSION.md
- docs/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
