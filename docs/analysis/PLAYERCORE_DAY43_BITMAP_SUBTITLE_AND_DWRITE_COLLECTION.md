# PLAYERCORE Day43: Embedded bitmap subtitle path and DirectWrite custom font collection
Date: 2026-03-25

## Background
- Day40/Day42 closed embedded text subtitle auto-load and multi-track selection.
- Subtitle attachment fonts were already extracted/registered, but text rendering still relied on system collection fallback after `AddFontResourceExW`.
- PGS/DVD subtitle streams were still outside the embedded subtitle playback path.

## Implementation Planner
1. Extend embedded subtitle codec policy from text-only to supported-codec so track listing/selection can include bitmap subtitle streams.
2. Add bitmap subtitle decode path (`hdmv_pgs_subtitle` / `dvd_subtitle`) into the embedded loader and map decoded rectangles into `SubtitleItem`.
3. Add bitmap subtitle rendering branches in OpenGL and D3D11 subtitle renderers using D2D bitmap upload/composition.
4. Add DirectWrite custom font collection construction from registered private subtitle fonts and apply it before system fallback.
5. Extend CLI checks to expose bitmap-track fields and add a dedicated DirectWrite collection check command.
6. Run Release build + CLI + OpenGL gate and update records.

## Problem
- Embedded subtitle selection could switch streams, but policy and diagnostics still centered on text-only semantics.
- Attachment and sidecar private fonts were registered, but not explicitly bound as a custom DirectWrite font collection for subtitle layout.
- Embedded bitmap subtitle codecs were discoverable in containers but not consumable end-to-end by the subtitle render path.

## Root Cause
1. `EmbeddedSubtitleTrackInfo` and related overlay/selection logic had partial text-focused assumptions in docs and surrounding policy wording.
2. Loader path had no `AVSubtitleRect` bitmap decode branch that converts FFmpeg subtitle rectangles into renderer-consumable model data.
3. Renderer subtitle loops only consumed text styles/runs and had no bitmap subtitle branch.
4. Font registration and text rendering were decoupled: registration existed, but custom collection construction/usage was missing.

## Implementation
### 1) Embedded subtitle policy and loader extension
- Added/used `supported_codec` consistently for selectable embedded subtitle streams.
- Added bitmap codec support in loader:
  - `AV_CODEC_ID_HDMV_PGS_SUBTITLE`
  - `AV_CODEC_ID_DVD_SUBTITLE`
- Added `SubtitleBitmap` model and bitmap fields in `SubtitleItem`.
- Added bitmap decode conversion from indexed + palette rect data into RGBA payload and timing metadata.

### 2) Renderer consumption for bitmap subtitles
- Added bitmap subtitle drawing branches in:
  - OpenGL subtitle renderer (D2D offscreen path)
  - D3D11 subtitle renderer
- Bitmap subtitle item render flow:
  - map subtitle rect to video rect using play resolution
  - convert RGBA to premultiplied BGRA
  - create D2D bitmap and draw into subtitle target

### 3) DirectWrite custom font collection
- Added `buildDirectWriteSubtitleFontCollection(...)` in subtitle font registry.
- Collection build uses:
  - `IDWriteFactory3`
  - `IDWriteFontSetBuilder`
  - `CreateFontFaceReference`
  - `AddFontFaceReference`
  - `CreateFontCollectionFromFontSet`
- OpenGL and D3D11 subtitle text rendering now:
  - try custom subtitle font collection first
  - fallback to system collection when unavailable

### 4) CLI diagnostics closure
- Extended embedded subtitle checks with bitmap fields:
  - `bitmap_codec`
  - `bitmap_item_count`
- Extended list check with:
  - `supported_track_count`
  - `supported_text_track_count`
  - `supported_bitmap_track_count`
- Added:
  - `--directwrite-font-collection-check <media_file>`

## Validation
### Build
```powershell
MSBuild.exe build/modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64
```
- Result: PASS

### CLI
```powershell
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-text-validation.mp4
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\embedded-ass-validation.mkv
```
- Result: PASS

### Gate
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```
- Result: `OpenGL gate result: PASS` (`16/16 PASS`)

## Notes / Known Gaps
- Bitmap subtitle model/renderer path is now available, but this round did not finish a dedicated real-media PGS/DVD sample corpus for broader runtime coverage.
- Display-level HDR present bridge and ICC/3D LUT output management remain separate backlog workstreams.

## Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY43_BITMAP_SUBTITLE_AND_DWRITE_COLLECTION.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
