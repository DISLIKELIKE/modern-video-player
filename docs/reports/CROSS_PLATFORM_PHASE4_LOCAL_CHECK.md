# CROSS_PLATFORM_PHASE4_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-401 ~ CP-406

## Commands

```powershell
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release

.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
.\build\Release\modern-video-player.exe --renderer-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4
.\build\Release\modern-video-player.exe --interaction-freeze-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800

.\build\Release\modern-video-player.exe --linux-software-audio-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
.\build\Release\modern-video-player.exe --linux-opengl-playback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
.\build\Release\modern-video-player.exe --linux-opengl-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
.\build\Release\modern-video-player.exe --linux-audio-backend-smoke .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
.\build\Release\modern-video-player.exe --core-playback-behavior-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
.\build\Release\modern-video-player.exe --ui-interaction-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1800
```

Linux host gate baseline:

```bash
bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800
```

## Result

- Build: PASS
- Windows regression sanity:
  - `performance-log-check.result=PASS`
  - `renderer-fallback-check.result=PASS`
  - `interaction-freeze-check.result=PASS`
- Linux-specific Phase4 commands on Windows:
  - each command returns `platform=Windows` and `platform_ok=false`
  - each Linux-only command exits `2` with `result=FAIL` (expected on non-Linux host)
  - CLI parsing/dispatch path confirmed working

## Notes

- Phase4 commands are Linux-targeted and include strict `startup_platform=Linux` checks, so Windows execution is expected to fail by design.
- Linux host `PASS` execution is still required as final cross-platform acceptance evidence.

## Conclusion

- Windows local sanity is complete; Linux-targeted checks are dispatch-valid but require Linux host runtime for full PASS closure.
