# PLAYERCORE Day45: OpenGL interaction freeze fix (event-thread affinity)
Date: 2026-03-25

## Problem
- User reported that OpenGL playback could freeze after 1-2 seconds once mouse moved/clicked.
- After freeze:
  - hotkeys no longer responded
  - maximize/minimize/fullscreen actions became unresponsive
  - UI looked hard-stalled (not just a temporary frame drop)

## Root Cause
The OpenGL backend had SDL event pumping on the render thread:
- `renderLoop()` called `pumpEvents()` internally.
- `SDL_PollEvent` and window-state interaction were not handled on the main thread.

This differs from the D3D11 path and violates practical SDL thread-affinity expectations on Windows under heavy mouse/window event traffic, which can manifest as message-pump stalls and apparent app deadlock.

## Implementation Planner
1. Move OpenGL SDL event pumping to `handleEvents()` (main-thread call path).
2. Keep render thread focused on frame processing/present, not event polling.
3. Process fullscreen toggle request from `handleEvents()` after pumping events.
4. Rebuild and run OpenGL regression gates.
5. Sync records/docs.

## Changes
- `src/render/opengl_video_renderer.cpp`
  - Changed `Impl::handleEvents()` from empty stub to real main-thread event pump.
  - Moved fullscreen toggle execution (`SDL_SetWindowFullscreen`) into `handleEvents()` path.
  - Removed `pumpEvents()` and fullscreen-window operations from `renderLoop()`.
  - Simplified render-loop wake predicate by removing fullscreen toggle from render-thread wait conditions.

## Validation
```powershell
# Build
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player

# OpenGL playback diagnostics check
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200

# OpenGL regression gate
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

Results:
- Release build: PASS
- `performance-log-check.result=PASS`
- OpenGL gate: `OpenGL gate result: PASS (16/16)`

## Residual Risk / Follow-up
- Automated checks cannot synthesize real interactive mouse stress on your desktop environment.
- Manual GUI smoke is still required for final closure:
  - continuous mouse move over control bar
  - click/drag progress and volume
  - hotkey spam (`Space`, arrows, `Enter`)
  - maximize/minimize/restore/fullscreen round-trips

## Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY45_OPENGL_EVENT_THREAD_AFFINITY_FREEZE_FIX.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
