# Cross-platform Vulkan Windows unsupported-platform availability detail assertion hardening local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-047` unsupported-platform canary availability-detail assertion and workflow wiring.

## 2. Commands and Results

### Build
```text
cmake --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk047-baseline.env"
```
Result: PASS

### Unsupported-platform canary (with availability-detail assertion)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk047.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk047.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-unsupported-platform-canary.actual_gate_exit_code=2
windows-vulkan-unsupported-platform-canary.gate_result=FAIL
windows-vulkan-unsupported-platform-canary.gate_failure_reason=unsupported-platform
windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail=unsupported-platform
windows-vulkan-unsupported-platform-canary.gate_playback_check_executed=false
windows-vulkan-unsupported-platform-canary.validation_failure_reason=none
windows-vulkan-unsupported-platform-canary.result=PASS
```

### Full Windows Vulkan canary matrix regression
```text
PowerShell batch run with tag vk047 (contract/playback/pass/strict/optional/unsupported/playback-semantic chain)
```
Result: PASS  
Key line:
```text
ALL_VK047_CHECKS_PASS
```

### Static scan
```text
rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_unsupported_platform_canary|Windows Vulkan Gate Unsupported Platform Canary|vulkanUnsupportedPlatformCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1
```
Result: PASS

## 3. Conclusion
- Unsupported-platform expected-fail canary now locks down availability detail `unsupported-platform`.
- CI summary now surfaces unsupported-platform detail directly for this branch.
