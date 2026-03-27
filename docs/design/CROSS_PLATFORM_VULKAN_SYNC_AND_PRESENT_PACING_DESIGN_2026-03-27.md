# Cross-platform Vulkan sync and present pacing design (2026-03-27)

## 1. Goal
Deliver `VK-008` synchronization and pacing hardening for Vulkan render path without changing existing strategy/factory boundaries.

## 2. Scope
- In scope:
  - fence/semaphore runtime robustness for in-flight frame path
  - present mode request + fallback policy
  - pacing observability logs (requested/active mode, wait statistics)
- Out of scope:
  - diagnostics CLI command (`VK-010`)
  - Linux gate/CI wiring (`VK-011`, `VK-012`)
  - zero-copy upload optimization

## 3. Present Mode Policy
- Input contract:
  - `MVP_VULKAN_PRESENT_MODE` (`auto|fifo|mailbox|immediate|fifo_relaxed`)
- Selection rules:
  - `auto`: `mailbox -> fifo -> immediate -> first-supported`
  - explicit mode: try exact mode first
  - if explicit mode unsupported: fallback to safe auto policy (with warning)
- Runtime observability:
  - swapchain init log prints:
    - `present_mode_requested`
    - `present_mode_active`
    - `present_mode_exact_match`

## 4. Frame Sync Hardening
- Existing single-frame-in-flight primitives remain:
  - `image_available_semaphore`
  - `render_finished_semaphore`
  - `in_flight_fence`
- Added guarded fence wait:
  - wait timeout: `250ms`
  - timeout recovery: `vkDeviceWaitIdle` + `swapchain_recreate_requested=true`
  - avoid infinite wait-induced deadlock behavior.

## 5. Pacing Observability
- Added counters:
  - `frame_present_submitted`
  - `frame_presented`
  - `fence_wait_time_ns_total`
  - `fence_wait_time_ns_max`
  - `fence_wait_timeout_count`
  - out-of-date counters (`acquire` / `present`)
- Log cadence:
  - every 120 presented frames, print average/max fence wait and timeout count.

## 6. Fallback Compatibility
- Startup fallback chain ownership remains in strategy/factory layer.
- Vulkan runtime failure path keeps fallback observable and unchanged for current staged rollout.
