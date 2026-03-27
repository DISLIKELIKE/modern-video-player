# Cross-platform Vulkan Windows strict-unavailable expected-fail canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail canary for strict policy branch when Vulkan is unavailable.

## 2. Non-goals
- Do not change strict/optional policy logic in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback contract required keys.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is contract-valid, platform-supported, but unavailable (`compiled_in=false`, `runtime_available=false`).
  - invokes gate with `-RequireVulkanAvailable`.
- Validation contract:
  - gate exit code `2`
  - summary file exists
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-false`
- Output contract:
  - machine-readable `windows-vulkan-strict-unavailable-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run strict-unavailable canary after PASS canary.
  - capture `$vulkanStrictUnavailableCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Strict Unavailable Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Existing diagnostics/playback/PASS canaries regression.
- New strict-unavailable canary command.
- Static scan for workflow/script wiring.
