# 适合当前项目的播放状态机设计草案

更新日期：2026-03-09

## 1. 文档目标

这份文档不是泛泛讲“播放器状态机是什么”，而是专门面向当前仓库 `modern-video-player` 的 `PlayerCore`，给出一份 **可以直接转成工程实现的播放状态机草案**。

它主要解决 4 个问题：

1. 当前 `PlayerCore` 里哪些状态已经存在，哪些其实只是“隐式状态”
2. 适合当前项目的主状态应该怎么拆
3. 事件、守卫条件、转换动作应该如何定义
4. 如果要落地到代码里，应该优先改哪些文件

---

## 2. 先看当前 `PlayerCore` 的真实状态现状

从当前实现看，`PlayerCore` 明面上的状态只有：

- `Stopped`
- `Playing`
- `Paused`

定义位置：`include/core/player_core.h`

但如果按真实运行过程来看，当前项目其实已经存在更多“隐式状态”，只是它们没有被明确建模成枚举。

### 2.1 当前显式状态

当前枚举：

- `Stopped`
- `Playing`
- `Paused`

### 2.2 当前隐式状态

| 隐式状态 | 当前代码中的表现 | 问题 |
| --- | --- | --- |
| 未加载媒体 | `opened_ == false` | 没有单独状态名，和“打开失败后空闲”混在一起 |
| 已打开未播放 | `opened_ == true && state_ == Stopped` | 语义上更像 `Ready`，但被挤在 `Stopped` 里 |
| 正在打开 | `open()` 函数执行中 | 没有状态，出了问题只能靠返回值和 error callback 判断 |
| 正在 seek | `seek()` 里临时暂停 scheduler / 停 demux / flush / 重启 | 这是重要的控制状态，但现在完全隐式 |
| 正在 stop | `stop()` 里停止线程、flush、seek(0)、reset clock | 也是有动作的过程态，但目前没有建模 |
| EOF 已结束 | `onRenderIdle()` 中检测到 EOF 后把状态改成 `Stopped` | “正常播完”和“手动 stop”语义不同，却被压成同一个状态 |
| 切下一项/上一项/退出请求 | `next_item_requested_`、`previous_item_requested_`、`quit_requested_` | 这些不是主状态，但属于重要的会话意图 |
| 打开失败/运行错误 | `emitError(...)` + 返回 false | 没有统一 `Error` 状态，UI 很难清晰呈现 |

### 2.3 当前最大的建模问题

当前最核心的问题不是“状态太少”，而是：

> **`Stopped` 被同时拿来表示 4 种完全不同的语义。**

当前 `Stopped` 实际上混合了：

1. 还没打开媒体
2. 已经打开，但尚未开始播放
3. 用户手动停止后回到起点
4. 媒体播放到 EOF 自动结束

这会带来几个典型问题：

- UI 很难区分“准备就绪”和“已经播完”
- `play()` 无法针对 `Ready` 与 `Ended` 做不同处理
- `seek()` 前后恢复逻辑只能靠局部变量 `was_playing`
- 未来做自动播放下一集、重播、历史恢复时，状态语义会越来越乱

---

## 3. 适合当前项目的状态机设计原则

这份草案建议遵循 5 条原则：

1. **主状态只表达播放生命周期**
2. **A-B Repeat、截图、字幕开关这类功能不要塞进主状态枚举**
3. **线程是否运行，不等于播放主状态**
4. **seek / stop / open 这种有明显过程动作的行为，要有过程态**
5. **先做扁平状态机，逻辑上再按层次状态机理解，不必一开始就上复杂框架**

换句话说：

- 主状态机负责“播放器现在处于什么生命周期阶段”
- 工作线程、退出意图、截图请求、AB Repeat 等，作为正交字段存在

---

## 4. 推荐的主状态枚举

基于当前仓库结构，我建议把 `PlaybackState` 演进为下面这组状态：

