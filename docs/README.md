# 文档索引

本项目包含以下文档，请根据需要查阅。

## 入门指南

| 文档 | 说明 |
|------|------|
| [README.md](../README.md) | 项目简介 |
| [WINDOWS_SETUP.md](./WINDOWS_SETUP.md) | Windows 环境配置 |
| [IMPLEMENTATION.md](./IMPLEMENTATION.md) | 早期原型的从零实现教程（历史实现基线） |

## 设计文档

| 文档 | 说明 |
|------|------|
| [ARCHITECTURE.md](./ARCHITECTURE.md) | 历史架构基线与设计背景 |
| [ARCHITECTURE_REFACTOR_2026-03-06.md](./ARCHITECTURE_REFACTOR_2026-03-06.md) | 当前主链重构说明 |
| [FILTERS.md](./FILTERS.md) | 滤镜接口、内置滤镜与当前接入方式 |
| [VERSION.md](./VERSION.md) | 版本记录与依赖说明 |
| [MPC_HC_GAP_ANALYSIS.md](./MPC_HC_GAP_ANALYSIS.md) | 与 MPC-HC 的功能差距评估 |
| [MPC_HC_ITERATION_PLAN.md](./MPC_HC_ITERATION_PLAN.md) | 2026-03-07 的单人迭代计划快照 |
| [PLAYER_REFERENCE_AND_FFMPEG_NOTES.md](./PLAYER_REFERENCE_AND_FFMPEG_NOTES.md) | 播放能力实现参考（格式/高分高帧/多音道） |
| [video-stream-index-fix.md](./video-stream-index-fix.md) | 早期流索引问题的历史分析归档 |
| [DOC_AUDIT_2026-03-08.md](./DOC_AUDIT_2026-03-08.md) | 本轮文档口径巡检与收敛结果总表 |

## 开发文档

| 文档 | 说明 |
|------|------|
| [CHANGELOG.md](./CHANGELOG.md) | 问题修复记录（重要） |
| [LOGGING.md](./LOGGING.md) | 日志系统说明 |
| [FORMAT_REGRESSION.md](./FORMAT_REGRESSION.md) | 格式样本回归脚本与报告说明 |
| [REGRESSION_OPERATION_PLAYBOOK.md](./REGRESSION_OPERATION_PLAYBOOK.md) | 样本准备与回归操作清单（含每步作用） |
| [format-regression.yml](../.github/workflows/format-regression.yml) | GitHub Actions 自动格式回归工作流 |
| [player_settings.ini](../config/player_settings.ini) | 播放器运行参数（音量/速度/音轨字幕时延/恢复索引/快捷键） |
| [reports/DELAY_ADJUST_LOCAL_CHECK.md](./reports/DELAY_ADJUST_LOCAL_CHECK.md) | 音轨/字幕延迟调节本地验收记录 |
| [reports/NUMERIC_SEEK_LOCAL_CHECK.md](./reports/NUMERIC_SEEK_LOCAL_CHECK.md) | 数字热键 `1..9` 跳转本地验收记录 |
| [reports/PERFORMANCE_LOG_LOCAL_CHECK.md](./reports/PERFORMANCE_LOG_LOCAL_CHECK.md) | 播放性能日志本地验收记录 |
| [reports/1080P60_STABILITY_LOCAL_CHECK.md](./reports/1080P60_STABILITY_LOCAL_CHECK.md) | 1080p60 稳定播放本地验收记录 |
| [reports/4K_PLAYBACK_LOCAL_CHECK.md](./reports/4K_PLAYBACK_LOCAL_CHECK.md) | 4K 播放与降级本地验收记录 |
| [reports/HIGH_BITRATE_LOCAL_CHECK.md](./reports/HIGH_BITRATE_LOCAL_CHECK.md) | >80Mbps 高码率样本本地验收记录 |
| [reports/LONG_PLAYBACK_LOCAL_CHECK.md](./reports/LONG_PLAYBACK_LOCAL_CHECK.md) | 长时播放稳定性本地验收记录 |
| [reports/PLUGIN_SYSTEM_LOCAL_CHECK.md](./reports/PLUGIN_SYSTEM_LOCAL_CHECK.md) | 插件系统本地验收记录 |
| [reports/STREAMING_BUFFER_LOCAL_CHECK.md](./reports/STREAMING_BUFFER_LOCAL_CHECK.md) | 流媒体 HTTP 分片与缓冲本地验收记录 |
| [reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md](./reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md) | HLS/DASH 自适应码率本地验收记录 |
| [samples/README.md](../samples/README.md) | 回归样本目录与命名规范 |

