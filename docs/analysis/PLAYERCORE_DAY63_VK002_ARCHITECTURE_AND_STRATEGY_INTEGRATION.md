# PLAYERCORE Day63: VK002 architecture and strategy integration

Date: 2026-03-26  
Status: Done

## 1. Problem
- Vulkan backend needs to be integrated without reintroducing platform policy logic into `PlayerCore`.
- Existing startup path already uses `PlatformCapabilities -> PlaybackStrategy -> RendererFactory -> fallback`.

## 2. Architecture Decision
- Keep the current strategy spine unchanged:
  - capability probe publishes renderer availability and priority
  - strategy builds ordered candidate list
  - factory owns create/support checks
  - `PlayerCore` only consumes the plan and executes fallback
- Vulkan is inserted as one renderer candidate type, not a special-case pipeline.

## 3. Integration Boundary
1. `VideoRendererType` adds `Vulkan`.
2. `PlaybackStrategy` supports env override token `vulkan` / `vk`.
3. `PlatformCapabilities` can publish Vulkan support with Linux-first priority.
4. `RendererFactory` resolves name/support/create for Vulkan backend.
5. Existing diagnostics (`startup_renderer_candidates`, `startup_selected_renderer`, fallback reason) remain the observability surface.

## 4. Thread/Fallback Model Freeze
- Thread model remains unchanged in this step:
  - main thread: event pump/input consumption
  - decode/scheduler/player runtime: existing model
- Fallback model remains strategy-driven and observable.
- Vulkan chain target on Linux remains: `Vulkan -> OpenGL -> SoftwareSDL`.

## 5. Outcome
- Architecture-level entry points and ownership for Vulkan are now frozen and implemented through existing abstractions.
- Next steps can continue from CMake/dependency closure (`VK-003`) and skeleton wiring (`VK-004`) without touching `PlayerCore` policy ownership.
