# Cross-platform Linux gate reporting plan (2026-03-26)

## Scope
- Add machine-readable Linux gate report output and CI artifact archival.

## Plan
1. Extend Linux gate script with optional report file contract.
2. Add per-check stable check IDs and status output (`PASS/FAIL/SKIPPED`) into report.
3. Record global gate result and fail reason for early exits.
4. Update Linux CI step:
   - enforce pipefail
   - tee gate output to log artifact
   - pass report-file argument
5. Update docs/records and run local build/sanity checks.

## Dependencies
- Step 2 depends on step 1 (check status needs report writer).
- Step 3 depends on step 1 (shared fail path helper).
- Step 4 depends on step 1 (new report-file argument contract).
- Step 5 depends on all implementation steps.

## Acceptance
- `tools/run_linux_mvp_checks.sh` supports report generation via arg/env.
- Linux CI uploads both `linux-mvp-gate.log` and `linux-mvp-gate-summary.env`.
- Local validation shows build/regression unaffected.
