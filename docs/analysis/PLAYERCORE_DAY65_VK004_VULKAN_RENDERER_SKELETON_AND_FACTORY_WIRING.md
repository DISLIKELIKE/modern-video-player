# PLAYERCORE Day65: VK004 Vulkan renderer skeleton and factory wiring

Date: 2026-03-26  
Status: Done

## 1. Problem
- No Vulkan renderer implementation unit existed, so factory/strategy could not wire a concrete backend target.

## 2. Root Cause
- `VideoRendererType` lacked `Vulkan`.
- `RendererFactory` had no Vulkan include/name/create branch.
- `PlatformCapabilities` and `PlaybackStrategy` did not expose Vulkan candidate path.

## 3. Solution
- Added `VideoRendererType::Vulkan`.
- Added `VulkanVideoRenderer` skeleton class (`include/render/vulkan_video_renderer.h`, `src/render/vulkan_video_renderer.cpp`).
- Wired Vulkan in:
  - `RendererFactory` (`rendererName` + `create`)
  - `PlatformCapabilities` renderer support list (Linux-first priority)
  - `PlaybackStrategy` env parser (`MVP_RENDERER_BACKEND=vulkan|vk`)
  - `main.cpp` renderer type naming helper
- Skeleton stage behavior:
  - `init()` intentionally returns `false` in `VK-004`
  - forces observable fallback to existing renderer chain.

## 4. Validation Outcome
- Build and diagnostics pass.
- With `MVP_RENDERER_BACKEND=vulkan`, startup candidates include Vulkan and fallback reason is machine-readable:
  - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
  - `startup_renderer_fallback_reason=fallback-after-renderer-failure`
  - overall check result remains `PASS`.

## 5. Remaining
- Real Vulkan instance/surface/device/swapchain init remains for `VK-005`.
