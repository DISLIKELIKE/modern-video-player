# CROSS PLATFORM MASTER TASKLIST
Date: 2026-03-26
Scope: Windows + Linux (current execution scope), macOS explicitly deferred
Owner: modern-video-player

## 1. Goal
- Provide one executable master checklist for cross-platform delivery.
- Keep Windows stable while landing Linux as the first production-grade second platform.
- Use unified strategy/fallback architecture so new platform backends do not keep modifying `PlayerCore`.

## 2. Execution Rules
- Follow `docs/workflows/WORKFLOW.md`.
- For substantial implementation, create/update implementation planner before coding.
- Every landed item must sync:
  - `docs/records/CHANGELOG.md`
  - `docs/records/VERSION.md`
  - `docs/records/DEVELOP_LOG.md`
  - matching `analysis/design/report/plans` docs
- Local build + validation must pass before merge/showcase.
- Do not commit/push without explicit user confirmation.

## 3. Status Legend
- `DONE`: already landed and validated in repository.
- `NEXT`: priority work to execute now.
- `LATER`: planned backlog after previous phase gates pass.

## 4. Milestone Gates
- `M1` Architecture clean: strategy extracted from core.
- `M2` Linux MVP playable: software decode + SDL audio + OpenGL/SDL render + text subtitles.
- `M3` Unified policy stable across Windows/Linux.
- `M4` Linux hw decode path (VAAPI) available with stable fallback.
- `M5` Color/HDR pipeline and packaging/CI converge.

## 5. Task Matrix

### Phase 0: Baseline and Boundary Freeze (`M1` prework)
- [x] `CP-001` (`DONE`) Freeze platform matrix and MVP acceptance:
  - Windows x64 (mainline), Linux x64 (first second-platform target)
  - Linux MVP: local file playback, audio/video/subtitle, seek/pause
- [x] `CP-002` (`DONE`) Freeze sample matrix:
  - H.264/H.265/10bit, ASS/SSA/SRT/WebVTT, PGS/DVD subtitle samples
- [x] `CP-003` (`DONE`) Freeze current platform-coupling inventory:
  - D3D11/DXGI/D2D/DirectWrite/WASAPI/WGL-specific code locations
- [x] `CP-004` (`DONE`) Freeze regression command matrix:
  - Windows gate and Linux gate command parity
- [x] `CP-005` (`DONE`) Create risk map:
  - OpenGL event freeze, subtitle/font ownership, fallback behavior

Acceptance:
- One frozen inventory doc for coupling points.
- One frozen media/regression sample list.

### Phase 1: Strategy and Capability Extraction (`M1`)
- [x] `CP-101` (`DONE`) Add `platform_capabilities` abstraction.
- [x] `CP-102` (`DONE`) Add `playback_strategy` abstraction with renderer/decoder candidate chains.
- [x] `CP-103` (`DONE`) Refactor `RendererFactory`:
  - keep creation/support checks only
  - remove default platform policy ownership
- [x] `CP-104` (`DONE`) Refactor `DecoderFactory`:
  - context-based candidate ordering
  - keep software fallback mandatory
- [x] `CP-105` (`DONE`) Refactor `PlayerCore::open()`:
  - consume strategy plan
  - remove hardcoded platform order logic
- [x] `CP-106` (`DONE`) Add machine-readable startup strategy diagnostics:
  - capabilities
  - renderer/decoder candidates
  - selected path + fallback reason

Key files:
- `include/platform/platform_capabilities.h` (new)
- `src/platform/platform_capabilities.cpp` (new)
- `include/core/playback_strategy.h` (new)
- `src/core/playback_strategy.cpp` (new)
- `include/render/renderer_factory.h`
- `src/render/renderer_factory.cpp`
- `include/decoder/decoder_factory.h`
- `src/decoder/decoder_factory.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`

Acceptance:
- `PlayerCore` no longer decides default platform backend directly.
- Windows default behavior remains equivalent.

### Phase 2: Renderer/Display/Input Responsibility Cleanup (`M1 -> M2`)
- [x] `CP-201` (`DONE`) Shrink `video_renderer` API to rendering concerns.
- [x] `CP-202` (`DONE`) Move playback command/input concerns out of renderer implementations.
- [x] `CP-203` (`DONE`) Keep SDL event pump/window state transitions on main-thread paths only.
- [x] `CP-204` (`DONE`) Split display responsibilities progressively:
  - window/events
  - overlay/ui rendering
  - present pipeline
- [x] `CP-205` (`DONE`) Add interaction-freeze stress checks:
  - mouse move/click
  - hotkeys
  - maximize/minimize/fullscreen

Acceptance:
- OpenGL path no longer regresses into event-thread deadlocks.
- Input/control behavior parity between renderer backends improves.

### Phase 3: Build and Dependency Cross-Platformization (`M2`)
- [x] `CP-301` (`DONE`) Add explicit backend feature switches in CMake:
  - `ENABLE_D3D11_RENDERER`
  - `ENABLE_OPENGL_RENDERER`
  - `ENABLE_SDL_RENDERER`
  - `ENABLE_D3D11VA`
  - `ENABLE_DXVA2`
  - `ENABLE_VAAPI`
  - `ENABLE_VIDEOTOOLBOX`
