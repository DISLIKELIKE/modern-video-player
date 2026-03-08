# 本地验收：1080p60 稳定播放

- 日期：`2026-03-08`
- 目标：验证 `2.2.1 1080p60 长时稳定` 与 `2.3.2 1080p60 稳定播放达标`
- 样本：`./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4`
- 采样时长：`5000ms`
- 样本准备：可通过 `.\tools\download_test_samples.ps1 -DurationSec 10` 生成

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000
```

## 输出摘要

```text
1080p60-check.path=.\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4
1080p60-check.sample_ms=5000
1080p60-check.probe_overall=PASS
1080p60-check.probe_width=1920
1080p60-check.probe_height=1080
1080p60-check.probe_fps=60
1080p60-check.probe_duration=10
1080p60-check.probe_recommend_hw=true
1080p60-check.probe_recommend_d3d11=true
1080p60-check.open_ok=true
1080p60-check.entered_playback_loop=true
1080p60-check.still_playing_after_window=true
1080p60-check.renderer_backend=D3D11
1080p60-check.decoder_backend=D3D11VA
1080p60-check.start_position=0
1080p60-check.end_position=4.97067
1080p60-check.advanced_seconds=4.97067
1080p60-check.advance_ratio=0.994133
1080p60-check.progress_ok=true
1080p60-check.duration_ok=true
1080p60-check.late_drops=0
1080p60-check.late_drops_ok=true
1080p60-check.demux_dropped_packets=0
1080p60-check.demux_drops_ok=true
1080p60-check.decode_video_ok=590
1080p60-check.render_frames=590
1080p60-check.result=PASS
```

## 结论

- `PASS`：当前 `1080p60` 样本在 `5s` 连续播放窗口内稳定推进，未出现 `late_drop` 与 demux 丢包。
- 本次验收同时确认 `D3D11 + D3D11VA` 组合可在该样本上持续进入稳定播放状态。
- 该命令可作为后续 `4K` 与高码率门禁之外的 `1080p60` 固定回归入口。
