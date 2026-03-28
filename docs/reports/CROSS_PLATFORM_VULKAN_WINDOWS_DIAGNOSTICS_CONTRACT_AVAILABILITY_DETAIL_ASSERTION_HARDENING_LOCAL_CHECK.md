# Cross-platform Vulkan Windows diagnostics-contract availability detail assertion hardening local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-046` contract-canary availability-detail assertion and workflow wiring.

## 2. Commands and Results

### Build
```text
cmake --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk046-baseline.env"
```
Result: PASS

### Diagnostics-contract canary (with new availability-detail assertion)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk046.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-contract-canary.actual_gate_exit_code=2
windows-vulkan-contract-canary.gate_result=FAIL
windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
windows-vulkan-contract-canary.gate_diag_contract_valid=false
windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail=diag-contract-missing-required-fields
windows-vulkan-contract-canary.validation_failure_reason=none
windows-vulkan-contract-canary.result=PASS
```

### Full Windows Vulkan canary matrix regression
```text
PowerShell batch run with tag vk046 (contract/playback/pass/strict/optional/unsupported/playback-semantic chain)
```
Result: PASS  
Key line:
```text
ALL_VK046_CHECKS_PASS
```

### Static scans
```text
rg -n "run_windows_vulkan_gate_strict_compiled_in_disabled_canary|vulkanStrictCompiledInDisabledCanaryExitCode|Windows Vulkan Gate Strict Compiled-In-Disabled Canary|windows-vulkan-strict-compiled-in-disabled-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1
rg -n "gate_vulkan_availability_failure_detail|diag-contract-missing-required-fields|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- Contract expected-fail canary now locks down availability detail `diag-contract-missing-required-fields`.
- CI summary now surfaces this value directly for diagnostics-contract failures.
