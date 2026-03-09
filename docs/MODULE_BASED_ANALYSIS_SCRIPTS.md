# 按模块的连续提问剧本

更新日期：2026-03-09

## 1. 文档目标

这份文档不是“单条 prompt 列表”，而是一套 **按模块、按轮次推进的连续提问剧本**。
适合你现在这种学习方式：

- 不是泛泛问“这项目是什么架构”
- 而是围绕一个模块连续深挖
- 先建立模块边界
- 再拆关键函数
- 再看线程/时序/状态
- 最后映射回你自己的项目

这份文档先给你三套最实用的剧本：

1. `ffplay` 的同步剧本
2. `MPC Video Renderer` 的渲染剧本
3. 你当前仓库 `modern-video-player` 的映射剧本

---

## 2. 怎么用这份剧本

### 2.1 不要一口气把整套都发给 AI

正确方式是：

- 一轮只发一个问题块
- 等 AI 回答后，再发下一轮
- 如果 AI 跑偏，就在下一轮明确纠偏

也就是说，这是一份 **连续对话脚本**，不是一条超长提示词。

### 2.2 每一轮都尽量附上这 4 个约束

你可以把下面这段作为通用前缀，反复加在每轮前面：

```text
请基于我提供的源码回答，不要泛泛讲概念。
请区分：
1. 代码中可以直接确认的事实
2. 你的推断

回答时尽量结合：
- 文件路径
- 结构体/类名
- 函数名
- 关键成员变量名

如果当前源码不完整，先明确说明缺失会影响哪些判断。
不要提前展开下一个主题，只回答我这一轮的问题。
```

### 2.3 一套好剧本的顺序

最稳的顺序通常是：

1. **先问模块边界**
2. **再问关键结构/类**
3. **再问关键函数调用链**
4. **再问线程/同步/状态转换**
5. **再问异常路径与 fallback**
6. **最后问对你项目的映射**

如果顺序反过来，一开始就追函数细节，很容易越看越乱。

---

## 3. 通用追问模板

当某一轮 AI 回答得还不错，你可以继续追问：

### 3.1 追“调用链”

```text
继续，不要重复上一轮的概念总结。
这次只追这条调用链：
<函数A> -> <函数B> -> <函数C>

请回答：
1. 每一步输入输出是什么
2. 哪一步改变了状态
3. 哪一步可能阻塞/等待
4. 哪一步最适合我映射到自己的项目
```

### 3.2 追“线程/同步”

```text
继续，这次只看线程和同步，不要再讲模块职责。
请说明：
1. 这条链路分别跑在哪些线程
2. 共享状态有哪些
3. 用了哪些锁、条件变量、serial、flush 语义
4. 最容易出错的竞态点是什么
5. 如果迁移到我的项目，最值得照搬的同步策略是什么
```

### 3.3 追“状态机”

```text
继续，这次只分析状态变化。
请围绕这条链说明：
1. 有哪些状态
2. 状态是在哪些函数里切换的
3. 哪些状态是显式枚举，哪些是隐式条件
4. 暂停、seek、flush、EOF 时状态如何流转
5. 哪些地方应该被我抽象成状态机而不是散落的 if 分支
```

### 3.4 追“工程价值”

```text
继续，这次不要再解释代码细节。
请直接总结：
1. 这套实现最值得学的 5 个工程点
2. 最不值得我当前阶段照抄的 3 个点
3. 如果让我在自己的播放器里先落一个最小版本，应该先抄哪部分
```

---

## 4. 剧本 A：`ffplay` 同步剧本

### 4.1 这套剧本要解决什么

这套剧本不是让 AI 给你讲 FFmpeg 基础，而是专门用来搞懂：

- `ffplay` 的 A/V sync 核心机制
- clock 是怎么组织的
- `video_refresh` 为什么是核心入口
- seek / flush / serial 为什么会直接影响同步
- 哪些思路可以映射到你的项目里

如果你本地文件就是：`C:\Users\PVT01\Downloads\ffplay.c`
可以直接用下面剧本。

如果不是这个路径，把路径替换掉就行。

### 4.2 第 1 轮：先建立同步模块总图

