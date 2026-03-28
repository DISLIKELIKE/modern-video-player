# Forced FailSession deferred release and headless logging override plan (2026-03-28)

## Objective
- Close the remaining runtime bug behind format-regression step-2 timeout by moving `FailSession` teardown off worker threads and validating the aggregate check entrypoint locally.

## Implementation Planner

| Step | Description | Dependency | Status |
| --- | --- | --- | --- |
| 1 | Add deferred fail-session state to `PlayerCore` | none | Done |
| 2 | Refactor `handleRuntimeFailure(FailSession)` to arm deferred state and request stop only | Step 1 | Done |
| 3 | Extend `serviceDeferredStop()` to perform session release and final failed-state publication | Step 2 | Done |
| 4 | Harden `runForcedFailSessionCheck()` against immediate injected failure timing | Step 3 | Done |
| 5 | Add `MVP_DISABLE_QUILL_LOGGING=1` local/headless override for validation | none | Done |
| 6 | Build and run local forced FailSession + aggregate checks | Steps 3, 4, 5 | Done |
| 7 | Sync records and doc indexes | Step 6 | Done |

## Validation Commands
- `cmake --build build --config Debug --target modern-video-player`
- `$env:MVP_DISABLE_QUILL_LOGGING='1'; $env:SDL_AUDIODRIVER='dummy'; .\build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`
- `$env:MVP_DISABLE_QUILL_LOGGING='1'; $env:SDL_AUDIODRIVER='dummy'; powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath 'build/Debug/modern-video-player.exe' -ProbeFile 'juren-30s.mp4' -SamplesFile 'tools/format_regression/format_samples_ci.csv' -RegressionOutputFile 'build/FORMAT_REGRESSION_CI_LOCAL_TEST.md' -ProbeTimeoutSec 120 -ForcedFailSessionTimeoutSec 240 -RegressionTimeoutSec 900`

## Exit Criteria
- forced FailSession check returns `PASS`
- `run_all_checks.ps1` returns exit code `0`
- docs/records + analysis/design/plans/reports indexes are updated
