# PLAYERCORE DAY46: Cross-Platform Master Tasklist Consolidation
Date: 2026-03-25
Status: Done

## 1. Problem Statement
- Cross-platform work items existed, but were split across multiple docs with different granularity:
  - `CROSS_PLATFORM_EVOLUTION_ROADMAP.md`
  - `CROSS_PLATFORM_REFACTOR_TASKLIST.md`
  - `PHASE1_CROSS_PLATFORM_TODO.md`
  - OpenGL post-top10 backlog notes
- User needed one execution-ready master checklist to run work directly without repeatedly merging context from multiple files.

## 2. Root Cause
- Existing docs are good for architecture and phase-level planning, but not a single operational board for end-to-end delivery.
- Recent OpenGL/subtitle/HDR progress changed the real baseline, while some older plan docs still represent earlier assumptions.

## 3. Consolidation Strategy
- Keep existing plan docs as source context; do not replace them.
- Add one new master tasklist in `docs/plans` as the executable entry.
- Use unified task IDs (`CP-xxx`) and phase gates (`M1..M5`) so future progress can be tracked without re-indexing.
- Explicitly map `DONE / NEXT / LATER` to current repository status and backlog.

## 4. Implementation Planner Used
1. Extract phase/task intent from the three cross-platform plan docs.
2. Merge with current repository status (OpenGL/subtitle/HDR already landed items and known backlog).
3. Produce one phase-ordered task matrix with:
   - task IDs
   - priority state
   - acceptance criteria
   - immediate execution order
4. Sync records (`CHANGELOG`, `VERSION`, `DEVELOP_LOG`) and run local validation.

## 5. Delivered Artifact
- Added:
  - `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- Scope covered:
  - strategy extraction
  - renderer/display/input cleanup
  - Linux MVP
  - subtitle/font closure
  - bitmap subtitle
  - VAAPI phase
  - HDR/ICC/LUT
  - CI and packaging
  - macOS later-stage path

## 6. Validation
- Documentation cross-reference check:
  - `rg -n "CROSS_PLATFORM_MASTER_TASKLIST" docs/plans docs/analysis docs/records`
- Local build validation:
  - `cmake --build build --config Release --target modern-video-player`

## 7. Next Recommended Start Point
- Start from `CP-101` to `CP-106` (strategy and platform capability extraction), because this is the highest leverage step and unblocks Linux cleanly without forking core logic.
