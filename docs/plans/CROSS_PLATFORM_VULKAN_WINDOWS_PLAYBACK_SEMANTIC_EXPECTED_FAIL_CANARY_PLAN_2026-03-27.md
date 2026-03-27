# Cross-platform Vulkan Windows playback-semantic expected-fail canary plan (2026-03-27)

## Scope
- Execute `VK-036`: add deterministic playback-semantic expected-fail canary for Windows Vulkan gate.

## Implementation Planner
1. Freeze `VK-036` acceptance boundary (playback-semantic canary coverage only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_playback_semantic_canary.ps1` with deterministic mock scenario and contract assertions.  
Dependency: step 1.
3. Integrate playback-semantic canary into Windows workflow with:
  - exit-code propagation guard
  - Step Summary publishing  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate
  - diagnostics expected-fail canary
  - playback contract expected-fail canary
  - PASS-contract canary
  - strict-unavailable canary
  - optional-skip canary
  - unsupported-platform canary
  - playback-semantic canary
  - static scan  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Playback-semantic canary script returns `PASS` for expected behavior.
- CI fails when playback semantic-failure branch contract drifts.
- Step Summary includes playback-semantic canary section.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
