# Cross-platform Vulkan Windows auto strict policy promotion plan (2026-03-27)

## Scope
- Promote Windows Vulkan gate from static strict toggle to auto strict policy tied to runner SDK availability.

## Implementation Planner
1. Freeze policy semantics and acceptance for auto promotion.
Dependency: none.
2. Implement script policy parser and effective-mode fields.
Dependency: step 1.
3. Promote workflow default policy to `auto`.
Dependency: step 2.
4. Run local validation matrix for policy combinations (`auto/off` x sdk availability signal).
Dependency: step 3.
5. Sync docs/records/index closure.
Dependency: step 4.

## Acceptance
- Script supports `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`.
- Summary output contains policy/effective-mode fields.
- Workflow default policy is `auto`.
- Local matrix confirms deterministic policy transitions.
- Documentation/records synchronized.
- No commit/push without explicit user confirmation.
