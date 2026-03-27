# Cross-platform Vulkan Linux gate checks design (2026-03-27)

## 1. Goal
Deliver `VK-011`: integrate Vulkan diagnostics check into Linux gate script with capability-aware behavior.

## 2. Scope
- In scope:
  - `tools/run_linux_mvp_checks.sh` Vulkan probe + optional gate check stage
  - machine-readable report fields for Vulkan probe/check status
- Out of scope:
  - CI workflow lane changes (`VK-012`)
  - Vulkan runtime implementation changes

## 3. Gate Behavior Contract
- Probe stage:
  - execute `--vulkan-diagnostics`
  - parse key fields:
    - `supported_platform`
    - `compiled_in`
    - `runtime_available`
  - determine `HAS_VK010`:
    - `1` only when all true and exit code is `0`
    - otherwise `0`
- Execution policy:
  - if `HAS_VK010=1`, execute `vk010_vulkan_diagnostics` check
  - if `HAS_VK010=0`, mark check as `SKIPPED` with machine-readable reason
  - if `REQUIRE_VULKAN_CHECKS=1` (arg11/env), unavailable Vulkan probe is a hard fail

## 4. Check Contract (`vk010_vulkan_diagnostics`)
- Command:
  - `<player> --vulkan-diagnostics`
- Required assertions:
  - `supported_platform=true`
  - `compiled_in=true`
  - `runtime_available=true`
  - candidate chain includes Linux fallback prefix:
    - `Vulkan -> OpenGL -> SoftwareSDL`
  - plan reason contains:
    - `linux-vulkan-fallback-chain`
  - `selected_renderer=Vulkan`
  - `fallback_target=OpenGL`
  - `result=PASS`

## 5. Report Contract Extension
- Gate-level:
  - `gate.require_vulkan_checks`
  - `gate.vulkan_probe_exit_code`
  - `gate.vulkan_supported_platform`
  - `gate.vulkan_compiled_in`
  - `gate.vulkan_runtime_available`
  - `gate.has_vk010`
  - `gate.vulkan_skip_reason`
- Check-level:
  - `check.vk010_vulkan_diagnostics.status`
  - `check.vk010_vulkan_diagnostics.reason` (when skipped)
