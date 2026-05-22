# Current Roadmap and TODO

This file is the lightweight current task board. It replaces the old long phase log as the primary planning entry.

## Current Scope

- Primary platforms: Windows and Linux.
- Deferred platform: macOS.
- Current architecture goal: add backend/platform work through `platform_capabilities`, factories and `playback_strategy`, not through hardcoded `PlayerCore` branches.

## Completed

| Area | Status |
| --- | --- |
| Core refactor | `VideoPlayer -> PlayerCore -> Scheduler` mainline is active. Legacy decoder/thread concepts are removed from the current architecture. |
| Worker lifecycle | `core::WorkerThread` is used for current `PlayerCore` long-running workers. |
| Platform strategy | `PlatformCapabilitiesProbe` and `PlaybackStrategy` plan renderer/decoder candidate chains. |
| Renderer factory | Factory creates/checks renderers; policy lives above it. |
| Decoder factory | Decoder ordering is context-driven with mandatory software fallback. |
| Renderer API cleanup | `IVideoRenderer` is rendering-focused. Input/playback command ownership lives above renderers. |
| Windows backend | D3D11 renderer and D3D11VA path exist with diagnostics and fallback. |
| OpenGL backend | OpenGL renderer exists with native interop diagnostics, present pacing, subtitle/overlay and color-management checks. |
| Vulkan backend | Vulkan renderer and diagnostics/gate scripts exist; availability is dependency/runtime gated. |
| Linux MVP | Linux software audio, OpenGL playback/fallback, VAAPI fallback and Linux gate commands exist. |
| Subtitles | External SRT/ASS/SSA, embedded text subtitles, policy selection, attachment fonts, libass probe, PGS/DVD bitmap subtitles. |
| Observability | Startup strategy, backend candidates, queue/decode/render counters, drop reasons and renderer diagnostics are machine-readable. |
| Packaging | Windows and Linux packaging scripts exist. |
| CI/gates | Windows CI gate, Linux MVP gate, OpenGL checks, Vulkan canaries and format regression scripts exist. |

## Remaining Work

| Priority | Item | Notes |
| --- | --- | --- |
| P0 | Rebuild/revalidate current source as `1.0.0-rc2` | Source config says `rc2`; stale local binaries may still report `rc1`. |
| P0 | Archive a fresh full Windows gate result | Use current build, not historical reports. |
| P0 | Archive a fresh full Linux gate result on a real Linux host | Linux path is implemented but should keep fresh environment evidence. |
| P1 | macOS baseline | Compile, software playback, renderer choice, VideoToolbox strategy and package smoke are still deferred. |
| P1 | Full streaming playback integration | Current HTTP/HLS/DASH/ABR infrastructure is validated as infrastructure, not end-user online playback. |
| P1 | Plugin productization | Add user-facing configuration, discovery/distribution and isolation policy. |
| P1 | Filter productization | Expose filter controls and persistence in the user surface. |
| P2 | Skin/theme system | `SkinEngine` is a skeleton; a complete theme/resource system is not done. |
| P2 | Driver quirk library growth | Keep adding real GPU/driver samples and fallback decisions. |

## Recommended Next Commands

Windows:

```powershell
cmake --build build --config Release --target modern-video-player sample_logger_plugin
.\build\Release\modern-video-player.exe --version
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_ci_gate.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4
```

Linux:

```bash
cmake --build build --parallel
bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800
```

Renderer-specific:

```powershell
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
.\build\Release\modern-video-player.exe --vulkan-diagnostics
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4
```

## Documentation Update Rule

- Update this file when current TODO priorities change.
- Put execution evidence in `docs/reports/`.
- Put version/release notes in `docs/records/VERSION.md`.
- Do not expand this file back into a day-by-day historical log.
