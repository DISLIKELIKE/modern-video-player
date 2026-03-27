# PLAYERCORE Day86: VK024 Windows Vulkan availability failure detail classification

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate strict failure was still summarized with a generic reason:
  - `vulkan-not-available-in-strict-mode`
- Even after `VK-023` added runtime probe detail, CI triage still lacked a dedicated machine-readable classification of availability-stage failure cause.

## 2. Gap Snapshot
- No explicit field indicated whether failure came from:
  - `compiled_in=false`
  - `runtime_available=false`
  - diagnostics exit/result mismatch
  - unsupported platform

## 3. Solution Direction
- Extend `run_windows_vulkan_checks.ps1` with availability-stage observability fields:
  - `windows-vulkan-check.vulkan_availability_probe_passed`
  - `windows-vulkan-check.vulkan_availability_failure_detail`
- Keep existing result/failure compatibility unchanged:
  - `result`, `failure_reason`, `skip_reason` semantics remain stable.

## 4. DoD
- New availability observability fields are emitted in all paths.
- Strict/optional behavior remains identical to `VK-022`.
- Local configure/build/gate matrix passes with expected outcomes.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan gate now reports actionable availability failure details.
- Current host classification is deterministic:
  - `vulkan_availability_failure_detail=compiled-in-disabled`
- CI debugging for strict failures is faster and less dependent on manual log scraping.

## 6. Remaining
- Strict PASS still depends on a Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
