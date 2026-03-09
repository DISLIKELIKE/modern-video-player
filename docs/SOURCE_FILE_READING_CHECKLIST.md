# 按文件名展开的阅读清单

更新日期：2026-03-09

## 1. 使用方法

这份清单不是“把代码从头看到尾”的目录，而是按 **最短反馈路径** 排的阅读顺序。

每读一个文件/函数，都只做三件事：

1. 先回答它在整条链上的职责
2. 再回答它和上/下游怎么连接
3. 最后写下“我项目里哪个文件对应它”

建议每一小节都配一页自己的笔记，笔记模板固定成：

- 这个文件/函数负责什么
- 它依赖谁
- 它把结果交给谁
- 哪个设计值得借鉴到 `modern-video-player`

---

## 2. `ffplay.c` 阅读清单

本地文件：`C:\Users\PVT01\Downloads\ffplay.c`

`ffplay.c` 是一份单文件播放器，因此阅读顺序不要按行号，而要按“主链职责”读。

## 2.1 第 1 遍：先建立状态模型

### 先看这些结构体

1. `PacketQueue`
2. `Clock`
3. `FrameQueue`
4. `Decoder`
5. `VideoState`

### 带着这些问题看

- 为什么 `ffplay` 既有 `PacketQueue` 又有 `FrameQueue`？
- `VideoState` 为什么像一个“总状态对象”？
- `Clock` 为什么被单独抽象出来？
- `Decoder` 为什么不只是一个 `AVCodecContext` 包装？

### 这一遍的目标

读完后你应该能自己画出：

`VideoState -> queues -> decoders -> clocks -> refresh`

---

## 2.2 第 2 遍：先看包队列，再看帧队列

### 文件

- `ffplay.c`

### 先看函数

1. `packet_queue_init()`
2. `packet_queue_put_private()`
3. `packet_queue_put()`
4. `packet_queue_get()`
5. `packet_queue_abort()`
6. `packet_queue_flush()`
7. `packet_queue_start()`

### 带着这些问题看

- 包队列的阻塞点在哪里？
- `serial` 为什么跟 queue 绑在一起？
- abort / flush / start 各自解决什么问题？
- 为什么 seek 之后旧包不能继续被消费？

### 再看函数

1. `frame_queue_init()`
2. `frame_queue_peek_writable()`
3. `frame_queue_peek_readable()`
4. `frame_queue_push()`
5. `frame_queue_next()`
6. `frame_queue_nb_remaining()`

### 带着这些问题看

- 为什么帧队列要区分 `writable` 和 `readable`？
- `keep_last` 的意义是什么？
- 显示线程为什么不直接拿 decoder 的输出帧？

### 对照你当前项目

- `src/core/frame_pool.cpp`
- `src/core/decoder_thread.cpp`

---

## 2.3 第 3 遍：看 decoder 是如何从包变成帧的

### 文件

- `ffplay.c`

### 先看函数

1. `decoder_decode_frame()`

### 再顺着看线程函数

1. `audio_thread()`
2. `video_thread()`
3. `subtitle_thread()`

### 带着这些问题看

- `decoder_decode_frame()` 为什么是整个解码路径的核心？
- 为什么视频、音频、字幕最终还是各走各的线程函数？
- decoder 线程什么时候等包，什么时候产帧，什么时候退出？
- flush / EOF / null packet 是怎么参与解码状态切换的？

### 这一遍要搞清楚

- 压缩包是怎么被送进 decoder 的
- 帧是怎么被送进 frame queue 的
- 三种媒体类型为什么共用一套总体思路，但细节不同

### 对照你当前项目

- `src/decoder/decoder_factory.cpp`
- `src/core/decoder_thread.cpp`
- `src/core/player_core.cpp`

---

## 2.4 第 4 遍：看播放主链从哪里启动

### 文件

- `ffplay.c`

### 先看函数

1. `stream_open()`
2. `stream_component_open()`
3. `audio_open()`
4. `read_thread()`
5. `stream_close()`

### 带着这些问题看

- 播放器的初始化入口到底在哪里？
- 什么时候打开 demuxer，什么时候打开 codec，什么时候开 SDL 音频设备？
- `read_thread()` 为什么经常是播放器“主生产线程”？
- `stream_component_open()` 为什么是连接 demux 和具体解码线程的桥？
- `stream_close()` 是如何收口所有线程和资源的？

### 这一遍的目标

读完后你应该能画出：

