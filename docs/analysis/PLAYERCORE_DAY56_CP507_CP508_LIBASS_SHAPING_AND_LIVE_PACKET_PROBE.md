# PLAYERCORE DAY56: CP-507 ~ CP-508 Linux Subtitle Backlog Closure
Date: 2026-03-26
Status: Done (Windows local build + command validation; Linux runtime verification pending)

## 1. Problem Statement
- Cross-platform master tasklist still had two Linux-scope subtitle backlog items open:
  - `CP-507`: fuller `libass` shaping/layout parity baseline
  - `CP-508`: live subtitle packet path baseline
- Existing subtitle flow focused on full-track load and lacked explicit machine-readable probe commands for:
  - Linux `libass` shaping/render pipeline readiness
  - packet-level embedded subtitle decode monotonicity/output checks

## 2. Root Cause
- There was no standalone `libass` probe command that could be executed in Linux gates.
- Embedded subtitle handling exposed full-track loading APIs, but no packet-level probe API/CLI to validate live packet behavior deterministically.
- Tasklist and command baseline therefore had no concrete closure artifacts for `CP-507`/`CP-508`.

## 3. Implementation Planner Used
1. Add Linux `libass` shaping probe module with machine-readable output surface.
2. Add embedded subtitle live packet probe API in `embedded_subtitle_loader`.
3. Wire both probes into `main.cpp` CLI/usage.
4. Extend Linux gate script with optional `CP-507`/`CP-508` stages.
5. Sync tasklist + docs + records and run local validation commands.

## 4. Landed Changes
- `CP-507`:
  - Added `subtitle/libass_probe` module:
    - `include/subtitle/libass_probe.h`
    - `src/subtitle/libass_probe.cpp`
  - Added CLI command:
    - `--libass-shaping-check <subtitle.(ass|ssa)>`
  - Output fields include platform, libass init, track/events, rendered image counts, and final pass/fail.
- `CP-508`:
  - Added packet-level probe API in embedded subtitle loader:
    - `probeEmbeddedSubtitleLivePacketPath(...)`
    - result structure `EmbeddedSubtitleLivePacketProbeResult`
  - Added CLI command:
    - `--embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
  - Output fields include selected stream/codec, packet counters, monotonic timestamp check, decode output check, and result.
- Linux gate script:
  - Updated `tools/run_linux_mvp_checks.sh`:
    - optional `CP-507` stage (`--libass-shaping-check`)
    - optional `CP-508` stage (`--embedded-subtitle-live-packet-check`)
    - dynamic total-count handling when optional probe files are absent.

## 5. Result Against CP IDs
- `CP-507`: done via Linux-only libass shaping/layout probe command and gate hook.
- `CP-508`: done via embedded subtitle live packet probe API/CLI and gate hook.

## 6. Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player`
  - result: PASS
- Regression sanity:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200`
  - result: PASS
  - key: `renderer_backend=D3D11`, `decoder_backend=D3D11VA`
- D3D11 diagnostics:
  - `.\build\Release\modern-video-player.exe --d3d11-diagnostics`
  - result: PASS
- New command (`CP-507`) on Windows host:
  - `.\build\Release\modern-video-player.exe --libass-shaping-check .\samples\subtitles\opengl_ass_style_validation.ass`
  - result: FAIL (expected on non-Linux host)
  - key: `platform=NonLinux`, `platform_ok=false`
- New command (`CP-508`) packet probe:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120`
  - result: PASS
  - key: `stream_index=2`, `codec=ass`, `subtitle_packets_read=3`, `monotonic_timestamps=true`, `produced_output=true`

## 7. Remaining Risks
- Linux runtime proof is still pending on an actual Linux host:
  - `--libass-shaping-check` currently only non-Linux negative-path validated on this Windows machine.
  - full `tools/run_linux_mvp_checks.sh` (including new optional stages) still requires Linux host execution evidence.

