# PLAYERCORE Day53: Phase 8 ICC output LUT, per-display binding, and output diagnostics closure
Date: 2026-03-26

## Background
- Day44 closed the OpenGL HDR policy + manual `.cube` LUT control plane.
- Phase 8 still lacked runtime color-management closure:
  - no ICC/profile-driven LUT generation
  - no per-display output binding / reload strategy
  - no machine-readable runtime diagnostics for display binding and LUT source
- `CP-801` (true DXGI display-HDR present bridge) remained open and was not safe to fake with policy flags.

## Problem
1. OpenGL output LUT activation still depended on a manually supplied `.cube` file only.
2. Output color state was initialized once and did not follow the display the SDL window was currently bound to.
3. Diagnostics could not explain which display, ICC profile, and LUT source were active during playback.
4. Regression coverage did not distinguish manual LUT validation from ICC auto-binding validation.

## Implementation Planner
1. Extract reusable output-color helper code for `.cube` parsing and ICC profile -> 3D LUT generation.
2. Resolve current window display binding and display ICC profile on the Windows path.
3. Rebuild output LUT when display binding changes or when ICC/manual LUT source changes.
4. Export display binding / ICC / LUT runtime fields through renderer diagnostics and CLI checks.
5. Extend OpenGL gate coverage to include manual cube and auto ICC output-color regressions.

## Implementation
### 1) Reusable output-color helper
- Added `render/output_color_profile` helper with:
  - `.cube` parsing (existing behavior preserved)
  - ICC matrix/TRC profile parsing
  - sampled 3D LUT generation for RGB matrix/shaper monitor profiles
- The ICC path now reads:
  - profile description (`desc` / `mluc`)
  - RGB matrix tags (`rXYZ`, `gXYZ`, `bXYZ`)
  - tone-response tags (`rTRC`, `gTRC`, `bTRC`)

### 2) Per-display binding
- Added window/display binding tracking in `OpenGLVideoRenderer`.
- Runtime now records:
  - SDL display index / display name
  - Windows monitor device name
  - active ICC profile path when auto ICC is enabled and available
- Output binding is marked dirty on:
  - resize / restore / maximize
  - move / display change
  - fullscreen transition

### 3) ICC/profile-driven LUT generation
- Added new env / CLI surfaces:
  - `MVP_OPENGL_ICC_PROFILE_FILE=<icc_profile_file>`
  - `MVP_OPENGL_AUTO_ICC=1`
  - `--opengl-icc-profile <icc_profile_file>`
  - `--opengl-auto-icc`
- Source selection policy is now deterministic:
  1. manual `.cube`
  2. manual ICC profile
  3. auto ICC from current display
  4. none

### 4) Diagnostics closure
- Extended `RendererDiagnostics` / `DiagnosticsSnapshot` / `performance-log-check` with:
  - output display index / display name / display device name
  - ICC profile availability / source / path / description
  - output LUT source / reload count / binding error
- Extended `--opengl-diagnostics` with `opengl-diagnostics.output_display.*`.
- Extended `--opengl-output-color-check` to print output display + ICC/LUT source details.
- Added dedicated regression CLI:
  - `--opengl-output-color-icc-check <media_file> [sample_ms]`

### 5) Regression coverage
- Updated `tools/run_opengl_checks.ps1` to `25` checks.
- Added two dedicated output-color regressions:
  - manual cube regression
  - auto ICC regression

## Validation
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200

.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200

.\build\Release\modern-video-player.exe --opengl-diagnostics

$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
```

Key observed results:
- manual LUT: `output_lut_source=cube`, `result=PASS`
- auto ICC: `output_lut_source=icc-display`, `output_icc_profile_description=sRGB IEC61966-2.1`, `result=PASS`
- diagnostics: `output_display.icc_profile_available=true`
- OpenGL gate: `PASS (25/25)`

## Remaining Gap
- `CP-801` remains open.
- This round closes `CP-802 ~ CP-805`; it does not claim the final Windows DXGI display-HDR present bridge.
- The remaining work is the real present path (`SetColorSpace1` / `SetHDRMetaData` + swap/present bridge), not more policy plumbing.

## Modified Files
- `include/render/output_color_profile.h`
- `src/render/output_color_profile.cpp`
- `src/render/opengl_video_renderer.cpp`
- `include/render/opengl_video_renderer.h`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY53_CP802_CP805_ICC_OUTPUT_BINDING_AND_OUTPUT_DIAGNOSTICS.md`
