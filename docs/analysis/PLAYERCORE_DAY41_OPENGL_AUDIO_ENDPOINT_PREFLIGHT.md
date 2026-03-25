# Day41 结论：OpenGL“播放一两秒后卡死”根因是 WASAPI 默认端点阻塞，不是渲染死锁

日期：2026-03-25  
范围：`src/audio_player.cpp`、`include/audio_player.h`、`src/core/player_core.cpp`、`include/core/player_core.h`、`src/main.cpp`、`CMakeLists.txt`

## implementation planner

1. 先复现用户反馈的 OpenGL 卡死，并用日志确认卡点是否发生在 render/present 还是 open/startup。
2. 检查 `PlayerCore::open()` 和 `AudioPlayer::init()` 的调用顺序，确认主线程上是否存在同步阻塞点。
3. 设计 Windows 下的快速前置探针，避免在“没有默认音频输出端点”的机器上进入阻塞的 `SDL_OpenAudioDevice(nullptr, ...)`。
4. 把音频初始化策略、耗时和是否真正尝试打开设备暴露到 diagnostics，方便后续区分“渲染卡住”和“音频输出阻塞”。
5. Release build 后执行 OpenGL 回归和总门禁，确认修复没有打坏现有 OpenGL/字幕能力。

## 复现结论

- 这次现象不是 OpenGL renderer 死锁，也不是 D3D11VA/OpenGL interop 在 present 阶段卡住。
- 修复前复现日志显示：
  - `2026-03-25 17:38:06.634 OpenGL renderer initialized: window=1728x972`
  - `2026-03-25 17:38:14.635 Audio output init failed, continuing with video-only playback`
- 两条日志之间相隔约 8 秒，说明窗口和 OpenGL context 已经初始化完成，但 `PlayerCore::open()` 仍被后续步骤阻塞。
- 控制台同时打印：
  - `Error: Could not open audio device: WASAPI can't find requested audio endpoint: 找不到元素。`
- 阻塞点落在：
  - `PlayerCore::open()`
  - `AudioPlayer::init()`
  - `SDL_OpenAudioDevice(nullptr, 0, ...)`

## 根因分析

- 当前实现里，`PlayerCore::open()` 会在主打开路径里同步初始化音频输出。
- `AudioPlayer::init()` 之前没有任何“默认音频端点是否存在”的前置判断，直接把 `nullptr` 设备名交给 SDL，等价于让 SDL/WASAPI 打开系统默认输出设备。
- 在这台机器上，默认 render endpoint 丢失，因此 `SDL_OpenAudioDevice(nullptr, ...)` 会卡住数秒后才失败。
- 因为阻塞发生在主打开路径，用户看到的表象是：
  - OpenGL 窗口已经出来
  - 播放器不再前进
  - 几秒后才恢复成 video-only
- 所以体感会误判为“OpenGL 播一两秒后卡死”，但实际根因是音频设备初始化阻塞。

## 方案选择

### 不采用的方案：线程包裹 `SDL_OpenAudioDevice` + 超时

- 这个方向会引入悬挂线程、对象生命周期和设备句柄收尾问题。
- 一旦 `SDL_OpenAudioDevice` 真正卡住，超时线程只能被放任存活，语义不干净。

### 采用的方案：Windows 默认端点快探针 + 直接 video-only 降级

- 只在 Windows 且 SDL 仍走默认/`wasapi` 输出时启用前置探针。
- 使用 `IMMDeviceEnumerator` 探测：
  - 是否存在默认 render endpoint
  - 是否存在 active render endpoints
- 如果默认端点缺失且 active render endpoints 也为 0：
  - 直接跳过 `SDL_OpenAudioDevice`
  - 立刻返回音频初始化失败
  - 视频文件继续沿用现有 `video-only fallback`
- 这样保留了之前“音频失败时视频仍能播放”的语义，同时消除了启动期卡顿。

## 本轮落地

### 1. `AudioPlayer` 增加 Windows 音频端点 preflight

- `AudioPlayer::init()` 改为返回 `AudioInitReport`。
- Windows 下新增默认 render endpoint 探针：
  - 默认或显式 `SDL_AUDIODRIVER=wasapi` 才启用
  - `SDL_AUDIODRIVER=dummy` 等非 WASAPI 场景不介入
- 无默认/活动 render endpoint 时，返回：
  - `strategy=skip-no-default-render-endpoint`
  - `device_open_attempted=false`
  - `detail=skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI`

### 2. `PlayerCore` 持久化音频初始化诊断

- `PlayerCore` 现在记录：
  - `audio_device_open_attempted`
  - `audio_init_latency_ms`
  - `audio_init_strategy`
  - `audio_init_detail`
- 即使音频输出最终被 `reset()` 掉，diagnostics 仍能保留这次尝试的真实结论。

### 3. 播放类 CLI 输出新增音频初始化维度

- `--performance-log-check`
- `--software-video-decode-check`
- `--1080p60-check`
- `--4k-playback-check`
- `--high-bitrate-check`
- `--long-playback-check`

以上命令现在都会打印：

- `audio_device_open_attempted`
- `audio_init_latency_ms`
- `audio_init_strategy`
- `audio_init_detail`

## 修复后验证

### Release build

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' -S . -B build
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release
```

结果：PASS

### OpenGL 启动期回归

```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Remove-Item Env:MVP_RENDERER_BACKEND
```

关键结果：

- `performance-log-check.audio_output_initialized=false`
- `performance-log-check.video_only_fallback=true`
- `performance-log-check.clock_source=Video`
- `performance-log-check.audio_device_open_attempted=false`
- `performance-log-check.audio_init_latency_ms=0`
- `performance-log-check.audio_init_strategy=skip-no-default-render-endpoint`
- `performance-log-check.audio_init_detail=skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI`
- `performance-log-check.result=PASS`

关键日志：

- `2026-03-25 17:59:13.711 OpenGL renderer initialized: window=1728x972`
- `2026-03-25 17:59:13.711 Audio output preflight skipped SDL_OpenAudioDevice ...`

说明：修复后不再出现 `OpenGL renderer initialized` 和音频失败日志之间的 8 秒空窗。

### OpenGL diagnostics

```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --opengl-diagnostics
Remove-Item Env:MVP_RENDERER_BACKEND
```

结果：

- `opengl-diagnostics.probe_succeeded=true`
- `opengl-diagnostics.native_interop.allowed=true`
- `opengl-diagnostics.result=PASS`

### OpenGL 总门禁

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'juren-30s.mp4'
```

结果：

- `OpenGL gate result: PASS`
- `16/16 PASS`

## 结论

- 本次“OpenGL 播放卡死”已经收敛到明确根因：默认 `WASAPI` 输出端点缺失导致的同步音频初始化阻塞。
- 修复后播放器会在启动期快速判定“本机无可用默认 render endpoint”，直接走 video-only fallback，不再把 UI/渲染链路一起拖住。
- OpenGL renderer、D3D11VA native interop、字幕和现有 OpenGL 门禁均未回退。
