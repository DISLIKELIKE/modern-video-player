# PLAYERCORE Day88: VK026 Windows Vulkan playback contract validation

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate already validated diagnostics contract (`VK-025`), but playback check output still relied on default-value parsing.
- If `--performance-log-check` misses required keys, failure could be reported as generic playback failure without contract-level diagnosis.

## 2. Gap Snapshot
- No required-key validation for playback output.
- No machine-readable fields for playback contract validity and missing required fields.

## 3. Solution Direction
- Add playback required-key validation in `run_windows_vulkan_checks.ps1` for:
  - `performance-log-check.result`
  - `performance-log-check.startup_selected_renderer`
  - `performance-log-check.renderer_backend`
  - `performance-log-check.startup_renderer_candidates`
  - `performance-log-check.startup_renderer_plan_reason`
- Add summary fields:
  - `windows-vulkan-check.playback_contract_valid`
  - `windows-vulkan-check.playback_missing_required_fields`
  - `windows-vulkan-check.playback_failure_detail`
- Add explicit contract-broken reason:
  - `failure_reason=vulkan-playback-contract-broken`

## 4. DoD
- Playback output contract is validated when playback check is executed.
- Contract-broken path is machine-readable and deterministic.
- Existing strict/optional behavior for non-playback paths remains unchanged.
- Local validation covers baseline, auto-policy matrix, and playback contract-broken simulation.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan gate can now distinguish:
  - playback contract broken
  - normal playback semantic failure
  - playback not executed (`n/a`)
- CI triage on Vulkan playback stage is more deterministic.

## 6. Remaining
- Strict PASS still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
