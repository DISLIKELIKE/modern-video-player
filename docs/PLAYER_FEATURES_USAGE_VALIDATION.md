# 当前功能、使用方式与验证指南

本文档汇总截至 `2026-03-08` 当前主程序**已经落地且可验证**的能力，回答三个问题：

- 现在程序已经有哪些功能；
- 你可以通过哪些方式使用它；
- 目前应该如何验证这些功能。

> 说明：本文档区分“用户可直接使用的功能”和“开发/验收能力”。像插件、流媒体缓冲、HLS/DASH ABR 这类能力虽然已经实现并有本地验收，但**不等于**已经成为完整的终端用户播放入口。

## 1. 当前功能总览

### 1.1 用户可直接使用的播放功能

| 类别 | 当前能力 | 说明 |
|------|----------|------|
| 本地播放 | 本地音视频文件播放 | 具备音视频解码、画面显示、音频播放与 A/V 同步 |
| 播放列表 | 多文件顺序播放、本地 `m3u8` 播放列表 | 支持上一项/下一项、EOF 自动下一项、恢复上次索引 |
| 字幕 | 外挂 `SRT` 加载、自动探测同名字幕、字幕开关 | 支持播放/暂停/seek 后同步与字幕时延调节 |
| 基础控制 | 播放/暂停、退出、全屏、拖动进度条 seek | 适合本地文件的日常播放 |
| 音量与静音 | 音量增减、鼠标拖动音量条、静音切换 | 当前基于 `SDL` 音频输出 |
| 播放速度 | 变速降速 / 升速 / 恢复 `1.0x` | 设置可持久化 |
| 精细跳转 | `5s / 30s` 快进快退、`1..9` 百分比跳转 | `1..9` 对应 `10%..90%` |
| 章节导航 | 上一章 / 下一章 | 依赖媒体章节信息 |
| A-B Repeat | 设置 A 点 / B 点 / 清除 A-B 循环 | 适合短片段反复观看 |
| 帧步进 | 暂停态单帧后退 / 前进 | 默认按键 `,` / `.` |
| 截图 | 保存当前画面截图 | 输出到 `screenshots/` 目录，格式为 `.ppm` |
| 设置持久化 | 启动加载、退出保存、失败回退默认值 | 当前覆盖音量、速度、音/字幕时延、硬解偏好、播放列表恢复、热键 |
| 热键系统 | 默认热键、持久化、自定义、冲突检测、恢复默认 | 自定义入口为 `config/player_settings.ini` |
| 格式探测 | 媒体探测与运行时能力查看 | 支持 `--capabilities`、`--probe-file`、`--evaluate-target` |

### 1.2 平台与后端能力

| 类别 | 当前能力 | 说明 |
|------|----------|------|
| 视频解码 | 软件解码 + `D3D11VA` 硬解 | 可通过配置偏好硬解，失败可回退软解 |
| 视频渲染 | `SDL` 可用，`D3D11` 最小链路可用 | `D3D11` 失败可回退 `SDL` |
| 格式覆盖 | 主力容器 / 视频编码 / 音频编码矩阵已补齐 | 结果可通过 `--capabilities` 和回归报告追溯 |
| 性能目标 | `1080p60`、`4K`、`>80Mbps` 样本已有本地验收 | 具备性能日志输出与降级验证 |

### 1.3 已实现但偏开发/验收向的能力

| 类别 | 当前能力 | 当前定位 |
|------|----------|----------|
| 插件系统 | `DLL` 动态加载、API 版本校验、初始化/卸载、示例插件 | 已可验证，暂无用户级插件管理 UI |
| 流媒体基础 | 真实 HTTP 下载、HLS/DASH 清单解析、分片缓冲 | 已可本地验收，尚未形成完整播放链路 |
| 自适应码率 | HLS/DASH 多码率解析、ABR 选择、升降档验证 | 已可本地验收，属于基础设施能力 |
| 滤镜/增强基础 | 内置视频/音频滤镜与注册表 | 管线已在代码中存在，但缺少稳定用户入口 |

## 2. 你现在可以怎么使用这个程序

### 2.1 普通播放模式

#### 方式 1：播放单个本地媒体文件

```powershell
.\build\Debug\modern-video-player.exe .\juren-30s.mp4
```

#### 方式 2：一次传入多个媒体文件，作为临时播放列表

