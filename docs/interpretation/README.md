# 项目解读与阅读指南

本目录用于存放你对项目的解读、阅读笔记与阶段性结论。建议每次深入一个模块就新增一份解读文档，避免把理解“藏在脑子里”。

**解读列表**
1. `PLAYBACK_MAIN_PATH.md`：播放主链路解读（第一版）

**建议阅读顺序**
1. `README_ZH.md`
2. `docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`
3. `docs/design/PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md`
4. `docs/workflows/SOURCE_FILE_READING_CHECKLIST.md`
5. `docs/workflows/AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md`
6. `docs/workflows/AI_FUNCTION_LEVEL_PROMPT_CHECKLIST.md`
7. 结合 `src/` 与 `include/` 做模块级扫描

**读取大型项目的方法**
1. 先跑通：构建、启动、打开样例文件，确认主流程可执行。
2. 建模块地图：按顶层目录列出模块职责，并标注输入输出。
3. 找入口与主链路：入口函数、初始化流程、播放状态机、主循环。
4. 先看接口再看实现：优先读 `include/`，再看 `src/` 具体实现。
5. 追一条完整链路：例如“打开文件 → 解封装 → 解码 → 渲染 → 同步”。
6. 记录证据：每条结论都配文件路径与函数名，避免记忆漂移。
7. 小步深挖：一次只解决一个问题，读完就写入解读文档。
8. 迭代复盘：每完成一条链路，就更新模块地图与疑问清单。

**跟 AI 沟通大型项目的方式**
1. 明确目标：你要的是“概览”“调用关系”“状态机细节”还是“性能瓶颈”。
2. 限定范围：给出明确文件或目录，让 AI 只基于已给材料回答。
3. 指定输出：要求结构化输出，例如“关键流程 + 关键类型 + 关键疑问”。
4. 要求不确定点：让 AI 列出不确定或需要验证的内容。
5. 分阶段推进：先要 10 行全景，再指定 1-2 个函数深挖。

**可直接复用的提问模板**
```text
[目标]
我想理解播放主链路中“打开文件到首帧渲染”的关键流程。

[范围]
只基于以下文件回答：
- src/core/player_core.cpp
- include/core/player_core.h
- src/media/*

[输出]
1) 10 行以内流程概述
2) 关键状态/事件枚举
3) 关键类与职责
4) 需要我补充的文件清单
5) 不确定点列表
```

**重点需要学习的模块（建议优先级）**
1. `src/core` / `include/core`：播放状态机、生命周期、线程与调度。
2. `src/media` + `src/decoder`：解封装/解码、帧队列、时间戳。
3. `src/render` + `src/audio`：视频渲染、音频输出、音画同步。
4. `src/streaming`：网络读取、缓冲策略、流媒体适配。
5. `src/subtitle`：字幕解析、渲染与同步。
6. `src/filters`：滤镜与处理链路。
7. `src/plugin`：插件架构与扩展点。
8. `src/input` + `src/playlist` + `src/ui`：交互输入、播放列表、界面联动。
9. `src/config`：配置模型、持久化与默认策略。

**解读记录模板**
```md
# 解读主题：
日期：
范围：

## 目标

## 关键流程

## 关键数据结构/状态

## 依赖与边界

## 证据（文件 + 函数）

## 不确定点

## 下一步
```
