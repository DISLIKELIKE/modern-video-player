# PlayerCore Day 39: Container attachment font pipeline

Date: 2026-03-25

## Summary
- Added container attachment font extraction and private registration for font attachment streams exposed by FFmpeg.
- Wired the attachment-font session into `PlayerCore::open()` / `close()` so media open registers fonts and session release unregisters them.
- Added `--attachment-font-check <media_file>` to validate extraction, registration, and cleanup in one machine-readable CLI pass.

## Why this mattered
- The previous subtitle font path only searched sidecar directories such as `fonts/` and `attachments/fonts/`.
- Mature players also consume container attachment fonts, which is critical for ASS/SSA portability because many releases ship fonts inside MKV attachments instead of next to the subtitle file.
- This is the highest-value remaining subtitle-font gap because missing attachment fonts immediately degrades subtitle fidelity even when the rest of the ASS renderer is present.

## Previous state
- `subtitle_font_registry` only handled file-system font discovery near the subtitle source path.
- No code iterated `AVMEDIA_TYPE_ATTACHMENT` streams or extracted attachment payloads.
- Opening a media file did not establish any subtitle-font session tied to the container itself.

## Implementation
### 1. Subtitle font registry
- Extended `SubtitleFontRegistrationSummary` with attachment-specific fields:
  - `discovered_attachment_stream_count`
  - `extracted_file_count`
  - `invalid_attachment_stream_count`
  - `extraction_cache_path`
  - `extracted_font_files`
  - `registered_font_files`
- Added:
  - `ensureMediaAttachmentFontsRegistered(media_source_path, format_ctx)`
  - `releaseMediaAttachmentFonts(media_source_path)`
- Attachment registration flow:
  - scan FFmpeg streams for `AVMEDIA_TYPE_ATTACHMENT`
  - identify font candidates by filename extension or attachment mimetype
  - extract font payloads from attachment `extradata`
  - write them to `%TEMP%\\modern-video-player\\subtitle-font-cache\\<media-hash>`
  - register them with private font loading on Windows

### 2. PlayerCore lifecycle
- `PlayerCore::open()` now registers container attachment fonts immediately after `demuxer_->open(...)`.
- `PlayerCore::applySessionReleaseSideEffects()` now releases the media attachment-font session before tearing down the rest of playback state.
- This means:
  - normal open/play/close works
  - failed opens also clean up correctly through the existing fail-open path
  - external ASS loaded after media open can use container-provided fonts

### 3. Diagnostics / CLI
- Added `--attachment-font-check <media_file>`.
- The command prints:
  - whether open succeeded
  - attachment stream count
  - extracted/registered font counts
  - invalid attachment count
  - cache path
  - extracted/registered font file list
  - final `PASS/FAIL`

## Validation
```powershell
ffmpeg -y `
  -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 `
  -c copy `
  -attach C:\Windows\Fonts\arial.ttf `
  -metadata:s:t mimetype=application/x-truetype-font `
  -metadata:s:t:0 filename=attachment-font-check.ttf `
  .\build\tmp\attachment-font-check.mkv

.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
```

Key result:
- `attachment-font-check.open_ok=true`
- `attachment-font-check.attachment_streams=1`
- `attachment-font-check.extracted_file_count=1`
- `attachment-font-check.registered_file_count=1`
- `attachment-font-check.invalid_attachment_stream_count=0`
- `attachment-font-check.result=PASS`

Cleanup result:
- extracted cache directory was removed after `releaseMediaAttachmentFonts(...)`

## Result
- The player now closes the main container-font gap relative to mature ASS-capable players.
- Container attachment fonts are available to the current subtitle stack as soon as media is opened.
- The new CLI gives a direct regression point for future subtitle/font work.

## Remaining gap vs mature players
- Embedded subtitle-track playback is still not implemented, so this is not full mpv/MPC-HC parity yet.
- The current implementation uses private Windows font registration, not a custom DirectWrite/libass font collection.
- Future work can still improve:
  - embedded subtitle-track consumption
  - custom font collection integration
  - stronger attachment-font diagnostics per subtitle track / per renderer

## Files
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY39_ATTACHMENT_FONT_PIPELINE.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
