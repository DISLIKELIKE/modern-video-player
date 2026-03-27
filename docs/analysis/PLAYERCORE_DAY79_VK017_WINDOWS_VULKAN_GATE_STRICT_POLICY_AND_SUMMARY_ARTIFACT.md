# PLAYERCORE Day79: VK017 Windows Vulkan gate strict policy and summary artifact

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-016` introduced Windows Vulkan gate command and CI integration, but still lacked two operational pieces:
  - no summary artifact output file from the Windows Vulkan gate command
  - no environment-based strict policy toggle for CI/runtime escalation

## 2. Gap Snapshot
- `run_windows_vulkan_checks.ps1` printed machine-readable lines to stdout only.
- CI Windows lane could not archive a dedicated Vulkan gate summary env file.
- Strict mode required explicit CLI switch only, with no CI environment policy hook.

## 3. Solution Direction
- Extend `run_windows_vulkan_checks.ps1`:
  - add `-SummaryOutputPath` to write `windows-vulkan-check.*` lines to file
  - add environment hook `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS` as strict-policy source
  - publish `windows-vulkan-check.strict_mode_env_requested`
- Update workflow Windows lane:
  - set default `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=0`
  - create `logs/` directory
  - run Vulkan gate script with summary output path and tee log capture

## 4. DoD
- Windows Vulkan gate can emit persisted summary artifact file.
- Strict/optional policy is controllable by environment in addition to CLI switch.
- CI Windows lane archives Vulkan gate log + summary through existing artifact path.
- Full docs/records/index chain synchronized.

## 5. Outcome
- Added summary artifact support and strict-env policy in `run_windows_vulkan_checks.ps1`.
- Added workflow env default and summary/log artifact capture in Windows gate step.
- Local checks confirm:
  - optional mode summary file emits `result=SKIPPED` on current host
  - env-driven strict mode emits `result=FAIL` + exit code `2` on current host

## 6. Remaining
- PASS-path runtime proof for strict mode still requires Windows host/runner with Vulkan dependency/runtime available.
