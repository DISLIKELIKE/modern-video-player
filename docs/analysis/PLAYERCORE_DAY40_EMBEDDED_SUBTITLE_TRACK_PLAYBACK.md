# PlayerCore Day 40: Embedded subtitle-track playback

Date: 2026-03-25

## Summary
- Added automatic loading for supported embedded text subtitle tracks during media open.
- Reused the existing ASS/SRT subtitle pipeline through new in-memory parser entry points instead of introducing a second subtitle model.
- Added `--embedded-subtitle-check <media_file>` and folded embedded subtitle validation into the OpenGL gate.

## Why this mattered
- After attachment fonts landed, a major remaining subtitle gap versus mature players was that muxed subtitle streams inside MKV/MP4 still did not play automatically.
- That meant files with built-in ASS/SSA or text subtitles opened without subtitle output unless the user manually loaded a sidecar file.
- Mature players treat embedded text subtitle tracks as first-class inputs, so this was one of the highest-value real-world playback gaps still left in the current stack.

## Previous state
- `PlayerCore` only consumed subtitle timelines loaded from external subtitle files.
- Subtitle ownership did not distinguish between external and embedded sources, so fallback behavior was not explicit.
- `AssParser` and `SrtParser` only exposed file/stream-oriented entry points, which made container-track reuse awkward.
- There was no dedicated regression command for embedded subtitle discovery and loading.

## Implementation
### 1. Parser reuse through in-memory text entry points
- Added `AssParser::parseText(...)` and `SrtParser::parseText(...)`.
- Refactored both parsers to share stream-based parsing internals.
- This lets the loader reconstruct ASS/SRT text in memory and feed it into the existing subtitle model without temp files.

### 2. Embedded subtitle loader
- Added `subtitle::loadBestEmbeddedSubtitleTrack(const std::string& media_source_path)`.
- Loader policy:
  - scan subtitle streams from FFmpeg
  - keep supported text subtitle codecs only
  - prefer `default` disposition and ASS/SSA when multiple supported tracks exist
- ASS/SSA path:
  - rebuild subtitle text from `subtitle_header` / `extradata`
  - reconstruct `Dialogue:` entries from packet timing
  - parse through `AssParser::parseText(...)`
- Plain-text path:
  - decode subtitle packets with FFmpeg subtitle decode
  - rebuild a synthetic SRT document
  - parse through `SrtParser::parseText(...)`
- Current targeted codecs:
  - `ass`
  - `ssa`
  - `subrip`
  - `text`
  - `mov_text`
  - `webvtt`

### 3. PlayerCore subtitle ownership
- Split subtitle storage into:
  - `external_subtitle_items_`
  - `embedded_subtitle_items_`
  - `subtitle_items_` active
- Active selection rule is now `external > embedded`.
- Clearing external subtitles falls back to embedded subtitles instead of leaving the player with no active subtitle timeline.
- Session release now clears embedded subtitles alongside the existing external-subtitle and attachment-font cleanup.

### 4. Diagnostics and OpenGL regression coverage
- Added `--embedded-subtitle-check <media_file>` with machine-readable output:
  - stream discovery/support flags
  - selected stream index / codec / language / title
  - source label
  - item count / first text
  - final `PASS/FAIL`
- Extended `tools/run_opengl_checks.ps1` to auto-generate and validate:
  - an embedded ASS sample
  - an embedded text subtitle sample using `mov_text`
- The OpenGL gate now checks both loader CLI output and actual OpenGL playback on embedded subtitle media.

## Validation
```powershell
ffmpeg -y `
  -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 `
  -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass `
  -map 0:v -map 0:a? -map 1:0 `
  -c:v copy -c:a copy -c:s ass `
  .\build\tmp\embedded-ass-validation.mkv

.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 `
  -ExecutablePath "build/Release/modern-video-player.exe" `
  -ProbeFile "juren-30s.mp4"
```

Key results:
- `embedded-subtitle-check.loaded=true`
- `embedded-subtitle-check.codec_name=ass`
- `embedded-subtitle-check.item_count=3`
- `embedded-subtitle-check.result=PASS`
- Gate result: `OpenGL gate result: PASS`

Text-path gate result:
- `embedded-subtitle-check.codec_name=mov_text`
- `embedded-subtitle-check.result=PASS`

## Result
- The player now auto-loads supported embedded text subtitle tracks during normal media open.
- OpenGL and D3D11 subtitle rendering consume those tracks through the same `SubtitleItem` timeline already used for external subtitles.
- Subtitle fallback is now stable: an external sidecar subtitle overrides the embedded track, and clearing the sidecar restores the embedded track.

## Remaining gap vs mature players
- There is still no user-facing subtitle track selection UI; the current policy auto-selects the best supported track.
- Bitmap subtitle formats such as PGS/DVD subtitle are still outside this path.
- Full libass shaping/layout parity, display-level HDR output, and ICC / 3D LUT output management remain separate backlog items.

## Files
- `include/subtitle/ass_parser.h`
- `src/subtitle/ass_parser.cpp`
- `include/subtitle/srt_parser.h`
- `src/subtitle/srt_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY40_EMBEDDED_SUBTITLE_TRACK_PLAYBACK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
