# Cross-platform Vulkan Windows playback-command-exit-nonzero expected-fail canary plan (2026-03-27)

## Scope
- Execute `VK-041`: add deterministic playback-command-exit-nonzero expected-fail canary for Windows Vulkan gate.

## Implementation Planner
1. Freeze `VK-041` acceptance boundary (`command-exit-nonzero` branch coverage only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1` with deterministic mock scenario and contract assertions.  
Dependency: step 1.
3. Integrate playback-command-exit-nonzero canary into Windows workflow with:
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
  - playback-backend semantic canary
  - playback-candidates semantic canary
  - playback-plan-reason semantic canary
  - playback-result-not-pass canary
  - playback-command-exit-nonzero canary
  - static scan  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Playback-command-exit-nonzero canary script returns `PASS` for expected behavior.
- CI fails when `command-exit-nonzero` branch contract drifts.
- Step Summary includes playback-command-exit-nonzero canary section.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