```cpp
enum class PlaybackState {
    Idle,       // 没有已加载媒体；等价于当前 opened_ == false
    Opening,    // 正在 open()：建 demuxer / renderer / audio / decoder / queue
    Ready,      // 已打开媒体，资源已就绪，但还未进入播放
    Playing,    // 正常播放中
    Paused,     // 暂停中
    Seeking,    // seek 处理中：停 demux / flush / seek / 重建时钟 / 恢复
    Stopping,   // stop 处理中：停线程、flush、回到起点
    Ended,      // 已播放到 EOF，等待用户重播/seek/关闭
    Error       // 当前会话进入错误态
};
```

### 4.1 为什么是这 8 个，而不是更多

因为它们刚好覆盖了当前仓库最真实的生命周期：

- `Idle`：对应还没 open 或 close 后
- `Opening`：对应当前 `open()` 整个过程
- `Ready`：对应当前 `opened_ == true && state_ == Stopped`
- `Playing` / `Paused`：对应当前显式状态
- `Seeking`：对应当前 `seek()` 期间的隐式过程态
- `Stopping`：对应当前 `stop()` 期间的隐式过程态
- `Ended`：对应当前 EOF 自动停止的语义
- `Error`：对应当前 `emitError()` 但没有状态承接的情况

### 4.2 为什么暂时不建议加 `Buffering`

当前项目虽然有流媒体相关代码，但 `PlayerCore` 的主链还没有真正把“网络缓冲不足”建成播放器一级状态。
所以当前阶段不建议急着把 `Buffering` 塞进主状态机。

更稳妥的顺序是：

1. 先把本地播放生命周期建模做干净
2. 再在后续流媒体链里补 `Buffering / Rebuffering`

---

## 5. 不应进入主状态枚举的正交信息

下面这些东西很重要，但不应该塞进 `PlaybackState`：

### 5.1 退出意图

建议新增：

```cpp
enum class ExitIntent {
    None,
    QuitApp,
    NextItem,
    PreviousItem
};
```

对应当前字段：

- `quit_requested_`
- `next_item_requested_`
- `previous_item_requested_`

这类信息本质上是 **会话控制意图**，不是播放生命周期本身。

### 5.2 seek 恢复目标

建议新增：

```cpp
enum class ResumeTarget {
    Ready,
    Playing,
    Paused,
    Ended
};
```

因为 `seek()` 完成后，并不总是回到 `Playing`：

- 播放中 seek：恢复到 `Playing`
- 暂停中 seek：恢复到 `Paused`
- `Ready` 状态 seek：仍然回到 `Ready`
- `Ended` 后 seek：通常回到 `Paused` 或 `Ready`，取决于产品定义

### 5.3 正交功能位

下面这些继续保持字段/flag 即可：

- `ab_repeat_enabled_`
- `ab_repeat_start_`
- `ab_repeat_end_`
- `screenshot_requested_`
- `subtitle_enabled_`
- `prefer_hardware_decode_`
- `audio_delay_seconds_`
- `subtitle_delay_seconds_`

这些都是功能开关或参数，不属于主状态。

### 5.4 工作线程状态

下面这些应继续作为执行层状态，而不是主播放状态：

- `demux_running_`
- `audio_consumer_running_`
- `scheduler_` 的 running / paused 内部状态

因为“线程在不在跑”不等于“播放状态是什么”。
例如：

- `Seeking` 时，主状态是 seek 中，但某些线程可能暂时被停下
- `Stopping` 时，主状态是 stop 中，但线程正在退出收尾

---

## 6. 事件模型建议

状态机不是只有“状态”，还要有“事件”。
按当前项目结构，建议把事件分为 4 类。

### 6.1 API 事件

来自上层 `VideoPlayer` 或外部控制层：

- `OpenRequested(file)`
- `CloseRequested`
- `PlayRequested`
- `PauseRequested`
- `StopRequested`
- `SeekRequested(target)`
- `StepForwardRequested`
- `StepBackwardRequested`
- `SetVolumeRequested`
- `SetSpeedRequested`

### 6.2 Renderer/UI 事件

当前主要来自 `pumpEvents()`：

