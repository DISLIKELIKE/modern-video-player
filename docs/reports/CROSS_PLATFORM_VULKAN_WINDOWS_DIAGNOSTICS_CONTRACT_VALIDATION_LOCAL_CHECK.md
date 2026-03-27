# Cross-platform Vulkan Windows diagnostics contract validation local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-025` diagnostics contract validation and observability fields.

## 2. Commands and Results

### Static path scan
```text
rg -n "diag_contract_valid|diag_missing_required_fields|vulkan-diagnostics-contract-broken|diag-contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1
```
Result: PASS

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline gate (valid diagnostics contract)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk025-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.diag_contract_valid=true
windows-vulkan-check.diag_missing_required_fields=none
windows-vulkan-check.result=SKIPPED
```

### Contract-broken simulation
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "C:/Windows/System32/cmd.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk025-contract-broken.env"
```
Result: expected FAIL (exit 2)  
Key lines:
```text
windows-vulkan-check.diag_contract_valid=false
windows-vulkan-check.diag_missing_required_fields=vulkan-diagnostics.platform,vulkan-diagnostics.supported_platform,vulkan-diagnostics.compiled_in,vulkan-diagnostics.runtime_available,vulkan-diagnostics.result
windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken
windows-vulkan-check.vulkan_availability_failure_detail=diag-contract-missing-required-fields
windows-vulkan-check.result=FAIL
```

### Behavior compatibility spot-check
```text
auto + sdk=1 + runtime_probe=0 => result=SKIPPED (exit 0)
auto + sdk=1 + runtime_probe=1 => result=FAIL (exit 2, expected on current host)
```
Result: PASS

## 3. Conclusion
- Diagnostics contract regressions are now surfaced as explicit gate failures.
- Valid-path strict/optional behavior remains unchanged.
- Strict PASS still depends on Vulkan-ready Windows environment.
