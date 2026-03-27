# Cross-platform Vulkan Windows gate and CI integration local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-016` Windows Vulkan gate command and CI integration wiring.

## 2. Commands and Results

### Static path scan
```text
rg -n "run_windows_vulkan_checks|DENABLE_VULKAN_RENDERER=ON|windows-vulkan-check" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
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

### Windows Vulkan gate command (optional mode)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200
```
Result: PASS (command exit code 0)
Key lines:
```text
windows-vulkan-check.supported_platform=true
windows-vulkan-check.compiled_in=false
windows-vulkan-check.runtime_available=false
windows-vulkan-check.skip_reason=vulkan-not-available
windows-vulkan-check.result=SKIPPED
```

### Windows Vulkan gate command (strict mode)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -RequireVulkanAvailable
```
Result: expected FAIL (command exit code 2)
Key lines:
```text
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```

## 3. Conclusion
- Windows Vulkan gate script and CI wiring are landed.
- Optional/strict mode behavior is deterministic and machine-readable.
- Current host cannot prove Vulkan PASS path because Vulkan package/runtime is unavailable.
