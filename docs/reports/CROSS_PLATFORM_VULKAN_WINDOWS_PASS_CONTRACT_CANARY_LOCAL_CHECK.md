# Cross-platform Vulkan Windows PASS-contract canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-032` deterministic PASS-contract canary and workflow integration.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk032-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Run diagnostics expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk032.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-contract-canary.actual_gate_exit_code=2
windows-vulkan-contract-canary.gate_result=FAIL
windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
windows-vulkan-contract-canary.result=PASS
```

### Run playback expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk032.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-playback-contract-canary.actual_gate_exit_code=2
windows-vulkan-playback-contract-canary.gate_result=FAIL
windows-vulkan-playback-contract-canary.gate_failure_reason=vulkan-playback-contract-broken
windows-vulkan-playback-contract-canary.result=PASS
```

### Run PASS-contract canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk032.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-pass-contract-canary.actual_gate_exit_code=0
windows-vulkan-pass-contract-canary.gate_result=PASS
windows-vulkan-pass-contract-canary.gate_mode=strict
windows-vulkan-pass-contract-canary.gate_strict_mode_effective=true
windows-vulkan-pass-contract-canary.gate_playback_contract_valid=true
windows-vulkan-pass-contract-canary.gate_playback_failure_detail=none
windows-vulkan-pass-contract-canary.validation_failure_reason=none
windows-vulkan-pass-contract-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_pass_contract_canary|vulkanPassCanaryExitCode|Windows Vulkan Gate PASS Contract Canary|windows-vulkan-pass-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows Vulkan CI now has deterministic canary coverage for both expected-fail branches and PASS branch contract.
- Workflow Step Summary and fail-fast propagation are complete for the PASS canary lane.
