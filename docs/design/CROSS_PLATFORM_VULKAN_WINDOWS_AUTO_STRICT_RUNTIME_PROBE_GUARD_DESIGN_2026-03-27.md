# Cross-platform Vulkan Windows auto strict runtime probe guard design (2026-03-27)

## 1. Goal
Prevent SDK-only signal from triggering premature strict mode in Windows Vulkan `auto` policy.

## 2. Design

### 2.1 Auto strict policy guard
- File: `tools/run_windows_vulkan_checks.ps1`
- `auto` policy decision:
  - strict when `MVP_WINDOWS_VULKAN_SDK_AVAILABLE` and `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE` are both truthy
  - otherwise optional
- Existing explicit strict controls remain:
  - CLI: `-RequireVulkanAvailable`
  - env: `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=on|1|true|yes`

### 2.2 Summary observability extension
- File: `tools/run_windows_vulkan_checks.ps1`
- Added machine-readable fields:
  - `windows-vulkan-check.runner_vulkan_runtime_probe_available`
  - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe`
  - `windows-vulkan-check.strict_mode_auto_prerequisites_met`

### 2.3 Workflow runtime probe signal
- File: `.github/workflows/cross-platform-gate.yml`
- Windows Vulkan SDK stage now:
  1. attempts SDK install/detection
  2. runs runtime probe via `vulkaninfo --summary` from SDK bin or PATH
  3. exports `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1|0`
  4. logs probe detail for troubleshooting

## 3. Compatibility and risk
- Safe downgrade behavior for non-Vulkan hosts is unchanged.
- Linux path is unaffected.
- On partially prepared runners (SDK present but runtime not ready), gate now stays optional in `auto` mode, reducing false strict failures.
