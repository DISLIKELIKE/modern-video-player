# PLAYERCORE Day70: VK009 fallback chain and startup policy

Date: 2026-03-27  
Status: Done

## 1. Problem
- Vulkan startup fallback chain on Linux was primarily implied by renderer priority ordering, not explicitly normalized by policy.
- Startup policy observability lacked machine-readable plan-reason fields, making strategy decisions harder to audit from command output.

## 2. Root Cause
- `PlaybackStrategy` generated renderer candidates by generic order append logic, with no Linux Vulkan chain-normalization contract.
- `PlayerCore` diagnostics snapshot exported candidates/fallback reasons, but not `renderer_plan_reason` / `decoder_plan_reason`.

## 3. Solution
- Added explicit Linux Vulkan fallback-chain normalization in `PlaybackStrategy`:
  - when Linux and first renderer candidate is `Vulkan`, normalize candidate prefix to:
    - `Vulkan -> OpenGL -> SoftwareSDL` (only for runtime-available backends)
  - append strategy reason tag:
    - `linux-vulkan-fallback-chain`
- Extended startup policy observability:
  - added diagnostics fields:
    - `startup_renderer_plan_reason`
    - `startup_decoder_plan_reason`
  - exposed both fields in `--performance-log-check` machine-readable output.

## 4. Validation Outcome
- Local checks PASS:
  - strategy/diagnostic symbol scan PASS.
  - Release build PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
  - `--performance-log-check` PASS with new fields:
    - `performance-log-check.startup_renderer_plan_reason=...`
    - `performance-log-check.startup_decoder_plan_reason=...`
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS on Windows host.
- Limitation:
  - Linux Vulkan runtime chain proof still requires Linux host/runner execution.

## 5. Remaining
- `VK-010` should add dedicated Vulkan diagnostics CLI (`--vulkan-diagnostics`) for direct Vulkan-path machine-readable probing.