`stream_open -> stream_component_open -> read_thread -> decoder thread -> frame queue`

### 对照你当前项目

- `src/main.cpp`
- `src/video_player.cpp`
- `src/demuxer.cpp`
- `src/audio_player.cpp`

---

## 2.5 第 5 遍：只看时钟和同步

### 文件

- `ffplay.c`

### 先看函数

1. `get_master_clock()`
2. `check_external_clock_speed()`
3. `compute_target_delay()`
4. `synchronize_audio()`
5. `video_refresh()`
6. `refresh_loop_wait_event()`

### 带着这些问题看

- 当前 master clock 是谁决定的？
- 为什么视频显示不只是“有帧就画”？
- `compute_target_delay()` 到底在修正什么？
- 音频为什么通常更适合做主时钟？
- `video_refresh()` 为什么是显示侧的总控函数？

### 这一遍的目标

把下面这条链讲清楚：

`clock selection -> delay correction -> frame display / drop / duplicate`

### 对照你当前项目

- `src/core/clock.cpp`
- `src/core/scheduler.cpp`
- `src/display.cpp`

---

## 2.6 第 6 遍：只看 seek / flush / serial

### 文件

- `ffplay.c`

### 先看函数

1. `stream_seek()`
2. `read_thread()` 里处理 seek 的部分
3. `packet_queue_flush()`
4. `packet_queue_put_nullpacket()`
5. `decoder_decode_frame()` 里处理 serial / flush 的部分

### 带着这些问题看

- seek 请求是从哪里发起的？
- 为什么 seek 不是简单改个 timestamp？
- `serial` 为什么是清理旧数据的关键机制？
- flush packet / null packet 各自扮演什么角色？

### 这一遍的目标

把“播放器为什么能 seek 后不把旧帧当成新帧”说清楚。

---

## 2.7 `ffplay.c` 读完后你应该输出什么

至少写出这 4 样东西：

1. `ffplay` 的总状态图
2. `PacketQueue / FrameQueue / Clock / Decoder / VideoState` 的职责表
3. 播放主链时序图
4. 和你当前项目的对照表

---

## 3. `MPC Video Renderer` 阅读清单

本地目录：`D:\VSProject\MPCVideoRenderer`

这部分不要先去看 shader，也不要先去看庞大的 `DX11VideoProcessor.cpp`。正确顺序是：

`Readme -> 主 renderer -> 输入 pin -> allocator -> processor 抽象 -> DX11 实现 -> 硬件 VP / shader 细节`

## 3.1 第 1 遍：先看项目入口和主类

### 先看文件

1. `D:\VSProject\MPCVideoRenderer\Readme.md`
2. `D:\VSProject\MPCVideoRenderer\Source\VideoRenderer.h`
3. `D:\VSProject\MPCVideoRenderer\Source\VideoRenderer.cpp`

### 先看函数/类

- 类：`CMpcVideoRenderer`
- 函数：
  - `CheckMediaType()`
  - `SetMediaType()`
  - `Receive()`
  - `DoRenderSample()`
  - `Run()`
  - `Pause()`
  - `Stop()`
  - `Init()`
  - `Redraw()`
  - `DeliverFrame()`

### 带着这些问题看

- `CMpcVideoRenderer` 的边界到底在哪里？
- 它什么时候只是“接 sample”，什么时候真的“画画面”？
- `Receive()` 和 `DoRenderSample()` 为什么拆开？
- `Init()` 是在初始化什么：窗口、设备、processor 还是全部？
- `Redraw()` 什么时候被主动触发？

### 这一遍的目标

先建立一个总图：

`InputPin -> Renderer -> VideoProcessor -> Present`

---

## 3.2 第 2 遍：只看输入 pin

### 先看文件

1. `D:\VSProject\MPCVideoRenderer\Source\VideoRendererInputPin.h`
2. `D:\VSProject\MPCVideoRenderer\Source\VideoRendererInputPin.cpp`

### 先看函数

- `GetAllocator()`
- `GetAllocatorRequirements()`
- `NewSegment()`
- `BeginFlush()`

### 带着这些问题看

- 为什么 renderer 还要自己定义 input pin？
- pin 和 renderer 主体的边界怎么划？
- allocator 协商为什么从 pin 开始？
- `NewSegment()`、`BeginFlush()` 为什么会出现在 pin 层？

### 这一遍的目标

搞清楚：

