# PLAYERCORE Day95: VK033 Windows Vulkan strict-unavailable expected-fail canary

Date: 2026-03-27  
Status: In Progress

## 1. Problem
- Windows Vulkan gate now has deterministic canary coverage for:
  - diagnostics contract expected-fail (`VK-029`)
  - playback contract expected-fail (`VK-031`)
  - strict-path PASS contract (`VK-032`)
- The strict-policy availability branch (`vulkan-not-available-in-strict-mode`) still lacks deterministic canary coverage.

## 2. Gap Snapshot
- If strict policy behavior regresses, CI may not catch it deterministically because real runner Vulkan availability is variable.
- We need stable coverage that validates strict-mode fail semantics when diagnostics contract is valid but Vulkan availability probe fails.

## 3. Solution Direction
- Add deterministic strict-unavailable canary script:
  - `tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1`
- Script creates a mock executable that emits valid diagnostics contract with:
  - `supported_platform=true`
  - `compiled_in=false`
  - `runtime_available=false`
  - `result=FAIL`
- Canary runs gate with `-RequireVulkanAvailable` and validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-false`
- Integrate canary into Windows workflow with fail-fast and Step Summary section.

## 4. DoD
- New strict-unavailable canary script exists and outputs machine-readable fields.
- Workflow runs strict-unavailable canary and fails on non-zero.
- Step Summary exposes strict-unavailable canary key fields.
- Local build + baseline gate + all Vulkan canaries pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round validates strict policy contract semantics only; it does not prove real Vulkan runtime readiness on runner.
- Strict PASS proof for real Vulkan playback remains dependent on Vulkan-ready runner/workstation.
