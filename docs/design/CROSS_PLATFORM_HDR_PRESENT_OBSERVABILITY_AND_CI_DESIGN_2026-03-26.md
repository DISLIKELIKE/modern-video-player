# Cross-Platform HDR Present, Observability, and CI Design
Date: 2026-03-26
Scope: `CP-801`, `CP-901 ~ CP-905`

## Goal
- Finish the real Windows D3D11 display-HDR present bridge.
- Make runtime scheduling / drop / present behavior observable in one CLI surface.
- Turn driver quirk collection into reusable structured data.
- Raise Linux and Windows gates to a common machine-readable contract.
- Close dual-platform packaging and CI wiring without adding more `PlayerCore` policy forks.

## Design Principles
1. Real HDR output must live on the actual D3D11 present path, not in policy-only stubs.
2. Diagnostics must describe runtime decisions, not only static capabilities.
3. Gate scripts must validate both process success and required machine-readable fields.
4. Driver quirk knowledge must be appendable without source-code edits.
5. Packaging scripts and CI should reuse the same command surface that local validation uses.

## 1. D3D11 HDR Present Model
### Trigger
- Content is considered HDR when frame transfer metadata indicates PQ/HLG.
- HDR present is requested when:
  - mode is not `off`
  - and content is HDR, or mode is `force`

### Decision order
1. mode disabled
2. display binding unavailable
3. HDR output probe unavailable
4. display HDR / advanced-color inactive
5. SDR content in `auto`
6. enable HDR present

### Active present path
- swapchain format: `R16G16B16A16_FLOAT`
- color space: `DXGI_COLOR_SPACE_RGB_FULL_G10_NONE_P709`
- metadata: `SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, ...)` when frame side data exists

### Fallback path
- swapchain format: `B8G8R8A8_UNORM`
- color space: `DXGI_COLOR_SPACE_RGB_FULL_G22_NONE_P709`
- decision string remains explicit and is exported through diagnostics

## 2. Runtime Observability Contract
### `performance-log-check`
- queue state:
  - frame queue size / capacity / peak / push timeout
- decode-wait / backpressure state:
  - video/audio backpressure events and wait ms
- render timing:
  - `scheduler_render_wait_ms`
- drop accounting:
  - `runtime_drop_total`
  - `runtime_drop_summary`
- D3D11 present timing:
  - present count / failures / total ms / avg ms / max us
- D3D11 HDR runtime state:
  - request / active / decision
  - output display + color space
  - metadata availability / application

### Dedicated HDR self-check
- `--d3d11-hdr-output-check` is the focused gate for `CP-801`
- It validates:
  - D3D11 backend selection
  - render progress
  - evaluated HDR decision
  - active HDR present when both content and display permit it
  - otherwise explicit inactive reason

## 3. Driver Quirk Sample Library Contract
### Storage
- `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`

### Capture command
- `tools/collect_driver_quirk_sample.ps1`

### Required columns
- host / platform / backend
- adapter ids and driver version
- native-path policy result
- HDR probe result
- OpenGL vendor / renderer / version when applicable
- free-form notes

### Usage
- append new field samples without editing source files
- compare future quirk rules against historical adapter snapshots
- upload runner samples from CI as artifacts

## 4. Gate Parity Model
### Windows gate
- keeps the richer OpenGL gate
- adds D3D11 HDR / diagnostics / performance legs alongside it

### Linux gate
- uses the same stage structure:
  - named check
  - required patterns
  - summary lines
  - PASS/FAIL result
- validates both playback behavior and observability counters

## 5. CI Matrix Model
### Windows leg
- configure + build
- D3D11 diagnostics / HDR check / performance check
- OpenGL gate
- driver sample capture
- ZIP packaging

### Linux leg
- configure + build
- `xvfb-run` Linux MVP gate
- `DEB/TGZ` packaging

## 6. Packaging / Readiness Model
### Windows
- `tools/package_windows.ps1`
- package format: `ZIP`

### Linux
- `tools/package_linux.sh`
- package formats: `DEB`, `TGZ`

### Release-readiness checklist
1. platform gate passes
2. package artifacts generated
3. diagnostics commands produce machine-readable output
4. driver sample capture is updated for at least one validation host
5. residual environment-specific gaps are explicitly documented
