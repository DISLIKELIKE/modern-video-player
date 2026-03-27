# Cross-platform Vulkan Windows CI step summary observability plan (2026-03-27)

## Scope
- Add Windows Vulkan gate compact status view to GitHub Actions Step Summary.

## Implementation Planner
1. Freeze `VK-027` acceptance and non-goal boundary (observability-only, no policy behavior changes).  
Dependency: none.
2. Extend workflow Windows gate step to parse Vulkan summary env and write Step Summary table.  
Dependency: step 1.
3. Add missing-file fallback message to Step Summary output path.  
Dependency: step 2.
4. Execute local validation:
  - build
  - generate baseline Vulkan summary env
  - run equivalent local summary parser preview and verify key rows  
Dependency: step 3.
5. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Step Summary table generation logic exists in workflow and references Windows Vulkan summary env.
- Local preview confirms expected key/value mapping.
- Gate decision behavior remains unchanged.
- No commit/push without explicit user confirmation.
