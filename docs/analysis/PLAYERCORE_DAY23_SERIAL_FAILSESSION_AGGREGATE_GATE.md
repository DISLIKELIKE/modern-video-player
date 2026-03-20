# Day23 实施记录：serial/failsession 一键聚合回归 gate

日期：2026-03-20  
范围：`src/main.cpp`

## implementation planner

1. 在现有 3 个 serial/failsession 探针之上补一个聚合命令，避免人工串行执行漏项。
2. 保持机器可读输出风格，统一输出 `key=value` 与总 `result=PASS/FAIL`。
3. 跑 Debug 构建并执行聚合命令，确认与单探针判定一致。

## 本轮改动

### 1) 新增聚合命令

- `src/main.cpp` 新增：
  - `runSerialFailSessionRegressionCheck(...)`
  - 命令行入口：`--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 聚合命令内部顺序执行：
  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--close-reopen-serial-check`

### 2) 新增聚合结果字段

命令输出补齐以下机器可读字段：

- `serial-failsession-regression-check.seek_burst_ok`
- `serial-failsession-regression-check.paused_seek_ok`
- `serial-failsession-regression-check.close_reopen_ok`
- `serial-failsession-regression-check.pass_count`
- `serial-failsession-regression-check.total_count`
- `serial-failsession-regression-check.result`

## 本地验证

### 构建

```text
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
结果：0 warnings / 0 errors
```

### 聚合命令验证（`juren-30s.mp4`）

```text
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.pass_count=3
serial-failsession-regression-check.total_count=3
serial-failsession-regression-check.result=PASS
```

## 观测与残余风险

- 该命令解决的是“执行层漏跑风险”，不改变 `PlayerCore` 运行时策略本身。
- 当前 `FailSession` 仍以真实运行时错误触发，聚合命令尚未引入故障注入型强制 `FailSession` 覆盖。
