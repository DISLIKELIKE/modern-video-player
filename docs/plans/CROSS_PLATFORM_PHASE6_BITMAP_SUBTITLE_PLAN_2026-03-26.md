# CROSS_PLATFORM_PHASE6_BITMAP_SUBTITLE_PLAN_2026-03-26
Date: 2026-03-26
Scope: CP-601 ~ CP-605
Status: Completed

## Problem Summary
- Bitmap subtitle baseline existed, but Phase 6 acceptance still lacked packet-level modeling, cache reuse, and dedicated regression coverage.

## Planner
1. Upgrade bitmap subtitle model from single-rect item to multi-rect item.
   - Dependency: none
2. Refactor bitmap loader to aggregate rects per decoded subtitle event and export richer stats.
   - Dependency: step 1
3. Add renderer bitmap cache and multi-rect composition loops in OpenGL / D3D11 paths.
   - Dependency: step 1, step 2
4. Add dedicated bitmap CLI checks and extend gate script with DVD / PGS / stress coverage.
   - Dependency: step 2, step 3
5. Build, validate, and sync docs / records / tasklist.
   - Dependency: step 1~4

## Completion Notes
- All five steps were executed in order.
- Phase 6 is now marked `DONE` in `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`.
