# CROSS_PLATFORM_PHASE5_BACKLOG_CP507_CP508_PLAN
Date: 2026-03-26
Scope: CP-507, CP-508
Status: Executed

## 1. Goal
- Close the remaining Linux subtitle backlog tasks in the master tasklist:
  - `CP-507` libass shaping/layout probe baseline
  - `CP-508` embedded subtitle live packet probe baseline

## 2. Execution Plan (with dependencies)
1. Add `libass` probe abstraction and Linux-only implementation (`CP-507`).
2. Add embedded subtitle packet-level probe API in loader (`CP-508`).
3. Wire CLI entry points and usage text in `main.cpp`.
4. Extend Linux gate script with optional `CP-507`/`CP-508` checks.
5. Run local build + command validation.
6. Sync tasklist, docs indexes, and records.

Dependency notes:
- Step 3 depends on Step 1 + Step 2.
- Step 4 depends on Step 3 command availability.
- Step 5 depends on Step 3 + Step 4.
- Step 6 depends on Step 5 validation completion.

## 3. Acceptance Mapping
- `CP-507`:
  - machine-readable command exists
  - Linux libass init/track/event/render probe fields are exposed.
- `CP-508`:
  - packet-level probe API exists in subtitle loader
  - command exposes packet counters/monotonicity/output indicators.

## 4. Outcome
- Both backlog items were moved to `DONE` in `CROSS_PLATFORM_MASTER_TASKLIST.md`.
- Linux host execution remains the final runtime evidence gap on this Windows workstation.

