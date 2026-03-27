# Cross-platform Vulkan Windows runtime probe detail observability plan (2026-03-27)

## Scope
- Close Windows Vulkan CI/gate diagnostics gap by publishing runtime probe detail in machine-readable summary.

## Implementation Planner
1. Freeze `VK-023` acceptance and non-goal boundary (observability-only, no policy behavior change).  
Dependency: none.
2. Extend workflow Vulkan SDK step to export runtime probe detail env.  
Dependency: step 1.
3. Extend Windows Vulkan gate summary output with runtime probe detail field.  
Dependency: step 2.
4. Execute local validation:
  - static scan of new keys
  - configure/build
  - gate baseline and policy matrix detail-field checks  
Dependency: step 3.
5. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL` is exported in workflow.
- `windows-vulkan-check.runner_vulkan_runtime_probe_detail` appears in gate summary.
- `VK-022` auto strict behavior remains unchanged.
- Local validation matrix passes with expected host-dependent outcomes.
- No commit/push without explicit user confirmation.
