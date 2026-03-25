# PLAYERCORE DAY32 - Idle Window and Drag-Drop Playback

Date: 2026-03-25

## Summary
- Added an idle-window startup mode when `modern-video-player.exe` is launched without media arguments.
- Added drag-and-drop media opening across the SDL, D3D11, and OpenGL renderer event paths.
- Added runtime media replacement so dropping a file onto an active playback window switches to the dropped media after validation.
- Kept invalid drop handling non-destructive: invalid paths are logged and ignored instead of stopping playback.

## implementation planner
1. Audit current startup path and renderer event ownership to find where an open-file request can be surfaced safely.
2. Add `consumeOpenFileRequest()` to the renderer interface and implement it in SDL, D3D11, and OpenGL backends.
3. Buffer renderer drop requests inside `PlayerCore` so the app layer can validate paths before switching playback.
4. Rework `main.cpp` to support idle startup, idle drag-drop selection, playback-time replacement, and return-to-idle behavior for empty-start sessions.
5. Build and record workflow-required docs.

## What Changed
### Startup behavior
- Launching the exe without media arguments now creates an idle player window instead of printing usage and exiting.
- The idle window uses the same renderer selection policy as normal playback, with fallback to `SoftwareSDL` if the preferred backend cannot initialize.

### Drag and drop plumbing
- Extended `IVideoRenderer` with `consumeOpenFileRequest(std::string& path)`.
- Implemented file-drop event capture in:
  - `Display` / `SdlVideoRenderer`
  - `D3D11VideoRenderer`
  - `OpenGLVideoRenderer`
- `PlayerCore` now caches pending open-file requests from the renderer without auto-stopping playback.
- `VideoPlayer` exposes the cached request to the app layer.

### Main loop behavior
- Idle-start sessions now wait for a dropped media file.
- Dropped files are validated before building a playlist:
  - empty path is rejected
  - local non-existent path is rejected
  - non-file local path is rejected
- During active playback, a valid dropped file replaces the current playlist.
- Invalid drops during playback are ignored and playback continues.
- Sessions that started without CLI media return to the idle window after playback ends.

## Validation
- `cmake --build build --config Release --target modern-video-player`: PASS
- Automated verification in this turn is build-only.
- Manual GUI smoke test is still recommended:
  - launch `build\\Release\\modern-video-player.exe`
  - confirm an idle window appears with no media args
  - drag a local video file onto the window
  - confirm playback starts
  - drag a second file during playback and confirm it replaces the current media

## Files Changed
- `src/main.cpp`
- `src/display.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- `include/render/video_renderer.h`
- `include/display.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/player_core.h`
- `include/video_player.h`
