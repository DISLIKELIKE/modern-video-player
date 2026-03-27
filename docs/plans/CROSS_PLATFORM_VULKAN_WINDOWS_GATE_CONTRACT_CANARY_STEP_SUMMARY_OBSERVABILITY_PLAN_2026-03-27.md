# Cross-platform Vulkan Windows gate contract-canary Step Summary observability plan (2026-03-27)

## Scope
- Execute `VK-030`: expose Windows Vulkan contract-canary signals in CI Step Summary.

## Implementation Planner
1. Freeze `VK-030` acceptance boundary (observability only, no policy changes).  
Dependency: none.
2. Extend canary script summary payload to include canary gate-summary file presence signal.  
Dependency: step 1.
3. Extend workflow Windows gate step to parse canary summary env and render Step Summary table with fallback message.  
Dependency: step 2.
4. Execute local validation:
  - release build
  - baseline gate command
  - canary command
  - local Step Summary preview generation and static scan  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Step Summary contains dedicated canary section with key rows.
- Missing canary summary file is reported explicitly in Step Summary.
- Canary pass/fail semantics remain unchanged.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
