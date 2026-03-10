# AI 逐函数提问模板清单

更新日期：2026-03-09

## 1. 文档目的

本文档用于帮助你把播放器源码分析进一步细化到 **函数级别**。

前面的文档已经覆盖了：

- 整体学习路径：`docs/reference/OPEN_SOURCE_PLAYER_LEARNING_PATH.md`
- 源码分析提问方案：`docs/workflows/AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md`
- 按文件名展开的阅读清单：`docs/workflows/SOURCE_FILE_READING_CHECKLIST.md`

而本文档解决的是更细的一层：

- **看到一个关键函数时，应该怎么问 AI，AI 才会真正讲到点上**

本文会先给出通用模板，再给出 `ffplay.c` 的逐函数提问清单，最后补一组 `MPC Video Renderer` 的示例函数。

---

## 2. 使用原则

函数级提问时，不要只问：

- “这个函数是什么意思？”

这类问题太宽，AI 往往会给你概念性解释，但很难帮你建立工程理解。

更好的问法应该固定包含下面 6 个维度：

1. **职责**：这个函数在整条链上负责什么
2. **输入/输出**：参数、返回值、依赖的状态字段是什么
3. **时序位置**：它在播放主链里的前后关系是什么
4. **线程/同步**：有没有锁、阻塞、serial、flush、等待条件
5. **异常路径**：失败、EOF、seek、flush 时它怎么处理
6. **借鉴价值**：映射到你当前项目时最值得学什么

---

## 3. 通用逐函数提问模板

先给你一份任何函数都能套的通用模板。

```text
请分析这个函数，但不要泛泛解释概念：

文件：<文件路径>
函数：<函数名>

请按下面结构回答：
1. 这个函数在整条播放/渲染链上的职责是什么
2. 输入参数、返回值、依赖的成员/状态字段分别是什么
3. 正常路径是怎么执行的
4. 异常路径/阻塞路径/退出路径是怎么执行的
5. 它的上游调用者和下游消费者分别是谁
6. 它涉及哪些线程同步点、锁、条件变量、serial、flush 语义
7. 这个函数最值得我学习的设计点是什么
8. 如果映射到我的项目 d:\VSProject\sssssssssssssss\modern-video-player，应该对应哪些模块

回答要求：
- 尽量结合函数内部的关键变量名、结构体字段名来回答
- 区分“代码中明确可见”和“你的推断”
- 如果这个函数只是某条链上的中间站，请明确说出它前后分别接什么
```

---

## 4. `ffplay.c` 逐函数提问模板清单

本地文件：`C:\Users\PVT01\Downloads\ffplay.c`

建议你按“先结构，再队列，再解码，再同步，再 seek”的顺序问。

## 4.1 先问结构体，不要一上来问函数

### `VideoState`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `VideoState` 结构体。

我想知道的不是字段翻译，而是它为什么是 ffplay 的总状态对象。

请回答：
1. `VideoState` 管了哪些子系统
2. 哪些字段最关键，为什么
3. 它是如何把 demux、decode、queue、clock、display 串起来的
4. 哪些字段明显服务于 seek / flush / sync
5. 如果我在自己的播放器里设计总状态对象，最值得借鉴哪些思路
```

### `PacketQueue`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `PacketQueue` 结构体。

请重点回答：
1. 它解决的工程问题是什么
2. 为什么需要 `serial`
3. 它和 `FrameQueue` 的职责边界是什么
4. 它的线程安全设计依赖哪些字段
5. 如果 seek 发生，为什么 `PacketQueue` 很关键
```

### `FrameQueue`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `FrameQueue` 结构体。

请重点回答：
1. 它与 `PacketQueue` 的区别
2. `readable` / `writable` 两种视角为什么重要
3. `keep_last` 的设计意义是什么
4. 它如何服务于视频显示和音频播放
5. 如果我自己的项目已经有 frame pool，这里哪些设计值得吸收
```

### `Clock`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `Clock` 结构体。

请重点回答：
1. 这个抽象解决什么问题
2. 为什么播放器不能只用一个简单时间戳变量
3. 它和 `audio clock / video clock / external clock` 的关系是什么
4. 哪些字段最关键
5. 这套设计最值得借鉴的地方是什么
```

