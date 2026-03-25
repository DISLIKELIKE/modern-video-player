# PlayerCore Day 34: OpenGL runtime diagnostics export and P010/P016 direct upload

Date: 2026-03-25

## Summary
- Completed the runtime export path for the new OpenGL renderer diagnostics counters so `--performance-log-check` now reports native-interop state, disable rule, native-frame count, disable events, and present wait timeouts.
- Extended the OpenGL software/direct upload path to accept `p010le` and `p016le` frames without forcing an extra `swscale -> yuv420p` downgrade.
- Verified that `D3D11VA -> copyback -> OpenGL` now keeps the 10-bit semi-planar software surface as `p010le` on the OpenGL path.

## Problem
- The renderer-side counters for OpenGL native interop had been added, but they still needed end-to-end verification in the machine-readable `--performance-log-check` output.
- OpenGL software upload only accepted `yuv420p` and `nv12`, so once native interop was disabled or unavailable, 10-bit copy-back frames could still be reduced to 8-bit through `swscale`.

## Root Cause
1. The diagnostics export had to be validated across renderer snapshot -> `PlayerCore` diagnostics snapshot -> `main.cpp` reporting.
2. `OpenGLVideoRenderer::Impl::copyFrameData()` and the GL texture upload path had no support for 16-bit semi-planar formats.
3. The color-matrix setup for software upload assumed 8-bit normalized texture sampling only.

## Changes
### Runtime diagnostics export
- Confirmed `renderer_opengl_native_interop_*` and `renderer_opengl_present_wait_timeouts` are emitted by `--performance-log-check`.
- Re-validated both native and forced-copyback OpenGL paths using the new counters.

### P010/P016 software upload
- Added OpenGL direct software-frame support for `AV_PIX_FMT_P010LE` and `AV_PIX_FMT_P016LE`.
- Allocated 16-bit luminance / luminance-alpha GL textures for semi-planar 10/16-bit upload.
- Switched software color coefficient generation to support both 8-bit normalized and 16-bit normalized sampling.
- Extended OpenGL color diagnostics to print the software input format, which now makes `path=software format=p010le` visible during validation.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200
```

## Key Results
- Native OpenGL path:
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames > 0`
  - `video_copy_back_frames = 0`
  - `video_swscale_frames = 0`
  - `renderer_opengl_native_interop_active=true`
  - `result=PASS`
- Forced copy-back on `juren-30s.mp4`:
  - `video_native_output_frames = 0`
  - `video_copy_back_frames > 0`
  - `renderer_opengl_native_interop_active=false`
  - `renderer_opengl_present_wait_timeouts=0`
  - `result=PASS`
- Forced copy-back on 10-bit HEVC sample:
  - `decoder_backend=D3D11VA`
  - `video_copy_back_frames=72`
  - `video_swscale_frames=0`
  - OpenGL log: `[diag:opengl-color] path=software format=p010le ...`
  - `result=PASS`

## Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY34_OPENGL_RUNTIME_DIAGNOSTICS_AND_P010_UPLOAD.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
