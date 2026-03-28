# Cross-platform Vulkan Windows unsupported-platform availability detail assertion hardening plan (2026-03-28)

## Scope
- Execute `VK-047`: harden unsupported-platform canary with explicit availability-detail assertion.

## Implementation Planner
1. Freeze `VK-047` acceptance boundary (unsupported-platform canary assertion/observability hardening only).  
Dependency: none.
2. Update `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`:
  - assert `gate_vulkan_availability_failure_detail=unsupported-platform`
  - publish canary summary field for that value.  
Dependency: step 1.
3. Update workflow unsupported-platform canary Step Summary table to include the new field.  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate
  - full Windows Vulkan canary matrix regression
  - static scan for wiring  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Unsupported-platform canary fails on availability-detail classification drift.
- Workflow Step Summary displays unsupported-platform availability detail.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
