# PLAYERCORE Day93: VK031 Windows Vulkan playback-contract expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Existing Windows Vulkan canary (`VK-029`) covered diagnostics-contract-broken branch only.
- Playback-contract-broken branch (`VK-026`) still lacked deterministic CI canary coverage.

## 2. Gap Snapshot
- No stable runner-independent check asserted:
  - `failure_reason=vulkan-playback-contract-broken`
  - `playback_contract_valid=false`
  - required missing playback fields are correctly reported.

## 3. Solution Direction
- Added deterministic playback-contract canary script:
  - `tools/run_windows_vulkan_gate_playback_contract_canary.ps1`
- Script creates a mock executable that:
  - returns valid `--vulkan-diagnostics` contract with `compiled_in/runtime_available=true` to enter playback stage.
  - returns intentionally incomplete `--performance-log-check` output to trigger playback-contract-broken path.
- Canary validates:
  - gate exit code = `2`
  - gate result = `FAIL`
  - failure reason = `vulkan-playback-contract-broken`
  - playback contract valid = `false`
  - missing-field list includes:
    - `performance-log-check.startup_selected_renderer`
    - `performance-log-check.renderer_backend`
    - `performance-log-check.startup_renderer_plan_reason`
- Wired canary into Windows CI gate with:
  - fail-fast exit propagation
  - Step Summary section (`Windows Vulkan Gate Playback Contract Canary`)

## 4. DoD
- New playback-contract canary script exists with machine-readable output.
- CI Windows gate runs playback-contract canary and fails on contract drift.
- Step Summary exposes playback-contract canary status.
- Local build + baseline gate + playback-contract canary validation pass.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan CI now has deterministic coverage for both contract branches:
  - diagnostics-contract-broken (`VK-029`)
  - playback-contract-broken (`VK-031`)
- Gate-contract regression detection no longer depends on real Vulkan hardware/runtime.

## 6. Remaining
- Strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
