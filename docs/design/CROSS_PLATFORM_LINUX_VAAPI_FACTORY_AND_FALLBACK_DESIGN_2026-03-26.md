# Cross-Platform Linux VAAPI Factory and Fallback Design

Date: 2026-03-26  
Scope: Linux Phase-7 (`CP-701~CP-705`) with Windows compatibility preserved

## 1. Design Goals
- Keep hardware decode extension out of `PlayerCore` platform branches.
- Add Linux VAAPI runtime observability without requiring compile-time platform forks in business logic.
- Preserve deterministic fallback contract: `VAAPI -> Software`.
- Keep current OpenGL renderer baseline stable with copy-back path first.

## 2. Architecture
- New abstraction: `platform::HwDeviceFactory`
  - backend support mapping (`DecoderBackend` -> `AVHWDeviceType`);
  - codec HW pixel-format discovery;
  - hardware device context creation:
    - D3D11 shared renderer-device path (Windows);
    - FFmpeg-managed device path (including VAAPI);
  - runtime probe API for capability detection.
- `platform_capabilities` integration:
  - VAAPI runtime availability is probed on Linux using factory probe API.
- `PlayerCore` integration:
  - generic hardware decode setup entry:
    - `tryConfigureHardwareDecode(codec, codec_ctx, backend)`
  - decoder open candidate flow supports `D3D11VA` and `VAAPI`;
  - `isHardwareDecoderBackend` centralizes hardware/backend checks;
  - `configureHardwareFramesContext` keeps D3D11-specific frames tuning isolated.

## 3. Fallback and Copy-Back Policy
- If VAAPI capability/runtime probing fails:
  - strategy exposes software path only, or
  - runtime falls back to software decoder when hardware init/open fails.
- If VAAPI decode is selected:
  - renderer baseline remains copy-back through existing `prepareVideoOutputFrame()` HW transfer path.
- Zero-copy (`dmabuf/EGL`) decision:
  - not implemented in this phase;
  - deferred after Linux host evidence stabilizes copy-back baseline.

## 4. CLI / Gate Contract
- New command:
  - `--linux-vaapi-fallback-check <media_file> [sample_ms]`
- Output surface includes:
  - capability probe completion/availability
  - decoder candidate chain and selected decoder
  - fallback reason and copy-back counters
  - final machine-readable `result=PASS|FAIL`
- Linux gate script consumes this command as a required stage.

## 5. Compatibility Notes
- macOS implementation is intentionally deferred by current scope decision.
- Existing Windows D3D11 path remains source-compatible and behavior-compatible.
