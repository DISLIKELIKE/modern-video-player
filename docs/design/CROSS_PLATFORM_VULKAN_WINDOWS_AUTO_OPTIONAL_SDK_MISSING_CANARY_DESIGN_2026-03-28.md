# Cross-platform Vulkan Windows auto optional sdk-missing canary design (2026-03-28)

## 1. Goal
Add deterministic canary coverage for `auto` downgrade path when SDK availability signal is missing, even if runtime probe is available.

## 2. Design

### 2.1 Canary scenario
- New script:
  - `tools/run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1`
- Injected environment:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=0`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Mock diagnostics force Vulkan unavailable (`compiled_in=false`, `runtime_available=false`) to assert optional skip path.

### 2.2 Assertions
- Gate outcome:
  - `windows-vulkan-check.mode=optional`
  - `windows-vulkan-check.strict_mode_effective=false`
  - `windows-vulkan-check.strict_mode_auto_prerequisites_met=false`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_any_available=true`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source=native`
  - `windows-vulkan-check.result=SKIPPED`
- Canary summary namespace:
  - `windows-vulkan-auto-optional-sdk-missing-canary.*`

### 2.3 CI integration
- File:
  - `tools/run_windows_ci_gate.ps1`
- New stage + Step Summary section:
  - `Windows Vulkan Gate Auto Optional SDK-Missing Canary`

## 3. Compatibility and Risk
- No production policy behavior changes.
- This round is branch-coverage hardening for auto prerequisites boundary.
