# PLAYERCORE Day71: VK010 Vulkan diagnostics CLI

Date: 2026-03-27  
Status: Done

## 1. Problem
- Vulkan path already has startup strategy integration (`VK-009`), but there is no dedicated machine-readable CLI like `--d3d11-diagnostics` / `--opengl-diagnostics`.
- `VK-011` Linux gate and `VK-012` CI lane cannot consume a stable Vulkan diagnostics contract without this command.

## 2. Root Cause
- `src/main.cpp` currently exposes diagnostics entry points only for D3D11/OpenGL.
- Existing Vulkan observability is indirect (mainly through `--performance-log-check` + startup snapshot), not a dedicated direct command.

## 3. Solution
- Added `--vulkan-diagnostics` command in `src/main.cpp` with machine-readable key/value output.
- Reused existing data sources instead of adding a new deep probe module:
  - `platform::PlatformCapabilitiesProbe::detect()`
  - `core::PlaybackStrategy::buildOpenPlan(...)`
  - renderer factory support checks (`RendererFactory::isSupported`)
- Exported fields:
  - platform support/compile/runtime visibility
  - renderer override input and startup candidate chain
  - startup plan reason and selected fallback target prediction
  - `PASS/FAIL` result contract
- Wired command into:
  - usage text (`printUsage`)
  - CLI dispatch branch (`main` argument parser)

## 4. Validation Outcome
- Build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin`
  - Result: PASS
- Runtime checks:
  - `.\build\Release\modern-video-player.exe --d3d11-diagnostics`
    - PASS
  - `.\build\Release\modern-video-player.exe --opengl-diagnostics`
    - PASS
  - `.\build\Release\modern-video-player.exe --vulkan-diagnostics`
    - expected on Windows host:
      - `vulkan-diagnostics.supported_platform=false`
      - `vulkan-diagnostics.result=FAIL`
  - `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200`
    - PASS
  - `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --vulkan-diagnostics`
    - fallback observability on Windows:
      - `vulkan-diagnostics.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
      - `vulkan-diagnostics.startup_renderer_plan_reason=renderer-override-env`
      - `vulkan-diagnostics.selected_renderer=D3D11`
      - `vulkan-diagnostics.fallback_target=D3D11`
      - `vulkan-diagnostics.result=FAIL` (expected non-Linux)

## 5. Follow-up
- `VK-011` should consume `--vulkan-diagnostics` in Linux gate script with machine-readable assertion rules.
