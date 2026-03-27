# Cross-platform Vulkan renderer skeleton and factory wiring plan (2026-03-26)

## Scope
- Deliver `VK-004`: compilable Vulkan renderer skeleton and full factory/strategy wiring.

## Plan
1. Add Vulkan renderer enum and backend class skeleton.
2. Wire factory create/name mapping under compile guards.
3. Publish Vulkan support in platform capabilities with Linux-first priority.
4. Add strategy override parser token (`vulkan|vk`) and verify fallback observability.
5. Run build and runtime diagnostics regression.

## Dependencies
- Depends on `VK-003` CMake compile switch/macro wiring.
- Unblocks `VK-005` Vulkan init chain implementation.

## Acceptance
- Project compiles with Vulkan wiring present.
- Startup candidate list includes Vulkan when requested.
- Fallback reason is observable and runtime check remains pass.
