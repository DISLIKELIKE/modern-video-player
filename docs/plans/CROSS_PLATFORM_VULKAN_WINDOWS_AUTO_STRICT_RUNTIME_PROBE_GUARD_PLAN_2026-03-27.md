# Cross-platform Vulkan Windows auto strict runtime probe guard plan (2026-03-27)

## Scope
- Close the Windows Vulkan `auto` strict false-escalation gap by requiring runtime probe signal in addition to SDK signal.

## Implementation Planner
1. Freeze acceptance criteria for `VK-022` strict-policy guard closure.  
Dependency: none.
2. Design dual-prerequisite strict policy (`sdk && runtime_probe`) and summary observability contract.  
Dependency: step 1.
3. Implement script-side policy update and summary fields in `run_windows_vulkan_checks.ps1`.  
Dependency: step 2.
4. Implement workflow runtime probe export (`MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE`).  
Dependency: step 3.
5. Execute local validation matrix: configure/build/diagnostics + policy matrix (`sdk=1,runtime=0/1`).  
Dependency: step 4.
6. Sync docs/records/index closure (`analysis/design/plan/report + VERSION/CHANGELOG/DEVELOP_LOG + README indexes`).  
Dependency: step 5.

## Acceptance
- `auto + sdk=1 + runtime_probe=0` remains optional (`SKIPPED` path available).
- `auto + sdk=1 + runtime_probe=1` escalates to strict.
- Summary and workflow logs expose runtime probe and auto-policy basis fields.
- Local build/check matrix passes with expected host-dependent outcomes.
- No commit/push without explicit user confirmation.
