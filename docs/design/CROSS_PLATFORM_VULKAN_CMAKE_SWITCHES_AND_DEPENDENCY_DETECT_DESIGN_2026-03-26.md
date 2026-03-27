# Cross-platform Vulkan CMake switches and dependency detect design (2026-03-26)

## 1. Goal
Provide deterministic build-time control for Vulkan renderer and safe downgrade when dependencies are unavailable.

## 2. Switch Contract
- New switch: `ENABLE_VULKAN_RENDERER`.
- Defaults:
  - Linux (`UNIX AND NOT APPLE`): `ON`
  - Others: `OFF`

## 3. Guard and Downgrade Contract
- Platform guard:
  - if enabled on non-Linux, force OFF + warning.
- Linux dependency guard:
  - `pkg_check_modules(VULKAN QUIET vulkan)`
  - missing package -> force OFF + warning.

## 4. Build Output Contract
- Added compile definition:
  - `MVP_HAVE_VULKAN_RENDERER`
- Added conditional source list inclusion for Vulkan renderer.
- “At least one renderer enabled” rule now includes Vulkan switch.

## 5. Risk Control
- No hard failure on missing Vulkan package in current wave.
- Existing renderer matrix remains available when Vulkan is downgraded.
