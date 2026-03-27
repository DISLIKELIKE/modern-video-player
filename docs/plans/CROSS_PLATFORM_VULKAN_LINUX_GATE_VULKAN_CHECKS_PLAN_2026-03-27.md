# Cross-platform Vulkan Linux gate checks plan (2026-03-27)

## Scope
- Deliver `VK-011`: integrate Vulkan diagnostics check path into Linux gate script.

## Implementation Planner
1. Add Vulkan probe stage in Linux gate script and freeze availability contract (`HAS_VK010`).  
Dependency: `VK-010` command exists.
2. Add capability-aware check branch:
  - run `vk010_vulkan_diagnostics` when available
  - otherwise emit machine-readable `SKIPPED` reason
  - support strict mode via `REQUIRE_VULKAN_CHECKS`  
Dependency: Step 1.
3. Extend gate report fields for Vulkan probe/check metadata.  
Dependency: Step 2.
4. Run local validation:
  - script syntax
  - non-Linux dispatch behavior
  - baseline build and Vulkan diagnostics command check  
Dependency: Step 3.
5. Sync docs indexes and records chain for `VK-011`.  
Dependency: Step 4.

## Acceptance
- `tools/run_linux_mvp_checks.sh` includes Vulkan sub-check path.
- Gate output/report includes machine-readable Vulkan probe/check fields.
- Script remains syntax-valid and keeps expected non-Linux guard behavior.
- Local build/verification and documentation chain are complete.
