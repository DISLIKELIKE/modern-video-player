# Cross-platform Vulkan fallback chain and startup policy local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-009` startup-policy integration does not regress existing paths.
- Record Linux-first Vulkan runtime verification limitation on current host.

## 2. Commands and Results

### Policy/observability code-path scan
```text
rg -n "normalizeLinuxVulkanFallbackChain|linux-vulkan-fallback-chain|startup_renderer_plan_reason|startup_decoder_plan_reason" src/core/playback_strategy.cpp include/core/player_core.h src/core/player_core.cpp src/main.cpp
```
Key lines:
```text
src/core/playback_strategy.cpp:109:bool normalizeLinuxVulkanFallbackChain(...)
src/core/playback_strategy.cpp:186:appendPlanReasonTag(..., "linux-vulkan-fallback-chain")
src/main.cpp:6408:performance-log-check.startup_renderer_plan_reason
src/main.cpp:6411:performance-log-check.startup_decoder_plan_reason
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
performance-log-check.startup_renderer_plan_reason=platform-default-order
performance-log-check.startup_decoder_plan_reason=hardware-first
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
performance-log-check.startup_renderer_plan_reason=renderer-override-env
performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
performance-log-check.result=PASS
```
Result: PASS

## 3. Conclusion
- `VK-009` integration is complete for explicit startup-policy contract and machine-readable observability fields.
- Linux host/runner execution remains required to confirm real Linux Vulkan fallback chain runtime behavior.
