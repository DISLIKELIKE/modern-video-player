# Day14 结论：已用最小计数把 software video decode blocker 收敛到“首个视频包送入 decoder 阶段卡住”

日期：2026-03-19  
范围：`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp`

## implementation planner

1. 不改 `SoftwareSDL` 渲染侧，也不继续扩大日志面，只在 `PlayerCore::decodeVideoFrame()` 附近补最小计数。
2. 补三个针对首包路径的核心观测值：
   - `video_packet_dequeue_count`
   - `video_send_packet_ok`
   - `video_send_packet_last_ret`
3. 把计数透传到 `DiagnosticsSnapshot`、低频诊断日志和 CLI 结构化输出，保证回归命令能直接观察。
4. 重跑 Debug 构建与 software decode 专项检查，判断 software path 目前究竟卡在 dequeue 前、`send_packet` 返回后，还是 `send_packet` 调用本身。

## 关键结论

### 1. 最小计数已经证明：software path 不是“没取到视频包”，而是“首包送入 decoder 阶段卡住”

本轮新增的最小计数字段：

- `video_packet_dequeue_count`
- `video_send_packet_ok`
- `video_send_packet_last_ret`

其中：

- `video_packet_dequeue_count`：视频包队列成功 `pop()` 一次就加一
- `video_send_packet_ok`：`avcodec_send_packet(video_codec_ctx_, packet.get())` 返回 `>= 0` 才加一
- `video_send_packet_last_ret`：记录最近一次视频包 `send_packet` 的原始返回码
- 初始哨兵值：`std::numeric_limits<int>::min()`，含义是“当前还没有任何一次完成返回的 packet send”

### 2. software decode 最新诊断已经把 blocker 再收紧一步

本轮重跑后，workspace 日志中的关键片段：

```text
Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808
[diag:audio-backpressure] ... dec(core v=0,a=155,v_pkt_deq=1,v_send_ok=0,v_send_ret=-2147483648,v_send_eagain=0,...) ...
```

这说明：

- software decode 线程确实已经从 `video packet queue` 里取到了首个视频包
- 但到 1 秒诊断点为止，`video_send_packet_ok=0`
- 同时 `video_send_packet_last_ret=-2147483648` 仍然是未返回哨兵值
- 并且控制台仍然只有 `Video decode first send_packet start`，没有 `returned`

因此当前最稳的结论是：

- software path 不是卡在 dequeue 前
- 也不是“`send_packet` 已经返回，但后面收不到 frame”
- 更像是卡在首个 `avcodec_send_packet(video_codec_ctx_, packet.get())` 调用本身，或者至少没有完成第一次返回

### 3. 正常主链也能输出新计数，说明诊断接线是通的

本轮用 `juren-30s.mp4` 重新执行：

```powershell
.\build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
```

关键输出：

- `renderer_backend=D3D11`
- `decoder_backend=D3D11VA`
- `video_packet_dequeue_count=57`
- `video_send_packet_ok=57`
- `video_send_packet_last_ret=0`
- `result=PASS`

说明新计数字段不仅已经写进 `PlayerCore`，也已经正确透传到 CLI 结构化输出。

## 本轮本地验证

### 构建

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `0 warnings / 0 errors`

### software decode 专项复核

命令：

```powershell
.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000
```

结合控制台与日志收敛到：

- `renderer_backend=SoftwareSDL`
- `decoder_backend=Software`
- `decode_video_ok=0`
- `video_packet_dequeue_count=1`
- `video_send_packet_ok=0`
- `video_send_packet_last_ret=-2147483648`
- `scheduler_video_decoded_frames=0`
- `render_frames=0`
- `result=FAIL`

### 正常主链对照

命令：

```powershell
.\build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
```

结果：

- `video_packet_dequeue_count=57`
- `video_send_packet_ok=57`
- `video_send_packet_last_ret=0`
- `result=PASS`

## 下一步建议

1. 下一轮优先继续围绕 software path 的首个 `avcodec_send_packet()` 调用本身排查，不要再分散到 renderer/display。
2. 如果继续加诊断，优先补：
   - `video_send_packet_enter_count`
   - `video_send_packet_fail_count`
   - software path 首包前后的线程/锁持有时间
3. 对照 `ffplay / mpv` 时，先核对 software H.264 path 的：
   - `AVCodecContext` 关键字段
   - `send_packet` 调用线程模型
   - 是否存在当前工程特有的锁/回调/FFmpeg context 状态交互
