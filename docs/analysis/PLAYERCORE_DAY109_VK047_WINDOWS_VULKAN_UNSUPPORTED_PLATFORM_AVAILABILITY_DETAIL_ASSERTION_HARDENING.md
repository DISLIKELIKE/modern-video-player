# PLAYERCORE Day109: VK047 Windows Vulkan unsupported-platform availability detail assertion hardening

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- `unsupported-platform` expected-fail canary already asserted gate result/failure reason/playback flag.
- But it did not assert `windows-vulkan-check.vulkan_availability_failure_detail`, so unsupported-platform detail-classification drift could be missed.

## 2. Gap Snapshot
- Availability-detail classification is now a core machine-readable contract across strict/optional/contract canaries.
- Unsupported-platform canary remained the last availability-failure branch without detail-level assertion parity.

## 3. Solution Direction
- Hardened `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - assert it equals `unsupported-platform`
  - export summary key:
    - `windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail`
- Updated workflow Step Summary table for unsupported-platform canary to include this key.

## 4. DoD
- Unsupported-platform canary fails if availability-detail no longer reports `unsupported-platform`.
- CI Step Summary explicitly exposes unsupported-platform availability detail.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round is assertion/observability hardening only; gate policy and playback behavior are unchanged.
