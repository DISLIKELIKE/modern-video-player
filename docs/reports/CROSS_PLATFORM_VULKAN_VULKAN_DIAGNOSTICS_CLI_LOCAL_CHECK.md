# Cross-platform Vulkan diagnostics CLI local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-010` command integration:
  - `--vulkan-diagnostics` usage + dispatch
  - machine-readable output contract
  - no regression to existing diagnostics commands

## 2. Commands and Results

### Code-path scan
```text
rg -n "runVulkanDiagnostics|--vulkan-diagnostics|vulkan-diagnostics\\.|rendererCandidateChainToString" src/main.cpp
```
Result: PASS

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### D3D11 diagnostics regression
```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Key lines:
```text
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.result=PASS
```
Result: PASS

### OpenGL diagnostics regression
```text
.\build\Release\modern-video-player.exe --opengl-diagnostics
```
Key lines:
```text
opengl-diagnostics.probe_succeeded=true
opengl-diagnostics.result=PASS
```
Result: PASS

### Vulkan diagnostics (default env)
```text
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Key lines:
```text
vulkan-diagnostics.platform=Windows
vulkan-diagnostics.supported_platform=false
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
vulkan-diagnostics.startup_renderer_plan_reason=platform-default-order
vulkan-diagnostics.selected_renderer=D3D11
vulkan-diagnostics.fallback_target=none
vulkan-diagnostics.result=FAIL
```
Result: Expected FAIL on non-Linux host

### Performance baseline regression
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

### Vulkan override observability
```text
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Key lines:
```text
vulkan-diagnostics.requested_renderer_override=vulkan
vulkan-diagnostics.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
vulkan-diagnostics.startup_renderer_plan_reason=renderer-override-env
vulkan-diagnostics.selected_renderer=D3D11
vulkan-diagnostics.fallback_target=D3D11
vulkan-diagnostics.result=FAIL
```
Result: Expected FAIL on non-Linux host, with fallback observability PASS

## 3. Conclusion
- `VK-010` is complete for CLI contract and machine-readable output integration.
- Existing diagnostics/performance commands remain PASS on this host.
- Linux host/runner execution is still required to observe `vulkan-diagnostics.result=PASS`.