- `TogglePauseRequested`
- `SeekRatioRequested`
- `SeekDeltaRequested`
- `NextChapterRequested`
- `PreviousChapterRequested`
- `ToggleSubtitleRequested`
- `SetABStartRequested`
- `SetABEndRequested`
- `ClearABRequested`
- `ScreenshotRequested`
- `NextItemRequested`
- `PreviousItemRequested`
- `QuitRequested`

### 6.3 Worker 内部事件

建议逐步显式化：

- `OpenSucceeded`
- `OpenFailed(error)`
- `SeekSucceeded`
- `SeekFailed(error)`
- `StopCompleted`
- `EofDrained`
- `FatalError(error)`

### 6.4 Tick/条件事件

这些不一定要做成真正消息，但逻辑上要当成事件看：

- `QueuesDrainedAtEof`
- `RendererClosed`
- `DemuxThreadExited`
- `AudioConsumerExited`

---

## 7. 推荐的状态图

下面这张图是按当前 `PlayerCore` 结构抽出来的最实用版本。

```text
Idle
  -- OpenRequested --> Opening
Opening
  -- OpenSucceeded --> Ready
  -- OpenFailed --> Error

Ready
  -- PlayRequested --> Playing
  -- SeekRequested --> Seeking(resume=Ready)
  -- CloseRequested --> Idle
  -- FatalError --> Error

Playing
  -- PauseRequested --> Paused
  -- SeekRequested --> Seeking(resume=Playing)
  -- StopRequested --> Stopping
  -- EofDrained --> Ended
  -- FatalError --> Error

Paused
  -- PlayRequested --> Playing
  -- SeekRequested --> Seeking(resume=Paused)
  -- StepForward/Backward --> Paused
  -- StopRequested --> Stopping
  -- CloseRequested --> Idle
  -- FatalError --> Error

Seeking
  -- SeekSucceeded(resume=Ready) --> Ready
  -- SeekSucceeded(resume=Playing) --> Playing
  -- SeekSucceeded(resume=Paused) --> Paused
  -- SeekSucceeded(resume=Ended) --> Ended
  -- SeekFailed --> Error 或 回原稳定状态

Stopping
  -- StopCompleted --> Ready
  -- FatalError --> Error

Ended
  -- PlayRequested --> Seeking(target=0, resume=Playing)
  -- SeekRequested --> Seeking(resume=Paused 或 Ready)
  -- StopRequested --> Stopping
  -- CloseRequested --> Idle

Error
  -- CloseRequested --> Idle
  -- OpenRequested --> Opening
```

---

## 8. 关键状态转换说明

### 8.1 `Idle -> Opening -> Ready`

#### 触发事件

- `open(file)`

#### 当前代码里的对应动作

当前 `open()` 做了这些事：

- `close()` 旧会话
- 创建 `Demuxer`
- 打开媒体
- 初始化 renderer
- 初始化 audio
- 初始化 decoders
- 创建 packet queues
- reset clock / reset diagnostics
- 清各种请求位

#### 为什么要单独建 `Opening`

因为 `open()` 不是一个纯赋值操作，而是一整段会失败的资源建立过程。
一旦有了 `Opening`，后面做这些事会更清晰：

- 打开失败时 UI 可以知道是“打开过程失败”，不是单纯 `Idle`
- 以后若做异步 open，也不需要重写状态语义

### 8.2 `Ready -> Playing`

#### 触发事件

- `play()`

#### 当前代码里的对应动作

- `startDemuxThread()`
- `startAudioConsumer()`
- `scheduler_.start()`
- `state_ = Playing`

#### 设计建议

在当前仓库里，这一步可以先不拆出 `Starting`，直接 `Ready -> Playing` 即可。
因为线程启动在当前实现里是同步触发、立刻进入可播放状态的。

### 8.3 `Playing -> Paused -> Playing`

#### 触发事件

- `pause()`
- `play()`（resume）

#### 当前代码里的动作

暂停：

- `scheduler_.pause()`
- `clock_.pause()`
- `audio_player_->pause()`

恢复：

- `scheduler_.resume()`
- `clock_.resume()`
- `audio_player_->resume()`

#### 设计要点

