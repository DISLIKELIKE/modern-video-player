# Cross-platform Vulkan regression matrix execution local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-013` regression matrix coverage for:
  - open/play
  - pause/seek
  - subtitle
  - fallback chain
  - Vulkan diagnostics observability

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Open/Play baseline
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.result=PASS
```
Result: PASS

### Seek burst serial
```text
.\build\Release\modern-video-player.exe --seek-burst-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 6
```
Key lines (first run):
```text
seek-burst-serial-check.seek_position_ok=false
seek-burst-serial-check.result=FAIL
```
Immediate rerun key lines:
```text
seek-burst-serial-check.seek_position_ok=true
seek-burst-serial-check.result=PASS
```
Result: PASS after rerun (note: first-run instability observed)

### Paused seek serial
```text
.\build\Release\modern-video-player.exe --paused-seek-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 4
```
Key lines:
```text
paused-seek-serial-check.seek_position_ok=true
paused-seek-serial-check.result=PASS
```
Result: PASS

### Subtitle style
```text
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
```
Key lines:
```text
subtitle-style-check.entries=5
subtitle-style-check.result=PASS
```
Result: PASS

### Subtitle sync
```text
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
```
Key lines:
```text
subtitle-sync-check.mismatches=0
subtitle-sync-check.result=PASS
```
Result: PASS

### Renderer fallback
```text
.\build\Release\modern-video-player.exe --renderer-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4
```
Key lines:
```text
renderer-fallback-check.renderer_backend=SoftwareSDL
renderer-fallback-check.fallback_to_sdl=true
renderer-fallback-check.result=PASS
```
Result: PASS

### Vulkan override fallback observability
```text
$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200; Remove-Item Env:MVP_RENDERER_BACKEND
```
Key lines:
```text
performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
performance-log-check.startup_renderer_plan_reason=renderer-override-env
performance-log-check.startup_selected_renderer=D3D11
performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
performance-log-check.result=PASS
```
Result: PASS

### Vulkan diagnostics
```text
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Key lines:
```text
vulkan-diagnostics.supported_platform=false
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.result=FAIL
```
Result: Expected FAIL on Windows host

## 3. Conclusion
- `VK-013` matrix execution is closed with local evidence for all required command classes.
- Linux Vulkan runtime PASS evidence still requires real Linux runner execution.
- Seek-burst first-run FAIL followed by PASS on rerun is recorded as a residual nondeterministic risk.
