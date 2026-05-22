# Features, Usage and Validation

This guide is based on the current `src/main.cpp` CLI and current source modules. It replaces older prototype-era feature text.

## Normal Playback

```powershell
.\build\Release\modern-video-player.exe .\juren-30s.mp4
.\build\Release\modern-video-player.exe .\video1.mp4 .\video2.mkv
.\build\Release\modern-video-player.exe .\playlist.m3u8
```

External subtitles:

```powershell
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle .\movie.srt
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle .\movie.ass
```

Embedded subtitle selection and policy:

```powershell
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle-track 2
.\build\Release\modern-video-player.exe .\movie.mkv --subtitle-language zh,en --subtitle-forced auto --subtitle-sdh avoid
```

Output color options:

```powershell
.\build\Release\modern-video-player.exe .\movie.mkv --d3d11-hdr-output-mode auto
.\build\Release\modern-video-player.exe .\movie.mkv --opengl-hdr-output-mode auto
.\build\Release\modern-video-player.exe .\movie.mkv --opengl-3dlut .\display.cube
.\build\Release\modern-video-player.exe .\movie.mkv --opengl-icc-profile .\display.icc
.\build\Release\modern-video-player.exe .\movie.mkv --opengl-auto-icc
```

## User-Facing Features

| Area | Current status |
| --- | --- |
| Local playback | FFmpeg demux/decode, SDL audio, renderer presentation and A/V sync. |
| Playback controls | Play/pause/stop, seek, volume, mute, speed, fullscreen. |
| Playlist | Multi-file startup, local `m3u8`, previous/next, EOF next item, persisted last index. |
| Chapters | Previous/next chapter controls and validation CLI. |
| A-B Repeat | Set start/end, clear, and runtime loop enforcement. |
| Frame step | Forward/backward frame stepping while paused. |
| Screenshot | Runtime screenshot request and validation CLI. |
| Subtitles | External SRT/ASS/SSA, embedded text subtitles, PGS/DVD bitmap subtitles, track selection, language/forced/SDH policy. |
| Fonts | Attachment font extraction/registration; DirectWrite custom collection path on Windows. |
| Settings | Volume, speed, delay, hardware preference, playlist index and hotkeys. |
| Hotkeys | Default bindings, persistence, conflict checks and restore defaults. |
| Renderers | Software SDL, D3D11, OpenGL, Vulkan depending on build/platform. |
| Decode | Software fallback plus hardware strategy paths including D3D11VA and VAAPI where available. |
| Diagnostics | Capabilities, media probe, performance log, backend diagnostics and machine-readable gate output. |

## Infrastructure Features

These exist in source and have validation entry points, but are not complete end-user product surfaces:

| Area | Current boundary |
| --- | --- |
| Plugin system | Dynamic plugin loading, API version check, lifecycle and sample plugin; no user-facing plugin manager UI. |
| Streaming | HTTP downloader, HLS/DASH parsers and ABR selector; full online playback path is not complete. |
| Filters | Filter interfaces, registry, audio/video chains and built-in brightness/contrast/saturation/volume-balance filters; no complete UI for tuning. |
| Skin/theme | `SkinEngine` and theme data skeleton; no complete skinnable UI system. |

## Common Validation Commands

Basic:

```powershell
.\build\Release\modern-video-player.exe --version
.\build\Release\modern-video-player.exe --capabilities
.\build\Release\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
.\build\Release\modern-video-player.exe --evaluate-target 3840 2160 60 6 80
```

Playback and stability:

```powershell
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
.\build\Release\modern-video-player.exe --1080p60-check <media_file> 2000
.\build\Release\modern-video-player.exe --4k-playback-check <media_file> 2000
.\build\Release\modern-video-player.exe --high-bitrate-check <media_file> 2000
.\build\Release\modern-video-player.exe --long-playback-check <media_file> 10000
```

Interaction:

```powershell
.\build\Release\modern-video-player.exe --playlist-flow-check <media1> <media2> <media3> <media4> <media5>
.\build\Release\modern-video-player.exe --settings-persistence-check
.\build\Release\modern-video-player.exe --chapter-nav-check <media_file>
.\build\Release\modern-video-player.exe --ab-repeat-check <media_file>
.\build\Release\modern-video-player.exe --frame-step-check <media_file>
.\build\Release\modern-video-player.exe --delay-adjust-check <media_file> <subtitle.srt>
.\build\Release\modern-video-player.exe --numeric-seek-check <media_file>
.\build\Release\modern-video-player.exe --screenshot-check <media_file>
```

Subtitles:

```powershell
.\build\Release\modern-video-player.exe --subtitle-sync-check <subtitle.srt>
.\build\Release\modern-video-player.exe --subtitle-style-check <subtitle.ass>
.\build\Release\modern-video-player.exe --embedded-subtitle-check <media_file>
.\build\Release\modern-video-player.exe --embedded-subtitle-list <media_file>
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check <media_file> <stream_index>
.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check <media_file> zh,en auto avoid
.\build\Release\modern-video-player.exe --bitmap-subtitle-check <media_file> [stream_index]
.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check
.\build\Release\modern-video-player.exe --attachment-font-check <media_file>
.\build\Release\modern-video-player.exe --directwrite-font-collection-check <media_file>
```

Renderer/backend diagnostics:

```powershell
.\build\Release\modern-video-player.exe --renderer-fallback-check <media_file>
.\build\Release\modern-video-player.exe --windows-backend-check <media_file>
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --d3d11-hdr-output-check <media_file> 2000
.\build\Release\modern-video-player.exe --opengl-diagnostics
.\build\Release\modern-video-player.exe --opengl-output-color-check <media_file> <cube_lut_file> 2000
.\build\Release\modern-video-player.exe --opengl-output-color-icc-check <media_file> 2000
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```

Linux-specific checks:

```bash
./build/modern-video-player --linux-software-audio-check <media_file> 2000
./build/modern-video-player --linux-opengl-playback-check <media_file> 2000
./build/modern-video-player --linux-opengl-fallback-check <media_file> 2000
./build/modern-video-player --linux-vaapi-fallback-check <media_file> 2000
./build/modern-video-player --libass-shaping-check <subtitle.ass>
./build/modern-video-player --embedded-subtitle-live-packet-check <media_file> -1 120
./build/modern-video-player --linux-audio-backend-smoke <media_file> 2000
./build/modern-video-player --core-playback-behavior-check <media_file> 2000
./build/modern-video-player --ui-interaction-check <media_file> 2000
```

Infrastructure:

```powershell
.\build\Release\modern-video-player.exe --plugin-check
.\build\Release\modern-video-player.exe --streaming-buffer-check <playlist_url> 3 128
.\build\Release\modern-video-player.exe --adaptive-bitrate-check <manifest_url> 900000,3500000,1500000 2 128
```

Aggregate scripts:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_ci_gate.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4
```

```bash
bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800
```

## Output Locations

- Settings: `config/player_settings.ini`
- Logs: `logs/`
- Screenshots: `screenshots/`
- Local validation reports: `docs/reports/`
- Format samples: `tools/format_regression/format_samples.csv`
- Driver quirk samples: `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`

## Current TODOs

- Complete macOS build/playback/package path.
- Integrate streaming infrastructure into a full online playback path.
- Add user-facing plugin management and filter controls.
- Expand skin/theme system beyond the current skeleton.
- Keep collecting real GPU/driver samples for renderer fallback decisions.
