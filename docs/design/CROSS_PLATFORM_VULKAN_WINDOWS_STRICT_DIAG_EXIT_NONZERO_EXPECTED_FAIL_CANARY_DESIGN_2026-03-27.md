# Cross-platform Vulkan Windows strict-diag-exit-nonzero expected-fail canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail canary for strict-mode availability detail `diag-exit-nonzero`.

## 2. Non-goals
- Do not change strict/optional policy behavior in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback required-key contracts.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is contract-valid, compiled-in=true, runtime_available=true.
  - diagnostics process exits non-zero intentionally.
  - invokes gate in strict mode (`-RequireVulkanAvailable`).
- Validation contract:
  - gate exit code `2`
  - summary file exists
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.vulkan_availability_failure_detail=diag-exit-nonzero`
  - `windows-vulkan-check.playback_check_executed=false`
- Output contract:
  - machine-readable `windows-vulkan-strict-diag-exit-nonzero-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run strict-diag-exit-nonzero canary after strict-runtime-unavailable canary.
  - capture `$vulkanStrictDiagExitNonzeroCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Existing canaries regression.
- New strict-diag-exit-nonzero canary command.
- Static scan for workflow/script wiring.
