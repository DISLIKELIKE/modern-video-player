# Cross-platform Vulkan Windows unsupported-platform expected-fail canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail canary for unsupported-platform branch.

## 2. Non-goals
- Do not change strict/optional policy behavior in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback contract required keys.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is contract-valid but unsupported-platform (`supported_platform=false`).
  - invokes gate with default optional policy.
- Validation contract:
  - gate exit code `2`
  - summary file exists
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=unsupported-platform`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.skip_reason` empty
- Output contract:
  - machine-readable `windows-vulkan-unsupported-platform-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run unsupported-platform canary after optional-skip canary.
  - capture `$vulkanUnsupportedPlatformCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Unsupported Platform Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Existing canaries regression.
- New unsupported-platform canary command.
- Static scan for workflow/script wiring.
