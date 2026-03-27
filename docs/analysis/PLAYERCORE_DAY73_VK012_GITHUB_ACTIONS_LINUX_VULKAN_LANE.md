# PLAYERCORE Day73: VK012 GitHub Actions Linux Vulkan lane

Date: 2026-03-27  
Status: Done

## 1. Problem
- `VK-011` added Linux gate Vulkan check support, but GitHub Actions Linux lane did not yet enforce Vulkan-ready build/check path.
- Without CI lane wiring, Vulkan gate checks could remain effectively optional in automation.

## 2. Root Cause
- Linux workflow dependencies did not explicitly include Vulkan development/runtime packages.
- Linux configure step did not explicitly pin `ENABLE_VULKAN_RENDERER=ON`.
- Linux gate invocation did not pass strict Vulkan requirement parameter (`REQUIRE_VULKAN_CHECKS`).

## 3. Solution
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - dependency install:
    - `libvulkan-dev`
    - `mesa-vulkan-drivers`
  - configure step:
    - explicit `-DENABLE_VULKAN_RENDERER=ON`
  - Linux gate call:
    - appended arg `11` as `1` to enforce Vulkan check requirement in gate script.
- This makes CI lane consume `VK-011` contract and fail early if Vulkan check cannot run/pass.

## 4. Validation Outcome
- Workflow static scan confirms expected fields are present:
  - Vulkan packages in Linux dependency list
  - explicit `ENABLE_VULKAN_RENDERER=ON`
  - Linux gate strict Vulkan argument
- Local script/build regression:
  - `bash -n tools/run_linux_mvp_checks.sh`: PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - `--vulkan-diagnostics` on Windows host: expected FAIL (`supported_platform=false`)
- Limitation:
  - real Linux runner execution proof remains CI-side and requires workflow run.

## 5. Follow-up
- `VK-013` should execute regression matrix (open/play/pause/seek/subtitle/fallback) with Vulkan lane in place.
