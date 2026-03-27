# PLAYERCORE Day81: VK019 Windows Vulkan auto strict policy promotion

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- After `VK-018`, CI can provision/probe Windows Vulkan SDK and publish SDK availability, but strict policy default remained manually controlled.
- Without auto policy promotion, Windows Vulkan checks could continue in optional mode and remain `SKIPPED` even when runner SDK is marked available.

## 2. Gap Snapshot
- `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS` had boolean semantics only.
- No built-in `auto` policy that upgrades to strict when runner Vulkan SDK availability signal is true.

## 3. Solution Direction
- Extend strict policy parsing in `run_windows_vulkan_checks.ps1`:
  - `auto`: strict when `MVP_WINDOWS_VULKAN_SDK_AVAILABLE` is truthy
  - `on/1/true/yes`: force strict
  - `off/0/false/no`: keep optional
  - CLI `-RequireVulkanAvailable` still overrides and forces strict
- Promote workflow default policy from `0` to `auto`.
- Add new machine-readable fields:
  - `windows-vulkan-check.strict_mode_policy`
  - `windows-vulkan-check.strict_mode_cli_requested`
  - `windows-vulkan-check.strict_mode_effective`

## 4. DoD
- Auto policy exists and is machine-readable.
- Workflow default uses auto policy.
- Local matrix validates auto/off behavior under simulated runner sdk availability values.
- Docs/records/index chain synchronized.

## 5. Outcome
- Script now supports auto strict promotion policy and reports effective strict state.
- CI job env now defaults to `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`.
- Local matrix confirms:
  - auto + sdk=0 => optional (`SKIPPED`)
  - auto + sdk=1 => strict (`FAIL` on current host without Vulkan compile/runtime)
  - off + sdk=1 => optional (`SKIPPED`)

## 6. Remaining
- Runtime PASS in strict mode still depends on Vulkan-ready Windows runner with compile/runtime availability.
