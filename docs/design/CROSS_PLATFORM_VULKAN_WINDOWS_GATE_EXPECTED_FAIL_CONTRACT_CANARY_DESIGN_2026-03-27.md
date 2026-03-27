# Cross-platform Vulkan Windows gate expected-fail contract canary design (2026-03-27)

## 1. Scope
- Add deterministic expected-fail contract canary for Windows Vulkan gate in CI.

## 2. Non-goals
- Do not change strict/optional gate policy semantics.
- Do not alter Vulkan runtime/probe decision logic.
- Do not change Linux gate behavior.

## 3. Design
- New script:
  - `tools/run_windows_vulkan_gate_contract_canary.ps1`
- Execution model:
  - run `run_windows_vulkan_checks.ps1` as child process with:
    - `ExecutablePath = %WINDIR%\System32\cmd.exe`
    - normal probe file path
  - this yields diagnostics-contract-broken path deterministically.
- Assertion set:
  - child gate exit code == `2`
  - `windows-vulkan-check.result == FAIL`
  - `windows-vulkan-check.failure_reason == vulkan-diagnostics-contract-broken`
  - `windows-vulkan-check.diag_contract_valid == false`
- Output contract:
  - publish `windows-vulkan-contract-canary.*` key-value lines.
  - write optional canary summary env file for CI artifacts.

## 4. Workflow Integration
- In `.github/workflows/cross-platform-gate.yml` Windows gate step:
  - execute canary script with summary/gate-summary output paths under `logs/`.
  - capture `$LASTEXITCODE` after canary pipeline.
  - fail-fast with `throw` if canary exit is non-zero.

## 5. Validation Strategy
- Build `Release` target.
- Run baseline Windows Vulkan gate (expected `SKIPPED` on current host).
- Run canary script and assert:
  - `windows-vulkan-contract-canary.result=PASS`
  - expected gate-failure contract fields are present.
