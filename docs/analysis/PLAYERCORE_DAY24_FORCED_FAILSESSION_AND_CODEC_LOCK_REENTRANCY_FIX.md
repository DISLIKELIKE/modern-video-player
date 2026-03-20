# Day24 实施记录：FailSession 强制覆盖探针与 codec 锁重入崩溃修复

日期：2026-03-20  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 补一条可重复触发的 `FailSession` 强制路径探针，覆盖“真实异常不易稳定复现”的空白。
2. 通过探针验证 `FailSession` 下状态迁移与非法迁移计数 gate。
3. 收敛探针触发过程中暴露的线程/锁异常，并复测 serial 回归主链。

## 本轮改动

### 1) 新增强制 FailSession 注入开关（仅测试用途）

- `PlayerCore::decodeVideoFrame()` 新增环境变量开关：
  - `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE=1`
- 在视频解码边界（首个有效 packet）触发一次 `FailureRecoveryPolicy::FailSession`，用于稳定命中会话级失败恢复路径。

### 2) 新增 CLI 专项检查命令

- `main.cpp` 新增：
  - `--forced-failsession-check <media_file> [sample_ms]`
  - 对应实现：`runForcedFailSessionCheck(...)`
- 输出统一 `key=value`，并包含：
  - `runtime_failure_stop_requests`
  - `runtime_failure_fail_sessions`
  - `illegal_session/run/pipeline_transitions`
  - `illegal_transition_total`
  - `result=PASS/FAIL`

### 3) 修复探针暴露的残余风险

- 首次强制触发时暴露：`FailSession` 从解码线程进入后，在资源释放路径出现 `device or resource busy` 异常（codec 锁重入）。
- 修复方式：
  - 将 `video_codec_mutex_`、`audio_codec_mutex_` 从 `std::mutex` 调整为 `std::recursive_mutex`；
  - 同步修正 `decodeVideoFrame/decodeAudioFrame` 的 `lock_guard` 类型。
- 结果：强制 `FailSession` 路径可稳定完成到 `session=Failed + playback=Stopped`，非法迁移计数维持 `0`。

## 本地验证

### 构建

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
结果：0 warnings / 0 errors
```

### 强制 FailSession 探针

```text
build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200
forced-failsession-check.runtime_failure_stop_requests=1
forced-failsession-check.runtime_failure_fail_sessions=1
forced-failsession-check.illegal_transition_total=0
forced-failsession-check.result=PASS
```

### serial 聚合回归复测

```text
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.result=PASS
```

## 观测与残余风险

- 本轮已补上 `FailSession` 强制覆盖，且修复了该路径上的 codec 锁重入崩溃风险。
- 当前仍可继续增强：把 `--forced-failsession-check` 纳入统一批量回归脚本入口，减少手动选择命令的误差。
