# PLAYERCORE Day114: VK051 Windows Vulkan auto optional no-probe canary

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- Auto strict source branches (`native` / `swiftshader`) were already canary-covered.
- But auto downgrade branch (`runtime probe unavailable`) had no dedicated canary.

## 2. Gap Snapshot
- Missing deterministic coverage for:
  - `auto + sdk=1 + native_probe=0 + swiftshader_probe=0`
- Risk:
  - unexpected strict promotion regression in no-probe hosts.

## 3. Solution
- Added:
  - `tools/run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1`
- Integrated canary into:
  - `tools/run_windows_ci_gate.ps1`
- Added Step Summary reporting for this branch.

## 4. DoD
- Auto optional no-probe canary passes with expected optional/skip outcome.
- Native and SwiftShader auto strict canaries still pass.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan auto-policy canary matrix now covers:
  - no-probe downgrade (`optional`)
  - native-probe promotion (`strict`)
  - swiftshader-probe promotion (`strict`)
