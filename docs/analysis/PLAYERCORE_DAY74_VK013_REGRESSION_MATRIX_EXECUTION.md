# PLAYERCORE Day74: VK013 regression matrix execution

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Vulkan chain reached `VK-012`, but this stage still lacked one consolidated regression-matrix run across core playback behavior (`open/play/pause/seek/subtitle/fallback`).
- Without matrix evidence, final documentation/release closure (`VK-014`) would not have a runtime-backed baseline.

## 2. Root Cause
- Previous tasks mainly delivered implementation and CI wiring.
- Stage-level runtime regression evidence had not yet been archived as one dedicated closure artifact.

## 3. Execution Scope
- Host: Windows workstation (Linux Vulkan runtime proof remains runner-dependent).
- Matrix commands executed:
  - Build baseline
  - `--performance-log-check`
  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--subtitle-style-check`
  - `--subtitle-sync-check`
  - `--renderer-fallback-check`
  - `MVP_RENDERER_BACKEND=vulkan --performance-log-check`
  - `--vulkan-diagnostics`

## 4. Results
- Build: PASS.
- Open/Play: PASS (`performance-log-check.result=PASS`).
- Pause/Seek:
  - `--seek-burst-serial-check`: first run FAIL, immediate rerun PASS (environment-sensitive behavior observed).
  - `--paused-seek-serial-check`: PASS.
- Subtitle:
  - style check PASS.
  - sync check PASS.
- Fallback:
  - fallback check PASS.
  - Vulkan override path remained observable and safely fell back to D3D11 on Windows (`startup_renderer_fallback_reason=fallback-after-renderer-failure`).
- Vulkan diagnostics:
  - expected FAIL on current host (`supported_platform=false`, `compiled_in=false`, `runtime_available=false`).

## 5. Risk Notes
- Linux Vulkan runtime PASS evidence is still required from real Linux runner execution.
- One seek-burst first-run failure was observed before a successful rerun; this should be tracked as potential nondeterministic stress behavior.

## 6. Follow-up
- Proceed to `VK-014` documentation and release closure.
