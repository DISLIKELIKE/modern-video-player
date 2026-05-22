# Current Architecture

This document describes the current source architecture. It intentionally replaces early prototype notes and no longer documents removed `video_decoder` / `audio_decoder` / `DecoderThread` style implementations.

## Main Runtime Path

```text
src/main.cpp
  -> vp::VideoPlayer
  -> vp::core::PlayerCore
  -> Demuxer + PlatformCapabilitiesProbe + PlaybackStrategy
  -> RendererFactory + DecoderFactory
  -> Scheduler + FrameQueue + Clock
  -> AudioPlayer + selected IVideoRenderer
```

`VideoPlayer` is the public facade. `PlayerCore` owns media open/close, playback state, seek, A-B repeat, screenshots, subtitle state and diagnostics. Backend choice is produced by `PlaybackStrategy`, not hardcoded in `PlayerCore`.

## Key Layers

| Layer | Main files | Responsibility |
| --- | --- | --- |
| Application/CLI | `src/main.cpp` | Command-line parsing, validation commands, normal playback setup. |
| Facade | `include/video_player.h`, `src/video_player.cpp` | Stable player API around `PlayerCore`. |
| Core | `include/core/*`, `src/core/*` | Playback lifecycle, scheduler, queues, clocks, worker lifecycle and diagnostics. |
| Platform strategy | `include/platform/*`, `src/platform/*`, `include/core/playback_strategy.h` | Platform capability probing, hardware device creation, renderer/decoder candidate planning. |
| Media | `include/demuxer.h`, `src/demuxer.cpp`, `include/media/*`, `src/media/*` | FFmpeg open/probe, stream metadata, format capability decisions. |
| Decode | `include/decoder/*`, `src/decoder/*` | Decoder backend model and candidate ordering. |
| Render | `include/render/*`, `src/render/*` | Software SDL, D3D11, OpenGL and Vulkan renderers; output color diagnostics. |
| Audio | `include/audio_player.h`, `src/audio_player.cpp`, `src/audio/*` | SDL audio output plus mixer/equalizer helpers. |
| Subtitle | `include/subtitle/*`, `src/subtitle/*` | SRT/ASS parsing, embedded subtitle loading, bitmap subtitle model, font registration and libass probe. |
| Features | `playlist/`, `config/`, `input/`, `filters/`, `plugin/`, `streaming/`, `ui/` | User features and extension infrastructure. |

## Playback Open Flow

1. `PlayerCore::open()` opens media through `Demuxer`.
2. Platform capabilities are detected.
3. `PlaybackStrategy::buildOpenPlan()` builds renderer and decoder candidates from media info, user preference and platform capabilities.
4. `RendererFactory` creates the first usable renderer from the plan.
5. `DecoderFactory` orders decoder backends with mandatory software fallback.
6. `PlayerCore` initializes audio, queues, subtitle policy, diagnostics and scheduler state.
7. `Scheduler` drives decode/render loops and applies clock/backpressure decisions.

## Renderer Model

`IVideoRenderer` is now rendering-focused:

- `init`, `close`
- `renderFrame`, `present`, `clear`
- native/direct frame format support
- native device handle
- diagnostics
- backend name

Input and playback commands are handled above the renderer layer. This keeps renderer implementations focused on upload, color, overlay and presentation.

Available renderer types:

- `SoftwareSDL`
- `D3D11`
- `OpenGL`
- `Vulkan`

Actual compiled availability is controlled by CMake feature switches and platform guards.

## Decoder Model

Decoder backend enum:

- `Software`
- `CUDA`
- `D3D11VA`
- `DXVA2`
- `VAAPI`
- `VideoToolbox`

The code models more backends than every platform currently delivers. Selection is capability-driven, and software fallback is the stable baseline.

## Diagnostics

`core::DiagnosticsSnapshot` is the central machine-readable runtime snapshot. It includes:

- audio init state and strategy
- startup platform, compiled/runtime renderer and decoder sets
- renderer and decoder candidate chains
- selected renderer/decoder and fallback reasons
- queue generations, queue sizes, decode/render counters
- stale packet/frame drop counters
- scheduler wait/backpressure/restart counters
- OpenGL native interop, present pacing, HDR and LUT state
- D3D11 present/HDR state

Most validation CLIs in `src/main.cpp` consume this snapshot.

## Cross-Platform Boundary

Windows-specific code is guarded and concentrated in D3D11, DirectWrite, DXGI and Windows packaging paths. Linux-specific behavior is represented through feature switches, OpenGL/SDL path, libass/fontconfig/freetype, VAAPI capability probing and Linux gate scripts.

macOS is not a completed delivery target. `VideoToolbox` and platform switches are placeholders for a future phase.

## Current Known Gaps

- macOS compile/playback/package path is not complete.
- Streaming infrastructure exists, but full online playback integration is not complete.
- Plugin API and sample plugin exist, but user-facing plugin configuration/distribution is not complete.
- Skin/theme system is still a small skeleton.
- Filter pipelines exist, but there is no complete user-facing filter control UI.
