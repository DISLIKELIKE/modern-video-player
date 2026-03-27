# Cross-platform Vulkan Windows playback-contract expected-fail canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail canary for Windows Vulkan playback-contract branch.

## 2. Non-goals
- Do not change `run_windows_vulkan_checks.ps1` playback contract logic.
- Do not change strict/optional policy behavior.
- Do not change Linux lane behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_playback_contract_canary.ps1`
- Script model:
  - create temporary mock `.cmd` executable in `logs/`.
  - `--vulkan-diagnostics` path emits valid contract fields with runtime available.
  - `--performance-log-check` path intentionally omits required keys to trigger playback-contract-broken.
- Validation contract:
  - gate exit code `2`
  - `result=FAIL`
  - `failure_reason=vulkan-playback-contract-broken`
  - `playback_contract_valid=false`
  - required missing keys are present in reported missing-field list.
- Output contract:
  - machine-readable `windows-vulkan-playback-contract-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run playback-contract canary script.
  - capture `$vulkanPlaybackCanaryExitCode`.
  - fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Playback Contract Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command (sanity regression).
- Playback-contract canary command.
- Static scan for workflow/script wiring.
