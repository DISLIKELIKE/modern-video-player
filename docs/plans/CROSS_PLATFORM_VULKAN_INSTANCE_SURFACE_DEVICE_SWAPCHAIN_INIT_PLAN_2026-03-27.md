# Cross-platform Vulkan instance/surface/device/swapchain init plan (2026-03-27)

## Scope
- Deliver `VK-005`: complete Vulkan initialization/teardown baseline for instance/surface/device/swapchain.

## Implementation Planner
1. Build renderer state model for Vulkan handles and SDL ownership tracking.  
Dependency: none.
2. Implement init chain:
   - SDL subsystems
   - Vulkan window
   - instance extension query and `VkInstance`
   - `VkSurfaceKHR`
   - physical/logical device and queue selection
   - swapchain and image retrieval  
Dependency: Step 1.
3. Implement close chain with reverse-order destruction and partial-failure safety.  
Dependency: Step 2.
4. Keep render path conservative (`renderFrame/present/clear` no-op) and add baseline quit/file-drop event consumption.  
Dependency: Step 2.
5. Run local regression commands and record platform limitation for Linux-only Vulkan runtime evidence.  
Dependency: Steps 2-4.

## Acceptance
- Renderer init path reaches swapchain creation and image enumeration on supported Linux hosts.
- Renderer close path is deterministic and safe after partial/full init.
- Existing Windows checks remain PASS (no regression).
- Full docs chain (`analysis/design/plan/report + records + indexes`) is synchronized.
