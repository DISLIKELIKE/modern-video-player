# PLAYERCORE Day68: VK007 frame upload YUV path

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VK-006` delivered clear/present + swapchain recreate, but Vulkan path still could not display decoded video frames.
- `renderFrame()` had started YUV->RGBA conversion preparation, but `present()` still used clear-only recording path.

## 2. Root Cause
- Missing frame upload integration in `present()`:
  - no staging buffer map/copy/unmap from converted RGBA payload
  - no command-record branch to copy uploaded frame into swapchain image
- `supportsDirectFrameFormat()` was declared in Vulkan header but not implemented in cpp, leaving direct-frame path contract incomplete.

## 3. Solution
- Completed Vulkan CPU frame upload baseline:
  - `renderFrame()` accepts `YUV420P` / `NV12`, converts to RGBA via `swscale`, and stores upload payload with mutex protection.
  - `present()` now:
    - snapshots upload payload metadata (`width/height/stride`)
    - ensures host-visible staging buffer capacity
    - maps/copies/unmaps upload bytes
    - records and submits `recordPresentCommandBuffer(..., has_frame_upload, ...)`
  - command recording keeps `clear + optional copy + present` in one path.
- Implemented `VulkanVideoRenderer::supportsDirectFrameFormat()` with explicit support set:
  - `AV_PIX_FMT_YUV420P`
  - `AV_PIX_FMT_NV12`

## 4. Validation Outcome
- Local Windows regression checks PASS:
  - Release build PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
  - `--performance-log-check` PASS.
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS.
- Static code-path check PASS:
  - no stale `recordClearCommandBuffer` call in Vulkan present path.
  - `recordPresentCommandBuffer` and `supportsDirectFrameFormat` are present at expected call/definition points.

## 5. Remaining
- Linux-first Vulkan runtime verification still requires Linux host/runner build and execution evidence.
- Current `VK-007` path is baseline CPU upload; performance/pace optimization belongs to `VK-008`.
