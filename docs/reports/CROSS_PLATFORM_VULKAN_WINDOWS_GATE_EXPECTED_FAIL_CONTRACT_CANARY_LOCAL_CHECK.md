# Cross-platform Vulkan Windows gate expected-fail contract canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-029` expected-fail contract canary and workflow integration for Windows Vulkan gate.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk029-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.result=SKIPPED
windows-vulkan-check.mode=optional
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
```

### Run expected-fail contract canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk029.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk029.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-contract-canary.actual_gate_exit_code=2
windows-vulkan-contract-canary.gate_result=FAIL
windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
windows-vulkan-contract-canary.gate_diag_contract_valid=false
windows-vulkan-contract-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_contract_canary|vulkanCanaryExitCode|windows-vulkan-gate-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows CI gate now includes deterministic expected-fail Vulkan contract canary.
- Canary contract drift will fail CI with explicit non-zero propagation.
