# Cross-platform Vulkan Windows gate and CI integration design (2026-03-27)

## 1. Goal
Provide a Windows Vulkan gate contract that is machine-readable, CI-integrated, and safe on hosts without Vulkan availability.

## 2. Design

### 2.1 Command Contract
- New command wrapper script:
  - `tools/run_windows_vulkan_checks.ps1`
- Inputs:
  - `-ExecutablePath`
  - `-ProbeFile`
  - `-SampleMs`
  - `-RequireVulkanAvailable` (strict mode switch)
- Outputs:
  - machine-readable `windows-vulkan-check.*` fields
  - final `windows-vulkan-check.result=PASS|SKIPPED|FAIL`

### 2.2 Probe and Execution Policy
- Always execute `--vulkan-diagnostics` and parse key fields.
- Optional mode (default):
  - if Vulkan unavailable => `SKIPPED`
  - if Vulkan available => run override playback check
- Strict mode:
  - Vulkan unavailable => `FAIL`
  - Vulkan available but playback check fails => `FAIL`

### 2.3 Playback Verification (when Vulkan available)
- Run `MVP_RENDERER_BACKEND=vulkan --performance-log-check ...`
- Require:
  - `performance-log-check.result=PASS`
  - `startup_renderer_plan_reason=renderer-override-env`
  - `startup_selected_renderer=Vulkan`
  - `renderer_backend=Vulkan`

### 2.4 CI Wiring
- `.github/workflows/cross-platform-gate.yml` Windows lane:
  - configure explicitly with `-DENABLE_VULKAN_RENDERER=ON`
  - add `run_windows_vulkan_checks.ps1` into Windows gate sequence

## 3. Compatibility
- Keeps existing Windows D3D11/OpenGL gate checks unchanged.
- Adds observability without forcing strict Vulkan requirement by default.
- Enables future strict enforcement by passing `-RequireVulkanAvailable`.
