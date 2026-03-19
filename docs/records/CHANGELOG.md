# 问题修复记录

本文档记录开发过程中遇到的问题及其解决方案。

---

## 问题列表

| # | 日期 | 问题 | 状态 |
|---|------|------|------|
| 1 | 2026-02-17 | FFmpeg 8.0 兼容性问题 | ✅ 已修复 |
| 2 | 2026-02-24 | 视频流索引不匹配 | ✅ 已修复 |
| 3 | 2026-02-24 | 音频流索引不匹配 | ✅ 已修复 |
| 4 | 2026-02-24 | YUV 数据渲染错误 | ✅ 已修复 |
| 5 | 2026-02-24 | 企业级 Quill 日志通道 | ✅ 已修复 |
| 6 | 2026-02-25 | 多线程播放架构重构 | ✅ 已完成 |
| 7 | 2026-02-25 | 音频播放架构修复 | ✅ 已修复 |
| 9 | 2026-02-25 | VideoFrame/AudioFrame 移动语义缺陷 | ✅ 已修复 |
| 10 | 2026-02-25 | 多解码器实例竞争读取导致解码错误 | ✅ 已修复 |
| 11 | 2026-02-27 | 并发读取 AVFormatContext 导致崩溃 | ✅ 已修复 |
| 12 | 2026-02-27 | 企业级多线程架构重构 | ✅ 已完成 |
| 15 | 2026-03-06 | 小屏窗口过大且拖拽缩放不稳定 | ✅ 已修复 |
| 18 | 2026-03-07 | DASH 解析编译失败与格式能力矩阵缺失 | ✅ 已修复 |
| 19 | 2026-03-07 | D3D11VA 硬解最小闭环与软解回退 | ✅ 已修复 |
| 20 | 2026-03-07 | 探测入口与格式回归脚本落地 | ✅ 已修复 |
| 21 | 2026-03-07 | GitHub Actions 自动格式回归接入 | ✅ 已修复 |
| 22 | 2026-03-07 | 播放列表主链路、设置持久化与快捷键首版接入 | ✅ 已修复 |
| 23 | 2026-03-07 | 移除 Core 单元测试目标与测试文件 | ✅ 已修复 |
| 24 | 2026-03-07 | 外挂字幕加载入口（SRT）接入主流程 | ✅ 已修复 |
| 25 | 2026-03-07 | 字幕渲染叠加与播放时序同步接入 | ✅ 已修复 |
| 26 | 2026-03-08 | 字幕开关控制与字幕加载异常处理完善 | ✅ 已修复 |
| 27 | 2026-03-08 | 快捷键配置持久化接入（hotkey.*） | ✅ 已修复 |
| 28 | 2026-03-08 | 快捷键冲突检测与恢复默认能力 | ✅ 已修复 |
| 29 | 2026-03-08 | M1 验收 1.4.1：SRT seek 同步自检命令落地 | ✅ 已修复 |
| 30 | 2026-03-08 | M1 验收 1.4.2：播放列表连续播放 5 文件自检通过 | ✅ 已修复 |
| 31 | 2026-03-08 | M1 验收 1.4.3：设置重启恢复自检通过 | ✅ 已修复 |
| 32 | 2026-03-08 | M2 2.1.2：容器矩阵补齐 mov/avi/m2ts 并回归通过 | ✅ 已修复 |
| 33 | 2026-03-08 | M2 2.1.3：视频编码矩阵补齐 MPEG-2 并回归通过 | ✅ 已修复 |
| 34 | 2026-03-08 | M2 2.1.4：音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM 并回归通过 | ✅ 已修复 |
| 35 | 2026-03-08 | M3 3.1.1：DecoderFactory 接入真实初始化流程 | ✅ 已修复 |
| 36 | 2026-03-08 | M3 3.1.2：D3D11VA 协商失败软解兜底完善 | ✅ 已修复 |
| 37 | 2026-03-08 | M3 3.2.1：D3D11 渲染最小可用链路落地 | ✅ 已修复 |
| 38 | 2026-03-08 | M3 3.3.2：渲染失败降级回归入口补齐 | ✅ 已修复 |
| 39 | 2026-03-08 | M3 3.3.1：Windows 软解/硬解主力样本回归通过 | ✅ 已修复 |
| 40 | 2026-03-08 | M4 4.1：章节导航（上一章/下一章）接入与验收 | ✅ 已修复 |
| 41 | 2026-03-08 | M4 4.2：A-B Repeat（A/B/C）接入与验收 | ✅ 已修复 |
| 42 | 2026-03-08 | M4 4.3（截图） | ✅ 已修复 |
| 43 | 2026-03-08 | `MPC_HC_GAP_ANALYSIS` 评估结论过期 | ✅ 已修复 |
| 44 | 2026-03-08 | `docs/records/VERSION.md` 历史路径描述过期 | ✅ 已修复 |
| 45 | 2026-03-08 | README 与架构文档仍混用旧主链表述 | ✅ 已修复 |
| 46 | 2026-03-08 | 实现教程与迭代计划缺少历史/当前边界说明 | ✅ 已修复 |
| 47 | 2026-03-08 | 辅助说明文档仍缺少当前入口与状态边界 | ✅ 已修复 |
| 48 | 2026-03-08 | 根 README 故障排除与历史问题归档仍有旧口径 | ✅ 已修复 |
| 49 | 2026-03-08 | 缺少独立的文档巡检总表 | ✅ 已修复 |
| 50 | 2026-03-08 | M4 4.4：暂停态帧步进接入与验收 | ✅ 已修复 |
| 53 | 2026-03-08 | M2 2.2.4：输出播放性能日志（掉帧/队列/CPU/GPU） | ✅ 已修复 |
| 54 | 2026-03-08 | M2 2.2.1 / 2.3.2：1080p60 稳定播放验收 | ✅ 已修复 |
| 55 | 2026-03-08 | M2 2.2.2 / 2.3.3：4K 播放与降级验收 | ✅ 已修复 |
| 56 | 2026-03-08 | M2 2.2.3：>80Mbps 高码率样本验收 | ✅ 已修复 |
| 57 | 2026-03-18 | D3D11 原生 GPU 渲染链补齐 | ✅ 已修复 |
| 66 | 2026-03-18 | 全局构建阻塞清理与 ASS/SSA 原生 D3D11 字幕链 | ✅ 已修复 |
| 67 | 2026-03-18 | ASS 标签解析与 UTF-16 字幕范围修正 | ✅ 已修复 |
| 68 | 2026-03-18 | MSVC warning debt 分层清理（C4819 / C4996 / C4706） | ✅ 已修复 |
| 69 | 2026-03-19 | PlayerCore 停播收口、包队列所有权与 Clock/Demuxer 设计债修复 | ✅ 已修复 |

## 问题 69: PlayerCore 停播收口、包队列所有权与 Clock/Demuxer 设计债修复

**日期**: 2026-03-19

### 问题描述
- 在上一轮审查中发现，`PlayerCore` 的 EOF 自动停播和部分 UI 停播路径只修改了播放状态，没有完整收口 demux/audio/scheduler 线程，存在“状态已停、线程未清”的可重启风险。
- `PacketQueue` 仍以 `AVPacket*` 原始指针承载所有权，`flush/clear/reset` 路径会遗留未释放压缩包。
- `Clock` 在系统时钟路径上的 `pause()` / `setSpeed()` 基准更新不正确，纯视频或 system-clock 路径会出现时间跳变；`Demuxer::open()` 持锁调用 `close()` 还存在自锁风险。

### 原因分析
- EOF 自动停播发生在 scheduler 的 render 线程内，不能直接走同步 `stop()`，否则会触发线程自 join；旧实现因此退化成“只改 `state_`”。
- `ThreadSafeQueue<AVPacket*>` 只管理指针搬运，不管理 FFmpeg 包生命周期，导致 `clear()` 与队列析构不会释放队列中剩余包。
- `Clock::pause()` 先写 `paused_` 再读当前时间，`Clock::setSpeed()` 也没有先固化旧基准，导致 system clock 连续性被破坏。
- `Demuxer::open()` 在已持有 `mutex_` 时再次调用同样会上锁的 `close()`，接口复用时会卡死。

### 解决方案
- 为 `PlayerCore` 增加 deferred stop 收口路径：EOF 在 render 线程内只发出异步停机请求并标记待清理，后续由安全线程执行真实 `stop/join/flush`，同时修复 `next/previous/quit` 请求直接走完整 `stop()`。
- 把 `PacketQueue` 改为 `ThreadSafeQueue<unique_ptr<AVPacket, AvPacketDeleter>>`，并让 `ThreadSafeQueue::push()` 支持延迟 move，彻底把 FFmpeg 包生命周期收回到 RAII。
- 为 `Scheduler` 增加异步停机入口并在重新 `start()` 前回收已退出 worker，避免 EOF 后下一次启动触发 `std::terminate`。
- 修正 `Clock` 的 pause/speed 基准更新逻辑，保证 system-clock 路径的暂停和变速时间连续；`Demuxer::open()` 改为在同一锁域内直接关闭旧输入，不再重入 `close()`。
- 已重新执行 `MSBuild` 全量重建：`& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`，当前结果为 `0 个警告 / 0 个错误`。

### 修改文件
- `include/core/player_core.h`
- `include/core/scheduler.h`
- `include/thread_safe_queue.h`
- `src/core/player_core.cpp`
- `src/core/scheduler.cpp`
- `src/core/clock.cpp`
- `src/demuxer.cpp`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`

---
## 问题 68: MSVC warning debt 分层清理（C4819 / C4996 / C4706）

**日期**: 2026-03-18

### 问题描述
- GitHub Actions 的 Windows `Debug` 构建虽然已经能通过主流程编译，但长期残留大量 warning，影响 CI 可读性和后续回归定位。
- `C4819` 主要来自 MSVC 以本地代码页读取 UTF-8 源文件；`C4996` 同时出现在第三方头文件和项目本地代码；另有少量本地 `C4100 / C4706` 提示。
- 需要先按“编码告警、第三方 warning、本地 warning”三层拆开治理，避免简单全局静默把真实问题一起吞掉。

### 原因分析
- 本地源码中存在中文注释和 UTF-8 内容，但 MSVC 默认并不按 UTF-8 解释源文件，触发 `C4819`。
- FFmpeg / Quill 头文件属于第三方依赖，不应该要求项目通过改第三方源码来消除 warning，更合适的办法是把它们隔离到外部头文件 warning 层。
- 本地 `logger / subtitle / player_core / main` 中仍保留了若干可直接修掉的安全函数与表达式 warning。

### 解决方案
- 在 `CMakeLists.txt` 中为 MSVC 目标增加 `/utf-8 /external:anglebrackets /external:W0`，让本地源码按 UTF-8 编译，并把第三方 angle-bracket 头文件 warning 下降到外部层处理。
- 在 `src/logger.cpp` 中新增安全环境变量读取 helper，用 `_dupenv_s` 替换 Windows 下的 `std::getenv` 用法。
- 在 `src/subtitle/srt_parser.cpp` 和 `src/subtitle/ass_parser.cpp` 中采用“Windows 走 `sscanf_s`，其他平台保留 `std::sscanf`”的跨平台解析分支，清理本地 `C4996` 而不破坏可移植性。
- 在 `src/main.cpp` 中把信号处理参数标记为 `[[maybe_unused]]`；在 `src/core/player_core.cpp` 中把 demux push 重试逻辑改写为显式赋值，去掉条件表达式内赋值导致的 `C4706`。
- 重新执行 `MSBuild` 全量重建：`& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`，当前结果为 `0 个警告 / 0 个错误`。

### 修改文件
- `CMakeLists.txt`
- `src/logger.cpp`
- `src/main.cpp`
- `src/subtitle/srt_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/core/player_core.cpp`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`

