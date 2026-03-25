# PLAYERCORE DAY31 - ASS Transform, Vector Drawing/Clip, and Font Fallback Groundwork

Date: 2026-03-25

## Summary
- Completed the requested three-item batch across the current subtitle pipeline: `\org / \fax / \fay / \frx / \fry`, ASS vector drawing / vector clip, and font attachment / fallback groundwork.
- Kept the implementation inside the existing player architecture by extending the subtitle model/parser first and then wiring both D3D11 and OpenGL GPU subtitle renderers.
- Closed the OpenGL runtime regression discovered during validation. The failure was not a hardware capability issue; it was an internal Direct2D push/pop balance bug introduced by the new vector clip path.

## implementation planner
1. Extend subtitle model data so parser output can represent transform origin, shear, 3D-style projected rotation, pure drawing items, vector clip commands, and subtitle source path.
2. Add ASS parser coverage for `\org`, `\fax`, `\fay`, `\frx`, `\fry`, `\p`, and vector `\clip/\iclip` while keeping item-level and run-level semantics explicit.
3. Wire the new fields into both D3D11 and OpenGL subtitle rendering, including affine transform, vector drawing geometry, and vector clip masking.
4. Add subtitle-sidecar font registration and fallback family chain groundwork so missing ASS font names degrade more gracefully.
5. Re-run parser/runtime validation, diagnose any renderer regression, and sync workflow documentation.

## Implemented
### Subtitle model / parser
- Added transform fields on `SubtitleStyle`:
  - `rotation_x_degrees`
  - `rotation_y_degrees`
  - `shear_x`
  - `shear_y`
  - `has_rotation_origin`
  - `rotation_origin_x`
  - `rotation_origin_y`
  - `has_vector_clip`
  - `vector_clip_scale`
  - `vector_clip_commands`
- Added item fields on `SubtitleItem`:
  - `source_path`
  - `is_vector_drawing`
  - `drawing_scale`
  - `drawing_commands`
- Added ASS parser coverage for:
  - `\org`
  - `\fax`
  - `\fay`
  - `\frx`
  - `\fry`
  - `\p`
  - vector `\clip(...)`
  - vector `\iclip(...)`
- Preserved pure drawing events even when visible text is empty so ASS drawing-only cues are not dropped.
- Extended subtitle diagnostics so `--subtitle-style-check` now exposes source path, drawing fields, transform fields, and vector clip fields in machine-readable output.

### D3D11 / OpenGL subtitle rendering
- Added shared affine-transform logic for projected `frx/fry`, `fax/fay`, and `org`.
- Added ASS vector drawing geometry construction and rendering before the text path.
- Added vector clip geometry support for both text and drawing paths.
- Kept D3D11 and OpenGL implementations aligned so both GPU subtitle paths consume the same extended subtitle model.

### Font attachment / fallback groundwork
- Added `subtitle_font_registry`.
- Added local/private font registration from subtitle-adjacent directories such as:
  - subtitle directory
  - `fonts`
  - `attachments/fonts`
- Added fallback family-chain construction so missing ASS font families can fall back through registered/private and platform fonts.

### Sample / diagnostics
- Added `samples/subtitles/opengl_ass_transform_vector_font_validation.ass`.
- Extended the CLI diagnostics path to expose the new subtitle fields.

## Runtime Regression Found During Validation
### Symptom
- OpenGL runtime validation initially printed:
  - `OpenGL subtitle D2D draw failed: hr=-2003238890`

### Diagnosis
- `-2003238890` maps to `0x88990016`, which is `D2DERR_PUSH_POP_UNBALANCED`.
- This proved the failure was not caused by missing GPU support for NV12, SRV creation, or D3D11 capability.
- The regression came from the new subtitle clip-layer path:
  - `draw_text_pass()` performed `PopLayer()` before any matching `PushLayer()`.
  - `draw_box_region()` performed `PushLayer()` without a matching `PopLayer()`.
- The bug existed in the OpenGL subtitle path and had been mirrored into the D3D11 subtitle path.

### Fix
- Removed the incorrect pre-emptive `PopLayer()` call from the subtitle text clip path.
- Added the missing `PopLayer()` in the subtitle box path.
- Applied the same balance fix to both `src/render/opengl_video_renderer.cpp` and `src/render/d3d11_video_renderer.cpp`.
- Cleaned the resulting build warnings so the Release build returns to a clean state.

## Validation
```powershell
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build build --config Release --target modern-video-player
./build/Release/modern-video-player.exe --subtitle-style-check ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
./build/Release/modern-video-player.exe --subtitle-sync-check ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
$env:MVP_RENDERER_BACKEND='d3d11'
./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
```

## Validation Results
- Release build: PASS
- `subtitle-style-check.result=PASS`
- `subtitle-sync-check.result=PASS`
- OpenGL `delay-adjust-check.result=PASS`
- D3D11 `delay-adjust-check.result=PASS`
- The previous OpenGL `hr=-2003238890` failure no longer reproduced after the push/pop balance fix.

## Remaining Gaps vs Mature Players
- `\t(...)`, blur/be, more libass edge cases, and broader ASS override coverage are still incomplete.
- The current font attachment support is groundwork only. It covers subtitle-sidecar/private font directories, not full container attachment extraction and registration from formats such as Matroska.
- Complex script shaping, layout parity, and full libass compatibility are still below players such as mpv / MPC-HC + libass.
- HDR output and display-level color management remain a separate unfinished track.
