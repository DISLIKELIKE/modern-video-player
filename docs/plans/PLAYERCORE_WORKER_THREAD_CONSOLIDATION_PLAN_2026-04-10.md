# PlayerCore worker thread consolidation plan (2026-04-10)

## Scope
- Reduce duplicated thread lifecycle code in `PlayerCore` without changing playback policy, queue semantics, or renderer behavior.
- Extract one reusable worker-thread primitive for long-running loop workers.
- Land the first adoption on `demux` and `audio consumer` workers only; `Scheduler` remains behaviorally unchanged in this round.

## Problem
- `PlayerCore` keeps separate `std::thread` + `std::atomic<bool>` pairs for `demux` and `audio consumer`.
- Start/stop/reap/join behavior is duplicated across those workers.
- Repository still contains an unused legacy `DecoderThread` abstraction that is not aligned with current `PlayerCore -> Scheduler` mainline.

## Implementation Planner
1. Freeze scope to `PlayerCore` worker lifecycle consolidation only.
Dependency: none.
2. Add a reusable `core::WorkerThread` primitive with:
   - start
   - stop-request
   - join
   - reap-finished
   - running / joinable / current-thread checks
Dependency: Step 1.
3. Replace `PlayerCore` `demux_thread_ + demux_running_` with the new worker primitive.
Dependency: Step 2.
4. Replace `PlayerCore` `audio_consumer_thread_ + audio_consumer_running_` with the new worker primitive.
Dependency: Step 3.
5. Remove the unused legacy `DecoderThread` implementation and wire the build to the new worker primitive.
Dependency: Step 4.
6. Run minimal local validation:
   - configure/build target
   - confirm the refactor compiles cleanly
Dependency: Step 5.
7. Sync records and analysis/report closure.
Dependency: Step 6.

## Acceptance
- `PlayerCore` no longer owns duplicated thread+flag pairs for `demux` and `audio consumer`.
- Worker stop/join/reap behavior remains explicit and self-contained.
- No queue behavior change:
  - packet `generation`
  - `flush`
  - `eof`
  - bounded wait semantics
- `Scheduler` behavior is unchanged in this round.
- Build passes locally.
- No commit/push without explicit user confirmation.

## Design Notes
- Prefer a minimal worker primitive over a larger framework; this matches the repository rule of only introducing abstraction when it removes real duplication.
- Keep playback policy in `PlayerCore`/`Scheduler`, and keep the worker helper focused only on thread ownership and shutdown coordination.
- Reference direction from mature players:
  - dedicated long-running workers stay explicit
  - policy and fallback stay outside the thread primitive

## Non-goals
- Do not merge `ThreadSafeQueue`, `FrameQueue`, and `TaskQueue` in this round.
- Do not refactor renderer threads in `display` / `OpenGL` / `D3D11` in this round.
- Do not change decoder scheduling or audio master policy.
