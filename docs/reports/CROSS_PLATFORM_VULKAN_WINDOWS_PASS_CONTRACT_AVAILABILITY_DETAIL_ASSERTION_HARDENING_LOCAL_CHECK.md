# Cross-platform Vulkan Windows pass-contract availability detail assertion hardening local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-048` PASS-contract canary availability-detail assertion and workflow wiring.

## 2. Commands and Results

### Build
```text
cmake --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk048-baseline.env"
```
Result: PASS

### PASS-contract canary (with availability-detail assertion)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk048.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk048.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-pass-contract-canary.actual_gate_exit_code=0
windows-vulkan-pass-contract-canary.gate_result=PASS
windows-vulkan-pass-contract-canary.gate_mode=strict
windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail=none
windows-vulkan-pass-contract-canary.gate_playback_contract_valid=true
windows-vulkan-pass-contract-canary.gate_playback_failure_detail=none
windows-vulkan-pass-contract-canary.validation_failure_reason=none
windows-vulkan-pass-contract-canary.result=PASS
```

### Full Windows Vulkan canary matrix regression
```text
PowerShell batch run with tag vk048 (contract/playback/pass/strict/optional/unsupported/playback-semantic chain)
```
Result: PASS  
Key line:
```text
ALL_VK048_CHECKS_PASS
```

### Static scan
```text
rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_pass_contract_canary|Windows Vulkan Gate PASS Contract Canary|vulkanPassCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- PASS-contract canary now locks down success-path availability detail `none`.
- CI summary now surfaces PASS-contract availability detail for quick diagnostics.
