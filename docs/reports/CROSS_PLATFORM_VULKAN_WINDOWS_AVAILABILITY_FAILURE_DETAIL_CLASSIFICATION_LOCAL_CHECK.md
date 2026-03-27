# Cross-platform Vulkan Windows availability failure detail classification local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-024` closure:
  - new availability-stage classification fields
  - no regression in strict/optional behavior

## 2. Commands and Results

### Static path scan
```text
rg -n "vulkan_availability_probe_passed|vulkan_availability_failure_detail|compiled-in-disabled|diag-result-not-pass" tools/run_windows_vulkan_checks.ps1
```
Result: PASS

### Configure
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
```
Result: PASS  
Key warning (expected on current host):
```text
ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF
```

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline gate output
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk024-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.vulkan_availability_probe_passed=false
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
windows-vulkan-check.result=SKIPPED
```

### Policy matrix A: auto + sdk=1 + runtime_probe=0
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='0'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL='vulkaninfo-missing'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk024-auto-sdk1-runtime0.env"
```
Result: PASS (exit 0)  
Key lines:
```text
windows-vulkan-check.strict_mode_auto_prerequisites_met=false
windows-vulkan-check.mode=optional
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
windows-vulkan-check.result=SKIPPED
```

### Policy matrix B: auto + sdk=1 + runtime_probe=1
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL='vulkaninfo-path'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk024-auto-sdk1-runtime1.env"
```
Result: expected FAIL on current host (exit 2)  
Key lines:
```text
windows-vulkan-check.strict_mode_auto_prerequisites_met=true
windows-vulkan-check.mode=strict
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```

## 3. Conclusion
- Availability-stage failure classification is now explicit and machine-readable.
- Existing strict/optional behavior remains unchanged.
- Strict PASS still depends on Vulkan-ready Windows environment.
