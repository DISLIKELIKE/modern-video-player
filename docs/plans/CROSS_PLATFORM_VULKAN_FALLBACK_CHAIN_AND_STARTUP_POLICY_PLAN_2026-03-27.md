# Cross-platform Vulkan fallback chain and startup policy plan (2026-03-27)

## Scope
- Deliver `VK-009`: explicit Linux Vulkan fallback chain integration and startup policy observability.

## Implementation Planner
1. Add Linux Vulkan chain-normalization helper in `PlaybackStrategy`.  
Dependency: existing renderer candidate build flow.
2. Wire reason-tag append for normalized Linux Vulkan path (`linux-vulkan-fallback-chain`).  
Dependency: Step 1.
3. Extend `PlayerCore` diagnostics snapshot fields with startup plan reasons.  
Dependency: none.
4. Export startup plan reasons in `--performance-log-check`.  
Dependency: Step 3.
5. Run local build/regressions and sync docs/records/indexes.  
Dependency: Steps 1-4.

## Acceptance
- Linux policy contract is explicit in strategy code, not only indirect priority behavior.
- Startup plan reasons are machine-readable in performance output.
- Existing local Windows checks remain PASS.
- Linux runtime fallback proof remains tracked for runner validation.
