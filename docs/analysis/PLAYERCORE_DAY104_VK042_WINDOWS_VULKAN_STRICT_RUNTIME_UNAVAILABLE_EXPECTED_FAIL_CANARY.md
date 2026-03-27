# PLAYERCORE Day104: VK042 Windows Vulkan strict-runtime-unavailable expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-033` already covers strict unavailable expected-fail behavior when availability detail is `compiled-in-false`.
- Another strict unavailable detail branch still lacks deterministic CI coverage:
  - `windows-vulkan-check.vulkan_availability_failure_detail=runtime-unavailable`.

## 2. Gap Snapshot
- Current strict canary matrix does not lock down regressions where Vulkan is compiled-in but runtime probe is unavailable.
- This branch is independent from `compiled-in-false` and should be protected separately to avoid detail-classification drift.

## 3. Solution Direction
- Add deterministic canary script:
  - `tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1`
- Mock executable behavior:
  - diagnostics emits contract-valid output with:
    - `supported_platform=true`
    - `compiled_in=true`
    - `runtime_available=false`
    - `result=FAIL`
  - strict mode enabled via gate CLI (`-RequireVulkanAvailable`).
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.vulkan_availability_failure_detail=runtime-unavailable`
  - `windows-vulkan-check.playback_check_executed=false`
- Integrate canary into Windows workflow with Step Summary and fail-fast.

## 4. DoD
- New strict-runtime-unavailable expected-fail canary script exists with machine-readable output.
- Workflow runs this canary and fails on non-zero.
- Step Summary exposes this canary result table.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round hardens strict availability-detail branch coverage only; it does not prove real Vulkan runtime playback PASS.
- Real strict PASS proof still depends on Vulkan-ready Windows runner/workstation.
