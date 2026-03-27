# Cross-platform Vulkan scope and acceptance freeze plan (2026-03-26)

## Scope
- Complete `VK-001` and freeze a non-ambiguous implementation contract for `VK-002 ~ VK-014`.

## Implementation Planner
| Step | Task | Depends On | Deliverable |
| --- | --- | --- | --- |
| VK-001 | Freeze scope and acceptance (Linux first, macOS deferred) | none | analysis/design/plan/report + records sync |
| VK-002 | Architecture design and strategy integration (`RendererFactory` / `PlaybackStrategy`) | VK-001 | module/ownership/fallback design doc |
| VK-003 | Build switch and dependency detection (`ENABLE_VULKAN_RENDERER`) | VK-002 | CMake switch + macro + downgrade rules |
| VK-004 | Vulkan renderer skeleton + factory wiring | VK-003 | compilable empty backend path |
| VK-005 | SDL + Vulkan init chain (instance/surface/device/swapchain) | VK-004 | init/destroy lifecycle closure |
| VK-006 | Minimal render path (clear + present + resize recreate) | VK-005 | stable visible output |
| VK-007 | CPU frame upload path (`YUV420`/`NV12` at least one) | VK-006 | decoded frame display on Vulkan |
| VK-008 | Sync and pacing (fence/semaphore/present mode) | VK-007 | no obvious deadlock/tearing regressions |
| VK-009 | Linux startup fallback chain (`Vulkan -> OpenGL -> SoftwareSDL`) | VK-008 | observable auto-fallback |
| VK-010 | Vulkan diagnostics CLI (`--vulkan-diagnostics`) | VK-009 | machine-readable diagnostics output |
| VK-011 | Linux gate script Vulkan checks | VK-010 | `tools/run_linux_mvp_checks.sh` Vulkan stage |
| VK-012 | GitHub Actions Linux Vulkan lane | VK-011 | CI logs + summary artifacts |
| VK-013 | Regression matrix execution (open/play/pause/seek/subtitle/fallback) | VK-012 | local validation report |
| VK-014 | Documentation and release closure | VK-013 | records/indexes fully synchronized |

## Step Dependencies
- Strict sequential execution; each step depends on previous step completion.
- Code changes start from `VK-003` after `VK-002` design confirmation.
- Gate/CI work (`VK-011`/`VK-012`) must not start before diagnostics contract (`VK-010`) is stable.

## VK-001 Acceptance
- Scope, non-goals, and DoD are explicit and documented.
- Records and index docs are synchronized.
- Baseline local build/regression commands still pass.

## Next Action (after confirmation)
- Execute `VK-002` architecture and strategy integration design, then start code changes from `VK-003`.
