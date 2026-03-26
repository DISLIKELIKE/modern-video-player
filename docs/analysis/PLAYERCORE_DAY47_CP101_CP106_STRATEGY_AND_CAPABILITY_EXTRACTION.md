# PLAYERCORE DAY47: CP-101 ~ CP-106 Strategy and Capability Extraction
Date: 2026-03-25
Status: Done

## 1. Problem Statement
- `PlayerCore::open()` still mixed execution logic with policy logic.
- `RendererFactory` and `DecoderFactory` still owned default platform policy.
- Startup decision data was not exported as machine-readable diagnostics.
- This blocked Linux-first expansion because backend order assumptions were still hardcoded in core paths.

## 2. Root Cause
- Platform capability detection had no unified model.
- Playback plan generation had no dedicated strategy layer.
- Decoder selection only consumed narrow inputs (`codec + prefer_hardware`) and could not evolve safely.
- Startup diagnostics only exposed selected backends, not decision context.

## 3. Implementation Planner Used
1. Add `platform_capabilities` abstraction and probe.
2. Add `playback_strategy` abstraction and open-plan builder.
3. Refactor `RendererFactory` to support/check/create responsibilities only.
4. Refactor `DecoderFactory` to context-driven ordering with software mandatory fallback.
5. Refactor `PlayerCore::open()` and decoder init to consume strategy plan.
6. Export startup strategy diagnostics through `DiagnosticsSnapshot` and `--performance-log-check`.

## 4. Landed Changes
- Added:
  - `include/platform/platform_capabilities.h`
  - `src/platform/platform_capabilities.cpp`
  - `include/core/playback_strategy.h`
  - `src/core/playback_strategy.cpp`
- Refactored:
  - `include/render/renderer_factory.h`
  - `src/render/renderer_factory.cpp`
  - `include/decoder/decoder_factory.h`
  - `src/decoder/decoder_factory.cpp`
  - `include/core/player_core.h`
  - `src/core/player_core.cpp`
  - `src/main.cpp`
  - `CMakeLists.txt`

## 5. Result Against CP IDs
- `CP-101`: done (`platform_capabilities` abstraction + probe).
- `CP-102`: done (`playback_strategy` with renderer/decoder candidate chains).
- `CP-103`: done (`RendererFactory` no longer decides default policy).
- `CP-104`: done (`DecoderFactory` context-driven ordering + software fallback).
- `CP-105`: done (`PlayerCore::open()` executes strategy plan).
- `CP-106`: done (machine-readable startup strategy diagnostics exported).

## 6. Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player`
- Runtime checks:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4`
- Key outcomes:
  - Build succeeded.
  - `performance-log-check.result=PASS`.
  - `renderer-fallback-check.result=PASS`.
  - Startup diagnostics now expose:
    - platform capabilities
    - renderer/decoder candidates
    - selected renderer/decoder
    - fallback reasons

## 7. Remaining Risks
- Linux/macOS compile-time feature closure is still pending `CP-301+` (explicit backend switches and strict source guards).
- Current runtime availability probe is intentionally conservative (`runtime_available` currently tracks compiled/runtime baseline, not deep device probing).
- Existing repository still has legacy source-encoding warnings unrelated to this change set.
