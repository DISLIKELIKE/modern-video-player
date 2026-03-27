# PLAYERCORE Day101: VK039 Windows Vulkan playback-plan-reason semantic expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-036`, `VK-037`, `VK-038` already cover playback semantic failures:
  - `selected-renderer-not-vulkan`
  - `renderer-backend-not-vulkan`
  - `candidates-missing-vulkan`
- Another playback semantic branch still lacks deterministic CI coverage:
  - `playback_failure_detail=plan-reason-not-renderer-override-env`.

## 2. Gap Snapshot
- Current canary matrix does not lock down regressions where playback contract remains valid but plan reason no longer equals `renderer-override-env`.
- This is an independent failure path in `tools/run_windows_vulkan_checks.ps1` and should be protected separately.

## 3. Solution Direction
- Add deterministic canary script:
  - `tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1`
- Mock executable behavior:
  - diagnostics emits PASS and Vulkan available.
  - playback emits contract-valid output with:
    - `startup_selected_renderer=Vulkan`
    - `renderer_backend=Vulkan`
    - `startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL`
    - `startup_renderer_plan_reason=strategy-default` (intentional mismatch)
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=plan-reason-not-renderer-override-env`
- Integrate canary into Windows workflow with Step Summary and fail-fast.

## 4. DoD
- New playback-plan-reason semantic canary script exists with machine-readable output.
- Workflow runs this canary and fails on non-zero.
- Step Summary exposes this canary result table.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round only hardens deterministic semantic-branch coverage; it does not validate real Vulkan runtime playback.
- Real strict PASS proof still depends on Vulkan-ready Windows runner/workstation.
