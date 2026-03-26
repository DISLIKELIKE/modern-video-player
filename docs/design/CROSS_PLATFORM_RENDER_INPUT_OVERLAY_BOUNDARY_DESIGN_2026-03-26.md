# Cross-Platform Renderer/Input/Overlay Boundary Design
Date: 2026-03-26
Scope: CP-201 ~ CP-205
Status: Implemented Baseline

## 1. Objective
- Keep renderer abstraction focused on frame rendering and present pipeline.
- Split player command/input collection from renderer rendering APIs.
- Split overlay/subtitle state sink from renderer rendering APIs.
- Enforce event-pump thread affinity constraints explicitly.

## 2. Interface Boundaries

### 2.1 `render::IVideoRenderer`
- Owns rendering-only lifecycle and frame submission:
  - `init/close`
  - `renderFrame/present/clear`
  - diagnostics/native format capability helpers
- Does not own player command polling semantics.
- Does not own subtitle/overlay state command semantics.

### 2.2 `input::IPlaybackInputSource`
- Owns event pumping and command request consumption:
  - `handleEvents/shouldQuit`
  - input command consume methods (seek, volume, speed, subtitle toggle, AB repeat, chapter, playlist/file request)
  - `setHotkeyManager`
- Implemented by renderer backends that host SDL event surfaces.

### 2.3 `render::IRenderOverlaySink`
- Owns overlay/subtitle state sink responsibilities:
  - `setOverlayState`
  - `setSubtitleClock`
  - `setSubtitleText` / `setSubtitleItems`
  - `setSubtitleTrackState`
- Allows `PlayerCore` subtitle/OSD logic to remain backend-agnostic.

## 3. Core Wiring
1. `PlayerCore::open()` creates renderer backend.
2. `PlayerCore` cross-casts selected renderer to:
   - `IPlaybackInputSource`
   - `IRenderOverlaySink`
3. `pumpEvents()` consumes input requests only through `IPlaybackInputSource`.
4. Subtitle/overlay update path emits state only through `IRenderOverlaySink`.

## 4. Thread-Affinity Contract
- SDL event pump must execute on renderer/event owner thread (main path in current architecture).
- Guards are added in:
  - `PlayerCore::pumpEvents()`
  - `Display::handleEvents()`
  - `D3D11VideoRenderer::handleEvents()`
  - `OpenGLVideoRenderer::handleEvents()`
- On violation: log once and ignore call (fail-safe behavior; no hard crash/assert).

## 5. Idle Window Contract
- Idle startup window now depends only on `IPlaybackInputSource`, not renderer concrete control API.
- This keeps drag-drop/open-file behavior decoupled from backend-specific renderer types.

## 6. Regression Gate Contract
- Added machine-readable command:
  - `--interaction-freeze-check <media_file> [sample_ms]`
- Check injects mouse/hotkey/window events and asserts render-frame progress + no fail-session/illegal-transition growth.
- OpenGL gate script now includes this check as required pass item.

## 7. Deferred Work
- Compile-time backend switches and strict platform source guards (`CP-301/302`).
- Linux dependency closure and package baseline (`CP-303/305`).
- Linux runtime gate parity and CI matrix expansion (`CP-903/904`).
