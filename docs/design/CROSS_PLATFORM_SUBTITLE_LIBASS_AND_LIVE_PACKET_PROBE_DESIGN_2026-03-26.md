# CROSS_PLATFORM_SUBTITLE_LIBASS_AND_LIVE_PACKET_PROBE_DESIGN
Date: 2026-03-26
Scope: CP-507, CP-508

## 1. Design Goals
- Close remaining Linux subtitle backlog with machine-readable probe surfaces.
- Avoid invasive player-core pipeline rewrite in this round.
- Keep command contracts deterministic for Linux gate/CI adoption.

## 2. CP-507 Design: libass shaping/layout probe
- New module: `subtitle/libass_probe`.
- API:
  - `probeLibassShaping(subtitle_path, frame_width, frame_height, sample_duration_ms, sample_step_ms)`
  - return `LibassShapingProbeSummary`.
- Platform policy:
  - Linux: execute real `libass` probe (`ass_library_init`, `ass_renderer_init`, `ass_read_file`, `ass_render_frame`).
  - non-Linux: explicit unsupported result with machine-readable reason.
- Probe strategy:
  - validate file + extension (`.ass/.ssa`)
  - collect timestamps from fixed window and event-derived points
  - count rendered image outputs and max image dimensions
  - expose pass/fail via one command output block.

## 3. CP-508 Design: embedded subtitle live packet probe
- Extend existing `embedded_subtitle_loader` with packet-level probe API:
  - `probeEmbeddedSubtitleLivePacketPath(media_source_path, stream_index, max_packets)`
  - return `EmbeddedSubtitleLivePacketProbeResult`.
- Selection policy:
  - if stream index is invalid/missing, auto-select best supported subtitle stream.
  - keep supported codec boundary aligned with existing loader capabilities.
- Probe behavior:
  - open demux + subtitle decoder
  - read packets until `max_packets` subtitle packets or EOF
  - evaluate:
    - packet counters
    - timestamp monotonicity
    - decoded text event count
    - decoded bitmap rect count
  - expose deterministic machine-readable pass/fail fields.

## 4. CLI Contracts
- Added commands in `main.cpp`:
  - `--libass-shaping-check <subtitle.(ass|ssa)>`
  - `--embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
- Both commands:
  - print structured `key=value` diagnostics
  - return `0` on PASS, non-zero on FAIL.

## 5. Gate Integration
- `tools/run_linux_mvp_checks.sh` adds optional stages:
  - `CP-507` if ASS probe file exists
  - `CP-508` if embedded-subtitle media exists
- Uses dynamic total-stage counting to avoid false failures when optional probe assets are missing.

## 6. Non-goals (this round)
- No full runtime replacement of existing subtitle rendering path.
- No full player-core demux/decode subtitle thread redesign.
- No macOS subtitle path work (explicitly deferred scope).

