# PlayerCore Day 36: OpenGL HDR probe, quirk table expansion, subtitle gate and final gap matrix

Date: 2026-03-25

## Summary
- Added display-output HDR capability probing to `--opengl-diagnostics`.
- Expanded the OpenGL native interop quirk rule table with additional software/virtual-context entries and finer matching keys.
- Expanded the OpenGL regression gate to cover the current ASS subtitle sample suite.
- Closed the current OpenGL next-stage task table with a backend-level mature-player gap matrix.

## Completed Tasks
- Task 4: HDR output capability probe
- Task 7: ASS/SSA stress sample bolster
- Task 8: driver/adapter quirk rule table expansion
- Task 9: HDR/color design convergence update
- Task 10: mature-player gap analysis

## 1. HDR output capability probe
### Implementation
- Added `OpenGLHdrOutputDiagnosticsSnapshot` to `OpenGLDiagnosticsSnapshot`.
- The probe now enumerates DXGI adapters/outputs based on the D3D11 diagnostics adapter and reports:
  - matched adapter index
  - output found / attached-to-desktop
  - output6 availability
  - current output color space
  - advanced-color active state
  - HDR active state
  - bits-per-color
  - min/max/full-frame luminance
- These fields are exported through `--opengl-diagnostics` under `opengl-diagnostics.hdr_output.*`.

### Current local result
- In the current shell environment, the probe reports:
  - adapter matched
  - no desktop DXGI output found
  - therefore HDR output capability details remain unavailable in this session
- This is still useful because it separates “renderer only does internal HDR-aware conversion” from “the current execution environment cannot see the display-output layer”.

## 2. Quirk rule table expansion
### Implementation
- Extended `OpenGLNativeInteropRule` with `device_id` and `subsystem_id` match keys so future rules can be more specific without restructuring the table again.
- Added conservative quirk entries for:
  - Google SwiftShader software OpenGL
  - Mesa llvmpipe software OpenGL
  - VMware virtual GPU OpenGL
  - Parallels virtual GPU OpenGL
- Retained the existing AMD Radeon conservative disable rule and the software-context rule.

### Rationale
- Mature players keep accumulating “known-bad combinations” as data, not as scattered conditional branches.
- The rule-table shape now supports that style of growth directly.

## 3. Subtitle stress gate completion
### Implementation
- Extended `tools/run_opengl_checks.ps1`.
- The OpenGL gate now validates:
  1. diagnostics
  2. native playback
  3. forced copy-back playback
  4. immediate present mode
  5. 10-bit forced copy-back playback
  6. subtitle delay regression
  7. `opengl_ass_style_validation.ass`
  8. `opengl_ass_karaoke_clip_validation.ass`
  9. `opengl_ass_animation_validation.ass`
  10. `opengl_ass_transform_vector_font_validation.ass`
- This makes the current ASS sample corpus part of the reusable OpenGL gate instead of leaving it as manual terminal history.

## 4. Final OpenGL mature-player gap matrix
### Status vs mature players
| Area | Current OpenGL status | Gap vs mpv / MPC-HC / VLC |
|---|---|---|
| Core playback | Stable local playback, seek, drag-drop, native/copy-back switching, present pacing diagnostics | Small |
| D3D11 native interop | Mature enough for daily Windows playback with startup diagnostics and quirk-based downgrade | Small to medium |
| 10-bit fallback path | `p010le/p016le` copy-back upload is now preserved without forced 8-bit swscale | Small |
| Subtitle rendering | GPU subtitle path is far beyond plain text overlay and covers major ASS batches already added | Medium |
| Full libass parity | Still below mature players in shaping, attachment extraction, full layout parity, and some effect semantics | Large |
| Display-level HDR output | Probe/design are now present, but no DXGI HDR present bridge or HDR metadata output yet | Large |
| Color management | Internal matrix/gamut/tone-map path exists; ICC/3D LUT output management is still missing | Large |
| User shader/filter ecosystem | No mpv/VLC-class user shader/filter stack yet | Large |
| Driver quirk coverage | Mechanism is in place, but the real rule corpus is still small | Medium |
| Automated OpenGL gating | Now has a concrete reusable gate script, but still not image-diff based | Medium |

### Practical conclusion
- For Windows local playback, OpenGL is no longer in the “experimental M0” stage.
- For display output and subtitle semantics, it is still behind mature players in the exact places that usually take the longest: full libass parity, display-level HDR, color-managed output, and larger driver rule corpora.
- The remaining gaps are now explicit and scoped, not diffuse.

## Validation
```powershell
cmake --build build --config Release --target modern-video-player

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --opengl-diagnostics

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## Key Results
- `opengl-diagnostics.result=PASS`
- `opengl-diagnostics.hdr_output.probe_succeeded=true`
- current shell result: `hdr_output.output_found=false` with note `matched adapter has no DXGI outputs`
- `OpenGL gate result: PASS`

## Files
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/analysis/PLAYERCORE_DAY36_OPENGL_HDR_PROBE_QUIRK_TABLE_AND_GAP_MATRIX.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
