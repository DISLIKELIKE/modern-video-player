# 播放主链路解读（第一版）

日期：2026-03-10
范围：`src/core/player_core.cpp`、`include/core/player_core.h`、`src/core/scheduler.cpp`、`include/core/scheduler.h`、`include/demuxer.h`、`include/core/frame.h`、`include/core/frame_queue.h`、`include/core/clock.h`、`src/audio_player.cpp`、`include/audio_player.h`、`src/video_player.cpp`、`include/video_player.h`

**目标**
梳理“打开媒体到首帧渲染”的主链路与线程/队列/时钟协作方式，形成可复用的阅读地图。

**总体流程（10 行版）**
1. `VideoPlayer::open()` 调用 `PlayerCore::open()` 打开媒体。
2. `PlayerCore::open()` 创建 `Demuxer`，探测流信息，初始化渲染器与音频设备，初始化解码器。
3. 为音视频创建 `PacketQueue`（大小 256），重置状态/时钟。
4. `VideoPlayer::play()` 调用 `PlayerCore::play()`。
5. `play()` 启动解封装线程、音频消费线程、调度器线程组。
6. 解封装线程持续读包，按音/视频流推入 `PacketQueue`。
7. 调度器启动视频/音频解码线程，把帧送入 `FrameQueue`（视频 16、音频 32）。
8. 调度器渲染线程按主时钟取视频帧，早到等待、晚到丢弃，调用 `renderFrame()`。
9. 音频消费线程从音频 `FrameQueue` 取帧，经滤镜后写入 `AudioPlayer`。
10. `pumpEvents()` 处理 UI/快捷键/Seek，必要时触发暂停/播放/跳转。

**数据流与队列**
```text
Demuxer.readPacket()
  -> PacketQueue(video/audio, cap=256)
     -> Scheduler video/audio decoder threads
        -> FrameQueue(video cap=16, audio cap=32)
           -> Scheduler render thread -> PlayerCore::renderFrame()
           -> Audio consumer thread   -> AudioPlayer::play() -> SDL audio callback
```

**线程模型（关键）**
- 解封装线程：`PlayerCore::startDemuxThread()`。
- 调度器线程：`Scheduler::start()` 启 3 线程（video/audio decode + render）。
- 音频消费线程：`PlayerCore::startAudioConsumer()`。
- SDL 音频回调线程：`AudioPlayer::audioCallback()`。

**同步与时钟**
- `Clock` 支持 `Audio` / `Video` / `System` 源。
- `PlayerCore::open()` 根据是否有音频设备选择 `ClockSource`。
- `Scheduler::pumpRenderOnce()` 取视频帧后与 `clock_->getTime()` 对齐。
- 早到等待：`diff > 0` 时最多等待 5ms；晚到丢弃：`diff < -0.25` 直接丢帧。
- 音频消费线程以播放 PTS 更新 `Clock` 与 `position_`。

**关键函数与职责**
- `PlayerCore::open()`：创建 `Demuxer`，初始化渲染器/音频/解码器，设置时钟与队列。
- `PlayerCore::play()`：启动解封装、调度器、音频消费线程。
- `Scheduler::videoDecoderLoop()`：从 `PacketQueue` 拉取包并解码为视频帧。
- `Scheduler::audioDecoderLoop()`：从 `PacketQueue` 解码为音频帧。
- `Scheduler::renderLoop()`：按时钟节拍渲染视频帧。
- `PlayerCore::renderFrame()`：滤镜处理、渲染、更新时钟与位置。
- `PlayerCore::startAudioConsumer()`：把音频帧送入 `AudioPlayer`。
- `AudioPlayer::audioCallback()`：SDL 线程实际输出音频、更新播放 PTS。
- `PlayerCore::pumpEvents()`：处理播放/暂停/Seek/字幕/AB 循环/截图等交互事件。

**关键阈值/参数（易影响体验）**
- `PacketQueue` 容量：256。
- `FrameQueue` 容量：视频 16、音频 32。
- 解码线程背压：`FrameQueue` 填充 >= 0.8 时暂缓；< 0.5 继续。
- 视频迟到丢帧：`diff < -0.25s`。
- 视频早到等待上限：`5ms`。
- 音频缓冲上限：`0.35s`。

**诊断计数与线程来源（谁在更新）**
- 解封装线程（`startDemuxThread()`）：
  - `demux_video_packets_ / demux_audio_packets_ / demux_push_retries_ / demux_dropped_packets_`
- 调度器视频解码线程（`Scheduler::videoDecoderLoop()`）：
  - `decode_video_ok_`（`decodeVideoFrame()` 成功时）
  - `scheduler.video_decoded_frames`
- 调度器音频解码线程（`Scheduler::audioDecoderLoop()`）：
  - `decode_audio_ok_`（`decodeAudioFrame()` 成功时）
  - `scheduler.audio_decoded_frames`
- 调度器渲染线程（`Scheduler::renderLoop()` / `pumpRenderOnce()`）：
  - `render_frames_`（`renderFrame()` 内）
  - `scheduler.rendered_frames / scheduler.dropped_late_frames / scheduler.wait_events`
- 音频消费线程（`startAudioConsumer()`）：
  - `audio_submitted_frames_`
- 额外说明：
  - `decodeVideoFrame()` 在“单帧步进/暂停渲染”路径也会被主线程调用，因此 `decode_video_ok_` 不是 100% 只来自调度器线程。
  - `maybeLogDiagnostics()` 在解封装、解码、渲染、音频消费等多线程路径都会被触发，用于每秒打印一次汇总。

**入口与主循环（应用层）**
- `src/main.cpp` 使用 `VideoPlayer` 包装 `PlayerCore`。
- 主循环示例：
  - `g_player->open(...)`
  - `g_player->play()`
  - `while (g_player->isPlaying() || g_player->isPaused()) { g_player->pumpEvents(); ... }`

**命令行播放路径（exe 后跟文件）**
1. `main()` 进入“播放模式”分支后调用 `parsePlaybackCliArgs(...)` 收集 `argv` 中的媒体输入（`src/main.cpp:760`、`src/main.cpp:3428`）。
2. `buildPlaylistFromInputs(...)` 把 `media_inputs` 转成播放列表（`src/main.cpp:792`、`src/main.cpp:3452`）。
3. 播放循环内依次调用 `g_player->open(item.uri)` 与 `g_player->play()`（`src/main.cpp:3482`、`src/main.cpp:3506`）。

**证据清单（定位用）**
- `src/core/player_core.cpp`：`open()`、`play()`、`startDemuxThread()`、`startAudioConsumer()`、`decodeVideoFrame()`、`decodeAudioFrame()`、`renderFrame()`、`pumpEvents()`。
- `src/core/scheduler.cpp`：`start()`、`videoDecoderLoop()`、`audioDecoderLoop()`、`pumpRenderOnce()`。
- `include/core/frame_queue.h`：队列容量与阻塞策略。
- `src/audio_player.cpp`：音频输出线程与 PTS 维护。
- `src/video_player.cpp`：应用层包装与回调。

**不确定点 / 需要补读的文件**
- `Demuxer` 的 `detectStreams()` 细节与封装策略。
- `render::VideoRenderer` 的 `handleEvents()` 具体事件映射。
- `filters::FilterPipeline` 对视频/音频链路的影响范围。

**下一步建议（从主链路继续深入）**
1. 补读 `src/demuxer.cpp`：确认 `MediaInfo` 的填充细节与 `readPacket()` 行为。
2. 补读 `render/*`：确认渲染后端（SDL/D3D11/OpenGL）差异。
3. 补读 `filters/*`：明确过滤链路插入点与性能开销。
