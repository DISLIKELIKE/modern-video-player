# Cross-platform Vulkan diagnostics CLI plan (2026-03-27)

## Scope
- Deliver `VK-010`: dedicated `--vulkan-diagnostics` command with stable machine-readable contract.

## Implementation Planner
1. Freeze command contract fields and PASS/FAIL rule in docs.  
Dependency: none.
2. Add `runVulkanDiagnostics()` implementation in `src/main.cpp`, reusing capability + strategy data sources.  
Dependency: Step 1.
3. Wire command into usage text and CLI dispatch (`argc/argv` branch).  
Dependency: Step 2.
4. Run local build and diagnostics regression set (`d3d11`, `opengl`, `vulkan`, `performance-log-check`).  
Dependency: Step 3.
5. Sync report/index/records documents for `VK-010` closure.  
Dependency: Step 4.

## Acceptance
- Build passes.
- `--vulkan-diagnostics` runs and prints machine-readable key/value lines with `vulkan-diagnostics.result=PASS|FAIL`.
- Existing diagnostics commands keep passing.
- Documentation chain and records chain are complete for this task.
