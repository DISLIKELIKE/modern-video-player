# Cross-platform Vulkan clear/present and swapchain recreate design (2026-03-27)

## 1. Goal
Deliver `VK-006` minimal visible Vulkan render path:
- clear + present loop
- resize-driven swapchain recreate

## 2. Scope
- In scope:
  - frame synchronization primitives
  - acquire/submit/present loop
  - transfer-based clear command recording
  - recreate on SDL resize and Vulkan out-of-date/suboptimal returns
- Out of scope:
  - video texture upload (`VK-007`)
  - advanced pacing policy (`VK-008`)
  - diagnostics CLI (`VK-010`)

## 3. Render Loop Contract
1. Wait in-flight fence when previous frame was submitted.
2. Acquire swapchain image (`image_available_semaphore`).
3. Record one-time command buffer:
   - `UNDEFINED -> TRANSFER_DST_OPTIMAL`
   - `vkCmdClearColorImage`
   - `TRANSFER_DST_OPTIMAL -> PRESENT_SRC_KHR`
4. Submit to graphics queue, signal `render_finished_semaphore`.
5. Present on present queue.

## 4. Recreate Contract
- Trigger conditions:
  - SDL window resize/minimize/maximize/restore events.
  - `vkAcquireNextImageKHR` returns `OUT_OF_DATE`.
  - `vkQueuePresentKHR` returns `OUT_OF_DATE` or `SUBOPTIMAL`.
- Recreate behavior:
  - `vkDeviceWaitIdle`
  - rebuild swapchain/images
  - if drawable size is zero, keep deferred recreate request and skip frame safely.

## 5. Safety / Fallback
- Existing startup fallback contract remains unchanged.
- On unsupported/failed Vulkan path, `PlayerCore` fallback remains observable via startup diagnostics.
