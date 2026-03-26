# PlayerCore Day55: CP-001~CP-005 and CP-701~CP-705 Linux VAAPI closure

Date: 2026-03-26  
Scope: Windows + Linux execution scope (macOS deferred in this round)

## 1. Problem Statement
- Phase-0 baseline freeze tasks (`CP-001~CP-005`) were still marked `NEXT`, so the cross-platform boundary was not formally frozen.
- Linux Phase-7 hardware decode tasks (`CP-701~CP-705`) remained open:
  - no unified hardware-device factory abstraction;
  - VAAPI capability stayed at compile-switch level (`runtime=false`);
  - `PlayerCore` decode setup only implemented `D3D11VA` hardware path;
  - no Linux gate command for `VAAPI -> Software` fallback/copy-back baseline.

## 2. Root Cause
- Earlier phases prioritized architecture/render/subtitle/HDR/CI closure; Linux hardware decode was explicitly deferred.
- Hardware device creation logic was still embedded in `PlayerCore` and Windows-centric.
- Runtime capability surface and regression commands did not include VAAPI lifecycle evidence.

## 3. Changes Introduced
- Added unified hardware device factory:
  - `include/platform/hw_device_factory.h`
  - `src/platform/hw_device_factory.cpp`
- Extended platform capability probe:
  - VAAPI now uses runtime probe via FFmpeg HW device context creation (`runtime=true|false` on Linux host).
- Refactored decoder hardware setup in `PlayerCore`:
  - replaced hardcoded D3D11-only setup with generic `tryConfigureHardwareDecode(...)`;
  - added VAAPI decode candidate handling (`VAAPI -> Software` fallback);
  - generalized hardware backend checks and downgrade logs;
  - kept D3D11 shader-resource frames context path unchanged for Windows.
- Added Linux VAAPI regression command:
  - `--linux-vaapi-fallback-check <media_file> [sample_ms]`
- Upgraded Linux gate script:
  - `tools/run_linux_mvp_checks.sh` includes VAAPI fallback/copy-back baseline stage.
- Completed Phase-0 boundary freeze docs/tasks and updated master tasklist statuses.
- `CP-705` zero-copy evaluation conclusion:
  - keep copy-back baseline in current phase;
  - defer dmabuf/EGL zero-copy implementation to dedicated future phase.

## 4. Validation Status
- Local Windows build: PASS.
- Existing regression command (`--performance-log-check`): PASS.
- Existing D3D11 diagnostics: PASS.
- New Linux-only command on non-Linux host:
  - returns `platform=NonLinux`, `result=FAIL` as designed.

## 5. Risk / Limitations
- No Linux runtime environment on this workstation, so Linux host execution evidence remains pending for:
  - actual VAAPI runtime availability;
  - real `VAAPI -> Software` fallback path under Linux host conditions;
  - Linux gate script end-to-end PASS.
- Zero-copy (`dmabuf/EGL`) is evaluated but intentionally not implemented in this round.