---
## 问题 67: ASS 标签解析与 UTF-16 字幕范围修正

**日期**: 2026-03-18

### 问题描述
- `ASS/SSA` override block 中的 `\fnArial`、`\rDefault` 之类紧凑写法会被错误识别成标签名 `fnArial`、`rDefault`，导致常见样式标签失效。
- `SubtitleTextRun.start/length` 之前按 UTF-8 code point 计数，但 D3D11 字幕渲染最终直接把它们传给 DirectWrite 的 `DWRITE_TEXT_RANGE`，遇到 emoji 或非 BMP 字符时会产生范围错位。
- 需要在前一轮 D3D11 原生字幕链提交后，再做一次整工程构建复核并把当前结果同步到记录文档。

### 原因分析
- 旧解析逻辑在读取 override 标签名时会把连续字母整体吞掉，缺少对常用 ASS 标签前缀的显式匹配。
- 字幕解析阶段和渲染阶段使用了不同的文本长度语义：前者偏向 UTF-8 code point，后者实际需要 UTF-16 code unit。
- 这类问题不会直接造成崩溃或泄漏，但会破坏 ASS/SSA 样式字幕在原生 D3D11 链中的语义正确性。

### 解决方案
- 在 `src/subtitle/ass_parser.cpp` 中加入常用 ASS 标签的显式前缀匹配，按最长标签优先识别 `alpha / bord / shad / pos / fn / fs / an / 1c / 1a / c / a / b / i / u / s / r`，未命中时再回退到旧的宽松解析逻辑。
- 将 `ASS/SSA` 文本 run 长度和纯文本字幕 fallback 路径统一改为按 UTF-16 code unit 计数，使 `SubtitleTextRun` 与 DirectWrite `DWRITE_TEXT_RANGE` 语义一致。`ass_parser.cpp` 内顺手清掉了本地 `sscanf` / 局部变量遮蔽告警。
- 重新执行 `MSBuild` 全量重建：`& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`，当前结果为 `167 个警告 / 0 个错误`。

### 修改文件
- `src/subtitle/ass_parser.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`

---
## 问题 66: 全局构建阻塞清理与 ASS/SSA 原生 D3D11 字幕链

**日期**: 2026-03-18

### 问题描述
- 全局 `Debug|x64` 构建曾被多处头文件/源文件的编码误读阻塞，导致“D3D11 原生链代码已改好，但无法整体验证”。
- D3D11 原生字幕链此前只覆盖纯文本叠加，`.ass/.ssa` 的样式、定位和多条同时激活字幕还没有进入 native renderer。
- 外挂字幕自动探测与显式加载需要覆盖 `.ass`、`.ssa`、`.srt` 三种格式，而不是只面向 SRT。

### 原因分析
- 部分带中文注释的头文件和源文件在当前 MSVC/代码页组合下被误读，触发全局语法/标记化错误，构成与业务逻辑无关的构建阻塞。
- 旧字幕模型只有纯文本语义，`IVideoRenderer` 接口也只接收单字符串，导致 `PlayerCore` 无法把 ASS/SSA 的样式、层级和定位信息传给 D3D11 渲染器。
- 时间线上原本只解析单条活动字幕，不足以表达 ASS/SSA 常见的多 cue 同屏场景。

### 解决方案
- 将受影响的头文件和源文件改写为 ASCII-safe 形式，恢复 `MSBuild` 全量构建基线。
- 扩展字幕数据模型，新增 `SubtitleStyle`、`SubtitleTextRun`、`SubtitleItem.layer/play_res/runs`，并让解析工厂支持 `.ass/.ssa/.srt`。
- 新增 `AssParser`，解析 `Script Info / Styles / Events`，覆盖常用 `\b \i \u \s \fs \fn \an \a \pos \c/\1c \alpha/\1a \bord \shad \r` 标签。
- 让 `PlayerCore` 计算多条当前激活字幕，并通过 `IVideoRenderer::setSubtitleItems()` 把结构化字幕对象直接送入 `D3D11VideoRenderer`。
- 在 `D3D11VideoRenderer` 同一块 swap chain backbuffer 上完成 ASS/SSA 文本填充、描边、阴影、背景框、对齐和定位绘制；非 D3D11 渲染器默认退化为纯文本显示。
- 更新 `main.cpp` 的自动外挂字幕探测顺序为 `.ass -> .ssa -> .srt`，并在整工程级别重新验证构建通过。

### 修改文件
- `CMakeLists.txt`
- `include/subtitle/subtitle_parser.h`
- `include/subtitle/ass_parser.h`
- `include/subtitle/srt_parser.h`
- `include/subtitle/subtitle_timeline.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/subtitle/srt_parser.cpp`
- `src/subtitle/subtitle_timeline.cpp`
- `include/render/video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `src/render/d3d11_video_renderer.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- `src/main.cpp`
- `include/config/settings_manager.h`
- `include/media/format_support.h`
- `include/playlist/playlist_manager.h`
- `include/plugin/plugin_api.h`
- `include/plugin/plugin_manager.h`
- `include/filters/filter_registry.h`
- `include/filters/video_filter_chain.h`
- `include/filters/audio_filter_chain.h`
- `include/streaming/http_stream_downloader.h`
- `include/streaming/hls_manifest_parser.h`
- `include/streaming/dash_manifest_parser.h`
- `include/streaming/adaptive_bitrate_selector.h`
- `include/display.h`
- `include/render/sdl_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/frame_queue.h`
- `src/streaming/adaptive_bitrate_selector.cpp`
- `src/render/renderer_factory.cpp`
- `src/ui/skin_engine.cpp`
- `src/core/scheduler.cpp`
- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
---
## 问题 57: D3D11 原生 GPU 渲染链补齐

**日期**: 2026-03-18

### 问题描述
- `D3D11VideoRenderer` 已经具备独立的 D3D11 视频呈现能力，但字幕仍然只保存文本状态，没有真正绘制到同一块 swap chain backbuffer 上。
- 旧分析文档仍把 D3D11 renderer 描述为 SDL 包装器，已经不符合当前仓库状态。

### 原因分析
- 原生视频主面与 D3D11VA device sharing 已经落地，剩余缺口集中在字幕叠加这一块最后的 UI/overlay 合成路径。
- 如果字幕仍依赖 SDL `Display` 或软件纹理链，整条渲染链就不能称为“完整、独立、原生 D3D11 GPU 链路”。

### 解决方案
- 在 `D3D11VideoRenderer` 内新增 D2D1 / DirectWrite 资源，直接对 DXGI swap chain backbuffer 进行字幕文本绘制。
- 保留现有 D3D11 视频面采样路径，并将字幕绘制串到同一帧 present 前。
- 对暂停态字幕变化增加即时重绘，确保 seek / frame-step 后字幕与视频状态一致。
- 同步更新设计文档和旧分析文档的历史说明，避免后续继续沿用过期结论。

### 修改文件
- `src/render/d3d11_video_renderer.cpp`
- `src/render/renderer_factory.cpp`
- `CMakeLists.txt`
- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`
- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
---

## 问题 12: 企业级多线程架构重构

**日期**: 2026-02-27

### 问题描述

原有架构存在以下问题：
1. 组件职责不清晰，VideoPlayer 承担过多职责
2. 线程模型复杂，难以维护
3. 内存管理容易出错，导致双重释放等 bug

### 解决方案

重构为企业级多线程架构：

```
┌─────────────────────────────────────────────────────────────┐
│                    VideoPlayer (主控制器)                    │
├─────────────────────────────────────────────────────────────┤
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │ Demuxer      │  │ DecoderWorker│  │ Display      │      │
│  │ (解封装器)    │  │ (解码工作线程)│  │ (渲染器)     │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
│         │                 │                 │               │
│         ▼                 ▼                 ▼               │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐      │
│  │PacketQueue   │  │ Clock        │  │ AudioPlayer  │      │
│  │ (包队列)     │  │ (时钟同步)   │  │ (音频播放)   │      │
│  └──────────────┘  └──────────────┘  └──────────────┘      │
└─────────────────────────────────────────────────────────────┘
```

### 新增组件

1. **Demuxer (解封装器)**
   - 封装 AVFormatContext 的读取操作
   - 提供统一的 packet 读取接口
   - 支持 seek 操作

2. **DecoderWorker (解码工作线程)**
   - 封装单个流的解码逻辑
   - 从 PacketQueue 获取 packet，解码后通过回调输出
   - 支持暂停/恢复/flush

3. **ThreadSafeQueue (线程安全队列)**
   - 通用的线程安全队列模板
   - 支持阻塞和非阻塞操作
   - 支持 EOF 信号传递

4. **Clock (时钟同步)**
   - 管理主时钟
   - 计算音视频同步延迟
   - 支持多种同步模式

### 修改文件

- 新增 `include/demuxer.h`, `src/demuxer.cpp`
- 新增 `include/decoder_worker.h`, `src/decoder_worker.cpp`
- 新增 `include/thread_safe_queue.h`
- 新增 `include/clock.h`, `src/clock.cpp`
- 重构 `include/video_player.h`, `src/video_player.cpp`
- 修改 `CMakeLists.txt`
- 修复 `src/packet_reader.cpp` 双重释放 bug

---

## 问题 11: 并发读取 AVFormatContext 导致崩溃

**日期**: 2026-02-27

### 问题描述

播放视频时出现大量 FFmpeg 解码错误和访问冲突崩溃：
```
[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).
[h264 @ ...] missing picture in access unit with size 12342
0xC0000005: 写入位置 0x... 时发生访问冲突
```

### 原因分析

视频解码线程 (`VideoDecodeThread`) 和音频解码线程 (`AudioDecodeThread`) 各自拥有独立的解码器实例，但共享同一个 `AVFormatContext`。两个线程并发调用 `av_read_frame(format_ctx_, packet)` 导致数据竞争，读取到的 packet 数据错乱，引发 H264 解码错误和内存访问冲突。

### 解决方案

引入统一的 `PacketReaderThread` 作为唯一的 packet 读取入口：

1. **新增 PacketReaderThread 类**
   - 作为唯一的 `av_read_frame()` 调用点
   - 根据 stream_index 将 packet 分发到对应的 PacketQueue

2. **新增 PacketRef 和 PacketQueue 类**
   - `PacketRef`: 包装 AVPacket 的智能结构体，支持移动语义
   - `PacketQueue`: 线程安全的 packet 队列，支持阻塞等待

3. **修改解码器接口**
   - `VideoDecoder` 和 `AudioDecoder` 新增 `decodePacket()` 方法
   - 接收外部传入的 packet，而非内部读取

4. **重构解码线程**
   - `VideoDecodeThread` 和 `AudioDecodeThread` 从 `PacketQueue` 获取 packet
   - 不再直接调用 `av_read_frame()`

### 修改文件

- 新增 `include/packet_reader.h`
- 新增 `src/packet_reader.cpp`
- 修改 `include/video_decoder.h`
- 修改 `src/video_decoder.cpp`
- 修改 `include/audio_decoder.h`
- 修改 `src/audio_decoder.cpp`
- 修改 `include/video_decode_thread.h`
- 修改 `src/video_decode_thread.cpp`
- 修改 `include/audio_decode_thread.h`
- 修改 `src/audio_decode_thread.cpp`
- 修改 `include/video_player.h`
- 修改 `src/video_player.cpp`
- 修改 `CMakeLists.txt`

---

## 问题 10: 多解码器实例竞争读取导致解码错误

## 问题 1: FFmpeg 8.0 兼容性问题

**日期**: 2026-02-17

### 问题描述

编译时报错，`codec_ctx_->avctx->priv_data` 在 FFmpeg 8.0 中不可用。

### 原因

FFmpeg 8.0 更改了 API，移除了对 `avctx->priv_data` 的直接访问。

### 解决方案

修改 `video_decoder.cpp` 和 `audio_decoder.cpp`：
- 在解码器类中添加 `format_ctx_` 成员变量
- 直接使用传入的 format context 而非从 codec context 获取

### 修改文件

- `include/video_decoder.h`
- `include/audio_decoder.h`
- `src/video_decoder.cpp`
- `src/audio_decoder.cpp`

---

## 问题 2: 视频流索引不匹配

**日期**: 2026-02-24

### 问题描述

播放 mp4 文件时，视频无法正常显示。日志显示：
```
decodeFrame: read packet, stream_index=0, expected=1
decodeFrame: packet stream mismatch, skipping
```
程序循环 48 次才能读到正确的视频帧。

### 原因

MP4 文件的流顺序是：音频流(索引 0) 在前，视频流(索引 1) 在后。`av_read_frame()` 返回的包可能是任意流的（通常是第一个流 - 音频流）。原代码遇到不匹配的流时直接返回 false，导致视频帧无法解码。

### 解决方案

修改 `src/video_decoder.cpp` 的 `decodeFrame()` 方法：
- 将遇到不匹配流时返回 false，改为 continue 跳过该包
- 循环读取直到找到正确流索引的包

### 修改文件

- `src/video_decoder.cpp`

### 代码变更

```cpp
// 修改前
if (packet->stream_index != stream_idx_) {
    av_packet_unref(packet);
    av_packet_free(&packet);
    return false;
}

