# Cross-platform Vulkan GitHub Actions Linux lane plan (2026-03-27)

## Scope
- Deliver `VK-012`: wire Vulkan build/check requirements into GitHub Actions Linux lane.

## Implementation Planner
1. Update Linux dependency install list to include Vulkan packages.  
Dependency: none.
2. Update Linux configure command with explicit `ENABLE_VULKAN_RENDERER=ON`.  
Dependency: Step 1.
3. Update Linux gate call to enforce Vulkan strict mode (`arg11=1`).  
Dependency: `VK-011` script support + Step 2.
4. Run local static/regression validation:
  - workflow field scan
  - gate script syntax
  - baseline build + Vulkan diagnostics command sanity  
Dependency: Step 3.
5. Sync docs/index/records closure for `VK-012`.  
Dependency: Step 4.

## Acceptance
- Linux workflow includes Vulkan dependency + build switch + strict gate flag.
- Local validation passes for static checks/build baseline.
- Documentation chain and records chain are fully synced.
- Real Linux runner evidence is tracked as post-push validation item.
