# 本地验收：4K 播放与降级

- 日期：`2026-03-08`
- 目标：验证 `2.2.2 4K 可播放（优先稳定，再提性能）` 与 `2.3.3 4K 播放可用并可降级`
- 样本：`./samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv`
- 采样时长：`2000ms`

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000
```

## 输出摘要

```text
4k-playback-check.path=.\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv
4k-playback-check.sample_ms=2000
4k-playback-check.probe_overall=PASS
4k-playback-check.probe_width=3840
4k-playback-check.probe_height=2160
4k-playback-check.probe_fps=60
4k-playback-check.probe_duration=4
4k-playback-check.open_ok=true
4k-playback-check.entered_playback_loop=true
4k-playback-check.still_playing_after_window=true
4k-playback-check.renderer_backend=D3D11
4k-playback-check.decoder_backend=D3D11VA
4k-playback-check.start_position=0
4k-playback-check.end_position=1.93633
4k-playback-check.advanced_seconds=1.93633
4k-playback-check.advance_ratio=0.968167
4k-playback-check.progress_ok=true
4k-playback-check.duration_ok=true
4k-playback-check.late_drops=0
4k-playback-check.late_drops_ok=true
4k-playback-check.demux_dropped_packets=125
4k-playback-check.hard.mode_ok=true
4k-playback-check.hard.renderer_backend=D3D11
4k-playback-check.hard.decoder_backend=D3D11VA
4k-playback-check.hard.exit_code=0
4k-playback-check.soft.mode_ok=true
4k-playback-check.soft.renderer_backend=D3D11
4k-playback-check.soft.decoder_backend=Software
4k-playback-check.soft.exit_code=0
4k-playback-check.fallback_ok=true
4k-playback-check.result=PASS
```

## 结论

- `PASS`：当前 `4K` 样本在连续播放窗口内可稳定推进，未出现 `late_drop`。
- hard / soft 两个后端会话均通过，说明硬解失败时仍可降级到 `Software` 解码继续播放。
- 当前 `4K` 门禁已收口，后续高分辨率剩余重点转为 `>80Mbps` 高码率样本稳定性。
