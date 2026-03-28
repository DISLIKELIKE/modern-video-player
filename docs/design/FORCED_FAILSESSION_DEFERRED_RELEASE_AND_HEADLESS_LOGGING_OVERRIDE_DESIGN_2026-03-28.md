# Forced FailSession deferred release and headless logging override design (2026-03-28)

## Goals
- Eliminate worker-thread inline session teardown for `FailureRecoveryPolicy::FailSession`.
- Preserve existing stop / close / diagnostics semantics for non-failing paths.
- Make local headless CLI validation possible without changing default logging behavior.

## Non-Goals
- No change to renderer priority, decoder policy, or format-regression sample scope.
- No change to GitHub Actions workflow structure in this round.
- No change to Vulkan task execution.

## Design

### 1. Deferred FailSession state
- Add `DeferredFailSessionState` to `PlayerCore`.
- Store:
  - pending flag
  - error code
  - message
  - reason

### 2. Worker-thread FailSession behavior
- `handleRuntimeFailure(FailSession)` now:
  - increments fail-session diagnostics
  - arms deferred fail-session state
  - transitions runtime state toward stopping
  - requests deferred stop if needed
  - publishes stopped playback state
- It no longer performs:
  - `applyStopCompletionSideEffects(...)`
  - `applySessionReleaseSideEffects(...)`
  - final `SessionState::Failed` transition inline

### 3. Control-path servicing
- `serviceDeferredStop()` remains the single place that consumes deferred stop.
- After stop completion, it now checks deferred fail-session state.
- When present, it performs:
  - session release
  - `Stopped / Idle / Failed` state closure
  - final error emission

### 4. Check stability
- `runForcedFailSessionCheck()` now treats an observed forced fail-session as proof that playback entered the active path, avoiding false negatives when failure is injected before `VideoPlayer` facade state callbacks settle.

### 5. Headless logging override
- Add env override:
  - `MVP_DISABLE_QUILL_LOGGING=1`
- Applied in logger config/env override stage.
- Result:
  - default desktop behavior stays unchanged
  - local CLI validation can force stdout/stderr logging path

## Tradeoffs
- `FailSession` final error emission is slightly deferred until control-path servicing, but this keeps teardown on the correct thread/ownership boundary.
- Added runtime state is intentionally minimal and isolated to failure-recovery flow.

## Acceptance
- `--forced-failsession-check` must stop cleanly and return `PASS`.
- `tools/run_all_checks.ps1` must complete with exit code `0` under headless local validation.
- No default logging behavior change unless `MVP_DISABLE_QUILL_LOGGING=1` is explicitly set.
