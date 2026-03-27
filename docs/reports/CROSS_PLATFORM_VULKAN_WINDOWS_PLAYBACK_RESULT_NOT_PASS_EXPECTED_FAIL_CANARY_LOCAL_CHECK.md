# Cross-platform Vulkan Windows playback-result-not-pass expected-fail canary local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-040` deterministic playback-result-not-pass expected-fail canary and workflow integration.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk040-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Diagnostics expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-contract-canary.result=PASS
```

### Playback expected-fail canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-contract-canary.result=PASS
```

### PASS-contract canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-pass-contract-canary.result=PASS
```

### Strict-unavailable canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-strict-unavailable-canary.result=PASS
```

### Optional-skip canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-optional-skip-canary.result=PASS
```

### Unsupported-platform canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-unsupported-platform-canary.result=PASS
```

### Playback-semantic canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-semantic-canary.result=PASS
```

### Playback-backend semantic canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-backend-semantic-canary.result=PASS
```

### Playback-candidates semantic canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-candidates-semantic-canary.result=PASS
```

### Playback-plan-reason semantic canary regression
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk040.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-playback-plan-reason-semantic-canary.result=PASS
```

### Playback-result-not-pass expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk040.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-playback-result-not-pass-canary.actual_gate_exit_code=2
windows-vulkan-playback-result-not-pass-canary.gate_result=FAIL
windows-vulkan-playback-result-not-pass-canary.gate_failure_reason=vulkan-playback-check-failed
windows-vulkan-playback-result-not-pass-canary.gate_playback_contract_valid=true
windows-vulkan-playback-result-not-pass-canary.gate_playback_failure_detail=result-not-pass
windows-vulkan-playback-result-not-pass-canary.gate_playback_result=FAIL
windows-vulkan-playback-result-not-pass-canary.gate_playback_selected_renderer=Vulkan
windows-vulkan-playback-result-not-pass-canary.gate_playback_renderer_backend=Vulkan
windows-vulkan-playback-result-not-pass-canary.validation_failure_reason=none
windows-vulkan-playback-result-not-pass-canary.result=PASS
```

### Static scan
```text
rg -n "run_windows_vulkan_gate_playback_result_not_pass_canary|vulkanPlaybackResultNotPassCanaryExitCode|Windows Vulkan Gate Playback Result-Not-Pass Canary|windows-vulkan-playback-result-not-pass-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows Vulkan CI now has deterministic coverage for playback `result-not-pass` expected-fail branch.
- Combined canary matrix now covers diagnostics contract fail, playback contract fail, strict PASS, strict-unavailable fail, optional-skip, unsupported-platform fail, selected-renderer semantic fail, backend semantic fail, candidates semantic fail, plan-reason semantic fail, and result-not-pass fail branches.
