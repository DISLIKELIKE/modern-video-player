# Cross-Platform Build Switch and Packaging Baseline Design
Date: 2026-03-26
Scope: CP-301 ~ CP-305
Status: Implemented Baseline

## 1. Objective
- Make backend compilation explicit and configurable from CMake options.
- Prevent platform-incompatible backend sources from being built on wrong targets.
- Surface backend compiled/runtime availability in startup diagnostics.
- Establish Linux package output baseline (`DEB` first, with `TGZ` companion).

## 2. Feature Switch Set
- Renderer:
  - `ENABLE_D3D11_RENDERER`
  - `ENABLE_OPENGL_RENDERER`
  - `ENABLE_SDL_RENDERER`
- Decoder capability toggles:
  - `ENABLE_D3D11VA`
  - `ENABLE_DXVA2`
  - `ENABLE_VAAPI`
  - `ENABLE_VIDEOTOOLBOX`

Switches are normalized into effective values (`MVP_ENABLE_*`) with platform force-off rules:
- Windows-only switches are forced off on non-Windows.
- Linux-only/Apple-only switches are forced off on incompatible platforms.

## 3. Build Guard Strategy
- Renderer source files are appended conditionally:
  - `src/render/d3d11_video_renderer.cpp` only when `MVP_ENABLE_D3D11_RENDERER`.
  - `src/render/opengl_video_renderer.cpp` only when `MVP_ENABLE_OPENGL_RENDERER`.
  - `src/render/sdl_video_renderer.cpp` only when `MVP_ENABLE_SDL_RENDERER`.
- At least one renderer switch must remain enabled (fatal on empty set).
- Main diagnostics entry points are compile-guarded by capability macros to avoid unresolved symbol paths when backend sources are disabled.

## 4. Capability Macro Contract
Generated compile definitions:
- `MVP_HAVE_SOFTWARE_SDL_RENDERER`
- `MVP_HAVE_OPENGL_RENDERER`
- `MVP_HAVE_D3D11_RENDERER`
- `MVP_HAVE_D3D11VA_DECODER`
- `MVP_HAVE_DXVA2_DECODER`
- `MVP_HAVE_VAAPI_DECODER`
- `MVP_HAVE_VIDEOTOOLBOX_DECODER`

These macros drive:
- factory source inclusion paths
- platform capability probe exposure
- diagnostics command behavior

## 5. Startup Observability Contract
Added startup diagnostics fields:
- `startup_renderer_compiled_set`
- `startup_renderer_runtime_set`
- `startup_decoder_compiled_set`
- `startup_decoder_runtime_set`

Export path:
- `PlayerCore` snapshot fields -> `DiagnosticsSnapshot` -> `--performance-log-check`.

## 6. Linux Dependency Closure Baseline
On Linux (`UNIX` and not `APPLE`), CMake now requires:
- `libass`
- `fontconfig`
- `freetype2`

And OpenGL path links through:
- `OpenGL::GL` when `ENABLE_OPENGL_RENDERER=ON`.

## 7. Packaging Baseline
- CPack generator policy:
  - Windows: `ZIP`
  - Linux: `DEB;TGZ`
- Linux DEB metadata baseline:
  - maintainer/vendor
  - package section
  - shared-library dependency scan (`CPACK_DEBIAN_PACKAGE_SHLIBDEPS`)
  - baseline runtime dependency list
- Added helper script:
  - `tools/package_linux.sh`

## 8. Deferred Work
- Linux runtime gate execution (`CP-401+`) and CI matrix validation (`CP-903/904`).
- Package signing/distribution repo integration for Linux releases.
