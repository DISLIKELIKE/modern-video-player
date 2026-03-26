# PLAYERCORE DAY49: CP-301 ~ CP-305 Build Switch/Guard/Packaging Baseline
Date: 2026-03-26
Status: Done

## 1. Problem Statement
- CMake lacked explicit backend feature switches, so cross-platform backend composition was not configurable.
- Windows-only source compilation boundaries were not fully controlled by build switches.
- Linux dependency closure and packaging baseline were not codified as first-class build/packaging paths.
- Startup diagnostics lacked explicit compiled-set/runtime-set outputs for backend observability.

## 2. Root Cause
- Backend capability macros were hardcoded rather than derived from user-visible build switches.
- Renderer source list and diagnostics command references were not fully switch-aware.
- Linux package dependencies and package generator defaults were not standardized in build config.

## 3. Implementation Planner Used
1. Add explicit CMake feature switches and map them to compile-time capability macros.
2. Make renderer source inclusion/link dependencies switch-aware and platform-guarded.
3. Extend capability model/diagnostics output with compiled-set/runtime-set fields.
4. Add Linux dependency closure checks and Linux packaging baseline path.
5. Validate default build plus switch combinations.

## 4. Landed Changes
- CMake feature switches and source/link guards:
  - `CMakeLists.txt`
- Decoder backend enum/extensions for capability exposure:
  - `include/decoder/decoder_capability.h`
  - `src/decoder/decoder_factory.cpp`
- Capability probe and startup diagnostics set fields:
  - `src/platform/platform_capabilities.cpp`
  - `include/core/player_core.h`
  - `src/core/player_core.cpp`
  - `src/main.cpp`
- OpenGL diagnostics compatibility when D3D11 renderer is disabled:
  - `src/render/opengl_video_renderer.cpp`
- Linux packaging baseline script:
  - `tools/package_linux.sh`

## 5. Result Against CP IDs
- `CP-301`: done (explicit backend switches in CMake).
- `CP-302`: done (source/link/platform guard enforcement by switch + platform conditions).
- `CP-303`: done (Linux dependency closure requirements added to CMake: `libass/fontconfig/freetype/OpenGL` path).
- `CP-304`: done (startup diagnostics now export compiled/runtime backend sets).
- `CP-305`: done (Linux packaging baseline path with `DEB/TGZ` + helper script).

## 6. Validation
- Default configure/build:
  - `cmake -S . -B build`
  - `cmake --build build --config Release --target modern-video-player`
- Runtime regression on default build:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4`
  - `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800`
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Switch-matrix build checks:
  - `cmake -S . -B build_nod3d11 -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON`
  - `cmake --build build_nod3d11 --config Release --target modern-video-player`
  - `cmake -S . -B build_noopengl -DENABLE_OPENGL_RENDERER=OFF -DENABLE_D3D11_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_D3D11VA=ON`
  - `cmake --build build_noopengl --config Release --target modern-video-player`
- Key outcomes:
  - All three build configurations succeeded.
  - Default runtime regressions passed (`performance-log-check/renderer-fallback-check/interaction-freeze-check/OpenGL gate`).
  - Startup diagnostics include:
    - `startup_renderer_compiled_set`
    - `startup_renderer_runtime_set`
    - `startup_decoder_compiled_set`
    - `startup_decoder_runtime_set`

## 7. Remaining Risks
- Linux native build/package execution was not run in this Windows environment; Linux runtime/package validation remains pending.
- `build_nod3d11` runtime `performance-log-check` still hits known software decode blocker path (existing debt, unrelated to CP-301~305 switch mechanics).
