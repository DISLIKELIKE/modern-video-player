# 文档索引

`docs/` 目录已按用途重新分类，建议优先从本页进入查找。
每个分类目录下也补充了一个就近的 `README.md`，方便在 IDE 侧边栏按文件夹浏览。

## 目录分类

| 目录 | 适合查看什么 | 代表文档 |
|------|--------------|----------|
| [`guides/README.md`](./guides/README.md) | 环境搭建、功能使用、快速上手 | `WINDOWS_SETUP.md`、`PLAYER_FEATURES_USAGE_VALIDATION.md` |
| [`design/README.md`](./design/README.md) | 当前架构、历史架构、模块设计草案 | `ARCHITECTURE_REFACTOR_2026-03-06.md`、`FILTERS.md` |
| [`analysis/README.md`](./analysis/README.md) | 差距评估、问题分析、文档巡检 | `MPC_HC_GAP_ANALYSIS.md`、`DOC_AUDIT_2026-03-08.md` |
| [`reference/README.md`](./reference/README.md) | 外部能力参考、学习资料、日志库补充说明 | `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`、`OPEN_SOURCE_PLAYER_LEARNING_PATH.md` |
| [`plans/README.md`](./plans/README.md) | 路线图、阶段计划、TODO 清单 | `CROSS_PLATFORM_EVOLUTION_ROADMAP.md`、`PHASE1_CROSS_PLATFORM_TODO.md` |
| [`workflows/README.md`](./workflows/README.md) | 回归手册、工作流、阅读清单、AI 提问剧本 | `REGRESSION_OPERATION_PLAYBOOK.md`、`WORKFLOW.md` |
| [`records/README.md`](./records/README.md) | 版本记录、修复记录、开发日志 | `VERSION.md`、`CHANGELOG.md`、`DEVELOP_LOG.md` |
| [`reports/README.md`](./reports/README.md) | 本地验收与功能验证结果 | `*_LOCAL_CHECK.md` |

## 按场景查找

