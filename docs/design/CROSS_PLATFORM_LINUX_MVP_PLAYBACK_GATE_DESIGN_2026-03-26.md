# Cross-Platform Linux MVP Playback Gate Design
Date: 2026-03-26
Scope: CP-401 ~ CP-406
Status: Implemented Baseline

## 1. Objective
- Close Linux MVP playback baseline with machine-readable command gates.
- Keep fallback behavior deterministic and observable (`OpenGL -> SoftwareSDL`).
- Make Linux audio backend coverage reproducible across PulseAudio/ALSA/PipeWire.

## 2. Command Contract
Added dedicated commands:
- `--linux-software-audio-check <media_file> [sample_ms]`
- `--linux-opengl-playback-check <media_file> [sample_ms]`
- `--linux-opengl-fallback-check <media_file> [sample_ms]`
- `--linux-audio-backend-smoke <media_file> [sample_ms]`
- `--core-playback-behavior-check <media_file> [sample_ms]`
- `--ui-interaction-check <media_file> [sample_ms]`

Each command prints `key=value` diagnostics and exits with:
- `0`: PASS
- `2`: FAIL (check failed)
- `1`: CLI argument error

## 3. OpenGL Fallback Injection Design
- New environment variable:
  - `MVP_OPENGL_FORCE_INIT_FAIL`
- Injection point:
  - `OpenGLVideoRenderer::Impl::init(...)` early return before SDL init.
- Accepted truthy values:
  - `1/true/yes/on/force`

This guarantees fallback checks are not dependent on incidental runtime failures.

## 4. Linux Audio Backend Smoke Strategy
- Enumerate runtime SDL audio drivers with `SDL_GetNumAudioDrivers` / `SDL_GetAudioDriver`.
- Target matrix:
  - `pulseaudio`
  - `alsa`
  - `pipewire`
- For each available target driver:
  - force `SDL_AUDIODRIVER=<driver>`
  - force `SoftwareSDL` renderer + audio enabled
  - verify audio init strategy/output/decode/render counters

Unavailable drivers are marked `SKIP`; available drivers must all pass.

## 5. UI and Core Behavior Gate Strategy
- Core behavior gate (`CP-405`):
  - open -> play -> pause -> seek -> resume -> stop
  - validates state transitions + decode/render progress + runtime stability counters.
- UI interaction gate (`CP-406`):
  - inject fullscreen toggle, resize events, and `SDL_DROPFILE`
  - validates drag-drop request consumption and no freeze/regression counters.

## 6. Scripted Linux Gate
- New script:
  - `tools/run_linux_mvp_checks.sh`
- Runs six checks in sequence and fails fast on first non-zero exit.
- Intended as Phase-4 baseline for future Linux CI integration (`CP-903/CP-904`).
