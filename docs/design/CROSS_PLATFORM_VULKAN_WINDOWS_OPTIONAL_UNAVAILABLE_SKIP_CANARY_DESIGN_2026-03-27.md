# Cross-platform Vulkan Windows optional-unavailable skip canary design (2026-03-27)

## 1. Scope
- Add deterministic canary for optional-mode unavailable path where gate should skip.

## 2. Non-goals
- Do not change strict/optional policy logic in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback contract required keys.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_optional_skip_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is contract-valid and platform-supported, but unavailable (`compiled_in=false`, `runtime_available=false`).
  - invokes gate without strict override.
- Validation contract:
  - gate exit code `0`
  - gate summary exists
  - `windows-vulkan-check.result=SKIPPED`
  - `windows-vulkan-check.mode=optional`
  - `windows-vulkan-check.skip_reason=vulkan-not-available`
  - `windows-vulkan-check.failure_reason` empty
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-false`
- Output contract:
  - machine-readable `windows-vulkan-optional-skip-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run optional-skip canary after strict-unavailable canary.
  - capture `$vulkanOptionalSkipCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Optional Skip Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Existing diagnostics/playback/PASS/strict-unavailable canaries regression.
- New optional-skip canary command.
- Static scan for workflow/script wiring.
