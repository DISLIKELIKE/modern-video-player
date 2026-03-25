# PlayerCore Day 38: OpenGL bottom-bar player chrome

Date: 2026-03-25

## Summary
- Expanded the OpenGL path from a minimal OSD into a full bottom-bar control surface.
- Added a clickable play/pause button, segmented time text, and larger interaction zones for seek and volume.
- Added hover-aware keep-visible logic plus idle fade-out auto-hide.

## Why this mattered
- The OpenGL path already had playback capability, subtitles, and diagnostics, but the interaction surface still looked like a transient debug overlay.
- A mature player backend needs control chrome that is usable with the mouse, not only hotkeys.
- The missing pieces were layout, hit-testing, and lightweight text rendering rather than decode/render functionality.

## Implementation
### 1. Layout model
- Expanded `ControlLayout` to include:
  - `play_button`
  - `time_text`
  - `progress_hit_box`
  - `volume_hit_box`
- The progress rail now lives on the top row of the bottom bar, while the play button, time text, and volume rail occupy the lower row.

### 2. Drawing helpers
- Added OpenGL immediate-mode helpers for:
  - filled triangle
  - filled circle
  - segmented time text glyph rendering
- Time text is rendered without a new text library dependency.

### 3. Interaction model
- Added hover state tracking for the panel, play button, progress rail, and volume rail.
- Play/pause button clicks now issue the same toggle request path used by hotkeys.
- Seek and volume interactions now use larger hit boxes than the visible rails.

### 4. Auto-hide behavior
- The bar remains visible while:
  - paused
  - hovered
  - dragging seek
  - dragging volume
- During idle playback it now fades out instead of disappearing abruptly.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## Result
- OpenGL now has a more player-like UI surface instead of only a utility OSD.
- Mouse-first playback control is substantially improved.
- The underlying OpenGL playback/subtitle/diagnostics gate still passes after the UI upgrade.

## Residual risk
- The control chrome is covered by build and gate regression only indirectly.
- Pointer hover timing, click feel, and visual spacing still need a real GUI smoke pass on the packaged executable.

## Files
- `src/render/opengl_video_renderer.cpp`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
