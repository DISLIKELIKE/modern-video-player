# PLAYERCORE Day78: VK016 Windows Vulkan gate and CI integration

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan enablement has landed, but Windows gate/CI still lacked a dedicated Vulkan check contract.
- Existing Windows lane validated D3D11/OpenGL paths only, so Vulkan availability and override-path behavior were not continuously observable.

## 2. Gap Snapshot
- No Windows-side machine-readable Vulkan gate command.
- CI Windows lane did not explicitly execute a Vulkan-focused check stage.
- No optional/strict policy boundary for hosts where Vulkan dependency/runtime is unavailable.

## 3. Solution Direction
- Add a dedicated Windows Vulkan check script (`tools/run_windows_vulkan_checks.ps1`) that:
  - probes `--vulkan-diagnostics`
  - publishes machine-readable `windows-vulkan-check.*` output
  - supports optional mode (default) and strict mode (`-RequireVulkanAvailable`)
  - runs Vulkan override playback check only when probe says Vulkan is available
- Wire this script into `.github/workflows/cross-platform-gate.yml` Windows gate stage.
- Keep current host behavior deterministic:
  - unavailable Vulkan => `SKIPPED` in optional mode
  - unavailable Vulkan => `FAIL` in strict mode

## 4. DoD
- New Windows Vulkan gate command exists and is machine-readable.
- CI Windows lane executes Vulkan gate script.
- Optional vs strict mode behavior is explicit and testable locally.
- Documentation and records chain fully synchronized.

## 5. Outcome
- Added `run_windows_vulkan_checks.ps1` with:
  - diagnostics parse contract
  - conditional override playback verification contract
  - `windows-vulkan-check.result=PASS|SKIPPED|FAIL`
- Added workflow integration:
  - Windows configure explicitly keeps `-DENABLE_VULKAN_RENDERER=ON`
  - Windows gate now calls Vulkan check script before OpenGL checks
- Local validation completed:
  - configure/build PASS
  - optional mode output: `SKIPPED` on current host
  - strict mode output: `FAIL` with `failure_reason=vulkan-not-available-in-strict-mode`

## 6. Remaining
- Real CI evidence of `windows-vulkan-check.result=PASS` requires a Windows runner with Vulkan package/runtime available.
