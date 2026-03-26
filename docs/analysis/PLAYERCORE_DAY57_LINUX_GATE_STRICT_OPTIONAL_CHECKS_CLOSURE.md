# PLAYERCORE Day57: Linux gate strict optional checks closure (`CP-507` / `CP-508`)

Date: 2026-03-26  
Status: Done (implementation landed, Linux host runtime evidence still pending)

## 1. Background
- The master tasklist already marked Linux scope tasks as done (`CP-001 ~ CP-905` within current scope).
- Remaining delivery gap is runtime evidence closure on Linux host/CI.
- `tools/run_linux_mvp_checks.sh` had optional checks for:
  - `CP-507` (`--libass-shaping-check`)
  - `CP-508` (`--embedded-subtitle-live-packet-check`)
- In CI, `CP-508` could be skipped if `build/tmp/embedded-ass-validation.mkv` was absent.

## 2. Problem
- Linux gate parity existed in command shape, but coverage determinism was weak:
  - `CP-508` depended on a pre-existing fixture file that is not versioned.
  - When fixture was missing, check was skipped instead of being enforced in CI.

## 3. Root Cause
1. Linux gate script accepted an optional embedded subtitle media path but had no fixture generation fallback.
2. CI Linux dependency setup did not ensure `ffmpeg` binary availability for fixture generation.
3. No strict mode existed to fail gate when optional checks were skipped.

## 4. Landed Changes
- `tools/run_linux_mvp_checks.sh`:
  - Added `ensure_embedded_ass_sample(...)` to generate CP-508 fixture through `ffmpeg` when missing.
  - Added `REQUIRE_OPTIONAL_CHECKS` (arg #7, also supports env `MVP_REQUIRE_OPTIONAL_CHECKS`) to force `CP-507`/`CP-508` presence.
  - Added extra args for generation inputs:
    - arg #8 `EMBEDDED_SUBTITLE_BASE_MEDIA_FILE`
    - arg #9 `EMBEDDED_ASS_SUBTITLE_FILE`
- `.github/workflows/cross-platform-gate.yml`:
  - Linux dependency set now installs `ffmpeg`.
  - Linux gate call now runs strict optional-check mode with explicit fixture inputs.

## 5. Expected Outcome
- CI Linux lane can deterministically execute both subtitle backlog checks instead of silently skipping `CP-508`.
- Local non-Linux development remains compatible because strict mode is opt-in.

## 6. Remaining Gap
- This workstation is Windows-only (no WSL/Linux runtime), so real Linux execution evidence still requires:
  - GitHub Actions Linux run or
  - external Linux host run.
