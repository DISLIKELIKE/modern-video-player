# Cross-Platform Subtitle/Font Platform Closure Design
Date: 2026-03-26
Scope: CP-501 ~ CP-506
Status: Implemented Baseline

## 1. Objective
- Close subtitle policy and font-path determinism across Windows/Linux baseline.
- Keep ownership and selection behavior machine-verifiable from CLI checks.
- Keep platform-specific font registration details inside subtitle/font infrastructure, not `PlayerCore`.

## 2. Embedded Subtitle Policy Model (CP-501)
- Policy object: `EmbeddedSubtitleSelectionPolicy`
  - `preferred_languages`
  - `forced_policy` (`auto/prefer/avoid/only`)
  - `sdh_policy` (`auto/prefer/avoid/only`)
- Selection strategy:
  - score-based ranking over supported embedded tracks;
  - `only` modes act as hard candidate filters;
  - `prefer/avoid` modes adjust ranking while preserving fallback determinism.
- Policy source merge:
  - base from persisted settings;
  - CLI overrides applied per run;
  - final merged policy pushed to `VideoPlayer -> PlayerCore`.

## 3. Subtitle Track UI/Ownership/CLI Closure (CP-502/503/504)
- UI/overlay contract:
  - `IRenderOverlaySink::setSubtitleTrackCatalog(track_labels, current_ordinal)` to expose list + active selection.
- Ownership policy:
  - active subtitle source priority: `external > embedded`;
  - clearing external subtitle reactivates previously attached embedded track.
- CLI closure:
  - playback args: `--subtitle-language`, `--subtitle-forced`, `--subtitle-sdh`
  - checks: `--embedded-subtitle-policy-check`, `--subtitle-ownership-check`.

## 4. Linux Attachment Font Lifecycle (CP-505)
- Source:
  - container `AVMEDIA_TYPE_ATTACHMENT` font streams extracted to media-scoped temp cache.
- Registration:
  - Linux path now uses `fontconfig` app-font registration (`FcConfigAppFontAddFile`).
- Cleanup:
  - media-session release removes extracted temp files;
  - app-font collection is rebuilt from remaining registered font file set to avoid stale attachment fonts after close.
- Concurrency:
  - registry map operations are guarded by `SubtitleFontRegistryState::mutex`.

## 5. Windows DirectWrite Custom Collection Maturation (CP-506)
- Registered private font files are converted into `IDWriteFontSet` and custom `IDWriteFontCollection`.
- Renderers (`D3D11` + `OpenGL`) attempt to use custom collection first for subtitle text format creation.
- Diagnostics:
  - `--directwrite-font-collection-check` exports collection-create status and counts.
- Gate integration:
  - Windows OpenGL gate script now includes DirectWrite collection check as required pass item.

## 6. Verification Contract
- Local baseline:
  - build must pass;
  - `--embedded-subtitle-policy-check` / `--subtitle-ownership-check` / `--attachment-font-check` / `--directwrite-font-collection-check` must pass;
  - `run_opengl_checks.ps1` must pass with DirectWrite check included.
- Linux follow-up:
  - run equivalent checks on Linux host to verify fontconfig runtime branch behavior.
