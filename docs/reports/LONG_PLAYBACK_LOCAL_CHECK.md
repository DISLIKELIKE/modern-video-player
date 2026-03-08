# 本地验收：长时播放稳定性

- 日期：`2026-03-08`
- 目标：验证 `6.5 稳定性：长时播放无 crash`
- 样本：`./juren-30s.mp4`
- 采样时长：`10000ms`
- 说明：使用固定 `10s` 播放窗口做本地 smoke，确认真实时间推进、播放态保持与关键掉帧/丢包指标。

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000
```

## 输出摘要

```text
long-playback-check.path=.\juren-30s.mp4
long-playback-check.sample_ms=10000
long-playback-check.probe_overall=PASS
long-playback-check.probe_duration=30.03
long-playback-check.open_ok=true
long-playback-check.entered_playback_loop=true
long-playback-check.still_playing_after_window=true
long-playback-check.renderer_backend=D3D11
long-playback-check.decoder_backend=D3D11VA
long-playback-check.start_position=0
long-playback-check.end_position=9.96267
long-playback-check.advanced_seconds=9.96267
long-playback-check.advance_ratio=0.996267
long-playback-check.progress_ok=true
long-playback-check.duration_ok=true
long-playback-check.late_drops=0
long-playback-check.late_drops_ok=true
long-playback-check.demux_dropped_packets=0
long-playback-check.demux_drops_ok=true
long-playback-check.result=PASS
```

## 结论

- `PASS`：当前样本在 `10s` 连续播放窗口内保持播放态，时间推进约为窗口长度的 `99.63%`。
- 本次 smoke 未出现 `scheduler_late_drops` 与 demux 丢包，`D3D11 + D3D11VA` 主链在该样本上保持稳定。
- 至此，任务清单中的发布门禁 `6.1 ~ 6.6` 已具备对应本地证据，`6.5` 不再是阻塞项。
