# PLAYERCORE Day91: VK029 Windows Vulkan gate expected-fail contract canary

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-028` already fixed Windows gate fail propagation in CI.
- But CI still lacked a hardware-independent canary to continuously validate Vulkan gate failure contract behavior.

## 2. Gap Snapshot
- No deterministic CI check existed for:
  - expected gate failure exit code contract (`exit 2`)
  - machine-readable failure fields contract (`result=FAIL`, expected failure reason)
- Current host/runner Vulkan availability varies, so normal gate path alone cannot always exercise failure-contract branches.

## 3. Solution Direction
- Add a dedicated canary command:
  - `tools/run_windows_vulkan_gate_contract_canary.ps1`
- Canary intentionally runs `run_windows_vulkan_checks.ps1` against `cmd.exe` diagnostics path to trigger diagnostics-contract-broken branch.
- Canary asserts:
  - gate exit code must be `2`
  - `windows-vulkan-check.result=FAIL`
  - `windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken`
  - `windows-vulkan-check.diag_contract_valid=false`
- Wire canary into Windows CI gate with explicit exit-code propagation guard.

## 4. DoD
- New canary script exists and returns machine-readable `windows-vulkan-contract-canary.*` output.
- Windows workflow runs canary and fails if canary contract assertions fail.
- Local build and local canary execution pass.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan CI now has a deterministic contract canary independent of real Vulkan hardware/runtime availability.
- Gate failure semantics and failure-shape contracts are continuously guarded.

## 6. Remaining
- Strict PASS runtime proof still depends on Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