## 文档更新历史

### 2026-02-24 更新

- 新增 [CHANGELOG.md](./CHANGELOG.md) - 问题修复记录文档
- 更新 [VERSION.md](./VERSION.md) - 记录所有问题修复
- 更新 [LOGGING.md](./LOGGING.md) - 日志系统说明
- 同步归档早期问题修复：视频流索引、音频流索引、YUV 渲染错误

### 2026-03-08 更新

- 更新 [MPC_HC_GAP_ANALYSIS.md](./MPC_HC_GAP_ANALYSIS.md) - 按当前代码与本地验收报告刷新差距评估
- 更新 [ARCHITECTURE.md](./ARCHITECTURE.md) / [ARCHITECTURE_REFACTOR_2026-03-06.md](./ARCHITECTURE_REFACTOR_2026-03-06.md) 的索引说明，区分历史基线与当前主链
- 清理 README / VERSION / ARCHITECTURE 中的历史路径描述，避免将旧模块误写为现行结构
- 更新 [IMPLEMENTATION.md](./IMPLEMENTATION.md) / [MPC_HC_ITERATION_PLAN.md](./MPC_HC_ITERATION_PLAN.md) 的状态说明，区分历史教程、计划快照与当前实际进度
- 更新 [FILTERS.md](./FILTERS.md) / [PLAYER_REFERENCE_AND_FFMPEG_NOTES.md](./PLAYER_REFERENCE_AND_FFMPEG_NOTES.md) / [WINDOWS_SETUP.md](./WINDOWS_SETUP.md) 的状态说明与当前入口描述
- 更新根 README 的故障排除口径，并为 [video-stream-index-fix.md](./video-stream-index-fix.md) 增加历史归档说明
- 新增 [DOC_AUDIT_2026-03-08.md](./DOC_AUDIT_2026-03-08.md) - 汇总本轮文档巡检范围、结论与保留策略
- 新增 [reports/DELAY_ADJUST_LOCAL_CHECK.md](./reports/DELAY_ADJUST_LOCAL_CHECK.md) - 记录 4.5 音轨/字幕延迟调节本地验收结果
- 新增 [reports/NUMERIC_SEEK_LOCAL_CHECK.md](./reports/NUMERIC_SEEK_LOCAL_CHECK.md) - 记录 4.6 数字热键跳转本地验收结果
- 新增 [reports/PERFORMANCE_LOG_LOCAL_CHECK.md](./reports/PERFORMANCE_LOG_LOCAL_CHECK.md) - 记录 2.2.4 播放性能日志本地验收结果
- 新增 [reports/1080P60_STABILITY_LOCAL_CHECK.md](./reports/1080P60_STABILITY_LOCAL_CHECK.md) - 记录 2.2.1 / 2.3.2 1080p60 稳定播放本地验收结果
- 新增 [reports/4K_PLAYBACK_LOCAL_CHECK.md](./reports/4K_PLAYBACK_LOCAL_CHECK.md) - 记录 2.2.2 / 2.3.3 4K 播放与降级本地验收结果
- 新增 [reports/HIGH_BITRATE_LOCAL_CHECK.md](./reports/HIGH_BITRATE_LOCAL_CHECK.md) - 记录 2.2.3 >80Mbps 高码率样本本地验收结果
- 新增 [reports/LONG_PLAYBACK_LOCAL_CHECK.md](./reports/LONG_PLAYBACK_LOCAL_CHECK.md) - 记录 6.5 长时播放稳定性本地验收结果
- 新增 [reports/PLUGIN_SYSTEM_LOCAL_CHECK.md](./reports/PLUGIN_SYSTEM_LOCAL_CHECK.md) - 记录 7.1 插件系统本地验收结果
- 新增 [reports/STREAMING_BUFFER_LOCAL_CHECK.md](./reports/STREAMING_BUFFER_LOCAL_CHECK.md) - 记录 7.2 流媒体 HTTP 分片与缓冲本地验收结果
- 新增 [reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md](./reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md) - 记录 7.3 HLS/DASH 自适应码率本地验收结果

---

## 快速链接

- **遇到问题？** 查看 [CHANGELOG.md](./CHANGELOG.md)
- **需要编译？** 查看 [WINDOWS_SETUP.md](./WINDOWS_SETUP.md)
- **了解架构？** 查看 [ARCHITECTURE.md](./ARCHITECTURE.md)




