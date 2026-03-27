# Cross-platform Vulkan Windows enablement plan (2026-03-27)

## Scope
- Deliver analysis-backed implementation planner for enabling Vulkan on Windows without regressing current Linux Vulkan chain.

## Implementation Planner
1. Freeze Windows Vulkan enablement acceptance and rollback criteria.
Dependency: none.
2. Refactor CMake Vulkan switch/dependency logic for Linux+Windows parity.
Dependency: Step 1.
3. Publish Windows Vulkan capability only when compiled and runtime-available.
Dependency: Step 2.
4. Update Vulkan diagnostics platform contract (Linux-only -> Linux+Windows).
Dependency: Step 3.
5. Verify startup policy/fallback observability on Windows with and without Vulkan override.
Dependency: Step 4.
6. Add or update Windows Vulkan local check command/report contract.
Dependency: Step 5.
7. Run local validation matrix (build + diagnostics + performance/fallback checks).
Dependency: Step 6.
8. Sync docs/records/index closure for implementation round.
Dependency: Step 7.

## Validation Baseline (current pre-implementation)
- Build: PASS (current branch baseline).
- `--vulkan-diagnostics` on Windows: expected FAIL (`supported_platform=false`, `compiled_in=false`).

## Acceptance for implementation round
- Windows build supports `ENABLE_VULKAN_RENDERER=ON` when Vulkan dependency exists.
- Vulkan diagnostics on Windows no longer hardcoded unsupported; outputs runtime truth.
- Vulkan override playback path is observable and deterministic.
- Documentation/records/index chain synchronized.
- No commit/push without explicit user confirmation.
