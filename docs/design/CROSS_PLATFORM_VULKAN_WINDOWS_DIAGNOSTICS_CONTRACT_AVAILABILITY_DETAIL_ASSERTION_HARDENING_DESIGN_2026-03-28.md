# Cross-platform Vulkan Windows diagnostics-contract availability detail assertion hardening design (2026-03-28)

## 1. Scope
- Harden diagnostics-contract canary to assert availability-detail classification for missing required diagnostics fields.

## 2. Non-goals
- Do not change gate policy logic or failure routing in `run_windows_vulkan_checks.ps1`.
- Do not add new canary lanes.
- Do not change Linux lane behavior.

## 3. Design
- Update script:
  - `tools/run_windows_vulkan_gate_contract_canary.ps1`
- Script changes:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - extend validation contract with:
    - expected value `diag-contract-missing-required-fields`
  - publish summary key:
    - `windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail`
- Workflow changes:
  - `.github/workflows/cross-platform-gate.yml`
  - contract canary Step Summary rows include `gate_vulkan_availability_failure_detail`.

## 4. Validation Strategy
- Release build.
- Baseline gate command.
- Contract canary and full canary matrix regression.
- Static scan for script/workflow key wiring.
