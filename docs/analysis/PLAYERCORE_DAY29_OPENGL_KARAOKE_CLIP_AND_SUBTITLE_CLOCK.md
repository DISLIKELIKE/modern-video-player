# PLAYERCORE DAY29 - OpenGL Karaoke Clip and Subtitle Clock

Date: 2026-03-24

## Summary
- Completed the second subtitle capability batch for the GPU subtitle path.
- Closed the loop from ASS parser -> subtitle model -> PlayerCore subtitle clock -> D3D11/OpenGL DirectWrite/D2D rendering.
- Added a new regression sample for karaoke and rectangular clip coverage.

## Implemented
- Subtitle model
  - Added `secondary_color` to subtitle style.
  - Added rectangular `clip` / `iclip` fields to subtitle style.
  - Added karaoke mode and karaoke timing fields to subtitle text runs.
- ASS parser
  - Added support for `SecondaryColour` style field.
  - Added support for `\2c \2a \3c \3a \4c \4a \alpha`.
  - Added support for `\clip(...)` and `\iclip(...)` rectangular clip parsing.
  - Added support for `\k \kf \ko` and uppercase `\K` mapping.
  - Prevented merging runs when karaoke timing is present.
- PlayerCore / renderer interface
  - Added `IVideoRenderer::setSubtitleClock()` and forwarded the adjusted subtitle clock from PlayerCore.
  - D3D11 and OpenGL renderers now update animated subtitle state and redraw paused frames when karaoke timing changes.
- D3D11 subtitle rendering
  - Added run-level fill / outline / shadow brushes.
  - Added karaoke sweep highlight overlay rendering.
  - Added rectangular `clip` and basic `iclip` rendering using clip-region decomposition.
- OpenGL subtitle rendering
  - Ported the same run-level fill / outline / shadow / karaoke overlay logic to the D2D subtitle texture path.
  - Added animated subtitle texture invalidation based on subtitle clock.
  - Added rectangular `clip` and basic `iclip` rendering using clip-region decomposition.
- Diagnostics / regression
  - Extended `--subtitle-style-check` output with `secondary_rgba`, `has_clip`, `inverse_clip`, clip bounds, karaoke mode and karaoke timing.
  - Added `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_karaoke_clip_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
```

## Validation Results
- `subtitle-style-check.result=PASS` for `opengl_ass_style_validation.ass`
- `subtitle-style-check.result=PASS` for `opengl_ass_karaoke_clip_validation.ass`
- `subtitle-sync-check.result=PASS`
- `delay-adjust-check.result=PASS`
- `d3d11-diagnostics.result=PASS`
- `opengl-diagnostics.result=PASS`
- Local probe machine: `NVIDIA GeForce GTX 1080`, driver `32.0.15.6094`, `WGL_NV_DX_interop` available, `NV12` shader sampling supported, decoder profiles present for `H.264 / HEVC / VP9`, AV1 decoder profile absent on this adapter.

## Remaining Gaps vs Mature Players
- `iclip` is only implemented for rectangular clip geometry; vector clip paths are still missing.
- Advanced ASS animation semantics are still incomplete: `move`, `fad`, `fade`, `fax`, `fay`, `frx`, `fry`, vector drawing and attachment font workflows are not implemented.
- Text shaping and fallback are still below a `libass`-class pipeline.
- Display-level HDR output, metadata propagation and color-managed presentation are still not complete.