```text
请分析这个文件里的“同步模块”，不是泛泛介绍 ffplay：
C:\Users\PVT01\Downloads\ffplay.c

我这次只想搞懂 A/V sync 的总图。
请回答：
1. 这个文件里与同步最相关的结构体、函数有哪些
2. `VideoState` 在同步系统里扮演什么角色
3. 音频时钟、视频时钟、外部时钟分别由哪些字段和函数维护
4. `video_refresh`、`get_master_clock`、`compute_target_delay` 各自位于同步链的什么位置
5. 请给我一张“同步模块文字结构图”，说明模块之间的依赖关系

回答要求：
- 尽量引用真实函数名、字段名
- 区分代码事实与推断
- 不要展开 seek、flush 细节，先只讲总图
```

### 4.3 第 2 轮：专看 clock 体系

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c。
这次只看 clock 体系，不要重复上一轮的总图。

请重点回答：
1. `Clock` 结构体里最关键的字段是什么，各自作用是什么
2. `get_clock`、`set_clock`、`set_clock_at`、`set_clock_speed`、`sync_clock_to_slave` 分别做什么
3. 为什么 ffplay 需要三套 clock，而不是只保留一个 master time
4. 哪个 clock 在什么时候会被选为 master，决策点在哪
5. 这套 clock 体系的优点与复杂点是什么

最后请补一段：
如果我要在自己的播放器里设计 clock 模块，最值得先借鉴哪 3 个点。
```

### 4.4 第 3 轮：专看 master clock 选择逻辑

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c。
这次只看 master clock 选择逻辑。

请回答：
1. `get_master_sync_type` 和 `get_master_clock` 的职责区别是什么
2. 在有音频、无音频、外部时钟介入时，master clock 是如何切换的
3. 这个选择逻辑和播放器的“谁跟谁同步”是什么关系
4. 哪些场景下 video 跟 audio，哪些场景下 audio/video 跟 external
5. 这套设计如果映射到工程实现，最应该抽象成哪些枚举和策略函数
```

### 4.5 第 4 轮：专看视频同步与 `video_refresh`

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c。
这次只看视频同步链，重点是 `video_refresh`。

请回答：
1. `video_refresh` 为什么是同步核心，而不仅仅是“画图函数”
2. 它如何决定当前帧该不该显示、延迟显示还是丢帧
3. `compute_target_delay`、`vp_duration`、`update_video_pts` 在这条链上的配合关系是什么
4. 视频帧显示时，如何拿当前视频时间去对齐 master clock
5. 这条链路里最关键的状态变量和判断条件有哪些

请尽量用“输入 -> 判断 -> 输出动作”的方式讲清楚。
```

### 4.6 第 5 轮：专看 seek / flush / serial 对同步的影响

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c。
这次不要讲普通播放，只讲 seek / flush / serial 如何影响同步。

请回答：
1. `stream_seek` 在同步系统里触发了什么后果
2. `PacketQueue` / `FrameQueue` 里的 `serial` 为什么关键
3. flush packet 的作用到底是什么
4. 为什么不处理 serial，就容易出现“旧帧/旧包污染新播放位置”
5. 这套机制如何保证 seek 后同步重新建立

最后请补一段：
如果我要在自己的播放器里做 seek 后的同步重建，应该最低限度抄哪些机制。
```

### 4.7 第 6 轮：专看音频回调与同步基准

```text
继续分析 C:\Users\PVT01\Downloads\ffplay.c。
这次只看音频输出和同步基准，不要重复视频逻辑。

请回答：
1. `sdl_audio_callback` / `audio_decode_frame`（或当前版本对应函数）在同步体系中的职责是什么
2. 音频时钟是在哪个时刻被更新的
3. 音频设备缓冲和“用户以为当前播放到哪里了”之间为什么会有偏差
4. ffplay 是如何处理这种偏差的
5. 为什么很多播放器默认更愿意以 audio 作为 master
```

### 4.8 第 7 轮：映射到你的项目

```text
请把 C:\Users\PVT01\Downloads\ffplay.c 里的同步设计，映射到我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

请重点对照这些文件：
- src/core/player_core.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
- src/core/frame_pool.cpp
- src/demuxer.cpp
- src/display.cpp
- src/audio_player.cpp

请输出：
1. 一张职责映射表
2. 我当前项目已经具备了 ffplay 哪些同步基础
3. 我当前项目和 ffplay 在同步体系上最关键的差距
4. 如果我只做一个最小可落地改造，应该优先改哪 3 个点
5. 哪些 ffplay 细节值得借鉴，哪些不适合机械照搬
```

