# 样本与回归操作手册

本文档把“样本准备 + 探测 + 回归”流程整理为一套可直接执行的步骤，并标注每个操作的作用。

## 1. 编译播放器

操作命令：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.sln `
  /t:modern-video-player `
  /p:Configuration=Debug `
  /p:Platform=x64
```

操作作用：

- 生成最新 `modern-video-player.exe`，确保后续探测和回归使用的是当前代码。

## 2. 准备回归样本（下载+生成）

操作命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\download_test_samples.ps1
```

操作作用：

- 下载基础公开视频到 `samples/source/`。
- 自动生成 `format_samples.csv` 需要的样本（mp4/mkv/mov/avi/webm/flv/ts/m2ts，含 h265/vp9/av1 和多音轨样本）。

相关文件：

- 脚本：[download_test_samples.ps1](../tools/download_test_samples.ps1)
- 样本目录规范：[samples/README.md](../samples/README.md)

## 3. 单文件探测（文本模式）

操作命令：

```powershell
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4
```

操作作用：

- 快速查看 `probe.*` 键值输出（容器、编解码器、分辨率、声道、建议项）。
- 适合人工阅读和临时排查。

## 4. 单文件探测（JSON 模式）

操作命令：

```powershell
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

操作作用：

- 输出结构化 JSON，便于脚本/CI 自动解析。
- 用于定位 `recommendation.reason`，快速判断是否建议硬解/D3D11。

## 5. 批量格式回归

操作命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\format_regression\run_format_regression.ps1
```

可选参数：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\format_regression\run_format_regression.ps1 `
  -ExecutablePath "build/Debug/modern-video-player.exe" `
  -SamplesFile "tools/format_regression/format_samples.csv" `
  -OutputFile "docs/reports/FORMAT_REGRESSION_CUSTOM.md"
```

操作作用：

- 按 `format_samples.csv` 批量执行探测并生成回归报告。
- 输出 PASS/PARTIAL/FAIL/SKIP 汇总与逐样本明细。

输出位置：

- 默认：`docs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`

返回码作用：

- `0`: 全部 PASS
- `1`: 存在 PARTIAL（无 FAIL）
- `2`: 存在 FAIL

## 6. 一键检查（推荐日常执行）

操作命令：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1
```

可选参数：

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 `
  -ExecutablePath "build/Debug/modern-video-player.exe" `
  -ProbeFile "juren-30s.mp4" `
  -SamplesFile "tools/format_regression/format_samples.csv" `
  -RegressionOutputFile "docs/reports/FORMAT_REGRESSION_FINAL.md"
```

操作作用：

- 串联执行两步：
- 第一步：单文件 `--probe-file --json`
- 第二步：批量 `run_format_regression.ps1`
- 用一条命令得到“快速健康检查 + 全量回归”结果。

## 7. 更新样本清单

操作位置：

- 文件：[format_samples.csv](../tools/format_regression/format_samples.csv)

操作作用：

- 注册新样本路径及期望容器/编解码器。
- 让新样本自动纳入回归流程。

推荐命名规则：

- `<tag>__<video_codec>_<audio_codec>__<width>x<height>__<fps>fps__<channels>ch[__maN].<ext>`

## 8. 常见排查

现象：样本显示 `SKIP`

操作作用：

- 说明文件不存在或路径不匹配，优先检查 `sample_path` 和本地文件位置。

现象：`probe.overall=FAIL`

操作作用：

- 先单独跑 `--probe-file --json`，查看 `recommendation.reason` 和容器/编解码器字段。

现象：回归脚本返回码 `2`

操作作用：

- 说明至少一个样本 FAIL，查看报告明细中对应样本的 `Notes` 字段定位问题。

## 9. 最短执行路径

只做一次完整验证，建议顺序：

1. 编译项目
2. 执行 `download_test_samples.ps1`
3. 执行 `run_all_checks.ps1`
4. 查看 `docs/reports/` 报告

## 10. GitHub Actions 自动回归

工作流文件：

- `.github/workflows/format-regression.yml`

触发时机：

- 提交 PR
- 推送到 `main/master`
- 手动触发

操作作用：

- 在 `windows-latest` 自动安装依赖并构建播放器；
- 自动生成样本并执行 `run_all_checks.ps1`；
- 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 作为回归产物。
