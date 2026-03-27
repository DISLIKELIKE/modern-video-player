# PLAYERCORE Day66: VK005 Vulkan instance/surface/device/swapchain init

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VulkanVideoRenderer` was still a pure skeleton; no real Vulkan initialization path existed.
- `VK-006` requires a valid swapchain lifecycle baseline before implementing clear/present/recreate.

## 2. Root Cause
- Missing SDL Vulkan window and instance extension negotiation.
- Missing physical/logical device and queue-family selection.
- Missing swapchain creation and teardown ordering.

## 3. Solution
- Implemented real Vulkan bring-up in `src/render/vulkan_video_renderer.cpp`:
  - SDL subsystem initialization (`VIDEO|AUDIO`) with owned-subsystem tracking.
  - SDL Vulkan window creation.
  - Vulkan instance creation using SDL-required instance extensions.
  - Surface creation via `SDL_Vulkan_CreateSurface`.
  - Physical device suitability selection (graphics/present queue + `VK_KHR_swapchain` + surface support).
  - Logical device + queue creation.
  - Swapchain creation and image acquisition.
- Implemented full close-time resource release ordering:
  - `vkDeviceWaitIdle`
  - swapchain -> device -> surface -> instance -> window -> owned SDL subsystems
- Added baseline event handling for quit and drag-drop open-file request consumption.

## 4. Validation Outcome
- Windows local build/regression remains stable:
  - Release build PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
  - `--performance-log-check` PASS.
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability still PASS on Windows (`fallback-after-renderer-failure`).
- Host limitation:
  - Vulkan backend is Linux-first and disabled on Windows build switch path, so Vulkan compilation/runtime evidence still requires Linux host/runner.

## 5. Remaining
- `VK-006` to add minimal rendering path (clear + present) and swapchain recreate on resize.
