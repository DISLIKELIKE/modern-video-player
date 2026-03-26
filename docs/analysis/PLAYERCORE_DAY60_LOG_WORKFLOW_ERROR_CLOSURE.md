# PLAYERCORE Day60: workflow log error closure

Date: 2026-03-26  
Status: Done (log-driven fixes landed and locally validated)

## 1. Problem
- `log` file still showed Linux CI compile errors after previous stabilization round.
- Main unresolved item in latest error set:
  - `AVFrame` has no member `duration` on older FFmpeg headers (suggested `pkt_duration`).

## 2. Root Cause
1. `PlayerCore` still used `frame->duration` directly in video/audio decode output timing.
2. Compatibility layer covered channel layout differences but not `AVFrame` duration field differences.

## 3. Solution
- Extended `include/media/ffmpeg_channel_layout_compat.h` with:
  - `frameDuration(const AVFrame* frame)` using
    - `frame->duration` for newer libavutil
    - `frame->pkt_duration` for older libavutil
- Replaced direct `AVFrame::duration` reads in `src/core/player_core.cpp` with compatibility helper.

## 4. Outcome
- The remaining log-reported FFmpeg-old compile compatibility gap is removed in code.
- Windows local build and regression checks remain green after compatibility patch.

## 5. Remaining Gap
- Final Linux runner evidence still depends on push + GitHub Actions run.
