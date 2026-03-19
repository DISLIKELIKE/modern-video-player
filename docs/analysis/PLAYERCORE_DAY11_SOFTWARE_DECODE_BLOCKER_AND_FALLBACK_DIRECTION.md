# Day11 结论：`software-first` 方向符合成熟播放器思路，但当前工程的 FFmpeg 软件视频解码接入仍存在阻塞，暂不应默认切换

日期：2026-03-19  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`

## implementation planner

1. 验证 `SoftwareSDL` 是否适合通过 renderer-aware `software-first` 来进一步减少或规避 `av_hwframe_transfer_data()` copy-back。
2. 复现并定位 `SoftwareSDL + Software decode` 在当前工程里不产出视频帧的真实阻塞点。
3. 对比 `D3D11 + Software decode`，判断问题是否来自 `SoftwareSDL` renderer，还是来自软件视频解码接入本身。
4. 在主流程不回归的前提下收敛策略：保留当前已验证通过的 `D3D11VA copy-back` fallback，不把 `software-first` 默认带进主线。
5. 为后续单独修软件视频解码接入补上最小必要诊断，避免重复盲试。

## 关键结论

### 1. 方向本身与开源播放器常见设计是一致的，但前提是软件解码链必须稳定

- 从设计理念上看，`system-memory renderer` 优先避免 GPU 解码后的 copy-back，是符合 `ffplay / mpv / MPC-HC` 一类播放器常见思路的。
- 如果 renderer 只能消费系统内存帧，那么最理想的情况通常是：
  - 要么直接软件解码；
  - 要么让硬解输出能被 renderer 直接消费的系统内存格式；
  - 再不行才走 `av_hwframe_transfer_data()` 这类 copy-back。
- 所以“尽量减少或规避 copy-back”这个优化方向本身没有问题。

### 2. 当前工程里，问题不在 `SoftwareSDL`，而在“FFmpeg 软件视频解码接入路径”本身

- 这轮临时实验里，一旦把 `SoftwareSDL` 自动改成 `software-first`，`--performance-log-check` 会出现：
  - `decode_video_ok=0`
  - `render_frames=0`
  - `video_frame_queue_peak_size=0`
  - `pkt_q(v=full or near full)`
- 这说明问题不是“软解太慢”，而是“视频解码线程没有形成有效产出”。
- 进一步对比 `D3D11 + Software decode` 的强制软解 session 检查后，仍然可以稳定复现“首个 `send_packet()` 返回，但软件视频解码链后续不产出可用帧”的现象。
- 因此当前 blocker 不属于 `SoftwareSDL` 上传/显示层，而属于软件视频解码接入本身。

### 3. 当前最安全的收敛策略：撤回自动 `software-first`，继续保留 `D3D11VA copy-back` fallback

- 上一轮已经确认：
  - 默认 `D3D11 + D3D11VA` 主链是 zero-copy；
  - `SoftwareSDL` fallback 已通过 `NV12` 直传和 `AVFrame` 引用复用，基本消掉 `swscale + display memcpy`。
- 在这个前提下，虽然 `copy-back` 仍是 fallback 主瓶颈，但也不能用一个当前会卡死的软件解码链去替换它。
- 所以本轮最终策略是：
  - 不保留自动 renderer-aware `software-first`；
  - 恢复 `D3D11VA -> Software` 默认 decoder 顺序；
  - 保留针对软件视频解码的低频诊断日志，作为后续单独修复入口。

## 为什么这仍然是正确方向

如果未来软件视频解码链被真正修通，这条方向理论上能带来的收益仍然非常明确：

1. 可以把当前 `SoftwareSDL` fallback 的 `video_copy_back_ratio_percent` 进一步压低，理想情况下接近 `0`。
2. 已经消掉的 `swscale / display memcpy` 不会再回流，fallback 热点会进一步减少。
3. 视频路径会更短，frame latency 和内存带宽压力都会更小。

但也必须看到代价：

1. 软解会把更多负载转移到 CPU。
2. 4K/60 这类样本未必适合默认走软解。
3. 所以即便未来软解接入修好，也更适合做成“按样本能力/策略分级”的选择，而不是当前这种一刀切默认切换。

## 这轮定位证据

### A. 当前最终代码下，默认主链仍正常

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
- `result=PASS`

结论：

- 默认 zero-copy 主链未受本轮 investigation 影响。

### B. 当前最终代码下，`SoftwareSDL` fallback 继续保持“只剩 copy-back”状态

命令：

```powershell
$env:MVP_RENDERER_BACKEND='software'
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200
```

关键结果：

- `renderer_backend=SoftwareSDL`
- `decoder_backend=D3D11VA`
- `video_copy_back_ratio_percent=33.5958`
- `video_swscale_ratio_percent=0`
- `display_copy_ratio_percent=0`
- `result=PASS`

结论：

- 当前最终代码中，`SoftwareSDL` fallback 已恢复到安全可用状态；
- 热点仍集中在 `copy-back`，但不再被错误的 `software-first` 策略拖进 0 帧输出回归。

### C. 当前最终代码下，强制软解仍可见软件视频解码接入 blocker

命令：

```powershell
$env:SDL_AUDIODRIVER='dummy'
.\build\Debug\modern-video-player.exe --windows-backend-session-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv soft
```

关键日志：

- `Video decode first send_packet start: backend=Software ...`
- `Video decode first send_packet returned: backend=Software ret=0 message=unknown`
- `windows-backend-session-check.soft.decoder_backend=Software`
- `windows-backend-session-check.result=PASS`

说明：

- 这个命令只证明“能打开并进入播放循环”，不证明“真的稳定产出并渲染视频帧”。
- 结合本轮 `software-first` 实验里 `decode_video_ok=0 / render_frames=0` 的结果，可以确认：
  - blocker 不在 renderer；
  - blocker 在当前软件视频解码接入链。

## 本轮代码收敛

### 保留

- `prepareVideoOutputFrame()` 的 direct software frame 路径；
- 软件解码问题的低频诊断能力：
  - FFmpeg 错误码转字符串；
  - 首次 `send_packet` 探针；
  - stall 上下文日志。

### 撤回

- 自动 renderer-aware `software-first` decoder 重新排序。

## 下一步建议

1. 单独为 “Software video decode” 建一个可直接判定“是否真的产帧”的专项检查命令，而不是继续依赖 session-check。
2. 对照 `ffplay / mpv` 继续核对当前 `AVCodecContext` 初始化差异，优先排查：
   - 软件视频解码线程配置；
   - packet feed / flush / drain 语义；
   - 可能影响软件解码路径的额外 flags。
3. 在软解链稳定之前，不要再次默认启用 `software-first`；最多作为显式实验开关存在。
