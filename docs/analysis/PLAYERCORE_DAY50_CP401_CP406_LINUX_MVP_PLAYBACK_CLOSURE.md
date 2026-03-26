# PLAYERCORE DAY50: CP-401 ~ CP-406 Linux MVP Playback Closure
Date: 2026-03-26
Status: Done

## 1. Problem Statement
- Phase-4 Linux MVP tasks (`CP-401` to `CP-406`) still lacked an end-to-end machine-readable gate path.
- Only partial `CP-401` scaffolding had landed in `main.cpp`; no complete CLI command set or command dispatch existed.
- No deterministic OpenGL initialization-fail injection existed to verify `OpenGL -> SoftwareSDL` fallback behavior.
- Linux audio backend smoke coverage and Linux gate script were not implemented.

## 2. Root Cause
- The existing validation commands were rich but mostly Windows/OpenGL-oriented, while Linux MVP closure needed dedicated command contracts.
- Fallback-chain validation relied on passive failures instead of a controlled failure-injection hook.
- Phase4 planning/doc record chain existed, but implementation and verification artifacts were not yet connected.

## 3. Implementation Planner Used
1. Complete dedicated Linux MVP command implementations in `src/main.cpp`.
2. Add OpenGL init fail injection controlled by `MVP_OPENGL_FORCE_INIT_FAIL`.
3. Add `tools/run_linux_mvp_checks.sh` to run `CP-401~CP-406` as a single gate.
4. Update tasklist + analysis/design/report + records docs consistently.
5. Build and run local validation on current environment.

## 4. Landed Changes
- Added Linux MVP check commands in `src/main.cpp`:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added command usage lines and CLI argument dispatch for all above commands.
- Added OpenGL force-fail injection in `src/render/opengl_video_renderer.cpp`:
  - `MVP_OPENGL_FORCE_INIT_FAIL=1` forces `OpenGLVideoRenderer::Impl::init(...)` to fail early.
- Added Linux gate script:
  - `tools/run_linux_mvp_checks.sh`

## 5. Result Against CP IDs
- `CP-401`: done (`SoftwareSDL + Software decode + SDL audio` Linux check command complete).
- `CP-402`: done (Linux OpenGL playback path check command complete).
- `CP-403`: done (controlled OpenGL fail injection + fallback check command complete).
- `CP-404`: done (PulseAudio/ALSA/PipeWire smoke command complete).
- `CP-405`: done (open/play/pause/seek/resume/stop core behavior check command complete).
- `CP-406`: done (drag-drop + fullscreen toggle + resize stability check command complete).

## 6. Validation
- Build:
  - `cmake -S . -B build`
  - `cmake --build build --config Release --target modern-video-player`
- Windows regression sanity:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4`
  - `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800`
- Linux Phase4 gate command baseline:
  - `bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800`

## 7. Remaining Risks
- Current local environment is Windows; Linux-only pass criteria (`startup_platform=Linux`) cannot be fully validated here.
- `tools/run_linux_mvp_checks.sh` executable bit and end-to-end result still require Linux host verification.
