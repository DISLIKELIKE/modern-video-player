# Format Regression CI Timeout Containment Design

## Scope
- `tools/download_test_samples.ps1`
- `tools/run_all_checks.ps1`
- `tools/format_regression/run_format_regression.ps1`
- `.github/workflows/format-regression.yml`

## Design
- Add manifest-driven sample generation so CI only builds samples listed in a dedicated regression manifest.
- Add explicit timeout handling and process-tree cleanup for script-owned child commands.
- Keep `--forced-failsession-check` on console passthrough mode so timeout handling does not change its runtime I/O behavior.
- Emit fixed workflow logs under `logs/` so failures produce artifacts before the job-level timeout is reached.

## CI Sample Policy
- Keep baseline container/codec coverage in CI.
- Drop high-cost 4K multi-audio HEVC/AV1 permutations from the CI lane.
- Preserve broader format coverage for local/full runs through the existing full manifest.