这组转换非常稳定，建议继续保留。
它是当前 `PlayerCore` 里最“像状态机”的一部分。

### 8.4 `Playing/Paused/Ready/Ended -> Seeking`

#### 触发事件

- `seek(timestamp)`
- `seekToNextChapter()`
- `seekToPreviousChapter()`
- `stepFrameForward()` / `stepFrameBackward()` 内部也会调用 seek

#### 当前代码里的动作

- 记录 `was_playing`
- 如在播放，先 `scheduler_.pause()`
- 如果 demux 在跑，先 `stopDemuxThread()`
- `flushPipelines()`
- `demuxer_->seek(timestamp)`
- `avcodec_flush_buffers(...)`
- `audio_player_->stop()`
- 重置 `position_ / clock_ / subtitle`
- 如果 seek 前在播放，则重启 demux / 恢复 audio / resume scheduler

#### 为什么 `Seeking` 必须显式化

因为 seek 不只是“位置改一下”，而是一次 **小型重建流程**。
当前它虽然工作正常，但它的生命周期完全埋在函数内部：

- 外部看不到 seek 正在发生
- UI 无法准确呈现 seek 过程
- 错误恢复只能依赖局部变量
- 后续要做更复杂同步时很难扩展

### 8.5 `Playing -> Ended`

#### 触发事件

- EOF 且音视频队列耗尽

#### 当前代码里的位置

`onRenderIdle()` 中：

- `demuxer_->isEof()`
- packet queue 为空
- frame queue 为空
- 然后把 `state_` 改成 `Stopped`

#### 为什么应该改成 `Ended`

因为 EOF 播完并不等于用户手动 stop。
如果单独建 `Ended`，后面很多产品行为会更合理：

- 点击播放：从头重播
- 自动播放下一项：只在 `Ended` 才触发
- UI 能显示“播放完成”而不是“已停止”

### 8.6 `Playing/Paused/Ended -> Stopping -> Ready`

#### 触发事件

- `stop()`
- `NextItemRequested`
- `PreviousItemRequested`
- `QuitRequested`

#### 当前代码里的 stop 动作

- `stopDemuxThread()`
- `scheduler_.stop()`
- `stopAudioConsumer()`
- `flushPipelines()`
- `audio_player_->stop()`
- `demuxer_->seek(0.0)`
- `position_ = 0`
- `clock_.reset()`
- `state_ = Stopped`

#### 设计建议

建议把 stop 的目标状态定义成 `Ready`，而不是 `Idle`。
因为当前 stop 并不会释放媒体资源，它只是把播放会话回到起点。

也就是说：

- `stop()` 是“回到已加载未播放”
- `close()` 才是“释放媒体会话，回到空闲”

### 8.7 `AnyLoadedState -> Error`

#### 触发事件

- 打开失败
- seek 失败
- decoder init 失败
- renderer init 失败
- 未来的 fatal worker error

#### 当前代码的问题

现在项目已有 `emitError(...)`，但没有一致的错误状态承接。
这会导致：

- UI 只能看到错误回调，状态仍然可能停在旧值
- 错误后系统是否还可继续操作，语义不清楚

#### 设计建议

`Error` 作为主状态保留，但实现上可以分两种：

- **可恢复错误**：保留 media session，可 `seek` / `play` / `close`
- **不可恢复错误**：只允许 `close` 或重新 `open`

当前阶段先不必做这么细，先统一收口到 `Error` 即可。

---

## 9. 事件-状态-动作对照表

下面这张表是最适合直接转代码的版本。

