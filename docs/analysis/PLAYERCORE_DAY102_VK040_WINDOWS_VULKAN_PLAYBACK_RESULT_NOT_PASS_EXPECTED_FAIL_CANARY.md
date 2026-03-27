# PLAYERCORE Day102: VK040 Windows Vulkan playback-result-not-pass expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-036`, `VK-037`, `VK-038`, `VK-039` already cover playback semantic failures:
  - `selected-renderer-not-vulkan`
  - `renderer-backend-not-vulkan`
  - `candidates-missing-vulkan`
  - `plan-reason-not-renderer-override-env`
- Another playback failure branch still lacks deterministic CI coverage:
  - `playback_failure_detail=result-not-pass`.

## 2. Gap Snapshot
- Current canary matrix does not lock down regressions where playback contract remains valid but `performance-log-check.result` is not `PASS`.
- This branch is evaluated before downstream semantic checks in `tools/run_windows_vulkan_checks.ps1`, so missing coverage can hide ordering or behavior regressions.

## 3. Solution Direction
- Add deterministic canary script:
  - `tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1`
- Mock executable behavior:
  - diagnostics emits PASS and Vulkan available.
  - playback emits contract-valid output with:
    - `performance-log-check.result=FAIL` (intentional)
    - `startup_selected_renderer=Vulkan`
    - `renderer_backend=Vulkan`
    - `startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL`
    - `startup_renderer_plan_reason=renderer-override-env`
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=result-not-pass`
- Integrate canary into Windows workflow with Step Summary and fail-fast.

## 4. DoD
- New playback-result-not-pass expected-fail canary script exists with machine-readable output.
- Workflow runs this canary and fails on non-zero.
- Step Summary exposes this canary result table.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round only hardens deterministic branch coverage; it does not prove real Vulkan runtime playback PASS on runner hardware.
- Real strict PASS proof still depends on Vulkan-ready Windows runner/workstation.
