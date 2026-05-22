# Architecture Refactor Status

This file used to describe the 2026-03-06 refactor from the prototype chain to `VideoPlayer -> PlayerCore -> Scheduler`. That migration is now complete and is no longer a future plan.

Use [ARCHITECTURE.md](./ARCHITECTURE.md) as the current source of truth.

## Current Result

- `VideoPlayer` is the facade.
- `PlayerCore` owns playback lifecycle, state, commands, subtitle state and diagnostics.
- `PlaybackStrategy` and `PlatformCapabilitiesProbe` own renderer/decoder planning.
- `Scheduler`, `FrameQueue`, `Clock` and `WorkerThread` own concurrent playback execution.
- Renderers implement `IVideoRenderer` and focus on rendering/presentation.
- Decoder and renderer selection have software/fallback paths instead of old one-off hardcoded branches.

## Removed Historical Concepts

The following names are no longer the active architecture:

- `video_decoder` / `audio_decoder`
- `video_decode_thread` / `audio_decode_thread`
- `sync_manager`
- `packet_reader`
- old `decoder_worker`
- unused legacy `DecoderThread`

## Current Main Files

```text
include/video_player.h
src/video_player.cpp
include/core/player_core.h
src/core/player_core.cpp
include/core/playback_strategy.h
src/core/playback_strategy.cpp
include/platform/platform_capabilities.h
src/platform/platform_capabilities.cpp
include/core/scheduler.h
src/core/scheduler.cpp
include/core/worker_thread.h
src/core/worker_thread.cpp
include/render/video_renderer.h
src/render/renderer_factory.cpp
src/render/sdl_video_renderer.cpp
src/render/d3d11_video_renderer.cpp
src/render/opengl_video_renderer.cpp
src/render/vulkan_video_renderer.cpp
```

## Rule Going Forward

Do not add new platform policy directly to `PlayerCore::open()`. Add platform capability or backend-specific logic in the platform/factory layer, then let `PlaybackStrategy` produce the plan.
