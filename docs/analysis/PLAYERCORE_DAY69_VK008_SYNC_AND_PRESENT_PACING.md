# PLAYERCORE Day69: VK008 sync and present pacing

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VK-007` completed frame upload, but Vulkan frame synchronization and pacing still had two gaps:
  - in-flight fence wait used infinite timeout (`vkWaitForFences(..., UINT64_MAX)`), which can stall forever on driver/device anomalies.
  - swapchain present mode was fixed strategy (mailbox->fifo) and not operator-configurable for pacing/tearing tradeoff validation.

## 2. Root Cause
- No timeout/recovery path in frame wait loop.
- No explicit present-mode request contract (env override) and no active-mode observability in Vulkan startup/swapchain logs.

## 3. Solution
- Added Vulkan present mode request contract:
  - env: `MVP_VULKAN_PRESENT_MODE`
  - accepted values: `auto`, `fifo`, `mailbox`, `immediate`, `fifo_relaxed` (`paced/vsync/tearing/relaxed` aliases supported)
  - unsupported/invalid request falls back safely to auto policy and logs warning.
- Added present-mode selection and fallback logic in swapchain creation:
  - `auto`: mailbox -> fifo -> immediate -> first available
  - explicit mode: exact if available, otherwise safe fallback
  - startup/swapchain logs now include requested/active mode and exact-match status.
- Added frame sync hardening for pacing stability:
  - fence wait timeout budget: `250ms`
  - timeout path logs warning, performs `vkDeviceWaitIdle` recovery, and requests swapchain recreate to avoid deadlock loops.
- Added pacing observability counters:
  - submitted/presented frame counters
  - fence wait total/max timeout counters
  - periodic pacing log snapshot every 120 presented frames.

## 4. Validation Outcome
- Local checks PASS:
  - Vulkan sync/pacing symbol scan PASS.
  - Release build PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
  - `--performance-log-check` PASS.
  - `MVP_RENDERER_BACKEND=vulkan` + `MVP_VULKAN_PRESENT_MODE=immediate` fallback observability PASS on Windows host.
- Limitation:
  - Linux-first Vulkan runtime pacing behavior still requires Linux host/runner execution evidence.

## 5. Remaining
- `VK-009` should integrate runtime startup fallback chain closure for Linux target order:
  - `Vulkan -> OpenGL -> SoftwareSDL`.
