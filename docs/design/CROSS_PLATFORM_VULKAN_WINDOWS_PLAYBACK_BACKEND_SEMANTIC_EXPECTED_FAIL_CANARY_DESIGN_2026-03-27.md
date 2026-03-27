# Cross-platform Vulkan Windows playback-backend semantic expected-fail canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail canary for playback backend semantic mismatch branch.

## 2. Non-goals
- Do not change strict/optional policy behavior in `run_windows_vulkan_checks.ps1`.
- Do not change diagnostics/playback required-key contracts.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1`
- Script behavior:
  - creates temporary mock executable in `logs/`.
  - diagnostics output is fully PASS and available.
  - playback output is contract-valid but semantically invalid:
    - selected renderer remains Vulkan
    - backend is non-Vulkan (`OpenGL`)
  - invokes gate in default optional mode.
- Validation contract:
  - gate exit code `2`
  - summary file exists
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=renderer-backend-not-vulkan`
- Output contract:
  - machine-readable `windows-vulkan-playback-backend-semantic-canary.*`.

## 4. Workflow Integration
- In Windows gate step:
  - run playback-backend semantic canary after playback-semantic canary.
  - capture `$vulkanPlaybackBackendSemanticCanaryExitCode` and fail-fast on non-zero.
  - parse canary summary env and append Step Summary table:
    - `Windows Vulkan Gate Playback Backend Semantic Canary`.

## 5. Validation Strategy
- Release build.
- Baseline gate command.
- Existing canaries regression.
- New playback-backend semantic canary command.
- Static scan for workflow/script wiring.
