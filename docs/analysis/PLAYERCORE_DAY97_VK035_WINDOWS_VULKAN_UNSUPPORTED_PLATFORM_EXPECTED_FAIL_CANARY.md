# PLAYERCORE Day97: VK035 Windows Vulkan unsupported-platform expected-fail canary

Date: 2026-03-27  
Status: In Progress

## 1. Problem
- Windows Vulkan gate now has deterministic canaries for:
  - diagnostics-contract expected-fail (`VK-029`)
  - playback-contract expected-fail (`VK-031`)
  - strict PASS (`VK-032`)
  - strict-unavailable expected-fail (`VK-033`)
  - optional-unavailable skip (`VK-034`)
- Unsupported-platform branch (`failure_reason=unsupported-platform`) still lacks deterministic canary coverage.

## 2. Gap Snapshot
- If platform-support gating regresses, CI may not catch it deterministically.
- Regular Windows runs cannot naturally exercise unsupported-platform branch.

## 3. Solution Direction
- Add deterministic unsupported-platform canary script:
  - `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
- Script creates mock executable with contract-valid diagnostics and explicit unsupported-platform state:
  - `platform=Linux`
  - `supported_platform=false`
  - `compiled_in=false`
  - `runtime_available=false`
  - `result=FAIL`
- Canary validates:
  - gate exit code `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=unsupported-platform`
  - `windows-vulkan-check.playback_check_executed=false`
  - `windows-vulkan-check.skip_reason` empty
- Integrate canary into Windows workflow with fail-fast and Step Summary.

## 4. DoD
- New unsupported-platform canary script exists with machine-readable output.
- Workflow runs unsupported-platform canary and fails on non-zero.
- Step Summary exposes unsupported-platform canary status.
- Local build + full Vulkan canary set pass.
- Docs/records/index chain synchronized.

## 5. Risk Notes
- This round validates gate contract semantics only, not real runtime portability.
- Real strict PASS proof remains dependent on Vulkan-ready Windows runner/workstation.