- 上游解码器/滤镜是怎么把 sample 送进来的
- renderer 为什么需要在 pin 层处理协商、flush、segment 切换

---

## 3.3 第 3 遍：只看 allocator

### 先看文件

1. `D:\VSProject\MPCVideoRenderer\Source\CustomAllocator.h`
2. `D:\VSProject\MPCVideoRenderer\Source\CustomAllocator.cpp`

### 先看函数

- 类：`CCustomAllocator`
- 函数：
  - `Alloc()`
  - `SetProperties()`
  - `GetBuffer()`
  - `SetNewMediaType()`
  - `ClearNewMediaType()`

### 带着这些问题看

- 为什么渲染器需要自定义 allocator？
- allocator 解决的是内存问题、sample 生命周期问题，还是媒体类型协商问题？
- `GetBuffer()` 为什么是关键路径？
- “新的 media type 跟着 sample 走”这件事是怎么实现的？

### 这一遍的目标

明白 `allocator` 在渲染器中的价值：

- 控制 sample 来源
- 控制缓冲布局
- 承接媒体类型变化

---

## 3.4 第 4 遍：先看 processor 抽象，再看 DX11 实现

### 先看文件

1. `D:\VSProject\MPCVideoRenderer\Source\VideoProcessor.h`
2. `D:\VSProject\MPCVideoRenderer\Source\VideoProcessor.cpp`

### 带着这些问题看

- `CVideoProcessor` 是“图像处理器”还是“渲染后端抽象”？
- 哪些职责被它抽象出来了：尺寸、纵横比、显示、统计、颜色、HDR？
- 为什么 renderer 主体不把这些逻辑全都自己扛着？

### 再看文件

1. `D:\VSProject\MPCVideoRenderer\Source\DX11VideoProcessor.h`
2. `D:\VSProject\MPCVideoRenderer\Source\DX11VideoProcessor.cpp`

### 先看函数/方法

- 类：`CDX11VideoProcessor`
- 方法：
  - `Init()`
  - `SetCallbackDevice()`
  - `UpdateSubPic()`
  - `DrawStats()`
  - `UpdateRenderRect()`
  - `MemCopyToTexSrcVideo()`

### 带着这些问题看

- DX11 设备和 swap chain 是在哪层创建和维护的？
- sample 进入后，什么时候被 copy，什么时候直接处理？
- subtitle / OSD 是在哪一步叠加的？
- stats 和 sync 统计为什么放在 processor 里？
- `UpdateRenderRect()` 为什么对 renderer 很关键？

### 这一遍的目标

画出一条更细的链：

`sample -> texture/surface -> video processor -> subtitle/osd -> swap chain present`

---

## 3.5 第 5 遍：最后再看硬件 VP 和 shader 细节

### 先看文件

1. `D:\VSProject\MPCVideoRenderer\Source\D3D11VP.h`
2. `D:\VSProject\MPCVideoRenderer\Source\D3D11VP.cpp`
3. `D:\VSProject\MPCVideoRenderer\Source\DXVA2VP.h`
4. `D:\VSProject\MPCVideoRenderer\Source\DXVA2VP.cpp`
5. `D:\VSProject\MPCVideoRenderer\Source\Shaders.h`
6. `D:\VSProject\MPCVideoRenderer\Source\Shaders.cpp`

### 带着这些问题看

- `D3D11VP` 和 `DXVA2VP` 分别解决什么问题？
- 硬件 video processor 和 shader path 的边界在哪里？
- 哪些后处理适合硬件 VP，哪些适合 shader？
- 为什么 modern renderer 需要同时保留多条处理路径？

### 这一遍的目标

这不是为了一次看懂全部算法，而是为了理解：

- 渲染器为什么不只是“画一张纹理”
- 为什么 renderer 内部会有多条数据路径和能力路径

---

## 3.6 `MPCVR` 读完后你应该输出什么

至少写出这 4 样东西：

1. `CMpcVideoRenderer` 的职责图
2. `InputPin -> Allocator -> Renderer -> Processor` 的链路图
3. `DX11` 路径的资源流转图
4. 和你当前 `src/render/d3d11_video_renderer.cpp` 的差异清单

---

## 4. `MPC-BE` 阅读清单

本地目录：`D:\VSProject\MPC-BE`

### 当前限制

基于当前环境扫描结果，`D:\VSProject\MPC-BE` 目前**还看不到完整源码树**，只能确认目录存在，尚不足以给出可靠的“逐文件函数级”阅读顺序。

