# Format Regression CI Timeout Containment Plan

1. Add a CI-only sample manifest for `format-regression`.
2. Teach sample generation to honor a manifest, skip existing files, and enforce per-process timeouts.
3. Add timeout/cleanup guards to `run_all_checks.ps1`.
4. Add per-sample timeout guards to `run_format_regression.ps1`.
5. Update `format-regression.yml` to:
   - use the CI manifest
   - lower sample duration
   - tee logs into `logs/`
   - fail immediately on non-zero script exit
6. Validate locally with:
   - PowerShell parse checks
   - CI-manifest sample generation
   - direct `run_format_regression.ps1`
   - `run_all_checks.ps1` failure-mode containment
