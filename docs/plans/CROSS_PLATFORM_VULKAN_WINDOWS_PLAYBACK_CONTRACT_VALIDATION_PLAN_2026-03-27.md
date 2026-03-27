# Cross-platform Vulkan Windows playback contract validation plan (2026-03-27)

## Scope
- Harden Windows Vulkan gate by validating playback output contract and exposing playback contract observability fields.

## Implementation Planner
1. Freeze `VK-026` acceptance and compatibility boundary.  
Dependency: none.
2. Implement playback required-key validation in `run_windows_vulkan_checks.ps1`.  
Dependency: step 1.
3. Add playback contract observability fields and contract-broken failure reason.  
Dependency: step 2.
4. Execute local validation:
  - static scan for new keys/reasons
  - build
  - baseline gate
  - playback contract-broken simulation via mock executable
  - auto policy compatibility matrix  
Dependency: step 3.
5. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Playback contract fields appear in summary output.
- Contract-broken playback path fails with deterministic reason/detail.
- Non-playback paths preserve prior behavior.
- No commit/push without explicit user confirmation.