因此这一部分分成两步：

1. **先确认本地副本完整**
2. **再按播放器壳层 -> 输出设置 -> 内部解码器/滤镜 -> 渲染器联动** 的顺序展开

## 4.1 第 0 步：先确认源码完整性

### 先做什么

- 先确认顶层是否能看到完整源码目录，而不是只有 `.git`
- 先找到：
  - `README`
  - 解决方案 / 工程文件
  - 源码目录
  - 播放器应用层目录
  - 解码器 / 滤镜 / renderer 目录

### 带着这些问题看

- 当前副本是完整 clone、浅克隆、空仓、还是只初始化了 git 目录？
- 如果源码不完整，哪些架构判断会失真？

---

## 4.2 源码完整后，建议按这个顺序读

### 第 1 组：先看项目入口和播放器壳层

#### 先找这类文件

- 项目根 README / build 说明
- 解决方案文件
- 播放器应用层入口文件
- 主窗口 / 主框架文件

#### 带着这些问题看

- 真正控制“打开文件、播放、暂停、seek、切输出”的主对象是谁？
- 设置项最终是如何落到播放链上的？
- 播放器壳层和内部 filter / renderer 是如何解耦的？

### 第 2 组：再看输出设置和 renderer 选择

#### 先找这类文件

- output / options / settings 相关文件
- renderer 选择和 graph 重建相关文件

#### 带着这些问题看

- `EVR / MPCVR / madVR` 这些选项是在哪里切换的？
- 切换 renderer 后，哪些对象需要重建？
- 为什么解码方式和渲染器需要联动？

### 第 3 组：再看内部解码器 / 滤镜组织

#### 先找这类文件

- internal filters
- source / splitter
- video decoder
- audio decoder

#### 带着这些问题看

- 这是一个“外部 graph 驱动”的播放器，还是“内建组件为主”的播放器？
- 哪些解码器是内建的，哪些依赖外部组件？
- 现代硬解路径是如何跟 renderer 配套的？

### 第 4 组：最后看成熟产品能力

#### 先找这类文件

- playlist
- subtitle
- hotkey / command
- screenshot
- chapter / navigation

#### 带着这些问题看

- 哪些能力是成熟桌面播放器必备的？
- 哪些能力对当前项目是“以后再做”更合理？

---

## 4.3 `MPC-BE` 这部分最正确的打开方式

不要让 AI 一上来就“讲 MPC-BE 架构”，而是先这样问：

1. 请先确认 `D:\VSProject\MPC-BE` 是否是完整源码副本
2. 请列出播放器壳层的入口文件
3. 请列出 output / renderer 设置相关文件
4. 请列出 internal video decoder / renderer 相关文件
5. 再给我阅读顺序

只有这样，AI 才能给出真正可靠的文件级清单。

---

## 5. 建议你按这个节奏读

### 如果今天只有 1 小时

- `ffplay.c`：只看 `VideoState`、`PacketQueue`、`FrameQueue`、`read_thread()`、`video_refresh()`

### 如果今天有半天

- `ffplay.c`：完整看完第 1~6 遍
- `MPCVR`：看完 `VideoRenderer.*`、`VideoRendererInputPin.*`、`CustomAllocator.*`

### 如果今天有一天

- `ffplay.c`：主链 + 同步 + seek
- `MPCVR`：主 renderer + pin + allocator + DX11VideoProcessor 主路径

---

## 6. 读完这份清单后，最值得回看你自己项目的文件

建议立即回看：

- `src/core/player_core.cpp`
- `src/core/scheduler.cpp`
- `src/core/clock.cpp`
- `src/core/frame_pool.cpp`
- `src/demuxer.cpp`
- `src/display.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/renderer_factory.cpp`

并强制回答这 4 个问题：

1. 我的总状态对象够不够清晰？
2. 我的 queue / frame / clock 语义是否明确？
3. 我的 renderer 边界是否清晰？
4. 我的设置项和实际播放链是否已经能一一对应？

---

## 7. 一句话版顺序

如果只记一条顺序，就记这个：

- `ffplay.c`：先看状态，再看 queue，再看 decode，再看 sync，再看 seek
- `MPCVR`：先看 renderer 主体，再看 input pin，再看 allocator，再看 DX11 processor
- `MPC-BE`：先确认源码完整，再看播放器壳层，再看输出设置，再看内部解码/渲染联动
