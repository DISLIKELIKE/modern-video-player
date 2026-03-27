# Cross-platform Vulkan Windows gate contract-canary Step Summary observability design (2026-03-27)

## 1. Scope
- Publish Windows Vulkan contract-canary result into GitHub Step Summary.

## 2. Non-goals
- Do not change canary pass/fail semantics.
- Do not change strict/optional policy logic for real Vulkan gate.
- Do not change Linux workflow path.

## 3. Design
- Workflow extension in Windows gate step:
  - after canary command execution, parse:
    - `logs/windows-vulkan-gate-contract-canary-summary.env`
  - render Step Summary table with key canary contract fields.
  - fallback message if file missing.
- Preserve existing canary fail-fast:
  - `if ($vulkanCanaryExitCode -ne 0) { throw ... }`
- Script extension:
  - `run_windows_vulkan_gate_contract_canary.ps1` now emits:
    - `windows-vulkan-contract-canary.gate_summary_file_present=true|false`

## 4. Summary Contract
- Step Summary section title:
  - `Windows Vulkan Gate Contract Canary`
- Table rows:
  - `result`
  - `expected_gate_exit_code`
  - `actual_gate_exit_code`
  - `gate_summary_file_present`
  - `gate_result`
  - `gate_failure_reason`
  - `gate_diag_contract_valid`
  - `validation_failure_reason`

## 5. Validation Strategy
- Build `Release`.
- Run baseline gate command (sanity regression).
- Run canary command and verify:
  - canary summary includes new `gate_summary_file_present` field.
  - canary result remains `PASS` on expected-fail contract.
- Generate local preview markdown from canary summary env and verify row values.