| 当前状态 | 事件 | 守卫条件 | 下一个状态 | 主要动作 |
| --- | --- | --- | --- | --- |
| `Idle` | `OpenRequested(file)` | 文件路径有效 | `Opening` | 初始化会话、清旧资源 |
| `Opening` | `OpenSucceeded` | renderer/decoder/audio 初始化成功 | `Ready` | `opened=true`、`position=0`、`clock.reset()` |
| `Opening` | `OpenFailed` | 任一初始化失败 | `Error` | `emitError()` |
| `Ready` | `PlayRequested` | 媒体已打开 | `Playing` | 启 demux / scheduler / audio consumer |
| `Ready` | `SeekRequested(t)` | 媒体已打开 | `Seeking` | seek 到指定位置，`resume=Ready` |
| `Playing` | `PauseRequested` | 无 | `Paused` | pause scheduler/clock/audio |
| `Playing` | `SeekRequested(t)` | 无 | `Seeking` | pause + stop demux + flush + seek |
| `Playing` | `StopRequested` | 无 | `Stopping` | stop demux / scheduler / audio |
| `Playing` | `EofDrained` | 队列空且 EOF | `Ended` | 发出 ended 通知 |
| `Paused` | `PlayRequested` | 无 | `Playing` | resume scheduler/clock/audio |
| `Paused` | `SeekRequested(t)` | 无 | `Seeking` | seek 到指定位置，`resume=Paused` |
| `Paused` | `StepForward/Backward` | 有视频且可渲染 | `Paused` | seek + renderPausedFrameAtOrAfter |
| `Paused` | `StopRequested` | 无 | `Stopping` | stop workers + reset |
| `Seeking` | `SeekSucceeded` | `resume=Playing` | `Playing` | start/resume demux/audio/scheduler |
| `Seeking` | `SeekSucceeded` | `resume=Paused` | `Paused` | 更新画面与位置，不恢复播放 |
| `Seeking` | `SeekSucceeded` | `resume=Ready` | `Ready` | 更新位置，不启动播放 |
| `Seeking` | `SeekFailed` | 可恢复 | 原稳定状态或 `Error` | 发错误，决定是否回滚 |
| `Stopping` | `StopCompleted` | stop 流程完成 | `Ready` | `position=0`、`clock.reset()` |
| `Ended` | `PlayRequested` | 无 | `Seeking` | 目标 `0.0`，`resume=Playing` |
| `Ended` | `SeekRequested(t)` | 无 | `Seeking` | 从 EOF 位置跳走 |
| `Ended` | `StopRequested` | 无 | `Stopping` | 回到 `Ready` |
| `AnyLoaded` | `CloseRequested` | 无 | `Idle` | stop + release renderer/audio/decoder/demux |
| `AnyState` | `FatalError` | 无 | `Error` | 记录错误并上报 |

---

## 10. 与当前 `PlayerCore` 字段的映射关系

### 10.1 建议保留的字段

这些字段继续保留，语义清楚：

- `position_`
- `volume_`
- `speed_`
- `audio_delay_seconds_`
- `subtitle_delay_seconds_`
- `ab_repeat_*`
- `screenshot_requested_`
- `demux_running_`
- `audio_consumer_running_`
- `video_packet_queue_`
- `audio_packet_queue_`
- `video_queue_`
- `audio_queue_`

### 10.2 建议重构的字段

| 当前字段 | 建议演进 |
| --- | --- |
| `state_` | 改成新的 `PlaybackState` 枚举 |
| `opened_` | 短期可保留；长期建议从 `state_ != Idle` 派生 |
| `quit_requested_` / `next_item_requested_` / `previous_item_requested_` | 收口成 `ExitIntent exit_intent_` |
| `was_playing`（seek 内局部变量） | 提升为 `ResumeTarget resume_target_` |

### 10.3 建议新增的字段

```cpp
std::atomic<PlaybackState> state_{PlaybackState::Idle};
std::atomic<ExitIntent> exit_intent_{ExitIntent::None};
std::atomic<ResumeTarget> resume_target_{ResumeTarget::Ready};
std::atomic<ErrorCode> last_error_{ErrorCode::None};
```

如果你暂时不想在原子枚举上做太多工作，也可以先用普通字段 + mutex 收口。

---

## 11. 推荐的实现方式：不要把状态切换散落在每个函数里

当前 `PlayerCore` 最大的状态机问题，不只是状态少，而是：

> 状态切换动作分散在 `open / play / pause / stop / seek / pumpEvents / onRenderIdle` 各处。

建议逐步收口成下面这种模式：

```cpp
bool PlayerCore::transitionTo(PlaybackState next);
bool PlayerCore::handleEvent(const PlaybackEvent& event);
```

