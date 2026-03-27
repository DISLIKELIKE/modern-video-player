# Cross-platform Vulkan frame upload YUV path plan (2026-03-27)

## Scope
- Deliver `VK-007`: baseline CPU frame upload path so Vulkan can display decoded video frame data.

## Implementation Planner
1. Complete `present()` upload integration.  
Dependency: `VK-006` present/swapchain baseline and existing `VK-007` staging/sws helper scaffolding.
2. Implement direct-frame format contract (`supportsDirectFrameFormat`).  
Dependency: none.
3. Remove stale clear-only call path and enforce unified present recording path (`recordPresentCommandBuffer`).  
Dependency: Step 1.
4. Run local build/regression checks (build + diagnostics + performance + Vulkan override fallback observability).  
Dependency: Steps 1-3.
5. Sync round documents/records/indexes (`analysis/design/plan/report` + `VERSION/CHANGELOG/DEVELOP_LOG` + README indexes).  
Dependency: Step 4.

## Acceptance
- Vulkan renderer code path contains frame upload chain (`YUV420P`/`NV12` accepted, RGBA upload to swapchain present path).
- No stale `recordClearCommandBuffer` use in Vulkan present path.
- Existing Windows baseline checks remain PASS.
- Linux-first Vulkan runtime proof is explicitly tracked as pending runner validation.
