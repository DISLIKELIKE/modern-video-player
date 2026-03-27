# PLAYERCORE Day98: VK036 Windows Vulkan playback-semantic expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate now has deterministic canaries for contract, strict, optional-skip, and unsupported-platform branches.
- Playback semantic failure branch (`vulkan-playback-check-failed`) still lacks deterministic canary coverage.

## 2. Gap Snapshot
- A regression that keeps playback contract valid but violates semantic selection rules may not be detected deterministically.
- Existing playback canary (`VK-031`) covers contract-missing path only, not semantic-failure path.

## 3. Solution Direction
- Add deterministic playback-semantic expected-fail canary script:
  - `tools/run_windows_vulkan_gate_playback_semantic_canary.ps1`
- Script creates mock executable that emits:
  - valid diagnostics PASS contract (`compiled_in/runtime_available=true`)
  - valid playback contract with intentional semantic mismatch:
    - `startup_selected_renderer=D3D11` (not Vulkan)
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=selected-renderer-not-vulkan`
- Integrate canary into Windows workflow with fail-fast and Step Summary.

## 4. DoD
- New playback-semantic canary script exists with machine-readable output.
- Workflow runs playback-semantic canary and fails on non-zero.
- Step Summary exposes playback-semantic canary status.
- Local build + full Vulkan canary set pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round validates playback semantic contract logic only; it does not validate real Vulkan runtime playback.
- Real strict PASS proof remains dependent on Vulkan-ready Windows runner/workstation.
