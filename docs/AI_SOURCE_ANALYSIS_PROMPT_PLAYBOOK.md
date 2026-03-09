# AI 源码分析提问方案

更新日期：2026-03-09

## 1. 文档目的

本文档用于帮助你把以下本地源码交给 AI 分析时，能够 **快速让 AI 理解你的意图**，并输出真正有用的结构化结果，而不是泛泛而谈：

- `C:\Users\PVT01\Downloads\ffplay.c`
- `D:\VSProject\MPCVideoRenderer`
- `D:\VSProject\MPC-BE`

本文重点不是介绍这些项目本身，而是给你一套 **提问模板 + 提问顺序 + 目标输出格式**。

---

## 2. 使用原则

让 AI 分析源码时，尽量遵守下面 6 条规则：

1. **先让 AI 确认源码是否完整**
   - 特别是目录型项目，如 `MPC-BE`
   - 如果副本不完整，先让 AI 说清楚“当前能看到什么，缺什么”

2. **先要架构图，再要细节**
   - 不要一上来问“这段代码什么意思”
   - 先问“模块、线程、数据流、状态模型是什么”

3. **始终要求 AI 结合文件路径和函数名回答**
   - 避免 AI 只讲概念
   - 你要它明确指出“在哪个文件、哪个类、哪个函数”

4. **每次只聚焦一个维度**
   - 比如先问播放主链，再问时钟同步，再问 fallback
   - 不要一次把所有问题全堆上去

5. **要求 AI 区分“事实、推断、未知”**
   - 事实：代码中明确可见
   - 推断：根据命名/调用关系做的工程判断
   - 未知：当前副本没看到，不能硬猜

6. **始终让 AI 和你的项目做映射**
   - 最终目的不是读懂别人，而是帮助当前仓库成长

---

## 3. 推荐的提问顺序

建议始终按照下面 6 步走。

### 第一步：让 AI 先做“目录与入口盘点”

目标：确认项目是否完整，并找出入口文件。

### 第二步：让 AI 输出“架构总览”

目标：先得到模块图、线程图、数据流图。

### 第三步：让 AI 输出“推荐阅读顺序”

目标：不要盲读，而是得到文件级阅读路径。

### 第四步：让 AI 聚焦一个专题

例如：

- `ffplay` 的 queue / clock / seek
- `MPCVR` 的 input pin / allocator / processor / present
- `MPC-BE` 的设置项与解码渲染联动

### 第五步：让 AI 和你的项目做映射

目标：回答“我当前仓库里哪些文件对应它的哪些职责”。

### 第六步：让 AI 给出“学习价值总结”

目标：说明这个项目最值得学的是什么，不值得现在学的是什么。

---

## 4. 通用总模板

下面这份模板适合任何源码项目。

```text
我在学习一个播放器/渲染器项目，请你先不要泛泛介绍，也不要先给优化建议。

请按下面顺序分析：
1. 先确认源码是否完整，并给出目录级入口文件
2. 总结项目的架构组成：模块、主状态对象、线程模型、数据流、控制流
3. 说明播放主链/渲染主链是如何跑通的
4. 给出推荐阅读顺序，精确到文件和关键函数/类
5. 总结这个项目最适合学习的地方，以及不适合我当前阶段深挖的地方
6. 最后把它和我当前项目做映射

回答要求：
- 必须结合具体文件路径、类名、函数名
- 区分“代码中明确可见的事实”和“你的推断”
- 如果源码不完整，请先说明当前缺失会影响哪些判断
- 输出尽量结构化，优先表格和分点

我的当前项目路径是：
d:\VSProject\sssssssssssssss\modern-video-player
```

---

## 5. 针对 `ffplay.c` 的提问模板

### 5.1 首轮提问：看整体架构

