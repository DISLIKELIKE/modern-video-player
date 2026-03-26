# Cross-platform workflow-log error fix design (2026-03-26)

## 1. Goal
- Close unresolved compile errors captured in workflow log without changing feature behavior.

## 2. Compatibility extension design
- Keep FFmpeg compatibility logic centralized in `ffmpeg_channel_layout_compat`.
- Add one timing accessor for `AVFrame` duration:
  - newer FFmpeg: `duration`
  - older FFmpeg: `pkt_duration`

## 3. Integration point
- Replace direct frame-duration access only where decode timing is emitted:
  - video decoded-frame duration
  - audio decoded-frame duration

## 4. Non-goals
- No CLI contract changes.
- No strategy/fallback policy changes.
- No rendering behavior changes.
