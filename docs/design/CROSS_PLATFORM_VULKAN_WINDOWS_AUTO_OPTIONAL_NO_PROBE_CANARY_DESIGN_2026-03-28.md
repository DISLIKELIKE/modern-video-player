# Cross-platform Vulkan Windows auto optional no-probe canary design (2026-03-28)

## 1. Goal
Add deterministic canary coverage for Windows Vulkan `auto` downgrade behavior when runtime probe prerequisites are not met.

## 2. Design

### 2.1 Canary scenario
- New script:
  - `tools/run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1`
- Injected environment:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=0`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Mock diagnostics force Vulkan unavailable (`compiled_in=false`, `runtime_available=false`) to exercise skip path.

### 2.2 Assertions
- Gate outcome:
  - `windows-vulkan-check.mode=optional`
  - `windows-vulkan-check.strict_mode_effective=false`
  - `windows-vulkan-check.strict_mode_auto_prerequisites_met=false`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source=none`
  - `windows-vulkan-check.result=SKIPPED`
- Canary summary namespace:
  - `windows-vulkan-auto-optional-no-probe-canary.*`

### 2.3 CI gate integration
- File:
  - `tools/run_windows_ci_gate.ps1`
- Add stage + Step Summary section:
  - `Windows Vulkan Gate Auto Optional No-Probe Canary`

## 3. Compatibility and Risk
- No behavior changes in production policy logic.
- This round closes branch-coverage gap for `auto` downgrade path only.
