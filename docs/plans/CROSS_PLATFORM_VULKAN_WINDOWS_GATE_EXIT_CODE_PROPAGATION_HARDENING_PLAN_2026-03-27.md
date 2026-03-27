# Cross-platform Vulkan Windows gate exit-code propagation hardening plan (2026-03-27)

## Scope
- Execute `VK-028`: ensure Windows Vulkan gate failures are not swallowed by PowerShell pipeline behavior in CI.

## Implementation Planner
1. Freeze `VK-028` boundary and acceptance criteria (exit-code propagation only).  
Dependency: none.
2. Update Windows gate workflow step to capture Vulkan gate exit code after `Tee-Object` pipeline.  
Dependency: step 1.
3. Keep Step Summary rendering logic unchanged and append fail-fast guard on captured exit code.  
Dependency: step 2.
4. Execute local validation:
  - release build
  - baseline Vulkan gate run
  - legacy-vs-guarded pipeline behavior reproduction  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Workflow contains explicit Vulkan gate exit-code propagation check.
- Non-zero Vulkan gate result fails step deterministically.
- Baseline local behavior remains stable (`SKIPPED` on current host).
- No commit/push without explicit user confirmation.
