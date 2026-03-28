# Cross-platform Vulkan Windows strict-compiled-in-disabled expected-fail canary local check

Date: 2026-03-28  
Host: Windows workstation

## 1. Scope
- Validate `VK-045` deterministic strict-compiled-in-disabled expected-fail canary and workflow integration.

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
Key lines:
```text
windows-vulkan-check.result=SKIPPED
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
windows-vulkan-check.diag_dependency_source=disabled
```

### Strict-compiled-in-disabled expected-fail canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-gate-vk046.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-strict-compiled-in-disabled-canary.actual_gate_exit_code=2
windows-vulkan-strict-compiled-in-disabled-canary.gate_result=FAIL
windows-vulkan-strict-compiled-in-disabled-canary.gate_mode=strict
windows-vulkan-strict-compiled-in-disabled-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-strict-compiled-in-disabled-canary.gate_vulkan_availability_failure_detail=compiled-in-disabled
windows-vulkan-strict-compiled-in-disabled-canary.gate_playback_check_executed=false
windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_exit_code=0
windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_dependency_source=disabled
windows-vulkan-strict-compiled-in-disabled-canary.validation_failure_reason=none
windows-vulkan-strict-compiled-in-disabled-canary.result=PASS
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

### Static scan
```text
rg -n "run_windows_vulkan_gate_strict_compiled_in_disabled_canary|vulkanStrictCompiledInDisabledCanaryExitCode|Windows Vulkan Gate Strict Compiled-In-Disabled Canary|windows-vulkan-strict-compiled-in-disabled-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows Vulkan CI now has deterministic strict availability detail coverage for `compiled-in-disabled`.
- Strict unavailable detail branches (`compiled-in-false`, `runtime-unavailable`, `diag-exit-nonzero`, `diag-result-not-pass`, `compiled-in-disabled`) are all canary-protected.
