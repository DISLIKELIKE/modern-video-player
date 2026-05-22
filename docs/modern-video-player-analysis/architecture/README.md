# 架构文档索引

| 文档 | 内容 |
|---|---|
| [overview.md](overview.md) | 分层模型、职责矩阵、核心持有关系 |
| [dataflow.md](dataflow.md) | 播放、字幕、流媒体、设置、诊断数据流 |
| [ui-layer.md](ui-layer.md) | SDL 窗口 UI、overlay、输入请求消费 |
| [runtime-flow.md](runtime-flow.md) | 冷启动、打开媒体、播放控制、EOF、关闭流程 |

## 阅读顺序

1. 先读 [overview.md](overview.md)，确认模块边界。
2. 再读 [dataflow.md](dataflow.md)，理解一帧视频和一段音频如何穿过系统。
3. UI 相关问题读 [ui-layer.md](ui-layer.md)。
4. 状态机、seek、EOF、退出相关问题读 [runtime-flow.md](runtime-flow.md)。

