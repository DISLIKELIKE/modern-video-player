# Day16 结论：先把 `PlayerCore` 的 UI 播放态与内核会话/流水线态拆开，再继续做 serial 化

日期：2026-03-19  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`、`src/core/scheduler.cpp`、`include/core/scheduler.h`

## implementation planner

1. 先盘点 `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 里的状态改写点。
2. 把对外 `PlaybackState` 保持兼容，只在 `PlayerCore` 内部新增三层状态和统一快照。
3. 新增统一的状态迁移入口，收口散落的 `state_.store/exchange` 与 `deferred stop` 布尔位语义。
4. 保持当前线程模型和 `Scheduler` 契约基本不变，不在这一轮提前引入 serial、Ended 语义重写或新的 Scheduler 控制快照。
5. 最后重新跑 Debug 构建，验证第一阶段状态机重设计至少能稳定编译回主程序。

## 先给结论

- 当前仓库最大的问题不是 `PlaybackState` 状态太少本身，而是它同时承载了 UI 播放态、会话可用态、流水线过程态和 EOF/延迟 stop 旁路语义。
- `PlayerCore` 里原本的 `open/play/pause/stop/seek/onRenderIdle/requestDeferredStop/serviceDeferredStop` 都会直接写 `state_`，导致全局语义没有单一入口。
- `Scheduler` 目前仍然只有 `running_ / paused_` 两个控制位，所以第一阶段最稳妥的做法，是先把“播放器现在是什么态”的决定权收回到 `PlayerCore`，而不是提前把 `Scheduler` 也改成业务状态机。
- 这轮只做第一阶段：内部加三层状态、统一 transition、输出状态迁移日志、建立非法迁移保护；不引入 timeline serial，不把 EOF 改成 Ended。

## 当前问题拆解

### 1. 对外 `PlaybackState` 被迫承载了多种不同语义

当前对外仍只有：

```cpp
enum class PlaybackState {
    Stopped,
    Playing,
    Paused
};
```

但代码里实际已经隐含了下面几类状态：

- 会话层：是否已打开、是否正在关闭、是否已失败
- 运行层：停止、启动中、运行中、暂停中、已暂停、停止中
- 流水线层：空闲、正常、seek 中、flush 中、EOF drain 中
- 旁路标志：`deferred stop`、`demuxer_->isEof()`、本次 seek 是否未完成

这些语义压在一个枚举里，后续一旦继续演进 seek/EOF/恢复路径，逻辑就会越来越散。

### 2. 状态迁移点过于分散

第一阶段改造前，`PlayerCore` 里最关键的状态改写点分散在：

- `open()`：直接把 `state_` 写回 `Stopped`
- `play()`：从 `Paused` 直写 `Playing`，或在线程启动后直写 `Playing`
- `pause()`：直写 `Paused`
- `stop()`：直写或 `exchange` 回 `Stopped`
- `serviceDeferredStop()`：再次直写 `Stopped`
- `onRenderIdle()`：EOF 时再次 `exchange` 到 `Stopped`

这导致两个直接后果：

- 想知道“哪个入口真正改变了播放器语义”很困难
- 想加非法迁移保护和统一日志时，没有可靠的收口点

### 3. `deferred stop` 之前是旁路状态

`deferred stop` 之前是独立的原子布尔语义：

- `requestDeferredStop()` 负责置位
- `serviceDeferredStop()` 负责清位
- 但这个标志并不受统一状态机管理

这意味着：

- 全局语义一部分在 `state_`
- 另一部分在 deferred stop 标志
- 状态观察和调试时必须靠人脑把两套语义拼起来

### 4. `Scheduler` 仍然是“执行器”，不是“状态机”

这轮分析确认了 `Scheduler` 当前只有：

- `running_`
- `paused_`

它能控制：

- video decode loop
- audio decode loop
- render loop

但它并不知道：

- 会话是否 `Ready / Closing / Failed`
- 流水线是否处于 `Seeking / Flushing / Draining`
- deferred stop 是否已经挂起

因此第一阶段不适合把业务语义继续往 `Scheduler` 里塞，正确方向是先让它继续做线程执行器，业务状态由 `PlayerCore` 统一维护。

## 第一阶段落地方案

### 1. 内部新增三层状态和统一快照

在 `PlayerCore` 内部新增：

```cpp
enum class SessionState { Closed, Opening, Ready, Closing, Failed };
enum class RunState { Stopped, Starting, Running, Pausing, Paused, Stopping, Ended };
enum class PipelinePhase { Idle, Normal, Seeking, Draining, Flushing };
```

并新增：