```text
请分析这个单文件播放器实现：
C:\Users\PVT01\Downloads\ffplay.c

我不是要你逐行解释，而是要你帮我建立整体理解。

请按下面结构回答：
1. 这个文件里最重要的状态结构有哪些，它们分别承担什么职责
2. 播放主链是怎么跑起来的：demux、packet queue、decode、frame queue、audio callback、video refresh
3. 线程模型是什么，哪些地方是生产者-消费者关系
4. 时钟和 A/V sync 的核心逻辑在哪里
5. seek / flush / serial 的作用是什么
6. 给我一个推荐阅读顺序，精确到结构体、函数名
7. 最后告诉我，这份代码最适合我学习的 5 个点

回答要求：
- 用文件内的结构体名和函数名回答
- 不要泛泛讲 FFmpeg 概念
- 如果一个函数是关键入口，请解释它在整条链上的位置
```

### 5.2 第二轮提问：只看同步

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c

这次只聚焦 A/V sync。

请回答：
1. master clock 是怎么选的
2. get_master_clock、compute_target_delay、video_refresh 各自负责什么
3. audio clock、video clock、external clock 的关系是什么
4. 这套同步方案的优点和局限是什么
5. 如果我要把这套思路借鉴到我自己的项目，最该抄哪些结构，不该机械照搬哪些细节

请尽量结合函数名、变量名和调用链说明。
```

### 5.3 第三轮提问：映射到你的项目

```text
请把 C:\Users\PVT01\Downloads\ffplay.c 的主链设计，映射到我的项目：
d:\VSProject\sssssssssssssss\modern-video-player

请重点对照这些文件：
- src/core/player_core.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
- src/core/frame_pool.cpp
- src/demuxer.cpp
- src/display.cpp

请输出：
1. 一张职责映射表
2. 我当前项目和 ffplay 在主链设计上的主要差异
3. 我当前项目最值得借鉴的 5 个点
4. 我当前项目暂时不必照搬的部分
```

---

## 6. 针对 `MPC Video Renderer` 的提问模板

### 6.1 首轮提问：先做架构拆解

```text
请分析这个 Windows 视频渲染器项目：
D:\VSProject\MPCVideoRenderer

我希望你先从架构层面拆解，不要先解释单个函数。

请按下面结构回答：
1. 项目的核心模块有哪些
2. renderer 主体类是什么，输入 pin、allocator、video processor、present 相关类分别是什么
3. 解码后的 sample 是怎么进入 renderer 的
4. D3D11 / DXVA2 / shader / subtitle / OSD / HDR 这些能力分别落在哪些文件/类
5. 给我一个按文件和类组织的阅读顺序
6. 总结这个项目最适合学习的地方

回答要求：
- 必须点出具体文件路径
- 必须区分“代码中明确可见”和“你的推断”
- 请优先从 Source 目录中归纳
```

### 6.2 第二轮提问：只看渲染主链

```text
继续分析 D:\VSProject\MPCVideoRenderer

这次只聚焦渲染主链。

请回答：
1. 从输入 sample 到最终显示，主链经过了哪些关键对象
2. CVideoRendererInputPin、CCustomAllocator、CVideoProcessor、CMpcVideoRenderer 各自负责什么
3. 哪些地方是资源管理关键点，哪些地方是 present 关键点
4. D3D11 和 DXVA2 的处理路径是如何分开的
5. 这套设计里最值得我借鉴到自己 D3D11 渲染器中的点是什么

请尽量画出文字版链路图。
```

### 6.3 第三轮提问：映射到你的项目

```text
请把 D:\VSProject\MPCVideoRenderer 的架构，映射到我的项目：
d:\VSProject\sssssssssssssss\modern-video-player

请重点对照：
- src/render/d3d11_video_renderer.cpp
- src/render/renderer_factory.cpp
- src/display.cpp

请输出：
1. 职责映射表
2. 我当前 renderer 设计上可能还缺哪些中间层
3. 哪些设计适合当前阶段就吸收，哪些会引入过早复杂度
```

---

## 7. 针对 `MPC-BE` 的提问模板

### 7.1 第一步一定先确认源码完整性

我当前环境下能看到 `D:\VSProject\MPC-BE`，但扫描时尚未看到完整源码树，因此这里的第一条提问必须是完整性检查。

```text
请先检查这个项目目录是否完整：
D:\VSProject\MPC-BE

