# Day12 结论：`--software-video-decode-check` 已把 software video decode blocker 从“会话能否打开”收敛到“软件视频解码根本不产帧”

日期：2026-03-19  
范围：`src/main.cpp`

## implementation planner

1. 单开一个只面向 `SoftwareSDL + Software decode` 的专项检查命令，不再继续复用只能证明“会话已启动”的 `session-check`。
2. 在命令内部强制 `MVP_RENDERER_BACKEND=software`、`SDL_AUDIODRIVER=dummy` 和 `preferHardwareDecode=false`，避免环境差异污染结论。
3. 把通过条件收紧为“真实产帧”而不是“打开成功”：要求 `decode_video_ok > 0`、`scheduler_video_decoded_frames > 0`、`render_frames > 0`、`video_frame_queue_peak_size > 0`，并验证 `video_copy_back_frames == 0`。
4. 由于当前 blocker 下 `stop/close` 也可能被拖挂，命令本身改成 probe 式硬退出，保证 FAIL 也能稳定收口。
5. 重跑构建并执行专项检查，用结构化输出来判断 blocker 到底落在 soft decode 链的哪一层。

## 关键结论

### 1. 新命令已经把“会话能打开”和“软件视频真的产帧”彻底拆开

- 本次新增命令：
  - `--software-video-decode-check <media_file> [sample_ms]`
- 它不会再把 `open_ok=true` 或 `entered_playback_loop=true` 当成成功条件。
- 它要求目标链路必须同时满足：
  - `renderer_backend=SoftwareSDL`
  - `decoder_backend=Software`
  - `decode_video_ok > 0`
  - `scheduler_video_decoded_frames > 0`
  - `render_frames > 0`
  - `video_frame_queue_peak_size > 0`
  - `video_copy_back_frames == 0`

### 2. 当前 blocker 已明确不在 copy-back / swscale / display copy，而在 software video decode 本身

本机验证命令：

```powershell
.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000
```

关键结果：

- `open_ok=true`
- `entered_playback_loop=true`
- `renderer_backend=SoftwareSDL`
- `decoder_backend=Software`
- `audio_output_initialized=true`
- `clock_source=Audio`
- `advanced_seconds=1.89867`
- `demux_video_packets=163`
- `demux_ignored_packets=0`
- `demux_queue_drop_packets=0`
- `decode_video_ok=0`
- `scheduler_video_decoded_frames=0`
- `render_frames=0`
- `video_frame_queue_peak_size=0`
- `video_copy_back_frames=0`
- `video_swscale_frames=0`
- `result=FAIL`

结论：

- 这条链路已经确认命中了目标组合：`SoftwareSDL + Software decode`。
- demux 和 audio clock 都在正常推进，说明问题不是“没打开”“没进播放循环”“没喂包”。
- 但软件视频解码没有形成任何可消费视频帧，甚至连 video frame queue 都没有被填过。
- 同时 `video_copy_back_frames=0 / video_swscale_frames=0` 说明这次失败与 copy-back、swscale、display copy 都无关。
- 因而 blocker 现在可以直接定性为：当前工程里的 FFmpeg software video decode 接入链没有形成视频帧产出。

### 3. `stop/close` 被拖挂是 blocker 的伴随症状，不再应该影响专项检查收口

- 第一次直接运行这个专项检查时，虽然 `sample_ms=2000`，但命令尾部的常规 `stop/close` 被 soft decode blocker 一起拖住，导致命令本身无法及时退出。
- 因此本轮把该命令收敛成 probe 式硬退出：
  - 打印完结构化结果后直接 `TerminateProcess(GetCurrentProcess(), code)` / `std::_Exit(code)`。
- 这样命令本身不会再被 blocker 挂死，后续可以稳定用于回归和继续定位。

## 为什么这个结论重要

1. 之前只能说“soft decode 方向可疑”；现在已经能明确说“它确实没有产出任何视频帧”。
2. 之前 `session-check` 只能证明“打开成功”；现在新命令把“真产帧”定义成了硬门槛，后续不会再混淆。
3. 之前 fallback 优化仍有可能被误解成 copy-back 问题没解决；现在可以明确区分：
   - `SoftwareSDL` fallback 的 copy-back 热点仍在硬解回退链；
   - 当前 software decode blocker 则根本还没走到 copy-back 那一步。

## 本轮代码收敛

### 新增

- `src/main.cpp`
  - `--software-video-decode-check <media_file> [sample_ms]`
  - 强制 `SoftwareSDL + Software decode + dummy audio`
  - probe 式硬退出收口

### 保留

- Day11 已保留的软件视频解码低频诊断：
  - FFmpeg 错误码字符串
  - 首次 `send_packet` 起止探针
  - stall 上下文日志

## 本地验证

### 构建

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `0 warnings / 0 errors`

### 专项检查

```powershell
.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000
```

结果：

- 退出状态：非零
- 结构化输出：`result=FAIL`
- blocker 已稳定复现且不再把命令本身拖挂。

## 下一步建议

1. 继续围绕 software video decode 产帧链做最小化定位，不要重新把 `software-first` 带回 fallback 主流程。
2. 重点核对当前工程与 `ffplay / mpv` 的 software decode 差异，优先排查：
   - `AVCodecContext` 初始化参数
   - software decode 线程数 / thread type
   - `receive -> send -> drain` 语义
   - 可能只影响 software path 的 codec flags
3. 如果继续深化，可再补一个更低层的专项日志，把 `avcodec_receive_frame()` 的返回序列、EOF/drain 切换点和首个成功帧条件单独打出来。