# PLAYERCORE Day94: VK032 Windows Vulkan PASS-contract canary

Date: 2026-03-27  
Status: In Progress

## 1. Problem
- Windows Vulkan CI currently has deterministic canaries for expected-fail branches only:
  - diagnostics-contract-broken (`VK-029`)
  - playback-contract-broken (`VK-031`)
- PASS branch contract has no deterministic, runner-independent canary.

## 2. Gap Snapshot
- A regression that breaks PASS semantics (for example, contract drift or wrong gate decision logic) may survive if only expected-fail canaries are present.
- Real Vulkan availability on CI runners is variable, so normal gate runs cannot reliably prove PASS-path stability.

## 3. Solution Direction
- Add a deterministic PASS-contract canary script:
  - `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
- Script creates a mock executable that emits:
  - valid `--vulkan-diagnostics` contract with `compiled_in/runtime_available=true`
  - valid `--performance-log-check` contract with Vulkan selected backend and PASS result
- Canary invokes `run_windows_vulkan_checks.ps1` in strict mode and validates:
  - gate exit code = `0`
  - `windows-vulkan-check.result=PASS`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.failure_reason` is empty
- Integrate canary into Windows workflow with:
  - explicit exit-code propagation
  - dedicated Step Summary section

## 4. DoD
- New PASS-contract canary script exists and outputs machine-readable fields.
- Windows workflow executes PASS canary after existing canaries and fails fast on non-zero.
- Step Summary exposes PASS canary key outputs.
- Local validation passes:
  - Release build
  - baseline gate
  - diagnostics expected-fail canary
  - playback expected-fail canary
  - PASS-contract canary
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round validates gate contract semantics only; it does not prove real hardware Vulkan playback on runner.
- Strict PASS proof on real Vulkan runtime remains dependent on Vulkan-ready runner/workstation.
