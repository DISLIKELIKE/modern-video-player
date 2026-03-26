# Cross-Platform Phase0 + Phase7 Linux VAAPI Execution Plan

Date: 2026-03-26  
Scope: `CP-001~CP-005`, `CP-701~CP-705` (Windows + Linux scope, macOS deferred)

## 1. Objectives
- Close Phase-0 baseline freeze tasks.
- Close Linux hardware decode baseline tasks for VAAPI capability/fallback/copy-back.

## 2. Planned Steps
1. Add unified hardware device factory (`CP-701`).
2. Wire VAAPI runtime probing into platform capabilities (`CP-702`).
3. Refactor `PlayerCore` to use generic hardware decode setup (`CP-703`).
4. Add Linux VAAPI fallback/copy-back CLI check and gate script stage (`CP-704`).
5. Produce zero-copy evaluation conclusion and keep copy-back baseline for this phase (`CP-705`).
6. Freeze baseline docs for platform matrix/sample matrix/coupling inventory/regression/risk (`CP-001~CP-005`).

## 3. Delivered Result
- Steps 1~6 completed in code/docs.
- Master tasklist status updated:
  - `CP-001~CP-005`: `DONE`
  - `CP-701~CP-705`: `DONE`
- Scope note updated to explicit macOS deferment for current execution.

## 4. Validation Plan
- Build on local Windows host.
- Regression sanity on Windows commands:
  - `--performance-log-check`
  - `--d3d11-diagnostics`
- Non-Linux host behavior check:
  - `--linux-vaapi-fallback-check` should fail with `platform=NonLinux`.

## 5. Open Validation Gap
- Linux host execution is still required to close runtime evidence for:
  - VAAPI runtime availability on target Linux hardware/driver stack.
  - Linux gate end-to-end `PASS` including VAAPI stage.
