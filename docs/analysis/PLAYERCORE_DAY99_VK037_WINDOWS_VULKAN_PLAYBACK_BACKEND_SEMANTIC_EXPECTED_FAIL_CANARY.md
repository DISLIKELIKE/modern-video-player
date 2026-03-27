# PLAYERCORE Day99: VK037 Windows Vulkan playback-backend semantic expected-fail canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-036` already covers playback semantic failure branch where `startup_selected_renderer` is not Vulkan.
- Another semantic branch still lacks deterministic CI coverage: `renderer_backend` not Vulkan (`playback_failure_detail=renderer-backend-not-vulkan`).

## 2. Gap Snapshot
- Existing canary matrix validates:
  - diagnostics/playback contract failures
  - strict PASS
  - strict unavailable fail
  - optional unavailable skip
  - unsupported platform fail
  - selected-renderer semantic fail
- No deterministic branch coverage exists for backend semantic mismatch (`renderer_backend != Vulkan` with contract-valid payload).

## 3. Solution Direction
- Add deterministic canary script:
  - `tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1`
- Mock executable behavior:
  - diagnostics emits PASS and Vulkan available.
  - playback emits contract-valid output with:
    - `startup_selected_renderer=Vulkan`
    - `renderer_backend=OpenGL` (intentional mismatch)
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
  - `windows-vulkan-check.playback_contract_valid=true`
  - `windows-vulkan-check.playback_failure_detail=renderer-backend-not-vulkan`
- Integrate canary into Windows workflow with Step Summary and fail-fast.

## 4. DoD
- New playback-backend semantic canary script exists with machine-readable output.
- Workflow runs this canary and fails on non-zero.
- Step Summary exposes this canary result table.
- Local build + baseline gate + full canary matrix pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round only hardens deterministic semantic-branch coverage; it does not validate real Vulkan runtime playback.
- Real strict PASS proof still depends on Vulkan-ready Windows runner/workstation.
