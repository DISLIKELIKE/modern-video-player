# Cross-platform Vulkan Windows availability failure detail classification plan (2026-03-27)

## Scope
- Improve Windows Vulkan gate observability by adding machine-readable availability failure classification.

## Implementation Planner
1. Freeze `VK-024` acceptance and compatibility boundary.  
Dependency: none.
2. Add availability probe pass/failure-detail fields in `run_windows_vulkan_checks.ps1`.  
Dependency: step 1.
3. Keep strict/optional decision behavior unchanged (`VK-022` compatibility).  
Dependency: step 2.
4. Execute local validation:
  - static key scan
  - configure/build
  - baseline + policy matrix gate checks  
Dependency: step 3.
5. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Summary outputs contain new availability fields.
- Existing strict/optional behavior and exit codes remain unchanged.
- Local validation matrix passes with expected host-dependent outcomes.
- No commit/push without explicit user confirmation.
