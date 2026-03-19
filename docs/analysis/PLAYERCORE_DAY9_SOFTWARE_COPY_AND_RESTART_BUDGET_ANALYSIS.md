# Day9 结论：高码率/4K 下需要区分 D3D11 zero-copy 主链与 SoftwareSDL 回退链，`Display::copyFrameData()` 在软件链上已是明确热点

日期：2026-03-19  
范围：`include/display.h`、`src/display.cpp`、`include/render/video_renderer.h`、`include/render/sdl_video_renderer.h`、`src/render/sdl_video_renderer.cpp`、`include/core/scheduler.h`、`src/core/scheduler.cpp`、`src/render/renderer_factory.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 把 `SoftwareSDL -> Display::copyFrameData() -> SDL_UpdateYUVTexture` 这条软件显示链的 memcpy 成本正式纳入诊断，而不是继续凭经验估算。
2. 把 `Scheduler` 从“固定重启次数”改为“时间窗口预算 + 冷却”，并统计 limit hit，避免短时抖动过早中断播放。
3. 补一个显式 renderer override，让 `--renderer-fallback-check` 和 `--performance-log-check` 能稳定命中 `SoftwareSDL`，从而得到软件路径真值。
4. 分别验证 `D3D11` 主链和 `SoftwareSDL` 回退链，重新判断高码率/4K 的真实瓶颈分布。

## 本轮落地

### 1. `Display::copyFrameData()` 诊断穿透

- `Display` 新增 `FrameCopyStats`：
  - `frames`
  - `bytes`
  - `time_us_total`
  - `time_us_max`
- `SdlVideoRenderer` 通过 `RendererDiagnostics` 把这组统计透传给 `PlayerCore::DiagnosticsSnapshot`。
- `--performance-log-check` 现已输出：
  - `display_copy_frames`
  - `display_copy_bytes`
  - `display_copy_time_ms`
  - `display_copy_ratio_percent`
  - `display_copy_avg_ms`
  - `display_copy_max_us`

### 2. `Scheduler` 重启策略改成窗口预算

- 旧实现按固定次数允许 worker restart，过于粗糙。
- 新实现改成：
  - 每个 worker 独立预算
  - `30s` 窗口内最多 `4` 次 restart
  - 每次 restart 之间 `100ms` 冷却
  - 若 worker 在 `running_=true` 时异常返回，也视为 unexpected exit 并按同一预算处理
- 诊断新增：
  - `scheduler_*_restart_limit_hits`
- 这更接近成熟播放器常见的“错误可恢复，但要有时间窗口预算，避免线程抖动把会话直接拖死”的思路。

### 3. renderer override 补齐

- `RendererFactory::detectBestRendererType()` 之前完全无视环境变量，导致：
  - `--renderer-fallback-check` 实际上无法稳定命中 `SoftwareSDL`
  - `Display::copyFrameData()` 也无法做真实采样
- 现在新增：
  - `MVP_RENDERER_BACKEND=software|softwaresdl|sdl|d3d11|opengl`
  - Windows 下兼容 `MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`
- 这不会改变默认主链，只影响明确要求的软件回退/诊断场景。

## 实测结果

### A. 默认主链：`D3D11 + D3D11VA`

命令：

```powershell
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500
```

关键结果：

- `renderer_backend=D3D11`
- `decoder_backend=D3D11VA`
- `video_native_output_frames=96`
- `video_copy_back_frames=0`
- `video_swscale_frames=0`
- `display_copy_frames=0`
- `scheduler_video_restart_attempts=0`
- `scheduler_video_restart_limit_hits=0`

结论：

- 当前默认 4K 主链依旧是 native zero-copy。
- `av_hwframe_transfer_data()`、`sws_scale()`、`Display::copyFrameData()` 都没有命中。
- 因此“当前机器上高码率/4K 输出帧不稳”的主因仍然不是软件拷贝链，而更接近队列水位、背压节流和时钟策略。

### B. 强制软件显示链：`SoftwareSDL + D3D11VA`

命令：

```powershell
$env:MVP_RENDERER_BACKEND='software'
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200
```

关键结果：

- `renderer_backend=SoftwareSDL`
- `decoder_backend=D3D11VA`
- `video_native_output_frames=0`
- `video_copy_back_frames=64`
- `video_copy_back_ratio_percent=30.1018`
- `video_swscale_ratio_percent=18.6623`
- `display_copy_frames=46`
- `display_copy_ratio_percent=21.8407`
- `display_copy_avg_ms=5.69759`
- `display_copy_max_us=10667`
- `scheduler_video_restart_attempts=0`
- `scheduler_video_restart_limit_hits=0`

结论：

- 在 `SoftwareSDL` 回退链上，`Display::copyFrameData()` 已经是明确热点，大约占这次 `1200ms` 采样窗口的 `21.84%`。
- 但它不是唯一热点：
  - copy-back 约 `30.10%`
  - swscale 约 `18.66%`
- 也就是说，软件回退链当前更像是“三段重成本叠加”：
  - `av_hwframe_transfer_data()`
  - `sws_scale()`
  - `Display::copyFrameData()`
- 如果用户的不稳定场景发生在 `SoftwareSDL` 或 fallback 路径，这条链已经足以解释 4K60 明显吃紧。

## 对问题 8 / 9 / 10 的更新判断

8. `Display::copyFrameData()` 在默认 `D3D11` 主链占比仍为 `0`；但在 `SoftwareSDL` 4K60 回退链的本机实测里，占比约 `21.84%`，已经不能忽略。  
9. 硬解回退到软件显示时，`av_hwframe_transfer_data()` 依旧是更大的单点热点，本机实测约 `30.10%`；它和 swscale、display copy 是叠加关系。  
10. `Scheduler` 的“固定次数重启”确实偏硬，这轮已改成窗口预算；当前实测 `restart_limit_hits=0`，说明它不是这批样本下的主因，但新策略比旧实现更不容易误杀会话。  

## 是否需要部分重构

- 默认 `D3D11 + D3D11VA` 主链：
  - 暂时不需要大重构。
  - 继续沿着 queue / pacing / audio-master lateness 做增量优化就够。
- `SoftwareSDL` 回退链：
  - 需要“有限重构”，但不建议一步做大。
  - 更合理的次序是：
    1. 保持硬解，但减少/避免每帧 copy-back
    2. 若必须软件显示，优先减少 swscale + memcpy 次数
    3. 再考虑更深的 upload 路径重做

## 下一步建议

1. 若主目标仍是 Windows 主链稳定性，继续优先优化 audio-master lateness 与 render catch-up，不做大重构。  
2. 若要保证 fallback 质量，优先评估 `SoftwareSDL` 是否能改成“decoder 直接产出渲染目标格式 + 避免二次深拷贝”的路径。  
3. 若后续要继续对标 mpv / MPC-HC，建议把“renderer policy / decoder policy / fallback policy”显式配置化，而不是只靠自动探测。  