### 4.9 第 8 轮：收束成你的设计任务

```text
继续，基于上一轮的映射结果，不要再解释 ffplay。
请直接输出我项目里“同步模块改造任务单”。

要求：
1. 具体到文件名
2. 具体到建议新增的结构/类/函数
3. 标出先后顺序
4. 标出哪些是必须做，哪些是后续增强
5. 给出一个最小版本的验收标准
```

---

## 5. 剧本 B：`MPC Video Renderer` 渲染剧本

### 5.1 这套剧本要解决什么

这套剧本不是让 AI 给你讲“DirectShow 是什么”，而是专门搞懂：

- `MPCVR` 的渲染管线是怎么组织的
- sample 从输入 pin 进来后，怎么变成屏幕输出
- allocator / presenter / compositor / processor 分别扮演什么角色
- 为什么成熟渲染器会比“一个 renderFrame 函数”复杂很多
- 哪些实现思路适合你当前项目借鉴

如果你本地拷了 `clsid2/MPCVideoRenderer` 源码，建议把实际目录替换进下面的 `<MPCVR源码目录>`。

### 5.2 第 1 轮：先问渲染体系总图

```text
请分析这个渲染器项目的整体渲染体系：
<MPCVR源码目录>

我不是要你泛泛介绍 DirectShow，而是要你告诉我这个项目自己的渲染架构。
请回答：
1. 这个项目的入口类、主 renderer 类、输入 pin、presenter、processor、compositor 分别在哪里
2. sample 从上游 decoder/filter 进入后，到最终 present 到屏幕，主链路是怎样的
3. 哪些类负责资源分配，哪些类负责渲染调度，哪些类负责屏幕呈现
4. 这套架构和普通“播放器内建 renderer”相比，最大的结构差异是什么
5. 请给一张“Decoder -> Input Pin -> Compositor/Processor -> Presenter -> Screen”的文字结构图

回答要求：
- 尽量引用真实类名、文件名、函数名
- 如果源码里没有显式叫这些名字，请按职责归类
- 区分代码事实与推断
```

### 5.3 第 2 轮：专看输入 pin 和 sample 接收链

```text
继续分析 <MPCVR源码目录>。
这次只看输入 pin / sample 接收链。

请回答：
1. 哪个类负责接收上游送来的媒体 sample
2. DirectShow 的样本接收入口函数是哪几个
3. sample 进入 renderer 后，先经过哪些转换、缓存或状态判断
4. 时间戳、格式、分辨率、色彩空间等信息是在这条链上的哪一步被消费的
5. 这里的设计最值得我学习的工程点是什么
```

### 5.4 第 3 轮：专看 allocator / presenter

```text
继续分析 <MPCVR源码目录>。
这次只看 allocator-presenter，不要展开 shader 细节。

请回答：
1. 哪些类或模块负责 surface / texture / swap chain / backbuffer 的所有权
2. allocator 和 presenter 的职责边界是什么
3. 为什么成熟渲染器喜欢把“资源分配”和“最终呈现”单独建模
4. 设备重建、窗口变化、格式变化时，这一层通常怎么处理
5. 如果映射到我的播放器，哪些能力应当落在 renderer 内部，哪些不该混进 PlayerCore
```

### 5.5 第 4 轮：专看 processor / compositor / shader 路径

```text
继续分析 <MPCVR源码目录>。
这次只看 processor / compositor / shader 路径。

请回答：
1. 视频 sample 从原始 surface 到最终可 present 的图像，中间有哪些处理阶段
2. 哪一层负责颜色转换、缩放、色彩空间处理、HDR/SDR 相关处理
3. 哪一层负责字幕/OSD/overlay 合成
4. compositor 和 presenter 的边界在哪里
5. 这套分层的优点是什么，为什么不把所有逻辑塞进一个 renderFrame 里
```

### 5.6 第 5 轮：专看 present 时序与稳定性

```text
继续分析 <MPCVR源码目录>。
这次只看 present、时序、稳定性。

请回答：
1. renderer 是如何决定什么时候 present 的
2. 有没有独立的调度/等待/队列机制
3. 丢帧、重复帧、vsync、设备阻塞这些问题在什么层处理
4. 设备丢失、D3D reset、窗口切换、全屏切换时，链路如何恢复
5. 这套 renderer 在“稳定输出”上最值得学习的点是什么
```

