# Cross-platform Vulkan sync and present pacing local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-008` synchronization/pacing hardening integration does not regress local baseline.
- Record Linux-first Vulkan runtime limitation on current host.

## 2. Commands and Results

### Sync/pacing code-path scan
```text
rg -n "MVP_VULKAN_PRESENT_MODE|present_mode_requested|present_mode_active|kFrameFenceWaitTimeoutNs|fence_wait_timeout|Vulkan pacing stats|choosePresentMode\(" src/render/vulkan_video_renderer.cpp
```
Key lines:
```text
src/render/vulkan_video_renderer.cpp:58:constexpr uint64_t kFrameFenceWaitTimeoutNs = 250000000ULL;
src/render/vulkan_video_renderer.cpp:395:VkPresentModeKHR choosePresentMode(...)
src/render/vulkan_video_renderer.cpp:1168:parseVulkanPresentModeRequest(readEnvVar("MVP_VULKAN_PRESENT_MODE"));
src/render/vulkan_video_renderer.cpp:1591:LOG_INFO("Vulkan pacing stats:" ...)
```
Result: PASS

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

### Vulkan override + present-mode override fallback observability
```text
$env:MVP_RENDERER_BACKEND='vulkan'
$env:MVP_VULKAN_PRESENT_MODE='immediate'
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
- `VK-008` code integration is complete for present-mode control and sync/pacing hardening path.
- Linux host/runner execution remains required to validate real Vulkan pacing behavior in runtime.
