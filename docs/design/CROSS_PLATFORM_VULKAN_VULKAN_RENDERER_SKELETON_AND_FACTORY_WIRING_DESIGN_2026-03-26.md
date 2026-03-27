# Cross-platform Vulkan renderer skeleton and factory wiring design (2026-03-26)

## 1. Goal
Land a compilable Vulkan backend skeleton and wire it into renderer selection/factory contracts without breaking existing renderers.

## 2. Skeleton Contract
- New class: `VulkanVideoRenderer`.
- Implements `IVideoRenderer`, `IPlaybackInputSource`, `IRenderOverlaySink`.
- `VK-004` behavior:
  - `init()` returns `false` intentionally.
  - all other methods are safe no-op stubs.
- Rationale:
  - prove integration points first
  - keep runtime fallback explicit until real Vulkan init is implemented.

## 3. Wiring Contract
- Enum: `VideoRendererType::Vulkan`.
- Factory:
  - name mapping: `"Vulkan"`
  - create path behind `MVP_HAVE_VULKAN_RENDERER`.
- Strategy:
  - env override parsing supports `vulkan`/`vk`.
- Capabilities:
  - renderer support publishes Vulkan candidate with Linux-first priority when compiled.

## 4. Fallback Behavior
- When Vulkan candidate fails in skeleton stage, `PlayerCore` fallback path must stay active and observable.
- Existing startup diagnostics fields are used as acceptance evidence.

## 5. Non-goals
- No Vulkan swapchain/resource/renderpass implementation in this step.
- No Vulkan diagnostics CLI in this step (`VK-010` target).
