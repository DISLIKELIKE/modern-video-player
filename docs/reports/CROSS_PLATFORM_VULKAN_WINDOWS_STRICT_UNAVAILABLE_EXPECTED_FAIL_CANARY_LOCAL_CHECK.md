# Cross-platform Vulkan Windows strict-unavailable expected-fail canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-033` deterministic strict-unavailable expected-fail canary and workflow integration.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk033-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Diagnostics expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk033.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-contract-canary.result=PASS
```

### Playback expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk033.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-contract-canary.result=PASS
```

### PASS-contract canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk033.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-pass-contract-canary.result=PASS
```

### Strict-unavailable expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk033.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-strict-unavailable-canary.actual_gate_exit_code=2
windows-vulkan-strict-unavailable-canary.gate_result=FAIL
windows-vulkan-strict-unavailable-canary.gate_mode=strict
windows-vulkan-strict-unavailable-canary.gate_strict_mode_effective=true
windows-vulkan-strict-unavailable-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-strict-unavailable-canary.gate_vulkan_availability_failure_detail=compiled-in-false
windows-vulkan-strict-unavailable-canary.gate_playback_check_executed=false
windows-vulkan-strict-unavailable-canary.validation_failure_reason=none
windows-vulkan-strict-unavailable-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_strict_unavailable_canary|vulkanStrictUnavailableCanaryExitCode|Windows Vulkan Gate Strict Unavailable Canary|windows-vulkan-strict-unavailable-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows Vulkan CI now has deterministic coverage for strict-unavailable expected-fail branch.
- Combined canary set now covers diagnostics-contract fail, playback-contract fail, strict PASS, and strict-unavailable fail branches.
