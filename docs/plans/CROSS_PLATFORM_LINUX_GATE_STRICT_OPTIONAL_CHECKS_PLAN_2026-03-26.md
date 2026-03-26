# Cross-platform Linux gate strict optional checks plan (2026-03-26)

## Scope
- Close Linux gate determinism gap for subtitle backlog probe coverage in CI.
- No new player runtime feature; this is gate/automation closure work.

## Plan
1. Confirm the current skip path in `tools/run_linux_mvp_checks.sh` for `CP-508`.
2. Add embedded ASS fixture auto-generation in Linux gate script.
3. Add strict optional-check switch so skipped `CP-507/CP-508` can fail gate in CI.
4. Update `.github/workflows/cross-platform-gate.yml` Linux lane:
   - install `ffmpeg`
   - invoke Linux gate with strict mode and explicit fixture arguments
5. Sync docs/records and run local validation reachable on Windows host.

## Dependencies
- Step 3 depends on step 2 (strict mode needs presence signals after generation attempt).
- Step 4 depends on steps 2/3 (argument contract and strict behavior must exist).
- Validation/report depends on all implementation/doc sync steps.

## Acceptance
- Linux gate script supports auto-generation of CP-508 fixture when absent.
- Linux gate script can enforce `CP-507/CP-508` as required checks in strict mode.
- CI Linux lane is configured to run the strict mode with `ffmpeg` installed.
- Records and plan/report documents are synced.
