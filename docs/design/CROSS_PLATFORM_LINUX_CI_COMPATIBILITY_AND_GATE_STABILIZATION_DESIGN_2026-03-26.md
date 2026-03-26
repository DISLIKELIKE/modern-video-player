# Cross-platform Linux CI compatibility and gate stabilization design (2026-03-26)

## 1. Goal
- Keep macOS deferred and close Linux-first cross-platform CI instability.
- Ensure the same codebase compiles against both legacy and modern FFmpeg channel APIs.
- Make CI gate deterministic when probe media is not committed into repository history.

## 2. FFmpeg compatibility design
- Introduce `ffmpeg_channel_layout_compat` as the only translation layer for:
  - channel count extraction from `AVCodecParameters` / `AVCodecContext` / `AVFrame`
  - channel layout mask extraction/defaulting
- Use compile-time preprocessor branching (`LIBAVUTIL_VERSION_MAJOR`) instead of mixed in-function field probing.
- Keep `PlayerCore` resampler cache state as plain values:
  - input/output layout masks
  - input/output channel counts
  - sample format/rate

## 3. Resampler API compatibility design
- Use `swr_alloc_set_opts2` branch for modern swresample + channel-layout struct.
- Keep `swr_alloc_set_opts` fallback branch for legacy swresample ABI.
- Shared reinit trigger logic compares normalized layout/channels/sample format/sample rate state.

## 4. Linux compile robustness design
- `libass` include order:
  - prefer `<ass/ass.h>`
  - fallback to `<libass/ass.h>`
- Avoid enum token names with high macro-collision risk (`None`) in OpenGL path.

## 5. CI determinism design
- Add probe fixture generation stage in both OS lanes:
  - create `samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4` via ffmpeg when absent
- Build both package-relevant targets:
  - `modern-video-player`
  - `sample_logger_plugin`

## 6. Compatibility impact
- No CLI contract changes.
- No strategy-policy behavior change in `CP-101 ~ CP-106`.
- Change scope is build/runtime compatibility and CI execution determinism.
