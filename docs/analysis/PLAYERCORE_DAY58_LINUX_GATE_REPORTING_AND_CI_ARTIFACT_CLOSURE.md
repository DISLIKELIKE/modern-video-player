# PLAYERCORE Day58: Linux gate reporting and CI artifact closure

Date: 2026-03-26  
Status: Done (implementation landed, Linux runtime PASS evidence still pending)

## 1. Problem
- Linux gate already had strict optional checks (`CP-507` / `CP-508`) and fixture generation.
- CI logs were still mostly stdout-only, making post-run evidence collection less deterministic.
- No machine-readable gate summary file was produced by `tools/run_linux_mvp_checks.sh`.

## 2. Root Cause
1. `run_linux_mvp_checks.sh` only printed check status to console.
2. CI Linux job did not capture Linux gate output into dedicated artifacts.
3. Structured pass/fail signals for each sub-check were unavailable as a reusable file.

## 3. Solution
- Extended `tools/run_linux_mvp_checks.sh` with report support:
  - arg #10 / env `MVP_LINUX_GATE_REPORT_FILE`
  - machine-readable output lines (`gate.*`, `check.<id>.*`)
  - explicit `gate.result=PASS|FAIL` and fail-reason capture
- Updated Linux CI lane:
  - `set -euo pipefail`
  - gate stdout/stderr piped to `logs/linux-mvp-gate.log`
  - structured summary written to `logs/linux-mvp-gate-summary.env`
  - upload `logs/*.env` artifacts in addition to `logs/*.log`

## 4. Outcome
- Linux gate now provides two evidence artifacts:
  - human-readable execution log
  - machine-readable summary for CI/review automation

## 5. Remaining Gap
- Runtime PASS proof still requires Linux host/CI execution result; local Windows host can only validate syntax and non-Linux dispatch behavior.
