# Cross-platform Vulkan Windows dependency-source observability and find_package hardening plan (2026-03-27)

## Scope
- Close Windows Vulkan dependency-resolution ambiguity after `VK-020` by adding completeness checks and dependency-source observability.

## Implementation Planner
1. Freeze acceptance for Windows Vulkan dependency-source hardening follow-up.  
Dependency: none.
2. Design complete/incomplete metadata decision contract for Windows `find_package(Vulkan)`.  
Dependency: step 1.
3. Implement CMake dependency-source marker and complete/incomplete fallback policy.  
Dependency: step 2.
4. Extend diagnostics and gate summary outputs with dependency source.  
Dependency: step 3.
5. Execute local validation (`configure/build/--vulkan-diagnostics/run_windows_vulkan_checks.ps1`).  
Dependency: step 4.
6. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 5.

## Acceptance
- Windows Vulkan CMake path rejects incomplete `find_package` metadata and falls back safely.
- Diagnostics and gate summary include dependency source.
- Local checks pass without regression.
- Documentation chain is fully synchronized.
- No commit/push without explicit user confirmation.
