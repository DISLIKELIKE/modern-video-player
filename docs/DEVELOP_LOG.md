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
