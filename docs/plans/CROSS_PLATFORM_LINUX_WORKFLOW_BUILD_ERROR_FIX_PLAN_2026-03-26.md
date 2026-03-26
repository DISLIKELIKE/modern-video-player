# Cross-platform Linux workflow build error fix plan (2026-03-26)

## Scope
- Fix current Linux lane compile errors observed in `cross-platform-gate` workflow runs.

## Plan
1. Parse failed workflow run logs and isolate direct compile blockers.
2. Fix libass timestamp type mismatch in `src/subtitle/libass_probe.cpp`.
3. Fix Linux helper/type visibility gap in `src/render/opengl_video_renderer.cpp`.
4. Run local build/regression commands on Windows host.
5. Sync records and index docs (`analysis/design/plans/reports` + `records`).

## Dependencies
- Step 2 and step 3 depend on step 1 findings.
- Step 4 depends on implementation completion.
- Step 5 depends on validated command outputs.

## Acceptance
- Known Linux compile errors from workflow log are removed in source code.
- Windows local build and key checks remain pass.
- Documentation chain is updated for this round.
