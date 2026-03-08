# ??????? / ??????

- ???`2026-03-08`
- ????? `4.5 ????/??????`
- ???`./juren-30s.mp4`
- ???`./samples/subtitle/subtitle_seek_sync_sample.srt`

## ????

```powershell
.\build\Debug\modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt
```

## ????

```text
delay-adjust-check.path=.\juren-30s.mp4
delay-adjust-check.subtitle=.\samples\subtitle\subtitle_seek_sync_sample.srt
delay-adjust-check.open_ok=true
delay-adjust-check.subtitle_loaded=true
delay-adjust-check.subtitle_entries=5
delay-adjust-check.entered_playback_loop=true
delay-adjust-check.paused_before_adjust=true
delay-adjust-check.probe_found=true
delay-adjust-check.probe_index=1
delay-adjust-check.probe_before=1.45
delay-adjust-check.probe_inside=1.55
delay-adjust-check.baseline_before_clear=true
delay-adjust-check.baseline_inside_visible=true
delay-adjust-check.subtitle_early_ok=true
delay-adjust-check.subtitle_late_ok=true
delay-adjust-check.audio_roundtrip_ok=true
delay-adjust-check.subtitle_roundtrip_ok=true
delay-adjust-check.final_audio_delay_ms=100
delay-adjust-check.final_subtitle_delay_ms=-100
delay-adjust-check.result=PASS
```

## ??

- `PASS`?????????????????????? getter/setter ??????????
- ?????????
  - `J / K` ?????????????
  - `Ctrl+J / Ctrl+K` ????????????????
  - `--settings-persistence-check` ????/?????????
