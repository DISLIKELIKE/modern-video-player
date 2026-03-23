# 1.0.0-rc1 发布准备结论

- 日期：`2026-03-23`
- 目标版本：`1.0.0-rc1`
- 适用范围：`Windows x64`，当前主力路径为 `D3D11 renderer + D3D11VA decode`
- 发布结论：`可打 RC 标签`，但`不建议直接打正式版 1.0.0`
- Release 页正文：`[V1_0_0_RC1_RELEASE_NOTES.md](./V1_0_0_RC1_RELEASE_NOTES.md)`

## 结论摘要

当前版本已经满足“发布候选版”的基本要求：主播放链可用、`seek` 主链顺滑、常见容器/编码探测覆盖面足够、`D3D11VA + D3D11` 主路径已具备原生直采样、运行时 fallback 与启动期 diagnostics。

本轮结论不是“所有路径都已经完全成熟”，而是“已经足以进入 RC 阶段，用真实用户和真实机器继续收敛最后一批兼容性问题”。

## 本轮新增验证证据

### 1. Release 一键 gate

执行命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 `
  -ExecutablePath "build/Release/modern-video-player.exe" `
  -ProbeFile "juren-30s.mp4" `
  -ForcedFailSessionSampleMs 2200
```

结果摘要：

- `probe exit code = 0`
- `forced-failsession exit code = 0`
- `format regression exit code = 0`
- 最新格式回归报告：`[FORMAT_REGRESSION_20260323_224615.md](./FORMAT_REGRESSION_20260323_224615.md)`
- 该报告汇总结果：`Total=17 / PASS=17 / PARTIAL=0 / FAIL=0 / SKIP=0`

### 2. Release D3D11 诊断 CLI

执行命令：

```powershell
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```

结果摘要：

- `d3d11-diagnostics.probe_succeeded=true`
- `adapter_name=NVIDIA GeForce GTX 1080`
- `decoder_profiles.h264_any=true`
- `decoder_profiles.hevc_any=true`
- `decoder_profiles.vp9_any=true`
- `decoder_profiles.av1_any=false`
- `native_direct.allowed=true`
- `result=PASS`

### 3. Release 主路径性能 smoke

执行命令：

```powershell
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
```

结果摘要：

- `renderer_backend=D3D11`
- `decoder_backend=D3D11VA`
- `video_native_output_frames=62`
- `video_copy_back_frames=0`
- `result=PASS`

### 4. Release 长时播放 smoke

执行命令：

```powershell
.\build\Release\modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000
```

结果摘要：

- `still_playing_after_window=true`
- `advanced_seconds=10.2827`
- `late_drops=0`
- `demux_dropped_packets=0`
- `result=PASS`

### 5. Release seek / serial / failsession 聚合检查

执行命令：

```powershell
.\build\Release\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
```

结果摘要：

- `seek_burst_ok=true`
- `paused_seek_ok=true`
- `close_reopen_ok=true`
- `pass_count=3`
- `total_count=3`
- `result=PASS`

## 既有能力证据入口

以下报告继续作为 RC 说明的历史验收证据：

- [1080P60_STABILITY_LOCAL_CHECK.md](./1080P60_STABILITY_LOCAL_CHECK.md)
- [4K_PLAYBACK_LOCAL_CHECK.md](./4K_PLAYBACK_LOCAL_CHECK.md)
- [HIGH_BITRATE_LOCAL_CHECK.md](./HIGH_BITRATE_LOCAL_CHECK.md)
- [LONG_PLAYBACK_LOCAL_CHECK.md](./LONG_PLAYBACK_LOCAL_CHECK.md)
- [SUBTITLE_SYNC_LOCAL_CHECK.md](./SUBTITLE_SYNC_LOCAL_CHECK.md)
- [PLAYLIST_FLOW_LOCAL_CHECK.md](./PLAYLIST_FLOW_LOCAL_CHECK.md)
- [SETTINGS_PERSISTENCE_LOCAL_CHECK.md](./SETTINGS_PERSISTENCE_LOCAL_CHECK.md)
- [CHAPTER_NAV_LOCAL_CHECK.md](./CHAPTER_NAV_LOCAL_CHECK.md)
- [AB_REPEAT_LOCAL_CHECK.md](./AB_REPEAT_LOCAL_CHECK.md)
- [FRAME_STEP_LOCAL_CHECK.md](./FRAME_STEP_LOCAL_CHECK.md)
- [SCREENSHOT_LOCAL_CHECK.md](./SCREENSHOT_LOCAL_CHECK.md)
- [NUMERIC_SEEK_LOCAL_CHECK.md](./NUMERIC_SEEK_LOCAL_CHECK.md)
- [PERFORMANCE_LOG_LOCAL_CHECK.md](./PERFORMANCE_LOG_LOCAL_CHECK.md)

## RC 发布说明

### 适合对外强调的能力

- 已支持主流本地容器与编码探测，当前格式回归样本 `17/17 PASS`
- `D3D11 renderer + D3D11VA decode` 主力路径已恢复原生直采样
- `seek / flush / serial / failsession` 主链已完成重构，并具备机器可读回归探针
- 已具备字幕、播放列表、章节导航、A-B Repeat、帧步进、截图、设置持久化等基础播放器交互能力
- 已具备 `--d3d11-diagnostics` 这类面向现场排障和驱动兼容性归档的独立 CLI

### 不应过度承诺的点

- 不要把当前 RC 表述成“所有 GPU / driver / software fallback 路径都已经完全成熟”
- 不要把当前 RC 表述成“AV1 硬解普遍可用”；当前能力取决于具体适配器和驱动
- 不要把当前 RC 表述成“已经达到正式版冻结后零风险状态”

## 已知问题

### 1. software video decode 运行态路径仍未完全收口

- records 中的 `问题 79` 仍处于“已定位”状态。
- 当前已知结论是：默认主路径不再依赖 `software-first`，因此它不阻塞当前 `D3D11VA` 主链发布 RC；但它仍然是正式版前必须继续收敛的重要风险项。

### 2. D3D11 driver / adapter 兼容矩阵仍不够广

- 当前机器（`NVIDIA GeForce GTX 1080`）已验证通过。
- 启动期 quirk / blacklist 机制已经落地，但规则表还很小，仍需要更多真实显卡和驱动样本继续积累。

### 3. AV1 硬解能力依赖适配器

- 当前机器 `--d3d11-diagnostics` 明确显示 `av1_any=false`。
- 这不影响当前 RC 的“可播放”结论，但意味着 AV1 在部分机器上仍将回落到非硬解路径，需继续关注软件链稳定性。

## 发布清单

- 已完成：代码冻结到当前工作区快照
- 已完成：Release `run_all_checks.ps1` 通过
- 已完成：Release `--d3d11-diagnostics` 通过
- 已完成：Release `--performance-log-check` 通过
- 已完成：Release `--long-playback-check` 通过
- 已完成：Release `--serial-failsession-regression-check` 通过
- 已完成：发布说明、已知问题、records 文档同步
- 待执行：创建提交并打 `v1.0.0-rc1` 标签
- 待执行：产出对外分发包与最终发布页文案

## 建议的 RC 标签与命名

- Git 标签：`v1.0.0-rc1`
- 对外版本名：`Modern Video Player 1.0.0-rc1`
- 当前口径：`可发布 RC，不建议直接 GA`