- [x] `CP-302` (`DONE`) Enforce strict platform guards:
  - Windows-only code never compiled on Linux/macOS
- [x] `CP-303` (`DONE`) Linux dependency closure:
  - SDL2, FFmpeg, OpenGL/EGL(or GLX), libass, fontconfig, freetype
- [x] `CP-304` (`DONE`) Startup diagnostics print compiled backend set and runtime-available set.
- [x] `CP-305` (`DONE`) Add Linux packaging baseline path:
  - AppImage or deb first

Acceptance:
- Linux build path is first-class, not fallback compile mode.
- Runtime/backend availability is explicit and observable.

### Phase 4: Linux MVP Playback Path (`M2`)
- [x] `CP-401` (`DONE`) Linux software decode + SDL audio path stable.
- [x] `CP-402` (`DONE`) Linux OpenGL renderer path stable.
- [x] `CP-403` (`DONE`) Fallback chain stabilized:
  - `OpenGL -> SoftwareSDL`
- [x] `CP-404` (`DONE`) Audio backend coverage on Linux:
  - PulseAudio / ALSA / PipeWire smoke runs
- [x] `CP-405` (`DONE`) Core playback behaviors:
  - open/play/pause/seek/resume/stop
- [x] `CP-406` (`DONE`) UI interactions:
  - drag-drop, fullscreen toggle, resize stability

Acceptance:
- Linux can play local media with usable baseline UX.

### Phase 5: Subtitle and Font Platform Closure (`M2 -> M3`)
- [x] `CP-501` (`DONE`) Multi embedded subtitle track policy completion:
  - language preference
  - forced/SDH policy
  - persistence strategy
- [x] `CP-502` (`DONE`) Subtitle track UI completion:
  - clear track list, current selection, switch feedback
- [x] `CP-503` (`DONE`) CLI completion for subtitle control:
  - list/select/check commands stable
- [x] `CP-504` (`DONE`) Ownership policy hardening:
  - `external > embedded`
  - clear external falls back to embedded
- [x] `CP-505` (`DONE`) Linux attachment-font closure:
  - extract/register/cleanup flow on Linux path
- [x] `CP-506` (`DONE`) Improve DirectWrite path with custom font collection integration (Windows path maturation)
- [x] `CP-507` (`DONE`) Land Linux libass shaping/layout probe baseline:
  - `--libass-shaping-check <subtitle.ass|subtitle.ssa>`
  - machine-readable pass/fail signals for libass init/track/event/render image output
