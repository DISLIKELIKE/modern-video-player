# 本地验收：播放性能日志

- 日期：`2026-03-08`
- 目标：验证 `2.2.4 输出性能日志（掉帧/队列/CPU/GPU）`
- 样本：`./samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv`
- 采样时长：`1200ms`

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200
```

## 输出摘要

```text
performance-log-check.path=.\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv
performance-log-check.sample_ms=1200
performance-log-check.open_ok=true
performance-log-check.entered_playback_loop=true
performance-log-check.video_width=3840
performance-log-check.video_height=2160
performance-log-check.audio_channels=2
performance-log-check.audio_sample_rate=48000
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.cpu_avg_percent=100.023
performance-log-check.cpu_logical_cores=8
performance-log-check.demux_video_packets=240
performance-log-check.demux_audio_packets=125
performance-log-check.demux_push_retries=0
performance-log-check.demux_dropped_packets=125
performance-log-check.decode_video_ok=103
performance-log-check.decode_audio_ok=75
performance-log-check.audio_submitted_frames=49
performance-log-check.render_frames=90
performance-log-check.scheduler_video_decoded_frames=103
performance-log-check.scheduler_audio_decoded_frames=75
performance-log-check.scheduler_late_drops=0
performance-log-check.video_packet_queue_size=126
performance-log-check.audio_packet_queue_size=50
performance-log-check.video_frame_queue_size=12
performance-log-check.audio_frame_queue_size=26
performance-log-check.result=PASS
```

## 结论

- `PASS`：命令行验收入口已可稳定输出掉帧、队列、CPU 与当前 GPU 路径（decoder / renderer backend）指标。
- 当前样本在本次 `1200ms` 采样窗口内未出现 `scheduler_late_drops`，并能导出 demux / decode / render / queue 的结构化快照。
- 该命令可作为后续 `1080p60 / 4K / 高码率` 样本稳定性门禁的统一性能观测入口。
