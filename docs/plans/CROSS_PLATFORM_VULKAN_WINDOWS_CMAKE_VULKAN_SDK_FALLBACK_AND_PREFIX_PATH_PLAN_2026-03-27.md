# Cross-platform Vulkan Windows CMake SDK fallback and prefix-path closure plan (2026-03-27)

## Scope
- Close Windows Vulkan dependency-resolution gap after `VK-019` by hardening CMake SDK fallback and CI package-discovery assist.

## Implementation Planner
1. Freeze closure scope and acceptance for Windows Vulkan dependency-resolution follow-up.
Dependency: none.
2. Design Windows CMake fallback contract (`find_package` + `VULKAN_SDK` fallback + downgrade guard).
Dependency: step 1.
3. Implement CMake fallback path in `CMakeLists.txt`.
Dependency: step 2.
4. Implement workflow `CMAKE_PREFIX_PATH` export in Windows SDK stage.
Dependency: step 3.
5. Execute local validation (`configure/build/--vulkan-diagnostics/run_windows_vulkan_checks.ps1`).
Dependency: step 4.
6. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).
Dependency: step 5.

## Acceptance
- Windows Vulkan configure path supports explicit SDK fallback.
- Workflow exports `CMAKE_PREFIX_PATH` when SDK detected.
- Local validation remains stable and machine-readable.
- Documentation chain is fully synchronized.
- No commit/push without explicit user confirmation.