# Cross-platform Vulkan Windows auto strict SwiftShader runtime probe promotion local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-049`:
  - auto strict promotes with SwiftShader runtime probe
  - new summary observability fields are emitted
  - existing canaries remain stable

## 2. Commands and Results

### Static scan
```text
rg -n "sdk_and_runtime_probe_or_swiftshader_probe|strict_mode_auto_runtime_probe_any_available|strict_mode_auto_runtime_probe_source|auto_strict_swiftshader_probe_canary" tools/run_windows_vulkan_checks.ps1 tools/run_windows_ci_gate.ps1 tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
```
Result: PASS

### New canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-auto-strict-swiftshader-probe-canary.gate_mode=strict
windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_effective=true
windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_basis=sdk_and_runtime_probe_or_swiftshader_probe
windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_runtime_probe_source=swiftshader
windows-vulkan-auto-strict-swiftshader-probe-canary.result=PASS
```

### Regression canary: optional skip
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-optional-skip-canary.gate_mode=optional
windows-vulkan-optional-skip-canary.gate_strict_mode_effective=false
windows-vulkan-optional-skip-canary.result=PASS
```

### Regression canary: pass contract
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-pass-contract-canary.gate_mode=strict
windows-vulkan-pass-contract-canary.gate_strict_mode_effective=true
windows-vulkan-pass-contract-canary.result=PASS
```

## 3. Conclusion
- `VK-049` auto strict policy extension works as expected.
- SwiftShader runtime probe can now satisfy auto strict promotion.
- Existing Windows Vulkan canary behavior remains compatible.
