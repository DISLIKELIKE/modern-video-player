# Cross-platform Vulkan Windows runtime probe detail observability local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-023` closure:
  - workflow exports runtime probe detail env
  - gate summary includes runtime probe detail key
  - `VK-022` strict behavior remains unchanged

## 2. Commands and Results

### Static path scan
```text
rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL|runner_vulkan_runtime_probe_detail" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
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

### Gate baseline (detail absent -> unknown)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk023-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.runner_vulkan_runtime_probe_available=false
windows-vulkan-check.runner_vulkan_runtime_probe_detail=unknown
windows-vulkan-check.result=SKIPPED
```

### Policy matrix A: auto + sdk=1 + runtime_probe=0 + detail=vulkaninfo-missing
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='0'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL='vulkaninfo-missing'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk023-auto-sdk1-runtime0.env"
```
Result: PASS (exit 0)  
Key lines:
```text
windows-vulkan-check.runner_vulkan_runtime_probe_detail=vulkaninfo-missing
windows-vulkan-check.strict_mode_auto_prerequisites_met=false
windows-vulkan-check.mode=optional
windows-vulkan-check.result=SKIPPED
```

### Policy matrix B: auto + sdk=1 + runtime_probe=1 + detail=vulkaninfo-path
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL='vulkaninfo-path'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk023-auto-sdk1-runtime1.env"
```
Result: expected FAIL on current host (exit 2)  
Key lines:
```text
windows-vulkan-check.runner_vulkan_runtime_probe_detail=vulkaninfo-path
windows-vulkan-check.strict_mode_auto_prerequisites_met=true
windows-vulkan-check.mode=strict
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```

## 3. Conclusion
- Runtime probe detail is now emitted as machine-readable gate output.
- Auto strict policy behavior remains identical to `VK-022`.
- Strict PASS still depends on a Vulkan-ready Windows environment.
