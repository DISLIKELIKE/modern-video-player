# Cross-platform Vulkan Windows auto strict native runtime probe canary plan (2026-03-28)

## Scope
- Deliver `VK-050`: add dedicated canary coverage for Windows Vulkan `auto` strict promotion when native runtime probe is available.

## Implementation Planner
1. Freeze acceptance for native-probe auto strict branch (`sdk=1 + native_probe=1 + swiftshader_probe=0`).
Dependency: none.
2. Add dedicated canary script and summary contract.
Dependency: step 1.
3. Integrate canary into `tools/run_windows_ci_gate.ps1` with Step Summary output.
Dependency: step 2.
4. Run local canary regression (`native auto strict`, `swiftshader auto strict`, `optional skip`).
Dependency: step 3.
5. Sync docs/records/index chain for `VK-050`.
Dependency: step 4.

## Acceptance
- New canary exits PASS with:
  - `gate_mode=strict`
  - `gate_strict_mode_effective=true`
  - `gate_strict_mode_auto_runtime_probe_source=native`
- `run_windows_ci_gate.ps1` executes and reports the new canary.
- Existing auto strict SwiftShader canary and optional-skip canary remain PASS.
