# Cross-platform Linux gate strict optional checks design (2026-03-26)

## 1. Goal
- Ensure Linux gate includes deterministic coverage for subtitle backlog probes:
  - `CP-507`: libass shaping/layout probe
  - `CP-508`: embedded subtitle live packet probe
- Keep default local usage lightweight while enabling CI strictness.

## 2. Constraints
- Linux gate script must stay runnable as a standalone shell entry.
- Existing invocations with first six args must remain backward compatible.
- CI should not rely on manually pre-generated media artifacts.

## 3. Script contract update
- Existing args kept:
  1. executable path
  2. probe media
  3. sample ms
  4. ASS subtitle probe file
  5. embedded subtitle media file
  6. max packet count
- New args:
  7. `REQUIRE_OPTIONAL_CHECKS` (`0|1`, default `0`, env fallback `MVP_REQUIRE_OPTIONAL_CHECKS`)
  8. `EMBEDDED_SUBTITLE_BASE_MEDIA_FILE` (default probe media)
  9. `EMBEDDED_ASS_SUBTITLE_FILE` (default ASS transition subtitle sample)

## 4. Fixture strategy
- If embedded subtitle media file is missing:
  - Generate it via `ffmpeg` by muxing base media + ASS subtitle.
  - Output uses same path passed in arg #5.
- If generation fails:
  - Non-strict mode: keep compatibility and allow skip.
  - Strict mode: fail fast before running checks.

## 5. CI wiring
- Linux job installs `ffmpeg`.
- Linux gate uses strict mode (`arg #7 = 1`) with explicit fixture paths.
- Outcome: `CP-507` and `CP-508` are treated as required CI checks.

## 6. Risk and mitigation
- Risk: Linux gate can fail due to fixture generation issues (media/subtitle missing, ffmpeg unavailable).
- Mitigation:
  - Provide explicit and stable sample paths in CI call.
  - Install `ffmpeg` as part of Linux dependencies.
  - Keep clear error output for missing inputs.
