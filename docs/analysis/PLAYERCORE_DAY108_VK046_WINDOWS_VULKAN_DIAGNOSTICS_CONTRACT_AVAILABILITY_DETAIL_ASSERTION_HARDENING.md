# PLAYERCORE Day108: VK046 Windows Vulkan diagnostics-contract availability detail assertion hardening

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- Diagnostics-contract canary (`VK-029`) already asserts `failure_reason=vulkan-diagnostics-contract-broken` and `diag_contract_valid=false`.
- But it did not assert `windows-vulkan-check.vulkan_availability_failure_detail`, so `diag-contract-missing-required-fields` classification drift could be missed.

## 2. Gap Snapshot
- Availability-detail classification is a core machine-readable contract used by CI diagnostics and triage.
- Missing assertion in contract canary created a blind spot compared to newer strict/optional canaries that already validate this field.

## 3. Solution Direction
- Hardened `tools/run_windows_vulkan_gate_contract_canary.ps1`:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - assert it equals `diag-contract-missing-required-fields`
  - export new summary key:
    - `windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail`
- Updated workflow Step Summary table for contract canary to include the new key.

## 4. DoD
- Contract canary fails if diagnostics-contract branch no longer reports `diag-contract-missing-required-fields`.
- CI Step Summary explicitly exposes contract-branch availability detail value.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round is assertion/observability hardening only; runtime Vulkan playback behavior is unchanged.
