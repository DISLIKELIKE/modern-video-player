# Day25 实施记录：run_all_checks 接入 forced-failsession 一键 gate

日期：2026-03-20  
范围：`tools/run_all_checks.ps1`

## implementation planner

1. 在批量回归脚本中加入 `--forced-failsession-check` 执行步骤，确保 FailSession 路径纳入一键 gate。
2. 设计参数保持向后兼容：默认复用 `ProbeFile`，仅新增可选覆盖参数。
3. 以退出码作为硬 gate：FailSession 检查非零时直接中断，不进入 format regression。

## 本轮改动

### 1) run_all_checks 参数扩展

- 新增参数：
  - `ForcedFailSessionFile`（默认空，空值时复用 `ProbeFile`）
  - `ForcedFailSessionSampleMs`（默认 `2200`）

### 2) 执行流程从 2 步扩为 3 步

- `[1/3]` `--probe-file --json`
- `[2/3]` `--forced-failsession-check`
- `[3/3]` `run_format_regression.ps1`

### 3) gate 规则

- 若 probe 失败：退出并跳过后续步骤。
- 若 forced-failsession 失败：退出并跳过 format regression。
- 仅当前两步成功时，才进入 format regression。

## 本地验证

```text
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 \
  -ExecutablePath "build/Debug/modern-video-player.exe" \
  -ProbeFile "juren-30s.mp4" \
  -ForcedFailSessionSampleMs 2200

结果：
- [1/3] probe exit code = 0
- [2/3] forced FailSession exit code = 0
- [3/3] regression exit code = 0
- 总退出码 = 0
```

## 观测与残余风险

- 一键脚本已把 `FailSession` 稳定覆盖纳入默认路径，降低“只跑普通链路、不跑失败恢复链路”的漏检风险。
- 当前 `run_all_checks.ps1` 仍以串行方式执行，后续可考虑在不影响 gate 可读性的前提下增加报告聚合输出（例如统一 Markdown 摘要）。
