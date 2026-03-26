# CROSS_PLATFORM_PHASE5_BACKLOG_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-507, CP-508

## Commands

```powershell
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --linux-vaapi-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200

.\build\Release\modern-video-player.exe --libass-shaping-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
```

## Result

- Build: PASS
- `performance-log-check.result=PASS`
- `d3d11-diagnostics.result=PASS`
- `linux-vaapi-fallback-check.result=FAIL` (expected on non-Linux host)
- `libass-shaping-check.result=FAIL` (expected on non-Linux host)
- `embedded-subtitle-live-packet-check.result=PASS`

## Key Signals

- `libass-shaping-check.platform=NonLinux`
- `libass-shaping-check.platform_ok=false`
- `embedded-subtitle-live-packet-check.subtitle_stream_found=true`
- `embedded-subtitle-live-packet-check.supported_stream_found=true`
- `embedded-subtitle-live-packet-check.stream_index=2`
- `embedded-subtitle-live-packet-check.codec=ass`
- `embedded-subtitle-live-packet-check.subtitle_packets_read=3`
- `embedded-subtitle-live-packet-check.monotonic_timestamps=true`
- `embedded-subtitle-live-packet-check.produced_output=true`

## Notes

- This workstation cannot provide Linux runtime PASS evidence for `--libass-shaping-check`.
- Full `tools/run_linux_mvp_checks.sh` verification (including new optional `CP-507/CP-508` stages) still requires Linux host execution.

## Conclusion

- `CP-508` packet-level probe path is locally validated.
- `CP-507` command wiring is validated, and Linux runtime execution is the remaining environment-bound verification step.

