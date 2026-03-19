# Day10 结论：主链继续保留 D3D11 zero-copy，`SoftwareSDL` 回退链通过 `NV12` 直传和 `AVFrame` 引用已去掉 `swscale + display memcpy`，`audio-master` 改为分段等待并动态 late-drop

日期：2026-03-19  
范围：`include/render/video_renderer.h`、`include/render/sdl_video_renderer.h`、`src/render/sdl_video_renderer.cpp`、`include/display.h`、`src/display.cpp`、`src/core/player_core.cpp`、`src/core/scheduler.cpp`

## implementation planner

1. 保持默认 `D3D11 + D3D11VA` 主链不动，只在 `SoftwareSDL` 回退链做有限重构，避免把 zero-copy 主链拖回软件路径。
2. 给 renderer 增加“可直接消费的软件像素格式”能力，让 `PlayerCore::prepareVideoOutputFrame()` 在 copy-back 后优先直通 `NV12/YUV420P`，没开视频滤镜时不再强制 `swscale -> YUV420P`。
3. 把 `Display` 从“永远深拷贝三平面”改成“优先持有 `AVFrame` 引用，必要时再复制”，并补 `SDL_UpdateNVTexture()`，让 `SoftwareSDL` 真正能吃 `NV12`。
4. 收紧 `audio-master` 的 render pacing：
   - 正向 diff 走分段等待并重复读 master clock
   - 负向 diff 不再固定 `-250ms` 才丢帧，而是按 `frame.duration + queue fill ratio` 动态决定 late-drop 窗口
   - 小于 `1ms` 的残余等待直接并入 slack，避免伪忙等
5. 分别验证默认主链、强制 `SoftwareSDL` 回退链，以及 `SDL_AUDIODRIVER=dummy` 下的 `Audio` master 路径。

## 设计取向

- 这轮没有做大重构，思路更接近 mpv / ffplay / MPC-HC 常见做法：
  - 主链能 native 就继续 native
  - fallback 链优先减少格式归一化和中间深拷贝次数
  - audio master 不靠单次粗睡眠，而是小步重检时钟
- 因此当前结论仍然是：
  - 默认 Windows 主链不需要推倒重来
  - `SoftwareSDL` 只做“少一次 swscale、少一次 display memcpy”的有限重构就已经有明显收益

## 本轮落地

### 1. `SoftwareSDL` 直通软件格式

- `IVideoRenderer` 新增 `supportsDirectFrameFormat(AVPixelFormat)`。
- `SdlVideoRenderer` 明确声明支持：
  - `AV_PIX_FMT_YUV420P`
  - `AV_PIX_FMT_NV12`
- `PlayerCore::prepareVideoOutputFrame()` 现在会在以下条件下直接保留软件帧：
  - 已经做完 `av_hwframe_transfer_data()` 或本身就是软件帧
  - renderer 支持该软件格式
  - 没有启用视频滤镜
- 这意味着 `D3D11VA -> av_hwframe_transfer_data() -> NV12 -> SoftwareSDL` 现在可以直接进入显示层，不再强制 `sws_scale()`。

### 2. `Display` 减少深拷贝次数

- `Display::PendingVideoFrame` 现在可以持有 `AVFrame` 引用，而不是只能存 `y/u/v` 深拷贝向量。
- `Display::copyFrameData()` 新策略：
  - 若帧是 `YUV420P/NV12` 且 stride 为正，直接 `av_frame_ref()` 保留引用
  - 若遇到负 stride 或不适配布局，再回退到深拷贝
- SDL texture 现已支持：
  - `SDL_PIXELFORMAT_IYUV`
  - `SDL_PIXELFORMAT_NV12`
- 上传路径现已分流：
  - `YUV420P -> SDL_UpdateYUVTexture`
  - `NV12 -> SDL_UpdateNVTexture`
- `renderLoop()` 也去掉了 `last_frame = frame_to_render` 的隐式像素复制，上一帧缓存只保留所有权，不再复制整块图像数据。

### 3. `audio-master` lateness / catch-up 收紧

- `Scheduler::pumpRenderOnce()` 在 `Audio` master 下不再是“最多睡 5ms 然后直接 render”。
- 新逻辑改成：
  - 以 `1ms~4ms` 的窗口分段等待
  - 每次等待后重新读取 `clock_->getTime()`
  - 若只剩小于 `1ms` 的残余差值，直接视为可接受 slack
- late drop 阈值现在改成动态值：
  - 基础来自 `frame.duration`
  - queue 越满，阈值越紧
  - queue 越空，阈值越宽
- 默认 `Video` master 仍保留原来的 wall-clock pacing，不额外扩大改动面。

## 验证结果

### A. 默认主链：`D3D11 + D3D11VA`

命令：

```powershell
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500
```

关键结果：

- `renderer_backend=D3D11`
- `decoder_backend=D3D11VA`
- `video_copy_back_ratio_percent=0`
- `video_swscale_ratio_percent=0`
- `display_copy_ratio_percent=0`
- `scheduler_video_restart_limit_hits=0`

结论：

- 默认主链没有回退。
- 这轮修改没有破坏 zero-copy 主链。

### B. 强制软件回退链：`SoftwareSDL + D3D11VA`

命令：

```powershell
$env:MVP_RENDERER_BACKEND='software'
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200
```

关键结果：

- `renderer_backend=SoftwareSDL`
- `decoder_backend=D3D11VA`
- `video_copy_back_ratio_percent=46.4067`
- `video_swscale_ratio_percent=0`
- `display_copy_ratio_percent=0`
- `display_copy_frames=0`

结论：

- 这条链现在的主要成本已经收敛到 `av_hwframe_transfer_data()`。
- 旧的 `swscale + Display::copyFrameData()` 两段热点已被当前有限重构基本消掉。
- 因此软件回退链暂时不需要大重构，下一阶段若继续优化，重点应该是 copy-back 本身，而不是继续抠 `Display::copyFrameData()`。

### C. `Audio` master 路径验证（dummy audio）

命令：

```powershell
$env:SDL_AUDIODRIVER='dummy'
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500
```

关键结果：

- `audio_output_initialized=true`
- `clock_source=Audio`
- `scheduler_late_drops=2`
- `scheduler_wait_events=274`
- `video_frame_queue_push_timeouts=0`
- `audio_frame_queue_push_timeouts=0`

结论：

- `Audio` master 路径在本机已跑通。
- 新的分段等待不会再出现前一版那种几万次 `wait_events` 的伪忙等。
- 由于本机真实 `WASAPI` 设备仍不可用，这里只验证了调度逻辑和 clock path，不代表真实声卡输出时延的最终体感结论。

## 当前判断

1. 这轮已经证明：`SoftwareSDL` 不需要大拆，只要允许 `NV12` 直传和 `AVFrame` 引用复用，就能先把 `swscale` 和 display memcpy 两段热路径拿掉。  
2. 默认 `D3D11 + D3D11VA` 主链仍然维持 zero-copy，所以主链后续重点仍是 queue / pacing / audio-master 微调，而不是重做 renderer。  
3. 回退链剩余的主要瓶颈已收敛到 `copy-back`；如果下一轮还要继续追 4K60 fallback 稳定性，应该优先评估“何时避免 `av_hwframe_transfer_data()`”，而不是继续纠结 `Display::copyFrameData()`。  