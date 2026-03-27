# Cross-platform Vulkan Windows playback contract validation design (2026-03-27)

## 1. Goal
Detect and classify malformed playback-check outputs as contract failures, not generic playback failures.

## 2. Design

### 2.1 Required playback keys
- File: `tools/run_windows_vulkan_checks.ps1`
- Validate required keys when playback check executes:
  - `performance-log-check.result`
  - `performance-log-check.startup_selected_renderer`
  - `performance-log-check.renderer_backend`
  - `performance-log-check.startup_renderer_candidates`
  - `performance-log-check.startup_renderer_plan_reason`

### 2.2 Contract-broken behavior
- If required playback keys are missing:
  - `result=FAIL`
  - `failure_reason=vulkan-playback-contract-broken`
  - `playback_failure_detail=contract-missing-required-fields`

### 2.3 New summary fields
- `windows-vulkan-check.playback_contract_valid`
- `windows-vulkan-check.playback_missing_required_fields`
- `windows-vulkan-check.playback_failure_detail`

### 2.4 Compatibility
- For paths where playback is not executed, fields emit:
  - `playback_contract_valid=n/a`
  - `playback_missing_required_fields=n/a`
  - `playback_failure_detail=not-executed`
- Existing result/failure contracts for strict/optional availability paths remain unchanged.

## 3. Risk and mitigation
- Risk: stricter contract checking may fail previously tolerated malformed outputs.
- Mitigation: intentional fail-fast to avoid silent false positives and improve CI signal integrity.
