# Cross-platform Vulkan Windows auto strict SwiftShader runtime probe promotion plan (2026-03-28)

## Scope
- Deliver `VK-049`: make Windows Vulkan `auto` strict mode consumable in CI when native runtime probe is absent but SwiftShader runtime probe is available.

## Implementation Planner
1. Freeze acceptance for `VK-049` policy extension (`sdk && (native_probe || swiftshader_probe)`).
Dependency: none.
2. Update strict policy logic in `tools/run_windows_vulkan_checks.ps1`.
Dependency: step 1.
3. Extend summary observability fields for auto strict probe-source diagnostics.
Dependency: step 2.
4. Add a dedicated canary for `auto + sdk=1 + native_probe=0 + swiftshader_probe=1`.
Dependency: step 3.
5. Integrate canary into `tools/run_windows_ci_gate.ps1` and Step Summary rendering.
Dependency: step 4.
6. Execute local canary regression and sync docs/records/indexes.
Dependency: step 5.

## Acceptance
- `auto + sdk=1 + native_probe=0 + swiftshader_probe=1` enters strict mode.
- Gate summary emits:
  - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe_or_swiftshader_probe`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_any_available`
  - `windows-vulkan-check.strict_mode_auto_runtime_probe_source`
- New canary exits PASS and is wired into `run_windows_ci_gate.ps1`.
- Existing canaries (`optional-skip`, `pass-contract`) remain PASS.
