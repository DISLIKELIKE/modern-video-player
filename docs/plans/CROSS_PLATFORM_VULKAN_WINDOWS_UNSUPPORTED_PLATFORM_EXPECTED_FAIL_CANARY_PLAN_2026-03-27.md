# Cross-platform Vulkan Windows unsupported-platform expected-fail canary plan (2026-03-27)

## Scope
- Execute `VK-035`: add deterministic unsupported-platform expected-fail canary for Windows Vulkan gate.

## Implementation Planner
1. Freeze `VK-035` acceptance boundary (unsupported-platform canary coverage only).  
Dependency: none.
2. Add `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1` with deterministic mock scenario and contract assertions.  
Dependency: step 1.
3. Integrate unsupported-platform canary into Windows workflow with:
  - exit-code propagation guard
  - Step Summary publishing  
Dependency: step 2.
4. Run local validation:
  - release build
  - baseline gate
  - diagnostics expected-fail canary
  - playback expected-fail canary
  - PASS-contract canary
  - strict-unavailable canary
  - optional-skip canary
  - unsupported-platform canary
  - static scan  
Dependency: step 3.
5. Sync docs/records/index chain (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 4.

## Acceptance
- Unsupported-platform canary script returns `PASS` for expected behavior.
- CI fails when unsupported-platform branch contract drifts.
- Step Summary includes unsupported-platform canary section.
- Local validation commands pass.
- No commit/push without explicit user confirmation.
