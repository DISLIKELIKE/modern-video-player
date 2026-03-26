# PLAYERCORE Day54: CP-801 and Phase 9 observability / CI / packaging closure
Date: 2026-03-26

## Background
- Phase 8 had already landed `CP-802 ~ CP-805`:
  - ICC/profile-driven LUT generation
  - per-display output binding
  - runtime output diagnostics
  - OpenGL output-color gate coverage
- The remaining Phase 8 gap was `CP-801`: the real Windows DXGI HDR present bridge on the D3D11 renderer path.
- Phase 9 (`CP-901 ~ CP-905`) also remained open:
  - driver quirk sample capture
  - unified runtime counters in machine-readable CLI
  - Linux gate parity
  - CI matrix
  - dual-platform packaging/readiness closure

## Problem
1. D3D11 playback still exposed only SDR swapchain behavior even when the renderer had enough diagnostics context to detect HDR displays.
2. Newly added scheduler / present counters were not fully surfaced through CLI and validation commands.
3. Driver quirk knowledge still lived mostly in logs and narrative docs, not in a reusable sample library.
4. Linux gate automation was much thinner than the Windows gate and relied mostly on exit codes.
5. Dual-platform delivery lacked one explicit CI + packaging + readiness closure point.

## Implementation Planner
1. Finish `CP-801` on the real D3D11 render path.
   - dependency: existing Phase 8 output-binding/runtime diagnostics state
2. Close CLI diagnostics and self-checks.
   - dependency: step 1
3. Surface unified counters in `--performance-log-check`.
   - dependency: step 2
4. Add structured driver sample capture and seed the first library rows.
   - dependency: step 2
5. Raise Linux gate script to pattern-checked PASS/FAIL parity.
   - dependency: step 3
6. Add GitHub Actions Windows/Linux matrix gate and dual-platform packaging helpers/checklist.
   - dependency: steps 4-5
7. Sync records and local validation reports.
   - dependency: steps 1-6

## Implementation
### 1) D3D11 DXGI HDR present bridge
- Completed the actual runtime bridge in `D3D11VideoRenderer`:
  - HDR-aware shader path for `BT.2020 / PQ / HLG`
  - runtime output probe using DXGI output state
  - swapchain format/color-space switching
  - HDR10 metadata forwarding through `SetHDRMetaData`
  - present timing / failure counters
- Chosen present strategy:
  - float backbuffer (`R16G16B16A16_FLOAT`) when HDR present is active
  - SDR fallback (`B8G8R8A8_UNORM`) otherwise
- This keeps subtitle / overlay composition compatible with the existing D2D-based overlay model.

### 2) CLI and diagnostics closure
- Added new playback CLI override:
  - `--d3d11-hdr-output-mode <auto|off|force>`
- Extended `--d3d11-diagnostics` with `hdr_output.*` fields.
- Added dedicated self-check:
  - `--d3d11-hdr-output-check <media_file> [sample_ms]`
- The self-check validates:
  - playback opens and renders through `D3D11`
  - HDR decision is evaluated
  - HDR becomes active when both HDR content and HDR-capable output are present
  - otherwise the inactive decision is still explicit and machine-readable

### 3) Unified runtime counters
- Surfaced missing Phase 9 fields in `--performance-log-check`:
  - `scheduler_render_wait_ms`
  - `runtime_drop_total`
  - `runtime_drop_summary`
  - D3D11 HDR present runtime state
  - D3D11 present timing (`present_count`, `present_failures`, `present_time_ms`, `present_avg_ms`, `present_time_us_max`)
- Existing queue and backpressure counters remained the canonical decode-wait / frame-queue signals.

### 4) Driver quirk sample library
- Added `tools/collect_driver_quirk_sample.ps1`.
- Added a structured CSV library:
  - `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
- The script captures:
  - adapter / driver identifiers
  - backend (`OpenGL` / `D3D11`)
  - native-path policy state
  - HDR probe state
  - OpenGL GL vendor / renderer / version
- Seeded the library with two real local rows from the current validation host:
  - `OpenGL` / `NVIDIA GeForce GTX 1080`
  - `D3D11` / `NVIDIA GeForce GTX 1080`

### 5) Linux gate parity
- Reworked `tools/run_linux_mvp_checks.sh` from exit-code-only smoke coverage into a pattern-checked gate.
- The Linux gate now validates:
  - `CP-902` observability baseline via `--performance-log-check`
  - `CP-401 ~ CP-406` results with required machine-readable fields
- Output format now matches the Windows gate model better:
  - `[n/N]` stage markers
  - command echo
  - summary lines
  - missing-pattern diagnostics
  - PASS/FAIL status

### 6) CI matrix and packaging/readiness closure
- Added `.github/workflows/cross-platform-gate.yml`:
  - Windows matrix leg:
    - configure/build
    - `--d3d11-diagnostics`
    - `--d3d11-hdr-output-check`
    - D3D11 `--performance-log-check`
    - `tools/run_opengl_checks.ps1`
    - driver sample collection
    - Windows ZIP packaging
  - Linux matrix leg:
    - configure/build
    - `xvfb-run` Linux MVP gate
    - Linux package generation (`DEB/TGZ`)
- Added `tools/package_windows.ps1` so Windows packaging is now scripted like Linux packaging already was.

## Validation
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --d3d11-diagnostics

.\build\Release\modern-video-player.exe --d3d11-hdr-output-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200

$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200

powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -OutputCsvPath .\docs\reference\DRIVER_QUIRK_SAMPLE_LIBRARY.csv -HostLabel local-win-gtx1080 -Notes "Local Windows validation sample after CP-801 and Phase-9 closure"

powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release -SkipBuild
```

Key observed results:
- `d3d11-diagnostics.result=PASS`
- `d3d11-hdr-output-check.result=PASS`
- `performance-log-check.result=PASS`
- `driver-quirk-sample.record_count=2`
- `modern-video-player-1.0.0-rc1-windows-x64.zip` generated successfully

## Environment Limitation
- The local Windows validation host reports:
  - `d3d11-diagnostics.hdr_output.output_found=false`
  - `d3d11-hdr-output-check.hdr_present_decision=probe-unavailable`
- This means the current workstation did not expose a DXGI output that the probe could bind to.
- Therefore, this round validates:
  - the real D3D11 HDR decision/present code path compiles and runs
  - the runtime decision is explicit and machine-readable
  - present counters and diagnostics are surfaced correctly
- It does not claim a locally observed `hdr_present_active=true` result on this workstation.

## Modified Files
- `src/main.cpp`
- `include/render/d3d11_video_renderer.h`
- `include/render/video_renderer.h`
- `src/render/d3d11_video_renderer.cpp`
- `include/core/scheduler.h`
- `src/core/scheduler.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `tools/run_linux_mvp_checks.sh`
- `tools/collect_driver_quirk_sample.ps1`
- `tools/package_windows.ps1`
- `.github/workflows/cross-platform-gate.yml`
- `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
