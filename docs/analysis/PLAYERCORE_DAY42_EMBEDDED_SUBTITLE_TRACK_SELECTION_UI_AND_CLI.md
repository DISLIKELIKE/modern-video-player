# PLAYERCORE Day42: Embedded Subtitle Track Selection UI + CLI
Date: 2026-03-25

## Background
- Day40 closed embedded text subtitle auto-load, but still treated subtitle selection as a single-track path.
- OpenGL control bar had no subtitle-track switching interaction.
- CLI had no direct way to list/select embedded subtitle streams for machine-readable regression.

## Problem
- Multi-track embedded subtitle containers could not be switched from UI.
- There was no playback-mode argument to prefer a specific embedded subtitle stream index.
- Existing embedded subtitle diagnostics only validated "best stream auto-load", not explicit stream selection.

## Root Cause
1. `PlayerCore` already had embedded subtitle load support, but no full public selection surface exposed to renderer/UI + playback CLI together.
2. OpenGL control bar refactor was mid-state: layout placeholders existed, while draw/hit-test/request-consume hooks were incomplete.
3. CLI path lacked `list/select` command coverage for embedded subtitle streams.

## Implementation
### 1) Embedded subtitle track policy and core state
- Kept embedded track catalog and selected stream index in `PlayerCore`.
- Selection scope now follows supported embedded subtitle codecs (`supported_codec`), including text codecs and bitmap codecs (`hdmv_pgs_subtitle` / `dvd_subtitle`).
- `setSubtitleTrackState(current_ordinal, track_count)` now reflects supported selectable tracks, not raw subtitle stream count.

### 2) OpenGL subtitle-track UI closure
- Completed OpenGL bottom-bar controls for:
  - previous item / next item buttons
  - subtitle previous / next track buttons
  - mute button
  - fullscreen button
  - subtitle track state text (`current / total`)
- Completed hit-test and click request plumbing in `handleMouseButtonDown/handleMouseMotion`.
- Added renderer request consumers:
  - `consumePreviousSubtitleTrackRequest()`
  - `consumeNextSubtitleTrackRequest()`
- Added renderer state sync:
  - `setSubtitleTrackState(int current_ordinal, int track_count)`

### 3) CLI closure for multi-track control
- Playback CLI:
  - Added `--subtitle-track <stream_index>` to set preferred embedded subtitle stream index before media open.
- Diagnostics CLI:
  - Added `--embedded-subtitle-list <media_file>`
  - Added `--embedded-subtitle-select-check <media_file> <stream_index>`
- Existing `--embedded-subtitle-check` remains as best-stream auto-selection regression.

## Validation
### Build
```powershell
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && msbuild build\modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64"
```
- Result: PASS (warnings from existing non-UTF8 source are unchanged and pre-existing)

### Embedded subtitle CLI regression
```powershell
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-text-validation.mp4
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-text-validation.mp4 2
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-text-validation.mp4
```
- Results: all PASS

### OpenGL gate regression
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```
- Result: `OpenGL gate result: PASS` (16/16 PASS)

## Files
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
