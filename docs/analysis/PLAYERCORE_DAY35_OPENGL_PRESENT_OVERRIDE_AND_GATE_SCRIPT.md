# PlayerCore Day 35: OpenGL present override and gate script

Date: 2026-03-25

## Summary
- Added `MVP_OPENGL_PRESENT_MODE=auto|paced|immediate` so field diagnostics can switch OpenGL present pacing without editing code.
- Exported both the requested mode and the actually active mode into `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to gate the main OpenGL playback paths in one command.

## Problem
- After the present pacing fix, the runtime still lacked an explicit operator control to force immediate presents during现场排障.
- OpenGL regression commands were scattered across notes and terminal history instead of being collected into a reusable PASS/FAIL gate.

## Root Cause
1. Swap interval behavior was hard-coded in the renderer startup path.
2. The final diagnostics snapshot did not expose whether the renderer was actually running in paced or immediate mode.
3. Regressions across diagnostics/native/copy-back/10-bit/subtitle remained manual and easy to skip.

## Changes
### Present override
- Added `MVP_OPENGL_PRESENT_MODE` parsing with `auto`, `paced`, and `immediate`.
- Logged startup present-mode selection as `[diag:opengl-present] requested=... active=...`.
- Exported `renderer_opengl_present_mode_requested` and `renderer_opengl_present_mode_active` into `RendererDiagnostics`, `PlayerCore::DiagnosticsSnapshot`, and `--performance-log-check`.

### OpenGL gate script
- Added `tools/run_opengl_checks.ps1`.
- The script runs:
  1. `--opengl-diagnostics`
  2. OpenGL native `--performance-log-check`
  3. OpenGL forced copy-back `--performance-log-check`
  4. OpenGL immediate-present `--performance-log-check`
  5. OpenGL 10-bit forced copy-back `--performance-log-check`
  6. OpenGL subtitle `--delay-adjust-check`
- The script auto-generates a temporary 10-bit HEVC sample under `build/tmp` when none is provided.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_PRESENT_MODE='immediate'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## Key Results
- `auto` mode validation:
  - `renderer_opengl_present_mode_requested=auto`
  - `renderer_opengl_present_mode_active=paced`
  - `renderer_opengl_present_wait_timeouts=0`
  - `result=PASS`
- `immediate` mode validation:
  - `[diag:opengl-present] requested=immediate active=immediate`
  - `renderer_opengl_present_mode_requested=immediate`
  - `renderer_opengl_present_mode_active=immediate`
  - `result=PASS`
- Gate script:
  - `OpenGL gate result: PASS`

## Files
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY35_OPENGL_PRESENT_OVERRIDE_AND_GATE_SCRIPT.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
