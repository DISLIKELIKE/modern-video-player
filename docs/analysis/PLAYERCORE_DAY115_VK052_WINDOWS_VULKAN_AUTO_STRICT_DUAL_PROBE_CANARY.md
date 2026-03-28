# PLAYERCORE Day115: VK052 Windows Vulkan auto strict dual-probe canary

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- Auto-policy canary coverage already had:
  - no-probe optional path
  - native strict path
  - swiftshader strict path
- Dual-probe strict path (`native+swiftshader`) still lacked dedicated coverage.

## 2. Gap Snapshot
- Missing deterministic canary for:
  - `auto + sdk=1 + native_probe=1 + swiftshader_probe=1`
- Risk:
  - source-classification drift on combined probe branch.

## 3. Solution
- Added:
  - `tools/run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1`
- Integrated into:
  - `tools/run_windows_ci_gate.ps1`
- Canary validates strict PASS and source classification:
  - `strict_mode_auto_runtime_probe_source=native+swiftshader`

## 4. DoD
- Dual-probe canary passes.
- CI gate executes and reports dual-probe canary.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan auto-policy canary matrix now covers all runtime-probe source states:
  - none
  - native
  - swiftshader
  - native+swiftshader
