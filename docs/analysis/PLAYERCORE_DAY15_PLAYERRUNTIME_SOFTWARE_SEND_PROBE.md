# Day15 结论：software decode blocker 已收敛到 PlayerCore 运行态 software send 卡死
日期：2026-03-19  
范围：`src/main.cpp`、`src/core/player_core.cpp`

## implementation planner

1. 先用独立 `--software-video-send-probe` 验证 FFmpeg software decode 本体是否能正常 `send_packet`。
2. 在 probe 内逐步补齐与真实链路一致的对照：`pre-receive`、`packet queue round-trip`、`demux read-ahead`。
3. 在 `--software-video-decode-check` 里补 `video-only` 对照，确认 blocker 是否依赖音频链或 Audio master。
4. 在 `PlayerCore::decodeVideoFrame()` 里补仅环境变量开启的 `offthread send` 诊断，确认是否只是当前 video decode 线程上下文卡死。

## 关键结论

### 1. software decoder 本体不是 blocker

`--software-video-send-probe .\juren-30s.mp4 1500` 的最新结果：

- `pre_send_receive_ret=-11`
- `packet_queue_push_ok=true`
- `packet_queue_pop_ok=true`
- `read_ahead_packets=512`
- `send_completed=true`
- `send_ret=0`
- `receive_got_frame=true`
- `result=PASS`

含义：

- 先 `receive_frame(EAGAIN)` 再 `send_packet` 没问题。
- 首包先经过同款 packet queue 再送没问题。
- 首包取出后，后台继续 read-ahead 512 个包再送也没问题。

因此，`send-probe` 已经排除了以下怀疑点：

- FFmpeg software H.264 decoder 本体不可用
- `receive -> send` 顺序问题
- `ThreadSafeQueue<AVPacket>` 交接问题
- demux 继续读后续包导致首包立刻失效

### 2. 音频链不是 blocker

`$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'`
` .\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200`

关键结果：

- `audio_probe_mode=disabled`
- `audio_output_initialized=false`
- `video_only_fallback=true`
- `clock_source=Video`
- `video_packet_dequeue_count=1`
- `video_send_packet_ok=0`
- `video_send_packet_last_ret=-2147483648`
- `decode_video_ok=0`
- `render_frames=0`
- `result=FAIL`

含义：

- 就算彻底绕开音频输出和 Audio master，software decode 仍然卡在首个视频包 `send_packet` 阶段。

### 3. 不是“当前 video decode 线程上下文”独有问题

`$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'`
`$env:MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD='1'`
` .\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200`

关键日志：

- `Offthread software video send_packet probe timed out after 500ms`

关键结果：

- `video_packet_dequeue_count=1`
- `video_send_packet_ok=0`
- `video_send_packet_last_ret=-2147483648`
- `decode_video_ok=0`
- `render_frames=0`
- `result=FAIL`

含义：

- 进入 `PlayerCore` 真实运行态后，即使把 software `send_packet` 临时挪到另一个线程，仍然会超时。
- 因此 blocker 已经进一步收敛为：
  `PlayerCore` 运行态里的 software decode context / surrounding state 与独立 probe 存在关键差异。

## 本地验证

### 构建

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `0 warnings / 0 errors`

### 独立 software send probe

```powershell
.\build\Debug\modern-video-player.exe --software-video-send-probe .\juren-30s.mp4 1500
```

结果：

- `packet_queue_push_ok=true`
- `packet_queue_pop_ok=true`
- `read_ahead_packets=512`
- `pre_send_receive_ret=-11`
- `send_ret=0`
- `receive_got_frame=true`
- `result=PASS`

### video-only software decode check

```powershell
$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'
.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200
```

结果：

- `clock_source=Video`
- `video_packet_dequeue_count=1`
- `video_send_packet_ok=0`
- `decode_video_ok=0`
- `render_frames=0`
- `result=FAIL`

### PlayerCore offthread send 诊断

```powershell
$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'
$env:MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD='1'
.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200
```

结果：

- 日志出现 `Offthread software video send_packet probe timed out after 500ms`
- `result=FAIL`

## 下一步建议

1. 直接对比 `PlayerCore::initDecoders()` 产出的 software `AVCodecContext` 与独立 probe 的 codec ctx 字段差异，不要再继续扩展 renderer 侧。
2. 优先检查 `PlayerCore` 在 open/play 后是否对 software video codec context 留下了 probe 没有的额外状态。
3. 如果继续加专项，优先做 “PlayerCore codec ctx field dump / diff”，而不是再加更外围的 packet 或 render 统计。
