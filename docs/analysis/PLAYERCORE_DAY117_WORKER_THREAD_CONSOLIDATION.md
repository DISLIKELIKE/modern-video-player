# PLAYERCORE DAY117: Worker thread consolidation
Date: 2026-04-10
Status: Done

## 1. Problem Statement
- `PlayerCore` still carried duplicated lifecycle code for `demux` and `audio consumer` workers.
- Each worker owned a separate `std::thread` plus `std::atomic<bool>` pair with repeated start/stop/reap/join branches.
- Repository still shipped an unused `DecoderThread` abstraction that no longer matched the current `PlayerCore -> Scheduler` mainline.

## 2. Root Cause
- Thread ownership concerns were never extracted from the old prototype-era helper.
- Current mainline evolved toward explicit worker loops, but the common ownership/shutdown mechanics stayed duplicated in `PlayerCore`.
- The existing `DecoderThread` abstraction was pause/resume oriented and not actually wired into current playback workers.

## 3. Implementation Planner Used
1. Freeze scope to `PlayerCore` worker lifecycle only.
2. Add a reusable `core::WorkerThread` primitive.
3. Migrate `demux` worker ownership to the new helper.
4. Migrate `audio consumer` worker ownership to the new helper.
5. Remove unused legacy `DecoderThread`.
6. Validate with local build + short playback diagnostics.
7. Sync docs/records/index closure.

## 4. Landed Changes
- Added:
  - `include/core/worker_thread.h`
  - `src/core/worker_thread.cpp`
- Refactored:
  - `include/core/player_core.h`
  - `src/core/player_core.cpp`
  - `CMakeLists.txt`
- Removed:
  - `include/core/decoder_thread.h`
  - `src/core/decoder_thread.cpp`

## 5. Result
- `PlayerCore` no longer keeps duplicated `thread + running-flag` pairs for `demux` and `audio consumer`.
- Worker lifecycle behavior is now centralized in `core::WorkerThread`:
  - `start`
  - `requestStop`
  - `join`
  - `joinIfFinished`
  - `running/joinable/stopRequested`
- Playback policy remains in `PlayerCore` and `Scheduler`; the worker helper only owns thread lifecycle.
- Queue semantics were intentionally not changed:
  - `ThreadSafeQueue`
  - `FrameQueue`
  - EOF / generation / flush behavior

## 6. Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player`
- Runtime:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
- Key outcomes:
  - Build succeeded.
  - `performance-log-check.open_ok=true`
  - `performance-log-check.entered_playback_loop=true`
  - `performance-log-check.audio_output_initialized=true`
  - `performance-log-check.scheduler_video_restart_attempts=0`
  - `performance-log-check.scheduler_audio_restart_attempts=0`
  - `performance-log-check.result=PASS`

## 7. Remaining Risks
- `Scheduler` still owns three explicit worker threads; this round intentionally did not refactor them.
- Renderer-side threads in `display` / OpenGL / D3D11 remain separate and were intentionally left untouched.
- Build still emits unrelated historical source-encoding warnings from `src/video_player.cpp`.
