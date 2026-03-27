# Cross-platform Vulkan Windows playback-semantic expected-fail canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-036` deterministic playback-semantic expected-fail canary and workflow integration.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk036-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Diagnostics expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-contract-canary.result=PASS
```

### Playback expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-contract-canary.result=PASS
```

### PASS-contract canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-pass-contract-canary.result=PASS
```

### Strict-unavailable canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-strict-unavailable-canary.result=PASS
```

### Optional-skip canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-optional-skip-canary.result=PASS
```

### Unsupported-platform canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk036.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-unsupported-platform-canary.result=PASS
```

### Playback-semantic expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk036.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-playback-semantic-canary.actual_gate_exit_code=2
windows-vulkan-playback-semantic-canary.gate_result=FAIL
windows-vulkan-playback-semantic-canary.gate_failure_reason=vulkan-playback-check-failed
windows-vulkan-playback-semantic-canary.gate_playback_contract_valid=true
windows-vulkan-playback-semantic-canary.gate_playback_failure_detail=selected-renderer-not-vulkan
windows-vulkan-playback-semantic-canary.gate_playback_selected_renderer=D3D11
windows-vulkan-playback-semantic-canary.validation_failure_reason=none
windows-vulkan-playback-semantic-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_playback_semantic_canary|vulkanPlaybackSemanticCanaryExitCode|Windows Vulkan Gate Playback Semantic Canary|windows-vulkan-playback-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_semantic_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows Vulkan CI now has deterministic coverage for playback-semantic expected-fail branch.
- Combined canary matrix now covers diagnostics contract fail, playback contract fail, strict PASS, strict-unavailable fail, optional-unavailable skip, unsupported-platform fail, and playback-semantic fail branches.