---

## 4.2 `PacketQueue` 相关函数

### `packet_queue_init()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_init()`。

请回答：
1. 这个初始化函数除了“初始化”之外，还建立了哪些后续语义
2. 哪些字段的初始值会影响 queue 的生命周期
3. 为什么 queue 的初始化阶段就要考虑 abort / serial 这些概念
4. 它是整个播放链的哪个起点
```

### `packet_queue_put_private()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_put_private()`。

请重点回答：
1. 它和 `packet_queue_put()` 的边界是什么
2. 为什么会有 private 版本
3. 真正入队时做了哪些关键状态更新
4. 这个函数在多线程生产者-消费者模型中的意义是什么
5. 这里最值得我在自己的队列实现里借鉴的地方是什么
```

### `packet_queue_put()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_put()`。

请重点回答：
1. 它相对 `packet_queue_put_private()` 多承担了什么责任
2. 对上层调用者来说，这个函数提供了什么抽象边界
3. 它是否包含错误处理/拷贝/生命周期管理语义
4. 在调用链里，谁最常调用它
```

### `packet_queue_get()`

```text
请重点分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_get()`。

这是我最关心的 queue 函数之一，请按下面结构回答：
1. 这个函数解决的核心问题是什么
2. `block` 参数如何改变它的行为
3. 它的阻塞路径、唤醒路径、退出路径分别是什么
4. `serial` 是怎么从 queue 传到 decoder 侧的
5. 哪些地方体现了线程同步设计
6. 为什么它是 seek / flush 语义的关键一环
7. 如果映射到我的项目，应该对应什么队列接口
```

### `packet_queue_flush()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_flush()`。

请重点回答：
1. flush 具体清掉了什么
2. 为什么 flush 不只是“把容器清空”这么简单
3. 它和 seek / stream switch / close 有什么关系
4. flush 后为什么还需要 serial 机制配合
```

### `packet_queue_abort()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_abort()`。

请重点回答：
1. `abort` 和 `flush` 的语义差异
2. 为什么要显式支持 abort
3. 它如何帮助线程安全退出
4. 如果没有这层语义，会出现什么问题
```

### `packet_queue_start()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `packet_queue_start()`。

请重点回答：
1. 为什么 queue 不是 init 后立刻就算 fully active
2. start 这个动作为什么值得单独抽出来
3. 它和 serial 的关系是什么
4. 它在哪个时序点被调用最合理
```

---

## 4.3 `FrameQueue` 相关函数

### `frame_queue_init()`

```text
请分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `frame_queue_init()`。

请回答：
1. 它是如何把 `FrameQueue` 和对应 `PacketQueue` 关联起来的
2. `max_size` 和 `keep_last` 对播放行为有什么影响
3. 为什么 frame queue 的初始化需要知道上游 packet queue
4. 这里有哪些设计适合我自己的 frame pool 抽象
```

### `frame_queue_peek_writable()`

```text
请分析 `frame_queue_peek_writable()`。

请重点回答：
1. 为什么 decoder 线程不能直接随手写 frame queue
2. 这个函数的等待条件是什么
3. 它如何体现生产者-消费者模型
4. 它和 `frame_queue_push()` 是什么关系
```

### `frame_queue_peek_readable()`

```text
请分析 `frame_queue_peek_readable()`。

请重点回答：
1. 为什么显示/播放线程需要 readable 语义
2. 它的阻塞条件是什么
3. 它如何保证消费者看到的是有效帧
4. seek / flush 时它如何避免旧帧被误用
```

### `frame_queue_push()`

```text
请分析 `frame_queue_push()`。

