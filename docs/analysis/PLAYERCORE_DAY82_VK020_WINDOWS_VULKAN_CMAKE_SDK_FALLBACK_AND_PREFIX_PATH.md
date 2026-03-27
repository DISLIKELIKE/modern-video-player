# PLAYERCORE Day82: VK020 Windows Vulkan CMake SDK fallback and prefix-path closure

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan SDK provisioning was already added in CI (`VK-018`), but CMake Vulkan resolution still depended heavily on `find_package(Vulkan)` success.
- On some runners, SDK may exist while package discovery fails, keeping `compiled_in=false` and making Windows Vulkan gate stay on `SKIPPED`.

## 2. Gap Snapshot
- Windows CMake path had no explicit SDK include/lib fallback when `find_package(Vulkan)` failed.
- Workflow exported `VULKAN_SDK` but lacked `CMAKE_PREFIX_PATH` injection to help CMake package lookup.

## 3. Solution Direction
- Extend `CMakeLists.txt` Windows Vulkan branch:
  - keep `find_package(Vulkan)` first.
  - when package lookup fails, fallback to `VULKAN_SDK` explicit probe:
    - `${VULKAN_SDK}/Include/vulkan/vulkan.h`
    - `${VULKAN_SDK}/Lib/vulkan-1.lib`
  - if fallback probe is complete, append include/lib and keep Vulkan enabled.
  - if fallback probe is incomplete, warn and force OFF (safe downgrade preserved).
- Extend workflow SDK stage:
  - when SDK is detected, prepend `CMAKE_PREFIX_PATH` with Vulkan SDK root.

## 4. DoD
- Windows CMake supports explicit SDK fallback path when package lookup fails.
- Workflow exports `CMAKE_PREFIX_PATH` together with existing SDK signals.
- Local configure/build/check commands pass on current host without regression.
- Docs/records/index chain fully synchronized.

## 5. Outcome
- CMake now has Windows-side `VULKAN_SDK` fallback path and keeps downgrade guard behavior.
- Workflow now exports `CMAKE_PREFIX_PATH` on SDK detection, improving `find_package(Vulkan)` success odds in CI.
- Local host (without Vulkan SDK) remains deterministic:
  - configure PASS with expected downgrade warning
  - build PASS
  - `--vulkan-diagnostics` remains truthful (`compiled_in=false`)
  - Windows Vulkan gate remains machine-readable `SKIPPED`.

## 6. Remaining
- Strict PASS proof still requires a Vulkan-ready Windows runner where SDK/runtime are actually available.