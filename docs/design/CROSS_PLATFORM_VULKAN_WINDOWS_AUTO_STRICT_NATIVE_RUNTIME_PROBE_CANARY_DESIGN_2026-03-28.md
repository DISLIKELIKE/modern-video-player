# Cross-platform Vulkan Windows auto strict native runtime probe canary design (2026-03-28)

## 1. Goal
Add deterministic canary protection for the Windows Vulkan auto strict native-probe branch after strict-policy expansion in `VK-049`.

## 2. Design

### 2.1 Canary scenario
- New script:
  - `tools/run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1`
- Injected environment:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Mock executable emits PASS diagnostics/playback contract.

### 2.2 Assertions
- Gate result path:
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.strict_mode_effective=true`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source=native`
  - `windows-vulkan-check.result=PASS`
- Canary output contract:
  - `windows-vulkan-auto-strict-native-probe-canary.*`

### 2.3 CI gate integration
- File:
  - `tools/run_windows_ci_gate.ps1`
- Add execution and Step Summary section:
  - `Windows Vulkan Gate Auto Strict Native Probe Canary`

## 3. Compatibility and Risk
- No change to production gate decision logic; this round is branch-coverage hardening.
- Existing strict/optional paths remain unchanged.