请重点回答：
1. 它具体完成了什么状态切换
2. 为什么 push 不是简单 index++
3. 它如何通知下游消费者
4. frame 生命周期在这里发生了什么变化
```

### `frame_queue_next()`

```text
请分析 `frame_queue_next()`。

请重点回答：
1. 它对消费者意味着什么
2. 为什么显示完成后必须显式 next
3. 它和 `keep_last` 的关系是什么
4. 如果我自己的播放器要做 paused frame / step frame，这里最值得学什么
```

---

## 4.4 解码主链函数

### `decoder_decode_frame()`

```text
请重点分析 C:\Users\PVT01\Downloads\ffplay.c 中的 `decoder_decode_frame()`。

这是 ffplay 解码主链的关键函数，请按下面结构回答：
1. 这个函数在解码链上的职责是什么
2. 它如何和 `PacketQueue` 交互
3. 它如何处理 flush / serial / EOF / null packet
4. 它什么时候返回有效帧，什么时候继续循环取包
5. 为什么这个函数值得作为我自己 decoder worker 的参考模板
6. 如果我要在自己的项目里实现统一 decoder loop，最该抄哪些结构
```

### `audio_thread()`

```text
请分析 `audio_thread()`。

请重点回答：
1. 它和 `decoder_decode_frame()` 的关系是什么
2. 音频线程的输出目标是什么
3. 它如何把解码结果送到音频帧队列/播放路径
4. 它和音频 callback 的边界怎么划分
5. 我自己的 `AudioPlayer + PlayerCore` 架构中，哪些点值得参考
```

### `video_thread()`

```text
请分析 `video_thread()`。

请重点回答：
1. 视频线程如何从 decoder loop 产出显示帧
2. 它如何决定送入 `pictq` 的是什么数据
3. `queue_picture()` 在这里扮演什么角色
4. 这条链和最终 `video_refresh()` 的关系是什么
5. 哪些逻辑适合放在线程里，哪些不应该放在线程里
```

### `subtitle_thread()`

```text
请分析 `subtitle_thread()`。

请重点回答：
1. 字幕线程为什么和音视频线程分开
2. 它的输出如何被后续显示逻辑消费
3. 字幕帧队列和视频帧队列的配合关系是什么
4. 这对我自己的字幕时间线设计有什么启发
```

### `queue_picture()`

```text
请分析 `queue_picture()`。

请重点回答：
1. 它只是“往队列里塞帧”吗，还是还承担了别的语义
2. 它如何把 pts、duration、serial、位置等信息绑定到显示帧
3. 为什么它是视频解码和视频显示之间的重要桥梁
4. 我自己的项目中，类似职责更适合放在哪一层
```

---

## 4.5 播放生命周期与主线程函数

### `stream_open()`

```text
请分析 `stream_open()`。

请重点回答：
1. 为什么它可以视为播放器主链的真正启动入口之一
2. 它创建/初始化了哪些关键对象
3. 哪些子线程或子模块从这里开始联动
4. 这和我自己的 `VideoPlayer::open()` / `PlayerCore::open()` 有什么对应关系
```

### `stream_component_open()`

```text
请重点分析 `stream_component_open()`。

请回答：
1. 它为什么是 demux 和 decoder 之间的桥梁
2. 打开 audio/video/subtitle stream 时分别做了哪些不同事情
3. 它如何触发后续线程和设备初始化
4. 如果我要设计统一的 stream open 路径，这里最值得借鉴什么
```

### `audio_open()`

```text
请分析 `audio_open()`。

请重点回答：
1. 它除了打开音频设备，还承担了哪些协商责任
2. 它如何确定 sample rate、channel layout、buffer size
3. 音频设备参数为什么会反过来影响同步和重采样
4. 这和我的 `src/audio_player.cpp` 有什么可对照的地方
```

### `read_thread()`

```text
请重点分析 `read_thread()`。

