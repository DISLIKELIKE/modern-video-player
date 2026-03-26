# Cross-platform workflow-log error fix plan (2026-03-26)

## Scope
- Fix remaining compile errors identified in `log` for Linux workflow run.

## Plan
1. Parse workflow log and isolate unresolved errors from already-fixed ones.
2. Implement FFmpeg `AVFrame` duration field compatibility helper.
3. Replace direct `frame->duration` reads in `PlayerCore` decode path.
4. Run local build + regression sanity checks.
5. Sync records and docs indexes.

## Dependencies
- Step 3 depends on step 2.
- Step 4 depends on implementation completion.
- Step 5 depends on validated command results.

## Acceptance
- No direct `AVFrame::duration` usage remains in `PlayerCore` decode outputs.
- Build and key local regressions pass.
