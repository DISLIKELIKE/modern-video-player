# Cross-platform Vulkan Windows playback-contract expected-fail canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-031` playback-contract expected-fail canary and workflow integration.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk031-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Run playback-contract canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk031.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk031.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-playback-contract-canary.actual_gate_exit_code=2
windows-vulkan-playback-contract-canary.gate_summary_file_present=true
windows-vulkan-playback-contract-canary.gate_result=FAIL
windows-vulkan-playback-contract-canary.gate_failure_reason=vulkan-playback-contract-broken
windows-vulkan-playback-contract-canary.gate_playback_contract_valid=false
windows-vulkan-playback-contract-canary.gate_playback_failure_detail=contract-missing-required-fields
windows-vulkan-playback-contract-canary.validation_failure_reason=none
windows-vulkan-playback-contract-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_playback_contract_canary|vulkanPlaybackCanaryExitCode|Windows Vulkan Gate Playback Contract Canary|windows-vulkan-playback-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows CI now has deterministic playback-contract branch canary coverage.
- Playback-contract contract drift will fail CI with explicit summary and non-zero propagation.