这是 ffplay 的核心线程之一，请按下面结构回答：
1. 它在整个播放器中的职责是什么
2. 它与 demux、seek、packet queue、EOF 处理分别是什么关系
3. 它为什么可以看作“主生产者线程”
4. 它如何把不同类型的 packet 分发到不同 queue
5. seek 请求在这个线程里是如何被真正执行的
6. 如果我要在自己的播放器里做主读线程，这个函数最值得抄哪些结构
```

### `stream_close()`

```text
请分析 `stream_close()`。

请重点回答：
1. 播放器关闭时必须按什么顺序收资源
2. 为什么 close 不是简单 free 几个对象
3. 它如何和 queue abort、thread join、decoder shutdown 配合
4. 这个关闭路径最值得我借鉴哪些设计
```

---

## 4.6 同步与显示函数

### `get_master_clock()`

```text
请分析 `get_master_clock()`。

请重点回答：
1. master clock 的选择规则是什么
2. 为什么播放器需要“主时钟”这个概念
3. 这个函数虽然短，但为什么对整个同步系统很关键
4. 如果我要设计自己的同步策略，这里最值得抽象出来的是什么
```

### `check_external_clock_speed()`

```text
请分析 `check_external_clock_speed()`。

请重点回答：
1. external clock 为什么需要动态调速
2. 它依据什么信号来调整速度
3. 这套思路适用于哪些场景，不适用于哪些场景
4. 对本地文件播放器来说，这部分值得学到什么程度
```

### `compute_target_delay()`

```text
请重点分析 `compute_target_delay()`。

请回答：
1. 它是在修正什么问题
2. 它如何根据 master clock 和视频时钟的差异调整 delay
3. 哪些阈值最关键
4. 为什么播放器同步通常不是“简单 sleep 到 pts”
5. 这套算法里最值得我借鉴的思路是什么
```

### `synchronize_audio()`

```text
请分析 `synchronize_audio()`。

请重点回答：
1. 它如何通过音频样本数调整来做 A/V sync
2. 它和 `compute_target_delay()` 的思路有何不同
3. 为什么音频同步常常通过“样本级修正”而不是粗暴跳变
4. 这对我自己的音频时钟设计有什么启发
```

### `video_refresh()`

```text
请重点分析 `video_refresh()`。

这是 ffplay 最关键的显示控制函数之一，请按下面结构回答：
1. 它的职责是什么
2. 它如何从 `pictq` 取帧
3. 它如何决定显示、延迟、丢帧、重复显示
4. 它和 `compute_target_delay()`、`get_master_clock()` 的关系是什么
5. 为什么说它是“视频显示总控函数”
6. 如果我自己的项目只有一个显示循环，这个函数最值得抄什么
```

### `refresh_loop_wait_event()`

```text
请分析 `refresh_loop_wait_event()`。

请重点回答：
1. 它如何把 SDL 事件循环和视频刷新结合起来
2. 为什么播放器主线程不能只做普通 GUI 事件循环
3. 它和 `video_refresh()` 的调用关系是什么
4. 这对我的 `pumpEvents / display loop` 设计有什么启发
```

---

## 4.7 seek、切流、切状态相关函数

### `stream_seek()`

```text
请重点分析 `stream_seek()`。

请回答：
1. 这个函数本身做了什么，它没做什么
2. 为什么 seek 通常要分“发请求”和“真正执行”两个阶段
3. 它和 `read_thread()` 的关系是什么
4. 为什么 seek 语义一定会牵扯到 queue flush 和 serial
5. 我的播放器如果要做稳定 seek，这里最该借鉴什么
```

### `stream_cycle_channel()`

```text
请分析 `stream_cycle_channel()`。

请重点回答：
1. 切音轨/切字幕/切视频流时，为什么需要专门的 channel cycle 逻辑
2. 这会影响哪些 queue、decoder、时钟状态
3. 它对理解播放器“动态切流”有什么帮助
4. 我当前项目如果以后要做多音轨/多字幕，这里哪些设计值得参考
```

---

## 4.8 最值得优先问的 `ffplay.c` 函数组合

如果时间有限，建议优先按下面的组合来问，而不是零散地一个个问。

### 组合 A：播放器主链

```text
请组合分析以下函数在 ffplay 主链中的关系：
- stream_open()
- stream_component_open()
- read_thread()
- decoder_decode_frame()
- video_thread()
- queue_picture()
- video_refresh()

