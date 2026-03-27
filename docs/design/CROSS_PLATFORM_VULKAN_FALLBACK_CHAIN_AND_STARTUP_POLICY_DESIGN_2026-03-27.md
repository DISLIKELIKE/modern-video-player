# Cross-platform Vulkan fallback chain and startup policy design (2026-03-27)

## 1. Goal
Deliver `VK-009` integration contract so Linux startup path deterministically prioritizes:
- `Vulkan -> OpenGL -> SoftwareSDL`
with observable policy decisions.

## 2. Scope
- In scope:
  - startup renderer candidate normalization in strategy layer
  - machine-readable startup policy reason fields
- Out of scope:
  - Vulkan direct diagnostics command (`VK-010`)
  - Linux gate script wiring (`VK-011`)
  - CI lane updates (`VK-012`)

## 3. Strategy Contract
- Location:
  - `PlaybackStrategy::buildOpenPlan(...)`
- Rule:
  - if platform is Linux and renderer candidate head is `Vulkan`, normalize prefix chain:
    - `Vulkan`
    - `OpenGL` if available
    - `SoftwareSDL` if available
  - preserve uniqueness and retain remaining candidates after normalized prefix.
- Reason tag:
  - append `linux-vulkan-fallback-chain` into renderer plan reason.

## 4. Observability Contract
- `DiagnosticsSnapshot` adds:
  - `startup_renderer_plan_reason`
  - `startup_decoder_plan_reason`
- `--performance-log-check` exports:
  - `performance-log-check.startup_renderer_plan_reason=<...>`
  - `performance-log-check.startup_decoder_plan_reason=<...>`

## 5. Compatibility
- Existing Windows default chain remains unchanged.
- Existing fallback reason semantics remain unchanged:
  - `none`
  - `fallback-after-renderer-failure`
  - `all-renderer-candidates-failed`
  - etc.
- Linux runtime proof remains a runner-side validation item.