```powershell
.\build\Debug\modern-video-player.exe .\video1.mp4 .\video2.mkv .\video3.webm
```

#### 方式 3：加载本地 `m3u8` 播放列表文件

```powershell
.\build\Debug\modern-video-player.exe .\playlist.m3u8
```

说明：这里的 `.m3u8` 指**本地播放列表文件**，不是完整 HLS 在线播放入口。

#### 方式 4：显式指定外挂字幕

```powershell
.\build\Debug\modern-video-player.exe .\movie.mkv --subtitle .\movie.srt
```

说明：如果不显式指定字幕，程序还会尝试自动加载同名 `.srt` 文件。

### 2.2 运行时能力 / 诊断模式

#### 方式 5：查看当前运行时能力矩阵

```powershell
.\build\Debug\modern-video-player.exe --capabilities
```

#### 方式 6：探测单个媒体文件

```powershell
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

#### 方式 7：评估某个目标播放场景是否适合当前机器

```powershell
.\build\Debug\modern-video-player.exe --evaluate-target 3840 2160 60 6 80
```

### 2.3 配置与自定义方式

配置文件：`config/player_settings.ini`

当前已落地的主要配置项：

```ini
player.volume_percent=100
player.playback_speed=1.0
player.audio_delay_ms=0
player.subtitle_delay_ms=0
decoder.prefer_hardware_decode=true
player.resume_last_playlist=true
player.last_playlist_index=0
hotkey.restore_defaults=false
hotkey.play_pause=SPACE
hotkey.seek_backward=LEFT
hotkey.seek_forward=RIGHT
...
```

说明：

- `player.*` 用于持久化运行参数；
- `decoder.prefer_hardware_decode` 控制是否优先尝试硬解；
- `hotkey.*` 可覆盖默认键位；
- 如需恢复默认热键，可将 `hotkey.restore_defaults=true` 后重启程序。

### 2.4 交互方式（默认热键 / 鼠标）

| 方式 | 当前作用 |
|------|----------|
| `Space` | 播放 / 暂停 |
| `Enter` / `F` / `Alt+Enter` | 切换全屏 |
| `Esc` | 退出全屏；窗口态下退出播放器 |
| `Q` | 退出播放器 |
| `Left / Right` | `-5s / +5s` |
| `Ctrl+Left / Ctrl+Right` | `-30s / +30s` |
| `Up / Down / +/-` | 音量增减 |
| `M` | 静音 |
| `[` / `]` / `R` | 降速 / 升速 / 恢复 `1.0x` |
| `PageUp / PageDown` | 上一项 / 下一项 |
| `Home / End` | 上一章 / 下一章 |
| `A / B / C` | 设置 A 点 / B 点 / 清除 A-B 重复 |
| `S` | 截图 |
| `,` / `.` | 暂停态单帧后退 / 前进 |
| `J / K` | 字幕时延 `-100ms / +100ms` |
| `Ctrl+J / Ctrl+K` | 音频时延 `-100ms / +100ms` |
| `1..9` | 跳转到 `10%..90%` |
| `V` | 字幕开关 |
| 鼠标拖动进度条 | seek |
| 鼠标拖动音量条 | 调整音量 |

### 2.5 作为开发/验收工具使用

当前程序除了“正常播放”，还可以直接作为以下工具使用：

- 本地能力探测工具：`--capabilities`、`--probe-file`、`--evaluate-target`
- 交互功能验收工具：`--chapter-nav-check`、`--ab-repeat-check`、`--frame-step-check`、`--delay-adjust-check`、`--numeric-seek-check`、`--screenshot-check`
- 播放稳定性 / 性能验收工具：`--performance-log-check`、`--1080p60-check`、`--4k-playback-check`、`--high-bitrate-check`、`--long-playback-check`
- 后端 / 平台验收工具：`--renderer-fallback-check`、`--windows-backend-check`
- 扩展与流媒体基础设施验收工具：`--plugin-check`、`--streaming-buffer-check`、`--adaptive-bitrate-check`

## 3. 目前怎么验证这个项目现有的功能

### 3.1 最短验证路径（推荐）

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.sln `
  /t:modern-video-player `
  /p:Configuration=Debug `
  /p:Platform=x64

powershell -ExecutionPolicy Bypass -File .\tools\download_test_samples.ps1
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1
.\build\Debug\modern-video-player.exe --capabilities
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