请输出一张文字版时序图，并解释每个函数在主链中的位置。
```

### 组合 B：队列体系

```text
请组合分析以下函数：
- packet_queue_put()
- packet_queue_get()
- packet_queue_flush()
- frame_queue_peek_writable()
- frame_queue_push()
- frame_queue_peek_readable()
- frame_queue_next()

请重点说明：
1. 生产者-消费者模型是怎么建立的
2. seek / flush 时旧数据怎么失效
3. 这套设计为什么比“一个简单队列”更适合播放器
```

### 组合 C：同步体系

```text
请组合分析以下函数：
- get_master_clock()
- check_external_clock_speed()
- compute_target_delay()
- synchronize_audio()
- video_refresh()

请重点说明 ffplay 的 A/V sync 是如何形成闭环的。
```

### 组合 D：seek 与状态切换

```text
请组合分析以下函数：
- stream_seek()
- read_thread()
- packet_queue_flush()
- packet_queue_put_nullpacket()
- decoder_decode_frame()

请重点说明 ffplay 是如何保证 seek 后旧包/旧帧不会污染新播放位置的。
```

---

## 5. `MPC Video Renderer` 函数级提问示例

本地目录：`D:\VSProject\MPCVideoRenderer`

这里先给你一组最值得问的函数，便于你后续继续往渲染器方向深入。

## 5.1 `CMpcVideoRenderer` 相关

### `CMpcVideoRenderer::Receive()`

```text
请分析 D:\VSProject\MPCVideoRenderer\Source\VideoRenderer.cpp 中的 `CMpcVideoRenderer::Receive()`。

请重点回答：
1. 它和 `DoRenderSample()` 的边界是什么
2. 为什么 renderer 不能在 receive 路径里简单粗暴地直接画
3. flushing、pause、sample 生命周期在这里如何处理
4. 这对我自己的 renderer 主路径设计有什么启发
```

### `CMpcVideoRenderer::DoRenderSample()`

```text
请分析 `CMpcVideoRenderer::DoRenderSample()`。

请重点回答：
1. 它在渲染主链中的准确位置
2. 输入 sample 在这里经历了哪些关键处理
3. 它和 video processor 的边界是什么
4. 为什么这一步通常是 renderer 的关键热路径
```

### `CMpcVideoRenderer::Init()`

```text
请分析 `CMpcVideoRenderer::Init()`。

请重点回答：
1. 它初始化了哪些资源
2. 哪些资源属于窗口层，哪些属于设备层，哪些属于 video processor 层
3. 为什么 renderer 初始化通常需要支持重入/重建
4. 这和我的 `d3d11_video_renderer` 初始化路径有什么可对应之处
```

### `CMpcVideoRenderer::Redraw()`

```text
请分析 `CMpcVideoRenderer::Redraw()`。

请重点回答：
1. redraw 和 receive 的职责差异是什么
2. 为什么 renderer 需要主动 redraw 能力
3. subtitle、OSD、窗口变化为什么会牵扯到 redraw
4. 这对 paused frame / resize / expose 场景有什么意义
```

## 5.2 `InputPin` / `Allocator` 相关

### `CVideoRendererInputPin::GetAllocator()`

```text
请分析 `CVideoRendererInputPin::GetAllocator()`。

请重点回答：
1. 为什么 renderer 的 input pin 要参与 allocator 协商
2. 这个函数在 DirectShow renderer 架构中扮演什么角色
3. 它如何影响 sample 的来源与生命周期
```

### `CVideoRendererInputPin::NewSegment()`

```text
请分析 `CVideoRendererInputPin::NewSegment()`。

