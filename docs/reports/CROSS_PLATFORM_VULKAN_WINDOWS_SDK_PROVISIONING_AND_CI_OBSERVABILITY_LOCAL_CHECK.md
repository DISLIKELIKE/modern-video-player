# Cross-platform Vulkan Windows SDK provisioning and CI observability local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-018` follow-up for Windows Vulkan SDK provisioning workflow stage and SDK observability fields.

## 2. Commands and Results

### Static path scan
```text
rg -n "Install Windows Vulkan SDK|VULKAN_SDK|MVP_WINDOWS_VULKAN_SDK_AVAILABLE|vulkan_sdk_present|runner_vulkan_sdk_available" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
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

### Windows Vulkan check with summary output
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
```
Result: PASS
Key lines:
```text
windows-vulkan-check.vulkan_sdk_present=false
windows-vulkan-check.vulkan_sdk_path=
windows-vulkan-check.runner_vulkan_sdk_available=false
windows-vulkan-check.result=SKIPPED
```
Summary file generated and validated.

## 3. Conclusion
- Workflow now includes optional Windows Vulkan SDK provisioning/probe stage and env export contract.
- Windows Vulkan gate summary now includes SDK observability context.
- Current host still lacks Vulkan SDK/runtime; runtime PASS path remains dependent on Vulkan-ready runner.
