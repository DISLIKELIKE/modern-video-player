# CROSS_PLATFORM_PHASE6_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-601 ~ CP-605

## Commands

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv
.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv
.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check

$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
```

## Result
- Build: PASS
- DVD bitmap CLI:
  - `codec_name=dvd_subtitle`
  - `bitmap_item_count=1`
  - `bitmap_rect_count=1`
  - `mismatches=0`
  - `result=PASS`
- PGS bitmap CLI:
  - `codec_name=hdmv_pgs_subtitle`
  - `bitmap_item_count=1`
  - `bitmap_rect_count=1`
  - `ordered_checks=18`
  - `mismatches=0`
  - `result=PASS`
- Synthetic multi-rect stress:
  - `multi_rect_item_count=2`
  - `cache_reuse_candidate_count=2`
  - `max_active_item_count=3`
  - `max_active_rect_count=5`
  - `result=PASS`
- OpenGL gate:
  - `OpenGL gate result: PASS`
  - `23/23` checks passed

## Notes
- DVD sample is downloaded from FFmpeg sample corpus and already contains video + `dvd_subtitle`.
- PGS sample is downloaded from FFmpeg sample corpus and remuxed into local base media for playback regression.
- Linux-host bitmap subtitle playback still needs separate runtime evidence before claiming full platform-parity closure.

## Conclusion
- Windows-local Phase 6 acceptance is complete.
- Repository now has dedicated bitmap subtitle decode/model/composition/cache/stress regression coverage.
