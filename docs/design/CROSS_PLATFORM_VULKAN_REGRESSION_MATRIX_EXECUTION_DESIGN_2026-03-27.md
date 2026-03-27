# Cross-platform Vulkan regression matrix execution design (2026-03-27)

## 1. Goal
Deliver `VK-013`: execute and archive a regression matrix that proves Vulkan-chain integration did not regress core playback behaviors (`open/play/pause/seek/subtitle/fallback`) before final closure.

## 2. Scope
- In scope:
  - command-level matrix execution on current host (Windows)
  - machine-readable result capture from existing diagnostics/check commands
  - explicit platform-aware interpretation for Vulkan diagnostics outcome
- Out of scope:
  - new renderer feature implementation
  - Linux runtime proof replacement (still requires Linux runner evidence)

## 3. Matrix Design
- Build baseline:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin`
- Open/Play:
  - `--performance-log-check`
- Pause/Seek:
  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
- Subtitle:
  - `--subtitle-style-check`
  - `--subtitle-sync-check`
- Fallback:
  - `--renderer-fallback-check`
  - `MVP_RENDERER_BACKEND=vulkan --performance-log-check`
- Vulkan diagnostics:
  - `--vulkan-diagnostics`

## 4. Pass Criteria
- Each command exits successfully where platform/runtime supports it.
- Expected platform-limited cases are explicitly marked and justified:
  - On Windows host, `--vulkan-diagnostics` is expected to report unsupported/FAIL for Linux-first Vulkan path.
- Existing fallback observability remains visible and machine-readable.

## 5. Risk and Mitigation
- Risk: Windows host cannot provide Linux Vulkan runtime PASS proof.
- Mitigation:
  - treat Windows result as local regression guard only
  - keep Linux runner proof as follow-up requirement in report/records.
