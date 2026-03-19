# Day8 结论：高码率/4K 的首要优化点不是大重构，而是队列容量、硬解 surface 预算、video-master 节流和 copy-back 可观测性

日期：2026-03-19  
范围：`include/core/frame_queue.h`、`include/core/player_core.h`、`include/core/scheduler.h`、`src/core/player_core.cpp`、`src/core/scheduler.cpp`、`src/main.cpp`

## implementation planner

1. 先把 `FrameQueue(16/32)`、`Scheduler(0.8 -> 0.5)` 和 `prepareVideoOutputFrame()` 的现状量化出来，避免继续凭感觉调参。
2. 增加 frame queue 峰值/timeout、scheduler 背压等待时长、copy-back/swscale 时间统计，并接入 `--performance-log-check`。
3. 按媒体属性自适应队列容量，但对 `D3D11VA` 路径额外考虑硬解 surface pool，不允许“只靠放大队列”制造新的硬件帧池耗尽。
4. 修正 `Video` master 下的渲染节流与 late-frame catch-up，避免无音频输出时 24fps/30fps 样本超速播放，或 4K 音频主时钟下落后后追帧过慢。
5. 重新验证 `--performance-log-check`、`--high-bitrate-check`、`--4k-playback-check`、`--long-playback-check`。

## 结论

- 当前仓库在 `D3D11 + D3D11VA` 主链上仍然优先命中 native zero-copy：
  - 4K 性能采样里 `video_native_output_frames=101`
  - `video_copy_back_frames=0`
  - `video_swscale_frames=0`
- 因此这轮高码率/4K 抖动的主要问题，不是 `av_hwframe_transfer_data()` 或 `Display::copyFrameData()` 已经成为当前主路径瓶颈，而是：
  - 队列容量和背压阈值过于静态
  - 硬解 surface pool 预算没有跟 frame queue 协同
  - `Video` master 的节流逻辑不足
  - render loop 落后后一次只丢一帧，catch-up 太慢

## 本轮落地

### 1. FrameQueue 诊断增强

- `FrameQueue` 新增：
  - `peak_size`
  - `push_timeout_count`
  - `getStats() / resetStats()`
- 这样可以直接判断：
  - 队列到底有没有顶满
  - 是否真的因为 frame queue push 超时而背压失败

### 2. 自适应 frame queue 容量

- `PlayerCore::open()` 在 decoder 初始化完成后，按媒体属性配置 frame queue：
  - 视频队列：`16 ~ 24`
  - 音频队列：`32 ~ 48`
- 规则参考了成熟播放器常见的“按 buffered duration 而不是固定帧数”思路，但保留了当前工程可接受的内存上限。
- 同时对 `D3D11VA` 路径增加硬件感知限制，不再把 4K60 native path 的视频队列无限拉高。

### 3. D3D11VA surface pool 预算补强

- 仅把视频队列从 `16` 加到更大值会在 4K native path 下打到 FFmpeg 的固定硬件帧池。
- 这轮新增：
  - `video_codec_ctx_->extra_hw_frames`
  - `planned video queue capacity` 对应的硬解余量日志
- 实际结果是：
  - 不再出现 `Static surface pool size exceeded`
  - 4K `D3D11VA` 主链保持稳定通过

### 4. Scheduler 背压迟滞与等待统计

- `Scheduler` 背压阈值从单点 `0.8` 改成 enter/exit hysteresis：
  - video: `0.92 -> 0.65`
  - audio: `0.94 -> 0.70`
- 并新增：
  - `video_backpressure_wait_ms`
  - `audio_backpressure_wait_ms`
- 这让“经常进入背压”与“只是 decoder 正常跑得比 render 快”能被直接区分。

### 5. Video master 节流和 late-frame catch-up

- `Video` master 下不再只用固定 `5ms` wait cap。
- 改为按“上一帧真实呈现完成时间 + 当前 frame.duration”做 wall-clock pacing。
- render loop 现在会在一次 `pumpRenderOnce()` 内连续丢弃过期帧，直到拿到可显示帧，而不是一次只丢一帧。
- 这两个修复分别解决了：
  - 无音频输出时 24fps/30fps 样本超速推进
  - 4K 音频主时钟/高压力场景下追帧过慢导致的堆积

### 6. performance-log-check 扩展

- 新增输出：
  - `video_copy_back_time_ms / ratio_percent / avg_ms / max_us`
  - `video_swscale_time_ms / ratio_percent / avg_ms / max_us`
  - `scheduler_video_backpressure_wait_ms / scheduler_audio_backpressure_wait_ms`
  - `video_frame_queue_capacity / peak_size / push_timeouts`
  - `audio_frame_queue_capacity / peak_size / push_timeouts`

## 本轮验证

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.vcxproj `
  /t:Build /p:Configuration=Debug /p:Platform=x64 /m

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500
.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000
.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000
.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000
```

结果：

- `MSBuild`: PASS
- `performance-log-check`: PASS
- `high-bitrate-check`: PASS
- `4k-playback-check`: PASS
- `long-playback-check`: PASS

关键观察：

- 4K 性能采样：
  - `renderer_backend=D3D11`
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=101`
  - `video_copy_back_frames=0`
  - `video_swscale_frames=0`
  - `video_frame_queue_capacity=24`
  - `video_frame_queue_peak_size=23`
  - `video_frame_queue_push_timeouts=0`
  - `scheduler_video_backpressure_wait_ms=1252`
- 说明：
  - 当前 4K 主链仍是 native path
  - copy-back / swscale 不是这台机器上的当前主瓶颈
  - decoder 的确经常因为 queue 已接近满水位而等待，但没有形成真正的 queue drop

## 对问题 6~10 的当前判断

6. `FrameQueue 16/32` 在高码率素材上偏保守，已经改成自适应；但硬解路径不能只靠放大视频队列，否则会打爆硬件帧池。  
7. `decodeVideoFrame()` 的 receive-first 这轮不再是主要风险点，当前更关键的是队列和节流策略。  
8. `Display::copyFrameData()` 仍是 `SoftwareSDL` 路径的 memcpy 热点，但当前 D3D11 主链不经过这段代码，不是本轮 4K 现象的主因。  
9. 当前硬解路径下 `av_hwframe_transfer_data()` 不是主要瓶颈，因为采样中根本没有命中 copy-back。  
10. `Scheduler` 的一次只丢一帧/固定小步等待比“重启次数”更像当前真实问题；这轮已优先修正追帧和 video-master pacing。  

## 剩余风险

- 当前环境的音频设备初始化结果不稳定，有时会进入 `video_only_fallback`，有时会进入 `Audio` master。
- 这轮已经把 `Video` master 的超速和 queue/surface blind spot 收口，但如果后续要继续打磨 4K + 音频设备稳定场景，下一步建议优先做：
  1. audio master 下的 render lateness 容忍窗口和 catch-up 策略复核
  2. demux / frame queue serial 语义补强
  3. `SoftwareSDL` 路径的 `Display::copyFrameData()` 定时统计与替换策略
