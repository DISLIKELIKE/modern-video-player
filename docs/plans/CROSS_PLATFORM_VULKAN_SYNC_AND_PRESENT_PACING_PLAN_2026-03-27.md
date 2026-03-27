# Cross-platform Vulkan sync and present pacing plan (2026-03-27)

## Scope
- Deliver `VK-008`: sync hardening and present pacing controls on top of `VK-007` upload baseline.

## Implementation Planner
1. Add present-mode request parsing and naming helpers (`MVP_VULKAN_PRESENT_MODE`).  
Dependency: `VK-005` swapchain bootstrap + `VK-006` present loop.
2. Extend swapchain present-mode selection with explicit fallback policy and startup logging.  
Dependency: Step 1.
3. Harden in-flight fence wait path with timeout + recovery + recreate request.  
Dependency: `VK-006` frame-sync object baseline.
4. Add pacing observability counters/logs for submit/present/wait behavior.  
Dependency: Steps 2-3.
5. Run local regression checks and sync docs/records/indexes.  
Dependency: Steps 1-4.

## Acceptance
- Present mode request is configurable and fallback-safe.
- Fence wait no longer blocks forever in one path; timeout has deterministic recovery.
- Existing Windows regression commands remain PASS.
- Linux Vulkan runtime pacing evidence remains explicitly tracked for runner validation.
