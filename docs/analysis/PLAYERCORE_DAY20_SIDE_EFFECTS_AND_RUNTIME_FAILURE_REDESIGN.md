# Day20 结论：把 `PlayerCore` 的副作用和 runtime failure/recovery policy 收口到统一入口

日期：2026-03-20  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`、`include/core/scheduler.h`、`src/core/scheduler.cpp`

## implementation planner

1. 先复核上一轮 `queue generation + SchedulerControlSnapshot` 已经提供了哪些基础能力，确认这轮不需要回退重做 timeline serial 或 EOF/Ended。
2. 盘点 `play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 里仍然混杂的线程、设备、队列、时钟副作用。
3. 把 stopping 路径收口成统一的 `apply*SideEffects()`，让入口函数只负责状态迁移和发布，不再散点操纵 worker/device。
4. 扩 `SchedulerControlSnapshot`，继续把 `clock_source`、audio-master 约束和 ended policy 结构化，而不是再加新的零散布尔位。
5. 统一 runtime failure/recovery policy，让 decode/resample/output 失败不再只是各处 `emitError + return false`。
6. 最后同步 records 文档并重新确认 Debug 构建通过。

## 当前问题

### 1. 状态迁移已经集中，但副作用还没真正集中

前几轮已经把 `SessionState / RunState / PipelinePhase` 和 transition 入口落下来了，但 `play()`、`pause()`、`stop()`、`seek()`、`close()` 仍然夹杂着：

- 线程起停
- packet/frame queue 清理
- demux rewind
- `AudioPlayer` pause/stop/close
- `Clock` reset/pause/resume

这会带来两个问题：

- 状态迁移和执行动作仍然耦合，入口函数读起来像半个状态机、半个 worker 管理器。
- `stop` / `deferred stop` / `runtime failure` 容易各自维护一套近似但不完全相同的停机逻辑。

### 2. `deferred stop` 还像旁路，而不是统一 stopping 路径的一部分

`deferred stop` 的根本原因并不是业务上要有“第四套 stop 语义”，而是为了避免 worker 线程在自己的调用栈里直接 join 自己。  
因此真正应该统一的是：

- stop request side effects
- stop completion side effects

`requestDeferredStop()` / `serviceDeferredStop()` 只应成为“异步完成 stop 的入口”，而不应继续拥有独立业务语义。

### 3. `SchedulerControlSnapshot` 仍然过于最小

上一轮已经让 scheduler 看到了：

- `run_state`
- `pipeline_phase`
- `accepted_timeline_serial`

但 render wait / audio-master pacing 实际还需要更明确的约束来源，否则 scheduler 仍会自己从外部环境猜业务语义。最直接缺的就是：

- `clock_source`
- audio output 是否真的初始化
- 当前是否处于 audio-master sync
- `Ended` 时是停输出还是保最后一帧

### 4. runtime failure 还没有统一 recovery policy

`decodeVideoFrame()`、`decodeAudioFrame()` 里已经有很多 fatal 点：

- `prepareVideoOutputFrame` 失败
- `avcodec_send_packet / receive_frame` fatal
- audio resampler 初始化或转换失败
- 音频 buffer 分配失败

如果每个点都自己决定“只报错”还是“停播”，最后一定会重新长出分散的恢复路径。  
所以这轮要先统一策略对象，而不是继续堆新的 `emitError + return false`。

## 这轮方案

### 1. 引入统一 side-effects helper

`PlayerCore` 新增：

- `applyStartPlaybackSideEffects()`
- `applyResumePlaybackSideEffects()`
- `applyPausePlaybackSideEffects()`
- `applyStopRequestSideEffects()`
- `applyStopCompletionSideEffects()`
- `applySeekSideEffects()`
- `applySessionReleaseSideEffects()`

入口函数的职责变成：

- 做合法状态迁移
- 选择合适的 helper
- 发布对外 `PlaybackState`

具体动作则统一落到 helper 内部。

### 2. 把 `deferred stop` 并回统一 stopping 路径

这轮没有硬删 `deferred stop`，但已经把它变成 stop helper 的异步入口：

- `requestDeferredStop()` 只负责：
  - `transitionRunState(Stopping)`
  - `transitionPipelinePhase(Flushing)`
  - `applyStopRequestSideEffects()`
  - 发布状态
- `serviceDeferredStop()` 只负责：
  - 检查 `deferred_stop_pending`
  - `applyStopCompletionSideEffects()`
  - 回到 `Stopped / Idle`
  - 发布状态

这样 stop request / completion 逻辑不再分叉维护。

### 3. 扩 `SchedulerControlSnapshot`

新增字段：

```cpp
ClockSource clock_source;
bool audio_output_initialized;
bool audio_master_sync_active;
SchedulerEndedPolicy ended_policy;
```

并且：

