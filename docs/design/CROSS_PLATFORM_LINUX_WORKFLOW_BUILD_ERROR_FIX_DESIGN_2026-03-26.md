# Cross-platform Linux workflow build error fix design (2026-03-26)

## 1. Goal
- Remove Linux CI compile blockers with minimal behavior change and no Windows-path regression.

## 2. Design Constraints
- Keep Windows code path unchanged.
- Limit Linux-side changes to type safety and helper visibility.
- Avoid large-scale refactor of existing `#if` structure in this round.

## 3. Design Decisions
- `libass_probe` timestamp conversion:
  - Convert `ASS_Event` timing fields through explicit clamp-to-int helper to avoid mixed-type template deduction failures.
- `opengl_video_renderer` Linux helper visibility:
  - Add a dedicated `#if !defined(_WIN32)` block outside the Windows-only helper section.
  - Provide Linux-visible definitions for the cross-platform symbols consumed by renderer class logic.

## 4. Non-goals
- No changes to user-facing CLI contract.
- No changes to rendering policy semantics.
- No changes to macOS scope.
