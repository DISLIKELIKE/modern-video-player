# Cross-platform Vulkan Windows gate expected-fail contract canary plan (2026-03-27)

## Scope
- Execute `VK-029`: add CI-level expected-fail contract canary for Windows Vulkan gate.

## Implementation Planner
1. Freeze `VK-029` boundary and acceptance (contract canary only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_contract_canary.ps1` with deterministic failure-path assertions and machine-readable output.  
Dependency: step 1.
3. Integrate canary command into Windows gate workflow with explicit non-zero exit propagation check.  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate command
  - canary command and static scans  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Canary script exists and returns `PASS` on expected contract behavior.
- Workflow runs canary and fails when canary detects contract drift.
- Local build and verification commands pass.
- No commit/push without explicit user confirmation.
