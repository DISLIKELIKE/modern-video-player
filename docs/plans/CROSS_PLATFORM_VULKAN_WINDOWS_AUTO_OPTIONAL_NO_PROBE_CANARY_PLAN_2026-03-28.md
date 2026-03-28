# Cross-platform Vulkan Windows auto optional no-probe canary plan (2026-03-28)

## Scope
- Deliver `VK-051`: add dedicated canary coverage for Windows Vulkan `auto` policy downgrade branch when runtime probes are unavailable.

## Implementation Planner
1. Freeze acceptance for `auto` downgrade branch (`sdk=1 + native_probe=0 + swiftshader_probe=0`).
Dependency: none.
2. Implement dedicated canary script and summary contract.
Dependency: step 1.
3. Integrate canary into `tools/run_windows_ci_gate.ps1` with Step Summary output.
Dependency: step 2.
4. Execute local auto-policy branch matrix regression:
  - auto optional no-probe
  - auto strict native probe
  - auto strict swiftshader probe
Dependency: step 3.
5. Sync docs/records/index chain.
Dependency: step 4.

## Acceptance
- New canary exits PASS with:
  - `gate_mode=optional`
  - `gate_strict_mode_effective=false`
  - `gate_strict_mode_auto_prerequisites_met=false`
  - `gate_strict_mode_auto_runtime_probe_source=none`
  - `gate_result=SKIPPED`
- `run_windows_ci_gate.ps1` executes and reports new canary.
- Native and SwiftShader auto strict canaries remain PASS.
