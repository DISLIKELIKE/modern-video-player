# Cross-platform Vulkan Windows gate and CI integration plan (2026-03-27)

## Scope
- Implement Windows Vulkan gate command and integrate it into GitHub Actions Windows lane.

## Implementation Planner
1. Freeze acceptance for Windows Vulkan gate behavior and mode policy.
Dependency: none.
2. Design machine-readable output contract and optional/strict mode semantics.
Dependency: step 1.
3. Implement `tools/run_windows_vulkan_checks.ps1` with diagnostics parsing and conditional playback verification.
Dependency: step 2.
4. Wire Windows gate script invocation into `.github/workflows/cross-platform-gate.yml`.
Dependency: step 3.
5. Run local validation matrix (configure/build/script optional/strict mode).
Dependency: step 4.
6. Sync docs/records/index closure.
Dependency: step 5.

## Acceptance
- New Windows Vulkan gate command runs independently and prints machine-readable status lines.
- CI Windows lane includes Vulkan gate stage.
- Optional mode does not fail when Vulkan unavailable; strict mode fails deterministically.
- Docs/index/records synchronized.
- No commit/push without explicit user confirmation.
