# PLAYERCORE Day92: VK030 Windows Vulkan gate contract-canary Step Summary observability

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-029` added deterministic Windows Vulkan gate contract canary, but canary outcome was only visible in logs/artifacts.
- CI reviewer could not quickly see canary status from GitHub Step Summary panel.

## 2. Gap Snapshot
- Workflow had Step Summary section for main Vulkan gate (`VK-027`) only.
- Contract-canary summary file (`windows-vulkan-gate-contract-canary-summary.env`) was not rendered into Step Summary.

## 3. Solution Direction
- Extend Windows gate workflow step:
  - parse `logs/windows-vulkan-gate-contract-canary-summary.env`
  - append compact Markdown section:
    - `Windows Vulkan Gate Contract Canary`
  - include key fields:
    - `result`
    - `expected_gate_exit_code`
    - `actual_gate_exit_code`
    - `gate_summary_file_present`
    - `gate_result`
    - `gate_failure_reason`
    - `gate_diag_contract_valid`
    - `validation_failure_reason`
  - keep fallback message when canary summary file is missing
- Extend canary summary payload with `gate_summary_file_present` for clearer diagnosis.

## 4. DoD
- Step Summary includes contract-canary section when summary env exists.
- Missing canary summary file has explicit fallback Step Summary message.
- Canary fail behavior remains deterministic (`$vulkanCanaryExitCode` still enforced).
- Local validation confirms canary summary fields and preview rendering.
- Docs/records/index chain synchronized.

## 5. Outcome
- CI page now provides at-a-glance visibility for both:
  - real Windows Vulkan gate
  - expected-fail contract canary
- Contract drift triage becomes faster without downloading artifacts first.

## 6. Remaining
- Strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
