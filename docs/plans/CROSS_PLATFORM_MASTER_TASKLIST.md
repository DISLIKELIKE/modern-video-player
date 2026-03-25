# CROSS PLATFORM MASTER TASKLIST
Date: 2026-03-25
Scope: Windows + Linux first, macOS later
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
- [ ] `CP-001` (`NEXT`) Freeze platform matrix and MVP acceptance:
  - Windows x64 (mainline), Linux x64 (first second-platform target)
  - Linux MVP: local file playback, audio/video/subtitle, seek/pause
- [ ] `CP-002` (`NEXT`) Freeze sample matrix:
  - H.264/H.265/10bit, ASS/SSA/SRT/WebVTT, PGS/DVD subtitle samples
- [ ] `CP-003` (`NEXT`) Freeze current platform-coupling inventory:
  - D3D11/DXGI/D2D/DirectWrite/WASAPI/WGL-specific code locations
- [ ] `CP-004` (`NEXT`) Freeze regression command matrix:
  - Windows gate and Linux gate command parity
- [ ] `CP-005` (`NEXT`) Create risk map:
  - OpenGL event freeze, subtitle/font ownership, fallback behavior

Acceptance:
- One frozen inventory doc for coupling points.
- One frozen media/regression sample list.

### Phase 1: Strategy and Capability Extraction (`M1`)
- [ ] `CP-101` (`NEXT`) Add `platform_capabilities` abstraction.
- [ ] `CP-102` (`NEXT`) Add `playback_strategy` abstraction with renderer/decoder candidate chains.
- [ ] `CP-103` (`NEXT`) Refactor `RendererFactory`:
  - keep creation/support checks only
  - remove default platform policy ownership
- [ ] `CP-104` (`NEXT`) Refactor `DecoderFactory`:
  - context-based candidate ordering
  - keep software fallback mandatory
- [ ] `CP-105` (`NEXT`) Refactor `PlayerCore::open()`:
  - consume strategy plan
  - remove hardcoded platform order logic
- [ ] `CP-106` (`NEXT`) Add machine-readable startup strategy diagnostics:
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
- [ ] `CP-201` (`NEXT`) Shrink `video_renderer` API to rendering concerns.
- [ ] `CP-202` (`NEXT`) Move playback command/input concerns out of renderer implementations.
- [ ] `CP-203` (`NEXT`) Keep SDL event pump/window state transitions on main-thread paths only.
- [ ] `CP-204` (`NEXT`) Split display responsibilities progressively:
  - window/events
  - overlay/ui rendering
  - present pipeline
- [ ] `CP-205` (`NEXT`) Add interaction-freeze stress checks:
  - mouse move/click
  - hotkeys
  - maximize/minimize/fullscreen

Acceptance:
- OpenGL path no longer regresses into event-thread deadlocks.
- Input/control behavior parity between renderer backends improves.

### Phase 3: Build and Dependency Cross-Platformization (`M2`)
- [ ] `CP-301` (`NEXT`) Add explicit backend feature switches in CMake:
  - `ENABLE_D3D11_RENDERER`
  - `ENABLE_OPENGL_RENDERER`
  - `ENABLE_SDL_RENDERER`
  - `ENABLE_D3D11VA`
  - `ENABLE_DXVA2`
  - `ENABLE_VAAPI`
  - `ENABLE_VIDEOTOOLBOX`
- [ ] `CP-302` (`NEXT`) Enforce strict platform guards:
  - Windows-only code never compiled on Linux/macOS
- [ ] `CP-303` (`NEXT`) Linux dependency closure:
  - SDL2, FFmpeg, OpenGL/EGL(or GLX), libass, fontconfig, freetype
- [ ] `CP-304` (`NEXT`) Startup diagnostics print compiled backend set and runtime-available set.
- [ ] `CP-305` (`NEXT`) Add Linux packaging baseline path:
  - AppImage or deb first

Acceptance:
- Linux build path is first-class, not fallback compile mode.
- Runtime/backend availability is explicit and observable.

### Phase 4: Linux MVP Playback Path (`M2`)
- [ ] `CP-401` (`NEXT`) Linux software decode + SDL audio path stable.
- [ ] `CP-402` (`NEXT`) Linux OpenGL renderer path stable.
- [ ] `CP-403` (`NEXT`) Fallback chain stabilized:
  - `OpenGL -> SoftwareSDL`
- [ ] `CP-404` (`NEXT`) Audio backend coverage on Linux:
  - PulseAudio / ALSA / PipeWire smoke runs
- [ ] `CP-405` (`NEXT`) Core playback behaviors:
  - open/play/pause/seek/resume/stop
- [ ] `CP-406` (`NEXT`) UI interactions:
  - drag-drop, fullscreen toggle, resize stability

Acceptance:
- Linux can play local media with usable baseline UX.

### Phase 5: Subtitle and Font Platform Closure (`M2 -> M3`)
- [ ] `CP-501` (`NEXT`) Multi embedded subtitle track policy completion:
  - language preference
  - forced/SDH policy
  - persistence strategy
- [ ] `CP-502` (`NEXT`) Subtitle track UI completion:
  - clear track list, current selection, switch feedback
