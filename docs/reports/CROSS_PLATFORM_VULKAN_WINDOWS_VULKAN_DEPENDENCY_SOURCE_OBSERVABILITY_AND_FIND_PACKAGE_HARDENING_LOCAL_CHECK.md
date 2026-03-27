# Cross-platform Vulkan Windows dependency-source observability and find_package hardening local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-021` closure:
  - Windows Vulkan dependency completeness contract in CMake
  - Vulkan dependency source observability in diagnostics and gate summary

## 2. Commands and Results

### Static path scan
```text
rg -n "vulkan-diagnostics\.dependency_source|diag_dependency_source|MVP_VULKAN_DEPENDENCY_SOURCE|find_package\(Vulkan\) returned incomplete package info" CMakeLists.txt src/main.cpp tools/run_windows_vulkan_checks.ps1
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

### Vulkan diagnostics
```text
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Result: expected FAIL on current host  
Key lines:
```text
vulkan-diagnostics.platform=Windows
vulkan-diagnostics.supported_platform=true
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.dependency_source=disabled
vulkan-diagnostics.result=FAIL
```

### Windows Vulkan gate summary
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk021.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.diag_dependency_source=disabled
windows-vulkan-check.skip_reason=vulkan-not-available
windows-vulkan-check.result=SKIPPED
```

## 3. Conclusion
- Windows Vulkan dependency-resolution decision path now has explicit completeness guard.
- Diagnostics and gate summary now expose dependency source for CI diagnosis.
- Current host remains non-Vulkan-ready; strict PASS still requires Vulkan-ready runner.
