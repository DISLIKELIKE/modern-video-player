# Cross-platform Vulkan Windows link/runtime probe local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate Windows Vulkan closure updates:
  - Windows link stage consumes Vulkan platform libraries.
  - Vulkan runtime availability publication is probe-based.

## 2. Commands and Results

### Static path scan
```text
rg -n "PLATFORM_EXTRA_LIBRARIES|probeVulkanRuntimeAvailability|vkEnumeratePhysicalDevices" CMakeLists.txt src/platform/platform_capabilities.cpp
```
Result: PASS

### Configure with Vulkan ON
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
Result: Expected FAIL on this host (dependency missing)
Key lines:
```text
vulkan-diagnostics.platform=Windows
vulkan-diagnostics.supported_platform=true
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.result=FAIL
```

### Fallback observability with Vulkan override
```text
$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200; Remove-Item Env:MVP_RENDERER_BACKEND
```
Result: PASS
Key lines:
```text
performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
performance-log-check.startup_renderer_plan_reason=renderer-override-env
performance-log-check.startup_selected_renderer=D3D11
performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
performance-log-check.result=PASS
```

### Windows baseline regression safety
```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Result: PASS

## 3. Conclusion
- Windows Vulkan closure for link/runtime publication is landed.
- Current host still lacks Vulkan package, so compiled/runtime Vulkan remains unavailable by design and fallback remains deterministic.
- `compiled_in=true` runtime validation still requires a Windows host with Vulkan SDK/runtime installed.
