# PLAYERCORE Day111: format-regression CI timeout containment

## Problem
- GitHub Actions `format-regression.yml` was timing out at the job level (`90` minutes) instead of failing at the script level.
- `tools/download_test_samples.ps1` regenerated a heavy full sample matrix even when the CI regression only needed a smaller subset.
- `tools/run_all_checks.ps1` and `tools/format_regression/run_format_regression.ps1` used blocking child-process waits without timeout guards.

## Root Cause
- The workflow spent too much of its budget on unnecessary sample generation.
- Once a child command stalled, the workflow had no script-level timeout or process-tree cleanup path.
- The resulting failure signal was a late GitHub cancellation instead of an actionable script failure.

## Containment Goal
- Make CI fail fast at the script boundary.
- Limit format regression CI to a curated, lower-cost sample set.
- Preserve machine-readable outputs and log artifacts for diagnosis.
