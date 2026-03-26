# PLAYERCORE DAY48: CP-201 ~ CP-205 Renderer/Display/Input Cleanup
Date: 2026-03-26
Status: Done

## 1. Problem Statement
- `IVideoRenderer` still mixed rendering responsibilities with playback input/event and overlay update semantics.
- Event pumping behavior was not explicitly guarded to main-thread-only call paths.
- OpenGL interaction-freeze regression lacked a dedicated machine-readable stress check command.
- Idle window input handling still coupled to renderer concrete API assumptions.

## 2. Root Cause
- Renderer abstraction grew beyond rendering scope and became a broad control surface.
- Input and overlay responsibilities were implemented in renderers, but no explicit role interfaces existed.
- Thread-affinity constraints were implicit and could be violated silently by caller paths.
- Gate scripts had no dedicated interaction stress signal to block regressions.

## 3. Implementation Planner Used
1. Split renderer responsibilities into rendering/input/overlay role interfaces.
2. Refactor `PlayerCore` to consume role interfaces instead of renderer control APIs.
3. Move idle window command handling to input role abstraction.
4. Add main-thread event pump guards in `PlayerCore`/renderers/display.
5. Add machine-readable interaction-freeze stress command and include it in OpenGL gate.
6. Run local build and regression checks, then sync records/docs/plans.

## 4. Landed Changes
- Added role interfaces:
  - `include/input/playback_input_source.h`
  - `include/render/render_overlay_sink.h`
- Shrunk renderer contract:
  - `include/render/video_renderer.h`
- Updated renderer implementations to expose role interfaces:
  - `include/render/sdl_video_renderer.h`
  - `include/render/d3d11_video_renderer.h`
  - `include/render/opengl_video_renderer.h`
- Refactored `PlayerCore` role consumption and event-pump path:
  - `include/core/player_core.h`
  - `src/core/player_core.cpp`
- Refactored idle window path and added `--interaction-freeze-check`:
  - `src/main.cpp`
- Added event-thread guards:
  - `include/display.h`
  - `src/display.cpp`
  - `src/render/d3d11_video_renderer.cpp`
  - `src/render/opengl_video_renderer.cpp`
- Added gate coverage:
  - `tools/run_opengl_checks.ps1`

## 5. Result Against CP IDs
- `CP-201`: done (`IVideoRenderer` reduced to rendering-only surface).
- `CP-202`: done (input command flow consumed via `IPlaybackInputSource` in core + idle path).
- `CP-203`: done (main-thread guard enforcement for SDL event pumping).
- `CP-204`: done (overlay state push separated through `IRenderOverlaySink`).
- `CP-205`: done (`--interaction-freeze-check` added and wired into OpenGL gate).

## 6. Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player`
- Runtime checks:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4`
  - `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800`
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Key outcomes:
  - Build succeeded.
  - `performance-log-check.result=PASS`.
  - `renderer-fallback-check.result=PASS`.
  - `interaction-freeze-check.result=PASS`.
  - OpenGL gate summary: `OpenGL gate result: PASS`.

## 7. Remaining Risks
- Linux build/dependency closure and feature switch enforcement are still pending in `CP-301~CP-305`.
- Runtime behavior currently validated on Windows local machine; Linux runtime parity still needs dedicated gate execution.
- Audio device absence in current local environment causes expected `video-only fallback` warnings during checks.
