# Cross-platform Vulkan Windows strict-compiled-in-disabled expected-fail canary design (2026-03-28)

## 1. Scope
- Add deterministic expected-fail canary for strict-mode availability detail `compiled-in-disabled`.

## 2. Non-goals
- Do not change strict/optional policy behavior in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback required-key contracts.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is contract-valid, `compiled_in=false`, `runtime_available=false`.
  - diagnostics emits `dependency_source=disabled` to hit disabled-compile branch.
  - invokes gate in strict mode (`-RequireVulkanAvailable`).
- Validation contract:
  - gate exit code `2`
  - summary file exists
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.diag_exit_code=0`
  - `windows-vulkan-check.diag_dependency_source=disabled`
- Output contract:
  - machine-readable `windows-vulkan-strict-compiled-in-disabled-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run strict-compiled-in-disabled canary after strict-diag-result-not-pass canary.
  - capture `$vulkanStrictCompiledInDisabledCanaryExitCode` and fail-fast on non-zero.
  - parse summary env and append Step Summary table:
    - `Windows Vulkan Gate Strict Compiled-In-Disabled Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Full Windows Vulkan canary matrix regression.
- Static scan for workflow/script wiring.
