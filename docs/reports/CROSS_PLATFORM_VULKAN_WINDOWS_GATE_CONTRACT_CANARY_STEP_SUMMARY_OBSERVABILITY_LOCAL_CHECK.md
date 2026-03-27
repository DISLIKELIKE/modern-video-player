# Cross-platform Vulkan Windows gate contract-canary Step Summary observability local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-030` canary Step Summary observability and compatibility.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk030-baseline.env"
```
Result: PASS  
Key line:
```text
windows-vulkan-check.result=SKIPPED
```

### Run contract canary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk030.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk030.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-contract-canary.actual_gate_exit_code=2
windows-vulkan-contract-canary.gate_summary_file_present=true
windows-vulkan-contract-canary.gate_result=FAIL
windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
windows-vulkan-contract-canary.gate_diag_contract_valid=false
windows-vulkan-contract-canary.result=PASS
```

### Local Step Summary preview rendering
```text
Parse logs/windows-vulkan-gate-contract-canary-summary-vk030.env and write:
logs/windows-vulkan-canary-step-summary-preview-vk030.md
```
Result: PASS  
Rendered key lines:
```text
| result | PASS |
| expected_gate_exit_code | 2 |
| actual_gate_exit_code | 2 |
| gate_summary_file_present | true |
| gate_result | FAIL |
| gate_failure_reason | vulkan-diagnostics-contract-broken |
| gate_diag_contract_valid | false |
| validation_failure_reason | none |
```

### Static scan
```text
rg -n "Windows Vulkan Gate Contract Canary|gate_summary_file_present|vulkanCanarySummaryPath|vulkanCanaryExitCode|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
```
Result: PASS

## 3. Conclusion
- Windows CI Step Summary now includes dedicated contract-canary section with machine-readable key signals.
- Canary pass/fail semantics remain unchanged and continue to fail CI on contract drift.