## 3.2 功能分项验证清单

下表列出“当前主要功能 -> 推荐验证命令 -> 对应报告”的映射。具体样本路径、端口和输出摘要，优先以对应报告文件为准。

| 功能 | 推荐验证命令 | 对应报告 |
|------|--------------|----------|
| 外挂字幕加载与同步 | `--subtitle-sync-check <subtitle.srt>` | `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md` |
| 播放列表连续播放 | `--playlist-flow-check <media1> <media2> ...` | `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md` |
| 设置持久化 | `--settings-persistence-check [settings_file]` | `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md` |
| 渲染失败回退 | `--renderer-fallback-check <media_file>` | `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md` |
| Windows 后端与硬解回退 | `--windows-backend-check <media_file>` | `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md` |
| 章节导航 | `--chapter-nav-check <media_file>` | `docs/reports/CHAPTER_NAV_LOCAL_CHECK.md` |
| A-B Repeat | `--ab-repeat-check <media_file>` | `docs/reports/AB_REPEAT_LOCAL_CHECK.md` |
| 帧步进 | `--frame-step-check <media_file>` | `docs/reports/FRAME_STEP_LOCAL_CHECK.md` |
| 音频 / 字幕时延调节 | `--delay-adjust-check <media_file> <subtitle.srt>` | `docs/reports/DELAY_ADJUST_LOCAL_CHECK.md` |
| `1..9` 百分比跳转 | `--numeric-seek-check <media_file>` | `docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md` |
| 截图 | `--screenshot-check <media_file>` | `docs/reports/SCREENSHOT_LOCAL_CHECK.md` |
| 播放性能日志 | `--performance-log-check <media_file> [sample_ms]` | `docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md` |
| `1080p60` 稳定性 | `--1080p60-check <media_file> [sample_ms]` | `docs/reports/1080P60_STABILITY_LOCAL_CHECK.md` |
| `4K` 播放与降级 | `--4k-playback-check <media_file> [sample_ms]` | `docs/reports/4K_PLAYBACK_LOCAL_CHECK.md` |
| `>80Mbps` 高码率 | `--high-bitrate-check <media_file> [sample_ms]` | `docs/reports/HIGH_BITRATE_LOCAL_CHECK.md` |
| 长时播放稳定性 | `--long-playback-check <media_file> [sample_ms]` | `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md` |
| 插件系统 | `--plugin-check [plugin_dir_or_file]` | `docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md` |
| 流媒体 HTTP 分片与缓冲 | `--streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]` | `docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md` |
| HLS/DASH 自适应码率 | `--adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]` | `docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md` |
| 主力格式矩阵 | `tools/format_regression/run_format_regression.ps1` | `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md` |

## 3.3 输出与痕迹位置

- 截图输出：`screenshots/`
- 持久化配置：`config/player_settings.ini`
- 本地验收报告：`docs/reports/`
- 格式回归脚本与操作手册：`docs/FORMAT_REGRESSION.md`、`docs/REGRESSION_OPERATION_PLAYBOOK.md`
- 每周收敛留痕：`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`、`.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`

## 4. 当前边界与未完成功能

以下能力**不要误认为已经是完整终端用户功能**：

- 流媒体：当前已经具备真实 HTTP 下载、HLS/DASH 清单解析、分片缓冲与 ABR 验证，但**真正播放链路接入仍待补齐**。
- 插件系统：当前已经具备 `DLL` 动态加载和生命周期管理，但还没有用户级配置界面、插件分发与隔离策略。
- 皮肤系统：当前仍是骨架 / 未接入状态。
- 字幕系统：当前可用的是外挂 `SRT`；内嵌字幕、字幕轨切换、`ASS/SSA` 等能力尚未补齐。
- 渲染后端：`SDL` 与 `D3D11` 已有可用路径，`OpenGL` 仍不是当前主路径。
- 滤镜增强：内置视频/音频增强基础设施已存在，但暂未形成稳定、可见、可调的用户功能入口。

## 5. 推荐阅读

- 入口索引：`docs/README.md`
- 回归手册：`docs/REGRESSION_OPERATION_PLAYBOOK.md`
- 格式回归说明：`docs/FORMAT_REGRESSION.md`
- 当前差距评估：`docs/MPC_HC_GAP_ANALYSIS.md`
- 根文档概览：`README.md`
