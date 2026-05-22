# Modern Video Player

Modern Video Player is a C++17 desktop media player built on FFmpeg and SDL2. The current codebase is a multi-backend player with platform strategy, renderer/decoder fallback, subtitles, diagnostics, regression CLIs, and packaging scripts.

The source version is `1.0.0` with the default prerelease suffix configured as `rc2` in `CMakeLists.txt`.

## Current Capabilities

- Local playback: open, play, pause, stop, seek, volume, mute, playback speed.
- User controls: fullscreen, chapters, A-B repeat, frame step, screenshots, percentage seek keys.
- Playlists: multiple input files, local `m3u8`, previous/next item, EOF next item, persisted last index.
- Subtitles: external SRT/ASS/SSA, auto-detected sidecar subtitles, embedded subtitle list/selection, language/forced/SDH policy, PGS/DVD bitmap subtitles, attachment font registration.
- Renderers: Software SDL, D3D11, OpenGL, Vulkan. Build availability depends on CMake switches and platform dependencies.
- Decoders: Software plus hardware strategy slots for D3D11VA, VAAPI, VideoToolbox and related fallback paths.
- Platform strategy: `platform_capabilities` and `playback_strategy` produce renderer/decoder candidate chains; `PlayerCore` consumes the plan and executes fallback.
- Diagnostics and regression: media probe, performance log, D3D11/OpenGL/Vulkan diagnostics, Windows/Linux gates, format regression, subtitle, screenshot, plugin and streaming infrastructure checks.
- Extension foundation: filter pipeline, plugin API, HTTP/HLS/DASH/ABR infrastructure, settings persistence and hotkey management.

## Boundaries

- Windows and Linux are the current primary targets.
- macOS has strategy/build placeholders but is not a completed delivery target.
- Streaming currently validates downloader, manifest and ABR infrastructure; it is not yet a complete online playback path.
- Plugin loading and a sample plugin exist; there is no user-facing plugin manager UI.
- The skin system is a lightweight theme skeleton.

## Build

Windows:

```powershell
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target modern-video-player sample_logger_plugin
```

Linux:

```bash
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

Main CMake feature switches:

```text
ENABLE_D3D11_RENDERER
ENABLE_OPENGL_RENDERER
ENABLE_SDL_RENDERER
ENABLE_VULKAN_RENDERER
ENABLE_D3D11VA
ENABLE_DXVA2
ENABLE_VAAPI
ENABLE_VIDEOTOOLBOX
```

## Usage

```powershell
.\build\Release\modern-video-player.exe .\juren-30s.mp4
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle .\movie.ass
.\build\Release\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

Common diagnostics:

```powershell
.\build\Release\modern-video-player.exe --version
.\build\Release\modern-video-player.exe --capabilities
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```

## Keyboard Shortcuts

| Key | Action |
| --- | --- |
| Space | Play/pause |
| Enter / Alt+Enter / F | Toggle fullscreen |
| Esc / Q | Exit fullscreen or quit |
| Left / Right | Seek -/+5 seconds |
| Ctrl+Left / Ctrl+Right | Seek -/+30 seconds |
| Up / Down / +/- | Volume |
| M | Mute |
| [ / ] / R | Speed down/up/reset |
| PageUp / PageDown | Previous/next item |
| Home / End | Previous/next chapter |
| A / B / C | Set/clear A-B repeat |
| S | Screenshot |
| , / . | Frame step while paused |
| J / K | Subtitle delay |
| Ctrl+J / Ctrl+K | Audio delay |
| 1..9 | Seek to 10%..90% |
| V | Toggle subtitles |

## Source Map

```text
core/        PlayerCore, Scheduler, Clock, queues, WorkerThread, PlaybackStrategy
platform/    platform capability probing and hardware device creation
decoder/     decoder capability and backend ordering
render/      SDL, D3D11, OpenGL and Vulkan renderers
subtitle/    SRT/ASS, embedded subtitles, bitmap subtitles, font registration
audio/       mixer/equalizer helpers; SDL output lives in audio_player
filters/     audio/video filter interfaces, chains and built-ins
streaming/   HTTP, HLS, DASH and ABR infrastructure
plugin/      plugin API, manager and sample plugin
playlist/    playlist model and navigation
config/      settings persistence
input/       hotkey and input abstractions
ui/          lightweight skin/theme skeleton
```

## Documentation

- Current architecture: [docs/design/ARCHITECTURE.md](docs/design/ARCHITECTURE.md)
- Feature and validation guide: [docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md](docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md)
- Roadmap and TODOs: [docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md](docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md)
- Reports index: [docs/reports/README.md](docs/reports/README.md)
- Version notes: [docs/records/VERSION.md](docs/records/VERSION.md)
