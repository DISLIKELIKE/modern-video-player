# Cross-platform Vulkan Windows pass-contract availability detail assertion hardening design (2026-03-28)

## 1. Scope
- Harden PASS-contract canary by asserting success-path availability detail classification.

## 2. Non-goals
- Do not change gate policy logic in `run_windows_vulkan_checks.ps1`.
- Do not change playback semantic checks.
- Do not change Linux lane behavior.

## 3. Design
- Update script:
  - `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
- Script changes:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - assert expected value `none`
  - export summary key:
    - `windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail`
- Workflow changes:
  - `.github/workflows/cross-platform-gate.yml`
  - PASS-contract canary Step Summary rows include:
    - `gate_vulkan_availability_failure_detail`

## 4. Validation Strategy
- Release build.
- Baseline gate command.
- Full Windows Vulkan canary matrix regression.
- Static scan for script/workflow key wiring.
