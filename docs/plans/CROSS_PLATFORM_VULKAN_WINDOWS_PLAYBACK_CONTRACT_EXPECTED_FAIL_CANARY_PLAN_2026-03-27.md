# Cross-platform Vulkan Windows playback-contract expected-fail canary plan (2026-03-27)

## Scope
- Execute `VK-031`: add deterministic expected-fail canary for Windows Vulkan playback-contract branch.

## Implementation Planner
1. Freeze `VK-031` acceptance boundary (playback-contract canary coverage only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_playback_contract_canary.ps1` with deterministic mock-based failure scenario and contract assertions.  
Dependency: step 1.
3. Integrate playback-contract canary into Windows gate workflow with:
  - exit-code propagation guard
  - Step Summary publishing  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate
  - playback-contract canary
  - static scan  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Playback-contract canary script returns `PASS` for expected behavior.
- CI fails when playback-contract branch contract drifts.
- Step Summary includes playback-contract canary section.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
