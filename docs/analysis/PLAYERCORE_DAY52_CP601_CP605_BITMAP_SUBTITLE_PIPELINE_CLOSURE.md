# PLAYERCORE Day52: CP-601 ~ CP-605 bitmap subtitle pipeline closure
Date: 2026-03-26

## Background
- Day43 landed the first bitmap subtitle baseline: `hdmv_pgs_subtitle` / `dvd_subtitle` decode, `SubtitleBitmap`, and Windows renderer bitmap branches.
- That baseline still left Phase 6 acceptance gaps:
  - bitmap subtitle items were modeled as one rect per `SubtitleItem`
  - no explicit bitmap cache / reuse policy in renderer paths
  - no dedicated Phase 6 CLI or gate coverage for PGS/DVD and multi-rect stress
  - PGS timeline fallback still allowed invalid `end_display_time` values to explode item duration

## Problem
- PGS/DVD streams were technically decodable, but packet-level timeline semantics and multi-rect composition were not explicitly modeled.
- Renderer bitmap branches recreated upload payloads and D2D bitmap resources for every bitmap rect consumption path.
- Validation remained text-subtitle-centric, so bitmap regression could pass silently without codec/timeline/cache observability.
- Real PGS samples could surface invalid display-window metadata, causing pathological subtitle durations.

## Root Cause
1. Loader modeled each decoded bitmap rect as an independent `SubtitleItem`, so packet-level grouping and multi-rect statistics were lost.
2. OpenGL / D3D11 renderer bitmap branches converted RGBA to premultiplied BGRA and created D2D bitmaps inline with no reuse layer.
3. Existing CLI only exposed `bitmap_codec` and `bitmap_item_count`, which was insufficient for Phase 6 acceptance.
4. Bitmap timeline fallback trusted `end_display_time` too eagerly, even when FFmpeg surfaced sentinel-like values for PGS packets.

## Solution
### 1. Packet-level bitmap subtitle model
- `SubtitleItem` now stores `bitmap_rects` instead of a single bitmap payload.
- Bitmap subtitle loader now emits one `SubtitleItem` per decoded subtitle event and aggregates all bitmap rects into that item.
- `EmbeddedSubtitleLoadResult` now exports:
  - `bitmap_rect_count`
  - `bitmap_multi_rect_item_count`
  - `bitmap_max_rects_per_item`

### 2. Timeline hardening for PGS/DVD
- Added bitmap display-window sanity guard:
  - invalid / oversized `start_display_time` or `end_display_time` falls back to packet-duration-based timing
- This closes the real PGS sample case where invalid display metadata inflated one subtitle item to an absurd duration.

### 3. Renderer composition + cache reuse
- OpenGL subtitle D2D path now iterates `item.bitmap_rects` and composites all rects for one subtitle event.
- D3D11 subtitle D2D path now does the same.
- Both renderers now keep a small bitmap cache keyed by bitmap dimensions + pixel payload hash and reuse cached D2D bitmap resources.
- Cache is cleared when subtitle text resources are reset/recreated.

### 4. Phase 6 diagnostics and gate coverage
- Added CLI:
  - `--bitmap-subtitle-check <media_file> [stream_index]`
  - `--bitmap-subtitle-stress-check`
- `--bitmap-subtitle-check` validates:
  - selected bitmap stream
  - codec name
  - item / rect / multi-rect statistics
  - active-index timeline consistency
- `--bitmap-subtitle-stress-check` validates:
  - multi-rect composition overlap
  - cache reuse candidates from repeated bitmap payloads
  - active item/rect counts under overlap pressure
- `tools/run_opengl_checks.ps1` now adds:
  - embedded DVD bitmap subtitle CLI regression
  - embedded DVD bitmap subtitle playback regression
  - embedded PGS bitmap subtitle CLI regression
  - embedded PGS bitmap subtitle playback regression
  - bitmap subtitle multi-rect stress regression

## Validation
### Build
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
```
- Result: PASS

### Phase 6 CLI
```powershell
.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv
.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv
.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check
```
- Result: PASS
- Key outputs:
  - DVD: `codec_name=dvd_subtitle`, `bitmap_item_count=1`, `bitmap_rect_count=1`, `mismatches=0`
  - PGS: `codec_name=hdmv_pgs_subtitle`, `bitmap_item_count=1`, `bitmap_rect_count=1`, `ordered_checks=18`, `mismatches=0`
  - Stress: `multi_rect_item_count=2`, `cache_reuse_candidate_count=2`, `max_active_item_count=3`, `result=PASS`

### Gate
```powershell
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
```
- Result: `OpenGL gate result: PASS` (`23/23`)

## Outcome
- Phase 6 (`CP-601 ~ CP-605`) is now closed in the repository for Windows-local validation.
- PGS/DVD bitmap subtitle path now has explicit packet-level modeling, multi-rect composition, cache reuse, and dedicated regression coverage.
- The master tasklist can advance to Phase 8 / Phase 9 priority work.

## Remaining Risk
- Linux host runtime still needs its own real bitmap-subtitle playback evidence before claiming full cross-platform runtime parity.
- The PGS sample corpus is still narrow; current validation proves one real PGS sample plus synthetic multi-rect stress, not a broad corpus.

## Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY52_CP601_CP605_BITMAP_SUBTITLE_PIPELINE_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_BITMAP_SUBTITLE_PIPELINE_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE6_BITMAP_SUBTITLE_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE6_LOCAL_CHECK.md`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
