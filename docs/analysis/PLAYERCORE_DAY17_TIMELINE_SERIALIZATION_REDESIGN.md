# Day17 结论：用 timeline serial 把 `seek/flush` 从“软清理”补成“硬失效”

日期：2026-03-20  
范围：`include/core/frame.h`、`src/core/frame.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 先复核第一阶段是否已落地，确认 `SessionState / RunState / PipelinePhase`、统一 transition 入口和 `publishPlaybackStateFromInternalState()` 都已经存在。
2. 盘点当前 `seek/flush` 路径里旧时间线泄漏的位置，明确哪些地方还在依赖“清队列成功了所以应该没旧帧”。
3. 为 packet/frame 增加 timeline serial 元数据，并把 serial 透传到 demux、decode、render、audio consumer 整条链路。
4. 在 `PlayerCore` 内部增加 `timeline_serial / pending_seek_serial` 及统一分配、激活、判定入口。
5. 保留 `flush`，但把它降级为辅助清扫；真正的硬边界改由 serial 决定。
6. 最后补 diagnostics / logs / records，并重新跑 Debug 构建。

## 先给结论

- 第一阶段已经落地，所以第二阶段可以直接在统一状态机之上继续做 serial 化，而不需要回退去补基础设施。
- 当前 `seek` 的真实问题，不是“flush 不够多”，而是 packet/frame 不带时间线身份，导致即使做了 `pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop() + 二次 flush`，旧数据仍然只能靠“碰巧被清空”来失效。
- 第二阶段的核心改动，是把“旧时间线失效”从队列副作用改成显式 serial 语义：packet 入队时绑定 serial，decoded `VideoFrame / AudioFrame` 继承 serial，render / audio consumer / decoder 收到旧 serial 立即丢弃。
- 本轮仍然不改 EOF -> Ended，不引入 `SchedulerControlSnapshot`，也不动 copy-back、SoftwareSDL、UI 层和外部 `PlaybackState`。

## 当前缺口

### 1. `ThreadSafeQueue` 只有 stop/start/eof/clear，没有时间线语义

当前包队列只能解决“物理上把队列清空”或“让生产/消费停下来”，但不能表达“这个元素属于哪一条时间线”。一旦旧 packet 在 seek 边界前已经离开队列，单靠 queue 本身就再也无法判定它是否过期。

### 2. `FrameQueue` 只有 `flush()`，没有 epoch / serial

视频帧队列和音频帧队列此前只知道 push、pop、flush。若 seek 前旧帧已经进入队列，但 flush 时机稍晚，消费端仍可能拿到旧帧；若某个 worker 在 flush 之后又晚到地吐出旧帧，队列本身也无法硬判定它过期。

### 3. seek 路径此前主要靠“软清理”

改造前 seek 的主要路径是：

1. `scheduler.pause()`
2. `stopDemuxThread()`
3. `flushPipelines()`
4. `demuxer_->seek(timestamp)`
5. `avcodec_flush_buffers()`
6. `audio_player_->stop()`
7. 再做一次 `flushPipelines()`

这些动作都很有必要，但它们只是在努力“把旧东西冲掉”。缺的那一层，是“即使旧东西穿过边界，也一定会被判定为废数据”的硬规则。

### 4. audio consumer 线程没有靠时间线失效旧音频

这条线程 seek 时不会自然退出，之前更多依赖 `audio_queue_.flush()` 和 `audio_player_->stop()` 来让旧音频失效。如果 seek 边界附近还有旧 `AudioFrame` 晚到，消费端并不知道它是否属于旧时间线。

### 5. render 路径此前也没有 serial 防线

渲染侧如果拿到 seek 前残帧，之前只能依赖队列刚好已经被 flush，或 scheduler 没有把旧帧继续送过来。这会让“暂停态 seek 后残帧误显示”和“连续 seek 残留旧画面”一直处于软约束状态。

## 第二阶段方案

### 1. 统一时间线类型

新增：

```cpp
using TimelineSerial = uint64_t;
constexpr TimelineSerial kInvalidTimelineSerial = 0;
```

### 2. packet/frame 改成携带 serial

本轮没有给 `ThreadSafeQueue` 本体塞 generation，而是选择 item-level serial：

```cpp
struct DemuxPacket {
    PacketPtr packet;
    TimelineSerial serial;
};
```

并且：

- `VideoFrame` 新增 `serial`
- `AudioFrame` 新增 `serial`

这样 queue 仍保持通用容器，不被播放器业务语义绑死；真正的时间线语义跟着 packet/frame 走，跨线程、跨队列都不会丢。

### 3. `PlayerCore` 成为 serial 权威来源

内部新增：

- `timeline_serial`
- `pending_seek_serial`

以及统一入口：

- `allocateNextTimelineSerial(...)`
- `activateTimelineSerial(...)`
- `setPendingSeekSerial(...)`
- `clearPendingSeekSerial(...)`
- `currentTimelineSerial()`
- `acceptedTimelineSerial()`
- `isCurrentTimelineSerial(...)`
- `isAcceptedTimelineSerial(...)`

关键约束是：不允许 `seek / stop / open / deferred stop` 各处自己随手 `++`；所有 serial 分配和激活都必须收口到 helper。

### 4. `pending_seek_serial` 先行切换“接受边界”

这轮最关键的一点，是 seek 不是等到完成后才产生新时间线概念，而是：

1. seek 开始时先分配 `pending_seek_serial`
2. `acceptedTimelineSerial()` 在 pending 存在时优先返回 pending
3. 因此旧 serial 从 seek 开始那一刻起，就已经不再被消费端接受
4. seek 成功后再 `activateTimelineSerial(pending_seek_serial, ...)`
5. seek 失败则清掉 pending，恢复接受旧的 current serial

### 5. demux 线程在启动时捕获 serial

`startDemuxThread()` 不再每次 push 时读全局 current serial，而是在启动线程时就捕获：

```cpp
const TimelineSerial demux_serial = currentTimelineSerial();
```

然后所有入队 packet 都带这个 captured serial。这样能避免“旧 demux 线程还在收尾，但外部已经推进了新 serial，导致旧 packet 被错标成新时间线”的竞态。

## 链路接法

### demux -> packet queue

- video/audio packet 入队时改为 `DemuxPacket{packet, demux_serial}`
- queue 本身不拥有 epoch，但每个元素都带了 serial 身份

### decodeVideoFrame()

- 出队时先检查 `DemuxPacket.serial`
- 旧 serial packet 直接丢弃，不再送进 codec
- `avcodec_send_packet()` 成功后记录 `video_decoder_packet_serial_`
- `avcodec_receive_frame()` 成功后，用 `video_decoder_packet_serial_` 作为 decoded frame 的 serial
- 若 decoded frame 的 serial 已过期，则直接丢弃，不再进入 `VideoFrame` 输出

### decodeAudioFrame()

- 逻辑与视频一致
- 旧 serial packet 不再送 codec
- decoded audio frame 继承 `audio_decoder_packet_serial_`
- 过期音频帧直接丢弃，不再形成可提交的 `AudioFrame`

### renderFrame()

- `VideoFrame` 到达渲染入口后，先做 `isAcceptedTimelineSerial(frame.serial)` 判定
- 旧 serial 视频帧直接返回
- 不更新 `position_`、`clock_`、overlay、`last_rendered_frame_` 和 screenshot cache

### audio consumer

- `AudioFrame` 出队后先检查 serial
- 旧 serial 音频帧直接丢弃，不 submit 到 `AudioPlayer`
- `audio_output_serial_` 只在真正提交当前可接受 serial 的音频后更新
- 音频主时钟回写也需要 `audio_output_serial_` 仍被接受，避免旧设备播放进度污染 seek 后的位置

### paused seek / frame step

- `renderPausedFrameAtOrAfter()` 现在也会检查 `frame.serial`
- 这样即使暂停态下 seek 前的残帧仍滞留在 frame queue 中，也不会被误显示成 seek 后结果

## 这轮如何处理 `flush`

`flush` 还保留，而且仍然必要，因为它能减少无意义旧数据堆积、缩短 seek/stop 后恢复时间，并清扫 packet/frame queue 和 scheduler backlog。但语义已经改变：以前 `flush` 是唯一失效机制，现在 `flush` 只是辅助清扫，真正的硬失效由 serial 判定负责。

## 诊断与观测

`CoreStateSnapshot` 与 `DiagnosticsSnapshot` 现在都带：

- `timeline_serial`
- `pending_seek_serial`

日志输出也加入 serial 观测：状态投影日志会带 timeline / pending seek serial，diagnostics 日志会打印 `current / accepted / pending`，`main.cpp` 的专项检查命令会输出这两个 serial 字段。

## 本轮故意没做的事

- 没把 EOF 改成 `Ended`
- 没正式引入 `SchedulerControlSnapshot`
- 没重写 `Scheduler` 成业务状态机消费者
- 没改 copy-back、SoftwareSDL、UI 层或外部 `PlaybackState`
- 没给 `ThreadSafeQueue` / `FrameQueue` 本体做播放器专用 generation 字段

## 残余风险

### 1. queue 容器本体仍然是通用容器

当前的硬失效已经成立，因为消费边界和 item 本身都带 serial。但 `ThreadSafeQueue` / `FrameQueue` 本体仍不知道 epoch，后续如果需要更强的“按 serial 定向清理”能力，可以再考虑第三阶段之后是否要做容器侧辅助 API。

### 2. `Scheduler` 统计仍可能轻微高估 render 尝试

`renderFrame()` 现在会丢弃 stale frame，但 `Scheduler` 本身还没有消费更细粒度的业务状态快照。因此在 seek/stop 边界附近，某些“render 尝试次数”类统计可能仍比真正呈现到屏幕的帧数略大。

### 3. close/reopen 的更严格语义仍可在后续继续收紧

本轮重点是 seek/stop/deferred stop 的时间线硬边界。`close()` 继续依赖 stop 路径与 reopen 后的新 session serial 来隔离旧工作；若下一阶段要继续把 session 生命周期做得更严格，可以再把 `close / reopen` 的 serial 语义进一步显式化。

## 本地验证

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 结果：通过

## 下一轮建议

第三阶段应聚焦：

1. `EOF -> Draining -> Ended` 的行为重写
2. `Ended` 下的复播策略
3. 更明确的 `Stopping / Closing` side-effect dispatcher
4. `SchedulerControlSnapshot`

到那一轮时，seek/flush 的硬边界已经由 serial 先稳住，EOF/Ended 语义就不会再和残帧残音问题缠在一起。
