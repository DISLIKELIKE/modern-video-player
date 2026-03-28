# PlayerCore Day111: forced FailSession deferred release and headless logging override

Date: 2026-03-28
Scope: format-regression / `run_all_checks.ps1` / PlayerCore runtime-failure recovery

## Problem
- The format-regression workflow no longer died only at the outer 90-minute job timeout, but it still failed at step 2:
  - `--forced-failsession-check`
- GitHub Actions log stalled after:
  - `PlayerCore applied stop-completion side effects reason=forced fail-session video decode injection ...`
- Local reproduction matched the same boundary:
  - the process no longer advanced through session release on the same thread path
  - headless local validation also hit a separate Quill `ConsoleSink` console-handle failure on process exit

## Observed Runtime Path
- `decodeVideoFrame()` can inject `FailureRecoveryPolicy::FailSession` from the scheduler video-decode thread.
- `handleRuntimeFailure(FailSession)` previously executed:
  - `applyStopRequestSideEffects(...)`
  - `applyStopCompletionSideEffects(...)`
  - `applySessionReleaseSideEffects(...)`
  - state transitions to `Stopped / Idle / Failed`
- That meant worker-thread code was synchronously stopping scheduler threads and tearing down renderer / decoder / demuxer resources.

## Root Cause
- `FailSession` teardown was performed inline on worker threads instead of being serviced on the control path.
- This created an unsafe ownership boundary around:
  - `scheduler_.stop()`
  - renderer shutdown
  - decoder and demuxer release
- CI reproduced the issue as a hang; local headless runs reproduced it as a process-exit failure after the same stop-completion boundary.

## Decision
- Keep worker-thread `FailSession` handling minimal:
  - arm deferred fail-session metadata
  - request stop
  - publish stopped playback state
- Move stop-completion + session-release + final `Failed` state publication into `serviceDeferredStop()`.
- Add a headless-only logger override:
  - `MVP_DISABLE_QUILL_LOGGING=1`
  - keeps default runtime behavior unchanged
  - enables local CLI validation without Quill `ConsoleSink` dependency on a real console handle

## Affected Files
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/logger.cpp`

## Validation Target
- `.\build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 ...`
- Expected outcome:
  - forced FailSession check exits promptly
  - result is `PASS`
  - aggregate check script returns exit code `0`
