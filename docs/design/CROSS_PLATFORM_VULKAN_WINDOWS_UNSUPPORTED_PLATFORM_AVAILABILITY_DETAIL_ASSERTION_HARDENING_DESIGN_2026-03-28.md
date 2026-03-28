# Cross-platform Vulkan Windows unsupported-platform availability detail assertion hardening design (2026-03-28)

## 1. Scope
- Harden unsupported-platform expected-fail canary by asserting availability-detail classification.

## 2. Non-goals
- Do not change unsupported-platform routing logic in `run_windows_vulkan_checks.ps1`.
- Do not change strict/optional policy behavior.
- Do not change Linux lane behavior.

## 3. Design
- Update script:
  - `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
- Script changes:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - assert expected value `unsupported-platform`
  - export summary key:
    - `windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail`
- Workflow changes:
  - `.github/workflows/cross-platform-gate.yml`
  - unsupported-platform canary Step Summary rows include:
    - `gate_vulkan_availability_failure_detail`

## 4. Validation Strategy
- Release build.
- Baseline gate command.
- Full Windows Vulkan canary matrix regression.
- Static scan for script/workflow key wiring.