// 修改后
while (true) {
    ret = av_read_frame(format_ctx_, packet);
    // ...
    if (packet->stream_index != stream_idx_) {
        av_packet_unref(packet);
        continue;  // 继续循环读取
    }
    break;  // 找到正确的流
}
```

---

## 问题 3: 音频流索引不匹配

**日期**: 2026-02-24

### 问题描述

与视频流索引相同的问题，但出现在音频解码器中。

### 原因

同样的问题：音频包可能不是第一个被读取的流。

### 解决方案

修改 `src/audio_decoder.cpp` 的 `decodeFrame()` 方法，应用与视频解码器相同的修复。

### 修改文件

- `src/audio_decoder.cpp`

---

## 问题 4: YUV 数据渲染错误

**日期**: 2026-02-24

### 问题描述

解码成功后程序立即退出，没有画面显示。

### 原因

`renderFrame` 函数使用错误的 YUV 数据：
- 原来传递的是 `frame->data[0]`（只是 Y 平面指针）
- 然后错误地假设 Y/U/V 是连续存储的

实际上 AVFrame 中 Y/U/V 是分开存储的，使用 `linesize` 来计算每行的步长。

### 解决方案

1. 传递整个 AVFrame 指针而不是 `data[0]`
2. 正确使用 Y/U/V 平面的数据和行大小

### 修改文件

- `src/display.cpp`
- `src/video_player.cpp`

### 代码变更

```cpp
// video_player.cpp - 修改前
display_->renderFrame(frame->data[0], frame->width, frame->height);

// video_player.cpp - 修改后
display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);

// display.cpp - renderFrame 函数
// 修改前
int ret = SDL_UpdateYUVTexture(
    texture_, nullptr,
    data, width,
    data + width * height, width / 2,
    data + width * height * 5 / 4, width / 2
);

// 修改后
AVFrame* frame = (AVFrame*)data;
int ret = SDL_UpdateYUVTexture(
    texture_, nullptr,
    frame->data[0], frame->linesize[0],
    frame->data[1], frame->linesize[1],
    frame->data[2], frame->linesize[2]
);
```

---

## 问题 5: 企业级 Quill 日志通道

**日期**: 2026-02-24

### 问题描述

- 旧日志系统只使用 `std::cout/std::cerr`，无法满足企业记录、异步写盘与轮转需求。
- 无运行时配置通道，无法根据环境调整日志目录、文件大小与等级阈值。
- 缺乏健壮性：目录不可写或 Quill 初始化失败时没有明确告警与降级逻辑。

### 原因分析

- 为规避 Quill v6.x API 变更曾临时禁用 Quill，引起功能倒退。
- Logger 逻辑集中在头文件宏内，扩展点有限，新增配置与降级路径困难。

### 解决方案

- 重新启用 Quill，构建异步 Backend + ConsoleSink + RotatingFileSink 双通道；日志按照 `[time][level][thread][logger][category] message` 统一格式输出。
- 新增 `LoggingConfigLoader`，解析 `config/logging.conf` 及 `MVP_LOG_*` 环境变量，非法值自动纠正并输出 `LOG_WARNING`。
- 当 `USE_QUILL_LOGGING` 未定义、目录不可写或 Quill 抛出异常时，自动降级到 stdout/stderr，并保留旧宏行为。
- 同步更新文档（LOGGING.md、VERSION.md、CHANGELOG.md）并提供默认配置文件。

### 修改文件

- `include/logger.h`
- `src/logger.cpp`
- `config/logging.conf`
- `docs/design/LOGGING.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`

---

## 问题 6: 多线程播放架构重构

**日期**: 2026-02-25

### 问题描述

- 原有架构为单线程 playLoop，解码和渲染在同一线程
- 视频解码会阻塞渲染线程，导致画面卡顿
- 音视频同步实现困难
- 队列满时 CPU 忙轮询导致占用过高

### 解决方案

1. **新增 FrameQueue 模板类**
   - 实现线程安全的帧队列
   - 使用 condition_variable 实现阻塞等待，避免 CPU 忙轮询
   - 支持 push/pop/clear/stop 操作

2. **新增 VideoDecodeThread 和 AudioDecodeThread**
   - 独立的视频/音频解码线程
   - 解码后的帧通过 FrameQueue 传递给渲染线程
   - 支持 pause/resume/flush 控制

3. **新增 SyncManager 同步管理器**
   - 支持 AudioMaster/VideoMaster/Free 三种同步模式
   - 实现帧延迟计算
   - 实现跳帧/重复帧策略

4. **重构 VideoPlayer**
   - 从单线程 playLoop 改为多线程 renderLoop
   - 添加 setSyncMode 方法支持同步模式切换
   - 修复 AudioPlayer::play 签名不匹配问题

### 修改文件

- 新增 `include/frame_queue.h`
- 新增 `include/video_decode_thread.h`
- 新增 `include/audio_decode_thread.h`
- 新增 `include/sync_manager.h`
- 新增 `src/video_decode_thread.cpp`
- 新增 `src/audio_decode_thread.cpp`
- 新增 `src/sync_manager.cpp`
- 修改 `include/video_player.h`
- 修改 `include/audio_decoder.h`
- 修改 `src/video_player.cpp`
- 修改 `CMakeLists.txt`

---

## 问题 7: 音频播放架构修复

**日期**: 2026-02-25

### 问题描述

- AudioDecodeThread 解码后的音频通过 FrameQueue 传递给 renderLoop
- renderLoop 逐帧调用 AudioPlayer::play()，导致音频断断续续
- SDL 回调机制需要持续的数据流，当前实现无法满足

### 解决方案

1. 修改 AudioDecodeThread::start() 方法，增加 AudioPlayer* 参数
2. 解码线程解码完成后，直接调用 audio_player_->play() 将数据放入 SDL 队列
3. 移除 renderLoop 中的音频播放代码，由解码线程直接处理

### 修改文件

- `include/audio_decode_thread.h`
- `src/audio_decode_thread.cpp`
- `src/video_player.cpp`

---

## 待解决的问题

### 问题 8: 硬件加速解码支持

**状态**: 待实现

需要添加 CUDA/D3D11VA 硬件加速解码支持，提升解码性能。

---

## 问题 9: VideoFrame/AudioFrame 移动语义缺陷导致崩溃

**日期**: 2026-02-25

### 问题描述

程序启动播放后立即崩溃，错误信息：
```
modern-video-player.exe - 应用程序错误
0x00007FFF7A80DA4C 指令引用了 0xFFFFFFFFFFFFFFFF 内存。该内存不能为 read
```

### 原因分析

`VideoFrame` 和 `AudioFrame` 类缺少正确的移动语义实现。

在 `FrameQueue::pop()` 中使用 `std::move` 将帧移动出队列：
```cpp
frame = std::move(queue_.front());
queue_.pop();
```

由于没有定义移动构造函数和移动赋值运算符，默认的移动操作只是浅拷贝 `frame_` 指针。当原对象析构时调用 `av_frame_free(&frame_)` 释放内存，目标对象的 `frame_` 变成悬空指针。渲染循环访问此悬空指针时导致崩溃。

### 解决方案

1. 为 `VideoFrame` 类添加移动构造函数和移动赋值运算符
2. 为 `AudioFrame` 类添加移动构造函数和移动赋值运算符
3. 显式删除拷贝构造函数和拷贝赋值运算符
4. 移动时将原对象的 `frame_` 指针置为 nullptr，防止析构时释放仍被使用的内存

### 修改文件

- `include/video_decoder.h`
- `src/video_decoder.cpp`
- `include/audio_decoder.h`
- `src/audio_decoder.cpp`

### 代码变更

```cpp
// video_decoder.h - 添加移动语义声明
VideoFrame(VideoFrame&& other) noexcept;
VideoFrame& operator=(VideoFrame&& other) noexcept;
VideoFrame(const VideoFrame&) = delete;
VideoFrame& operator=(const VideoFrame&) = delete;

// video_decoder.cpp - 实现移动构造函数
VideoFrame::VideoFrame(VideoFrame&& other) noexcept
    : frame_(other.frame_)
    , pts_(other.pts_) {
    other.frame_ = nullptr;
}

