# Cross-platform Linux CI compatibility and gate stabilization plan (2026-03-26)

## Scope
- Stabilize Linux-first cross-platform closure after CI gate failures.
- Exclude macOS (`CP-1001 ~ CP-1005` remains deferred).

## Plan
1. Finish FFmpeg channel-layout compatibility abstraction and replace direct field usage.
2. Complete `PlayerCore` resampler path migration to compatibility state.
3. Fix Linux compile blockers (`libass` include compatibility + OpenGL enum macro-safe rename).
4. Harden CI workflow:
   - auto-generate probe media if missing
   - build plugin target required by packaging/install
5. Run local validation commands on Windows host and Linux script syntax checks.
6. Sync records + analysis/design/report/index docs.

## Dependencies
- Step 2 depends on step 1.
- Step 4 should run after steps 1~3 to avoid repeated CI red runs from code compile failures.
- Step 5 depends on implementation completion (steps 1~4).
- Step 6 depends on validated command outputs from step 5.

## Acceptance
- Windows local build passes with `modern-video-player` and `sample_logger_plugin`.
- `--performance-log-check`, `--embedded-subtitle-live-packet-check`, `--d3d11-diagnostics` pass locally.
- `tools/run_linux_mvp_checks.sh` passes syntax check and keeps non-Linux guard behavior.
- CI workflow no longer assumes committed probe media and no longer misses plugin target build.
