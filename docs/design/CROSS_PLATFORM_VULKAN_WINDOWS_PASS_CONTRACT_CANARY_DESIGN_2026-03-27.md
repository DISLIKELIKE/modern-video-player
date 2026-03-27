# Cross-platform Vulkan Windows PASS-contract canary design (2026-03-27)

## 1. Scope
- Add deterministic PASS-contract canary coverage for Windows Vulkan gate.

## 2. Non-goals
- Do not change strict/optional policy behavior in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics or playback required-field contracts.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - `--vulkan-diagnostics` branch emits complete PASS contract fields.
  - `--performance-log-check` branch emits complete PASS playback contract fields with Vulkan selected.
  - invokes gate script with `-RequireVulkanAvailable` to enforce strict path.
- Validation contract:
  - gate exit code `0`
  - gate summary exists
  - `windows-vulkan-check.result=PASS`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=none`
  - `windows-vulkan-check.failure_reason` empty
- Output contract:
  - machine-readable `windows-vulkan-pass-contract-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run PASS-contract canary script after existing expected-fail canaries.
  - capture `$vulkanPassCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate PASS Contract Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Diagnostics expected-fail canary command.
- Playback expected-fail canary command.
- PASS-contract canary command.
- Static scan for workflow/script wiring.
