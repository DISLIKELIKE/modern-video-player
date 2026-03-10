# 周节奏与周五收敛日操作手册

本文档将任务清单 `5.2 每周五只做收敛（修复、回归、文档）` 固化为一套可直接执行的流程约束，用来规范单人迭代时的周节奏、周五边界与里程碑收口动作。

## 1. 目标

- 让“周五只做收敛”从口号变成可核对、可留痕、可复用的执行流程。
- 把周一到周四的功能推进，与周五的回归/修复/文档同步明确拆开。
- 在里程碑周的收敛日结束前，给出“可打 RC 标签 / 暂不可打 RC 标签”的明确结论。

## 2. 周节奏说明

### 周一

- 确定本周主任务，必要时只保留 1 个副任务，保持 `WIP <= 2`。
- 明确本周的验收目标、样本、脚本与文档入口。

### 周二到周四

- 允许推进主线功能开发。
- 每天结束前至少完成一次 smoke 检查，并把新增问题同步到 `docs/records/DEVELOP_LOG.md`。
- 如果出现 blocker，优先降级为“周五收敛日处理”，不要在当日继续扩张范围。

### 周五（收敛日）

- 冻结新功能开发，只做修复、回归、文档、任务状态与发布准备。
- 周五新出现的功能想法，只记录到下周候选，不在当天落地。
- 里程碑周的周五，必须额外给出 RC 准备结论。

## 3. 周五允许事项

- 修复已经复现、已经定级的 bug / blocker。
- 运行 smoke、专项验收、格式回归、流媒体/插件等本地检查。
- 补齐报告、索引、版本记录、开发日志、任务状态。
- 为完成当天验收所必需的最小脚本、样本、说明文档调整。
- 核查里程碑标签前置条件，例如发布门禁、报告入口、工作区状态、标签命名。

## 4. 周五禁止事项

- 开新功能分支或追加未计划的新能力。
- 做与当前 blocker 无关的大范围重构、目录迁移、接口改名。
- 为“顺手优化”引入新的技术风险或新的验收范围。
- 在缺少本地验证证据时直接勾选任务或声称已具备 RC 条件。

## 5. 收敛日进入条件

开始执行周五收敛前，至少确认以下输入已经齐备：

- 当前目标分支明确，且本周主任务范围已经冻结。
- 本周遗留 blocker / bug 列表已整理。
- 当前可执行程序、样本、脚本路径已确认可用。
- 已打开以下入口文档：
  - `docs/README.md`
  - `docs/records/VERSION.md`
  - `docs/records/CHANGELOG.md`
  - `docs/records/DEVELOP_LOG.md`
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
- 本周若涉及流媒体能力，已准备好本地 HTTP 夹具与样本目录。

若上述输入不完整，先补齐输入，不要在周五扩展新功能范围。

## 6. 标准执行顺序

### 步骤 1：冻结当天范围

- 明确当天只允许处理“修复 / 回归 / 文档 / 状态同步 / RC 结论”。
- 把未完成的新功能开发移入下周，不在收敛日继续推进。

### 步骤 2：编译并运行基础回归

推荐先执行以下基础命令：

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.sln `
  /t:modern-video-player `
  /p:Configuration=Debug `
  /p:Platform=x64

powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1
```

作用：

- 确认当前可执行文件是最新构建产物。
- 快速拿到基础健康检查与格式回归结果。

### 步骤 3：执行本周专项验收

根据本周主线选择相关命令；常用示例如下：

```powershell
.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000
.\build\Debug\modern-video-player.exe --plugin-check
.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765
.\build\Debug\modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128
.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766
.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128
.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128
```

作用：

- 验证本周主线能力没有被回归破坏。
- 为周报、版本记录和任务状态提供可追溯证据。

### 步骤 4：只修 blocker，不扩范围

- 如果回归失败，只修复已复现的问题。
- 修复后只重跑受影响的检查，再视情况补跑全量检查。
- 若问题需要大重构，记录为下周任务，不在周五继续扩张。

### 步骤 5：同步文档与任务状态

当天至少同步以下内容：

- 本周新增或更新的本地验收报告。
- `docs/README.md` 索引入口。
- `docs/records/VERSION.md`、`docs/records/CHANGELOG.md`、`docs/records/DEVELOP_LOG.md`。
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`（仅在状态确实变化时更新）。

### 步骤 6：给出 RC 准备结论

如果当前周是里程碑收口周，周五结束前必须明确写出以下二选一结论：

- **可打 RC 标签**：发布门禁、关键报告、任务状态和工作区状态都已满足。
- **暂不可打 RC 标签**：列出 blocker、缺少的报告或未完成门禁，并把它们移入下一轮优先级最高项。

## 7. 最短执行路径

如果当天时间有限，至少按下面顺序完成：

1. 编译当前主线。
2. 跑 `run_all_checks.ps1` 或最关键的 smoke / 专项验收。
3. 修复已复现 blocker。
4. 回写报告与三本记录文档（`VERSION / CHANGELOG / DEVELOP_LOG`）。
5. 更新任务状态，并写明“可打 RC / 暂不可打 RC”的结论。

## 8. 收敛日输出物

一次完整的周五收敛日，至少应产出以下结果：

- 一组可追溯的本地验收输出或报告。
- 已同步的版本记录、变更记录、开发日志与 README 入口。
- 一份明确的任务状态更新（如果状态没有变化，也要在记录中说明原因）。
- 一份周报实例文件，用来沉淀本周是否满足 `5.2` 的过程证据。
- 一句明确的阶段结论：
  - “本周已收敛，可继续进入下一阶段。”
  - 或“仍有 blocker，禁止打 RC，下一步先处理 xxx。”

## 9. `5.2` 的勾选口径

- 新增本文档，表示 `5.2` 已完成“流程定义”，但**不等于**已经完成“按周执行”。
- `5.2` 是否勾选，仍应以真实的周节奏证据为准。
- 建议至少满足以下条件后，再回写任务清单：
  - 至少连续两个周五按本文档执行并留档；
  - 每次都有回归/修复/文档同步记录；
  - 至少有一次里程碑收口周给出明确的 RC 准备结论。

## 10. 关联文档

- 回归执行细节：`docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md`
- 单人迭代计划快照：`docs/plans/MPC_HC_ITERATION_PLAN.md`
- 当前任务清单：`.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`
- 每日节奏样例：`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`
- 周报留痕模板：`.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`