请回答：
1. 当前目录下是否能看到完整源码树
2. 顶层目录和关键入口文件有哪些
3. 如果当前副本不完整，缺失会影响哪些架构判断
4. 在当前可见内容下，你最多能给出什么层级的分析

请先做完整性判断，再决定后续分析深度。
```

### 7.2 当源码完整后，做产品级架构分析

```text
请分析这个播放器项目：
D:\VSProject\MPC-BE

我的目标不是读懂全部历史代码，而是理解它作为成熟 Windows 播放器的架构组织。

请按下面结构回答：
1. 项目的顶层模块组成
2. 播放器壳层、内部滤镜、解码器、renderer、字幕、播放列表、设置项分别在哪些目录
3. 输出设置是如何影响解码器/renderer 选择的
4. graph/播放链重建大概发生在哪些模块
5. 给出推荐阅读顺序，按“先整体、再主链、再设置联动”的顺序排
6. 总结这个项目最适合我学习的地方

请不要试图一次覆盖整个项目，重点关注和视频播放主链强相关的部分。
```

### 7.3 第三轮提问：只看成熟产品能力

```text
继续分析 D:\VSProject\MPC-BE

这次只聚焦“成熟播放器产品化能力”。

请回答：
1. 这个项目如何组织播放控制、快捷键、字幕、播放列表、截图、输出设置
2. 哪些能力是成熟桌面播放器的必备项
3. 哪些能力对我当前项目属于“以后再做”
4. 如果我的项目还在主链建设期，哪些模块最值得优先借鉴
```

---

## 8. 跨项目对比模板

当你想让 AI 做三者对照时，用下面这版最合适。

```text
请对比分析以下三个参考对象：
1. C:\Users\PVT01\Downloads\ffplay.c
2. D:\VSProject\MPCVideoRenderer
3. D:\VSProject\MPC-BE

我的目标不是比较“谁更强”，而是比较“谁最适合学习哪一部分”。

请按下面维度输出对照表：
1. 项目类型（单文件播放器 / 渲染器 / 成品播放器）
2. 最适合学习的主题
3. 核心架构对象
4. 阅读门槛
5. 与我当前项目的相关性
6. 建议的学习顺序

最后请给我一个明确结论：
- 我应该先学谁
- 第二个学谁
- 第三个学谁
- 各自学到什么程度就可以先停
```

---

## 9. 映射到你当前项目的总模板

这是最重要的一版，因为它直接服务当前仓库。

```text
请把以下参考源码的设计，映射到我的项目：

参考对象：
- C:\Users\PVT01\Downloads\ffplay.c
- D:\VSProject\MPCVideoRenderer
- D:\VSProject\MPC-BE

我的项目：
d:\VSProject\sssssssssssssss\modern-video-player

请重点对照这些路径：
- src/video_player.cpp
- src/demuxer.cpp
- src/display.cpp
- src/core/player_core.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
- src/core/frame_pool.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/renderer_factory.cpp
- src/subtitle/*

请输出：
1. 一张职责映射表
2. 我当前项目在主链、同步、渲染、fallback、设置组织上的主要缺口
3. 哪些缺口最值得优先补
4. 哪些点适合借鉴 ffplay
5. 哪些点适合借鉴 MPC Video Renderer
6. 哪些点适合借鉴 MPC-BE
7. 给我一个按优先级排序的学习/改造路线
```

---

## 10. 建议 AI 输出格式

为了让回答真正可用，你可以要求 AI 固定按下面格式输出：

```text
1. 先给结论
2. 再给架构图/表
3. 再给文件级入口
4. 再给关键函数/类
5. 再给与你当前项目的映射
6. 最后给“最值得学 / 可以暂缓”的总结
```

如果你想让 AI 更工程化一点，再追加一句：

```text
请尽量避免空泛概念，优先给出文件路径、类名、函数名和模块边界。
```

---

## 11. 一句话版使用建议

最有效的方式不是问：

- “这个项目讲讲”

而是问：

- “先确认源码完整性”
- “再给我架构图”
- “再给我阅读顺序”
- “最后映射到我的项目”

这样 AI 才能真正理解你的意图：

**你不是想听科普，而是想把这些源码转化为你当前播放器项目的成长路径。**
