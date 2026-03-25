# PLAYERCORE DAY30 - ASS Move Fade Animation and GPU Subtitle Animation Clock

Date: 2026-03-24

## Summary
- Added the next practical ASS animation step toward mature players: `\move`, `\fad` and `\fade`.
- Kept the implementation aligned with the current architecture by storing line-level animation on `SubtitleItem.animation` and evaluating it in both D3D11 and OpenGL subtitle renderers.
- Extended subtitle diagnostics so animation fields are machine-readable.

## Implemented
- Subtitle model
  - Added `SubtitleStyleAnimation` and `SubtitleFadeMode`.
  - Added `SubtitleItem.animation` for line-level ASS motion/fade semantics.
- ASS parser
  - Added tag recognition for `\move`, `\fad` and `\fade`.
  - Added parsing for full-duration move, timed move, simple fade and complex fade windows.
  - Preserved line-level semantics by keeping animation data out of per-run styles.
- D3D11 / OpenGL subtitle rendering
  - Added animation-state evaluation from subtitle clock.
  - Added anchor interpolation for `\move`.
  - Added opacity evaluation for `\fad` and `\fade`.
  - Marked subtitle lines with move/fade as animated content so paused redraw and texture invalidation continue to work.
  - Applied animation opacity to fill / outline / shadow / box brushes.
- Diagnostics / regression
  - Extended `--subtitle-style-check` output with move/fade animation fields.
  - Added `samples/subtitles/opengl_ass_animation_validation.ass`.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_animation_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_animation_validation.ass
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_karaoke_clip_validation.ass
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
```

## Validation Results
- `subtitle-style-check.result=PASS` for `opengl_ass_animation_validation.ass`
- `subtitle-sync-check.result=PASS` for `opengl_ass_animation_validation.ass`
- `subtitle-style-check.result=PASS` for `opengl_ass_karaoke_clip_validation.ass`
- `delay-adjust-check.result=PASS`
- `d3d11-diagnostics.result=PASS`
- `opengl-diagnostics.result=PASS`

## Remaining Gaps vs Mature Players
- `\move / \fad / \fade` are now present, but the next ASS animation gap is still large: `\t(...)`, `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector drawing and vector clip are still missing.
- Subtitle shaping, attachment fonts, advanced fallback and libass-level edge cases are still not covered.
- Display-level HDR output and full color-managed presentation remain separate unfinished workstreams.
