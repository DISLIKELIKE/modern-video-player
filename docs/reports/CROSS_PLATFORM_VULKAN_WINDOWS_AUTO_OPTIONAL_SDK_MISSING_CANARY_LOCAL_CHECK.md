# Cross-platform Vulkan Windows auto optional sdk-missing canary local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-053` sdk-missing auto downgrade branch.

## 2. Commands and Results

### Auto optional sdk-missing canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-auto-optional-sdk-missing-canary.gate_mode=optional
windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_effective=false
windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_prerequisites_met=false
windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_any_available=true
windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_source=native
windows-vulkan-auto-optional-sdk-missing-canary.gate_result=SKIPPED
windows-vulkan-auto-optional-sdk-missing-canary.result=PASS
```

## 3. Conclusion
- SDK-missing boundary no longer relies on implicit behavior and is now canary-protected.
