# Cross-platform Vulkan Windows strict-diag-exit-nonzero expected-fail canary plan (2026-03-27)

## Scope
- Execute `VK-043`: add deterministic strict-diag-exit-nonzero expected-fail canary for Windows Vulkan gate.

## Implementation Planner
1. Freeze `VK-043` acceptance boundary (strict availability detail `diag-exit-nonzero` coverage only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1` with deterministic mock scenario and contract assertions.  
Dependency: step 1.
3. Integrate strict-diag-exit-nonzero canary into Windows workflow with:
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
  - strict-runtime-unavailable canary
  - strict-diag-exit-nonzero canary
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
- Strict-diag-exit-nonzero canary script returns `PASS` for expected behavior.
- CI fails when strict diag-exit-nonzero detail classification drifts.
- Step Summary includes strict-diag-exit-nonzero canary section.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
