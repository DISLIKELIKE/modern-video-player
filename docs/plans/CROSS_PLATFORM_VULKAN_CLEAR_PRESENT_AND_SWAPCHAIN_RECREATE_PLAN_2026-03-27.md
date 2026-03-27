# Cross-platform Vulkan clear/present and swapchain recreate plan (2026-03-27)

## Scope
- Deliver `VK-006`: minimal render path (`clear + present`) and swapchain recreate stability.

## Implementation Planner
1. Add Vulkan frame resources (command pool/buffer + semaphores + fence).  
Dependency: `VK-005` init lifecycle.
2. Add swapchain helper routines for create/destroy/recreate and drawable-size checks.  
Dependency: Step 1.
3. Implement `present()` acquire/record/submit/present loop with transfer clear.  
Dependency: Steps 1-2.
4. Implement resize-driven recreate trigger in SDL event handling and out-of-date/suboptimal handling in present path.  
Dependency: Step 3.
5. Keep `clear()` path observable (`clear()` -> `present()`), then run local regressions and sync docs/records.  
Dependency: Steps 3-4.

## Acceptance
- Vulkan renderer can output a visible clear frame through swapchain present path on supported Linux host.
- Window resize/minimize/restore no longer leaves swapchain permanently stale.
- Existing Windows checks remain PASS.