// audio_decoder.h/cpp - 类似实现
```

---

## 问题 10: 多解码器实例竞争读取导致解码错误

**日期**: 2026-02-25

### 问题描述

播放视频时出现大量 FFmpeg 解码错误：
```
[h264 @ ...] Invalid NAL unit size (0 > 1045).
[h264 @ ...] missing picture in access unit with size 1049
[aac @ ...] channel element 0.0 duplicate
[mov,mp4,m4a,3gp,3g2,mj2 @ ...] DTS 2625751 < 2632415 out of order
```

### 原因分析

在 `VideoPlayer::open()` 中创建了 `video_decoder_` 和 `audio_decoder_` 解码器实例用于获取视频信息。然后在 `play()` -> `initDecodeThreads()` 中又创建了 `VideoDecodeThread` 和 `AudioDecodeThread`，这两个类内部又各自创建了新的解码器实例。

两个解码器同时从同一个 `AVFormatContext` 读取 packet，造成数据竞争，导致解码错误和数据损坏。

### 解决方案

在 `play()` 方法中，调用 `initDecodeThreads()` 之前，先关闭并释放 `video_decoder_` 和 `audio_decoder_`：

```cpp
void VideoPlayer::play() {
    // ...
    playing_.store(true);
    
    video_decoder_.reset();
    audio_decoder_.reset();
    
    if (!initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_)) {
        // ...
    }
}
```

### 修改文件

- `src/video_player.cpp`

---

## 相关文档

- [VERSION.md](./VERSION.md) - 版本记录
- [ARCHITECTURE.md](../design/ARCHITECTURE.md) - 架构设计
- [WINDOWS_SETUP.md](../guides/WINDOWS_SETUP.md) - Windows 配置指南
- [LOGGING.md](../design/LOGGING.md) - 日志系统说明

---

## 问题 13: Core API + Scheduler + Filter 多线程重构落地

**日期**: 2026-03-06

### 问题描述
- 需要按规格引入 Core API、Scheduler 和 Filter 插件框架，并保持 `VideoPlayer` 外部接口稳定。

### 原因分析
- 旧架构以 `VideoPlayer` 聚合大部分职责，缺少可演进的核心层与调度层。
- 缺少 `USE_NEW_PLAYER_CORE` 受控迁移路径下的新核心实现。

### 解决方案
- 新增 `core` 模块：`PlayerCore`、`Scheduler`、`FrameQueue`、`Clock`、`Command`、`Frame`。
- 新增 `filters` 模块：过滤器接口、注册中心、处理管道、亮度/对比度/饱和度内置滤镜。
- `VideoPlayer` 改造为双路径：`USE_NEW_PLAYER_CORE=ON` 走新核心，OFF 保持旧实现。
- 新增 `tests/core_frame_queue_tests.cpp`、`tests/core_clock_tests.cpp`。

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

## 问题 14: 架构收敛为 Core 单路径

**日期**: 2026-03-06

### 问题描述
- 历史上并存的新旧播放链路增加了维护成本和行为不一致风险。

### 原因分析
- 旧链路和新链路共存导致排障路径复杂，且旧链路存在结构性并发隐患。

### 解决方案
- 删除旧链路模块，统一到 `VideoPlayer -> PlayerCore -> Scheduler -> Queue -> Output`。
- 构建系统改为仅编译新核心模块。
- 新增重构文档说明保留文件与职责边界。

### 修改文件
- CMakeLists.txt
- include/video_player.h
- src/video_player.cpp
- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md
- 旧模块头源文件删除（见 DEVELOP_LOG 问题 14）

---

## 问题 15: 小屏窗口过大且拖拽缩放不稳定

**日期**: 2026-03-06

### 问题描述
- 小屏设备播放高分辨率视频时，窗口初始尺寸过大，影响操作。
- 窗口拖拽后部分场景下渲染区域未及时更新，用户感知为“窗口不能调整”。

### 原因分析
- `Display::init()` 直接使用视频原始分辨率创建窗口，未按屏幕可用区域做首屏缩放。
- 事件处理仅监听 `SDL_WINDOWEVENT_RESIZED`，未覆盖 `SDL_WINDOWEVENT_SIZE_CHANGED`。
- 渲染目标区域直接使用窗口宽高，缺少按视频比例计算的目标矩形。

### 解决方案
- 启动时通过 `SDL_GetDisplayUsableBounds()` 计算可用屏幕区域，将初始窗口限制在可用区 90% 内并保持视频比例。
- 同时处理 `SDL_WINDOWEVENT_RESIZED` 与 `SDL_WINDOWEVENT_SIZE_CHANGED`，确保窗口尺寸变化实时生效。
- 按源视频比例计算 `SDL_RenderCopy` 的目标矩形，避免拖拽后画面拉伸。

### 修改文件
- src/display.cpp
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

## 问题 16: 最大化/缩放时画面卡住，并补充基础交互控制

**日期**: 2026-03-07

### 问题描述
- 播放时最大化窗口或拖动缩放窗口，视频画面可能卡住，音频继续播放。
- 缺少进度条、音量调节、拖动进度条等基础交互能力。

### 原因分析
- SDL 窗口事件处理与渲染调用分散在不同线程路径，窗口尺寸变化时更容易触发画面刷新停滞。
- 显示层没有控制条与鼠标交互请求上报能力。

### 解决方案
- 调整事件处理路径：在渲染/空闲渲染路径中轮询 SDL 事件，主线程仅消费交互请求。
- 为 Display 添加控制层绘制：进度条、音量条、暂停状态提示。
- 新增鼠标交互：拖动进度条触发 seek，拖动音量条调节音量。
- PlayerCore 增加对 seek/音量请求的消费执行。

### 修改文件
- include/display.h
- src/display.cpp
- src/core/player_core.cpp
- src/main.cpp
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

## 问题 17: 企业级 MPC-HC 模块骨架落地（阶段二/三推进）

**日期**: 2026-03-07

### 问题描述
- 企业级模块规划已定义，但多数模块缺少代码入口，无法继续并行开发。

### 原因分析
- 旧实现以核心播放链路为主，模块边界不完整，难以分工推进。

### 解决方案
- 新增并接入企业级基础设施和模块骨架：任务队列、帧池、解码线程基类。
- 引入渲染抽象层（`IVideoRenderer` + `RendererFactory`），并让 `PlayerCore` 切换到抽象接口。
- 增加音频均衡器/混音器、解码器工厂、字幕 SRT 解析、播放列表、设置/快捷键、皮肤、插件、格式与流媒体解析模块。
- 完善滤镜基类与音视频滤镜链，补齐音量平衡滤镜。
- 同步更新 tasklist 对应已实现项。

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
- docs/records/DEVELOP_LOG.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md

---

## 问题 18: DASH 解析编译失败与格式能力矩阵缺失

**日期**: 2026-03-07

### 问题描述
- `src/streaming/dash_manifest_parser.cpp` 在 MSVC 下编译失败，阻塞全量构建。
- 缺少一个可直接复用的“运行时格式能力矩阵”入口，不利于单人迭代中快速验证格式覆盖。

### 原因分析
- 原始字符串正则使用了默认分隔符，表达式中出现 `)"` 触发提前终止，导致语法错误。
- 现有格式支持模块虽有基础接口，但缺少统一 CLI 检查入口与主力格式覆盖输出。

### 解决方案
- 修复 DASH 正则：改为自定义 raw-string 分隔符，恢复 MSVC 编译通过。
- 扩展 `FormatSupport`：
  - 增加运行时容器/编解码器枚举（`av_demuxer_iterate` / `av_codec_iterate`）
  - 增加播放目标评估（高分辨率/高帧率/多音道）
- 改造 `main`，新增命令：
  - `--capabilities`
  - `--evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>`
- 增强 `Demuxer` 与 `PlayerCore` 音频链路稳健性（多音道输出参数对齐、重采样器复用等）。

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

---

## 问题 19: D3D11VA 硬解最小闭环与软解回退

**日期**: 2026-03-07

### 问题描述
- 需要在 Windows 下优先利用 D3D11VA 硬解高分辨率/高帧率视频，并确保失败时可自动回退软解。
- 硬件解码输出通常是 GPU 帧或 `NV12`，现有 SDL 渲染链路要求 `YUV420P`，存在格式不匹配风险。

### 解决方案
- `PlayerCore` 增加 D3D11VA 尝试逻辑：
  - 检测 codec 的 D3D11VA HW config；
  - 创建 `AV_HWDEVICE_TYPE_D3D11VA` 设备上下文；
  - 绑定 `get_format` 回调选择硬件像素格式。
- 若 `avcodec_open2` 在硬解路径失败，自动重建解码上下文并回退到软解。
- 新增视频帧输出规整链路：
  - 硬件帧先 `av_hwframe_transfer_data` 转到系统内存；
  - 非 `YUV420P` 帧统一经 `sws_scale` 转为 `YUV420P` 再进入渲染。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp

---

## 问题 20: 探测入口与格式回归脚本落地

**日期**: 2026-03-07

### 问题描述
- 需要把格式覆盖验证从“手工打开视频观察”升级为“可重复的命令行回归”。
- 现有能力入口只有总体能力评估，缺少单文件探测和批量样本报告。

### 解决方案
- 在 `main` 中新增 `--probe-file <media_file>`：输出 `probe.*` 机器可读字段，包含容器/视频/音频状态、分辨率、帧率、声道与建议信息。
- 新增 `tools/format_regression/run_format_regression.ps1`：
  - 读取 `tools/format_regression/format_samples.csv`；
  - 逐个调用 `--probe-file`；
  - 生成 `docs/reports/FORMAT_REGRESSION_*.md` 报告；
  - 返回码语义：`0=全部PASS`，`1=存在PARTIAL`，`2=存在FAIL`。
- 补充 `docs/workflows/FORMAT_REGRESSION.md` 与文档索引，便于在 VS2022/PowerShell 下直接执行。

### 修改文件
- src/main.cpp
- tools/format_regression/run_format_regression.ps1
- tools/format_regression/format_samples.csv
- docs/workflows/FORMAT_REGRESSION.md
- docs/README.md

---

## 问题 21: GitHub Actions 自动格式回归接入

**日期**: 2026-03-07

### 问题描述
- 回归链路虽可在本地运行，但缺少 PR/主分支自动执行，无法在提交阶段及时拦截格式退化。
- Windows CI 环境与本地依赖发现方式不一致，需补齐构建与脚本兼容性。

### 解决方案
- 新增工作流 `.github/workflows/format-regression.yml`：
  - 在 `windows-latest` 下载 `SDL2/FFmpeg` 预编译包并构建 `Debug`；
  - 执行 `download_test_samples.ps1` 与 `run_all_checks.ps1`；
  - 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 报告产物。
- 调整 `CMakeLists.txt`：
  - Windows 下优先识别 `SDL2::`、`FFMPEG::` 与 `unofficial::ffmpeg::` 导入目标；
  - 保留 `external/` 目录回退逻辑，兼容本地既有构建。
- 调整 `download_test_samples.ps1`：
  - `-FfmpegPath` 支持 PATH 命令名（如 `ffmpeg`），便于 CI 直接调用。
- 更新回归文档与任务清单状态，补齐自动回归入口说明。

### 修改文件
- .github/workflows/format-regression.yml
- CMakeLists.txt
- tools/download_test_samples.ps1
- docs/workflows/FORMAT_REGRESSION.md
- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 22: 播放列表主链路、设置持久化与快捷键首版接入

**日期**: 2026-03-07

### 问题描述
- 播放器主流程仅支持单文件，不支持上一首/下一首与 EOF 自动切换。
- 设置模块未接入运行链路，启动/退出时无法恢复音量和播放速度。
- 默认快捷键缺失关键能力（相对 seek、变速、静音、播放列表切换）。

### 解决方案
- 主链路接入 `PlaylistManager`：
  - 支持命令行传入多个媒体文件；
  - 支持传入 `.m3u8` 作为播放列表；
  - 支持 `PageUp/PageDown` 上一首/下一首；
  - EOF 自动切换下一项。
- 主链路接入 `SettingsManager`：
  - 启动时加载 `config/player_settings.ini`；
  - 缺失或解析失败时回退默认值（音量 100%、速度 1.0x、恢复上次索引）；
  - 退出时保存当前音量、播放速度和播放列表索引。
- 扩展 SDL 事件到播放器控制链路：
  - `Left/Right` seek ±5 秒；
  - `Ctrl+Left/Ctrl+Right` seek ±30 秒；
  - `[`/`]` 调速，`R` 恢复 1.0x；
  - `M` 静音/恢复；
  - `Enter/Alt+Enter/F` 全屏切换；
  - `Esc` 全屏态退全屏，窗口态退出。

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

### 问题描述
- 当前仓库保留了两个 Core 相关测试目标与测试文件，但本次需求要求移除这两项测试内容并删除文件。
- 若仅删除测试源码而不清理构建脚本，会导致构建配置存在悬挂路径风险。

### 解决方案
- 从 `CMakeLists.txt` 移除：
  - `BUILD_CORE_TESTS` 选项；
  - `core_frame_queue_tests`、`core_clock_tests` 两个测试目标；
  - `core_tests` 聚合目标。
- 删除测试文件：
  - `tests/core_frame_queue_tests.cpp`
  - `tests/core_clock_tests.cpp`
- 同步更新变更文档，确保记录与当前仓库状态一致。