### 5.7 第 6 轮：专看配置项与 fallback

```text
继续分析 <MPCVR源码目录>。
这次只看配置项与 fallback。

请回答：
1. renderer 暴露给播放器或用户的关键选项有哪些
2. 哪些选项本质上在切换渲染路径、surface 路径、present 路径
3. 如果某个高级路径不可用，fallback 一般是怎么设计的
4. 哪些能力是 runtime capability 检测决定的，哪些是 compile-time 决定的
5. 这套设计如何帮助播放器减少“这台机器能播，那台机器黑屏”的问题
```

### 5.8 第 7 轮：映射到你的项目

```text
请把 <MPCVR源码目录> 的渲染体系，映射到我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

请重点对照这些文件：
- include/render/video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- src/render/renderer_factory.cpp
- src/core/player_core.cpp
- src/display.cpp

请输出：
1. 一张职责映射表
2. 我当前项目的 renderer 接口哪些地方过宽或过窄
3. 哪些职责应该学 MPCVR 单独分层
4. 如果我要逐步演进到“更成熟的 renderer 架构”，第一步应该改哪几个文件
5. 哪些 MPCVR 设计适合当前阶段借鉴，哪些太重、暂时不该学
```

### 5.9 第 8 轮：收束成改造任务

```text
继续，基于上一轮映射结果，不要再介绍 MPCVR。
请直接输出我项目的“渲染架构演进任务单”。

要求：
1. 具体到文件名
2. 具体到建议新增的抽象层
3. 标出先后顺序
4. 标出哪些改动是为了跨平台，哪些改动是为了 renderer 可扩展性
5. 给一个最小版本验收标准
```

---

## 6. 剧本 C：你自己项目的映射剧本

项目路径：`D:\VSProject\sssssssssssssss\modern-video-player`

### 6.1 这套剧本要解决什么

这套剧本不是让 AI 重复读代码摘要，而是让它围绕你的仓库做三件事：

1. 建立当前模块边界
2. 找出和成熟播放器相比的关键缺口
3. 收束成可执行的工程任务

### 6.2 第 1 轮：先建立你自己项目的总图

```text
请分析我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

这次先不要提优化建议，也不要先谈跨平台。
我只想先建立整体模块图。

请重点对照这些文件：
- src/video_player.cpp
- src/core/player_core.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
- src/demuxer.cpp
- src/audio_player.cpp
- src/display.cpp
- src/render/renderer_factory.cpp
- src/decoder/decoder_factory.cpp

请回答：
1. 这个项目当前的主播放链是怎样的
2. 哪些模块负责控制流，哪些负责数据流
3. 哪些模块已经有明确边界，哪些模块还混杂了多种职责
4. 哪些模块最像 ffplay 风格，哪些更像播放器工程风格
5. 请给我一张“Source -> Demux -> Decode -> Queue/Clock/Scheduler -> Renderer/Audio -> Screen/Speaker”的文字图
```

### 6.3 第 2 轮：专看同步与调度

```text
继续分析我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

这次只看同步与调度，不要重复整体架构。

请重点对照：
- src/core/player_core.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
- src/core/frame_pool.cpp
- src/audio_player.cpp

请回答：
1. 当前项目里的 clock、scheduler、frame queue 各自负责什么
2. 当前同步基准更偏 audio master、video master，还是 system master
3. 暂停、seek、step frame、截图这些操作是怎样影响调度链的
4. 目前最可能导致时序复杂化的点在哪里
5. 如果参考 ffplay，同步模块最值得补哪几个结构或状态
```

### 6.4 第 3 轮：专看 renderer / display 边界

```text
继续分析我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

这次只看 renderer 和 display 边界。

请重点对照：
- include/render/video_renderer.h
- src/render/sdl_video_renderer.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- src/display.cpp
- src/core/player_core.cpp

请回答：
1. 当前 renderer 接口承担了哪些职责
2. 哪些职责其实应该属于 display / input / ui / command 层
3. `display.cpp` 当前承担了哪些不应长期耦合在一起的职责
4. 如果以后做跨平台，renderer 抽象的主要阻力在哪
5. 请给一张“Decoder -> Compositor -> Presenter -> Screen”映射到我当前实现的职责对照图
```

