# 本地验收：>80Mbps 高码率样本

- 日期：`2026-03-08`
- 目标：验证 `2.2.3 大码率样本（>80Mbps）可播放`
- 样本：`./samples/mp4/stress100m__h264_aac__1920x1080__60fps__2ch.mp4`
- 采样时长：`3000ms`
- 样本准备：可通过 `.\tools\download_test_samples.ps1 -DurationSec 6` 生成

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000
```

## 输出摘要

```text
high-bitrate-check.path=.\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4
high-bitrate-check.sample_ms=3000
high-bitrate-check.probe_overall=PASS
high-bitrate-check.probe_width=1920
high-bitrate-check.probe_height=1080
high-bitrate-check.probe_fps=60
high-bitrate-check.probe_duration=6
high-bitrate-check.format_bitrate_bps=102829290
high-bitrate-check.bitrate_ok=true
high-bitrate-check.open_ok=true
high-bitrate-check.entered_playback_loop=true
high-bitrate-check.still_playing_after_window=true
high-bitrate-check.renderer_backend=D3D11
high-bitrate-check.decoder_backend=D3D11VA
high-bitrate-check.start_position=0
high-bitrate-check.end_position=2.96533
high-bitrate-check.advanced_seconds=2.96533
high-bitrate-check.advance_ratio=0.988444
high-bitrate-check.progress_ok=true
high-bitrate-check.duration_ok=true
high-bitrate-check.late_drops=0
high-bitrate-check.late_drops_ok=true
high-bitrate-check.demux_dropped_packets=0
high-bitrate-check.demux_drops_ok=true
high-bitrate-check.result=PASS
```

## 结论

- `PASS`：样本格式码率约 `102.8Mbps`，已超过 `80Mbps` 门禁，并能在连续播放窗口内稳定推进。
- 当前样本未出现 `late_drop` 与 demux 丢包，说明主链在本地 `100Mbps` 级别样本上可稳定工作。
- 至此，任务清单中的 `2.2` 高分辨率 / 高码率分支已全部完成。