### 修改文件
- CMakeLists.txt
- tests/core_frame_queue_tests.cpp（删除）
- tests/core_clock_tests.cpp（删除）
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 24: 外挂字幕加载入口（SRT）接入主流程

**日期**: 2026-03-07

### 问题描述
- 任务清单 `1.1.1` 要求支持外挂字幕加载入口，但当前主流程只有视频/音频播放链路，未提供外部字幕文件入口。
- 项目已存在 `subtitle::SrtParser`，但未接入 `VideoPlayer` 与命令行参数。

### 解决方案
- 在 `VideoPlayer` 增加外挂字幕加载接口：
  - `loadExternalSubtitle()` / `clearExternalSubtitle()`；
  - 支持 `.srt` 文件解析与容错日志；
  - 暴露已加载字幕路径与条目数量，便于后续渲染接入。
- 在 `main` 增加命令行入口：
  - 新增 `--subtitle <file.srt>`；
  - 保持现有播放列表参数逻辑；
  - 未显式传参时，自动尝试加载与媒体同名的 `.srt`。
- 更新任务清单，标记 `1.1.1 外挂字幕加载入口` 已完成。

### 修改文件
- include/video_player.h
- src/video_player.cpp
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 25: 字幕渲染叠加与播放时序同步接入

**日期**: 2026-03-07

### 问题描述
- 任务清单 `1.1.2` 要求字幕可渲染叠加到画面，但现有渲染接口没有字幕文本通道。
- 任务清单 `1.1.3` 要求字幕与播放/暂停/seek 同步，但主播放时钟链路没有字幕时间轴更新逻辑。

### 解决方案
- 扩展渲染抽象：
  - 在 `IVideoRenderer` 增加 `setSubtitleText()`；
  - SDL 渲染器转发字幕文本到 `Display`；
  - D3D11/OpenGL 先提供兼容桩实现，保持接口一致。
- 在 `Display` 增加字幕叠加层：
  - 新增字幕状态存储与线程安全更新；
  - 在视频帧渲染后、控制条渲染前绘制字幕面板；
  - 支持多行字幕、超长截断与基础可读性样式（阴影+半透明底板）。
  - 当前使用轻量字模渲染，非 ASCII 字符会降级显示。
- 在 `PlayerCore` 增加字幕时间轴驱动：
  - 新增外挂字幕轨道状态与索引缓存；
  - 渲染帧路径与空闲事件路径均调用 `updateSubtitleOverlay()`；
  - 基于当前播放时间选择活跃字幕，覆盖播放、暂停与 seek 场景；
  - 修复锁内调用渲染接口的问题，避免在字幕互斥锁内触发渲染回调。
- 调整 `VideoPlayer::open()` 字幕状态处理，消除“先清空再判断加载”的矛盾逻辑。
- 更新任务清单，标记 `1.1.2`、`1.1.3` 已完成。

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

---

## 问题 26: 字幕开关控制与字幕加载异常处理完善

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.1.4` 要求字幕开关与异常处理，但当前字幕仅支持“加载后显示”，缺少运行时开关。
- 外挂字幕加载路径在文件系统异常场景下容错不足，影响稳定性预期。

### 解决方案
- 增加字幕开关控制链路（按键 `V`）：
  - `Display` 新增字幕开关请求；
  - `Renderer` 抽象新增 `consumeToggleSubtitleRequest()`；
  - `PlayerCore` 新增字幕显示状态管理与切换接口；
  - 关闭字幕时立即清空叠加层，开启时按当前播放时间恢复同步。
- 增强外挂字幕异常处理：
  - `VideoPlayer::loadExternalSubtitle()` 改为使用 `std::error_code` 路径检查；
  - 捕获解析器异常并降级为告警日志，不中断播放主流程；
  - 保持“加载失败自动清空旧字幕”的状态一致性。
- 更新帮助信息，补充 `V - Toggle subtitles on/off`。
- 更新任务清单，标记 `1.1.4` 已完成。

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

---

## 问题 27: 快捷键配置持久化接入（hotkey.*）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.3.2` 要求支持键位配置持久化，但当前快捷键逻辑固定写死在 `Display` 事件分支中。
- `HotkeyManager` 仅有骨架实现，未接入主播放链，无法通过配置文件保持自定义键位。

### 解决方案
- 扩展 `HotkeyManager`：
  - 对齐当前首版快捷键默认映射（播放、seek、音量、静音、变速、切集、字幕开关、全屏、退出）；
  - 增加配置序列化能力：`actionConfigKey`、`keyCodeToToken`、`keyCodeFromToken`。
- 将快捷键映射接入渲染输入链：
  - `Display` 事件处理改为由 `HotkeyManager` 驱动；
  - 保留 `Esc` 与 `Enter` 的兼容行为；
  - `Renderer`/`PlayerCore`/`VideoPlayer` 增加热键管理透传接口。
- 在 `main` 的设置加载/保存流程中接入 `hotkey.*`：
  - 启动读取并应用 `player_settings.ini` 的 `hotkey.*`；
  - 非法键位配置降级为默认并记录告警；
  - 退出时将当前键位回写配置，实现持久化。
- 更新默认配置样例 `config/player_settings.ini`，补齐全部 `hotkey.*` 项。
- 更新任务清单，标记 `1.3.2` 已完成。

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

---

## 问题 28: 快捷键冲突检测与恢复默认能力

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.3.3` 要求支持键位冲突检测与恢复默认。
- 现有 `hotkey.*` 持久化已可工作，但重复键位配置会产生动作冲突，且缺少“一键回到默认”能力。

### 解决方案
- 扩展 `HotkeyManager`：
  - 新增 `findConflicts()` / `hasConflicts()`，用于检测重复键位绑定；
  - 新增 `resetToDefaults()`，统一恢复默认键位映射。
- 在热键配置加载流程中加入冲突治理：
  - 启动时先应用配置到候选映射，再执行冲突检测；
  - 若发现冲突，记录冲突动作与键位日志，自动回退默认键位；
  - 对非法 token 保留默认并输出告警。
- 新增恢复默认开关：
  - 在 `player_settings.ini` 增加 `hotkey.restore_defaults`；
  - 设置为 `true` 后下次启动自动恢复默认并回写为 `false`。
- 更新帮助输出，补充恢复默认说明。
- 更新任务清单，标记 `1.3.3` 已完成。

### 修改文件
- include/input/hotkey_manager.h
- src/input/hotkey_manager.cpp
- src/main.cpp
- config/player_settings.ini
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 29: M1 验收 1.4.1（SRT seek 同步自检命令落地）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.1` 要求“`SRT 字幕可用且 seek 后同步`”，现有实现缺少可重复执行的验收入口。
- 若只依赖人工播放观察，回归成本高且难以稳定复现。

### 原因分析
- 字幕时间轴匹配逻辑仅存在于 `PlayerCore` 内部，无法单独验证“顺序播放 + seek 跳转”两类场景。

### 解决方案
- 提取公共时间轴函数 `subtitle::resolveActiveSubtitleIndex(...)`，并由 `PlayerCore` 复用。
- 在 `main` 增加 `--subtitle-sync-check <subtitle.srt>` 命令：
  - 顺序时间轴检查（ordered）；
  - 非顺序 seek 跳转检查（seek）；
  - 输出 `mismatches` 与 `PASS/FAIL`。
- 新增样例字幕 `samples/subtitle/subtitle_seek_sync_sample.srt` 和本地报告 `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`。
- 任务清单 `1.4.1` 标记完成。

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

---

## 问题 30: M1 验收 1.4.2（播放列表连续播放 5 文件自检）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.2` 要求“播放列表连续播放 5 文件通过”，但缺少可重复执行的验收命令。
- 仅靠手工点击验证，回归效率低且结果不稳定。

### 原因分析
- 当前主流程具备 EOF 自动切换逻辑，但没有结构化输出可用于快速验收。

### 解决方案
- 在 `main` 新增 `--playlist-flow-check` 命令：
  - 读取输入并构建播放列表；
  - 校验至少 5 条目；
  - 对前 5 条执行可打开探测（`--probe-file` 同源逻辑）；
  - 模拟 EOF 自动切换顺序，验证 `0,1,2,3,4` 连续覆盖；
  - 输出 `PASS/FAIL` 与失败索引。
- 新增本地报告 `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`。
- 更新任务清单，标记 `1.4.2` 完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md
- samples/README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 31: M1 验收 1.4.3（设置重启恢复自检）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `1.4.3` 要求“设置重启后可恢复”，但缺少可重复执行的验收入口。
- 仅手工重启验证难以覆盖关键字段（音量、速度、恢复标志、播放列表索引、快捷键）。

### 原因分析
- 主流程已有 `loadAppSettings/saveAppSettings`，但没有独立命令行输出用于回归验收。

### 解决方案
- 在 `main` 新增 `--settings-persistence-check [settings_file]` 命令：
  - 写入一组测试设置；
  - 重新加载并逐项校验 volume/speed/resume/index/hotkey；
  - 输出 `settings-persistence-check.result=PASS/FAIL`。
- 默认使用系统临时目录进行检查，不污染项目内 `config/player_settings.ini`。
- 新增本地报告 `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`。
- 更新任务清单，标记 `1.4.3` 完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 32: M2 2.1.2（容器矩阵补齐 mov/avi/m2ts）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.2` 目标要求容器覆盖 `mp4/mkv/mov/avi/webm/flv/ts/m2ts`。
- 现有格式回归样本仅覆盖了 `mp4/mkv/webm/flv/ts`，缺少 `mov/avi/m2ts` 实测闭环。

### 原因分析
- 回归样本列表与自动生成脚本未包含 `mov/avi/m2ts` 输出，导致容器矩阵覆盖不完整。

### 解决方案
- 扩展样本矩阵 `format_samples.csv`，新增：
  - `mov`（h264 + aac）
  - `avi`（h264 + mp3）
  - `m2ts`（h264 + ac3）
- 扩展 `tools/download_test_samples.ps1`：
  - 新增 `samples/mov`、`samples/avi`、`samples/m2ts` 目录生成；
  - 新增三类容器样本生成命令。
- 更新样本目录文档与忽略规则，补齐 `.gitkeep`。
- 执行本地回归并更新报告：`docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`。
- 任务清单 `2.1.2` 标记完成。

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

---

## 问题 33: M2 2.1.3（视频编码矩阵补齐 MPEG-2）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.3` 目标为视频编码支持 `H.264/H.265/VP9/AV1/MPEG-2`。
- 现有回归样本已覆盖前四项，但缺少 `MPEG-2` 视频编码样本，验收闭环不完整。

### 原因分析
- `format_samples.csv` 与 `download_test_samples.ps1` 未包含 `mpeg2video` 样本。

### 解决方案
- 在回归样本矩阵中新增 `mpeg2video` 条目：
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`
- 在样本生成脚本中新增 MPEG-2 样本生成流程：
  - 视频编码 `mpeg2video`；
  - 音频编码 `ac3`；
  - 容器 `mpegts`。
- 运行本地格式回归并更新报告。
- 任务清单 `2.1.3` 标记完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 34: M2 2.1.4（音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.1.4` 目标要求音频编码覆盖 `AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`。
- 现有回归样本缺少 `E-AC3/DTS/Vorbis/PCM` 实测，验收闭环不完整。

### 原因分析
- 回归样本清单和自动样本生成脚本未覆盖上述四类音频编码。

### 解决方案
- 扩展 `format_samples.csv` 新增四条样本：
  - `h264 + eac3 (mkv)`
  - `h264 + dts (mkv)`
  - `vp9 + vorbis (webm)`
  - `h264 + pcm_s16le (mov)`
- 扩展 `download_test_samples.ps1` 生成流程并修复 DTS 编码：
  - `dca` 编码使用 `-strict -2` 通过实验特性限制。
