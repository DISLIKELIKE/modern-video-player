# CROSS_PLATFORM_PHASE5_SUBTITLE_FONT_CLOSURE_PLAN
Date: 2026-03-26
Scope: CP-501 ~ CP-506
Status: Executed

## 1. Goal
- Close Phase-5 subtitle/font baseline tasks as a deterministic, machine-verified set.
- Keep Linux prioritized for capability closure while preserving Windows gate maturity.

## 2. Execution Plan (with dependencies)
1. Fix compile blocker in current branch (`main.cpp` subtitle policy write to const settings snapshot).
2. Complete `CP-505` Linux font lifecycle closure in `subtitle_font_registry`:
   - attachment extraction already landed;
   - add Linux registration (`fontconfig`);
   - add cleanup rebuild flow after media-session release.
3. Complete `CP-506` gate coverage:
   - include `--directwrite-font-collection-check` in Windows OpenGL gate.
4. Run build + CLI checks + gate script.
5. Sync tasklist/docs/records.

Dependency notes:
- Step 1 is required before any validation.
- Step 2 and Step 3 can be implemented independently after Step 1.
- Step 4 depends on Step 2 + Step 3.
- Step 5 depends on Step 4.

## 3. Acceptance Mapping
- `CP-501`: embedded subtitle policy language/forced/SDH + persistence applied.
- `CP-502`: subtitle track catalog/current selection/switch feedback surfaced to UI overlay path.
- `CP-503`: subtitle policy CLI options and check command stable.
- `CP-504`: ownership policy (`external > embedded`, clear fallback) machine-check stable.
- `CP-505`: Linux attachment-font extract/register/cleanup logic implemented.
- `CP-506`: DirectWrite custom collection integration validated and included in gate.

## 4. Outcome
- All six tasks (`CP-501` to `CP-506`) moved to `DONE` in master tasklist.
- Windows local checks are passing; Linux runtime follow-up remains a tracked risk for `fontconfig` branch execution.
