# Cross-Platform Startup Strategy and Capabilities Design
Date: 2026-03-25
Scope: CP-101 ~ CP-106
Status: Implemented Baseline

## 1. Objective
- Separate startup policy decisions from startup execution.
- Keep `PlayerCore` as a plan executor.
- Make startup decision chain machine-readable for diagnostics and regression.

## 2. Core Objects

### 2.1 `platform::PlatformCapabilities`
- Platform identity (`Windows/Linux/MacOS/Unknown`).
- Renderer capability list (`compiled_in`, `runtime_available`, `default_priority`).
- Decoder capability list (`compiled_in`, `runtime_available`, `hardware_accelerated`, `default_priority`).
- SDL baseline flags (`has_sdl_windowing`, `has_sdl_audio`).

### 2.2 `core::PlaybackOpenRequest`
- Media info and probed codec name.
- Playback preferences (`prefer_hardware_decode`, preferred renderer if provided).
- Current platform capabilities snapshot.

### 2.3 `core::PlaybackOpenPlan`
- Ordered renderer candidates.
- Ordered video decoder backend candidates.
- Plan reasons for renderer/decoder decision path.

## 3. Responsibility Boundaries
- `PlatformCapabilitiesProbe`: detect and materialize capability snapshot.
- `PlaybackStrategy`: generate ordered startup plan from request input.
- `RendererFactory`: only support check + name + renderer creation.
- `DecoderFactory`: context-driven backend ordering + backend naming.
- `PlayerCore`: execute renderer/decoder initialization using plan order and emit execution diagnostics.

## 4. Startup Flow
1. `Demuxer` open and media info collection.
2. Capability probe (`PlatformCapabilitiesProbe::detect()`).
3. Build request (`PlaybackOpenRequest`).
4. Build plan (`PlaybackStrategy::buildOpenPlan()`).
5. Try renderer candidates in order.
6. Try decoder candidates in order.
7. Export startup diagnostics snapshot.

## 5. Machine-Readable Diagnostics Fields
- `startup_platform`
- `startup_renderer_capabilities`
- `startup_decoder_capabilities`
- `startup_renderer_candidates`
- `startup_decoder_candidates`
- `startup_selected_renderer`
- `startup_selected_decoder`
- `startup_renderer_fallback_reason`
- `startup_decoder_fallback_reason`

These are exported via `DiagnosticsSnapshot` and surfaced by `--performance-log-check`.

## 6. Compatibility Notes
- Existing env controls remain honored in strategy resolution:
  - `MVP_RENDERER_BACKEND`
  - `MVP_D3D11_DRIVER_HINT`
- Windows default behavior remains equivalent (`D3D11` + hardware decode priority when available).

## 7. Deferred Work
- Deep runtime availability probing per backend/device (`CP-301+`).
- Linux/macOS feature switches and strict source-guard closure (`CP-301+`).
- Additional hardware backends (`VAAPI/VideoToolbox`) by extending capabilities and strategy inputs.
