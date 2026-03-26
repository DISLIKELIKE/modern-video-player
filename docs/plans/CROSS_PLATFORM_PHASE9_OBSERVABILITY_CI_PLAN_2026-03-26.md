# Cross-Platform Phase 9 Observability and CI Closure Plan
Date: 2026-03-26
Status: Landed
Scope: `CP-801`, `CP-901 ~ CP-905`

## Scope
- Close the remaining Phase 8 item (`CP-801`) on the real D3D11 DXGI path.
- Close Phase 9 observability, Linux gate parity, CI matrix, and dual-platform packaging readiness.

## Dependency-Ordered Steps
1. Finish D3D11 HDR present runtime and expose it through diagnostics.
   - dependency: existing Phase 8 output-binding and diagnostics work
2. Close CLI surface:
   - `--d3d11-diagnostics`
   - `--d3d11-hdr-output-check`
   - `--performance-log-check`
   - dependency: step 1
3. Add reusable driver sample capture + CSV library.
   - dependency: step 2
4. Raise Linux gate to required-pattern parity with the Windows gate.
   - dependency: step 2
5. Add Windows/Linux CI matrix and packaging helpers.
   - dependency: steps 3-4
6. Sync task board, records, and local reports.
   - dependency: steps 1-5

## Landed Result
- Step 1: done
- Step 2: done
- Step 3: done
- Step 4: done
- Step 5: done
- Step 6: done

## Deliverables
- D3D11 HDR runtime diagnostics and self-check
- unified performance-log counter surface
- `tools/collect_driver_quirk_sample.ps1`
- `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
- upgraded `tools/run_linux_mvp_checks.sh`
- `.github/workflows/cross-platform-gate.yml`
- `tools/package_windows.ps1`
- updated Phase 8 / Phase 9 reports and records

## Residual Risk
- Local Windows validation host has no bindable DXGI output, so HDR-active present was not observed locally.
- Linux gate and CI workflow are implemented but cannot be executed end-to-end on this Windows workstation.
