# PLAYERCORE Day77: VK015 Windows Vulkan link/runtime probe closure

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan enablement (`Issue 142`) opened platform/switch contract but still had two closure gaps:
  - Windows target link line did not consume `PLATFORM_EXTRA_LIBRARIES`, so Vulkan libraries resolved by `find_package(Vulkan)` were not guaranteed to link into `modern-video-player`.
  - Vulkan renderer support publication in `platform_capabilities` marked `runtime_available=true` whenever compiled, which did not reflect real runtime probe behavior.

## 2. Gap Snapshot
- `CMakeLists.txt`:
  - Vulkan dependency could be discovered, but `target_link_libraries` (Windows branch) omitted `PLATFORM_EXTRA_LIBRARIES`.
- `src/platform/platform_capabilities.cpp`:
  - Vulkan capability used compile-time constant runtime availability.

## 3. Solution Direction
- Wire `PLATFORM_EXTRA_LIBRARIES` into Windows target link stage to complete dependency closure.
- Add a lightweight Vulkan runtime probe:
  - `vkEnumerateInstanceExtensionProperties`
  - `vkCreateInstance`
  - `vkEnumeratePhysicalDevices`
- Publish Vulkan renderer support with:
  - `compiled_in=true` only when macro enabled.
  - `runtime_available=<probe result>`.

## 4. DoD
- Windows build graph contains Vulkan link inputs when Vulkan package is found.
- Capability output no longer hardcodes Vulkan runtime availability.
- Diagnostics/fallback behavior remains deterministic on current host.
- Full docs/records/index sync completed for this round.

## 5. Outcome
- Linked `PLATFORM_EXTRA_LIBRARIES` in Windows `target_link_libraries` branch.
- Added Vulkan runtime probe helper and used probe result when publishing Vulkan support in `PlatformCapabilitiesProbe`.
- Local verification completed on current Windows host (Vulkan package missing):
  - configure/build PASS
  - `--vulkan-diagnostics` now remains truthful (`compiled_in=false`, `runtime_available=false`)
  - Vulkan override fallback observability remains PASS.

## 6. Remaining
- Validate the `compiled_in=true` + runtime-available path on a Windows host with Vulkan SDK/runtime package installed.
