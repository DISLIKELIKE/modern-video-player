# Day22 实施记录：serial/failsession 回归检查补齐（连续 seek / 暂停态 seek / close-reopen）

日期：2026-03-20  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 最小增补 `PlayerCore` diagnostics 字段，支持非法迁移机器可读计数。
2. 在 CLI check framework 中新增 3 个回归探针，统一 `key=value` 与 `result=PASS/FAIL`。
3. 把非法迁移计数同步到现有 `performance/software` 检查输出。
4. 跑 Debug 构建并对新命令做样本验证。

## 本轮改动

### 1) PlayerCore 观测字段增强

- `DiagnosticsSnapshot` 新增：
  - `illegal_session_transitions`
  - `illegal_run_transitions`
  - `illegal_pipeline_transitions`
- `transitionSessionState / transitionRunState / transitionPipelinePhase` 在非法迁移分支累计计数。
- `getDiagnosticsSnapshot()`、`resetDiagnostics()`、低频 diagnostics 日志同步接入新字段。

### 2) 新增 3 个 CLI 回归检查

- `--seek-burst-serial-check <media_file> [seek_count]`
  - 连续 seek 场景，关注 serial 连续推进、边界 stale 增量与稳定窗口收敛。
- `--paused-seek-serial-check <media_file> [seek_count]`
  - 暂停态 seek，关注 paused 保持、serial 推进、无旧音频提交。
- `--close-reopen-serial-check <media_file> [sample_ms]`
  - close/reopen，关注 reopen 后 stale/failure 计数重置与 serial 连续性。

每个命令都输出：
- 关键计数与判定项（`*_ok=true/false`）
- `runtime_failure_*`
- `illegal_*`
- 最终 `result=PASS/FAIL`

并在判定里增加：
- `fail_session_transition_ok`：若出现 `FailSession`，要求非法迁移计数不增加。

### 3) 现有检查输出补齐

- `--performance-log-check` 与 `--software-video-decode-check` 新增导出：
  - `illegal_session_transitions`
  - `illegal_run_transitions`
  - `illegal_pipeline_transitions`

## 本地验证

### 构建

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
结果：0 warnings / 0 errors
```

### 新增检查命令样本验证（`juren-30s.mp4`）

```text
build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4
result=PASS

build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4
result=PASS

build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4
result=PASS
```

## 观测与残余风险

- 连续 seek 样本中可观察到 `illegal_pipeline_transitions` 在 `Draining -> Seeking` 场景有增量，但当前未伴随 `FailSession`。
- 本轮 gate 以 `FailSession` 路径非法跳转约束为核心（`fail_session_transition_ok`），后续可考虑把非 FailSession 的 illegal transition 进一步分类治理。
