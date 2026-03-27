# Cross-platform Vulkan Windows CMake SDK fallback and prefix-path closure design (2026-03-27)

## 1. Goal
Improve Windows Vulkan dependency resolution robustness without changing fallback safety policy.

## 2. Design

### 2.1 Windows CMake fallback extension
- File: `CMakeLists.txt`
- Resolution order for Windows Vulkan:
  1. `find_package(Vulkan QUIET)` (primary path)
  2. `VULKAN_SDK` explicit fallback when package lookup fails
- Fallback contract:
  - require `${VULKAN_SDK}/Include/vulkan/vulkan.h`
  - require `${VULKAN_SDK}/Lib/vulkan-1.lib`
  - if both exist, append to `PLATFORM_EXTRA_INCLUDE_DIRS` / `PLATFORM_EXTRA_LIBRARIES`
  - if missing, emit warning and force `MVP_ENABLE_VULKAN_RENDERER=OFF`

### 2.2 Workflow CMake prefix-path assist
- File: `.github/workflows/cross-platform-gate.yml`
- In Windows SDK detect stage:
  - keep exporting `VULKAN_SDK`
  - prepend SDK root into `CMAKE_PREFIX_PATH`
  - keep existing `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1|0`

## 3. Compatibility and risk
- No regression to existing non-Vulkan hosts: downgrade-to-OFF contract stays unchanged.
- No change to renderer default policy (Windows still keeps D3D11 mainline).
- Linux lane behavior remains unaffected by this follow-up.