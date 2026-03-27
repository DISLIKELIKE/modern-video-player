# Cross-platform Vulkan Windows CMake SDK fallback and prefix-path closure local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-020` Windows dependency-resolution closure:
  - CMake `VULKAN_SDK` fallback contract
  - workflow `CMAKE_PREFIX_PATH` export wiring
  - local build and diagnostics stability

## 2. Commands and Results

### Static path scan
```text
rg -n "VULKAN_SDK fallback|CMAKE_PREFIX_PATH|VULKAN_SDK_ROOT|MVP_WINDOWS_VULKAN_SDK_AVAILABLE|find_package\(Vulkan\) failed and VULKAN_SDK fallback is incomplete" CMakeLists.txt .github/workflows/cross-platform-gate.yml
```
Result: PASS

### Configure (Vulkan switch ON)
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
vulkan-diagnostics.result=FAIL
```

### Windows Vulkan gate check
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
```
Result: PASS
Key lines:
```text
windows-vulkan-check.strict_mode_policy=off
windows-vulkan-check.compiled_in=false
windows-vulkan-check.runtime_available=false
windows-vulkan-check.skip_reason=vulkan-not-available
windows-vulkan-check.result=SKIPPED
```
Summary file generated and validated.

## 3. Conclusion
- Windows CMake Vulkan dependency resolution now has explicit SDK fallback and keeps safe downgrade behavior.
- Workflow now injects SDK root into `CMAKE_PREFIX_PATH`, improving package discovery on CI runners.
- Current host remains non-Vulkan-ready; strict runtime PASS proof still depends on a Vulkan-ready Windows runner.