# Format Regression CI Timeout Containment Local Check

## Goal
- Validate the new timeout containment path for `format-regression`.

## Commands
```powershell
powershell -NoProfile -Command "[void][scriptblock]::Create((Get-Content 'tools/download_test_samples.ps1' -Raw)); [void][scriptblock]::Create((Get-Content 'tools/run_all_checks.ps1' -Raw)); [void][scriptblock]::Create((Get-Content 'tools/format_regression/run_format_regression.ps1' -Raw)); 'POWERSHELL_PARSE_OK'"

powershell -ExecutionPolicy Bypass -File .\tools\download_test_samples.ps1 -DurationSec 2 -SamplesFile 'tools/format_regression/format_samples_ci.csv' -SkipExisting -ProcessTimeoutSec 420

powershell -ExecutionPolicy Bypass -File .\tools\format_regression\run_format_regression.ps1 -ExecutablePath 'build/Debug/modern-video-player.exe' -SamplesFile 'tools/format_regression/format_samples_ci.csv' -OutputFile 'build/FORMAT_REGRESSION_CI_ONLY_LOCAL_TEST.md' -ProbeTimeoutSec 120

powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath 'build/Debug/modern-video-player.exe' -ProbeFile 'juren-30s.mp4' -SamplesFile 'tools/format_regression/format_samples_ci.csv' -RegressionOutputFile 'build/FORMAT_REGRESSION_CI_LOCAL_TEST.md' -ProbeTimeoutSec 120 -ForcedFailSessionTimeoutSec 240 -RegressionTimeoutSec 900
```

## Result
- PowerShell parse check: PASS
- CI-manifest sample generation: PASS
- `run_format_regression.ps1`: PASS
  - `build/FORMAT_REGRESSION_CI_ONLY_LOCAL_TEST.md`
  - `Total=13 PASS=13 PARTIAL=0 FAIL=0`
- `run_all_checks.ps1`: contained non-zero failure without hanging
  - probe stage: PASS
  - forced FailSession stage: exited in about 60s with `Forced FailSession exit code: -805306369`
  - important containment result: script returned promptly instead of waiting for the job-level timeout

## Notes
- The local host currently reproduces a non-zero `--forced-failsession-check` runtime issue independent of this workflow containment change.
- This round fixes timeout containment and observability; it does not change the player runtime semantics of `--forced-failsession-check`.
