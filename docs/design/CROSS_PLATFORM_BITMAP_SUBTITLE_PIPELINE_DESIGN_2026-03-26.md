# CROSS_PLATFORM_BITMAP_SUBTITLE_PIPELINE_DESIGN_2026-03-26
Date: 2026-03-26
Scope: CP-601 ~ CP-605

## Goal
Close the bitmap subtitle maturity gap so PGS/DVD playback has deterministic timeline modeling, renderer composition, cache reuse, and machine-readable regression coverage.

## Design Decisions
### 1. Model one decoded subtitle event as one timeline item
- Keep `SubtitleItem` as the common subtitle timeline node.
- For bitmap subtitles, store all rects decoded from the same `AVSubtitle` inside `SubtitleItem.bitmap_rects`.
- This keeps `PlayerCore` and timeline selection unchanged while restoring packet/event semantics.

### 2. Export bitmap-specific observability from the loader
`EmbeddedSubtitleLoadResult` exposes:
- `bitmap_item_count`
- `bitmap_rect_count`
- `bitmap_multi_rect_item_count`
- `bitmap_max_rects_per_item`

These fields are consumed by CLI checks and reports so bitmap regressions are visible without reading logs.

### 3. Renderer cache policy
- Cache key: bitmap dimensions + pixel payload hash
- Exclude screen position from key so identical subtitle images can be reused at different coordinates/times.
- Cache value:
  - premultiplied BGRA buffer
  - renderer-native D2D bitmap resource
- Eviction: bounded small LRU-style vector cache (`32` entries)
- Reset policy: clear cache when subtitle text/D2D resources are recreated

### 4. Timeline safety policy
- Use decoded display times only when the display window is sane.
- If display offsets are missing, negative, or unreasonably large, fall back to packet-duration-based timing with a minimum visible window.
- This prevents PGS sentinel-like `end_display_time` values from corrupting the entire bitmap subtitle timeline.

### 5. Regression strategy
- Real-media coverage:
  - DVD sample from FFmpeg sample corpus
  - PGS sample from FFmpeg sample corpus, remuxed into local base media for playback validation
- Synthetic coverage:
  - multi-rect overlap
  - cache reuse candidates via repeated bitmap payloads at different positions

## Dependencies
- FFmpeg subtitle decode path
- existing `SubtitleItem` timeline plumbing in `PlayerCore`
- Windows D2D subtitle composition path in OpenGL / D3D11 renderers
- `tools/run_opengl_checks.ps1`

## Acceptance Mapping
- `CP-601`: PGS decode/model timeline closure
  - covered by packet-level bitmap item modeling + sane display-window fallback + PGS CLI/gate regression
- `CP-602`: DVD decode/model timeline closure
  - covered by DVD real-sample CLI/gate regression
- `CP-603`: sync/composition integration
  - covered by renderer multi-rect composition and active-index timeline checks
- `CP-604`: cache/reuse/upload optimization
  - covered by renderer bitmap cache keyed on dimensions + payload hash
- `CP-605`: multi-rect stress regression
  - covered by `--bitmap-subtitle-stress-check`
