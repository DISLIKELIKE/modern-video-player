# Cross-platform Vulkan CMake switches and dependency detect plan (2026-03-26)

## Scope
- Deliver `VK-003`: build switch, compile macro, and dependency downgrade behavior.

## Plan
1. Add `ENABLE_VULKAN_RENDERER` option with Linux-first defaults.
2. Add platform/host guardrails (force OFF on unsupported platforms).
3. Add Linux dependency probe (`pkg-config vulkan`) and force-OFF downgrade path.
4. Propagate `MVP_HAVE_VULKAN_RENDERER` and conditional source compilation.
5. Validate configure/build behavior on current host.

## Dependencies
- Depends on `VK-002` architecture contract.
- Unblocks `VK-004` renderer skeleton compile wiring.

## Acceptance
- Enabling Vulkan on unsupported host is safely downgraded.
- Build remains green with stable fallback behavior.
