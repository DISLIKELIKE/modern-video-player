# Playlist Flow Local Check

Generated: 2026-03-08
Task: `1.4.2 播放列表连续播放 5 文件通过`

## Command

```powershell
.\build\Debug\modern-video-player.exe --playlist-flow-check `
  samples\mp4\demo__hevc_aac__1920x1080__30fps__2ch.mp4 `
  samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv `
  samples\webm\demo__vp9_opus__1920x1080__30fps__2ch.webm `
  samples\flv\demo__h264_aac__1280x720__30fps__2ch.flv `
  samples\ts\demo__h264_aac__1920x1080__25fps__2ch.ts
```

## Output

```text
playlist-flow-check.required_entries=5
playlist-flow-check.total_entries=5
playlist-flow-check.checked_entries=5
playlist-flow-check.open_passes=5
playlist-flow-check.open_failed_indices=-
playlist-flow-check.sequence=0,1,2,3,4
playlist-flow-check.sequence_ok=true
playlist-flow-check.result=PASS
```

## Conclusion

- `PASS`: 5 entries were opened successfully and EOF auto-next flow covered `0 -> 1 -> 2 -> 3 -> 4`.
