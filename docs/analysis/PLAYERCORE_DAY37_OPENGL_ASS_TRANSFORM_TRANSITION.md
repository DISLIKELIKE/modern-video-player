# PlayerCore Day 37: ASS transform transition support for OpenGL and D3D11

Date: 2026-03-25

## Summary
- Added subtitle parser/model support for ASS `\t(...)` transitions.
- Added machine-readable transition diagnostics to `--subtitle-style-check`.
- Added runtime style interpolation for transition-driven subtitle fields in both OpenGL and D3D11 renderers.
- Added a dedicated transition validation sample and folded it into the OpenGL gate.

## Why this mattered
- The OpenGL Top 10 plan was complete, but a visible subtitle parity gap still remained against mature players: transform transitions were still static.
- Without parser support, the renderer could not distinguish timed style change from ordinary override tags.
- Without machine-readable diagnostics, regressions in `\t(...)` behavior would be difficult to isolate.

## 1. Parser and model changes
### Added structures
- Added transition property masks under the subtitle parser model.
- Added `SubtitleStyleTransition`.
- Added `style_transitions` to `SubtitleStyleAnimation`.

### Parser changes
- Added `\t` to the supported ASS tag set.
- Added nested-parenthesis matching for override tag value extraction.
- Added top-level comma splitting for transform argument parsing.
- Added dedicated transform parsing so these forms are accepted:
  - `\t(<tags>)`
  - `\t(<accel>,<tags>)`
  - `\t(<t1>,<t2>,<tags>)`
  - `\t(<t1>,<t2>,<accel>,<tags>)`

### Practical outcome
- Transition bodies can now contain nested calls such as `\clip(...)` without being truncated.
- Parsed output now preserves timing, accel, affected property mask, and target style values.

## 2. Runtime subtitle evaluation
### Added runtime behavior
- Added transition progress evaluation with timing-window and accel handling.
- Added style interpolation helpers for numeric and color fields.
- Added transition-aware subtitle style resolution for both item-level and run-level styles.

### Fields covered in this batch
- `font_size`
- `primary/secondary/outline/background` color
- `outline_x/y`
- `shadow_x/y`
- `spacing`
- `scale_x/y`
- `rotation_z/x/y`
- `shear_x/y`
- `rotation_origin`

### Important scope note
- This is not full libass parity.
- The goal of this batch was to close a concrete missing semantic slice and make it testable across both GPU subtitle paths.

## 3. Diagnostics and regression assets
### `--subtitle-style-check`
- Added:
  - `transition_count`
  - per-transition `has_timing`
  - per-transition timing and accel
  - `property_mask`
  - `property_names`
  - transition target style dump

### Validation sample
- Added `samples/subtitles/opengl_ass_transform_transition_validation.ass`.
- The sample covers:
  - timed color/scale/rotation transition
  - accel-only spacing/shear transition
  - nested `\clip(...)` inside `\t(...)`

## 4. Validation
```powershell
cmake --build build --config Release --target modern-video-player

.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass

$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## Key results
- `subtitle-style-check.result=PASS`
- OpenGL `delay-adjust-check.result=PASS`
- D3D11 `delay-adjust-check.result=PASS`
- `OpenGL gate result: PASS`

## Remaining gap after this batch
### Improved
- ASS subtitle semantics are now closer to mature players for transform-driven style change.
- Parser diagnostics can now explain transition timing and target state.
- The OpenGL gate now catches this regression category automatically.

### Still missing vs mature players
- fuller libass shaping/layout behavior
- attachment extraction and broader font fallback parity
- additional ASS semantics outside the current transition field set
- display-level HDR output and output color management remain separate larger gaps

## Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_transform_transition_validation.ass`
- `tools/run_opengl_checks.ps1`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