```cpp
struct CoreStateSnapshot {
    SessionState session_state;
    RunState run_state;
    PipelinePhase pipeline_phase;
    bool eof_reached;
    bool pending_seek;
    bool deferred_stop_pending;
};
```

注意：

- 这轮没有引入 `timeline_serial`
- `RunState::Ended` 先只占位，不提前重写 EOF 行为
- 对外 `PlaybackState` 保持原样

### 2. 新增统一状态迁移入口

新增的内部入口包括：

- `transitionSessionState(...)`
- `transitionRunState(...)`
- `transitionPipelinePhase(...)`
- `setEofReached(...)`
- `setPendingSeek(...)`
- `setDeferredStopPending(...)`
- `consumeDeferredStopPending(...)`
- `publishPlaybackStateFromInternalState(...)`

这些入口负责两件事：

1. 收口内部状态变更
2. 输出结构化的迁移日志，并在非法迁移时记录 warning

### 3. 对外 `PlaybackState` 变成内部状态的投影

第一阶段采用的投影规则是：

- `SessionState != Ready` 时，对外一律投影为 `Stopped`
- `RunState == Starting || RunState == Running` 时，对外投影为 `Playing`
- `RunState == Pausing || RunState == Paused` 时，对外投影为 `Paused`
- `RunState == Stopped || Stopping || Ended` 时，对外投影为 `Stopped`

因此：

- UI 侧接口不需要改
- `VideoPlayer` 继续只关心 `PlaybackState`
- 内核内部可以开始表达更细的状态语义

### 4. 第一阶段只收口状态，不重写线程契约

这轮没有把线程生命周期彻底改成统一 side-effect dispatcher，只做了更稳妥的第一步：

- `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle`
  改成通过 transition helper 改内部状态
- 原有的线程启停、flush、clock reset、audio pause/resume 仍保留在各自入口中

这样做的目的，是先把“状态权威来源”统一，再进行第二阶段 serial 和第三阶段 EOF/Ended 语义拆分。

## 这一轮具体改了什么

### `PlayerCore` 状态模型

- 新增 `SessionState / RunState / PipelinePhase`
- 新增 `CoreStateSnapshot`
- 移除独立的 deferred stop 原子字段，改为纳入 `CoreStateSnapshot`
- 保留对外 `std::atomic<PlaybackState> state_`，但只允许 `publishPlaybackStateFromInternalState()` 更新

### 状态迁移日志与非法迁移保护

- 每次 `session/run/pipeline` 迁移都会打印：
  - `from -> to`
  - `reason`
- 非法迁移不会静默通过，而是打 `LOG_WARNING`
- 对外 `PlaybackState` 变化也会单独打印日志，并附带当前内部快照

### 迁移点收口

下列入口已不再直接写 `state_`：

- `open()`
- `close()`
- `play()`
- `pause()`
- `stop()`
- `seek()`
- `requestDeferredStop()`
- `serviceDeferredStop()`
- `onRenderIdle()`

同时：

- demux 线程启动时会重置 `eof_reached`
- demux 读到 EOF 时会把 `eof_reached` 纳入内部状态快照

## 本轮对 `Scheduler` 的处理边界

这轮分析后的结论是：`Scheduler` 先不改业务语义接口。

原因：

- 它当前仍只是执行 video/audio decode 与 render loop 的调度器
- 第一阶段的核心问题是 `PlayerCore` 内部语义分裂，而不是 `Scheduler` 不够复杂
- 如果在还没有统一 `PlayerCore` 状态入口前，就把 `Seeking / Draining / Failed` 等语义提前塞给 `Scheduler`，只会扩大改动面

因此第一阶段对 `Scheduler` 的结论是：

- 保持 `running_ / paused_` 契约不变
- 继续由 `PlayerCore` 决定何时 `start / pause / resume / stop / flush`
- 下一阶段再考虑 `SchedulerControlSnapshot`

## 本地验证

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 结果：通过

## 这轮故意没做的事

- 没有引入 `timeline_serial`
- 没有给 packet/frame 加 serial
- 没有把 EOF 正式改成 `RunState::Ended`
- 没有把 `Scheduler` 升级为消费 `run_state / pipeline_phase` 的控制快照
- 没有重写 copy-back、SoftwareSDL 或其它渲染路径

## 下一轮建议

下一轮应进入“第二阶段 serial 化”：

1. 为 packet/frame 引入 timeline serial
2. seek/flush/stop/reopen 改成 serial + flush 双保险
3. decode/render/audio consumer 全链路按 serial 丢弃旧时间线数据

在那之后，再进入：

1. `EOF -> Draining -> Ended`
2. `Ended` 下的复播策略
3. `SchedulerControlSnapshot`