- 扩展回归脚本兼容等价编码名：
  - `dts` <-> `dca`
  - `hevc` <-> `h265`
  - `pcm` <-> `pcm_*`
- 运行本地回归并更新报告。
- 任务清单 `2.1.4` 标记完成。

### 修改文件
- tools/format_regression/format_samples.csv
- tools/download_test_samples.ps1
- tools/format_regression/run_format_regression.ps1
- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 35: M3 3.1.1（DecoderFactory 接入真实初始化流程）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.1.1` 要求 `DecoderFactory` 接入真实解码初始化流程。
- 现有链路中，`DecoderFactory` 未形成统一的“候选后端 -> 逐个尝试 -> 失败回退”主流程。

### 原因分析
- `DecoderFactory` 仅提供“最佳后端”选择，缺少候选序列接口。
- `PlayerCore::initDecoders` 的初始化与回退逻辑耦合在局部条件分支中，不利于统一扩展。

### 解决方案
- `DecoderFactory` 新增 `selectBackendOrder(codec_name, prefer_hardware)`，输出按优先级排序的后端候选序列，并保留软件解码兜底。
- `PlayerCore::initDecoders` 改为按候选序列逐个尝试初始化：
  - 对每个候选后端重建并配置 `AVCodecContext`；
  - 后端配置失败或 `avcodec_open2` 失败时自动切换下一个候选；
  - 成功后统一记录最终解码后端。
- `tryConfigureD3D11HardwareDecode` 调整为纯 D3D11 配置职责，不再在函数内做后端策略决策。
- 任务清单 `3.1.1` 标记完成。

### 修改文件
- include/decoder/decoder_factory.h
- src/decoder/decoder_factory.cpp
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 36: M3 3.1.2（D3D11VA 初始化失败回退软解兜底）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.1.2` 要求 D3D11VA 初始化失败时可靠回退软解。
- 现有逻辑在像素格式协商失败场景下仅日志提示，未显式更新后端状态，存在状态不一致风险。

### 原因分析
- `selectVideoPixelFormat` 在协商不到 D3D11VA 格式时会返回软件格式；
- 但此前没有同步切换 `video_decoder_backend_` 与硬件像素格式状态。

### 解决方案
- 在 `PlayerCore::selectVideoPixelFormat` 中补充显式软解降级：
  - `video_hw_pixel_fmt_ = AV_PIX_FMT_NONE`；
  - `video_decoder_backend_ = Software`。
- 在 `initDecoders` 后端尝试链路中补充“D3D11VA 协商阶段降级软解”日志。
- 更新任务清单，标记 `3.1.2` 完成。

### 修改文件
- src/core/player_core.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 37: M3 3.2.1（D3D11 渲染最小可用链路）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.2.1` 要求 D3D11 渲染具备最小可用能力（`init/upload/present`）。
- 现有 `D3D11VideoRenderer` 为桩实现，无法初始化、无法消费帧，也无法呈现。

### 原因分析
- 渲染后端接口已定义，但 D3D11 后端未接入实际渲染链路；
- 缺少“当前 SDL renderer 实际后端”的可观测能力，无法判定是否真的跑在 D3D11。

### 解决方案
- 在 `Display` 中新增：
  - 渲染驱动偏好设置（`setPreferredRendererDriver`）；
  - 当前渲染后端观测（`currentRendererDriver` / `isUsingRendererDriver`）。
- 完整实现 `D3D11VideoRenderer` 最小功能：
  - `init` 时请求 `direct3d11` 驱动并创建渲染链路；
  - 接通 `renderFrame`（上传）、`present`（呈现）、`clear`、事件与控制请求透传。
- 初始化后校验实际后端：
  - 若非 `direct3d11/d3d11`，`init` 失败并记录日志，交由上层回退 `SoftwareSDL`。
- 更新任务清单，标记 `3.2.1` 完成。

### 修改文件
- include/display.h
- src/display.cpp
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 38: M3 3.3.2（渲染失败降级不中断播放）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.3.2` 要求渲染失败时自动降级且不中断播放。
- 现有实现缺少可重复执行的自动化验收入口，难以稳定验证回退链路。

### 原因分析
- D3D11 初始化失败场景此前主要依赖人工触发，无法作为稳定回归项。
- 缺少渲染后端/解码后端的运行时可观测字段，不便于命令化验证。

### 解决方案
- 扩展渲染接口，增加后端名称查询（`rendererBackendName`）。
- 扩展播放器对外接口，暴露当前渲染后端和解码后端名称。
- 新增命令 `--renderer-fallback-check <media_file>`：
  - 通过环境变量 `MVP_D3D11_DRIVER_HINT=software` 强制 D3D11 渲染初始化失败；
  - 验证主链路是否自动回退到 `SoftwareSDL` 并能进入播放循环；
  - 输出 `renderer-fallback-check.*` 字段和 `PASS/FAIL`。
- 新增本地回归报告 `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`。
- 任务清单 `3.3.2` 标记完成。

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

---

## 问题 39: M3 3.3.1（Windows 软解+硬解主力样本可播）
**日期**: 2026-03-08

### 问题描述
- 任务清单 `3.3.1` 要求 Windows 下硬解与软解主力样本均可稳定进入播放链路。
- 现有检查路径缺少统一聚合命令，且同进程连续会话在部分场景存在停止阶段卡死风险。

### 原因分析
- 之前的 `--windows-backend-check` 在同进程顺序执行硬解+软解，会触发二次会话资源回收不稳定。
- 缺少可复用的会话级诊断输出，不利于回归报告自动化采集。

