# PLAYERCORE Day107: VK045 Windows Vulkan strict-compiled-in-disabled expected-fail canary

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- `VK-044` 已覆盖 strict unavailable 的 `diag-result-not-pass` 分支。
- strict unavailable 分支里仍有一个细分来源没有确定性 canary：
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled`

## 2. Gap Snapshot
- `run_windows_vulkan_checks.ps1` 已按 `diag_dependency_source=disabled` 分类出 `compiled-in-disabled`，但 CI 尚无专用 expected-fail canary 锁定该行为。
- 如果后续分支顺序或依赖来源判定回归，这条分支可能漂移且不被现有 canary 及时发现。

## 3. Solution Direction
- Added deterministic canary script:
  - `tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1`
- Mock executable behavior:
  - diagnostics emits contract-valid output with:
    - `supported_platform=true`
    - `compiled_in=false`
    - `runtime_available=false`
    - `dependency_source=disabled`
    - `result=FAIL`
  - strict mode enabled via gate CLI (`-RequireVulkanAvailable`).
  - playback path should stay unexecuted.
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.mode=strict`
  - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
  - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.diag_exit_code=0`
  - `windows-vulkan-check.diag_dependency_source=disabled`
- Integrated canary into Windows workflow with Step Summary and fail-fast.

## 4. DoD
- New strict-compiled-in-disabled expected-fail canary exists with machine-readable output.
- Workflow runs this canary and fails on non-zero.
- Step Summary exposes strict-compiled-in-disabled table.
- Local build + baseline gate + canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round hardens strict availability-detail branch coverage only; runtime strict PASS proof still depends on Vulkan-ready Windows runner/workstation.