请重点回答：
1. segment 语义为什么重要
2. 这个函数如何影响时间戳和渲染状态
3. 它和 seek / rate change 的关系是什么
```

### `CCustomAllocator::Alloc()`

```text
请分析 `CCustomAllocator::Alloc()`。

请重点回答：
1. allocator 在 renderer 架构中解决什么问题
2. 为什么这里不只是“分配内存”
3. sample buffer 的布局设计会如何影响后续渲染路径
4. 这对理解 render input path 有什么帮助
```

### `CCustomAllocator::GetBuffer()`

```text
请分析 `CCustomAllocator::GetBuffer()`。

请重点回答：
1. 为什么 `GetBuffer()` 是 allocator 的关键路径
2. 它如何连接上游 sample 请求和下游实际存储
3. media type 变化为什么会在这里出现
4. 我自己的播放器如果以后要强化 renderer input，这里最值得学什么
```

## 5.3 `CDX11VideoProcessor` 相关

### `CDX11VideoProcessor::Init()`

```text
请分析 `CDX11VideoProcessor::Init()`。

请重点回答：
1. DX11 video processor 初始化了哪些关键对象
2. device、swap chain、纹理、shader、subpic 支持在这里如何建立
3. 为什么 processor 层值得单独抽象，而不是塞进 renderer 主类
4. 这对我自己的 D3D11 renderer 分层有什么启发
```

### `CDX11VideoProcessor::UpdateSubPic()`

```text
请分析 `CDX11VideoProcessor::UpdateSubPic()`。

请重点回答：
1. subtitle / subpic 在 DX11 renderer 中的叠加路径是什么
2. 为什么 subtitle 支持常常会影响 native/copy-back 路径选择
3. 这对我自己的字幕与渲染整合有什么启发
```

### `CDX11VideoProcessor::DrawStats()`

```text
请分析 `CDX11VideoProcessor::DrawStats()`。

请重点回答：
1. 为什么 renderer 内部会维护这么多统计信息
2. copy / paint / present / sync offset 这些指标各自说明什么
3. 这些可观测性设计对播放器排障有什么价值
4. 我自己的项目是否值得尽早加入类似统计面板
```

---

## 6. 连续追问的推荐方式

函数级分析最好的方式不是“一次问完”，而是 **链式追问**。

推荐顺序如下：

1. 先问结构体
2. 再问单个函数
3. 再问函数组合
4. 最后问“映射到我的项目”

例如你分析 `video_refresh()` 时，可以这样连续问：

### 第 1 问：单函数职责

```text
请分析 `video_refresh()` 的职责、时序位置和关键逻辑。
```

### 第 2 问：与相关函数联动

```text
请把 `video_refresh()` 与 `compute_target_delay()`、`get_master_clock()` 组合起来分析。
```

### 第 3 问：映射到当前项目

```text
请把 `video_refresh()` 这一套设计映射到我的项目：
- src/display.cpp
- src/core/scheduler.cpp
- src/core/clock.cpp
并指出我的设计缺口。
```

---

## 7. 你最值得优先使用的 10 个 `ffplay.c` 提问模板

如果你现在就准备开始问 AI，建议优先顺序如下：

1. `VideoState`
2. `PacketQueue`
3. `packet_queue_get()`
4. `FrameQueue`
5. `decoder_decode_frame()`
6. `read_thread()`
7. `stream_component_open()`
8. `get_master_clock()`
9. `compute_target_delay()`
10. `video_refresh()`

这 10 个问题基本能把 `ffplay` 的骨架问出来。

---

## 8. 一句话版结论

函数级提问的关键不是“问这个函数做了什么”，而是：

- **它在整条链上的职责是什么**
- **它如何和上下游函数配合**
- **它处理了哪些异常和同步语义**
- **它哪些设计最值得借鉴到你自己的播放器项目中**

只要你始终按这个框架问，AI 的回答质量会比简单问“解释下这个函数”高很多。