- [x] `CP-508` (`DONE`) Land embedded subtitle live packet probe baseline:
  - `--embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
  - packet-level monotonic/output diagnostics for text/bitmap subtitle codecs

Acceptance:
- Embedded subtitle + attachment font behavior is deterministic across supported platforms.

### Phase 6: Bitmap Subtitle Pipeline (`M3`)
- [x] `CP-601` (`DONE`) PGS decode/model timeline closure.
- [x] `CP-602` (`DONE`) DVD subtitle decode/model timeline closure.
- [x] `CP-603` (`DONE`) Bitmap subtitle sync and composition integration in renderer path.
- [x] `CP-604` (`DONE`) Bitmap texture cache/reuse/upload policy optimization.
- [x] `CP-605` (`DONE`) Multi-rect composition stress regression.

Acceptance:
- PGS/DVD playback is functionally stable and test-covered.

### Phase 7: Linux Hardware Decode (`M4`)
- [x] `CP-701` (`DONE`) Add unified `hw_device_factory`.
- [x] `CP-702` (`DONE`) Add Linux VAAPI capability probing in platform capabilities.
- [x] `CP-703` (`DONE`) Land `VAAPI -> Software` fallback chain.
- [x] `CP-704` (`DONE`) OpenGL + VAAPI integration first with stable copy-back.
- [x] `CP-705` (`DONE`) Evaluate zero-copy (`dmabuf/EGL`) after stable baseline.

Acceptance:
- Linux has at least one production-usable hardware decode path with safe fallback.

### Phase 8: Display-Level HDR and Color Management (`M5`)
- [x] `CP-801` (`DONE`) Display-level HDR present bridge completion (Windows DXGI path).
- [x] `CP-802` (`DONE`) ICC/profile-driven LUT generation pipeline.
- [x] `CP-803` (`DONE`) Per-display LUT selection/update strategy.
- [x] `CP-804` (`DONE`) Output diagnostics closure for HDR/ICC/LUT runtime states.
- [x] `CP-805` (`DONE`) OpenGL output color stage regression matrix (SDR/HDR/LUT permutations).

Acceptance:
- HDR and color management are no longer policy-only; display/output behavior is deterministic and measurable.

### Phase 9: Observability, Regression, and CI (`M5`)
- [x] `CP-901` (`DONE`) Expand driver quirk sample library.
- [x] `CP-902` (`DONE`) Unify runtime counters:
  - frame queue
  - decode wait
  - render wait
  - present wait
  - drop reasons
- [x] `CP-903` (`DONE`) Linux gate script parity with Windows gate.
- [x] `CP-904` (`DONE`) CI matrix:
  - Windows build + gate
  - Linux build + gate
- [x] `CP-905` (`DONE`) Release packaging/readiness checklist for dual-platform delivery.

Acceptance:
- Cross-platform regressions can be blocked automatically in CI.

### Phase 10: macOS Third-Stage Platform (`LATER`)
Note: deferred by current scope decision (2026-03-26), no implementation work in this round.
- [ ] `CP-1001` (`LATER`) macOS compile baseline.
- [ ] `CP-1002` (`LATER`) macOS software decode playback baseline.
- [ ] `CP-1003` (`LATER`) Strategy path for `VideoToolbox -> Software`.
- [ ] `CP-1004` (`LATER`) Renderer path decision (OpenGL transition or Metal target).
- [ ] `CP-1005` (`LATER`) macOS CI smoke + package baseline.

Acceptance:
- macOS is integrated by extending the same strategy/capability framework instead of new core forks.

## 6. Immediate Recommended Execution Order
1. Execute full Linux gate (`tools/run_linux_mvp_checks.sh`) on a real Linux host and archive results.
2. Monitor CI adoption of `.github/workflows/cross-platform-gate.yml`:
  - Linux lane now runs strict optional checks (`CP-507` / `CP-508`) with auto-generated CP-508 media fixture.
  - Windows/Linux lanes now auto-generate probe media when missing (`samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4`).
  - Build stage now includes `sample_logger_plugin` target required by package install path.
  - Linux lane now archives machine-readable gate summary: `logs/linux-mvp-gate-summary.env`.
  - Archive and review Linux gate artifacts from the first green run.
3. Keep appending real adapter samples into `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`.

## 7. Command Gate Baseline
- Linux MVP gate:
  - `bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800`
  - Strict optional-check mode (recommended for CI):
    - `bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800 samples/subtitles/opengl_ass_style_validation.ass build/tmp/embedded-ass-validation.mkv 120 1 ./juren-30s.mp4 samples/subtitles/opengl_ass_transform_transition_validation.ass`
  - Strict mode + machine-readable report output:
    - `bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800 samples/subtitles/opengl_ass_style_validation.ass build/tmp/embedded-ass-validation.mkv 120 1 ./juren-30s.mp4 samples/subtitles/opengl_ass_transform_transition_validation.ass logs/linux-mvp-gate-summary.env`
- Linux Phase4 single-command checks:
  - `./build/modern-video-player --linux-software-audio-check <media_file> [sample_ms]`
  - `./build/modern-video-player --linux-opengl-playback-check <media_file> [sample_ms]`
  - `./build/modern-video-player --linux-opengl-fallback-check <media_file> [sample_ms]`
  - `./build/modern-video-player --linux-vaapi-fallback-check <media_file> [sample_ms]`
  - `./build/modern-video-player --libass-shaping-check <subtitle.ass|subtitle.ssa>`
  - `./build/modern-video-player --embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
  - `./build/modern-video-player --linux-audio-backend-smoke <media_file> [sample_ms]`
  - `./build/modern-video-player --core-playback-behavior-check <media_file> [sample_ms]`
  - `./build/modern-video-player --ui-interaction-check <media_file> [sample_ms]`
- Windows OpenGL gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- OpenGL diagnostics:
  - `.\build\Release\modern-video-player.exe --opengl-diagnostics`
- D3D11 diagnostics:
  - `.\build\Release\modern-video-player.exe --d3d11-diagnostics`
- D3D11 HDR output check:
  - `.\build\Release\modern-video-player.exe --d3d11-hdr-output-check <media_file> [sample_ms]`
- Driver quirk sample capture:
  - `powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -OutputCsvPath "docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv"`
- Windows packaging:
  - `powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release`
- CI workflow:
  - `.github/workflows/cross-platform-gate.yml`
- Embedded subtitle checks:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-check <media_file>`
  - `.\build\Release\modern-video-player.exe --bitmap-subtitle-check <media_file> [stream_index]`
  - `.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list <media_file>`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check <media_file> <stream_index>`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check <media_file> [lang_list] [forced_policy] [sdh_policy]`
  - `.\build\Release\modern-video-player.exe --subtitle-ownership-check <media_file>`
- Font closure checks:
  - `.\build\Release\modern-video-player.exe --attachment-font-check <media_file>`
  - `.\build\Release\modern-video-player.exe --directwrite-font-collection-check <media_file>`
- Output color check:
  - `.\build\Release\modern-video-player.exe --opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`
  - `.\build\Release\modern-video-player.exe --opengl-output-color-icc-check <media_file> [sample_ms]`

## 8. Related Plan Docs
- `docs/plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md`
- `docs/plans/CROSS_PLATFORM_REFACTOR_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
