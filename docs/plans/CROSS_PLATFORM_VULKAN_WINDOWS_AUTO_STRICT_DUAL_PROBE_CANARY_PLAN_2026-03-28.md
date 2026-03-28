# Cross-platform Vulkan Windows auto strict dual-probe canary plan (2026-03-28)

## Scope
- Deliver `VK-052`: add dedicated canary coverage for Windows Vulkan `auto` strict branch when native and SwiftShader probes are both available.

## Implementation Planner
1. Freeze acceptance for dual-probe auto strict branch (`sdk=1 + native_probe=1 + swiftshader_probe=1`).
Dependency: none.
2. Add dedicated canary script and summary contract.
Dependency: step 1.
3. Integrate canary into `tools/run_windows_ci_gate.ps1` and Step Summary.
Dependency: step 2.
4. Execute local validation for dual-probe branch.
Dependency: step 3.
5. Sync docs/records/index chain.
Dependency: step 4.

## Acceptance
- New canary exits PASS with:
  - `gate_mode=strict`
  - `gate_strict_mode_effective=true`
  - `gate_strict_mode_auto_runtime_probe_source=native+swiftshader`
  - `gate_result=PASS`
- `run_windows_ci_gate.ps1` executes and reports new canary.
