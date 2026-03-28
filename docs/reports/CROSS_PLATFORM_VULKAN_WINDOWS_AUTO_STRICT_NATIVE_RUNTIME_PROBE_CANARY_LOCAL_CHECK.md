# Cross-platform Vulkan Windows auto strict native runtime probe canary local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-050` native auto strict branch coverage and no regression on existing auto-policy canaries.

## 2. Commands and Results

### New native-probe auto strict canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-auto-strict-native-probe-canary.gate_mode=strict
windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_effective=true
windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_runtime_probe_source=native
windows-vulkan-auto-strict-native-probe-canary.result=PASS
```

### Regression: SwiftShader auto strict canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
```
Result: PASS

### Regression: optional skip canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1
```
Result: PASS

## 3. Conclusion
- Native-probe auto strict branch is now deterministically canary-protected.
- Existing auto strict SwiftShader and optional downgrade behavior remain stable.