- [ ] `CP-503` (`NEXT`) CLI completion for subtitle control:
  - list/select/check commands stable
- [ ] `CP-504` (`NEXT`) Ownership policy hardening:
  - `external > embedded`
  - clear external falls back to embedded
- [ ] `CP-505` (`NEXT`) Linux attachment-font closure:
  - extract/register/cleanup flow on Linux path
- [ ] `CP-506` (`NEXT`) Improve DirectWrite path with custom font collection integration (Windows path maturation)
- [ ] `CP-507` (`LATER`) Fuller libass shaping/layout parity backlog
- [ ] `CP-508` (`LATER`) Live subtitle packet path backlog

Acceptance:
- Embedded subtitle + attachment font behavior is deterministic across supported platforms.

### Phase 6: Bitmap Subtitle Pipeline (`M3`)
- [ ] `CP-601` (`NEXT`) PGS decode/model timeline closure.
- [ ] `CP-602` (`NEXT`) DVD subtitle decode/model timeline closure.
- [ ] `CP-603` (`NEXT`) Bitmap subtitle sync and composition integration in renderer path.
- [ ] `CP-604` (`NEXT`) Bitmap texture cache/reuse/upload policy optimization.
- [ ] `CP-605` (`NEXT`) Multi-rect composition stress regression.

Acceptance:
- PGS/DVD playback is functionally stable and test-covered.

### Phase 7: Linux Hardware Decode (`M4`)
- [ ] `CP-701` (`LATER`) Add unified `hw_device_factory`.
- [ ] `CP-702` (`LATER`) Add Linux VAAPI capability probing in platform capabilities.
- [ ] `CP-703` (`LATER`) Land `VAAPI -> Software` fallback chain.
- [ ] `CP-704` (`LATER`) OpenGL + VAAPI integration first with stable copy-back.
- [ ] `CP-705` (`LATER`) Evaluate zero-copy (`dmabuf/EGL`) after stable baseline.

Acceptance:
- Linux has at least one production-usable hardware decode path with safe fallback.

### Phase 8: Display-Level HDR and Color Management (`M5`)
- [ ] `CP-801` (`NEXT`) Display-level HDR present bridge completion (Windows DXGI path).
- [ ] `CP-802` (`NEXT`) ICC/profile-driven LUT generation pipeline.
- [ ] `CP-803` (`NEXT`) Per-display LUT selection/update strategy.
- [ ] `CP-804` (`NEXT`) Output diagnostics closure for HDR/ICC/LUT runtime states.
- [ ] `CP-805` (`NEXT`) OpenGL output color stage regression matrix (SDR/HDR/LUT permutations).

Acceptance:
- HDR and color management are no longer policy-only; display/output behavior is deterministic and measurable.

### Phase 9: Observability, Regression, and CI (`M5`)
- [ ] `CP-901` (`NEXT`) Expand driver quirk sample library.
- [ ] `CP-902` (`NEXT`) Unify runtime counters:
  - frame queue
  - decode wait
  - render wait
  - present wait
  - drop reasons
- [ ] `CP-903` (`NEXT`) Linux gate script parity with Windows gate.
- [ ] `CP-904` (`NEXT`) CI matrix:
  - Windows build + gate
  - Linux build + gate
- [ ] `CP-905` (`NEXT`) Release packaging/readiness checklist for dual-platform delivery.

Acceptance:
- Cross-platform regressions can be blocked automatically in CI.

### Phase 10: macOS Third-Stage Platform (`LATER`)
- [ ] `CP-1001` (`LATER`) macOS compile baseline.
- [ ] `CP-1002` (`LATER`) macOS software decode playback baseline.
- [ ] `CP-1003` (`LATER`) Strategy path for `VideoToolbox -> Software`.
- [ ] `CP-1004` (`LATER`) Renderer path decision (OpenGL transition or Metal target).
- [ ] `CP-1005` (`LATER`) macOS CI smoke + package baseline.

Acceptance:
- macOS is integrated by extending the same strategy/capability framework instead of new core forks.

## 6. Immediate Recommended Execution Order
1. Finish `CP-101` to `CP-106` (strategy/capabilities extraction).
2. Finish `CP-201` to `CP-205` (event/display responsibility cleanup).
3. Finish `CP-301` to `CP-406` (Linux build + MVP playback closure).
4. Finish `CP-501` to `CP-605` (subtitle/font/bitmap maturity).
5. Execute `CP-801` to `CP-905` (HDR/color + observability + CI closure).

## 7. Command Gate Baseline
- Windows OpenGL gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- OpenGL diagnostics:
  - `.\build\Release\modern-video-player.exe --opengl-diagnostics`
- D3D11 diagnostics:
  - `.\build\Release\modern-video-player.exe --d3d11-diagnostics`
- Embedded subtitle checks:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-check <media_file>`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list <media_file>`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check <media_file> <stream_index>`
- Output color check:
  - `.\build\Release\modern-video-player.exe --opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`

## 8. Related Plan Docs
- `docs/plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md`
- `docs/plans/CROSS_PLATFORM_REFACTOR_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
