# Cross-platform Vulkan instance/surface/device/swapchain init local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-005` changes do not regress existing runtime paths on local host.
- Record Linux-first Vulkan validation gap on non-Linux host.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### D3D11 diagnostics
```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Key lines:
```text
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.adapter_name=NVIDIA GeForce GTX 1080
d3d11-diagnostics.result=PASS
```
Result: PASS

### OpenGL diagnostics
```text
.\build\Release\modern-video-player.exe --opengl-diagnostics
```
Key lines:
```text
opengl-diagnostics.probe_succeeded=true
opengl-diagnostics.gl_renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2
opengl-diagnostics.result=PASS
```
Result: PASS

### Performance baseline
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
performance-log-check.startup_selected_renderer=D3D11
performance-log-check.startup_renderer_fallback_reason=none
performance-log-check.result=PASS
```
Result: PASS

### Vulkan override fallback observability on Windows
```text
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
performance-log-check.startup_selected_renderer=D3D11
performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
performance-log-check.result=PASS
```
Result: PASS

## 3. Conclusion
- `VK-005` code integration does not regress current Windows build and diagnostics paths.
- Real Vulkan init/runtime evidence is still pending on Linux host/runner because Vulkan renderer is disabled by build switch policy on Windows.
