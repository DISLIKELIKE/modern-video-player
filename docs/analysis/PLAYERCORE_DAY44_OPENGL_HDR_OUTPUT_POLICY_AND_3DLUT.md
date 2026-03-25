# PLAYERCORE Day44: OpenGL HDR output policy and 3D LUT output baseline
Date: 2026-03-25

## Background
- Day43 closed embedded subtitle + bitmap subtitle + DirectWrite custom collection.
- In the OpenGL backlog, HDR output and ICC/LUT remained the highest non-subtitle gap.
- Existing OpenGL color handling was still mostly internal conversion/tone-map logic, with no output-stage policy surface for runtime control and diagnostics.

## Problem
1. No explicit runtime policy switch to describe whether OpenGL output should stay SDR, follow auto policy, or force HDR-style handling.
2. No output-stage 3D LUT path to converge toward mature-player color-management style post-processing.
3. No machine-readable CLI check for "output color control plane is wired and active."

## Implementation Planner
1. Add OpenGL HDR output mode policy control (env + CLI override) and expose requested/active state to diagnostics.
2. Add `.cube` parser and OpenGL 3D LUT upload path with robust parse errors.
3. Wire LUT sampling into both OpenGL software-upload and native-interop final output paths.
4. Extend `RendererDiagnostics -> PlayerCore DiagnosticsSnapshot -> performance-log-check` output fields.
5. Add a dedicated regression CLI (`--opengl-output-color-check`) and update reports/plans/records.

## Implementation

### 1) HDR output policy surface (control plane)
- Added environment variable:
  - `MVP_OPENGL_HDR_OUTPUT_MODE=auto|off|force`
- Added playback CLI:
  - `--opengl-hdr-output-mode <auto|off|force>`
- Added OpenGL diagnostics fields:
  - `opengl_hdr_bridge_requested`
  - `opengl_hdr_bridge_active`
  - `opengl_hdr_bridge_mode`
  - `opengl_hdr_bridge_decision`
- Shader control constants now carry HDR mode selection to the output stage.

### 2) 3D LUT output pipeline
- Added environment variable:
  - `MVP_OPENGL_3DLUT_FILE=<cube_file>`
- Added playback CLI:
  - `--opengl-3dlut <cube_lut_file>`
- Implemented `.cube` parsing support:
  - `LUT_3D_SIZE`
  - `DOMAIN_MIN`
  - `DOMAIN_MAX`
  - sample table parsing + validation
- Added OpenGL 3D texture allocation/upload and LUT sampling path in output shading.

### 3) Native interop final output path convergence
- Added LUT-aware RGB post output path for native interop final compositing.
- Ensured LUT controls apply consistently in final output pass, not only in one upload branch.

### 4) Diagnostics convergence
- Extended `RendererDiagnostics` and `DiagnosticsSnapshot` with OpenGL HDR/LUT fields.
- Extended `--performance-log-check` output:
  - `renderer_opengl_hdr_bridge_*`
  - `renderer_opengl_output_lut_*`

### 5) Regression CLI
- Added:
  - `--opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`
- Output is machine-readable and reports both policy and LUT status fields.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --opengl-output-color-check .\juren-30s.mp4 .\samples\lut\identity_2.cube 1200

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_3DLUT_FILE='.\samples\lut\identity_2.cube'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

Key expected lines:
- `opengl-output-color-check.output_lut_active=true`
- `opengl-output-color-check.hdr_bridge_mode=auto`
- `performance-log-check.renderer_opengl_output_lut_active=true`
- `OpenGL gate result: PASS`

## Remaining gaps (explicitly not closed in Day44)
- Full display-level HDR present bridge is still pending:
  - DXGI HDR swapchain path
  - `SetColorSpace1` / `SetHDRMetaData` output integration
- ICC/profile driven LUT generation and automatic per-display binding are still pending.
- This day closes output policy + LUT plumbing baseline, not the final HDR delivery architecture.

## Modified Files
- `src/render/opengl_video_renderer.cpp`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY44_OPENGL_HDR_OUTPUT_POLICY_AND_3DLUT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