- **先跑起来**：`guides/WINDOWS_SETUP.md`、`guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- **理解当前主链**：`design/ARCHITECTURE_REFACTOR_2026-03-06.md`、`design/ARCHITECTURE.md`
- **看模块设计**：`design/FILTERS.md`、`design/LOGGING.md`、`design/PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md`
- **看规划与路线**：`plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md`、`plans/MPC_HC_ITERATION_PLAN.md`
- **看差距和分析**：`analysis/MPC_HC_GAP_ANALYSIS.md`、`analysis/video-stream-index-fix.md`
- **跑回归/查流程**：`workflows/REGRESSION_OPERATION_PLAYBOOK.md`、`workflows/FORMAT_REGRESSION.md`、`workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`
- **查历史记录**：`records/CHANGELOG.md`、`records/VERSION.md`、`records/DEVELOP_LOG.md`
- **看验收结果**：`reports/` 目录下各项 `*_LOCAL_CHECK.md`

## 按目录浏览

### `guides/`

| 文档 | 说明 |
|------|------|
| [WINDOWS_SETUP.md](./guides/WINDOWS_SETUP.md) | Windows 环境配置与构建入口 |
| [PLAYER_FEATURES_USAGE_VALIDATION.md](./guides/PLAYER_FEATURES_USAGE_VALIDATION.md) | 当前功能、使用方式与验证入口总览 |
| [IMPLEMENTATION.md](./guides/IMPLEMENTATION.md) | 早期原型的从零实现教程（历史实现基线） |

### `design/`

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE_REFACTOR_2026-03-06.md](./design/ARCHITECTURE_REFACTOR_2026-03-06.md) | 当前主链重构说明 |
| [ARCHITECTURE.md](./design/ARCHITECTURE.md) | 历史架构基线与设计背景 |
| [FILTERS.md](./design/FILTERS.md) | 滤镜接口、内置滤镜与当前接入方式 |
| [LOGGING.md](./design/LOGGING.md) | 日志系统说明 |
| [PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md](./design/PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md) | 播放状态机设计草案 |
| [WINDOWS_PLAYER_DEFAULT_STRATEGY_DRAFT.md](./design/WINDOWS_PLAYER_DEFAULT_STRATEGY_DRAFT.md) | Windows 默认策略建议稿 |

### `analysis/`

| 文档 | 说明 |
|------|------|
| [MPC_HC_GAP_ANALYSIS.md](./analysis/MPC_HC_GAP_ANALYSIS.md) | 与 MPC-HC 的功能差距评估 |
| [DOC_AUDIT_2026-03-08.md](./analysis/DOC_AUDIT_2026-03-08.md) | 本轮文档口径巡检与收敛结果总表 |
| [video-stream-index-fix.md](./analysis/video-stream-index-fix.md) | 早期流索引问题的历史分析归档 |

### `reference/`

| 文档 | 说明 |
|------|------|
| [PLAYER_REFERENCE_AND_FFMPEG_NOTES.md](./reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md) | 播放能力实现参考（格式 / 高分高帧 / 多音道） |
| [OPEN_SOURCE_PLAYER_LEARNING_PATH.md](./reference/OPEN_SOURCE_PLAYER_LEARNING_PATH.md) | 开源播放器整合学习路径 |
| [QUILL_LOGGING.md](./reference/QUILL_LOGGING.md) | Quill 日志库集成补充说明 |

### `plans/`

| 文档 | 说明 |
|------|------|
| [CROSS_PLATFORM_EVOLUTION_ROADMAP.md](./plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md) | 从 Windows 演进到跨平台架构的路线图 |
| [CROSS_PLATFORM_REFACTOR_TASKLIST.md](./plans/CROSS_PLATFORM_REFACTOR_TASKLIST.md) | 当前仓库跨平台改造任务清单 |
| [MPC_HC_ITERATION_PLAN.md](./plans/MPC_HC_ITERATION_PLAN.md) | MPC-HC 对齐迭代计划（快照） |
| [PHASE1_CROSS_PLATFORM_TODO.md](./plans/PHASE1_CROSS_PLATFORM_TODO.md) | Phase 1 逐文件 TODO 实施单 |

### `workflows/`

| 文档 | 说明 |
|------|------|
| [WORKFLOW.md](./workflows/WORKFLOW.md) | 工作流程规范与文档同步要求 |
| [REGRESSION_OPERATION_PLAYBOOK.md](./workflows/REGRESSION_OPERATION_PLAYBOOK.md) | 样本准备与回归操作手册 |
| [FORMAT_REGRESSION.md](./workflows/FORMAT_REGRESSION.md) | 格式回归脚本与报告说明 |
| [WEEKLY_CONVERGENCE_PLAYBOOK.md](./workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md) | 周节奏与周五收敛手册 |
| [SOURCE_FILE_READING_CHECKLIST.md](./workflows/SOURCE_FILE_READING_CHECKLIST.md) | 按文件名展开的阅读清单 |
| [MODULE_BASED_ANALYSIS_SCRIPTS.md](./workflows/MODULE_BASED_ANALYSIS_SCRIPTS.md) | 按模块的连续提问剧本 |
| [AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md](./workflows/AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md) | AI 源码分析提问方案 |
| [AI_FUNCTION_LEVEL_PROMPT_CHECKLIST.md](./workflows/AI_FUNCTION_LEVEL_PROMPT_CHECKLIST.md) | AI 逐函数提问模板清单 |

### `records/`

| 文档 | 说明 |
|------|------|
| [VERSION.md](./records/VERSION.md) | 项目版本记录与依赖说明 |
| [CHANGELOG.md](./records/CHANGELOG.md) | 问题修复记录 |
| [DEVELOP_LOG.md](./records/DEVELOP_LOG.md) | 开发日志 |

### `reports/`

| 文档 | 说明 |
|------|------|
| [DELAY_ADJUST_LOCAL_CHECK.md](./reports/DELAY_ADJUST_LOCAL_CHECK.md) | 音轨 / 字幕延迟调节本地验收记录 |
| [NUMERIC_SEEK_LOCAL_CHECK.md](./reports/NUMERIC_SEEK_LOCAL_CHECK.md) | 数字热键 `1..9` 跳转本地验收记录 |
| [PERFORMANCE_LOG_LOCAL_CHECK.md](./reports/PERFORMANCE_LOG_LOCAL_CHECK.md) | 播放性能日志本地验收记录 |
| [1080P60_STABILITY_LOCAL_CHECK.md](./reports/1080P60_STABILITY_LOCAL_CHECK.md) | 1080p60 稳定播放本地验收记录 |
| [4K_PLAYBACK_LOCAL_CHECK.md](./reports/4K_PLAYBACK_LOCAL_CHECK.md) | 4K 播放与降级本地验收记录 |
| [HIGH_BITRATE_LOCAL_CHECK.md](./reports/HIGH_BITRATE_LOCAL_CHECK.md) | >80Mbps 高码率样本本地验收记录 |
| [LONG_PLAYBACK_LOCAL_CHECK.md](./reports/LONG_PLAYBACK_LOCAL_CHECK.md) | 长时播放稳定性本地验收记录 |
| [PLUGIN_SYSTEM_LOCAL_CHECK.md](./reports/PLUGIN_SYSTEM_LOCAL_CHECK.md) | 插件系统本地验收记录 |
| [STREAMING_BUFFER_LOCAL_CHECK.md](./reports/STREAMING_BUFFER_LOCAL_CHECK.md) | 流媒体 HTTP 分片与缓冲本地验收记录 |
| [ADAPTIVE_BITRATE_LOCAL_CHECK.md](./reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md) | HLS / DASH 自适应码率本地验收记录 |

## 关联入口

- 项目总览：[../README.md](../README.md)
- 回归样本目录：[../samples/README.md](../samples/README.md)
- 周报模板：[../.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md](../.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md)
- 周看板：[../.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md](../.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md)
- 格式回归工作流：[../.github/workflows/format-regression.yml](../.github/workflows/format-regression.yml)
- 运行参数示例：[../config/player_settings.ini](../config/player_settings.ini)

## 本次整理

- 2026-03-10：按用途将文档拆分为 `guides`、`design`、`analysis`、`reference`、`plans`、`workflows`、`records`、`reports` 八类。
- 所有仓库内主要文档入口已同步到新路径，便于在 IDE 中按目录快速浏览。
