# Current Player Gap Analysis

This document compares the current source tree with a mature desktop player feature set. It replaces the early 2026-03 prototype-era gap analysis.

## Current Baseline

The project now has a usable local playback core with multiple renderer backends, subtitle support, hardware-decode strategy, diagnostics and regression tooling. It is still not an MPC-HC-level product because several features exist as engineering infrastructure rather than complete user-facing workflows.

## Module Status

| Module | Current status |
| --- | --- |
| Core playback | Strong baseline. `PlayerCore`, `Scheduler`, queues, clock, seek, A-B repeat, screenshot, frame step and diagnostics exist. |
| Renderer | D3D11, OpenGL, Software SDL and Vulkan implementations exist. Maturity depends on platform/runtime; diagnostics and fallback are important parts of the design. |
| Decode | Software fallback exists; D3D11VA/VAAPI/VideoToolbox strategy slots exist. Hardware availability is platform and driver dependent. |
| Audio | SDL audio output is the stable path. Mixer/equalizer helpers exist, but there is no full user-facing audio device/equalizer UI. |
| Subtitle | External SRT/ASS/SSA, embedded text, PGS/DVD bitmap, font attachment and libass probe paths exist. |
| Playlist | Multi-file and local `m3u8` flow exists, with previous/next and last-index persistence. |
| Settings/hotkeys | Settings persistence and hotkey manager exist. |
| Filters | Filter API, registry, chains and built-ins exist; product UI is missing. |
| Plugins | API, plugin manager and sample plugin exist; product UI/distribution/isolation are missing. |
| Streaming | HTTP downloader, HLS/DASH parsers and ABR selection exist; complete online playback integration is missing. |
| Skin/theme | `SkinEngine` exists as a skeleton only. |
| Cross-platform | Windows and Linux are active. macOS is deferred. |

## Main Gaps

| Priority | Gap | Why it matters |
| --- | --- | --- |
| P0 | Fresh rc2 validation | Source is configured for `1.0.0-rc2`; current binaries/reports may be stale. |
| P0 | Full current Windows/Linux gate evidence | Historical reports are useful, but current source should have fresh PASS artifacts before release. |
| P1 | macOS delivery path | Strategy placeholders exist, but compile/playback/package are not complete. |
| P1 | Online streaming playback | Infrastructure exists but is not integrated as a full playback path. |
| P1 | Plugin and filter user workflows | Engineering hooks exist, but users cannot manage/tune them cleanly. |
| P2 | Skin/theme productization | Current implementation is a minimal theme container. |
| P2 | Larger hardware/driver compatibility corpus | Renderer/hardware fallback quality depends on real adapter samples. |

## Not Gaps Anymore

The following items were early gaps but are now represented in source:

- OpenGL is no longer just a stub.
- Renderer choice is no longer directly hardcoded in `PlayerCore`.
- Embedded subtitle track list/selection and policies exist.
- PGS/DVD bitmap subtitle model/checks exist.
- Linux MVP and VAAPI fallback checks exist.
- Vulkan renderer/diagnostics/gate scripts exist.
- Startup strategy diagnostics and runtime counters exist.

## Evidence Entry Points

- Architecture: `docs/design/ARCHITECTURE.md`
- Current task board: `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- Feature guide: `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- CLI implementation: `src/main.cpp`
- Core implementation: `include/core/player_core.h`, `src/core/player_core.cpp`
- Renderer interface: `include/render/video_renderer.h`
- Strategy files: `include/core/playback_strategy.h`, `src/core/playback_strategy.cpp`
- Platform capability files: `include/platform/platform_capabilities.h`, `src/platform/platform_capabilities.cpp`
