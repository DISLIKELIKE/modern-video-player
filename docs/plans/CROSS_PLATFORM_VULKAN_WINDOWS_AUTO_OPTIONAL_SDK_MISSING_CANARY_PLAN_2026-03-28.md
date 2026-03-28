# Cross-platform Vulkan Windows auto optional sdk-missing canary plan (2026-03-28)

## Scope
- Deliver `VK-053`: add dedicated canary coverage for `auto` policy when SDK signal is missing but runtime probe signal is present.

## Implementation Planner
1. Freeze acceptance for sdk-missing downgrade branch (`sdk=0 + native_probe=1`).
Dependency: none.
2. Add dedicated canary script and summary contract.
Dependency: step 1.
3. Integrate canary into `tools/run_windows_ci_gate.ps1` and Step Summary.
Dependency: step 2.
4. Execute local validation on the new branch.
Dependency: step 3.
5. Sync docs/records/index chain.
Dependency: step 4.

## Acceptance
- New canary exits PASS with:
  - `gate_mode=optional`
  - `gate_strict_mode_effective=false`
  - `gate_strict_mode_auto_prerequisites_met=false`
  - `gate_strict_mode_auto_runtime_probe_any_available=true`
  - `gate_strict_mode_auto_runtime_probe_source=native`
  - `gate_result=SKIPPED`
- `run_windows_ci_gate.ps1` executes and reports the canary.
