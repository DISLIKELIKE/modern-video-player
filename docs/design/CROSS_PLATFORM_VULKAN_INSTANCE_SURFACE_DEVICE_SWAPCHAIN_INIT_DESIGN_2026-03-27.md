# Cross-platform Vulkan instance/surface/device/swapchain init design (2026-03-27)

## 1. Goal
Land `VK-005` baseline Vulkan initialization lifecycle on Linux-first path without changing existing OpenGL/D3D11 behavior.

## 2. Scope
- In scope:
  - SDL Vulkan window bring-up.
  - Vulkan instance/surface/device/queue/swapchain creation.
  - Deterministic teardown ordering.
  - Minimal event handling for quit and file-drop requests.
- Out of scope:
  - Actual Vulkan frame rendering (`VK-006`).
  - YUV upload path (`VK-007`).
  - pacing/sync policy (`VK-008`).

## 3. Initialization Contract
1. Ensure SDL subsystems (`VIDEO|AUDIO`) and track renderer-owned flags.
2. Create `SDL_WINDOW_VULKAN` window.
3. Query SDL-required Vulkan instance extensions.
4. Create `VkInstance`.
5. Create `VkSurfaceKHR` from SDL window.
6. Select physical device with:
   - graphics queue support
   - present queue support on created surface
   - required extension `VK_KHR_swapchain`
   - non-empty surface formats/present modes
7. Create logical device and acquire graphics/present queues.
8. Create swapchain and fetch swapchain images.

## 4. Teardown Contract
- `close()` must release resources in strict reverse order:
  - `vkDeviceWaitIdle`
  - `vkDestroySwapchainKHR`
  - `vkDestroyDevice`
  - `vkDestroySurfaceKHR`
  - `vkDestroyInstance`
  - `SDL_DestroyWindow`
  - `SDL_QuitSubSystem(owned_flags)`
- Teardown must be safe for partial init failure paths.

## 5. Fallback / Safety
- Any init-stage failure returns `false` and triggers existing startup fallback in `PlayerCore`.
- Existing renderer backends are unchanged and remain fallback targets.
- Windows builds remain unaffected because Vulkan renderer is disabled by switch policy outside Linux.
