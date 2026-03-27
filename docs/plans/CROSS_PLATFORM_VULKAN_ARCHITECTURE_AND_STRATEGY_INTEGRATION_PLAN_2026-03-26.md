# Cross-platform Vulkan architecture and strategy integration plan (2026-03-26)

## Scope
- Deliver `VK-002`: architecture integration through existing strategy/factory abstractions.

## Plan
1. Enumerate renderer integration points (`video_renderer`, `playback_strategy`, `platform_capabilities`, `renderer_factory`).
2. Freeze Vulkan insertion rule and Linux-first priority policy.
3. Keep `PlayerCore` policy-neutral and fallback-execution-only.
4. Validate observability remains machine-readable through existing startup diagnostics fields.

## Dependencies
- Depends on `VK-001` scope/DoD freeze.
- Unblocks `VK-003` (build switch/dependency detect) and `VK-004` (skeleton wiring).

## Acceptance
- Vulkan is integrated as a strategy candidate contract, not a `PlayerCore` special branch.
- Fallback and startup diagnostics contracts stay stable.
