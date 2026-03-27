# Cross-platform Vulkan diagnostics CLI design (2026-03-27)

## 1. Goal
Deliver `VK-010` by introducing `--vulkan-diagnostics` with machine-readable output and deterministic pass/fail semantics.

## 2. Scope
- In scope:
  - CLI usage text and argument dispatch
  - dedicated `runVulkanDiagnostics()` in `src/main.cpp`
  - startup strategy + capability projection for Vulkan observability
- Out of scope:
  - Linux gate script integration (`VK-011`)
  - GitHub Actions Linux lane wiring (`VK-012`)
  - deep Vulkan runtime probing beyond existing capability/strategy contracts

## 3. Data Sources
- Capability snapshot:
  - `platform::PlatformCapabilitiesProbe::detect()`
- Startup candidate/reason generation:
  - `core::PlaybackStrategy::buildOpenPlan(...)`
- Runtime support judgement:
  - `render::RendererFactory::isSupported(VideoRendererType::Vulkan, capabilities)`

## 4. Output Contract
Prefix: `vulkan-diagnostics.*`

Required fields:
- `supported_platform` (`true` for Linux, else `false`)
- `compiled_in`
- `runtime_available`
- `requested_renderer_override`
- `startup_renderer_candidates`
- `startup_renderer_plan_reason`
- `selected_renderer`
- `fallback_target`
- `result` (`PASS|FAIL`)

Decision baseline:
- `PASS` when Vulkan is compiled-in and runtime-available.
- `FAIL` otherwise.

## 5. Compatibility Strategy
- No behavior change to existing `--d3d11-diagnostics` / `--opengl-diagnostics`.
- No changes to playback open path; this is diagnostics-only projection.
- On non-Linux hosts, command remains callable and returns explicit unsupported/FAIL machine-readable output.