### 6.5 第 4 轮：专看默认策略与 fallback

```text
继续分析我的项目：
D:\VSProject\sssssssssssssss\modern-video-player

这次只看默认策略与 fallback。

请重点对照：
- src/render/renderer_factory.cpp
- src/decoder/decoder_factory.cpp
- src/core/player_core.cpp
- CMakeLists.txt

请回答：
1. 当前 renderer 默认选择策略是什么
2. 当前 decoder backend 默认选择策略是什么
3. 哪些 fallback 已经有了，哪些 fallback 还只是硬编码的临时实现
4. 当前策略最明显的 Windows 假设是什么
5. 如果要演进成跨平台策略层，最先应该抽出哪些输入、输出和决策对象
```

### 6.6 第 5 轮：让 AI 和 ffplay / MPCVR 对照

```text
请把我的项目和两个参考对象做对照：
1. ffplay 的同步体系
2. MPCVR 的渲染体系

项目路径：
D:\VSProject\sssssssssssssss\modern-video-player

请输出：
1. 我项目里最接近 ffplay 的模块有哪些
2. 我项目里最接近 MPCVR 的模块有哪些
3. 当前最缺的是“同步层能力”还是“renderer 分层能力”，为什么
4. 如果只能选一个方向先补，应该先补哪个
5. 请给我一份先后顺序明确的改造建议
```

### 6.7 第 6 轮：收束成任务清单

```text
继续，基于上一轮对照结果，不要再重复概念总结。
请直接输出：
1. 当前仓库下一阶段最值得做的 10 个任务
2. 每个任务具体影响哪些文件
3. 哪些任务是架构任务，哪些任务是功能任务
4. 哪些任务之间有前置依赖
5. 哪三个任务最适合作为我当前阶段的“成长型改造”
```

---

## 7. 三套剧本的配合顺序

如果你想最大化学习效率，建议按这个顺序跑：

### 第一阶段：先学 `ffplay` 同步

目标：

- 建立你对“播放主链 + 时钟 + sync”的底层感知
- 知道成熟播放链为什么离不开 queue、clock、serial、flush

### 第二阶段：再学 `MPCVR` 渲染

目标：

- 建立你对“成熟 renderer 为什么要分 allocator / presenter / compositor”的理解
- 知道渲染器不只是 `renderFrame()`

### 第三阶段：最后映射到自己项目

目标：

- 不只是“看懂别人代码”
- 而是把理解落成你自己仓库里的结构判断和改造任务

一句话说：

- `ffplay` 教你 **同步主链**
- `MPCVR` 教你 **渲染分层**
- 你自己的项目负责 **把前两者变成工程方案**

---

## 8. 如果 AI 跑偏了，怎么拉回来

### 8.1 AI 开始泛泛讲概念

你直接补一句：

```text
不要泛泛解释概念，请回到源码本身。
请结合真实文件名、类名、函数名、关键字段名回答。
```

### 8.2 AI 开始提前讲下一轮内容

```text
先停在这一轮，不要提前展开下一主题。
这轮只回答我当前问的模块边界/调用链/同步点。
```

### 8.3 AI 只会“翻译代码”，没有工程总结

```text
不要只做代码翻译。
请补充：
1. 这段实现解决了什么工程问题
2. 为什么要这样设计
3. 映射到我的项目时最值得借鉴什么
```

### 8.4 AI 只会下结论，不给依据

```text
请把结论和依据分开写：
- 代码事实
- 你的推断
- 你的工程建议
```

---

## 9. 最后给你的实用建议

如果你真的按这三套剧本连续跑，你得到的就不只是“AI 讲解”，而是三层能力：

1. **读底层播放链的能力**：来自 `ffplay`
2. **读成熟渲染器架构的能力**：来自 `MPCVR`
3. **把理解转成自己项目任务的能力**：来自映射剧本

这三层能力叠起来，才是你从“会看一点源码”走向“能做播放器工程设计”的关键。

如果你愿意，下一条我可以继续给你补一份：

- **“按源码目录直接填空的版本”**

也就是把 `<MPCVR源码目录>`、关键文件、关键类位都预留好，你只要替换本地路径就能直接喂给 AI。
