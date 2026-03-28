# Cross-platform Vulkan Windows auto strict SwiftShader runtime probe promotion design (2026-03-28)

## 1. Goal
Allow Windows Vulkan `auto` strict mode to promote on CI hosts where Vulkan availability is provided by SwiftShader runtime probe instead of native probe.

## 2. Design

### 2.1 Auto strict prerequisite expansion
- File: `tools/run_windows_vulkan_checks.ps1`
- Previous rule:
  - `strict_auto = sdk_available && native_runtime_probe_available`
- New rule:
  - `strict_auto = sdk_available && (native_runtime_probe_available || swiftshader_runtime_probe_available)`
- Explicit strict controls remain unchanged:
  - CLI `-RequireVulkanAvailable`
  - env `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=on|1|true|yes`

### 2.2 Summary observability extension
- File: `tools/run_windows_vulkan_checks.ps1`
- New/updated fields:
  - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe_or_swiftshader_probe`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_any_available`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source` (`none|native|swiftshader|native+swiftshader`)

### 2.3 Canary coverage and CI wiring
- New file:
  - `tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1`
- Canary scenario:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=0`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=1`
- Expected:
  - gate `mode=strict`
  - `strict_mode_effective=true`
  - gate result `PASS`
- Integration:
  - `tools/run_windows_ci_gate.ps1` executes canary and prints Step Summary table.

## 3. Compatibility and Risk
- Linux path unaffected.
- Existing strict semantics for explicit CLI/env overrides unchanged.
- Optional downgrade path remains intact when both runtime probes are unavailable.