- `PlayerCore::makeSchedulerControlSnapshot()` 成为这些约束的唯一业务映射点。
- scheduler 的 render wait 逻辑改为消费 `clock_source` 和 `audio_master_sync_active`。
- `Ended` 下的 render 许可改为显式走 `ended_policy`，不再靠内部猜测。

这一步的重点不是一次性把 scheduler 做成完整业务状态机，而是先把“已经明确知道的语义”结构化，避免再长出零散布尔位。

### 4. 统一 runtime failure/recovery policy

新增：

```cpp
enum class FailureRecoveryPolicy {
    EmitOnly,
    StopPlayback,
    FailSession
};
```

以及统一入口：

```cpp
bool handleRuntimeFailure(ErrorCode code,
                          const char* message,
                          const char* reason,
                          FailureRecoveryPolicy policy);
```

当前策略分工是：

- `EmitOnly`
  - 只发错误，不改播放会话
  - 当前主要用于 seek 失败
- `StopPlayback`
  - 统一转入 `Stopping`
  - 走 stop-request side effects
  - 发布状态并发错误
  - 让主线程后续通过 `serviceDeferredStop()` 完成收尾
- `FailSession`
  - 预留给更重的 session 级失败
  - 本轮先完成统一入口，不提前做更大范围行为重写

### 5. 先覆盖最容易重新分叉的 runtime failure 点

这轮已经把以下 fatal 点收进统一 policy：

- 视频：
  - `prepareVideoOutputFrame()` 失败
  - `avcodec_receive_frame()` fatal
  - software offthread probe clone 失败 / 超时
  - `avcodec_send_packet()` fatal
- 音频：
  - `receive_frame()` fatal
  - `send_packet()` fatal
  - drain send fatal
  - `ensureAudioResampler()` 失败
  - `av_samples_alloc()` 失败
  - `swr_convert()` 失败
  - `byte_count <= 0`

## 结果

### 1. `play / pause / stop / seek / close` 更像状态机入口，而不是副作用集合

这些入口现在主要负责：

- 判断当前状态是否合法
- 触发 transition
- 调用统一 helper
- 发布对外状态

线程、设备、队列和时钟动作已经明显从入口函数里抽离出来。

### 2. `deferred stop` 不再是另一条独立停机语义

它现在只是 stop completion 的异步桥接层。  
真正的 stopping 语义已经收敛到：

- `applyStopRequestSideEffects()`
- `applyStopCompletionSideEffects()`

### 3. scheduler 对业务语义的依赖更显式

虽然这轮还没有做完整的 `SchedulerControlSnapshot` 终版，但 scheduler 已经不需要继续靠额外零散布尔位去猜：

- 当前时钟主源是谁
- audio master 是否真的有效
- `Ended` 是否允许保留最后一帧

### 4. runtime failure 后续可以继续扩，而不用重新拆入口

现在 decode/resample/output 的 fatal 点已经有统一 recovery policy。  
下一轮即使继续扩大覆盖范围，也是在 `handleRuntimeFailure()` 这个中心上扩，而不是再往 worker 里塞新的 ad-hoc 处理。

## 本轮故意没做的事

- 没把 EOF/Ended policy 进一步细化成自动连播、停留最后帧、结束复播策略。
- 没把 `SchedulerControlSnapshot` 扩到完整的 `clock_source + ended policy + audio-master mode + more recovery hints` 最终形态。
- 没把 `FailSession` 扩成更重的 worker 自治停机会话模型。
- 没重写 UI 层或外部 `PlaybackState`。

## 仍然保留的风险

### 1. `SchedulerControlSnapshot` 还不是终版

当前已经有：

- `run_state`
- `pipeline_phase`
- `accepted_timeline_serial`
- `clock_source`
- `audio_output_initialized`
- `audio_master_sync_active`
- `ended_policy`

但如果后续要把 ended 行为、audio-master 约束和 clock policy 做得更细，还需要继续扩结构，而不是回退去加旁路布尔位。

### 2. `FailSession` 仍然是预留策略

这轮先把统一入口和大部分 `StopPlayback` 场景打通了。  
如果后续真的开始大量使用 `FailSession`，需要继续核对 worker 线程上下文下的 stop/session release 安全边界。

### 3. queue generation 仍是容器边界中断，不替代 item-level serial

上一轮引入的 queue generation 解决的是：

- `clear()/flush()` 后让旧的阻塞 `push()/pop()` 及时醒来

它不能替代 packet/frame 自身的 timeline serial。  
真正的旧时间线硬失效，仍然必须继续以 item-level serial 为主判定。

## 本地验证

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 结果：通过，`0 warnings / 0 errors`

## 下一轮建议

下一轮如果继续沿这条线推进，优先顺序应该是：

1. 继续细化 `SchedulerControlSnapshot`
2. 把 `Stopping` / `Closing` side effects 进一步推进成更完整的 `apply*SideEffects()` 调度模型
3. 根据真正使用场景扩 `FailSession`
4. 再去收 tighter ended policy / audio-master 约束