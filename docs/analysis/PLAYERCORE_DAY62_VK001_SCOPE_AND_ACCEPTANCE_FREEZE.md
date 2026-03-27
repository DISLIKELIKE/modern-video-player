# PLAYERCORE Day62: VK001 scope and acceptance freeze

Date: 2026-03-26  
Status: Done (scope frozen, implementation not started)

## 1. Current Baseline
- Renderer backends currently in tree: `D3D11`, `OpenGL`, `SoftwareSDL`.
- No Vulkan build switch (`ENABLE_VULKAN_RENDERER`) exists in `CMakeLists.txt`.
- No Vulkan renderer type/factory wiring exists in:
  - `include/render/video_renderer.h`
  - `src/render/renderer_factory.cpp`
  - `src/platform/platform_capabilities.cpp`
  - `src/core/playback_strategy.cpp`
- Linux gate and CI currently validate:
  - `OpenGL -> SoftwareSDL` renderer fallback
  - `VAAPI -> Software` decoder fallback
  - no Vulkan diagnostics/check lane yet.

## 2. Scope Freeze (Goals)
- Linux-first Vulkan renderer delivery with staged rollout (`VK-001` ~ `VK-014`).
- Keep existing renderer architecture (`PlaybackStrategy` + `RendererFactory`) as the integration spine.
- Add explicit runtime observability and gate/CI coverage before calling Vulkan path closed.
- Keep existing Windows production paths (`D3D11`, `OpenGL`) behavior-stable while Vulkan lands.

## 3. Non-goals (This Wave)
- No macOS Vulkan/Metal delivery in this round (explicitly deferred).
- No decoder architecture rewrite (decoder chain remains existing policy + software mandatory fallback).
- No promise of day-one parity with all OpenGL advanced output features (HDR/ICC/3DLUT) during initial Vulkan MVP (`clear/present + one upload path first`).

## 4. Acceptance Definition (DoD)
- Build:
  - `ENABLE_VULKAN_RENDERER` switch works and can be toggled on/off without breaking existing backends.
  - Linux build passes with Vulkan enabled on CI.
- Runtime:
  - Vulkan backend can initialize, clear, present, and survive resize/swapchain recreate.
  - At least one video upload path (`YUV420` or `NV12`) displays decoded frames.
  - Startup fallback chain on Linux is observable and deterministic: `Vulkan -> OpenGL -> SoftwareSDL`.
- Diagnostics:
  - New machine-readable command `--vulkan-diagnostics` returns explicit pass/fail and key capabilities.
- Gates/CI:
  - `tools/run_linux_mvp_checks.sh` includes Vulkan sub-check(s).
  - `.github/workflows/cross-platform-gate.yml` Linux lane runs Vulkan build/check and archives evidence.
- Documentation:
  - Every Vulkan step keeps `analysis/design/plan/report` + `VERSION/CHANGELOG/DEVELOP_LOG` synchronized.

## 5. Risks Frozen for Follow-up
1. Vulkan loader/SDK availability differs across Linux runners.
2. Swapchain and present mode behavior varies by driver.
3. Upload path complexity can expand quickly if multi-format support is attempted too early.
4. Dual helper stacks (OpenGL and Vulkan) can drift without explicit cleanup tasks.

## 6. Resolution for VK-001
- Scope and DoD are now frozen for Linux-first Vulkan execution.
- Next step is `VK-002` architecture design and strategy integration details.
