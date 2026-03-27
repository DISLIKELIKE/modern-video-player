# PLAYERCORE Day67: VK006 clear/present and swapchain recreate

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VK-005` completed Vulkan init/teardown, but no visible render output existed (`present()`/`clear()` were no-op).
- Resize path still lacked `swapchain` recreate handling, so `OUT_OF_DATE/SUBOPTIMAL` states were not recoverable.

## 2. Root Cause
- Missing per-frame Vulkan sync and command recording loop.
- Missing swapchain-recreate trigger/execute path on window resize and present/acquire return codes.

## 3. Solution
- Implemented minimal Vulkan frame loop:
  - frame resources: command pool/buffer, acquire/present semaphores, in-flight fence.
  - `present()` path:
    - wait previous in-flight fence
    - acquire swapchain image
    - record one-time command buffer
    - transition image layout + `vkCmdClearColorImage`
    - submit and present
- Implemented swapchain recreate handling:
  - SDL `WINDOWEVENT` resize/minimize/maximize/restore flags `swapchain_recreate_requested`.
  - `present()` handles `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR`.
  - recreate flow waits device idle, rebuilds swapchain, keeps deferred behavior when drawable size is zero.
- `clear()` now calls `present()` so idle/initial clear path is observable.

## 4. Validation Outcome
- Local Windows regression checks remain PASS:
  - Release build PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
  - `--performance-log-check` PASS.
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS.
- Limitation:
  - Linux-first Vulkan renderer is still compile/runtime verifiable only on Linux host/runner.

## 5. Remaining
- `VK-007`: add first video frame upload path (YUV420/NV12 at least one path) on top of current clear/present baseline.
