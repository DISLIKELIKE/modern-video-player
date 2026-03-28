# Cross-platform Vulkan Windows diagnostics-contract availability detail assertion hardening plan (2026-03-28)

## Scope
- Execute `VK-046`: harden diagnostics-contract expected-fail canary with explicit availability-detail assertion.

## Implementation Planner
1. Freeze `VK-046` acceptance boundary (contract-canary assertion/observability hardening only).  
Dependency: none.
2. Update `tools/run_windows_vulkan_gate_contract_canary.ps1`:
  - assert `gate_vulkan_availability_failure_detail=diag-contract-missing-required-fields`
  - publish canary summary field for that value.  
Dependency: step 1.
3. Update workflow contract canary Step Summary table to include the new field.  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate
  - full Windows Vulkan canary matrix regression
  - static scans for new key wiring  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Contract canary fails on availability-detail classification drift.
- Workflow Step Summary displays contract-canary availability detail.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
