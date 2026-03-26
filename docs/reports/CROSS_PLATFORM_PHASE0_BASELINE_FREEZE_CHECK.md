# CROSS PLATFORM PHASE0 BASELINE FREEZE CHECK

Date: 2026-03-26  
Scope: `CP-001 ~ CP-005`

## 1. Platform Matrix (`CP-001`)
- In-scope (current execution): `Windows x64`, `Linux x64`
- Out-of-scope (deferred): `macOS`
- Linux MVP acceptance baseline:
  - local media playback
  - audio/video/subtitle
  - seek/pause/resume/stop

## 2. Sample Matrix (`CP-002`)
- Video:
  - H.264 / H.265 / 10-bit variants
- Subtitle:
  - ASS/SSA, SRT, WebVTT, PGS, DVD subtitle samples
- Validation media references:
  - `samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4`
  - bitmap/embedded subtitle validation samples used in prior phase reports

## 3. Platform-Coupling Inventory (`CP-003`)
- Windows-coupled surfaces:
  - D3D11/DXGI present and diagnostics
  - DirectWrite / D2D subtitle/font paths
  - WASAPI audio endpoint behavior
  - WGL interop path in OpenGL renderer
- Linux-focused surfaces:
  - SDL audio driver matrix (PulseAudio/ALSA/PipeWire)
  - OpenGL + SoftwareSDL fallback
  - VAAPI runtime capability probing and fallback chain

## 4. Regression Command Matrix (`CP-004`)
- Windows gate:
  - `tools/run_opengl_checks.ps1`
  - `--d3d11-diagnostics`
  - `--d3d11-hdr-output-check`
  - `--performance-log-check`
- Linux gate:
  - `tools/run_linux_mvp_checks.sh`
  - includes `--linux-software-audio-check`
  - includes `--linux-opengl-playback-check`
  - includes `--linux-opengl-fallback-check`
  - includes `--linux-vaapi-fallback-check`
  - includes `--linux-audio-backend-smoke`

## 5. Risk Map (`CP-005`)
- R1: Linux host runtime evidence gap on Windows workstation.
  - Impact: cannot claim Linux runtime closure locally.
  - Mitigation: run Linux gate in CI Linux runner / Linux host.
- R2: VAAPI runtime availability depends on host GPU/driver stack.
  - Impact: backend may fall back to software on unsupported nodes.
  - Mitigation: keep deterministic `VAAPI -> Software` path and diagnostics.
- R3: Zero-copy (`dmabuf/EGL`) not implemented this round.
  - Impact: performance ceiling not yet reached for Linux hw decode.
  - Mitigation: keep copy-back baseline stable first, then evaluate zero-copy phase.

## 6. Result
- `CP-001 ~ CP-005`: baseline frozen and documented.
