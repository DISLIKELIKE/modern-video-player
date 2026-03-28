# Forced FailSession deferred release and headless logging override local check

Date: 2026-03-28
Build: `Debug`

## Commands

### Build
```powershell
cmake --build build --config Debug --target modern-video-player
```
Result: PASS

### Forced FailSession check
```powershell
$env:MVP_DISABLE_QUILL_LOGGING='1'
$env:SDL_AUDIODRIVER='dummy'
.\build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200
```
Result: PASS

Key lines:
- `forced-failsession-check.entered_playback_loop=true`
- `forced-failsession-check.fail_session_observed=true`
- `forced-failsession-check.stopped_after_failure=true`
- `forced-failsession-check.runtime_failure_stop_requests=1`
- `forced-failsession-check.runtime_failure_fail_sessions=1`
- `forced-failsession-check.result=PASS`

### Aggregate check entrypoint
```powershell
$env:MVP_DISABLE_QUILL_LOGGING='1'
$env:SDL_AUDIODRIVER='dummy'
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 `
  -ExecutablePath 'build/Debug/modern-video-player.exe' `
  -ProbeFile 'juren-30s.mp4' `
  -SamplesFile 'tools/format_regression/format_samples_ci.csv' `
  -RegressionOutputFile 'build/FORMAT_REGRESSION_CI_LOCAL_TEST.md' `
  -ProbeTimeoutSec 120 `
  -ForcedFailSessionTimeoutSec 240 `
  -RegressionTimeoutSec 900
```
Result: PASS

Key lines:
- `Probe exit code: 0`
- `Forced FailSession exit code: 0`
- `Regression exit code: 0`

## Notes
- The previous CI failure point at forced FailSession teardown no longer reproduces.
- `MVP_DISABLE_QUILL_LOGGING=1` is only required for local/headless validation in this environment; default runtime logging behavior is unchanged.
