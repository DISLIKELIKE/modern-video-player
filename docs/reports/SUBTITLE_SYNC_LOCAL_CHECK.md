# Subtitle Sync Local Check

Generated: 2026-03-08
Task: `1.4.1 SRT 字幕可用且 seek 后同步`

## Command

```powershell
.\build\Debug\modern-video-player.exe --subtitle-sync-check samples\subtitle\subtitle_seek_sync_sample.srt
```

## Output

```text
subtitle-sync-check.path=samples\subtitle\subtitle_seek_sync_sample.srt
subtitle-sync-check.entries=5
subtitle-sync-check.ordered_checks=70
subtitle-sync-check.seek_checks=74
subtitle-sync-check.mismatches=0
subtitle-sync-check.result=PASS
```

## Conclusion

- `PASS`: subtitle timeline lookup stays correct in ordered playback and seek jump scenarios.
