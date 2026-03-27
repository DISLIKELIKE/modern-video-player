# PLAYERCORE Day90: VK028 Windows Vulkan gate exit-code propagation hardening

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate command in CI is executed through a PowerShell pipeline (`... | Tee-Object`).
- In PowerShell, non-zero native-command exit code is stored in `$LASTEXITCODE`, but the step does not fail automatically.
- Result: `run_windows_vulkan_checks.ps1` can return `FAIL` (`exit 2`) while CI step may still continue and pass.

## 2. Gap Snapshot
- Workflow lacked explicit exit-code propagation check after Vulkan gate command.
- Step Summary could still be generated, but gate blocking semantics were not guaranteed.

## 3. Solution Direction
- In `.github/workflows/cross-platform-gate.yml` Windows gate step:
  - capture `$LASTEXITCODE` right after Vulkan gate command pipeline.
  - keep Step Summary generation logic unchanged.
  - after Step Summary write, force fail-fast with `throw` when captured exit code is non-zero.

## 4. DoD
- Windows Vulkan gate non-zero exit code is propagated and blocks CI step.
- Step Summary rendering from `windows-vulkan-gate-summary.env` remains intact.
- Baseline optional path on current host remains deterministic (`SKIPPED`).
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan gate now preserves original pass/fail semantics even when output is piped to `Tee-Object`.
- CI can no longer silently pass after Vulkan strict/check failure.

## 6. Remaining
- Strict PASS still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
