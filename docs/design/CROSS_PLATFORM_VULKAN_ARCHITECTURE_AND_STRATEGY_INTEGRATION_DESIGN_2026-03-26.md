# Cross-platform Vulkan architecture and strategy integration design (2026-03-26)

## 1. Goal
Integrate Vulkan as a first-class renderer candidate while keeping core policy boundaries stable.

## 2. Module Contract
- `platform_capabilities`:
  - publishes `RendererSupport{type, compiled_in, runtime_available, default_priority}`
  - Vulkan appears only when compiled in.
- `playback_strategy`:
  - accepts `MVP_RENDERER_BACKEND=vulkan|vk`
  - merges override + platform default list into one ordered candidate chain.
- `renderer_factory`:
  - maps enum/name/create/isSupported for Vulkan.
- `player_core`:
  - unchanged ownership, still executes candidate loop and fallback reasons.

## 3. Linux-first Priority Rule
- Renderer priority intent:
  - Linux: `Vulkan(120) > OpenGL(100) > SoftwareSDL(60)`
  - Windows remains unchanged for this wave (Vulkan build switch forced OFF).

## 4. Fallback Policy
- Candidate failure types remain existing semantics:
  - unsupported at runtime
  - creation failure
  - init failure
- Fallback reason is recorded by existing fields:
  - `startup_renderer_fallback_reason`
  - `startup_renderer_candidates`
  - `startup_selected_renderer`

## 5. Non-goals
- No Vulkan swapchain/device/resource implementation in this step.
- No CLI command contract changes in this step (`--vulkan-diagnostics` is a later step).
