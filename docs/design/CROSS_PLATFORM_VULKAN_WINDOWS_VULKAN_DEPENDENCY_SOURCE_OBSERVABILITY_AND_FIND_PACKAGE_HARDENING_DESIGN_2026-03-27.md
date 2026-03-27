# Cross-platform Vulkan Windows dependency-source observability and find_package hardening design (2026-03-27)

## 1. Goal
Close remaining ambiguity in Windows Vulkan dependency resolution and make dependency source machine-readable.

## 2. Design

### 2.1 Windows CMake completeness contract
- File: `CMakeLists.txt`
- Resolution policy on Windows:
  1. `find_package(Vulkan QUIET)` probe
  2. Accept `find_package` path only when metadata is complete
  3. If incomplete, try `VULKAN_SDK` fallback
  4. If fallback incomplete, force `MVP_ENABLE_VULKAN_RENDERER=OFF`
- Build-time source marker:
  - `MVP_VULKAN_DEPENDENCY_SOURCE=find_package|vulkan_sdk_fallback|disabled`

### 2.2 Diagnostics observability extension
- File: `src/main.cpp`
- `--vulkan-diagnostics` adds:
  - `vulkan-diagnostics.dependency_source=<source>`

### 2.3 Windows gate summary extension
- File: `tools/run_windows_vulkan_checks.ps1`
- Summary adds:
  - `windows-vulkan-check.diag_dependency_source=<source>`

## 3. Compatibility and risk
- Safe downgrade remains unchanged for non-Vulkan hosts.
- Existing strict/optional policy behavior remains unchanged.
- Linux lane behavior is unaffected (field addition is additive).
