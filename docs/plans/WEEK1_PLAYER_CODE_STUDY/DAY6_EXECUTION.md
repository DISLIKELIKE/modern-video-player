# Day 6 执行版（测试回归与诊断体系专项）

日期：2026-03-15  
目标：掌握项目的本地回归方式、日志定位路径、报告沉淀方式，形成可复用检查单。

---

## 1. 今日执行安排（8 小时）

1. `09:00-09:30` 读 `docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md` 与 `docs/reports/README.md`。
2. `09:30-10:20` 读 `tools/run_all_checks.ps1`，确认可执行回归命令与前置条件。
3. `10:20-11:10` 按功能分组整理最短回归集（播放、seek、字幕、列表、性能）。
4. `11:10-12:00` 读诊断相关代码与日志点，确认“问题到指标”的映射方式。
5. `13:30-14:30` 运行或模拟回归流程（按环境能力），记录执行结果模板。
6. `14:30-15:20` 生成“故障排查路径图”：现象 -> 日志 -> 线程 -> 模块。
7. `15:20-16:10` 统一整理验收报告写作模板（问题、证据、结论、风险）。
8. `16:10-17:20` 输出 Day6 结论：一页回归总清单 + 一页诊断指北。

---

## 2. 明日必须产出

1. 一份“最短回归命令清单”（可直接执行）。
2. 一份“日志定位速查表”（模块 -> 关键日志 -> 常见问题）。
3. 一份“报告模板”示例（可复制到 `docs/reports`）。

---

## 3. 重点资料与代码定位

- `docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md`
- `docs/reports/README.md`
- `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- `tools/run_all_checks.ps1`
- `src/core/player_core.cpp`（diagnostics 计数与日志汇总）
- `src/core/scheduler.cpp`
- `src/logger.cpp`

---

## 4. Day6 验收标准

- 你能从任一“播放异常现象”快速定位到对应日志入口。
- 你能在 15 分钟内列出一次完整回归所需命令。
- 你能写出一份结构化且可复现的问题报告。
