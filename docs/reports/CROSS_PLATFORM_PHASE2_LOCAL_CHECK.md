# CROSS_PLATFORM_PHASE2_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-201 ~ CP-205

## Commands

```powershell
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## Result

- Build: PASS
- `performance-log-check.result=PASS`
- `renderer-fallback-check.result=PASS`
- `interaction-freeze-check.result=PASS`
- `run_opengl_checks.ps1`: `OpenGL gate result: PASS`

## Key Signals

- `performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL`
- `performance-log-check.startup_selected_renderer=D3D11`
- `renderer-fallback-check.renderer_backend=SoftwareSDL`
- `renderer-fallback-check.fallback_to_sdl=true`
- `interaction-freeze-check.injected_events_total=251`
- `interaction-freeze-check.render_progress_ok=true`
- `interaction-freeze-check.illegal_transition_ok=true`

## Notes

- Local machine has no active WASAPI default render endpoint, so audio preflight warnings are expected.
- Checks still pass with `video_only_fallback=true`, and this does not affect CP-201~205 acceptance criteria.

## Conclusion

- CP-201~CP-205 are closed on local Windows gate:
  - renderer/input/overlay responsibilities are split and consumed by role interfaces
  - main-thread event-pump guards are in place
  - interaction freeze stress is machine-verifiable and integrated into OpenGL gate
