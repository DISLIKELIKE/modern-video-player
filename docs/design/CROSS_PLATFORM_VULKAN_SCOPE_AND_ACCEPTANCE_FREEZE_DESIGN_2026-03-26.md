# Cross-platform Vulkan scope and acceptance freeze design (2026-03-26)

## 1. Design Goal
Freeze the first Vulkan delivery boundary so implementation can proceed without changing acceptance targets mid-flight.

## 2. Platform Boundary
- In scope: Linux x64 (primary execution and CI target).
- Out of scope (deferred): macOS.
- Windows: keep current behavior stable; no Vulkan default-policy switch in this wave.

## 3. Integration Contract
- Vulkan renderer is added as one backend candidate in existing policy chain:
  - `PlaybackStrategy` decides candidate order.
  - `RendererFactory` owns capability check + creation.
  - `PlayerCore` keeps plan-consume + fallback execution model unchanged.
- Linux startup policy target:
  - `Vulkan -> OpenGL -> SoftwareSDL`.

## 4. Acceptance Contract
- Build switch contract: `ENABLE_VULKAN_RENDERER` + compile macro propagation.
- Runtime contract: init/draw/present/resize-recreate + one frame upload path.
- Diagnostics contract: machine-readable `--vulkan-diagnostics`.
- Gate/CI contract: Linux gate + workflow lane include Vulkan checks and artifact summary.

## 5. Rollout Guardrails
- No destructive impact on existing `D3D11/OpenGL/SoftwareSDL` code paths.
- Do not expand Vulkan scope to advanced color/HDR features before baseline stability.
- Keep fallback reason observability on every failing candidate.

## 6. Deferred Topics
- macOS renderer strategy (`Vulkan/Metal`) remains explicitly deferred.
- Vulkan advanced output policy (HDR metadata, ICC/LUT parity) deferred until post-MVP.
