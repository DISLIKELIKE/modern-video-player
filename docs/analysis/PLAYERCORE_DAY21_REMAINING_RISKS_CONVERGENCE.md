# Day21 结论：收敛剩余风险（Scheduler 策略终版、FailSession 实化、serial/generation 观测强化）

日期：2026-03-20  
范围：`include/core/scheduler.h`、`src/core/scheduler.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 扩展 `SchedulerControlSnapshot`，把 clock policy、audio-master policy 与 ended policy 结构化。
2. 把 `FailSession` 从预留入口推进到真实运行路径，并补齐跨线程 stop 安全边界。
3. 强化 diagnostics，明确 `queue generation` 只负责容器中断，`item-level serial` 仍是旧时间线硬失效主判定。
4. 更新 records 并重跑 Debug 构建。

## 当前缺口

- `SchedulerControlSnapshot` 虽已扩过一轮，但 scheduler 仍有“布尔拼语义”问题。
- `FailSession` 之前几乎没有被关键失败点实际使用。
- serial 与 generation 的职责边界在日志和诊断输出里不够直观。

## 本轮改动

### 1. Scheduler 策略结构化

- 新增：
  - `SchedulerClockPolicy { UseClockSource, AudioMaster, VideoMaster, SystemMonotonic }`
  - `SchedulerAudioMasterPolicy { Disabled, SoftWhenAudioReady, RequireBufferedAudio }`
  - `SchedulerEndedPolicy` 扩展为 `StopOutput / HoldLastFrame / HoldLastFrameNoClockSync`
- `SchedulerControlSnapshot` 新增：
  - `clock_policy`
  - `audio_master_policy`
  - `audio_buffered_seconds`
- `Scheduler` render wait 改为消费策略：
  - `isAudioMasterActive()`
  - `isVideoMasterActive()`
  - `shouldApplyClockSync()`
- `Scheduler::stop()` 增加 self-join 保护，避免在 worker 线程触发 stop 时自连接。

### 2. FailSession 实化

- `handleRuntimeFailure()` 收口增强：
  - `StopPlayback` 分支统一走 stop-request side effects，并统计 `runtime_failure_stop_requests`。
  - `FailSession` 分支统一先 stop-request，再 stop-completion/session release，并统计 `runtime_failure_fail_sessions`。
- 关键失败点升级到 `FailSession`：
  - `prepareVideoOutputFrame` 失败
  - software send probe clone 失败 / timeout
  - audio decode frame 分配失败
  - audio resampler setup / alloc / convert / invalid byte count 失败

### 3. serial 与 generation 可观测性

- 新增 stale serial 丢弃计数：
  - `stale_video_packets_dropped`
  - `stale_audio_packets_dropped`
  - `stale_video_frames_dropped`
  - `stale_audio_frames_dropped`
  - `stale_audio_submit_frames_dropped`
  - `stale_render_frames_dropped`
  - `stale_paused_render_frames_dropped`
- 这些计数已接入：
  - `DiagnosticsSnapshot`
  - `PlayerCore` 低频 diagnostics 日志
  - `--performance-log-check`
  - `--software-video-decode-check`

## 结果

- scheduler 不再只靠 `clock_source + bool` 猜业务语义，而是由策略快照驱动。
- `FailSession` 已有真实失败点覆盖，不再只是占位枚举。
- queue generation 仍负责容器边界中断；item-level serial 通过 stale drop 计数继续作为硬失效主判定。

## 本地验证

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
- 结果：通过，`0 warnings / 0 errors`

## 下一轮建议

1. 继续把 `SchedulerControlSnapshot` 对齐更细的 ended policy 与 clock fallback 策略。
2. 对 `FailSession` 增加场景化回归（连续 seek、暂停态 seek、close/reopen 连续切换）。
3. 把 stale serial 计数接入长期回归阈值，形成自动化 gate。