### 11.1 最小落地版本

你不需要一下子做完整事件总线。
先做下面两件事就已经很值：

1. 增加统一的 `setState(next, reason)` 或 `transitionTo(next)`
2. 把 `play/pause/stop/seek/onRenderIdle` 里的状态切换收口到这一处

### 11.2 为什么要收口

因为后面你一定会继续加：

- `Ended`
- `Error`
- `Seeking`
- 跨平台 renderer fallback
- 设备丢失恢复
- 自动播放下一项

如果状态切换还散在各个函数里，后面每加一个分支都会越来越难维护。

---

## 12. 建议的代码改造顺序

如果你要把这套草案落地到当前仓库，推荐按这个顺序做：

### 第一步：只改状态定义，不改行为

先改：

- `include/core/player_core.h`

把：

- `Stopped`
- `Playing`
- `Paused`

扩成：

- `Idle`
- `Opening`
- `Ready`
- `Playing`
- `Paused`
- `Seeking`
- `Stopping`
- `Ended`
- `Error`

先让编译过，再逐步修行为。

### 第二步：先把 `Stopped` 拆成 `Idle / Ready / Ended`

这是收益最大的一步。

对应主要文件：

- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- 可能还有 `src/main.cpp` 中依赖状态值的地方

### 第三步：把 `seek()` 显式化成 `Seeking`

对应主要文件：

- `include/core/player_core.h`
- `src/core/player_core.cpp`

新增：

- `ResumeTarget`
- `seek` 前后的状态保存与恢复逻辑

### 第四步：把 EOF 从 `Stopped` 改成 `Ended`

对应主要位置：

- `src/core/player_core.cpp` 的 `onRenderIdle()`

### 第五步：把退出请求从三个 bool 收口成 `ExitIntent`

对应主要文件：

- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- `src/main.cpp`

### 第六步：最后再引入统一 `transitionTo()`

等状态定义基本稳定以后，再做这一层最稳。

---

## 13. 最小可落地版本（我最推荐你先做的）

如果你不想一次改太大，我最推荐的第一版不是完整 8 状态全上，而是这个 **V1 最小版**：

```cpp
enum class PlaybackState {
    Idle,
    Ready,
    Playing,
    Paused,
    Seeking,
    Ended,
    Error
};
```

### V1 为什么够用

它已经解决了当前最大的 4 个问题：

- 区分 `Idle` 和 `Ready`
- 区分 `Stopped` 和 `Ended`
- 给 `seek()` 一个明确过程态
- 给错误流程一个明确着陆点

### V1 暂时可以不显式建模的

下面两项先用动作过程处理即可，不必先上状态：

- `Opening`
- `Stopping`

也就是说：

- `open()` 成功后直接进入 `Ready`
- `stop()` 完成后直接进入 `Ready`

等 V1 跑稳，再上完整版本。

---

## 14. 一张贴近当前代码的“小图”

如果只用一张图来帮助你记忆，我建议记这张：

```text
[Idle]
  open
    v
[Ready] <------ stop ------ [Playing] ------ pause ------> [Paused]
   ^  ^                         |   ^                          |
   |  |                         |   |                          |
   |  +------ seek ------------+   +----------- play ---------+
   |                            
   |                             \ eof drained
   |                              v
   +----------- close ---------- [Ended]

seek from Ready / Playing / Paused / Ended
   --> [Seeking] --> 回到原目标状态

fatal error from any loaded state
   --> [Error] -- close/open --> [Idle/Ready]
```

---

## 15. 一句话结论

对当前项目来说，最值得做的不是把状态机搞得很复杂，而是先把当前这个过载的 `Stopped` 拆开。

最核心的演进方向是：

> **把当前 `Stopped/Playing/Paused` 的三态，演进成 `Idle/Ready/Playing/Paused/Seeking/Ended/Error` 的工程化状态机。**

这样既贴合你当前 `PlayerCore` 的结构，又能为后续增强、自动播放下一项、跨平台渲染 fallback、错误恢复留出干净空间。
