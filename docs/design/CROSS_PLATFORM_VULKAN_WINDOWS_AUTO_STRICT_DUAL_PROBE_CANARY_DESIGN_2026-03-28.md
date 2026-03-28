# Cross-platform Vulkan Windows auto strict dual-probe canary design (2026-03-28)

## 1. Goal
Add deterministic canary coverage for Windows Vulkan auto strict branch where both native probe and SwiftShader probe are available.

## 2. Design

### 2.1 Canary scenario
- New script:
  - `tools/run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1`
- Injected environment:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=1`
- Mock diagnostics/playback emit PASS contracts.

### 2.2 Assertions
- Gate outcome:
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.strict_mode_effective=true`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source=native+swiftshader`
  - `windows-vulkan-check.result=PASS`
- Canary summary namespace:
  - `windows-vulkan-auto-strict-dual-probe-canary.*`

### 2.3 CI integration
- File:
  - `tools/run_windows_ci_gate.ps1`
- New stage and Step Summary section:
  - `Windows Vulkan Gate Auto Strict Dual-Probe Canary`

## 3. Compatibility and Risk
- No production policy behavior change.
- This round is branch-coverage hardening for auto strict source classification.
