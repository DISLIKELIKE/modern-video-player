# PLAYERCORE Day96: VK034 Windows Vulkan optional-unavailable skip canary

Date: 2026-03-27  
Status: In Progress

## 1. Problem
- Windows Vulkan gate currently has deterministic canary coverage for:
  - diagnostics contract expected-fail (`VK-029`)
  - playback contract expected-fail (`VK-031`)
  - strict-path PASS (`VK-032`)
  - strict-unavailable expected-fail (`VK-033`)
- Optional policy unavailable branch (`result=SKIPPED`) still has no deterministic canary.

## 2. Gap Snapshot
- If optional-mode skip semantics regress, CI may not detect it deterministically.
- Real runner Vulkan availability varies, so normal gate output cannot reliably prove optional-skip contract stability.

## 3. Solution Direction
- Add deterministic optional-unavailable skip canary script:
  - `tools/run_windows_vulkan_gate_optional_skip_canary.ps1`
- Script creates mock executable with contract-valid diagnostics and unavailable probe state:
  - `supported_platform=true`
  - `compiled_in=false`
  - `runtime_available=false`
  - `result=FAIL`
- Canary runs gate without strict override and validates:
  - gate exit code `0`
  - `windows-vulkan-check.result=SKIPPED`
  - `windows-vulkan-check.mode=optional`
  - `windows-vulkan-check.skip_reason=vulkan-not-available`
  - `windows-vulkan-check.failure_reason` is empty
  - `windows-vulkan-check.playback_check_executed=false`
- Integrate canary into Windows workflow with fail-fast and Step Summary.

## 4. DoD
- New optional-skip canary script exists and outputs machine-readable fields.
- Workflow executes optional-skip canary and fails on non-zero.
- Step Summary exposes optional-skip canary key fields.
- Local build + baseline gate + full Vulkan canary set pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round validates optional skip contract semantics only; it does not prove real Vulkan runtime availability.
- Real strict PASS proof remains dependent on Vulkan-ready runner/workstation.
