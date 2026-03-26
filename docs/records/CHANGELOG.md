# CHANGELOG

## 索引说明（2026-03-26 编码清理批次）

- 本轮仅清理 `records/readme` 索引范围，不批量改写历史正文。
- 最近收口条目位于文件顶部（`Issue 127` 到 `Issue 122`）。
- 历史段落若出现旧编码乱码，将在后续专题批次逐步处理。

## Issue 127: Linux workflow Build Linux Release compile blocker closure

**Date**: 2026-03-26

### Problem Description
- GitHub Actions Linux lane failed in `Build Linux Release` while Windows lane passed.
- Failing runs included:
  - `23601824744` (`push`)
  - `23601841417` (`workflow_dispatch`)

### Root Cause Analysis
- `src/subtitle/libass_probe.cpp` used `std::max(0, event.Start)` and `std::max(0, event.Duration)`, which caused template deduction conflict on Linux (`int` vs `long long`).
- `src/render/opengl_video_renderer.cpp` had cross-platform class code referencing helper symbols that were only defined inside a Windows-only helper block.

### Solution
- `libass_probe`:
  - Added clamp helper to safely convert libass event timestamps to `int`.
- `opengl_video_renderer`:
  - Added Linux-visible (`#if !defined(_WIN32)`) helper/type block outside Windows-only section, including:
    - subtitle animated-run detection helpers
    - OpenGL present/HDR enums and env parsers
    - string trimming and swap-interval helpers
    - output display binding state + Linux resolver

### Validation
- `gh run view 23601824744 --job 68733664519 --log` -> failure root errors confirmed.
- `gh run view 23601841417 --json status,conclusion,event,jobs` -> Linux `Build Linux Release` failure confirmed.
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS

### Modified Files
- `src/subtitle/libass_probe.cpp`
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY61_LINUX_WORKFLOW_BUILD_ERROR_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 126: Workflow log remaining FFmpeg duration compatibility closure

**Date**: 2026-03-26

### Problem Description
- The `log` file still reported Linux compile failures after previous compatibility fixes.
- Remaining hard failure was old-FFmpeg `AVFrame` duration field mismatch:
  - `AVFrame has no member duration; did you mean pkt_duration`

### Root Cause Analysis
- `PlayerCore` decode output timing still read `frame->duration` directly.
- Existing FFmpeg compatibility layer covered channel-layout API drift but not frame-duration field drift.

### Solution
- Extended `include/media/ffmpeg_channel_layout_compat.h` with `frameDuration(const AVFrame*)`.
- Added compile-time branch:
  - newer FFmpeg -> `frame->duration`
  - older FFmpeg -> `frame->pkt_duration`
- Replaced direct frame duration access in `src/core/player_core.cpp` video/audio decode timing paths.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS

### Modified Files
- `include/media/ffmpeg_channel_layout_compat.h`
- `src/core/player_core.cpp`
- `docs/analysis/PLAYERCORE_DAY60_LOG_WORKFLOW_ERROR_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 125: Linux CI compatibility stabilization (FFmpeg/libass + gate workflow determinism)

**Date**: 2026-03-26

### Problem Description
- Cross-platform task matrix was marked complete for Windows/Linux (`CP-001 ~ CP-905`), but CI gate still failed on Linux and Windows lanes.
- Linux compile failed on FFmpeg channel-layout API drift and `libass` header path mismatch.
- Windows CI gate/packaging could fail when probe media fixture and plugin build output were absent in CI workspace.

### Root Cause Analysis
- Audio channel layout usage mixed old/new FFmpeg field assumptions directly in runtime code.
- `libass` include path was Linux-distribution dependent (`ass/ass.h` vs `libass/ass.h`).
- OpenGL color enum used `None`, which has macro collision risk on Linux stacks.
- CI workflow built only `modern-video-player` but packaging/install path also required `sample_logger_plugin`, and probe media presence was assumed.

### Solution
- Added `include/media/ffmpeg_channel_layout_compat.h` and migrated channel-layout access in:
  - `src/core/player_core.cpp`
  - `src/main.cpp`
  - `src/demuxer.cpp`
- Updated `PlayerCore` resampler state/initialization to support old/new swresample interfaces:
  - modern path: `swr_alloc_set_opts2`
  - legacy path: `swr_alloc_set_opts`
- Fixed Linux compile blockers:
  - `src/subtitle/libass_probe.cpp` include fallback (`ass/ass.h` first)
  - `src/render/opengl_video_renderer.cpp` enum rename (`None` -> `Disabled`)
- Hardened CI workflow `.github/workflows/cross-platform-gate.yml`:
  - generate probe media fixture when missing (Windows + Linux lanes)
  - build `sample_logger_plugin` together with `modern-video-player`

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)

### Modified Files
- `include/media/ffmpeg_channel_layout_compat.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/demuxer.cpp`
- `src/subtitle/libass_probe.cpp`
- `src/render/opengl_video_renderer.cpp`
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY59_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION.md`
- `docs/design/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 124: Linux gate reporting/artifact closure for CI evidence reuse

**Date**: 2026-03-26

### Problem Description
- Linux gate checks were strict, but evidence output was mainly console text.
- CI lacked a dedicated machine-readable Linux gate summary artifact for downstream review/automation.

### Root Cause Analysis
- `tools/run_linux_mvp_checks.sh` had no report-file output contract.
- Linux CI gate step did not tee gate logs to dedicated artifact files.

### Solution
- Added Linux gate report contract in `tools/run_linux_mvp_checks.sh`:
  - arg #10 / env `MVP_LINUX_GATE_REPORT_FILE`
  - per-check fields `check.<id>.*` and global `gate.*`
  - explicit fail result/reason recording via unified fail path
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - enforce `set -euo pipefail`
  - tee output to `logs/linux-mvp-gate.log`
  - write summary to `logs/linux-mvp-gate-summary.env`
  - upload `logs/*.env` artifacts
- Synced master tasklist/docs indexes and round analysis/design/plan/report documents.

### Validation
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS

### Modified Files
- `tools/run_linux_mvp_checks.sh`
- `.github/workflows/cross-platform-gate.yml`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY58_LINUX_GATE_REPORTING_AND_CI_ARTIFACT_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_GATE_REPORTING_AND_ARTIFACT_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_GATE_REPORTING_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_GATE_REPORTING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 123: Linux gate strict optional checks closure for CP-507/CP-508 in CI

**Date**: 2026-03-26

### Problem Description
- Linux gate script accepted `CP-507` and `CP-508` as optional checks, but `CP-508` depended on a non-versioned fixture file (`build/tmp/embedded-ass-validation.mkv`).
- In CI Linux runs, missing fixture could silently skip `CP-508`, reducing deterministic subtitle backlog coverage.

### Root Cause Analysis
- No embedded ASS fixture auto-generation path existed in `tools/run_linux_mvp_checks.sh`.
- CI Linux dependency setup did not install `ffmpeg` binary for fixture generation.
- No strict mode existed to force optional checks as required in CI.

### Solution
- Updated `tools/run_linux_mvp_checks.sh`:
  - added `ensure_embedded_ass_sample(...)` with `ffmpeg`-based fixture generation
  - added strict optional-check switch (`REQUIRE_OPTIONAL_CHECKS`, arg #7 or `MVP_REQUIRE_OPTIONAL_CHECKS`)
  - added generation input args #8/#9 (base media + ASS subtitle)
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - install `ffmpeg`
  - invoke Linux gate in strict mode with explicit CP-508 fixture paths
- Synced tasklist/docs indexes and round docs for analysis/design/plan/report.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS (syntax check)
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)


### Modified Files
- `tools/run_linux_mvp_checks.sh`
- `.github/workflows/cross-platform-gate.yml`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY57_LINUX_GATE_STRICT_OPTIONAL_CHECKS_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 122: CP-507 and CP-508 Linux subtitle backlog closure

**Date**: 2026-03-26

### Problem Description
- The cross-platform master tasklist still had two Linux subtitle backlog items open:
  - `CP-507`: no machine-readable Linux `libass` shaping/layout probe entry
  - `CP-508`: no packet-level embedded subtitle live-path probe entry
- Existing validation focused on full-track loading and lacked packet/renderer probe contracts for these backlog items.

### Root Cause Analysis
- There was no dedicated `libass` probe module/CLI command.
- `embedded_subtitle_loader` had full-track loading APIs but not a packet-level probe surface.
- Linux gate script could not include these checks because command contracts did not exist.

### Solution
- Added `CP-507` probe module and CLI:
  - `include/subtitle/libass_probe.h`
  - `src/subtitle/libass_probe.cpp`
  - `--libass-shaping-check <subtitle.(ass|ssa)>`
- Added `CP-508` packet probe API and CLI:
  - `probeEmbeddedSubtitleLivePacketPath(...)`
  - `EmbeddedSubtitleLivePacketProbeResult`
  - `--embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
- Extended Linux gate script with optional `CP-507`/`CP-508` stages:
  - `tools/run_linux_mvp_checks.sh`
- Updated tasklist/docs indexes and added round-specific analysis/design/plan/report docs.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `.\build\Release\modern-video-player.exe --libass-shaping-check .\samples\subtitles\opengl_ass_style_validation.ass` -> FAIL on this host (`platform=NonLinux`, expected)


### Modified Files
- `CMakeLists.txt`
- `include/subtitle/libass_probe.h`
- `src/subtitle/libass_probe.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `src/main.cpp`
- `tools/run_linux_mvp_checks.sh`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY56_CP507_CP508_LIBASS_SHAPING_AND_LIVE_PACKET_PROBE.md`
- `docs/design/CROSS_PLATFORM_SUBTITLE_LIBASS_AND_LIVE_PACKET_PROBE_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE5_BACKLOG_CP507_CP508_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE5_BACKLOG_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
# 闂淇璁板綍

## Issue 121: CP-801 and CP-901 ~ CP-905 HDR present / observability / CI closure

**Date**: 2026-03-26

### Problem Description
- `CP-801` was still open, so Phase 8 had not yet landed the real Windows DXGI HDR present bridge.
- Phase 9 (`CP-901 ~ CP-905`) was also still open:
  - no structured driver quirk sample library
  - missing unified runtime counter exposure in CLI
  - Linux gate script was weaker than the Windows gate
  - no Windows/Linux matrix gate workflow
  - no scripted Windows package step paired with Linux packaging

### Root Cause Analysis
- Existing Phase 8 work had already built output binding / diagnostics primitives, but the D3D11 present path itself was still SDR-only.
- Runtime counters existed in multiple layers but were not fully exported through one command surface.
- Regression knowledge for adapter / driver combinations lived mainly in narrative docs instead of reusable structured data.
- Linux gate automation and release packaging remained phase-local rather than cross-platform.

### Solution
- Completed the real D3D11 HDR present/runtime diagnostics closure:
  - HDR-aware DXGI swapchain mode switching
  - output probe diagnostics
  - `--d3d11-hdr-output-check`
  - D3D11 present timing counters in `--performance-log-check`
- Added Phase 9 observability / automation assets:
  - `tools/collect_driver_quirk_sample.ps1`
  - `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
  - upgraded `tools/run_linux_mvp_checks.sh`
  - `.github/workflows/cross-platform-gate.yml`
  - `tools/package_windows.ps1`
- Synced new Phase 8/9 analysis/design/plan/report docs and updated the master task list.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-hdr-output-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='d3d11'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -OutputCsvPath .\docs\reference\DRIVER_QUIRK_SAMPLE_LIBRARY.csv ...` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release -SkipBuild` -> PASS


### Modified Files
- `src/main.cpp`
- `include/render/d3d11_video_renderer.h`
- `include/render/video_renderer.h`
- `src/render/d3d11_video_renderer.cpp`
- `include/core/scheduler.h`
- `src/core/scheduler.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `tools/run_linux_mvp_checks.sh`
- `tools/collect_driver_quirk_sample.ps1`
- `tools/package_windows.ps1`
- `.github/workflows/cross-platform-gate.yml`
- `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY54_CP801_CP901_CP905_HDR_PRESENT_AND_CI_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_HDR_PRESENT_OBSERVABILITY_AND_CI_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE9_OBSERVABILITY_CI_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE8_LOCAL_CHECK.md`
- `docs/reports/CROSS_PLATFORM_PHASE9_LOCAL_CHECK.md`
# 闂傤噣顣芥穱顔碱槻鐠佹澘缍?
## Issue 120: CP-501 ~ CP-506 subtitle/font platform closure

**Date**: 2026-03-26

### Problem Description
- Phase-5 tasks (`CP-501`~`CP-506`) were partially landed but not fully closed:
  - embedded subtitle policy flow had integration regression in `main.cpp`;
  - Linux attachment-font runtime path still lacked registration/cleanup closure;
  - DirectWrite custom font collection check was not included in Windows gate coverage.
- Phase-4 local report still had pending placeholders.

### Root Cause Analysis
- `main.cpp` attempted to mutate `const AppSettings`, causing compile failure after policy merge wiring.
- `subtitle_font_registry` only implemented private-font registration on Windows; non-Windows branch always returned failure.
- `tools/run_opengl_checks.ps1` had no explicit `--directwrite-font-collection-check` stage.

### Solution
- Fixed compile blocker in `src/main.cpp` by removing invalid write to `const app_settings`.
- Completed Linux fontconfig closure in `src/subtitle/subtitle_font_registry.cpp`:
  - add-file registration (`FcConfigAppFontAddFile`) + font rebuild (`FcConfigBuildFonts`);
  - release-time app-font collection rebuild from remaining registry files.
- Added DirectWrite custom font collection gate stage in `tools/run_opengl_checks.ps1`.
- Synced Phase5 docs (`analysis/design/report/plans`) and updated Phase5 statuses in master tasklist.
- Updated `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md` with actual local execution results.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid` -> PASS
- `.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --settings-persistence-check` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"` -> `OpenGL gate result: PASS` (`18/18`)


### Modified Files
- `src/main.cpp`
- `src/subtitle/subtitle_font_registry.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/CROSS_PLATFORM_PHASE5_SUBTITLE_FONT_CLOSURE_PLAN_2026-03-26.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY51_CP501_CP506_SUBTITLE_FONT_PLATFORM_CLOSURE.md`
- `docs/analysis/README.md`
- `docs/design/CROSS_PLATFORM_SUBTITLE_FONT_PLATFORM_CLOSURE_DESIGN_2026-03-26.md`
- `docs/design/README.md`
- `docs/reports/CROSS_PLATFORM_PHASE5_LOCAL_CHECK.md`
- `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 119: CP-401 ~ CP-406 Linux MVP playback closure and gate commands

**Date**: 2026-03-26

### Problem Description
- Phase-4 Linux MVP tasks (`CP-401`~`CP-406`) were still open and not connected as a complete machine-readable gate.
- Only partial `linux-software-audio-check` scaffolding existed; command usage and CLI dispatch were incomplete.
- No deterministic OpenGL init fail injection existed for fallback-chain verification.

### Root Cause Analysis
- Existing checks were mostly Windows/OpenGL focused and did not provide a Linux MVP closure command set.
- Fallback verification relied on incidental failures rather than a controlled injection mechanism.
- Linux gate script baseline was missing.

### Solution
- Added complete Phase-4 command set in `src/main.cpp`:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added command usage lines and CLI argument dispatch for all above commands.
- Added OpenGL force-fail injection in `src/render/opengl_video_renderer.cpp`:
  - `MVP_OPENGL_FORCE_INIT_FAIL` (truthy values force init failure).
- Added Linux gate script:
  - `tools/run_linux_mvp_checks.sh`
- Updated Phase4 status and command baseline in task planning docs.

### Validation
- `cmake -S . -B build` -> PASS
- `cmake --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
- Linux-only Phase4 commands are implemented and dispatchable, but full PASS verification remains pending Linux host execution.


### Modified Files
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `tools/run_linux_mvp_checks.sh`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY50_CP401_CP406_LINUX_MVP_PLAYBACK_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_MVP_PLAYBACK_GATE_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 118: CP-301 ~ CP-305 build switch platformization and Linux packaging baseline

**Date**: 2026-03-26

### Problem Description
- Cross-platform backend composition still lacked explicit CMake feature switches.
- Platform-specific source boundaries were not fully controlled by build-time backend switches.
- Startup diagnostics did not explicitly separate compiled backend set from runtime-available set.
- Linux dependency closure and packaging baseline path were not codified in build/packaging config.

### Root Cause Analysis
- Capability compile macros were hardcoded and not directly driven by user-visible build options.
- Renderer source inclusion and diagnostics command paths were not fully switch-aware.
- Linux package generator/dependency baseline had no standardized CPack path.

### Solution
- Added explicit CMake switches:
  - `ENABLE_D3D11_RENDERER`, `ENABLE_OPENGL_RENDERER`, `ENABLE_SDL_RENDERER`
  - `ENABLE_D3D11VA`, `ENABLE_DXVA2`, `ENABLE_VAAPI`, `ENABLE_VIDEOTOOLBOX`
- Added platform force-off rules and effective `MVP_HAVE_*` compile definitions.
- Made renderer source inclusion/link dependencies switch-aware and platform-guarded.
- Extended startup diagnostics with:
  - `startup_renderer_compiled_set`
  - `startup_renderer_runtime_set`
  - `startup_decoder_compiled_set`
  - `startup_decoder_runtime_set`
- Added Linux dependency closure requirements (`libass/fontconfig/freetype/OpenGL`) and Linux package baseline:
  - CPack generator: `DEB;TGZ`
  - helper script: `tools/package_linux.sh`

### Validation
- Default configure/build:
  - `cmake -S . -B build` -> PASS
  - `cmake --build build --config Release --target modern-video-player` -> PASS
- Default runtime checks:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
  - `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS`
- Switch matrix builds:
  - `build_nod3d11` (`ENABLE_D3D11_RENDERER=OFF`, `ENABLE_D3D11VA=OFF`) build PASS
  - `build_noopengl` (`ENABLE_OPENGL_RENDERER=OFF`) build PASS
- Command behavior under switch matrix:
  - `build_nod3d11 --d3d11-diagnostics` reports unsupported without linker/runtime crash.
  - `build_noopengl --opengl-diagnostics` reports unsupported without linker/runtime crash.


### Modified Files
- `CMakeLists.txt`
- `include/decoder/decoder_capability.h`
- `src/decoder/decoder_factory.cpp`
- `src/platform/platform_capabilities.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `tools/package_linux.sh`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY49_CP301_CP305_BUILD_SWITCH_GUARD_AND_PACKAGING_BASELINE.md`
- `docs/design/CROSS_PLATFORM_BUILD_SWITCH_AND_PACKAGING_BASELINE_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE3_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 117: CP-201 ~ CP-205 renderer/input/overlay responsibility cleanup and interaction freeze gate

**Date**: 2026-03-26

### Problem Description
- Renderer abstraction still exposed input and overlay responsibilities mixed with rendering concerns.
- Event-pump thread affinity constraints were implicit and could regress into interaction freeze/deadlock behavior.
- No machine-readable dedicated check existed for interaction freeze scenarios (mouse/hotkey/window-event stress).

### Root Cause Analysis
- `IVideoRenderer` historically accumulated non-render responsibilities.
- `PlayerCore` and idle startup window consumed renderer command APIs directly, coupling control flow to renderer internals.
- Event handling call paths lacked explicit guardrails for non-owner thread invocation.

### Solution
- Introduced role interfaces:
  - `input::IPlaybackInputSource`
  - `render::IRenderOverlaySink`
- Shrank `IVideoRenderer` to rendering-focused contract only.
- Refactored `PlayerCore` and idle-window path to consume input/overlay roles via interface cross-cast.
- Added main-thread guard handling for event pumping in `PlayerCore`, `Display`, `D3D11VideoRenderer`, and `OpenGLVideoRenderer`.
- Added machine-readable `--interaction-freeze-check` and integrated it into `tools/run_opengl_checks.ps1`.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS`


### Modified Files
- `include/input/playback_input_source.h`
- `include/render/render_overlay_sink.h`
- `include/render/video_renderer.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `include/display.h`
- `src/display.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/analysis/PLAYERCORE_DAY48_CP201_CP205_RENDERER_DISPLAY_INPUT_CLEANUP.md`
- `docs/design/CROSS_PLATFORM_RENDER_INPUT_OVERLAY_BOUNDARY_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE2_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 116: CP-101 ~ CP-106 cross-platform startup strategy extraction

**Date**: 2026-03-25

### Problem Description
- `PlayerCore` startup path still mixed platform policy and execution logic.
- `RendererFactory`/`DecoderFactory` still held default policy responsibilities.
- Startup strategy lacked machine-readable diagnostics (capabilities/candidates/selected/fallback reason).

### Root Cause Analysis
- No unified capability model for platform/backend availability.
- No dedicated strategy object for open-time planning.
- Decoder selection inputs were too narrow to evolve safely across platforms.

### Solution
- Added `platform_capabilities` abstraction and probe (`CP-101`).
- Added `playback_strategy` abstraction and open plan generation (`CP-102`).
- Refactored `RendererFactory` to support-check + creation only (`CP-103`).
- Refactored `DecoderFactory` to context-driven ordering with mandatory software fallback (`CP-104`).
- Refactored `PlayerCore::open()`/decoder init to consume startup plan (`CP-105`).
- Added machine-readable startup strategy diagnostics exported by `--performance-log-check` (`CP-106`).

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS


### Modified Files
- `include/platform/platform_capabilities.h`
- `src/platform/platform_capabilities.cpp`
- `include/core/playback_strategy.h`
- `src/core/playback_strategy.cpp`
- `include/render/renderer_factory.h`
- `src/render/renderer_factory.cpp`
- `include/decoder/decoder_factory.h`
- `src/decoder/decoder_factory.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `CMakeLists.txt`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/analysis/PLAYERCORE_DAY47_CP101_CP106_STRATEGY_AND_CAPABILITY_EXTRACTION.md`
- `docs/design/CROSS_PLATFORM_STARTUP_STRATEGY_AND_CAPABILITIES_DESIGN_2026-03-25.md`
- `docs/reports/CROSS_PLATFORM_STRATEGY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 115: Cross-platform Phase 8 ICC output LUT and per-display diagnostics closure

**Date**: 2026-03-26

### Problem Description
- Phase 8 still lacked ICC/profile-driven LUT generation, per-display output binding, and runtime diagnostics closure.
- OpenGL output color regression still validated only the manual `.cube` path.
- `CP-801` remained open, but the runtime/output model required by the rest of Phase 8 was still missing.

### Root Cause Analysis
- Output LUT initialization was static and only consumed `MVP_OPENGL_3DLUT_FILE`.
- There was no display-binding model for the SDL window's current display / monitor ICC profile.
- Diagnostics could not explain which display, ICC profile, or LUT source was active during playback.

### Solution
- Added reusable output-color helper with `.cube` parsing and ICC matrix/TRC profile -> sampled 3D LUT generation.
- Added OpenGL runtime display binding and Windows ICC profile discovery.
- Added output LUT source priority:
  - manual `.cube`
  - manual ICC profile
  - auto ICC from current display
  - none
- Added new CLI / env surfaces:
  - `MVP_OPENGL_ICC_PROFILE_FILE`
  - `MVP_OPENGL_AUTO_ICC`
  - `--opengl-icc-profile <icc_profile_file>`
  - `--opengl-auto-icc`
  - `--opengl-output-color-icc-check <media_file> [sample_ms]`
- Extended diagnostics and OpenGL gate coverage with display / ICC / LUT runtime fields.
- Marked `CP-802 ~ CP-805` done in the master tasklist; kept `CP-801` open.

### Validation
- `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path; powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'` -> `OpenGL gate result: PASS` (`25/25`)


### Modified Files
- `include/render/output_color_profile.h`
- `src/render/output_color_profile.cpp`
- `src/render/opengl_video_renderer.cpp`
- `include/render/opengl_video_renderer.h`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY53_CP802_CP805_ICC_OUTPUT_BINDING_AND_OUTPUT_DIAGNOSTICS.md`
- `docs/design/CROSS_PLATFORM_OUTPUT_COLOR_PROFILE_BINDING_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE8_OUTPUT_COLOR_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE8_LOCAL_CHECK.md`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 114: Cross-platform Phase 6 bitmap subtitle pipeline closure

**Date**: 2026-03-26

### Problem Description
- Phase 6 bitmap subtitle baseline existed, but packet-level timeline modeling, cache reuse, and dedicated regression coverage were still incomplete.
- Real PGS samples could surface invalid display-window metadata, causing pathological subtitle duration expansion.
- Existing diagnostics could not expose rect-level and multi-rect bitmap subtitle behavior.

### Root Cause Analysis
- Bitmap subtitle loader still modeled each rect as an independent `SubtitleItem`, which lost packet/event grouping.
- Renderer bitmap branches recreated premultiplied payloads and D2D bitmap resources inline without reuse.
- Phase 6 had no dedicated CLI / gate path for PGS/DVD bitmap subtitles and multi-rect stress.
- Bitmap timeline fallback trusted `end_display_time` too eagerly.

### Solution
- Switched bitmap subtitle modeling to packet-level `SubtitleItem.bitmap_rects` aggregation.
- Extended `EmbeddedSubtitleLoadResult` with `bitmap_rect_count`, `bitmap_multi_rect_item_count`, and `bitmap_max_rects_per_item`.
- Added bitmap display-window sanity fallback for invalid / oversized `start_display_time` / `end_display_time`.
- Added renderer bitmap cache/reuse in OpenGL and D3D11 subtitle D2D paths.
- Added CLI:
  - `--bitmap-subtitle-check <media_file> [stream_index]`
  - `--bitmap-subtitle-stress-check`
- Extended `tools/run_opengl_checks.ps1` with DVD / PGS / stress bitmap regressions.

### Validation
- `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check` -> PASS
- `$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path; powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'` -> `OpenGL gate result: PASS` (`23/23`)


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY52_CP601_CP605_BITMAP_SUBTITLE_PIPELINE_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_BITMAP_SUBTITLE_PIPELINE_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE6_BITMAP_SUBTITLE_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE6_LOCAL_CHECK.md`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 闂 98: OpenGL libass 宸窛娓呭崟銆佹樉绀虹骇 HDR 璁捐涓?quirk diagnostics 姝ｅ紡鍖?
**鏃ユ湡**: 2026-03-24

### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰鎶婁笂涓€杞粰鍑虹殑涓変釜鍚庣画寤鸿鍏ㄩ儴钀藉湴锛?  - 琛?OpenGL 涓?`libass/mpv` 鐨勮兘鍔涘樊璺濇竻鍗?  - 琛ユ樉绀虹骇 HDR 杈撳嚭璁捐
  - 鎶?OpenGL quirk 鏈哄埗鍋氭垚鍙墿灞曡鍒欒〃鍜岀嫭绔嬭瘖鏂揩鐓?- 褰撳墠 OpenGL 铏藉凡鍏峰 `ASS/SSA` 鍩虹娓叉煋銆乶ative interop 鍜屽熀纭€ HDR aware 澶勭悊锛屼絾瀵瑰浠嶇己灏戞槑纭殑宸窛鍙ｅ緞涓庢寮忚瘖鏂叆鍙ｃ€?
### 鍘熷洜鍒嗘瀽
- `ASS/SSA` 涔嬪墠铏界劧宸茶兘娓叉煋锛屼絾鈥滆繕宸灏戔€濅粛鐒跺仠鐣欏湪妯＄硦鎻忚堪锛屾病鏈夋媶鍒?`karaoke / vector / shaping / font fallback` 绛夋垚鐔熸挱鏀惧櫒鐪熸鏁忔劅鐨勭淮搴︺€?- 鐜版湁 OpenGL HDR 澶勭悊杩樺彧鏄?renderer 鍐呴儴鑹插僵鍙樻崲锛屾病鏈夋妸鏄剧ず绾?HDR 杈撳嚭闇€瑕佺殑 `swapchain / metadata / ICC/3D LUT` 缁撴瀯鍖栦笅鏉ャ€?- OpenGL native interop 鐨勫惎鍔ㄦ湡鍐崇瓥姝ゅ墠浠嶅亸鍐呴儴瀹炵幇缁嗚妭锛岀己灏戝儚 `--d3d11-diagnostics` 閭ｆ牱鍙崟鐙墽琛岀殑鏈哄櫒鍙蹇収銆?
### 瑙ｅ喅鏂规
- 鍦?`include/render/opengl_video_renderer.h` 涓?`src/render/opengl_video_renderer.cpp` 涓柊澧?`OpenGLDiagnosticsSnapshot`銆乣probeSystemDiagnostics()` 鍜岀粨鏋勫寲 `hard blocker + quirk rule + env override` 鍚姩鏈熷喅绛栥€?- 鍦?`src/main.cpp` 涓柊澧?`--opengl-diagnostics` CLI锛屼竴娆℃€ц緭鍑?OpenGL 涓婁笅鏂囥€乣WGL_NV_DX_interop`銆丏3D11 鍩虹鑳藉姏銆佹渶缁堢敓鏁堣鍒欎笌 override 缁撴灉銆?- 鏂板 `docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`锛屾妸 `libass` 宸窛鎷嗕负鏄庣‘娓呭崟銆?- 鏂板 `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`锛屾槑纭樉绀虹骇 HDR 杈撳嚭搴旇蛋 `DXGI HDR swapchain bridge` 璺緞锛屽苟瀹氫箟 metadata/ICC 绛栫暐杈圭晫銆?- 鏇存柊 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md` 涓?records 鏂囨。銆?
### 鏈湴楠屾敹
- `cmake --build build --config Release`锛氶€氳繃銆?- `./build/Release/modern-video-player.exe --opengl-diagnostics`
  - `probe_succeeded=true`
  - `has_wgl_dx_interop=true`
  - `native_interop.allowed=true`
  - `result=PASS`
- `$env:MVP_OPENGL_NATIVE_INTEROP='disable'; ./build/Release/modern-video-player.exe --opengl-diagnostics`
  - `native_interop.env_override=disable`
  - `native_interop.allowed=false`
  - `native_interop.disable_rule=env_disable`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000`
  - `renderer_backend=OpenGL`
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `result=PASS`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass`
  - `mismatches=0`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass`
  - `subtitle_loaded=true`
  - `probe_found=true`
  - `result=PASS`

### 淇敼鏂囦欢
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 112: OpenGL interaction freeze on mouse/keyboard/window events

**Date**: 2026-03-25

### Problem Description
- OpenGL playback could freeze after mouse move/click interaction.
- After freeze, hotkeys and window operations (maximize/minimize/fullscreen) became unresponsive.
- This looked like a hard UI stall instead of normal performance backpressure.

### Root Cause Analysis
- OpenGL renderer pumped SDL events on the render thread (`renderLoop -> pumpEvents()`), while the control loop and window lifecycle lived on the main thread.
- Under Windows event-heavy interaction, this thread-affinity split can stall the message path and produce apparent deadlock behavior.

### Solution
- Moved OpenGL SDL event pumping to `handleEvents()` (main-thread call path via `PlayerCore::pumpEvents()`).
- Removed `pumpEvents()` from the render thread loop.
- Moved fullscreen toggle execution (`SDL_SetWindowFullscreen`) to `handleEvents()` after event pumping.
- Kept render thread focused on frame processing/present and request-driven redraw.

### Validation
- `cmake --build build --config Release --target modern-video-player` (VS CMake path) -> PASS
- `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY45_OPENGL_EVENT_THREAD_AFFINITY_FREEZE_FIX.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 111: OpenGL HDR output policy + 3D LUT output baseline

**Date**: 2026-03-25

### Problem Description
- OpenGL still lacked an explicit output-color control plane for HDR-mode policy and post-output LUT handling.
- HDR/SDR output behavior could not be explicitly selected for playback diagnostics beyond internal tone-map logic.
- There was no dedicated machine-readable regression command to verify OpenGL output-stage HDR/LUT wiring.

### Root Cause Analysis
- Existing OpenGL HDR logic primarily lived inside per-frame color conversion/tone-map decisions.
- Renderer diagnostics did not expose enough output-stage policy signals to distinguish request vs active state.
- ICC/LUT backlog had design docs, but runtime plumbing for a practical LUT path was still missing.

### Solution
- Added OpenGL HDR output mode policy controls:
  - env: `MVP_OPENGL_HDR_OUTPUT_MODE=auto|off|force`
  - CLI: `--opengl-hdr-output-mode <auto|off|force>`
- Added OpenGL output 3D LUT controls:
  - env: `MVP_OPENGL_3DLUT_FILE=<cube_lut_file>`
  - CLI: `--opengl-3dlut <cube_lut_file>`
- Implemented `.cube` parser + OpenGL 3D LUT texture upload and output-stage sampling.
- Extended diagnostics chain (`RendererDiagnostics` -> `DiagnosticsSnapshot` -> `--performance-log-check`) with:
  - `renderer_opengl_hdr_bridge_*`
  - `renderer_opengl_output_lut_*`
- Added machine-readable regression command:
  - `--opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`

### Validation
- `cmake --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-check .\juren-30s.mp4 .\samples\lut\identity_2.cube 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='opengl'; $env:MVP_OPENGL_3DLUT_FILE='.\samples\lut\identity_2.cube'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY44_OPENGL_HDR_OUTPUT_POLICY_AND_3DLUT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 110: Embedded bitmap subtitle path and DirectWrite subtitle font collection

**Date**: 2026-03-25

### Problem Description
- Embedded subtitle multi-track switching and text playback were available, but bitmap subtitle codecs (PGS/DVD) were still outside the rendering path.
- Private subtitle fonts were registered, but renderers still lacked an explicit DirectWrite custom font collection binding.
- CLI diagnostics could not clearly expose bitmap-track coverage and custom font collection readiness.

### Root Cause Analysis
- Embedded subtitle policy and documentation still partially treated track selection as text-only.
- The embedded loader lacked a bitmap decode branch from `AVSubtitleRect` to renderer model payload.
- OpenGL/D3D11 subtitle loops handled text-only rendering paths.
- Font registration and DirectWrite collection construction were not connected end-to-end.

### Solution
- Extended embedded subtitle selection policy to supported codecs (`supported_codec`) instead of text-only.
- Added embedded bitmap subtitle decode path for:
  - `AV_CODEC_ID_HDMV_PGS_SUBTITLE`
  - `AV_CODEC_ID_DVD_SUBTITLE`
- Added `SubtitleBitmap` model fields and renderer bitmap branches in both OpenGL and D3D11 subtitle renderers.
- Added DirectWrite custom subtitle font collection builder from registered private fonts and integrated it into both subtitle renderers.
- Added/extended diagnostics:
  - `--directwrite-font-collection-check <media_file>`
  - `embedded-subtitle-check.bitmap_codec`
  - `embedded-subtitle-check.bitmap_item_count`
  - `embedded-subtitle-list.supported_bitmap_track_count`
  - `embedded-subtitle-select-check.bitmap_codec`
  - `embedded-subtitle-select-check.bitmap_item_count`

### Validation
- `MSBuild.exe build/modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2` -> PASS
- `.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY43_BITMAP_SUBTITLE_AND_DWRITE_COLLECTION.md`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 闂 97: OpenGL ASS/SSA銆乨river quirk 鏀舵暃涓?HDR/鑹插僵绠＄悊琛ラ綈

**鏃ユ湡**: 2026-03-24

### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰缁х画鎶?`OpenGL` 寰€鎴愮啛鎾斁鍣ㄦ柟鍚戞帹杩涳紝浼樺厛琛ラ綈 `ASS/SSA`銆乨river quirk 鏀舵暃鍜?`HDR / 鑹插僵绠＄悊`銆?- 褰撳墠 `OpenGL` 铏藉凡鍏峰鍙挱鏀鹃摼璺拰 `D3D11VA -> OpenGL` 鍘熺敓浜掓搷浣滐紝浣嗗瓧骞曘€佸惎鍔ㄦ湡绛栫暐鍜岃壊褰╁鐞嗕粛鍋?`M0/M1-` 杩囨浮鐘舵€併€?
### 鍘熷洜鍒嗘瀽
- 鏃?OpenGL 瀛楀箷璺緞浠嶅亸绠€鍗曞彔瀛楋紝涓嶈兘绋冲畾鎵挎帴 `ASS/SSA` 鐨?item/run 鏍峰紡淇℃伅銆?- native interop 涓昏渚濊禆杩愯鏈熷け璐ュ洖閫€锛岀己灏戝惎鍔ㄦ湡鐜绛栫暐銆侀€傞厤鍣?椹卞姩璇婃柇鍜屼繚瀹?blacklist銆?- 杞欢涓婁紶鍜屽師鐢?interop 涓ゆ潯 OpenGL 璺緞鐨勮壊褰╁鐞嗘鍓嶆病鏈夋敹鏁涘埌涓€濂楃粺涓€涓斿彲瑙傛祴鐨勭绾裤€?
### 瑙ｅ喅鏂规
- 鍦?`src/render/opengl_video_renderer.cpp` 涓皢 OpenGL 瀛楀箷娓叉煋鍗囩骇涓?`DirectWrite + D2D offscreen -> GL texture`锛屾敮鎸佸 `SubtitleItem` 鎺掑簭銆乮tem/run 鍒嗘鏍峰紡銆佸畾浣嶅榻愩€佹弿杈广€侀槾褰便€佽儗鏅鍜屽～鍏呫€?- 涓?OpenGL native interop 澧炲姞 `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`銆佽蒋浠?GL blacklist 鍜?`[diag:opengl-native]` 鍚姩鏈熻瘖鏂€?- 缁熶竴杞欢/鍘熺敓 OpenGL 鑹插僵閾捐矾锛氳ˉ榻?`BT.601 / 709 / 2020`銆乣PQ / HLG`銆佸熀纭€ tone-map 鍜?`BT.2020 -> BT.709` gamut mapping锛屽苟杈撳嚭 `[diag:opengl-color]`銆?- 鏂板 `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md` 璁板綍 implementation planner锛屽苟鏇存柊 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`銆?
### 鏈湴楠屾敹
- `cmake --build build --config Release`锛氶€氳繃銆?- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000`
  - `renderer_backend=OpenGL`
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `render_frames=47`
  - `result=PASS`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass`
  - `entries=2`
  - `mismatches=0`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass`
  - `subtitle_loaded=true`
  - `subtitle_entries=2`
  - `probe_found=true`
  - `result=PASS`

### 淇敼鏂囦欢
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 闂 96: OpenGL 娓叉煋閾捐矾 M0 钀藉湴骞跺畬鎴愭湰鍦伴獙鏀?
**鏃ユ湡**: 2026-03-24

### 闂鎻忚堪
- `OpenGLVideoRenderer` 涔嬪墠浠嶆槸 stub锛屾樉寮忛€夋嫨 `OpenGL` 鍚庣鏃跺彧鑳藉洖閫€ `SoftwareSDL`銆?- 鐢ㄦ埛瑕佹眰鎶?OpenGL 閾捐矾琛ユ垚鍙疄闄呮挱鏀剧殑鍚庣锛屽苟鏄庣‘瀹冧笌鎴愮啛鎾斁鍣?GPU 娓叉煋閾捐矾鐨勫樊璺濄€?
### 鍘熷洜鍒嗘瀽
- 鏃у疄鐜扮己灏?SDL OpenGL 绐楀彛銆佷笂涓嬫枃銆乻hader銆佺汗鐞嗕笂浼犮€侀鑹茶浆鎹笌浜嬩欢娉碉紝鍙湁鎺ュ彛娌℃湁鐪熸娓叉煋璺緞銆?- 鍥犳鏄惧紡璁剧疆 `MVP_RENDERER_BACKEND=opengl` 鏃讹紝骞朵笉瀛樺湪鍙氦浠樼殑瑙嗛鏄剧ず鑳藉姏銆?
### 瑙ｅ喅鏂规
- 瀹炵幇 `SDL + OpenGL 2.1 compatibility context + GLSL 120` 鐨勬渶灏忓彲鐢ㄦ覆鏌撶嚎绋嬨€?- 鏀寔 `YUV420P / NV12` 鐩存帴甯ф牸寮忋€佸熀纭€鐑敭鎺у埗銆佸叏灞忓垏鎹€侀€€鍑轰簨浠朵笌澶辫触鑷姩鍥為€€銆?- 鏂板 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md` 璁板綍鏈湴楠屾敹缁撴灉锛屽苟鍚屾鍔熻兘璇存槑涓庡樊璺濇枃妗ｅ彛寰勩€?- 淇濇寔榛樿鍚庣绛栫暐涓嶅彉锛屽綋鍓?`OpenGL` 浠嶄负 opt-in 鐨?M0 杩囨浮鍚庣銆?
### 鏈湴楠屾敹
- `Release` 鏋勫缓閫氳繃銆?- `MVP_RENDERER_BACKEND=opengl` + `--performance-log-check .\juren-30s.mp4 2000` 杈撳嚭锛?  - `performance-log-check.renderer_backend=OpenGL`
  - `performance-log-check.decoder_backend=D3D11VA`
  - `performance-log-check.result=PASS`

### 淇敼鏂囦欢
- `CMakeLists.txt`
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md`
- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`







閺堫剚鏋冨锝堫唶瑜版洖绱戦崣鎴ｇ箖缁嬪鑵戦柆鍥у煂閻ㄥ嫰妫舵０妯哄挤閸忔儼袙閸愯櫕鏌熷鍫涒偓?






---







## 闂傤噣顣介崚妤勩€?






| # | 閺冦儲婀?| 闂傤噣顣?| 閻樿埖鈧?|



|---|------|------|------|



| 1 | 2026-02-17 | FFmpeg 8.0 閸忕厧顔愰幀褔妫舵０?| 閴?瀹歌弓鎱ㄦ径?|



| 2 | 2026-02-24 | 鐟欏棝顣跺ù浣哄偍瀵洑绗夐崠褰掑帳 | 閴?瀹歌弓鎱ㄦ径?|



| 3 | 2026-02-24 | 闂婃娊顣跺ù浣哄偍瀵洑绗夐崠褰掑帳 | 閴?瀹歌弓鎱ㄦ径?|



| 4 | 2026-02-24 | YUV 閺佺増宓佸〒鍙夌厠闁挎瑨顕?| 閴?瀹歌弓鎱ㄦ径?|



| 5 | 2026-02-24 | 娴间椒绗熺痪?Quill 閺冦儱绻旈柅姘朵壕 | 閴?瀹歌弓鎱ㄦ径?|



| 6 | 2026-02-25 | 婢舵氨鍤庣粙瀣尡閺€鐐仸閺嬪嫰鍣搁弸?| 閴?瀹告彃鐣幋?|



| 7 | 2026-02-25 | 闂婃娊顣堕幘顓熸杹閺嬭埖鐎穱顔碱槻 | 閴?瀹歌弓鎱ㄦ径?|



| 9 | 2026-02-25 | VideoFrame/AudioFrame 缁夎濮╃拠顓濈疅缂傛椽娅?| 閴?瀹歌弓鎱ㄦ径?|



| 10 | 2026-02-25 | 婢舵俺袙閻礁娅掔€圭偘绶ョ粩鐐扮挨鐠囪褰囩€佃壈鍤х憴锝囩垳闁挎瑨顕?| 閴?瀹歌弓鎱ㄦ径?|



| 11 | 2026-02-27 | 楠炶泛褰傜拠璇插絿 AVFormatContext 鐎佃壈鍤у畷鈺傜皾 | 閴?瀹歌弓鎱ㄦ径?|



| 12 | 2026-02-27 | 娴间椒绗熺痪褍顦跨痪璺ㄢ柤閺嬭埖鐎柌宥嗙€?| 閴?瀹告彃鐣幋?|



| 15 | 2026-03-06 | 鐏忓繐鐫嗙粣妤€褰涙潻鍥с亣娑撴梹瀚嬮幏鐣岀級閺€鍙ョ瑝缁嬪啿鐣?| 閴?瀹歌弓鎱ㄦ径?|



| 18 | 2026-03-07 | DASH 鐟欙絾鐎界紓鏍槯婢惰精瑙︽稉搴㈢壐瀵繗鍏橀崝娑氱叐闂冪數宸辨径?| 閴?瀹歌弓鎱ㄦ径?|



| 19 | 2026-03-07 | D3D11VA 绾剝袙閺堚偓鐏忓繘妫撮悳顖欑瑢鏉烆垵袙閸ョ偤鈧偓 | 閴?瀹歌弓鎱ㄦ径?|



| 20 | 2026-03-07 | 閹恒垺绁撮崗銉ュ經娑撳孩鐗稿蹇撴礀瑜版帟鍓奸張顒冩儰閸?| 閴?瀹歌弓鎱ㄦ径?|



| 21 | 2026-03-07 | GitHub Actions 閼奉亜濮╅弽鐓庣础閸ョ偛缍婇幒銉ュ弳 | 閴?瀹歌弓鎱ㄦ径?|



| 22 | 2026-03-07 | 閹绢厽鏂侀崚妤勩€冩稉濠氭懠鐠侯垬鈧浇顔曠純顔藉瘮娑斿懎瀵叉稉搴℃彥閹圭兘鏁＃鏍閹恒儱鍙?| 閴?瀹歌弓鎱ㄦ径?|



| 23 | 2026-03-07 | 缁夊娅?Core 閸楁洖鍘撳ù瀣槸閻╊喗鐖ｆ稉搴㈢ゴ鐠囨洘鏋冩禒?| 閴?瀹歌弓鎱ㄦ径?|



| 24 | 2026-03-07 | 婢舵牗瀵曠€涙绠烽崝鐘烘祰閸忋儱褰涢敍鍦玆T閿涘甯撮崗銉ゅ瘜濞翠胶鈻?| 閴?瀹歌弓鎱ㄦ径?|



| 25 | 2026-03-07 | 鐎涙绠峰〒鍙夌厠閸欑姴濮炴稉搴㈡尡閺€鐐鎼村繐鎮撳銉﹀复閸?| 閴?瀹歌弓鎱ㄦ径?|



| 26 | 2026-03-08 | 鐎涙绠峰鈧崗铏付閸掓湹绗岀€涙绠烽崝鐘烘祰瀵倸鐖舵径鍕倞鐎瑰苯鏉?| 閴?瀹歌弓鎱ㄦ径?|



| 27 | 2026-03-08 | 韫囶偅宓庨柨顕€鍘ょ純顔藉瘮娑斿懎瀵查幒銉ュ弳閿涘潝otkey.*閿?| 閴?瀹歌弓鎱ㄦ径?|



| 28 | 2026-03-08 | 韫囶偅宓庨柨顔煎暱缁愪焦顥呭ù瀣╃瑢閹垹顦叉妯款吇閼宠棄濮?| 閴?瀹歌弓鎱ㄦ径?|



| 29 | 2026-03-08 | M1 妤犲本鏁?1.4.1閿涙瓔RT seek 閸氬本顒為懛顏咁梾閸涙垝鎶ら拃钘夋勾 | 閴?瀹歌弓鎱ㄦ径?|



| 30 | 2026-03-08 | M1 妤犲本鏁?1.4.2閿涙碍鎸遍弨鎯у灙鐞涖劏绻涚紒顓熸尡閺€?5 閺傚洣娆㈤懛顏咁梾闁俺绻?| 閴?瀹歌弓鎱ㄦ径?|



| 31 | 2026-03-08 | M1 妤犲本鏁?1.4.3閿涙俺顔曠純顕€鍣搁崥顖涗划婢跺秷鍤滃Λ鈧柅姘崇箖 | 閴?瀹歌弓鎱ㄦ径?|



| 32 | 2026-03-08 | M2 2.1.2閿涙艾顔愰崳銊х叐闂冧絻藟姒?mov/avi/m2ts 楠炶泛娲栬ぐ鎺椻偓姘崇箖 | 閴?瀹歌弓鎱ㄦ径?|



| 33 | 2026-03-08 | M2 2.1.3閿涙俺顫嬫０鎴犵椽閻胶鐓╅梼浣兯夋?MPEG-2 楠炶泛娲栬ぐ鎺椻偓姘崇箖 | 閴?瀹歌弓鎱ㄦ径?|



| 34 | 2026-03-08 | M2 2.1.4閿涙岸鐓舵０鎴犵椽閻胶鐓╅梼浣兯夋?E-AC3/DTS/Vorbis/PCM 楠炶泛娲栬ぐ鎺椻偓姘崇箖 | 閴?瀹歌弓鎱ㄦ径?|



| 35 | 2026-03-08 | M3 3.1.1閿涙ecoderFactory 閹恒儱鍙嗛惇鐔风杽閸掓繂顫愰崠鏍ㄧウ缁?| 閴?瀹歌弓鎱ㄦ径?|



| 36 | 2026-03-08 | M3 3.1.2閿涙3D11VA 閸楀繐鏅㈡径杈Е鏉烆垵袙閸忔粌绨崇€瑰苯鏉?| 閴?瀹歌弓鎱ㄦ径?|



| 37 | 2026-03-08 | M3 3.2.1閿涙3D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻劑鎽肩捄顖濇儰閸?| 閴?瀹歌弓鎱ㄦ径?|



| 38 | 2026-03-08 | M3 3.3.2閿涙碍瑕嗛弻鎾炽亼鐠愩儵妾风痪褍娲栬ぐ鎺戝弳閸欙綀藟姒?| 閴?瀹歌弓鎱ㄦ径?|



| 39 | 2026-03-08 | M3 3.3.1閿涙瓙indows 鏉烆垵袙/绾剝袙娑撹濮忛弽閿嬫拱閸ョ偛缍婇柅姘崇箖 | 閴?瀹歌弓鎱ㄦ径?|



| 40 | 2026-03-08 | M4 4.1閿涙氨鐝烽懞鍌氼嚤閼割亷绱欐稉濠佺缁?娑撳绔寸粩鐙呯礆閹恒儱鍙嗘稉搴ㄧ崣閺€?| 閴?瀹歌弓鎱ㄦ径?|



| 41 | 2026-03-08 | M4 4.2閿涙-B Repeat閿涘湏/B/C閿涘甯撮崗銉ょ瑢妤犲本鏁?| 閴?瀹歌弓鎱ㄦ径?|



| 42 | 2026-03-08 | M4 4.3閿涘牊鍩呴崶鎾呯礆 | 閴?瀹歌弓鎱ㄦ径?|



| 43 | 2026-03-08 | `MPC_HC_GAP_ANALYSIS` 鐠囧嫪鍙婄紒鎾诡啈鏉╁洦婀?| 閴?瀹歌弓鎱ㄦ径?|



| 44 | 2026-03-08 | `docs/records/VERSION.md` 閸樺棗褰剁捄顖氱窞閹诲繗鍫潻鍥ㄦ埂 | 閴?瀹歌弓鎱ㄦ径?|



| 45 | 2026-03-08 | README 娑撳孩鐏﹂弸鍕瀮濡楋絼绮涘ǎ椋庢暏閺冄傚瘜闁炬崘銆冩潻?| 閴?瀹歌弓鎱ㄦ径?|



| 46 | 2026-03-08 | 鐎圭偟骞囬弫娆戔柤娑撳氦鍑禒锝堫吀閸掓帞宸辩亸鎴濆坊閸?瑜版挸澧犳潏鍦櫕鐠囧瓨妲?| 閴?瀹歌弓鎱ㄦ径?|



| 47 | 2026-03-08 | 鏉堝懎濮拠瀛樻閺傚洦銆傛禒宥囧繁鐏忔垵缍嬮崜宥呭弳閸欙絼绗岄悩鑸碘偓浣界珶閻?| 閴?瀹歌弓鎱ㄦ径?|



| 48 | 2026-03-08 | 閺?README 閺佸懘娈伴幒鎺楁珟娑撳骸宸婚崣鏌ユ６妫版ê缍婂锝勭矝閺堝妫崣锝呯窞 | 閴?瀹歌弓鎱ㄦ径?|



| 49 | 2026-03-08 | 缂傚搫鐨悪顒傜彌閻ㄥ嫭鏋冨锝呰窗濡偓閹槒銆?| 閴?瀹歌弓鎱ㄦ径?|



| 50 | 2026-03-08 | M4 4.4閿涙碍娈忛崑婊勨偓浣告姎濮濄儴绻橀幒銉ュ弳娑撳酣鐛欓弨?| 閴?瀹歌弓鎱ㄦ径?|



| 53 | 2026-03-08 | M2 2.2.4閿涙俺绶崙鐑樻尡閺€鐐偓褑鍏橀弮銉ョ箶閿涘牊甯€鐢?闂冪喎鍨?CPU/GPU閿?| 閴?瀹歌弓鎱ㄦ径?|



| 54 | 2026-03-08 | M2 2.2.1 / 2.3.2閿?080p60 缁嬪啿鐣鹃幘顓熸杹妤犲本鏁?| 閴?瀹歌弓鎱ㄦ径?|



| 55 | 2026-03-08 | M2 2.2.2 / 2.3.3閿?K 閹绢厽鏂佹稉搴ㄦ缁狙囩崣閺€?| 閴?瀹歌弓鎱ㄦ径?|



| 56 | 2026-03-08 | M2 2.2.3閿?80Mbps 妤傛鐖滈悳鍥ㄧ壉閺堫剟鐛欓弨?| 閴?瀹歌弓鎱ㄦ径?|



| 57 | 2026-03-18 | D3D11 閸樼喓鏁?GPU 濞撳弶鐓嬮柧鎹愃夋?| 閴?瀹歌弓鎱ㄦ径?|



| 66 | 2026-03-18 | 閸忋劌鐪弸鍕紦闂冭顢ｅ〒鍛倞娑?ASS/SSA 閸樼喓鏁?D3D11 鐎涙绠烽柧?| 閴?瀹歌弓鎱ㄦ径?|



| 67 | 2026-03-18 | ASS 閺嶅洨顒风憴锝嗙€芥稉?UTF-16 鐎涙绠烽懠鍐ㄦ纯娣囶喗顒?| 閴?瀹歌弓鎱ㄦ径?|



| 68 | 2026-03-18 | MSVC warning debt 閸掑棗鐪板〒鍛倞閿涘湑4819 / C4996 / C4706閿?| 閴?瀹歌弓鎱ㄦ径?|



| 69 | 2026-03-19 | PlayerCore 閸嬫粍鎸遍弨璺哄經閵嗕礁瀵橀梼鐔峰灙閹碘偓閺堝娼堟稉?Clock/Demuxer 鐠佹崘顓搁崐杞版叏婢?| 閴?瀹歌弓鎱ㄦ径?|



| 70 | 2026-03-19 | 闂婃娊顣剁拋鎯ь槵婢惰精瑙﹂弮鍓佹畱鐟欏棝顣?only闂勫秶楠囨稉搴℃礀瑜版帡妫粋浣虹眰閸?| 閴?瀹歌弓鎱ㄦ径?|



| 71 | 2026-03-19 | 4K backend session 鐎涙劘绻樼粙瀣偓鈧崙楦跨熅瀵板嫪鎱ㄦ径?| 閴?瀹歌弓鎱ㄦ径?|



| 72 | 2026-03-19 | 妤傛鐖滈悳?4K 闂冪喎鍨€瑰綊鍣洪妴浣藉殰闁倸绨查懞鍌涚ウ娑?copy-back 鐠囧﹥鏌囨晶鐐插繁 | 閴?瀹歌弓鎱ㄦ径?|



| 73 | 2026-03-19 | SoftwareSDL 閹风柉绀夐柧鎹愮熅闁插繐瀵查妴涓糲heduler 闁插秴鎯庢０鍕暬娑?renderer override | 閴?瀹歌弓鎱ㄦ径?|



| 74 | 2026-03-19 | Audio-master lateness 閺€鍓佹彛娑?SoftwareSDL 閸戝繑瀚圭拹婵囨箒闂勬劙鍣搁弸?| 閴?瀹歌弓鎱ㄦ径?|



| 75 | 2026-03-19 | 閹俱倕娲?SoftwareSDL automatic software-first 楠炴儼藟鏉烆垵袙闂冭顢ｇ拠濠冩焽 | 閴?瀹歌弓鎱ㄦ径?|



| 76 | 2026-03-19 | Software video decode 閻喎鐤勬禍褍鎶氭稉鎾汇€嶅Λ鈧弻銉ょ瑢 blocker 鐎规矮缍?| 閴?瀹歌弓鎱ㄦ径?|



| 77 | 2026-03-19 | Software decode 妫ｆ牕瀵橀崑婊勭哺婢跺秵鐗虫稉?SDL renderer 濞夈劑鍣存稊杈╃垳娣囶喖顦?| 閴?瀹歌弓鎱ㄦ径?|



| 78 | 2026-03-19 | software decode 閺堚偓鐏?send/dequeue 鐠佲剝鏆熼幒銉ュ弳娑撳酣顩婚崠鍛粹偓浣稿瘶閸嬫粍绮搁柦澶嬵劥 | 閴?瀹歌弓鎱ㄦ径?|



| 79 | 2026-03-19 | PlayerCore 鏉╂劘顢戦幀?software send probe 鐎靛湱鍙庨弨鑸垫殐 | 棣冩敵 瀹告彃鐣炬担?|



| 80 | 2026-03-19 | 閺傚洦銆傛稉鈧懛瀛樷偓褑藟姒绘劧绱癈HANGELOG 缁便垹绱╂穱顔碱槻娑撳酣妫舵０?69 analysis 閸ョ偛锝?| 閴?瀹歌弓鎱ㄦ径?|



| 81 | 2026-03-20 | PlayerCore seek/flush timeline serial 閸栨牜顑囨禍宀勬▉濞?| 閴?瀹歌弓鎱ㄦ径?|
| 82 | 2026-03-20 | PlayerCore EOF/Ended 缂佸牊鈧浇顕㈡稊澶愬櫢鐠佹崘顓?| 閴?瀹歌弓鎱ㄦ径?|
| 83 | 2026-03-20 | PlayerCore queue generation 娑?Scheduler 閹貉冨煑韫囶偆鍙庨弨璺哄經 | 閴?瀹歌弓鎱ㄦ径?|
| 84 | 2026-03-20 | PlayerCore 閸擃垯缍旈悽銊╂肠娑擃厼瀵叉稉?runtime failure/recovery policy 閺€璺哄經 | 閴?瀹歌弓鎱ㄦ径?|
| 85 | 2026-03-20 | PlayerCore 閸撯晙缍戞搴ㄦ珦閺€鑸垫殐閿涙瓔cheduler 缂佸牏澧楃粵鏍殣閵嗕笚ailSession 鐎圭偛瀵叉稉?serial/generation 鐟欏倹绁村鍝勫 | 閴?瀹歌弓鎱ㄦ径?|
| 86 | 2026-03-20 | 婢х偠藟 serial/failsession 閸ョ偛缍婇幒銏ゆ嫛閿涘牐绻涚紒?seek閵嗕焦娈忛崑婊勨偓?seek閵嗕恭lose/reopen閿?| 閴?瀹歌弓鎱ㄦ径?|
| 87 | 2026-03-20 | serial/failsession 閸ョ偛缍婃晶鐐插娑撯偓闁款喛浠涢崥?gate閿涘牓妾锋担搴㈢础鐠烘垿顥撻梽鈺嬬礆 | 閴?瀹歌弓鎱ㄦ径?|
| 88 | 2026-03-20 | 瀵搫鍩?FailSession 閸ョ偛缍婇幒銏ゆ嫛娑?codec 闁夸線鍣搁崗銉ョ┛濠у啩鎱ㄦ径?| 閴?瀹歌弓鎱ㄦ径?|
| 89 | 2026-03-20 | run_all_checks 閹恒儱鍙?forced-failsession 娑撯偓闁?gate | 閴?瀹歌弓鎱ㄦ径?|
| 90 | 2026-03-23 | D3D11 閸樼喓鏁撻惄鎾櫚閺嶇兘绮︾仦蹇ョ窗鏉╂劘顢戦弮鍓侇洣閻?native direct 楠炶泛娲栭柅鈧?copy-back | 閴?瀹歌弓鎱ㄦ径?|
| 91 | 2026-03-23 | D3D11VA 閼奉亜鐣炬稊?hw_frames_ctx閿涙氨鏁电拠宄板讲闁插洦鐗辩憴锝囩垳鐞涖劑娼伴獮鑸典划婢跺秹娴傞幏鐤閻╂挳鍣伴弽?| 閴?瀹歌弓鎱ㄦ径?|
| 92 | 2026-03-23 | D3D11 閸氼垰濮╅張鐔诲厴閸旀稒甯板ù瀣╃瑢 adapter/driver 鐠囧﹥鏌囬弮銉ョ箶鐞涖儵缍?| 閴?瀹歌弓鎱ㄦ径?|
| 93 | 2026-03-23 | D3D11 decoder profile 閹恒垺绁撮妴涔箄irk blacklist 娑撳海瀚粩?diagnostics CLI | 閴?瀹歌弓鎱ㄦ径?|
| 94 | 2026-03-23 | 1.0.0-rc1 閸欐垵绔烽崙鍡楊槵閿涙艾褰傜敮鍐╃閸楁洏鈧礁鍑￠惌銉╂６妫版ü绗岄崣鎴濈鐠囧瓨妲戦弨璺哄經 | 閴?瀹歌弓鎱ㄦ径?|
| 95 | 2026-03-23 | RC 閻楀牊婀伴崗鍐╂殶閹诡喓鈧阜elease 妞ゅ灚顒滈弬鍥︾瑢鐎瑰顥婇崠鍛閺堫剚鐖ｇ拠鍡毸夋?| 閴?瀹歌弓鎱ㄦ径?|







## 闂傤噣顣?95: RC 閻楀牊婀伴崗鍐╂殶閹诡喓鈧阜elease 妞ゅ灚顒滈弬鍥︾瑢鐎瑰顥婇崠鍛閺堫剚鐖ｇ拠鍡毸夋?
**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴閺勫海鈥?Release 妞ゅ灚顒滈弬鍥ㄦ杹閸︺劌鎽㈤柌宀嬬礉楠炴儼顩﹀Ч鍌涘Ω缁嬪绨崘鍛村劥閻楀牊婀伴妴涔刬ndows 閸欘垱澧界悰灞炬瀮娴犲墎澧楅張顒€鎷扮€瑰顥婇崠鍛閺堫剟鍏樼紒鐔剁閺勫墽銇氭稉?`rc1`閵?
- 瑜版挻妞傚銉р柤闁插瞼娈?`CMakeLists.txt` 娴犲秴褰ч張?`project(... VERSION 1.0.0)`閿涘奔绮ㄦ惔鎾诲櫡娑旂喐鐥呴張澶婂礋閻欘剛娈?Release 濮濓絾鏋冮弬鍥︽閵嗕梗--version` CLI閵嗕梗VERSIONINFO` 鐠у嫭绨崪?`CPack` 閸栧懎鎳￠崥宥堫潐閸掓瑣鈧?
- `HTTP` 娑撳娴囬柧鎹愮熅閻?`user_agent` 娴犲秴娴愮€规矮璐?`modern-video-player/1.0`閿涘奔绗夐崚鈺€绨崠鍝勫瀻 RC 閺嬪嫬缂撴稉搴℃倵缂侇厽顒滃蹇曞/閸氬海鐢婚崐娆撯偓澶屽閵?
### 閸樼喎娲滈崚鍡樼€?
- 閸樼喐婀侀悧鍫熸拱娣団剝浼呴崣顏勪粻閻ｆ瑥婀?`CMake project version`閿涘本鐥呴張澶嬪Ω prerelease suffix 娴肩姵鎸遍崚鎵柤鎼村繐鍞撮柈銊ョ摟缁楋缚瑕嗛妴涔刬ndows 鐠у嫭绨穱鈩冧紖閸滃苯褰傜敮鍐ㄥ瘶閸涜棄鎮曢妴?
- 妞ゅ湱娲板銈呭缂傚搫鐨?`VERSIONINFO` 鐠у嫭绨稉搴ｅ缁?`Release Notes` 閺傚洣娆㈤敍灞芥礈濮濄倐鈧粌褰查崣?RC閳ユ繄娈戦崘鍛村劥缂佹捁顔戦崪灞糕偓婊冨讲閻╁瓨甯寸拹鏉戝毉閸樿崵娈戦崣鎴濈濮濓絾鏋冮垾婵呯闂傜繝绮涢張澶屽繁閸欙絻鈧?
- 濞屸剝婀侀幍鎾冲瘶鐟欏嫬鍨弮璁圭礉閸樺缂夐崠鍛嚒閸氬秲鈧礁鍞寸€圭绔熼悾灞芥嫲閺勵垰鎯佸ǎ宄板弳閺堫剙婀撮柊宥囩枂娑旂喖鍏橀弮鐘崇《鐞氼偉鍤滈崝銊╃崣鐠囦降鈧?
### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`閿涘奔缍旀稉?GitHub Release 妞ら潧褰查惄瀛樺复娴ｈ法鏁ら惃鍕劀閺傚洢鈧?
- 閸?`CMakeLists.txt` 娑擃厼绱╅崗銉х埠娑撯偓閻楀牊婀板┃鎰剁礉閻㈢喐鍨?`mvp_version.h` 娑?Windows `version_info.rc`閿涘瞼绮烘稉鈧潏鎾冲毉 `1.0.0-rc1`閵?
- `main` 閺傛澘顤?`--version`閿涙矖http_stream_downloader.cpp` 閺€閫涜礋婢跺秶鏁ょ紒鐔剁閻楀牊婀版径杈剧礉`user_agent` 閸欐ü璐?`modern-video-player/1.0.0-rc1`閵?
- 閺傛澘顤?`CPack ZIP` 閹垫挸瀵樼憴鍕灟娑撳骸鐣ㄧ憗鍛淬€嶉敍宀勭崣鐠囦椒楠囬悧鈺傛瀮娴犺泛鎮曟稉?`modern-video-player-1.0.0-rc1-windows-x64.zip`閿涘苯鑻熺亸?`RELEASE_NOTES.md` 娑撯偓楠炶埖澧﹂崠鍜冪礉閸氬本妞傞幒鎺楁珟閺堫剙婀?`config/player_settings.ini`閵?
- 閸╄桨绨?Release 閺嬪嫬缂撴宀冪槈閿涙瓪--version`閵嗕箘indows `FileVersionInfo`閵嗕梗PACKAGE` 閻╊喗鐖ｉ崪?`--d3d11-diagnostics`閵?
### 娣囶喗鏁奸弬鍥︽

- CMakeLists.txt

- cmake/mvp_version.h.in

- cmake/version_info.rc.in

- src/main.cpp

- src/streaming/http_stream_downloader.cpp

- docs/reports/V1_0_0_RC1_RELEASE_NOTES.md

- docs/reports/V1_0_0_RC1_RELEASE_READINESS.md

- docs/reports/README.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/records/VERSION.md

## 闂傤噣顣?94: 1.0.0-rc1 閸欐垵绔烽崙鍡楊槵閿涙艾褰傜敮鍐╃閸楁洏鈧礁鍑￠惌銉╂６妫版ü绗岄崣鎴濈鐠囧瓨妲戦弨璺哄經

**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犻幘顓熸杹閸ｃ劌鍑＄紒蹇撳徔婢跺洣瀵屽ù浣规拱閸︾増鎸遍弨鎹愬厴閸旀稏鈧胶菙鐎?seek 娑撳鎽兼禒銉ュ挤 `D3D11VA + D3D11` 娑撹濮忓〒鍙夌厠鐠侯垰绶為敍灞肩稻娴犳挸绨遍柌灞肩矝缂傚搫鐨稉鈧禒浠嬫桨閸?`RC` 閸欐垵绔烽惃鍕埠娑撯偓缂佹捁顔戦弬鍥ㄣ€傞妴?
- 閻滅増婀佹宀冪槈鐠囦焦宓侀崚鍡樻殠閸?`VERSION / CHANGELOG / DEVELOP_LOG` 娴犮儱寮锋径姘嚋 `reports/*_LOCAL_CHECK.md` 娑擃叏绱濈紓鍝勭毌娑撯偓娑擃亜褰叉禒銉ф纯閹恒儱娲栫粵鏂衡偓婊呭箛閸︺劏鍏樻稉宥堝厴閸?`1.0.0-rc1`閵嗕礁鍑￠惌銉╂６妫版ɑ妲告禒鈧稊鍫涒偓浣割嚠婢舵牞顕氶幀搴濈疄閹诲繗鍫垾婵堟畱閺€璺哄經閸忋儱褰涢妴?
### 閸樼喎娲滈崚鍡樼€?
- 鏉╁洤骞撻惃?records 閺囨潙浜搁崥鎴︽６妫版ü鎱ㄦ径宥呮嫲閼宠棄濮忔晶鐐哄櫤鐠佹澘缍嶉敍灞肩瑝閻╁瓨甯寸粵澶夌幆娴滃骸褰傜敮鍐嚛閺勫簺鈧?
- 閸楀厖绌舵潻鎴炴埂 D3D11閵嗕够erial/failsession閵嗕公ormat regression 缁涘鍏橀崝娑樺嚒缂佸繒鎴风紒顓熸暪閸欙綇绱濆▽鈩冩箒閸楁洜瀚惃?RC 濮瑰洦鈧粯鏋冨锝忕礉娴犲秶鍔х€硅妲楅幎濞锯偓婊冨讲閸?RC閳ユ繂鎷伴垾婊冨讲閸欐垶顒滃蹇曞閳ユ繃璐╂稉杞扮鐠嬪牄鈧?
- 閸氬本妞傞敍灞界秼閸撳秷绻曢張澶夌娑擃亜绻€妞ょ粯妯夊蹇撴啞閻儳娈戝▓瀣╃稇妞嬪酣娅撻敍姝氶梻顕€顣?79` 鐎电懓绨查惃?software video decode 鏉╂劘顢戦幀浣界熅瀵板嫬鐨婚張顏勭暚閸忋劍鏁归崣锝冣偓?
### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?RC 濮瑰洦鈧粯濮ら崨?`docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`閿涘瞼绮烘稉鈧崠鍛儓閿?  - `1.0.0-rc1` 閸欐垵绔风紒鎾诡啈
  - 閺堫剝鐤嗛弬鏉款杻妤犲矁鐦夌拠浣瑰祦
  - 閺冦垺婀侀懗钘夊鐠囦焦宓侀崗銉ュ經
  - 閸欐垵绔风拠瀛樻
  - 瀹歌尙鐓￠梻顕€顣?  - 閸欐垵绔峰〒鍛礋

- 闁插秵鏌婇崺杞扮艾 `Release` 閺嬪嫬缂撻幍褑顢?RC 閻╁瓨甯撮惄绋垮彠濡偓閺屻儻绱?  - `tools/run_all_checks.ps1`
  - `--d3d11-diagnostics`
  - `--performance-log-check .\juren-30s.mp4 2000`
  - `--long-playback-check .\juren-30s.mp4 10000`
  - `--serial-failsession-regression-check .\juren-30s.mp4`

- 閸氬本顒為弴瀛樻煀 `docs/README.md`閵嗕梗docs/reports/README.md` 閸?`docs/records/VERSION.md`閿涘本濡歌ぐ鎾冲 RC 閸婃瑩鈧濮搁幀浣告嫲閸忋儱褰涢弬鍥ㄣ€傞弰鎯х础閸栨牓鈧?
### 娣囶喗鏁奸弬鍥︽

- docs/reports/V1_0_0_RC1_RELEASE_READINESS.md

- docs/reports/FORMAT_REGRESSION_20260323_224615.md

- docs/reports/README.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/records/VERSION.md
## 闂傤噣顣?93: D3D11 decoder profile 閹恒垺绁撮妴涔箄irk blacklist 娑撳海瀚粩?diagnostics CLI

**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 閸︺劑妫舵０?92 瀹歌尪藟姒?D3D11 閸氼垰濮╅張?adapter/driver/format 閺冦儱绻旈崥搴礉瑜版挸澧犳い鍦窗娴犲秶宸辩亸鎴炲灇閻旂喐鎸遍弨鎯ф珤鐢瓕顫嗛惃鍕瑏妞ょ懓鐔€绾偓鐠佺偓鏌﹂敍?  - 鐟欏棝顣剁憴锝囩垳 profile 缁狙冨焼閻ㄥ嫯鍏橀崝娑欏赴濞村绱濋弮鐘崇《閻╁瓨甯撮崶鐐电摕瑜版挸澧犻張鍝勬珤鐎?`H.264 / HEVC / VP9 / AV1` 閸掓澘绨抽弨顖涘瘮閸濐亙绨?decoder profile
  - 閸氼垰濮╅張?quirk / blacklist 缁涙牜鏆愰敍灞炬￥濞夋洖婀鑼叀娑撳秶菙鐎规俺顔曟径鍥︾瑐閹绘劕澧犵粋浣烘暏 native direct
  - 閻欘剛鐝涢妴浣规簚閸ｃ劌褰茬拠鑽ゆ畱 D3D11 鐠囧﹥鏌?CLI閿涘矁鍤滈崝銊ュ閸滃矂妫舵０妯侯槻閻滄澘婧€閺咁垯绮涢崣顏囧厴娓氭繆绂嗛幘顓熸杹閺堢喐妫╄箛?
### 閸樼喎娲滈崚鍡樼€?
- 閺冄冪杽閻滄澘褰ч張?format-level 閼宠棄濮忛幒銏＄ゴ閿涘本鐥呴張澶嬬亣娑?`ID3D11VideoDevice::GetVideoDecoderProfile*`閿涘苯娲滃銈傗偓婊勭壐瀵繗鍏樺铏规睏閻炲棌鈧繂鎷伴垾婊嗩嚉缂傛牜鐖?profile 閼宠棄鎯佺涵顒冃掗垾婵呯闂傜繝绮涚€涙ê婀惄鎻掑隘閵?
- native direct 閻ㄥ嫬鎯庨崑婊勵劃閸撳秳瀵岀憰浣风贩鐠ф牞绻嶇悰灞炬閻旀梹鏌囬敍宀€宸辩亸鎴濆剼 `mpv / MPC-HC` 闁絾鐗遍惃鍕儙閸斻劍婀℃穱婵嗙暓缁涙牜鏆愰敍宀勪海閸?software adapter閵嗕胶宸辨径鍗炲彠闁款喗甯撮崣锝嗗灗閺勫海鈥樻鎴濇倳閸楁洟鈹嶉崝銊︽閿涘奔绗夐懗钘夋躬閹绢厽鏂侀崜宥囨纯閹恒儵妾风痪褋鈧?
- 妞ゅ湱娲伴悳鐗堟箒濡偓閺屻儱鎳℃禒銈勪簰閹绢厽鏂侀柧鎹愮熅娑撹桨鑵戣箛鍐跨礉缂傚搫鐨稉鈧稉顏冪瑝娓氭繆绂嗙€圭偤妾幘顓熸杹閵嗕礁褰叉稉鈧▎鈩冣偓褑绶崙鍝勭暚閺?D3D11 閼宠棄濮忚箛顐ゅ弾閻ㄥ嫮瀚粩瀣弳閸欙絻鈧?
### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`D3D11VideoRenderer` 娑擃厽鏌婃晶鐐电波閺嬪嫬瀵?`D3D11DiagnosticsSnapshot`閿涘瞼绮烘稉鈧Ч鍥ㄢ偓浼欑窗
  - adapter / driver / feature level / interface availability
  - `NV12 / P010 / P016` 閺嶇厧绱￠弨顖涘瘮娴?  - `H.264 / HEVC / VP9 / AV1` decoder profile 閺€顖涘瘮閹懎鍠?  - native direct 閸氼垰濮╅張?allow / disable policy閵嗕礁鎳℃稉顓☆潐閸掓瑥鎷伴崢鐔锋礈

- 閺傛澘顤?decoder profile 閹恒垺绁撮柅鏄忕帆閿涘瞼娲块幒銉︾亣娑?`ID3D11VideoDevice` 閺嗘挳婀堕惃?decoder profiles閿涘苯鑻熸潏鎾冲毉閿?  - `h264_vld_nofgt / h264_vld_fgt`
  - `hevc_main / hevc_main10`
  - `vp9_profile0 / vp9_profile2_10bit`
  - `av1_profile0 / av1_profile1 / av1_profile2 / av1_profile2_12bit / av1_profile2_12bit_420`

- 閺傛澘顤冮崥顖氬З閺?native direct 缁涙牜鏆愰崚銈嗘焽閿涙稖瀚㈤幒銏＄ゴ婢惰精瑙﹂妴涔籵ftware adapter閵嗕胶宸辨径?`ID3D11Device3 / ID3D11VideoDevice / ID3D11VideoContext`閵嗕梗NV12` 娑撳秵寮х搾?`texture2d + shader_sample + decoder_output`閿涘本鍨ㄩ崨鎴掕厬 blacklist閿涘苯鍨崷?renderer 閸掓繂顫愰崠鏍▉濞堢數娲块幒銉ュ彠闂?native direct閵?
- 妫ｆ牜澧?quirk / blacklist 鐟欏嫬鍨弰鎯х础閽€钘夋勾閿?  - `microsoft-basic-render-driver`

- `main` 閺傛澘顤?`--d3d11-diagnostics`閿涘奔浜?`key=value` 瑜般垹绱￠張鍝勬珤閸欘垵顕版潏鎾冲毉閺佺繝閲?D3D11 閼宠棄濮忚箛顐ゅ弾閿涘苯鑻熺紒娆忓毉 `result=PASS/FAIL`閵?
### 娣囶喗鏁奸弬鍥︽

- include/render/d3d11_video_renderer.h

- src/render/d3d11_video_renderer.cpp

- src/main.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?92: D3D11 閸氼垰濮╅張鐔诲厴閸旀稒甯板ù瀣╃瑢 adapter/driver 鐠囧﹥鏌囬弮銉ョ箶鐞涖儵缍?
**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 閸︺劑妫舵０?90 閸滃矂妫舵０?91 瀹告彃鍘涢崥搴ば掗崘鏂モ偓婊堢拨鐏炲繐鍘规惔鏇椻偓婵嗘嫲閳ユ窉3D11VA 閸欘垶鍣伴弽宄版姎濮圭姭鈧繂鎮楅敍瀛?D11 娴犲秶宸辩亸鎴炲灇閻旂喐鎸遍弨鎯ф珤鐢瓕顫嗛惃鍕儙閸斻劍婀￠懗钘夊閹恒垺绁存稉?adapter/driver 鐠囧﹥鏌囬弮銉ョ箶閵?
- 瑜版挸澧犳俊鍌涚亯閸氬海鐢婚崘宥変海閸掔増鐓囬崣鐗堟簚閸ｃ劎娈戦弽鐓庣础閸忕厧顔愰妴渚€鈹嶉崝銊ユ▕瀵倶鈧够wap chain 閹?feature level 闂傤噣顣介敍灞炬）韫囨妫ゅ▔鏇炴躬閸掓繂顫愰崠鏍▉濞堢數娲块幒銉х舶閸戦缚鍐绘径鐔剁瑐娑撳鏋冮妴?
### 閸樼喎娲滈崚鍡樼€?
- 閺冄冪杽閻滄澘褰ч崷銊ュ灥婵瀵查幋鎰閸氬氦绶崙?`Native D3D11 renderer initialized`閿涘本鐥呴張澶庮唶瑜?adapter 閸氬秶袨閵嗕箍endor/device id閵嗕龚river version閵嗕公eature level閵嗕焦鐗宠箛鍐╁复閸欙絽褰查悽銊︹偓褋鈧焦鐗稿蹇旀暜閹镐椒缍呮稉?swap chain 閸欏倹鏆熼妴?
- 鏉╂瑥顕遍懛瀛樺笓閺屻儲妞傞崣顏囧厴娴犲孩鎸遍弨鐐埂閻ュ洨濮搁崣宥嗗腹閿涘矁鈧奔绗夐懗钘夊剼閹存劗鍟涢幘顓熸杹閸ｃ劑鍋呴弽宄版躬閸氼垰濮╅張鐔锋皑閸掋倖鏌囬垾婊冪秼閸撳秷顔曟径鍥ㄦ暜閹镐椒绮堟稊鍫涒偓浣风瑝閺€顖涘瘮娴犫偓娑斿牃鈧縿鈧?
### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`D3D11VideoRenderer` 閸氼垰濮╅弮鑸垫煀婢х偟绮ㄩ弸鍕鐠囧﹥鏌囬弮銉ョ箶閿涘矁绶崙鐚寸窗
  - adapter 閹诲繗鍫妴涔縠ndor/device/subsystem/revision閵嗕龚river version閵嗕焦妯夌€?閸忓彉闊╅崘鍛摠閵嗕焦妲搁崥?software adapter
  - feature level閵嗕龚ebug layer閵嗕沟ultithread protection閵嗕梗ID3D11Device3` / `ID3D11VideoDevice` / `ID3D11VideoContext` 閸欘垳鏁ら幀?  - `NV12 / P010 / P016` 閻?`CheckFormatSupport` 缂佹挻鐏?  - swap chain 鐎逛粙鐝妴浣圭壐瀵繈鈧攻uffer count閵嗕够wap effect閵嗕工lpha mode閵嗕菇sage

- `MakeWindowAssociation` 閺€閫涜礋閺勬儳绱″Λ鈧弻銉ャ亼鐠愩儱鑻熸潏鎾冲毉鐠€锕€鎲￠敍宀勪缉閸忓秹娼ゆ妯规丢婢惰京鐛ラ崣锝呭彠閼辨棃鏁婄拠顖樷偓?
### 娣囶喗鏁奸弬鍥︽

- src/render/d3d11_video_renderer.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?91: D3D11VA 閼奉亜鐣炬稊?hw_frames_ctx閿涙氨鏁电拠宄板讲闁插洦鐗辩憴锝囩垳鐞涖劑娼伴獮鑸典划婢跺秹娴傞幏鐤閻╂挳鍣伴弽?
**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 閾忕晫鍔ч梻顕€顣?90 瀹歌尙鏁ゆ潻鎰攽閺?fallback 閸忔粈缍囨禍鍡涚拨鐏炲骏绱濇担鍡樼壌閸ョ姳绮涢崷顭掔窗瑜版挸澧?`PlayerCore` 閸欘亞绮︾€规矮绨?`D3D11VA` 鐠佹儳顦敍灞剧梾閺堝鍤滃杈ㄥ复缁?`hw_frames_ctx`閿涘苯顕遍懛?FFmpeg/妞瑰崬濮╂妯款吇閸掑棝鍘ら崙鐑樻降閻ㄥ嫭妲?decoder-only surface閵?
- 鏉╂瑤绱扮拋?D3D11 renderer 閸楀厖濞囬弨顖涘瘮閸樼喓鏁?`AV_PIX_FMT_D3D11`閿涘奔绡冮幏澶哥瑝閸掓澘褰查惄瀛樺复閸掓稑缂?shader resource view 閻ㄥ嫯袙閻浇銆冮棃顫礉闂嗚埖瀚圭拹婵堟纯闁插洦鐗遍弮鐘崇《缁嬪啿鐣鹃幋鎰彌閵?
### 閸樼喎娲滈崚鍡樼€?
- FFmpeg 閻?`AVD3D11VAFramesContext` 閹绘劒绶垫禍?`BindFlags`閿涘苯鍘戠拋姝岀殶閻劍鏌熼崷?`get_format()` 闂冭埖顔岄幐鍥х暰鐟欙絿鐖滅敮褎鐫滈惃鍕睏閻炲棛绮︾€规碍鏌熷蹇ョ幢瑜版挸澧犳い鍦窗濮濄倕澧犲▽鈩冩箒鐠ф媽绻栭弶陇鐭惧鍕剁礉閸欘亣顔曠純顔荤啊 `hw_device_ctx`閵?
- 鐎圭偞绁撮崷銊ョ秼閸撳秵婧€閸ｃ劋绗傞敍灞藉涧鐟曚焦濡?frames context 閻㈠疇顕稉?`D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE`閿涘瓕3D11 閸樼喓鏁撻惄鎾櫚閺嶅嘲宓嗛崣顖涗划婢跺稄绱濈拠瀛樻娑斿澧犻惃鍕壋韫囧啰宸遍崣锝嗘Ц閳ユ粌鎶氬Ч鐘插瀻闁板秶鐡ラ悾銉ょ瑝鐎瑰本鏆ｉ垾婵撶礉娑撳秵妲哥涵顑挎缂佹繂顕稉宥嗘暜閹?D3D11 閻╂挳鍣伴弽鏋偓?
### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`PlayerCore::selectVideoPixelFormat()` 娑擃厽鏁兼稉鐑樻▔瀵繗鐨熼悽?`avcodec_get_hw_frames_parameters()`閿涘苯鍨卞鍝勮嫙閸掓繂顫愰崠鏍殰鐎规矮绠?`hw_frames_ctx`閵?
- 閸?`AVD3D11VAFramesContext::BindFlags` 娑撳﹨鎷烽崝?`D3D11_BIND_SHADER_RESOURCE`閿涘苯鑻熼幎?`extra_hw_frames` 妫板嫮鐣婚崣鐘插閸?`initial_pool_size`閿涘矁顔€鐟欙絿鐖滅敮褎鐫滈弮銏ｅ厴缂?decoder 娴ｈ法鏁ら敍灞肩瘍閼冲€燁潶 D3D11 renderer 閻╁瓨甯撮柌鍥ㄧ壉閵?
- 婵″倹鐏夐懛顏勭暰娑?frames ctx 閸掓繂顫愰崠鏍с亼鐠愩儻绱濋崚娆庣矌閸ョ偤鈧偓閸?decoder-owned D3D11VA surface閿涘奔绗夋稉顓熸焽绾剝袙閿涙稑鎮撻弮鍫曟６妫?90 瀹稿弶婀侀惃鍕箥鐞涘本妞?fallback 缂佈呯敾娴ｆ粈璐熼張鈧崥搴″幑鎼存洏鈧?
### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?90: D3D11 閸樼喓鏁撻惄鎾櫚閺嶇兘绮︾仦蹇ョ窗鏉╂劘顢戦弮鍓侇洣閻?native direct 楠炶泛娲栭柅鈧?copy-back

**閺冦儲婀?*: 2026-03-23

### 闂傤噣顣介幓蹇氬牚

- 娴ｈ法鏁?`Release` 閺嬪嫬缂撶粙瀣碍閹绢厽鏂?`juren-30s.mp4` 閺冭绱濋崙铏瑰箛閳ユ粍婀佹竟浼寸叾閵嗕焦妫ら悽濠氭桨閳ユ縿鈧?
- `--performance-log-check` 瀹稿弶妯夌粈?`renderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗render_frames > 0`閿涘矁顕╅弰搴㈡尡閺€鍙ュ瘜闁炬崘鐭炬禒宥呮躬鏉╂劘顢戦敍宀勭拨鐏炲繘娉︽稉顓炴躬 D3D11 閸樼喓鏁撻惄鎾櫚閺嶉攱妯夌粈娲▉濞堢偣鈧?
### 閸樼喎娲滈崚鍡樼€?
- 閺冄団偓鏄忕帆閸欘亣顩﹀〒鍙夌厠閸ｃ劌锛愰弰搴㈡暜閹?`AV_PIX_FMT_D3D11`閿涘苯姘ㄩ幐浣虹敾閹跺﹦鈥栫憴锝呮姎閹?native direct 鐠侯垰绶為柅浣稿弳閸嶅繒绀岄惈鈧懝鎻掓珤閿涘矂绮拋銈呬海鐠佹崘袙閻浇銆冮棃銏♀偓鏄忓厴閸︺劏绻嶇悰灞炬閹存劕濮涢崚娑樼紦 Y/UV plane 閻?shader resource view閵?
- 鐎圭偞绁磋ぐ鎾冲鐠佹儳顦?妞瑰崬濮╃紒鍕値娑撳绱漙CreateShaderResourceView1` 鐎?`NV12` 鐟欙絿鐖滅悰銊╂桨鏉╂柨娲栨径杈Е閿涘本妫╄箛妞捐礋 `y_plane_hr=-2147024809`閿涘苯顕遍懛鏉戝剼缁辩姷娼冮懝鎻掓珤閹峰じ绗夐崚鏉块挬闂堛垼绁┃鎰剁礉娴?swap chain 娴犲秶鎴风紒?present閿涘奔绨弰顖滄暏閹撮婀呴崚鎵斥偓婊冨涧閺堝锛愰棅绛圭礉濞屸剝婀侀悽濠氭桨閳ユ縿鈧?
- 鏉╂瑦妲告潻鎰攽閺冩儼顔曟径鍥у悑鐎硅鈧囨６妫版﹫绱濇稉宥嗘Ц婵帊缍嬮弬鍥︽閹圭喎娼栭敍灞肩瘍娑撳秵妲?D3D11VA 鐟欙絿鐖滈張顒冮煩婢惰精瑙﹂妴?
### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`D3D11VideoRenderer` 娑擃厽鏌婃晶鐐剁箥鐞涘本妞傞悢鏃€鏌囬敍姘冲 Y/UV plane 閻?`CreateShaderResourceView1` 婢惰精瑙﹂敍灞惧灗鐟欙絿鐖滅悰銊╂桨閺嶇厧绱℃稉宥嗘暜閹镐胶娲块幒銉╁櫚閺嶅嚖绱濋崚娆戠彌閸楀啿鍙ч梻顓炵秼閸撳秳绱扮拠婵堟畱 native direct rendering閵?
- native direct 鐞氼偄鍙ч梻顓炴倵閿涘畭supportsNativeFrameFormat()` 鏉╂柨娲?`false`閿涘畭PlayerCore::prepareVideoOutputFrame()` 娴兼碍濡搁崥搴ｇ敾绾剝袙鐢嗚泲 `av_hwframe_transfer_data()` copy-back 閸掓媽钂嬫禒璺烘姎閿涘苯鍟€婢跺秶鏁ら悳鐗堟箒鏉烆垯娆㈢痪鍦倞娑撳﹣绱剁捄顖氱窞缂佈呯敾閺勫墽銇氶妴?
- 閸氬本妞傜悰銉ュ帠閺勫海鈥橀崨濠咁劅閺冦儱绻旈敍宀冪翻閸戝搫銇戠拹銉ュ斧閸ョ姰鈧胶姹楅悶鍡樼壐瀵繈鈧焦鏆熺紒鍕亣鐏忓繈鈧胶姹楅悶鍡欏偍瀵洏鈧笭RESULT閿涘苯鑻熼弽鍥唶 `fallback=copyback-to-software`閿涘奔绌舵禍搴℃倵缂侇厾鎴风紒顓炰粵妞瑰崬濮╁顔肩磽閹烘帗鐓￠妴?
### 娣囶喗鏁奸弬鍥︽

- src/render/d3d11_video_renderer.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?80: 閺傚洦銆傛稉鈧懛瀛樷偓褑藟姒绘劧绱癈HANGELOG 缁便垹绱╂穱顔碱槻娑撳酣妫舵０?69 analysis 閸ョ偛锝?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 娴犲﹤銇夋稉澶嬵偧閹绘劒姘︾€瑰本鍨氶崥搴礉records / analysis 鐎靛湱鍙庨柌宀冪箷閻ｆ瑤绨℃稉銈咁槱閺傚洦銆傛稉鈧懛瀛樷偓褏宸遍崣锝忕窗



  - `CHANGELOG` 閻ㄥ嫰妫舵０妯烩偓鏄忋€冪紓鍝勭毌 `闂傤噣顣?78`閿涘奔绲惧锝嗘瀮瀹歌尙绮＄€涙ê婀?`闂傤噣顣?78`



  - `闂傤噣顣?69` 瀹告彃鍟撻崗?`CHANGELOG / DEVELOP_LOG / VERSION`閿涘奔绲惧▽鈩冩箒鐎电懓绨查惃?implementation planner analysis 閺傚洦銆?


- 鏉╂瑤琚辨稉顏堟６妫版﹢鍏樻稉宥呭閸濆秳鍞惍浣界箥鐞涘矉绱濇担鍡曠窗閻╁瓨甯磋ぐ鍗炴惙閸氬海鐢婚弬鍥ㄣ€傚Λ鈧槐顫偓渚€妫舵０妯挎嫹闊亜鎷伴弬棰佺窗鐠囨繃甯寸紒顓″窛闁插繈鈧?






### 閸樼喎娲滈崚鍡樼€?


- `CHANGELOG` 閻ㄥ嫰妫舵０妯烩偓鏄忋€冩笟婵婄閹靛浼愮紒瀛樺Б閿涙稑婀崥灞肩婢垛晛顦垮▎陇绻涚紒顓∷夐崘娆撴６妫版顔囪ぐ鏇熸閿涘畭78` 閻ㄥ嫮鍌ㄥ鏇☆攽鐞氼偅绱￠幒澶夌啊閵?


- 娑撳﹤宕?`fix: 閺€璺哄經 PlayerCore 閸嬫粍鎸辩痪璺ㄢ柤娑撳氦绻嶇悰灞炬鐠佹崘顓搁崐绡?闁絾顐奸幓鎰唉閸欘亜鐢禍?records 娑撳娆㈡總妤嬬礉濞屸剝婀侀幎濠傤嚠鎼存梻娈戦崚鍡樼€介弬鍥ㄣ€傛稉鈧挧宄版礀婵夘偁鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐞涖儵缍?`CHANGELOG` 闂傤噣顣介幀鏄忋€冩稉顓犳畱 `闂傤噣顣?78` 缁便垹绱╅敍灞借嫙閻ф槒顔囬張顒侇偧 `闂傤噣顣?80`閵?


- 閺傛澘顤冮崶鐐诧綖閸掑棙鐎介弬鍥ㄣ€?`docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`閿涘本濡?`闂傤噣顣?69` 閻?implementation planner閵嗕胶绮ㄧ拋鍝勬嫲鏉堝湱鏅崡鏇犲濞屽绌╂稉瀣降閵?


- 閸氬本顒為弴瀛樻煀 `闂傤噣顣?69` 閻?records 閺傚洣娆㈡穱顔芥暭濞撳懎宕熼敍灞间簰閸?`VERSION / DEVELOP_LOG` 娑擃厾娈戦弬鍥ㄣ€傛稉鈧懛瀛樷偓褑顔囪ぐ鏇樷偓?






### 娣囶喗鏁奸弬鍥︽



- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?79: PlayerCore 鏉╂劘顢戦幀?software send probe 鐎靛湱鍙庨弨鑸垫殐



**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閸︺劑妫舵０?78 瀹歌尙鈥樼拋?software decode 缁捐法鈻奸懗?dequeue 閸掍即顩婚崠鍛偓浣风稻妫ｆ牔閲?`avcodec_send_packet()` 濞屸剝婀佺€瑰本鍨氭潻鏂挎礀閸氬函绱濋張顒冪枂缂佈呯敾閹?implementation planner 閸嬫埃鈧粎婀＄€圭偠绻嶇悰灞锯偓浣风瑩妞ょ懓顕悡褉鈧繐绱濋惄顔界垼閺勵垱濡?blocker 閸愬秴鐣惧璁崇鐏炲倶鈧?


- 閻劍鍩涢弰搴ｂ€樼憰浣圭湴缂佈呯敾閸ュ绮?software decode blocker 閹恒劏绻橀敍灞肩瑝鐟曚礁鍟€閸忓牆濮?`SoftwareSDL` 濞撳弶鐓嬫笟褋鈧?






### 閸樼喎娲滈崚鍡樼€?


- 閻欘剛鐝?`--software-video-send-probe` 瀹歌尪鐦夐弰?FFmpeg software H.264 decode 閺堫兛缍嬮崣顖滄暏閿涘奔绲剧€瑰啳绻曞▽鈩冩箒鐎瑰苯鍙忕憰鍡欐磰 `PlayerCore` 閻喎鐤勬潻鎰攽閹胶娈戦崗銊╁劥瀹割喖绱撻妴?


- 閸ョ姵顒濋棁鈧憰浣烘埛缂侇參鈧劙銆嶉幒鎺楁珟閿涙瓪receive->send` 妞ゅ搫绨妴涔竌cket queue 娴溿倖甯撮妴涔╡mux read-ahead閵嗕線鐓舵０鎴︽懠閵嗕礁缍嬮崜?video decode 缁捐法鈻兼稉濠佺瑓閺傚洢鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫?`--software-video-send-probe`閿涘矁藟閸忓拑绱?


  - `pre_send_receive_ret`



  - `packet_queue_push_ok / packet_queue_pop_ok`



  - `read_ahead_packets / read_ahead_done`



- 閹碘晛鐫?`--software-video-decode-check`閿?


  - 閺傛澘顤?`audio_probe_mode`



  - 閺€顖涘瘮 `MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO=1` 閻?video-only 鐎靛湱鍙?


- 閸?`PlayerCore::decodeVideoFrame()` 閺傛澘顤冩禒鍛箚婢у啫褰夐柌蹇撶磻閸氼垳娈?`MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD` 鐠囧﹥鏌囩捄顖氱窞閿涘瞼鏁ゆ禍搴ｂ€樼拋?software `send_packet` 閺勵垰鎯侀崣顏冪窗閸椻€虫躬瑜版挸澧?decode 缁捐法鈻奸妴?


- 閺堫剝鐤嗛崗鎶芥暛缂佹捁顔戦敍?


  - 閻欘剛鐝?probe 閸?`pre-receive + packet queue round-trip + read-ahead=512` 閸氬簼绮?`send_ret=0 / result=PASS`



  - video-only software decode 娴?`video_packet_dequeue_count=1 / video_send_packet_ok=0 / result=FAIL`



  - `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD=1` 娑撳绮涢崙铏瑰箛 `Offthread software video send_packet probe timed out after 500ms`



- 缂佹捁顔戦崶鐘愁劃缂佈呯敾閺€鑸垫殐娑撶尨绱癰locker 娑撳秴婀?FFmpeg software decoder 閺堫兛缍嬮妴涔竌cket queue閵嗕龚emux read-ahead閵嗕線鐓舵０鎴︽懠閹存牕缍嬮崜?video decode 缁捐法鈻奸張顒冮煩閿涘矁鈧苯婀?`PlayerCore` 鏉╂劘顢戦幀渚€鍣烽惃?software codec context / surrounding state 瀹割喖绱撻妴?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY15_PLAYERRUNTIME_SOFTWARE_SEND_PROBE.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?78: software decode 閺堚偓鐏?send/dequeue 鐠佲剝鏆熼幒銉ュ弳娑撳酣顩婚崠鍛粹偓浣稿瘶閸嬫粍绮搁柦澶嬵劥



**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閻劍鍩涢弰搴ｂ€樼憰浣圭湴娑撳秷顩﹂崘宥呭帥閸?`SoftwareSDL` 濞撳弶鐓嬫笟褝绱濋懓灞炬Ц閻╁瓨甯村▽?software decode 妫ｆ牕瀵橀崑婊勭哺閺傜懓鎮滅悰銉︽付鐏忓繗顓搁弫鑸偓?


- 瑜版挸澧犲鑼叀 software path 娴兼艾鍤悳?`decode_video_ok=0 / render_frames=0`閿涘奔绲炬禒鍛存浆閺冄勬）韫囨绻曟稉宥堝厴閹跺﹪妫舵０妯诲閹存劏鈧粍鐥?dequeue 閸掓澘瀵橀垾婵婄箷閺勵垪鈧竴send_packet` 閺堫剝闊╅崡鈥茬秶閳ユ縿鈧?






### 閸樼喎娲滈崚鍡樼€?


- 閺冄嗙槚閺傤參鍣烽崣顏呮箒 `decode_video_ok / decode_video_send_eagain / video_decoder_drain_signals`閿涘瞼宸辩亸?packet dequeue 娑?`send_packet` 閹存劕濮涙潻鏂挎礀鐏炲倿娼伴惃鍕付鐏忓繗顫囧ù瀣偓绗衡偓?


- 閸ョ姵顒濋崡鍏呭▏瀹歌尙绮￠幀鈧悿鎴︻浕閸栧懘鈧礁鍙?decoder 闂冭埖顔岄張澶愭６妫版﹫绱濇稊鐔告￥濞夋洜鏁ょ紒鎾寸€崠鏍ㄦ殶鐎涙娲块幒銉ㄧ槈閺勫簺鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`PlayerCore` 鐞涖儰绗佹い瑙勬付鐏忓繗顓搁弫甯窗



  - `video_packet_dequeue_count`



  - `video_send_packet_ok`



  - `video_send_packet_last_ret`



- 鐏忓棗鐣犳禒顒勨偓蹇庣炊閸掑府绱?


  - `DiagnosticsSnapshot`



  - 娴ｅ酣顣堕柧鎹愮熅鐠囧﹥鏌囬弮銉ョ箶



  - `--performance-log-check`



  - `--software-video-decode-check`



- 閺堫剝鐤嗘径宥堢獓 software decode 閺嶉攱婀伴崥搴礉閸忔娊鏁拠濠冩焽瀹稿弶鏁归弫娑樺煂閿?


  - `v_pkt_deq=1`



  - `v_send_ok=0`



  - `v_send_ret=-2147483648`



  - `decode_video_ok=0`



  - `render_frames=0`



- 缂佹捁顔戦崶鐘愁劃鏉╂稐绔村銉︽暪缁毖傝礋閿涙oftware decode 缁捐法鈻煎鑼病閸欐牕鍩屾＃鏍﹂嚋鐟欏棝顣堕崠鍜冪礉娴ｅ棝顩绘稉?`avcodec_send_packet()` 濞屸剝婀佽ぐ銏″灇閹存劕濮涙潻鏂挎礀閵?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY14_VIDEO_SEND_PACKET_MIN_COUNTERS.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?77: Software decode 妫ｆ牕瀵橀崑婊勭哺婢跺秵鐗虫稉?SDL renderer 濞夈劑鍣存稊杈╃垳娣囶喖顦?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閸?Day12 瀹歌尙绮￠幎?`SoftwareSDL + Software decode` 閻ㄥ嫧鈧? 鐢嗙翻閸戣　鈧繂宕熼悪顒勬嫟濮濊鎮楅敍灞炬拱鏉烆喚鎴风紒顓熷瘻 implementation planner 婢跺秷绐囬獮鍫曠崣鐠囦緤绱版穱婵嗙暓 software decode 缁捐法鈻奸柊宥囩枂閺勵垰鎯侀懗鍊熜掗梽?blocker閵?


- 閸氬本妞傞敍灞煎敩閻焦鏋冩禒鍫曞櫡鏉╂ɑ鐣悾娆庣婢跺嫮婀＄€圭偞鏁為柌濠佽础閻緤绱癭src/render/sdl_video_renderer.cpp` 閻ㄥ嫬鍤遍弫鏉裤仈濞夈劑鍣寸€涙ê婀弰搴㈡▔ mojibake閿涘苯濂栭崫宥呮倵缂侇參妲勭拠璇叉嫲 diff 鐎光剝鐗抽妴?






### 閸樼喎娲滈崚鍡樼€?


- 閺堫剝鐤嗘径宥堢獓閸氬函绱漙Video decoder threading: backend=Software thread_count=1 thread_type=none` 瀹歌尪鐦夐弰搴濈箽鐎瑰牏鍤庣粙瀣帳缂冾喖鍑＄紒蹇撶杽闂勫懐鏁撻弫鍫幢娴?`decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0` 娴犲秶鍔ч崗銊╁劥娑?0閿涘矁顕╅弰?blocker 娑撳秵妲搁垾婊勭负鏉╂稓鍤庣粙瀣帳缂冾喖顕遍懛瀵告畱閸嬭泛褰傛禍銈勭鞍闂傤噣顣介垾婵勨偓?


- 缂佹挸鎮庢潻鎰攽閺堢喕鐦栭弬顓熸）韫?`demux(v=163) / pkt_q(v=162)` 娑撳簼绮庨崙铏瑰箛 `Video decode first send_packet start` 閻ㄥ嫪绨ㄧ€圭儑绱濋崣顖欎簰閹恒劍鏌?software decode 缁捐法鈻奸弴鏉戝剼閺勵垰婀＃鏍﹂嚋鐟欏棝顣堕崠鍛絹娴溿倝妯佸▓闈涙倵閸嬫粈缍囬妴?


- `src/render/sdl_video_renderer.cpp` 閻ㄥ嫪璐￠惍浣稿灟閺勵垰宕熺痪顖滅椽閻線浠愰悾娆撴６妫版﹫绱濇稉宥呭閸濆秹鈧槒绶敍灞肩稻娴兼碍瀵旂紒顓熻杽閺屾挷鍞惍渚€妲勭拠璇叉嫲鐎癸繝妲勭紒鎾寸亯閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 娣囨繃瀵旇ぐ鎾冲 software decode 娣囨繂鐣х痪璺ㄢ柤闁板秶鐤嗘稉宥呮礀闁偓閿涘苯鑻熼柌宥嗘煀閹笛嗩攽 `--software-video-decode-check`閿涘本濡?blocker 缂佹捁顔戞禒搴樷偓? 鐢嗙翻閸戣　鈧繆绻樻稉鈧銉︽暪閺佹稑鍩岄垾婊堫浕娑擃亣顫嬫０鎴濆瘶閸氬骸浠犳担蹇娾偓婵勨偓?


- 娣囶喖顦?`src/render/sdl_video_renderer.cpp` 閻?9 婢跺嫬鍤遍弫鏉裤仈濞夈劑鍣存稊杈╃垳閿涘奔绮庨弨閫涜厬閺傚洦鏁為柌濠忕礉娑撳秵鏁奸柅鏄忕帆閵?


- 閸愬秵顐奸幍顐ｅ伎 `src/`閵嗕梗include/` 閻?`///` 娑?`//` 濞夈劑鍣寸悰宀嬬礉绾喛顓婚張顒冪枂閺堫亜鍟€閸涙垝鑵戦弬鎵畱閸欘垳鏋掓稊杈╃垳濞夈劑鍣撮妴?






### 娣囶喗鏁奸弬鍥︽



- src/render/sdl_video_renderer.cpp



- docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?76: Software video decode 閻喎鐤勬禍褍鎶氭稉鎾汇€嶅Λ鈧弻銉ょ瑢 blocker 鐎规矮缍?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 娑撳﹣绔存潪顔煎嚒缂佸繒鈥樼拋銈傗偓婊呮埛缂侇厼鍣虹亸鎴炲灗鐟欏嫰浼?`av_hwframe_transfer_data()` copy-back閳ユ繄娈戞稉瀣╃濮濄儻绱濇稉宥堫嚉閻╁瓨甯撮幎?fallback 姒涙顓婚崚鍥у煂 `software-first`閿涘矁鈧苯绨茬拠銉ュ帥閹跺﹤缍嬮崜宥呬紣缁嬪娈?software video decode blocker 鐎规矮缍呭〒鍛殶閵?


- 閺冄呮畱 `--windows-backend-session-check soft` 閸欘亣鍏樼拠浣规閳ユ粏鍏橀幍鎾崇磻楠炴儼绻橀崗銉︽尡閺€鎯ф儕閻滎垪鈧繐绱濇稉宥堝厴鐠囦焦妲戦垾婊嗚拫娴犳儼顫嬫０鎴Ｐ掗惍浣烘埂閻ㄥ嫪楠囬崙鍝勮嫙濞撳弶鐓嬬憴鍡涱暥鐢€鈧繐绱遍崥灞炬娑撯偓閺?soft decode 閸椻剝顒撮敍宀€娲块幒?`stop/close` 鏉╂ê褰查懗鑺ュΩ娑撴捇銆嶉崨鎴掓姢閺堫剝闊╅幏鏍ㄥ瘯閵?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀侀崶鐐茬秺閸涙垝鎶ょ紓鍝勭毌鐎靛厜鈧粎婀＄€圭偘楠囩敮褉鈧繄娈戠涵顒勬，濡叉冻绱濋弮鐘崇《閸栧搫鍨庨垾娓乽dio clock 閸︺劍甯规潻娑掆偓婵呯瑢閳ユ抚ideo frame 閻喎鍤弶銉ょ啊閳ユ縿鈧?


- 瑜版挸澧?blocker 娑撳绱漙SoftwareSDL + Software decode` 娑撳秳绮庢导姘炽€冮悳棰佽礋 `decode_video_ok=0 / render_frames=0 / video_frame_queue_peak_size=0`閿涘矁绻曟导姘愁唨鐢瓕顫夐弨璺虹啲鐠侯垰绶為崣妯虹繁娑撳秴褰查棃鐙呯幢閸ョ姵顒濇稉鎾汇€嶅Λ鈧弻銉ユ嚒娴犮倓绡冭箛鍛淬€忛崓?probe 娑撯偓閺嶇柉鍤滅敮锔锯€栭柅鈧崙楦垮厴閸旀稏鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`--software-video-decode-check <media_file> [sample_ms]`閵?


- 閸涙垝鎶ら崘鍛村劥瀵搫鍩楅敍?


  - `MVP_RENDERER_BACKEND=software`



  - `SDL_AUDIODRIVER=dummy`



  - `preferHardwareDecode=false`



- 闁俺绻冮弶鈥叉閺€鍓佹彛娑撹　鈧粎婀＄€圭偘楠囩敮褉鈧繆鈧奔绗夐弰顖椻偓婊冨涧閹垫挸绱戦幋鎰閳ユ繐绱?


  - `renderer_backend=SoftwareSDL`



  - `decoder_backend=Software`



  - `decode_video_ok > 0`



  - `scheduler_video_decoded_frames > 0`



  - `render_frames > 0`



  - `video_frame_queue_peak_size > 0`



  - `video_copy_back_frames == 0`



- 閸涙垝鎶ら弨瑙勫灇 probe 瀵繒鈥栭柅鈧崙鐚寸礉闁灝鍘ら崷?soft decode blocker 娑撳顫?`stop/close` 閸椻€茬秶閵?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY12_SOFTWARE_VIDEO_DECODE_REAL_FRAME_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?75: 閹俱倕娲?SoftwareSDL automatic software-first 楠炴儼藟鏉烆垵袙闂冭顢ｇ拠濠冩焽



**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 瑜版挸澧犻惄顔界垼閸樼喐婀伴幆宕囨埛缂侇厽閮ㄩ惈鈧垾婊冨櫤鐏忔垶鍨ㄧ憴鍕缉 `av_hwframe_transfer_data()` copy-back閳ユ繃甯规潻娑崇礉娴滃孩妲告稉瀛樻鐏忔繆鐦幎?`SoftwareSDL` fallback 閺€瑙勫灇 renderer-aware `software-first`閵?


- 娴ｅ棗鐤勯梽鍛寸崣鐠囦焦妯夌粈鐚寸礉娑撯偓閺?`SoftwareSDL + Software decode` 閼奉亜濮╅崥顖滄暏閿涘本鎸遍弨鎯ф珤娴兼艾鍤悳?`decode_video_ok=0 / render_frames=0`閿涘畭--performance-log-check` 閺冪姵纭跺锝呯埗閺€璺哄經閵?






### 閸樼喎娲滈崚鍡樼€?


- 娴犲孩鎸遍弨鎯ф珤鐠佹崘顓搁悶鍡楀悍娑撳﹦婀呴敍瀹粂stem-memory renderer 娴兼ê鍘涢柆鍨帳 copy-back 閻ㄥ嫭鏌熼崥鎴炴拱闊偅妲搁幋鎰彌閻ㄥ嫸绱濇稊鐔侯儊閸氬牊鍨氶悢鐔告尡閺€鎯ф珤鐢瓕顫嗛幀婵婄熅閵?


- 娴ｅ棗缍嬮崜宥呬紣缁嬪娈?FFmpeg 鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖滈幒銉ュ弳鐠侯垰绶為張顒冮煩鐎涙ê婀梼璇差敚閿涙艾宸遍崚?`D3D11 + Software decode` 閺冩湹绡冮懗钘夘槻閻滄媽钂嬬憴锝夋懠娑撳秴鑸伴幋鎰箒閺佸牐顫嬫０鎴滈獓閸戠尨绱濋崶鐘愁劃闂傤噣顣芥稉宥呮躬 `SoftwareSDL` renderer閿涘矁鈧苯婀潪顖欐鐟欏棝顣剁憴锝囩垳閹恒儱鍙嗛妴?


- 閸︺劏绻栨稉顏勫閹绘劒绗呴敍宀€鎴风紒顓㈢帛鐠併倕鎯庨悽?`software-first` 閸欘亙绱伴幎濠傜秼閸撳秶菙鐎规氨娈?fallback 闁炬儳鐢崗銉ユ礀瑜版帇鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹俱倕娲栭懛顏勫З renderer-aware `software-first` decoder 妞ゅ搫绨拫鍐╂殻閿涘本浠径宥夌帛鐠?`D3D11VA -> Software`閵?


- 娣囨繄鏆€娑撳﹣绔存潪顔煎嚒缂佸繘鐛欑拠渚€鈧俺绻冮惃?`NV12` 閻╃繝绱堕妴涔VFrame` 瀵洜鏁ゆ径宥囨暏閸?`SoftwareSDL` 闂?`swscale` / 闂?`display_copy` 閺€褰掆偓鐘偓?


- 娑撳搫鎮楃紒顓炲礋閻欘兛鎱ㄦ潪顖欐鐟欏棝顣剁憴锝囩垳閹恒儱鍙嗙悰銉ュ帠娴ｅ酣顣剁拠濠冩焽閿?


  - FFmpeg 闁挎瑨顕ら惍浣哥摟缁楋缚瑕?


  - 妫ｆ牗顐?`send_packet` 閹恒垽鎷?


  - stall 娑撳﹣绗呴弬鍥ㄦ）韫?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY11_SOFTWARE_DECODE_BLOCKER_AND_FALLBACK_DIRECTION.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?74: Audio-master lateness 閺€鍓佹彛娑?SoftwareSDL 閸戝繑瀚圭拹婵囨箒闂勬劙鍣搁弸?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閻劍鍩涘鍙夊閸斻劍褰佹禍銈勭瑐娑撯偓鏉烆喚绮ㄩ弸婊冩倵閿涘矁顩﹀Ч鍌欏瘜闁惧墽鎴风紒顓熷ⅵ绾?`audio-master lateness / catch-up`閿涘苯鎮撻弮璺烘躬鏉烆垯娆㈤崶鐐衡偓鈧柧鍙ョ瑐閸嬫埃鈧粌鍣虹亸?copy-back + swscale + 濞ｈ鲸瀚圭拹婵囶偧閺佹壋鈧繄娈戦張澶愭闁插秵鐎妴?


- 閺冄呮畱 `Scheduler::pumpRenderOnce()` 閸?`Audio` master 娑撳绮涢悞鑸垫Ц閳ユ粍娓舵径姘辨蒋 5ms閿涘瞼鍔ч崥搴ｆ纯閹?render閳ユ繐绱濈€硅妲楁潻鍥ㄦ－閸戝搫鎶氶妴?


- `SoftwareSDL` 閸ョ偤鈧偓闁炬崘娅ч悞璺哄嚒缂佸繗鍏橀柌蹇撳閻戭厾鍋ｉ敍灞肩稻鐠侯垰绶炴禒宥嗘Ц `copy-back + swscale + display memcpy` 娑撳顔岄崣鐘插閿涘矁鎯ら崚?4K60 閺冭埖鍨氶張顒冪箖妤傛ǜ鈧?






### 閸樼喎娲滈崚鍡樼€?


- `Audio` master 濮濓絽鎮?diff 閸欘亞娼稉鈧▎鈥茬瑬娑撳秹鍣哥拠璁冲瘜閺冨爼鎸撻敍灞肩窗鐠?renderer 閸︺劑鐓舵０鎴炴闁界喎鐨婚張顏囨嫹娑撳﹥妞傞幓鎰閹绘劒姘︾憴鍡涱暥鐢佲偓?


- `-250ms` 閸ュ搫鐣?late-drop 闂冨牆鈧壈绻冪划妤嬬礉鐎?24fps/60fps 娴犮儱寮锋稉宥呮倱闂冪喎鍨繅顐㈠帠鎼达箓鍏樻稉宥咁檮閸氬牏鎮婇妴?


- `SoftwareSDL` 娑斿澧犻崣顏囧厴閸?`IYUV` 濞ｈ鲸瀚圭拹婵嗘姎閿涘苯顕遍懛?copy-back 娑斿鎮楁潻妯款洣妫版繂顦?`swscale`閵嗕梗Display::copyFrameData()` 濞ｈ鲸瀚圭拹婵撶礉閸愬秳姘︾紒?SDL 娑撳﹣绱堕妴?






### 鐟欙絽鍠呴弬瑙勵攳



- `IVideoRenderer` 閺傛澘顤?`supportsDirectFrameFormat()`閿涘畭SdlVideoRenderer` 婢圭増妲戦弨顖涘瘮 `YUV420P/NV12`閵?


- `PlayerCore::prepareVideoOutputFrame()` 閸︺劍妫ょ憴鍡涱暥濠娿倝鏆呴弮璺哄帒鐠?copy-back 閸氬海娈戞潪顖欐鐢呮纯閹恒儰姘︾紒?`SoftwareSDL`閿涘奔绗夐崘宥呭繁閸?`swscale -> YUV420P`閵?


- `Display` 閺€瑙勫灇閳ユ粈绱崗鍫滅箽閻?`AVFrame` 瀵洜鏁ら敍宀冪 stride/娑撳秹鈧倿鍘ら弮鑸靛濞ｈ鲸瀚圭拹婵冣偓婵撶礉楠炴儼藟 `SDL_UpdateNVTexture()` 閺€顖涘瘮 `NV12` 閻╃繝绱堕妴?


- `Scheduler::pumpRenderOnce()` 閻?`Audio` master 闁槒绶弨瑙勫灇閸掑棙顔岀粵澶婄窡楠炲爼鍣哥拠?clock閿涘苯鎮撻弮鑸靛Ω late-drop 闂冨牆鈧吋鏁奸幋鎰唨娴?`frame.duration + queue fill ratio` 閻ㄥ嫬濮╅幀浣虹崶閸欙綇绱濋獮鎯八夐張鈧亸?sleep 闁插繐鐡欓柆鍨帳娴碱亜绻栫粵澶堚偓?


- 瀹告煡鍣搁弬鐗堝⒔鐞涘矉绱版妯款吇 `D3D11 --performance-log-check`閵嗕礁宸遍崚?`SoftwareSDL --performance-log-check`閵嗕梗SDL_AUDIODRIVER=dummy` 娑撳娈?`Audio` master `--performance-log-check`閵?






### 娣囶喗鏁奸弬鍥︽



- `include/render/video_renderer.h`



- `include/render/sdl_video_renderer.h`



- `src/render/sdl_video_renderer.cpp`



- `include/display.h`



- `src/display.cpp`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `docs/analysis/PLAYERCORE_DAY10_AUDIOMASTER_AND_SOFTWARESDL_REFACTOR.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂傤噣顣?73: SoftwareSDL 閹风柉绀夐柧鎹愮熅闁插繐瀵查妴涓糲heduler 闁插秴鎯庢０鍕暬娑?renderer override



**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閸︺劋绗傛稉鈧潪顔锯€樼拋?D3D11 娑撳鎽兼禒宥嗘Ц zero-copy 娑斿鎮楅敍宀€鏁ら幋椋庢埛缂侇叀顩﹀Ч鍌氬灲閺?`Display::copyFrameData()` 娑?`Scheduler` 闁插秴鎯庣粵鏍殣閺勵垰鎯侀弰顖炵彯閻胶宸?4K 娑撳秶菙鐎规氨娈戦惇鐔风杽閸樼喎娲滈妴?


- 閺冄冪杽閻滅増妫ゅ▔鏇㈠櫤閸?`SoftwareSDL` 鐠侯垰绶炲В蹇撴姎 memcpy 閻ㄥ嫬鐤勯梽鍛灇閺堫剨绱漙Scheduler` 娑旂喍绮涢悞鏈靛▏閻劌娴愮€规碍顐奸弫浼村櫢閸氼垳鐡ラ悾銉礉娑?renderer 闁瀚ㄩ柧鐐梾閺堝婀″锝嗘暜閹镐礁宸遍崚?`SoftwareSDL` 闁插洦鐗遍妴?


- 鏉╂瑥顕遍懛瀵割儑 8閵?0 閻愰€涚矝閻掕泛褰ч懗钘夊磹鐎规碍鈧冨灲閺傤叏绱濋弮鐘崇《閸嶅繋瀵岄柧?zero-copy 闁絾鐗辩紒娆忓毉绾剚鏆熼幑顔衡偓?






### 閸樼喎娲滈崚鍡樼€?


- `Display::copyFrameData()` 閻ㄥ嫮绮虹拋鈩冪梾閺堝鈧繋绱堕崚?`PlayerCore` 閸?CLI閿涘矁鍤滈悞鏈电瘍鐏忚鲸妫ゅ▔鏇犵暬閸?4K60 娑撳娈戦崡鐘崇槷閵?


- `Scheduler` 閸ュ搫鐣鹃柌宥呮儙濞嗏剝鏆熸潻鍥︾艾閻㈢喓鈥栭敍宀€鐓弮璺虹磽鐢鎷伴幐浣虹敾閹舵牕濮╅弮鐘崇《閸栧搫鍨庨妴?


- `RendererFactory` 娑斿澧犵€瑰苯鍙忛弮鐘侯潒 `MVP_D3D11_DRIVER_HINT` / renderer override閿涘苯顕遍懛?`--renderer-fallback-check` 閸?`SoftwareSDL` 閹嗗厴闁插洦鐗遍柈鎴掔瑝閸欘垶娼妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑?`Display` 婢х偛濮?`FrameCopyStats`閿涘苯鑻熺紒蹇曟暠 `SdlVideoRenderer -> RendererDiagnostics -> PlayerCore::DiagnosticsSnapshot` 闁繋绱堕崚?`--performance-log-check`閵?


- `Scheduler` 闁插秴鎯庣粵鏍殣閺€瑙勫灇閳?0s 缁愭褰涢崘鍛付婢?4 濞?+ 100ms 閸愬嘲宓堥垾婵撶礉楠炶埖鏌婃晶?`scheduler_*_restart_limit_hits`閵?


- `RendererFactory::detectBestRendererType()` 閺傛澘顤?`MVP_RENDERER_BACKEND` override閿涘苯鑻熼崷?Windows 娑撳鏁幐?`MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`閵?


- 瀹告煡鍣搁弬鐗堝⒔鐞涘矉绱癭MSBuild`閵嗕梗--renderer-fallback-check`閵嗕線绮拋?`D3D11 --performance-log-check`閵嗕礁宸遍崚?`SoftwareSDL --performance-log-check`閵?


- 閸忔娊鏁紒鎾寸亯閿涙瓪SoftwareSDL` 4K60 闁插洦鐗遍柌?`display_copy_ratio_percent=21.8407`閵嗕梗video_copy_back_ratio_percent=30.1018`閵嗕梗video_swscale_ratio_percent=18.6623`閿涙盯绮拋?`D3D11` 娑撳鎽奸崚娆庣瑏閼板懘鍏樻稉?`0`閵?






### 娣囶喗鏁奸弬鍥︽



- `include/display.h`



- `src/display.cpp`



- `include/render/video_renderer.h`



- `include/render/sdl_video_renderer.h`



- `src/render/sdl_video_renderer.cpp`



- `src/render/renderer_factory.cpp`



- `include/core/scheduler.h`



- `src/core/scheduler.cpp`



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY9_SOFTWARE_COPY_AND_RESTART_BUDGET_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







## 闂傤噣顣?72: 妤傛鐖滈悳?4K 闂冪喎鍨€瑰綊鍣洪妴浣藉殰闁倸绨查懞鍌涚ウ娑?copy-back 鐠囧﹥鏌囨晶鐐插繁



**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閻劍鍩涚憰浣圭湴缂佈呯敾閸ュ绮妯肩垳閻滃洩顫嬫０鎴滅瑝缁嬪啿鐣炬潏鎾冲毉鐢呮畱闂傤噣顣介敍宀勫櫢閻愮懓鍨介弬?`FrameQueue` 鐎瑰綊鍣洪妴涔cheduler` 閼冲苯甯?閼哄倹绁﹂妴涔isplay::copyFrameData()`閵嗕梗av_hwframe_transfer_data()` 娑?`Scheduler` 缁捐法鈻肩粵鏍殣閺勵垰鎯侀弰顖滄埂濮濓絿鎽辨０鍫涒偓?


- 閻滅増婀佹稉濠氭懠瀹歌尙绮￠崗宄邦槵 D3D11 native zero-copy閿涘奔绲剧紓鍝勭毌 queue 瀹勬澘鈧鈧浇鍎楅崢瀣搼瀵板懏妞傞梹瑁も偓涔py-back/swscale 閺冨爼妫跨紒鐔活吀閿涘苯顕遍懛鎾Е閸掓妲搁崥锕佺箖鐏忓繈鈧恭opy-back 閺勵垰鎯侀悜顓犲仯闁棄褰ч懗浠嬫浆閻氭嚎鈧?


- 閸氬本妞傞敍瀹峍ideo` master 鐠侯垰绶為惃鍕搼瀵板懐鐡ラ悾銉ㄧ箖娴滃海鐭栫化娆欑礉render loop 閽€钘夋倵閺冩湹绔村▎鈥冲涧娑擃澀绔寸敮褝绱濇稊鐔剁窗閺€鎯с亣妤傛ê鍨庢潏銊у芳閺嶉攱婀版稉瀣畱閺冭泛绨幎鏍уЗ閵?






### 閸樼喎娲滈崚鍡樼€?


- `FrameQueue` 娑斿澧犻崣顏呮箒瑜版挸澧?size/capacity閿涘本鐥呴張?peak 娑?push timeout閿涘本妫ゅ▔鏇犫€樼拋銈嗘Ц閸氾妇婀￠惃鍕潶 frame queue 妞よ埖寮ч妴?


- `Scheduler` 閻ㄥ嫬宕熼悙纭呭剹閸樺妲囬崐鐓庢嫲閸ュ搫鐣剧亸蹇旑劄缁涘绶熸导姘躬妤傛鐖滈悳?4K 娑撳鑸伴幋鎰ㄢ偓婊嗩洣娑斿牐绻冮弮鈺呮濞翠降鈧浇顩︽稊鍫ｆ嫹鐢冦亰閹扁懇鈧繄娈戞稉銈囶潚閺嬩胶顏妴?


- 閻╁瓨甯撮弨鎯с亣 4K `D3D11VA` 鐟欏棝顣堕梼鐔峰灙閸欏牅绱伴幍鎾冲煂 FFmpeg 閻ㄥ嫬娴愮€规氨鈥栨禒璺烘姎濮圭媴绱濋崶鐘愁劃 frame queue 閸?`extra_hw_frames` 韫囧懘銆忛懕鏂垮З閵?


- 闁插洦鐗辨宀冪槈鐞涖劍妲戣ぐ鎾冲 4K 娑撳鎽奸崨鎴掕厬閻ㄥ嫪绮涢悞鑸垫Ц `D3D11VA -> D3D11` native path閿涘矁鈧奔绗夐弰?copy-back / swscale閿涙稑娲滃?`av_hwframe_transfer_data()` 楠炴湹绗夐弰顖氱秼閸撳秷绻栭崣鐗堟簚閸ｃ劋绗傞惃鍕瘜閻″爼顣妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑?`FrameQueue` 婢х偛濮?`peak_size / push_timeout_count / getStats / resetStats`閵?


- `PlayerCore::open()` 閻滄澘婀导姘瘻婵帊缍嬬仦鐐粹偓褔鍘ょ純顔款潒妫?闂婃娊顣?frame queue 鐎瑰綊鍣洪敍灞借嫙閸?`D3D11VA` 閹垫挸绱戦崜宥堫啎缂?`extra_hw_frames`閿涘矂浼╅崗?4K native path 閹垫挾鍨?surface pool閵?


- `Scheduler` 閼冲苯甯囬弨瑙勫灇 enter/exit hysteresis閿涘苯鑻熼弬鏉款杻 `video/audio_backpressure_wait_ms` 缂佺喕顓搁妴?


- `Scheduler::pumpRenderOnce()` 閻滄澘婀导姘剧窗



  - 閸?`Video` master 娑撳瀵滈惇鐔风杽 wall-clock 閸?frame pacing



  - 閸︺劋绔村▎?pump 閸愬懓绻涚紒顓濇丢瀵啳绻冮張鐔锋姎閿涘瞼娲块崚鐗堝瑏閸掓澘褰查弰鍓с仛鐢?


- `--performance-log-check` 閹碘晛鐫嶆潏鎾冲毉 copy-back/swscale 閺冨爼妫块妴涔箄eue capacity/peak/timeout 娑撳氦鍎楅崢瀣搼瀵板懏妞傞梹瑁も偓?


- 瀹告煡鍣搁弬鐗堝⒔鐞涘矉绱?


  - `--performance-log-check ...4k... 1500`



  - `--high-bitrate-check ... 3000`



  - `--4k-playback-check ... 2000`



  - `--long-playback-check .\juren-30s.mp4 6000`



  瑜版挸澧犻崸鍥偓姘崇箖閵?






### 娣囶喗鏁奸弬鍥︽



- `include/core/frame_queue.h`



- `include/core/player_core.h`



- `include/core/scheduler.h`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY8_QUEUE_PACING_AND_COPYBACK_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂傤噣顣?71: 4K backend session 鐎涙劘绻樼粙瀣偓鈧崙楦跨熅瀵板嫪鎱ㄦ径?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閸︺劋绗傛稉鈧潪顔绘叏婢?video-only 闂勫秶楠囬崪?demux 闂傘劎顩﹂崥搴礉`--4k-playback-check` 娴犲秶鍔ф径杈Е閿涘奔绲炬径杈Е閻愮懓鍑￠弨鑸垫殐閸?`fallback_ok=false`閵?


- 鏉╂稐绔村銉ヮ槻鐠烘垵褰傞悳甯窗`--windows-backend-session-check hard` 娴兼艾婀幍鎾冲祪 `PASS` 閸氬骸宕遍崚鎵煑鏉╂稓鈻肩搾鍛閿涘畭soft` 娴兼艾婀幍鎾冲祪 `PASS` 閸氬簼浜掑鍌氱埗闁偓閸戣櫣鐖滅紒鎾存将閵?


- 鏉╂瑤濞囧?4K 閸ョ偛缍婃禒宥囧姧鐞?backend probe 鐎涙劘绻樼粙瀣嚖閸掋倖瀚嬮幋?FAIL閵?






### 閸樼喎娲滈崚鍡樼€?


- `runWindowsBackendSessionCheck()` 閺堫剝宸濋弰顖欑瑩娓氭稓鍩楁潻娑氣柤濞戝牐鍨傞惃?probe 鐎涙劘绻樼粙瀣剁礉娑撳秵妲搁悽銊﹀煕閹線鏆遍張鐔荤箥鐞涘本鎸遍弨鎯ф珤娴兼俺鐦介妴?


- 閺冄冪杽閻滀即鏁婄拠顖氭勾婢跺秶鏁ゆ禍鍡楃埗鐟欏嫰鈧偓閸戝搫浜ｇ拋鎾呯礉鐎佃壈鍤ф潻娆愭蒋 probe 鐠侯垰绶為崷銊ょ瑝閸?backend 缂佸嫬鎮庢稉瀣毉閻滄壋鈧粎绮ㄩ弸婊冨嚒閹垫挸宓冮妴浣界箻缁嬪婀锝呯埗缂佹挻娼垾婵堟畱闂傤噣顣介妴?


- 閸ョ姳璐?`runBackendSessionSubprocess()` 閸氬本妞傜憰浣圭湴 `mode_ok=true` 娑?`exit_code==0`閿涘本澧嶆禒銉ㄧ箹缁夊秷绉撮弮?瀵倸鐖堕柅鈧崙杞扮窗閻╁瓨甯撮幎?`fallback_ok` 閹峰鍨?false閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐏?`runWindowsBackendSessionCheck` 閺€閫涜礋娑撴挾鏁ら惃?`runWindowsBackendSessionCheckAndExit()`閵?


- 鐠囥儱鎳℃禒銈呮躬閹垫挸宓冪紒鎾寸€崠鏍波閺嬫粌鎮楁导姘▔瀵?flush `stdout/stderr`閿涘苯鑻熼崷?Windows 娑撳娲块幒銉ㄧ殶閻?`TerminateProcess(GetCurrentProcess(), code)` 闁偓閸戝搫鐡欐潻娑氣柤閵?


- `main()` 閻?`--windows-backend-session-check` 閸掑棙鏁鎻掑瀼閸掓媽绻栭弶鈥茬瑩閻劑鈧偓閸戦缚鐭惧鍕┾偓?


- 瀹告煡鍣搁弬鐗堝⒔鐞涘矉绱?


  - `--windows-backend-session-check ... hard`



  - `--windows-backend-session-check ... soft`



  - `--windows-backend-check ...`



  - `--4k-playback-check ... 2000`



  瑜版挸澧犻崸鍥偓姘崇箖閵?






### 娣囶喗鏁奸弬鍥︽



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY7_BACKEND_SESSION_EXIT_FIX.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂傤噣顣?70: 闂婃娊顣剁拋鎯ь槵婢惰精瑙﹂弮鍓佹畱鐟欏棝顣?only闂勫秶楠囨稉搴℃礀瑜版帡妫粋浣虹眰閸?


**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 瑜版挸澧犻張鍝勬珤娑?`WASAPI` 闂婃娊顣剁拋鎯ь槵閸掓繂顫愰崠鏍с亼鐠愩儲妞傞敍瀹峆layerCore::open()` 閾忕晫鍔ф导姘辨埛缂侇叀铔嬫稉瀣箵閿涘奔绲炬禒宥囧姧娴兼艾褰傞崙?`AudioInitFailed` 闁挎瑨顕ら敍灞筋嚤閼风补鈧粓娼懛鏉戞嚒閼宠棄濮忛梽宥囬獓閳ユ繂鎷伴垾婊呮埂濮濓絾澧﹀鈧径杈Е閳ユ繆顕㈡稊澶嬭穿閸︺劋绔寸挧鏋偓?


- 妤傛鐖滈悳鍥ф簚閺咁垳娈戦崶鐐茬秺濡偓閺屻儰绮涢悞鑸靛Ω `demux_dropped_packets` 瑜版挻鍨氱紒鐔剁婢惰精瑙﹂梻銊ь洣閿涙稑婀?video-only 闂勫秶楠囬弮璁圭礉鐞氼偆顩﹂悽銊╃叾妫版垶绁﹂惃鍕瘶娴兼氨鐤拋陇绻?`demux_ignored_packets`閿涘奔绮犻懓灞惧Ω閳ユ粓顣╅張鐔锋嫹閻ｃ儮鈧繆顕ら崚銈嗗灇閳ユ粓妲﹂崚妤勫剹閸樺娑崠鍛偓婵勨偓?


- 閺冪娀鐓舵０鎴ｇ翻閸戠儤妞傞敍灞炬＋闁槒绶幎濠佸瘜閺冨爼鎸撻柅鈧崶?`System`閿涘矁绻栫€靛湱鍑界憴鍡涱暥閹恒劏绻樻稉宥咁檮缁嬪啿鐣鹃敍灞肩瘍娑撳秴鍩勬禍搴ょ槚閺?video-only 閸︾儤娅欓妴?






### 閸樼喎娲滈崚鍡樼€?


- `initDecoders()` 瀹歌尙绮￠悽?`audio_player_->isInitialized()` 閹貉冨煑闂婃娊顣剁憴锝囩垳閺勵垰鎯侀崥顖滄暏閿涘奔绲?`open()` 鐏炲倻宸辩亸鎴炴▔瀵繒娈戦垾婊嗩潒妫版垵褰茬紒褏鐢婚幘?/ 闂婃娊顣?only 韫囧懘銆忔径杈Е閳ユ繂鍨庨弨顖ょ礉閹碘偓娴犮儴顢戞稉杞扮贩鐠ф牕澹囨担婊呮暏閼板奔绗夐弰顖滅摜閻ｃ儯鈧?


- `demux_dropped_packets` 閸氬本妞傚ǎ宄版値娴?ignored 閸?queue-drop 娑撱倗琚拠顓濈疅閿涘矂鈧倸鎮庨崑姘偓濠氬櫤鐟欏倹绁撮敍灞肩瑝闁倸鎮庨惄瀛樺复瑜版捇鐝惍浣哄芳缁嬪啿鐣鹃幀褔妫粋浣碘偓?


- 濞屸剝婀侀幎濠傜秼閸撳秵鎸遍弨鐐佸蹇曟纯閹恒儲姣氶棁鎻掑煂 diagnostics閿涘苯顕遍懛瀛樼槨濞嗭繝鍏樼憰浣告礀閻妫╄箛妤佸閼崇晫鈥樼拋銈嗘Ц閸氾箑褰傞悽鐔剁啊 video-only 闂勫秶楠囬妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐠嬪啯鏆?`PlayerCore::open()`閿?


  - 閺堝顫嬫０鎴炵ウ閺冭绱濋棅鎶筋暥鐠佹儳顦崚婵嗩潗閸栨牕銇戠拹銉ュ涧鐠?warning閿涘苯鑻熺紒褏鐢绘禒?video-only 濡€崇础閹垫挸绱?


  - 濞屸剝婀佺憴鍡涱暥濞翠焦妞傞敍宀勭叾妫版垼顔曟径鍥у灥婵瀵叉径杈Е娴犲秶娲块幒銉ㄧ箲閸ョ偛銇戠拹?


- 鐠嬪啯鏆ｆ稉缁樻闁界喖鈧瀚ㄩ敍?


  - 閺堝鐓舵０鎴ｇ翻閸戠儤妞傛担璺ㄦ暏 `Audio`



  - 閺冪娀鐓舵０鎴ｇ翻閸戣桨绲鹃張澶庮潒妫版垶绁﹂弮鏈靛▏閻?`Video`



  - 閸欘亝婀侀柈鎴掔瑝閸欘垳鏁ら弮鑸靛閸ョ偤鈧偓 `System`



- 閹碘晛鐫?`DiagnosticsSnapshot` 娑?CLI 鏉堟挸鍤敍灞炬煀婢?`audio_output_initialized / video_only_fallback / clock_source`閵?


- 鐏?`1080p60-check`閵嗕梗high-bitrate-check`閵嗕梗long-playback-check` 閻?demux 闂傘劎顩﹂弨閫涜礋 `demux_queue_drop_packets == 0`閿涘苯鑻熺悰銉ュ帠閹垫挸宓?`demux_ignored_packets / demux_queue_drop_packets`閵?


- 瀹告煡鍣搁弬鐗堝⒔鐞?`MSBuild`閵嗕梗--1080p60-check`閵嗕梗--high-bitrate-check`閵嗕梗--long-playback-check`閵嗕梗--performance-log-check`閿涙稑缍嬮崜宥夆偓姘崇箖閵嗕繖--4k-playback-check` 娴犲秴澧?`fallback_ok` 鐎涙劘绻樼粙瀣熅瀵板嫬绶熼崥搴ｇ敾婢跺嫮鎮婇妴?






### 娣囶喗鏁奸弬鍥︽



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY7_AUDIO_FALLBACK_AND_REGRESSION_GATES.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- docs/records/VERSION.md







## 闂傤噣顣?69: PlayerCore 閸嬫粍鎸遍弨璺哄經閵嗕礁瀵橀梼鐔峰灙閹碘偓閺堝娼堟稉?Clock/Demuxer 鐠佹崘顓搁崐杞版叏婢?






**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 閸︺劋绗傛稉鈧潪顔碱吀閺屻儰鑵戦崣鎴犲箛閿涘畭PlayerCore` 閻?EOF 閼奉亜濮╅崑婊勬尡閸滃矂鍎撮崚?UI 閸嬫粍鎸辩捄顖氱窞閸欘亙鎱ㄩ弨閫涚啊閹绢厽鏂侀悩鑸碘偓渚婄礉濞屸剝婀佺€瑰本鏆ｉ弨璺哄經 demux/audio/scheduler 缁捐法鈻奸敍灞界摠閸︺劉鈧粎濮搁幀浣稿嚒閸嬫嚎鈧胶鍤庣粙瀣弓濞撳應鈧繄娈戦崣顖炲櫢閸氼垶顥撻梽鈹库偓?


- `PacketQueue` 娴犲秳浜?`AVPacket*` 閸樼喎顫愰幐鍥嫛閹佃儻娴囬幍鈧張澶嬫綀閿涘畭flush/clear/reset` 鐠侯垰绶炴导姘朵粣閻ｆ瑦婀柌濠冩杹閸樺缂夐崠鍛偓?


- `Clock` 閸︺劎閮寸紒鐔告闁界喕鐭惧鍕瑐閻?`pause()` / `setSpeed()` 閸╁搫鍣弴瀛樻煀娑撳秵顒滅涵顕嗙礉缁绢垵顫嬫０鎴炲灗 system-clock 鐠侯垰绶炴导姘毉閻滅増妞傞梻纾嬬儲閸欐﹫绱盽Demuxer::open()` 閹镐線鏀ｇ拫鍐暏 `close()` 鏉╂ê鐡ㄩ崷銊ㄥ殰闁夸線顥撻梽鈹库偓?






### 閸樼喎娲滈崚鍡樼€?


- EOF 閼奉亜濮╅崑婊勬尡閸欐垹鏁撻崷?scheduler 閻?render 缁捐法鈻奸崘鍜冪礉娑撳秷鍏橀惄瀛樺复鐠ф澘鎮撳?`stop()`閿涘苯鎯侀崚娆庣窗鐟欙箑褰傜痪璺ㄢ柤閼?join閿涙稒妫€圭偟骞囬崶鐘愁劃闁偓閸栨牗鍨氶垾婊冨涧閺€?`state_`閳ユ縿鈧?


- `ThreadSafeQueue<AVPacket*>` 閸欘亞顓搁悶鍡樺瘹闁藉牊鎯夋潻鎰剁礉娑撳秶顓搁悶?FFmpeg 閸栧懐鏁撻崨钘夋噯閺堢噦绱濈€佃壈鍤?`clear()` 娑撳酣妲﹂崚妤佺€介弸鍕瑝娴兼岸鍣撮弨楣冩Е閸掓ぞ鑵戦崜鈺€缍戦崠鍛偓?


- `Clock::pause()` 閸忓牆鍟?`paused_` 閸愬秷顕拌ぐ鎾冲閺冨爼妫块敍瀹岰lock::setSpeed()` 娑旂喐鐥呴張澶婂帥閸ュ搫瀵查弮褍鐔€閸戝棴绱濈€佃壈鍤?system clock 鏉╃偟鐢婚幀褑顫﹂惍鏉戞綎閵?


- `Demuxer::open()` 閸︺劌鍑￠幐浣规箒 `mutex_` 閺冭泛鍟€濞喡ょ殶閻劌鎮撻弽铚傜窗娑撳﹪鏀ｉ惃?`close()`閿涘本甯撮崣锝咁槻閻劍妞傛导姘幢濮濇眹鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑?`PlayerCore` 婢х偛濮?deferred stop 閺€璺哄經鐠侯垰绶為敍娆礝F 閸?render 缁捐法鈻奸崘鍛涧閸欐垵鍤鍌涱劄閸嬫粍婧€鐠囬攱鐪伴獮鑸电垼鐠佹澘绶熷〒鍛倞閿涘苯鎮楃紒顓犳暠鐎瑰鍙忕痪璺ㄢ柤閹笛嗩攽閻喎鐤?`stop/join/flush`閿涘苯鎮撻弮鏈垫叏婢?`next/previous/quit` 鐠囬攱鐪伴惄瀛樺复鐠ф澘鐣弫?`stop()`閵?


- 閹?`PacketQueue` 閺€閫涜礋 `ThreadSafeQueue<unique_ptr<AVPacket, AvPacketDeleter>>`閿涘苯鑻熺拋?`ThreadSafeQueue::push()` 閺€顖涘瘮瀵ゆ儼绻?move閿涘苯浜ゆ惔鏇熷Ω FFmpeg 閸栧懐鏁撻崨钘夋噯閺堢喐鏁归崶鐐插煂 RAII閵?


- 娑?`Scheduler` 婢х偛濮炲鍌涱劄閸嬫粍婧€閸忋儱褰涢獮璺烘躬闁插秵鏌?`start()` 閸撳秴娲栭弨璺哄嚒闁偓閸?worker閿涘矂浼╅崗?EOF 閸氬簼绗呮稉鈧▎鈥虫儙閸斻劏袝閸?`std::terminate`閵?


- 娣囶喗顒?`Clock` 閻?pause/speed 閸╁搫鍣弴瀛樻煀闁槒绶敍灞肩箽鐠?system-clock 鐠侯垰绶為惃鍕畯閸嬫粌鎷伴崣姗€鈧喐妞傞梻纾嬬箾缂侇叏绱盽Demuxer::open()` 閺€閫涜礋閸︺劌鎮撴稉鈧柨浣哥厵閸愬懐娲块幒銉ュ彠闂傤厽妫潏鎾冲弳閿涘奔绗夐崘宥夊櫢閸?`close()`閵?


- 瀹告煡鍣搁弬鐗堝⒔鐞?`MSBuild` 閸忋劑鍣洪柌宥呯紦閿涙瓪& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`閿涘苯缍嬮崜宥囩波閺嬫粈璐?`0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?






### 娣囶喗鏁奸弬鍥︽



- `include/core/player_core.h`



- `include/core/scheduler.h`



- `include/thread_safe_queue.h`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `src/core/clock.cpp`



- `src/demuxer.cpp`



- `docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂傤噣顣?68: MSVC warning debt 閸掑棗鐪板〒鍛倞閿涘湑4819 / C4996 / C4706閿?






**閺冦儲婀?*: 2026-03-18







### 闂傤噣顣介幓蹇氬牚



- GitHub Actions 閻?Windows `Debug` 閺嬪嫬缂撻搹鐣屽姧瀹歌尙绮￠懗浠嬧偓姘崇箖娑撶粯绁︾粙瀣椽鐠囨埊绱濇担鍡涙毐閺堢喐鐣悾娆忋亣闁?warning閿涘苯濂栭崫?CI 閸欘垵顕伴幀褍鎷伴崥搴ｇ敾閸ョ偛缍婄€规矮缍呴妴?


- `C4819` 娑撴槒顩﹂弶銉ㄥ殰 MSVC 娴犮儲婀伴崷棰佸敩閻線銆夌拠璇插絿 UTF-8 濠ф劖鏋冩禒璁圭幢`C4996` 閸氬本妞傞崙铏瑰箛閸︺劎顑囨稉澶嬫煙婢跺瓨鏋冩禒璺烘嫲妞ゅ湱娲伴張顒€婀存禒锝囩垳閿涙稑褰熼張澶婄毌闁插繑婀伴崷?`C4100 / C4706` 閹绘劗銇氶妴?


- 闂団偓鐟曚礁鍘涢幐澶嗏偓婊呯椽閻礁鎲＄拃锔衡偓浣侯儑娑撳鏌?warning閵嗕焦婀伴崷?warning閳ユ繀绗佺仦鍌涘瀵偓濞岃崵鎮婇敍宀勪缉閸忓秶鐣濋崡鏇炲弿鐏炩偓闂堟瑩绮幎濠勬埂鐎圭偤妫舵０妯圭鐠у嘲鎮堕幒澶堚偓?






### 閸樼喎娲滈崚鍡樼€?


- 閺堫剙婀村┃鎰垳娑擃厼鐡ㄩ崷銊よ厬閺傚洦鏁為柌濠傛嫲 UTF-8 閸愬懎顔愰敍灞肩稻 MSVC 姒涙顓婚獮鏈电瑝閹?UTF-8 鐟欙綁鍣村┃鎰瀮娴犺绱濈憴锕€褰?`C4819`閵?


- FFmpeg / Quill 婢跺瓨鏋冩禒璺虹潣娴滃海顑囨稉澶嬫煙娓氭繆绂嗛敍灞肩瑝鎼存棁顕氱憰浣圭湴妞ゅ湱娲伴柅姘崇箖閺€鍦儑娑撳鏌熷┃鎰垳閺夈儲绉烽梽?warning閿涘本娲块崥鍫モ偓鍌滄畱閸旂偞纭堕弰顖涘Ω鐎瑰啩婊戦梾鏃傤瀲閸掓澘顦婚柈銊ャ仈閺傚洣娆?warning 鐏炲倶鈧?


- 閺堫剙婀?`logger / subtitle / player_core / main` 娑擃厺绮涙穱婵堟殌娴滃棜瀚㈤獮鎻掑讲閻╁瓨甯存穱顔藉竴閻ㄥ嫬鐣ㄩ崗銊ュ毐閺侀绗岀悰銊ㄦ彧瀵?warning閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`CMakeLists.txt` 娑擃厺璐?MSVC 閻╊喗鐖ｆ晶鐐插 `/utf-8 /external:anglebrackets /external:W0`閿涘矁顔€閺堫剙婀村┃鎰垳閹?UTF-8 缂傛牞鐦ч敍灞借嫙閹跺﹦顑囨稉澶嬫煙 angle-bracket 婢跺瓨鏋冩禒?warning 娑撳妾烽崚鏉款樆闁劌鐪版径鍕倞閵?


- 閸?`src/logger.cpp` 娑擃厽鏌婃晶鐐茬暔閸忋劎骞嗘晶鍐ㄥ綁闁插繗顕伴崣?helper閿涘瞼鏁?`_dupenv_s` 閺囨寧宕?Windows 娑撳娈?`std::getenv` 閻劍纭堕妴?


- 閸?`src/subtitle/srt_parser.cpp` 閸?`src/subtitle/ass_parser.cpp` 娑擃參鍣伴悽銊⑩偓娣瞚ndows 鐠?`sscanf_s`閿涘苯鍙炬禒鏍ч挬閸欓绻氶悾?`std::sscanf`閳ユ繄娈戠捄銊ラ挬閸欐媽袙閺嬫劕鍨庨弨顖ょ礉濞撳懐鎮婇張顒€婀?`C4996` 閼板奔绗夐惍鏉戞綎閸欘垳些濡炲秵鈧佲偓?


- 閸?`src/main.cpp` 娑擃厽濡告穱鈥冲娇婢跺嫮鎮婇崣鍌涙殶閺嶅洩顔囨稉?`[[maybe_unused]]`閿涙稑婀?`src/core/player_core.cpp` 娑擃厽濡?demux push 闁插秷鐦柅鏄忕帆閺€鐟板晸娑撶儤妯夊蹇氱ゴ閸婄》绱濋崢缁樺竴閺夆€叉鐞涖劏鎻蹇撳敶鐠у鈧厧顕遍懛瀵告畱 `C4706`閵?


- 闁插秵鏌婇幍褑顢?`MSBuild` 閸忋劑鍣洪柌宥呯紦閿涙瓪& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`閿涘苯缍嬮崜宥囩波閺嬫粈璐?`0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?






### 娣囶喗鏁奸弬鍥︽



- `CMakeLists.txt`



- `src/logger.cpp`



- `src/main.cpp`



- `src/subtitle/srt_parser.cpp`



- `src/subtitle/ass_parser.cpp`



- `src/core/player_core.cpp`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂傤噣顣?67: ASS 閺嶅洨顒风憴锝嗙€芥稉?UTF-16 鐎涙绠烽懠鍐ㄦ纯娣囶喗顒?






**閺冦儲婀?*: 2026-03-18







### 闂傤噣顣介幓蹇氬牚



- `ASS/SSA` override block 娑擃厾娈?`\fnArial`閵嗕梗\rDefault` 娑斿琚槐褍鍣鹃崘娆愮《娴兼俺顫﹂柨娆掝嚖鐠囧棗鍩嗛幋鎰垼缁涙儳鎮?`fnArial`閵嗕梗rDefault`閿涘苯顕遍懛鏉戠埗鐟欎焦鐗卞蹇旂垼缁涙儳銇戦弫鍫涒偓?


- `SubtitleTextRun.start/length` 娑斿澧犻幐?UTF-8 code point 鐠佲剝鏆熼敍灞肩稻 D3D11 鐎涙绠峰〒鍙夌厠閺堚偓缂佸牏娲块幒銉﹀Ω鐎瑰啩婊戞导鐘电舶 DirectWrite 閻?`DWRITE_TEXT_RANGE`閿涘矂浜ｉ崚?emoji 閹存牠娼?BMP 鐎涙顑侀弮鏈电窗娴溠呮晸閼煎啫娲块柨娆庣秴閵?


- 闂団偓鐟曚礁婀崜宥勭鏉?D3D11 閸樼喓鏁撶€涙绠烽柧鐐絹娴溿倕鎮楅敍灞藉晙閸嬫矮绔村▎鈩冩殻瀹搞儳鈻奸弸鍕紦婢跺秵鐗抽獮鑸靛Ω瑜版挸澧犵紒鎾寸亯閸氬本顒為崚鎷岊唶瑜版洘鏋冨锝冣偓?






### 閸樼喎娲滈崚鍡樼€?


- 閺冄喰掗弸鎰扳偓鏄忕帆閸︺劏顕伴崣?override 閺嶅洨顒烽崥宥嗘娴兼碍濡告潻鐐电敾鐎涙鐦濋弫缈犵秼閸氱偞甯€閿涘瞼宸辩亸鎴濐嚠鐢摜鏁?ASS 閺嶅洨顒烽崜宥囩磻閻ㄥ嫭妯夊蹇撳爱闁板秲鈧?


- 鐎涙绠风憴锝嗙€介梼鑸殿唽閸滃本瑕嗛弻鎾绘▉濞堝吀濞囬悽銊ょ啊娑撳秴鎮撻惃鍕瀮閺堫剟鏆辨惔锕侇嚔娑斿绱伴崜宥堚偓鍛焊閸?UTF-8 code point閿涘苯鎮楅懓鍛杽闂勫懘娓剁憰?UTF-16 code unit閵?


- 鏉╂瑧琚梻顕€顣芥稉宥勭窗閻╁瓨甯撮柅鐘冲灇瀹曗晜绨濋幋鏍ㄧ濠曞骏绱濇担鍡曠窗閻潙娼?ASS/SSA 閺嶅嘲绱＄€涙绠烽崷銊ュ斧閻?D3D11 闁惧彞鑵戦惃鍕嚔娑斿顒滅涵顔解偓褋鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`src/subtitle/ass_parser.cpp` 娑擃厼濮為崗銉ョ埗閻?ASS 閺嶅洨顒烽惃鍕▔瀵繐澧犵紓鈧崠褰掑帳閿涘本瀵滈張鈧梹鎸庣垼缁涘彞绱崗鍫ｇ槕閸?`alpha / bord / shad / pos / fn / fs / an / 1c / 1a / c / a / b / i / u / s / r`閿涘本婀崨鎴掕厬閺冭泛鍟€閸ョ偤鈧偓閸掔増妫惃鍕啍閺夋崘袙閺嬫劙鈧槒绶妴?


- 鐏?`ASS/SSA` 閺傚洦婀?run 闂€鍨閸滃瞼鍑介弬鍥ㄦ拱鐎涙绠?fallback 鐠侯垰绶炵紒鐔剁閺€閫涜礋閹?UTF-16 code unit 鐠佲剝鏆熼敍灞煎▏ `SubtitleTextRun` 娑?DirectWrite `DWRITE_TEXT_RANGE` 鐠囶厺绠熸稉鈧懛娣偓淇檃ss_parser.cpp` 閸愬懘銆庨幍瀣閹哄绨￠張顒€婀?`sscanf` / 鐏炩偓闁劌褰夐柌蹇涗紕閽勮棄鎲＄拃锔衡偓?


- 闁插秵鏌婇幍褑顢?`MSBuild` 閸忋劑鍣洪柌宥呯紦閿涙瓪& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`閿涘苯缍嬮崜宥囩波閺嬫粈璐?`167 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?






### 娣囶喗鏁奸弬鍥︽



- `src/subtitle/ass_parser.cpp`



- `src/render/d3d11_video_renderer.cpp`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂傤噣顣?66: 閸忋劌鐪弸鍕紦闂冭顢ｅ〒鍛倞娑?ASS/SSA 閸樼喓鏁?D3D11 鐎涙绠烽柧?






**閺冦儲婀?*: 2026-03-18







### 闂傤噣顣介幓蹇氬牚



- 閸忋劌鐪?`Debug|x64` 閺嬪嫬缂撻弴鎹愵潶婢舵艾顦╂径瀛樻瀮娴?濠ф劖鏋冩禒鍓佹畱缂傛牜鐖滅拠顖濐嚢闂冭顢ｉ敍灞筋嚤閼风补鈧窉3D11 閸樼喓鏁撻柧鍙ュ敩閻礁鍑￠弨鐟般偨閿涘奔绲鹃弮鐘崇《閺佺繝缍嬫宀冪槈閳ユ縿鈧?


- D3D11 閸樼喓鏁撶€涙绠烽柧鐐劃閸撳秴褰х憰鍡欐磰缁绢垱鏋冮張顒€褰旈崝鐙呯礉`.ass/.ssa` 閻ㄥ嫭鐗卞蹇嬧偓浣哥暰娴ｅ秴鎷版径姘蒋閸氬本妞傚┑鈧ú璇茬摟楠炴洝绻曞▽鈩冩箒鏉╂稑鍙?native renderer閵?


- 婢舵牗瀵曠€涙绠烽懛顏勫З閹恒垺绁存稉搴㈡▔瀵繐濮炴潪浠嬫付鐟曚浇顩惄?`.ass`閵嗕梗.ssa`閵嗕梗.srt` 娑撳顫掗弽鐓庣础閿涘矁鈧奔绗夐弰顖氬涧闂堛垹鎮?SRT閵?






### 閸樼喎娲滈崚鍡樼€?


- 闁劌鍨庣敮锔胯厬閺傚洦鏁為柌濠勬畱婢跺瓨鏋冩禒璺烘嫲濠ф劖鏋冩禒璺烘躬瑜版挸澧?MSVC/娴狅絿鐖滄い鐢电矋閸氬牅绗呯悮顐ヮ嚖鐠囦紮绱濈憴锕€褰傞崗銊ョ湰鐠囶厽纭?閺嶅洩顔囬崠鏍晩鐠囶垽绱濋弸鍕灇娑撳簼绗熼崝锟犫偓鏄忕帆閺冪姴鍙ч惃鍕€娲▎婵夌偑鈧?


- 閺冄冪摟楠炴洘膩閸ㄥ褰ч張澶屽嚱閺傚洦婀扮拠顓濈疅閿涘畭IVideoRenderer` 閹恒儱褰涙稊鐔峰涧閹恒儲鏁归崡鏇炵摟缁楋缚瑕嗛敍灞筋嚤閼?`PlayerCore` 閺冪姵纭堕幎?ASS/SSA 閻ㄥ嫭鐗卞蹇嬧偓浣哥湴缁狙冩嫲鐎规矮缍呮穱鈩冧紖娴肩姷绮?D3D11 濞撳弶鐓嬮崳銊ｂ偓?


- 閺冨爼妫跨痪澶哥瑐閸樼喐婀伴崣顏囆掗弸鎰礋閺夆剝妞块崝銊ョ摟楠炴洩绱濇稉宥堝喕娴犮儴銆冩潏?ASS/SSA 鐢瓕顫嗛惃鍕樋 cue 閸氬苯鐫嗛崷鐑樻珯閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐏忓棗褰堣ぐ鍗炴惙閻ㄥ嫬銇旈弬鍥︽閸滃本绨弬鍥︽閺€鐟板晸娑?ASCII-safe 瑜般垹绱￠敍灞句划婢?`MSBuild` 閸忋劑鍣洪弸鍕紦閸╄櫣鍤庨妴?


- 閹碘晛鐫嶇€涙绠烽弫鐗堝祦濡€崇€烽敍灞炬煀婢?`SubtitleStyle`閵嗕梗SubtitleTextRun`閵嗕梗SubtitleItem.layer/play_res/runs`閿涘苯鑻熺拋鈺勑掗弸鎰紣閸樺倹鏁幐?`.ass/.ssa/.srt`閵?


- 閺傛澘顤?`AssParser`閿涘矁袙閺?`Script Info / Styles / Events`閿涘矁顩惄鏍х埗閻?`\b \i \u \s \fs \fn \an \a \pos \c/\1c \alpha/\1a \bord \shad \r` 閺嶅洨顒烽妴?


- 鐠?`PlayerCore` 鐠侊紕鐣绘径姘蒋瑜版挸澧犲┑鈧ú璇茬摟楠炴洩绱濋獮鍫曗偓姘崇箖 `IVideoRenderer::setSubtitleItems()` 閹跺﹦绮ㄩ弸鍕鐎涙绠风€电钖勯惄瀛樺复闁礁鍙?`D3D11VideoRenderer`閵?


- 閸?`D3D11VideoRenderer` 閸氬奔绔撮崸?swap chain backbuffer 娑撳﹤鐣幋?ASS/SSA 閺傚洦婀版繅顐㈠帠閵嗕焦寮挎潏骞库偓渚€妲捐ぐ渚库偓浣藉剹閺咁垱顢嬮妴浣割嚠姒绘劕鎷扮€规矮缍呯紒妯哄煑閿涙盯娼?D3D11 濞撳弶鐓嬮崳銊╃帛鐠併倝鈧偓閸栨牔璐熺痪顖涙瀮閺堫剚妯夌粈鎭掆偓?


- 閺囧瓨鏌?`main.cpp` 閻ㄥ嫯鍤滈崝銊ヮ樆閹稿倸鐡ч獮鏇熷赴濞村銆庢惔蹇庤礋 `.ass -> .ssa -> .srt`閿涘苯鑻熼崷銊︽殻瀹搞儳鈻肩痪褍鍩嗛柌宥嗘煀妤犲矁鐦夐弸鍕紦闁俺绻冮妴?






### 娣囶喗鏁奸弬鍥︽



- `CMakeLists.txt`



- `include/subtitle/subtitle_parser.h`



- `include/subtitle/ass_parser.h`



- `include/subtitle/srt_parser.h`



- `include/subtitle/subtitle_timeline.h`



- `src/subtitle/subtitle_parser.cpp`



- `src/subtitle/ass_parser.cpp`



- `src/subtitle/srt_parser.cpp`



- `src/subtitle/subtitle_timeline.cpp`



- `include/render/video_renderer.h`



- `include/render/d3d11_video_renderer.h`



- `src/render/d3d11_video_renderer.cpp`



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/video_player.cpp`



- `src/main.cpp`



- `include/config/settings_manager.h`



- `include/media/format_support.h`



- `include/playlist/playlist_manager.h`



- `include/plugin/plugin_api.h`



- `include/plugin/plugin_manager.h`



- `include/filters/filter_registry.h`



- `include/filters/video_filter_chain.h`



- `include/filters/audio_filter_chain.h`



- `include/streaming/http_stream_downloader.h`



- `include/streaming/hls_manifest_parser.h`



- `include/streaming/dash_manifest_parser.h`



- `include/streaming/adaptive_bitrate_selector.h`



- `include/display.h`



- `include/render/sdl_video_renderer.h`



- `include/render/opengl_video_renderer.h`



- `include/core/frame_queue.h`



- `src/streaming/adaptive_bitrate_selector.cpp`



- `src/render/renderer_factory.cpp`



- `src/ui/skin_engine.cpp`



- `src/core/scheduler.cpp`



- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



---



## 闂傤噣顣?57: D3D11 閸樼喓鏁?GPU 濞撳弶鐓嬮柧鎹愃夋?






**閺冦儲婀?*: 2026-03-18







### 闂傤噣顣介幓蹇氬牚



- `D3D11VideoRenderer` 瀹歌尙绮￠崗宄邦槵閻欘剛鐝涢惃?D3D11 鐟欏棝顣堕崨鍫㈠箛閼宠棄濮忛敍灞肩稻鐎涙绠锋禒宥囧姧閸欘亙绻氱€涙ɑ鏋冮張顒傚Ц閹緤绱濆▽鈩冩箒閻喐顒滅紒妯哄煑閸掓澘鎮撴稉鈧崸?swap chain backbuffer 娑撳鈧?


- 閺冄冨瀻閺嬫劖鏋冨锝勭矝閹?D3D11 renderer 閹诲繗鍫稉?SDL 閸栧懓顥婇崳顭掔礉瀹歌尙绮℃稉宥囶儊閸氬牆缍嬮崜宥勭波鎼存挾濮搁幀浣碘偓?






### 閸樼喎娲滈崚鍡樼€?


- 閸樼喓鏁撶憴鍡涱暥娑撳娼版稉?D3D11VA device sharing 瀹歌尙绮￠拃钘夋勾閿涘苯澧挎担娆戝繁閸欙綁娉︽稉顓炴躬鐎涙绠烽崣鐘插鏉╂瑤绔撮崸妤佹付閸氬海娈?UI/overlay 閸氬牊鍨氱捄顖氱窞閵?


- 婵″倹鐏夌€涙绠锋禒宥勭贩鐠?SDL `Display` 閹存牞钂嬫禒鍓佹睏閻炲棝鎽奸敍灞炬殻閺夆剝瑕嗛弻鎾绘懠鐏忓彉绗夐懗鐣屝炴稉琛♀偓婊冪暚閺佹番鈧胶瀚粩瀣ㄢ偓浣稿斧閻?D3D11 GPU 闁炬崘鐭鹃垾婵勨偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`D3D11VideoRenderer` 閸愬懏鏌婃晶?D2D1 / DirectWrite 鐠у嫭绨敍宀€娲块幒銉ヮ嚠 DXGI swap chain backbuffer 鏉╂稖顢戠€涙绠烽弬鍥ㄦ拱缂佹ê鍩楅妴?


- 娣囨繄鏆€閻滅増婀?D3D11 鐟欏棝顣堕棃銏ゅ櫚閺嶇柉鐭惧鍕剁礉楠炶泛鐨㈢€涙绠风紒妯哄煑娑撴彃鍩岄崥灞肩鐢?present 閸撳秲鈧?


- 鐎佃娈忛崑婊勨偓浣哥摟楠炴洖褰夐崠鏍ь杻閸旂姴宓嗛弮鍫曞櫢缂佹﹫绱濈涵顔荤箽 seek / frame-step 閸氬骸鐡ч獮鏇氱瑢鐟欏棝顣堕悩鑸碘偓浣风閼锋番鈧?


- 閸氬本顒為弴瀛樻煀鐠佹崘顓搁弬鍥ㄣ€傞崪灞炬＋閸掑棙鐎介弬鍥ㄣ€傞惃鍕坊閸欒尪顕╅弰搴礉闁灝鍘ら崥搴ｇ敾缂佈呯敾濞岃法鏁ゆ潻鍥ㄦ埂缂佹捁顔戦妴?






### 娣囶喗鏁奸弬鍥︽



- `src/render/d3d11_video_renderer.cpp`



- `src/render/renderer_factory.cpp`



- `CMakeLists.txt`



- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`



- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



---







## 闂傤噣顣?12: 娴间椒绗熺痪褍顦跨痪璺ㄢ柤閺嬭埖鐎柌宥嗙€?






**閺冦儲婀?*: 2026-02-27







### 闂傤噣顣介幓蹇氬牚







閸樼喐婀侀弸鑸电€€涙ê婀禒銉ょ瑓闂傤噣顣介敍?


1. 缂佸嫪娆㈤懕宀冪煑娑撳秵绔婚弲甯礉VideoPlayer 閹垫寧濯存潻鍥ь樋閼卞矁鐭?


2. 缁捐法鈻煎Ο鈥崇€锋径宥嗘絽閿涘矂姣︽禒銉ф樊閹?


3. 閸愬懎鐡ㄧ粻锛勬倞鐎硅妲楅崙娲晩閿涘苯顕遍懛鏉戝蓟闁插秹鍣撮弨鍓х搼 bug







### 鐟欙絽鍠呴弬瑙勵攳







闁插秵鐎稉杞扮磼娑撴氨楠囨径姘卞殠缁嬪鐏﹂弸鍕剁窗







```



閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?


閳?                   VideoPlayer (娑撶粯甯堕崚璺烘珤)                    閳?


閳规壕鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?


閳? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?     閳?


閳? 閳?Demuxer      閳? 閳?DecoderWorker閳? 閳?Display      閳?     閳?


閳? 閳?(鐟欙絽鐨濈憗鍛珤)    閳? 閳?(鐟欙絿鐖滃銉ょ稊缁捐法鈻?閳? 閳?(濞撳弶鐓嬮崳?     閳?     閳?


閳? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?     閳?


閳?        閳?                閳?                閳?              閳?


閳?        閳?                閳?                閳?              閳?


閳? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳瑰备鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?     閳?


閳? 閳逛揪acketQueue   閳? 閳?Clock        閳? 閳?AudioPlayer  閳?     閳?


閳? 閳?(閸栧懘妲﹂崚?     閳? 閳?(閺冨爼鎸撻崥灞绢劄)   閳? 閳?(闂婃娊顣堕幘顓熸杹)   閳?     閳?


閳? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞? 閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?     閳?


閳规柡鏀㈤埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞鈧埞?


```







### 閺傛澘顤冪紒鍕







1. **Demuxer (鐟欙絽鐨濈憗鍛珤)**



   - 鐏忎浇顥?AVFormatContext 閻ㄥ嫯顕伴崣鏍ㄦ惙娴?


   - 閹绘劒绶电紒鐔剁閻?packet 鐠囪褰囬幒銉ュ經



   - 閺€顖涘瘮 seek 閹垮秳缍?






2. **DecoderWorker (鐟欙絿鐖滃銉ょ稊缁捐法鈻?**



   - 鐏忎浇顥婇崡鏇氶嚋濞翠胶娈戠憴锝囩垳闁槒绶?


   - 娴?PacketQueue 閼惧嘲褰?packet閿涘矁袙閻礁鎮楅柅姘崇箖閸ョ偠鐨熸潏鎾冲毉



   - 閺€顖涘瘮閺嗗倸浠?閹垹顦?flush







3. **ThreadSafeQueue (缁捐法鈻肩€瑰鍙忛梼鐔峰灙)**



   - 闁氨鏁ら惃鍕殠缁嬪鐣ㄩ崗銊╂Е閸掓膩閺?


   - 閺€顖涘瘮闂冭顢ｉ崪宀勬姜闂冭顢ｉ幙宥勭稊



   - 閺€顖涘瘮 EOF 娣団€冲娇娴肩娀鈧?






4. **Clock (閺冨爼鎸撻崥灞绢劄)**



   - 缁狅紕鎮婃稉缁樻闁?


   - 鐠侊紕鐣婚棅瀹狀潒妫版垵鎮撳銉ユ鏉?


   - 閺€顖涘瘮婢舵氨顫掗崥灞绢劄濡€崇础







### 娣囶喗鏁奸弬鍥︽







- 閺傛澘顤?`include/demuxer.h`, `src/demuxer.cpp`



- 閺傛澘顤?`include/decoder_worker.h`, `src/decoder_worker.cpp`



- 閺傛澘顤?`include/thread_safe_queue.h`



- 閺傛澘顤?`include/clock.h`, `src/clock.cpp`



- 闁插秵鐎?`include/video_player.h`, `src/video_player.cpp`



- 娣囶喗鏁?`CMakeLists.txt`



- 娣囶喖顦?`src/packet_reader.cpp` 閸欏矂鍣搁柌濠冩杹 bug







---







## 闂傤噣顣?11: 楠炶泛褰傜拠璇插絿 AVFormatContext 鐎佃壈鍤у畷鈺傜皾







**閺冦儲婀?*: 2026-02-27







### 闂傤噣顣介幓蹇氬牚







閹绢厽鏂佺憴鍡涱暥閺冭泛鍤悳鏉裤亣闁?FFmpeg 鐟欙絿鐖滈柨娆掝嚖閸滃矁顔栭梻顔煎暱缁愪礁绌垮┃鍐跨窗



```



[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).



[h264 @ ...] missing picture in access unit with size 12342



0xC0000005: 閸愭瑥鍙嗘担宥囩枂 0x... 閺冭泛褰傞悽鐔活問闂傤喖鍟跨粣?


```







### 閸樼喎娲滈崚鍡樼€?






鐟欏棝顣剁憴锝囩垳缁捐法鈻?(`VideoDecodeThread`) 閸滃矂鐓舵０鎴Ｐ掗惍浣哄殠缁?(`AudioDecodeThread`) 閸氬嫯鍤滈幏銉︽箒閻欘剛鐝涢惃鍕掗惍浣告珤鐎圭偘绶ラ敍灞肩稻閸忓彉闊╅崥灞肩娑?`AVFormatContext`閵嗗倷琚辨稉顏嗗殠缁嬪鑻熼崣鎴ｇ殶閻?`av_read_frame(format_ctx_, packet)` 鐎佃壈鍤ч弫鐗堝祦缁旂偘绨ら敍宀冾嚢閸欐牕鍩岄惃?packet 閺佺増宓侀柨娆庤础閿涘苯绱╅崣?H264 鐟欙絿鐖滈柨娆掝嚖閸滃苯鍞寸€涙顔栭梻顔煎暱缁愪降鈧?






### 鐟欙絽鍠呴弬瑙勵攳







瀵洖鍙嗙紒鐔剁閻?`PacketReaderThread` 娴ｆ粈璐熼崬顖欑閻?packet 鐠囪褰囬崗銉ュ經閿?






1. **閺傛澘顤?PacketReaderThread 缁?*



   - 娴ｆ粈璐熼崬顖欑閻?`av_read_frame()` 鐠嬪啰鏁ら悙?


   - 閺嶈宓?stream_index 鐏?packet 閸掑棗褰傞崚鏉款嚠鎼存梻娈?PacketQueue







2. **閺傛澘顤?PacketRef 閸?PacketQueue 缁?*



   - `PacketRef`: 閸栧懓顥?AVPacket 閻ㄥ嫭娅ら懗鐣岀波閺嬪嫪缍嬮敍灞炬暜閹镐胶些閸斻劏顕㈡稊?


   - `PacketQueue`: 缁捐法鈻肩€瑰鍙忛惃?packet 闂冪喎鍨敍灞炬暜閹镐線妯嗘繅鐐电搼瀵?






3. **娣囶喗鏁肩憴锝囩垳閸ｃ劍甯撮崣?*



   - `VideoDecoder` 閸?`AudioDecoder` 閺傛澘顤?`decodePacket()` 閺傝纭?


   - 閹恒儲鏁规径鏍劥娴肩姴鍙嗛惃?packet閿涘矁鈧矂娼崘鍛村劥鐠囪褰?






4. **闁插秵鐎憴锝囩垳缁捐法鈻?*



   - `VideoDecodeThread` 閸?`AudioDecodeThread` 娴?`PacketQueue` 閼惧嘲褰?packet



   - 娑撳秴鍟€閻╁瓨甯寸拫鍐暏 `av_read_frame()`







### 娣囶喗鏁奸弬鍥︽







- 閺傛澘顤?`include/packet_reader.h`



- 閺傛澘顤?`src/packet_reader.cpp`



- 娣囶喗鏁?`include/video_decoder.h`



- 娣囶喗鏁?`src/video_decoder.cpp`



- 娣囶喗鏁?`include/audio_decoder.h`



- 娣囶喗鏁?`src/audio_decoder.cpp`



- 娣囶喗鏁?`include/video_decode_thread.h`



- 娣囶喗鏁?`src/video_decode_thread.cpp`



- 娣囶喗鏁?`include/audio_decode_thread.h`



- 娣囶喗鏁?`src/audio_decode_thread.cpp`



- 娣囶喗鏁?`include/video_player.h`



- 娣囶喗鏁?`src/video_player.cpp`



- 娣囶喗鏁?`CMakeLists.txt`







---







## 闂傤噣顣?10: 婢舵俺袙閻礁娅掔€圭偘绶ョ粩鐐扮挨鐠囪褰囩€佃壈鍤х憴锝囩垳闁挎瑨顕?






## 闂傤噣顣?1: FFmpeg 8.0 閸忕厧顔愰幀褔妫舵０?






**閺冦儲婀?*: 2026-02-17







### 闂傤噣顣介幓蹇氬牚







缂傛牞鐦ч弮鑸靛Г闁挎瑱绱漙codec_ctx_->avctx->priv_data` 閸?FFmpeg 8.0 娑擃厺绗夐崣顖滄暏閵?






### 閸樼喎娲?






FFmpeg 8.0 閺囧瓨鏁兼禍?API閿涘瞼些闂勩倓绨＄€?`avctx->priv_data` 閻ㄥ嫮娲块幒銉問闂傤喓鈧?






### 鐟欙絽鍠呴弬瑙勵攳







娣囶喗鏁?`video_decoder.cpp` 閸?`audio_decoder.cpp`閿?


- 閸︺劏袙閻礁娅掔猾璁宠厬濞ｈ濮?`format_ctx_` 閹存劕鎲抽崣姗€鍣?


- 閻╁瓨甯存担璺ㄦ暏娴肩姴鍙嗛惃?format context 閼板矂娼禒?codec context 閼惧嘲褰?






### 娣囶喗鏁奸弬鍥︽







- `include/video_decoder.h`



- `include/audio_decoder.h`



- `src/video_decoder.cpp`



- `src/audio_decoder.cpp`







---







## 闂傤噣顣?2: 鐟欏棝顣跺ù浣哄偍瀵洑绗夐崠褰掑帳







**閺冦儲婀?*: 2026-02-24







### 闂傤噣顣介幓蹇氬牚







閹绢厽鏂?mp4 閺傚洣娆㈤弮璁圭礉鐟欏棝顣堕弮鐘崇《濮濓絽鐖堕弰鍓с仛閵嗗倹妫╄箛妤佹▔缁€鐚寸窗



```



decodeFrame: read packet, stream_index=0, expected=1



decodeFrame: packet stream mismatch, skipping



```



缁嬪绨顏嗗箚 48 濞嗏剝澧犻懗鍊燁嚢閸掔増顒滅涵顔炬畱鐟欏棝顣剁敮褋鈧?






### 閸樼喎娲?






MP4 閺傚洣娆㈤惃鍕ウ妞ゅ搫绨弰顖ょ窗闂婃娊顣跺ù?缁便垹绱?0) 閸︺劌澧犻敍宀冾潒妫版垶绁?缁便垹绱?1) 閸︺劌鎮楅妴淇檃v_read_frame()` 鏉╂柨娲栭惃鍕瘶閸欘垵鍏橀弰顖欐崲閹板繑绁﹂惃鍕剁礄闁艾鐖堕弰顖滎儑娑撯偓娑擃亝绁?- 闂婃娊顣跺ù渚婄礆閵嗗倸甯禒锝囩垳闁洤鍩屾稉宥呭爱闁板秶娈戝ù浣规閻╁瓨甯存潻鏂挎礀 false閿涘苯顕遍懛纾嬵潒妫版垵鎶氶弮鐘崇《鐟欙絿鐖滈妴?






### 鐟欙絽鍠呴弬瑙勵攳







娣囶喗鏁?`src/video_decoder.cpp` 閻?`decodeFrame()` 閺傝纭堕敍?


- 鐏忓棝浜ｉ崚棰佺瑝閸栧綊鍘ゅù浣规鏉╂柨娲?false閿涘本鏁兼稉?continue 鐠哄疇绻冪拠銉ュ瘶



- 瀵邦亞骞嗙拠璇插絿閻╂潙鍩岄幍鎯у煂濮濓絿鈥樺ù浣哄偍瀵洜娈戦崠?






### 娣囶喗鏁奸弬鍥︽







- `src/video_decoder.cpp`







### 娴狅絿鐖滈崣妯绘纯







```cpp



// 娣囶喗鏁奸崜?


if (packet->stream_index != stream_idx_) {



    av_packet_unref(packet);



    av_packet_free(&packet);



    return false;



}







// 娣囶喗鏁奸崥?


while (true) {



    ret = av_read_frame(format_ctx_, packet);



    // ...



    if (packet->stream_index != stream_idx_) {



        av_packet_unref(packet);



        continue;  // 缂佈呯敾瀵邦亞骞嗙拠璇插絿



    }



    break;  // 閹垫儳鍩屽锝団€橀惃鍕ウ



}



```







---







## 闂傤噣顣?3: 闂婃娊顣跺ù浣哄偍瀵洑绗夐崠褰掑帳







**閺冦儲婀?*: 2026-02-24







### 闂傤噣顣介幓蹇氬牚







娑撳氦顫嬫０鎴炵ウ缁便垹绱╅惄绋挎倱閻ㄥ嫰妫舵０姗堢礉娴ｅ棗鍤悳鏉挎躬闂婃娊顣剁憴锝囩垳閸ｃ劋鑵戦妴?






### 閸樼喎娲?






閸氬本鐗遍惃鍕６妫版﹫绱伴棅鎶筋暥閸栧懎褰查懗鎴掔瑝閺勵垳顑囨稉鈧稉顏囶潶鐠囪褰囬惃鍕ウ閵?






### 鐟欙絽鍠呴弬瑙勵攳







娣囶喗鏁?`src/audio_decoder.cpp` 閻?`decodeFrame()` 閺傝纭堕敍灞界安閻劋绗岀憴鍡涱暥鐟欙絿鐖滈崳銊ф祲閸氬瞼娈戞穱顔碱槻閵?






### 娣囶喗鏁奸弬鍥︽







- `src/audio_decoder.cpp`







---







## 闂傤噣顣?4: YUV 閺佺増宓佸〒鍙夌厠闁挎瑨顕?






**閺冦儲婀?*: 2026-02-24







### 闂傤噣顣介幓蹇氬牚







鐟欙絿鐖滈幋鎰閸氬海鈻兼惔蹇曠彌閸楁娊鈧偓閸戠尨绱濆▽鈩冩箒閻㈠娼伴弰鍓с仛閵?






### 閸樼喎娲?






`renderFrame` 閸戣姤鏆熸担璺ㄦ暏闁挎瑨顕ら惃?YUV 閺佺増宓侀敍?


- 閸樼喐娼垫导鐘烩偓鎺旀畱閺?`frame->data[0]`閿涘牆褰ч弰?Y 楠炴娊娼伴幐鍥嫛閿?


- 閻掕泛鎮楅柨娆掝嚖閸︽澘浜ｇ拋?Y/U/V 閺勵垵绻涚紒顓炵摠閸屻劎娈?






鐎圭偤妾稉?AVFrame 娑?Y/U/V 閺勵垰鍨庡鈧€涙ê鍋嶉惃鍕剁礉娴ｈ法鏁?`linesize` 閺夈儴顓哥粻妤佺槨鐞涘瞼娈戝銉╂毐閵?






### 鐟欙絽鍠呴弬瑙勵攳







1. 娴肩娀鈧帗鏆ｆ稉?AVFrame 閹稿洭鎷￠懓灞肩瑝閺?`data[0]`



2. 濮濓絿鈥樻担璺ㄦ暏 Y/U/V 楠炴娊娼伴惃鍕殶閹诡喖鎷扮悰灞姐亣鐏?






### 娣囶喗鏁奸弬鍥︽







- `src/display.cpp`



- `src/video_player.cpp`







### 娴狅絿鐖滈崣妯绘纯







```cpp



// video_player.cpp - 娣囶喗鏁奸崜?


display_->renderFrame(frame->data[0], frame->width, frame->height);







// video_player.cpp - 娣囶喗鏁奸崥?


display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);







// display.cpp - renderFrame 閸戣姤鏆?


// 娣囶喗鏁奸崜?


int ret = SDL_UpdateYUVTexture(



    texture_, nullptr,



    data, width,



    data + width * height, width / 2,



    data + width * height * 5 / 4, width / 2



);







// 娣囶喗鏁奸崥?


AVFrame* frame = (AVFrame*)data;



int ret = SDL_UpdateYUVTexture(



    texture_, nullptr,



    frame->data[0], frame->linesize[0],



    frame->data[1], frame->linesize[1],



    frame->data[2], frame->linesize[2]



);



```







---







## 闂傤噣顣?5: 娴间椒绗熺痪?Quill 閺冦儱绻旈柅姘朵壕







**閺冦儲婀?*: 2026-02-24







### 闂傤噣顣介幓蹇氬牚







- 閺冄勬）韫囨閮寸紒鐔峰涧娴ｈ法鏁?`std::cout/std::cerr`閿涘本妫ゅ▔鏇熷姬鐡掑厖绱掓稉姘愁唶瑜版洏鈧礁绱撳銉ュ晸閻╂ü绗屾潪顔挎祮闂団偓濮瑰倶鈧?


- 閺冪姾绻嶇悰灞炬闁板秶鐤嗛柅姘朵壕閿涘本妫ゅ▔鏇熺壌閹诡喚骞嗘晶鍐殶閺佸瓨妫╄箛妤冩窗瑜版洏鈧焦鏋冩禒璺恒亣鐏忓繋绗岀粵澶岄獓闂冨牆鈧鈧?


- 缂傝桨绠崑銉ワ紟閹嶇窗閻╊喖缍嶆稉宥呭讲閸愭瑦鍨?Quill 閸掓繂顫愰崠鏍с亼鐠愩儲妞傚▽鈩冩箒閺勫海鈥橀崨濠咁劅娑撳酣妾风痪褔鈧槒绶妴?






### 閸樼喎娲滈崚鍡樼€?






- 娑撻缚顫夐柆?Quill v6.x API 閸欐ɑ娲块弴鍙ュ閺冨墎顩﹂悽?Quill閿涘苯绱╃挧宄板閼宠棄鈧帡鈧偓閵?


- Logger 闁槒绶梿鍡曡厬閸︺劌銇旈弬鍥︽鐎瑰繐鍞撮敍灞惧⒖鐏炴洜鍋ｉ張澶愭閿涘本鏌婃晶鐐哄帳缂冾喕绗岄梽宥囬獓鐠侯垰绶為崶浼存閵?






### 鐟欙絽鍠呴弬瑙勵攳







- 闁插秵鏌婇崥顖滄暏 Quill閿涘本鐎鍝勭磽濮?Backend + ConsoleSink + RotatingFileSink 閸欏矂鈧岸浜鹃敍娑欐）韫囨瀵滈悡?`[time][level][thread][logger][category] message` 缂佺喍绔撮弽鐓庣础鏉堟挸鍤妴?


- 閺傛澘顤?`LoggingConfigLoader`閿涘矁袙閺?`config/logging.conf` 閸?`MVP_LOG_*` 閻滎垰顣ㄩ崣姗€鍣洪敍宀勬姜濞夋洖鈧壈鍤滈崝銊х眰濮濓絽鑻熸潏鎾冲毉 `LOG_WARNING`閵?


- 瑜?`USE_QUILL_LOGGING` 閺堫亜鐣炬稊澶堚偓浣烘窗瑜版洑绗夐崣顖氬晸閹?Quill 閹舵稑鍤鍌氱埗閺冭绱濋懛顏勫З闂勫秶楠囬崚?stdout/stderr閿涘苯鑻熸穱婵堟殌閺冄冪暞鐞涘奔璐熼妴?


- 閸氬本顒為弴瀛樻煀閺傚洦銆傞敍鍦燨GGING.md閵嗕箓ERSION.md閵嗕竼HANGELOG.md閿涘鑻熼幓鎰返姒涙顓婚柊宥囩枂閺傚洣娆㈤妴?






### 娣囶喗鏁奸弬鍥︽







- `include/logger.h`



- `src/logger.cpp`



- `config/logging.conf`



- `docs/design/LOGGING.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---







## 闂傤噣顣?6: 婢舵氨鍤庣粙瀣尡閺€鐐仸閺嬪嫰鍣搁弸?






**閺冦儲婀?*: 2026-02-25







### 闂傤噣顣介幓蹇氬牚







- 閸樼喐婀侀弸鑸电€稉鍝勫礋缁捐法鈻?playLoop閿涘矁袙閻礁鎷板〒鍙夌厠閸︺劌鎮撴稉鈧痪璺ㄢ柤



- 鐟欏棝顣剁憴锝囩垳娴兼岸妯嗘繅鐐磋閺屾挾鍤庣粙瀣剁礉鐎佃壈鍤ч悽濠氭桨閸楋繝銆?


- 闂婂疇顫嬫０鎴濇倱濮濄儱鐤勯悳鏉挎炊闂?


- 闂冪喎鍨鈩冩 CPU 韫囨瑨鐤嗙拠銏狀嚤閼锋潙宕伴悽銊ㄧ箖妤?






### 鐟欙絽鍠呴弬瑙勵攳







1. **閺傛澘顤?FrameQueue 濡剝婢樼猾?*



   - 鐎圭偟骞囩痪璺ㄢ柤鐎瑰鍙忛惃鍕姎闂冪喎鍨?


   - 娴ｈ法鏁?condition_variable 鐎圭偟骞囬梼璇差敚缁涘绶熼敍宀勪缉閸?CPU 韫囨瑨鐤嗙拠?


   - 閺€顖涘瘮 push/pop/clear/stop 閹垮秳缍?






2. **閺傛澘顤?VideoDecodeThread 閸?AudioDecodeThread**



   - 閻欘剛鐝涢惃鍕潒妫?闂婃娊顣剁憴锝囩垳缁捐法鈻?


   - 鐟欙絿鐖滈崥搴ｆ畱鐢団偓姘崇箖 FrameQueue 娴肩娀鈧帞绮板〒鍙夌厠缁捐法鈻?


   - 閺€顖涘瘮 pause/resume/flush 閹貉冨煑







3. **閺傛澘顤?SyncManager 閸氬本顒炵粻锛勬倞閸?*



   - 閺€顖涘瘮 AudioMaster/VideoMaster/Free 娑撳顫掗崥灞绢劄濡€崇础



   - 鐎圭偟骞囩敮褍娆㈡潻鐔活吀缁?


   - 鐎圭偟骞囩捄鍐叉姎/闁插秴顦茬敮褏鐡ラ悾?






4. **闁插秵鐎?VideoPlayer**



   - 娴犲骸宕熺痪璺ㄢ柤 playLoop 閺€閫涜礋婢舵氨鍤庣粙?renderLoop



   - 濞ｈ濮?setSyncMode 閺傝纭堕弨顖涘瘮閸氬本顒炲Ο鈥崇础閸掑洦宕?


   - 娣囶喖顦?AudioPlayer::play 缁涙儳鎮曟稉宥呭爱闁板秹妫舵０?






### 娣囶喗鏁奸弬鍥︽







- 閺傛澘顤?`include/frame_queue.h`



- 閺傛澘顤?`include/video_decode_thread.h`



- 閺傛澘顤?`include/audio_decode_thread.h`



- 閺傛澘顤?`include/sync_manager.h`



- 閺傛澘顤?`src/video_decode_thread.cpp`



- 閺傛澘顤?`src/audio_decode_thread.cpp`



- 閺傛澘顤?`src/sync_manager.cpp`



- 娣囶喗鏁?`include/video_player.h`



- 娣囶喗鏁?`include/audio_decoder.h`



- 娣囶喗鏁?`src/video_player.cpp`



- 娣囶喗鏁?`CMakeLists.txt`







---







## 闂傤噣顣?7: 闂婃娊顣堕幘顓熸杹閺嬭埖鐎穱顔碱槻







**閺冦儲婀?*: 2026-02-25







### 闂傤噣顣介幓蹇氬牚







- AudioDecodeThread 鐟欙絿鐖滈崥搴ｆ畱闂婃娊顣堕柅姘崇箖 FrameQueue 娴肩娀鈧帞绮?renderLoop



- renderLoop 闁劕鎶氱拫鍐暏 AudioPlayer::play()閿涘苯顕遍懛鎾叾妫版垶鏌囬弬顓犵敾缂?


- SDL 閸ョ偠鐨熼張鍝勫煑闂団偓鐟曚焦瀵旂紒顓犳畱閺佺増宓佸ù渚婄礉瑜版挸澧犵€圭偟骞囬弮鐘崇《濠娐ゅ喕







### 鐟欙絽鍠呴弬瑙勵攳







1. 娣囶喗鏁?AudioDecodeThread::start() 閺傝纭堕敍灞筋杻閸?AudioPlayer* 閸欏倹鏆?


2. 鐟欙絿鐖滅痪璺ㄢ柤鐟欙絿鐖滅€瑰本鍨氶崥搴礉閻╁瓨甯寸拫鍐暏 audio_player_->play() 鐏忓棙鏆熼幑顔芥杹閸?SDL 闂冪喎鍨?


3. 缁夊娅?renderLoop 娑擃厾娈戦棅鎶筋暥閹绢厽鏂佹禒锝囩垳閿涘瞼鏁辩憴锝囩垳缁捐法鈻奸惄瀛樺复婢跺嫮鎮?






### 娣囶喗鏁奸弬鍥︽







- `include/audio_decode_thread.h`



- `src/audio_decode_thread.cpp`



- `src/video_player.cpp`







---







## 瀵板懓袙閸愬磭娈戦梻顕€顣?






### 闂傤噣顣?8: 绾兛娆㈤崝鐘烩偓鐔恍掗惍浣规暜閹?






**閻樿埖鈧?*: 瀵板懎鐤勯悳?






闂団偓鐟曚焦鍧婇崝?CUDA/D3D11VA 绾兛娆㈤崝鐘烩偓鐔恍掗惍浣规暜閹镐緤绱濋幓鎰磳鐟欙絿鐖滈幀褑鍏橀妴?






---







## 闂傤噣顣?9: VideoFrame/AudioFrame 缁夎濮╃拠顓濈疅缂傛椽娅＄€佃壈鍤у畷鈺傜皾







**閺冦儲婀?*: 2026-02-25







### 闂傤噣顣介幓蹇氬牚







缁嬪绨崥顖氬З閹绢厽鏂侀崥搴ｇ彌閸楀啿绌垮┃鍐跨礉闁挎瑨顕ゆ穱鈩冧紖閿?


```



modern-video-player.exe - 鎼存梻鏁ょ粙瀣碍闁挎瑨顕?


0x00007FFF7A80DA4C 閹稿洣鎶ゅ鏇犳暏娴?0xFFFFFFFFFFFFFFFF 閸愬懎鐡ㄩ妴鍌濐嚉閸愬懎鐡ㄦ稉宥堝厴娑?read



```







### 閸樼喎娲滈崚鍡樼€?






`VideoFrame` 閸?`AudioFrame` 缁崵宸辩亸鎴烆劀绾喚娈戠粔璇插З鐠囶厺绠熺€圭偟骞囬妴?






閸?`FrameQueue::pop()` 娑擃厺濞囬悽?`std::move` 鐏忓棗鎶氱粔璇插З閸戞椽妲﹂崚妤嬬窗



```cpp



frame = std::move(queue_.front());



queue_.pop();



```







閻㈠彉绨▽鈩冩箒鐎规矮绠熺粔璇插З閺嬪嫰鈧姴鍤遍弫鏉挎嫲缁夎濮╃挧瀣偓鑹扮箥缁犳顑侀敍宀勭帛鐠併倗娈戠粔璇插З閹垮秳缍旈崣顏呮Ц濞村懏瀚圭拹?`frame_` 閹稿洭鎷￠妴鍌氱秼閸樼喎顕挒鈩冪€介弸鍕鐠嬪啰鏁?`av_frame_free(&frame_)` 闁插﹥鏂侀崘鍛摠閿涘瞼娲伴弽鍥ь嚠鐠烇紕娈?`frame_` 閸欐ɑ鍨氶幃顒傗敄閹稿洭鎷￠妴鍌涜閺屾挸鎯婇悳顖濐問闂傤喗顒濋幃顒傗敄閹稿洭鎷￠弮璺侯嚤閼锋潙绌垮┃鍐︹偓?






### 鐟欙絽鍠呴弬瑙勵攳







1. 娑?`VideoFrame` 缁粯鍧婇崝鐘敌╅崝銊︾€柅鐘插毐閺佹澘鎷扮粔璇插З鐠у鈧壈绻嶇粻妤冾儊



2. 娑?`AudioFrame` 缁粯鍧婇崝鐘敌╅崝銊︾€柅鐘插毐閺佹澘鎷扮粔璇插З鐠у鈧壈绻嶇粻妤冾儊



3. 閺勬儳绱￠崚鐘绘珟閹风柉绀夐弸鍕偓鐘插毐閺佹澘鎷伴幏鐤鐠у鈧壈绻嶇粻妤冾儊



4. 缁夎濮╅弮璺虹殺閸樼喎顕挒锛勬畱 `frame_` 閹稿洭鎷＄純顔昏礋 nullptr閿涘矂妲诲銏＄€介弸鍕闁插﹥鏂佹禒宥堫潶娴ｈ法鏁ら惃鍕敶鐎?






### 娣囶喗鏁奸弬鍥︽







- `include/video_decoder.h`



- `src/video_decoder.cpp`



- `include/audio_decoder.h`



- `src/audio_decoder.cpp`







### 娴狅絿鐖滈崣妯绘纯







```cpp



// video_decoder.h - 濞ｈ濮炵粔璇插З鐠囶厺绠熸竟鐗堟



VideoFrame(VideoFrame&& other) noexcept;



VideoFrame& operator=(VideoFrame&& other) noexcept;



VideoFrame(const VideoFrame&) = delete;



VideoFrame& operator=(const VideoFrame&) = delete;







// video_decoder.cpp - 鐎圭偟骞囩粔璇插З閺嬪嫰鈧姴鍤遍弫?


VideoFrame::VideoFrame(VideoFrame&& other) noexcept



    : frame_(other.frame_)



    , pts_(other.pts_) {



    other.frame_ = nullptr;



}







// audio_decoder.h/cpp - 缁鎶€鐎圭偟骞?


```







---







## 闂傤噣顣?10: 婢舵俺袙閻礁娅掔€圭偘绶ョ粩鐐扮挨鐠囪褰囩€佃壈鍤х憴锝囩垳闁挎瑨顕?






**閺冦儲婀?*: 2026-02-25







### 闂傤噣顣介幓蹇氬牚







閹绢厽鏂佺憴鍡涱暥閺冭泛鍤悳鏉裤亣闁?FFmpeg 鐟欙絿鐖滈柨娆掝嚖閿?


```



[h264 @ ...] Invalid NAL unit size (0 > 1045).



[h264 @ ...] missing picture in access unit with size 1049



[aac @ ...] channel element 0.0 duplicate



[mov,mp4,m4a,3gp,3g2,mj2 @ ...] DTS 2625751 < 2632415 out of order



```







### 閸樼喎娲滈崚鍡樼€?






閸?`VideoPlayer::open()` 娑擃厼鍨卞杞扮啊 `video_decoder_` 閸?`audio_decoder_` 鐟欙絿鐖滈崳銊ョ杽娓氬鏁ゆ禍搴ゅ箯閸欐牞顫嬫０鎴滀繆閹垬鈧倻鍔ч崥搴℃躬 `play()` -> `initDecodeThreads()` 娑擃厼寮甸崚娑樼紦娴?`VideoDecodeThread` 閸?`AudioDecodeThread`閿涘矁绻栨稉銈勯嚋缁鍞撮柈銊ュ嫉閸氬嫯鍤滈崚娑樼紦娴滃棙鏌婇惃鍕掗惍浣告珤鐎圭偘绶ラ妴?






娑撱倓閲滅憴锝囩垳閸ｃ劌鎮撻弮鏈电矤閸氬奔绔存稉?`AVFormatContext` 鐠囪褰?packet閿涘矂鈧姵鍨氶弫鐗堝祦缁旂偘绨ら敍灞筋嚤閼风袙閻線鏁婄拠顖氭嫲閺佺増宓侀幑鐔锋綎閵?






### 鐟欙絽鍠呴弬瑙勵攳







閸?`play()` 閺傝纭舵稉顓ㄧ礉鐠嬪啰鏁?`initDecodeThreads()` 娑斿澧犻敍灞藉帥閸忔娊妫撮獮鍫曞櫞閺€?`video_decoder_` 閸?`audio_decoder_`閿?






```cpp



void VideoPlayer::play() {



    // ...



    playing_.store(true);



    



    video_decoder_.reset();



    audio_decoder_.reset();



    



    if (!initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_)) {



        // ...



    }



}



```







### 娣囶喗鏁奸弬鍥︽







- `src/video_player.cpp`







---







## 閻╃鍙ч弬鍥ㄣ€?






- [VERSION.md](./VERSION.md) - 閻楀牊婀扮拋鏉跨秿



- [ARCHITECTURE.md](../design/ARCHITECTURE.md) - 閺嬭埖鐎拋鎹愵吀



- [WINDOWS_SETUP.md](../guides/WINDOWS_SETUP.md) - Windows 闁板秶鐤嗛幐鍥у础



- [LOGGING.md](../design/LOGGING.md) - 閺冦儱绻旂化鑽ょ埠鐠囧瓨妲?






---







## 闂傤噣顣?13: Core API + Scheduler + Filter 婢舵氨鍤庣粙瀣櫢閺嬪嫯鎯ら崷?






**閺冦儲婀?*: 2026-03-06







### 闂傤噣顣介幓蹇氬牚



- 闂団偓鐟曚焦瀵滅憴鍕壐瀵洖鍙?Core API閵嗕讣cheduler 閸?Filter 閹绘帊娆㈠鍡樼仸閿涘苯鑻熸穱婵囧瘮 `VideoPlayer` 婢舵牠鍎撮幒銉ュ經缁嬪啿鐣鹃妴?






### 閸樼喎娲滈崚鍡樼€?


- 閺冄勭仸閺嬪嫪浜?`VideoPlayer` 閼辨艾鎮庢径褔鍎撮崚鍡氫捍鐠愶綇绱濈紓鍝勭毌閸欘垱绱ㄦ潻娑氭畱閺嶇绺剧仦鍌欑瑢鐠嬪啫瀹崇仦鍌樷偓?


- 缂傚搫鐨?`USE_NEW_PLAYER_CORE` 閸欐甯舵潻浣盒╃捄顖氱窞娑撳娈戦弬鐗堢壋韫囧啫鐤勯悳鑸偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`core` 濡€虫健閿涙瓪PlayerCore`閵嗕梗Scheduler`閵嗕梗FrameQueue`閵嗕梗Clock`閵嗕梗Command`閵嗕梗Frame`閵?


- 閺傛澘顤?`filters` 濡€虫健閿涙俺绻冨銈呮珤閹恒儱褰涢妴浣规暈閸愬奔鑵戣箛鍐︹偓浣割槱閻炲棛顓搁柆鎾扁偓浣峰瘨鎼?鐎佃鐦惔?妤楀崬鎷版惔锕€鍞寸純顔芥姢闂€婧库偓?


- `VideoPlayer` 閺€褰掆偓鐘辫礋閸欏矁鐭惧鍕剁窗`USE_NEW_PLAYER_CORE=ON` 鐠х増鏌婇弽绋跨妇閿涘FF 娣囨繃瀵旈弮褍鐤勯悳鑸偓?


- 閺傛澘顤?`tests/core_frame_queue_tests.cpp`閵嗕梗tests/core_clock_tests.cpp`閵?






### 娣囶喗鏁奸弬鍥︽



- include/core/frame.h



- include/core/frame_queue.h



- include/core/clock.h



- include/core/command.h



- include/core/scheduler.h



- include/core/player_core.h



- src/core/frame.cpp



- src/core/clock.cpp



- src/core/scheduler.cpp



- src/core/player_core.cpp



- include/filters/video_filter.h



- include/filters/audio_filter.h



- include/filters/filter_registry.h



- include/filters/filter_pipeline.h



- include/filters/builtin_filters.h



- src/filters/filter_registry.cpp



- src/filters/filter_pipeline.cpp



- src/filters/brightness_filter.cpp



- src/filters/contrast_filter.cpp



- src/filters/saturation_filter.cpp



- src/filters/builtin_filters.cpp



- include/video_player.h



- src/video_player.cpp



- CMakeLists.txt



- tests/core_frame_queue_tests.cpp



- tests/core_clock_tests.cpp







---







## 闂傤噣顣?14: 閺嬭埖鐎弨鑸垫殐娑?Core 閸楁洝鐭惧?






**閺冦儲婀?*: 2026-03-06







### 闂傤噣顣介幓蹇氬牚



- 閸樺棗褰舵稉濠傝嫙鐎涙娈戦弬鐗堟＋閹绢厽鏂侀柧鎹愮熅婢х偛濮炴禍鍡欐樊閹躲倖鍨氶張顒€鎷扮悰灞艰礋娑撳秳绔撮懛鎾棑闂勨斂鈧?






### 閸樼喎娲滈崚鍡樼€?


- 閺冄囨懠鐠侯垰鎷伴弬浼存懠鐠侯垰鍙＄€涙ê顕遍懛瀛樺笓闂呮粏鐭惧鍕槻閺夊偊绱濇稉鏃€妫柧鎹愮熅鐎涙ê婀紒鎾寸€幀褍鑻熼崣鎴︽閹絻鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸掔娀娅庨弮褔鎽肩捄顖浤侀崸妤嬬礉缂佺喍绔撮崚?`VideoPlayer -> PlayerCore -> Scheduler -> Queue -> Output`閵?


- 閺嬪嫬缂撶化鑽ょ埠閺€閫涜礋娴犲懐绱拠鎴炴煀閺嶇绺惧Ο鈥虫健閵?


- 閺傛澘顤冮柌宥嗙€弬鍥ㄣ€傜拠瀛樻娣囨繄鏆€閺傚洣娆㈡稉搴や捍鐠愶綀绔熼悾灞烩偓?






### 娣囶喗鏁奸弬鍥︽



- CMakeLists.txt



- include/video_player.h



- src/video_player.cpp



- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md



- 閺冄勀侀崸妤€銇斿┃鎰瀮娴犺泛鍨归梽銈忕礄鐟?DEVELOP_LOG 闂傤噣顣?14閿?






---







## 闂傤噣顣?15: 鐏忓繐鐫嗙粣妤€褰涙潻鍥с亣娑撴梹瀚嬮幏鐣岀級閺€鍙ョ瑝缁嬪啿鐣?






**閺冦儲婀?*: 2026-03-06







### 闂傤噣顣介幓蹇氬牚



- 鐏忓繐鐫嗙拋鎯ь槵閹绢厽鏂佹妯哄瀻鏉堛劎宸肩憴鍡涱暥閺冭绱濈粣妤€褰涢崚婵嗩潗鐏忓搫顕潻鍥с亣閿涘苯濂栭崫宥嗘惙娴ｆ嚎鈧?


- 缁愭褰涢幏鏍ㄥ閸氬酣鍎撮崚鍡楁簚閺咁垯绗呭〒鍙夌厠閸栧搫鐓欓張顏勫挤閺冭埖娲块弬甯礉閻劍鍩涢幇鐔虹叀娑撹　鈧粎鐛ラ崣锝勭瑝閼冲€熺殶閺佺补鈧縿鈧?






### 閸樼喎娲滈崚鍡樼€?


- `Display::init()` 閻╁瓨甯存担璺ㄦ暏鐟欏棝顣堕崢鐔奉潗閸掑棜椴搁悳鍥у灡瀵よ櫣鐛ラ崣锝忕礉閺堫亝瀵滅仦蹇撶閸欘垳鏁ら崠鍝勭厵閸嬫岸顩荤仦蹇曠級閺€淇扁偓?


- 娴滃娆㈡径鍕倞娴犲懐娲冮崥?`SDL_WINDOWEVENT_RESIZED`閿涘本婀憰鍡欐磰 `SDL_WINDOWEVENT_SIZE_CHANGED`閵?


- 濞撳弶鐓嬮惄顔界垼閸栧搫鐓欓惄瀛樺复娴ｈ法鏁ょ粣妤€褰涚€逛粙鐝敍宀€宸辩亸鎴炲瘻鐟欏棝顣跺В鏂剧伐鐠侊紕鐣婚惃鍕窗閺嶅洨鐓╄ぐ顫偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸氼垰濮╅弮鍫曗偓姘崇箖 `SDL_GetDisplayUsableBounds()` 鐠侊紕鐣婚崣顖滄暏鐏炲繐绠烽崠鍝勭厵閿涘苯鐨㈤崚婵嗩潗缁愭褰涢梽鎰煑閸︺劌褰查悽銊ュ隘 90% 閸愬懎鑻熸穱婵囧瘮鐟欏棝顣跺В鏂剧伐閵?


- 閸氬本妞傛径鍕倞 `SDL_WINDOWEVENT_RESIZED` 娑?`SDL_WINDOWEVENT_SIZE_CHANGED`閿涘瞼鈥樻穱婵堢崶閸欙絽鏄傜€电褰夐崠鏍х杽閺冨墎鏁撻弫鍫涒偓?


- 閹稿绨憴鍡涱暥濮ｆ柧绶ョ拋锛勭暬 `SDL_RenderCopy` 閻ㄥ嫮娲伴弽鍥╃叐瑜邦澁绱濋柆鍨帳閹锋牗瀚块崥搴ｆ暰闂堛垺濯烘导鎼炩偓?






### 娣囶喗鏁奸弬鍥︽



- src/display.cpp



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







## 闂傤噣顣?16: 閺堚偓婢堆冨/缂傗晜鏂侀弮鍓佹暰闂堛垹宕辨担蹇ョ礉楠炴儼藟閸忓懎鐔€绾偓娴溿倓绨伴幒褍鍩?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 閹绢厽鏂侀弮鑸垫付婢堆冨缁愭褰涢幋鏍ㄥ珛閸斻劎缂夐弨鍓х崶閸欙綇绱濈憴鍡涱暥閻㈠娼伴崣顖濆厴閸椻€茬秶閿涘矂鐓舵０鎴犳埛缂侇厽鎸遍弨淇扁偓?


- 缂傚搫鐨潻娑樺閺壜扳偓渚€鐓堕柌蹇氱殶閼哄倶鈧焦瀚嬮崝銊ㄧ箻鎼达附娼粵澶婄唨绾偓娴溿倓绨伴懗钘夊閵?






### 閸樼喎娲滈崚鍡樼€?


- SDL 缁愭褰涙禍瀣╂婢跺嫮鎮婃稉搴㈣閺屾捁鐨熼悽銊ュ瀻閺侊絽婀稉宥呮倱缁捐法鈻肩捄顖氱窞閿涘瞼鐛ラ崣锝呮槀鐎电褰夐崠鏍ㄦ閺囨潙顔愰弰鎾剐曢崣鎴犳暰闂堛垹鍩涢弬鏉夸粻濠婄偑鈧?


- 閺勫墽銇氱仦鍌涚梾閺堝甯堕崚鑸垫蒋娑撳酣绱堕弽鍥︽唉娴滄帟顕Ч鍌欑瑐閹躲儴鍏橀崝娑栤偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐠嬪啯鏆ｆ禍瀣╂婢跺嫮鎮婄捄顖氱窞閿涙艾婀〒鍙夌厠/缁屾椽妫藉〒鍙夌厠鐠侯垰绶炴稉顓＄枂鐠?SDL 娴滃娆㈤敍灞煎瘜缁捐法鈻兼禒鍛Х鐠愰€涙唉娴滄帟顕Ч鍌樷偓?


- 娑?Display 濞ｈ濮為幒褍鍩楃仦鍌滅帛閸掕绱版潻娑樺閺壜扳偓渚€鐓堕柌蹇旀蒋閵嗕焦娈忛崑婊呭Ц閹焦褰佺粈鎭掆偓?


- 閺傛澘顤冩Η鐘崇垼娴溿倓绨伴敍姘珛閸斻劏绻樻惔锔芥蒋鐟欙箑褰?seek閿涘本瀚嬮崝銊╃叾闁插繑娼拫鍐Ν闂婃娊鍣洪妴?


- PlayerCore 婢х偛濮炵€?seek/闂婃娊鍣虹拠閿嬬湴閻ㄥ嫭绉风拹瑙勫⒔鐞涘被鈧?






### 娣囶喗鏁奸弬鍥︽



- include/display.h



- src/display.cpp



- src/core/player_core.cpp



- src/main.cpp



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







## 闂傤噣顣?17: 娴间椒绗熺痪?MPC-HC 濡€虫健妤犮劍鐏﹂拃钘夋勾閿涘牓妯佸▓鍏哥癌/娑撳甯规潻娑崇礆







**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 娴间椒绗熺痪褎膩閸ф顫夐崚鎺戝嚒鐎规矮绠熼敍灞肩稻婢舵碍鏆熷Ο鈥虫健缂傚搫鐨禒锝囩垳閸忋儱褰涢敍灞炬￥濞夋洜鎴风紒顓炶嫙鐞涘苯绱戦崣鎴欌偓?






### 閸樼喎娲滈崚鍡樼€?


- 閺冄冪杽閻滈浜掗弽绋跨妇閹绢厽鏂侀柧鎹愮熅娑撹桨瀵岄敍灞灸侀崸妤勭珶閻ｅ奔绗夌€瑰本鏆ｉ敍宀勬娴犮儱鍨庡銉﹀腹鏉╂稏鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤冮獮鑸靛复閸忋儰绱掓稉姘遍獓閸╄櫣顢呯拋鐐煢閸滃本膩閸ф顎囬弸璁圭窗娴犺濮熼梼鐔峰灙閵嗕礁鎶氬Ч鐘偓浣叫掗惍浣哄殠缁嬪鐔€缁眹鈧?


- 瀵洖鍙嗗〒鍙夌厠閹跺€熻杽鐏炲偊绱檂IVideoRenderer` + `RendererFactory`閿涘绱濋獮鎯邦唨 `PlayerCore` 閸掑洦宕查崚鐗堝▕鐠炩剝甯撮崣锝冣偓?


- 婢х偛濮為棅鎶筋暥閸у洩銆€閸?濞ｇ兘鐓堕崳銊ｂ偓浣叫掗惍浣告珤瀹搞儱宸堕妴浣哥摟楠?SRT 鐟欙絾鐎介妴浣规尡閺€鎯у灙鐞涖劊鈧浇顔曠純?韫囶偅宓庨柨顔衡偓浣烘瘖閼层們鈧焦褰冩禒韬测偓浣圭壐瀵繋绗屽ù浣哥崯娴ｆ捁袙閺嬫劖膩閸фぜ鈧?


- 鐎瑰苯鏉藉銈夋殔閸╄櫣琚稉搴ㄧ叾鐟欏棝顣跺銈夋殔闁炬拝绱濈悰銉╃秷闂婃娊鍣洪獮瀹犮€€濠娿倝鏆呴妴?


- 閸氬本顒為弴瀛樻煀 tasklist 鐎电懓绨插鎻掔杽閻滀即銆嶉妴?






### 娣囶喗鏁奸弬鍥︽



- CMakeLists.txt



- include/core/task_queue.h



- include/core/frame_pool.h



- src/core/frame_pool.cpp



- include/core/decoder_thread.h



- src/core/decoder_thread.cpp



- include/render/*



- src/render/*



- include/audio/*



- src/audio/*



- include/decoder/*



- src/decoder/*



- include/subtitle/*



- src/subtitle/*



- include/playlist/*



- src/playlist/*



- include/config/*



- src/config/*



- include/input/*



- src/input/*



- include/media/*



- src/media/*



- include/streaming/*



- src/streaming/*



- include/ui/*



- src/ui/*



- include/plugin/*



- src/plugin/*



- include/filters/filter_base.h



- include/filters/video_filter_chain.h



- src/filters/video_filter_chain.cpp



- include/filters/audio_filter_chain.h



- src/filters/audio_filter_chain.cpp



- include/filters/audio_filter.h



- include/filters/video_filter.h



- include/filters/builtin_filters.h



- src/filters/volume_balance_filter.cpp



- src/filters/builtin_filters.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/audio_player.h



- src/audio_player.cpp



- .monkeycode/specs/enterprise-quill-logging/tasklist.md



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







---







## 闂傤噣顣?18: DASH 鐟欙絾鐎界紓鏍槯婢惰精瑙︽稉搴㈢壐瀵繗鍏橀崝娑氱叐闂冪數宸辨径?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- `src/streaming/dash_manifest_parser.cpp` 閸?MSVC 娑撳绱拠鎴濄亼鐠愩儻绱濋梼璇差敚閸忋劑鍣洪弸鍕紦閵?


- 缂傚搫鐨稉鈧稉顏勫讲閻╁瓨甯存径宥囨暏閻ㄥ嫧鈧粏绻嶇悰灞炬閺嶇厧绱￠懗钘夊閻晠妯€閳ユ繂鍙嗛崣锝忕礉娑撳秴鍩勬禍搴″礋娴滈缚鍑禒锝勮厬韫囶偊鈧喖鐛欑拠浣圭壐瀵繗顩惄鏍モ偓?






### 閸樼喎娲滈崚鍡樼€?


- 閸樼喎顫愮€涙顑佹稉鍙夘劀閸掓瑤濞囬悽銊ょ啊姒涙顓婚崚鍡涙缁楋讣绱濈悰銊ㄦ彧瀵繋鑵戦崙铏瑰箛 `)"` 鐟欙箑褰傞幓鎰缂佸牊顒涢敍灞筋嚤閼风顕㈠▔鏇㈡晩鐠囶垬鈧?


- 閻滅増婀侀弽鐓庣础閺€顖涘瘮濡€虫健閾忚姤婀侀崺铏诡攨閹恒儱褰涢敍灞肩稻缂傚搫鐨紒鐔剁 CLI 濡偓閺屻儱鍙嗛崣锝勭瑢娑撹濮忛弽鐓庣础鐟曞棛娲婃潏鎾冲毉閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 娣囶喖顦?DASH 濮濓絽鍨敍姘暭娑撻缚鍤滅€规矮绠?raw-string 閸掑棝娈х粭锔肩礉閹垹顦?MSVC 缂傛牞鐦ч柅姘崇箖閵?


- 閹碘晛鐫?`FormatSupport`閿?


  - 婢х偛濮炴潻鎰攽閺冭泛顔愰崳?缂傛牞袙閻礁娅掗弸姘閿涘潉av_demuxer_iterate` / `av_codec_iterate`閿?


  - 婢х偛濮為幘顓熸杹閻╊喗鐖ｇ拠鍕強閿涘牓鐝崚鍡氶哺閻?妤傛ê鎶氶悳?婢舵岸鐓堕柆鎿勭礆



- 閺€褰掆偓?`main`閿涘本鏌婃晶鐐叉嚒娴犮倧绱?


  - `--capabilities`



  - `--evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>`



- 婢х偛宸?`Demuxer` 娑?`PlayerCore` 闂婃娊顣堕柧鎹愮熅缁嬪啿浠撮幀褝绱欐径姘剁叾闁捁绶崙鍝勫棘閺佹澘顕鎰┾偓渚€鍣搁柌鍥ㄧ壉閸ｃ劌顦查悽銊х搼閿涘鈧?






### 娣囶喗鏁奸弬鍥︽



- src/streaming/dash_manifest_parser.cpp



- include/media/format_support.h



- src/media/format_support.cpp



- include/demuxer.h



- src/demuxer.cpp



- include/audio_player.h



- src/audio_player.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md



- docs/README.md







---







## 闂傤噣顣?19: D3D11VA 绾剝袙閺堚偓鐏忓繘妫撮悳顖欑瑢鏉烆垵袙閸ョ偤鈧偓







**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 闂団偓鐟曚礁婀?Windows 娑撳绱崗鍫濆焺閻?D3D11VA 绾剝袙妤傛ê鍨庢潏銊у芳/妤傛ê鎶氶悳鍥潒妫版埊绱濋獮鍓佲€樻穱婵嗐亼鐠愩儲妞傞崣顖濆殰閸斻劌娲栭柅鈧潪顖澬掗妴?


- 绾兛娆㈢憴锝囩垳鏉堟挸鍤柅姘埗閺?GPU 鐢勫灗 `NV12`閿涘瞼骞囬張?SDL 濞撳弶鐓嬮柧鎹愮熅鐟曚焦鐪?`YUV420P`閿涘苯鐡ㄩ崷銊︾壐瀵繋绗夐崠褰掑帳妞嬪酣娅撻妴?






### 鐟欙絽鍠呴弬瑙勵攳



- `PlayerCore` 婢х偛濮?D3D11VA 鐏忔繆鐦柅鏄忕帆閿?


  - 濡偓濞?codec 閻?D3D11VA HW config閿?


  - 閸掓稑缂?`AV_HWDEVICE_TYPE_D3D11VA` 鐠佹儳顦稉濠佺瑓閺傚浄绱?


  - 缂佹垵鐣?`get_format` 閸ョ偠鐨熼柅澶嬪绾兛娆㈤崓蹇曠閺嶇厧绱￠妴?


- 閼?`avcodec_open2` 閸︺劎鈥栫憴锝堢熅瀵板嫬銇戠拹銉礉閼奉亜濮╅柌宥呯紦鐟欙絿鐖滄稉濠佺瑓閺傚洤鑻熼崶鐐衡偓鈧崚鎷岃拫鐟欙絻鈧?


- 閺傛澘顤冪憴鍡涱暥鐢嗙翻閸戦缚顫夐弫鎾懠鐠侯垽绱?


  - 绾兛娆㈢敮褍鍘?`av_hwframe_transfer_data` 鏉烆剙鍩岀化鑽ょ埠閸愬懎鐡ㄩ敍?


  - 闂?`YUV420P` 鐢呯埠娑撯偓缂?`sws_scale` 鏉烆兛璐?`YUV420P` 閸愬秷绻橀崗銉﹁閺屾挶鈧?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- src/core/player_core.cpp







---







## 闂傤噣顣?20: 閹恒垺绁撮崗銉ュ經娑撳孩鐗稿蹇撴礀瑜版帟鍓奸張顒冩儰閸?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 闂団偓鐟曚焦濡搁弽鐓庣础鐟曞棛娲婃宀冪槈娴犲簶鈧粍澧滃銉﹀ⅵ瀵偓鐟欏棝顣剁憴鍌氱檪閳ユ繂宕岀痪褌璐熼垾婊冨讲闁插秴顦查惃鍕嚒娴犮倛顢戦崶鐐茬秺閳ユ縿鈧?


- 閻滅増婀侀懗钘夊閸忋儱褰涢崣顏呮箒閹缍嬮懗钘夊鐠囧嫪鍙婇敍宀€宸辩亸鎴濆礋閺傚洣娆㈤幒銏＄ゴ閸滃本澹掗柌蹇旂壉閺堫剚濮ら崨濞库偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 娑擃厽鏌婃晶?`--probe-file <media_file>`閿涙俺绶崙?`probe.*` 閺堝搫娅掗崣顖濐嚢鐎涙顔岄敍灞藉瘶閸氼偄顔愰崳?鐟欏棝顣?闂婃娊顣堕悩鑸碘偓浣碘偓浣稿瀻鏉堛劎宸奸妴浣告姎閻滃洢鈧礁锛愰柆鎾茬瑢瀵ら缚顔呮穱鈩冧紖閵?


- 閺傛澘顤?`tools/format_regression/run_format_regression.ps1`閿?


  - 鐠囪褰?`tools/format_regression/format_samples.csv`閿?


  - 闁劒閲滅拫鍐暏 `--probe-file`閿?


  - 閻㈢喐鍨?`docs/reports/FORMAT_REGRESSION_*.md` 閹躲儱鎲￠敍?


  - 鏉╂柨娲栭惍浣筋嚔娑斿绱癭0=閸忋劑鍎碢ASS`閿涘畭1=鐎涙ê婀狿ARTIAL`閿涘畭2=鐎涙ê婀狥AIL`閵?


- 鐞涖儱鍘?`docs/workflows/FORMAT_REGRESSION.md` 娑撳孩鏋冨锝囧偍瀵洩绱濇笟澶哥艾閸?VS2022/PowerShell 娑撳娲块幒銉﹀⒔鐞涘被鈧?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- tools/format_regression/run_format_regression.ps1



- tools/format_regression/format_samples.csv



- docs/workflows/FORMAT_REGRESSION.md



- docs/README.md







---







## 闂傤噣顣?21: GitHub Actions 閼奉亜濮╅弽鐓庣础閸ョ偛缍婇幒銉ュ弳







**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 閸ョ偛缍婇柧鎹愮熅閾忚棄褰查崷銊︽拱閸︽媽绻嶇悰宀嬬礉娴ｅ棛宸辩亸?PR/娑撹鍨庨弨顖濆殰閸斻劍澧界悰宀嬬礉閺冪姵纭堕崷銊﹀絹娴溿倝妯佸▓闈涘挤閺冭埖瀚ら幋顏呯壐瀵繘鈧偓閸栨牓鈧?


- Windows CI 閻滎垰顣ㄦ稉搴㈡拱閸﹂绶风挧鏍у絺閻滅増鏌熷蹇庣瑝娑撯偓閼疯揪绱濋棁鈧悰銉╃秷閺嬪嫬缂撴稉搴ゅ壖閺堫剙鍚嬬€硅鈧佲偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤冨銉ょ稊濞?`.github/workflows/format-regression.yml`閿?


  - 閸?`windows-latest` 娑撳娴?`SDL2/FFmpeg` 妫板嫮绱拠鎴濆瘶楠炶埖鐎?`Debug`閿?


  - 閹笛嗩攽 `download_test_samples.ps1` 娑?`run_all_checks.ps1`閿?


  - 娑撳﹣绱?`docs/reports/FORMAT_REGRESSION_CI.md` 閹躲儱鎲℃禍褏澧块妴?


- 鐠嬪啯鏆?`CMakeLists.txt`閿?


  - Windows 娑撳绱崗鍫ｇ槕閸?`SDL2::`閵嗕梗FFMPEG::` 娑?`unofficial::ffmpeg::` 鐎电厧鍙嗛惄顔界垼閿?


  - 娣囨繄鏆€ `external/` 閻╊喖缍嶉崶鐐衡偓鈧柅鏄忕帆閿涘苯鍚嬬€硅婀伴崷鐗堟＆閺堝鐎鎭掆偓?


- 鐠嬪啯鏆?`download_test_samples.ps1`閿?


  - `-FfmpegPath` 閺€顖涘瘮 PATH 閸涙垝鎶ら崥宥忕礄婵?`ffmpeg`閿涘绱濇笟澶哥艾 CI 閻╁瓨甯寸拫鍐暏閵?


- 閺囧瓨鏌婇崶鐐茬秺閺傚洦銆傛稉搴濇崲閸斺剝绔婚崡鏇犲Ц閹緤绱濈悰銉╃秷閼奉亜濮╅崶鐐茬秺閸忋儱褰涚拠瀛樻閵?






### 娣囶喗鏁奸弬鍥︽



- .github/workflows/format-regression.yml



- CMakeLists.txt



- tools/download_test_samples.ps1



- docs/workflows/FORMAT_REGRESSION.md



- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md



- docs/README.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂傤噣顣?22: 閹绢厽鏂侀崚妤勩€冩稉濠氭懠鐠侯垬鈧浇顔曠純顔藉瘮娑斿懎瀵叉稉搴℃彥閹圭兘鏁＃鏍閹恒儱鍙?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 閹绢厽鏂侀崳銊ゅ瘜濞翠胶鈻兼禒鍛暜閹镐礁宕熼弬鍥︽閿涘奔绗夐弨顖涘瘮娑撳﹣绔存＃?娑撳绔存＃鏍︾瑢 EOF 閼奉亜濮╅崚鍥ㄥ床閵?


- 鐠佸墽鐤嗗Ο鈥虫健閺堫亝甯撮崗銉ㄧ箥鐞涘矂鎽肩捄顖ょ礉閸氼垰濮?闁偓閸戠儤妞傞弮鐘崇《閹垹顦查棅鎶藉櫤閸滃本鎸遍弨楣冣偓鐔峰閵?


- 姒涙顓昏箛顐ｅ祹闁款喚宸辨径鍗炲彠闁款喛鍏橀崝娑崇礄閻╃顕?seek閵嗕礁褰夐柅鐔粹偓渚€娼ら棅鐐解偓浣规尡閺€鎯у灙鐞涖劌鍨忛幑顫礆閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑撳鎽肩捄顖涘复閸?`PlaylistManager`閿?


  - 閺€顖涘瘮閸涙垝鎶ょ悰灞肩炊閸忋儱顦挎稉顏勭崯娴ｆ挻鏋冩禒璁圭幢



  - 閺€顖涘瘮娴肩姴鍙?`.m3u8` 娴ｆ粈璐熼幘顓熸杹閸掓銆冮敍?


  - 閺€顖涘瘮 `PageUp/PageDown` 娑撳﹣绔存＃?娑撳绔存＃鏍电幢



  - EOF 閼奉亜濮╅崚鍥ㄥ床娑撳绔存い骞库偓?


- 娑撳鎽肩捄顖涘复閸?`SettingsManager`閿?


  - 閸氼垰濮╅弮璺哄鏉?`config/player_settings.ini`閿?


  - 缂傚搫銇戦幋鏍掗弸鎰亼鐠愩儲妞傞崶鐐衡偓鈧妯款吇閸婄》绱欓棅鎶藉櫤 100%閵嗕線鈧喎瀹?1.0x閵嗕焦浠径宥勭瑐濞嗭紕鍌ㄥ鏇礆閿?


  - 闁偓閸戠儤妞傛穱婵嗙摠瑜版挸澧犻棅鎶藉櫤閵嗕焦鎸遍弨楣冣偓鐔峰閸滃本鎸遍弨鎯у灙鐞涖劎鍌ㄥ鏇樷偓?


- 閹碘晛鐫?SDL 娴滃娆㈤崚鐗堟尡閺€鎯ф珤閹貉冨煑闁炬崘鐭鹃敍?


  - `Left/Right` seek 鍗? 缁夋帪绱?


  - `Ctrl+Left/Ctrl+Right` seek 鍗?0 缁夋帪绱?


  - `[`/`]` 鐠嬪啴鈧噦绱漙R` 閹垹顦?1.0x閿?


  - `M` 闂堟瑩鐓?閹垹顦查敍?


  - `Enter/Alt+Enter/F` 閸忋劌鐫嗛崚鍥ㄥ床閿?


  - `Esc` 閸忋劌鐫嗛幀渚€鈧偓閸忋劌鐫嗛敍宀€鐛ラ崣锝嗏偓渚€鈧偓閸戞亽鈧?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/config/settings_manager.h



- src/config/settings_manager.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂傤噣顣?23: 缁夊娅?Core 閸楁洖鍘撳ù瀣槸閻╊喗鐖ｆ稉搴㈢ゴ鐠囨洘鏋冩禒?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 瑜版挸澧犳禒鎾崇氨娣囨繄鏆€娴滃棔琚辨稉?Core 閻╃鍙уù瀣槸閻╊喗鐖ｆ稉搴㈢ゴ鐠囨洘鏋冩禒璁圭礉娴ｅ棙婀板▎锟犳付濮瑰倽顩﹀Ч鍌溞╅梽銈堢箹娑撱倝銆嶅ù瀣槸閸愬懎顔愰獮璺哄灩闂勩倖鏋冩禒韬测偓?


- 閼汇儰绮庨崚鐘绘珟濞村鐦┃鎰垳閼板奔绗夊〒鍛倞閺嬪嫬缂撻懘姘拱閿涘奔绱扮€佃壈鍤ч弸鍕紦闁板秶鐤嗙€涙ê婀幃顒佸瘯鐠侯垰绶炴搴ㄦ珦閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 娴?`CMakeLists.txt` 缁夊娅庨敍?


  - `BUILD_CORE_TESTS` 闁銆嶉敍?


  - `core_frame_queue_tests`閵嗕梗core_clock_tests` 娑撱倓閲滃ù瀣槸閻╊喗鐖ｉ敍?


  - `core_tests` 閼辨艾鎮庨惄顔界垼閵?


- 閸掔娀娅庡ù瀣槸閺傚洣娆㈤敍?


  - `tests/core_frame_queue_tests.cpp`



  - `tests/core_clock_tests.cpp`



- 閸氬本顒為弴瀛樻煀閸欐ɑ娲块弬鍥ㄣ€傞敍宀€鈥樻穱婵婎唶瑜版洑绗岃ぐ鎾冲娴犳挸绨遍悩鑸碘偓浣风閼锋番鈧?






### 娣囶喗鏁奸弬鍥︽



- CMakeLists.txt



- tests/core_frame_queue_tests.cpp閿涘牆鍨归梽銈忕礆



- tests/core_clock_tests.cpp閿涘牆鍨归梽銈忕礆



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?24: 婢舵牗瀵曠€涙绠烽崝鐘烘祰閸忋儱褰涢敍鍦玆T閿涘甯撮崗銉ゅ瘜濞翠胶鈻?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.1.1` 鐟曚焦鐪伴弨顖涘瘮婢舵牗瀵曠€涙绠烽崝鐘烘祰閸忋儱褰涢敍灞肩稻瑜版挸澧犳稉缁樼ウ缁嬪褰ч張澶庮潒妫?闂婃娊顣堕幘顓熸杹闁炬崘鐭鹃敍灞炬弓閹绘劒绶垫径鏍劥鐎涙绠烽弬鍥︽閸忋儱褰涢妴?


- 妞ゅ湱娲板鎻掔摠閸?`subtitle::SrtParser`閿涘奔绲鹃張顏呭复閸?`VideoPlayer` 娑撳骸鎳℃禒銈堫攽閸欏倹鏆熼妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`VideoPlayer` 婢х偛濮炴径鏍ㄥ瘯鐎涙绠烽崝鐘烘祰閹恒儱褰涢敍?


  - `loadExternalSubtitle()` / `clearExternalSubtitle()`閿?


  - 閺€顖涘瘮 `.srt` 閺傚洣娆㈢憴锝嗙€芥稉搴☆啇闁挎瑦妫╄箛妤嬬幢



  - 閺嗘挳婀跺鎻掑鏉炶棄鐡ч獮鏇＄熅瀵板嫪绗岄弶锛勬窗閺佷即鍣洪敍灞肩┒娴滃骸鎮楃紒顓熻閺屾挻甯撮崗銉ｂ偓?


- 閸?`main` 婢х偛濮為崨鎴掓姢鐞涘苯鍙嗛崣锝忕窗



  - 閺傛澘顤?`--subtitle <file.srt>`閿?


  - 娣囨繃瀵旈悳鐗堟箒閹绢厽鏂侀崚妤勩€冮崣鍌涙殶闁槒绶敍?


  - 閺堫亝妯夊蹇庣炊閸欏倹妞傞敍宀冨殰閸斻劌鐨剧拠鏇炲鏉炴垝绗屾刊鎺嶇秼閸氬苯鎮曢惃?`.srt`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.1.1 婢舵牗瀵曠€涙绠烽崝鐘烘祰閸忋儱褰沗 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?25: 鐎涙绠峰〒鍙夌厠閸欑姴濮炴稉搴㈡尡閺€鐐鎼村繐鎮撳銉﹀复閸?






**閺冦儲婀?*: 2026-03-07







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.1.2` 鐟曚焦鐪扮€涙绠烽崣顖涜閺屾挸褰旈崝鐘插煂閻㈠娼伴敍灞肩稻閻滅増婀佸〒鍙夌厠閹恒儱褰涘▽鈩冩箒鐎涙绠烽弬鍥ㄦ拱闁岸浜鹃妴?


- 娴犺濮熷〒鍛礋 `1.1.3` 鐟曚焦鐪扮€涙绠锋稉搴㈡尡閺€?閺嗗倸浠?seek 閸氬本顒為敍灞肩稻娑撶粯鎸遍弨鐐闁界喖鎽肩捄顖涚梾閺堝鐡ч獮鏇熸闂傜閰遍弴瀛樻煀闁槒绶妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫嶅〒鍙夌厠閹跺€熻杽閿?


  - 閸?`IVideoRenderer` 婢х偛濮?`setSubtitleText()`閿?


  - SDL 濞撳弶鐓嬮崳銊ㄦ祮閸欐垵鐡ч獮鏇熸瀮閺堫剙鍩?`Display`閿?


  - D3D11/OpenGL 閸忓牊褰佹笟娑樺悑鐎硅銆呯€圭偟骞囬敍灞肩箽閹镐焦甯撮崣锝勭閼锋番鈧?


- 閸?`Display` 婢х偛濮炵€涙绠烽崣鐘插鐏炲偊绱?


  - 閺傛澘顤冪€涙绠烽悩鑸碘偓浣哥摠閸屻劋绗岀痪璺ㄢ柤鐎瑰鍙忛弴瀛樻煀閿?


  - 閸︺劏顫嬫０鎴濇姎濞撳弶鐓嬮崥搴涒偓浣瑰付閸掕埖娼〒鍙夌厠閸撳秶绮崚璺虹摟楠炴洟娼伴弶鍖＄幢



  - 閺€顖涘瘮婢舵俺顢戠€涙绠烽妴浣界Т闂€鎸庡焻閺傤厺绗岄崺铏诡攨閸欘垵顕伴幀褎鐗卞蹇ョ礄闂冩潙濂?閸楀﹪鈧繑妲戞惔鏇熸緲閿涘鈧?


  - 瑜版挸澧犳担璺ㄦ暏鏉炲鍣虹€涙膩濞撳弶鐓嬮敍宀勬姜 ASCII 鐎涙顑佹导姘舵缁狙勬▔缁€鎭掆偓?


- 閸?`PlayerCore` 婢х偛濮炵€涙绠烽弮鍫曟？鏉炴挳鈹嶉崝顭掔窗



  - 閺傛澘顤冩径鏍ㄥ瘯鐎涙绠锋潪銊╀壕閻樿埖鈧椒绗岀槐銏犵穿缂傛挸鐡ㄩ敍?


  - 濞撳弶鐓嬬敮褑鐭惧鍕瑢缁屾椽妫芥禍瀣╂鐠侯垰绶為崸鍥殶閻?`updateSubtitleOverlay()`閿?


  - 閸╄桨绨ぐ鎾冲閹绢厽鏂侀弮鍫曟？闁瀚ㄥú鏄忕┈鐎涙绠烽敍宀冾洬閻╂牗鎸遍弨淇扁偓浣规畯閸嬫粈绗?seek 閸︾儤娅欓敍?


  - 娣囶喖顦查柨浣稿敶鐠嬪啰鏁ゅ〒鍙夌厠閹恒儱褰涢惃鍕６妫版﹫绱濋柆鍨帳閸︺劌鐡ч獮鏇氱鞍閺傘儵鏀ｉ崘鍛靶曢崣鎴炶閺屾挸娲栫拫鍐︹偓?


- 鐠嬪啯鏆?`VideoPlayer::open()` 鐎涙绠烽悩鑸碘偓浣割槱閻炲棴绱濆☉鍫ユ珟閳ユ粌鍘涘〒鍛敄閸愬秴鍨介弬顓炲鏉炶В鈧繄娈戦惌娑氭禈闁槒绶妴?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.1.2`閵嗕梗1.1.3` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?26: 鐎涙绠峰鈧崗铏付閸掓湹绗岀€涙绠烽崝鐘烘祰瀵倸鐖舵径鍕倞鐎瑰苯鏉?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.1.4` 鐟曚焦鐪扮€涙绠峰鈧崗鍏呯瑢瀵倸鐖舵径鍕倞閿涘奔绲捐ぐ鎾冲鐎涙绠锋禒鍛暜閹镐讲鈧粌濮炴潪钘夋倵閺勫墽銇氶垾婵撶礉缂傚搫鐨潻鎰攽閺冭泛绱戦崗鐐解偓?


- 婢舵牗瀵曠€涙绠烽崝鐘烘祰鐠侯垰绶為崷銊︽瀮娴犲墎閮寸紒鐔风磽鐢婧€閺咁垯绗呯€瑰綊鏁婃稉宥堝喕閿涘苯濂栭崫宥嚽旂€规碍鈧囶暕閺堢喆鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 婢х偛濮炵€涙绠峰鈧崗铏付閸掑爼鎽肩捄顖ょ礄閹稿鏁?`V`閿涘绱?


  - `Display` 閺傛澘顤冪€涙绠峰鈧崗瀹狀嚞濮瑰偊绱?


  - `Renderer` 閹跺€熻杽閺傛澘顤?`consumeToggleSubtitleRequest()`閿?


  - `PlayerCore` 閺傛澘顤冪€涙绠烽弰鍓с仛閻樿埖鈧胶顓搁悶鍡曠瑢閸掑洦宕查幒銉ュ經閿?


  - 閸忔娊妫寸€涙绠烽弮鍓佺彌閸楄櫕绔荤粚鍝勫綌閸旂姴鐪伴敍灞界磻閸氼垱妞傞幐澶婄秼閸撳秵鎸遍弨鐐闂傚瓨浠径宥呮倱濮濄儯鈧?


- 婢х偛宸辨径鏍ㄥ瘯鐎涙绠峰鍌氱埗婢跺嫮鎮婇敍?


  - `VideoPlayer::loadExternalSubtitle()` 閺€閫涜礋娴ｈ法鏁?`std::error_code` 鐠侯垰绶炲Λ鈧弻銉幢



  - 閹规洝骞忕憴锝嗙€介崳銊ョ磽鐢鑻熼梽宥囬獓娑撳搫鎲＄拃锔芥）韫囨绱濇稉宥勮厬閺傤厽鎸遍弨鍙ュ瘜濞翠胶鈻奸敍?


  - 娣囨繃瀵旈垾婊冨鏉炶棄銇戠拹銉ㄥ殰閸斻劍绔荤粚鐑樻＋鐎涙绠烽垾婵堟畱閻樿埖鈧椒绔撮懛瀛樷偓褋鈧?


- 閺囧瓨鏌婄敮顔煎И娣団剝浼呴敍宀兯夐崗?`V - Toggle subtitles on/off`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.1.4` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?27: 韫囶偅宓庨柨顕€鍘ょ純顔藉瘮娑斿懎瀵查幒銉ュ弳閿涘潝otkey.*閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.3.2` 鐟曚焦鐪伴弨顖涘瘮闁款喕缍呴柊宥囩枂閹镐椒绠欓崠鏍电礉娴ｅ棗缍嬮崜宥呮彥閹圭兘鏁柅鏄忕帆閸ュ搫鐣鹃崘娆愵劥閸?`Display` 娴滃娆㈤崚鍡樻暜娑擃厹鈧?


- `HotkeyManager` 娴犲懏婀佹銊︾仸鐎圭偟骞囬敍灞炬弓閹恒儱鍙嗘稉缁樻尡閺€楣冩懠閿涘本妫ゅ▔鏇⑩偓姘崇箖闁板秶鐤嗛弬鍥︽娣囨繃瀵旈懛顏勭暰娑斿鏁担宥冣偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫?`HotkeyManager`閿?


  - 鐎靛綊缍堣ぐ鎾冲妫ｆ牜澧楄箛顐ｅ祹闁款噣绮拋銈嗘Ё鐏忓嫸绱欓幘顓熸杹閵嗕够eek閵嗕線鐓堕柌蹇嬧偓渚€娼ら棅鐐解偓浣稿綁闁喆鈧礁鍨忛梿鍡愨偓浣哥摟楠炴洖绱戦崗鐐解偓浣稿弿鐏炲繈鈧線鈧偓閸戠尨绱氶敍?


  - 婢х偛濮為柊宥囩枂鎼村繐鍨崠鏍厴閸旀冻绱癭actionConfigKey`閵嗕梗keyCodeToToken`閵嗕梗keyCodeFromToken`閵?


- 鐏忓棗鎻╅幑鐑芥暛閺勭姴鐨犻幒銉ュ弳濞撳弶鐓嬫潏鎾冲弳闁炬拝绱?


  - `Display` 娴滃娆㈡径鍕倞閺€閫涜礋閻?`HotkeyManager` 妞瑰崬濮╅敍?


  - 娣囨繄鏆€ `Esc` 娑?`Enter` 閻ㄥ嫬鍚嬬€圭顢戞稉鐚寸幢



  - `Renderer`/`PlayerCore`/`VideoPlayer` 婢х偛濮為悜顓㈡暛缁狅紕鎮婇柅蹇庣炊閹恒儱褰涢妴?


- 閸?`main` 閻ㄥ嫯顔曠純顔煎鏉?娣囨繂鐡ㄥù浣衡柤娑擃厽甯撮崗?`hotkey.*`閿?


  - 閸氼垰濮╃拠璇插絿楠炶泛绨查悽?`player_settings.ini` 閻?`hotkey.*`閿?


  - 闂堢偞纭堕柨顔荤秴闁板秶鐤嗛梽宥囬獓娑撴椽绮拋銈呰嫙鐠佹澘缍嶉崨濠咁劅閿?


  - 闁偓閸戠儤妞傜亸鍡楃秼閸撳秹鏁担宥呮礀閸愭瑩鍘ょ純顕嗙礉鐎圭偟骞囬幐浣风畽閸栨牓鈧?


- 閺囧瓨鏌婃妯款吇闁板秶鐤嗛弽铚傜伐 `config/player_settings.ini`閿涘矁藟姒绘劕鍙忛柈?`hotkey.*` 妞ゅ箍鈧?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.3.2` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?28: 韫囶偅宓庨柨顔煎暱缁愪焦顥呭ù瀣╃瑢閹垹顦叉妯款吇閼宠棄濮?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.3.3` 鐟曚焦鐪伴弨顖涘瘮闁款喕缍呴崘鑼崐濡偓濞村绗岄幁銏狀槻姒涙顓婚妴?


- 閻滅増婀?`hotkey.*` 閹镐椒绠欓崠鏍у嚒閸欘垰浼愭担婊愮礉娴ｅ棝鍣告径宥夋暛娴ｅ秹鍘ょ純顔荤窗娴溠呮晸閸斻劋缍旈崘鑼崐閿涘奔绗栫紓鍝勭毌閳ユ粈绔撮柨顔兼礀閸掍即绮拋銈傗偓婵婂厴閸旀稏鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫?`HotkeyManager`閿?


  - 閺傛澘顤?`findConflicts()` / `hasConflicts()`閿涘瞼鏁ゆ禍搴㈩梾濞村鍣告径宥夋暛娴ｅ秶绮︾€规熬绱?


  - 閺傛澘顤?`resetToDefaults()`閿涘瞼绮烘稉鈧幁銏狀槻姒涙顓婚柨顔荤秴閺勭姴鐨犻妴?


- 閸︺劎鍎归柨顕€鍘ょ純顔煎鏉炶姤绁︾粙瀣╄厬閸旂姴鍙嗛崘鑼崐濞岃崵鎮婇敍?


  - 閸氼垰濮╅弮璺哄帥鎼存梻鏁ら柊宥囩枂閸掓澘鈧瑩鈧妲х亸鍕剁礉閸愬秵澧界悰灞藉暱缁愪焦顥呭ù瀣剁幢



  - 閼汇儱褰傞悳鏉垮暱缁愪緤绱濈拋鏉跨秿閸愯尙鐛婇崝銊ょ稊娑撳酣鏁担宥嗘）韫囨绱濋懛顏勫З閸ョ偤鈧偓姒涙顓婚柨顔荤秴閿?


  - 鐎靛綊娼▔?token 娣囨繄鏆€姒涙顓婚獮鎯扮翻閸戝搫鎲＄拃锔衡偓?


- 閺傛澘顤冮幁銏狀槻姒涙顓诲鈧崗绛圭窗



  - 閸?`player_settings.ini` 婢х偛濮?`hotkey.restore_defaults`閿?


  - 鐠佸墽鐤嗘稉?`true` 閸氬簼绗呭▎鈥虫儙閸斻劏鍤滈崝銊︿划婢跺秹绮拋銈呰嫙閸ョ偛鍟撴稉?`false`閵?


- 閺囧瓨鏌婄敮顔煎И鏉堟挸鍤敍宀兯夐崗鍛划婢跺秹绮拋銈堫嚛閺勫簺鈧?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.3.3` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- src/main.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?29: M1 妤犲本鏁?1.4.1閿涘湯RT seek 閸氬本顒為懛顏咁梾閸涙垝鎶ら拃钘夋勾閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.4.1` 鐟曚焦鐪伴垾娓€SRT 鐎涙绠烽崣顖滄暏娑?seek 閸氬骸鎮撳顧﹂垾婵撶礉閻滅増婀佺€圭偟骞囩紓鍝勭毌閸欘垶鍣告径宥嗗⒔鐞涘瞼娈戞灞炬暪閸忋儱褰涢妴?


- 閼汇儱褰ф笟婵婄娴滃搫浼愰幘顓熸杹鐟欏倸鐧傞敍灞芥礀瑜版帗鍨氶張顒勭彯娑撴棃姣︽禒銉旂€规艾顦查悳鑸偓?






### 閸樼喎娲滈崚鍡樼€?


- 鐎涙绠烽弮鍫曟？鏉炴潙灏柊宥夆偓鏄忕帆娴犲懎鐡ㄩ崷銊ょ艾 `PlayerCore` 閸愬懘鍎撮敍灞炬￥濞夋洖宕熼悪顒勭崣鐠囦讲鈧粓銆庢惔蹇旀尡閺€?+ seek 鐠哄疇娴嗛垾婵呰⒈缁婧€閺咁垬鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹绘劕褰囬崗顒€鍙￠弮鍫曟？鏉炴潙鍤遍弫?`subtitle::resolveActiveSubtitleIndex(...)`閿涘苯鑻熼悽?`PlayerCore` 婢跺秶鏁ら妴?


- 閸?`main` 婢х偛濮?`--subtitle-sync-check <subtitle.srt>` 閸涙垝鎶ら敍?


  - 妞ゅ搫绨弮鍫曟？鏉炲瓨顥呴弻銉礄ordered閿涘绱?


  - 闂堢偤銆庢惔?seek 鐠哄疇娴嗗Λ鈧弻銉礄seek閿涘绱?


  - 鏉堟挸鍤?`mismatches` 娑?`PASS/FAIL`閵?


- 閺傛澘顤冮弽铚傜伐鐎涙绠?`samples/subtitle/subtitle_seek_sync_sample.srt` 閸滃本婀伴崷鐗堝Г閸?`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`閵?


- 娴犺濮熷〒鍛礋 `1.4.1` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- include/subtitle/subtitle_timeline.h



- src/subtitle/subtitle_timeline.cpp



- src/core/player_core.cpp



- src/main.cpp



- CMakeLists.txt



- samples/subtitle/subtitle_seek_sync_sample.srt



- samples/README.md



- docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?30: M1 妤犲本鏁?1.4.2閿涘牊鎸遍弨鎯у灙鐞涖劏绻涚紒顓熸尡閺€?5 閺傚洣娆㈤懛顏咁梾閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.4.2` 鐟曚焦鐪伴垾婊勬尡閺€鎯у灙鐞涖劏绻涚紒顓熸尡閺€?5 閺傚洣娆㈤柅姘崇箖閳ユ繐绱濇担鍡欏繁鐏忔垵褰查柌宥咁槻閹笛嗩攽閻ㄥ嫰鐛欓弨璺烘嚒娴犮們鈧?


- 娴犲懘娼幍瀣紣閻愮懓鍤宀冪槈閿涘苯娲栬ぐ鎺撴櫏閻滃洣缍嗘稉鏃傜波閺嬫粈绗夌粙鍐茬暰閵?






### 閸樼喎娲滈崚鍡樼€?


- 瑜版挸澧犳稉缁樼ウ缁嬪鍙挎径?EOF 閼奉亜濮╅崚鍥ㄥ床闁槒绶敍灞肩稻濞屸剝婀佺紒鎾寸€崠鏍翻閸戝搫褰查悽銊ょ艾韫囶偊鈧喖鐛欓弨韬测偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 閺傛澘顤?`--playlist-flow-check` 閸涙垝鎶ら敍?


  - 鐠囪褰囨潏鎾冲弳楠炶埖鐎鐑樻尡閺€鎯у灙鐞涱煉绱?


  - 閺嶏繝鐛欓懛鍐茬毌 5 閺夛紕娲伴敍?


  - 鐎电懓澧?5 閺夆剝澧界悰灞藉讲閹垫挸绱戦幒銏＄ゴ閿涘潉--probe-file` 閸氬本绨柅鏄忕帆閿涘绱?


  - 濡剝瀚?EOF 閼奉亜濮╅崚鍥ㄥ床妞ゅ搫绨敍宀勭崣鐠?`0,1,2,3,4` 鏉╃偟鐢荤憰鍡欐磰閿?


  - 鏉堟挸鍤?`PASS/FAIL` 娑撳骸銇戠拹銉у偍瀵洏鈧?


- 閺傛澘顤冮張顒€婀撮幎銉ユ啞 `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.4.2` 鐎瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md



- samples/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?31: M1 妤犲本鏁?1.4.3閿涘牐顔曠純顕€鍣搁崥顖涗划婢跺秷鍤滃Λ鈧敍?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `1.4.3` 鐟曚焦鐪伴垾婊嗩啎缂冾噣鍣搁崥顖氭倵閸欘垱浠径宥佲偓婵撶礉娴ｅ棛宸辩亸鎴濆讲闁插秴顦查幍褑顢戦惃鍕崣閺€璺哄弳閸欙絻鈧?


- 娴犲懏澧滃銉╁櫢閸氼垶鐛欑拠渚€姣︽禒銉洬閻╂牕鍙ч柨顔肩摟濞堢绱欓棅鎶藉櫤閵嗕線鈧喎瀹抽妴浣逛划婢跺秵鐖ｈ箛妞尖偓浣规尡閺€鎯у灙鐞涖劎鍌ㄥ鏇樷偓浣告彥閹圭兘鏁敍澶堚偓?






### 閸樼喎娲滈崚鍡樼€?


- 娑撶粯绁︾粙瀣嚒閺?`loadAppSettings/saveAppSettings`閿涘奔绲惧▽鈩冩箒閻欘剛鐝涢崨鎴掓姢鐞涘矁绶崙铏规暏娴滃骸娲栬ぐ鎺楃崣閺€韬测偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 閺傛澘顤?`--settings-persistence-check [settings_file]` 閸涙垝鎶ら敍?


  - 閸愭瑥鍙嗘稉鈧紒鍕ゴ鐠囨洝顔曠純顕嗙幢



  - 闁插秵鏌婇崝鐘烘祰楠炲爼鈧劙銆嶉弽锟犵崣 volume/speed/resume/index/hotkey閿?


  - 鏉堟挸鍤?`settings-persistence-check.result=PASS/FAIL`閵?


- 姒涙顓绘担璺ㄦ暏缁崵绮烘稉瀛樻閻╊喖缍嶆潻娑滎攽濡偓閺屻儻绱濇稉宥嗚杽閺屾捇銆嶉惄顔煎敶 `config/player_settings.ini`閵?


- 閺傛澘顤冮張顒€婀撮幎銉ユ啞 `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`1.4.3` 鐎瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?32: M2 2.1.2閿涘牆顔愰崳銊х叐闂冧絻藟姒?mov/avi/m2ts閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.1.2` 閻╊喗鐖ｇ憰浣圭湴鐎圭懓娅掔憰鍡欐磰 `mp4/mkv/mov/avi/webm/flv/ts/m2ts`閵?


- 閻滅増婀侀弽鐓庣础閸ョ偛缍婇弽閿嬫拱娴犲懓顩惄鏍︾啊 `mp4/mkv/webm/flv/ts`閿涘瞼宸辩亸?`mov/avi/m2ts` 鐎圭偞绁撮梻顓犲箚閵?






### 閸樼喎娲滈崚鍡樼€?


- 閸ョ偛缍婇弽閿嬫拱閸掓銆冩稉搴ゅ殰閸斻劎鏁撻幋鎰壖閺堫剚婀崠鍛儓 `mov/avi/m2ts` 鏉堟挸鍤敍灞筋嚤閼锋潙顔愰崳銊х叐闂冧絻顩惄鏍︾瑝鐎瑰本鏆ｉ妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫嶉弽閿嬫拱閻晠妯€ `format_samples.csv`閿涘本鏌婃晶鐑囩窗



  - `mov`閿涘潝264 + aac閿?


  - `avi`閿涘潝264 + mp3閿?


  - `m2ts`閿涘潝264 + ac3閿?


- 閹碘晛鐫?`tools/download_test_samples.ps1`閿?


  - 閺傛澘顤?`samples/mov`閵嗕梗samples/avi`閵嗕梗samples/m2ts` 閻╊喖缍嶉悽鐔稿灇閿?


  - 閺傛澘顤冩稉澶岃鐎圭懓娅掗弽閿嬫拱閻㈢喐鍨氶崨鎴掓姢閵?


- 閺囧瓨鏌婇弽閿嬫拱閻╊喖缍嶉弬鍥ㄣ€傛稉搴℃嫹閻ｃ儴顫夐崚娆欑礉鐞涖儵缍?`.gitkeep`閵?


- 閹笛嗩攽閺堫剙婀撮崶鐐茬秺楠炶埖娲块弬鐗堝Г閸涘绱癭docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`閵?


- 娴犺濮熷〒鍛礋 `2.1.2` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- samples/.gitignore



- samples/mov/.gitkeep



- samples/avi/.gitkeep



- samples/m2ts/.gitkeep



- samples/README.md



- docs/workflows/FORMAT_REGRESSION.md



- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?33: M2 2.1.3閿涘牐顫嬫０鎴犵椽閻胶鐓╅梼浣兯夋?MPEG-2閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.1.3` 閻╊喗鐖ｆ稉楦款潒妫版垹绱惍浣规暜閹?`H.264/H.265/VP9/AV1/MPEG-2`閵?


- 閻滅増婀侀崶鐐茬秺閺嶉攱婀板鑼额洬閻╂牕澧犻崶娑€嶉敍灞肩稻缂傚搫鐨?`MPEG-2` 鐟欏棝顣剁紓鏍垳閺嶉攱婀伴敍宀勭崣閺€鍫曟４閻滎垯绗夌€瑰本鏆ｉ妴?






### 閸樼喎娲滈崚鍡樼€?


- `format_samples.csv` 娑?`download_test_samples.ps1` 閺堫亜瀵橀崥?`mpeg2video` 閺嶉攱婀伴妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸︺劌娲栬ぐ鎺撶壉閺堫剛鐓╅梼鍏歌厬閺傛澘顤?`mpeg2video` 閺夛紕娲伴敍?


  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`



- 閸︺劍鐗遍張顒傛晸閹存劘鍓奸張顑胯厬閺傛澘顤?MPEG-2 閺嶉攱婀伴悽鐔稿灇濞翠胶鈻奸敍?


  - 鐟欏棝顣剁紓鏍垳 `mpeg2video`閿?


  - 闂婃娊顣剁紓鏍垳 `ac3`閿?


  - 鐎圭懓娅?`mpegts`閵?


- 鏉╂劘顢戦張顒€婀撮弽鐓庣础閸ョ偛缍婇獮鑸垫纯閺傜増濮ら崨濞库偓?


- 娴犺濮熷〒鍛礋 `2.1.3` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?34: M2 2.1.4閿涘牓鐓舵０鎴犵椽閻胶鐓╅梼浣兯夋?E-AC3/DTS/Vorbis/PCM閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.1.4` 閻╊喗鐖ｇ憰浣圭湴闂婃娊顣剁紓鏍垳鐟曞棛娲?`AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`閵?


- 閻滅増婀侀崶鐐茬秺閺嶉攱婀扮紓鍝勭毌 `E-AC3/DTS/Vorbis/PCM` 鐎圭偞绁撮敍宀勭崣閺€鍫曟４閻滎垯绗夌€瑰本鏆ｉ妴?






### 閸樼喎娲滈崚鍡樼€?


- 閸ョ偛缍婇弽閿嬫拱濞撳懎宕熼崪宀冨殰閸斻劍鐗遍張顒傛晸閹存劘鍓奸張顒佹弓鐟曞棛娲婃稉濠呭牚閸ユ稓琚棅鎶筋暥缂傛牜鐖滈妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫?`format_samples.csv` 閺傛澘顤冮崶娑欐蒋閺嶉攱婀伴敍?


  - `h264 + eac3 (mkv)`



  - `h264 + dts (mkv)`



  - `vp9 + vorbis (webm)`



  - `h264 + pcm_s16le (mov)`



- 閹碘晛鐫?`download_test_samples.ps1` 閻㈢喐鍨氬ù浣衡柤楠炴湹鎱ㄦ径?DTS 缂傛牜鐖滈敍?


  - `dca` 缂傛牜鐖滄担璺ㄦ暏 `-strict -2` 闁俺绻冪€圭偤鐛欓悧瑙勨偓褔妾洪崚韬测偓?


- 閹碘晛鐫嶉崶鐐茬秺閼存碍婀伴崗鐓庮啇缁涘鐜紓鏍垳閸氬稄绱?


  - `dts` <-> `dca`



  - `hevc` <-> `h265`



  - `pcm` <-> `pcm_*`



- 鏉╂劘顢戦張顒€婀撮崶鐐茬秺楠炶埖娲块弬鐗堝Г閸涘鈧?


- 娴犺濮熷〒鍛礋 `2.1.4` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- tools/format_regression/run_format_regression.ps1



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?35: M3 3.1.1閿涘湒ecoderFactory 閹恒儱鍙嗛惇鐔风杽閸掓繂顫愰崠鏍ㄧウ缁嬪绱?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `3.1.1` 鐟曚焦鐪?`DecoderFactory` 閹恒儱鍙嗛惇鐔风杽鐟欙絿鐖滈崚婵嗩潗閸栨牗绁︾粙瀣ㄢ偓?


- 閻滅増婀侀柧鎹愮熅娑擃叏绱漙DecoderFactory` 閺堫亜鑸伴幋鎰埠娑撯偓閻ㄥ嫧鈧粌鈧瑩鈧鎮楃粩?-> 闁劒閲滅亸婵婄槸 -> 婢惰精瑙﹂崶鐐衡偓鈧垾婵呭瘜濞翠胶鈻奸妴?






### 閸樼喎娲滈崚鍡樼€?


- `DecoderFactory` 娴犲懏褰佹笟娑掆偓婊勬付娴ｅ啿鎮楃粩顖椻偓婵嬧偓澶嬪閿涘瞼宸辩亸鎴濃偓娆撯偓澶婄碍閸掓甯撮崣锝冣偓?


- `PlayerCore::initDecoders` 閻ㄥ嫬鍨垫慨瀣娑撳骸娲栭柅鈧柅鏄忕帆閼帮箑鎮庨崷銊ョ湰闁劍娼禒璺哄瀻閺€顖欒厬閿涘奔绗夐崚鈺€绨紒鐔剁閹碘晛鐫嶉妴?






### 鐟欙絽鍠呴弬瑙勵攳



- `DecoderFactory` 閺傛澘顤?`selectBackendOrder(codec_name, prefer_hardware)`閿涘矁绶崙鐑樺瘻娴兼ê鍘涚痪褎甯撴惔蹇曟畱閸氬海顏崐娆撯偓澶婄碍閸掓绱濋獮鏈电箽閻ｆ瑨钂嬫禒鎯靶掗惍浣稿幑鎼存洏鈧?


- `PlayerCore::initDecoders` 閺€閫涜礋閹稿鈧瑩鈧绨崚妤呪偓鎰嚋鐏忔繆鐦崚婵嗩潗閸栨牭绱?


  - 鐎佃鐦℃稉顏勨偓娆撯偓澶婃倵缁旑垶鍣稿鍝勮嫙闁板秶鐤?`AVCodecContext`閿?


  - 閸氬海顏柊宥囩枂婢惰精瑙﹂幋?`avcodec_open2` 婢惰精瑙﹂弮鎯板殰閸斻劌鍨忛幑顫瑓娑撯偓娑擃亜鈧瑩鈧绱?


  - 閹存劕濮涢崥搴ｇ埠娑撯偓鐠佹澘缍嶉張鈧紒鍫Ｐ掗惍浣告倵缁旑垬鈧?


- `tryConfigureD3D11HardwareDecode` 鐠嬪啯鏆ｆ稉铏瑰嚱 D3D11 闁板秶鐤嗛懕宀冪煑閿涘奔绗夐崘宥呮躬閸戣姤鏆熼崘鍛粵閸氬海顏粵鏍殣閸愬磭鐡ラ妴?


- 娴犺濮熷〒鍛礋 `3.1.1` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- include/decoder/decoder_factory.h



- src/decoder/decoder_factory.cpp



- src/core/player_core.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?36: M3 3.1.2閿涘湒3D11VA 閸掓繂顫愰崠鏍с亼鐠愩儱娲栭柅鈧潪顖澬掗崗婊冪俺閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `3.1.2` 鐟曚焦鐪?D3D11VA 閸掓繂顫愰崠鏍с亼鐠愩儲妞傞崣顖炴浆閸ョ偤鈧偓鏉烆垵袙閵?


- 閻滅増婀侀柅鏄忕帆閸︺劌鍎氱槐鐘崇壐瀵繐宕楅崯鍡椼亼鐠愩儱婧€閺咁垯绗呮禒鍛）韫囨褰佺粈鐚寸礉閺堫亝妯夊蹇旀纯閺傛澘鎮楃粩顖滃Ц閹緤绱濈€涙ê婀悩鑸碘偓浣风瑝娑撯偓閼锋挳顥撻梽鈹库偓?






### 閸樼喎娲滈崚鍡樼€?


- `selectVideoPixelFormat` 閸︺劌宕楅崯鍡曠瑝閸?D3D11VA 閺嶇厧绱￠弮鏈电窗鏉╂柨娲栨潪顖欐閺嶇厧绱￠敍?


- 娴ｅ棙顒濋崜宥嗙梾閺堝鎮撳銉ュ瀼閹?`video_decoder_backend_` 娑撳海鈥栨禒璺哄剼缁辩姵鐗稿蹇曞Ц閹降鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`PlayerCore::selectVideoPixelFormat` 娑擃叀藟閸忓懏妯夊蹇氳拫鐟欙綁妾风痪褝绱?


  - `video_hw_pixel_fmt_ = AV_PIX_FMT_NONE`閿?


  - `video_decoder_backend_ = Software`閵?


- 閸?`initDecoders` 閸氬海顏亸婵婄槸闁炬崘鐭炬稉顓∷夐崗鍛偓娣?D11VA 閸楀繐鏅㈤梼鑸殿唽闂勫秶楠囨潪顖澬掗垾婵囨）韫囨ぜ鈧?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`3.1.2` 鐎瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- src/core/player_core.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?37: M3 3.2.1閿涘湒3D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻劑鎽肩捄顖ょ礆







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `3.2.1` 鐟曚焦鐪?D3D11 濞撳弶鐓嬮崗宄邦槵閺堚偓鐏忓繐褰查悽銊ㄥ厴閸旀冻绱檂init/upload/present`閿涘鈧?


- 閻滅増婀?`D3D11VideoRenderer` 娑撶儤銆呯€圭偟骞囬敍灞炬￥濞夋洖鍨垫慨瀣閵嗕焦妫ゅ▔鏇熺Х鐠愮懓鎶氶敍灞肩瘍閺冪姵纭堕崨鍫㈠箛閵?






### 閸樼喎娲滈崚鍡樼€?


- 濞撳弶鐓嬮崥搴ｎ伂閹恒儱褰涘鎻掔暰娑斿绱濇担?D3D11 閸氬海顏張顏呭复閸忋儱鐤勯梽鍛閺屾捇鎽肩捄顖ょ幢



- 缂傚搫鐨垾婊冪秼閸?SDL renderer 鐎圭偤妾崥搴ｎ伂閳ユ繄娈戦崣顖濐潎濞村鍏橀崝娑崇礉閺冪姵纭堕崚銈呯暰閺勵垰鎯侀惇鐔烘畱鐠烘垵婀?D3D11閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`Display` 娑擃厽鏌婃晶鐑囩窗



  - 濞撳弶鐓嬫す鍗炲З閸嬪繐銈界拋鍓х枂閿涘潉setPreferredRendererDriver`閿涘绱?


  - 瑜版挸澧犲〒鍙夌厠閸氬海顏憴鍌涚ゴ閿涘潉currentRendererDriver` / `isUsingRendererDriver`閿涘鈧?


- 鐎瑰本鏆ｇ€圭偟骞?`D3D11VideoRenderer` 閺堚偓鐏忓繐濮涢懗鏂ょ窗



  - `init` 閺冩儼顕Ч?`direct3d11` 妞瑰崬濮╅獮璺哄灡瀵ょ儤瑕嗛弻鎾绘懠鐠侯垽绱?


  - 閹恒儵鈧?`renderFrame`閿涘牅绗傛导鐙呯礆閵嗕梗present`閿涘牆鎲熼悳甯礆閵嗕梗clear`閵嗕椒绨ㄦ禒鏈电瑢閹貉冨煑鐠囬攱鐪伴柅蹇庣炊閵?


- 閸掓繂顫愰崠鏍ф倵閺嶏繝鐛欑€圭偤妾崥搴ｎ伂閿?


  - 閼汇儵娼?`direct3d11/d3d11`閿涘畭init` 婢惰精瑙﹂獮鎯邦唶瑜版洘妫╄箛妤嬬礉娴溿倗鏁辨稉濠傜湴閸ョ偤鈧偓 `SoftwareSDL`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`3.2.1` 鐎瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- include/display.h



- src/display.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?38: M3 3.3.2閿涘牊瑕嗛弻鎾炽亼鐠愩儵妾风痪褌绗夋稉顓熸焽閹绢厽鏂侀敍?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `3.3.2` 鐟曚焦鐪板〒鍙夌厠婢惰精瑙﹂弮鎯板殰閸斻劑妾风痪褌绗栨稉宥勮厬閺傤厽鎸遍弨淇扁偓?


- 閻滅増婀佺€圭偟骞囩紓鍝勭毌閸欘垶鍣告径宥嗗⒔鐞涘瞼娈戦懛顏勫З閸栨牠鐛欓弨璺哄弳閸欙綇绱濋梾鍙ヤ簰缁嬪啿鐣炬宀冪槈閸ョ偤鈧偓闁炬崘鐭鹃妴?






### 閸樼喎娲滈崚鍡樼€?


- D3D11 閸掓繂顫愰崠鏍с亼鐠愩儱婧€閺咁垱顒濋崜宥勫瘜鐟曚椒绶风挧鏍︽眽瀹搞儴袝閸欐埊绱濋弮鐘崇《娴ｆ粈璐熺粙鍐茬暰閸ョ偛缍婃い骞库偓?


- 缂傚搫鐨〒鍙夌厠閸氬海顏?鐟欙絿鐖滈崥搴ｎ伂閻ㄥ嫯绻嶇悰灞炬閸欘垵顫囧ù瀣摟濞堢绱濇稉宥勭┒娴滃骸鎳℃禒銈呭妤犲矁鐦夐妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫嶅〒鍙夌厠閹恒儱褰涢敍灞筋杻閸旂姴鎮楃粩顖氭倳缁夌増鐓＄拠顫礄`rendererBackendName`閿涘鈧?


- 閹碘晛鐫嶉幘顓熸杹閸ｃ劌顕径鏍ㄥ复閸欙綇绱濋弳鎾苟瑜版挸澧犲〒鍙夌厠閸氬海顏崪宀冃掗惍浣告倵缁旑垰鎮曠粔鑸偓?


- 閺傛澘顤冮崨鎴掓姢 `--renderer-fallback-check <media_file>`閿?


  - 闁俺绻冮悳顖氼暔閸欐﹢鍣?`MVP_D3D11_DRIVER_HINT=software` 瀵搫鍩?D3D11 濞撳弶鐓嬮崚婵嗩潗閸栨牕銇戠拹銉幢



  - 妤犲矁鐦夋稉濠氭懠鐠侯垱妲搁崥锕佸殰閸斻劌娲栭柅鈧崚?`SoftwareSDL` 楠炴儼鍏樻潻娑樺弳閹绢厽鏂佸顏嗗箚閿?


  - 鏉堟挸鍤?`renderer-fallback-check.*` 鐎涙顔岄崪?`PASS/FAIL`閵?


- 閺傛澘顤冮張顒€婀撮崶鐐茬秺閹躲儱鎲?`docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`閵?


- 娴犺濮熷〒鍛礋 `3.3.2` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?39: M3 3.3.1閿涘湹indows 鏉烆垵袙+绾剝袙娑撹濮忛弽閿嬫拱閸欘垱鎸遍敍?


**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `3.3.1` 鐟曚焦鐪?Windows 娑撳鈥栫憴锝勭瑢鏉烆垵袙娑撹濮忛弽閿嬫拱閸у洤褰茬粙鍐茬暰鏉╂稑鍙嗛幘顓熸杹闁炬崘鐭鹃妴?


- 閻滅増婀佸Λ鈧弻銉ㄧ熅瀵板嫮宸辩亸鎴犵埠娑撯偓閼辨艾鎮庨崨鎴掓姢閿涘奔绗栭崥宀冪箻缁嬪绻涚紒顓濈窗鐠囨繂婀柈銊ュ瀻閸︾儤娅欑€涙ê婀崑婊勵剾闂冭埖顔岄崡鈩冾劥妞嬪酣娅撻妴?






### 閸樼喎娲滈崚鍡樼€?


- 娑斿澧犻惃?`--windows-backend-check` 閸︺劌鎮撴潻娑氣柤妞ゅ搫绨幍褑顢戠涵顒冃?鏉烆垵袙閿涘奔绱扮憴锕€褰傛禍灞绢偧娴兼俺鐦界挧鍕爱閸ョ偞鏁规稉宥嚽旂€规哎鈧?


- 缂傚搫鐨崣顖氼槻閻劎娈戞导姘崇樈缁狙嗙槚閺傤叀绶崙鐚寸礉娑撳秴鍩勬禍搴℃礀瑜版帗濮ら崨濠呭殰閸斻劌瀵查柌鍥肠閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤冩导姘崇樈缁狙冩嚒娴?`--windows-backend-session-check <media_file> <hard|soft>`閿涘矁绶崙铏圭波閺嬪嫬瀵茬€涙顔岄獮鎯扮箲閸ョ偞膩瀵繒绮ㄩ弸婧库偓?


- 鐏忓棜浠涢崥鍫濇嚒娴?`--windows-backend-check <media_file>` 閺€閫涜礋閻栨儼绻樼粙瀣鐠ц渹琚辨稉顏勭摍鏉╂稓鈻奸敍鍧攁rd/soft閿涘鑻熷Ч鍥ㄢ偓鑽ょ波閺嬫粣绱濋梾鏃傤瀲娴兼俺鐦介悩鑸碘偓浣碘偓?


- 閸?Windows 娑撳濞囬悽?`CreateProcess` 闁插秴鐣鹃崥鎴ｇ翻閸戠尨绱濋柆鍨帳 shell 闁插秴鐣鹃崥鎴Ｐ掗弸鎰瑝缁嬪啿鐣鹃妴?


- 閺傛澘顤冮張顒€婀撮崶鐐茬秺閹躲儱鎲?`docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`閵?


- 鐎瑰本鍨氭禒璇插濞撳懎宕熼崥灞绢劄閿涙瓪3.3.1`閵嗕梗3.3.3` 閺嶅洩顔囩€瑰本鍨氶妴?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?40: M4 4.1閿涘牏鐝烽懞鍌氼嚤閼割亷绱版稉濠佺缁?娑撳绔寸粩鐙呯礆







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `4.1` 闂団偓鐟曚焦鏁幐浣虹彿閼哄倸顕遍懜顏庣礄娑撳﹣绔寸粩?娑撳绔寸粩鐙呯礆閵?


- 瑜版挸澧犻幘顓熸杹闁炬崘鐭剧紓鍝勭毌缁旂姾濡崗鍐╂殶閹诡喗绉风拹閫涚瑢缁旂姾濡捄瀹犳祮閸忋儱褰涢敍灞炬￥濞夋洟鈧俺绻冭箛顐ｅ祹闁款喚娲块幒銉ㄧ儲缁旂姰鈧?






### 閸樼喎娲滈崚鍡樼€?


- `Demuxer` 閾忓€熷厴鐠囪褰囨刊鎺嶇秼閸╃儤婀版穱鈩冧紖閿涘奔绲鹃張顏呭絹閸?`AVChapter` 閺佺増宓侀妴?


- 鏉堟挸鍙嗛柧鎹愮熅缂傚搫鐨粩鐘哄Ν閸斻劋缍旂拠閿嬬湴閿涘潉Display -> Renderer -> PlayerCore`閿涘鈧?


- 娑撶粯绁︾粙瀣繁鐏忔垵褰查柌宥咁槻閹笛嗩攽閻ㄥ嫮鐝烽懞鍌氼嚤閼割亪鐛欓弨璺烘嚒娴犮們鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`Demuxer` 娑擃叀袙閺嬫劗鐝烽懞鍌氬帗閺佺増宓侀敍灞炬煀婢?`ChapterInfo` 娑?`MediaInfo::chapters`閵?


- 閺傛澘顤冪粩鐘哄Ν鐎佃壈鍩呴崝銊ょ稊娑撳氦顕Ч鍌炴懠鐠侯垽绱?


  - `HotkeyManager` 婢х偛濮?`PreviousChapter/NextChapter`閿?


  - 姒涙顓婚柨顔荤秴缂佹垵鐣?`HOME/END`閿?


  - `Display`閵嗕焦瑕嗛弻鎾虫珤閹恒儱褰涢妴涔layerCore`閵嗕梗VideoPlayer` 閸忋劑鎽肩捄顖炩偓蹇庣炊缁旂姾濡拠閿嬬湴閵?


- 閸?`PlayerCore` 娑擃厽鏌婃晶鐐电彿閼哄倽鐑︽潪顒冨厴閸旀冻绱?


  - 閹垫挸绱戞刊鎺嶇秼閺冭埖鐎铏圭彿閼哄倹妞傞梻瀵稿仯閿?


  - `seekToNextChapter()` / `seekToPreviousChapter()` 閹笛嗩攽鐠哄磭鐝烽妴?


- 閸?`main` 閺傛澘顤?`--chapter-nav-check <media_file>` 閼奉亝顥呴崨鎴掓姢閿涘苯鑻熼弴瀛樻煀鐢喖濮潏鎾冲毉閵?


- 閺傛澘顤冮張顒€婀撮幎銉ユ啞 `docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`閿涘矁顔囪ぐ鏇犵彿閼哄倹鐗遍張顑跨瑢 PASS 缂佹挻鐏夐妴?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`4.1` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/demuxer.h



- src/demuxer.cpp



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/CHAPTER_NAV_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?41: M4 4.2閿涘湏-B Repeat閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `4.2` 闂団偓鐟曚焦鏁幐?A-B Repeat閵?


- 瑜版挸澧犻幘顓熸杹閸ｃ劎宸辩亸?A/B/C 韫囶偅宓庨柨顔煎З娴ｆ粈绗屽顏嗗箚閸栨椽妫块幒褍鍩楅柅鏄忕帆閿涘本妫ゅ▔鏇炴躬閹绢厽鏂佹稉顓＄箻鐞涘苯灏梻鎾櫢婢跺秲鈧?






### 閸樼喎娲滈崚鍡樼€?


- 鏉堟挸鍙嗛柧鎹愮熅閺堫亜鐣炬稊?A-B Repeat 鐠囬攱鐪伴崝銊ょ稊閵?


- `PlayerCore` 缂傚搫鐨?A 閻?B 閻愬湱濮搁幀浣侯吀閻炲棔绗屽顏嗗箚鐟欙箑褰傞柅鏄忕帆閵?


- 缂傚搫鐨崣顖炲櫢婢跺秵澧界悰宀€娈?A-B Repeat 妤犲本鏁归崨鎴掓姢閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫嶉悜顓㈡暛閸斻劋缍旈敍?


  - 閺傛澘顤?`SetABRepeatStart` / `SetABRepeatEnd` / `ClearABRepeat`閿?


  - 姒涙顓婚柨顔荤秴缂佹垵鐣?`A/B/C`閵?


- 閹碘晛鐫嶇拠閿嬬湴闁炬崘鐭鹃敍?


  - `Display` 婢х偛濮?A-B Repeat 鐠囬攱鐪伴弽鍥唶娑撳孩绉风拹瑙勫复閸欙綇绱?


  - `IVideoRenderer` 娑撳骸鎮囧〒鍙夌厠閸ｃ劌鐤勯悳鏉款杻閸旂娀鈧繋绱堕幒銉ュ經閵?


- `PlayerCore` 閺傛澘顤?A-B Repeat 閻樿埖鈧椒绗岄幒褍鍩楅敍?


  - `setABRepeatStart()` 鐠佸墽鐤?A 閻愮懓鑻熷〒鍛敄閺?B 閻愮櫢绱?


  - `setABRepeatEnd()` 鐠佸墽鐤?B 閻愮懓鑻熼崥顖滄暏瀵邦亞骞嗛敍?


  - `clearABRepeat()` 濞撳懘娅庡顏嗗箚閿?


  - `handleABRepeatLoop()` 閸︺劍鎸遍弨鍙ヨ厬濡偓濞村鍩屾潏?B 閻愮懓鎮楅懛顏勫З seek 閸?A 閻愬箍鈧?


- `VideoPlayer` 閺嗘挳婀?A-B Repeat API 娓氭稐瀵屽ù浣衡柤娑撳酣鐛欓弨璺烘嚒娴犮倛鐨熼悽銊ｂ偓?


- 閺傛澘顤?`--ab-repeat-check <media_file>` 閸涙垝鎶ら敍宀冪翻閸?`ab-repeat-check.*` 鐎涙顔岄崪?`PASS/FAIL`閵?


- 娣囶喖顦查崶鐐茬秺濡偓閺屻儱鍟跨粣渚婄窗



  - `--settings-persistence-check` 閻ㄥ嫭绁寸拠鏇㈡暛娴ｅ秶鏁?`b` 鐠嬪啯鏆ｆ稉?`x`閿涘矂浼╅崗宥勭瑢閺備即绮拋銈囧劰闁款喖鍟跨粣浣碘偓?


- 閺傛澘顤冮張顒€婀撮幎銉ユ啞 `docs/reports/AB_REPEAT_LOCAL_CHECK.md`閵?


- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`4.2` 瀹告彃鐣幋鎰┾偓?






### 娣囶喗鏁奸弬鍥︽



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/AB_REPEAT_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?42: M4 4.3閿涘牊鍩呴崶鎾呯礆







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `4.3` 闂団偓鐟曚焦鏁幐浣瑰焻閸ヤ勘鈧?


- 瑜版挸澧犵€圭偟骞囬搹鐣屽姧瀹歌尙绮￠幒銉ュ弳閹搭亜娴橀悜顓㈡暛閸?`--screenshot-check`閿涘奔绲鹃弳鍌氫粻閹焦鐥呴張澶岀处鐎涙ɑ娓舵潻鎴滅鐢嶇礉閹搭亜娴樼拠閿嬬湴閺冪姵纭剁粙鍐茬暰娣囨繂鐡ㄨぐ鎾冲閻㈠娼伴妴?






### 閸樼喎娲滈崚鍡樼€?


- 閹搭亜娴橀拃鐣屾磸闁槒绶崣顏嗙拨鐎规艾婀〒鍙夌厠缁捐法鈻奸惃鍕煀鐢冾槱閻炲棜鐭惧鍕瑐閵?


- 閹绢厽鏂侀弳鍌氫粻閸氬函绱濈拫鍐ㄥ閸ｃ劋绗夐崘宥囨埛缂侇參鈧礁鎶氶敍灞筋嚤閼峰瓨娈忛崑婊勨偓浣瑰焻閸ョ偓鐥呴張澶婂讲濞戝牐鍨傞惃鍕禈閸嶅繑鏆熼幑顔界爱閵?






### 鐟欙絽鍠呴弬瑙勵攳



- `PlayerCore` 閺傛澘顤冮張鈧潻鎴炶閺屾挸鎶氱紓鎾崇摠閿涘苯鑻熼弨顖涘瘮娴犲海绱︾€涙ê鎶氶惄瀛樺复閽€鐣屾磸閹搭亜娴橀妴?


- `requestScreenshot()` 鐠嬪啯鏆ｆ稉鐚寸窗閹绢厽鏂佹稉顓炵磽濮濄儲甯撻梼鐕傜礉閺嗗倸浠犻幀浣烘纯閹恒儰濞囬悽銊х处鐎涙ê鎶氭穱婵嗙摠閵?


- `--screenshot-check` 閸楀洨楠囨稉鐑樻畯閸嬫粍鈧焦鍩呴崶楣冪崣閺€璁圭礉鐟曞棛娲婃潻娆愵偧娣囶喖顦查惃鍕壋韫囧啫婧€閺咁垬鈧?


- 閺囧瓨鏌婅箛顐ｅ祹闁款喗鏋冨锝忕礉鐞涖儱鍘?`S` 閹搭亜娴橀妴浣虹彿閼哄倸顕遍懜顏傗偓涓?B Repeat閵嗕礁鐡ч獮鏇炵磻閸忓磭鐡戦悳鐗堟箒閼宠棄濮忕拠瀛樻閵?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- README.md



- README_ZH.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/SCREENSHOT_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?43: `MPC_HC_GAP_ANALYSIS` 鐠囧嫪鍙婄紒鎾诡啈鏉╁洦婀?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 娴犲秳绻氶悾娆愭＋缂佹捁顔戦敍灞惧Ω婢舵岸銆嶅鍙夊复閸忋儰瀵屽ù浣衡柤閻ㄥ嫯鍏橀崝娑樺晸閹存劏鈧粓顎囬弸?閺堫亝甯撮崗銉⑩偓婵勨偓?


- 鏉╂瑤绱拌ぐ鍗炴惙閸氬海鐢绘潻顓濆敩娴兼ê鍘涚痪褍鍨介弬顓ㄧ礉娑旂喍绱扮拋鈺傛瀮濡楋綀顕伴懓鍛嚠瑜版挸澧犵€圭偟骞囨潻娑樺瑜般垺鍨氶柨娆掝嚖鐠併倗鐓￠妴?






### 閸樼喎娲滈崚鍡樼€?


- 闁插瞼鈻肩喊鎴炴瀮濡楋絻鈧礁绱戦崣鎴炴）韫囨鎷伴張顒€婀撮幎銉ユ啞閹镐胶鐢婚弴瀛樻煀閿涘奔绲惧顔跨獩鐠囧嫪鍙婇弬鍥ㄣ€傚▽鈩冩箒閸氬本顒炵紒瀛樺Б閵?


- 閺冄嗙槑娴奸瀵岀憰浣风贩閹诡喒鈧粈鍞惍渚€顎囬弸鑸垫Ц閸氾箑鐡ㄩ崷銊⑩偓婵撶礉濞屸剝婀侀崥鍛婃暪閸氬海鐢?`docs/reports/*` 妤犲本鏁圭紒鎾寸亯閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐏?`docs/analysis/MPC_HC_GAP_ANALYSIS.md` 閺囧瓨鏌婇崚?2026-03-08 閸欙絽绶為妴?


- 闁插秴鍟撻垾婊冪秼閸撳秴鍑￠崗宄邦槵閼宠棄濮忛垾婵冣偓婊冩▕鐠烘繃鈧槒顫嶉垾婵冣偓婊冨彠闁款喗婀€圭偟骞囬崝鐔诲厴濞撳懎宕熼垾婵冣偓婊€鍞惍浣哥湴鐠囦焦宓侀幗妯款洣閳ユ績鈧粌缂撶拋顕€鍣风粙瀣暥閳ユ縿鈧?


- 閺傛澘顤冮垾婊堢崣閺€鏈电瑢閹躲儱鎲＄拠浣瑰祦閳ユ繄鐝烽懞鍌︾礉閹跺﹤鐡ч獮鏇樷偓浣规尡閺€鎯у灙鐞涖劊鈧浇顔曠純顔衡偓浣告彥閹圭兘鏁妴涓?D11/閸ョ偤鈧偓閵嗕胶鐝烽懞鍌氼嚤閼割亗鈧竸-B Repeat閵嗕焦鍩呴崶淇扁偓浣圭壐瀵繒鐓╅梼鐢垫畱閺堫剙婀撮幎銉ユ啞缁惧啿鍙嗙拠鍕強娓氭繃宓侀妴?


- 閺囧瓨鏌?`docs/README.md`閿涘矁藟閸忓懏婀板▎鈩冩瀮濡楋絽顕鎰嚛閺勫簺鈧?






### 娣囶喗鏁奸弬鍥︽



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?44: `docs/records/VERSION.md` 閸樺棗褰剁捄顖氱窞閹诲繗鍫潻鍥ㄦ埂







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `docs/records/VERSION.md` 閻ㄥ嫧鈧粓妯佸▓鍏哥閳ユ繂宸婚崣鑼彿閼哄倷绮涢幎濠冩－閺?decoder/thread/test 閺傚洣娆㈤崥宥呭晸閹存劕缍嬮崜宥勭波鎼存挾绮ㄩ弸鍕嚛閺勫簺鈧?


- 閸︺劌鐔€娴滃孩鏋冨锝変憾閸樺棔绮ㄦ惔鎾存閿涘苯顔愰弰鎾村Ω瀹歌尙些闂勩倗娈戦弮褍鐤勯悳棰佺瑢瑜版挸澧?`PlayerCore + Scheduler + core/*` 娑撳鎽煎ǎ閿嬬┋閵?






### 閸樼喎娲滈崚鍡樼€?


- `2026-03-06` 娑斿鎮楅弸鑸电€鑼病閺€鑸垫殐閿涘奔绲鹃悧鍫熸拱閺傚洦銆傛穱婵堟殌娴滃棙妫張鐔告瀮娴犲墎楠囬幓蹇氬牚閵?


- 閸氬海鐢婚崝鐔诲厴閹镐胶鐢绘潻钘夊閺傛媽顔囪ぐ鏇礉閺堫亜娲栨径瀛樼閻炲棌鈧粌缍嬮崜宥夋▉濞堢鈧績鈧粈绗呮稉鈧銉吀閸掓巻鈧繆绻栫猾璇插嚒鏉╁洦妞傞崣锝呯窞閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐏忓棝妯佸▓鍏哥閺嶅洭顣介弨閫涜礋閳ユ粌宸婚崣鑼舵崳閻愬厜鈧繐绱濋獮鎯八夐崗鍛扮箹閺勵垱妫張鐔风杽閻滄澘鐔€缁捐法娈戠拠瀛樻閵?


- 鐏?`video_decoder` / `audio_decoder`閵嗕梗VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 缁涘妫捄顖氱窞閺€鐟板晸娑撻缚鍏橀崝娑氶獓閸樺棗褰剁拋鏉跨秿閿涘苯鑻熼弰鐘茬殸閸掓澘缍嬮崜?`core/*` 娑撳鎽奸妴?


- 鐏忓棌鈧粈绗呮稉鈧銉吀閸掓巻鈧縿鈧梗USE_NEW_PLAYER_CORE`閵嗕椒澶嶉弮?`tests/core_*` 缁涘銆冩潻鐗堟暭閸愭瑤璐熼崢鍡楀蕉鐠囧瓨妲戦敍宀勪缉閸忓秷顕ょ€电厧缍嬮崜宥堢箻鎼达箑鍨介弬顓溾偓?


- 閸氬本顒炵悰銉ュ晸閻楀牊婀伴弬鍥ㄣ€傞惃鍕纯閺傜増妫╄箛妤佹蒋閻╊喓鈧?






### 娣囶喗鏁奸弬鍥︽



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?45: README 娑撳孩鐏﹂弸鍕瀮濡楋絼绮涘ǎ椋庢暏閺冄傚瘜闁炬崘銆冩潻?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `README.md`閵嗕梗README_ZH.md` 閻ㄥ嫰銆嶉惄顔剧波閺嬪嫬鎷伴弸鑸电€粈鐑樺壈娴犲秴鐫嶇粈?`video_decoder` / `audio_decoder` 缁涘妫捄顖氱窞閵?


- `docs/design/ARCHITECTURE.md` 娑旂喎鎯堥張澶嗏偓婊冪秼閸撳秴鐤勯悳鎵斥偓婵嗙摟閺嶅嘲鎷伴弮褎膩閸ф鎳￠崥宥忕礉鐎硅妲楁稉搴ｅ箛鐞?`PlayerCore + Scheduler + core/*` 娑撳鎽煎ǎ閿嬬┋閵?






### 閸樼喎娲滈崚鍡樼€?


- 閺嶅湱娲拌ぐ?README 閻ㄥ嫮娲拌ぐ鏇熺埐閸滃本鐏﹂弸鍕禈閺夈儴鍤滈弮鈺傛埂閸楁洑缍?閺冄冾樋缁捐法鈻肩€圭偟骞囬梼鑸殿唽閿涘苯鎮楃紒顓熸弓闂呭繘鍣搁弸鍕倱濮濄儱鍩涢弬鑸偓?


- `docs/design/ARCHITECTURE.md` 娣囨繄鏆€娴滃棗銇囬柌蹇撳坊閸欒尪顔曠拋鈥冲敶鐎圭櫢绱濇担鍡欏繁鐏忔垶绔婚弲鎵畱閳ユ粌宸婚崣?瑜版挸澧犻垾婵婄珶閻ｅ本褰佺粈鎭掆偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺囧瓨鏌?`README.md` 娑?`README_ZH.md` 閻ㄥ嫰銆嶉惄顔剧波閺嬪嫨鈧焦鐏﹂弸鍕仛閹板繐鎷伴弬鍥ㄣ€傞柧鐐复閿涘瞼绮烘稉鈧幐鍥ф倻瑜版挸澧犳稉濠氭懠閵?


- 閸?`docs/design/ARCHITECTURE.md` 妞ゅ爼鍎存晶鐐插閻樿埖鈧浇顕╅弰搴礉楠炶泛鐨㈤弮褎膩閸ф鐝烽懞鍌涙▔瀵繑鐖ｇ拋棰佽礋閳ユ粌宸婚崣鎻掔杽閻滄壋鈧縿鈧?


- 鐏忓棙妫╄箛妤冦仛娓氬绮?`spdlog` 閺€閫涜礋瑜版挸澧犳い鍦窗娴ｈ法鏁ら惃?`Quill` 鐎瑰繑甯撮崣锝冣偓?


- 閺囧瓨鏌?`docs/README.md`閿涘苯灏崚鍡楀坊閸欏弶鐏﹂弸鍕唨缁惧じ绗岃ぐ鎾冲闁插秵鐎拠瀛樻閵?






### 娣囶喗鏁奸弬鍥︽



- README.md



- README_ZH.md



- docs/design/ARCHITECTURE.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?46: 鐎圭偟骞囬弫娆戔柤娑撳氦鍑禒锝堫吀閸掓帞宸辩亸鎴濆坊閸?瑜版挸澧犳潏鍦櫕鐠囧瓨妲?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `docs/guides/IMPLEMENTATION.md` 娴犲秳浜掗弮鈺傛埂 `video_decoder/audio_decoder/playLoop` 閸樼喎鐎风捄顖氱窞鐠佽尪袙鐎圭偟骞囧銉╊€冮敍灞筋啇閺勬捁顫︾拠顖濐嚢娑撳搫缍嬮崜宥勭波鎼存挾娈戦柅鎰瀮娴犺泛绱戦崣鎴炲瘹閸楁ぜ鈧?


- `docs/plans/MPC_HC_ITERATION_PLAN.md` 閺?`2026-03-07` 閻ㄥ嫯顫夐崚鎺戞彥閻撗嶇礉娴ｅ棙婀弰搴ｂ€樼拠瀛樻闁劌鍨庣拋鈥冲灊妞ょ懓鍑￠崷?`2026-03-08` 閸撳秴鎮楅拃钘夋勾閵?






### 閸樼喎娲滈崚鍡樼€?


- 鏉╂瑤琚辨禒鑺ユ瀮濡楋絾婀伴煬顐＄矝閺堝寮懓鍐х幆閸婄》绱濇担鍡欏繁鐏忔垟鈧粌宸婚崣鍙夋殌缁?/ 鐠佲€冲灊韫囶偆鍙?/ 瑜版挸澧犳潻娑樺閳ユ繀绠ｉ梻瀵告畱鏉堝湱鏅拠瀛樻閵?


- 瑜版挾鏁ら幋閿嬪瘻閺傚洦銆傞柆宥呭坊娴犳挸绨遍弮璁圭礉娴兼碍濡搁弫娆戔柤缁€杞扮伐閸滃矁顫夐崚鎺撴瀮鐎涙顕よぐ鎾茬稊閻滄媽顢戠紒鎾寸€稉搴＄秼閸撳秴绶熼崝鐐偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑?`docs/guides/IMPLEMENTATION.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋弰搴ｂ€橀崗璺虹潣娴滃孩妫張鐔峰斧閸ㄥ鏆€缁嬪绱濋獮鑸靛瘹閸氭垵缍嬮崜宥勫瘜闁炬儳寮懓鍐╂瀮濡楋絻鈧?


- 娑?`docs/plans/MPC_HC_ITERATION_PLAN.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋弰搴ｂ€橀崗璺虹潣娴?`2026-03-07` 閻ㄥ嫯顓搁崚鎺戞彥閻撗嶇礉楠炴儼藟閸忓懎缍嬮崜宥堢箻鎼达箑寮懓鍐ㄥ弳閸欙絻鈧?


- 閺囧瓨鏌?`docs/README.md`閵嗕梗README.md`閵嗕梗README_ZH.md` 閻ㄥ嫭鏋冨锝堫嚛閺勫函绱濈紒鐔剁閸栧搫鍨庨崢鍡楀蕉閺佹瑧鈻奸妴浣筋潐閸掓帒鎻╅悡褌绗岃ぐ鎾冲鐎圭偟骞囩拠瀛樻閵?






### 娣囶喗鏁奸弬鍥︽



- README.md



- README_ZH.md



- docs/guides/IMPLEMENTATION.md



- docs/plans/MPC_HC_ITERATION_PLAN.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?47: 鏉堝懎濮拠瀛樻閺傚洦銆傛禒宥囧繁鐏忔垵缍嬮崜宥呭弳閸欙絼绗岄悩鑸碘偓浣界珶閻?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `docs/design/FILTERS.md`閵嗕梗docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`閵嗕梗docs/guides/WINDOWS_SETUP.md` 閾忕晫鍔ч崘鍛啇娴犲秵婀侀崣鍌濃偓鍐х幆閸婄》绱濇担鍡欏繁鐏忔垵顕垾婊冪秼閸撳秳瀵屽ù浣衡柤閸忋儱褰涢垾婵嗘嫲閳ユ粍鏋冨锝夆偓鍌滄暏閼煎啫娲块垾婵堟畱閺勫海鈥樼拠瀛樻閵?


- 閸忔湹鑵?`docs/guides/WINDOWS_SETUP.md` 鏉╂ü绻氶悾娆庣啊娑撳骸缍嬮崜?`CMakeLists.txt` 娑撳秴鐣崗銊ょ閼峰娈戦幍瀣З闁板秶鐤嗙粈杞扮伐閿涘苯顔愰弰鎾诡嚖鐎?Windows 閺嬪嫬缂撶捄顖氱窞閵?






### 閸樼喎娲滈崚鍡樼€?


- 鏉╂瑥鍤戞禒鑺ユ瀮濡楋絽鍨庨崚顐ｆ箛閸斺€茬艾濠娿倝鏆呴妴浣藉厴閸旀稑寮懓鍐︹偓涔刬ndows 閻滎垰顣ㄩ柊宥囩枂閿涘矂鏆遍張鐔诲嚡娴狅絽鎮楅崘鍛啇濞屸剝婀佺紒鐔剁鐞涖儵缍堥垾婊冪秼閸撳秴褰涘鍕ㄢ偓婵婎嚛閺勫簺鈧?


- 閺傚洦銆傜拠鏄忊偓鍛洤閺嬫粎娲块幒銉﹀瘻閺冄勫伎鏉╃増澧界悰宀嬬礉閸欘垵鍏樻导姘Ω閸欏倽鈧啯鈧冨敶鐎圭顕よぐ鎾村灇瑜版挸澧犻崬顖欑閸忋儱褰涢敍灞惧灗濞岃法鏁ゆ潻鍥ㄦ閻ㄥ嫬寮弫棰佺炊闁帗鏌熷蹇嬧偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑?`docs/design/FILTERS.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋崠鍝勫瀻瑜版挸澧犻悽鐔告櫏閻?`FilterPipeline` 娑撳鎽兼稉搴暕閻ｆ瑩鎽煎蹇撶殱鐟佸懌鈧?


- 娑?`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 婢х偛濮為柅鍌滄暏閼煎啫娲跨拠瀛樻閿涘苯鑻熺亸鍡樷偓鏄忕箻鎼达箑寮懓鍐ㄥ弳閸欙絾瀵氶崥鎴濇▕鐠烘繆鐦庢导鑸偓浣哄閺堫剝顔囪ぐ鏇炴嫲閸欐ɑ娲块弮銉ョ箶閵?


- 娑?`docs/guides/WINDOWS_SETUP.md` 婢х偛濮炶ぐ鎾冲娓氭繆绂嗛幒銏＄ゴ妞ゅ搫绨拠瀛樻閿涘瞼些闂勩倛顕ょ€靛吋鈧呮畱 `SDL2_DIR` / `FFMPEG_DIR` 娴肩姴寮粈杞扮伐閿涘本鏁兼稉杞扮瑢閻滅増婀?`CMakeLists.txt` 娑撯偓閼峰娈戞禒鎾崇氨閸?`external/*` 閸ョ偤鈧偓閸欙絽绶為妴?


- 閺囧瓨鏌?`docs/README.md` 缁便垹绱╂稉搴㈡拱鏉烆喗鏋冨锝嗘纯閺傛媽顔囪ぐ鏇樷偓?






### 娣囶喗鏁奸弬鍥︽



- docs/design/FILTERS.md



- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md



- docs/guides/WINDOWS_SETUP.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?48: 閺?README 閺佸懘娈伴幒鎺楁珟娑撳骸宸婚崣鏌ユ６妫版ê缍婂锝勭矝閺堝妫崣锝呯窞







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 閺嶅湱娲拌ぐ?`README.md` / `README_ZH.md` 閻?Windows 閺佸懘娈伴幒鎺楁珟娴犲秴缂撶拋顔诲▏閻?`FFMPEG_DIR` 娴肩姴寮敍灞芥嫲瑜版挸澧?`CMakeLists.txt` 閻ㄥ嫪绶风挧鏍ㄥ赴濞村鏌熷蹇庣瑝娑撯偓閼锋番鈧?


- `docs/analysis/video-stream-index-fix.md` 閺勵垱妫張鐔峰斧閸ㄥ妯佸▓鐢垫畱闂傤噣顣介崚鍡樼€介敍灞肩稻缂傚搫鐨垾婊冨坊閸欐彃缍婂锝傗偓婵婎嚛閺勫函绱濈€硅妲楃拋鈺勵嚢閼板懓顕ゆ禒銉よ礋閸忔湹鑵戦惃?`playLoop` / `video_decoder.cpp` 娴犲秴鐫樿ぐ鎾冲娑撳鎽奸妴?






### 閸樼喎娲滈崚鍡樼€?


- README 閻ㄥ嫭鏅犻梾婊勫笓闂勩倖顔岄拃鎴掔箽閻ｆ瑤绨￠弮褎澧滈崝銊ょ贩鐠ф牜鏁ゅ▔鏇礉濞屸剝婀佺捄鐔兼 Windows 閺嬪嫬缂撻崗銉ュ經娑撯偓鐠ч攱娲块弬鑸偓?


- 閸樺棗褰堕梻顕€顣介崚鍡樼€介弬鍥ㄣ€傞崘鍛啇閺堫剝闊╅張澶夌幆閸婄》绱濇担鍡涙付鐟曚焦妯夊蹇氼嚛閺勫骸鍙鹃柅鍌滄暏闂冭埖顔岄妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 鐏?README 娑?FFmpeg 閺佸懘娈伴幒鎺楁珟閺€閫涜礋瑜版挸澧犻幒銊ㄥ礃閸欙絽绶為敍姘喘閸忓牐顕╅弰?`vcpkg` toolchain閿涘本澧滈崝銊ョ暔鐟佸懎鍨担璺ㄦ暏娴犳挸绨遍崘?`external/ffmpeg/` 閸ョ偤鈧偓鐢啫鐪妴?


- 娑?`docs/analysis/video-stream-index-fix.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋弽鍥唶娑撶儤妫張鐔峰斧閸ㄥ妫舵０妯哄瀻閺嬫劕缍婂锝冣偓?


- 閸?`docs/README.md` 娑擃叀藟閸忓懓顕氶崢鍡楀蕉閸掑棙鐎介弬鍥ㄣ€傞惃鍕偍瀵洑绗岄張顒冪枂閺囧瓨鏌婄拋鏉跨秿閵?






### 娣囶喗鏁奸弬鍥︽



- README.md



- README_ZH.md



- docs/analysis/video-stream-index-fix.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?49: 缂傚搫鐨悪顒傜彌閻ㄥ嫭鏋冨锝呰窗濡偓閹槒銆?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 閸撳秴鍤戞潪顔芥瀮濡楋絾鏆ｉ悶鍡楀嚒缂佸繐鐣幋鎰剁礉娴ｅ棗璐板Λ鈧紒鎾寸亯閸掑棙鏆庨崷銊ヮ嚠鐠囨縿鈧焦妫╄箛妤€鎷版径姘嚋閹绘劒姘︽稉顓ㄧ礉缂傚搫鐨稉鈧禒鐣屽缁斿娈戦幀鏄忋€冮弬鍥ㄣ€傞妴?


- 閸氬海鐢荤紒瀛樺Б閼板懎顩ч弸婊勫厒韫囶偊鈧喍绨＄憴锝傗偓婊冩憿娴滄稒鏋冨锝呭嚒閺€鑸垫殐閵嗕礁鎽㈡禍娑樺敶鐎圭懓鐫樻禍搴″坊閸欒弓绻氶悾娆嶁偓浣疯礋娴犫偓娑斿牐顩︽穱婵堟殌閳ユ繐绱濋棁鈧憰浣规降閸ョ偟鐐曢梼鍛樋娑擃亝鏋冩禒韬测偓?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀?`CHANGELOG.md` / `DEVELOP_LOG.md` 闁倸鎮庣拋鏉跨秿鏉╁洨鈻奸敍灞肩稻娑撳秹鈧倸鎮庢担婊€璐熼棃銏犳倻閸氬海鐢荤紒瀛樺Б閻ㄥ嫭鎲崇憰浣瑰Г閸涘鈧?


- `docs/README.md` 閾忕晫鍔ч弰顖滃偍瀵洖鍙嗛崣锝忕礉娴ｅ棙鐥呴張澶屽缁斿澹欐潪鑺ユ拱鏉烆喖璐板Λ鈧紒鎾诡啈閻ㄥ嫪绗撴０妯绘瀮濡楋絻鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`docs/analysis/DOC_AUDIT_2026-03-08.md`閿涘矂娉︽稉顓炵秺濡楋絾婀版潪顔芥瀮濡楋絽璐板Λ鈧惃鍕瘱閸ユ番鈧焦鏌熷▔鏇樷偓浣稿嚒鐎瑰本鍨氱€靛綊缍堟い骞库偓浣风箽閻ｆ瑥宸婚崣鎻掑敶鐎瑰箍鈧礁鎮楃紒顓犳樊閹躲倕缂撶拋顔荤瑢閸忓疇浠堥幓鎰唉閵?


- 閺囧瓨鏌?`docs/README.md`閿涘本濡哥拠銉﹀Г閸涘﹤濮為崗銉у偍瀵洩绱濋獮鎯八夋稉鈧弶鈩冩拱鏉烆喗娲块弬鎷岊唶瑜版洏鈧?






### 娣囶喗鏁奸弬鍥︽



- docs/analysis/DOC_AUDIT_2026-03-08.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md











---







## 闂傤噣顣?50: M4 4.4閿涙碍娈忛崑婊勨偓浣告姎濮濄儴绻橀幒銉ュ弳娑撳酣鐛欓弨?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `4.4` 鐟曚焦鐪伴弨顖涘瘮閺嗗倸浠犻幀浣告姎濮濄儴绻橀妴?


- 瑜版挸澧犻幘顓熸杹閸ｃ劏娅ч悞璺哄嚒閺堝娈忛崑婧库偓浣瑰焻閸ヤ勘鈧胶鐝烽懞鍌氼嚤閼割亜鎷?A-B Repeat閿涘奔绲剧紓鍝勭毌閸欘垳娲块幒銉┾偓鎰姎濡偓閺屻儳鏁鹃棃銏㈡畱娴溿倓绨伴崗銉ュ經閵?






### 閸樼喎娲滈崚鍡樼€?


- 鏉堟挸鍙嗙仦鍌涚梾閺堝宕熼悪顒傛畱鐢勵劄鏉╂稑濮╂担婊愮礉娑旂喐鐥呴張澶婎嚠鎼存梻娈戞妯款吇闁款喕缍呴妴?


- `PlayerCore` 閻ㄥ嫭娈忛崑婊勨偓浣稿涧娴兼艾鍠曠紒鎾圭殶鎼达箑娅掗敍宀€宸辩亸鎴斺偓娓焑ek 閸氬骸鍩涢弬鎵窗閺嶅洤鎶氶垾婵堟畱閸楁洖鎶氬〒鍙夌厠鐠侯垰绶為妴?


- 闂婃娊顣跺☉鍫ｅ瀭缁捐法鈻奸崷銊︽畯閸嬫粍鈧椒绮涙导姘贩閹诡喗妫?`playback_pts` 閸ョ偛鍟撴担宥囩枂閿涘苯顕遍懛鏉戝礋鐢勵劄鏉╂稑鎮楅惃鍕闂傚鍋ｉ崣顖濆厴鐞氼偊鐓舵０鎴炴闁界喕顩惄鏍モ偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 娑撹櫣鍎归柨顔鹃兇缂佺喐鏌婃晶?`step_frame_backward` / `step_frame_forward` 閸斻劋缍旈敍宀勭帛鐠併倗绮︾€?`,` / `.`閵?


- 閸?`Display -> Renderer -> PlayerCore` 闁炬崘鐭鹃弬鏉款杻鐢勵劄鏉╂稖顕Ч鍌炩偓姘朵壕閵?


- `PlayerCore` 閺傛澘顤冮弳鍌氫粻閹礁鎶氬銉ㄧ箻閼宠棄濮忛敍?


  - 娴兼壆鐣婚崡鏇炴姎濮濄儵鏆遍敍?


  - 闁俺绻?seek 閸掗攱鏌婇棅瀹狀潒妫版垹濮搁幀渚婄幢



  - 娑撹濮╁〒鍙夌厠閻╊喗鐖ｉ弮鍫曟？閻愬湱娈戞＃鏍ф姎楠炴湹绻氶幐浣规畯閸嬫嚎鈧?


- 閺€鍓佹彛闂婃娊顣跺☉鍫ｅ瀭缁捐法鈻奸惃鍕秴缂冾喖娲栭崘娆愭蒋娴犺绱濋柆鍨帳閺嗗倸浠犻幀浣筋洬閻╂牗顒炴潻娑氱波閺嬫嚎鈧?


- 閸?`main` 閺傛澘顤?`--frame-step-check <media_file>` 妤犲本鏁归崨鎴掓姢閿涘苯鑻熼崥灞绢劄 README / 閻楀牊婀伴弬鍥ㄣ€?/ 瀹割喛绐涚拠鍕強 / 娴犺濮熷〒鍛礋閵?






### 娣囶喗鏁奸弬鍥︽



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- README.md



- README_ZH.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/FRAME_STEP_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## ?? 51: M4 4.5???/??????????







**??**: 2026-03-08







### ????



- ???? `4.5` ????????????????



- ??????????????????????????????????????????????????







### ????



- ?????????????????????? `J/K` ? `Ctrl+J/K` ?????????



- `PlayerCore` ????????????????????????????? PTS????????/??????????



- ?????????????????/???????????????????







### ????



- ??????????? `J / K` ??? `- / +100ms`??? `Ctrl` ?????? `- / +100ms`?



- ? `Display -> Renderer -> PlayerCore -> VideoPlayer` ??????/??????? API?



- ?????????????????????????? PTS ?????????????????????????



- ???????? `player.audio_delay_ms` / `player.subtitle_delay_ms`???? `--settings-persistence-check`?



- ?? `--delay-adjust-check <media_file> <subtitle.srt>` ????????????







### ????



- include/core/player_core.h



- include/video_player.h



- include/display.h



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- include/input/hotkey_manager.h



- src/core/player_core.cpp



- src/video_player.cpp



- src/display.cpp



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- src/input/hotkey_manager.cpp



- src/main.cpp



- config/player_settings.ini



- README.md



- README_ZH.md



- docs/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md



- docs/reports/DELAY_ADJUST_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## ?? 52: M4 4.6????? `1..9` ????







**??**: 2026-03-08







### ????



- `4.6` ???????????? `A/B/C/S,./J/K/1..9`?



- ? `4.5` ???????????????????????? `1..9` ?????????







### ????



- ??????? `1..9` ???????????????????????



- ?????????????????? seek ???????????????????



- ??????????? `1..9` ???????????????????







### ????



- ? `HotkeyManager` ??? `seek_to_10_percent` ~ `seek_to_90_percent` ?????????? `1..9`?



- `Display` ????????????? `seek_ratio_` ????? `PlayerCore::pumpEvents()` ???? seek ???



- ? `main` ?? `--numeric-seek-check <media_file>` ???????? README????????????????







### ????



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- src/display.cpp



- src/main.cpp



- README.md



- README_ZH.md



- docs/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂傤噣顣?53: M2 2.2.4閿涙俺绶崙鐑樻尡閺€鐐偓褑鍏橀弮銉ョ箶閿涘牊甯€鐢?闂冪喎鍨?CPU/GPU閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.2.4` 鐟曚焦鐪版潏鎾冲毉閹绢厽鏂侀幀褑鍏橀弮銉ョ箶閿涘瞼鏁ゆ禍搴ょ槑娴间即鐝崚鍡氶哺閻滃洢鈧線鐝惍浣哄芳閺嶉攱婀伴惃鍕竴鐢佲偓渚€妲﹂崚妞剧瑢鐠у嫭绨崡鐘垫暏鐞涖劎骞囬妴?


- 瑜版挸澧犳稉濠氭懠閾忕晫鍔ч崘鍛村劥瀹歌尙绮＄槐顖濐吀娴滃棜袙鐏忎浇顥婇妴浣叫掗惍浣碘偓浣硅閺屾挸鎷扮拫鍐ㄥ缂佺喕顓搁敍灞肩稻缂傚搫鐨稉鈧稉顏勫讲婢跺秶鏁ら妴浣稿讲鐎佃鐦妴浣稿讲閻╁瓨甯存灞炬暪閻ㄥ嫮绮烘稉鈧潏鎾冲毉閸忋儱褰涢妴?






### 閸樼喎娲滈崚鍡樼€?


- `PlayerCore` 閸愬懐娈戠拠濠冩焽鐠佲剝鏆熼崳銊ユ嫲 `Scheduler` 缂佺喕顓搁崚鍡樻殠閸︺劌鍞撮柈銊ョ杽閻滈鑵戦敍灞筋樆闁劏鐨熼悽銊︽煙閺冪姵纭舵稉鈧▎鈩冣偓褑骞忛崣鏍х暚閺佹潙鎻╅悡褋鈧?


- 閸涙垝鎶ょ悰宀冨殰濡偓閸忋儱褰涚亸姘弓鐟曞棛娲婇幀褑鍏樼憴鍌涚ゴ閸︾儤娅欓敍灞筋嚤閼?`1080p60 / 4K / 妤傛鐖滈悳鍢?閺嶉攱婀伴崣顏囧厴娓氭繆绂嗛梿鑸垫殠閺冦儱绻旈敍宀勬娴犮儱鑸伴幋鎰旂€规岸妫粋浣碘偓?


- GPU 閸掆晝鏁ら悳鍥硶楠炲啿褰撮惄瀛樺复闁插洦鐗遍幋鎰拱鏉堝啴鐝敍灞芥礈濮濄倖娲块柅鍌氭値閸忓牐绶崙鍝勭秼閸撳秵绺哄ú鑽ゆ畱鐟欙絿鐖?濞撳弶鐓?backend閿涘奔缍旀稉?GPU 鐠侯垰绶為弽鍥槕閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`PlayerCore` 娑擃厽鏌婃晶?`DiagnosticsSnapshot`閿涘瞼绮烘稉鈧€电厧鍤?demux閵嗕龚ecode閵嗕购ender閵嗕够cheduler 娑撳酣妲﹂崚妤佸瘹閺嶅洢鈧?


- 閸?`VideoPlayer` 娑擃參鈧繋绱?`getInfo()` / `getDiagnosticsSnapshot()`閿涘矂浼╅崗宥夌崣閺€鍫曗偓鏄忕帆閻╁瓨甯撮懓锕€鎮庨崘鍛村劥鐎圭偟骞囬妴?


- 閸?`main` 娑擃厽鏌婃晶?`--performance-log-check <media_file> [sample_ms]`閿?


  - 闁插洦鐗遍幘顓熸杹閺堢喖妫块惃鍕挬閸?CPU 閸楃姷鏁ら敍?


  - 鏉堟挸鍤?renderer / decoder backend閿?


  - 鏉堟挸鍤幒澶婃姎閵嗕線妲﹂崚妞尖偓浣叫掗惍浣告姎閺佽埇鈧焦瑕嗛弻鎾虫姎閺佹壆鐡戠紒鎾寸€崠鏍ㄥ瘹閺嶅洢鈧?


- 閸氬本顒炵悰銉╃秷娴犺濮熷〒鍛礋閵嗕線鐛欓弨鑸靛Г閸涘鈧礁妯婄捄婵婄槑娴艰埇鈧胶澧楅張顒冾唶瑜版洑绗屽鈧崣鎴炴）韫囨ぜ鈧?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- include/video_player.h



- src/core/player_core.cpp



- src/video_player.cpp



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂傤噣顣?54: M2 2.2.1 / 2.3.2閿?080p60 缁嬪啿鐣鹃幘顓熸杹妤犲本鏁?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.2.1` 娑?`2.3.2` 闂団偓鐟曚胶鈥樼拋?`1080p60` 閺嶉攱婀伴懗钘夘檮鏉╃偟鐢荤粙鍐茬暰閹绢厽鏂侀妴?


- 瑜版挸澧犳稉濠氭懠閾忕晫鍔у鍙夋箒閹嗗厴閺冦儱绻旈崗銉ュ經閿涘奔绲剧紓鍝勭毌娑撯偓娑擃亞娲块幒銉╂桨閸?`1080p60` 闂傘劎顩﹂惃鍕旂€规碍鈧囩崣閺€璺烘嚒娴犮倕鎷伴柊宥咁殰閺嶉攱婀伴崗銉ュ經閵?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀?`--performance-log-check` 閺囨潙浜搁崥鎴ｎ潎濞村瀵氶弽鍥ь嚤閸戠尨绱濇稉宥囨纯閹恒儱鍨介弬顓熸闂傚瓨甯规潻娑栤偓浣界箾缂侇厽鎸遍弨鍓х崶閸欙絼绗岄幒澶婃姎闂傘劎顩﹂弰顖氭儊鏉堢偓鐖ｉ妴?


- 娴犳挸绨遍悳鐗堟箒閺嶉攱婀伴梿鍡曡厬閸?`1080p30` 娑?`4K60`閿涘瞼宸辩亸鎴炴绾喚娈?`1080p60` 缁嬪啿鐣鹃幀褎鐗遍張顒傛晸閹存劕鍙嗛崣锝冣偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 娑擃厽鏌婃晶?`--1080p60-check <media_file> [sample_ms]`閿涘矁浠堥崥?`collectFileProbeReport()` 娑?`DiagnosticsSnapshot` 鏉堟挸鍤粙鍐茬暰閹囨，缁備胶绮ㄩ弸婧库偓?


- 妤犲本鏁归柅鏄忕帆闁插秶鍋ｅΛ鈧弻銉窗



  - 閺嶉攱婀伴弰顖氭儊娑?`1920x1080 @ 60fps`閿?


  - `5s` 鏉╃偟鐢婚幘顓熸杹缁愭褰涢崘鍛闂傚瓨妲搁崥锔厩旂€规碍甯规潻娑崇幢



  - `scheduler_late_drops` 娑?`demux_dropped_packets` 閺勵垰鎯佹稉?`0`閵?


- 閸?`tools/download_test_samples.ps1` 娑擃叀藟閸?`1080p60 AAC 2ch` 閺嶉攱婀伴悽鐔稿灇閿涘苯鑻熼崷?`samples/README.md` 娑擃叀顔囪ぐ鏇犳暏闁柣鈧?


- 閸氬本顒炵悰銉╃秷娴犺濮熷〒鍛礋閵嗕焦濮ら崨濞库偓浣告▕鐠烘繆鐦庢导鑸偓浣哄閺堫剚鏋冨锝勭瑢瀵偓閸欐垶妫╄箛妞尖偓?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- tools/download_test_samples.ps1



- samples/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/1080P60_STABILITY_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂傤噣顣?55: M2 2.2.2 / 2.3.3閿?K 閹绢厽鏂佹稉搴ㄦ缁狙囩崣閺€?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.2.2` 娑?`2.3.3` 鐟曚焦鐪扮涵顔款吇 `4K` 閺嶉攱婀伴崣顖欎簰閹绢厽鏂侀敍灞借嫙娑撴柨婀涵顒冃掓稉宥呭讲閻劍妞傞懗钘夘檮闂勫秶楠囬崚鎷岃拫鐟欙絿鎴风紒顓熸尡閺€淇扁偓?


- 瑜版挸澧犳禒鎾崇氨瀹稿弶婀侀幀褑鍏橀弮銉ョ箶閸忋儱褰涢崪?Windows 閸氬海顏崶鐐衡偓鈧弽锟犵崣閿涘奔绲剧紓鍝勭毌娑撯偓娑擃亞娲块幒銉╂桨閸?`4K` 闂傘劎顩﹂惃鍕粵閸氬牓鐛欓弨璺烘嚒娴犮們鈧?






### 閸樼喎娲滈崚鍡樼€?


- `--performance-log-check` 閸欘垯浜掔拠瀛樻 4K 閺嶉攱婀伴懗鍊熺箻閸忋儲鎸遍弨楣冩懠鐠侯垽绱濇担鍡曠瑝閻╁瓨甯寸憰鍡欐磰閳ユ粏钂嬬憴锝夋缁狙勬Ц閸氾附鍨氶崝鐔测偓婵勨偓?


- `--windows-backend-check` 閸欘垶鐛欑拠?hard / soft 娑撱倓閲滈崥搴ｎ伂濡€崇础閿涘奔绲炬稉宥嗘儭鐢?`4K` 鏉╃偟鐢婚幘顓熸杹缁愭褰涢崪灞炬闂傚瓨甯规潻娑㈡，缁備降鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 娑擃厽鏌婃晶?`--4k-playback-check <media_file> [sample_ms]`閿涘奔瀵屾潻娑氣柤妤犲矁鐦?`4K` 閺嶉攱婀伴惇鐔风杽閹恒劏绻樻稉?`late_drop`閿涘苯鐡欐潻娑氣柤婢跺秶鏁?hard / soft backend session 妤犲矁鐦夐崣顖炴缁狙佲偓?


- 鏉堟挸鍤?probe 鐎逛粙鐝?FPS閵嗕焦妞傞梻瀛樺腹鏉╂稒鐦悳鍥モ偓浣哥秼閸?backend閵嗕弓ard / soft 娴兼俺鐦界紒鎾寸亯缁涘绮ㄩ弸鍕鐎涙顔岄妴?


- 閸氬本顒炵悰銉╃秷娴犺濮熷〒鍛礋閵嗕焦濮ら崨濞库偓浣告▕鐠烘繆鐦庢导鑸偓浣哄閺堫剝顔囪ぐ鏇氱瑢瀵偓閸欐垶妫╄箛妞尖偓?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂傤噣顣?56: M2 2.2.3閿?80Mbps 妤傛鐖滈悳鍥ㄧ壉閺堫剟鐛欓弨?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `2.2.3` 鐟曚焦鐪伴懛鍐茬毌妤犲矁鐦夋稉鈧稉?`>80Mbps` 閺嶉攱婀伴懗钘夘檮閹绢厽鏂侀妴?


- 瑜版挸澧犳禒鎾崇氨閾忚棄鍑＄€瑰本鍨?`1080p60`閵嗕梗4K` 娑撳孩鈧嗗厴閺冦儱绻旈梻銊ь洣閿涘奔绲剧紓鍝勭毌閺勫海鈥橀惃鍕彯閻胶宸奸弽閿嬫拱閸滃奔绗撻悽銊╃崣閺€璺哄弳閸欙絻鈧?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀侀弽閿嬫拱閺咁噣浜堕崣顏呮箒 `3~4Mbps` 缁狙冨焼閿涘本妫ゅ▔鏇＄槈閺勫孩鎸遍弨鎯ф珤閸︺劑鐝惍浣哄芳閸︾儤娅欐稉瀣畱鐟欙絽鐨濈憗鍛偓浣叫掗惍浣风瑢濞撳弶鐓嬮柧鎹愮熅缁嬪啿鐣鹃幀褋鈧?


- 閻滅増婀佹灞炬暪閸涙垝鎶ら張顏勵嚠閳ユ粍鐗稿蹇曠垳閻滃洦妲搁崥锕佺Т鏉?80Mbps閳ユ繂浠涢崜宥囩枂閸掋倖鏌囬妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`--high-bitrate-check <media_file> [sample_ms]`閿涘苯鍘涚拠璇插絿閺嶇厧绱￠惍浣哄芳閿涘苯鍟€閹笛嗩攽鏉╃偟鐢婚幘顓熸杹缁愭褰涢弽锟犵崣閵?


- 閸?`tools/download_test_samples.ps1` 娑擃厽鏌婃晶?`stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 閻㈢喐鍨氶崗銉ュ經閿涘奔绻氱拠浣规拱閸︽澘褰叉径宥囧箛鐎圭偤鐛欓弽閿嬫拱閵?


- 閸氬本顒炵悰銉╃秷娴犺濮熷〒鍛礋閵嗕焦濮ら崨濞库偓浣告▕鐠烘繆鐦庢导鑸偓浣哄閺堫剝顔囪ぐ鏇氱瑢瀵偓閸欐垶妫╄箛妞尖偓?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- tools/download_test_samples.ps1



- samples/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/HIGH_BITRATE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂傤噣顣?57: 閸欐垵绔烽梻銊ь洣 6.5閿涙岸鏆遍弮鑸垫尡閺€鍓旂€规碍鈧囩崣閺€?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `6.5` 鐟曚焦鐪扮涵顔款吇閹绢厽鏂侀崳銊ユ躬閹镐胶鐢婚幘顓熸杹缁愭褰涢崘鍛￥ crash 娑撴棁鍏橀幐浣虹敾閹恒劏绻橀妴?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀?`1080p60`閵嗕梗4K`閵嗕線鐝惍浣哄芳娑撳孩鈧嗗厴閺冦儱绻旈梻銊ь洣鐟曞棛娲婃禍鍡欑叚缁愭褰涚粙鍐茬暰閹傜瑢閸欘垵顫囧ù瀣偓褝绱濇担鍡欏繁鐏忔垳绔存稉顏嗘纯閹恒儵娼伴崥鎴斺偓婊堟毐閺冭埖鎸遍弨鐐￥ crash閳ユ繄娈戦崶鍝勭暰 smoke 閸涙垝鎶ら妴?


- 閸欐垵绔烽梻銊ь洣 `6.1 ~ 6.6` 閻ㄥ嫭娓堕崥搴ｅ繁閸欙絾妲哥粙鍐茬暰閹嗙槈閹诡噯绱濈紓鍝勭毌閸楁洜瀚幎銉ユ啞鐏忚鲸妫ゅ▔鏇熸暪閸?DoD閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`main` 娑擃厽鏌婃晶?`--long-playback-check <media_file> [sample_ms]`閿涘矁顩﹀Ч鍌涙付閻參鍣伴弽椋庣崶閸?`5000ms`閿涘苯鑻熸潏鎾冲毉 `open_ok`閵嗕焦妲搁崥锕佺箻閸忋儲鎸遍弨鎯ф儕閻滎垬鈧胶鐛ラ崣锝囩波閺夌喎鎮楅弰顖氭儊娴犲秴婀幘顓熸杹閵嗕焦妞傞梻瀛樺腹鏉╂稒鐦悳鍥モ偓涔ate_drop`閵嗕龚emux 娑撱垹瀵樻稉?backend 娣団剝浼呴妴?


- 閺傛澘顤?`docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`閿涘矁顔囪ぐ?`./juren-30s.mp4` 娑?`10000ms` 鏉╃偟鐢婚幘顓熸杹 smoke 缂佹挻鐏夐敍灞借嫙閸氬本顒炴禒璇插濞撳懎宕熼妴浣告▕鐠烘繆鐦庢导鑸偓浣哄閺堫剝顔囪ぐ鏇氱瑢瀵偓閸欐垶妫╄箛妞尖偓?






### 娣囶喗鏁奸弬鍥︽



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂傤噣顣?58: 7.1 閹绘帊娆㈢化鑽ょ埠閿涘牆濮╅幀浣稿鏉炴垝绗岄悽鐔锋嚒閸涖劍婀￠梻顓犲箚閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `7.1` 闂団偓鐟曚焦濡搁悳鐗堟箒娴犲懏鏁幐浣稿敶鐎涙ɑ鏁為崘?閸氼垰浠犻悩鑸碘偓浣烘畱閹绘帊娆㈡銊︾仸閿涘矁藟閹存劕褰茬€圭偤妾崝鐘烘祰閸滃矂鐛欓弨鍓佹畱閹绘帊娆㈢化鑽ょ埠閵?






### 閸樼喎娲滈崚鍡樼€?


- `PluginManager` 娑斿澧犻崣顏嗘樊閹躲倕鍘撻弫鐗堝祦閸掓銆冮敍灞剧梾閺?`DLL` 閸斻劍鈧礁濮炴潪濮愨偓浣哄閺堫剙鍚嬬€硅鐗庢灞烩偓浣烘晸閸涜棄鎳嗛張鐔锋礀鐠嬪啫鎷伴崡姝屾祰濞撳懐鎮婇懗钘夊閵?


- `FilterRegistry` 缂傚搫鐨▔銊╂敘閹恒儱褰涢敍灞筋嚤閼锋潙宓嗘担鎸庡絻娴犳儼鍏樺▔銊ュ斀濠娿倝鏆呴敍灞肩瘍閺冪姵纭堕崷銊ュ祻鏉炶姤妞傜€瑰鍙忛崶鐐存暪閹碘晛鐫嶉悙骞库偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`include/plugin/plugin_api.h`閿涘苯鐣炬稊澶嬪絻娴犺泛顔栨稉缁樺复閸欙絻鈧梗API` 閻楀牊婀扮敮鎼佸櫤閸滃苯顕遍崙铏诡儊閸欓瀹崇€规哎鈧?


- 闁插秴鍟?`PluginManager`閿涙碍鏁幐浣瑰瘻閺傚洣娆?閻╊喖缍嶉崝鐘烘祰閹绘帊娆㈤妴浣圭墡妤?`API` 閻楀牊婀伴妴浣界殶閻?`initialize/shutdown`閵嗕焦宕熼懢閿嬪絻娴犺泛绱撶敮闈╃礉楠炶泛婀崡姝屾祰閺冭埖鏁為柨鈧幓鎺嶆濞夈劌鍞介惃鍕姢闂€婊冧紣閸樺倶鈧?


- 閺傛澘顤?`sample_logger_plugin` 缁€杞扮伐 `DLL` 娑?`--plugin-check [plugin_dir_or_file]` 閸涙垝鎶ら敍宀勭崣鐠?`sample_identity` 鐟欏棝顣跺銈夋殔閻ㄥ嫭鏁為崘灞肩瑢閸楁瓕娴囧〒鍛倞闂傤厾骞嗛妴?






### 娣囶喗鏁奸弬鍥︽



- CMakeLists.txt



- include/plugin/plugin_api.h



- include/plugin/plugin_manager.h



- include/filters/filter_registry.h



- src/plugin/plugin_manager.cpp



- src/plugin/sample_logger_plugin.cpp



- src/filters/filter_registry.cpp



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂傤噣顣?59: 7.2 濞翠礁鐛熸担鎿勭礄閻喎鐤?HTTP 閸掑棛澧栨稉搴ｇ处閸愯绱?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `7.2` 鐟曚焦鐪伴幎濠冪ウ婵帊缍嬮懗钘夊娴犲簶鈧粏袙閺嬫劕娅掓銊︾仸閳ユ繃甯规潻娑樺煂閻喎鐤?HTTP 閸掑棛澧栨稉瀣祰娑撳海绱﹂崘鏌ユ４閻滎垬鈧?






### 閸樼喎娲滈崚鍡樼€?


- `HttpStreamDownloader` 娑斿澧犻崣顏冪箽鐎?URL閿涘奔绗夐崑姘崲娴ｆ洜婀＄€圭偟缍夌紒婊嗩嚢閸欐牭绱濇稊鐔哥梾閺堝鍞撮柈銊х处閸愯弓绗?EOF/闁挎瑨顕ら悩鑸碘偓浣碘偓?


- 閻滅増婀?HLS/DASH 鐟欙絾鐎介崳銊ュ涧閼宠棄顦╅悶鍡樻瀮閺堫剨绱濈紓鍝勭毌娑撯偓婵傛褰查柌宥咁槻閹笛嗩攽閻ㄥ嫭婀伴崷?HTTP 婢剁懓鍙块弶銉╃崣鐠囦礁鍨庨悧鍥︾瑓鏉炰粙鎽肩捄顖樷偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 闁插秴鍟?`HttpStreamDownloader`閿涘苯鐔€娴?FFmpeg `avio` 閺€顖涘瘮閻喎鐤?HTTP 閹垫挸绱戦妴浣稿瀻閸ф顕伴崣鏍モ偓浣稿敶闁劎绱﹂崘灞傗偓涓扥F 閻樿埖鈧椒绗岄柨娆掝嚖闁繋绱堕妴?


- 閸?`main` 娑擃厽鏌婃晶?`--streaming-buffer-check`閿涘奔绗呮潪?HLS 婵帊缍嬪〒鍛礋閵嗕浇袙閺嬫劕鑻熼幎鎾冲絿閸?N 娑擃亜鍨庨悧鍥风礉妤犲矁鐦夌紓鎾冲暱鐎涙濡弫棰佺瑢娑撳娴囩紒鎾寸亯閵?


- 閺傛澘顤?`samples/streaming/hls_local/*` 閺堫剙婀存径鐟板徔娑?`tools/start_streaming_fixture_server.ps1`閿涘矂鈧俺绻冮張顒佹簚 HTTP 閺堝秴濮熸径宥囧箛鐎圭偤鐛欓妴?






### 娣囶喗鏁奸弬鍥︽



- include/streaming/http_stream_downloader.h



- src/streaming/http_stream_downloader.cpp



- src/main.cpp



- tools/start_streaming_fixture_server.ps1



- samples/README.md



- samples/streaming/hls_local/sample.m3u8



- samples/streaming/hls_local/segment000.ts



- samples/streaming/hls_local/segment001.ts



- samples/streaming/hls_local/segment002.ts



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---











---







## 闂傤噣顣?60: 7.3 HLS/DASH 閼奉亪鈧倸绨查惍浣哄芳







**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `7.3` 鐟曚焦鐪伴幎濠冪ウ婵帊缍嬮懗钘夊娴犲簶鈧粌娴愮€规碍绔婚崡?smoke閳ユ繃甯规潻娑樺煂 HLS/DASH 婢舵氨鐖滈悳鍥掗弸鎰┾偓浣广€傛担宥夆偓澶嬪娑撳骸褰查柌宥咁槻閸ョ偛缍婇妴?






### 閸樼喎娲滈崚鍡樼€?


- `HlsManifestParser` 閸欘亣鍏樼拠璇插絿婵帊缍嬮幘顓熸杹閸掓銆冮敍灞炬￥濞夋洝鐦戦崚?`master playlist` 閻?variant 娣団剝浼呴妴?


- `DashManifestParser` 娑斿澧犻崣顏呭絹閸?`Representation` 鐢箑顔旈敍宀€宸辩亸?`BaseURL`閵嗕礁鍨垫慨瀣閸掑棛澧栨稉搴＄崯娴ｆ挸鍨庨悧鍥ㄦ缂佸棎鈧?


- 娑撹崵鈻兼惔蹇曞繁鐏忔垹绮烘稉鈧惃?ABR 闁瀚ㄩ柅鏄忕帆閸滃本婀伴崷浼寸崣閺€璺哄弳閸欙綇绱濋弮鐘崇《妤犲矁鐦夐崡鍥╃垳閻?闂勫秶鐖滈悳鍥у瀼閹广垼鐭惧鍕┾偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閹碘晛鐫?HLS/DASH 鐟欙絾鐎介崳顭掔礉鐞涖儵缍堟径姘辩垳閻滃洦绔婚崡鏇樷偓浣姐€冪粈娲肠娑撳骸鍨庨悧鍥ㄦ缂佸棎鈧?


- 閺傛澘顤?`AdaptiveBitrateSelector`閿涘本瀵滄导鎵暬鐢箑顔旈柅澶嬪閺堚偓閸栧綊鍘ら惃鍕€傛担宥忕礉楠炶泛婀?`main` 娑擃厼顤冮崝?`--adaptive-bitrate-check`閵?


- 閺傛澘顤?`samples/streaming/abr_local/{hls,dash}` 婢剁懓鍙块敍灞筋槻閻劍婀伴崷?HTTP 閺堝秴濮熸宀冪槈 HLS/DASH 閻ㄥ嫬宕岄梽宥嗐€傛稉搴″瀻閻楀洣绗呮潪濮愨偓?






### 娣囶喗鏁奸弬鍥︽



- include/streaming/hls_manifest_parser.h



- src/streaming/hls_manifest_parser.cpp



- include/streaming/dash_manifest_parser.h



- src/streaming/dash_manifest_parser.cpp



- include/streaming/adaptive_bitrate_selector.h



- src/streaming/adaptive_bitrate_selector.cpp



- src/main.cpp



- CMakeLists.txt



- tools/start_streaming_fixture_server.ps1



- samples/README.md



- samples/streaming/abr_local/hls/master.m3u8



- samples/streaming/abr_local/hls/low/index.m3u8



- samples/streaming/abr_local/hls/low/segment000.ts



- samples/streaming/abr_local/hls/low/segment001.ts



- samples/streaming/abr_local/hls/medium/index.m3u8



- samples/streaming/abr_local/hls/medium/segment000.ts



- samples/streaming/abr_local/hls/medium/segment001.ts



- samples/streaming/abr_local/hls/high/index.m3u8



- samples/streaming/abr_local/hls/high/segment000.ts



- samples/streaming/abr_local/hls/high/segment001.ts



- samples/streaming/abr_local/dash/sample.mpd



- samples/streaming/abr_local/dash/low/init.mp4



- samples/streaming/abr_local/dash/low/segment000.m4s



- samples/streaming/abr_local/dash/low/segment001.m4s



- samples/streaming/abr_local/dash/medium/init.mp4



- samples/streaming/abr_local/dash/medium/segment000.m4s



- samples/streaming/abr_local/dash/medium/segment001.m4s



- samples/streaming/abr_local/dash/high/init.mp4



- samples/streaming/abr_local/dash/high/segment000.m4s



- samples/streaming/abr_local/dash/high/segment001.m4s



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---







## 闂傤噣顣?61: 瀵よ櫣鐝涢柌宀€鈻肩喊鎴炵垼缁涙拝绱檝0.2.0-rc1 / v0.2.0閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋 `0.4` 鐟曚焦鐪版稉鍝勭秼閸撳秹妯佸▓闈涚紦缁?`v0.2.0-rc1` 娑?`v0.2.0` 闁插瞼鈻肩喊鎴炵垼缁涙拝绱濇担鍡曠波鎼存挷鑵戝銈呭濞屸剝婀佹禒璁崇秿 Git 閺嶅洨顒烽妴?






### 閸樼喎娲滈崚鍡樼€?


- 閸欐垵绔烽梻銊ь洣閸滃矂妯佸▓鍨偓褑鍏橀崝娑樺嚒缂佸繑鏁归崣锝忕礉娴ｅ棛澧楅張顒勫櫡缁嬪顣剁紓鍝勭毌閸欘垵鎷峰┃顖滄畱 Git 閺嶅洩顔囬妴?


- 瀹割喛绐涚拠鍕強娑撳簼鎹㈤崝鈩冪閸楁洑绮涙穱婵堟殌閳ユ粌褰у顔界垼缁涚偓鎼锋担婧锯偓婵堟畱閺冄冨經瀵板嫸绱濋棁鈧憰浣风瑢鐎圭偤妾禒鎾崇氨閻樿埖鈧礁鎮撳銉ｂ偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸︺劋瀵岀痪璺ㄇ旂€规艾鎻╅悡褌绗傚铏圭彌 `v0.2.0-rc1` 娑?`v0.2.0` 娑撱倓閲滈柌宀€鈻肩喊鎴炵垼缁涗勘鈧?


- 閸氬本顒為弴瀛樻煀 `VERSION / DEVELOP_LOG / MPC_HC_GAP_ANALYSIS / tasklist`閿涘矁顔囪ぐ鏇熺垼缁涙儳鍑″铏圭彌閵?


- 閸╄桨绨?`v0.2.0-rc1` 瀹稿弶鍨氶崝鐔风紦缁斿绻栨稉鈧禍瀣杽閿涘苯鎮撳銉ュ瑎闁澧界悰宀€瀹抽弶?`5.3 濮ｅ繋閲滈柌宀€鈻肩喊鎴犵波閺夌喎澧犺箛鍛淬€忛崣顖涘ⅵ RC 閺嶅洨顒穈閵?






### 娣囶喗鏁奸弬鍥︽



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---







## 闂傤噣顣?62: 閹笛嗩攽鐎瑰牆鍨崣锝呯窞閸氬本顒為敍?.1 / 5.2閿?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋娑擃厾娈戦幍褑顢戠€瑰牆鍨?`5.1 / 5.2` 娴犲秵婀弴瀛樻煀閿涘奔绲捐ぐ鎾冲娴犳挸绨遍悩鑸碘偓浣稿嚒缂佸繗鍐绘禒銉ュ灲閺傤厼鍙炬稉顓濈闁劌鍨庣痪锔芥将閺勵垰鎯佸陇鍐婚妴?






### 閸樼喎娲滈崚鍡樼€?


- `5.1` 閸忚櫕鏁為惃鍕Ц閺堫剝鐤嗛獮鎯邦攽瀹搞儰缍旈柌蹇斿付閸掕绱濋懗鎴掔矤鐎圭偤妾禒璇插閹恒劏绻樻い鍝勭碍娑擃厼绶遍崚鎷岀槈閹诡喓鈧?


- `5.2` 閸忚櫕鏁為惃鍕Ц閹稿鎳嗛懞鍌氼殧閹笛嗩攽閳ユ粌褰ч崑姘暪閺佹稈鈧繐绱濋棁鈧憰浣芥硶閸涖劊鈧線鍣告径宥嗏偓褏娈戞潻鍥┾柤鐠囦焦宓侀敍灞肩瑝閼虫垝绮庨崙顓濈濞嗏€叉唉娴犳ɑ鏁归崣锝囨纯閹恒儱瀣€闁鈧?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸曢箖鈧?`5.1 WIP 闂勬劕鍩楅敍姘倱閺冩儼绻樼悰灞兼崲閸斺€茬瑝鐡掑懓绻?2 娑撶寯閵?


- 娣囨繄鏆€ `5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 娑撳搫绶熺€瑰本鍨氶敍灞借嫙閸︺劍鏋冨锝勮厬閺勫海鈥橀崢鐔锋礈閵?






### 娣囶喗鏁奸弬鍥︽



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md



- docs/README.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂傤噣顣?63: 閽€钘夋勾 5.2 閸涖劋绨查弨鑸垫殐閺冦儲澧界悰灞惧閸?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 娴犺濮熷〒鍛礋娑擃厾娈?`5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 娴犲秴浠犻悾娆忔躬閸樼喎鍨崣锝呯窞閿涘瞼宸辩亸鎴濆讲閹笛嗩攽閻ㄥ嫬鎳嗛懞鍌氼殧娑撳孩鏁归弫娑欐）缁撅附娼妴?






### 閸樼喎娲滈崚鍡樼€?


- 閻滅増婀侀弬鍥ㄣ€傚鑼病鐟曞棛娲婇崶鐐茬秺閸涙垝鎶ら崪宀勬▉濞堜絻顓搁崚鎺炵礉娴ｅ棜绻曞▽鈩冩箒閹跺ň鈧粌鎳嗘禍鏂垮帒鐠侀晲绮堟稊鍫涒偓浣侯洣濮濐澀绮堟稊鍫涒偓浣虹波閺夌喐妞傜憰浣烽獓閸戣桨绮堟稊鍫氣偓婵嗗晸閹存劕娴愮€规碍绁︾粙瀣ㄢ偓?


- 婵″倹鐏夊▽鈩冩箒鏉╂瑥鐪板ù浣衡柤缁撅附娼敍灞藉祮娴ｅ灝缍嬮崜宥呭經瀵板嫭顒滅涵顕嗙礉閸氬海鐢绘稊鐔风发闂呭墽菙鐎规氨袧缁鳖垵娉曢崨銊﹀⒔鐞涘矁鐦夐幑顔衡偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`閿涘本濡?`5.2` 閸ュ搫瀵叉稉鍝勫讲閻╁瓨甯撮幍褑顢戦惃鍕噯閼哄倸顨旂拠瀛樻娑撳骸鎳嗘禍鏃€鏁归弫娑欏閸愬被鈧?


- 閺囧瓨鏌?`docs/README.md` 娑撳氦顔囪ぐ鏇熸瀮濡楋綇绱濋弰搴ｂ€?`5.2` 閻滀即妯佸▓闈涚暚閹存劗娈戦弰顖椻偓婊勭ウ缁嬪鎯ら崷鎵斥偓婵撶礉閼板奔绗夐弰顖椻偓婊€鎹㈤崝鈥冲瑎闁鐣幋鎰ㄢ偓婵勨偓?






### 娣囶喗鏁奸弬鍥︽



- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂傤噣顣?64: 鐞涖儵缍?5.2 閻ｆ瑧妫斿Ο鈩冩緲閿涘潐aily_board / 閸涖劍濮ら敍?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- `5.2` 閻ㄥ嫭澧界悰灞惧閸愬苯鍑＄紒蹇氭儰閸﹀府绱濇担?`daily_board` 閸滃苯鎳嗛幎銉ョ湴闂堫澀绮涚紓鍝勭毌閸ュ搫鐣惧Ο鈩冩緲閿涘苯顕遍懛鏉戞倵缂侇叀娉曢崨銊ㄧ槈閹诡喕绗夐弰鎾剁埠娑撯偓閻ｆ瑥鐡ㄩ妴?






### 閸樼喎娲滈崚鍡樼€?


- 閸欘亝婀佸ù浣衡柤鐠囧瓨妲戦敍灞剧梾閺堝缍嗛幋鎰拱閵嗕礁娴愮€规碍鐗稿蹇曟畱婵夘偄鍟撳Ο鈩冩緲閿涘本澧界悰灞炬鐎硅妲楅崣锝呯窞濠曞倻些閵?


- `5.2` 閺勵垰鎯侀崟楣冣偓澶婂絿閸愬厖绨捄銊ユ噯鐠囦焦宓侀敍灞芥礈濮濄倖膩閺夋寧婀伴煬顐＄瘍閺勵垰鐣ч崚娆掓儰閸︽壆娈戞稉鈧柈銊ュ瀻閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 缂?`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 閻ㄥ嫪琚辨稉顏勬噯娴滄棁藟娑撳﹥鏁归弫娑欐）鐠佹澘缍嶉崡掳鈧?


- 閺傛澘顤?`.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md` 娴ｆ粈璐熷В蹇撴噯閺€鑸垫殐/閸涖劍濮ゅΟ鈩冩緲閵?


- 閸氬本顒為弴瀛樻煀 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md` 閸?`docs/README.md` 閻ㄥ嫬鍙嗛崣锝堫嚛閺勫簺鈧?






### 娣囶喗鏁奸弬鍥︽



- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md



- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂傤噣顣?65: 濮瑰洦鈧缍嬮崜宥呭閼冲鈧椒濞囬悽銊︽煙瀵繋绗屾宀冪槈閸忋儱褰?






**閺冦儲婀?*: 2026-03-08







### 闂傤噣顣介幓蹇氬牚



- 闂団偓鐟曚焦濡哥粙瀣碍瑜版挸澧犲鑼病鐎圭偟骞囬惃鍕閼冲鈧礁褰查悽銊ф畱娴ｈ法鏁ら弬鐟扮础閿涘奔浜掗崣濠勫箛閺堝鐛欑拠浣界熅瀵板嫰娉︽稉顓炲晸閹存劒绔存禒钘夊讲閺屻儲鏋冨锝忕礉闁灝鍘ゆ穱鈩冧紖閺侊綀鎯ら妴?






### 閸樼喎娲滈崚鍡樼€?


- 瑜版挸澧犻懗钘夊瀹歌尙绮″Ο顏囨硶閳ユ粍顒滅敮鍛婃尡閺€淇扁偓浣界槚閺傤厼鎳℃禒銈冣偓浣风瑩妞ゅ綊鐛欓弨韬测偓浣瑰絻娴?濞翠礁鐛熸担鎾崇唨绾偓鐠佺偓鏌﹂垾婵撶礉娴ｅ棗鍙嗛崣锝呭瀻閺侊絽婀径姘嚋閺傚洦銆傛稉搴㈢爱閻礁搴滈崝鈺勭翻閸戣桨鑵戦妴?


- 婵″倹鐏夊▽鈩冩箒缂佺喍绔撮幀鏄忣潔閿涘苯鎮楃紒顓犳埛缂侇厾娣幎銈嗘瀮濡楋絾妞傜€硅妲楅柆妤佺础閳ユ粌濮涢懗瑙ｂ偓婵嗘嫲閳ユ粓鐛欑拠浣测偓婵呯闂傚娈戠€电懓绨查崗宕囬兇閵?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`閿涘瞼绮烘稉鈧拋鏉跨秿瑜版挸澧犻崝鐔诲厴閵嗕椒濞囬悽銊︽煙瀵繈鈧線鍘ょ純顔煎弳閸欙絻鈧椒绗撴い褰掔崣閺€璺烘嚒娴犮倓绗岄幎銉ユ啞閺勭姴鐨犻妴?


- 閺囧瓨鏌?`docs/README.md` 閸滃本鐗?`README.md`閿涘苯顤冮崝鐘侯嚉閹槒顫嶉弬鍥ㄣ€傞惃鍕弳閸欙絻鈧?






### 娣囶喗鏁奸弬鍥︽



- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md



- docs/README.md



- README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md















---







## 闂傤噣顣?69: 閹绢厽鏂侀柧鎹愮槚閺傤厼鍨庣仦鍌欑瑢 decoder drain / scheduler 鐎瑰綊鏁婄悰銉ュ繁







**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- 妤傛鐖滈悳鍥ㄦ尡閺€鍓旂€规碍鈧勫笓閺屻儵娓剁憰浣规绾喖灏崚鍡忊偓婊呮埂濮濓絿娈戦懗灞藉竾/閸忋儵妲︽径杈Е閳ユ繂鎷伴垾婊堟姜閻╊喗鐖ｅù浣稿瘶鐞氼偄鎷烽悾銉⑩偓婵撶礉楠炴儼藟姒?decoder drain閵嗕苟ative path 閸涙垝鑵戦悳鍥︾瑢 scheduler 鐎瑰綊鏁婃潏鍦櫕閻ㄥ嫬褰茬憴鍌涚ゴ閹佲偓?


- 閺冄冪杽閻滈鑵?packet queue EOF 閸氬孩鐥呴張澶婃倻 codec 閸欐垿鈧?`nullptr` drain閿涘奔绗?send 閸氬骸褰ч崑姘濞?receive閿涘苯顔愰弰鎾村Ω閳ユ粍娈忛弮鑸垫￥鏉堟挸鍤垾婵嗘嫲閳ユ粎婀″锝呫亼鐠愩儮鈧繃璐╅崷銊ょ鐠ф灚鈧?






### 閸樼喎娲滈崚鍡樼€?


- 鐠囧﹥鏌囪箛顐ゅ弾濮濄倕澧犻崣顏呮箒 `demux_dropped_packets` 閹鍣洪敍灞剧梾閺堝绮忛崚?drop 閸樼喎娲滈妴?


- scheduler 閸欘亙绻氶幎銈勭啊鐟欙絿鐖滅痪璺ㄢ柤閿涘ender thread 濞屸剝婀侀崥灞剧壉閻ㄥ嫬绱撶敮闀愮箽閹躲倧绱眗estart 濞嗏剝鏆熼崪宀冨剹閸樺顣堕悳鍥︾瘍濞屸剝婀佺紒鎾寸€崠鏍ь嚤閸戞亽鈧?


- 閸楀厖濞囨稉濠氭懠瀹告彃鍙挎径鍥ㄦ蒋娴犺泛绱?D3D11 native path閿涘矁绻嶇悰灞炬娑旂喓宸辩亸?`native / copy-back / swscale` 鐠侯垰绶炵拋鈩冩殶閿涘矂姣︽禒銉ф纯閹恒儵鐛欑拠浣哥秼閸撳秶鍎归悙骞库偓?






### 鐟欙絽鍠呴弬瑙勵攳



- 闁插秴鍟?video/audio decoder 閻?drain/feed 瀵邦亞骞嗛敍灞借嫙閸?packet EOF 閸氬骸顕?codec 閸欐垿鈧?`nullptr` 鐟欙箑褰?drain閵?


- 閸?`PlayerCore` 娑擃厼顤冮崝?demux drop 閸掑棛琚妴涔╡coder `send_packet(EAGAIN)`閵嗕龚rain 濞嗏剝鏆熼崪宀冾潒妫版垼绶崙楦跨熅瀵板嫯顓搁弫鑸偓?


- 閸?`Scheduler` 娑擃厼顤冮崝鐘哄剹閸樺绗?restart 缂佺喕顓搁敍灞借嫙閹?render thread 缁惧啿鍙?`runProtectedLoop()`閿涙硜estart 娑撳﹪妾洪弨鎯ь啍娑撶儤婀侀梽鎰畱婢舵碍顐肩亸婵婄槸閵?


- 閹碘晛鐫?`--performance-log-check` 鏉堟挸鍤敍灞筋嚤閸戠儤鏌婇惃鍕波閺嬪嫬瀵茬拠濠冩焽鐎涙顔岄妴?






### 娣囶喗鏁奸弬鍥︽



- include/core/scheduler.h



- src/core/scheduler.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂傤噣顣?70: PlayerCore 閻樿埖鈧焦婧€闁插秷顔曠拋锛勵儑娑撯偓闂冭埖顔?






**閺冦儲婀?*: 2026-03-19







### 闂傤噣顣介幓蹇氬牚



- `PlayerCore` 娑斿澧犻崣顏呮箒鐎电懓顦?`PlaybackState::Stopped / Playing / Paused` 娑撳鈧緤绱濇担鍡楀敶閺嶇鐤勯梽鍛扮箷闂呮劕鎯堟禍鍡曠窗鐠囨繃鈧降鈧浇绻嶇悰灞锯偓浣碘偓浣圭ウ濮樺鍤庢潻鍥┾柤閹礁鎷?deferred stop 閺冧浇鐭剧拠顓濈疅閵?


- `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 閻╁瓨甯撮弫锝囧仯閺€?`state_`閿涘苯顕遍懛瀵稿Ц閹礁褰夐崠鏍ㄧ梾閺堝绮烘稉鈧崗銉ュ經閿涘奔绡冪紓鍝勭毌闂堢偞纭舵潻浣盒╂穱婵囧Б娑撳海绮烘稉鈧弮銉ョ箶閵?






### 閸樼喎娲滈崚鍡樼€?


- UI 閹绢厽鏂侀幀浣告嫲閸愬懏鐗虫潻鎰攽鐠囶厺绠熼梹鎸庢埂濞ｅ嘲婀稉鈧稉顏呯亣娑撻箖鍣烽敍灞筋嚤閼?`PlaybackState` 鐞氼偉鎻╅幍鑳祰鏉╁洤顦块崥顐＄疅閵?


- `deferred stop` 娑斿澧犻弰顖滃缁斿绔风亸鏂剧秴閿涘奔绗夐崷銊х埠娑撯偓閻樿埖鈧焦婧€閸愬拑绱濋悩鑸碘偓浣筋潎鐎电喕顩﹂崥灞炬閹峰吋甯?`state_` 閸滃本姊虹捄顖涚垼韫囨ぜ鈧?


- `Scheduler` 瑜版挸澧犻崣顏嗘倞鐟?`running_ / paused_`閿涘苯娲滃銈囶儑娑撯偓闂冭埖顔屾惔鏂垮帥閹跺﹣绗熼崝锛勫Ц閹焦娼堟繛浣规降濠ф劖鏁归崶?`PlayerCore`閿涘矁鈧奔绗夐弰顖涘絹閸撳秵鏁?`Scheduler` 婵傛垹瀹抽妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `SessionState / RunState / PipelinePhase` 閸?`CoreStateSnapshot`閿涘本濡告导姘崇樈閹降鈧浇绻嶇悰灞锯偓浣碘偓浣圭ウ濮樺鍤庨幀浣瑰瀵偓瀵ょ儤膩閵?


- 閺傛澘顤?`transitionSessionState / transitionRunState / transitionPipelinePhase / publishPlaybackStateFromInternalState`閿涘本鏁归崣锝咁嚠婢?`PlaybackState` 閸欐ɑ娲块妴?


- 閹?`eof_reached / pending_seek / deferred_stop_pending` 缁惧啿鍙嗙紒鐔剁韫囶偆鍙庣粻锛勬倞閿涘苯鑻熸稉铏瑰Ц閹浇绺肩粔鏄忕翻閸戠儤妫╄箛妤€鎷伴棃鐐寸《鏉╀胶些 warning閵?


- 閺堫剝鐤嗘穱婵囧瘮婢舵牠鍎?`PlaybackState` 閹恒儱褰涢崗鐓庮啇閿涘奔绗夊鏇炲弳 timeline serial閿涘奔绗夐幓鎰閹?EOF 閺€瑙勫灇 `Ended`閵?






### 閺堫剙婀存灞炬暪缂佹挻鐏?


- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?






### 娣囶喗鏁奸弬鍥︽



- include/core/player_core.h



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂傤噣顣?81: PlayerCore seek/flush timeline serial 閸栨牜顑囨禍宀勬▉濞?






**閺冦儲婀?*: 2026-03-20







### 闂傤噣顣介幓蹇氬牚



- 缁楊兛绔撮梼鑸殿唽鐎瑰本鍨氶崥搴礉`PlayerCore` 瀹歌尙绮￠幎?UI 閹绢厽鏂侀幀浣告嫲閸愬懏鐗抽悩鑸碘偓浣瑰鐏炲偊绱濇担?seek/flush 娴犲秳瀵岀憰浣风贩鐠?`scheduler pause -> stopDemuxThread -> flushPipelines -> avcodec_flush_buffers() -> audio_player_->stop() -> 娴滃本顐?flush` 鏉╂瑧琚潪顖涚閻炲棜鐭惧鍕┾偓?


- `ThreadSafeQueue` 閸欘亝婀?`stop/start/eof/clear`閿涘畭FrameQueue` 閸欘亝婀?`flush()`閿涘acket/frame 闁姤鐥呴張?timeline serial閿涘苯顕遍懛瀛樻＋閺冨爼妫跨痪鎸庢殶閹诡喖宓嗘担璺ㄢ敍鏉?seek/stop 鏉堝湱鏅敍灞肩瘍閸欘亣鍏橀棃鐘偓婊咁潾瀹秆嗩潶濞撳懐鈹栭垾婵囨降婢惰鲸鏅ラ妴?


- audio consumer 缁捐法鈻奸崪?render 鐠侯垰绶為崷銊ㄧ箹娑斿澧犻柈鑺ョ梾閺?serial 闂冭尙鍤庨敍宀冪箾缂?seek閵嗕焦娈忛崑婊勨偓?seek閵嗕勾ate worker 閺€璺虹啲閺冩湹绮涢崣顖濆厴閸戣櫣骞囬弮褎鐣棅鐐解偓浣规＋濞堝鎶氶幋鏍﹀悏 EOF閵?






### 閸樼喎娲滈崚鍡樼€?


- seek/flush 娑斿澧犻崣顏呮箒閻椻晝鎮婂〒鍛閿涘本鐥呴張澶嗏偓婊嗙箹閺夆剝鏆熼幑顔肩潣娴滃骸鎽㈤弶鈩冩闂傚鍤庨垾婵堟畱閺勬儳绱￠煬顐″敜閿涘畳ecoder閵嗕购enderer閵嗕工udio consumer 閺冪姵纭剁涵顒€鍨界€规碍鏆熼幑顔芥Ц閸氾箒绻冮張鐔粹偓?


- 閺?demux 瀹搞儰缍旈妴浣规＋ codec 閸愬懐绱︾€涙ê鎶氶妴浣规＋ frame queue 閺佺増宓侀崪灞炬＋ audio submit 闁晫宸辩亸鎴犵埠娑撯偓閻ㄥ嫭妞傞梻瀵稿殠鏉堝湱鏅妴?


- `flush` 閺堫剝闊╅崣顏囧厴鐏忎粙鍣洪梽宥勭秵濞夊嫭绱″鍌滃芳閿涘奔绗夐懗钘夌暰娑斿绮风€靛湱娈戞惔鐔风磾鐟欏嫬鍨妴?






### 鐟欙絽鍠呴弬瑙勵攳



- 閺傛澘顤?`TimelineSerial`閿涘苯鑻熺拋?packet/frame 闁姤妯夊蹇旀儭鐢?serial閿涙瓪DemuxPacket { PacketPtr packet; TimelineSerial serial; }`閵嗕梗VideoFrame::serial`閵嗕梗AudioFrame::serial`閵?


- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `timeline_serial / pending_seek_serial`閿涘奔浜掗崣濠勭埠娑撯偓閻?`allocateNextTimelineSerial / activateTimelineSerial / setPendingSeekSerial / isAcceptedTimelineSerial` 閸忋儱褰涢妴?


- `open` 閹存劕濮涢弮璺虹紦缁斿顩绘稉?serial閿涙矖seek` 閸忓牆鍨庨柊?`pending_seek_serial`閿涘eek 閹存劕濮涢崥搴″晙濠碘偓濞蹭紮绱盽stop / requestDeferredStop` 閻╁瓨甯撮幒銊ㄧ箻 serial閿涘奔濞囬弮?worker 閺呮艾鍩岄弮鏈电瘍閸欘亣鍏樻禍褍鍤惔鐔告殶閹诡喓鈧?


- demux 缁捐法鈻奸崥顖氬З閺冭埖宕熼懢宄扮秼閸?serial閿涘矂浼╅崗宥嗘＋ demux 瀹搞儰缍旂悮顐ヮ嚖閺嶅洦鍨氶弬鐗堟闂傚鍤庨妴?


- `decodeVideoFrame / decodeAudioFrame / renderFrame / renderPausedFrameAtOrAfter / audio consumer` 閸忋劑鎽肩捄顖涘复閸?serial 閸掋倕鐣鹃敍灞炬＋ serial 閻╁瓨甯存稉銏犵磾閿涙矖flush` 娣囨繄鏆€閿涘奔绲鹃梽宥囬獓娑撻缚绶熼崝鈺傜閹殿偁鈧?


- `DiagnosticsSnapshot`閵嗕胶濮搁幀浣规）韫囨鎷版稉鎾汇€嶅Λ鈧弻銉ユ嚒娴犮倖鏌婃晶?`timeline_serial / pending_seek_serial` 鐟欏倹绁寸€涙顔岄妴?






### 閺堫剙婀存灞炬暪缂佹挻鐏?


- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?






### 娣囶喗鏁奸弬鍥︽



- include/core/frame.h



- src/core/frame.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---



---



## ?? 82: PlayerCore EOF/Ended ????????



**??**: 2026-03-20



### ????

- ????????seek/stop ???????? serial ???? EOF ?????????????????? stop/deferred-stop ??????? `Stopped`???????? `Ended` ???

- ??????????????`close()` ? `stop()` ???????????????scheduler ???? render callback ??? `rendered_frames`??? stale frame ?????????



### ????

- ??? `PlaybackState` ?????????? stop????? EOF ??????????????????????

- EOF ??????? demux/queue ?????????????????????????

- ???????? serial ????? `close/reopen` ????? stop ??????? scheduler ???????????????????? present??



### ????

- ? `PlayerCore` ?????? `EndedReason`?????? `CoreStateSnapshot`?`DiagnosticsSnapshot` ?????????? `PlaybackState` ?????

- ?? `onRenderIdle()` ? EOF ?????? `PipelinePhase::Draining`??? packet queue EOF + frame queue ?? + ????????????? `RunState::Ended`???? `deferred stop` ?? `Stopped`?

- `play()` ? `Ended` ???? `seek(0.0)` ????`seek()` ? `Ended` ????? `Stopped`???? ended ??/???????????

- `close()` ? `stop()` ??????? timeline serial???? worker ??????? stale serial ?????

- ?? scheduler render callback ? `bool` ?????????? render/present ????? `rendered_frames` ? `last_render_wall_tp_`?



### ??????

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`????



### ????

- include/core/player_core.h

- include/core/scheduler.h

- src/core/player_core.cpp

- src/core/scheduler.cpp

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY18_EOF_ENDED_AND_TERMINAL_STATE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

---

## ?? 83: PlayerCore queue generation ? Scheduler ????????

**??**: 2026-03-20

### ????
- ????????? item-level serial ? EOF/Ended ?????? queue ????? `clear()/flush()` ?????????????? generation/epoch ???????? producer/consumer?
- `Scheduler` ????? `running_ / paused_`?? `Seeking / Flushing / Stopping / Ended` ???????????render loop ? clock wait ??????????????

### ????
- item-level serial ?????????????????????? queue ???? generation???????? `push()/pop()` ?????????????
- scheduler ?????? `RunState / PipelinePhase / accepted timeline serial`?????? callback ???????????/???????????????

### ????
- ? `ThreadSafeQueue` ? `FrameQueue` ?? generation?`clear()/flush()` ?? generation????? `push()/pop()` ????? generation ???????????
- ? `scheduler.h` ????? `SchedulerControlSnapshot`???? `run_state / pipeline_phase / accepted_timeline_serial`?????? scheduler ??????????
- `PlayerCore` ?? `makeSchedulerControlSnapshot()` ???????? `setControlSnapshotProvider()` ??? scheduler?
- ??/?? decode loop ???????? run/phase ??????render loop ? pop ????? clock wait ??????????? accepted serial????? callback ???????????
- `DiagnosticsSnapshot`??? diagnostics ????????? packet/frame queue generation ????????? flush ???

### ??????
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`????

### ????
- include/thread_safe_queue.h
- include/core/frame_queue.h
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY19_QUEUE_GENERATION_AND_SCHEDULER_CONTROL_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 闂傤噣顣?84: PlayerCore 閸擃垯缍旈悽銊╂肠娑擃厼瀵叉稉?runtime failure/recovery policy 閺€璺哄經

**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- timeline serial 閸?queue generation 瀹歌尙绮￠幎?seek/flush 鏉堝湱鏅涵顒€瀵查敍灞肩稻 `play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 閸忋儱褰涢柌灞肩矝閻掕埖璐╅惈鈧痪璺ㄢ柤閵嗕浇顔曟径鍥モ偓渚€妲﹂崚妤€鎷伴弮鍫曟寭閸擃垯缍旈悽銊ｂ偓?- `SchedulerControlSnapshot` 娑斿澧犻崣顏囶洬閻?`run_state / pipeline_phase / accepted_timeline_serial`閿涘cheduler 娴犲秹娓剁憰浣风矤婢舵牠鍎寸拠顓烆暔閻?`clock_source`閵嗕工udio-master 閺勵垰鎯侀惇鐔烘畱閺堝鏅ラ敍灞间簰閸?`Ended` 閺冭埖妲搁崥锕€鍘戠拋闀愮箽閺堚偓閸氬簼绔寸敮褋鈧?- decode/resample/output 閻?fatal 閻愰€涚矝鐎硅妲楅梹鍨毉閸氬嫯鍤滈惃?`emitError + return false` 鐠侯垰绶為敍灞句划婢跺秶鐡ラ悾銉у繁鐏忔垹绮烘稉鈧崗銉ュ經閵?
### 閸樼喎娲滈崚鍡樼€?- 缁楊兛绔撮梼鑸殿唽閺€璺哄經閻ㄥ嫭妲搁垾婊呭Ц閹浇绺肩粔璇插晸閸忋儳鍋ｉ垾婵撶礉娴ｅ棜绻曞▽鈩冩箒閹跺ň鈧粎濮搁幀浣界讣缁夋槒袝閸欐垹娈戦崝銊ょ稊閳ユ繃濞婄粋缁樺灇缂佺喍绔撮崜顖欑稊閻劍膩閸ㄥ鈧?- `deferred stop` 閻ㄥ嫭婀扮拹銊︽Ц瀵倹顒炵€瑰本鍨?stop閿涘矁鈧奔绗夐弰顖氬綗娑撯偓婵傛浠犻張杞扮瑹閸斅ゎ嚔娑斿绱辨俊鍌涚亯 request/completion 闁槒绶稉宥囩埠娑撯偓閿涘苯鎮楃紒顓炵发鐎硅妲楅崘宥嗩偧閸掑棗寮堕妴?- scheduler 閼汇儳鎴风紒顓㈡浆闂嗚埖鏆庣敮鍐ㄧ毜娴ｅ秵瀚炬稉姘鐠囶厺绠熼敍灞芥皑娴兼碍濡?`clock_source`閵嗕工udio-master 娑?ended policy 缂佈呯敾閺侊綀鎯ら崷?loop 閸愬懘鍎撮妴?- runtime failure 閼汇儰绗夐崗鍫㈢埠娑撯偓閹?policy閿涘苯顕惔鏃傛畱 recovery path 娴兼岸娈㈤惈鈧弴鏉戭樋 fatal 閻愬湱鎴风紒顓熷⒖閺侊絻鈧?
### 鐟欙絽鍠呴弬瑙勵攳
- 閹?`SchedulerControlSnapshot`閿涙碍鏌婃晶?`clock_source`閵嗕梗audio_output_initialized`閵嗕梗audio_master_sync_active`閵嗕梗ended_policy`閿涘苯鑻熺拋?scheduler 閻╁瓨甯村☉鍫ｅ瀭鏉╂瑤绨虹紒鎾寸€崠鏍閺夌喆鈧?- 閸?`PlayerCore` 閺傛澘顤冪紒鐔剁 helper閿涙瓪applyStartPlaybackSideEffects`閵嗕梗applyResumePlaybackSideEffects`閵嗕梗applyPausePlaybackSideEffects`閵嗕梗applyStopRequestSideEffects`閵嗕梗applyStopCompletionSideEffects`閵嗕梗applySeekSideEffects`閵嗕梗applySessionReleaseSideEffects`閵?- `requestDeferredStop()` 娑?`serviceDeferredStop()` 閺€閫涜礋婢跺秶鏁ら崥灞肩婵?stop-request / stop-completion helpers閿涘本濡?deferred stop 楠炶泛娲栫紒鐔剁 stopping 鐠侯垰绶為妴?- 閺傛澘顤?`FailureRecoveryPolicy` 閸?`handleRuntimeFailure()`閿涘本濡?decode/resample/output 閸忔娊鏁?fatal 閻愬湱绮烘稉鈧弨璺哄經閸?`EmitOnly / StopPlayback / FailSession` 缁涙牜鏆愰崗銉ュ經閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂傤噣顣?85: PlayerCore 閸撯晙缍戞搴ㄦ珦閺€鑸垫殐閿涙瓔cheduler 缂佸牏澧楃粵鏍殣閵嗕笚ailSession 鐎圭偛瀵叉稉?serial/generation 鐟欏倹绁村鍝勫

**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- `SchedulerControlSnapshot` 娴犲秵婀ぐ銏″灇缂佸牏澧楃粵鏍殣鐞涖劏鎻敍瀹憀ock/audio-master/ended 鐠囶厺绠熸潻妯绘箒闂呮劕绱￠幒銊ヮ嚤閵?- `FailSession` 閾忚姤婀佺紒鐔剁閸忋儱褰涢敍灞肩稻閸忔娊鏁径杈Е閻愮懓鐨婚張顏囩箻閸忋儱鐤勯梽鍛邦洬閻╂牓鈧?- queue generation 娑?item-level serial 閻ㄥ嫯浜寸拹锝堢珶閻ｅ瞼宸辩亸鎴濆讲鐟欏倹绁存担鎰槈閵?
### 閸樼喎娲滈崚鍡樼€?- scheduler 娑斿澧犳禒宥勫瘜鐟曚焦绉风拹?`clock_source + bool`閿涘苯顕遍懛瀵哥摜閻ｃ儲鍓伴崶鍙ョ瑝婢剁喐妯夊蹇嬧偓?- `FailSession` 鐠嬪啰鏁ら悙閫涚瑝鐡掔绱濋幁銏狀槻鐠侯垰绶為崷銊ф埂鐎圭偤鐝搴ㄦ珦闁挎瑨顕ゆ稉濠佺矝閸嬪繐鎮?`StopPlayback`閵?- diagnostics 缂傚搫鐨?stale serial 娑撱垹绱旂拋鈩冩殶閿涘矂姣︽禒銉ф纯閹恒儴鐦夐弰搴ｂ€栨径杈ㄦ櫏娑撹鍨界€规碍娼甸懛?serial閵?
### 鐟欙絽鍠呴弬瑙勵攳
- 閹?`SchedulerControlSnapshot`閿涙碍鏌婃晶?`SchedulerClockPolicy`閵嗕梗SchedulerAudioMasterPolicy`閵嗕梗audio_buffered_seconds`閿涘苯鑻熼幍鈺佺潔 `SchedulerEndedPolicy`閵?- `Scheduler` 婢х偛濮炵粵鏍殣濞戝牐鍨傞崙鑺ユ殶 `isAudioMasterActive / isVideoMasterActive / shouldApplyClockSync`閿涘苯鑻熺悰?`Scheduler::stop()` self-join 闂冭尙鍤庨妴?- `handleRuntimeFailure()` 閺€璺哄經婢х偛宸遍敍姝歋topPlayback`/`FailSession` 閸у洨绮烘稉鈧挧?stop request side effects閿涙矖FailSession` 鐟曞棛娲婇崗鎶芥暛娑撳秴褰查幁銏狀槻婢惰精瑙﹂悙骞库偓?- 閺傛澘顤?stale serial 娑撱垹绱旂拋鈩冩殶楠炶埖甯撮崗?`DiagnosticsSnapshot`閵嗕椒缍嗘０鎴炴）韫囨ぜ鈧梗--performance-log-check` 娑?`--software-video-decode-check`閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂傤噣顣?86: 婢х偠藟 serial/failsession 閸ョ偛缍婇幒銏ゆ嫛閿涘牐绻涚紒?seek閵嗕焦娈忛崑婊勨偓?seek閵嗕恭lose/reopen閿?
**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- 闂団偓鐟曚椒璐?`FailSession + timeline serial` 閺€鑸垫殐闂冭埖顔岀悰銉ュ帠閺堝搫娅掗崣顖濐嚢閸ョ偛缍婂Λ鈧弻銉礉鐟曞棛娲婇敍?  1. 鏉╃偟鐢?seek
  2. 閺嗗倸浠犻幀?seek
  3. close/reopen
- 閻滅増婀佸Λ鈧弻銉┿€嶉搹鐣屽姧閼冲€熺翻閸?diagnostics閿涘奔绲剧亸姘弓閹跺﹨绻栨稉澶岃鏉堝湱鏅崷鐑樻珯閼辨艾鎮庨幋鎰讲閻╁瓨甯?gate 閻?`key=value + result=PASS/FAIL`閵?
### 閸樼喎娲滈崚鍡樼€?- 閻滅増婀?`--performance-log-check` / `--software-video-decode-check` 閸嬪繐鎮滈柧鎹愮熅閸嬨儱鎮嶈箛顐ゅ弾閿涘奔绗夐惄瀛樺复鐞涖劏鎻垾婊嗙珶閻ｅ苯澧犻崥?stale 婢х偤鍣?+ serial 鏉╀胶些 + FailSession 闂堢偞纭剁捄瀹犳祮缁撅附娼垾婵勨偓?- `PlayerCore` 閸愬懘鍎撮棃鐐寸《鏉╀胶些濮濄倕澧犳禒鍛）韫囨鎲＄拃锔肩礉濞屸剝婀?diagnostics 鐠佲剝鏆熺€涙顔岄敍瀛婰I 娑撳秵妲楅張鍝勬珤閸掋倕鐣鹃妴?
### 鐟欙絽鍠呴弬瑙勵攳
- 閸?`DiagnosticsSnapshot` 婢х偛濮為獮鑸垫Ё鐏忓嫰娼▔鏇＄讣缁夋槒顓搁弫甯窗
  - `illegal_session_transitions`
  - `illegal_run_transitions`
  - `illegal_pipeline_transitions`
- 閸?`PlayerCore` 閻?`transitionSessionState / transitionRunState / transitionPipelinePhase` 闁插苯顕棃鐐寸《鏉╀胶些鐠佲剝鏆熼敍灞借嫙閸?diagnostics reset/閺冦儱绻旀稉顓熷复閸忋儯鈧?- 閸?`src/main.cpp` 閺傛澘顤冩稉澶夐嚋 CLI 閸ョ偛缍婇崨鎴掓姢閿?  - `--seek-burst-serial-check <media_file> [seek_count]`
  - `--paused-seek-serial-check <media_file> [seek_count]`
  - `--close-reopen-serial-check <media_file> [sample_ms]`
- 濮ｅ繋閲滈崨鎴掓姢鏉堟挸鍤紒鐔剁 `key=value`閿涘苯鑻熺紒娆忓毉 `result=PASS/FAIL`閿涙稑鍨界€规碍鐗宠箛鍐洬閻╂牭绱?  - serial 鏉╀胶些閺勵垰鎯侀幐浣虹敾閹恒劏绻?  - stale 閺勵垰鎯佹禒鍛躬鏉堝湱鏅粣妤€褰涢崙铏瑰箛閵嗕胶菙鐎规氨鐛ラ崣锝嗘Ц閸氾附鏁归弫?  - `FailSession` 鐟欙箑褰傞弮鑸垫Ц閸氾箑鐡ㄩ崷銊╂姜濞夋洝绺肩粔浼欑礄`fail_session_transition_ok`閿?- 閸氬本顒為幎濠囨姜濞夋洝绺肩粔鏄忣吀閺佹澘顕遍崙鍝勫煂閿?  - `--performance-log-check`
  - `--software-video-decode-check`

### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻敍?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缂佹挻鐏夐敍姘垛偓姘崇箖閿涘畭0 warnings / 0 errors`
- 閺傛澘顤冨Λ鈧弻銉ユ嚒娴犮倧绱欓弽閿嬫拱閿涙瓪juren-30s.mp4`閿涘绱?  - `build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4`閿涙瓪PASS`
  - `build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4`閿涙瓪PASS`
  - `build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4`閿涙瓪PASS`

### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂傤噣顣?87: serial/failsession 閸ョ偛缍婃晶鐐插娑撯偓闁款喛浠涢崥?gate閿涘牓妾锋担搴㈢础鐠烘垿顥撻梽鈺嬬礆

**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- 瀹稿弶婀?`--seek-burst-serial-check`閵嗕梗--paused-seek-serial-check`閵嗕梗--close-reopen-serial-check` 娑撳閲滈幒銏ゆ嫛閿涘奔绲鹃幍褑顢戦弮鏈电矝闂団偓娴滃搫浼愭稉鑼额攽鐠嬪啰鏁ら妴?- 閸︺劑鐝０鎴ｅ嚡娴狅綁妯佸▓纰夌礉娴滃搫浼愭稉鑼额攽閹笛嗩攽鐎涙ê婀蹇氱獓閺屾劒绔存い鍦畱妞嬪酣娅撻敍灞肩瑝閸掆晙绨粙鍐茬暰 gate閵?
### 閸樼喎娲滈崚鍡樼€?- 娑撳閲滈幒銏ゆ嫛閻ㄥ嫬鍨界€规艾褰涘鍕嚒缂佸繒绮烘稉鈧稉?`key=value + result=PASS/FAIL`閿涘奔绲剧紓鍝勭毌缂佺喍绔撮懕姘値閸忋儱褰涢妴?- 閼汇儲鐥呴張澶庝粵閸氬牆鍙嗛崣锝忕礉鐠嬪啰鏁ら弬褰掓付鐟曚浇鍤滅悰宀€娣幎銈夈€庢惔蹇嬧偓浣稿棘閺佹澘鎷伴幀鑽ょ波閺嬫粣绱濈€硅妲楅崙铏瑰箛閼存碍婀版稉宥勭閼锋番鈧?
### 鐟欙絽鍠呴弬瑙勵攳
- 閸?`src/main.cpp` 閺傛澘顤冮懕姘値閸涙垝鎶ら敍?  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 閼辨艾鎮庨崨鎴掓姢閸愬懘鍎存い鍝勭碍閹笛嗩攽娑撳娼悳鐗堟箒閹恒垽鎷￠敍灞借嫙鏉堟挸鍤紒鐔剁閹崵绮ㄩ弸婊冪摟濞堢绱?  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`

### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻敍?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缂佹挻鐏夐敍姘垛偓姘崇箖閿涘畭0 warnings / 0 errors`
- 閼辨艾鎮庨崨鎴掓姢閺嶉攱婀版宀冪槈閿涘潉juren-30s.mp4`閿涘绱?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`
  - 缂佹挻鐏夐敍姝歴erial-failsession-regression-check.pass_count=3`閵嗕梗serial-failsession-regression-check.total_count=3`閵嗕梗serial-failsession-regression-check.result=PASS`

### 娣囶喗鏁奸弬鍥︽
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂傤噣顣?88: 瀵搫鍩?FailSession 閸ョ偛缍婇幒銏ゆ嫛娑?codec 闁夸線鍣搁崗銉ョ┛濠у啩鎱ㄦ径?
**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- 閻滅増婀?serial/failsession 閸ョ偛缍婃稉鏄忣洣鐟曞棛娲婂锝呯埗闁炬崘鐭鹃崪宀冪珶閻ｅ苯鍨忛幑顫礉娴?`FailSession` 娴犲秳绶风挧鏍埂鐎圭偛绱撶敮姝屝曢崣鎴礉缂傚搫鐨粙鍐茬暰閸欘垶鍣告径宥囨畱瀵搫鍩楃憰鍡欐磰閵?- 閸︺劍鏌婃晶鐐插繁閸掓儼鐭惧鍕赴闁藉牐绻冪粙瀣╄厬閿涘本姣氶棁鎻掑毉 `FailSession` 娴犲氦袙閻胶鍤庣粙瀣箻閸忋儲妞傞惃鍕カ濠ф劕娲栭弨璺虹磽鐢潻绱癭device or resource busy`閵?
### 閸樼喎娲滈崚鍡樼€?- 缂傚搫鐨垾婊勭ゴ鐠囨洑绗撻悽銊ｂ偓浣稿讲閹貉喰曢崣鎴斺偓婵堟畱 `FailSession` 濞夈劌鍙嗛悙鐧哥礉鐎佃壈鍤х拠銉ㄧ熅瀵板嫬绶㈤梾鍓旂€?gate閵?- `FailSession` 闁插﹥鏂佺挧鍕爱閺冩湹绱版潻娑樺弳 decoder 闁插﹥鏂侀柅鏄忕帆閿涘矁鈧矁顕氱捄顖氱窞閸欘垵鍏樻稉搴ば掗惍浣哄殠缁嬪鍑￠幐浣规箒閻?codec 闁夸礁褰傞悽鐔锋倱缁捐法鈻奸柌宥呭弳閸愯尙鐛婇妴?
### 鐟欙絽鍠呴弬瑙勵攳
- `PlayerCore::decodeVideoFrame` 婢х偛濮炲ù瀣槸濞夈劌鍙嗗鈧崗绛圭窗
  - `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE=1` 閺冭泛婀憴鍡涱暥鐟欙絿鐖滄潏鍦櫕鐟欙箑褰傛稉鈧▎?`FailureRecoveryPolicy::FailSession`閵?- `main` 閺傛澘顤冩稉鎾汇€嶉崨鎴掓姢閿?  - `--forced-failsession-check <media_file> [sample_ms]`
  - 鏉堟挸鍤?`runtime_failure_*`閵嗕梗illegal_*` 閸?`result=PASS/FAIL`閵?- 娣囶喖顦?codec 闁夸線鍣搁崗銉ョ磽鐢潻绱?  - `video_codec_mutex_`閵嗕梗audio_codec_mutex_` 閺€閫涜礋 `std::recursive_mutex`閿?  - `decodeVideoFrame/decodeAudioFrame` 閻?`lock_guard` 閸氬本顒為弨閫涜礋 `std::recursive_mutex` 閻楀牊婀伴妴?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻敍?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缂佹挻鐏夐敍姘垛偓姘崇箖閿涘畭0 warnings / 0 errors`
- 瀵搫鍩?FailSession 閹恒垽鎷￠敍?  - `build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`
  - 缂佹挻鐏夐敍姝歳untime_failure_stop_requests=1`閵嗕梗runtime_failure_fail_sessions=1`閵嗕梗illegal_transition_total=0`閵嗕梗result=PASS`
- serial 閼辨艾鎮庢径宥嗙ゴ閿?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`
  - 缂佹挻鐏夐敍姝歳esult=PASS`

### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂傤噣顣?89: run_all_checks 閹恒儱鍙?forced-failsession 娑撯偓闁?gate

**閺冦儲婀?*: 2026-03-20

### 闂傤噣顣介幓蹇氬牚
- `tools/run_all_checks.ps1` 閸樼喐绁︾粙瀣╃矌鐟曞棛娲?`probe + format regression`閿涘本婀妯款吇鐟曞棛娲?`FailSession` 閹垹顦查柧鎹愮熅閵?- 鏉╂瑤绱扮€佃壈鍤ч垾婊勬）鐢晲绔撮柨顔兼礀瑜版帒褰ф宀冪槈鐢瓕顫夐柧鎹愮熅閿涘矂浠愬蹇撱亼鐠愩儲浠径宥堢熅瀵板嫧鈧繄娈戝▓瀣╃稇妞嬪酣娅撻妴?
### 閸樼喎娲滈崚鍡樼€?- `--forced-failsession-check` 瀹告彃鐡ㄩ崷銊ょ瑬缁嬪啿鐣鹃敍灞肩稻閺堫亞鎾奸崗銉﹀婢跺嫮鎮婇懘姘拱姒涙顓诲ù浣衡柤閵?- 缂傚搫鐨涵?gate 娴兼矮濞?FailSession 閸ョ偛缍婇幍褑顢戞笟婵婄娴滃搫浼愰懛顏囶潕閿涘矂鏆遍張鐔奉啇閺勬挻绱＄捄鎴欌偓?
### 鐟欙絽鍠呴弬瑙勵攳
- 閹碘晛鐫?`tools/run_all_checks.ps1` 閸欏倹鏆熼敍?  - `ForcedFailSessionFile`閿涘牓绮拋銈団敄閿涘苯娲栭拃钘夘槻閻?`ProbeFile`閿?  - `ForcedFailSessionSampleMs`閿涘牓绮拋?`2200`閿?- 鐏忓棙澧界悰灞剧ウ缁嬪宕岀痪褌璐熸稉澶嬵劄閿?  1. `[1/3]` `--probe-file --json`
  2. `[2/3]` `--forced-failsession-check`
  3. `[3/3]` `run_format_regression.ps1`
- gate 鐟欏嫬鍨敍?  - probe 闂堢偤娴傞敍姘辨纯閹恒儵鈧偓閸戠尨绱?  - forced-failsession 闂堢偤娴傞敍姘辨纯閹恒儵鈧偓閸戝搫鑻熺捄瀹犵箖 format regression閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- 閹笛嗩攽閸涙垝鎶ら敍?  - `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Debug/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200`
- 缂佹挻鐏夐敍?  - `probe exit code = 0`
  - `forced-failsession exit code = 0`
  - `regression exit code = 0`
  - 閼存碍婀伴幀濠氣偓鈧崙铏圭垳閿涙瓪0`

### 娣囶喗鏁奸弬鍥︽
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 闂 90: OpenGL 鍘熺敓 D3D11 浜掓搷浣滃仠姝㈡湡寮傚父閫€鍑轰笌浣庡悶鍚愪慨澶?
**鏃ユ湡**: 2026-03-24

### 闂鎻忚堪
- `OpenGL` 鍘熺敓纭В璺緞宸茬粡鍚姩锛屼絾 `--performance-log-check` 鍦?stop/close 闃舵寮傚父閫€鍑猴紝瀵艰嚧娌℃湁鏈€缁?`PASS/FAIL` 杈撳嚭銆?- 鍘熺敓璺緞鍚炲悙鏄庢樉寮傚父锛宍juren-30s.mp4` 鍦?2 绉掗噰鏍风獥鍙ｅ唴鍙兘璺戝嚭绾?3 甯э紝鏃犳硶杈惧埌涓?D3D11 涓婚摼鎺ヨ繎鐨勭ǔ瀹氬害銆?
### 鍘熷洜鍒嗘瀽
- OpenGL 鍘熺敓浜掓搷浣滈摼璺噷锛孎Fmpeg 鐨?`D3D11VA` 瑙ｇ爜涓?OpenGL 娓叉煋绾跨▼鍏变韩鍚屼竴涓?renderer-owned D3D11 device/context銆?- 璇?D3D11 immediate context 鏈紑鍚?`ID3D11Multithread::SetMultithreadProtected(TRUE)`锛屽鑷磋法绾跨▼璁块棶璁惧鏃跺嚭鐜颁笉绋冲畾鍜屼綆鍚炲悙銆?- `PlayerCore` 鍏抽棴 session 鏃跺厛閲婃斁 decoder/hw context銆佸悗鍏抽棴 renderer锛宯ative `AVFrame` 缂撳瓨鍜屾覆鏌撶嚎绋嬬殑閲婃斁椤哄簭涓嶅畨鍏ㄣ€?
### 瑙ｅ喅鏂规
- 鍦?`src/render/opengl_video_renderer.cpp` 涓负 renderer-owned D3D11 context 鍚敤 `ID3D11Multithread` 澶氱嚎绋嬩繚鎶ゃ€?- 鍦?`src/core/player_core.cpp` 涓皟鏁?session release 椤哄簭锛氬厛娓呯悊缂撳瓨 native frame 骞跺叧闂?renderer锛屽啀閲婃斁 decoder/hw context銆?- 淇濈暀褰撳墠鎴愮啛鎾斁鍣ㄩ鏍肩殑浜掓搷浣滃疄鐜帮細`D3D11VA decode surface -> D3D11 shader convert -> WGL_NV_DX_interop -> OpenGL draw`銆?
### 鏈湴楠岃瘉
- Release 鏋勫缓閫氳繃锛歚cmake --build build --config Release`
- 鍛戒护锛歚$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`
- 缁撴灉锛歚performance-log-check.renderer_backend=OpenGL`
- 缁撴灉锛歚performance-log-check.decoder_backend=D3D11VA`
- 缁撴灉锛歚performance-log-check.video_native_output_frames=62`
- 缁撴灉锛歚performance-log-check.video_copy_back_frames=0`
- 缁撴灉锛歚performance-log-check.render_frames=47`
- 缁撴灉锛歚performance-log-check.result=PASS`
- 缁撴灉锛氳繘绋嬮€€鍑虹爜 `0`

### 淇敼鏂囦欢
- src/render/opengl_video_renderer.cpp
- src/core/player_core.cpp
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 91: OpenGL/D3D11 瀛楀箷閾捐ˉ榻?ASS 鏍峰紡鍙樻崲涓庢牱寮忚瘖鏂?
**鏃ユ湡**: 2026-03-24

### 闂鎻忚堪
- OpenGL 涓?D3D11 鎾斁閾惧凡缁忚兘绋冲畾鎾斁锛屼絾 `ASS/SSA` 鏍峰紡璇箟浠嶆槑鏄惧急浜庢垚鐔熸挱鏀惧櫒銆?- 涔嬪墠 `ASS parser` 鍙鐩栧皯閲?tag锛屽緢澶氬父瑙佹牱寮忚櫧鐒跺湪瀛楀箷鏂囦欢閲屽瓨鍦紝浣嗚繘鍏ユ挱鏀惧櫒鍚庡苟娌℃湁鐪熸浣滅敤鍒?GPU 瀛楀箷娓叉煋銆?- 缂哄皯鏈哄櫒鍙鐨勬牱寮忔鏌ュ叆鍙ｏ紝瀵艰嚧 OpenGL 瀛楀箷闂鎺掓煡浠嶅亸渚濊禆鑲夌溂瑙傚療銆?
### 鍘熷洜鍒嗘瀽
- `SubtitleStyle` 鏁版嵁妯″瀷涓嶅瀹屾暣锛岀己灏戯細`wrap_style`銆乣spacing`銆乣scale_x/scale_y`銆乣rotation`銆乣x/y border`銆乣x/y shadow`銆?- `src/subtitle/ass_parser.cpp` 鏈В鏋?`WrapStyle`銆乣ScaleX/ScaleY`銆乣Spacing`銆乣Angle` 浠ュ強 `\q/\fsp/\fscx/\fscy/\fr/\xbord/\ybord/\xshad/\yshad`銆?- OpenGL / D3D11 鐨?D2D/DirectWrite 瀛楀箷璺緞涔熸病鏈夋秷璐硅繖浜涘瓧娈点€?
### 瑙ｅ喅鏂规
- 鎵╁睍 `SubtitleStyle` 鍜屾瘮杈冮€昏緫锛屽鍔?ASS 鏍峰紡鍙樻崲瀛楁銆?- 澧炲己 `ASS parser`锛屾妸鑴氭湰绾с€乻tyle 绾у拰 override 绾у父瑙?ASS 鏍峰紡鏄犲皠鍒扮粺涓€瀛楀箷妯″瀷銆?- 鍚屾鏇存柊 `src/render/opengl_video_renderer.cpp` 涓?`src/render/d3d11_video_renderer.cpp`锛岃 GPU 瀛楀箷閾剧湡姝ｆ秷璐硅繖浜涘瓧娈点€?- 鏂板 `--subtitle-style-check`锛岃緭鍑?item/run/style 鍏ㄩ噺 `key=value` 璇婃柇淇℃伅銆?- 鏂板鏍锋湰 `samples/subtitles/opengl_ass_style_validation.ass`銆?
### 鏈湴楠岃瘉
- `./build/Release/modern-video-player.exe --subtitle-style-check ./samples/subtitles/opengl_ass_style_validation.ass`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./samples/subtitles/opengl_ass_style_validation.ass`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_style_validation.ass`
- `./build/Release/modern-video-player.exe --opengl-diagnostics`
- 缁撴灉锛歚subtitle-style-check.result=PASS`銆乣subtitle-sync-check.result=PASS`銆乣delay-adjust-check.result=PASS`銆乣opengl-diagnostics.result=PASS`

### 淇敼鏂囦欢
- include/subtitle/subtitle_parser.h
- src/subtitle/subtitle_parser.cpp
- src/subtitle/ass_parser.cpp
- src/subtitle/srt_parser.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- src/main.cpp
- samples/subtitles/opengl_ass_style_validation.ass
- docs/analysis/PLAYERCORE_DAY28_OPENGL_ASS_STYLE_TRANSFORM_AND_PARSER_DIAGNOSTICS.md
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## Issue 92: OpenGL/D3D11 subtitle run-level karaoke, clip and subtitle clock convergence

**Date**: 2026-03-24

### Problem Description
- The second ASS capability batch was only partially wired in: parser/model changes existed, but renderer-side run-level coloring, karaoke timing, clip rendering and paused redraw behavior were not fully closed.
- OpenGL still rendered subtitles mostly as item-level text, which left a visible gap against mature players for `secondary color`, `clip`, `iclip` and karaoke sweep behavior.

### Root Cause Analysis
- Subtitle timing data was not fully propagated from `PlayerCore` into renderer-side subtitle texture invalidation / redraw decisions.
- The OpenGL subtitle D2D path had not yet been ported to the same run-level brush/effect logic already drafted for D3D11.
- `iclip` had parser coverage but no rendering coverage.

### Solution
- Added `setSubtitleClock()` plumbing from `PlayerCore` into `IVideoRenderer`, `D3D11VideoRenderer` and `OpenGLVideoRenderer`.
- Completed ASS parser support for `SecondaryColour`, `2c/2a/3c/3a/4c/4a`, `clip`, `iclip`, `k/kf/ko` and uppercase `K`.
- Completed run-level D2D subtitle rendering for D3D11/OpenGL with per-run fill/outline/shadow brushes and karaoke sweep highlight overlays.
- Added rectangular `clip` and basic rectangular `iclip` rendering by decomposing the inverse clip area into visible outer regions.
- Extended `--subtitle-style-check` output and added `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`.


### Modified Files
- `include/render/video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/subtitle/subtitle_parser.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/subtitle/srt_parser.cpp`
- `src/subtitle/subtitle_parser.cpp`
- `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`
- `docs/analysis/PLAYERCORE_DAY29_OPENGL_KARAOKE_CLIP_AND_SUBTITLE_CLOCK.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 93: ASS move/fad/fade animation converged into D3D11/OpenGL subtitle rendering

**Date**: 2026-03-24

### Problem Description
- The GPU subtitle path already supported style transforms, karaoke and clip, but still lacked a usable subset of ASS line animation semantics.
- `\move`, `\fad` and `\fade` were still a visible gap against mature players because subtitles remained spatially static and fully opaque once displayed.

### Root Cause Analysis
- The subtitle model only held style/run data and had no dedicated line-level animation container.
- Renderer-side animated subtitle detection only covered karaoke timing, so even if move/fade were parsed they would not trigger correct redraw behavior.
- D3D11/OpenGL brush generation had no item-level opacity modulation stage.

### Solution
- Added `SubtitleStyleAnimation` / `SubtitleFadeMode` and stored line-level animation on `SubtitleItem.animation`.
- Added ASS parser support for `\move`, `\fad` and `\fade`.
- Added subtitle-clock-based move interpolation and fade opacity evaluation in both D3D11 and OpenGL subtitle renderers.
- Extended animated subtitle detection to include move/fade and extended `--subtitle-style-check` output accordingly.
- Added `samples/subtitles/opengl_ass_animation_validation.ass`.


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_animation_validation.ass`
- `docs/analysis/PLAYERCORE_DAY30_ASS_MOVE_FADE_GPU_SUBTITLE_ANIMATION.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`


## Issue 94: ASS transform, vector drawing/clip, and font fallback groundwork converged into GPU subtitle rendering

**Date**: 2026-03-25

### Problem Description
- The subtitle pipeline had already covered style expansion, karaoke, clip, and move/fade, but the next practical ASS batch was still missing: `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector drawing, vector clip, and font fallback groundwork.
- During OpenGL runtime validation of this batch, the new vector clip path also exposed an internal Direct2D state-stack regression.

### Root Cause Analysis
- The subtitle model/parser could not yet carry transform-origin, projected-rotation, shear, vector drawing, and vector clip data through to the GPU renderers.
- The renderer-side subtitle path lacked geometry generation and masking logic for ASS drawing and vector clip semantics.
- The new clip-layer integration introduced mismatched `PushLayer()/PopLayer()` usage, which produced `D2DERR_PUSH_POP_UNBALANCED (0x88990016)` during `EndDraw()`.

### Solution
- Extended the subtitle model and ASS parser with transform, drawing, vector clip, and source-path fields.
- Added affine transform, vector drawing, and vector clip rendering to both D3D11 and OpenGL subtitle renderers.
- Added subtitle-sidecar/private font registration and fallback family-chain groundwork through `subtitle_font_registry`.
- Fixed the D2D layer-stack regression by removing the stray pre-emptive `PopLayer()` and restoring the missing `PopLayer()` in the subtitle box path across both renderers.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `build\Release\modern-video-player.exe --subtitle-style-check samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `build\Release\modern-video-player.exe --subtitle-sync-check samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `MVP_RENDERER_BACKEND=opengl build\Release\modern-video-player.exe --delay-adjust-check samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `MVP_RENDERER_BACKEND=d3d11 build\Release\modern-video-player.exe --delay-adjust-check samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- Results: all PASS


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_transform_vector_font_validation.ass`
- `docs/analysis/PLAYERCORE_DAY31_ASS_TRANSFORM_VECTOR_FONT_FALLBACK.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 99: Idle window startup and drag-drop playback
**Date**: 2026-03-25

### Problem Description
- Launching the exe without media arguments exited immediately instead of opening a usable player window.
- There was no drag-and-drop media open path for SDL, D3D11, or OpenGL playback windows.
- Dropping a new file during playback could not switch media because the app layer never received an open-file request.

### Root Cause
- The startup path required at least one CLI media input.
- Renderer event pumps exposed quit/seek/navigation requests but not file-drop requests.
- `PlayerCore` stopped playback for next/previous/quit, but had no buffered handoff for a validated media replacement flow.

### Solution
- Added idle-window mode for empty-start sessions.
- Added `consumeOpenFileRequest()` across the renderer interface and implementations.
- Buffered file-drop requests in `PlayerCore` and exposed them through `VideoPlayer`.
- Updated `main.cpp` to validate dropped paths before replacing playback and to return to idle mode for sessions launched without CLI media.

### Local Validation
- `cmake --build build --config Release --target modern-video-player`: PASS
- Manual GUI smoke test still pending for this turn.


### Modified Files
- `src/main.cpp`
- `src/display.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- `include/render/video_renderer.h`
- `include/display.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/player_core.h`
- `include/video_player.h`
- `docs/analysis/PLAYERCORE_DAY32_IDLE_WINDOW_AND_DRAG_DROP_PLAYBACK.md`

## Issue 100: OpenGL playback stutter caused by async present pacing
**Date**: 2026-03-25

### Problem Description
- OpenGL playback could visibly stutter on the user's AMD machine even when decoder, queue, and native-output counters still looked healthy.
- The existing diagnostics mostly reflected submit-side progress, not the moment a frame was actually shown by the OpenGL render thread.

### Root Cause Analysis
- OpenGL `present()` returned immediately after queueing work to the renderer thread, so `PlayerCore` advanced the video clock before on-screen presentation actually completed.
- A single pending-frame slot allowed silent replacement when the renderer thread lagged behind the scheduler.
- OpenGL forced `swap interval=0`, which made display pacing less stable than the D3D11 path.
- The native interop path also executed a per-frame `ID3D11DeviceContext::Flush()`.

### Solution
- Made OpenGL `present()` wait for the submitted frame to be displayed.
- Added submission/presentation IDs to synchronize scheduler pacing with actual OpenGL presentation.
- Changed OpenGL to prefer `swap interval=1`, with fallback to `0` only when needed.
- Removed the per-frame native interop `Flush()` call.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`
- Results: build PASS, diagnostics PASS, OpenGL native path PASS, OpenGL copy-back path PASS


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY33_OPENGL_PRESENT_PACING_AND_AMD_STUTTER.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
## Issue 101: OpenGL runtime diagnostics export and P010/P016 copy-back upload

**Date**: 2026-03-25

### Problem Description
- The new OpenGL runtime counters had been wired into the renderer, but still needed end-to-end verification through `--performance-log-check`.
- OpenGL software upload only accepted `yuv420p/nv12`, so 10-bit copy-back frames could still fall through an extra `swscale -> yuv420p` downgrade when native interop was disabled.

### Root Cause Analysis
- The machine-readable reporting path spans renderer diagnostics, `PlayerCore` diagnostics snapshot, and `main.cpp`, so the new keys had to be verified on the final CLI output instead of assuming the plumbing was complete.
- The OpenGL software upload path had no 16-bit semi-planar texture allocation/upload support.
- Software color coefficients only modeled 8-bit normalized sampling.

### Solution
- Verified and retained the `renderer_opengl_native_interop_*` / `renderer_opengl_present_wait_timeouts` exports in `--performance-log-check`.
- Added `AV_PIX_FMT_P010LE` and `AV_PIX_FMT_P016LE` direct upload support to the OpenGL renderer.
- Added 16-bit GL texture allocation/upload and 16-bit normalized color coefficient handling for the semi-planar software path.
- Extended the OpenGL color diagnostics to print the software input format during validation.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200`
- Results: all PASS; 10-bit forced copy-back path reported `video_copy_back_frames=72` and `video_swscale_frames=0`


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY34_OPENGL_RUNTIME_DIAGNOSTICS_AND_P010_UPLOAD.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 102: OpenGL present-mode override and gate script

**Date**: 2026-03-25

### Problem Description
- OpenGL present pacing had been fixed, but鐜板満鎺掗殰 still needed a supported way to switch between paced and immediate present behavior.
- The OpenGL regression commands for diagnostics/native/copy-back/10-bit/subtitle were still scattered and manual.

### Root Cause Analysis
- Swap interval selection was still effectively hard-coded in renderer startup.
- Diagnostics snapshots did not expose the requested and active OpenGL present mode.
- There was no single OpenGL-focused gate script comparable to the project's other validation helpers.

### Solution
- Added `MVP_OPENGL_PRESENT_MODE=auto|paced|immediate`.
- Added `[diag:opengl-present] requested=... active=...` startup logging.
- Exported `renderer_opengl_present_mode_requested` and `renderer_opengl_present_mode_active` in `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to run OpenGL diagnostics, native playback, copy-back playback, immediate present mode, 10-bit copy-back, and subtitle delay regression in one command.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_PRESENT_MODE=immediate .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: all PASS; gate script reported `OpenGL gate result: PASS`


### Modified Files
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY35_OPENGL_PRESENT_OVERRIDE_AND_GATE_SCRIPT.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 103: OpenGL HDR probe, quirk-table expansion, subtitle gate completion, and final gap matrix

**Date**: 2026-03-25

### Problem Description
- The OpenGL next-stage task table still had unfinished items around HDR output capability probing, quirk-table growth, subtitle sample gating, and a final mature-player gap summary.

### Root Cause Analysis
- Diagnostics previously stopped at adapter/decoder capability and did not reach the display-output layer.
- The quirk mechanism existed, but the rule table still covered only a very small set of known conditions.
- Subtitle sample validation was still spread across manual commands instead of a single OpenGL-focused gate.
- The remaining OpenGL gap against mature players was known informally, but not yet closed into one final backend-level matrix.

### Solution
- Added `opengl-diagnostics.hdr_output.*` fields to probe DXGI output color space and luminance state.
- Expanded the OpenGL quirk table with `device_id` / `subsystem_id` match capacity and additional software / virtual GPU context rules.
- Expanded `tools/run_opengl_checks.ps1` into a 10-step OpenGL gate including the ASS subtitle sample suite.
- Added a final OpenGL mature-player gap matrix and aligned the HDR design document with the implemented probe surface.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `opengl-diagnostics.result=PASS`, `opengl-diagnostics.hdr_output.probe_succeeded=true`, `OpenGL gate result: PASS`


### Modified Files
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/analysis/PLAYERCORE_DAY36_OPENGL_HDR_PROBE_QUIRK_TABLE_AND_GAP_MATRIX.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 104: OpenGL hotkey stall and missing control OSD

**Date**: 2026-03-25

### Problem Description
- The OpenGL playback path could stutter or appear frozen after hotkeys, especially on seek/volume-related input.
- The OpenGL window did not visibly expose the progress bar and volume bar unless a paused redraw happened, so the UI looked like it had no controls.

### Root Cause Analysis
- The OpenGL renderer consumed every `SDL_KEYDOWN`, including auto-repeat events, which could accumulate repeated control requests on one physical key press.
- Hotkey-triggered OSD visibility only called `requestRedrawIfPaused()`, so normal playback often did not repaint the OSD immediately.
- Unlike the D3D11 path, the OpenGL path had no mouse-driven progress/volume interaction loop wired into SDL events.

### Solution
- Suppressed repeated `SDL_KEYDOWN` handling in the OpenGL path to avoid hotkey request storms.
- Changed OpenGL hotkey OSD wakeup from paused-only redraw to unconditional `requestRedraw()`.
- Added startup OSD visibility, mouse-move OSD wakeup, and left-button progress/volume drag handling with seek preview and live volume updates.
- Refactored OpenGL control layout calculation so rendering and hit-testing use the same geometry.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `OpenGL gate result: PASS`
- Manual GUI smoke is still recommended for actual hotkey feel and control dragging.


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 105: ASS transform transition parser/runtime support

**Date**: 2026-03-25

### Problem Description
- The OpenGL Top 10 table was already closed, but a visible libass parity gap still remained: `\t(...)` style transitions were not parsed or executed.
- `--subtitle-style-check` could not expose machine-readable transition fields, so transition regressions were hard to verify.
- OpenGL and D3D11 subtitle paths still rendered these subtitles as static styles instead of time-varying transforms/colors/scales.

### Root Cause Analysis
- The ASS override parser still assumed simple argument extraction and did not handle nested parentheses or top-level comma splitting required by `\t(...)`.
- Subtitle runtime style resolution only consumed the base item/run style plus existing move/fade state; it had no transition interpolation stage.
- The OpenGL gate had no dedicated transition sample, so this semantic gap could survive without a targeted regression.

### Solution
- Added `SubtitleStyleTransition` plus transition property masks to the subtitle model and parser.
- Added nested-parenthesis matching and top-level comma splitting so `\t(...)` bodies parse correctly, including embedded `\clip(...)`.
- Added runtime style interpolation for supported transition fields across both OpenGL and D3D11 subtitle renderers.
- Extended `--subtitle-style-check` with `transition_count`, per-transition timing/accel/property names, and target-style dumps.
- Added `samples/subtitles/opengl_ass_transform_transition_validation.ass` and wired it into `tools/run_opengl_checks.ps1`.
- Fixed the MSVC `std::clamp` ambiguity in transition byte interpolation during final integration.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `$env:MVP_RENDERER_BACKEND='d3d11'; .\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `subtitle-style-check.result=PASS`, OpenGL `delay-adjust-check.result=PASS`, D3D11 `delay-adjust-check.result=PASS`, `OpenGL gate result: PASS`


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_transform_transition_validation.ass`
- `tools/run_opengl_checks.ps1`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/analysis/PLAYERCORE_DAY37_OPENGL_ASS_TRANSFORM_TRANSITION.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 106: OpenGL bottom-bar player chrome

**Date**: 2026-03-25

### Problem Description
- The OpenGL path only exposed a lightweight OSD with progress and volume rails, not a complete player-style control bar.
- There was no clickable play/pause button, no time text, and no hover-aware keep-visible/auto-hide behavior.
- The result still felt like a diagnostics overlay instead of a mature player UI.

### Root Cause Analysis
- `ControlLayout` only modeled `progress_track` and `volume_track`.
- `drawOsdOverlay()` only painted flat bars plus a center pause badge.
- Mouse handling only understood seek/volume drag and had no concept of play-button hit-testing or panel hover state.

### Solution
- Expanded the OpenGL control layout to include a bottom bar, play/pause button, time text region, and larger progress/volume hit boxes.
- Added geometry-based OpenGL UI drawing helpers for circles, triangles, and lightweight segmented time text rendering.
- Reworked OpenGL OSD drawing into a full bottom player chrome with top progress rail, play/pause button, current/total time text, and volume rail.
- Added hover state tracking so the bar stays visible while hovered and fades out automatically after idle playback.
- Wired play/pause button clicks into the existing request path and kept seek/volume drag behavior on the expanded layout.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `OpenGL gate result: PASS`
- Manual GUI smoke is still recommended for hover timing, hit testing, and visual spacing.


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY38_OPENGL_BOTTOM_BAR_PLAYER_CHROME.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 107: Container attachment font pipeline

**Date**: 2026-03-25

### Problem Description
- Subtitle font registration only covered sidecar font folders near external subtitle files.
- Container font attachments were ignored, so ASS/SSA subtitles that depended on MKV-attached fonts could still fall back to the wrong system font.
- There was no dedicated CLI regression for attachment extraction, registration, and cleanup.

### Root Cause Analysis
- `subtitle_font_registry` only scanned file-system directories and never consumed `AVMEDIA_TYPE_ATTACHMENT` streams from FFmpeg.
- `PlayerCore::open()` did not create any media-scoped subtitle font session after opening the demuxer.
- Because there was no attachment-specific diagnostics command, this gap could survive without a direct machine-readable test.

### Solution
- Extended `SubtitleFontRegistrationSummary` with attachment-specific extraction and registration fields.
- Added media-scoped attachment APIs in `subtitle_font_registry` to extract font payloads from container attachments into a temp cache and register them as private fonts.
- Wired `PlayerCore::open()` to register attachment fonts immediately after `demuxer_->open(...)`, and wired session release to unregister fonts and delete the cache.
- Added `--attachment-font-check <media_file>` for machine-readable validation.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8`
- `ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -c copy -attach C:\Windows\Fonts\arial.ttf -metadata:s:t mimetype=application/x-truetype-font -metadata:s:t:0 filename=attachment-font-check.ttf .\build\tmp\attachment-font-check.mkv`
- `.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv`
- Results: build PASS, `attachment-font-check.result=PASS`, extracted cache directory cleanup PASS


### Modified Files
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY39_ATTACHMENT_FONT_PIPELINE.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 108: Embedded subtitle-track playback

**Date**: 2026-03-25

### Problem Description
- After the attachment-font pipeline landed, a major remaining player gap was that muxed subtitle streams inside media containers still did not play automatically.
- Files with embedded ASS/SSA or text subtitles opened without subtitle output unless the user manually loaded a sidecar file.
- There was no dedicated machine-readable regression for embedded subtitle discovery, selection, and playback.

### Root Cause Analysis
- `PlayerCore` only owned one subtitle timeline fed from external subtitle parsing, so embedded subtitles had no first-class storage or fallback policy.
- `AssParser` and `SrtParser` did not expose a simple in-memory text entry point, which made reuse from demuxed container packets awkward.
- The project had no loader that scanned FFmpeg subtitle streams, selected a supported text track, and converted it into the existing `SubtitleItem` model.

### Solution
- Added `AssParser::parseText(...)` and `SrtParser::parseText(...)` so reconstructed container subtitle text can reuse the existing subtitle pipeline.
- Added `subtitle::loadBestEmbeddedSubtitleTrack(...)` with support for ASS/SSA plus plain-text subtitle codecs such as `subrip`, `text`, `mov_text`, and `webvtt`.
- Split `PlayerCore` subtitle ownership into external and embedded stores, with active selection rule `external > embedded`.
- Added `--embedded-subtitle-check <media_file>` for machine-readable validation.
- Expanded `tools/run_opengl_checks.ps1` to auto-generate embedded ASS and embedded `mov_text` samples and verify both CLI load and OpenGL playback.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8`
- `ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass -map 0:v -map 0:a? -map 1:0 -c:v copy -c:a copy -c:s ass .\build\tmp\embedded-ass-validation.mkv`
- `.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, embedded ASS check PASS, embedded text check PASS, `OpenGL gate result: PASS`


### Modified Files
- `include/subtitle/ass_parser.h`
- `src/subtitle/ass_parser.cpp`
- `include/subtitle/srt_parser.h`
- `src/subtitle/srt_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY40_EMBEDDED_SUBTITLE_TRACK_PLAYBACK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
## 闂 99: OpenGL 鎾斁鍚姩鏈熻 WASAPI 榛樿绔偣闃诲锛岃鍒や负娓叉煋鍗℃
**鏃ユ湡**: 2026-03-25

### 闂鎻忚堪
- 鐢ㄦ埛鍙嶉浣跨敤 OpenGL 閾捐矾鎾斁瑙嗛鏃讹紝澶х害 1-2 绉掑悗绋嬪簭浼氬崱姝汇€?- 淇鍓嶆棩蹇楁樉绀?`OpenGL renderer initialized` 涔嬪悗锛宍Audio output init failed, continuing with video-only playback` 浼氭櫄绾?8 绉掓墠鍑虹幇銆?- 鎺у埗鍙板悓鏃舵墦鍗?`WASAPI can't find requested audio endpoint: 鎵句笉鍒板厓绱犮€俙锛岃鏄庡崱鐐瑰彂鐢熷湪榛樿闊抽璁惧鍒濆鍖栵紝涓嶆槸 OpenGL render/present 姝婚攣銆?
### 鍘熷洜鍒嗘瀽
- `PlayerCore::open()` 鍦ㄤ富鎵撳紑璺緞閲屽悓姝ヨ皟鐢?`AudioPlayer::init()`銆?- `AudioPlayer::init()` 鐩存帴鎵ц `SDL_OpenAudioDevice(nullptr, ...)`锛岀瓑浠蜂簬璁?SDL/WASAPI 鎵撳紑绯荤粺榛樿杈撳嚭璁惧銆?- 鍦ㄥ綋鍓嶆満鍣ㄩ粯璁?render endpoint 缂哄け鏃讹紝`SDL_OpenAudioDevice` 浼氶樆濉炴暟绉掑悗鎵嶅け璐ャ€?- 鍥犱负闃诲鍙戠敓鍦ㄤ富绾跨▼锛岀敤鎴蜂細鎶婅繖娈电┖绐楄鍒や负 鈥淥penGL 鍗℃鈥濄€?
### 瑙ｅ喅鏂规
- Windows 涓嬨€佷笖 SDL 浠嶈蛋榛樿/`wasapi` 闊抽杈撳嚭鏃讹紝鍏堢敤 `IMMDeviceEnumerator` 鍋氶粯璁?render endpoint preflight銆?- 褰撻粯璁ょ鐐圭己澶变笖 active render endpoints 涔熶负 0 鏃讹紝鐩存帴璺宠繃 `SDL_OpenAudioDevice`锛岀珛鍒昏蛋鐜版湁 `video-only fallback`銆?- `DiagnosticsSnapshot` 鍜屾挱鏀剧被 CLI 鏂板锛?  - `audio_device_open_attempted`
  - `audio_init_latency_ms`
  - `audio_init_strategy`
  - `audio_init_detail`
- 杩欐牱鍚庣画鍙互鐩存帴浠庢鏌ヨ緭鍑哄尯鍒?鈥淥penGL 鍗′綇鈥?鍜?鈥滈煶棰戣澶囧垵濮嬪寲琚烦杩?澶辫触鈥濄€?
### 鏈湴楠屾敹
- Release build: PASS
- `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `audio_device_open_attempted=false`
  - `audio_init_strategy=skip-no-default-render-endpoint`
  - `audio_init_detail=skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI`
  - `result=PASS`
- `.\build\Release\modern-video-player.exe --opengl-diagnostics`
  - `probe_succeeded=true`
  - `native_interop.allowed=true`
  - `result=PASS`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'juren-30s.mp4'`
  - `OpenGL gate result: PASS`
  - `16/16 PASS`

### 淇敼鏂囦欢
- `include/audio_player.h`
- `src/audio_player.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `CMakeLists.txt`
- `docs/analysis/PLAYERCORE_DAY41_OPENGL_AUDIO_ENDPOINT_PREFLIGHT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 109: Embedded subtitle multi-track selection UI + CLI closure

**Date**: 2026-03-25

### Problem Description
- Embedded subtitle auto-load was already available, but multi-track selection was still not closed as a player feature.
- OpenGL bottom bar lacked subtitle-track switching controls.
- CLI lacked machine-readable list/select checks for embedded subtitle streams.

### Root Cause Analysis
- `PlayerCore` had embedded subtitle loading capability, but cross-layer control surface (renderer interaction + playback CLI + regression CLI) was incomplete.
- OpenGL control layout had placeholder fields for subtitle-track buttons, but draw/hit-test/request-consume links were not fully implemented.
- Existing diagnostics only covered "best-stream auto-load", not explicit stream listing/selection.

### Solution
- Completed OpenGL subtitle-track controls:
  - previous/next subtitle track buttons
  - subtitle track state text (`current / total`)
  - request consumption and PlayerCore event pump linkage
- Added renderer state push:
  - `setSubtitleTrackState(int current_ordinal, int track_count)`
- Added playback CLI:
  - `--subtitle-track <stream_index>`
- Added diagnostic CLI:
  - `--embedded-subtitle-list <media_file>`
  - `--embedded-subtitle-select-check <media_file> <stream_index>`
- Refined `PlayerCore` overlay-state count to supported selectable subtitle tracks (`supported_codec`) instead of raw subtitle stream count.

### Validation
- Build:
  - `cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && msbuild build\modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64"`
  - Result: PASS
- CLI:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-text-validation.mp4`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-text-validation.mp4 2`
  - Results: PASS
- Gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
  - Result: `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 113: Cross-platform master tasklist consolidation for direct execution
**Date**: 2026-03-25

### Problem Description
- Cross-platform tasks were split across multiple plan documents, making direct execution inefficient.
- There was no single document containing full task IDs, execution order, milestone gates, and acceptance criteria.

### Root Cause Analysis
- Existing docs were designed for different scopes (roadmap / refactor list / phase TODO), not a unified execution board.
- Recent progress changed baseline assumptions and required a consolidated up-to-date master list.

### Solution
- Added `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md` with:
  - phase-based task matrix
  - `CP-xxx` task IDs
  - `DONE/NEXT/LATER` status
  - milestones (`M1..M5`) and acceptance criteria
- Added analysis companion:
  - `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- Rebuilt `docs/plans/README.md` as clean index and set master tasklist as default execution entry.

### Validation
- `rg -n "CROSS_PLATFORM_MASTER_TASKLIST" docs/plans docs/analysis docs/records` -> PASS
- `cmake --build build --config Release --target modern-video-player` -> PASS


### Modified Files
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`








