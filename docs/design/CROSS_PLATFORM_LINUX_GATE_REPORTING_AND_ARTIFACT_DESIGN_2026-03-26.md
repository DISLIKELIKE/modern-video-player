# Cross-platform Linux gate reporting and artifact design (2026-03-26)

## 1. Goal
- Provide deterministic Linux gate evidence artifacts for CI and release review.
- Keep existing gate command behavior compatible while adding structured reporting.

## 2. Reporting contract
- New optional Linux gate input:
  - arg #10 `REPORT_FILE`
  - env fallback `MVP_LINUX_GATE_REPORT_FILE`
- Report format:
  - `gate.*` global metadata/status
  - `check.<check_id>.*` per-check status/exit/missing-pattern counters
- Failure path:
  - write `gate.result=FAIL`
  - write `gate.fail_reason=...`

## 3. Check ID strategy
- Use stable snake-case IDs (for example `cp507_libass_shaping_layout_probe`).
- Keep display titles unchanged for console readability.
- Benefit: easy parser logic for future CI dashboards.

## 4. CI artifact strategy
- Linux gate step enforces `pipefail` and tees output to:
  - `logs/linux-mvp-gate.log`
- Linux gate summary output path:
  - `logs/linux-mvp-gate-summary.env`
- Upload both log and env artifacts.

## 5. Compatibility
- Existing invocations without arg #10 continue to work.
- Report output is additive and does not change check semantics.