### 解决方案
- 新增会话级命令 `--windows-backend-session-check <media_file> <hard|soft>`，输出结构化字段并返回模式结果。
- 将聚合命令 `--windows-backend-check <media_file>` 改为父进程拉起两个子进程（hard/soft）并汇总结果，隔离会话状态。
- 在 Windows 下使用 `CreateProcess` 重定向输出，避免 shell 重定向解析不稳定。
- 新增本地回归报告 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`。
- 完成任务清单同步：`3.3.1`、`3.3.3` 标记完成。

### 修改文件
- src/main.cpp
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 40: M4 4.1（章节导航：上一章/下一章）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `4.1` 需要支持章节导航（上一章/下一章）。
- 当前播放链路缺少章节元数据消费与章节跳转入口，无法通过快捷键直接跳章。

### 原因分析
- `Demuxer` 虽能读取媒体基本信息，但未提取 `AVChapter` 数据。
- 输入链路缺少章节动作请求（`Display -> Renderer -> PlayerCore`）。
- 主流程缺少可重复执行的章节导航验收命令。

### 解决方案
- 在 `Demuxer` 中解析章节元数据，新增 `ChapterInfo` 与 `MediaInfo::chapters`。
- 新增章节导航动作与请求链路：
  - `HotkeyManager` 增加 `PreviousChapter/NextChapter`；
  - 默认键位绑定 `HOME/END`；
  - `Display`、渲染器接口、`PlayerCore`、`VideoPlayer` 全链路透传章节请求。
- 在 `PlayerCore` 中新增章节跳转能力：
  - 打开媒体时构建章节时间点；
  - `seekToNextChapter()` / `seekToPreviousChapter()` 执行跳章。
- 在 `main` 新增 `--chapter-nav-check <media_file>` 自检命令，并更新帮助输出。
- 新增本地报告 `docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`，记录章节样本与 PASS 结果。
- 更新任务清单，标记 `4.1` 已完成。

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

---

## 问题 41: M4 4.2（A-B Repeat）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `4.2` 需要支持 A-B Repeat。
- 当前播放器缺少 A/B/C 快捷键动作与循环区间控制逻辑，无法在播放中进行区间重复。

### 原因分析
- 输入链路未定义 A-B Repeat 请求动作。
- `PlayerCore` 缺少 A 点/B 点状态管理与循环触发逻辑。
- 缺少可重复执行的 A-B Repeat 验收命令。

### 解决方案
- 扩展热键动作：
  - 新增 `SetABRepeatStart` / `SetABRepeatEnd` / `ClearABRepeat`；
  - 默认键位绑定 `A/B/C`。
- 扩展请求链路：
  - `Display` 增加 A-B Repeat 请求标记与消费接口；
  - `IVideoRenderer` 与各渲染器实现增加透传接口。
- `PlayerCore` 新增 A-B Repeat 状态与控制：
  - `setABRepeatStart()` 设置 A 点并清空旧 B 点；
  - `setABRepeatEnd()` 设置 B 点并启用循环；
  - `clearABRepeat()` 清除循环；
  - `handleABRepeatLoop()` 在播放中检测到达 B 点后自动 seek 回 A 点。
- `VideoPlayer` 暴露 A-B Repeat API 供主流程与验收命令调用。
- 新增 `--ab-repeat-check <media_file>` 命令，输出 `ab-repeat-check.*` 字段和 `PASS/FAIL`。
- 修复回归检查冲突：
  - `--settings-persistence-check` 的测试键位由 `b` 调整为 `x`，避免与新默认热键冲突。
- 新增本地报告 `docs/reports/AB_REPEAT_LOCAL_CHECK.md`。
- 更新任务清单，标记 `4.2` 已完成。

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

---

## 问题 42: M4 4.3（截图）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `4.3` 需要支持截图。
- 当前实现虽然已经接入截图热键和 `--screenshot-check`，但暂停态没有缓存最近一帧，截图请求无法稳定保存当前画面。

### 原因分析
- 截图落盘逻辑只绑定在渲染线程的新帧处理路径上。
- 播放暂停后，调度器不再继续送帧，导致暂停态截图没有可消费的图像数据源。

### 解决方案
- `PlayerCore` 新增最近渲染帧缓存，并支持从缓存帧直接落盘截图。
- `requestScreenshot()` 调整为：播放中异步排队，暂停态直接使用缓存帧保存。
- `--screenshot-check` 升级为暂停态截图验收，覆盖这次修复的核心场景。
- 更新快捷键文档，补充 `S` 截图、章节导航、A-B Repeat、字幕开关等现有能力说明。

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

---

## 问题 43: `MPC_HC_GAP_ANALYSIS` 评估结论过期

**日期**: 2026-03-08

### 问题描述
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 仍保留旧结论，把多项已接入主流程的能力写成“骨架/未接入”。
- 这会影响后续迭代优先级判断，也会让文档读者对当前实现进度形成错误认知。

### 原因分析
- 里程碑文档、开发日志和本地报告持续更新，但差距评估文档没有同步维护。
- 旧评估主要依据“代码骨架是否存在”，没有吸收后续 `docs/reports/*` 验收结果。

### 解决方案
- 将 `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 更新到 2026-03-08 口径。
- 重写“当前已具备能力”“差距总览”“关键未实现功能清单”“代码层证据摘要”“建议里程碑”。
- 新增“验收与报告证据”章节，把字幕、播放列表、设置、快捷键、D3D11/回退、章节导航、A-B Repeat、截图、格式矩阵的本地报告纳入评估依据。
- 更新 `docs/README.md`，补充本次文档对齐说明。

### 修改文件
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 问题 44: `docs/records/VERSION.md` 历史路径描述过期

**日期**: 2026-03-08

### 问题描述
- `docs/records/VERSION.md` 的“阶段一”历史章节仍把早期 decoder/thread/test 文件名写成当前仓库结构说明。
- 在基于文档遍历仓库时，容易把已移除的旧实现与当前 `PlayerCore + Scheduler + core/*` 主链混淆。

### 原因分析
- `2026-03-06` 之后架构已经收敛，但版本文档保留了早期文件级描述。
- 后续功能持续追加新记录，未回头清理“当前阶段”“下一步计划”这类已过时口径。

### 解决方案
- 将阶段一标题改为“历史起点”，并补充这是早期实现基线的说明。
- 将 `video_decoder` / `audio_decoder`、`VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 等旧路径改写为能力级历史记录，并映射到当前 `core/*` 主链。
- 将“下一步计划”、`USE_NEW_PLAYER_CORE`、临时 `tests/core_*` 等表述改写为历史说明，避免误导当前进度判断。
- 同步补写版本文档的更新日志条目。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

---

## 问题 45: README 与架构文档仍混用旧主链表述

**日期**: 2026-03-08

### 问题描述
- `README.md`、`README_ZH.md` 的项目结构和架构示意仍展示 `video_decoder` / `audio_decoder` 等旧路径。
- `docs/design/ARCHITECTURE.md` 也含有“当前实现”字样和旧模块命名，容易与现行 `PlayerCore + Scheduler + core/*` 主链混淆。

### 原因分析
- 根目录 README 的目录树和架构图来自早期单体/旧多线程实现阶段，后续未随重构同步刷新。
- `docs/design/ARCHITECTURE.md` 保留了大量历史设计内容，但缺少清晰的“历史/当前”边界提示。

### 解决方案
- 更新 `README.md` 与 `README_ZH.md` 的项目结构、架构示意和文档链接，统一指向当前主链。
- 在 `docs/design/ARCHITECTURE.md` 顶部增加状态说明，并将旧模块章节显式标记为“历史实现”。
- 将日志示例从 `spdlog` 改为当前项目使用的 `Quill` 宏接口。
- 更新 `docs/README.md`，区分历史架构基线与当前重构说明。

### 修改文件
- README.md
- README_ZH.md
- docs/design/ARCHITECTURE.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

---

## 问题 46: 实现教程与迭代计划缺少历史/当前边界说明

**日期**: 2026-03-08

### 问题描述
- `docs/guides/IMPLEMENTATION.md` 仍以早期 `video_decoder/audio_decoder/playLoop` 原型路径讲解实现步骤，容易被误读为当前仓库的逐文件开发指南。
- `docs/plans/MPC_HC_ITERATION_PLAN.md` 是 `2026-03-07` 的规划快照，但未明确说明部分计划项已在 `2026-03-08` 前后落地。

### 原因分析
- 这两份文档本身仍有参考价值，但缺少“历史教程 / 计划快照 / 当前进度”之间的边界说明。
- 当用户按文档遍历仓库时，会把教程示例和规划文字误当作现行结构与当前待办。

### 解决方案
- 为 `docs/guides/IMPLEMENTATION.md` 增加状态说明，明确其属于早期原型教程，并指向当前主链参考文档。
- 为 `docs/plans/MPC_HC_ITERATION_PLAN.md` 增加状态说明，明确其属于 `2026-03-07` 的计划快照，并补充当前进度参考入口。
- 更新 `docs/README.md`、`README.md`、`README_ZH.md` 的文档说明，统一区分历史教程、规划快照与当前实现说明。

### 修改文件
- README.md
- README_ZH.md
- docs/guides/IMPLEMENTATION.md
- docs/plans/MPC_HC_ITERATION_PLAN.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

---

## 问题 47: 辅助说明文档仍缺少当前入口与状态边界

**日期**: 2026-03-08

### 问题描述
- `docs/design/FILTERS.md`、`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`、`docs/guides/WINDOWS_SETUP.md` 虽然内容仍有参考价值，但缺少对“当前主流程入口”和“文档适用范围”的明确说明。
- 其中 `docs/guides/WINDOWS_SETUP.md` 还保留了与当前 `CMakeLists.txt` 不完全一致的手动配置示例，容易误导 Windows 构建路径。

### 原因分析
- 这几份文档分别服务于滤镜、能力参考、Windows 环境配置，长期迭代后内容没有统一补齐“当前口径”说明。
- 文档读者如果直接按旧描述执行，可能会把参考性内容误当成当前唯一入口，或沿用过时的参数传递方式。

### 解决方案
- 为 `docs/design/FILTERS.md` 增加状态说明，区分当前生效的 `FilterPipeline` 主链与预留链式封装。
- 为 `docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 增加适用范围说明，并将总进度参考入口指向差距评估、版本记录和变更日志。
- 为 `docs/guides/WINDOWS_SETUP.md` 增加当前依赖探测顺序说明，移除误导性的 `SDL2_DIR` / `FFMPEG_DIR` 传参示例，改为与现有 `CMakeLists.txt` 一致的仓库内 `external/*` 回退口径。
- 更新 `docs/README.md` 索引与本轮文档更新记录。

### 修改文件
- docs/design/FILTERS.md
- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md
- docs/guides/WINDOWS_SETUP.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

---

## 问题 48: 根 README 故障排除与历史问题归档仍有旧口径

**日期**: 2026-03-08

### 问题描述
- 根目录 `README.md` / `README_ZH.md` 的 Windows 故障排除仍建议使用 `FFMPEG_DIR` 传参，和当前 `CMakeLists.txt` 的依赖探测方式不一致。
- `docs/analysis/video-stream-index-fix.md` 是早期原型阶段的问题分析，但缺少“历史归档”说明，容易让读者误以为其中的 `playLoop` / `video_decoder.cpp` 仍属当前主链。

### 原因分析
- README 的故障排除段落保留了旧手动依赖用法，没有跟随 Windows 构建入口一起更新。
- 历史问题分析文档内容本身有价值，但需要显式说明其适用阶段。

### 解决方案
- 将 README 中 FFmpeg 故障排除改为当前推荐口径：优先说明 `vcpkg` toolchain，手动安装则使用仓库内 `external/ffmpeg/` 回退布局。
- 为 `docs/analysis/video-stream-index-fix.md` 增加状态说明，标记为早期原型问题分析归档。
- 在 `docs/README.md` 中补充该历史分析文档的索引与本轮更新记录。

### 修改文件
- README.md
- README_ZH.md
- docs/analysis/video-stream-index-fix.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md

---

## 问题 49: 缺少独立的文档巡检总表

**日期**: 2026-03-08

### 问题描述
- 前几轮文档整理已经完成，但巡检结果分散在对话、日志和多个提交中，缺少一份独立的总表文档。
- 后续维护者如果想快速了解“哪些文档已收敛、哪些内容属于历史保留、为什么要保留”，需要来回翻阅多个文件。

### 原因分析
- 现有 `CHANGELOG.md` / `DEVELOP_LOG.md` 适合记录过程，但不适合作为面向后续维护的摘要报告。
- `docs/README.md` 虽然是索引入口，但没有独立承载本轮巡检结论的专题文档。

### 解决方案
- 新增 `docs/analysis/DOC_AUDIT_2026-03-08.md`，集中归档本轮文档巡检的范围、方法、已完成对齐项、保留历史内容、后续维护建议与关联提交。
- 更新 `docs/README.md`，把该报告加入索引，并补一条本轮更新记录。

### 修改文件
- docs/analysis/DOC_AUDIT_2026-03-08.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md


---

## 问题 50: M4 4.4：暂停态帧步进接入与验收

**日期**: 2026-03-08

### 问题描述
- 任务清单 `4.4` 要求支持暂停态帧步进。
- 当前播放器虽然已有暂停、截图、章节导航和 A-B Repeat，但缺少可直接逐帧检查画面的交互入口。

### 原因分析
- 输入层没有单独的帧步进动作，也没有对应的默认键位。
- `PlayerCore` 的暂停态只会冻结调度器，缺少“seek 后刷新目标帧”的单帧渲染路径。
- 音频消费线程在暂停态仍会依据旧 `playback_pts` 回写位置，导致单帧步进后的时间点可能被音频时钟覆盖。

### 解决方案
- 为热键系统新增 `step_frame_backward` / `step_frame_forward` 动作，默认绑定 `,` / `.`。
- 在 `Display -> Renderer -> PlayerCore` 链路新增帧步进请求通道。
- `PlayerCore` 新增暂停态帧步进能力：
  - 估算单帧步长；
  - 通过 seek 刷新音视频状态；
  - 主动渲染目标时间点的首帧并保持暂停。
- 收紧音频消费线程的位置回写条件，避免暂停态覆盖步进结果。
- 在 `main` 新增 `--frame-step-check <media_file>` 验收命令，并同步 README / 版本文档 / 差距评估 / 任务清单。

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

---

## ?? 51: M4 4.5???/??????????

**??**: 2026-03-08

### ????
- ???? `4.5` ????????????????
- ??????????????????????????????????????????????????

### ????
- ?????????????????????? `J/K` ? `Ctrl+J/K` ?????????
- `PlayerCore` ????????????????????????????? PTS????????/??????????
- ?????????????????/???????????????????

### ????
- ??????????? `J / K` ??? `- / +100ms`??? `Ctrl` ?????? `- / +100ms`?
- ? `Display -> Renderer -> PlayerCore -> VideoPlayer` ??????/??????? API?
- ?????????????????????????? PTS ?????????????????????????
- ???????? `player.audio_delay_ms` / `player.subtitle_delay_ms`???? `--settings-persistence-check`?
- ?? `--delay-adjust-check <media_file> <subtitle.srt>` ????????????

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

---

## ?? 52: M4 4.6????? `1..9` ????

**??**: 2026-03-08

### ????
- `4.6` ???????????? `A/B/C/S,./J/K/1..9`?
- ? `4.5` ???????????????????????? `1..9` ?????????

### ????
- ??????? `1..9` ???????????????????????
- ?????????????????? seek ???????????????????
- ??????????? `1..9` ???????????????????

### ????
- ? `HotkeyManager` ??? `seek_to_10_percent` ~ `seek_to_90_percent` ?????????? `1..9`?
- `Display` ????????????? `seek_ratio_` ????? `PlayerCore::pumpEvents()` ???? seek ???
- ? `main` ?? `--numeric-seek-check <media_file>` ???????? README????????????????

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


---

## 问题 53: M2 2.2.4：输出播放性能日志（掉帧/队列/CPU/GPU）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.2.4` 要求输出播放性能日志，用于评估高分辨率、高码率样本的掉帧、队列与资源占用表现。
- 当前主链虽然内部已经累计了解封装、解码、渲染和调度统计，但缺少一个可复用、可对比、可直接验收的统一输出入口。

### 原因分析
- `PlayerCore` 内的诊断计数器和 `Scheduler` 统计分散在内部实现中，外部调用方无法一次性获取完整快照。
- 命令行自检入口尚未覆盖性能观测场景，导致 `1080p60 / 4K / 高码率` 样本只能依赖零散日志，难以形成稳定门禁。
- GPU 利用率跨平台直接采样成本较高，因此更适合先输出当前激活的解码/渲染 backend，作为 GPU 路径标识。

### 解决方案
- 在 `PlayerCore` 中新增 `DiagnosticsSnapshot`，统一导出 demux、decode、render、scheduler 与队列指标。
- 在 `VideoPlayer` 中透传 `getInfo()` / `getDiagnosticsSnapshot()`，避免验收逻辑直接耦合内部实现。
- 在 `main` 中新增 `--performance-log-check <media_file> [sample_ms]`：
  - 采样播放期间的平均 CPU 占用；
  - 输出 renderer / decoder backend；
  - 输出掉帧、队列、解码帧数、渲染帧数等结构化指标。
- 同步补齐任务清单、验收报告、差距评估、版本记录与开发日志。

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


---

## 问题 54: M2 2.2.1 / 2.3.2：1080p60 稳定播放验收

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.2.1` 与 `2.3.2` 需要确认 `1080p60` 样本能够连续稳定播放。
- 当前主链虽然已有性能日志入口，但缺少一个直接面向 `1080p60` 门禁的稳定性验收命令和配套样本入口。

### 原因分析
- 现有 `--performance-log-check` 更偏向观测指标导出，不直接判断时间推进、连续播放窗口与掉帧门禁是否达标。
- 仓库现有样本集中在 `1080p30` 与 `4K60`，缺少明确的 `1080p60` 稳定性样本生成入口。

### 解决方案
- 在 `main` 中新增 `--1080p60-check <media_file> [sample_ms]`，联合 `collectFileProbeReport()` 与 `DiagnosticsSnapshot` 输出稳定性门禁结果。
- 验收逻辑重点检查：
  - 样本是否为 `1920x1080 @ 60fps`；
  - `5s` 连续播放窗口内时间是否稳定推进；
  - `scheduler_late_drops` 与 `demux_dropped_packets` 是否为 `0`。
- 在 `tools/download_test_samples.ps1` 中补充 `1080p60 AAC 2ch` 样本生成，并在 `samples/README.md` 中记录用途。
- 同步补齐任务清单、报告、差距评估、版本文档与开发日志。

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


---

## 问题 55: M2 2.2.2 / 2.3.3：4K 播放与降级验收

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.2.2` 与 `2.3.3` 要求确认 `4K` 样本可以播放，并且在硬解不可用时能够降级到软解继续播放。
- 当前仓库已有性能日志入口和 Windows 后端回退校验，但缺少一个直接面向 `4K` 门禁的聚合验收命令。

### 原因分析
- `--performance-log-check` 可以说明 4K 样本能进入播放链路，但不直接覆盖“软解降级是否成功”。
- `--windows-backend-check` 可验证 hard / soft 两个后端模式，但不携带 `4K` 连续播放窗口和时间推进门禁。

### 解决方案
- 在 `main` 中新增 `--4k-playback-check <media_file> [sample_ms]`，主进程验证 `4K` 样本真实推进与 `late_drop`，子进程复用 hard / soft backend session 验证可降级。
- 输出 probe 宽高/FPS、时间推进比率、当前 backend、hard / soft 会话结果等结构化字段。
- 同步补齐任务清单、报告、差距评估、版本记录与开发日志。

### 修改文件
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md


---

## 问题 56: M2 2.2.3：>80Mbps 高码率样本验收

**日期**: 2026-03-08

### 问题描述
- 任务清单 `2.2.3` 要求至少验证一个 `>80Mbps` 样本能够播放。
- 当前仓库虽已完成 `1080p60`、`4K` 与性能日志门禁，但缺少明确的高码率样本和专用验收入口。

### 原因分析
- 现有样本普遍只有 `3~4Mbps` 级别，无法证明播放器在高码率场景下的解封装、解码与渲染链路稳定性。
- 现有验收命令未对“格式码率是否超过 80Mbps”做前置判断。

### 解决方案
- 新增 `--high-bitrate-check <media_file> [sample_ms]`，先读取格式码率，再执行连续播放窗口校验。
- 在 `tools/download_test_samples.ps1` 中新增 `stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 生成入口，保证本地可复现实验样本。
- 同步补齐任务清单、报告、差距评估、版本记录与开发日志。

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


---

## 问题 57: 发布门禁 6.5：长时播放稳定性验收

**日期**: 2026-03-08

### 问题描述
- 任务清单 `6.5` 要求确认播放器在持续播放窗口内无 crash 且能持续推进。

### 原因分析
- 现有 `1080p60`、`4K`、高码率与性能日志门禁覆盖了短窗口稳定性与可观测性，但缺少一个直接面向“长时播放无 crash”的固定 smoke 命令。
- 发布门禁 `6.1 ~ 6.6` 的最后缺口是稳定性证据，缺少单独报告就无法收口 DoD。

### 解决方案
- 在 `main` 中新增 `--long-playback-check <media_file> [sample_ms]`，要求最短采样窗口 `5000ms`，并输出 `open_ok`、是否进入播放循环、窗口结束后是否仍在播放、时间推进比率、`late_drop`、demux 丢包与 backend 信息。
- 新增 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`，记录 `./juren-30s.mp4` 上 `10000ms` 连续播放 smoke 结果，并同步任务清单、差距评估、版本记录与开发日志。

### 修改文件
- src/main.cpp
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- docs/README.md
- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 58: 7.1 插件系统（动态加载与生命周期闭环）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `7.1` 需要把现有仅支持内存注册/启停状态的插件骨架，补成可实际加载和验收的插件系统。

### 原因分析
- `PluginManager` 之前只维护元数据列表，没有 `DLL` 动态加载、版本兼容校验、生命周期回调和卸载清理能力。
- `FilterRegistry` 缺少注销接口，导致即使插件能注册滤镜，也无法在卸载时安全回收扩展点。

### 解决方案
- 新增 `include/plugin/plugin_api.h`，定义插件宿主接口、`API` 版本常量和导出符号约定。
- 重写 `PluginManager`：支持按文件/目录加载插件、校验 `API` 版本、调用 `initialize/shutdown`、捕获插件异常，并在卸载时注销插件注册的滤镜工厂。
- 新增 `sample_logger_plugin` 示例 `DLL` 与 `--plugin-check [plugin_dir_or_file]` 命令，验证 `sample_identity` 视频滤镜的注册与卸载清理闭环。

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

---

## 问题 59: 7.2 流媒体（真实 HTTP 分片与缓冲）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `7.2` 要求把流媒体能力从“解析器骨架”推进到真实 HTTP 分片下载与缓冲闭环。

### 原因分析
- `HttpStreamDownloader` 之前只保存 URL，不做任何真实网络读取，也没有内部缓冲与 EOF/错误状态。
- 现有 HLS/DASH 解析器只能处理文本，缺少一套可重复执行的本地 HTTP 夹具来验证分片下载链路。

### 解决方案
- 重写 `HttpStreamDownloader`，基于 FFmpeg `avio` 支持真实 HTTP 打开、分块读取、内部缓冲、EOF 状态与错误透传。
- 在 `main` 中新增 `--streaming-buffer-check`，下载 HLS 媒体清单、解析并抓取前 N 个分片，验证缓冲字节数与下载结果。
- 新增 `samples/streaming/hls_local/*` 本地夹具与 `tools/start_streaming_fixture_server.ps1`，通过本机 HTTP 服务复现实验。

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

---


---

## 问题 60: 7.3 HLS/DASH 自适应码率

**日期**: 2026-03-08

### 问题描述
- 任务清单 `7.3` 要求把流媒体能力从“固定清单 smoke”推进到 HLS/DASH 多码率解析、档位选择与可重复回归。

### 原因分析
- `HlsManifestParser` 只能读取媒体播放列表，无法识别 `master playlist` 的 variant 信息。
- `DashManifestParser` 之前只提取 `Representation` 带宽，缺少 `BaseURL`、初始化分片与媒体分片明细。
- 主程序缺少统一的 ABR 选择逻辑和本地验收入口，无法验证升码率/降码率切换路径。

### 解决方案
- 扩展 HLS/DASH 解析器，补齐多码率清单、表示集与分片明细。
- 新增 `AdaptiveBitrateSelector`，按估算带宽选择最匹配的档位，并在 `main` 中增加 `--adaptive-bitrate-check`。
- 新增 `samples/streaming/abr_local/{hls,dash}` 夹具，复用本地 HTTP 服务验证 HLS/DASH 的升降档与分片下载。

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
---

## 问题 61: 建立里程碑标签（v0.2.0-rc1 / v0.2.0）

**日期**: 2026-03-08

### 问题描述
- 任务清单 `0.4` 要求为当前阶段建立 `v0.2.0-rc1` 与 `v0.2.0` 里程碑标签，但仓库中此前没有任何 Git 标签。

### 原因分析
- 发布门禁和阶段性能力已经收口，但版本里程碑缺少可追溯的 Git 标记。
- 差距评估与任务清单仍保留“只差标签操作”的旧口径，需要与实际仓库状态同步。

### 解决方案
- 在主线稳定快照上建立 `v0.2.0-rc1` 与 `v0.2.0` 两个里程碑标签。
- 同步更新 `VERSION / DEVELOP_LOG / MPC_HC_GAP_ANALYSIS / tasklist`，记录标签已建立。
- 基于 `v0.2.0-rc1` 已成功建立这一事实，同步勾选执行约束 `5.3 每个里程碑结束前必须可打 RC 标签`。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md
- docs/analysis/MPC_HC_GAP_ANALYSIS.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
---

## 问题 62: 执行守则口径同步（5.1 / 5.2）

**日期**: 2026-03-08

### 问题描述
- 任务清单中的执行守则 `5.1 / 5.2` 仍未更新，但当前仓库状态已经足以判断其中一部分约束是否满足。

### 原因分析
- `5.1` 关注的是本轮并行工作量控制，能从实际任务推进顺序中得到证据。
- `5.2` 关注的是按周节奏执行“只做收敛”，需要跨周、重复性的过程证据，不能仅凭一次交付收口直接勾选。

### 解决方案
- 勾选 `5.1 WIP 限制：同时进行任务不超过 2 个`。
- 保留 `5.2 每周五只做收敛（修复、回归、文档）` 为待完成，并在文档中明确原因。

### 修改文件
- docs/records/VERSION.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md
- docs/README.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

---

## 问题 63: 落地 5.2 周五收敛日执行手册

**日期**: 2026-03-08

### 问题描述
- 任务清单中的 `5.2 每周五只做收敛（修复、回归、文档）` 仍停留在原则口径，缺少可执行的周节奏与收敛日约束。

### 原因分析
- 现有文档已经覆盖回归命令和阶段计划，但还没有把“周五允许什么、禁止什么、结束时要产出什么”写成固定流程。
- 如果没有这层流程约束，即使当前口径正确，后续也很难稳定积累跨周执行证据。

### 解决方案
- 新增 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`，把 `5.2` 固化为可直接执行的周节奏说明与周五收敛手册。
- 更新 `docs/README.md` 与记录文档，明确 `5.2` 现阶段完成的是“流程落地”，而不是“任务勾选完成”。

### 修改文件
- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md


---

## 问题 64: 补齐 5.2 留痕模板（daily_board / 周报）

**日期**: 2026-03-08

### 问题描述
- `5.2` 的执行手册已经落地，但 `daily_board` 和周报层面仍缺少固定模板，导致后续跨周证据不易统一留存。

### 原因分析
- 只有流程说明，没有低成本、固定格式的填写模板，执行时容易口径漂移。
- `5.2` 是否勾选取决于跨周证据，因此模板本身也是守则落地的一部分。

### 解决方案
- 给 `.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 的两个周五补上收敛日记录卡。
- 新增 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md` 作为每周收敛/周报模板。
- 同步更新 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md` 和 `docs/README.md` 的入口说明。

### 修改文件
- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md
- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md
- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md
- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md


---

## 问题 65: 汇总当前功能、使用方式与验证入口

**日期**: 2026-03-08

### 问题描述
- 需要把程序当前已经实现的功能、可用的使用方式，以及现有验证路径集中写成一份可查文档，避免信息散落。

### 原因分析
- 当前能力已经横跨“正常播放、诊断命令、专项验收、插件/流媒体基础设施”，但入口分散在多个文档与源码帮助输出中。
- 如果没有统一总览，后续继续维护文档时容易遗漏“功能”和“验证”之间的对应关系。

### 解决方案
- 新增 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`，统一记录当前功能、使用方式、配置入口、专项验收命令与报告映射。
- 更新 `docs/README.md` 和根 `README.md`，增加该总览文档的入口。

### 修改文件
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/README.md
- README.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md


