# PLAYERCORE Day72: VK011 Linux gate Vulkan checks

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VK-010` already introduced `--vulkan-diagnostics`, but Linux gate script did not consume it.
- Without Linux gate integration, Vulkan diagnostics contract cannot be validated in gate execution flow.

## 2. Root Cause
- `tools/run_linux_mvp_checks.sh` only covered existing CP checks (`CP-401~CP-406`, `CP-507`, `CP-508`) and had no Vulkan stage.
- Gate lacked a capability-aware mechanism to decide whether Vulkan diagnostics should run or be skipped/fail-fast.

## 3. Solution
- Added Vulkan pre-probe and check integration in `tools/run_linux_mvp_checks.sh`:
  - new script option/env:
    - arg `11`: `REQUIRE_VULKAN_CHECKS`
    - env fallback: `MVP_REQUIRE_VULKAN_CHECKS`
  - new probe function:
    - `probe_vulkan_check_availability`
    - runs `--vulkan-diagnostics` once and records:
      - `supported_platform`
      - `compiled_in`
      - `runtime_available`
      - probe exit code
  - new conditional gate stage:
    - check id: `vk010_vulkan_diagnostics`
    - runs only when Vulkan diagnostics is available (`platform+compiled+runtime+exit=0`)
    - required machine-readable assertions:
      - `vulkan-diagnostics.startup_renderer_candidates=Vulkan -> OpenGL -> SoftwareSDL`
      - `startup_renderer_plan_reason` contains `linux-vulkan-fallback-chain`
      - `selected_renderer=Vulkan`
      - `fallback_target=OpenGL`
      - `result=PASS`
- Added gate report fields:
  - `gate.require_vulkan_checks`
  - `gate.vulkan_probe_exit_code`
  - `gate.vulkan_supported_platform`
  - `gate.vulkan_compiled_in`
  - `gate.vulkan_runtime_available`
  - `gate.has_vk010`
  - `gate.vulkan_skip_reason`
  - `check.vk010_vulkan_diagnostics.*`

## 4. Validation Outcome
- Static path check (`rg`) confirms new Vulkan gate integration points are present.
- `bash -n tools/run_linux_mvp_checks.sh`: PASS.
- `bash tools/run_linux_mvp_checks.sh` on Windows host: expected FAIL with:
  - `This gate script only supports Linux.`
- Local build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin`: PASS.
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` on Windows host remains expected `FAIL` (`supported_platform=false`).

## 5. Follow-up
- `VK-012` should wire Linux CI lane to enforce Vulkan gate check (`REQUIRE_VULKAN_CHECKS=1`) and ensure Vulkan dependency closure in CI image.
