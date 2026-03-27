# PLAYERCORE Day87: VK025 Windows Vulkan diagnostics contract validation

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate relied on default-value parsing for `--vulkan-diagnostics`.
- If diagnostics output contract regressed (missing required keys), gate could misclassify it as generic "not available" instead of diagnostics contract break.

## 2. Gap Snapshot
- No explicit required-key validation existed for diagnostics output.
- No machine-readable fields exposed diagnostics contract validity and missing-key list.

## 3. Solution Direction
- Add required-key validation in `run_windows_vulkan_checks.ps1` for:
  - `vulkan-diagnostics.platform`
  - `vulkan-diagnostics.supported_platform`
  - `vulkan-diagnostics.compiled_in`
  - `vulkan-diagnostics.runtime_available`
  - `vulkan-diagnostics.result`
- Add summary fields:
  - `windows-vulkan-check.diag_contract_valid`
  - `windows-vulkan-check.diag_missing_required_fields`
- If contract is broken, fail fast with:
  - `failure_reason=vulkan-diagnostics-contract-broken`
  - `vulkan_availability_failure_detail=diag-contract-missing-required-fields`

## 4. DoD
- Diagnostics required keys are validated before availability/strict decisions.
- Contract-broken path is machine-readable and deterministic.
- Existing strict/optional behavior remains unchanged for valid diagnostics output.
- Local validation includes both normal path and contract-broken simulation path.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan gate now distinguishes "Vulkan unavailable" from "diagnostics contract broken".
- CI triage can directly identify missing diagnostics fields from summary artifact.

## 6. Remaining
- Strict PASS still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
