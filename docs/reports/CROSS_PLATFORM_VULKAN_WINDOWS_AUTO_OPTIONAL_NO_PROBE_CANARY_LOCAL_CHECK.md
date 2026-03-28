# Cross-platform Vulkan Windows auto optional no-probe canary local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-051` auto no-probe downgrade canary and full auto-policy branch matrix stability.

## 2. Commands and Results

### Auto optional no-probe canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-auto-optional-no-probe-canary.gate_mode=optional
windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_effective=false
windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_prerequisites_met=false
windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_runtime_probe_source=none
windows-vulkan-auto-optional-no-probe-canary.gate_result=SKIPPED
windows-vulkan-auto-optional-no-probe-canary.result=PASS
```

### Regression: auto strict native probe canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1
```
Result: PASS

### Regression: auto strict SwiftShader probe canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
```
Result: PASS

## 3. Conclusion
- Auto-policy downgrade branch is now deterministically covered.
- Windows Vulkan auto-policy matrix branch coverage is complete at canary level.
