# Cross-platform Vulkan renderer skeleton and factory wiring local check

Date: 2026-03-26  
Host: Windows workstation

## 1. Scope
- Validate `VK-004` skeleton compile wiring and fallback observability.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline diagnostics
```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
```
Result: PASS

### Default performance baseline
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
performance-log-check.result=PASS
```
Result: PASS

### Vulkan override fallback observability
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
- Vulkan skeleton is fully wired into strategy/factory/capabilities.
- Fallback chain remains stable and machine-readable in current skeleton stage.
