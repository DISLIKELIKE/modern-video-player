# 格式回归脚本使用说明

## 1. 目的

批量执行样本文件回归，自动输出 `PASS / PARTIAL / FAIL` 报告，便于单人迭代时持续跟踪格式覆盖与退化风险。

## 2. 样本清单

默认清单：

- `tools/format_regression/format_samples.csv`

字段：

- `sample_path`：样本路径（相对仓库根目录或绝对路径）
- `expected_container`：期望容器扩展名
- `expected_video_codec`：期望视频编码名
- `expected_audio_codec`：期望音频编码名
- `notes`：备注

## 3. 运行方式（Windows PowerShell）

```powershell
.\tools\format_regression\run_format_regression.ps1
```

可选参数：

```powershell
.\tools\format_regression\run_format_regression.ps1 `
  -ExecutablePath "build/Debug/modern-video-player.exe" `
  -SamplesFile "tools/format_regression/format_samples.csv" `
  -OutputFile "docs/reports/FORMAT_REGRESSION_CUSTOM.md"
```

## 4. 输出结果

- 默认输出到：`docs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`
- 报告包含：
  - 汇总统计（PASS/PARTIAL/FAIL/SKIP）
  - 每个样本的 open 状态、容器/视频/音频状态、兼容性与备注

## 5. 依赖命令

脚本会调用播放器可执行文件的探测入口：

```powershell
.\modern-video-player.exe --probe-file <media_file>
```

## 6. 返回码约定

- `0`：所有样本 `PASS`
- `1`：存在 `PARTIAL`（无 `FAIL`）
- `2`：存在 `FAIL`

## 7. 结果排查建议

- 先单独执行 `--probe-file` 看 `probe.reason` 字段。
- 对照报告中的 `Container/Video/Audio` 三列查看是解复用、视频解码还是音频解码路径不达标。
- 若样本不存在会标记为 `SKIP`，优先确认 `format_samples.csv` 路径。
