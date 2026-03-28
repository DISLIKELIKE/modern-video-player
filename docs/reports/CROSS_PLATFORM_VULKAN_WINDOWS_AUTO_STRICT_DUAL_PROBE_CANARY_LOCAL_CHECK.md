# Cross-platform Vulkan Windows auto strict dual-probe canary local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-052` dual-probe auto strict canary branch and source classification.

## 2. Commands and Results

### Dual-probe auto strict canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1
```
Result: PASS  
Key lines:
```text
windows-vulkan-auto-strict-dual-probe-canary.gate_mode=strict
windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_effective=true
windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_runtime_probe_source=native+swiftshader
windows-vulkan-auto-strict-dual-probe-canary.gate_result=PASS
windows-vulkan-auto-strict-dual-probe-canary.result=PASS
```

## 3. Conclusion
- Dual-probe branch is now deterministically canary-protected.
- Auto-policy source classification coverage is complete at canary layer.
