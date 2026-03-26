# DEVELOP_LOG

## 索引说明（2026-03-26 编码清理批次）

- 本轮仅清理 `records/readme` 索引范围，不批量改写历史日志正文。
- 最新开发日志条目位于文件顶部（`Issue 127` 到 `Issue 122`）。
- 历史段落若出现旧编码乱码，将在后续专题批次逐步处理。

## Issue 127: Linux workflow Build Linux Release compile blocker closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Parsed failed Linux workflow runs and fixed current compile blockers in subtitle/OpenGL paths.
- Kept Windows path behavior unchanged and added non-Windows helper visibility where class code depends on shared symbols.

### Log
```text
Workflow failure inspection:
gh run view 23601824744 --job 68733664519 --log
gh run view 23601841417 --json status,conclusion,event,jobs
Result: Linux Build Linux Release failure reproduced and root causes identified.

Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

Performance regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS
```

### Notes
1. Linux runner evidence still requires push + GitHub Actions rerun.
2. Existing encoding warnings in unrelated files remain out of this patch scope.

## Issue 126: Workflow log FFmpeg duration compatibility closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Parsed `log` workflow error output and closed remaining FFmpeg-old compile compatibility gap.
- Added `AVFrame` duration compatibility fallback (`duration` / `pkt_duration`) and updated decode timing reads.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS
```

### Notes
1. This closes the unresolved compile error subset from the provided workflow `log` file.
2. Final Linux runtime gate PASS still requires remote CI run after push.
## Issue 125: Linux CI compatibility stabilization (FFmpeg/libass + workflow determinism)

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Linux CI compatibility regressions found after previous cross-platform closure.
- Landed FFmpeg channel-layout compatibility migration, Linux compile blocker fixes, and workflow-level deterministic gate inputs.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)
```

### Notes
1. `CP-101 ~ CP-106` remain complete; this round is Linux CI compatibility/stability closure, not scope expansion.
2. macOS items `CP-1001 ~ CP-1005` stay deferred by scope decision.
3. Final Linux runtime PASS evidence still requires CI/Linux runner execution after push.
## Issue 124: Linux gate reporting/artifact closure for CI evidence reuse

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Continued Linux cross-platform closure by adding reusable Linux gate evidence artifacts.
- Added machine-readable report output in Linux gate script and CI log/report archival wiring.

### Log
```text
Linux gate script syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)

Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS
```

### Notes
1. Linux gate now supports a structured report output file (`MVP_LINUX_GATE_REPORT_FILE` / arg #10).
2. Linux CI lane now archives both raw log and summary report artifacts.
3. Full Linux runtime evidence still requires Linux host/CI run.

## Issue 123: Linux gate strict optional checks closure for CP-507/CP-508 in CI

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Continued Linux cross-platform closure work by hardening Linux gate determinism for subtitle backlog checks.
- Added CP-508 fixture auto-generation and strict optional-check mode into Linux gate script.
- Updated CI Linux lane to install `ffmpeg` and enforce strict mode execution.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS (syntax check)

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)
```

### Notes
1. Linux runtime proof still requires real Linux host or CI run results (syntax check alone is not runtime evidence).
2. CI input contract is now strict enough to prevent CP-507/CP-508 silent skips.

## Issue 122: CP-507 and CP-508 Linux subtitle backlog closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed remaining Linux subtitle backlog items in the cross-platform tasklist:
  - `CP-507` (`libass` shaping/layout probe baseline)
  - `CP-508` (embedded subtitle live packet probe baseline)
- Added CLI contracts and Linux gate-script integration points.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

CP-507 command:
.\build\Release\modern-video-player.exe --libass-shaping-check .\samples\subtitles\opengl_ass_style_validation.ass
Result: FAIL on this host (expected, platform=NonLinux)

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS
```

### Notes
1. Linux runtime evidence is still required for `--libass-shaping-check` and full Linux gate script execution.
2. Packet-level probe output for embedded ASS stream was validated locally (`stream_index=2`, `subtitle_packets_read=3`, `produced_output=true`).
# 寮€鍙戞棩蹇?
## Issue 121: CP-801 and CP-901 ~ CP-905 HDR present / observability / CI closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed the remaining Phase-8 item `CP-801` on the real D3D11 DXGI HDR present path.
- Closed Phase-9 tasks `CP-901` to `CP-905` with driver sample capture, Linux gate parity, CI matrix wiring, and Windows packaging automation.
- Synced corresponding analysis/design/plan/report docs and updated the cross-platform master task list.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

D3D11 HDR output check:
.\build\Release\modern-video-player.exe --d3d11-hdr-output-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

Performance log check:
$env:MVP_RENDERER_BACKEND='d3d11'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

Driver quirk sample capture:
powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -OutputCsvPath .\docs\reference\DRIVER_QUIRK_SAMPLE_LIBRARY.csv -HostLabel local-win-gtx1080 -Notes "Local Windows validation sample after CP-801 and Phase-9 closure"
Result: PASS (2 records)

Windows packaging:
powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release -SkipBuild
Result: PASS

Linux gate script syntax check:
bash -n tools/run_linux_mvp_checks.sh
Result: could not run because local workstation does not have WSL installed.
```

### Notes
- Local Windows validation host did not expose a bindable DXGI output, so HDR-active present was not observed locally.
- The new CLI/runtime diagnostics still verified explicit inactive decision output and present timing counters.
- Linux gate and CI workflow were authored but could not be executed end-to-end on this Windows workstation.
# 瀵偓閸欐垶妫╄箛?

## Issue 120: CP-501 ~ CP-506 subtitle/font platform closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Phase-5 tasks `CP-501` to `CP-506` end-to-end.
- Fixed policy integration compile blocker in `main.cpp`.
- Added Linux attachment-font registration/rebuild flow in `subtitle_font_registry` using `fontconfig`.
- Added DirectWrite custom collection check coverage into `tools/run_opengl_checks.ps1`.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release
Result: PASS

Phase5 CLI checks:
.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid
.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4
.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --settings-persistence-check
Result: PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"
Result: OpenGL gate result: PASS (18/18)
```

### Analysis Notes
1. `CP-505` Linux path is now implemented in code, but this Windows host cannot provide runtime validation for the new `fontconfig` branch.
2. `CP-506` gate maturation is now complete on Windows path (`--directwrite-font-collection-check` included in scripted gate).
3. Phase-4 report placeholders were replaced by actual local execution results and platform-expectation notes.

## Issue 119: CP-401 ~ CP-406 Linux MVP playback closure and gate commands

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Phase-4 Linux MVP playback tasks `CP-401` to `CP-406`.
- Added the full Linux MVP command surface and Linux gate baseline script.
- Added controlled OpenGL init failure injection so fallback-chain validation is deterministic.

### Log Output
```text
Build:
cmake -S . -B build
cmake --build build --config Release --target modern-video-player
configure/build succeeded

Windows-side validation of shared runtime gates:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
performance-log-check.result=PASS

.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
renderer-fallback-check.result=PASS

.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
interaction-freeze-check.result=PASS
```

### Analysis
1. Phase-4 closure needed explicit Linux-only CLI commands; otherwise the task list could say done while automation still had no stable command contract.
2. Fallback verification without a deterministic failure trigger is not a real gate, because it depends on incidental runtime conditions.
3. The Linux gate script had to exist even before a Linux host was available locally, otherwise CI could not assume one shared Phase-4 entry point.
4. This workstation can validate command wiring and shared playback checks, but cannot provide the missing Linux runtime evidence by itself.

### Result
- Added Phase-4 commands in `src/main.cpp`:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added `MVP_OPENGL_FORCE_INIT_FAIL` handling in `src/render/opengl_video_renderer.cpp` to force OpenGL init failure for fallback validation.
- Added `tools/run_linux_mvp_checks.sh` as the Phase-4 Linux MVP gate script baseline.
- Updated Phase-4 analysis/design/report docs and marked `CP-401 ~ CP-406` done in the master task list.

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
**Status**: Resolved

### Description
- Completed Phase-3 baseline tasks `CP-301` to `CP-305`.
- Added explicit backend feature switches and platform guard enforcement in CMake.
- Added startup diagnostics compiled/runtime set fields and Linux packaging baseline (`DEB/TGZ` + helper script).

### Log
```text
Default configure/build:
cmake -S . -B build
cmake --build build --config Release --target modern-video-player
Result: PASS

Default runtime regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: PASS
Key lines:
- performance-log-check.startup_renderer_compiled_set=D3D11, SoftwareSDL, OpenGL
- performance-log-check.startup_renderer_runtime_set=D3D11, SoftwareSDL, OpenGL
- performance-log-check.startup_decoder_compiled_set=D3D11VA, Software
- performance-log-check.startup_decoder_runtime_set=D3D11VA, Software

Switch matrix build check:
cmake -S . -B build_nod3d11 -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON
cmake --build build_nod3d11 --config Release --target modern-video-player
Result: PASS

cmake -S . -B build_noopengl -DENABLE_OPENGL_RENDERER=OFF -DENABLE_D3D11_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_D3D11VA=ON
cmake --build build_noopengl --config Release --target modern-video-player
Result: PASS

Command guard behavior:
.\build_nod3d11\Release\modern-video-player.exe --d3d11-diagnostics
-> supported_platform=false (expected fallback path)
.\build_noopengl\Release\modern-video-player.exe --opengl-diagnostics
-> supported_platform=false (expected fallback path)
```

### Notes
1. Backend source/link composition is now explicit and switch-controlled; no more implicit always-on backend compilation.
2. Linux dependency and package baseline are codified in CMake + script, but Linux native package execution is still pending Linux environment verification.
3. `build_nod3d11` software-decode runtime path still exposes an existing blocker unrelated to CP-301~305 scope.

## Issue 117: CP-201 ~ CP-205 renderer/input/overlay cleanup and interaction freeze regression gate

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Completed Phase-2 cleanup tasks `CP-201` to `CP-205`.
- Split renderer/input/overlay responsibilities into explicit interfaces and rewired `PlayerCore`/idle window paths to consume role interfaces.
- Added main-thread event-pump guards and landed machine-readable interaction freeze stress check with OpenGL gate integration.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Runtime regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.renderer_backend=D3D11
- performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.result=PASS

Renderer fallback:
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
Key lines:
- renderer-fallback-check.renderer_backend=SoftwareSDL
- renderer-fallback-check.fallback_to_sdl=true
- renderer-fallback-check.result=PASS

Interaction freeze stress:
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
Key lines:
- interaction-freeze-check.injected_events_total=251
- interaction-freeze-check.render_progress_ok=true
- interaction-freeze-check.illegal_transition_ok=true
- interaction-freeze-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- [5/17] OpenGL interaction freeze stress
- OpenGL gate result: PASS
```

### Notes
1. `PlayerCore` now treats input polling and overlay updates as independent role interfaces rather than renderer-control methods.
2. Event-pump affinity violations are now fail-safe (warn-once + ignore), reducing deadlock/freeze regression risk.
3. Phase-2 cleanup baseline is closed; next execution target is `CP-301+` build/dependency platformization.

## Issue 116: CP-101 ~ CP-106 startup strategy/capability extraction

**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed Phase-1 startup strategy extraction tasks `CP-101` to `CP-106`.
- Introduced platform capability probe + playback strategy planning and moved `PlayerCore` open path to plan execution.
- Added machine-readable startup strategy diagnostics export.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Startup diagnostics regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.startup_platform=Windows
- performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_decoder_candidates=D3D11VA -> Software
- performance-log-check.startup_selected_renderer=D3D11
- performance-log-check.startup_selected_decoder=D3D11VA
- performance-log-check.startup_renderer_fallback_reason=none
- performance-log-check.startup_decoder_fallback_reason=none
- performance-log-check.result=PASS

Renderer fallback regression:
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
Key lines:
- renderer-fallback-check.renderer_backend=SoftwareSDL
- renderer-fallback-check.fallback_to_sdl=true
- renderer-fallback-check.result=PASS
```

### Notes
1. `RendererFactory` no longer owns default backend policy; strategy ordering now lives in `PlaybackStrategy`.
2. `DecoderFactory` ordering now consumes context + capabilities and always keeps software fallback.
3. This closes Phase-1 strategy extraction baseline while preserving Windows default behavior.

## Issue 115: Cross-platform Phase 8 ICC output LUT and per-display diagnostics closure
**Date**: 2026-03-26
**Status**: Resolved (`CP-802 ~ CP-805`)

### Description
- Closed the Phase 8 runtime color-management gaps except for the real DXGI HDR present bridge (`CP-801`).
- Added ICC/profile-driven 3D LUT generation, per-display output binding, runtime reload, and machine-readable diagnostics.
- Added dedicated auto ICC regression coverage in the OpenGL gate.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

Manual output color:
.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200
Key lines:
- output_lut_source=cube
- output_lut_active=true
- output_display_device_name=WinDisc
- result=PASS

Auto ICC output color:
.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- output_lut_source=icc-display
- output_icc_profile_available=true
- output_icc_profile_path=C:\Windows\system32\spool\drivers\color\sRGB Color Space Profile.icm
- output_icc_profile_description=sRGB IEC61966-2.1
- result=PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Key lines:
- output_display.binding_succeeded=true
- output_display.icc_profile_available=true
- output_display.icc_profile_source=display-auto
- result=PASS

OpenGL gate:
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
Result: OpenGL gate result: PASS (25/25)
```

### Analysis Notes
1. Day44 already had the policy surface; the real missing work was runtime binding and observability.
2. The current display binding needed to be modeled first, otherwise auto ICC and future HDR present work would stay static and misleading.
3. Manual `.cube` override remains first priority so the new ICC path does not break existing workflows.
4. `CP-801` remains open because a real DXGI HDR present bridge is still a separate rendering/present integration task, not a diagnostics task.

### Files
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
**Status**: Resolved

### Description
- Closed `CP-601 ~ CP-605` by finishing packet-level bitmap subtitle modeling, cache reuse, and dedicated regression coverage.
- Added real DVD / PGS bitmap subtitle validation plus synthetic multi-rect stress validation.
- Fixed the real PGS sample case where invalid display-window metadata inflated bitmap subtitle duration.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

Bitmap subtitle CLI:
.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv
Key lines:
- codec_name=dvd_subtitle
- bitmap_item_count=1
- bitmap_rect_count=1
- mismatches=0
- result=PASS

.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv
Key lines:
- codec_name=hdmv_pgs_subtitle
- bitmap_item_count=1
- bitmap_rect_count=1
- ordered_checks=18
- mismatches=0
- result=PASS

Synthetic stress:
.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check
Key lines:
- multi_rect_item_count=2
- cache_reuse_candidate_count=2
- max_active_item_count=3
- max_active_rect_count=5
- result=PASS

OpenGL gate:
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
Result: OpenGL gate result: PASS (23/23)
```

### Analysis Notes
1. The existing bitmap subtitle baseline was real but incomplete; Phase 6 closure required modeling, cache, and validation to converge together.
2. Packet-level aggregation removes the old one-rect-per-item mismatch and makes multi-rect composition measurable.
3. Real PGS samples can surface invalid display timing metadata; fallback hardening is necessary for stable runtime behavior.
4. Linux host runtime evidence for bitmap subtitle playback is still needed before claiming full cross-platform parity.

### Files
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
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰鎶婁笂涓€杞缓璁殑涓夋潯鍚庣画宸ヤ綔鍏ㄩ儴钀藉湴锛?  1. `libass/mpv` 绾у樊璺濇竻鍗?  2. 鏄剧ず绾?HDR 杈撳嚭璁捐
  3. OpenGL quirk/diagnostics 姝ｅ紡鍖?- 鐩爣涓嶆槸缁х画鍫嗛浂鏁ｆ棩蹇楋紝鑰屾槸璁?OpenGL 鍚庣鍚屾椂鍏峰锛?  - 鏄庣‘鐨勬垚鐔熸挱鏀惧櫒宸窛杈圭晫
  - 缁撴瀯鍖栫殑鍚姩鏈熻瘖鏂叆鍙?  - 瀵?HDR 杈撳嚭鐪熸鍙墽琛岀殑鍚庣画璁捐

### 鏃ュ織杈撳嚭
```text
Build:
cmake --build build --config Release
modern-video-player.exe build succeeded

OpenGL diagnostics:
./build/Release/modern-video-player.exe --opengl-diagnostics
opengl-diagnostics.probe_succeeded=true
opengl-diagnostics.gl_vendor=NVIDIA Corporation
opengl-diagnostics.gl_renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2
opengl-diagnostics.has_wgl_dx_interop=true
opengl-diagnostics.native_interop.allowed=true
opengl-diagnostics.result=PASS

OpenGL diagnostics with env disable:
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
./build/Release/modern-video-player.exe --opengl-diagnostics
opengl-diagnostics.native_interop.env_override=disable
opengl-diagnostics.native_interop.allowed=false
opengl-diagnostics.native_interop.disable_rule=env_disable
opengl-diagnostics.result=PASS

OpenGL playback regression:
$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000
[diag:opengl-native] ... has_wgl_dx_interop=true
[diag:opengl-native] ... hard_blocker_matched=false quirk_rule_matched=false
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS

Subtitle regression:
./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass
subtitle-sync-check.mismatches=0
subtitle-sync-check.result=PASS

$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass
delay-adjust-check.subtitle_loaded=true
delay-adjust-check.probe_found=true
delay-adjust-check.result=PASS
```

### 鍒嗘瀽璁板綍
1. `ASS/SSA` 宸窛濡傛灉涓嶆媶鍒?`shaping / karaoke / vector / fallback` 绾у埆锛屽悗缁案杩滀細鍋滅暀鍦ㄢ€滆繕宸緢澶氣€濈殑绌鸿瘽銆?2. OpenGL 閲岀幇鍦ㄥ凡鏈?HDR aware 澶勭悊锛屼絾鏄剧ず绾?HDR 杈撳嚭涓?renderer 鍐呴儴 tone-map 涓嶆槸涓€鍥炰簨锛屽繀椤绘媶寮€璁捐銆?3. 鐪熸鎴愮啛鐨勫惎鍔ㄦ湡璇婃柇锛屼笉搴斾緷璧栬繘鎾斁鍣ㄥ悗鐪嬫棩蹇楋紝鑰岃鑳藉儚 `--d3d11-diagnostics` 涓€鏍风嫭绔嬫墽琛屻€佹満鍣ㄥ彲璇汇€佸拰瀹為檯鍐崇瓥鍏变韩鍚屼竴濂楄鍒欐簮銆?4. `force` 涓嶅簲璇ュ啀鏄€滄棤鑴戝己寮€鈥濓紱鏈疆鎶婂畠鏀舵暃鎴愨€滃彲瑕嗙洊 quirk锛屼笉瑕嗙洊 hard blocker鈥濈殑杈圭晫銆?
### 澶勭悊缁撴灉
- OpenGL 鏂板 `OpenGLDiagnosticsSnapshot` 涓?`OpenGLVideoRenderer::probeSystemDiagnostics()`銆?- `main` 鏂板 `--opengl-diagnostics`锛屽彲鐙珛杈撳嚭锛?  - GL vendor / renderer / version
  - `WGL_NV_DX_interop` 鍙敤鎬?  - D3D11 閫傞厤鍣ㄥ拰鍏抽敭鏍煎紡鑳藉姏
  - `hard blocker / quirk rule / env override` 缁撴灉
- OpenGL native interop 鍚姩鏈熷喅绛栨敹鏁涗负锛?  - `hard blocker`
  - `quirk rule`
  - `env override`
- 鏂板 `PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`锛屾槑纭?libass 宸窛涓?quirk 鏈哄埗銆?- 鏂板 `OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`锛屾槑纭樉绀虹骇 HDR 杈撳嚭瑕佽蛋 `DXGI swapchain + HDR metadata + ICC/3D LUT` 鍙岀粓绔璁°€?- `OPENGL_RENDERER_LOCAL_CHECK.md` 宸叉洿鏂颁负鍖呭惈 `--opengl-diagnostics` 涓庢柊缁撹鐨勬湰鍦伴獙鏀舵姤鍛娿€?
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

## 闂 97: OpenGL ASS/SSA銆乨river quirk 鏀舵暃涓?HDR/鑹插僵绠＄悊琛ラ綈

**鏃ユ湡**: 2026-03-24
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰缁х画鎶?`OpenGL` 鍚戞垚鐔熸挱鏀惧櫒鏂瑰悜鎺ㄨ繘锛屼紭鍏堣ˉ榻?`ASS/SSA`銆乨river quirk 鏀舵暃鍜?`HDR / 鑹插僵绠＄悊`銆?- 鐩爣涓嶆槸鍐嶅姞涓€鏉♀€滆兘鎾€濈殑澶囩敤閾捐矾锛岃€屾槸鎶婂綋鍓?Windows `OpenGL` 鍚庣鎺ㄥ埌鏇存帴杩戞垚鐔熸挱鏀惧櫒鐨?`M1` 褰㈡€侊細瀛楀箷鑳芥壙鎺ユ牱寮忋€乶ative interop 鏈夊惎鍔ㄦ湡绛栫暐銆佽壊褰╅摼璺湁娓呮櫚璇婃柇銆?
### 鏃ュ織杈撳嚭
```text
Release build:
cmake --build build --config Release
modern-video-player.exe build succeeded

OpenGL playback check:
$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000
[diag:opengl-native] adapter="NVIDIA GeForce GTX 1080" driver_version=32.0.15.6094 native_direct_allowed=true rule=none
[diag:opengl-native] decoder_profiles h264_any=true hevc_any=true vp9_any=true av1_any=false
[diag:opengl-color] path=native matrix=bt709 range=unspecified colorspace=unspecified transfer=unspecified primaries=unspecified hdr=false tone_map=off gamut_map=off
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.render_frames=47
performance-log-check.result=PASS

Subtitle parser check:
./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass
subtitle-sync-check.entries=2
subtitle-sync-check.mismatches=0
subtitle-sync-check.result=PASS

Subtitle playback check:
$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass
delay-adjust-check.subtitle_loaded=true
delay-adjust-check.subtitle_entries=2
delay-adjust-check.probe_found=true
delay-adjust-check.result=PASS
```

### 鍒嗘瀽璁板綍
1. 浠呴潬鈥淥penGL 宸茶兘鏄剧ず瑙嗛鈥濊繕涓嶅銆傛垚鐔熸挱鏀惧櫒鐨勫樊寮備富瑕佸湪浜庯細瀛楀箷涓嶆槸鍗曠函鏂囨湰鍙犲瓧銆乶ative interop 涓嶆槸鍙潬杩愯鏈熺偢浜嗗啀鍥為€€銆佽壊褰╅摼璺篃涓嶆槸榛戠洅銆?2. 杩欒疆浼樺厛澶嶇敤鐜版湁 `SubtitleItem / SubtitleTextRun` 鏁版嵁缁撴瀯锛岃€屼笉鏄啀閫犱竴濂?OpenGL 涓撶敤瀛楀箷妯″瀷锛涜繖鏍?`D3D11` 涓?`OpenGL` 鍦ㄥ瓧骞曡涔変笂鍙互缁х画瀵归綈銆?3. `HDR / 鑹插僵绠＄悊` 杩欎竴杞彧鍋氬埌 `M1`锛氳ˉ榻愮煩闃?transfer/gamut 鎰熺煡銆佸熀纭€ tone-map 鍜?`BT.2020 -> BT.709`锛屽苟鏄惧紡杈撳嚭璇婃柇锛涙病鏈夋妸鐩爣铏氭姤鎴愨€滃畬鏁?HDR 鏄剧ず杈撳嚭鈥濄€?4. OpenGL native interop 鐨?quirk 鏈哄埗褰撳墠淇濇寔淇濆畧锛屽彧鍏堟敹鏁涙渶鏄庣‘鐨勮蒋浠?GL 鍦烘櫙锛屽苟棰勭暀鐜鍙橀噺寮哄埗寮€鍏冲拰鍚庣画 blacklist 鎵╁睍浣嶃€?
### 澶勭悊缁撴灉
- `OpenGLVideoRenderer` 鐜板凡鏀寔 `ASS/SSA` item/run 绾у瓧骞曟覆鏌擄細`DirectWrite + D2D offscreen -> BGRA texture -> OpenGL overlay`銆?- 瀛楀箷娓叉煋鏀寔锛氬 `SubtitleItem` 鎺掑簭銆乻tyle run銆佸畾浣?瀵归綈銆佽儗鏅銆佹弿杈广€侀槾褰卞拰濉厖锛屽彔鍔犻樁娈垫敼涓?premultiplied alpha 娣峰悎銆?- OpenGL 鍚姩鏈熸柊澧?native interop 绛栫暐灞傦細
  - `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`
  - `Microsoft / GDI Generic` 杞欢 GL blacklist
  - `[diag:opengl-native]` 閫傞厤鍣ㄣ€侀┍鍔ㄣ€乸rofile 涓庤鍒欐棩蹇?- 杞欢涓婁紶鍜屽師鐢熶簰鎿嶄綔涓ゆ潯鑹插僵閾捐矾缁熶竴鎺ュ叆锛?  - `BT.601 / BT.709 / BT.2020` 鐭╅樀
  - `PQ / HLG` 妫€娴?  - 鍩虹 SDR tone-map
  - `BT.2020 -> BT.709` gamut mapping
  - `[diag:opengl-color]` 璇婃柇鏃ュ織
- 鏂板 `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md`锛屽苟鏇存柊 OpenGL 鏈湴楠屾敹鎶ュ憡涓?records 鏂囨。銆?
### 淇敼鏂囦欢
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 闂 96: OpenGL 娓叉煋閾捐矾 M0 钀藉湴骞跺畬鎴愭湰鍦伴獙鏀?
**鏃ユ湡**: 2026-03-24
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰缁х画琛ラ綈 `OpenGL` 鎾斁閾捐矾锛屽苟纭瀹冧笌 `mpv / MPC-HC` 杩欑被鎴愮啛鎾斁鍣ㄧ浉姣旇繕宸灏戙€?- 褰撳墠浠撳簱涓?`OpenGLVideoRenderer` 浠嶆槸 stub锛岄渶瑕佹妸瀹冭ˉ鎴愬彲杩愯鐨勬渶灏忓彲鐢ㄥ悗绔紝鑰屼笉鏄户缁仠鐣欏湪鍚嶄箟閫夐」銆?
### 鏃ュ織杈撳嚭
```text
Release build:
cmake --build build --config Release
modern-video-player.exe build succeeded

OpenGL validation:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
OpenGL context: vendor=NVIDIA Corporation renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2 version=4.6.0 NVIDIA 560.94
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.result=PASS
```

### 鍒嗘瀽璁板綍
1. 鏃у疄鐜板彧鏈夊伐鍘傞€夋嫨鍏ュ彛锛屾病鏈夌獥鍙ｃ€佷笂涓嬫枃銆乻hader銆佺汗鐞嗕笂浼犱笌浜嬩欢娉碉紝鍥犳鏈川涓婁笉鍏峰瀹為檯鎾斁鑳藉姏銆?2. 杩欐鍏堣惤鍦?`OpenGL M0`锛岀洰鏍囨槸璁?`YUV420P / NV12` 杞欢甯у彲浠ョǔ瀹氭樉绀猴紝鍚屾椂淇濇寔澶辫触鑷姩鍥為€€锛涗笉鍦ㄨ繖涓€杞彁鍓嶅仛瀛楀箷/OSD GPU 鍙犲姞銆?3. 楠屾敹缁撴灉宸茬粡璇佹槑 `OpenGL` 涓嶅啀鏄?stub锛屼絾瀹冧粛涓嶆槸鎴愮啛鎾斁鍣ㄧ骇 GPU 鍚庣锛屽綋鍓嶅畾浣嶅簲鏄庣‘涓?opt-in 杩囨浮璺緞銆?
### 澶勭悊缁撴灉
- `OpenGLVideoRenderer` 鐜板凡鍏峰 SDL OpenGL 涓婁笅鏂囥€丟LSL 120 鑹插僵杞崲銆乊UV420P/NV12 涓婁紶涓庡熀纭€閿洏浜嬩欢澶勭悊銆?- `CMakeLists.txt` 宸茶ˉ `opengl32 / OpenGL::GL` 閾炬帴渚濊禆銆?- 鏂板 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`锛屽苟鍚屾 `guides / analysis / README / records` 鏂囨。鍙ｅ緞銆?
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
# 瀵偓閸欐垶妫╄箛?
## 闂傤噣顣?95: RC 閻楀牊婀伴崗鍐╂殶閹诡喓鈧阜elease 妞ゅ灚顒滈弬鍥︾瑢鐎瑰顥婇崠鍛閺堫剚鐖ｇ拠鍡毸夋?
**閺冦儲婀?*: 2026-03-23
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涙潻浠嬫６閳ユ珐elease 妞ゅ灚顒滈弬鍥ф躬閸濐亪鍣烽垾婵撶礉楠炴儼顩﹀Ч鍌涘Ω缁嬪绨崘鍛村劥閻楀牊婀伴妴涔刬ndows 閸欘垱澧界悰灞炬瀮娴犲墎澧楅張顒€鎷扮€瑰顥婇崠鍛閺堫剟鍏樼悰銉﹀灇 `1.0.0-rc1`閵?- 鏉╂瑨鐤嗛惄顔界垼娑撳秵妲哥紒褏鐢婚弨瑙勬尡閺€楣冩懠閿涘矁鈧本妲搁幎?RC 閻楀牊婀伴弽鍥槕娴犲孩鏋冨锝呯湴鐞涖儵缍堥崚?CLI閵嗕笒XE 鐏炵偞鈧佲偓涓燭TP UA 閸滃苯褰傜敮鍐ㄥ瘶娴溠呭⒖鐏炲倶鈧?
### 閺冦儱绻旀潏鎾冲毉
```text
Version CLI:
build\Release\modern-video-player.exe --version
Modern Video Player 1.0.0-rc1

Windows file version:
FileVersion=1.0.0.0
ProductVersion=1.0.0-rc1
ProductName=Modern Video Player

Package:
cmake --build build --config Release --target PACKAGE
CPack: - package: D:/VSProject/sssssssssssssss/modern-video-player/build/modern-video-player-1.0.0-rc1-windows-x64.zip generated.

Package entries:
modern-video-player-1.0.0-rc1-windows-x64/modern-video-player.exe
modern-video-player-1.0.0-rc1-windows-x64/plugins/sample_logger_plugin.dll
modern-video-player-1.0.0-rc1-windows-x64/RELEASE_NOTES.md
modern-video-player-1.0.0-rc1-windows-x64/SDL2.dll
modern-video-player-1.0.0-rc1-windows-x64/avcodec-62.dll
...
config/player_settings.ini absent

Diagnostics smoke:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. `V1_0_0_RC1_RELEASE_READINESS.md` 閺勵垰鍞撮柈銊ュ絺鐢啰绮ㄧ拋鐚寸礉娑撳秶鐡戞禍搴″讲閻╁瓨甯寸拹鏉戝煂 GitHub Release 妞ょ數娈戝锝嗘瀮閿涘苯娲滃銈夋付鐟曚礁宕熼悪顒兯夋稉鈧禒?`Release Notes` 閺傚洦銆傞妴?2. 閻楀牊婀伴崣宄邦洤閺嬫粌褰х€涙ê婀禍?`project(... VERSION 1.0.0)`閿涘瞼鈻兼惔?CLI閵嗕箘indows 鐠у嫭绨妴浣稿竾缂傗晛瀵橀崥宥呮嫲 HTTP `user_agent` 娴兼氨鎴风紒顓炲瀻鐟佸偊绱漅C 閻滄澘婧€閹烘帡娈伴崪宀€澧楅張顒冪槕閸掝偅鍨氶張顒€绶㈡妯糕偓?3. Windows 閺傚洣娆㈤悧鍫熸拱閸滃奔楠囬崫浣哄閺堫剙绨茬拠銉ュ隘閸掑棴绱癭FileVersion` 缂佸瓨瀵旈崶娑欘唽閺佹澘鈧?`1.0.0.0`閿涘畭ProductVersion`/鐎电懓顦婚悧鍫熸拱閸氬秳濞囬悽?`1.0.0-rc1`閵?4. 閸欐垵绔烽崠鍛箑妞ゅ浼╅崗宥嗗ⅵ閸忋儲婀伴崷鎷屽壈閻?`config/player_settings.ini`閿涘苯鎯侀崚娆庨獓閻椻晙绱板ǎ宄板弳瀵偓閸欐垶婧€閺堫剙婀撮柊宥囩枂閿涙稒婀版潪顔藉ⅵ閸栧懎鍑￠弰鎯х础妤犲矁鐦夌拠銉︽瀮娴犺埖婀潻娑樺弳 ZIP閵?
### 婢跺嫮鎮婄紒鎾寸亯
- 閺傛澘顤?`docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`閿涘奔缍旀稉?Release 妞ゅ灚顒滈弬鍥ㄦ降濠ф劑鈧?- `CMake` 瀵洖鍙嗙紒鐔剁閻楀牊婀板┃鎰剁礉楠炲墎鏁撻幋?`mvp_version.h` 娑?`version_info.rc`閵?- `main` 閺傛澘顤?`--version`閿涙矖http_stream_downloader` 閻?`user_agent` 閺€閫涜礋 `modern-video-player/1.0.0-rc1`閵?- 閺傛澘顤?`CPack ZIP` 閹垫挸瀵樼憴鍕灟閿涘奔楠囬悧鈺佹倳閸ュ搫鐣炬稉?`modern-video-player-1.0.0-rc1-windows-x64.zip`閿涘苯鑻熺亸?`RELEASE_NOTES.md` 閹垫挸鍙嗛崠鍛敶閵?- `docs/README.md`閵嗕梗docs/reports/README.md` 娑?`V1_0_0_RC1_RELEASE_READINESS.md` 瀹歌尪藟閸?Release Notes 閸忋儱褰涢妴?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `build\Release\modern-video-player.exe --version`閿涙岸鈧俺绻冮妴?- Windows `FileVersionInfo`閿涙瓪FileVersion=1.0.0.0`閵嗕梗ProductVersion=1.0.0-rc1`閵嗕梗ProductName=Modern Video Player`閵?- `cmake --build build --config Release --target PACKAGE`閿涙岸鈧俺绻冮敍宀€鏁撻幋?`build/modern-video-player-1.0.0-rc1-windows-x64.zip`閵?- ZIP 瀹歌尙鈥樼拋銈呭瘶閸?`modern-video-player.exe`閵嗕椒绶风挧?DLL閵嗕梗plugins/sample_logger_plugin.dll` 娑?`RELEASE_NOTES.md`閿涘奔绗栨稉宥呭瘶閸?`config/player_settings.ini`閵?- `build\Release\modern-video-player.exe --d3d11-diagnostics`閿涙岸鈧俺绻冮敍瀹峳esult=PASS`閵?
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
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涢弰搴ｂ€樼涵顔款吇閹?`1.0.0-rc1` 閺傜懓鎮滈幒銊ㄧ箻閿涘苯绗囬張娑欏Ω RC 閸欐垵绔峰〒鍛礋閵嗕礁鍑￠惌銉╂６妫版ê鎷伴崣鎴濈鐠囧瓨妲戦弨璺哄經閹存劒绔存總妤€褰查惄瀛樺复娴ｈ法鏁ら惃鍕瀮濡楋絻鈧?- 鏉╂瑨鐤嗛惄顔界垼娑撳秵妲哥紒褏鐢婚幍鈺佸閼虫枻绱濋懓灞炬Ц閸掋倖鏌囬垾婊冪秼閸撳秶澧楅張顒佹Ц閸氾箑鍑＄紒蹇氬喕婢剁喎褰?RC閳ユ繐绱濋獮鑸靛Ω鐠囦焦宓佹稉搴ㄦ閸掓儼顔夊〒鍛殶閵?
### 閺冦儱绻旀潏鎾冲毉
```text
Release gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200
[1/3] Probe exit code: 0
[2/3] Forced FailSession exit code: 0
[3/3] Regression exit code: 0
Format regression report: docs/reports/FORMAT_REGRESSION_20260323_224615.md
Summary: Total=17 PASS=17 PARTIAL=0 FAIL=0 SKIP=0

Release diagnostics:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.hevc_any=true
d3d11-diagnostics.decoder_profiles.vp9_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.result=PASS

Release performance smoke:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS

Release long playback smoke:
build\Release\modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000
long-playback-check.still_playing_after_window=true
long-playback-check.late_drops=0
long-playback-check.demux_dropped_packets=0
long-playback-check.result=PASS

Release serial/failsession gate:
build\Release\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.pass_count=3
serial-failsession-regression-check.total_count=3
serial-failsession-regression-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 鏉╂瑨鐤嗙拠浣瑰祦瀹歌尙绮＄搾鍐差檮閺€顖涘瘮閳ユ粌褰查崣?RC閳ユ繆鈧奔绗夐弰顖欑矌閸嬫粎鏆€閸︺劉鈧粍婀伴崷鏉夸紦鐏忔棁鍏橀幘顓涒偓婵撶窗Release gate閵嗕焦鐗稿蹇撴礀瑜版帇鈧線鏆遍弮鑸垫尡閺€淇扁偓涔籩ek/serial閵嗕公orced failsession 閸?D3D11 diagnostics 闁姤婀侀張鈧弬鎵波閺嬫嚎鈧?2. 瑜版挸澧?RC 缂佹捁顔戞惔鏂跨秼閺勫海鈥橀崠鍝勫瀻娑撹　鈧粌褰查幍?`v1.0.0-rc1` 閺嶅洨顒烽垾婵呯瑢閳ユ粈绗夐崣顖滄纯閹恒儱顓虹粔鐗堫劀瀵繒澧楀鎻掔暚閹存劏鈧縿鈧倻婀″锝夋▎濮濄垻娲块幒?GA 閻ㄥ嫸绱濇稉宥嗘Ц娑撳鎽兼稉宥呭讲閻㈩煉绱濋懓灞炬Ц閸忕厧顔愰惌鈺呮█閸滃矂鏆辩亸鎹愮熅瀵板嫬鐨婚張顏勭暚閸忋劌鎮嗛柅蹇嬧偓?3. 閺堚偓闂団偓鐟曚焦妯夊蹇撳晸鏉╂稑鍑￠惌銉╂６妫版娈戦弰?`闂傤噣顣?79`閿涙oftware video decode 鏉╂劘顢戦幀浣界熅瀵板嫪绮涢張顏勭暚閸忋劍鏁归崣锝冣偓鍌氱暊瑜版挸澧犳稉宥夋▎婵?`D3D11VA` 娑撳鎽?RC閿涘奔绲剧€瑰啩绮涢弰顖涱劀瀵繒澧楅崜宥囨畱閸撯晙缍戞搴ㄦ珦閵?4. 瑜版挸澧犻張鍝勬珤閻?`AV1` profile 鐠囧﹥鏌囩紒鎾寸亯娴犲秳璐?`false`閿涘矁绻栭崘宥嗩偧鐠囦焦妲?RC 閸欐垵绔风拠瀛樻娑撳秷鍏橀幎濞锯偓婊冨讲閹绢厽鏂?AV1 閺傚洣娆㈤垾婵嗘嫲閳ユ粍娅橀柆宥呭徔婢?AV1 绾剝袙閼宠棄濮忛垾婵囪穿娑撹桨绔寸拫鍫涒偓?
### 婢跺嫮鎮婄紒鎾寸亯
- 閺傛澘顤?`docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`閿?  - 鏉堟挸鍤?RC 閸欐垵绔风紒鎾诡啈
  - 濮瑰洦鈧粯婀版潪?Release 妤犲矁鐦夌拠浣瑰祦
  - 閺€璺哄經閸欐垵绔风拠瀛樻閵嗕礁鍑￠惌銉╂６妫版ü绗岄崣鎴濈濞撳懎宕?- 閺囧瓨鏌?`docs/reports/README.md` 娑?`docs/README.md`閿?  - 娑?RC 濮瑰洦鈧粯濮ら崨濠傤杻閸旂姴鍙嗛崣?- 閺囧瓨鏌?`docs/records/VERSION.md`閿?  - 婢х偛濮?`瑜版挸澧犻崣鎴濈閸婃瑩鈧? 1.0.0-rc1`
  - 婢х偛濮?`閸欐垵绔烽悩鑸碘偓? 閸欘垰褰傜敮?RC閿涘奔绗夊楦款唴閻╁瓨甯?GA`
  - 鐠佹澘缍嶉張顒冪枂 RC 缂佹捁顔戞稉搴㈡付閺備即鐛欑拠浣界槈閹?- `CHANGELOG / DEVELOP_LOG` 瀹告彃鎮撳銉唶瑜版洘婀板▎鈥冲絺鐢啫鍣径鍥ㄦ暪閸欙絻鈧?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- 瑜版挸澧犵紒鎾诡啈閿涙瓪閸欘垱澧?v1.0.0-rc1 閺嶅洨顒穈閵?- 娑撳秴缂撶拋顔肩秼閸撳秶娲块幒銉﹀ⅵ `v1.0.0` 濮濓絽绱￠悧鍫涒偓?- 閺堚偓閺?Release 鐠囦焦宓侀敍?  - `run_all_checks.ps1`閿涙岸鈧俺绻?  - `FORMAT_REGRESSION_20260323_224615.md`閿涙瓪17 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`
  - `--d3d11-diagnostics`閿涙岸鈧俺绻?  - `--performance-log-check`閿涙岸鈧俺绻?  - `--long-playback-check`閿涙岸鈧俺绻?  - `--serial-failsession-regression-check`閿涙岸鈧俺绻?
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
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涚憰浣圭湴閹?D3D11 閸氬海鐢婚幋鎰暃閸栨牠銆嶆稉鈧▎陇藟姒绘劧绱癲ecoder profile 閹恒垺绁撮妴涔╮iver quirk/blacklist 閸氼垰濮╅張鐔兼缁狙呯摜閻ｃ儻绱濇禒銉ュ挤閸楁洜瀚惃?`--d3d11-diagnostics` CLI閵?- 閻╊喗鐖ｆ稉宥嗘Ц缂佈呯敾鐞涖儰绔寸紒鍕閺冭埖妫╄箛妤嬬礉閼板本妲搁幎?D3D11 閼宠棄濮忚箛顐ゅ弾閸嬫碍鍨氶崣顖涙簚閸ｃ劏顕伴崣鏍モ偓浣稿讲閸氼垰濮╅張鐔峰枀缁涙牓鈧礁褰查懘杈╊瀲閹绢厽鏂侀崡鏇犲閹笛嗩攽閻ㄥ嫬鐔€绾偓鐠佺偓鏌﹂妴?
### 閺冦儱绻旀潏鎾冲毉
```text
Release diagnostics:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.supported_platform=true
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.adapter_name=NVIDIA GeForce GTX 1080
d3d11-diagnostics.driver_version=32.0.15.6094
d3d11-diagnostics.format.nv12.shader_sample=true
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.hevc_any=true
d3d11-diagnostics.decoder_profiles.vp9_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.native_direct.disable_rule=none
d3d11-diagnostics.result=PASS

Release playback regression:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[diag:d3d11-init] decoder_profiles enumeration_succeeded=true enumerated_profile_count=20 h264_vld_nofgt=true h264_vld_fgt=false hevc_main=true hevc_main10=true vp9_profile0=true vp9_profile2_10bit=false av1_profile0=false av1_profile1=false av1_profile2=false av1_profile2_12bit=false av1_profile2_12bit_420=false
[diag:d3d11-init] native_direct startup_allowed=true startup_disabled=false rule=none
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 鏉╂瑨鐤嗛崗鎶芥暛閸欐ê瀵查弰顖涘Ω D3D11 閹恒垺绁存禒搴樷偓婊勬）韫囨澧﹂崡鎵斥偓婵嗗磳缁狙勫灇閳ユ粎绮ㄩ弸鍕韫囶偆鍙庨垾婵撶礉鏉╂瑦鐗?renderer 閸氼垰濮╅張鐔虹摜閻ｃ儱鎷伴悪顒傜彌 CLI 閸欘垯浜掗崗杈╂暏閸氬奔绔存禒鎴掔皑鐎圭偞绨妴?2. 瑜版挸澧犻張鍝勬珤閻ㄥ嫬鐤勫ù瀣波閺嬫粌鍑＄紒蹇氬厴閻╁瓨甯撮崶鐐电摕閻劍鍩涢張鈧崗鍐茬妇閻ㄥ嫰妫舵０姗堢窗`H.264 / HEVC / VP9` 閺堝鈥栫憴?profile閿涘畭AV1` 濞屸剝婀侀敍娌桸V12` 閺€顖涘瘮 shader sampling閿涙捕ative direct 閸氼垰濮╅張鐔峰帒鐠佺绱戦崥顖樷偓?3. `P016` 閻?`CheckFormatSupport` 娴犲秴銇戠拹銉礉`VP9 profile2 10bit` 娑撳孩澧嶉張?`AV1` profile 娑旂喖鍏樻稉宥嗘暜閹镐緤绱濇潻娆掝嚛閺勫簶鈧粍婀?D3D11閳ユ繀绗夌粵澶夌艾閳ユ粍澧嶉張?10bit / 閺傛壆绱惍?profile 闁棄褰查悽銊⑩偓婵撶礉閻欘剛鐝?diagnostics CLI 閻ㄥ嫪鐜崐鐓庢皑閺勵垱濡告潻娆庣昂鏉堝湱鏅弰鎯х础閸栨牓鈧?4. quirk / blacklist 閺堝搫鍩楄ぐ鎾冲閸忓牅绻氱€瑰牐鎯ゆ稉鈧稉顏呮付閺勫海鈥樼憴鍕灟閿涙瓪Microsoft Basic Render Driver` 閻╁瓨甯寸粋浣烘暏 native direct閿涙稑鎮楃紒顓″缁夘垳鐤崚鐗堟纯婢舵岸鈹嶉崝銊ユ綑娴ｅ稄绱濋崣顖滄埛缂侇厽閮ㄩ崥灞肩缁涙牜鏆愮悰銊﹀⒖閸忓懌鈧?
### 婢跺嫮鎮婄紒鎾寸亯
- `include/render/d3d11_video_renderer.h`閿?  - 閺傛澘顤?`D3D11FormatSupportSnapshot`
  - 閺傛澘顤?`D3D11DecoderProfileSupport`
  - 閺傛澘顤?`D3D11DiagnosticsSnapshot`
  - 閺傛澘顤?`D3D11VideoRenderer::probeSystemDiagnostics()`
- `src/render/d3d11_video_renderer.cpp`閿?  - 閺傛澘顤?D3D11 probe context閵嗕焦鐗稿蹇旀暜閹镐焦鐓＄拠顫偓涔╡coder profile 閺嬫矮濡囬妴涔籺artup policy 鐠囧嫪鍙婇崪灞芥儙閸斻劍妫╄箛妤勭翻閸?  - 閺傛澘顤冮張顒€婀?GUID 鐢悂鍣洪敍宀勪缉閸忓秶娲块幒銉ょ贩鐠ф牗鐓囨禍?Windows SDK 閻滎垰顣ㄦ稉瀣畱 decoder profile 婢舵牠鍎寸粭锕€褰?  - renderer 閸掓繂顫愰崠鏍▉濞堝灚甯撮崗?startup policy閿涘苯绻€鐟曚焦妞傞崷銊︽尡閺€鎯у鐏忚京顩﹂悽?native direct
- `src/main.cpp`閿?  - 閺傛澘顤?`runD3D11Diagnostics()`
  - 閺傛澘顤?`--d3d11-diagnostics`
  - 鏉堟挸鍤紒鐔剁 `key=value` 閺堝搫娅掗崣顖濐嚢缂佹挻鐏?- `CHANGELOG / VERSION / DEVELOP_LOG` 瀹告彃鎮撳銉唶瑜版洘婀板▎?D3D11 閹存劗鍟涢崠鏍ь杻瀵亽鈧?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `build\Release\modern-video-player.exe --d3d11-diagnostics`閿涙岸鈧俺绻冮敍瀹峳esult=PASS`閵?- 瑜版挸澧犻張鍝勬珤閻ㄥ嫭娓堕弬鎷岀槚閺傤厾绮ㄩ弸婊愮窗
  - `adapter_name=NVIDIA GeForce GTX 1080`
  - `driver_version=32.0.15.6094`
  - `decoder_profiles.h264_any=true`
  - `decoder_profiles.hevc_any=true`
  - `decoder_profiles.vp9_any=true`
  - `decoder_profiles.av1_any=false`
  - `native_direct.allowed=true`
  - `native_direct.disable_rule=none`
- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`閿涙岸鈧俺绻冮敍瀹峳enderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗video_native_output_frames=62`閵嗕梗video_copy_back_frames=0`閵嗕梗result=PASS`閵?
### 娣囶喗鏁奸弬鍥︽
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- src/main.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?92: D3D11 閸氼垰濮╅張鐔诲厴閸旀稒甯板ù瀣╃瑢 adapter/driver 鐠囧﹥鏌囬弮銉ョ箶鐞涖儵缍?
**閺冦儲婀?*: 2026-03-23
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涚憰浣圭湴缂佈呯敾閹稿鍨氶悢鐔告尡閺€鎯ф珤閺傜懓鎮滈幒銊ㄧ箻閿涘奔绗呮稉鈧銉ㄋ夋?D3D11 閸氼垰濮╅張鐔诲厴閸旀稒甯板ù瀣╃瑢 adapter/driver 鐠囧﹥鏌囬弮銉ョ箶閵?- 閻╊喗鐖ｉ弰顖氭躬閸掓繂顫愰崠鏍▉濞堢數娲块幒銉ф箙閸掓壋鈧粌缍嬮崜宥夆偓鍌炲帳閸ｃ劍妲哥拫浣碘偓渚€鈹嶉崝銊у閺堫剚妲告径姘毌閵嗕公eature level 閸掓澘鎽㈤妴浣稿彠闁款喗鐗稿蹇旀Ц閸氾附鏁幐浣测偓婵撶礉閼板奔绗夐弰顖氬涧閸︺劑绮︾仦蹇撴倵鐞氼偄濮╂潻鑺ユ）韫囨ぜ鈧?
### 閺冦儱绻旀潏鎾冲毉
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[diag:d3d11-init] adapter="NVIDIA GeForce GTX 1080" vendor_id=0x10DE device_id=0x1B80 subsystem_id=0x145119DA revision=161 driver_version=32.0.15.6094 software_adapter=false dedicated_video_mib=8059 dedicated_system_mib=0 shared_system_mib=16313
[diag:d3d11-init] device feature_level=11_1 debug_layer=true multithread_protected=true device3=true video_device=true video_context=true
[diag:d3d11-init] format_support NV12{raw=0xFA82C320 texture2d=true shader_sample=true shader_load=true decoder_output=true} P010{raw=0x3A82C320 texture2d=true shader_sample=true shader_load=true decoder_output=true} P016{check_failed_hr=-2147467259}
[diag:d3d11-init] swap_chain width=1318 height=742 format=B8G8R8A8_UNORM buffer_count=2 sample_count=1 swap_effect=FLIP_DISCARD alpha_mode=IGNORE usage=0x20
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 閸氼垰濮╅張鐔告）韫囨鍑＄紒蹇氬厴閻╁瓨甯撮弳鎾苟 adapter/driver/feature level 娑撳﹣绗呴弬鍥风礉閸氬海鐢婚崘宥変海閸掔増婧€閸ｃ劌妯婂鍌炴６妫版ɑ妞傞敍灞肩瑝闂団偓鐟曚礁鍘涚粵澶婂煂閹绢厽鏂佹径杈Е閸愬秴寮介幒銊啎婢跺洣淇婇幁顖樷偓?2. 閺嶇厧绱￠弨顖涘瘮閹恒垺绁撮弰鍓с仛瑜版挸澧犻張鍝勬珤閻?`NV12`閵嗕梗P010` 閸忓嘲顦?`shader_sample + decoder_output`閿涘矁鈧?`P016` 閻?`CheckFormatSupport` 閻╁瓨甯存径杈Е閿涘矁绻栫猾鑽ょ矎閼哄倹顒濋崜宥呯暚閸忋劋绗夐崣顖濐潌閵?3. 鏉╂瑧琚弮銉ョ箶閺勵垱鍨氶悢鐔告尡閺€鎯ф珤閻ㄥ嫰鍣哥憰浣哥唨绾偓鐠佺偓鏌﹂敍灞芥礈娑撳搫鐣犻幎濞锯偓婊嗩啎婢跺洩鍏橀崝娑掆偓婵呯瑢閳ユ粏绻嶇悰灞炬埂鐞涖劎骞囬垾婵婄箾閹恒儴鎹ｉ弶銉ょ啊閵?
### 婢跺嫮鎮婄紒鎾寸亯
- `src/render/d3d11_video_renderer.cpp`閿?  - 閺傛澘顤?adapter/driver version/閺勬儳鐡ㄦ穱鈩冧紖閺冦儱绻?  - 閺傛澘顤?feature level閵嗕龚ebug layer閵嗕沟ultithread閵嗕梗device3/video_device/video_context` 閺冦儱绻?  - 閺傛澘顤?`NV12/P010/P016` 閻ㄥ嫭鐗稿蹇旀暜閹镐焦鎲崇憰浣规）韫?  - 閺傛澘顤?swap chain 閸欏倹鏆熼弮銉ョ箶
  - `MakeWindowAssociation` 閺€閫涜礋閺勬儳绱″Λ鈧弻銉ャ亼鐠愩儱鑻熼崨濠咁劅
- `CHANGELOG / VERSION / DEVELOP_LOG` 瀹告彃鎮撳銉唶瑜版洘婀板▎陇鐦栭弬顓烆杻瀵亽鈧?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `Release` 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 闁俺绻冮妴?- 閺傛澘顤冮惃?`[diag:d3d11-init]` 閸ユ稓绮嶉弮銉ョ箶缁嬪啿鐣鹃幍鎾冲祪閿涘奔绗栭張顏勫閸濆秴缍嬮崜宥呭嚒缂佸繑浠径宥囨畱闂嗚埖瀚圭拹婵堟纯闁插洦鐗遍柧鎹愮熅閿涙瓪video_native_output_frames=62`閵嗕梗video_copy_back_frames=0`閵嗕梗result=PASS`閵?
### 娣囶喗鏁奸弬鍥︽
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?91: D3D11VA 閼奉亜鐣炬稊?hw_frames_ctx閿涙氨鏁电拠宄板讲闁插洦鐗辩憴锝囩垳鐞涖劑娼伴獮鑸典划婢跺秹娴傞幏鐤閻╂挳鍣伴弽?
**閺冦儲婀?*: 2026-03-23
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涙潻娑楃濮濄儴顩﹀Ч鍌涘Ω D3D11 鐠侯垰绶為崑姘灇閸嶅繑鍨氶悢鐔告尡閺€鎯ф珤闁絾鐗遍惃鍕埂鐎圭偛褰查悽銊ョ杽閻滃府绱濋懓灞肩瑝閺勵垯绮庢笟婵婄闂傤噣顣?90 閻ㄥ嫯绻嶇悰灞炬 copy-back fallback閵?- 闂団偓鐟曚線鐛欑拠浣哥秼閸撳秹銆嶉惄顔芥Ц娑撳秵妲搁崶鐘辫礋濞屸剝婀侀懛顏勭暰娑?`hw_frames_ctx`閿涘本澧犵€佃壈鍤?D3D11VA 鐟欙絿鐖滈棃銏ょ帛鐠併倓绗夐崣顖滄纯閹恒儵鍣伴弽鏋偓?
### 閺冦儱绻旀潏鎾冲毉
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS

Debug build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Debug check:
build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 瑜版挸澧犳い鍦窗濮濄倕澧犻崣顏囶啎缂冾喕绨?`hw_device_ctx`閿涘本鐥呴張澶婃躬 `get_format()` 娑擃厽甯寸粻?`hw_frames_ctx`閿涘矁绻栨稉?FFmpeg 婢跺瓨鏋冩禒鑸靛絹娓氭稓娈?D3D11 frames 閸掑棝鍘ら幒銉ュ經娑撳秴灏柊宥冣偓?2. 娑撯偓閺冿箑婀?`AVD3D11VAFramesContext::BindFlags` 娑撳﹨鎷烽崝?`D3D11_BIND_SHADER_RESOURCE`閿涘苯鎮撴稉鈧崣鐗堟簚閸ｃ劋绗傜粩瀣煝閹垹顦叉禍?`video_copy_back_frames=0` 閻?native direct 鐠侯垰绶為敍宀冾嚛閺勫孩鐗撮崶鐘虫Ц鐢勭潨缂佹垵鐣鹃弬鐟扮础閿涘矁鈧奔绗夐弰顖溾€栨禒鍓佺卜鐎甸€涚瑝閺€顖涘瘮閵?3. 閸ョ姵顒濋幋鎰暃閹绢厽鏂侀崳銊ユ嫲瑜版挸澧犻弮褍鐤勯悳鎵畱瀹割喛绐涢敍灞煎瘜鐟曚椒绗夐崷銊⑩偓婊勬箒濞屸剝婀?D3D11閳ユ繐绱濋懓灞芥躬閳ユ粍婀佸▽鈩冩箒鐎瑰本鏆ｉ幒銉ь吀 D3D11VA frames allocation policy 閸滃苯顦跨痪?fallback閳ユ縿鈧?
### 婢跺嫮鎮婄紒鎾寸亯
- `include/core/player_core.h`閿?  - 閺傛澘顤?`configureD3D11HardwareFramesContext(...)`
  - 閺傛澘顤?`selectSoftwarePixelFormat(...)`
- `src/core/player_core.cpp`閿?  - 閸?`get_format()` 闂冭埖顔岄柅姘崇箖 `avcodec_get_hw_frames_parameters()` 閸掓稑缂撻懛顏勭暰娑?`hw_frames_ctx`
  - 娑?`AVD3D11VAFramesContext::BindFlags` 鏉╄棄濮?`D3D11_BIND_SHADER_RESOURCE`
  - 閹?`extra_hw_frames` 妫板嫮鐣婚崣鐘插閸?`initial_pool_size`
  - 閼汇儴鍤滅€规矮绠?frames ctx 婢惰精瑙﹂敍灞藉灟闁偓閸?decoder-owned D3D11VA surface閿涙稖瀚㈡潻鎰攽閺冭泛鍟€婢惰精瑙﹂敍灞藉灟缂佈呯敾閻㈤亶妫舵０?90 閻?renderer fallback 閸忔粌绨?- `CHANGELOG / VERSION / DEVELOP_LOG` 瀹告彃鎮撳銉唶瑜版洘婀板▎鈥崇杽閻滄澘宕岀痪褋鈧?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `Release` / `Debug` 閺嬪嫬缂撻崸鍥偓姘崇箖閿涘畭0 warnings / 0 errors`閵?- `juren-30s.mp4` 閻?`--performance-log-check 2000` 閸?`Release` / `Debug` 娑撳娼庢潏鎾冲毉閿?  - `Configured D3D11VA frames context for direct shader sampling: bind_flags=520`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `result=PASS`
- 鐠囧瓨妲戣ぐ鎾冲閺堝搫娅掓稉濠傚嚒娑撳秴鍟€娓氭繆绂?copy-back閿涘瞼婀″锝嗕划婢跺秴鍩?D3D11VA -> D3D11 native direct 閻ㄥ嫰娴傞幏鐤閻╂挳鍣伴弽鐑芥懠鐠侯垬鈧?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?90: D3D11 閸樼喓鏁撻惄鎾櫚閺嶇兘绮︾仦蹇ョ窗鏉╂劘顢戦弮鍓侇洣閻?native direct 楠炶泛娲栭柅鈧?copy-back

**閺冦儲婀?*: 2026-03-23
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涢崣宥夘洯閿涙瓪Release` 閺嬪嫬缂撶粙瀣碍閹绢厽鏂?`juren-30s.mp4` 閺冭泛褰ч張澶婏紣闂婄绱濆▽鈩冩箒閻㈠娼伴敍灞借嫙閺勫海鈥樼憰浣圭湴閹烘帗鐓?`D3D11` 鐠侯垰绶炴鎴濈潌閵?- 闂団偓鐟曚胶鈥樼拋銈夋６妫版ê鐫樻禍搴ば掗惍浣搞亼鐠愩儯鈧焦瑕嗛弻鎾炽亼鐠愩儻绱濇潻妯绘Ц D3D11VA 娑?D3D11 renderer 閻ㄥ嫯浠堥崝銊ュ悑鐎硅鈧囨６妫版ǜ鈧?
### 閺冦儱绻旀潏鎾冲毉
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Debug build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[WARNING] D3D11 native direct rendering disabled for current session: CreateShaderResourceView1 for Y plane failed texture_format=NV12 texture_format_id=103 array_size=44 bind_flags=512 misc_flags=0 texture_index=43 y_plane_hr=-2147024809 uv_plane_hr=0 fallback=copyback-to-software
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=3
performance-log-check.video_copy_back_frames=59
performance-log-check.render_frames=47
performance-log-check.result=PASS

Debug check:
build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[WARNING] D3D11 native direct rendering disabled for current session: CreateShaderResourceView1 for Y plane failed texture_format=NV12 texture_format_id=103 array_size=44 bind_flags=512 misc_flags=0 texture_index=43 y_plane_hr=-2147024809 uv_plane_hr=0 fallback=copyback-to-software
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=1
performance-log-check.video_copy_back_frames=62
performance-log-check.render_frames=48
performance-log-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. `renderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗render_frames > 0` 鐠囧瓨妲戦幘顓熸杹娑撳鎽肩捄顖涙弓閺傤叏绱濋梻顕€顣芥稉宥呮躬閺傚洣娆㈤幒銏＄ゴ閵嗕浇袙婢跺秶鏁ら幋鏍叾妫版垿鎽奸妴?2. 閸涘﹨顒熺粙鍐茬暰閸涙垝鑵?`CreateShaderResourceView1 for Y plane failed`閿涘矁銆冮弰搴ㄧ拨鐏炲繑鐗撮崶鐘叉躬 D3D11 閸樼喓鏁撻惄鎾櫚閺嶇兘妯佸▓闈涱嚠绾剝袙鐞涖劑娼伴崚娑樼紦 SRV 婢惰精瑙﹂敍宀冣偓灞炬＋闁槒绶銈呭濞屸剝婀侀幎濠呯箹缁槒绻嶇悰灞炬婢惰精瑙︽潪顒佸灇閺勫海鈥橀梽宥囬獓閵?3. `video_copy_back_frames > 0` 娑?`result=PASS` 鐠囧瓨妲戞稉鈧弮锕€浠犲?native direct閿涘瞼骞囬張?copy-back + 鏉烆垯娆㈢痪鍦倞娑撳﹣绱堕柧鎹愮熅閼宠棄顧勭紒褏鐢荤粙鍐茬暰閸戝搫娴橀妴?
### 婢跺嫮鎮婄紒鎾寸亯
- `src/render/d3d11_video_renderer.cpp`閿?  - 閺傛澘顤?`disableNativeDirectRendering(...)`閿涘瞼绮烘稉鈧崶鐐存暪 native SRV 閻樿埖鈧礁鑻熺拋鏉跨秿娑撯偓濞嗏剝鈧冩啞鐠€锔衡偓?  - `ensureNativeShaderResourcesLocked(...)` 閸?Y/UV plane `CreateShaderResourceView1` 婢惰精瑙﹂弮鍓侇洣閻劌缍嬮崜宥勭窗鐠?native direct閿涘苯鑻熸潻鏂挎礀 `false`閵?  - `bindNativeFrameLocked(...)` 閸︺劏袙閻浇銆冮棃銏＄壐瀵繋绗夐弨顖涘瘮閻╁瓨甯撮柌鍥ㄧ壉閺冭泛鎮撻弽椋庮洣閻?native direct閵?  - `supportsNativeFrameFormat()` 閹恒儱鍙?`native_direct_rendering_disabled_` 閻樿埖鈧緤绱濈涵顔荤箽閸氬海鐢荤敮褎鏁肩挧?`PlayerCore` 閻滅増婀?copy-back 鐠侯垰绶為妴?- `CHANGELOG / VERSION / DEVELOP_LOG` 瀹告彃鎮撳銉唶瑜版洘婀板▎鈥叉叏婢跺秲鈧?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- `Release` / `Debug` 閺嬪嫬缂撻崸鍥偓姘崇箖閿涘畭0 warnings / 0 errors`閵?- `juren-30s.mp4` 閻?`--performance-log-check 2000` 閸?`Release` / `Debug` 娑撳娼庨柅姘崇箖閵?- 娑撱倓閲滈弸鍕紦閸у洩鍏樼粙鍐茬暰閹垫挸宓?`fallback=copyback-to-software`閿涘苯鑻熸穱婵囧瘮闂堢偤娴?`video_copy_back_frames`閵嗕梗render_frames`閿涘矁顕╅弰搴ㄧ拨鐏炲繗鐭惧鍕嚒鐞氼偉绻嶇悰灞炬闂勫秶楠囬幒銉ь吀閵?
### 娣囶喗鏁奸弬鍥︽
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?80: 閺傚洦銆傛稉鈧懛瀛樷偓褑藟姒绘劧绱癈HANGELOG 缁便垹绱╂穱顔碱槻娑撳酣妫舵０?69 analysis 閸ョ偛锝?
**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犲﹤銇夋稉澶嬵偧閹绘劒姘︾€瑰本鍨氶崥搴礉缂佈呯敾鐎靛湱鍙?records / analysis 閺冭泛褰傞悳棰佽⒈婢跺嫭鏋冨锝勭閼峰瓨鈧呭繁閸欙綇绱?
  - `CHANGELOG` 閻ㄥ嫰妫舵０妯烩偓鏄忋€冨蹇庣啊 `闂傤噣顣?78`

  - `闂傤噣顣?69` 閸欘亝婀?records 娑撳娆㈡總妤嬬礉濞屸剝婀佺€电懓绨查惃?implementation planner analysis 閺傚洦銆?
- 鏉╂瑤琚辨稉顏堟６妫版ü绗夋导姘閸濆秷绻嶇悰灞炬鐞涘奔璐熼敍灞肩稻娴兼艾濂栭崫宥嗘煀娴兼俺鐦介幒銉х敾閵嗕線妫舵０妯煎偍瀵洘顥呯槐銏犳嫲閸氬海鐢婚弬鍥ㄣ€傜€孤ゎ吀閵?


### 閺冦儱绻旀潏鎾冲毉

```text

docs/records/CHANGELOG.md 闂傤噣顣介幀鏄忋€冭ぐ鎾冲鐎涙ê婀?77 -> 79 閻ㄥ嫯鐑﹂崣鍑ょ礉娴ｅ棙顒滈弬鍥у嚒鐎涙ê婀垾婊堟６妫?78閳ユ縿鈧?
docs/analysis/ 閻╊喖缍嶈ぐ鎾冲缂傚搫鐨稉搴樷偓婊堟６妫?69: PlayerCore 閸嬫粍鎸遍弨璺哄經閵嗕礁瀵橀梼鐔峰灙閹碘偓閺堝娼堟稉?Clock/Demuxer 鐠佹崘顓搁崐杞版叏婢跺秮鈧繂顕惔鏃傛畱閸楁洜瀚崚鍡樼€介弬鍥ㄣ€傞妴?
```



### 閸掑棙鐎界拋鏉跨秿

1. `闂傤噣顣?78` 鐏炵偘绨锝嗘瀮瀹告彃鍟撻妴浣哄偍瀵洘绱＄悰銉礉闂傤噣顣介張顒冨窛閺勵垱澧滃銉ф樊閹躲倖鈧槒銆冮弮鍫曚粣濠曞骏绱濇稉宥嗘Ц閸愬懎顔愮紓鍝勩亼閵?
2. `闂傤噣顣?69` 閻?records 閸愬懎顔愰張顒冮煩鐎瑰本鏆ｉ敍灞肩稻娑撳骸缍嬮崜?implementation planner 瀹搞儰缍旈弬鐟扮础閻╁憡鐦敍宀€鈥樼€圭偟宸辩亸鎴濆讲閻欘剛鐝涘鏇犳暏閻?analysis 閺傚洦銆傞妴?
3. 閸ョ姵顒濇潻娆掔枂閺堚偓鐏忓繋鎱ㄥ锝呯安閺勵垽绱?
   - 鐞涖儳鍌ㄥ鏇礉娑撳秵鏁奸弮銏℃箒闂傤噣顣界紓鏍у娇閸滃本顒滈弬鍥€庢惔?
   - 閸ョ偛锝炴稉鈧弧鍥у涧鐟曞棛娲?`闂傤噣顣?69` 閻ㄥ嫬鍨庨弸鎰瀮濡楋綇绱濋獮鑸靛Ω records 娑擃厾娈戝鏇犳暏鐞涖儵缍?


### 婢跺嫮鎮婄紒鎾寸亯

- `CHANGELOG` 闂傤噣顣介幀鏄忋€冨鑼端?`闂傤噣顣?78`閿涘苯鑻熼弬鏉款杻閺堫剚顐奸弬鍥ㄣ€傛稉鈧懛瀛樷偓褌鎱ㄦ径宥囨畱 `闂傤噣顣?80`閵?
- 瀹稿弶鏌婃晶?`docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`閿涘苯娲栨繅?`闂傤噣顣?69` 閻?implementation planner 閸滃苯鍙ч柨顔剧波鐠佹亽鈧?
- `CHANGELOG / VERSION / DEVELOP_LOG` 瀹告彃鎮撳銉ㄋ夋鎰箹濞嗏剝鏋冨锝勭閼峰瓨鈧傛叏濮濓絻鈧?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閺堫亪鍣搁弬鐗堢€鐚寸幢閺堫剝鐤嗘禒鍛叏濮濓絾鏋冨锝勭閼峰瓨鈧嶇礉娑撳秵绉归崣濠佸敩閻線鈧槒绶崣妯绘纯閵?
- 瑜版挸澧?records 娑?analysis 閻ㄥ嫬顕惔鏂垮彠缁鍑＄悰銉╃秷閸掑府绱?
  - `闂傤噣顣?69 -> PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`

  - `闂傤噣顣?78 -> CHANGELOG 闂傤噣顣介幀鏄忋€冨鎻掑讲閻╁瓨甯寸槐銏犵穿`



### 娣囶喗鏁奸弬鍥︽

- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?79: PlayerCore 鏉╂劘顢戦幀?software send probe 鐎靛湱鍙庨弨鑸垫殐

**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀵板懓袙閸?
### 闂傤噣顣介幓蹇氬牚

- 閸︺劑妫舵０?78 瀹稿弶濡?software decode blocker 閺€鑸垫殐閸掓壋鈧粓顩绘稉?`avcodec_send_packet()` 濞屸剝婀佺€瑰本鍨氭潻鏂挎礀閳ユ繂鎮楅敍灞炬拱鏉烆喚鎴风紒顓熷瘻 implementation planner 閸嬫氨婀＄€圭偠绻嶇悰灞锯偓浣割嚠閻撗嶇礉閻╊喗鐖ｉ弰顖滄埛缂侇厾缂夌亸蹇氬瘱閸ユ番鈧?
- 瑜版挸澧犻棁鈧憰浣告礀缁涙梻娈戦梻顕€顣介弰顖ょ窗鏉╂瑤閲?blocker 閸掓澘绨抽弰?FFmpeg software decode 閺堫兛缍嬮妴涔竌cket 娴溿倖甯撮妴涔╡mux read-ahead閵嗕線鐓舵０鎴︽懠閿涘矁绻曢弰?`PlayerCore` 鏉╂劘顢戦幀浣藉殰闊偊鈧姵鍨氶惃鍕┾偓?
### 閺冦儱绻旀潏鎾冲毉

```text

.\build\Debug\modern-video-player.exe --software-video-send-probe .\juren-30s.mp4 1500

software-video-send-probe.packet_queue_push_ok=true

software-video-send-probe.packet_queue_pop_ok=true

software-video-send-probe.read_ahead_packets=512

software-video-send-probe.pre_send_receive_ret=-11

software-video-send-probe.send_ret=0

software-video-send-probe.receive_got_frame=true

software-video-send-probe.result=PASS



$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'

.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200

software-video-decode-check.audio_probe_mode=disabled

software-video-decode-check.clock_source=Video

software-video-decode-check.video_packet_dequeue_count=1

software-video-decode-check.video_send_packet_ok=0

software-video-decode-check.decode_video_ok=0

software-video-decode-check.render_frames=0

software-video-decode-check.result=FAIL



$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'

$env:MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD='1'

.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200

Offthread software video send_packet probe timed out after 500ms

```



### 閸掑棙鐎界拋鏉跨秿

1. 閻欘剛鐝?`send-probe` 閸?`pre-receive + packet queue round-trip + read-ahead=512` 閸氬簼绮?`result=PASS`閿涘矁顕╅弰?FFmpeg software decode 閺堫兛缍嬮妴涔竌cket queue 娴溿倖甯撮妴涔eceive->send` 妞ゅ搫绨崪?demux 缂佈呯敾鐠囪瀵橀柈鎴掔瑝閺勵垱鐗撮崶鐘偓?
2. `video-only` 鐎靛湱鍙庢稉瀣╃矝 `video_packet_dequeue_count=1 / video_send_packet_ok=0 / decode_video_ok=0 / render_frames=0`閿涘矁顕╅弰?blocker 娑撳酣鐓舵０鎴ｇ翻閸戞亽鈧竸udio master 閺冪姴鍙ч妴?
3. `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD=1` 娑撳绱漙PlayerCore` 鏉╂劘顢戦幀渚€鍣烽惃?software `send_packet` 娓氭繃妫?500ms 鐡掑懏妞傞敍宀冾嚛閺勫酣妫舵０妯圭瘍娑撳秴褰ч弰顖椻偓婊冪秼閸?video decode 缁捐法鈻兼稉濠佺瑓閺傚洠鈧繃婀伴煬顐犫偓?
4. 瑜版挸澧犻張鈧涵顒傛畱缂佹捁顔戦弰顖ょ窗blocker 瀹歌尙绮￠弨鑸垫殐閸?`PlayerCore` 鏉╂劘顢戦幀渚€鍣烽惃?software codec context / surrounding state 瀹割喖绱撻敍灞芥倵缂侇厼绨查惄瀛樺复鐎佃鐦?`PlayerCore::initDecoders()` 娴溠冨毉閻?codec ctx 鐎涙顔屾稉搴ｅ缁?probe閵?
### 婢跺嫮鎮婄紒鎾寸亯

- 閺傛澘顤?Day15 閸掑棙鐎介弬鍥ㄣ€傞敍宀€閮寸紒鐔活唶瑜版洘婀版潪顔藉閺?probe 鐎靛湱鍙庢稉搴ｇ波鐠佹亽鈧?
- 閹碘晛鐫?`--software-video-send-probe` 娑?`--software-video-decode-check` 閻ㄥ嫮绮ㄩ弸鍕鏉堟挸鍤敍宀勪缉閸忓秴鎮楃紒顓㈠櫢婢跺秷铔嬪顖濈熅閵?
- `PlayerCore` 瀹歌尪藟娑撯偓娑擃亙绮庨悳顖氼暔閸欐﹢鍣哄鈧崥顖滄畱 offthread send 鐠囧﹥鏌囩捄顖氱窞閿涘奔绶垫稉瀣╃鏉烆喚鎴风紒顓㈡嫟濮?runtime context 瀹割喖绱撻妴?
### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
- `--software-video-send-probe .\juren-30s.mp4 1500`閿涙瓪result=PASS`閵?
- `video-only --software-video-decode-check .\juren-30s.mp4 1200`閿涙瓪result=FAIL`閵?
- `video-only + offthread send --software-video-decode-check .\juren-30s.mp4 1200`閿涙瓪result=FAIL`閿涘苯鑻熼幍鎾冲祪鐡掑懏妞傞弮銉ョ箶閵?
### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY15_PLAYERRUNTIME_SOFTWARE_SEND_PROBE.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?78: software decode 閺堚偓鐏?send/dequeue 鐠佲剝鏆熼幒銉ュ弳娑撳酣顩婚崠鍛粹偓浣稿瘶閸嬫粍绮搁柦澶嬵劥

**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴閻╁瓨甯村▽?software decode 妫ｆ牕瀵橀崑婊勭哺閺傜懓鎮滅悰銉︽付鐏忓繗顓搁弫甯窗`video packet dequeue 濞嗏剝鏆?/ send 閹存劕濮涘▎鈩冩殶 / send 鏉╂柨娲栭惍涔ｉ妴?
- 鏉╂瑤绔村銉ょ瑝鎼存梻鎴风紒顓炲帥閸?`SoftwareSDL` 濞撳弶鐓嬫笟褝绱濋懓灞界安鐠囥儱鍘涢幎?software path 妫ｆ牕瀵橀梼鑸殿唽閸椻€虫躬閸濐亙绔寸仦鍌炴嫟濮濇眹鈧?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

0 warnings / 0 errors



.\build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200

performance-log-check.video_packet_dequeue_count=57

performance-log-check.video_send_packet_ok=57

performance-log-check.video_send_packet_last_ret=0

performance-log-check.result=PASS



[diag:audio-backpressure] ... dec(core v=0,a=155,v_pkt_deq=1,v_send_ok=0,v_send_ret=-2147483648,v_send_eagain=0,...) ...

```



### 閸掑棙鐎界拋鏉跨秿

1. `juren-30s.mp4` 閻?`--performance-log-check` 瀹歌尙绮￠幍鎾冲祪閸?`video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`閿涘矁顕╅弰搴㈡煀鐠佲剝鏆熺€涙顔屽鑼病娴?`PlayerCore` 濮濓絽鐖堕柅蹇庣炊閸?CLI閵?
2. software decode 閺嶉攱婀伴惃?1 缁夋帟鐦栭弬顓熸▔缁€?`v_pkt_deq=1`閿涘矁顕╅弰?video decode 缁捐法鈻奸獮鏈电瑝閺勵垰宕遍崷銊⑩偓婊冨絿娑撳秴鍩岀憴鍡涱暥閸栧應鈧縿鈧?
3. 閸氬本妞?`v_send_ok=0` 娑?`v_send_ret=-2147483648` 娴犲秵妲搁張顏囩箲閸ョ偛鎽犻崗闈涒偓纭风礉鐠囧瓨妲戦崚鎷岀槚閺傤厾鍋ｆ稉鐑橆剾妫ｆ牔閲?packet send 鏉╂ɑ鐥呴張澶婅埌閹存劒绔村▎鈥崇暚閹存劘绻戦崶鐐偓?
4. 缂佹挸鎮庡銈呭娑撯偓閻╁宸辨径杈╂畱 `Video decode first send_packet returned` 閺冦儱绻旈敍灞炬拱鏉烆喖褰叉禒銉﹀Ω blocker 閸愬秵鏁圭槐褌绔村銉窗瑜版挸澧?software path 閺囨潙鍎氶弰顖氬幢閸︺劑顩绘稉?`avcodec_send_packet(video_codec_ctx_, packet.get())` 鐠嬪啰鏁ら張顒冮煩閵?
5. 鏉╂瑤绡冩潻娑楃濮濄儴鐦夐弰搴窗瑜版挸澧?blocker 鏉╂ɑ鐥呯挧鏉垮煂 copy-back / swscale / display copy 闂冭埖顔岄妴?


### 婢跺嫮鎮婄紒鎾寸亯

- 閸?`PlayerCore` 鐞?`video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`閵?
- 閹跺﹥鏌婄€涙顔岄幒銉ュ煂 `DiagnosticsSnapshot`閵嗕椒缍嗘０鎴ｇ槚閺傤厽妫╄箛妞尖偓涔?-performance-log-check` 娑?`--software-video-decode-check`閵?
- 閺傛澘顤?Day14 閸掑棙鐎介弬鍥ㄣ€傞敍宀冾唶瑜版洍鈧粓顩绘稉顏囶潒妫版垵瀵樺鑼病 dequeue閿涘奔绲炬＃鏍﹂嚋 send 娴犲秵婀幋鎰鏉╂柨娲栭垾婵堟畱閺堚偓閺傛壆绮ㄧ拋鎭掆偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
- `--performance-log-check .\juren-30s.mp4 1200`閿涙瓪video_packet_dequeue_count=57 / video_send_packet_ok=57 / video_send_packet_last_ret=0 / result=PASS`閵?
- software decode 閺嶉攱婀版潻鎰攽閺堢喕鐦栭弬顓ㄧ窗`v_pkt_deq=1 / v_send_ok=0 / v_send_ret=-2147483648`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾娑撳绔村銉礉楠炲爼銆庨幍瀣Ω娴狅絿鐖滈弬鍥︽闁插本鐣悾娆戞畱濞夈劑鍣存稊杈╃垳濞撳懐鎮婇幒澶堚偓?
- 瑜版挸澧犻惄顔界垼娴犲秵妲?software video decode blocker閿涙氨鈥樼拋銈呮躬娣囨繂鐣х痪璺ㄢ柤闁板秶鐤嗘稉瀣剁礉soft decode 閺勵垰鎯佸鑼病閼崇晫婀＄€圭偘楠囩敮褋鈧?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

0 warnings / 0 errors



.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000

Video decoder threading: backend=Software thread_count=1 thread_type=none

Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808

[diag:audio-backpressure] demux(v=163,a=501,...) pkt_q(v=162,a=256) dec(core v=0,a=245,...)

software-video-decode-check.renderer_backend=SoftwareSDL

software-video-decode-check.decoder_backend=Software

software-video-decode-check.decode_video_ok=0

software-video-decode-check.scheduler_video_decoded_frames=0

software-video-decode-check.render_frames=0

software-video-decode-check.video_frame_queue_peak_size=0

software-video-decode-check.result=FAIL

```



### 閸掑棙鐎界拋鏉跨秿

1. `Video decoder threading: backend=Software thread_count=1 thread_type=none` 瀹歌尙绮＄拠浣规閺堫剝鐤嗘径宥嗙壋閸涙垝鑵戞禍?software decode 娣囨繂鐣х痪璺ㄢ柤闁板秶鐤嗛妴?
2. 閸楀厖绌舵俊鍌涱劃閿涘畭decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0 / video_frame_queue_peak_size=0` 娴犲秶鍔ч崗銊╁劥娑?0閿涘矁顕╅弰?blocker 娑撳秳绱伴崶鐘辫礋閹?FFmpeg 鏉烆垯娆㈢憴锝囩垳缁捐法鈻奸弨鍓佹彛閸掓澘宕熺痪璺ㄢ柤鐏忚精鍤滈崝銊︾Х婢朵究鈧?
3. 缂佹挸鎮?`demux(v=163)` 娑?`pkt_q(v=162)` 閸欘垯浜掗幒銊︽焽閿? 缁夋帞鐛ラ崣锝夊櫡 video decode 缁捐法鈻奸崣顏嗘埂濮濓絾绉风拹閫涚啊 1 娑擃亣顫嬫０鎴濆瘶閵?
4. 閸氬本妞傞弮銉ョ箶閸欘亜鍤悳?`Video decode first send_packet start`閿涘本鐥呴張澶屾箙閸掓澘顕惔鏃傛畱 `returned`閿涘苯娲滃銈呯秼閸撳秵娲块崓蹇旀Ц閸椻€虫躬妫ｆ牔閲滅憴鍡涱暥閸栧懏褰佹禍銈夋▉濞堢绱濋幋鏍у灠閹绘劒姘︽＃鏍у瘶閸氬骸姘ㄦ径鍗炲箵閸氬海鐢婚幒銊ㄧ箻閵?
5. `video_copy_back_frames=0 / video_swscale_frames=0` 缂佈呯敾鐠囧瓨妲戞潻娆庨嚋 blocker 鏉╂ɑ鐥呮潻娑樺弳 copy-back 閹?swscale 闂冭埖顔岄妴?
6. 娴狅絿鐖滃▔銊╁櫞娑旇京鐖滈弬褰掓桨閿涘本婀版潪顔锯€樼拋?`src/render/sdl_video_renderer.cpp` 娴犲秵婀?9 婢跺嫮婀＄€?mojibake 濞夈劑鍣撮敍娑楁叏婢跺秴鎮楅崘宥嗩偧閹殿偅寮?`src/`閵嗕梗include/` 閻?`///` 娑?`//` 濞夈劑鍣寸悰宀嬬礉閺堫亜鍟€閸涙垝鑵戦弬棰佽础閻降鈧?


### 婢跺嫮鎮婄紒鎾寸亯

- 娣囶喖顦?`src/render/sdl_video_renderer.cpp` 閻?9 婢跺嫬鍤遍弫鏉裤仈濞夈劑鍣存稊杈╃垳閿涘奔绮庨弨瑙勬暈闁插绱濇稉宥嗘暭闁槒绶妴?
- 闁插秵鏌婇幍褑顢?Debug 閺嬪嫬缂撻敍宀€绮ㄩ弸婊€璐?`0 warnings / 0 errors`閵?
- 闁插秵鏌婇幍褑顢?`--software-video-decode-check`閿涘苯缍嬮崜?blocker 缂佹捁顔戞潻娑楃濮濄儲鏁归弫娑楄礋閿涙oftware decode 閸︺劋绻氱€瑰牏鍤庣粙瀣帳缂冾喕绗呮禒宥囧姧娑撳秳楠囩敮褝绱濇稉鏃€娲块崓蹇旀Ц閳ユ粓顩绘稉顏囶潒妫版垵瀵橀崥搴′粻娴ｅ繆鈧縿鈧?
- 閺傛澘顤?Day13 閸掑棙鐎介弬鍥ㄣ€傜拋鏉跨秿閺堫剝鐤嗙紒鎾诡啈娑撳簼绗呮稉鈧銉ョ紦鐠侇喓鈧?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`閿涙氨绮ㄩ弸鍕鏉堟挸鍤禒宥勮礋 `result=FAIL`閵?
- 娴狅絿鐖滃▔銊╁櫞閹殿偅寮块敍姘拱鏉烆喗婀崘宥呭絺閻滅増鏌婇惃鍕讲閻ゆ垳璐￠惍浣告嚒娑擃厹鈧?


### 娣囶喗鏁奸弬鍥︽

- src/render/sdl_video_renderer.cpp

- docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?76: Software video decode 閻喎鐤勬禍褍鎶氭稉鎾汇€嶅Λ鈧弻銉ょ瑢 blocker 鐎规矮缍?
**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閹?implementation planner 閸楁洖绱戞稉鈧稉顏佲偓娓無ftware video decode 閻喎鐤勬禍褍鎶氭稉鎾汇€嶅Λ鈧弻銉⑩偓婵撶礉娑撳秷顩﹂崘宥夋浆閸欘亣鍏樼拠浣规閳ユ粍澧﹀鈧幋鎰閳ユ繄娈?`session-check` 闂傚瓨甯撮幒銊︽焽閵?
- 瑜版挸澧犲鑼叀 `SoftwareSDL + Software decode` 娴兼艾鍤悳?0 鐢嗙翻閸戠尨绱濇担鍡樻＋閸涙垝鎶ら弮顫瑝閼崇晫娲块幒銉х舶閸戣櫣绮ㄩ弸鍕缂佹捁顔戦敍灞芥嚒娴犮倛鍤滈煬顐ョ箷閸欘垵鍏樼悮?soft decode 閻?`stop/close` 閸椻€茬秶閵?


### 閺冦儱绻旀潏鎾冲毉

```text

& '.\build\Debug\modern-video-player.exe' --software-video-decode-check '.\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv' 2000

software-video-decode-check.open_ok=true

software-video-decode-check.entered_playback_loop=true

software-video-decode-check.renderer_backend=SoftwareSDL

software-video-decode-check.decoder_backend=Software

software-video-decode-check.audio_output_initialized=true

software-video-decode-check.clock_source=Audio

software-video-decode-check.advanced_seconds=1.89867

software-video-decode-check.demux_video_packets=163

software-video-decode-check.demux_ignored_packets=0

software-video-decode-check.demux_queue_drop_packets=0

software-video-decode-check.decode_video_ok=0

software-video-decode-check.scheduler_video_decoded_frames=0

software-video-decode-check.render_frames=0

software-video-decode-check.video_frame_queue_peak_size=0

software-video-decode-check.video_copy_back_frames=0

software-video-decode-check.video_swscale_frames=0

software-video-decode-check.real_frame_output_ok=false

software-video-decode-check.result=FAIL

```



### 閸掑棙鐎界拋鏉跨秿

1. 閺傛澘鎳℃禒銈呭嚒缂佸繑濡搁垾婊冨涧閺勵垵绻橀崗?playback loop閳ユ繂鎷伴垾婊呮埂鐎圭偘楠囩敮褉鈧繂浜ゆ惔鏇熷瀵偓閿涙碍婀板▎鈩冪壉閺堫剟鍣?`open_ok=true`閵嗕梗entered_playback_loop=true`閵嗕梗advanced_seconds=1.89867`閿涘矁顕╅弰搴㈡尡閺€鍙ョ窗鐠囨繂鎷伴棅鎶筋暥娑撶粯妞傞柦鐔煎厴閸︺劍甯规潻娑栤偓?
2. 閸氬本妞?`renderer_backend=SoftwareSDL`閵嗕梗decoder_backend=Software` 瀹歌尙绮＄拠浣规閸涙垝鎶ら崨鎴掕厬娴滃棛娲伴弽鍥懠鐠侯垽绱濋懓灞肩瑝閺勵垰寮甸幃鍕€撻柅鈧崶?`D3D11VA`閵?
3. 娴?`decode_video_ok=0`閵嗕梗scheduler_video_decoded_frames=0`閵嗕梗render_frames=0`閵嗕梗video_frame_queue_peak_size=0` 閸忋劋璐?0閿涘矁顕╅弰?blocker 閸欐垹鏁撻崷銊⑩偓婊嗚拫娴犳儼顫嬫０鎴Ｐ掗惍浣烽獓閸戝搫鎶氶垾婵呯閸撳稄绱濇潻?frame queue 闁姤鐥呴張澶庮潶婵夘偉绻冮妴?
4. `demux_video_packets=163`閵嗕梗demux_ignored_packets=0`閵嗕梗demux_queue_drop_packets=0` 鐠囧瓨妲戦梻顕€顣芥稊鐔剁瑝閸?demux 濞屸€虫澓閸栧懏鍨ㄩ梼鐔峰灙娑撱垹瀵橀妴?
5. `video_copy_back_frames=0`閵嗕梗video_swscale_frames=0` 鏉╂稐绔村銉ㄧ槈閺勫氦绻栨稉?blocker 娑?`copy-back / swscale / display copy` 閺冪姴鍙ч敍宀€鍔嶉悙鐟板嚒缂佸繑鏁归弫娑樺煂瑜版挸澧?FFmpeg software video decode 閹恒儱鍙嗛柧鐐拱闊偁鈧?
6. 閺冄呭閻╁瓨甯撮崷銊ユ嚒娴犮倕鐔柈?`stop/close` 娴兼俺顫?blocker 閹锋牗瀵曢敍娑樻礈濮濄倛绻栧▎鈥茬瑩妞よ顥呴弻銉潶閺€瑙勫灇 probe 瀵繒鈥栭柅鈧崙鐚寸礉鏉╂瑦婀伴煬顐＄瘍閺勵垰缍嬮崜?blocker 閻ㄥ嫪绔存稉顏呮⒑鐠囦降鈧?


### 婢跺嫮鎮婄紒鎾寸亯

- `src/main.cpp` 閺傛澘顤?`--software-video-decode-check <media_file> [sample_ms]`閵?
- 閸涙垝鎶ら崘鍛村劥瀵搫鍩?`SoftwareSDL + Software decode + dummy audio`閿涘矂浼╅崗宥囧箚婢у啫妯婂鍌涘Ω缂佹捁顔戝Ч鈩冪厠閹哄鈧?
- 閸涙垝鎶ら柅姘崇箖閺夆€叉鐞氼偅鏁圭槐褌璐熼敍?
  - `decoder_backend=Software`

  - `decode_video_ok > 0`

  - `scheduler_video_decoded_frames > 0`

  - `render_frames > 0`

  - `video_frame_queue_peak_size > 0`

  - `video_copy_back_frames == 0`

- 閸涙垝鎶ら弨閫涜礋 probe 瀵繒鈥栭柅鈧崙鐚寸礉绾喕绻氶崡鍏呭▏ soft decode 鐠侯垰绶為崡鈩冾劥閿涘奔绗撴い瑙勵梾閺屻儰绡冮懗鐣屒旂€规碍澧﹂崡鎵波閺嬪嫬瀵?FAIL 缂佹挻鐏夐妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮敍瀹? warnings / 0 errors`閵?
- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`閿涙俺绻戦崶鐐烘姜闂嗚绱濋獮鎯扮翻閸?`result=FAIL`閵?
- 瑜版挸澧?blocker 瀹歌尙绮＄悮顐㈠礋閻欘剟鎷ゅ浼欑窗鏉烆垯娆㈢捄顖氱窞閼宠姤澧﹀鈧妴浣藉厴鏉╂稒鎸遍弨鎯ф儕閻滎垬鈧線鐓舵０鎴炴闁界喕鍏橀幒銊ㄧ箻閿涘奔绲炬潪顖欐鐟欏棝顣剁憴锝囩垳闁剧偓鐥呴張澶婅埌閹存劒鎹㈡担鏇熸箒閺佸牐顫嬫０鎴濇姎娴溠冨毉閵?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY12_SOFTWARE_VIDEO_DECODE_REAL_FRAME_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?75: 閹俱倕娲?SoftwareSDL automatic software-first 楠炴儼藟鏉烆垵袙闂冭顢ｇ拠濠冩焽

**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾濞岃法娼冮垾婊冾洤娴ｆ洝绻樻稉鈧銉ュ櫤鐏忔垶鍨ㄧ憴鍕缉 `av_hwframe_transfer_data()` copy-back閳ユ繃甯规潻娑崇礉娴滃孩妲搁張顒冪枂閸忓牆鐨剧拠鏇熷Ω `SoftwareSDL` fallback 閺€瑙勫灇 renderer-aware `software-first`閵?
- 妫板嫭婀￠弨鍓佹抄閺勵垽绱版俊鍌涚亯 `SoftwareSDL` 閼崇晫娲块幒銉旂€规碍绉风拹纭呰拫娴犳儼袙閻礁鎶氶敍灞芥皑閼冲€熺箻娑撯偓濮濄儱甯囨担搴＄秼閸?fallback 闁炬儳褰ч崜鈺€绗呴惃?copy-back 閻戭厾鍋ｉ妴?
- 娴ｅ棗鐤勯梽鍛寸崣鐠囦椒鑵戦敍瀹峉oftwareSDL + Software decode` 娴兼俺绻橀崗銉⑩偓婊堢叾妫版垹鎴风紒顓熷腹鏉╂稏鈧浇顫嬫０?0 鐢嗙翻閸戣　鈧繄娈戦崶鐐茬秺閻樿埖鈧緤绱濋崶鐘愁劃闂団偓鐟曚礁鍘涢弨鑸垫殐缁涙牜鏆愰敍灞藉晙閸愬啿鐣鹃崥搴ｇ敾閺傜懓鎮滈妴?


### 閺冦儱绻旀潏鎾冲毉

```text

$env:SDL_AUDIODRIVER='dummy'

.\build\Debug\modern-video-player.exe --windows-backend-session-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv soft

Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808

Video decode first send_packet returned: backend=Software ret=0 message=unknown

windows-backend-session-check.soft.decoder_backend=Software

windows-backend-session-check.result=PASS



.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=33.5958

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS

```



### 閸掑棙鐎界拋鏉跨秿

1. 閳ユ笩ystem-memory renderer 娴兼ê鍘涢柆鍨帳 copy-back閳ユ繆绻栨稉顏呮煙閸氭垶婀伴煬顐ｇ梾閺堝妫舵０姗堢礉娑旂喓顑侀崥?`ffplay / mpv / MPC-HC` 娑撯偓缁粯鍨氶悢鐔告尡閺€鎯ф珤閻ㄥ嫬鐖剁憴浣筋啎鐠佲剝鈧繆鐭鹃妴?
2. 娴ｅ棙婀版潪顔诲閺?`software-first` 鐎圭偤鐛欓弰鍓с仛閿涘畭SoftwareSDL + Software decode` 娴兼艾鍤悳?`decode_video_ok=0 / render_frames=0 / video_frame_queue_peak_size=0`閿涘矁顕╅弰搴＄秼閸撳秴浼愮粙瀣畱鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖滈幒銉ュ弳闁剧偓婀伴煬顐＄瑝閹存劗鐝涢妴?
3. 鏉╂稐绔村銉ュ繁閸?`D3D11 + Software decode` 閸氬函绱濇禒宥呭讲婢跺秶骞囨潪顖欐鐟欙絿鐖滄＃鏍у瘶鏉╂稑鍙?`send_packet` 娴ｅ棗鎮楃紒顓濈瑝瑜般垺鍨氶張澶嬫櫏鐟欏棝顣舵潏鎾冲毉閿涘矁顕╅弰?blocker 娑撳秴婀?`SoftwareSDL` renderer閿涘矁鈧苯婀潪顖欐鐟欏棝顣剁憴锝囩垳閹恒儱鍙嗛妴?
4. 閸ョ姵顒濊ぐ鎾冲閺堚偓閸氬牏鎮婇惃鍕暪閺佹稒鏌熷蹇庣瑝閺勵垳鎴风紒顓熷Ω `software-first` 閻ｆ瑥婀稉鏄忕熅瀵板嫸绱濋懓灞炬Ц閹俱倕娲栭懛顏勫З閸掑洦宕查敍灞肩箽娴ｅ繐鍑＄紒蹇涒偓姘崇箖妤犲矁鐦夐惃?`D3D11VA copy-back` fallback閵?
5. 閸氬本妞傛穱婵堟殌閺堚偓鐏忓繐绻€鐟曚浇鐦栭弬顓ㄧ礉閸氬海鐢婚崘宥呭礋閻欘兛鎱ㄦ潪顖欐鐟欏棝顣剁憴锝囩垳閹恒儱鍙嗛敍宀冣偓灞肩瑝閺勵垱濡?fallback 娑撶粯绁︾粙瀣埛缂侇厽瀚嬫潻娑樻礀瑜版帇鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閹俱倕娲栭懛顏勫З renderer-aware `software-first` decoder 閹烘帒绨敍灞句划婢跺秹绮拋?`D3D11VA -> Software` 妞ゅ搫绨妴?
- 娣囨繄鏆€鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖滄担搴暥鐠囧﹥鏌囬懗钘夊閿?
  - FFmpeg 闁挎瑨顕ら惍浣哥摟缁楋缚瑕?
  - 妫ｆ牗顐?`send_packet` 鐠ч攱顒涢幒銏ゆ嫛

  - stall 娑撳﹣绗呴弬鍥ㄦ）韫?
- 缂佈呯敾娣囨繄鏆€娑撳﹣绔存潪顔煎嚒缂佸繐鐣幋鎰畱 `SoftwareSDL` fallback 閺堝妾洪柌宥嗙€敍?
  - `NV12 / YUV420P` 閻╃繝绱?
  - `AVFrame` 瀵洜鏁ゆ径宥囨暏

  - `swscale=0 / display_copy=0`



### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?
- 姒涙顓绘稉濠氭懠妤犲矁鐦夐敍?
  `./build/Debug/modern-video-player.exe --performance-log-check ./samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500`

  缂佹挻鐏夐敍姝歳enderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗video_copy_back_ratio_percent=0`閵嗕梗video_swscale_ratio_percent=0`閵嗕梗display_copy_ratio_percent=0`閵嗕梗result=PASS`閵?
- `SoftwareSDL` fallback 妤犲矁鐦夐敍?
  `$env:MVP_RENDERER_BACKEND='software'; .\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`

  缂佹挻鐏夐敍姝歳enderer_backend=SoftwareSDL`閵嗕梗decoder_backend=D3D11VA`閵嗕梗video_copy_back_ratio_percent=33.5958`閵嗕梗video_swscale_ratio_percent=0`閵嗕梗display_copy_ratio_percent=0`閵嗕梗result=PASS`閵?
- 瀵搫鍩?`D3D11 + Software decode` 閻?`session-check` 娴犲秴褰х拠浣规閳ユ粏鍏橀幍鎾崇磻楠炴儼绻橀崗銉︽尡閺€鎯ф儕閻滎垪鈧繐绱濇稉宥堝厴鐠囦焦妲戦垾婊吳旂€规矮楠囩敮褉鈧繐绱辫ぐ鎾冲鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖?blocker 娓氭繄鍔х€涙ê婀敍灞肩稻瀹歌弓绗夐崘宥呭閸濆秹绮拋?fallback 娑撴槒鐭惧鍕┾偓?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY11_SOFTWARE_DECODE_BLOCKER_AND_FALLBACK_DIRECTION.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?74: Audio-master lateness 閺€鍓佹彛娑?SoftwareSDL 閸戝繑瀚圭拹婵囨箒闂勬劙鍣搁弸?
**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚涵顔款吇瀹稿弶澧滈崝銊﹀絹娴溿倓绗傛稉鈧潪顔剧波閺嬫粣绱濈憰浣圭湴娑撳鎽肩紒褏鐢绘导妯哄 `audio-master lateness / catch-up`閿涘苯鎮撻弮璺烘躬鏉烆垯娆㈤崶鐐衡偓鈧柧鎯т粵閺堝妾洪柌宥嗙€敍灞肩喘閸忓牆鍣虹亸?`swscale` 娑撳孩妯夌粈鍝勭湴濞ｈ鲸瀚圭拹婵勨偓?
- 瑜版挸澧犳妯款吇 `D3D11` 娑撳鎽煎鑼€樼拋銈嗘Ц zero-copy閿涘苯娲滃銈堢箹鏉烆喕绗夐崑姘亣闁插秵鐎敍灞藉涧闁藉牆顕?`Audio` master 閺冭泛绨崪?`SoftwareSDL` fallback 閸嬫艾鐨懠鍐ㄦ纯娴兼ê瀵查妴?


### 閺冦儱绻旀潏鎾冲毉

```text

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=46.4067

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.display_copy_frames=0

performance-log-check.result=PASS



$env:SDL_AUDIODRIVER='dummy'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.audio_output_initialized=true

performance-log-check.clock_source=Audio

performance-log-check.scheduler_late_drops=2

performance-log-check.scheduler_wait_events=274

performance-log-check.result=PASS

```



### 閸掑棙鐎界拋鏉跨秿

- 鏉╂瑨鐤?`SoftwareSDL` 閻ㄥ嫬鍙ч柨顔芥暪閻╁﹣绗夐弰顖氬櫤鐏?`copy-back`閿涘矁鈧本妲搁幎?copy-back 閸氬酣娼伴惃?`swscale + display memcpy` 娑撱倖顔岄悜顓犲仯閻╁瓨甯撮幏鎸庡竴娴滃棎鈧?
- `Display` 閻滄澘婀崷銊︻劀 stride 閻?`YUV420P/NV12` 娑撳﹣绱崗鍫滅箽閻?`AVFrame` 瀵洜鏁ら敍灞芥礈濮?`display_copy_frames=0` 瀹歌尙绮＄拠瀛樻閺勫墽銇氱仦鍌涚箒閹风柉绀夋稉宥呭晙閺?fallback 閻戭厾鍋ｉ妴?
- `NV12` 閸欘垯浜掗惄瀛樺复鐠?`SDL_UpdateNVTexture()` 閸氬函绱漙video_swscale_ratio_percent` 娑旂喎鍑＄紒蹇涙閸?`0`閿涙稑娲栭柅鈧柧鎯у⒖娴ｆ瑤瀵岀憰浣烘懕妫板牆褰ч崜?`av_hwframe_transfer_data()`閵?
- `Audio` master 閺傛壆鐡ラ悾銉ょ瑓閻?dummy audio 妤犲矁鐦夐弰鍓с仛 `wait_events=274`閿涘矁顕╅弰搴″瀻濞堢數鐡戝鍛嚒缂佸繐娲栭崚鏉挎値閻炲棜瀵栭崶杈剧幢閸撳秳绔村▎鈥崇杽閻滀即鍣烽崙铏瑰箛閻ㄥ嫰鐝０鎴滃悏韫囨瑧鐡戝鑼额潶閺堚偓鐏?sleep 闁插繐鐡欐穱顔筋劀閵?
- 姒涙顓?`D3D11 + D3D11VA` 娑撳鎽兼禒宥囧姧娣囨繃瀵?zero-copy閿涘矁绻栨潪顔绘叏閺€瑙勭梾閺堝濡告稉濠氭懠闁插秵鏌婇幏鏍ф礀鏉烆垯娆㈢捄顖氱窞閵?


### 婢跺嫮鎮婄紒鎾寸亯

- `PlayerCore` 閸︺劍妫ょ憴鍡涱暥濠娿倝鏆呴弮璺哄帒鐠?copy-back 閸氬海娈戞潪顖欐 `NV12/YUV420P` 閻╁瓨甯存禍銈囩舶 `SoftwareSDL`閵?
- `Display` 閻滄澘鍑￠弨顖涘瘮 `NV12` SDL texture 閸?`AVFrame` 瀵洜鏁ゆ径宥囨暏閿涘矁绀?stride/娑撳秹鈧倿鍘ょ敮鍐ㄧ湰閹靛秴娲栭柅鈧ǎ杈ㄥ鐠愭縿鈧?
- `Scheduler::pumpRenderOnce()` 閻?`Audio` master 瀹稿弶鏁奸幋鎰瀻濞堢數鐡戝?+ 閸斻劍鈧?late-drop 闂冨牆鈧?+ 閺堚偓鐏?sleep 闁插繐鐡欓妴?
- 瑜版挸澧犵紒鎾诡啈瀹歌尙绮￠弨鑸垫殐閿?
  - 姒涙顓绘稉濠氭懠缂佈呯敾娣囨繃瀵?zero-copy

  - `SoftwareSDL` 閸ョ偤鈧偓闁惧彞绗呮稉鈧梼鑸殿唽閼汇儳鎴风紒顓濈喘閸栨牭绱濇惔鏂剧喘閸忓牆鍙у▔?`copy-back`閿涘矁鈧奔绗夐弰顖滄埛缂侇厼娲跨紒?`swscale`/`display memcpy`



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



---

## 闂傤噣顣?73: SoftwareSDL 閹风柉绀夐柧鎹愮熅闁插繐瀵查妴涓糲heduler 闁插秴鎯庢０鍕暬娑?renderer override

**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚紒褏鐢绘潻浠嬫６ `Display::copyFrameData()` 閸?`Scheduler` 閸ュ搫鐣鹃柌宥呮儙濞嗏剝鏆熼弰顖氭儊娴兼艾顕遍懛鎾彯閻胶宸?4K 鏉堟挸鍤敮褌绗夌粙鍐茬暰閵?
- 瑜版挸澧犳稉濠氭懠瀹歌尙绮＄涵顔款吇閺?`D3D11 + D3D11VA` zero-copy閿涘奔绲炬潪顖欐閸ョ偤鈧偓闁剧偓鐥呴張澶屾埂鐎圭偟绮虹拋鈽呯礉renderer override 娑旂喐鐥呴張澶屾埂濮濓絾甯撮崗銉┾偓澶嬪闁句勘鈧?
- 鏉╂瑨顔€缁?8閵?0 閻愮懓褰ч懗浠嬫浆閹恒劍鏌囬敍灞炬￥濞夋洖婀張顒佹簚缂佹瑥鍤潪顖欐鐠侯垰绶炵€圭偞绁撮崡鐘崇槷閵?
### 閺冦儱绻旀潏鎾冲毉

```text

.\build\Debug\modern-video-player.exe --renderer-fallback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv

renderer-fallback-check.renderer_backend=SoftwareSDL

renderer-fallback-check.decoder_backend=D3D11VA

renderer-fallback-check.fallback_to_sdl=true

renderer-fallback-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=30.1018

performance-log-check.video_swscale_ratio_percent=18.6623

performance-log-check.display_copy_ratio_percent=21.8407

performance-log-check.display_copy_avg_ms=5.69759

performance-log-check.scheduler_video_restart_limit_hits=0

performance-log-check.result=PASS



.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS

```



### 閸掑棙鐎界拋鏉跨秿

- 姒涙顓?`D3D11` 娑撳鎽兼稉瀣剁礉`Display::copyFrameData()` 閺嶈婀版稉宥呮躬閻戭叀鐭惧鍕剁礉瑜版挸澧犻張鍝勬珤娑撳﹦娈戞稉濠氭懠閻″爼顣笟婵囨＋娑撳秵妲告潪顖欐閹风柉绀夐妴?
- 娑撯偓閺冿箑鍨忛崚?`SoftwareSDL`閿涘矂鎽肩捄顖欑窗閸欐ɑ鍨?`copy-back + swscale + display memcpy` 娑撳顔岄崣鐘插閿涘苯鍙炬稉?`Display::copyFrameData()` 閺堫剚婧€鐎圭偞绁村鎻掑窗闁插洦鐗辩粣妤€褰涚痪?`21.84%`閵?
- `Scheduler` 閺傛壆鐡ラ悾銉ょ瑓 `restart_limit_hits=0`閿涘矁顕╅弰搴＄暊娑撳秵妲告潻娆愬閺嶉攱婀伴惃鍕瘜閸ョ媴绱濇担鍡欑崶閸欙綁顣╃粻妤佺槷閸ュ搫鐣惧▎鈩冩殶閺囨潙鐣ㄩ崗銊ｂ偓?
- 閺冄呮畱 fallback 濡偓閺屻儰绠ｉ幍鈧禒銉︾ゴ娑撳秴鍣敍灞肩瑝閺勵垰娲滄稉铏圭波鐠佹椽鏁婃禍鍡礉閼板本妲搁崶鐘辫礋 renderer 闁瀚ㄩ柧鍙ョ閸撳秵鐗撮張顒佺梾閺堝绉风拹?override閵?


### 婢跺嫮鎮婄紒鎾寸亯

- `Display` / `SdlVideoRenderer` / `PlayerCore` / CLI 閻滄澘婀鑼病閼宠棄鐣弫纾嬬翻閸戦缚钂嬫禒鑸垫▔缁€娲懠閻ㄥ嫭瀚圭拹婵堢埠鐠伮扳偓?
- `Scheduler` 閻滄澘鍑￠弨瑙勫灇缁愭褰涙０鍕暬閸掑爼鍣搁崥顖ょ礉楠炶埖鏌婃晶?limit hit 鐠囧﹥鏌囬妴?
- `RendererFactory` 閻滄澘鍑￠弨顖涘瘮 `MVP_RENDERER_BACKEND` 閸?`MVP_D3D11_DRIVER_HINT=software`閿涘畭--renderer-fallback-check` 瑜版挸澧犲鍙変划婢?PASS閵?
- 瑜版挸澧犻崣顖欎簰閺勫海鈥橀崠鍝勫瀻娑撱倖娼紒鎾诡啈閿?
  - 姒涙顓?`D3D11` 娑撳鎽奸敍姘辨埛缂侇厺绻氶悾?zero-copy閿涘奔绗夐崑姘亣闁插秵鐎?
  - `SoftwareSDL` 閸ョ偤鈧偓闁炬拝绱癭Display::copyFrameData()` 瀹稿弶妲搁弰搴ｂ€橀悜顓犲仯閿涘奔绲炬潻妯圭瑝閺勵垰鏁稉鈧悜顓犲仯



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



---

## 闂傤噣顣?72: 妤傛鐖滈悳?4K 闂冪喎鍨€瑰綊鍣洪妴浣藉殰闁倸绨查懞鍌涚ウ娑?copy-back 鐠囧﹥鏌囨晶鐐插繁

**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閸ュ绮垾婊堢彯閻胶宸肩憴鍡涱暥鏉堟挸鍤敮褌绗夌粙鍐茬暰閳ユ繂浠涙导妯哄閿涘奔绗夐惄瀛樺复閸嬫艾銇囬柌宥嗙€敍宀冣偓灞炬Ц閸忓牆鍨介弬?`FrameQueue` 鐎瑰綊鍣洪妴浣藉剹閸樺鈧恭opy-back 閸?scheduler 閺冭泛绨潻娆庣昂閺囧瓨甯存潻鎴犳埂鐎圭偟鎽辨０鍫㈡畱閻愬箍鈧?
- 瑜版挸澧犳い鍦窗瀹歌尙绮￠崗宄邦槵 D3D11 native zero-copy閿涘奔绲炬妯肩垳閻?4K 閻╃鍙х拠濠冩焽鏉╂宸?queue 瀹勬澘鈧鈧垢ush timeout閵嗕浇鍎楅崢瀣搼瀵板懏妞傞梹瑁も偓涔py-back/swscale 閺冨爼妫挎潻娆庣昂閸忔娊鏁幐鍥ㄧ垼閵?
- 閸氬本妞傞敍灞炬￥闂婃娊顣舵潏鎾冲毉閺冩儼铔?`Video` master閿涘ender 缁涘绶熼柅鏄忕帆婢额亞鐭栭敍娑滄儰閸氬孩妞傛稉鈧▎鈥冲涧娑擃澀绔寸敮褝绱濇稊鐔剁窗閺€鎯с亣鏉╄棄鎶氶幎鏍уЗ閵?
### 閺冦儱绻旀潏鎾冲毉

```text

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_native_output_frames=101

performance-log-check.video_copy_back_frames=0

performance-log-check.video_swscale_frames=0

performance-log-check.video_frame_queue_capacity=24

performance-log-check.video_frame_queue_peak_size=23

performance-log-check.video_frame_queue_push_timeouts=0

performance-log-check.scheduler_video_backpressure_wait_ms=1252

performance-log-check.result=PASS



.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

high-bitrate-check.advance_ratio=0.866667

high-bitrate-check.demux_queue_drop_packets=0

high-bitrate-check.result=PASS



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.advance_ratio=0.85

4k-playback-check.late_drops=0

4k-playback-check.demux_queue_drop_packets=0

4k-playback-check.result=PASS



.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000

long-playback-check.advance_ratio=0.938438

long-playback-check.demux_queue_drop_packets=0

long-playback-check.result=PASS

```



### 閸掑棙鐎界拋鏉跨秿

- 瑜版挸澧?4K 娑撳鎽奸柌鍥ㄧ壉闁?`video_copy_back_frames=0`閵嗕梗video_swscale_frames=0`閿涘矁顕╅弰搴ｅ箛閸︺劎婀″锝堢獓閻ㄥ嫭妲?native zero-copy閿涘奔绗夐弰?copy-back 閻戭厾鍋ｉ妴?
- 閸楁洜鍑介幎?4K `FrameQueue` 閺€鎯с亣閿涘奔绱扮憴锕€褰?FFmpeg `Static surface pool size exceeded`閿涙稑娲滃銈堫潒妫版垿妲﹂崚妤€銇囩亸蹇撶箑妞よ鎷?`D3D11VA extra_hw_frames` 閼辨柨濮╅妴?
- `scheduler_video_backpressure_wait_ms` 瀵板牓鐝獮鏈电瑝缁涘绨垾婊勬尡閺€鎯с亼鐠愩儮鈧繐绱濈€瑰啯娲挎径姘愁嚛閺?decoder 鐠烘垵绶卞В?render 韫囶偓绱遍惇鐔割劀鐟曚胶婀呴惃鍕Ц `push_timeout_count` 閸?`demux_queue_drop_packets`閿涘矁绻栨潪顔昏⒈閼板懘鍏樻穱婵囧瘮娑?0閵?
- `Video` master 娑斿澧犳导姘愁唨 24fps 閺嶉攱婀伴弰搴㈡▔鐡掑懘鈧噦绱盽pumpRenderOnce()` 娑撯偓濞嗏€冲涧娑擃澀绔寸敮褌绡冩导姘愁唨 4K 鏉╄棄鎶氭潻鍥ㄥ弮閵嗗倷琚遍懓鍛村厴濮ｆ柡鈧粎鍤庣粙瀣櫢閸氼垱顐奸弫浼存閸掑灈鈧繃娲块幒銉ㄧ箮瑜版挸澧犻惇鐔风杽閻ュ洨濮搁妴?


### 婢跺嫮鎮婄紒鎾寸亯

- `FrameQueue` 閻滄澘鍑￠崗宄邦槵 `peak_size / push_timeout_count` 缂佺喕顓搁敍瀹峆layerCore` 娴兼艾顕遍崙?frame queue capacity/peak/timeout閵?
- `PlayerCore::open()` 娴兼碍鐗撮幑顔肩崯娴ｆ挸鐫橀幀褑顔曠純?frame queue 鐎瑰綊鍣洪敍灞借嫙閸?`D3D11VA` 閹垫挸绱戦崜宥嗘▔瀵繘鍘ょ純?`extra_hw_frames`閵?
- `Scheduler` 閻滄澘鍑￠弨顖涘瘮閼冲苯甯囨潻鐔哥哺娑?`video/audio_backpressure_wait_ms` 缂佺喕顓搁妴?
- `Scheduler::pumpRenderOnce()` 瀹稿弶鏁奸幋鎰剁窗

  - `Video` master 娑撳鏁?wall-clock pacing

  - 閸楁洘顐?pump 閸愬懓绻涚紒顓濇丢瀵啳绻冮張鐔锋姎閻╂潙鍩岄幏鍨煂閸欘垱妯夌粈鍝勬姎

- 瑜版挸澧?4K / 妤傛鐖滈悳?/ 闂€鎸庢閸ョ偛缍婇柈钘夊嚒閹垹顦查柅姘崇箖閿涙稖绻栨潪顔煎⒖娴ｆ瑥浼愭担婊堝櫢閻愰€涚瑝閸愬秵妲搁垾婊勬箒濞屸剝婀?zero-copy閳ユ繐绱濋懓灞炬Ц閸氬海鐢婚弰顖氭儊缂佈呯敾鐞?audio-master 娑撳娲跨紒鍡欐畱 lateness 缁涙牜鏆愰妴?


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



---





## 闂傤噣顣?71: 4K backend session 鐎涙劘绻樼粙瀣偓鈧崙楦跨熅瀵板嫪鎱ㄦ径?
**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚

- 閸?video-only 闂勫秶楠囬崪?demux 闂傘劎顩︾痪鐘蹭焊娑斿鎮楅敍瀹?k-playback-check` 娴犲秶鍔ф径杈Е閿涘苯銇戠拹銉у仯闂嗗棔鑵戦崷?`fallback_ok=false`閵?
- 缂佈呯敾閹峰棗绱戞宀冪槈閸氬骸褰傞悳甯窗backend session 鐎涙劘绻樼粙瀣拱闊偄鍑＄紒蹇斿ⅵ閸?PASS閿涘奔绲?`hard` 娴兼俺绉撮弮鏈电瑝闁偓閸戠尨绱漙soft` 娴兼艾绱撶敮鎼佲偓鈧崙鎭掆偓?
- 鏉╂瑨顕╅弰搴㈢暙閻ｆ瑩妫舵０妯哄嚒缂佸繋绗夐崷銊︽尡閺€鍙ュ瘜闁炬拝绱濋懓灞芥躬濞村鐦?probe 鐎涙劘绻樼粙瀣畱闁偓閸戣櫣鐡ラ悾銉ｂ偓?
### 閺冦儱绻旀潏鎾冲毉

```text

.\build\Debug\modern-video-player.exe --windows-backend-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv

windows-backend-check.hard.mode_ok=true

windows-backend-check.hard.exit_code=0

windows-backend-check.soft.mode_ok=true

windows-backend-check.soft.exit_code=0

windows-backend-check.result=PASS



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.fallback_ok=true

4k-playback-check.result=PASS

```



### 閸掑棙鐎界拋鏉跨秿

- `runWindowsBackendSessionCheck()` 閻ㄥ嫯浜寸拹锝呭涧閺勵垰鎮滈悥鎯扮箻缁嬪濮ら崨濞锯偓婊嗙箹娑?backend 閼宠棄鎯侀崥顖氬З楠炴儼绻橀崗銉︽尡閺€鍓х崶閸欙絺鈧繐绱濋獮鏈电瑝闂団偓鐟曚焦閮ㄩ悽銊︻劀鐢憡鎸遍弨鎯ф珤娴兼俺鐦介惃鍕暚閺佸鏁撻崨钘夋噯閺堢喎娲栭弨鎯邦嚔娑斿鈧?
- 閺冄冪杽閻滄壆娈戦梻顕€顣介弰顖ょ窗probe 缂佹挻鐏夊鑼病閹垫挸宓冮崙鐑樻降娴滃棴绱濇担鍡楃摍鏉╂稓鈻奸張顒冮煩濞屸剝婀侀悽銊旂€规氨娈戦弬鐟扮础缂佹挻娼敍灞筋嚤閼峰鍩楁潻娑氣柤閻鍩岄惃鍕Ц timeout 閹存牕绱撶敮鎼佲偓鈧崙铏圭垳閿涘矁鈧奔绗夐弰?probe 缂佹挻鐏夐妴?
- 閸ョ姳璐?`4k-playback-check` 閹?`mode_ok` 閸?`exit_code==0` 娑撯偓鐠ч鎾奸崗?`fallback_ok`閿涘本澧嶆禒銉ㄧ箹缁鈧偓閸戣櫣鐡ラ悾銉╂晩鐠囶垯绱伴惄瀛樺复鐞涖劎骞囬幋?4K 閸ョ偛缍婃径杈Е閵?


### 婢跺嫮鎮婄紒鎾寸亯

- `src/main.cpp` 娑擃厾娈?`--windows-backend-session-check` 瀹稿弶鏁奸幋鎰瑩閻劑鈧偓閸戦缚鐭惧鍕剁窗閹垫挸宓冪紒鎾寸亯閸?flush閿涘苯鑻熼崷?Windows 娑撳鏁?`TerminateProcess(GetCurrentProcess(), code)` 缁斿宓嗙紒鎾存将 probe 鐎涙劘绻樼粙瀣ㄢ偓?
- `hard/soft` backend session 瑜版挸澧犻柈鍊熷厴缁嬪啿鐣炬潻鏂挎礀 `exit_code=0`閵?
- `--windows-backend-check` 娑?`--4k-playback-check` 瑜版挸澧犲鍙変划婢?PASS閿涘矁顕╅弰搴ょ箹鏉烆喗鐣悾娆忋亼鐠愩儱鍑＄紒蹇旀暪閸欙絻鈧?


### 娣囶喗鏁奸弬鍥︽

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_BACKEND_SESSION_EXIT_FIX.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 闂傤噣顣?70: 闂婃娊顣剁拋鎯ь槵婢惰精瑙﹂弮鍓佹畱鐟欏棝顣?only闂勫秶楠囨稉搴℃礀瑜版帡妫粋浣虹眰閸?
**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚涵顔款吇閸忓牅绗夐崑姘亣闁插秵鐎敍灞肩喘閸忓牆寮懓?ffmpeg/mpv/MPC-HC 閻ㄥ嫭鈧繆鐭鹃敍灞惧Ω閳ユ粓鐓舵０鎴ｎ啎婢跺洤銇戠拹銉︽閻ㄥ嫯顫嬫０?only闂勫秶楠囬垾婵嗘嫲閳ユ粓鐝惍浣哄芳閸ョ偛缍婄拠顖氬灲閳ユ繆绻栨稉鈧仦鍌涙暪閸欙絻鈧?
- 瑜版挸澧犻張鍝勬珤娑?`WASAPI` 闂婃娊顣剁拋鎯ь槵娑撳秴褰查悽顭掔礉鐟欏棝顣堕弬鍥︽鐎圭偤妾稉濠傚嚒缂佸繗鍏橀棃鐘靛箛閺堝瀵岄柧鍓ф埛缂侇厽鎸遍敍灞肩稻 `open()` 鐠囶厺绠熼妴涔ock source 閸滃苯娲栬ぐ鎺楁，缁備線鍏樺▽鈩冩箒閹跺﹨绻栨稉顏嗗Ц閹焦妯夊蹇氥€冩潏鎯у毉閺夈儯鈧?
- 鏉╂瑥顕遍懛鎾彯閻胶宸肩槐鐘虫綏濡偓閺屻儵鍣?`demux_dropped_packets` 娑撴槒顩﹂悽?ignored audio packets 缂佸嫭鍨氶敍灞藉祱娴犲秶鍔х悮顐㈢秼閹存劘鍎楅崢瀣亼鐠愩儯鈧?
### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

瀹稿弶鍨氶崝鐔烘晸閹存劑鈧? 娑擃亣顒熼崨濠忕礉0 娑擃亪鏁婄拠?


.\build\Debug\modern-video-player.exe --1080p60-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

1080p60-check.result=PASS

1080p60-check.audio_output_initialized=false

1080p60-check.video_only_fallback=true

1080p60-check.clock_source=Video

1080p60-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

high-bitrate-check.result=PASS

high-bitrate-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000

long-playback-check.result=PASS

long-playback-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.result=FAIL

4k-playback-check.fallback_ok=false

```



### 閸掑棙鐎界拋鏉跨秿

- `initDecoders()` 閺堫剝闊╁鑼病閹?`audio_player_->isInitialized()` 閹貉冨煑闂婃娊顣剁憴锝囩垳閺勵垰鎯侀崥顖滄暏閿涘本澧嶆禒銉ㄧ箹鏉烆喕绗夐棁鈧憰浣稿帥閸嬫艾銇囩憴鍕侀柌宥嗙€敍娑氭埂濮濓絿宸遍惃鍕Ц `open()` 鐏炲倿娼伴惃鍕▔瀵繒鐡ラ悾銉ｂ偓?
- 妤傛鐖滈悳鍥梾閺屻儱銇戠拹銉ф畱閺冄勭壌閸ョ姳绗夐弰顖濐潒妫版垿妲﹂崚妤冩埂閻ㄥ嫯顫﹂崢瀣瀻閿涘矁鈧本妲?disabled audio path 娴溠呮晸娴滃棗銇囬柌?`demux_ignored_packets`閿涘苯寮电悮顐ｆ＋闂傘劎顩︾拠顖氱秼閹存劗绮烘稉鈧惃?demux drop閵?
- video-only 閸︾儤娅欐稉瀣埛缂侇厺濞囬悽?`System` clock 娴兼俺顔€閹绢厽鏂侀幒銊ㄧ箻閸滃瞼婀＄€圭偞瑕嗛弻?PTS 閼磋精濡敍灞芥礈濮濄倝娓剁憰浣瑰Ω閺冪娀鐓舵０鎴ｇ翻閸戠儤妞傞惃鍕瘜閺冨爼鎸撻崚鍥у煂 `Video`閵?
- `4k-playback-check` 瑜版挸澧犻崜鈺€缍戞径杈Е閻愮懓鍑＄紒蹇曠級鐏忓繐鍩?`runBackendSessionSubprocess()` 閻╃鍙ч惃?`fallback_ok` 鐎涙劘绻樼粙瀣熅瀵板嫸绱濇稉宥呭晙閺勵垵绻栨潪顔绘叏婢跺秷瀵栭崶鎾櫡閻ㄥ嫯顕ら崚銈夋６妫版ǜ鈧?


### 婢跺嫮鎮婄紒鎾寸亯

- `PlayerCore::open()` 閻滄澘鍑￠崠鍝勫瀻閿?
  - 鐟欏棝顣堕弬鍥︽闂婃娊顣剁拋鎯ь槵婢惰精瑙﹂敍姝竌rning + video-only 缂佈呯敾閹绢厽鏂?
  - 闂婃娊顣?only 閺傚洣娆㈤棅鎶筋暥鐠佹儳顦径杈Е閿涙氨娲块幒銉ャ亼鐠?
- `DiagnosticsSnapshot` 閻滄澘鍑￠弬鏉款杻 `audio_output_initialized / video_only_fallback / clock_source`閿涘本鎸遍弨鍓ц濡偓閺屻儰绱伴惄瀛樺复閹垫挸宓冭ぐ鎾冲闂勫秶楠囧Ο鈥崇础閵?
- `1080p60-check`閵嗕梗high-bitrate-check`閵嗕梗long-playback-check` 瀹稿弶鏁兼稉鍝勫涧閻?`demux_queue_drop_packets`閿涘苯鑻熸穱婵堟殌 `demux_dropped_packets` 娴ｆ粈璐熼幀濠氬櫤鐟欏倹绁撮崐绗衡偓?
- 瑜版挸澧犳宀冪槈缂佹挻鐏夌悰銊︽閿涙俺绻栨潪顔绘叏婢跺秴鍑＄紒蹇斿Ω閳ユ粓鐓舵０鎴ｎ啎婢跺洨宸辨径鍗烆嚤閼峰娈戦崑鍥с亼鐠愩儮鈧繀绮犳妯肩垳閻滃洤娲栬ぐ鎺楀櫡閸撱儳顬囬崙鍝勫箵閿涙稑鎮楃紒顓烆洤閺嬫粎鎴风紒顓濈喘閸栨牭绱濇惔鏂剧喘閸忓牊甯撻弻?4K fallback 鐎涙劘绻樼粙瀣熅瀵板嫬鎷伴弴鎾毐閺堢喓娈?queue serial 鐠佹崘顓搁妴?


### 娣囶喗鏁奸弬鍥︽

- `include/core/player_core.h`

- `src/core/player_core.cpp`

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_AUDIO_FALLBACK_AND_REGRESSION_GATES.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 闂傤噣顣?69: PlayerCore 閸嬫粍鎸遍弨璺哄經閵嗕礁瀵橀梼鐔峰灙閹碘偓閺堝娼堟稉?Clock/Demuxer 鐠佹崘顓搁崐杞版叏婢?


**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閹跺﹣绗傛稉鈧潪顔碱吀閺屻儱褰傞悳鎵畱鐠佹崘顓搁崐铏规纯閹恒儴鎯ら崷棰佹叏閹哄绱濋柌宥囧仯閸栧懏瀚敍娆礝F/閸嬫粍鎸辩痪璺ㄢ柤閻樿埖鈧椒绗夌€瑰本鏆ｉ妴涔acketQueue` 閸樼喎顫愰幐鍥嫛閹碘偓閺堝娼堥妴涔lock` system-clock 閺冨爼妫跨捄鍐插綁閿涘奔浜掗崣?`Demuxer::open()` 閻ㄥ嫯鍤滈柨渚€顥撻梽鈹库偓?
- 鏉╂瑧琚梻顕€顣芥稉宥勭鐎规矮绱扮粩瀣祮娴ｆ挾骞囨稉铏圭椽鐠囨垵銇戠拹銉礉娴ｅ棔绱伴崷?replay / seek / EOF / close 缁涘鏆遍張鐔荤箥鐞涘矁鐭惧鍕瑐缁夘垳鐤幋鎰旂€规碍鈧冩嫲閸欘垳娣幎銈嗏偓褔妫舵０妯糕偓?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

瀹稿弶鍨氶崝鐔烘晸閹存劑鈧?
0 娑擃亣顒熼崨?
0 娑擃亪鏁婄拠?
```



### 閸掑棙鐎界拋鏉跨秿

- EOF 閼奉亜濮╅崑婊勬尡閸欐垹鏁撻崷?scheduler render 缁捐法鈻奸崘鍜冪礉娑撳秷鍏橀惄瀛樺复閸氬本顒?`stop()`閿涙稑鎯侀崚?render 缁捐法鈻兼导?join 閼奉亜绻侀妴鍌涙＋鐎圭偟骞囬崶鐘愁劃闁偓閸栨牗鍨氶垾婊冨涧閺€鍦Ц閹讲鈧繄娈戦崡濠備粻閺堥缚鐭惧鍕┾偓?
- `AVPacket*` 閺€鎹愮箻闁氨鏁ら梼鐔峰灙閸氬函绱漙clear()` / 闂冪喎鍨弸鎰€稉宥勭窗鐢噣銆嶉惄顕€鍣撮弨?FFmpeg 閸栧拑绱眘eek閵嗕够top閵嗕恭lose 闁垝绱伴崨鎴掕厬鏉╂瑦娼▔鍕础鐠侯垰绶為妴?
- `Clock` 閻?system-clock pause/speed 濞屸剝婀侀崗鍫濇祼閸栨牗妫弮鍫曟？閸╁搫鍣敍宀€鍑界憴鍡涱暥閹绢厽鏂侀弮鏈电窗閸戣櫣骞囬弳鍌氫粻閺冨爼妫块崐鎺椻偓鈧幋鏍р偓宥夆偓鐔峰瀼閹广垼鐑﹂弮韬测偓?
- `Demuxer::open()` 閹镐線鏀ｇ拫鍐暏 `close()` 閺勵垱甯撮崣锝囬獓閼奉亪鏀ｉ敍宀冩閻掓湹瀵屽ù浣衡柤娑撳秴鐖舵径宥囨暏閸氬奔绔寸€圭偘绶ラ敍灞肩稻鐠佹崘顓告稉濠傜箑妞ょ粯鏁归崣锝冣偓?


### 婢跺嫮鎮婄紒鎾寸亯

- `PlayerCore` 閻滄澘婀崗宄邦槵 deferred stop + worker reap 閺堝搫鍩楅敍瀛峅F閵嗕腐ext/Previous閵嗕傅uit 缁涘鐭惧鍕瑝閸愬秶鏆€娑撳鍓扮痪璺ㄢ柤閻樿埖鈧緤绱濋崥搴ｇ敾 replay / restart 閸欘垰鐣ㄩ崗銊╁櫢閸氼垬鈧?
- `PacketQueue` 瀹稿弶鏁兼稉?`unique_ptr<AVPacket, AvPacketDeleter>`閿涘畭ThreadSafeQueue` 閸氬本顒炵悰銉ょ瑐瀵ゆ儼绻?move push閿涘苯甯囩紓鈺佸瘶閻㈢喎鎳￠崨銊︽埂閸ョ偛缍?RAII閵?
- `Scheduler` 瀹稿弶鏌婃晶鐐茬磽濮濄儱浠犻張鍝勫弳閸欙絽鑻熼崷銊╁櫢閸氼垰澧犻崶鐐存暪閺?worker閿涘畭Clock` 瀹歌弓鎱ㄦ径?system-clock pause/speed 閸╁搫鍣弴瀛樻煀閵?
- `Demuxer::open()` 娑撳秴鍟€閸︺劑鏀ｉ崘鍛村櫢閸?`close()`閿涙稒鏆ｅ銉р柤 Windows `Debug` 閸忋劑鍣洪柌宥呯紦娴犲秳绻氶幐?`0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- include/core/scheduler.h

- include/thread_safe_queue.h

- src/core/player_core.cpp

- src/core/scheduler.cpp

- src/core/clock.cpp

- src/demuxer.cpp

- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---



## 闂傤噣顣?68: MSVC warning debt 閸掑棗鐪板〒鍛倞閿涘湑4819 / C4996 / C4706閿?


**閺冦儲婀?*: 2026-03-18

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾濞撳懐鎮婇崗銊ョ湰 warning debt閿涘苯鑻熸导妯哄帥婢跺嫮鎮?`C4819` 缂傛牜鐖滈崨濠咁劅閿涘奔浜掗崣濠勵儑娑撳鏌?/ 閺堫剙婀?`C4996` 閻ㄥ嫬鍨庣仦鍌涗笉閻炲棎鈧?
- 瑜版挸澧?Windows CI 瀹歌弓绗夐崘宥嗘箒缂傛牞鐦?blocker閿涘奔绲?warning 閹鍣烘潻鍥彯閿涘奔绱伴幒鈺冩磰閻喎鐤勯崶鐐茬秺娣団€冲娇閵?
- 闂団偓鐟曚礁婀稉宥呮礀闁偓閺冦垺婀?D3D11/鐎涙绠烽弨鐟板З閻ㄥ嫬澧犻幓鎰瑓閿涘本濡搁張顒€婀撮崣顖欐叏 warning 閸滃瞼顑囨稉澶嬫煙 warning 闂呮梻顬囩粵鏍殣娑撯偓鐠х柉藟姒绘劑鈧?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

瀹稿弶鍨氶崝鐔烘晸閹存劑鈧?
0 娑擃亣顒熼崨?
0 娑擃亪鏁婄拠?
```



### 閸掑棙鐎界拋鏉跨秿

- `C4819` 閻ㄥ嫭鐗宠箛鍐х瑝閺勵垰宕熸稉顏呯爱閺傚洣娆㈤崸蹇斿竴閿涘矁鈧本妲?MSVC 娴犲秵瀵滈張顒€婀存禒锝囩垳妞や絻顕伴崣?UTF-8 濠ф劖鏋冩禒璁圭幢鏉╂瑧琚梻顕€顣芥惔鏂剧喘閸忓牓鈧俺绻冪紓鏍槯闁銆嶇紒鐔剁濞岃崵鎮婇妴?
- 缁楊兛绗侀弬?warning 娑撳秴绨查棃鐘辨叏閺€?FFmpeg / Quill 濠ф劗鐖滅憴锝呭枀閿涘本娲跨粙鍐参曢惃鍕粵濞夋洘妲搁幎濠勵儑娑撳鏌熸径瀛樻瀮娴犺埖鏂侀崚鏉款樆闁?warning 鐏炲偊绱濋悽?MSVC 閻?`/external:*` 閺堝搫鍩楃紒鐔剁闂呮梻顬囬妴?
- 閺堫剙婀?`C4996` 閸?`C4706` 娴犲秶鍔ф惔鏃囶嚉闁劒閲滄穱顔藉竴閿涘苯娲滄稉楦跨箹娴?warning 閻╁瓨甯撮崣宥嗘Ё妞ゅ湱娲伴懛顏囬煩娴狅絿鐖滅拹銊╁櫤閵?
- 閺堫剝鐤嗛拃钘夋勾閸氬函绱濋崗銊╁櫤 `Rebuild` 瀹歌尪鎻崚?`0 warning / 0 error`閿涘矁顕╅弰搴″瀻鐏炲倻鐡ラ悾銉ユ嫲閺堫剙婀存穱顔碱槻闁棄鍑￠悽鐔告櫏閵?


### 婢跺嫮鎮婄紒鎾寸亯

- `CMakeLists.txt` 瀹歌弓璐?MSVC 閻╊喗鐖ｉ崥顖滄暏 `/utf-8 /external:anglebrackets /external:W0`閿涘本婀伴崷鎵椽閻礁鎲＄拃锔跨瑢缁楊兛绗侀弬鐟般仈閺傚洣娆?warning 瀹稿弶妯夐拋妤佹暪閸欙絻鈧?
- `src/logger.cpp` 瀹稿弶鏁兼稉鍝勭暔閸忋劎骞嗘晶鍐ㄥ綁闁插繗顕伴崣?helper閿涘本绔婚悶鍡樻拱閸?`getenv` 閸涘﹨顒熼妴?
- `src/subtitle/srt_parser.cpp` 娑?`src/subtitle/ass_parser.cpp` 瀹歌尪藟娑?Windows / 闂?Windows 閻?`sscanf_s` / `std::sscanf` 閸掑棙鏁妴?
- `src/main.cpp` 閻?`signalHandler` 閸欏倹鏆熼崪?`src/core/player_core.cpp` 閻?demux push 瀵邦亞骞嗗鍙夋暭閸愭瑱绱濆〒鍛竴閺堫剙婀撮崜鈺€缍戦惃?`C4100 / C4706`閵?
- 瑜版挸澧?Windows `Debug` 閸忋劑鍣洪柌宥呯紦瀹歌尙绮℃潏鎯у煂 `0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?


### 娣囶喗鏁奸弬鍥︽

- CMakeLists.txt

- src/logger.cpp

- src/main.cpp

- src/subtitle/srt_parser.cpp

- src/subtitle/ass_parser.cpp

- src/core/player_core.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---

## 闂傤噣顣?67: ASS 閺嶅洨顒风憴锝嗙€芥稉?UTF-16 鐎涙绠烽懠鍐ㄦ纯娣囶喗顒?


**閺冦儲婀?*: 2026-03-18

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- D3D11 閸樼喓鏁撶€涙绠烽柧鎯у嚒缂佸繑甯撮崗?`ASS/SSA`閿涘奔绲?override 閺嶅洨顒风憴锝嗙€界€?`\fnArial`閵嗕梗\rDefault` 鏉╂瑧琚槐褍鍣鹃崘娆愮《娑撳秵顒滅涵顕嗙礉鐢瓕顫嗛弽宄扮础娴兼俺顫﹂棃娆撶帛韫囩晫鏆愰妴?
- `SubtitleTextRun` 閻ㄥ嫬灏梻鎾毐鎼达箑顩ч弸婊呮埛缂侇厽閮ㄩ悽?UTF-8 code point閿涘矁鈧本瑕嗛弻鎾额伂閻╁瓨甯撮幐?DirectWrite 閻?UTF-16 range 娴ｈ法鏁ら敍灞芥皑娴兼艾婀?emoji閵嗕焦澧跨仦?CJK 閹存牕鍙炬禒鏍姜 BMP 鐎涙顑佹稉濠傚毉閻滅増鐗卞蹇涙晩娴ｅ秲鈧?
- 閻劍鍩涚憰浣圭湴閸︺劌澧犳稉鈧幍瑙勫絹娴溿倕鎮楃紒褏鐢婚幎濠傚⒖娴ｆ瑦顒滅涵顔解偓褔妫舵０妯荤閹哄绱濋獮鍓佺舶閸戦缚绻栨潪顔煎⒖娴ｆ瑨藟娑撲胶娈戦幓鎰唉閸涙垝鎶ら妴?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

瀹稿弶鍨氶崝鐔烘晸閹存劑鈧?
167 娑擃亣顒熼崨?
0 娑擃亪鏁婄拠?
```



### 閸掑棙鐎界拋鏉跨秿

- 鏉╂瑨鐤嗘径宥嗙叀濞屸剝婀侀崣鎴犲箛閺傛壆娈戞妯哄祫閸愬懎鐡ㄥ▔鍕础閻愮櫢绱遍梻顕€顣介梿鍡曡厬閸︺劌鐡ч獮鏇☆嚔娑斿顒滅涵顔解偓褝绱濋懓灞肩瑝閺?COM/AVFrame 閻㈢喎鎳￠崨銊︽埂缁狅紕鎮婇妴?
- `ASS/SSA` override 鐟欙絾鐎介崳銊︻劃閸撳秵濡搁弽鍥╊劮閸氬秷顕伴崣鏍ㄥ灇閳ユ粏绻涚紒顓熸殶鐎?+ 鏉╃偟鐢荤€涙鐦濋垾婵撶礉閸ョ姵顒?`fn/r` 鏉╂瑧琚崗浣筋啅閻╁瓨甯寸捄鐔封偓鑲╂畱閺嶅洨顒锋导姘Ω閸婂ジ顩诲▓浣冾嚖閸氱偠绻橀弽鍥╊劮閸氬秲鈧?
- `SubtitleTextRun.start/length` 閻╊喖澧犻惃鍕埂濮濓絾绉风拹纭呪偓鍛Ц Windows DirectWrite閿涘矁鈧苯鐣犵憰浣圭湴閻ㄥ嫭鏋冮張顒冨瘱閸ユ潙宕熸担宥嗘Ц UTF-16 code unit閿涘奔绗夐弰?UTF-8 code point閵?
- 瑜版挸澧犻崗銊╁櫤闁插秴缂撳鏌ュ櫢閺備即鈧俺绻冮敍娑樺⒖娴?167 娑?warning 娑撴槒顩﹂弶銉ㄥ殰缁楊兛绗侀弬鐟般仈閺傚洣娆㈤敍鍦楩mpeg / Quill閿涘鈧線銆嶉惄顔煎敶婢舵艾顦╁┃鎰垳閻?C4819 缂傛牜鐖滈崨濠咁劅閵嗕椒浜掗崣濠傜毌闁插繑妫﹂張澶屾畱 C4996/C4100 閹绘劗銇氶敍灞炬拱鏉烆喗婀幍鈺傛殠閹存劖鏌婇惃鍕€娲▎婵夌偑鈧?


### 婢跺嫮鎮婄紒鎾寸亯

- `src/subtitle/ass_parser.cpp` 瀹歌尪藟娑撳﹤鐖堕悽?ASS 閺嶅洨顒烽崜宥囩磻閸栧綊鍘ら敍灞兼叏婢?`\fnArial`閵嗕梗\rDefault`閵嗕梗\1c&H...&` 缁涘鐖ｇ粵鍓ф畱鐠囧棗鍩嗙捄顖氱窞閵?
- `src/subtitle/ass_parser.cpp` 娑?`src/render/d3d11_video_renderer.cpp` 瀹歌尙绮烘稉鈧担璺ㄦ暏 UTF-16 code unit 鐠侊紕鐣?run 闂€鍨閿涘瞼鈥樻穱婵囩壉瀵繗瀵栭崶缈犵瑢 DirectWrite 娑撯偓閼疯揪绱遍獮鍫曘€庨幍瀣閻炲棔绨?`ass_parser.cpp` 閸愬懏婀伴崷?`sscanf` / 鐏炩偓闁劌褰夐柌蹇涗紕閽勮棄鎲＄拃锔衡偓?
- 娑撱倓閲滃┃鎰垳閺傚洣娆㈤張顐㈢啲閻ㄥ嫬顦挎担娆戔敄鐞涘苯鍑″〒鍛倞閿涘奔绻氶幐浣规拱鏉?diff 閸欘亜瀵橀崥顐ｆ箒閺佸牊鏁奸崝銊ｂ偓?
- 鏉╂瑨鐤嗛崜鈺€缍戠悰銉ょ閻滄澘婀崣顏勫瘶閸氼偀鈧穾SS 閺嶅洨顒风憴锝嗙€芥穱顔筋劀 + UTF-16 閼煎啫娲挎穱顔筋劀 + 閺傚洦銆傞崥灞绢劄閳ユ繐绱濋崣顖滃缁斿褰佹禍銈冣偓?


### 娣囶喗鏁奸弬鍥︽

- src/subtitle/ass_parser.cpp

- src/render/d3d11_video_renderer.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---

## 闂傤噣顣?66: 閸忋劌鐪弸鍕紦闂冭顢ｅ〒鍛倞娑?ASS/SSA 閸樼喓鏁?D3D11 鐎涙绠烽柧?


**閺冦儲婀?*: 2026-03-18

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犲銉ょ稊閺嶆垼娅ч悞璺哄嚒缂佸繐鍙挎径鍥у斧閻?D3D11 鐟欏棝顣堕崨鍫㈠箛閼宠棄濮忛敍灞肩稻閸忋劌鐪?`Debug|x64` 閺嬪嫬缂撴禒宥堫潶婢舵艾顦╂径瀛樻瀮娴?濠ф劖鏋冩禒鍓佹畱缂傛牜鐖滅拠顖濐嚢闂冭顢ｉ敍灞筋嚤閼峰瓨妫ゅ▔鏇犳暏閺佹潙浼愮粙瀣波閺嬫粓鐛欑拠浣锋唉娴犳濮搁幀浣碘偓?
- D3D11 閸樼喓鏁撶€涙绠烽柧鐐劃閸撳秴褰х憰鍡欐磰缁绢垱鏋冮張顒€褰旈崝鐙呯礉`.ass/.ssa` 閻ㄥ嫭鐗卞蹇嬧偓浣哥暰娴ｅ秲鈧礁鐪扮痪褍鎷版径?cue 閸氬苯鐫嗛懗钘夊鏉╂ɑ鐥呴張澶庣箻閸?native renderer閵?
- 婢舵牗瀵曠€涙绠烽懛顏勫З閹恒垺绁存稉搴㈡▔瀵繐濮炴潪浠嬫付鐟曚浇顩惄?`.ass`閵嗕梗.ssa`閵嗕梗.srt` 娑撳顫掗弽鐓庣础閿涘矁鈧奔绗夐弰顖氫粻閻ｆ瑥婀禒鍛存桨閸?SRT 閻ㄥ嫮濮搁幀浣碘偓?


### 閺冦儱绻旀潏鎾冲毉

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

瀹稿弶鍨氶崝鐔烘晸閹存劑鈧?
0 娑擃亣顒熼崨?
0 娑擃亪鏁婄拠?
```



### 閸掑棙鐎界拋鏉跨秿

- 閺嬪嫬缂撻梼璇差敚閻ㄥ嫭鐗撮崶鐘辩瑝閸?D3D11 娑撳鎽奸柅鏄忕帆閿涘矁鈧苯婀径姘槱鐢缚鑵戦弬鍥ㄦ暈闁插﹦娈戞径瀛樻瀮娴?濠ф劖鏋冩禒鎯邦潶瑜版挸澧?MSVC/娴狅絿鐖滄い鐢电矋閸氬牐顕ょ拠璇叉倵鐟欙箑褰傞崗銊ョ湰鐠囶厽纭堕柨娆掝嚖閵?
- 閺冄冪摟楠炴洘膩閸ㄥ褰ч幍鑳祰缁绢垱鏋冮張顒婄礉`IVideoRenderer` 娑旂喎褰ч幒銉︽暪閸楁洖鐡х粭锔胯閿涘苯娲滃?`PlayerCore` 閺冪姵纭堕幎?ASS/SSA 閻ㄥ嫭鐗卞蹇嬧偓浣哥湴缁狙冩嫲鐎规矮缍呯拠顓濈疅闁礁鍙?D3D11 濞撳弶鐓嬮崳銊ｂ偓?
- 閺冨爼妫跨痪鑳掗弸鎰斧閺堫剙褰ч棃銏犳倻閸楁洘娼ú璇插З鐎涙绠烽敍灞肩瑝鐡掑厖浜掔悰銊ㄦ彧 ASS/SSA 鐢瓕顫嗛惃鍕樋 cue 閸氬苯鐫嗛崷鐑樻珯閵?
- 閺堚偓閸氬牓鈧倻娈戞穱顔碱槻鐠侯垰绶炴稉宥嗘Ц閹?ASS/SSA 閸愬秹鈧偓閸?SDL閿涘矁鈧本妲搁幍鈺佺潔鐎涙绠风€电钖勫Ο鈥崇€烽崥搴ｆ埛缂侇叀铔嬮崥灞肩閺?DXGI swap chain backbuffer 閸樼喓鏁撻崣鐘插闁句勘鈧?


### 婢跺嫮鎮婄紒鎾寸亯

- 濞撳懐鎮婃禍鍡楀弿鐏炩偓閺嬪嫬缂撻梼璇差敚閺傚洣娆㈡稉顓犳畱缂傛牜鐖滅拠顖濐嚢闂傤噣顣介敍灞句划婢跺秴鐣弫纾嬓掗崘铏煙濡楀牊鐎鍝勭唨缁捐￥鈧?
- 閺傛澘顤?`AssParser`閿涘苯鑻熼幍鈺佺潔 `SubtitleStyle / SubtitleTextRun / SubtitleItem` 娴犮儲澹欐潪?ASS/SSA 閺嶅嘲绱℃穱鈩冧紖閵?
- `PlayerCore` 閻滄澘婀导姘暪闂嗗棗顦块弶鈥崇秼閸撳秵绺哄ú璇茬摟楠炴洩绱濋柅姘崇箖 `IVideoRenderer::setSubtitleItems()` 閹跺﹦绮ㄩ弸鍕鐎涙绠风€电钖勯柅浣稿弳濞撳弶鐓嬮崳銊ｂ偓?
- `D3D11VideoRenderer` 瀹告彃婀崢鐔烘晸 D3D11/D2D 鐠侯垰绶炴稉顓熸暜閹?ASS/SSA 鐢摜鏁ら弽宄扮础鐎涙绠风紒妯哄煑閿涙艾锝為崗鍛偓浣瑰伎鏉堝箍鈧線妲捐ぐ渚库偓浣藉剹閺咁垱顢嬮妴浣割嚠姒绘劑鈧礁鐣炬担宥呮嫲 run 缁狙冪摟娴ｆ挻鐗卞蹇嬧偓?
- `VideoPlayer` 娑?`main.cpp` 瀹稿弶鏁幐?`.ass/.ssa/.srt` 閸旂姾娴囨稉搴ゅ殰閸斻劍甯板ù瀣剁幢鐎瑰本鏆?`MSBuild` 妤犲矁鐦夐柅姘崇箖閵?
---

## 闂傤噣顣?57: D3D11 閸樼喓鏁?GPU 濞撳弶鐓嬮柧鎹愃夋?


**閺冦儲婀?*: 2026-03-18

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犳禒鎾崇氨瀹歌尙绮￠崗宄邦槵閸樼喓鏁?D3D11 鐟欏棝顣堕棃銏犳啛閻滈绗?D3D11VA 鐠佹儳顦崗鍙橀煩閿涘奔绲剧€涙绠烽崷?D3D11 濞撳弶鐓嬮崳銊ュ敶閸欘亝婀侀悩鑸碘偓浣稿晸閸忋儻绱濆▽鈩冩箒閻喐顒滅紒妯哄煑閸?backbuffer閵?
- 閺傚洦銆傛禒宥勭箽閻ｆ瑢鈧窉3D11 閸欘亝妲?SDL 閸栧懓顥婇崳銊⑩偓婵堟畱閺冄呯波鐠佺尨绱濆鑼病娑撳骸缍嬮崜宥勫敩閻椒绨ㄧ€圭偘绗夋稉鈧懛娣偓?
- 閻劍鍩涢惄顔界垼閺勵垱瀣侀崚棰佺閺夆€崇暚閺佸娈戦妴浣哄缁斿娈戦妴浣稿斧閻?D3D11 GPU 濞撳弶鐓嬮柧鎾呯礉閸ョ姵顒濋棁鈧憰浣瑰Ω鐎涙绠烽崣鐘插娑撳氦顕╅弰搴㈡瀮濡楋絼绔寸挧鐤夋鎰┾偓?


### 閺冦儱绻旀潏鎾冲毉

```

Native D3D11 renderer initialized: window=...

D3D11VA decoder bound to renderer-owned D3D11 device

```



### 閸掑棙鐎界拋鏉跨秿

- 鐟欏棝顣舵稉濠氭桨瀹歌尙绮￠崣顖欎簰閻╁瓨甯村☉鍫ｅ瀭 `AV_PIX_FMT_D3D11`閿涘苯鑻熺憰鍡欐磰 `NV12 / P010 / P016` 绾兛娆㈤棃銏ゅ櫚閺嶅嚖绱遍惇鐔割劀缂傚搫褰涢弰顖氱摟楠炴洖褰旈崝鐘辩矝閺堫亣鎯ら崚鏉垮斧閻㈢喖鎽奸崘鍛偓?
- 閺堚偓閸氬牓鈧倻娈戠悰銉╃秷閺傜懓绱￠弰顖氭躬 renderer-owned DXGI swap chain backbuffer 娑撳﹤绱╅崗?D2D1 / DirectWrite 閺傚洦婀扮紒妯哄煑閿涘矁鈧奔绗夐弰顖炩偓鈧崶?`Display` 閹?SDL texture 閸欑姴濮為妴?
- `PlayerCore` 閻?copy-back 閸掑棙鏁棁鈧憰浣风箽閻ｆ瑱绱濇担婊€璐熼垾婊嗩潒妫版垶鎶ら梹婊冪磻閸?閺嶇厧绱℃稉宥嗘暜閹镐讲鈧繃妞傞惃鍕绾喖娲栭柅鈧潏鍦櫕閵?
- 鐎规艾鎮滄宀冪槈鐞涖劍妲?`src/render/d3d11_video_renderer.cpp` 鐠囶厽纭剁紓鏍槯闁俺绻冮敍娑欐殻瀹搞儳鈻奸弸鍕紦婢惰精瑙︽禒宥嗘降閼奉亜宸婚崣鍙夊疮閸у繐銇旈弬鍥︽閿涘矁鈧奔绗夐弰顖涙拱濞?D3D11 娣囶喗鏁奸張顒冮煩閵?


### 婢跺嫮鎮婄紒鎾寸亯

- `D3D11VideoRenderer` 閺傛澘顤?D2D1 / DirectWrite 鐎涙绠风紒妯哄煑鐠у嫭绨崪灞炬畯閸嬫粍鈧礁宓嗛弮鍫曞櫢缂佹ǜ鈧?
- `CMakeLists.txt` 鐞涖儵缍?`d2d1`閵嗕梗dwrite` 闁剧偓甯存笟婵婄閵?
- 閺傛澘顤?`docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`閿涘苯鑻熺紒?`PLAYERCORE_DAY4_RENDERER_ANALYSIS.md` 閸旂姴宸婚崣鑼额嚛閺勫簺鈧?
---



## 闂傤噣顣?12: 娴间椒绗熺痪褍顦跨痪璺ㄢ柤閺嬭埖鐎柌宥嗙€?


**閺冦儲婀?*: 2026-02-27

**閻樿埖鈧?*: 瀹告彃鐣幋?


### 闂傤噣顣介幓蹇氬牚

閸樼喐婀侀弸鑸电€€涙ê婀禒銉ょ瑓闂傤噣顣介敍?
1. 缂佸嫪娆㈤懕宀冪煑娑撳秵绔婚弲甯礉VideoPlayer 閹垫寧濯存潻鍥ь樋閼卞矁鐭?
2. 缁捐法鈻煎Ο鈥崇€锋径宥嗘絽閿涘矂姣︽禒銉ф樊閹?
3. 閸愬懎鐡ㄧ粻锛勬倞鐎硅妲楅崙娲晩閿涘苯顕遍懛鏉戝蓟闁插秹鍣撮弨鍓х搼 bug



### 鐟欙絽鍠呴弬瑙勵攳

闁插秵鐎稉杞扮磼娑撴氨楠囨径姘卞殠缁嬪鐏﹂弸鍕剁礉瀵洖鍙嗘禒銉ょ瑓閺傛壆绮嶆禒璁圭窗



1. **Demuxer** - 鐟欙絽鐨濈憗鍛珤閿涘苯鐨濈憗?AVFormatContext 閻ㄥ嫯顕伴崣鏍ㄦ惙娴?
2. **DecoderWorker** - 鐟欙絿鐖滃銉ょ稊缁捐法鈻奸敍灞界殱鐟佸懎宕熸稉顏呯ウ閻ㄥ嫯袙閻線鈧槒绶?
3. **ThreadSafeQueue** - 缁捐法鈻肩€瑰鍙忛梼鐔峰灙濡剝婢?
4. **Clock** - 閺冨爼鎸撻崥灞绢劄閸ｎ煉绱濈粻锛勬倞闂婂疇顫嬫０鎴濇倱濮?


### 閺嬭埖鐎崶?
```

VideoPlayer (娑撶粯甯堕崚璺烘珤)

    閳规壕鏀㈤埞鈧?Demuxer (鐟欙絽鐨濈憗鍛珤) 閳?PacketQueue

    閳规壕鏀㈤埞鈧?DecoderWorker (鐟欏棝顣剁憴锝囩垳缁捐法鈻?

    閳规壕鏀㈤埞鈧?DecoderWorker (闂婃娊顣剁憴锝囩垳缁捐法鈻?

    閳规壕鏀㈤埞鈧?Display (濞撳弶鐓嬮崳?

    閳规壕鏀㈤埞鈧?AudioPlayer (闂婃娊顣堕幘顓熸杹)

    閳规柡鏀㈤埞鈧?Clock (閺冨爼鎸撻崥灞绢劄)

```



---



## 闂傤噣顣?11: 楠炶泛褰傜拠璇插絿 AVFormatContext 鐎佃壈鍤у畷鈺傜皾



**閺冦儲婀?*: 2026-02-27

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

閹绢厽鏂佺憴鍡涱暥閺冭泛鍤悳鏉裤亣闁?FFmpeg 鐟欙絿鐖滈柨娆掝嚖閸滃矁顔栭梻顔煎暱缁愪礁绌垮┃鍐跨窗

```

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

[aac @ ...] Number of scalefactor bands in group (53) exceeds limit (49).

0xC0000005: 閸愭瑥鍙嗘担宥囩枂 0x0000022947799000 閺冭泛褰傞悽鐔活問闂傤喖鍟跨粣?
```

鐠嬪啰鏁ら崼鍡樼垽閺勫墽銇氶柨娆掝嚖閸?`av_read_frame(format_ctx_, packet)` 婢跺嫨鈧?


### 閺冦儱绻旀潏鎾冲毉

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Video decode thread started

Audio decode thread started

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

... (婢堆囧櫤缁鎶€闁挎瑨顕?

```



### 閸掑棙鐎界拋鏉跨秿

1. 闂傤噣顣介崙铏瑰箛閸︺劏顫嬫０鎴Ｐ掗惍浣哄殠缁嬪鎷伴棅鎶筋暥鐟欙絿鐖滅痪璺ㄢ柤閸氬本妞傛潻鎰攽閺?
2. 娑撱倓閲滅痪璺ㄢ柤閸氬嫯鍤滈幏銉︽箒閻欘剛鐝涢惃?VideoDecoder/AudioDecoder 鐎圭偘绶?
3. 娑撱倓閲滅€圭偘绶ラ崗鍙橀煩閸氬奔绔存稉?`AVFormatContext* format_ctx_`

4. 閸?`decodeFrame()` 娑擃參鍏樼拫鍐暏娴?`av_read_frame(format_ctx_, packet)`

5. 楠炶泛褰傜拠璇插絿鐎佃壈鍤?packet 閺佺増宓侀柨娆庤础閿涘瓛264 鐟欙絿鐖滈崳銊嚢閸欐牕鍩岄柨娆掝嚖閻?NAL 閸楁洖鍘?


### 閺嶈婀伴崢鐔锋礈

**楠炶泛褰傜拫鍐暏 `av_read_frame()` 鐎佃壈鍤ч弫鐗堝祦缁旂偘绨?*閵嗕繖AVFormatContext` 娑撳秵妲哥痪璺ㄢ柤鐎瑰鍙忛惃鍕剁礉娑撱倓閲滅痪璺ㄢ柤閸氬本妞傜拠璇插絿娴兼艾顕遍懛杈剧窗

- 鐠囪褰囨担宥囩枂闁挎瑤璐?
- Packet 閺佺増宓佺悮顐ヮ洬閻?
- 鐟欙絿鐖滈崳銊︽暪閸掔増宕崸蹇曟畱閺佺増宓?


### 鐟欙絽鍠呴弬瑙勵攳

瀵洖鍙嗙紒鐔剁閻?`PacketReaderThread` 娴ｆ粈璐熼崬顖欑閻?packet 鐠囪褰囬崗銉ュ經閿?
1. `PacketReaderThread` 閺勵垰鏁稉鈧拫鍐暏 `av_read_frame()` 閻ㄥ嫬婀撮弬?
2. 鐠囪褰囬崚鎵畱 packet 閺嶈宓?stream_index 閸掑棗褰傞崚?`PacketQueue`

3. `VideoDecodeThread` 閸?`AudioDecodeThread` 娴犲骸鎮囬懛顏嗘畱 `PacketQueue` 閼惧嘲褰?packet

4. 鐟欙絿鐖滈崳銊︽煀婢?`decodePacket()` 閺傝纭堕幒銉︽暪婢舵牠鍎存导鐘插弳閻?packet



---



## 闂傤噣顣?9: VideoFrame/AudioFrame 缁夎濮╃拠顓濈疅缂傛椽娅＄€佃壈鍤у畷鈺傜皾



**閺冦儲婀?*: 2026-02-25

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

缁嬪绨崥顖氬З閹绢厽鏂侀崥搴ｇ彌閸楀啿绌垮┃鍐跨礉闁挎瑨顕ゆ穱鈩冧紖閿?
```

modern-video-player.exe - 鎼存梻鏁ょ粙瀣碍闁挎瑨顕?
0x00007FFF7A80DA4C 閹稿洣鎶ゅ鏇犳暏娴?0xFFFFFFFFFFFFFFFF 閸愬懎鐡ㄩ妴鍌濐嚉閸愬懎鐡ㄦ稉宥堝厴娑?read

```



### 閺冦儱绻旀潏鎾冲毉

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Playing video...

Video decode thread started

Audio decode thread started

(缁嬪绨畷鈺傜皾)

```



### 閸掑棙鐎界拋鏉跨秿

1. 闁挎瑨顕ら崷鏉挎絻 0xFFFFFFFFFFFFFFFF = -1閿涘矁銆冪粈铏光敄閹稿洭鎷?閺冪姵鏅ラ幐鍥嫛

2. 瀹曗晜绨濋崣鎴犳晸閸︺劏顫嬫０鎴Ｐ掗惍浣哄殠缁嬪鎯庨崝銊ユ倵娑撳秳绠?
3. 娴狅絿鐖滅€光剝鐓￠崣鎴犲箛 `VideoFrame` 閸?`AudioFrame` 缁崵宸辩亸鎴烆劀绾喚娈戠粔璇插З鐠囶厺绠熺€圭偟骞?


### 閺嶈婀伴崢鐔锋礈

`FrameQueue::pop()` 娴ｈ法鏁?`std::move` 鐏忓棗鎶氱粔璇插З閸戞椽妲﹂崚妤嬬窗

```cpp

frame = std::move(queue_.front());

queue_.pop();

```



`VideoFrame` 缁粯鐥呴張澶婄暰娑斿些閸斻劍鐎柅鐘插毐閺佹澘鎷扮粔璇插З鐠у鈧壈绻嶇粻妤冾儊閿?
- 姒涙顓婚惃鍕╅崝銊︽惙娴ｆ粌褰ч弰顖涚ガ閹风柉绀?`frame_` 閹稿洭鎷?
- 閸樼喎顕挒鈩冪€介弸鍕鐠嬪啰鏁?`av_frame_free(&frame_)` 闁插﹥鏂侀崘鍛摠

- 閻╊喗鐖ｇ€电钖勯惃?`frame_` 閸欐ɑ鍨氶幃顒傗敄閹稿洭鎷?
- 濞撳弶鐓嬪顏嗗箚鐠佸潡妫堕幃顒傗敄閹稿洭鎷＄€佃壈鍤у畷鈺傜皾



### 鐟欙絽鍠呴弬瑙勵攳

娑?`VideoFrame` 閸?`AudioFrame` 缁鐤勯悳鐗堫劀绾喚娈戠粔璇插З鐠囶厺绠熼敍?
1. 濞ｈ濮炵粔璇插З閺嬪嫰鈧姴鍤遍弫甯礉鐏?`frame_` 閹稿洭鎷℃潪顒傂?
2. 濞ｈ濮炵粔璇插З鐠у鈧壈绻嶇粻妤冾儊閿涘苯鐨?`frame_` 閹稿洭鎷℃潪顒傂?
3. 鐏忓棗甯€电钖勯惃?`frame_` 鐠佸彞璐?nullptr閿涘矂妲诲銏＄€介弸鍕闁插﹥鏂侀崘鍛摠



---



## 闂傤噣顣?13: Core API + Scheduler + Filter 婢舵氨鍤庣粙瀣櫢閺嬪嫯鎯ら崷?


**閺冦儲婀?*: 2026-03-06

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻滅増婀侀幘顓熸杹閸ｃ劋瀵屽ù浣衡柤娴犲秳浜掗弮褎鐏﹂弸鍕礋娑擃厼绺鹃敍宀€宸辩亸鎴ｎ潐閺嶈壈顩﹀Ч鍌滄畱 `Core API / Scheduler / Filter` 濡€虫健閸栨牕鍨庣仦鍌樷偓?
- 闂婂疇顫嬫０鎴Ｐ掗惍浣哄殠缁嬪娓剁憰浣告躬閺傜増鐗宠箛鍐ц厬閻欘剛鐝涚拫鍐ㄥ閿涘苯鑻熼柅姘崇箖闂冪喎鍨€圭偟骞囬弮鐘绘▎婵夌偠袙閼帮负鈧?


### 閸掑棙鐎界拋鏉跨秿

- 閺傛澘顤?`core` 鐏炲偊绱癭frame/frame_queue/clock/command/scheduler/player_core`閵?
- 閺傛澘顤?`filters` 鐏炲偊绱癭video_filter/audio_filter/filter_registry/filter_pipeline/builtin filters`閵?
- `VideoPlayer` 婢х偛濮?`USE_NEW_PLAYER_CORE` 鏉╀胶些瀵偓閸忓疇鐭惧鍕剁礉娣囨繃瀵旈弮褎甯撮崣锝勭瑝閸欐ǜ鈧?


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



## 闂傤噣顣?14: 閹绢厽鏂侀崳銊︾仸閺嬪嫭鏁归弫娑楄礋 Core 閸楁洝鐭惧?


**閺冦儲婀?*: 2026-03-06

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 妞ゅ湱娲伴崥灞炬娣囨繄鏆€閺冄勬尡閺€楣冩懠鐠侯垰鎷伴弬鐗堢壋韫囧啴鎽肩捄顖ょ礉鐎涙ê婀紒瀛樺Б閸掑棗寮舵稉搴¤嫙閸欐垼顔曠拋锟狀棑闂勨斂鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- `VideoPlayer` 娴犲懍绻氶悾?`PlayerCore` 閸栧懓顥婄€圭偟骞囬敍灞藉灩闂勩倖妫柧鎹愮熅閸掑棙鏁妴?
- CMake 閸掔娀娅庨弮褎膩閸ф绱拠鎴濆弳閸欙綇绱濈紒鐔剁閸?`core/*` + `filters/*`閵?
- 濞撳懐鎮婇弮褍銇斿┃鎰瀮娴犺绱濇穱婵堟殌韫囧懓顩﹂崺铏诡攨鐠佺偓鏌﹂崪宀冪翻閸戠儤膩閸фぜ鈧?
- 閺傛澘顤冮弸鑸电€柌宥嗙€弬鍥ㄣ€傞敍姝歞ocs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`閵?


### 娣囶喗鏁奸弬鍥︽

- CMakeLists.txt

- include/video_player.h

- src/video_player.cpp

- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md

- 閸掔娀娅庨弮褎膩閸ф鏋冩禒璁圭礄decoder/thread/sync/packet/legacy clock/frame_queue閿?


---



## 闂傤噣顣?15: 鐏忓繐鐫嗙粣妤€褰涙潻鍥с亣娑撴梹瀚嬮幏鐣岀級閺€鍙ョ瑝缁嬪啿鐣?


**閺冦儲婀?*: 2026-03-06

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閹绢厽鏂侀崳銊ユ躬鐏忓繐鐫嗙拋鎯ь槵娑撳﹥瀵滅憴鍡涱暥閸樼喎顫愰崚鍡氶哺閻滃浄绱欐俊?1920x1080閿涘娲块幒銉ョ磻缁愭绱濈粣妤€褰涢崚婵嗩潗閺勫墽銇氭潻鍥с亣閵?
- 閻劍鍩涢幏鏍ㄥ缁愭褰涢崥搴礉闁劌鍨庨悳顖氼暔娑撳瑕嗛弻鎾冲隘閸╃喐婀粙鍐茬暰鐠虹喖娈㈤敍宀冦€冮悳棰佽礋閳ユ粎鐛ラ崣锝勭瑝閼宠姤顒滅敮姝岀殶閺佺补鈧縿鈧?


### 閺冦儱绻旀潏鎾冲毉

```

Display initialized: window 1306x734 (source 1920x1080)

```



### 閸掑棙鐎界拋鏉跨秿

- `PlayerCore::open()` 閻╁瓨甯撮幎濠咁潒妫版垵鍨庢潏銊у芳娴肩姷绮?`Display::init()`閿涘本鐥呴張澶嬬壌閹诡喖鐫嗛獮鏇炲讲閻劌灏崑姘额浕鐢呯崶閸欙絿缂夐弨淇扁偓?
- 娴滃娆㈡径鍕倞閸欘亞娲冮崥?`SDL_WINDOWEVENT_RESIZED`閿涘本婀憰鍡欐磰鐢瓕顫嗛惃?`SDL_WINDOWEVENT_SIZE_CHANGED`閵?
- 濞撳弶鐓嬮惄顔界垼閻晛鑸伴惄瀛樺复闁剧儤寮х粣妤€褰涢敍宀€宸辩亸鎴炲瘻濠ф劘顫嬫０鎴炵槷娓氬顓哥粻妤冩畱閻╊喗鐖ｉ崠鍝勭厵閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`Display::init()` 婢х偛濮為崺杞扮艾 `SDL_GetDisplayUsableBounds()` 閻ㄥ嫬鍨垫慨瀣崶閸欙絽鏄傜€垫瓕顓哥粻妤嬬礉娣囨繃瀵旈崢鐔奉潗鐎逛粙鐝В鏂胯嫙闂勬劕鍩楅崚鏉跨潌楠炴洖褰查悽銊ュ隘 90%閵?
- 閸氬本妞傛径鍕倞 `SDL_WINDOWEVENT_RESIZED` 閸?`SDL_WINDOWEVENT_SIZE_CHANGED`閿涘瞼鈥樻穱婵囧珛閹疯姤妞傜粣妤€褰涚亸鍝勵嚟閻樿埖鈧礁鐤勯弮鑸垫纯閺傝埇鈧?
- 濞撳弶鐓嬮弮鑸靛瘻鐟欏棝顣剁€逛粙鐝В鏃囶吀缁?`dst_rect`閿涘矂浼╅崗宥囩崶閸欙絽褰夐崠鏍ㄦ閹峰鍑犳径杈╂埂閵?


### 娣囶喗鏁奸弬鍥︽

- src/display.cpp



## 闂傤噣顣?16: 缁愭褰涢張鈧径褍瀵?缂傗晜鏂侀弮鎯邦潒妫版垹鏁鹃棃銏犲幢娴ｅ骏绱濈紓鍝勭毌閸╄櫣顢呴幒褍鍩楅弶?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 缁愭褰涢張鈧径褍瀵查幋鏍ㄥ珛閸斻劎缂夐弨鐐閿涘矁顫嬫０鎴犳暰闂堛垹顔愰弰鎾冲幢娴ｅ骏绱濋棅鎶筋暥娴犲秶鎴风紒顓熸尡閺€淇扁偓?
- 閹绢厽鏂侀崳銊у繁鐏忔垼绻樻惔锔芥蒋閵嗕線鐓堕柌蹇氱殶閼哄倸鎷伴幏鏍уЗ鏉╂稑瀹抽惃鍕唨绾偓閼宠棄濮忛妴?


### 閺冦儱绻旀潏鎾冲毉

```text

閻滄媽钖勬径宥囧箛閿涙氨鐛ラ崣锝嗘付婢堆冨/缂傗晜鏂侀弮鎯邦潒妫版垹鏁鹃棃銏犱粻濮濄垹鍩涢弬甯礉闂婃娊顣剁紒褏鐢婚妴?
```



### 閸掑棙鐎界拋鏉跨秿

- 鏉╂劘顢戦張?SDL 娴滃娆㈡径鍕倞娑撳孩瑕嗛弻鎾冲瀻鐢啫婀稉宥呮倱缁捐法鈻奸敍宀€鐛ラ崣锝呭綁閸栨牔绨ㄦ禒鏈电瑢濞撳弶鐓嬬拫鍐暏楠炶泛褰傞弮璺侯啇閺勬挸鍤悳鐗堣閺屾挸浠犲鐐偓?
- 閹绢厽鏂侀崳?UI 娴犲懏鏁幐渚€鏁惄妯绘畯閸?闁偓閸?閸忋劌鐫嗛敍宀€宸辩亸鎴濈唨绾偓娴溿倓绨伴幒褌娆㈤妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 鐏忓棛鐛ラ崣锝勭皑娴犺泛顦╅悶鍡樻暪閺佹稑鍩屽〒鍙夌厠鐠侯垰绶為敍鍧凞isplay::renderFrame`/`PlayerCore::onRenderIdle`閿涘鏅堕敍宀勬娴ｅ海缂夐弨鍙ョ瑢閺堚偓婢堆冨閺冨墎娈戝〒鍙夌厠闂冭顢ｆ搴ㄦ珦閵?
- `Display` 閺傛澘顤冮幒褍鍩楃仦鍌滅帛閸掕绱版潻娑樺閺?+ 闂婃娊鍣洪弶掳鈧?
- 閺傛澘顤冩Η鐘崇垼娴溿倓绨伴敍姘珛閸斻劏绻樻惔锔芥蒋閸欐垼鎹?seek閵嗕焦瀚嬮崝銊╃叾闁插繑娼€圭偞妞傜拫鍐Ν闂婃娊鍣洪妴?
- `PlayerCore::pumpEvents` 閺傛澘顤冪€?seek/volume 鐠囬攱鐪伴惃鍕Х鐠愰€涚瑢閹笛嗩攽閵?


### 娣囶喗鏁奸弬鍥︽

- include/display.h

- src/display.cpp

- src/core/player_core.cpp

- src/main.cpp



## 闂傤噣顣?17: 娴间椒绗熺痪?MPC-HC 閺嬭埖鐎崜鈺€缍戝Ο鈥虫健缂傚搫鐨崣顖濇儰閸﹂鍞惍渚€顎囬弸?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸愮绱欓梼鑸殿唽閹嶇礆



### 闂傤噣顣介幓蹇氬牚

- `enterprise-quill-logging/tasklist.md` 娑擃厽膩閸?02-14 娴犲秵婀佹径褔鍣洪張顏勭暚閹存劙銆嶉敍宀€宸辩亸鎴犵埠娑撯偓閹恒儱褰涙稉搴濆敩閻浇鎯ら崷鎵仯閵?
- 閻滅増婀侀幘顓熸杹閸ｃ劋瀵岄柧鎹愮熅閺堫亝濞婄挒鈩冭閺屾挸鎮楃粩顖ょ礉闂呭彞浜掗幍鈺佺潔 D3D11/OpenGL 缁涘绱掓稉姘遍獓濞撳弶鐓嬪Ο鈥虫健閵?


### 閸掑棙鐎界拋鏉跨秿

- 瑜版挸澧犲銉р柤瀹告彃鍙挎径?Core + Scheduler + Filter 閸╄櫣顢呴敍灞肩稻缂傚搫鐨捄銊δ侀崸妤勭珶閻ｅ苯鐣炬稊澶堚偓?
- 闂団偓鐟曚礁鍘涚悰銉╃彯娴兼ê鍘涘Ο鈥虫健妤犮劍鐏﹂獮鑸靛复閸忋儲鐎鐚寸礉閸愬秹鈧劖顒炴繅顐㈠帠鐎瑰本鏆ｉ懗钘夊閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤冮崺铏诡攨鐠佺偓鏌﹂敍姝歍askQueue`閵嗕梗FramePool`閵嗕梗DecoderThread`閵?
- 閺傛澘顤冨〒鍙夌厠濡€虫健閿涙瓪IVideoRenderer`閵嗕讣DL 闁倿鍘ら崳銊ｂ偓涓?D11/OpenGL 閸楃姳缍呯€圭偟骞囬妴涔endererFactory`閿涘苯鑻熼幒銉ュ弳 `PlayerCore`閵?
- 閺傛澘顤冮棅鎶筋暥婢х偛宸遍敍?0 濞堥潧娼庣悰鈥虫珤閵嗕礁顦垮ù浣硅穿闂婂啿娅掗敍灞惧⒖鐏?`AudioPlayer` 缂傛挸鍟跨憴鍌涚ゴ閹恒儱褰涢妴?
- 閺傛澘顤冪憴锝囩垳閸ｃ劎顓搁悶鍡窗`IDecoder`閵嗕梗DecoderCapability`閵嗕梗DecoderFactory` 閼奉亜濮╅柅澶嬪闁槒绶妴?
- 閺傛澘顤冪€涙绠?閹绢厽鏂侀崚妤勩€?鐠佸墽鐤?韫囶偅宓庨柨?閻喛鍋?閹绘帊娆?閺嶇厧绱￠弨顖涘瘮/濞翠礁鐛熸担鎾剁搼娴间椒绗熺痪褎膩閸ф顎囬弸韬测偓?
- 閺傛澘顤冨銈夋殔閸╄櫣琚稉搴ㄧ叾鐟欏棝顣跺銈夋殔闁炬拝绱濋獮鎯八夐崗鍛寸叾闁插繐閽╃悰鈩冩姢闂€婧库偓?
- 閸氬本顒為弴瀛樻煀 `tasklist.md` 閸曢箖鈧濮搁幀渚婄礄娴犲懎瀣€闁鍑￠拃钘夋勾娴狅絿鐖滄い鐧哥礆閵?


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



---



## 闂傤噣顣?18: 缂傛牞鐦ч崺铏瑰殠閹垹顦叉稉搴㈢壐瀵繗鍏橀崝娑氱叐闂冮潧鍙嗛崣?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `dash_manifest_parser.cpp` 閸?VS2022/MSVC 缂傛牞鐦ф径杈Е閿涘矂妯嗘繅鐐存拱閸︾増瀵旂紒顓炵磻閸欐垯鈧?
- 缂傚搫鐨崣顖滄纯閹恒儴绻嶇悰宀€娈戦懗钘夊濡偓閺屻儱鍙嗛崣锝忕礉娑撳秴鍩勬禍搴℃彥闁喖鐛欑拠浣测偓婊€瀵岄崝娑欑壐瀵?+ 妤傛ê鍨庢妯烘姎 + 婢舵岸鐓堕柆鎾偓婵堟窗閺嶅洢鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 娣囶喖顦?DASH 濮濓絽鍨?raw-string 閸掑棝娈х粭锔肩礉閹垹顦插銉р柤閸欘垳绱拠鎴欌偓?
- 閺傛澘顤冮崨鎴掓姢鐞涘矁鍏橀崝娑樺弳閸欙綇绱?
  - `--capabilities`閿涙俺绶崙楦跨箥鐞涘本妞傜€圭懓娅?缂傛牞袙閻礁娅掗懗钘夊娑撳簼瀵岄崝娑欑壐瀵繗顩惄鏍叐闂冪偣鈧?
  - `--evaluate-target`閿涙俺鐦庢导鐗堝瘹鐎规艾鍨庢潏銊у芳/鐢呭芳/婢逛即浜?閻胶宸奸惄顔界垼閺勵垰鎯佸楦款唴绾剝袙娑?D3D11 濞撳弶鐓嬮妴?
- 婢х偛宸遍幘顓熸杹闁炬崘鐭鹃崺铏诡攨閼宠棄濮忛敍?
  - `Demuxer` 娴ｈ法鏁?`av_find_best_stream` + probe 閸欏倹鏆熸晶鐐插繁閿?
  - `AudioPlayer` 閺嗘挳婀剁€圭偤妾潏鎾冲毉閸欏倹鏆熼敍?
  - `PlayerCore` 婢跺秶鏁?`SwrContext`閿涘本瀵滅拋鎯ь槵鏉堟挸鍤崣鍌涙殶鏉╂稖顢戦柌宥夊櫚閺嶆灚鈧?


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



## 闂傤噣顣?19: D3D11VA 绾剝袙閺堚偓鐏忓繘妫撮悳顖ょ礄婢惰精瑙﹂崶鐐衡偓鈧潪顖澬掗敍?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 闂団偓鐟曚礁婀?Windows 娑撳绱崗鍫滃▏閻?D3D11VA 鐟欙絿鐖滄妯跨鏉炲€燁潒妫版埊绱濋崥灞炬闁灝鍘ょ涵顒冃掓径杈Е鐎佃壈鍤ч弮鐘崇《閹绢厽鏂侀妴?
- 绾兛娆㈡潏鎾冲毉鐢勭壐瀵繋绗?SDL 濞撳弶鐓嬮柧鎹愮熅娑撳秳绔撮懛杈剧礄GPU/NV12 vs YUV420P閿涘绱濋棁鈧憰浣虹埠娑撯偓鏉堟挸鍤弽鐓庣础閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`PlayerCore::initDecoders` 娑擃厼濮為崗?D3D11VA 闁板秶鐤嗘稉搴ㄢ偓澶嬪闁槒绶妴?
- 瑜版挾鈥栫憴?`avcodec_open2` 婢惰精瑙﹂弮璁圭礉閼奉亜濮╅柌宥呯紦鐟欙絿鐖滈崳銊ヨ嫙閸ョ偤鈧偓鏉烆垵袙缂佈呯敾閹绢厽鏂侀妴?
- 閸︺劏顫嬫０鎴Ｐ掗惍浣界熅瀵板嫯藟閸忓懐鈥栨禒璺烘姎鏉烆剙鐡ㄦ稉搴″剼缁辩姵鐗稿蹇氼潐閺佽揪绱?
  - `av_hwframe_transfer_data`閿涘牏鈥栨禒璺烘姎 -> 缁崵绮洪崘鍛摠鐢嶇礆

  - `sws_scale`閿涘牓娼?YUV420P -> YUV420P閿?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp



---



## 闂傤噣顣?20: `--probe-file` 娑撳孩鐗稿蹇撴礀瑜版帟鍓奸張?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 缂傚搫鐨崡鏇熸瀮娴犺泛褰查懘姘拱閸栨牗甯板ù瀣弳閸欙綇绱濇稉宥勭┒娴滃骸鎻╅柅鐔风暰娴ｅ秮鈧粍鐓囨稉顏呯壉閺堫兛璐熸禒鈧稊鍫滅瑝鏉堢偓鐖ｉ垾婵勨偓?
- 缂傚搫鐨幍褰掑櫤閸ョ偛缍婇懘姘拱閿涘奔绗夐崚鈺€绨崡鏇氭眽瀵偓閸欐垼鍑禒锝勮厬閹镐胶鐢绘潻鍊熼嚋閺嶇厧绱￠崗鐓庮啇閹団偓鈧崠鏍モ偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤冮崨鎴掓姢閿涙瓪modern-video-player.exe --probe-file <media_file>`閿涘矁绶崙?`probe.*` 闁款喖鈧偐绮ㄩ弸婊愮礉娓氬じ绨懘姘拱鐟欙絾鐎介妴?
- 閺傛澘顤?`tools/format_regression/run_format_regression.ps1`閿涙碍瀵?CSV 閺嶉攱婀板〒鍛礋閹靛綊鍣洪幒銏＄ゴ楠炴儼绶崙?Markdown 閹躲儱鎲￠妴?
- 閹躲儱鎲℃妯款吇鐠侯垰绶為敍姝歞ocs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`閵?
- 鏉╂柨娲栭惍浣哄鐎规熬绱癭0=PASS`閿涘畭1=PARTIAL`閿涘畭2=FAIL`閵?


### 妤犲矁鐦夌拋鏉跨秿

- `.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4` 鏉╂柨娲?`probe.overall=PASS`閵?
- `.\tools\format_regression\run_format_regression.ps1` 閹存劕濮涢悽鐔稿灇閹躲儱鎲￠妴?
- `-OutputFile` 閼奉亜鐣炬稊澶庣熅瀵板嫰鐛欑拠渚€鈧俺绻冮妴?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- tools/format_regression/run_format_regression.ps1

- tools/format_regression/format_samples.csv

- docs/workflows/FORMAT_REGRESSION.md

- docs/README.md



---



## 闂傤噣顣?21: GitHub Actions 閼奉亜濮╅崶鐐茬秺閹恒儱鍙?


**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犻弽鐓庣础閸ョ偛缍婃禒鍛躬閺堫剙婀撮幍瀣З閹笛嗩攽閿涘R 缂傚搫鐨懛顏勫З閸栨牠妫粋浣碘偓?
- Windows CI 娑擃厺绶风挧鏍у絺閻滅増鏌熷蹇庣瑢閺堫剙婀?`external/` 閻╊喖缍嶉弬瑙勵攳娑撳秴鎮撻敍灞界摠閸︺劍鐎杞扮瑝娑撯偓閼锋挳顥撻梽鈹库偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`.github/workflows/format-regression.yml`閿?
  - 閼奉亜濮╂稉瀣祰 `SDL2/FFmpeg` 妫板嫮绱拠鎴滅贩鐠ф牕鑻熼弸鍕紦 `build/Debug/modern-video-player.exe`閿?
  - 鐠嬪啰鏁?`tools/download_test_samples.ps1` 閻㈢喐鍨氶弽閿嬫拱閿?
  - 鐠嬪啰鏁?`tools/run_all_checks.ps1` 閹笛嗩攽閸楁洘鏋冩禒鑸靛赴濞?+ 閹靛綊鍣洪崶鐐茬秺閿?
  - 娑撳﹣绱?`docs/reports/FORMAT_REGRESSION_CI.md` 娴溠呭⒖閵?
- 鐠嬪啯鏆?`CMakeLists.txt`閿涘湹indows閿涘绱?
  - 娴兼ê鍘涢弨顖涘瘮 `SDL2::`閵嗕梗FFMPEG::`閵嗕梗unofficial::ffmpeg::` 鐎电厧鍙嗛惄顔界垼闁剧偓甯撮敍?
  - 缂佈呯敾娣囨繄鏆€ `external/SDL2` 娑?`external/ffmpeg` 閸ョ偤鈧偓鐠侯垰绶為妴?
- 鐠嬪啯鏆?`tools/download_test_samples.ps1`閿?
  - 閺傛澘顤?PATH 閸欘垱澧界悰宀冃掗弸鎰扳偓鏄忕帆閿涘本鏁幐?`-FfmpegPath ffmpeg`閵?
- 閸氬本顒為弴瀛樻煀閸ョ偛缍婇弬鍥ㄣ€傛稉搴濇崲閸斺剝绔婚崡鏇炲嚒鐎瑰本鍨氭い骞库偓?


### 娣囶喗鏁奸弬鍥︽

- .github/workflows/format-regression.yml

- CMakeLists.txt

- tools/download_test_samples.ps1

- docs/workflows/FORMAT_REGRESSION.md

- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---



## 闂傤噣顣?22: M1 娑撳鎽肩捄顖涘腹鏉╂冻绱欓幘顓熸杹閸掓銆?+ 鐠佸墽鐤?+ 韫囶偅宓庨柨顕€顩婚悧鍫礆



**閺冦儲婀?*: 2026-03-07

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犳稉缁樼ウ缁嬪褰ч張澶婂礋閺傚洣娆㈤幘顓熸杹閿涘畭PlaylistManager`/`SettingsManager` 閾忚姤婀佸Ο鈥虫健娴ｅ棙婀幒銉ュ弳閵?
- 閸忔娊鏁箛顐ｅ祹闁款喗婀憰鍡欐磰娴犺濮熷〒鍛礋閻╊喗鐖ｉ敍宀€鏁ら幋铚傛唉娴滄帗鏅ラ悳鍥︾瑝鐡掔偨鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`main` 娑撶粯绁︾粙瀣复閸忋儲鎸遍弨鎯у灙鐞涱煉绱?
  - 閺€顖涘瘮婢舵碍鏋冩禒璺哄棘閺佺増鐎鐑樻尡閺€鎯у灙鐞涱煉绱?
  - 閺€顖涘瘮 `.m3u8` 閸旂姾娴囬敍?
  - 閺€顖涘瘮娑撳﹣绔存＃?娑撳绔存＃鏍︾瑢 EOF 閼奉亜濮╂稉瀣╃妫ｆ牓鈧?
- 閸?`main` 閹恒儱鍙嗙拋鍓х枂閹镐椒绠欓崠鏍电窗

  - 閸氼垰濮╅崝鐘烘祰 `config/player_settings.ini`閿?
  - 鐠佸墽鐤嗘径杈Е閸ョ偤鈧偓姒涙顓婚敍?
  - 闁偓閸戣桨绻氱€涙﹢鐓堕柌蹇嬧偓渚€鈧喎瀹抽妴浣哄偍瀵洏鈧?
- 閹碘晛鐫嶅〒鍙夌厠娴滃娆㈢拠閿嬬湴闁炬崘鐭鹃敍鍦杋splay -> Renderer -> PlayerCore -> VideoPlayer -> main閿涘绱?
  - seek 婢х偤鍣虹拠閿嬬湴閵嗕線鈧喎瀹崇拫鍐╂殻鐠囬攱鐪伴妴浣稿灙鐞涖劌鍨忛幑銏ｎ嚞濮瑰倶鈧線鈧偓閸戦缚顕Ч鍌︾幢

  - 鐎圭偟骞囨禒璇插濞撳懎宕熸＃鏍姒涙顓婚柨顔荤秴闂嗗棴绱檂Space/Enter/Esc/Q/Left/Right/Ctrl+Left/Ctrl+Right/Up/Down/M/PageUp/PageDown/[ ]/R`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 闂団偓濮瑰倽顩﹀Ч鍌溞╅梽銈勮⒈娑?Core 濞村鐦崣濠勬祲閸忚櫕鏋冩禒韬测偓?
- `CMakeLists.txt` 娑擃厺绮涚€涙ê婀ù瀣槸閻╊喗鐖ｆ稉搴＄磻閸忕绱濋惄瀛樺复閸掔娀娅庨弬鍥︽娴兼氨鏆€娑撳鐎鐑樼暙閻ｆ瑣鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 瀹歌尙鈥樼拋銈呯窡缁夊娅庡ù瀣槸閺傚洣娆㈡稉?`tests/core_frame_queue_tests.cpp` 娑?`tests/core_clock_tests.cpp`閵?
2. 瀹歌尙鈥樼拋銈嗙€楦垮壖閺堫兛鑵戠€涙ê婀?`BUILD_CORE_TESTS`閵嗕梗core_frame_queue_tests`閵嗕梗core_clock_tests` 娑?`core_tests` 閼辨艾鎮庨惄顔界垼瀵洜鏁ら妴?
3. 闂団偓鐟曚礁鎮撳銉︾閻?CMake 闁板秶鐤嗘稉搴㈡瀮濡楋綀顔囪ぐ鏇礉娣囨繆鐦夋禒鎾崇氨閻樿埖鈧椒绔撮懛娣偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 濞撳懐鎮?`CMakeLists.txt` 娑擃厾娈戞稉銈勯嚋濞村鐦惄顔界垼閵嗕浇浠涢崥鍫㈡窗閺嶅洣绗屽ù瀣槸瀵偓閸忕偨鈧?
- 閸掔娀娅庢稉銈勯嚋濞村鐦弬鍥︽閵?
- 閸氬本顒為弴瀛樻煀 `CHANGELOG.md`閵嗕梗VERSION.md`閵嗕梗DEVELOP_LOG.md`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.1.1` 闂団偓鐟曚焦褰佹笟娑橆樆閹稿倸鐡ч獮鏇炲鏉炶棄鍙嗛崣锝冣偓?
- 閻滅増婀佹禒锝囩垳閾忕晫鍔ч張?`subtitle::SrtParser`閿涘奔绲炬稉缁樼ウ缁嬪鐥呴張澶夋崲娴ｆ洖寮弫鏉垮弳閸欙絾鍨ㄩ幘顓熸杹閸ｃ劌濮炴潪鑺ュ复閸欙絻鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 鐎涙绠风憴锝嗙€藉Ο鈥虫健瀹告彃鍙挎径鍥风礄`include/subtitle/srt_parser.h` + `src/subtitle/srt_parser.cpp`閿涘鈧?
2. `main` 閸欏倹鏆熺憴锝嗙€芥禒鍛暜閹镐礁鐛熸担鎾圭翻閸忋儯鈧浇鍏橀崝娑欑叀鐠囥垹鎷?probe閿涘瞼宸辨径鍗炵摟楠炴洖鍙嗛崣锝冣偓?
3. `VideoPlayer` 娑撳秵瀵旈張澶婎樆閹稿倸鐡ч獮鏇犲Ц閹緤绱濋弮鐘崇《閸︺劍鎸遍弨鍙ョ窗鐠囨繀鑵戞潻鍊熼嚋瀹告彃濮炴潪钘夌摟楠炴洏鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- `VideoPlayer` 閺傛澘顤冩径鏍ㄥ瘯鐎涙绠烽崝鐘烘祰閼宠棄濮忛敍鍫滅矌 SRT閿涘苯鎯堢€瑰綊鏁婇敍澶涚窗

  - `loadExternalSubtitle()` / `clearExternalSubtitle()`閿?
  - 鐠佹澘缍嶅鎻掑鏉炶棄鐡ч獮鏇＄熅瀵板嫬鎷伴弶锛勬窗閺佽埇鈧?
- `main` 閺傛澘顤?`--subtitle <file.srt>` 閸忋儱褰涢敍灞借嫙娣囨繄鏆€閹绢厽鏂侀崚妤勩€冮崣鍌涙殶閼宠棄濮忛妴?
- 閼汇儲婀弰鎯х础娴?`--subtitle`閿涘矁鍤滈崝銊ョ毦鐠囨洖濮炴潪钘夋倱閸?`.srt` 閺傚洣娆㈤妴?
- 娴犺濮熷〒鍛礋 `1.1.1` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.1.2` 鐟曚焦鐪扮€涙绠峰〒鍙夌厠閸欑姴濮為敍灞肩稻瑜版挸澧犲〒鍙夌厠閹恒儱褰涢崣顏呮箒閻㈠娼版稉搴㈠付閸掕泛鐪伴敍灞剧梾閺堝鐡ч獮鏇⑩偓姘朵壕閵?
- 娴犺濮熷〒鍛礋 `1.1.3` 鐟曚焦鐪扮€涙绠锋稉搴㈡尡閺€?閺嗗倸浠?seek 閸氬本顒為敍灞肩稻閹绢厽鏂侀弽绋跨妇閺堫亝瀵滈弮鍫曟寭妞瑰崬濮╃€涙绠烽弴瀛樻煀閵?


### 閸掑棙鐎界拋鏉跨秿

1. `VideoPlayer` 瀹告彃鍙挎径鍥ь樆閹稿倸鐡ч獮鏇炲鏉炲€熷厴閸旀冻绱欓梻顕€顣?24閿涘绱濇担鍡楃摟楠炴洘鏆熼幑顔芥弓鏉╂稑鍙嗗〒鍙夌厠闁炬崘鐭鹃妴?
2. 濞撳弶鐓嬬仦鍌涘▕鐠烇紕宸辨径鍗炵摟楠炴洘甯撮崣锝忕礉SDL/D3D11/OpenGL 閸氬海顏弮鐘崇《缂佺喍绔撮幒銉︽暪鐎涙绠烽弬鍥ㄦ拱閵?
3. 閹绢厽鏂侀弽绋跨妇闂団偓鐟曚礁婀〒鍙夌厠鐢傜瑢缁屾椽妫芥禍瀣╂闁姤娲块弬鏉跨摟楠炴洩绱濋幍宥堝厴鐟曞棛娲婇弳鍌氫粻閹礁鎷?seek 閸氬酣娼ゅ銏㈡暰闂堫潿鈧?
4. 鐎涙绠烽弴瀛樻煀鐎圭偟骞囨稉顓炵摠閸︺劑鏀ｉ崘鍛扮殶閻劍瑕嗛弻鎾村复閸欙綁顥撻梽鈺嬬礉闂団偓鐟曚浇鐨熼弫鎾敚缁帒瀹抽妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 濞撳弶鐓嬮幎鍊熻杽閹碘晛鐫嶉敍?
  - `IVideoRenderer` 婢х偛濮?`setSubtitleText()`閿?
  - SDL 閸氬海顏潪顒€褰傞崚?`Display`閿?
  - D3D11/OpenGL 婢х偛濮為崗鐓庮啇濡椻晛鐤勯悳鑸偓?
- `Display` 閺傛澘顤冪€涙绠烽崣鐘插鐏炲偊绱?
  - 閺傛澘顤冪€涙绠烽悩鑸碘偓浣风瑢娴滄帗鏋兼穱婵囧Б閿?
  - 閸︺劍鐦＄敮褎瑕嗛弻鎾茶厬閸欑姴濮炵€涙绠烽棃銏℃緲娑撳孩鏋冮張顒婄幢

  - 閺€顖涘瘮婢舵俺顢戞稉搴ょТ闂€鎸庢瀮閺堫剙顦╅悶鍡愨偓?
  - 瑜版挸澧犵€涙膩娑撻缚浜ら柌蹇撶杽閻滃府绱濋棃?ASCII 鐎涙顑侀梽宥囬獓閺勫墽銇氶妴?
- `PlayerCore` 閺傛澘顤冪€涙绠烽弮鍫曟？鏉炴挳鈧槒绶敍?
  - 瀵洖鍙嗘径鏍ㄥ瘯鐎涙绠锋潪銊╀壕閻樿埖鈧胶顓搁悶鍡幢

  - 閸?`renderFrame()` 娑?`onRenderIdle()` 娑擃叀鐨熼悽?`updateSubtitleOverlay()`閿?
  - 娓氭繃宓佽ぐ鎾冲閹绢厽鏂侀弮鍫曟？闁瀚ㄥú鏄忕┈鐎涙绠烽敍灞炬暜閹镐焦鎸遍弨?閺嗗倸浠?seek 閸氬本顒為妴?
- 娣囶喖顦查獮璺哄絺缂佸棜濡敍?
  - 鐏忓棙瑕嗛弻鎾圭殶閻劎些閸戝搫鐡ч獮鏇氱鞍閺傘儵鏀ｉ敍宀勪缉閸忓秹鏀ｉ崘鍛礀鐠嬪啯瑕嗛弻鎾崇湴閵?
- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍?
  - `1.1.2`閵嗕梗1.1.3` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.1.4` 鐟曚焦鐪伴垾婊冪摟楠炴洖绱戦崗鍏呯瑢瀵倸鐖舵径鍕倞閳ユ繐绱濇担鍡楃秼閸撳秶宸辩亸鎴ｇ箥鐞涘本妞傜€涙绠峰鈧崗瀹犲厴閸旀稏鈧?
- 鐎涙绠烽崝鐘烘祰濞翠胶鈻奸崷銊︽瀮娴犲墎閮寸紒鐔风磽鐢婧€閺咁垯绗呯紓鍝勭毌娑撴捇妫€瑰綊鏁婇敍宀€菙鐎规碍鈧傜瑝鐡掔偨鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀佹潏鎾冲弳娴滃娆㈤柧鎹愮熅閿涘潉Display -> Renderer -> PlayerCore`閿涘褰查幍鈺佺潔鐠囬攱鐪伴崹瀣付閸掕绱濋柅鍌氭値閸旂姴鍙嗙€涙绠峰鈧崗鐐解偓?
2. 鐎涙绠烽弮鍫曟？鏉炲瓨娲块弬鏉垮嚒閸?`PlayerCore` 閸愬懘妫撮悳顖ょ礉閺傛澘顤冮垾婊冪磻閸忓磭濮搁幀浣测偓婵嗗祮閸欘垰顦查悽銊у箛閺堝鎮撳銉┾偓鏄忕帆閵?
3. `VideoPlayer::loadExternalSubtitle()` 娴ｈ法鏁ゆ妯款吇 `filesystem` 濡偓閺屻儲鏌熷蹇ョ礉鐎涙ê婀鍌氱埗娴肩姵鎸辨搴ㄦ珦閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤冪€涙绠峰鈧崗铏付閸掕绱?
  - 閸?`Display` 娑擃厽鏌婃晶鐐茬摟楠炴洖绱戦崗瀹狀嚞濮瑰倸鑻熺紒鎴濈暰 `V` 闁款噯绱?
  - 閸︺劍瑕嗛弻鎾虫珤閹恒儱褰涢弬鏉款杻 `consumeToggleSubtitleRequest()`閿?
  - 閸?`PlayerCore` 娑擃厽鏌婃晶?`setSubtitleEnabled()/toggleSubtitleEnabled()/isSubtitleEnabled()`閿?
  - 閸忔娊妫寸€涙绠烽弮鏈靛瘜閸斻劍绔荤粚鐑樿閺屾挻鏋冮張顒婄礉瀵偓閸氼垰鐡ч獮鏇炴倵閹稿缍嬮崜宥嗘闂傛挳鍣搁弬浼粹偓澶婂絿鐎涙绠烽妴?
- 閸旂姴宸卞鍌氱埗婢跺嫮鎮婇敍?
  - 鐎涙绠风捄顖氱窞濡偓閺屻儲鏁奸悽?`std::error_code`閿?
  - 閹规洝骞忕€涙绠风憴锝嗙€藉鍌氱埗楠炶泛鍟撻崨濠咁劅閺冦儱绻旈敍?
  - 閸旂姾娴囨径杈Е娣囨繃瀵旀稉缁樼ウ缁嬪褰查幘顓熸杹閿涘奔绗夋穱婵堟殌閼村繐鐡ч獮鏇犲Ц閹降鈧?
- 閸氬本顒炴禒璇插閻樿埖鈧緤绱?
  - `1.1.4` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.3.2` 闂団偓鐟曚焦鏁幐渚€鏁担宥夊帳缂冾喗瀵旀稊鍛閵?
- 瑜版挸澧犺箛顐ｅ祹闁款噣鈧槒绶惄瀛樺复閸愭瑥婀?`Display` 閻?`SDL_KEYDOWN` 閸掑棙鏁敍灞炬￥濞夋洑绮犻柊宥囩枂閸旂姾娴囬獮璺烘躬闁插秴鎯庨崥搴濈箽閹镐降鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 妞ゅ湱娲板鍙夋箒 `HotkeyManager`閿涘奔绲炬妯款吇闁款喕缍呮稉搴濆瘜濞翠胶鈻肩€圭偤妾柨顔荤秴娑撳秳绔撮懛杈剧礉娑撴梹婀幒銉ュ弳閹绢厽鏂侀柧淇扁偓?
2. `SettingsManager` 瀹歌尪鍏樼拠璇插晸 `player_settings.ini`閿涘苯褰叉径宥囨暏娑?`hotkey.*` 閹镐椒绠欓崠鏍偓姘朵壕閵?
3. 闂団偓鐟曚焦澧﹂柅?`Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 閻ㄥ嫮鍎归柨顔芥Ё鐏忓嫰鈧繋绱堕柧鎹愮熅閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閹碘晛鐫?`HotkeyManager`閿?
  - 鐎靛綊缍堟＃鏍姒涙顓婚崝銊ょ稊娑撳酣鏁担宥忕幢

  - 閺傛澘顤冮崝銊ょ稊闁款喖鎮曢崪宀勬暛閻礁绨崚妤€瀵?閸欏秴绨崚妤€瀵查懗钘夊閵?
- 娴滃娆㈤柧鎹愮熅閹恒儱鍙嗛敍?
  - `Display` 閺€閫涜礋闁俺绻?`HotkeyManager` 鐟欙絾鐎介柨顔荤秴閸斻劋缍旈敍?
  - 濞撳弶鐓嬮崳銊﹀复閸欙絾鏌婃晶?`setHotkeyManager()`閿?
  - `PlayerCore` 娑?`VideoPlayer` 婢х偛濮為悜顓㈡暛缁狅紕鎮?API 楠炲爼鈧繋绱堕崚?SDL 濞撳弶鐓嬮崳銊ｂ偓?
- 閹镐椒绠欓崠鏍ㄥ复閸忋儻绱?
  - `main` 閺傛澘顤?`hotkey.*` 闁板秶鐤嗛崝鐘烘祰娑撳骸娲栭崘娆欑幢

  - 闂堢偞纭堕柊宥囩枂閼奉亜濮╅梽宥囬獓姒涙顓婚獮鎯邦唶瑜版洖鎲＄拃锔肩幢

  - 閺囧瓨鏌?`config/player_settings.ini` 姒涙顓婚弽铚傜伐閵?
- 娴犺濮熼悩鑸碘偓浣告倱濮濄儻绱?
  - `1.3.2` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.3.3` 闂団偓鐟曚焦鏁幐浣测偓婊堟暛娴ｅ秴鍟跨粣浣诡梾濞村绗岄幁銏狀槻姒涙顓婚垾婵勨偓?
- 瑜版挸澧犲鎻掑徔婢?`hotkey.*` 閹镐椒绠欓崠鏍电礉娴ｅ棛宸辩亸鎴濆暱缁愪焦涓嶉悶鍡礉闁插秴顦查柨顔荤秴娴兼艾顕遍懛鏉戝З娴ｆ粏顕㈡稊澶夌瑝濞撳懏娅氶妴?


### 閸掑棙鐎界拋鏉跨秿

1. `HotkeyManager` 瀹稿弶婀侀崝銊ょ稊娑撳酣鏁担宥嗘Ё鐏忓嫸绱濋柅鍌氭値閹碘晛鐫嶉崘鑼崐閹殿偅寮块幒銉ュ經閵?
2. 闁板秶鐤嗛崝鐘烘祰濞翠胶鈻奸崷?`main` 娑擃參娉︽稉顓烆槱閻炲棴绱濋柅鍌氭値缂佺喍绔撮崝鐘插弳閸愯尙鐛婂Λ鈧ù瀣╃瑢閹垹顦茬粵鏍殣閵?
3. 闂団偓鐟曚椒绔存稉顏勫讲閺勬儳绱＄憴锕€褰傞幁銏狀槻姒涙顓婚惃鍕帳缂冾噣鈧岸浜鹃敍灞肩┒娴滃海鏁ら幋鐤殰閺佹垯鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`HotkeyManager` 婢х偛濮為敍?
  - `resetToDefaults()`閿?
  - `findConflicts()` / `hasConflicts()`閵?
- 閸︺劎鍎归柨顔煎鏉炶姤绁︾粙瀣杻閸旂姴鍟跨粣浣逛笉閻炲棴绱?
  - 閸旂姾娴囬柊宥囩枂閸氬骸鍘涢弸鍕紦閸婃瑩鈧妲х亸鍕剁幢

  - 濡偓濞村鍩岄崘鑼崐閸掓瑨顔囪ぐ鏇☆嚊缂佸棙妫╄箛妤€鑻熼懛顏勫З閸ョ偤鈧偓姒涙顓婚柨顔荤秴閿?
  - 闂堢偞纭?token 娣囨繄鏆€姒涙顓婚獮鍓佺舶閸戝搫鎲＄拃锔衡偓?
- 婢х偛濮為幁銏狀槻姒涙顓婚柊宥囩枂瀵偓閸忕绱?
  - `hotkey.restore_defaults=true` 閺冭泛鎯庨崝銊ㄥ殰閸斻劍浠径宥夌帛鐠併倧绱?
  - 閹垹顦查崥搴ゅ殰閸斻劎鐤嗛崶?`false`閵?
- 娴犺濮熷〒鍛礋閸氬本顒為敍?
  - `1.3.3` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.4.1` 闂団偓鐟曚線鐛欑拠?SRT 鐎涙绠烽崷?seek 閸氬簼绮涙稉搴㈡尡閺€鐐闂傛潙鎮撳銉ｂ偓?
- 瑜版挸澧犵紓鍝勭毌閸欘垶鍣告径宥囨畱閺堫剙婀存灞炬暪閸涙垝鎶ら敍灞炬￥濞夋洜菙鐎规艾浠涢崶鐐茬秺閵?


### 閸掑棙鐎界拋鏉跨秿

1. `PlayerCore::updateSubtitleOverlay()` 瀹告彃鍙挎径鍥╃处鐎涙鍌ㄥ?+ 娴滃苯鍨庣€规矮缍呴柅鏄忕帆閿涘奔绲鹃弮鐘崇《閼磋京顬囬幘顓熸杹 UI 閻欘剛鐝涙宀冪槈閵?
2. 闂団偓鐟曚焦濡哥€涙绠烽弮鍫曟？鏉炴潙灏柊宥囩暬濞夋洘褰侀崣鏍﹁礋閸欘垰顦查悽銊ュ毐閺佸府绱濋獮鑸靛絹娓氭稑鎳℃禒銈堫攽閼奉亝顥呴崗銉ュ經閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`subtitle::resolveActiveSubtitleIndex(...)` 楠炲墎鏁?`PlayerCore` 缂佺喍绔存径宥囨暏閵?
- 閺傛澘顤?`--subtitle-sync-check <subtitle.srt>`閿?
  - 閺堝绨弮鍫曟？鏉炲瓨顥呴弻銉幢

  - seek 鐠哄疇娴嗗Λ鈧弻銉幢

  - 鏉堟挸鍤?mismatch 缂佺喕顓告稉?PASS/FAIL閵?
- 閺傛澘顤冮弽铚傜伐閺傚洣娆?`samples/subtitle/subtitle_seek_sync_sample.srt` 娑撳孩婀伴崷鐗堝Г閸?`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`閵?
- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞藉瑎闁?`1.4.1`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.4.2` 闂団偓鐟曚線鐛欑拠浣规尡閺€鎯у灙鐞涖劏绻涚紒顓熸尡閺€?5 閺傚洣娆㈤妴?
- 缂傚搫鐨崣顖炲櫢婢跺秵澧界悰宀€娈戦懛顏勫З閸栨牠鐛欓弨璺哄弳閸欙絻鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 娑撶粯绁︾粙瀣嚒閺€顖涘瘮 EOF 閼奉亜濮╅崚鍥ㄥ床娑撳绔撮弶锛勬窗閿涘奔绲炬潻鎰攽缂佹挻鐏夐張顏嗙波閺嬪嫬瀵叉潏鎾冲毉閵?
2. 閻滅増婀?`--probe-file` 瀹告彃鍙挎径鍥┣旂€规氨娈戞刊鎺嶇秼閸欘垱澧﹀鈧Λ鈧ù瀣厴閸旀冻绱濋崣顖氼槻閻劋璐熸灞炬暪閸撳秶鐤嗗Λ鈧弻銉ｂ偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--playlist-flow-check`閿?
  - 閺嬪嫬缂撻幘顓熸杹閸掓銆冮獮鎯邦洣濮瑰倽鍤︾亸?5 閺夛紕娲伴敍?
  - 濡偓閺屻儱澧?5 閺夛紕娈戦崣顖涘ⅵ瀵偓閻樿埖鈧緤绱?
  - 閹?EOF 閼奉亜濮╅崚鍥ㄥ床闁槒绶宀冪槈妞ゅ搫绨憰鍡欐磰 `0,1,2,3,4`閿?
  - 鏉堟挸鍤?`playlist-flow-check.result=PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`閵?
- 娴犺濮熷〒鍛礋 `1.4.2` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `1.4.3` 闂団偓鐟曚線鐛欑拠浣筋啎缂冾喖婀柌宥呮儙閸氬骸褰查幁銏狀槻閵?
- 瑜版挸澧犵紓鍝勭毌閸欘垶鍣告径宥囨畱閸涙垝鎶ょ悰宀勭崣閺€璺哄弳閸欙絻鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `loadAppSettings/saveAppSettings` 瀹歌尪顩惄鏍ㄦ尡閺€鎯ф珤閺嶇绺剧拋鍓х枂閸滃瞼鍎归柨顔藉瘮娑斿懎瀵查妴?
2. 闂団偓鐟曚焦鏌婃晶鐐靛缁斿鎳℃禒銈呯殺閳ユ粌鍟撻崗?>闁插秷娴?>濮ｆ柨顕垾婵婄箖缁嬪绮ㄩ弸鍕鏉堟挸鍤妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--settings-persistence-check [settings_file]`閵?
- 娴ｈ法鏁ゅù瀣槸闁板秶鐤嗛幍褑顢?round-trip閿?
  - `volume`

  - `playback_speed`

  - `resume_last_playlist`

  - `last_playlist_index`

  - `toggle_subtitle` 閻戭參鏁?
- 鏉堟挸鍤В蹇庨嚋鐎涙顔岄惃?`*_ok` 缂佹挻鐏夋稉搴⑩偓鑽ょ波閺嬫嚎鈧?
- 姒涙顓绘担璺ㄦ暏娑撳瓨妞傜捄顖氱窞閿涘矂浼╅崗宥嗚杽閺屾挸鐤勯梽鍛村帳缂冾喓鈧?
- 娴犺濮熷〒鍛礋 `1.4.3` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `2.1.2` 闂団偓鐟曚浇顩惄?`mp4/mkv/mov/avi/webm/flv/ts/m2ts` 鐎圭懓娅掗妴?
- 瑜版挸澧犻崶鐐茬秺閺嶉攱婀扮紓鍝勩亼 `mov/avi/m2ts`閵?


### 閸掑棙鐎界拋鏉跨秿

1. `FormatSupport` 瀹告彃锛愰弰搴☆啇閸ｃ劍鏁幐渚婄礉娴ｅ棛宸辩亸鎴濐嚠鎼存柨娲栬ぐ鎺撶壉閺堫剚妫ゅ▔鏇炶埌閹存劙鐛欓弨鍫曟４閻滎垬鈧?
2. 閺嶉攱婀伴悽鐔稿灇閼存碍婀版禒鍛晸閹?5 缁顔愰崳顭掔礉闂団偓閸氬本顒為幍鈺佺潔閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閹碘晛鐫?`format_samples.csv` 閺傛澘顤?`mov/avi/m2ts` 娑撳娼弽閿嬫拱閵?
- 閹碘晛鐫?`download_test_samples.ps1` 閻㈢喐鍨氭稉澶岃閺傜増鐗遍張顑锯偓?
- 閺囧瓨鏌婇弽閿嬫拱閻╊喖缍嶇憴鍕灟娑撳孩鏋冨锝冣偓?
- 鏉╂劘顢?`run_format_regression.ps1` 娴溠冨毉閺堚偓閺傜増婀伴崷鐗堝Г閸涘鈧?
- 娴犺濮熷〒鍛礋 `2.1.2` 閺嶅洩顔囩€瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `FORMAT_REGRESSION_LOCAL_CHECK.md`閿?
  - Total=12

  - PASS=12

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `2.1.3` 闂団偓鐟曚浇顩惄?`H.264/H.265/VP9/AV1/MPEG-2` 鐟欏棝顣剁紓鏍垳閵?
- 閸ョ偛缍婇弽閿嬫拱鐏忔氨宸?`MPEG-2`閿涘本妫ゅ▔鏇炵暚閹存劕鐣弫鎾崣閺€韬测偓?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀侀惌鈺呮█閸栧懎鎯?h264/hevc/vp9/av1閵?
2. `FormatSupport` 瀹告彃瀵橀崥?`mpeg2video`閿涘瞼宸遍崣锝呮躬閺嶉攱婀伴崪灞芥礀瑜版帡鎽肩捄顖樷偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`mpeg2video` 閸ョ偛缍婇弽閿嬫拱閺夛紕娲伴敍鍧眘 鐎圭懓娅掗敍瀹巆3 闂婃娊顣堕敍澶堚偓?
- 閹碘晛鐫?`download_test_samples.ps1` 閼奉亜濮╅悽鐔稿灇 MPEG-2 閺嶉攱婀伴妴?
- 鏉╂劘顢戦崶鐐茬秺楠炶埖娲块弬鐗堟拱閸︾増濮ら崨濞库偓?
- 娴犺濮熷〒鍛礋 `2.1.3` 閺嶅洩顔囩€瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `FORMAT_REGRESSION_LOCAL_CHECK.md`閿?
  - Total=13

  - PASS=13

  - PARTIAL=0

  - FAIL=0

  - SKIP=0

- 閺傛澘顤冮弽閿嬫拱缂佹挻鐏夐敍?
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts` -> `PASS (mpeg2video)`



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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `2.1.4` 闂団偓鐟曚浇顩惄?`AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`閵?
- 閺嶉攱婀伴惌鈺呮█缂傚搫鐨?`E-AC3/DTS/Vorbis/PCM`閵?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀侀弽閿嬫拱瀹歌尪顩惄?aac/mp3/ac3/flac/opus閵?
2. DTS 缂傛牜鐖滈崳?`dca` 閸︺劌缍嬮崜?FFmpeg 娑擃厼鐫樻禍搴＄杽妤犲瞼澹掗幀褝绱濋棁鈧憰?`-strict -2`閵?
3. 閸忕厧顔愰幀褎鐦€靛綊娓舵径鍕倞 `dts/dca`閵嗕梗hevc/h265`閵嗕梗pcm/pcm_*` 缁涘鐜崗宕囬兇閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閹碘晛鐫嶉崶鐐茬秺閺嶉攱婀版稉搴ｆ晸閹存劘鍓奸張顒婄礉閺傛澘顤冮崶娑氳闂婃娊顣堕弽閿嬫拱閵?
- 閸?`download_test_samples.ps1` 娑?DTS 閸涙垝鎶ゆ晶鐐插 `-strict -2`閵?
- 閸?`run_format_regression.ps1` 婢х偛濮炵粵澶夌幆缂傛牜鐖滈崥宥嗙槷鏉堝啫鍤遍弫鑸偓?
- 閺囧瓨鏌婇張顒€婀撮崶鐐茬秺閹躲儱鎲￠獮鍓佲€樼拋銈呭弿闁插繘鈧俺绻冮妴?
- 娴犺濮熷〒鍛礋 `2.1.4` 閺嶅洩顔囩€瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `FORMAT_REGRESSION_LOCAL_CHECK.md`閿?
  - Total=17

  - PASS=17

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `3.1.1` 鐟曚焦鐪?`DecoderFactory` 閹恒儱鍙嗛惇鐔风杽鐟欙絿鐖滈崚婵嗩潗閸栨牗绁︾粙瀣ㄢ偓?
- 閻滄壆濮搁弰?`PlayerCore` 娑撴槒顩︾挧鏉垮敶瀹撳苯鍨庨弨顖炩偓鏄忕帆閿涘畭DecoderFactory` 閸欘亜婀涵顒冃掗柊宥囩枂鐏炩偓闁劏顫﹂崝銊ュ棘娑撳函绱濋張顏勮埌閹存劗绮烘稉鈧崐娆撯偓澶愭懠鐠侯垬鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `DecoderFactory` 瀹稿弶婀侀懗钘夊閹恒垺绁存稉搴濈喘閸忓牏楠囬敍灞肩稻缂傚搫鐨垾婊冩倵缁旑垰鈧瑩鈧绨崚妞烩偓婵囧复閸欙絻鈧?
2. `PlayerCore::initDecoders` 闂団偓鐟曚焦瀵滈崐娆撯偓澶婄碍閸掓鈧劒閲滅亸婵婄槸閿涘奔浜掔€圭偟骞囩紒鐔剁閸掓繂顫愰崠鏍︾瑢閸ョ偤鈧偓閵?
3. `3.1.3` 瀹稿弶褰佹笟娑掆偓婊勬Ц閸氾箑浜告總鐣屸€栫憴锝傗偓婵嬪帳缂冾噯绱濋棁鈧憰浣风箽閻ｆ瑥鑻熸径宥囨暏閵?


### 鐟欙絽鍠呴弬瑙勵攳

- `DecoderFactory` 閺傛澘顤?`selectBackendOrder(codec_name, prefer_hardware)`閿?
  - 閻㈢喐鍨氶幐澶夌喘閸忓牏楠囬幒鎺戠碍閻ㄥ嫯袙閻礁鎮楃粩顖氣偓娆撯偓澶婄碍閸掓绱?
  - 娣囨繆鐦夋潪顖欐鐟欙絿鐖滈崗婊冪俺閸︺劌鈧瑩鈧鎽肩捄顖欒厬閵?
- `PlayerCore::initDecoders` 閹恒儱鍙嗛崐娆撯偓澶婄碍閸掓绱?
  - 闁劒閲滈崥搴ｎ伂鐏忔繆鐦崚婵嗩潗閸栨牔绗?`avcodec_open2`閿?
  - 婢惰精瑙﹂懛顏勫З閸掑洦宕叉稉瀣╃娑擃亜鈧瑩鈧绱?
  - 閹存劕濮涢崥搴ｇ埠娑撯偓鐠佹澘缍嶉張鈧紒鍫濇倵缁旑垱妫╄箛妞尖偓?
- `tryConfigureD3D11HardwareDecode` 閸樺娅庨崘鍛村劥閸氬海顏柅澶嬪閸掋倖鏌囬敍灞炬暭娑撹櫣鍑?D3D11 闁板秶鐤嗛懕宀冪煑閿涘矂鈧瀚ㄧ粵鏍殣閻?`DecoderFactory` 缂佺喍绔撮崘鍐茬暰閵?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `3.1.2` 鐟曚焦鐪?D3D11VA 閸掓繂顫愰崠鏍с亼鐠愩儲妞傝箛鍛淬€忛崣顖炴浆閸ョ偤鈧偓閸掓媽钂嬬憴锝冣偓?
- 閻滅増婀侀柧鎹愮熅閸︺劌鍎氱槐鐘崇壐瀵繐宕楅崯鍡楁礀闁偓閸︾儤娅欐稉瀣╃矌閹垫挸宓冮弮銉ョ箶閿涘苯鎮楃粩顖滃Ц閹焦婀弰鎯х础閸掑洦宕叉稉楦胯拫鐟欙綇绱濈€涙ê婀悩鑸碘偓浣风瑝娑撯偓閼锋挳顥撻梽鈹库偓?


### 閸掑棙鐎界拋鏉跨秿

1. `PlayerCore::selectVideoPixelFormat` 閸︺劍婀崨鎴掕厬 D3D11VA 閸嶅繒绀岄弽鐓庣础閺冩湹绱版潻鏂挎礀鏉烆垯娆㈤崓蹇曠閺嶇厧绱￠妴?
2. 鐠囥儴鐭惧鍕劃閸撳秵婀崥灞绢劄閺囧瓨鏌?`video_decoder_backend_`閿涘苯褰查懗钘夘嚤閼锋潙鎮楃紒顓犫€栨禒鍓佺拨鐎规碍绔婚悶鍡樻蒋娴犺泛鍨介弬顓濈瑝閸戝棛鈥橀妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`selectVideoPixelFormat` 娑擃叀藟閸忓懏妯夊蹇涙缁狙嶇窗

  - 鐏?`video_hw_pixel_fmt_` 闁插秶鐤嗘稉?`AV_PIX_FMT_NONE`閿?
  - 鐏?`video_decoder_backend_` 鐠佸墽鐤嗘稉?`Software`閵?
- 閸?`initDecoders` 閻ㄥ嫬鎮楃粩顖氱毦鐠囨洟鎽肩捄顖欒厬閺傛澘顤冮梽宥囬獓閺冦儱绻旈敍?
  - 瑜?D3D11VA 閸掓繂顫愰崠鏍箖缁嬪鑵戠悮顐㈠礂閸熷棔璐熸潪顖欐鐠侯垰绶為弮璁圭礉鐠佹澘缍嶉垾婊堟缁狙傝礋鏉烆垵袙閳ユ繃褰佺粈鎭掆偓?
- 閸氬本顒為弴瀛樻煀娴犺濮熷〒鍛礋 `3.1.2` 娑撳搫鐣幋鎰┾偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --capabilities` 闁俺绻冮敍鍫滃瘜閸旀稓鐓╅梼?`PASS`閿涘鈧?


### 娣囶喗鏁奸弬鍥︽

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 闂傤噣顣?37: M3 3.2.1閿涘湒3D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻劑鎽肩捄顖ょ礆



**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `3.2.1` 鐟曚焦鐪?D3D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻㈩煉绱檂init/upload/present`閿涘鈧?
- 閻滅増婀?`D3D11VideoRenderer` 娑撶儤銆呯€圭偟骞囬敍瀹峣nit` 閸ュ搫鐣炬径杈Е閿涘本妫ゅ▔鏇炶埌閹存劕褰查悽銊﹁閺屾挸鎮楃粩顖樷偓?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀?`Display` 瀹告彃鍙挎径鍥┣旂€规氨娈?owner-thread 濞撳弶鐓嬫稉搴℃姎娑撳﹣绱堕柧鎹愮熅閿涘苯褰叉径宥囨暏娴?D3D11 閺堚偓鐏忓繘妫撮悳顖樷偓?
2. 闂団偓鐟曚礁褰茬憴鍌涚ゴ瑜版挸澧?SDL renderer 閸氬海顏敍灞间簰閸掋倖鏌囬弰顖氭儊閻喐顒滈崥顖滄暏娴?`direct3d11`閵?
3. 閼汇儱鐤勯梽鍛倵缁旑垯绗夐弰?D3D11閿涘苯绨查崷?`D3D11VideoRenderer::init` 婢惰精瑙﹂獮鏈垫唉缂佹瑤绗傜仦鍌澬曢崣?`3.2.2` 閼奉亜濮╅崶鐐衡偓鈧妴?


### 鐟欙絽鍠呴弬瑙勵攳

- `Display` 閺傛澘顤冨〒鍙夌厠妞瑰崬濮╅幒褍鍩楁稉搴ゎ潎濞村甯撮崣锝忕窗

  - `setPreferredRendererDriver()`閿涙艾鍘戠拋绋挎躬閸掓稑缂?renderer 閸撳秷顔曠純顔间焊婵備粙鈹嶉崝顭掔幢

  - `currentRendererDriver()` / `isUsingRendererDriver()`閿涙俺绻戦崶鐐茬秼閸撳秴鐤勯梽?renderer 閸氬海顏妴?
- `D3D11VideoRenderer` 閺€閫涜礋閺堚偓鐏忓繐褰查悽銊ョ杽閻滃府绱?
  - `init`閿涙艾鍨卞?`Display`閿涘本瀵氱€?`direct3d11` 閸嬪繐銈介敍灞界暚閹存劗鐛ラ崣?濞撳弶鐓嬮崚婵嗩潗閸栨牭绱?
  - `renderFrame/present/clear`閿涙碍甯撮柅姘姎娑撳﹣绱舵稉搴℃啛閻滃府绱?
  - 娴滃娆㈡稉搴㈠付閸掓儼顕Ч鍌涘复閸欙絿绮烘稉鈧柅蹇庣炊 `Display`閵?
- `init` 闂冭埖顔屾晶鐐插閸氬海顏弽锟犵崣閿?
  - 閼汇儱鐤勯梽?renderer 閸氬海顏棃?`direct3d11/d3d11`閿涘奔瀵岄崝銊ㄧ箲閸ョ偛銇戠拹銉ヨ嫙鐠佹澘缍嶉弮銉ョ箶閿涘矁袝閸欐垳绗傜仦?SDL 閸ョ偤鈧偓閵?
- 娴犺濮熷〒鍛礋 `3.2.1` 閺嶅洩顔囩€瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `3.3.2` 鐟曚焦鐪板〒鍙夌厠婢惰精瑙﹂弮璺哄讲閼奉亜濮╅梽宥囬獓娑撴柧绗夋稉顓熸焽閹绢厽鏂侀妴?
- 闂団偓鐟曚椒绔存稉顏勫讲闁插秴顦查幍褑顢戦惃鍕拱閸︿即鐛欓弨璺哄弳閸欙絾娼电粙鍐茬暰妤犲矁鐦?D3D11 婢惰精瑙﹂崥搴ｆ畱 SDL 閸ョ偤鈧偓闁炬崘鐭鹃妴?


### 閸掑棙鐎界拋鏉跨秿

1. `3.2.1/3.2.2` 瀹告彃鍙挎径?D3D11 閸掓繂顫愰崠鏍︾瑢 SDL 閸ョ偤鈧偓閺堝搫鍩楅敍灞肩稻缂傚搫鐨懛顏勫З閸栨牠鐛欓弨璺烘嚒娴犮們鈧?
2. 闂団偓鐟曚礁婀稉宥勭贩鐠ф牔姹夊銉︽惙娴ｆ粎娈戦崜宥嗗絹娑撳绱濆鍝勫煑閸掑爼鈧?D3D11 renderer 閸掓繂顫愰崠鏍с亼鐠愩儱婧€閺咁垬鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤冨〒鍙夌厠/鐟欙絿鐖滈崥搴ｎ伂閸欘垵顫囧ù瀣复閸欙綇绱檙enderer backend / decoder backend閿涘鈧?
- 閺傛澘顤冮崨鎴掓姢 `--renderer-fallback-check <media_file>`閿?
  - 娑撳瓨妞傚▔銊ュ弳 `MVP_D3D11_DRIVER_HINT=software`閿涘苯宸遍崚?D3D11 濞撳弶鐓嬮崚婵嗩潗閸栨牕銇戠拹銉幢

  - 闁俺绻冮幘顓熸杹閸ｃ劋瀵岄柧鎹愮熅妤犲矁鐦夐弰顖氭儊閼奉亜濮╅崶鐐衡偓鈧崚?`SoftwareSDL`閿?
  - 鏉堟挸鍤紒鎾寸€崠鏍х摟濞堝吀绗?`PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`閵?
- 閺囧瓨鏌婃禒璇插濞撳懎宕熼敍灞剧垼鐠?`3.3.2` 鐎瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`

  - `open_ok=true`

  - `renderer_backend=SoftwareSDL`

  - `entered_playback_loop=true`

  - `fallback_to_sdl=true`

  - `result=PASS`



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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `3.3.1` 鐟曚焦鐪伴崷?Windows 娑撳鐛欑拠浣衡€栫憴锝忕礄D3D11VA閿涘鎷版潪顖澬掗敍鍦玱ftware閿涘瀵岄崝娑欑壉閺堫剙娼庨崣顖涙尡閵?
- 閸樼喕浠涢崥鍫濇嚒娴犮倕婀崥宀冪箻缁嬪銆庢惔蹇斿⒔鐞涘苯寮绘导姘崇樈閺冭绱濋崙铏瑰箛閸嬫粍顒涢梼鑸殿唽閸椻€茬秶閿涘苯顕遍懛鏉戞嚒娴犮倛绉撮弮韬测偓?


### 閺冦儱绻旀潏鎾冲毉

```text

windows-backend-check.command=... 

... The filename, directory name, or volume label syntax is incorrect.

windows-backend-check.result=FAIL

```



### 閸掑棙鐎界拋鏉跨秿

1. 閸氬矁绻樼粙瀣箾缂侇叀绐?hard/soft 娴兼俺鐦介弮璁圭礉缁楊兛绨╂潪顔肩摠閸︺劌娲栭弨鍫曟懠鐠侯垯绗夌粙鍐茬暰閵?
2. `std::system` + 闁插秴鐣鹃崥鎴濇躬瑜版挸澧犵捄顖氱窞缂佸嫬鎮庢稉瀣摠閸?shell 鐟欙絾鐎芥稉宥嚽旂€规哎鈧?
3. 闂団偓鐟曚礁鐨㈡导姘崇樈闂呮梻顬囬獮鑸垫暭娑撶儤娲跨粙鍐茬暰閻ㄥ嫬鐡欐潻娑氣柤閸掓稑缂撻弬鐟扮础閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--windows-backend-session-check <media_file> <hard|soft>`閿涘本鐦″▎鈥冲涧妤犲矁鐦夋稉鈧稉顏冪窗鐠囨繂鑻熸潏鎾冲毉缂佹挻鐎崠鏍х摟濞堢偣鈧?
- 鐏?`--windows-backend-check` 閺€閫涜礋閻栨儼绻樼粙瀣鐠ц渹琚辨稉顏勭摍鏉╂稓鈻奸敍鍧攁rd閵嗕够oft閿涘鑻熷Ч鍥ㄢ偓鑽ょ波閺嬫嚎鈧?
- Windows 娑撳鏁?`CreateProcess` + 閺傚洣娆㈤崣銉︾労闁插秴鐣鹃崥鎴﹀櫚闂嗗棗鐡欐潻娑氣柤鏉堟挸鍤敍宀勪缉閸?shell 鐠囶厽纭堕梻顕€顣介妴?
- 婢х偛濮為張顒€婀撮幎銉ユ啞 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`閵?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 hard` 闁俺绻冮敍鍦SS閿涘鈧?
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 soft` 闁俺绻冮敍鍦SS閿涘鈧?
- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4` 闁俺绻冮敍鍦SS閿涘鈧?
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4` 闁俺绻冮敍鍦SS閿涘鈧?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍦SS閿涘鈧?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `3.3.1` 閺嶅洩顔囩€瑰本鍨氶妴?
  - `3.3.3` 閺嶅洩顔囩€瑰本鍨氶妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `4.1` 鐟曚焦鐪伴幓鎰返缁旂姾濡€佃壈鍩呴懗钘夊閿涘牅绗傛稉鈧粩?娑撳绔寸粩鐙呯礆閵?
- 瑜版挸澧犻幘顓熸杹娑撳鎽肩捄顖涚梾閺堝鐝烽懞鍌濐嚞濮瑰倸濮╂担婊冩嫲鐠哄磭鐝烽崗銉ュ經閿涘本妫ゅ▔鏇氱矤闁款喚娲忛惄瀛樺复鐠哄磭鐝烽妴?


### 閸掑棙鐎界拋鏉跨秿

1. 缁旂姾濡崗鍐╂殶閹诡喗娼甸懛?`AVFormatContext::chapters`閿涘矂娓剁憰浣告躬 `Demuxer` 瀵偓濡楋綁妯佸▓鍨絹閸欐牕鑻熸穱婵嗙摠閸︺劌鐛熸担鎾蹭繆閹垯鑵戦妴?
2. 鏉堟挸鍙嗘禍瀣╂闁炬崘鐭鹃棁鈧弬鏉款杻缁旂姾濡崝銊ょ稊闁繋绱堕敍姝欴isplay -> Renderer -> PlayerCore -> VideoPlayer`閵?
3. 闂団偓鐟曚焦鏌婃晶鐐叉嚒娴犮倛顢戦懛顏咁梾閸忋儱褰涢敍宀勪缉閸忓秶鐝烽懞鍌氼嚤閼割亙绮庢笟婵婄閹靛浼愰幘顓熸杹妤犲矁鐦夐妴?


### 鐟欙絽鍠呴弬瑙勵攳

- `Demuxer` 閺傛澘顤冪粩鐘哄Ν濡€崇€烽敍?
  - 婢х偛濮?`ChapterInfo` 缂佹挻鐎稉?`MediaInfo::chapters`閿?
  - 鐟欙絾鐎?`AVChapter` 閻ㄥ嫯鎹ｅ銏℃闂傛潙鎷伴弽鍥暯閿涘苯鑻熼幐澶庢崳婵妞傞梻瀛樺笓鎼村繈鈧?
- 韫囶偅宓庨柨顔荤瑢鐠囬攱鐪伴柧鎹愮熅閹碘晛鐫嶉敍?
  - `HotkeyManager` 閺傛澘顤?`PreviousChapter` / `NextChapter`閿?
  - 姒涙顓婚柨顔荤秴缂佹垵鐣?`HOME` / `END`閿?
  - `Display`閵嗕梗IVideoRenderer` 閸欏﹤鎮囧〒鍙夌厠閸ｃ劌鐤勯悳鐗堟煀婢х偟鐝烽懞鍌濐嚞濮瑰倹绉风拹瑙勫复閸欙絻鈧?
- `PlayerCore` 婢х偛濮炵粩鐘哄Ν鐠哄疇娴嗛柅鏄忕帆閿?
  - `rebuildChapterPoints()` 閸?`open()` 閸氬孩鐎铏圭彿閼哄倹妞傞梻瀵稿仯閿?
  - 閺傛澘顤?`seekToNextChapter()` / `seekToPreviousChapter()`閿?
  - `pumpEvents()` 濞戝牐鍨傜粩鐘哄Ν鐠囬攱鐪伴獮鎯靶曢崣?seek閵?
- `VideoPlayer` 婢х偛濮炵粩鐘哄Ν API 閸栧懓顥婇敍?
  - `seekToNextChapter()` / `seekToPreviousChapter()` / `chapterCount()`閵?
- `main` 閺傛澘顤?`--chapter-nav-check <media_file>`閿?
  - 閼奉亜濮╅幍褑顢戦幘顓熸杹閵嗕椒绗呮稉鈧粩鐘偓浣风瑐娑撯偓缁旂姵绁︾粙瀣剁幢

  - 鏉堟挸鍤?`chapter-nav-check.*` 鐎涙顔屾稉?`PASS/FAIL`閵?
- 娴犺濮熷〒鍛礋閸氬本顒為敍?
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 閺嶅洩顔?`4.1` 鐎瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `4.2` 鐟曚焦鐪伴弨顖涘瘮 A-B Repeat閵?
- 瑜版挸澧犳稉缁樼ウ缁嬪宸辩亸?A/B/C 閸栨椽妫垮顏嗗箚閹貉冨煑閿涘本妫ゅ▔鏇☆啎缂?A 閻愬箍鈧竻 閻愮懓鑻熼柌宥咁槻閹绢厽鏂侀崠娲？閵?


### 閸掑棙鐎界拋鏉跨秿

1. 閻戭參鏁稉搴濈皑娴犳儼顕Ч鍌炴懠鐠侯垰鍑￠崗宄邦槵閸欘垱澧跨仦鏇熌佸蹇ョ礉閸欘垰顦查悽銊よ礋 A-B Repeat 鐠囬攱鐪伴柅蹇庣炊閵?
2. `PlayerCore` 瀹稿弶婀?seek 娑撳孩鎸遍弨鍙ョ秴缂冾喚濮搁幀渚婄礉闁倸鎮庨弬鏉款杻閸栨椽妫块悩鑸碘偓浣歌嫙閸︺劍鎸遍弨鍙ヨ厬鐟欙箑褰傞崶鐐剁儲閵?
3. 闂団偓鐟曚焦鏌婃晶鐐叉嚒娴犮倛顢戦懛顏咁梾閸忋儱褰涢敍宀€鈥樻穱婵嗗隘闂傛潙鎯婇悳顖氬讲缁嬪啿鐣鹃崶鐐茬秺閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤冮悜顓㈡暛閸斻劋缍旀稉搴ㄧ帛鐠併倗绮︾€规熬绱?
  - `SetABRepeatStart`閿涘潉A`閿涘绱?
  - `SetABRepeatEnd`閿涘潉B`閿涘绱?
  - `ClearABRepeat`閿涘潉C`閿涘鈧?
- 閹碘晛鐫嶉柧鎹愮熅閿?
  - `Display` 婢х偛濮?A-B Repeat 鐠囬攱鐪伴弽鍥唶/濞戝牐鍨傞敍?
  - `Renderer` 閹跺€熻杽娑?SDL/D3D11/OpenGL 鐎圭偟骞囬弬鏉款杻鐎电懓绨查柅蹇庣炊閹恒儱褰涢妴?
- `PlayerCore` 婢х偛濮?A-B Repeat 閹貉冨煑閼宠棄濮忛敍?
  - `setABRepeatStart()`閵嗕梗setABRepeatEnd()`閵嗕梗clearABRepeat()`閿?
  - `isABRepeatEnabled()`閵嗕梗abRepeatStart()`閵嗕梗abRepeatEnd()`閿?
  - `handleABRepeatLoop()` 閸︺劍鎸遍弨鍙ヨ厬濡偓濞村鍩屾潏?B 閻愮懓鎮楅懛顏勫З seek 閸?A 閻愬箍鈧?
- `VideoPlayer` 婢х偛濮?A-B Repeat API 閸栧懓顥婇妴?
- `main` 閺傛澘顤?`--ab-repeat-check <media_file>` 妤犲本鏁归崨鎴掓姢娑撳骸搴滈崝鈺€淇婇幁顖樷偓?
- 娣囶喖顦查崶鐐茬秺濡偓閺屻儱鍟跨粣渚婄窗

  - `--settings-persistence-check` 濞村鐦柨顔荤秴閻?`b` 閺€閫涜礋 `x`閿涘矂浼╅崗宥勭瑢閺備即绮拋?`B` 閻戭參鏁崘鑼崐閵?
- 娴犺濮熷〒鍛礋閸氬本顒為敍?
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 閺嶅洩顔?`4.2` 鐎瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `4.3` 闂団偓鐟曚焦鏁幐浣瑰焻閸ヤ勘鈧?
- 瑜版挸澧犻幋顏勬禈闁炬崘鐭炬禒宥咁槱娴?WIP閿涙氨鍎归柨顔衡偓浣峰瘜濞翠胶鈻兼稉?`--screenshot-check` 瀹稿弶甯撮崗銉礉娴ｅ棙娈忛崑婊勨偓浣筋嚞濮瑰倻宸辩亸鎴斺偓婊勬付閸氬孩妯夌粈鍝勬姎閳ユ繄绱︾€涙﹫绱濈€佃壈鍤ч弳鍌氫粻閺冭埖妫ゅ▔鏇犌旂€规矮绻氱€涙ê缍嬮崜宥囨暰闂堫潿鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 閻ㄥ嫯顕Ч鍌炴懠鐠侯垰鍑＄紒蹇撳徔婢跺浄绱濋崣顏勬▕閹绢厽鏂侀崳銊︾壋韫囧啫顕垾婊勬付鏉╂垳绔寸敮褉鈧繄娈戞穱婵堟殌閼宠棄濮忛妴?
2. 閻滅増婀佺€圭偟骞囬崣顏勬躬濞撳弶鐓嬬痪璺ㄢ柤婢跺嫮鎮婇幋顏勬禈鐠囬攱鐪伴敍娑楃閺冿箒绻橀崗銉︽畯閸嬫粍鈧緤绱濈拫鍐ㄥ閸ｃ劌浠犲銏㈡埛缂侇參鈧礁鎶氶敍灞惧焻閸ユ崘顕Ч鍌氭皑閹峰じ绗夐崚鐗堟煀閻ㄥ嫬娴橀崓蹇旀殶閹诡喓鈧?
3. 鐟曚浇顔€閹搭亜娴橀幋鎰礋閸欘垳鏁ら崝鐔诲厴閿涘矂娅庢禍鍡毸夋鎰处鐎涙ê顦婚敍宀冪箷闂団偓鐟曚焦濡搁懛顏咁梾閺€瑙勫灇鐟曞棛娲婇垾婊勬畯閸嬫粍鈧焦鍩呴崶閿偓婵撶礉閸氾箑鍨弮鐘崇《鐠囦焦妲戦弽鐟版礈瀹歌弓鎱ㄦ径宥冣偓?


### 鐟欙絽鍠呴弬瑙勵攳

- `PlayerCore` 閺傛澘顤冮張鈧潻鎴滅濞嗏剝瑕嗛弻鎾虫姎缂傛挸鐡ㄩ敍?
  - `updateLastRenderedFrame()` 閸︺劍鐦″▎鈥虫啛閻滄澘鎮楃紓鎾崇摠瑜版挸澧犵敮褝绱?
  - `captureScreenshotFromCachedFrame()` 閺€顖涘瘮閸︺劍娈忛崑婊勨偓浣烘纯閹恒儲鍩呴崶鎾呯幢

  - `clearLastRenderedFrame()` 閸︺劑鍣搁弬鐗堝ⅵ瀵偓/閸忔娊妫存刊鎺嶇秼閺冭埖绔婚悶鍡欑处鐎涙ǜ鈧?
- `requestScreenshot()` 鐠嬪啯鏆ｆ稉鐚寸窗

  - 閹绢厽鏂佹稉顓ㄧ窗缂佸瓨瀵斿鍌涱劄鐠囬攱鐪伴敍?
  - 闂堢偞鎸遍弨鐐偓渚婄礄闁插秶鍋ｉ弰顖涙畯閸嬫粍鈧緤绱氶敍姘辨纯閹恒儰绮犵紓鎾崇摠鐢嗘儰閻╂ǜ鈧?
- `--screenshot-check` 鐠嬪啯鏆ｆ稉琛♀偓婊冨帥閹绢厽鏂?-> 閺嗗倸浠?-> 鐠囬攱鐪伴幋顏勬禈 -> 閺嶏繝鐛欐潏鎾冲毉閺傚洣娆㈤垾婵撶礉绾喕绻氶張顒侇偧娣囶喖顦查張澶愭嫛鐎佃鈧囩崣鐠囦降鈧?
- 閺傚洦銆傛稉搴濇崲閸斺剝绔婚崡鏇炴倱濮濄儻绱?
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 閺嶅洩顔?`4.3` 鐎瑰本鍨氶敍?
  - 閺傛澘顤冮張顒€婀撮幎銉ユ啞 `docs/reports/SCREENSHOT_LOCAL_CHECK.md`閿?
  - 閺囧瓨鏌?`README.md` / `README_ZH.md` 閻ㄥ嫬鎻╅幑鐑芥暛鐠囧瓨妲戦妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug --target modern-video-player` 闁俺绻冮妴?
- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 闁俺绻冮敍鍧凱ASS`閿涘鈧?
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 闁俺绻冮敍鍧凱ASS`閿涘鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 娴犲秴浠犻悾娆忔躬 2026-03-07 娑斿澧犻惃鍕槑娴兼澘褰涘鍕┾偓?
- 閺傚洦銆傞幎濠傜摟楠炴洏鈧礁鎻╅幑鐑芥暛閵嗕浇顔曠純顔衡偓浣规尡閺€鎯у灙鐞涖劊鈧浇袙閻礁娅掔粻锛勬倞缁涘鍏橀崝娑樺晸閹存劏鈧粍婀幒銉ュ弳/妤犮劍鐏﹂垾婵撶礉娑撳骸缍嬮崜宥勫敩閻礁鎷伴張顒€婀存灞炬暪閹躲儱鎲℃稉宥勭閼锋番鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `M1`閵嗕梗M3` 娑?`M4 4.1/4.2/4.3` 瀹告煡妾扮紒顓℃儰閸﹀府绱濇担鍡楁▕鐠烘繆鐦庢导鐗堟瀮濡楋絾鐥呴張澶婃倱濮濄儱鍩涢弬鑸偓?
2. 缂佈呯敾濞岃法鏁ら弮褏绮ㄧ拋杞扮窗鐠囶垰顕遍崥搴ｇ敾娴兼ê鍘涚痪褍鍨介弬顓ㄧ礉閻楃懓鍩嗛弰顖欑窗閹跺﹤鍑＄€瑰本鍨氭い鍦埛缂侇厼鍨崗?`P0/P1`閵?
3. 瀹割喛绐涚拠鍕強閺傚洦銆傞棁鈧憰浣告倱閺冭泛寮懓鍐у敩閻礁鍙嗛崣锝呮嫲 `docs/reports/*` 妤犲本鏁归幎銉ユ啞閿涘矁鈧奔绗夐弰顖氬涧閻琚?閹恒儱褰涢弰顖氭儊鐎涙ê婀妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 鐏?`docs/analysis/MPC_HC_GAP_ANALYSIS.md` 閸忋劍鏋冮弴瀛樻煀閸?2026-03-08 閻楀牊婀伴妴?
- 闁插秴鍟撳Ο鈥虫健閹槒顫嶉妴浣鼓侀崸妤冪埠鐠伮扳偓浣稿⒖娴ｆ瑥妯婄捄婵囩閸楁洑绗屽楦款唴闁插瞼鈻肩喊鎴欌偓?
- 閹跺﹤鐡ч獮鏇樷偓浣规尡閺€鎯у灙鐞涖劊鈧浇顔曠純顔衡偓浣告彥閹圭兘鏁妴浣叫掗惍浣告珤缁狅紕鎮婇妴浣瑰焻閸ュ墽鐡戝鑼舵儰閸︽媽鍏橀崝娑楃矤閳ユ粓顎囬弸?閺堫亝甯撮崗銉⑩偓婵堢眰濮濓絼璐熼垾婊堝劥閸掑棗鐤勯悳鎵斥偓婵勨偓?
- 閺傛澘顤冮垾婊堢崣閺€鏈电瑢閹躲儱鎲＄拠浣瑰祦閳ユ繄鐝烽懞鍌︾礉閺勫海鈥樼拠鍕強娓氭繃宓侀崠鍛儓閺堫剙婀撮崶鐐茬秺閹躲儱鎲￠妴?
- 閺囧瓨鏌婇弬鍥ㄣ€傜槐銏犵穿閿涘矁藟娑撯偓閺夆剝婀板▎鈥虫▕鐠烘繆鐦庢导鏉款嚠姒绘劘顕╅弰搴涒偓?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 閺嶅洭顣介弮銉︽埂瀹稿弶娲块弬棰佽礋 `2026-03-08`閵?
- 濡€虫健缂佺喕顓搁弴瀛樻煀娑撶尨绱癭闁劌鍨庣€圭偟骞?11 / 14`閵嗕梗妤犮劍鐏?閺堫亝甯撮崗?3 / 14`閵?
- `P0/P1/P2` 閸撯晙缍戞い鐟板嚒娑撳骸缍嬮崜宥勬崲閸斺剝绔婚崡鏇炴嫲妤犲本鏁归幎銉ユ啞娣囨繃瀵旀稉鈧懛娣偓?


### 娣囶喗鏁奸弬鍥︽

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 闂傤噣顣?44: `docs/records/VERSION.md` 閸樺棗褰剁捄顖氱窞閹诲繗鍫潻鍥ㄦ埂



**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `docs/records/VERSION.md` 閻ㄥ嫭妫張鐔兼▉濞堝灚寮挎潻棰佺矝娣囨繄鏆€閺冄呭 decoder/thread/test 閺傚洣娆㈢痪褑銆冩潻甯礉閻鎹ｉ弶銉ュ剼瑜版挸澧犳禒鎾崇氨娴犲秴婀担璺ㄦ暏鏉╂瑤绨虹捄顖氱窞閵?
- 鏉╂瑤绱拌ぐ鍗炴惙閹稿鏋冨锝変憾閸樺棔绮ㄦ惔鎾存閻ㄥ嫯绻樻惔锕€鍨介弬顓ㄧ礉娑旂喎顔愰弰鎾村Ω閸樺棗褰剁€圭偟骞囬崪灞界秼閸撳秳瀵岄柧鍓х波閺嬪嫭璐╂稉杞扮鐠嬪牄鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `2026-03-06` 閺嬭埖鐎弨鑸垫殐閸氬函绱濋幘顓熸杹閸ｃ劋瀵岄柧鎯у嚒缂佸繐鍨忛崚?`PlayerCore + Scheduler + core/*`閿涘奔绲鹃悧鍫熸拱閺傚洦銆傞惃鍕坊閸欒尙鐝烽懞鍌涚梾閺堝鎮撳銉ュ箵濮澭傜疅閵?
2. 閳ユ粓妯佸▓鍏哥閿涙艾鐔€绾偓閹绢厽鏂侀崳?(瑜版挸澧犻梼鑸殿唽)閳?娑撳簶鈧粈绗呮稉鈧銉吀閸掓巻鈧繄鐡戦幒顏囩犯瀹歌尙绮℃稉宥呭晙缁楋箑鎮?`2026-03-08` 閻ㄥ嫬鐤勯梽鍛Ц閹降鈧?
3. 閸樺棗褰剁粩鐘哄Ν鎼存棁顕氭穱婵堟殌閼宠棄濮忓鏃囩箻閸滃矂妫舵０妯规叏婢跺秷顔囪ぐ鏇礉娴ｅ棔绗夋惔鏂垮晙閹跺﹤鍑￠崚鐘绘珟閺傚洣娆㈣ぐ鎾茬稊瑜版挸澧犳禒鎾崇氨缂佹挻鐎拠瀛樻閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 鐏忓棝妯佸▓鍏哥閺€鐟板晸娑撹　鈧粌宸婚崣鑼舵崳閻愬厜鈧繐绱濋獮鎯八夐崗鍛＋閻?`decoder / playLoop` 閺嬭埖鐎鎻掕嫙閸忋儱缍嬮崜宥勫瘜闁惧墽娈戠拠瀛樻閵?
- 鐏?`video_decoder` / `audio_decoder`閵嗕梗VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 缁涘妫捄顖氱窞閺€鐟板晸娑撻缚鍏橀崝娑氶獓閸樺棗褰剁悰銊ㄥ牚閵?
- 鐏忓棌鈧粈绗呮稉鈧銉吀閸掓巻鈧縿鈧梗USE_NEW_PLAYER_CORE`閵嗕椒澶嶉弮?`tests/core_*` 閻ㄥ嫮娴夐崗铏伎鏉╂媽鐨熼弫缈犺礋閸樺棗褰剁拋鏉跨秿閸欙絽绶為妴?
- 閸?`docs/records/VERSION.md` 閻ㄥ嫭鏋冨锝嗘纯閺傜増妫╄箛妞捐厬鐞涖儴顔囬張顒侇偧濞撳懐鎮婇妴?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `docs/records/VERSION.md` 娑撳秴鍟€閸戣櫣骞囬垾婊冪秼閸撳秹妯佸▓纰樷偓婵冣偓婊€绗呮稉鈧銉吀閸掓巻鈧繄鐡戦弰鎾诡嚖鐎电厧缍嬮崜宥囧Ц閹胶娈戦弮褍褰涘鍕┾偓?
- 閺冣晜婀＄粩鐘哄Ν鐎佃妫悧?decoder/thread 閻ㄥ嫯銆冩潻鏉垮嚒閺€閫涜礋閸樺棗褰剁拠瀛樻閿涘苯鑻熼幐鍥ф倻瑜版挸澧?`core/*` 娑撳鎽奸妴?
- 閺堫剚顐奸崣妯绘纯闂勬劕鐣鹃崷銊︽瀮濡楋絽鐪伴敍灞炬弓閹碘晜鏆庨崚棰佸敩閻焦鍨ㄩ弸鍕紦闁板秶鐤嗛妴?


### 娣囶喗鏁奸弬鍥︽

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 闂傤噣顣?45: README 娑撳孩鐏﹂弸鍕瀮濡楋絼绮涘ǎ椋庢暏閺冄傚瘜闁炬崘銆冩潻?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閺嶅湱娲拌ぐ?`README.md` / `README_ZH.md` 娴犲秵濡?`video_decoder`閵嗕梗audio_decoder` 缁涘妫弬鍥︽閸愭瑦鍨氭い鍦窗瑜版挸澧犵紒鎾寸€妴?
- `docs/design/ARCHITECTURE.md` 閸氬本妞傚ǎ椋庢暏娴滃棌鈧粌缍嬮崜宥呯杽閻滄壋鈧繂鎷伴弮褎膩閸ф鎳￠崥宥忕礉鐠囨槒鈧懎顔愰弰鎾诡嚖閸掋倗骞囩悰灞煎瘜闁惧彞绮涙笟婵婄鏉╂瑤绨洪崢鍡楀蕉濡€虫健閵?


### 閸掑棙鐎界拋鏉跨秿

1. `2026-03-06` 閺嬭埖鐎弨鑸垫殐閸氬函绱濊ぐ鎾冲閹绢厽鏂侀崳銊ゅ瘜闁炬儳鍑＄紒蹇暻旂€规艾婀?`VideoPlayer + PlayerCore + Scheduler + core/*`閵?
2. `README` 閺囨潙浜搁崥鎴斺偓婊冩彥闁喓鎮婄憴锝勭波鎼存挴鈧繄娈戦崗銉ュ經閺傚洦銆傞敍灞芥礈濮濄倗娲拌ぐ鏇熺埐閸滃本鐏﹂弸鍕仛閹板繐绨叉导妯哄帥閸欏秵妲ч悳鎵Ц閿涘矁鈧奔绗夐弰顖欑箽閻ｆ瑥鍑￠崚鐘绘珟鐠侯垰绶為妴?
3. `docs/design/ARCHITECTURE.md` 娴ｆ粈璐熼懗灞炬珯閺傚洦銆傞崣顖欎簰娣囨繄鏆€閸樺棗褰堕崘鍛啇閿涘奔绲捐箛鍛淬€忛弰鎯х础閺嶅洦鏁為崫顏冪昂缁旂姾濡仦鐐扮艾閸樺棗褰剁€圭偟骞囬妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 闁插秴鍟?`README.md` / `README_ZH.md` 閻ㄥ嫰銆嶉惄顔剧波閺嬪嫭鐖查崪灞剧仸閺嬪嫮銇氶幇蹇ョ礉閺€閫涜礋瑜版挸澧犳稉濠氭懠閸欙絽绶為妴?
- 閸?`docs/design/ARCHITECTURE.md` 妞ゅ爼鍎存晶鐐插閻樿埖鈧浇顕╅弰搴礉楠炶泛鐨㈤弮?decoder/thread/sync 缁旂姾濡紒鐔剁閺嶅洦鍨氶垾婊冨坊閸欐彃鐤勯悳鎵斥偓婵勨偓?
- 鐏忓棙鏋冨锝勮厬閻ㄥ嫭妫╄箛妤冦仛娓氬鏁兼稉鍝勭秼閸?`Quill` 鐎瑰繑甯撮崣锝冣偓?
- 閺囧瓨鏌?`docs/README.md` 閻ㄥ嫭鐏﹂弸鍕瀮濡楋絿鍌ㄥ鏇☆嚛閺勫函绱濋崠鍝勫瀻閸樺棗褰堕崺铏瑰殠閸滃苯缍嬮崜宥夊櫢閺嬪嫯顕╅弰搴涒偓?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `README.md` / `README_ZH.md` 瀹歌弓绗夐崘宥嗗Ω `video_decoder` / `audio_decoder` 閸愭瑤璐熻ぐ鎾冲閻╊喖缍嶇紒鎾寸€妴?
- `docs/design/ARCHITECTURE.md` 瀹稿弶妲戠涵顔硷紣閺勫骸宸婚崣鑼彿閼哄倽绔熼悾宀嬬礉楠炴湹绗夐崘宥嗗Ω閺冄冾樋缁捐法鈻奸柧鎹愮熅閺嶅洣璐熼垾婊冪秼閸撳秴鐤勯悳鎵斥偓婵勨偓?
- 閺堫剚顐奸弨鐟板З娴犲秹妾虹€规艾婀弬鍥ㄣ€傜仦鍌︾礉閺堫亝绉归崣濠佸敩閻礁鎷伴弸鍕紦闁板秶鐤嗛妴?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `docs/guides/IMPLEMENTATION.md` 娣囨繄鏆€閻ㄥ嫭妲搁弮鈺傛埂閸樼喎鐎烽惃鍕矤闂嗚泛鐤勯悳鐗堟殌缁嬪绱濇担鍡樼梾閺堝妲戠涵顔硷紣閺勫骸鍙炬稉?`video_decoder`閵嗕梗audio_decoder`閵嗕礁宕熸担?`playLoop` 缁涘鍞寸€圭懓鐫樻禍搴″坊閸欐彃鐤勯悳鑸偓?
- `docs/plans/MPC_HC_ITERATION_PLAN.md` 鐠佹澘缍嶉惃鍕Ц `2026-03-07` 閻ㄥ嫯顓搁崚鎺戠唨缁惧尅绱濇担鍡樼梾閺堝褰佺粈鍝勫従娑擃參鍎撮崚鍡氬厴閸旀稑鍑￠崷銊ユ倵缂侇厽褰佹禍銈勮厬閽€钘夋勾閵?


### 閸掑棙鐎界拋鏉跨秿

1. 鏉╂瑤琚辨禒鑺ユ瀮濡楋綁鍏樻潻妯绘箒娴ｈ法鏁ゆ禒宄扳偓纭风窗閸撳秷鈧懎浜搁弫娆戔柤閿涘苯鎮楅懓鍛焊鐟欏嫬鍨濋敍娑㈡６妫版ê婀禍搴ｅ繁鐏忔垵鎷伴垾婊冪秼閸撳秳鍞惍浣哄Ц閹讲鈧繄娈戦弰鎯х础閸掑棛鏅妴?
2. 缂佸繗绻冮崜宥呭殤鏉烆喗绔婚悶鍡楁倵閿涘畭README`閵嗕梗VERSION`閵嗕梗ARCHITECTURE` 瀹歌尙绮￠崠鍝勫瀻閸樺棗褰舵稉搴＄秼閸撳稄绱濇俊鍌涚亯鏉╂瑤琚辨禒鑺ユ瀮濡楋絼绗夌捄鐔荤箻閿涘矁顕伴懓鍛矝娴兼艾婀崗銉ュ經鐏炲倷楠囬悽鐔歌穿濞ｅ棎鈧?
3. 閺堚偓閸氬牓鈧倻娈戦崑姘《娑撳秵妲搁柌宥呭晸閸忋劍鏋冮敍宀冣偓灞炬Ц閸︺劍鏋冨锝夈€婇柈銊ㄋ夐崗鍛Ц閹浇顕╅弰搴礉楠炶埖濡哥槐銏犵穿閹诲繗鍫紒鐔剁閸掓澘鎮撴稉鈧崣锝呯窞閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 娑?`docs/guides/IMPLEMENTATION.md` 婢х偛濮為垾婊呭Ц閹浇顕╅弰搴礄2026-03-08閿涘鈧繐绱濋弰搴ｂ€橀崗鏈佃礋閺冣晜婀￠崢鐔风€烽弫娆戔柤閿涘苯鑻熼幐鍥ф倻瑜版挸澧犳稉濠氭懠閸欏倽鈧啯鏋冨锝冣偓?
- 娑?`docs/plans/MPC_HC_ITERATION_PLAN.md` 婢х偛濮為垾婊呭Ц閹浇顕╅弰搴礄2026-03-08閿涘鈧繐绱濋弰搴ｂ€橀崗鏈佃礋 `2026-03-07` 閻ㄥ嫯顓搁崚鎺戞彥閻撗嶇礉楠炶泛鍨崙鍝勭秼閸撳秷绻樻惔锕€寮懓鍐ㄥ弳閸欙絻鈧?
- 閺囧瓨鏌?`docs/README.md`閵嗕梗README.md`閵嗕梗README_ZH.md` 閻ㄥ嫭鏋冨锝堫嚛閺勫函绱濇担鍨坊閸欏弶鏆€缁嬪鈧浇顓搁崚鎺戞彥閻撗佲偓浣哥秼閸撳秳瀵岄柧鎹愵嚛閺勫簼绗侀懓鍛扮珶閻ｅ奔绔撮懛娣偓?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `docs/guides/IMPLEMENTATION.md` 瀹歌弓绗夐崘宥堫潶鐞涖劏鍫稉鍝勭秼閸撳秳绮ㄦ惔鎾舵畱闁劖鏋冩禒璺虹杽閺傝姤绔婚崡鏇樷偓?
- `docs/plans/MPC_HC_ITERATION_PLAN.md` 瀹稿弶妲戠涵顔昏礋鐠佲€冲灊韫囶偆鍙庨敍灞借嫙閹稿洤鎮滆ぐ鎾冲鏉╂稑瀹抽弶銉︾爱閵?
- 閺堫剚顐奸弨鐟板З娴犲秹妾虹€规艾婀弬鍥ㄣ€傜仦鍌︾礉閺堫亝绉归崣濠佸敩閻降鈧焦鐎鍝勬嫲娴犺濮熷〒鍛礋閻樿埖鈧椒鎱ㄩ弨骞库偓?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `docs/design/FILTERS.md`閵嗕梗docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`閵嗕梗docs/guides/WINDOWS_SETUP.md` 娴犲秴浜搁崥鎴斺偓婊堟饯閹浇顕╅弰搴樷偓婵撶礉缂傚搫鐨稉搴＄秼閸撳秳鍞惍浣峰瘜闁句勘鈧椒绶风挧鏍ㄥ赴濞村鏌熷蹇撴嫲閺傚洦銆傞柅鍌滄暏閼煎啫娲块惃鍕敊閹恒儴顕╅弰搴涒偓?
- `docs/guides/WINDOWS_SETUP.md` 鏉╂ü绻氶悾娆庣啊閺冄呮畱 `SDL2_DIR` / `FFMPEG_DIR` 闁板秶鐤嗙粈杞扮伐閿涘奔绗岃ぐ鎾冲 `CMakeLists.txt` 閻ㄥ嫭澧滈崝銊ょ贩鐠ф牕娲栭柅鈧捄顖氱窞娑撳秴鐣崗銊ょ閼锋番鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 缂佸繗绻冮崜宥呭殤鏉烆喗绔婚悶鍡楁倵閿涘苯鍙嗛崣锝嗘瀮濡楋絽鎷伴弽绋跨妇鐠佹崘顓搁弬鍥ㄣ€傚鑼病閸栧搫鍨庢禍鍡忊偓婊冨坊閸欐彃鐔€缁惧簱鈧繂鎷伴垾婊冪秼閸撳秳瀵岄柧閿偓婵撶礉娴ｅ棜绶熼崝鈺勵嚛閺勫孩鏋冨锝堢箷濞屸剝婀佺€瑰苯鍙忕捄鐔剁瑐閵?
2. `FILTERS.md` 閻ㄥ嫪瀵岀憰渚€妫舵０妯圭瑝閺勵垶鏁婄拠顖ょ礉閼板本妲哥紓鍝勭毌閳ユ粌缍嬮崜宥夌帛鐠併倓瀵屽ù浣衡柤閸忋儱褰涢弰?`FilterPipeline`閳ユ繆绻栨稉鈧仦鍌澬掗柌濞库偓?
3. `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 閺囨潙鍎氭稉鎾活暯閸欏倽鈧啰鐟拋甯礉闂団偓鐟曚焦褰侀柋鎺曨嚢閼板懍绗夌憰浣瑰Ω閸忔湹鑵戦惃鍕窗閺嶅洭銆嶉惄瀛樺复缁涘鎮撴禍搴＄秼閸撳秵婀€瑰本鍨氭い骞库偓?
4. `WINDOWS_SETUP.md` 閸掓瑩娓剁憰浣风瑢閻滅増婀?`CMakeLists.txt` 閻ㄥ嫪绶风挧鏍ㄥ赴濞村銆庢惔蹇庣箽閹镐椒绔撮懛杈剧礉閸氾箑鍨导姘閸濆秴鐤勯梽鍛儗瀵よ桨缍嬫灞烩偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 娑?`docs/design/FILTERS.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋獮鎯八夋鎰秼閸撳秴鍞寸純顕€鐓舵０鎴炴姢闂€?`volume_balance` 娑撳酣鎽肩捄顖濐嚛閺勫簺鈧?
- 娑?`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋弰搴ｂ€橀崗鑸垫Ц娑撴捇顣介崣鍌濃偓鍐偓灞肩瑝閺勵垰鍙忛柌蹇氱箻鎼达箓娼伴弶瑁も偓?
- 娑?`docs/guides/WINDOWS_SETUP.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濇穱顔筋劀 Quill 鐠囧瓨妲戦妴浣瑰閸斻劌鐣ㄧ憗鍛仛娓氬鎷伴崗鍙橀煩鎼存挷濞囬悽銊嚛閺勫函绱濇担澶哥娑撳骸缍嬮崜?`CMakeLists.txt` 鐎靛綊缍堥妴?
- 閺囧瓨鏌?`docs/README.md` 缁便垹绱╅敍灞界殺 `FILTERS.md` 缁惧啿鍙嗛崣顖氬絺閻滄澘鍙嗛崣锝忕礉楠炴儼鎷烽崝鐘虫拱鏉烆喗鏋冨锝嗘殻閻炲棜顔囪ぐ鏇樷偓?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `docs/design/FILTERS.md` 瀹稿弶妲戠涵顔肩秼閸撳秶鏁撻弫鍫㈡畱濠娿倝鏆呮稉濠氭懠娑撳酣顣╅悾娆戠矋娴犳儼绔熼悾灞烩偓?
- `docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 瀹稿弶妲戠涵顔昏礋娑撴捇顣介崣鍌濃偓鍐╂瀮濡楋綇绱濋獮鑸靛瘹閸氭垵缍嬮崜宥嗏偓鏄忕箻鎼达附娼靛┃鎰┾偓?
- `docs/guides/WINDOWS_SETUP.md` 瀹歌弓绗夐崘宥呯紦鐠侇喕濞囬悽銊ょ瑢瑜版挸澧犳禒鎾崇氨娑撳秳绔撮懛瀵告畱 `SDL2_DIR` / `FFMPEG_DIR` 娴肩姴寮粈杞扮伐閵?
- 閺堫剚顐奸弨鐟板З娴犲秹妾虹€规艾婀弬鍥ㄣ€傜仦鍌︾礉閺堫亝绉归崣濠佸敩閻降鈧焦鐎楦垮壖閺堫剙鎷版禒璇插濞撳懎宕熼悩鑸碘偓浣锋叏閺€骞库偓?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閺嶅湱娲拌ぐ?`README.md` / `README_ZH.md` 閻?Windows 閺佸懘娈伴幒鎺楁珟娴犲秵褰佺粈杞板▏閻?`FFMPEG_DIR` 娴肩姴寮敍宀冪箹娑撳骸缍嬮崜?`CMakeLists.txt` 閻ㄥ嫯鍤滈崝銊﹀赴濞?/ `external/ffmpeg` 閸ョ偤鈧偓闁槒绶稉宥勭閼锋番鈧?
- `docs/analysis/video-stream-index-fix.md` 鐠佹澘缍嶉惃鍕Ц閺冣晜婀￠崢鐔风€烽梼鑸殿唽閻ㄥ嫰妫舵０姗堢礉娴ｅ棛宸辩亸鎴斺偓婊冨坊閸欐彃缍婂锝傗偓婵囩垼鐠囧棴绱濋弬鍥﹁厬閻?`playLoop` 娑?`src/video_decoder.cpp` 閺勬捁顫︾拠顖濐嚢娑撹櫣骞囩悰灞界杽閻滆埇鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 閸撳秴鍤戞潪顔煎嚒缂佸繑绔婚悶鍡曠啊 `WINDOWS_SETUP.md` 閻ㄥ嫭鐎鍝勫弳閸欙綇绱濇担鍡樼壌 README 閻ㄥ嫮鐣濋悧鍫熸櫊闂呮粍甯撻梽銈堢箷濞堝鏆€閺冄嗩嚛閺勫簺鈧?
2. `video-stream-index-fix.md` 娴ｆ粈璐熼梻顕€顣介崚鍡樼€介弽閿嬫拱娴犲秶鍔ч張澶夌幆閸婄》绱濋柅鍌氭値娣囨繄鏆€閿涙盯鍣搁悙瑙勬Ц鐠佲晞顕伴懓鍛绾喖鐣犵仦鐐扮艾閺冣晜婀￠梼鑸殿唽閵?
3. 鏉╂瑤琚辩猾濠氭６妫版﹢鍏樼仦鐐扮艾閳ユ粌鍙嗛崣锝嗘瀮濡楋絽鎷伴崢鍡楀蕉瑜版帗銆傛稊瀣？鏉堝湱鏅稉宥嗙閳ユ繐绱濋柅鍌氭値娑撯偓鐠ч攱鏁圭亸淇扁偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 鐏忓棙鐗?README 閻?FFmpeg 閺佸懘娈伴幒鎺楁珟閺€閫涜礋瑜版挸澧?`vcpkg toolchain / external/ffmpeg` 閸欙絽绶為妴?
- 娑?`docs/analysis/video-stream-index-fix.md` 婢х偛濮為悩鑸碘偓浣筋嚛閺勫函绱濋弽鍥唶閸忔湹璐?`2026-02-24` 閺冣晜婀￠崢鐔风€烽梻顕€顣介崚鍡樼€借ぐ鎺撱€傞妴?
- 閸?`docs/README.md` 娑擃厺璐熺拠銉ュ坊閸欏弶鏋冨锝埶夐崗鍛偍瀵洩绱濋獮鎯邦唶瑜版洘婀版潪顔芥纯閺傝埇鈧?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `README.md` / `README_ZH.md` 娑撳秴鍟€瀵ら缚顔呮担璺ㄦ暏娑撳骸缍嬮崜宥勫瘜闁惧彞绗夋稉鈧懛瀵告畱 `FFMPEG_DIR` 娴肩姴寮弬鐟扮础閵?
- `docs/analysis/video-stream-index-fix.md` 瀹稿弶妲戠涵顔昏礋閸樺棗褰惰ぐ鎺撱€傞敍灞肩瑝閸愬秵娈粈?`playLoop` / `video_decoder.cpp` 鐏炵偘绨ぐ鎾冲娴犳挸绨辩紒鎾寸€妴?
- 閺堫剚顐奸弨鐟板З娴犲秹妾虹€规艾婀弬鍥ㄣ€傜仦鍌︾礉閺堫亝绉归崣濠佸敩閻降鈧焦鐎楦垮壖閺堫剙鎷版禒璇插濞撳懎宕熼悩鑸碘偓浣锋叏閺€骞库偓?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閺堫剝鐤嗗鑼病鏉╃偟鐢荤€瑰本鍨氭径姘閺傚洦銆傚〒鍛倞閿涘奔绲剧紒鎾寸亯娴犲秶鍔ч崚鍡樻殠閸︺劌顦挎稉顏呭絹娴溿倕鎷伴弮銉ョ箶閺夛紕娲伴柌灞烩偓?
- 婵″倹鐏夐崥搴ｇ敾闂団偓鐟曚胶鎴风紒顓犳樊閹躲倖鏋冨锝呭經瀵板嫸绱濈紓鍝勭毌娑撯偓娴犺В鈧粈绔撮惇鑲╂箙閹冲倹婀版潪顔界閻炲棜瀵栭崶鏉戞嫲缂佹捁顔戦垾婵堟畱閹槒銆冮妴?


### 閸掑棙鐎界拋鏉跨秿

1. `CHANGELOG.md` 閸?`DEVELOP_LOG.md` 閼冲€燁唶瑜版洝绻冪粙瀣剁礉娴ｅ棗顕崥搴ｇ敾缂佸瓨濮㈤懓鍛降鐠囩繝绗夋径鐔讳粵閸氬牄鈧?
2. `docs/README.md` 閹绘劒绶垫禍鍡欏偍瀵洖鍙嗛崣锝忕礉娴ｅ棙鐥呴張澶夌娴犳垝绗撻梻銊ㄐ掗柌濞锯偓婊勬拱鏉烆喗鏋冨锝呰窗濡偓缂佹捁顔戦垾婵堟畱濮瑰洦鈧粯濮ら崨濞库偓?
3. 婢х偛濮為崡鏇犲閹躲儱鎲￠弬鍥ㄣ€傞敍灞藉讲娴犮儲濡搁垾婊冪秼閸撳秳瀵岄柧鎯х唨缁捐￥鈧礁鍑￠弨鑸垫殐閺傚洦銆傞妴浣风箽閻ｆ瑥宸婚崣鎻掑敶鐎瑰箍鈧礁鎮楃紒顓炵紦鐠侇喒鈧繂娴愮€规矮绗呴弶銉礉閸戝繐鐨柌宥咁槻鐟欙綁鍣撮幋鎰拱閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/analysis/DOC_AUDIT_2026-03-08.md`閿涘奔缍旀稉鐑樻拱鏉烆喗鏋冨锝呰窗濡偓閹槒銆冮妴?
- 閹躲儱鎲℃稉顓㈡肠娑擃叀顔囪ぐ鏇窗瀹糕剝顥呴懠鍐ㄦ纯閵嗕礁鍙ч柨顔跨槤閺傝纭堕妴浣哥秼閸撳秳瀵岄柧鎯х唨缁捐￥鈧礁鍑＄€瑰本鍨氱€靛綊缍堟い骞库偓浣风箽閻ｆ瑤绲鹃崥鍫㈡倞閻ㄥ嫬宸婚崣鎻掑敶鐎瑰箍鈧礁鎮楃紒顓犳樊閹躲倕缂撶拋顔衡偓浣稿彠閼辨梹褰佹禍銈冣偓?
- 閸?`docs/README.md` 娑擃厼鐨㈢拠銉﹀Г閸涘﹦鎾奸崗銉у偍瀵洩绱濋獮鎯版嫹閸旂姵娲块弬鏉垮坊閸欒尪顕╅弰搴涒偓?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `docs/analysis/DOC_AUDIT_2026-03-08.md` 瀹告彃褰查悪顒傜彌鐠囧瓨妲戦張顒冪枂閺傚洦銆傚〒鍛倞瀹搞儰缍旈惃鍕瘱閸ョ繝绗岀紒鎾诡啈閵?
- `docs/README.md` 瀹告彃褰查惄瀛樺复鐎规矮缍呴崚鎷岊嚉瀹糕剝顥呴幎銉ユ啞閵?
- 閺堫剚顐奸弨鐟板З娴犲秹妾虹€规艾婀弬鍥ㄣ€傜仦鍌︾礉閺堫亝绉归崣濠佸敩閻降鈧焦鐎楦垮壖閺堫剙鎷版禒璇插濞撳懎宕熼悩鑸碘偓浣锋叏閺€骞库偓?


### 娣囶喗鏁奸弬鍥︽

- docs/analysis/DOC_AUDIT_2026-03-08.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 闂傤噣顣?50: M4 4.4閿涘牆鎶氬銉ㄧ箻閿?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `4.4` 鐟曚焦鐪伴弨顖涘瘮閺嗗倸浠犻幀渚€鈧劕鎶氶弻銉ф箙閻㈠娼伴妴?
- 瑜版挸澧犳禒鎾崇氨缂傚搫鐨弳鍌氫粻閹礁鎶氬銉ㄧ箻閸忋儱褰涢敍灞炬￥濞夋洖鍎?MPC-HC 娑撯偓閺嶅嘲婀弳鍌氫粻閸氬酣鈧劕鎶氶崜宥呮倵濡偓閺屻儳鏁鹃棃顫偓?


### 閸掑棙鐎界拋鏉跨秿

1. 鏉堟挸鍙嗙仦鍌氱秼閸撳秴褰ч張澶嬫尡閺€淇扁偓涔籩ek閵嗕線鐓堕柌蹇嬧偓浣虹彿閼哄倶鈧竸-B Repeat閵嗕焦鍩呴崶鍓х搼閸斻劋缍旈敍灞剧梾閺堝鎶氬銉ㄧ箻閸斻劋缍旈崪宀勭帛鐠併倝鏁担宥冣偓?
2. `PlayerCore` 閺嗗倸浠犻弮鏈电窗閸愯崵绮ㄧ拫鍐ㄥ閸ｎ煉绱濋崶鐘愁劃娑撳秷鍏橀惄瀛樺复婢跺秶鏁ょ敮姝岊潐濞撳弶鐓嬪顏嗗箚閿涘矂娓剁憰浣风閺夆剝娈忛崑婊勨偓浣烘畱鐎规艾鎮滈崚閿嬫煀鐠侯垰绶為妴?
3. 閸掓繄澧楃€圭偟骞囬懕鏃囩殶閺冭泛褰傞悳甯礉闂婃娊顣跺☉鍫ｅ瀭缁捐法鈻奸崷銊︽畯閸嬫粍鈧椒绮涙导姘辨暏閺?`playback_pts` 閸ョ偛鍟?`position_`閿涘奔绱伴幎濠冾劄鏉╂稓绮ㄩ弸婊嗩洬閻╂牗甯€閿涘矂娓剁憰浣告倱濮濄儲鏁归崣锝冣偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`step_frame_backward` / `step_frame_forward` 閻戭參鏁崝銊ょ稊閿涘矂绮拋銈囩拨鐎?`,` / `.`閵?
- 閸?`Display`閵嗕焦瑕嗛弻鎾虫珤閹恒儱褰涢妴涔layerCore`閵嗕梗VideoPlayer` 娑斿妫块幍鎾烩偓姘姎濮濄儴绻樼拠閿嬬湴娑?API閵?
- 闁插洨鏁ら垾婊勬畯閸嬫粍鈧?seek + 妫ｆ牕鎶氶崚閿嬫煀閳ユ繄娈戦弬鐟扮础鐎圭偟骞囬崜宥呮倵閸楁洖鎶氬銉ㄧ箻閿涘苯鑻熼崷銊︻劄鏉╂稑鎮楃紒瀛樺瘮閺嗗倸浠犻悩鑸碘偓浣碘偓?
- 鐠嬪啯鏆ｉ棅鎶筋暥濞戝牐鍨傜痪璺ㄢ柤閿涙矮绮庨崷?`PlaybackState::Playing` 閺冭埖澧犻悽銊╃叾妫版垶鎸遍弨鍙ョ秴缂冾喖娲栭崘娆庡瘜閺冨爼妫挎潪娣偓?
- 閺傛澘顤?`--frame-step-check .\\juren-30s.mp4` 閼奉亝顥呴崨鎴掓姢閿涘苯鑻熺拋鏉跨秿閺堫剙婀存灞炬暪缂佹挻鐏夐妴?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`

- 姒涙顓婚悜顓㈡暛閺傚洦銆傚鑼端夐崗?`, / .` 閻ㄥ嫭娈忛崑婊勨偓浣告姎濮濄儴绻樼拠瀛樻閵?


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



## ?? 51: M4 4.5???/???????



**??**: 2026-03-08

**??**: ???



### ????

- ???? `4.5` ???????? / ???????

- ?????? `J/K` ? `Ctrl+J/K` ?????????????? MPC-HC ??????????



### ????

1. ??????????????????????????????????????????

2. ??????????????????????????????????????????????????

3. ???????????????????????????????????????????????



### ????

- ?? `SubtitleDelayDown` / `SubtitleDelayUp` ??????? `J / K`??? `Ctrl` ???????????

- ? `PlayerCore` ????/??????? getter/setter?

  - ???? `updateSubtitleOverlay` ???????????????

  - ??????? PTS???????????????????????????

- ?? `AppSettings`?`config/player_settings.ini` ? `--settings-persistence-check`?????????/???

- ?? `--delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt` ???????? README / ???? / ???? / ?????



### ??????

- `build/Debug/modern-video-player.exe --settings-persistence-check`?`PASS`

- `build/Debug/modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt`?`PASS`

- ????????? `J / K` ? `Ctrl+J / Ctrl+K` ????????



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



## ?? 52: M4 4.6????? `1..9` ???



**??**: 2026-03-08

**??**: ???



### ????

- `4.6` ????????? `1..9` ???????

- ????A-B Repeat???????????????????????? MPC-HC ???????????????????????



### ????

1. ??????????????????????????????????? `1..9` ????????????????????

2. `Display` ? `PlayerCore` ?????????????? seek ???????????????? `seek_ratio_` ?????????

3. ???????????????????????? 10% / 90% ???????????????????



### ????

- ?? `SeekTo10Percent` ~ `SeekTo90Percent` ????????? `1..9`?

- ? `Display` ????????????? `0.1` ~ `0.9` ???? seek ???

- ?? `--numeric-seek-check .\juren-30s.mp4` ???????? README / ???? / ???? / ???



### ??????

- `build/Debug/modern-video-player.exe --numeric-seek-check .\juren-30s.mp4`?`PASS`

- ????????? `1..9` ?? `10%..90%` ????



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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `2.2.4` 闂団偓鐟曚椒绔存稉顏嗙埠娑撯偓閻ㄥ嫭鈧嗗厴閺冦儱绻旈崗銉ュ經閿涘瞼鏁ゆ禍搴ゎ潎鐎电喖鐝崚鍡氶哺閻?妤傛鐖滈悳鍥ㄧ壉閺堫剛娈戦幒澶婃姎閵嗕線妲﹂崚妤€鎷扮挧鍕爱閸楃姷鏁ら幆鍛枌閵?
- 瑜版挸澧犻幘顓熸杹閸ｃ劋瀵岄柧鎯у嚒缂佸繐鍙挎径鍥窛婢舵艾鍞撮柈銊х埠鐠佲槄绱濇担鍡楊樆闁劍鐥呴張澶屒旂€规氨娈戠紒鎾寸€崠鏍崣閺€鎯扮翻閸戠尨绱濋弮鐘崇《閻╁瓨甯村▽澶嬬┅娑撶儤婀伴崷鐗堝Г閸涘鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `PlayerCore` 瀹歌尙绮＄紒瀛樺Б娴?demux閵嗕龚ecode閵嗕购ender 缁涘顦跨紒鍕斧鐎涙劘顓搁弫甯礉`Scheduler` 娑旂喐婀侀幒澶婃姎娑撳氦袙閻胶绮虹拋鈽呯礉娴ｅ棛宸辩亸鎴犵埠娑撯偓韫囶偆鍙庨幒銉ュ經閵?
2. 閻滅増婀侀崨鎴掓姢鐞涘矁鍤滃Λ鈧弴鏉戜焊閸氭垵濮涢懗浠嬬崣鐠囦緤绱濇稉宥夆偓鍌氭値閸?`1080p60 / 4K / 妤傛鐖滈悳鍢?閺嶉攱婀伴惃鍕偓褑鍏樼€佃鐦悾娆愩€傞妴?
3. GPU 閻╁瓨甯撮崡鐘垫暏閻滃洭鍣伴弽鐤硶楠炲啿褰撮幋鎰拱鏉堝啴鐝敍灞芥礈濮濄倖婀版潪顔煎帥鏉堟挸鍤ぐ鎾冲濠碘偓濞茶崵娈戠憴锝囩垳 backend / 濞撳弶鐓?backend閿涘奔缍旀稉?GPU 鐠侯垰绶炵憴鍌涚ゴ娣団剝浼呴妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`core::DiagnosticsSnapshot`閿涘苯婀?`PlayerCore` 娑擃厽鏁归弫?demux / decode / render / scheduler / queue 閹稿洦鐖ｉ妴?
- 閸?`VideoPlayer` 娑擃厼顤冮崝?`getInfo()` / `getDiagnosticsSnapshot()` 闁繋绱堕幒銉ュ經閵?
- 閸?`main` 娑擃厽鏌婃晶?`--performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200` 閼奉亝顥呴崨鎴掓姢閿涘矁绶崙?CPU 楠炲啿娼庨崡鐘垫暏閵嗕線鈧槒绶弽绋跨妇閺佽埇鈧攻ackend閵嗕焦甯€鐢傜瑢闂冪喎鍨幐鍥ㄧ垼閵?
- 閺傛澘顤?`docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`閿涘苯鑻熼崥灞绢劄娴犺濮熷〒鍛礋閵嗕礁妯婄捄婵婄槑娴艰埇鈧胶澧楅張顒佹瀮濡楋絼绗岄崣妯绘纯鐠佹澘缍嶉妴?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`閿涙瓪PASS`

- 瑜版挸澧犻弽閿嬫拱鏉堟挸鍤崠鍛儓 `renderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗cpu_avg_percent閳?00%`閵嗕梗scheduler_late_drops=0` 缁涘鍙ч柨顔藉瘹閺嶅洢鈧?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 闂団偓鐟曚椒璐?`1080p60` 缁嬪啿鐣鹃幘顓熸杹瀵よ櫣鐝涢弰搴ｂ€橀惃鍕拱閸︿即鐛欓弨璺哄弳閸欙綇绱濋柆鍨帳閸欘亜鍤熼幀褑鍏橀弮銉ョ箶閹存牔姹夊銉潎鐎电喎鍨介弬顓溾偓?
- 瑜版挸澧犻弽閿嬫拱閸栧懏鐥呴張澶嬫▔瀵繒娈?`1080p60` 缁嬪啿鐣鹃幀褎鐗遍張顒傛晸閹存劘顕╅弰搴礉婢跺秶骞囩€圭偤鐛欑捄顖氱窞娑撳秴顧勫〒鍛珰閵?


### 閸掑棙鐎界拋鏉跨秿

1. `collectFileProbeReport()` 瀹歌尙绮￠懗鐣岀舶閸戝搫顔旀妯糕偓涓桺S閵嗕焦甯归懡?backend 缁涘鍘撴穱鈩冧紖閿涘矂鈧倸鎮庨崑姘鼻旂€规碍鈧囩崣閺€璺哄缂冾喗娼禒韬测偓?
2. `DiagnosticsSnapshot` 瀹告彃褰查幓鎰返 `scheduler_late_drops` / `demux_dropped_packets` 缁涘鍙ч柨顔厩旂€规碍鈧勫瘹閺嶅洢鈧?
3. 鏉╂﹢娓剁憰浣剿夋稉鈧稉顏佲偓婊嗙箾缂侇厽鎸遍弨鍓х崶閸欙絽鍞撮弮鍫曟？閹恒劏绻橀垾婵堟畱閸掋倖鏌囬敍灞惧閼宠姤濡?`1080p60` 妤犲本鏁规禒搴ゎ潎濞村妫╄箛妤佸絹閸楀洣璐熼弰搴ｂ€橀梻銊ь洣閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`閿涘本顥呴弻?`probe`閵嗕焦妞傞梻瀛樺腹鏉╂稏鈧勾ate drop 娑?demux drop閵?
- 閸?`tools/download_test_samples.ps1` 婢х偛濮?`1080p60 AAC 2ch` 閺嶉攱婀伴悽鐔稿灇鐠侯垰绶為敍灞借嫙閸?`samples/README.md` 閺嶅洩顔囬崗鍓佹暏娴?`2.2.1 / 2.3.2`閵?
- 閺傛澘顤?`docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`閿涘苯鎮撳銉ゆ崲閸斺剝绔婚崡鏇樷偓浣告▕鐠烘繆鐦庢导棰佺瑢閻楀牊婀扮拋鏉跨秿閵?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`閿涙瓪PASS`

- 瑜版挸澧犻弽閿嬫拱鏉堟挸鍤崠鍛儓 `advance_ratio=0.994133`閵嗕梗late_drops=0`閵嗕梗demux_dropped_packets=0`閵嗕梗decoder_backend=D3D11VA`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 闂団偓鐟曚焦濡?`4K` 閺嶉攱婀伴幘顓熸杹娑撳簶鈧粌銇戠拹銉︽閸欘垶妾风痪褉鈧繃鏁归弫娑欏灇娑撯偓娑擃亞绮烘稉鈧惃鍕拱閸︿即鐛欓弨璺哄弳閸欙絻鈧?
- 閻滅増婀侀懗钘夊閸掑棙鏆庨崷銊︹偓褑鍏橀弮銉ョ箶娑?Windows 閸氬海顏崶鐐衡偓鈧Λ鈧弻銉よ厬閿涘奔绗夐崚鈺€绨惄瀛樺复鐎电懓绨叉禒璇插濞撳懎宕?`2.2.2 / 2.3.3`閵?


### 閸掑棙鐎界拋鏉跨秿

1. `collectFileProbeReport()` 瀹歌尪鍏樼涵顔款吇閺嶉攱婀伴弰顖氭儊娑?`3840x2160`閿涘矂鈧倸鎮庨崑?4K 闂傘劎顩﹂崜宥囩枂閸掋倖鏌囬妴?
2. `runBackendSessionSubprocess()` 瀹歌尪鍏樼粙鍐茬暰妤犲矁鐦?hard / soft 娑撱倓閲滈崥搴ｎ伂濡€崇础閿涘本妫ら棁鈧柌宥嗘煀鐎圭偟骞囬梽宥囬獓濞翠胶鈻奸妴?
3. 鏉╂﹢娓剁憰浣风娑擃亙瀵屾潻娑氣柤鏉╃偟鐢婚幘顓熸杹缁愭褰涢敍宀€鈥樻穱?`4K` 閺嶉攱婀版稉宥嗘Ц閳ユ粌褰ч幍鎾崇磻娑撳秵甯规潻娑掆偓婵撶礉閼板本妲搁惇鐔割劀鏉╂稑鍙嗛幘顓熸杹閻樿埖鈧降鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`閵?
- 娑撴槒绻樼粙瀣梾閺?`probe`閵嗕焦妞傞梻瀛樺腹鏉╂稏鈧梗late_drop` 娑撳骸缍嬮崜?backend閿涙稑鐡欐潻娑氣柤濡偓閺?hard / soft 濡€崇础闁€熷厴鏉╂稑鍙嗛幘顓熸杹閵?
- 閺傛澘顤?`docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`閿涘苯鎮撳銉ゆ崲閸斺剝绔婚崡鏇樷偓浣告▕鐠烘繆鐦庢导棰佺瑢閻楀牊婀扮拋鏉跨秿閵?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`閿涙瓪PASS`

- 瑜版挸澧犻弽閿嬫拱鏉堟挸鍤崠鍛儓 `advance_ratio=0.968167`閵嗕梗late_drops=0`閵嗕梗hard.decoder_backend=D3D11VA`閵嗕梗soft.decoder_backend=Software`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 闂団偓鐟曚椒绔存稉顏嗘埂濮濓綀绉存潻?`80Mbps` 閻ㄥ嫬娲栬ぐ鎺撶壉閺堫剙鎷扮€电懓绨叉灞炬暪閸忋儱褰涢敍灞惧閼宠棄鐣幋?`2.2.3`閵?
- 瑜版挸澧犻弽閿嬫拱闂嗗棗鎮庨搹鐣屽姧鐟曞棛娲婇崚鍡氶哺閻滃洣绗岄弽鐓庣础閿涘奔绲鹃惍浣哄芳閺咁噣浜堕崑蹇庣秵閿涘本妫ゅ▔鏇氱稊娑撴椽鐝惍浣哄芳闂傘劎顩﹂妴?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀?`collectFileProbeReport()` 濞屸剝婀侀惄瀛樺复鏉堟挸鍤惍浣哄芳閿涘奔绲?FFmpeg 閻?`AVFormatContext::bit_rate` 閸欘垳娲块幒銉ヮ槻閻劊鈧?
2. 妤傛鐖滈悳鍥︽崲閸旓紕娈戦弽绋跨妇娑撳秴婀崚鍡氶哺閻滃浄绱濋懓灞芥躬娴滃孩鐗遍張顒傛埂鐎圭偟鐖滈悳鍥モ偓浣界箾缂侇厽鎸遍弨鍓х崶閸欙絾甯规潻娑樻嫲閹哄鎶?娑撱垹瀵橀幆鍛枌閵?
3. 閸╄桨绨ぐ鎾冲 `D3D11VA + D3D11` 娑撳鎽奸敍瀹?00Mbps` 缁狙冨焼閻?`1080p60` H.264 閺嶉攱婀板鑼跺厴缁嬪啿鐣炬潻娑樺弳閹绢厽鏂侀柧鎹愮熅閿涘矂鈧倸鎮庢担婊€璐熼張顒€婀撮崶鐐茬秺閸╄櫣鍤庨妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`collectFormatBitrateBitsPerSecond()` 娑?`--high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`閵?
- 閸?`tools/download_test_samples.ps1` 婢х偛濮?`100Mbps` 閺嶉攱婀伴悽鐔稿灇鐠侯垰绶為敍灞借嫙閸?`samples/README.md` 閺嶅洦鏁為悽銊┾偓鏂烩偓?
- 閺傛澘顤?`docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`閿涘苯鎮撳銉ゆ崲閸斺剝绔婚崡鏇樷偓浣告▕鐠烘繆鐦庢导棰佺瑢閻楀牊婀扮拋鏉跨秿閵?


### 閺堫剙婀撮弽鈥愁嚠缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`閿涙瓪PASS`

- 瑜版挸澧犻弽閿嬫拱鏉堟挸鍤崠鍛儓 `format_bitrate_bps=102829290`閵嗕梗advance_ratio=0.988444`閵嗕梗late_drops=0`閵嗕梗demux_dropped_packets=0`閵?


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



## 闂傤噣顣?57: 閸欐垵绔烽梻銊ь洣 6.5閿涘牓鏆遍弮鑸垫尡閺€鍓旂€规碍鈧嶇礆



**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `6.5` 闂団偓鐟曚椒绔存稉顏勫讲闁插秴顦查幍褑顢戦惃鍕拱閸︽澘鍙嗛崣锝忕礉妤犲矁鐦夐梹鎸庢閹绢厽鏂佺粣妤€褰涢崘鍛￥ crash 娑撴柧绮涢懗鑺ュ瘮缂侇厽甯规潻娑栤偓?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀?`1080p60` / `4K` / `>80Mbps` 妤犲本鏁归崨鎴掓姢闁垝浜掗惌顓犵崶閸欙絼璐熸稉浼欑礉娑撳秷鍏橀惄瀛樺复娴ｆ粈璐熼垾婊堟毐閺冭埖鎸遍弨鐐￥ crash閳ユ繄娈戠拠浣规閵?
2. `VideoPlayer` 瀹稿弶褰佹笟?`play()` / `isPlaying()` / `getCurrentTime()` / `getDiagnosticsSnapshot()`閿涘苯褰叉禒銉ヮ槻閻劋璐?smoke 妤犲本鏁归梻顓犲箚閵?
3. 閸欘亣顩﹂崥灞炬妤犲矁鐦夐幘顓熸杹閹椒绻氶幐浣碘偓浣规闂傚瓨甯规潻娑楃瑢 `late_drop` / demux drop閿涘苯宓嗛崣顖氳埌閹存劒绔存稉顏囧喕婢剁喕浜ら柌蹇曟畱閸欐垵绔烽梻銊ь洣閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`--long-playback-check .\juren-30s.mp4 10000`閿涘矁顩﹀Ч鍌炲櫚閺嶉鐛ラ崣锝勭瑝鐏忔垳绨?`5000ms`閵?
- 閸涙垝鎶ゆ潏鎾冲毉 `probe_duration`閵嗕梗renderer_backend`閵嗕梗decoder_backend`閵嗕梗advance_ratio`閵嗕梗late_drops`閵嗕梗demux_dropped_packets` 缁涘绮ㄩ弸鍕鐎涙顔岄妴?
- 閺傛澘顤?`docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`閿涘苯鑻熼崥灞绢劄娴犺濮熷〒鍛礋閵嗕礁妯婄捄婵婄槑娴艰埇鈧胶澧楅張顒冾唶瑜版洑绗岄崣妯绘纯鐠佹澘缍嶉妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`閿涙瓪PASS`

- 瑜版挸澧犳潏鎾冲毉閸栧懎鎯?`probe_duration=30.03`閵嗕梗entered_playback_loop=true`閵嗕梗still_playing_after_window=true`閵嗕梗advance_ratio=0.996267`閵嗕梗late_drops=0`閵嗕梗demux_dropped_packets=0`閵嗕梗renderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `7.1` 闂団偓鐟曚椒绔存稉顏嗘埂濮濓絽褰叉潻鎰攽閻ㄥ嫭褰冩禒璺侯問娑撲紮绱濋懓灞肩瑝閸欘亝妲搁崘鍛摠娑擃厾娈戦崗鍐╂殶閹诡喖宕版担宥冣偓?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀?`PluginManager` 閸欘亣鍏橀崑姘舵饯閹焦寮挎潻鎵儊濞夈劌鍞芥稉搴℃儙閸嬫粍鐖ｇ拋甯礉閺冪姵纭剁憰鍡欐磰 `DLL` 閸斻劍鈧礁濮炴潪濮愨偓浣哄閺堫剙鍚嬬€圭懓鎷伴崡姝屾祰濞撳懐鎮婇崷鐑樻珯閵?
2. 娴狅絿鐖滄惔鎾冲嚒缂佸繑婀?`FilterRegistry` 鏉╂瑧琚径鈺冨姧閹碘晛鐫嶉悙鐧哥礉闁倸鎮庢担婊€璐熸＃鏍﹂嚋閹绘帊娆㈤梻顓犲箚閻ㄥ嫬顔栨稉鏄忓厴閸旀稏鈧?
3. 婵″倹鐏夊▽鈩冩箒缁€杞扮伐 `DLL` 閸滃苯鎳℃禒銈堫攽妤犲本鏁归崗銉ュ經閿涘苯姘ㄩ弮鐘崇《鐠囦焦妲戦幓鎺嶆缁崵绮哄韫矤閳ユ粓顎囬弸鍨涒偓婵婄箻閸忋儮鈧粌褰叉潻鎰攽閳ユ縿鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`include/plugin/plugin_api.h`閿涘苯鐣炬稊澶嬪絻娴犺泛顔栨稉缁樺复閸欙絼绗岀€电厧鍤粭锕€褰块妴?
- 闁插秴鍟?`PluginManager`閿涘本鏁幐浣瑰瘻鐠侯垰绶為崝鐘烘祰閹绘帊娆㈤妴浣圭墡妤?`API` 閻楀牊婀伴妴浣瑰⒔鐞?`initialize/shutdown` 閻㈢喎鎳￠崨銊︽埂閿涘苯鑻熺捄鐔婚嚋閹绘帊娆㈠▔銊ュ斀閻ㄥ嫭鎶ら梹婊冧紣閸樺倻鏁ゆ禍搴″祻鏉炶姤绔婚悶鍡愨偓?
- 閺傛澘顤?`sample_logger_plugin` 缁€杞扮伐閹绘帊娆㈡稉?`--plugin-check` 妤犲本鏁归崨鎴掓姢閿涙稒褰冩禒鏈电窗濞夈劌鍞?`sample_identity` 鐟欏棝顣跺銈夋殔閿涘奔绶电€瑰じ瀵屾宀冪槈濞夈劌鍞?閸楁瓕娴囬梻顓犲箚閵?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --plugin-check`閿涙瓪PASS`

- 瑜版挸澧犳潏鎾冲毉閸栧懎鎯?`loaded_count=1`閵嗕梗plugin_ids=sample_logger_plugin@0.1.0`閵嗕梗sample_video_filter_registered=true`閵嗕梗sample_video_filter_unloaded=true`閵嗕梗errors=none`閵?


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

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `7.2` 闂団偓鐟曚椒绔存稉顏嗘埂濮濓絽褰茬捄鎴犳畱 HTTP 閸掑棛澧栨稉瀣祰娑撳海绱﹂崘鎻掑弳閸欙綇绱濋懓灞肩瑝閺勵垰浠犻悾娆忔躬濞撳懎宕熺憴锝嗙€芥銊︾仸閵?


### 閸掑棙鐎界拋鏉跨秿

1. `HttpStreamDownloader` 娑斿澧犲▽鈩冩箒閻喎鐤勭拠缁樼ウ鐎圭偟骞囬敍瀹峳eadChunk()` 濮樻瓕绻欐潻鏂挎礀缁岀儤鏆熺紒鍕┾偓?
2. 瑜版挸澧犳禒锝囩垳瀹歌尙绮￠崗宄邦槵 HLS 濞撳懎宕熺憴锝嗙€介崳顭掔礉闁倸鎮庢禒?HLS 婵帊缍嬪〒鍛礋娴ｆ粈璐熸＃鏍﹂嚋濞翠礁鐛熸担?smoke 閸忋儱褰涢妴?
3. 閸︺劌褰堥梽鎰秹缂佹粎骞嗘晶鍐х瑓閿涘本娓剁粙鍐茬暰閻ㄥ嫰鐛欓弨鑸垫煙瀵繑妲搁崷銊︽拱閺堥缚鎹ｆ稉鈧稉顏勭毈閸?HTTP 婢剁懓鍙块張宥呭閿涘矂浼╅崗宥勭贩鐠ф牕顦婚柈銊х彲閻愬箍鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 闁插秴鍟?`HttpStreamDownloader`閿涘苯鐔€娴?FFmpeg `avio` 閺€顖涘瘮閻喎鐤?HTTP 閹垫挸绱戦妴浣稿瀻閸ф顕伴崣鏍モ偓浣稿敶闁劎绱﹂崘灞傗偓涓扥F 閻樿埖鈧椒绗岄柨娆掝嚖闁繋绱堕妴?
- 閺傛澘顤?`--streaming-buffer-check`閿涘奔绗呮潪?HLS 濞撳懎宕熼妴浣叫掗弸鎰祲鐎?URL閵嗕焦濮勯崣鏍у 3 娑擃亜鍨庨悧鍥ц嫙妤犲矁鐦夌紓鎾冲暱鐎涙濡弫鑸偓?
- 閺傛澘顤?`tools/start_streaming_fixture_server.ps1` 娑?`samples/streaming/hls_local/*` 婢剁懓鍙块敍宀€鏁ゆ禍搴㈡拱閺?HTTP 閸ョ偛缍婇妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`閿涙瓪PASS`

- 瑜版挸澧犳潏鎾冲毉閸栧懎鎯?`manifest_download_ok=true`閵嗕梗manifest_parse_ok=true`閵嗕梗segments_downloaded=3`閵嗕梗buffered_bytes=621`閵嗕梗buffer_ok=true`閵嗕梗error=none`閵?


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





## 闂傤噣顣?60: 7.3 HLS/DASH 閼奉亪鈧倸绨查惍浣哄芳



**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `7.3` 闂団偓鐟曚椒绔存稉顏勫讲闁插秴顦查幍褑顢戦惃?HLS/DASH 婢舵氨鐖滈悳鍥掗弸鎰瑢閼奉亪鈧倸绨查惍浣哄芳閺堫剙婀存灞炬暪閸忋儱褰涢妴?


### 閸掑棙鐎界拋鏉跨秿

1. 閻滅増婀?`HlsManifestParser` 閸欘亣鍏樼拠璇插絿婵帊缍嬮幘顓熸杹閸掓銆冮敍灞炬￥濞夋洖顦╅悶?`master playlist`閵?
2. 閻滅増婀?`DashManifestParser` 閸欘亞鐓￠柆?`Representation` 鐢箑顔旈敍灞炬￥濞夋洘瀣侀崚鏉垮灥婵瀵查崚鍡欏閸滃苯鐛熸担鎾冲瀻閻?URL閵?
3. 閺堚偓缁嬪啿螘閻ㄥ嫰鐛欓弨鑸垫煙瀵繋绮涢弰顖氭躬閺堫剚婧€ HTTP 婢剁懓鍙挎稉瀣剁礉閸掑棗鍩嗘宀冪槈 HLS/DASH 閻ㄥ嫭銆傛担宥夆偓澶嬪娑撳骸宕岄梽宥嗐€傜捄顖氱窞閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閹碘晛鐫?HLS/DASH 鐟欙絾鐎介崳顭掔礉鐞涖儵缍?variant / representation / `BaseURL` / 閸掓繂顫愰崠鏍у瀻閻?/ 婵帊缍嬮崚鍡欏閺勫海绮忛妴?
- 閺傛澘顤?`AdaptiveBitrateSelector`閿涘苯鑻熼崷?`main` 娑擃厽褰佹笟?`--adaptive-bitrate-check` 閸涙垝鎶ら妴?
- 閺傛澘顤?`samples/streaming/abr_local/{hls,dash}` 閺嶉攱婀版稉搴㈠Г閸涘绱濇宀冪槈 HLS/DASH 閸︺劎绮扮€规艾鐢€硅棄绨崚妞剧瑓閻ㄥ嫬宕岄惍浣哄芳娑撳酣妾烽惍浣哄芳閸掑洦宕查妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`閿涙瓪PASS`

- 瑜版挸澧犳潏鎾冲毉閸栧懎鎯?`switch_count=2`閵嗕梗upswitch_count=1`閵嗕梗downswitch_count=1`閿涘矁顕╅弰?HLS/DASH 娑撱倖娼捄顖氱窞闁棄鐣幋鎰啊娑撯偓濞嗏€冲磳濡楋絽鎷版稉鈧▎锟犳濡楋絻鈧?


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

## 闂傤噣顣?61: 瀵よ櫣鐝涢柌宀€鈻肩喊鎴炵垼缁涙拝绱檝0.2.0-rc1 / v0.2.0閿?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 娴犺濮熷〒鍛礋 `0.4` 鐏忔碍婀€瑰本鍨氶敍灞肩波鎼存挾宸辩亸?`v0.2.0-rc1` 娑?`v0.2.0` 閻ㄥ嫰鍣风粙瀣暥閺嶅洨顒烽妴?


### 閸掑棙鐎界拋鏉跨秿

1. 瑜版挸澧犳禒鎾崇氨 `git tag --list` 娑撹櫣鈹栭敍宀冾嚛閺勫氦绻曞▽鈩冩箒娴犺缍嶅锝呯础闁插瞼鈻肩喊鎴炵垼鐠佽埇鈧?
2. `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 娴犲秴鍟撻惈鈧垾婊冪秼閸撳秳绮庨崜鈺冨閺堫剚鐖ｇ粵鐐惙娴ｆ粍婀幍褑顢戦垾婵撶礉闂団偓鐟曚礁婀弽鍥╊劮瀵よ櫣鐝涢崥搴ｇ彌閸楀啿娲栭崘娆嶁偓?
3. 闁插瞼鈻肩喊鎴炵垼缁涙儳鐫樻禍搴濈波鎼存挾濮搁幀浣烘畱娑撯偓闁劌鍨庨敍宀勬珟娴滃棗鍨卞鐑樼垼缁涚偓婀伴煬顐礉鏉╂﹢娓剁憰浣告倱濮濄儰鎹㈤崝锛勫Ц閹椒绗岄悧鍫熸拱鐠佹澘缍嶉妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 瀵よ櫣鐝?`v0.2.0-rc1` 娑?`v0.2.0` 娑撱倓閲滈柌宀€鈻肩喊鎴炵垼缁涗勘鈧?
- 閸氬本顒為弴瀛樻煀閻楀牊婀扮拋鏉跨秿閵嗕礁褰夐弴纾嬵唶瑜版洏鈧礁绱戦崣鎴炴）韫囨ぜ鈧礁妯婄捄婵婄槑娴兼澘鎷版禒璇插濞撳懎宕熼妴?
- 閸╄桨绨?`v0.2.0-rc1` 瀹告彃褰茬粙鍐茬暰瀵よ櫣鐝涢敍灞惧⒔鐞涘瞼瀹抽弶?`5.3 濮ｅ繋閲滈柌宀€鈻肩喊鎴犵波閺夌喎澧犺箛鍛淬€忛崣顖涘ⅵ RC 閺嶅洨顒穈 閸欘垰鍨界€规矮璐熷陇鍐婚妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `git tag --list`閿涙艾鍨卞鍝勫娑撹櫣鈹栭敍灞藉灡瀵ゅ搫鎮楅崠鍛儓 `v0.2.0-rc1` 娑?`v0.2.0`閵?
- 娴犺濮熷〒鍛礋 `5.3` 瀹告彃褰查梾?`0.4` 娑撯偓楠炶泛瀣€闁鈧?
- 閺嶅洨顒峰铏圭彌閸氬骸褰查柅姘崇箖 `git show <tag> --no-patch --stat` 妤犲矁鐦夐幐鍥ф倻閹绘劒姘﹂妴?


### 娣囶喗鏁奸弬鍥︽

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 闂傤噣顣?62: 閹笛嗩攽鐎瑰牆鍨崣锝呯窞閸氬本顒為敍?.1 / 5.2閿?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 鐎瑰牆鍨い?`5.1 / 5.2` 闂団偓鐟曚焦鐗撮幑顔肩秼閸撳秳绮ㄦ惔鎾茬瑢娴溿倓绮懞鍌氼殧閸嬫矮绔村▎鈥冲經瀵板嫬鎮撳銉礉闁灝鍘ゆ禒璇插濞撳懎宕熸稉搴＄杽闂勫懏澧界悰宀€濮搁幀浣藉姎閼哄倶鈧?


### 閸掑棙鐎界拋鏉跨秿

1. 閺堫剝鐤嗘稉鏄忣洣娴犺濮熼幐澶嗏偓婊冨絺鐢啴妫粋?閳?閹绘帊娆㈢化鑽ょ埠 閳?濞翠礁鐛熸担鎾剁处閸?閳?ABR 閳?闁插瞼鈻肩喊鎴炵垼缁?閳?鐎瑰牆鍨崣锝呯窞閳ユ繄娈戞い鍝勭碍娑撹尪顢戦幒銊ㄧ箻閿涘本鐥呴張澶婂毉閻滄媽绉存潻?2 娑擃亙瀵屾禒璇插楠炴儼顢戦幒銊ㄧ箻閻ㄥ嫯鐦夐幑顔衡偓?
2. `5.2` 閻ㄥ嫬鍨介弬顓熸蒋娴犺埖妲搁垾婊勭槨閸涖劋绨查崣顏勪粵閺€鑸垫殐閳ユ繐绱濇潻娆掝洣濮瑰倹瀵旂紒顓熲偓褏娈戦崨銊ㄥΝ婵傚繗顔囪ぐ鏇礉閼板奔绗夋禒鍛Ц娑撯偓鏉烆喗鏁归崣锝囩波閺嬫嚎鈧?
3. 閸ョ姵顒?`5.1` 閸欘垯浜掗崟楣冣偓澶涚礉`5.2` 娴犲秴绨叉穱婵囧瘮瀵板懎鐣幋鎰┾偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸曢箖鈧?`5.1 WIP 闂勬劕鍩楅敍姘倱閺冩儼绻樼悰灞兼崲閸斺€茬瑝鐡掑懓绻?2 娑撶寯閵?
- 娣囨繄鏆€ `5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 娑撳搫绶熺€瑰本鍨氶敍灞借嫙閸︺劎澧楅張?閸欐ɑ娲跨拋鏉跨秿娑擃厼鍟撻弰搴″斧閸ョ姰鈧?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 娴犺濮熷〒鍛礋瀹稿弶娲块弬棰佽礋閿涙瓪5.1` 鐎瑰本鍨氶妴涔?.2` 瀵板懎鐣幋鎰┾偓涔?.3` 鐎瑰本鍨氶妴?
- 瑜版挸澧犳禒鎾崇氨濞屸剝婀侀弬鏉款杻娴狅絿鐖滈弨鐟板З閿涘本婀板▎鈥冲涧閸氬本顒炴潻鍥┾柤缁撅附娼崣锝呯窞閵?


### 娣囶喗鏁奸弬鍥︽

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 闂傤噣顣?63: 閽€钘夋勾 5.2 閸涖劋绨查弨鑸垫殐閺冦儲澧界悰灞惧閸?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾婢跺嫮鎮婄€瑰牆鍨い鐟板經瀵板嫸绱濋獮鑸靛Ω `5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 閽€鑺ュ灇娑撯偓娴犺棄褰查幍褑顢戦惃鍕ウ缁嬪瀹抽弶鐔告瀮濡楋絼绗岄懞鍌氼殧鐠囧瓨妲戦妴?


### 閸掑棙鐎界拋鏉跨秿

1. `5.2` 瑜版挸澧犳禒宥勭瑝鎼存梻娲块幒銉ュ瑎闁绱濋崶鐘辫礋鐎瑰啳顩﹀Ч鍌滄畱閺勵垵娉曢崨銊ｂ偓渚€鍣告径宥呭絺閻㈢喓娈戞潻鍥┾柤鐠囦焦宓侀妴?
2. 娴ｅ棗婀張顏勫瑎闁绠ｉ崜宥忕礉娴犲秶鍔ч崣顖欎簰閸忓牊濡搁垾婊冾洤娴ｆ洘澧界悰灞芥噯娴滄梹鏁归弫娑欐）閳ユ繃鏋冨锝呭閿涘矂妾锋担搴℃倵缂侇厽澧界悰灞藉經瀵板嫭绱撶粔姹団偓?
3. 閺堚偓閸氬牏鎮婇惃鍕儰閸︾増鏌熷蹇ョ礉閺勵垱鏌婃晶鐐扮娴犵晫琚导鍏兼惙娴ｆ粍澧滈崘宀€娈戦弬鍥ㄣ€傞敍灞惧Ω閸涖劏濡總蹇嬧偓浣稿帒鐠侀晲绨ㄦい骞库偓浣侯洣濮濐澀绨ㄦい骞库偓浣瑰⒔鐞涘矂銆庢惔蹇嬧偓浣界翻閸戣櫣澧挎稉搴″瑎闁绶烽幑顔煎弿闁劌鍟撳〒鍛殶閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`閿涘苯鐨?`5.2` 閸ュ搫瀵叉稉鍝勫讲閹笛嗩攽濞翠胶鈻奸妴?
- 閸?`docs/README.md` 婢х偛濮為崗銉ュ經閿涘苯鑻熼崥灞绢劄閻楀牊婀扮拋鏉跨秿娑撳骸褰夐弴纾嬵唶瑜版洏鈧?
- 娣囨繃瀵旀禒璇插濞撳懎宕?`5.2` 閺堫亜瀣€闁绱濆鍛倵缂侇叀娉曢崨銊﹀⒔鐞涘苯鑸伴幋鎰槈閹诡喖鎮楅崘宥嗘纯閺傛壆濮搁幀浣碘偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閺傜増鏋冨锝呭嚒鐟曞棛娲婇崨銊ょ閸掓澘鎳嗘禍鏃傛畱閼哄倸顨旈崚鍡椾紣閵?
- 閺傜増鏋冨锝呭嚒閸掓鍤弨鑸垫殐閺冦儱鍘戠拋闀愮皑妞ゅ箍鈧胶顩﹀顫皑妞ゅ箍鈧焦娓堕惌顓熷⒔鐞涘矁鐭惧鍕瑢鏉堟挸鍤悧鈹库偓?
- 瑜版挸澧犳禒鎾崇氨濞屸剝婀侀弬鏉款杻娴狅絿鐖滈弨鐟板З閿涘本婀板▎鈥冲涧閺傛澘顤冨ù浣衡柤閺傚洦銆傞獮璺烘倱濮濄儲鏋冨锝囧偍瀵洑绗岀拋鏉跨秿閵?


### 娣囶喗鏁奸弬鍥︽

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂傤噣顣?64: 鐞涖儵缍?5.2 閻ｆ瑧妫斿Ο鈩冩緲閿涘潐aily_board / 閸涖劍濮ら敍?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閹?`5.2` 閻ㄥ嫧鈧粎鏆€閻ユ洘膩閺夊簱鈧繆藟閸?`daily_board / 閸涖劍濮 闁插矉绱濇担鍨噯娴滄梹鏁归弫娑氭畱閹笛嗩攽鐠囦焦宓侀張澶婃祼鐎规俺娴囨担鎾扁偓?


### 閸掑棙鐎界拋鏉跨秿

1. `5.2` 閻滄澘婀鑼病閺堝甯崚娆忔嫲濞翠胶鈻奸敍灞肩稻鏉╂ɑ鐥呴張澶屾埂濮濓綁娼伴崥鎴炲⒔鐞涘瞼娈戞繅顐㈠晸濡剝婢橀妴?
2. 瑜版挻妫╅惇瀣緲閸滃本鐦￠崨銊︾湽閹鐫樻禍搴濊⒈娑擃亙绗夐崥宀€鐭戞惔锔肩窗閸撳秷鈧懓顔囪ぐ鏇椻偓婊冩噯娴滄柨缍嬫径鈺佷粵娴滃棔绮堟稊鍫氣偓婵撶礉閸氬氦鈧懓顔囪ぐ鏇椻偓婊嗙箹娑撯偓閸涖劍妲搁崥锔藉姬鐡?5.2閳ユ縿鈧?
3. 閸ョ姵顒濋棁鈧憰浣告倱閺冩儼藟姒绘劘绻栨稉銈囶潚濡剝婢橀敍灞惧閼宠棄婀崥搴ｇ敾鐠恒劌鎳嗙拠浣瑰祦缁鳖垳袧閺冩湹绻氶幐浣稿經瀵板嫪绔撮懛娣偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 娑擃叏绱濇稉?Day 5 / Day 10 婢х偛濮為弨鑸垫殐閺冦儴顔囪ぐ鏇炲幢閵?
- 閺傛澘顤?`.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`閿涘奔缍旀稉鐑樼槨閸涖劌鎳嗛幎銉ょ瑢閺€鑸垫殐閻ｆ瑧妫斿Ο鈩冩緲閵?
- 閸氬本顒?`tasklist / WEEKLY_CONVERGENCE_PLAYBOOK / README / 閻楀牊婀扮拋鏉跨秿`閿涘本濡搁崗銉ュ經閸滃瞼鏁ら柅鏂垮晸濞撳懏顨熼妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閻滄澘婀В蹇庨嚋閸涖劋绨查柈钘夊讲娴犮儱婀?`daily_board.md` 閻╁瓨甯存繅顐㈠晸閼煎啫娲块崘鑽ょ波閵嗕礁娲栬ぐ鎺戞嚒娴犮們鈧攻locker 缂佹捁顔戦崪宀勬▉濞堢數绮ㄧ拋鎭掆偓?
- 閻滄澘婀崣顖欎簰閸╄桨绨?`weekly_report_template.md` 婢跺秴鍩楅悽鐔稿灇濮ｅ繐鎳嗛崨銊﹀Г鐎圭偘绶ラ敍宀€鏁ゆ禍搴″灲閺傤厽妲搁崥锔藉姬鐡?`5.2`閵?
- 瑜版挸澧犳禒鎾崇氨濞屸剝婀侀弬鏉款杻娴狅絿鐖滈弨鐟板З閿涘本婀板▎鈥冲涧鐞涖儵缍堝Ο鈩冩緲娑撳孩鏋冨锝呭弳閸欙絻鈧?


### 娣囶喗鏁奸弬鍥︽

- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md

- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂傤噣顣?65: 濮瑰洦鈧缍嬮崜宥呭閼冲鈧椒濞囬悽銊︽煙瀵繋绗屾宀冪槈閸忋儱褰?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴閸掓鍤粙瀣碍瑜版挸澧犻惃鍕閺堝濮涢懗濮愨偓浣稿讲閻劋濞囬悽銊︽煙瀵骏绱濇禒銉ュ挤閻╊喖澧犳惔鏃囶嚉婵″倷缍嶆宀冪槈妞ゅ湱娲伴悳鐗堟箒閸旂喕鍏橀敍灞借嫙鐏忓棜绻栨禍娑椾繆閹垵顔囪ぐ鏇炲煂閺傚洦銆傛稉顓溾偓?


### 閸掑棙鐎界拋鏉跨秿

1. 瑜版挸澧犳い鍦窗閻ㄥ嫬濮涢懗钘夊嚒缂佸繋绗夐崘宥呭涧閺勵垪鈧粌鐔€绾偓閹绢厽鏂侀垾婵撶礉鏉╂ê瀵橀幏顒€鐡ч獮鏇樷偓浣规尡閺€鎯у灙鐞涖劊鈧浇顔曠純顔衡偓浣告彥閹圭兘鏁妴浣虹彿閼哄倶鈧竸-B閵嗕焦鍩呴崶淇扁偓浣告姎濮濄儴绻橀妴浣圭壐瀵繑甯板ù瀣ㄢ偓涔刬ndows 閸氬海顏妴浣瑰絻娴犳湹绗屽ù浣哥崯娴ｆ捇鐛欓弨鎯板厴閸旀稏鈧?
2. 鏉╂瑤绨洪懗钘夊閻╊喖澧犻崚鍡樻殠閸?`main.cpp` 鐢喖濮潏鎾冲毉閵嗕椒鎹㈤崝鈩冪閸楁洏鈧焦婀伴崷浼寸崣閺€鑸靛Г閸涘﹤鎷伴懟銉ュ叡閺傚洦銆傛稉顓ㄧ礉缂傚搫鐨棃銏犳倻闂冨懓顕伴懓鍛畱娑撯偓妞ら潧绱￠幀鏄忣潔閵?
3. 閺堚偓閸氬牓鈧倻娈戦崑姘《閺勵垱鏌婃晶鐐扮娴犺В鈧粌濮涢懗?/ 娴ｈ法鏁?/ 妤犲矁鐦夐垾婵団偓鏄忣潔閺傚洦銆傞敍灞借嫙閸氬本顒炵槐銏犵穿娑撳孩鐗?README 閸忋儱褰涢妴?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`閿涘本濡歌ぐ鎾冲娑撹崵鈻兼惔蹇氬厴閸旀稑鍨庨幋鎰ㄢ偓婊呮暏閹村嘲褰查惄瀛樺复娴ｈ法鏁ら垾婵嗘嫲閳ユ粌绱戦崣?妤犲本鏁归懗钘夊閳ユ繀琚辩仦鍌涙降閺佸鎮婇妴?
- 閸︺劍鏋冨锝勮厬缂佹瑥鍤弲顕€鈧碍鎸遍弨鐐煙瀵繈鈧浇鍏橀崝娑欏赴濞村鏌熷蹇嬧偓渚€鍘ょ純顔芥煙瀵繈鈧線绮拋銈勬唉娴滄帗鏌熷蹇撴嫲閸旂喕鍏樻宀冪槈鐞涖劊鈧?
- 閸氬本顒為弴瀛樻煀 `docs/README.md` 娑撳孩鐗?`README.md`閿涘奔绻氱拠浣稿弳閸欙絽褰茬憴浣碘偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閺傜増鏋冨锝呭嚒鐟曞棛娲婅ぐ鎾冲閸旂喕鍏橀崚妤勩€冮妴浣规珮闁矮濞囬悽銊︽煙瀵繈鈧浇鐦栭弬顓熸煙瀵繈鈧線鍘ょ純顔芥煙瀵繋绗屾稉鎾汇€嶆宀冪槈閸忋儱褰涢妴?
- 閺傜増鏋冨锝呭嚒閺勫海鈥樺ù浣哥崯娴ｆ挶鈧焦褰冩禒韬测偓浣规姢闂€婊冾杻瀵櫣鐡戦懗钘夊閻ㄥ嫬缍嬮崜宥堢珶閻ｅ矉绱濋柆鍨帳鐠囶垰鍨芥稉鍝勭暚閺佸鏁ら幋宄板閼冲鈧?
- 瑜版挸澧犳禒鎾崇氨濞屸剝婀侀弬鏉款杻娴狅絿鐖滈弨鐟板З閿涘本婀板▎鈥冲涧閺傛澘顤冪拠瀛樻閺傚洦銆傞獮璺烘倱濮濄儳鍌ㄥ鏇樷偓?


### 娣囶喗鏁奸弬鍥︽

- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md

- docs/README.md

- README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?69: 閹绢厽鏂侀柧鎹愮槚閺傤厼鍨庣仦鍌欑瑢 decoder drain / scheduler 鐎瑰綊鏁婄悰銉ュ繁



**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閸ュ绮妯肩垳閻滃洦鎸遍弨鍓旂€规碍鈧嶇礉鐠囧嫪鍙?`FrameQueue` 閼冲苯甯囬妴涔╡coder `receive/send` 閺冭泛绨妴涔isplay::copyFrameData()`閵嗕梗av_hwframe_transfer_data()` 娑?`Scheduler` 娑撯偓濞嗭繝鍣搁崥顖炴閸掑墎鐡戝婊冩躬闂傤噣顣介敍灞借嫙閸︺劎鈥樼拋銈呮倵閽€钘夋勾娴兼ê瀵查妴?


### 閸掑棙鐎界拋鏉跨秿

1. 瑜版挸澧犲┃鎰垳瀹告彃鍙挎径鍥ㄦ蒋娴犺埖鍨氱粩瀣閻?`D3D11` native render path閿涘苯娲滃銈嗘＋閸掑棙鐎介弬鍥ㄣ€傞柌灞糕偓婊€瀵岄柧鍙ョ鐎规氨绮℃潻?copy-back + SDL upload閳ユ繄娈戠紒鎾诡啈瀹歌尙绮℃潻鍥ㄦ閿涘矁绻嶇悰灞炬閺囨挳娓剁憰浣虹叀闁挴鈧粌缍嬮崜宥呭煂鎼存洖鎳℃稉顓濈啊 native / copy-back / swscale 閸濐亝娼捄顖氱窞閳ユ縿鈧?
2. `decodeVideoFrame()` / `decodeAudioFrame()` 闁插洨鏁?receive-first 閹繆鐭鹃張顒冮煩濞岋繝妫舵０姗堢礉娴ｅ棙顒濋崜?packet queue EOF 閸氬孩鐥呴張澶岀舶 codec 閸欐垿鈧?`nullptr` drain閿涘矁鈧奔绗?send 閸氬骸褰х亸婵婄槸娑撯偓濞?receive閿涘奔绗夋径鐔稿复鏉╂垶鍨氶悢鐔告尡閺€鎯ф珤鐢瓕顫嗛惃?drain/feed 鐠囶厺绠熼妴?
3. `demux_dropped_packets_` 娑斿澧犲▽鈩冩箒缂佸棗鍨庨垾婊堟姜閻╊喗鐖ｅù浣筋潶韫囩晫鏆愰垾婵呯瑢閳ユ粎娲伴弽鍥ㄧウ閸忋儵妲︽径杈Е閳ユ繐绱濇导姘愁嚖鐎电厧顕懗灞藉竾閸滃苯鎮堕崥鎰懕妫板牏娈戦崚銈嗘焽閵?
4. `Scheduler` 娑斿澧犻崣顏冪箽閹躲倓绨＄憴锝囩垳缁捐法鈻奸敍瀹篹nder thread 濞屸剝婀侀崥灞剧壉閻ㄥ嫬绱撶敮闀愮箽閹躲倧绱遍崥灞炬濞屸剝婀佺紒鎾寸€崠鏍ь嚤閸戦缚鍎楅崢瀣╃瑢 restart 閹稿洦鐖ｉ妴?
5. 閺堫剝鐤嗙拋鎹愵吀閸欏倽鈧啩绨?`ffplay` 鐢瓕顫嗛惃?decoder drain 濡€崇础閿涘奔浜掗崣?`mpv / MPC-HC` 鐢瓕顫嗛惃鍕ㄢ偓婊冨帥閹跺﹨鐭惧鍕瑢閸樼喎娲滈柌蹇撳毉閺夈儻绱濋崘宥堢殶閸欏倹鏆熼垾婵堟畱鐠囧﹥鏌囬幀婵婄熅閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 闁插秴鍟?`decodeVideoFrame()` / `decodeAudioFrame()`閿涙碍鏁奸幋鎰瘮缂?`receive -> send -> receive` 閻樿埖鈧焦婧€閿涘苯鑻熼崷?packet EOF 閸氬骸鎮?codec 閸欐垿鈧?`nullptr` 鐎瑰本鍨?drain閵?
- 娑?`DiagnosticsSnapshot` 婢х偛濮?demux drop 閸掑棛琚妴涔╡coder `send_packet(EAGAIN)`閵嗕龚rain 濞嗏剝鏆熼敍灞间簰閸?`native/copy-back/swscale/filter-blocked` 鐟欏棝顣剁捄顖氱窞鐠佲剝鏆熼妴?
- 娑?`SchedulerStats` 婢х偛濮?video/audio 閼冲苯甯囨禍瀣╂娑?video/audio/render restart 缂佺喕顓搁敍灞借嫙閹?render thread 缁惧啿鍙?`runProtectedLoop()`閵?
- 閹碘晛鐫?`--performance-log-check` 鏉堟挸鍤敍宀冾唨閸氬海鐢?4K/妤傛鐖滈悳鍥у瀻閺嬫劘鍏橀惄瀛樺复閻顫嗚ぐ鎾冲閺勵垰鎯侀崨鎴掕厬 native path閵嗕焦妲搁崥锔炬埂閻ㄥ嫬鍤悳?queue drop閵嗕焦妲搁崥锕傤暥缁讳浇鍎楅崢瀣ㄢ偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?
- `build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`閿涙岸鈧俺绻冮敍娑樼秼閸撳秶骞嗘晶鍐叾妫版垵鍨垫慨瀣婢惰精瑙﹂敍灞肩稻閺傛媽鐦栭弬顓炵摟濞堥潧鍑￠幋鎰鐎电厧鍤敍灞肩瑬閸欘垵顫?`demux_dropped_packets` 閸︺劍婀板▎鈩冪壉閺堫兛鑵戦崗銊╁劥鐏炵偘绨?`demux_ignored_packets`閿涘奔绗夐弰?`queue drop`閵?


### 娣囶喗鏁奸弬鍥︽

- include/core/scheduler.h

- src/core/scheduler.cpp

- include/core/player_core.h

- src/core/player_core.cpp

- src/main.cpp

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂傤噣顣?70: PlayerCore 閻樿埖鈧焦婧€闁插秷顔曠拋锛勵儑娑撯偓闂冭埖顔?


**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴閸忓牐鎯ら崷鐗堟尡閺€鎯ф珤閸愬懏鐗抽悩鑸碘偓浣规簚闁插秷顔曠拋锛勵儑娑撯偓闂冭埖顔岄敍姘箽閻ｆ瑥顕径?`PlaybackState` 閸忕厧顔愰敍灞藉涧閸?`PlayerCore` 閸愬懘鍎撮幏鍡楀毉娴兼俺鐦介幀浣碘偓浣界箥鐞涘本鈧礁鎷板ù浣规寜缁炬寧鈧緤绱濋獮鑸垫暪閸欙絾鏆庨悙鍦Ц閹焦鏁奸崘娆嶁偓?
- 閺堫剝鐤嗛弰搴ｂ€樻稉宥呬粵 timeline serial閵嗕笒OF -> Ended 鐠囶厺绠熼柌宥呭晸閿涘奔绡冩稉宥嗗絹閸撳秴宕岀痪?`Scheduler` 婵傛垹瀹抽妴?


### 閺冦儱绻旀潏鎾冲毉

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 閸掑棙鐎界拋鏉跨秿

1. 瑜版挸澧?`PlaybackState` 娑斿澧犻崥灞炬閹佃儻娴囨禍?UI 閹绢厽鏂侀幀浣碘偓浣风窗鐠囨繃鈧礁鎷板ù浣规寜缁捐儻绻冪粙瀣偓渚婄礉`deferred stop` 鏉╂﹢顤傛径鏍ㄧ埗缁傝婀紒鐔剁閻樿埖鈧焦婧€娑斿顦婚妴?
2. `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 閻╁瓨甯撮崘?`state_`閿涘苯顕遍懛瀵稿Ц閹浇绺肩粔缁樻）韫囨ぜ鈧線娼▔鏇＄讣缁夎绻氶幎銈呮嫲閸氬海鐢?serial 閸栨牠鍏樺▽鈩冩箒缁嬪啿鐣鹃拃鐣屽仯閵?
3. `Scheduler` 瑜版挸澧犳禒宥呭涧閺?`running_ / paused_`閿涙稒婀版潪顔芥付閸氬牏鎮婇惃鍕珶閻ｅ矉绱濋弰顖氬帥鐠?`PlayerCore` 閹存劒璐熼悩鑸碘偓浣规綀婵炰緤绱濋崘宥嗗Ω閺囧绮忛惃鍕付閸掕泛鎻╅悡褏鏆€缂佹瑤绗呮稉鈧梼鑸殿唽閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `SessionState / RunState / PipelinePhase` 娑?`CoreStateSnapshot`閵?
- 閺傛澘顤冪紒鐔剁 transition helper 娑?`publishPlaybackStateFromInternalState()`閿涘矁顔€鐎电懓顦?`PlaybackState` 閹存劒璐熼崘鍛村劥閻樿埖鈧胶娈戦幎鏇炲閵?
- 鐏?`eof_reached / pending_seek / deferred_stop_pending` 缁惧啿鍙嗙紒鐔剁韫囶偆鍙庣粻锛勬倞閿涘苯鑻熼崷銊уЦ閹浇绺肩粔缁樻鏉堟挸鍤紒鎾寸€崠鏍ㄦ）韫囨ぜ鈧?
- 娣囨繃瀵旈悳鐗堟箒缁捐法鈻肩挧宄颁粻娑?`Scheduler` 鐠嬪啰鏁ら弬鐟扮础閸╃儤婀版稉宥呭綁閿涘矂浼╅崗宥囶儑娑撯偓闂冭埖顔岄懠鍐ㄦ纯婢惰鲸甯堕妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- Debug 閺嬪嫬缂撻柅姘崇箖閵?
- `PlayerCore` 閻╃鍙ф禒锝囩垳娑擃叏绱濋弫锝囧仯 `state_.store/exchange` 瀹稿弶鏁归崣锝呭煂缂佺喍绔?publish 閸忋儱褰涢妴?
- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md`閵?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?81: PlayerCore seek/flush timeline serial 閸栨牜顑囨禍宀勬▉濞?


**閺冦儲婀?*: 2026-03-20

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閹笛嗩攽閹绢厽鏂侀崳銊ュ敶閺嶅摜濮搁幀浣规簚闁插秷顔曠拋锛勵儑娴滃矂妯佸▓纰夌礉閼煎啫娲块梽鎰暰閸?seek/flush timeline serial 閸栨牭绱濇稉宥呭З copy-back閵嗕讣oftwareSDL閵嗕箒I 鐏炲倸鎷版径鏍劥 `PlaybackState`閵?
- 閺堫剝鐤嗛惃鍕壋韫囧啰娲伴弽鍥风礉閺勵垱濡?seek/stop/deferred stop 閻╃鍙х捄顖氱窞闁插苯顕弮褎妞傞梻瀵稿殠閺佺増宓侀惃鍕槱閻炲棴绱濇禒搴樷偓娓噇ush 鏉烆垱绔婚悶鍡忊偓婵嗗磳缁狙傝礋閳ユ笩erial 绾剙銇戦弫鍫氣偓婵勨偓?


### 閺冦儱绻旀潏鎾冲毉

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 閸掑棙鐎界拋鏉跨秿

1. 缁楊兛绔撮梼鑸殿唽瀹歌尙绮￠幎?`SessionState / RunState / PipelinePhase` 閸滃瞼绮烘稉鈧?transition 閸忋儱褰涢拃钘夊煂娴?`PlayerCore`閿涘苯娲滃銈囶儑娴滃矂妯佸▓闈涘徔婢跺洨菙鐎规氨娈?serial 閺€璺哄經閻愬箍鈧?
2. seek 閺冄冪杽閻滄壆娈戦梻顕€顣介敍灞肩瑝閺?flush 娑撳秴顧勬径姘剧礉閼板本妲?packet/frame 缂傚搫鐨弮鍫曟？缁捐儻闊╂禒鏂ょ幢閸楀厖濞囬崑姘啊 `pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop() + 娴滃本顐?flush`閿涘本妫弫鐗堝祦娴犲秶鍔ч崣顖濆厴閺呮艾鍩岄妴?
3. `ThreadSafeQueue` / `FrameQueue` 閺堫兛缍嬮柈鑺ョ梾閺?generation/serial閿涘畮udio consumer 缁捐法鈻兼稊鐔剁瑝娴兼艾婀?seek 閺冩儼鍤滈悞鍫曗偓鈧崙鐚寸礉render 鐠侯垰绶炴稊瀣閸氬本鐗卞▽鈩冩箒 serial gate閿涘本澧嶆禒銉ョ箑妞ょ粯濡搁垾婊冪熬瀵啳顫夐崚娆屸偓婵囨杹閸?packet/frame 閺堫剝闊╅崪灞剧Х鐠愮绔熼悾灞肩瑐閵?
4. 鏉╂瑨鐤嗛崶鐘愁劃闁瀚?item-level serial閿涘矁鈧奔绗夐弰顖氬帥閹?queue 鐎圭懓娅掗崑姘灇閹绢厽鏂侀崳銊ょ瑩閻?epoch 鐎圭懓娅掗敍娑滅箹閺嶉攱鏁奸崝銊︽纯閼辨氨鍔嶉敍灞肩瘍閺囧顑侀崥鍫氣偓婊咁儑娴滃矂妯佸▓闈涘帥閸?seek/flush serial 閸栨牑鈧繄娈戦懠鍐ㄦ纯閹貉冨煑閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`TimelineSerial`閿涘苯鑻熺拋?`DemuxPacket`閵嗕梗VideoFrame`閵嗕梗AudioFrame` 闁姤鎯＄敮?serial閵?
- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `timeline_serial / pending_seek_serial` 閸欏﹦绮烘稉鈧?helper閿涘瞼顩﹀銏犳躬 `seek / stop / open / deferred stop` 閸氬嫬顦╅梿鑸垫殠閼奉亜顤?serial閵?
- `open` 閹存劕濮涢崥搴＄紦缁斿顩绘稉?serial閿涙矖seek` 瀵偓婵妞傞崗鍫濆瀻闁?`pending_seek_serial`閿涘矁顔€閹恒儱褰堟潏鍦櫕閸忓牆鍨忕挧甯礉seek 閹存劕濮涢崥搴″晙濠碘偓濞蹭紮绱盽stop / requestDeferredStop` 娴兼氨鐝涢崡铏腹鏉?serial閵?
- demux 缁捐法鈻奸崷銊ユ儙閸斻劍妞傞幑鏇″箯 active serial閿涘畳ecode/render/audio consumer 閸忋劑鎽肩捄顖涘瘻 serial 娑撱垹绱?stale 閺佺増宓侀敍娌梖lush` 缂佈呯敾娣囨繄鏆€閿涘奔绲鹃梽宥囬獓娑撻缚绶熼崝鈺傜閹殿偁鈧?
- diagnostics 閸滃奔绗撴い瑙勵梾閺屻儱鎳℃禒銈嗘煀婢?`timeline_serial / pending_seek_serial` 鏉堟挸鍤敍灞肩┒娴滃骸鎮楃紒顓犳埛缂侇厼浠?EOF/Ended 娑?Scheduler 婵傛垹瀹抽梼鑸殿唽閵?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- Debug 閺嬪嫬缂撻柅姘崇箖閵?
- packet/frame 閸?render/audio consumer 閻ㄥ嫰鎽肩捄顖氬嚒缂佸繐鍙挎径?serial 娑撱垹绱旀潏鍦櫕閵?
- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md`閵?


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



## ?? 82: PlayerCore EOF/Ended ????????



**??**: 2026-03-20

**??**: ???



### ????

- ??????????????????????????????????? copy-back?SoftwareSDL?UI ???? `PlaybackState`?

- ????? EOF ? stop/deferred-stop ?????????????????????? `Ended`???? `close/reopen` serial ????? scheduler stale render ?????



### ????

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### ????

1. ???????? packet/frame ? seek/stop ????? serial ???? EOF ?????????? `Stopped`???????????? stop ??????

2. ?? demux EOF?packet queue EOF ? frame queue ???????????????????????????????????? ended ?????

3. `close()` ???? `stop()` ???????? timeline ????????? scheduler ???? render callback ????????? stale serial ??????????



### ????

- ? `PlayerCore` ????? `EndedReason`??? `ended_reason` ?? `CoreStateSnapshot`?`DiagnosticsSnapshot`?????????????

- ?? `onRenderIdle()`?? demux EOF ???? `PipelinePhase::Draining`???? packet queue EOF + frame queue ?? + `audio_player_->getBufferedSeconds() <= 0.001`????? `RunState::Ended`?

- ??? EOF ? `deferred stop` ?? `Stopped`?`play()` ? `Ended` ???? `seek(0.0)`?`seek()` ? `Ended` ????? `Stopped`?

- ? `close()` ??? stop ???? serial ?????? worker ??????? stale serial ????

- ? scheduler render callback ???? `bool`????? render/present ?????? `rendered_frames` ???????

- ?????? `docs/analysis/PLAYERCORE_DAY18_EOF_ENDED_AND_TERMINAL_STATE_REDESIGN.md`?????????????????



### ??????

- Debug ?????

- EOF ???????? `Ended`???????? `Stopped`?

- `play()` ? `Ended` ??????????

- close/reopen serial ??? scheduler ??????????????



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

## ?? 83: PlayerCore queue generation ? Scheduler ????????

**??**: 2026-03-20
**??**: ???

### ????
- ????????????????????? UI ??????? flush ??? scheduler ???????????
- ???????? queue ? `clear()/flush()` ???????????? scheduler ? `Seeking / Flushing / Stopping / Ended` ????? decode/render ???

### ????
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

### ????
1. ??????? packet/frame ? serial ????????? queue ?????????????????????????????????? `push()/pop()` ? flush ?????????
2. ??????? `Ended` ???????? scheduler ??????? `RunState / PipelinePhase / accepted timeline serial`??? seek/flush/stopping ?????????????
3. ?????????????????? `Ended` ???????? queue generation ? scheduler ???????????????????????

### ????
- ? `ThreadSafeQueue` ? `FrameQueue` ?? generation??? `clear()/flush()` ?? generation????? `push()/pop()` ????? generation ???????????????
- ? `scheduler.h` ??? `SchedulerControlSnapshot`?? `Scheduler` ???? `run_state / pipeline_phase / accepted_timeline_serial`?
- `PlayerCore` ?? `makeSchedulerControlSnapshot()` ??????? `setControlSnapshotProvider()` ??? scheduler?
- ??/?? decode loop ????????????????render loop ????????????????? accepted serial?????????
- `DiagnosticsSnapshot`??? diagnostics ????????? queue generation ????????? flush ?????
- ?????? `docs/analysis/PLAYERCORE_DAY19_QUEUE_GENERATION_AND_SCHEDULER_CONTROL_REDESIGN.md`?

### ??????
- Debug ???????? `0 warnings / 0 errors`?
- queue flush ?????? generation ????????????????
- scheduler ?????????????? `Seeking / Flushing / Stopping / Ended` ?????????????
- diagnostics / ???????????? packet/frame queue generation?

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

## 闂傤噣顣?84: PlayerCore 閸擃垯缍旈悽銊╂肠娑擃厼瀵叉稉?runtime failure/recovery policy 閺€璺哄經

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- timeline serial 娑?queue generation 瀹歌尙绮￠幎?seek/flush 閻ㄥ嫭鏆熼幑顔跨珶閻ｅ瞼鈥栭崠鏍电礉娴?`PlayerCore` 閸忋儱褰涢柌灞肩矝濞ｉ娼冪痪璺ㄢ柤閵嗕浇顔曟径鍥モ偓渚€妲﹂崚妤€鎷伴弮鍫曟寭閸擃垯缍旈悽銊ｂ偓?- `SchedulerControlSnapshot` 鏉╂ê褰ч張澶嬫付鐏忓繑鈧緤绱漵cheduler 鐎?`clock_source`閵嗕工udio-master 娑?ended policy 閻ㄥ嫪绶风挧鏍︾矝閻掕埖鐥呴張澶婄暚閸忋劎绮ㄩ弸鍕閵?- runtime fatal 閻愮懓顩ч弸婊呮埛缂侇厼鍨庨弫锝咁槱閻炲棴绱濇导姘Ω stopping / session release / error emit 閸愬秵顐奸幏鍡樻殠閵?
### 閺冦儱绻旀潏鎾冲毉
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 閸掑棙鐎界拋鏉跨秿
1. 鏉╂瑨鐤嗛惃鍕櫢閻愰€涚瑝閺勵垳鎴风紒顓熸暭 timeline serial閿涘矁鈧本妲搁幎濞锯偓婊呭Ц閹浇绺肩粔璁崇閸氬氦顕氶崑姘矆娑斿牆濮╂担婧锯偓婵囨暪閸欙絾鍨氱紒鐔剁閸擃垯缍旈悽銊δ侀崹瀣ㄢ偓?2. `deferred stop` 鎼存棁顕氭穱婵堟殌娑撳搫绱撳?stop completion 閺堝搫鍩楅敍灞肩稻娑撳秴绨茬紒褏鐢绘担婊€璐熼弮浣界熅娑撴艾濮熼悩鑸碘偓浣规降濠ф劑鈧?3. scheduler 瀹稿弶婀?`run_state / pipeline_phase / accepted_timeline_serial`閿涘瞼鎴风紒顓熷⒖閹?`clock_source + audio master + ended policy` 缂佹挻鐎敍宀冨厴闁灝鍘ら崘宥呮礀閸掍即娴傞弫锝呯鐏忔柧缍呭Ο鈥崇础閵?4. runtime failure 韫囧懘銆忛崗鍫㈢埠娑撯偓 recovery policy閿涘苯鍟€闁劖顒為幍鈺勵洬閻╂牜鍋ｉ敍娑樻儊閸掓瑦鐦℃稉顏呮煀 fatal 閻愬綊鍏樻导姘槻閸掓湹绔撮柆?stop/release/error 閸掑棙鏁妴?
### 婢跺嫮鎮婄紒鎾寸亯
- `PlayerCore` 瀹稿弶鏌婃晶鐐扮缂佸嫮绮烘稉鈧?side-effects helpers閿涘苯鑻熼幎?`play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 閻ㄥ嫭鐗宠箛鍐ㄥ娴ｆ粎鏁ゆ潻浣稿弳鏉╂瑤绨?helpers閵?- `SchedulerControlSnapshot` 瀹稿弶鏌婃晶?`clock_source`閵嗕梗audio_output_initialized`閵嗕梗audio_master_sync_active`閵嗕梗ended_policy`閿涘cheduler 瀹稿弶甯撮崗銉ㄧ箹娴滄稑鐡у▓鐐光偓?- 閺傛澘顤?`FailureRecoveryPolicy` 娑?`handleRuntimeFailure()`閿涘苯鑻熼幎?decode/resample 閸忔娊鏁?fatal 閻愯甯撮崗銉х埠娑撯偓閹垹顦查崗銉ュ經閵?- 瀹稿弶鏌婃晶?analysis閿涙瓪docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md`閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻柅姘崇箖閵?- 鏉╂瑨鐤嗛張顏呮暭 UI 鐏炲倶鈧恭opy-back閵嗕讣oftwareSDL 閸滃苯顦婚柈?`PlaybackState`閵?- 瑜版挸澧犻崜鈺€缍戞搴ㄦ珦瀹稿弶妲戠涵顔芥暪閺佹稑鍩岄敍姘纯鐎瑰本鏆ｉ惃?`SchedulerControlSnapshot`閵嗕梗FailSession` 閻喐顒滈崥顖滄暏閺冨墎娈戠痪璺ㄢ柤鐎瑰鍙忔潏鍦櫕閿涘奔浜掗崣濠冩纯缂佸棛娈?ended/audio-master policy閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?85: PlayerCore 閸撯晙缍戞搴ㄦ珦閺€鑸垫殐閿涙瓔cheduler 缂佸牏澧楃粵鏍殣閵嗕笚ailSession 鐎圭偛瀵叉稉?serial/generation 鐟欏倹绁村鍝勫

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涚憰浣圭湴缂佈呯敾濞戝牆瀵查崜鈺€缍戞搴ㄦ珦閿涙瓪SchedulerControlSnapshot` 缂佸牏澧楃粵鏍殣閸栨牓鈧梗FailSession` 鐎圭偟鏁ら崠鏍モ偓浣蜂簰閸?generation 娑?serial 閼卞矁鐭楁潏鍦櫕瀵搫瀵查妴?
### 閺冦儱绻旀潏鎾冲毉
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 閸掑棙鐎界拋鏉跨秿
1. scheduler 闂団偓鐟曚焦妲戠涵顔界Х鐠愬湱鐡ラ悾銉︾亣娑撴拝绱濋懓灞肩瑝閺勵垳鎴风紒顓濈贩鐠ф牠娈ｅ蹇撶鐏忔梻绮嶉崥鍫涒偓?2. `FailSession` 闂団偓鐟曚浇顩惄鏍︾窗鐠囨繄楠囨稉宥呭讲閹垹顦查柨娆掝嚖閿涘苯鑻熺涵顔荤箽 stop/join 閸?worker 娑撳﹣绗呴弬鍥у讲鐎瑰鍙忛幍褑顢戦妴?3. generation 閸欘亣袙閸愬啿顔愰崳銊ㄧ珶閻ｅ奔鑵戦弬顓ㄧ幢serial 閹靛秵妲搁弮褎妞傞梻瀵稿殠绾剙銇戦弫鍫滃瘜閸掋倕鐣鹃敍宀勬付闁俺绻?stale drop 鐠佲剝鏆熼惄纾嬵潎鐟欏倸鐧傞妴?
### 婢跺嫮鎮婄紒鎾寸亯
- 閹碘晛鐫?`SchedulerControlSnapshot`閿涙碍鏌婃晶?`clock_policy`閵嗕梗audio_master_policy`閵嗕梗audio_buffered_seconds`閿涘苯鑻熼幍鈺佺潔 ended policy閵?- `Scheduler` render wait 閺€閫涜礋缁涙牜鏆愭す鍗炲З閿涙矖Scheduler::stop()` 鐞?self-join 娣囨繃濮㈤妴?- `handleRuntimeFailure()` 閺€璺哄經婢х偛宸遍獮鎯八夌紒鐔活吀閿涙稑鍙ч柨顔荤瑝閸欘垱浠径宥夋晩鐠囶垳鍋ｉ崚鍥у煂 `FailSession`閵?- 閺傛澘顤?stale serial drop 鐠佲剝鏆熼獮鑸靛复閸?diagnostics 娑撳孩顥呴弻銉ユ嚒娴犮倛绶崙鎭掆偓?- 閺傛澘顤?analysis閿涙瓪docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md`閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻柅姘崇箖閵?- 閺堫剝鐤嗛張顏呮暭 UI 鐏炲倶鈧礁顦婚柈?`PlaybackState`閵嗕恭opy-back閵嗕讣oftwareSDL閵?
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
## 闂傤噣顣?86: serial/failsession 閸ョ偛缍婂Λ鈧弻銉ㄋ夋鎰剁礄鏉╃偟鐢?seek閵嗕焦娈忛崑婊勨偓?seek閵嗕恭lose-reopen閿?
**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閻劍鍩涚憰浣圭湴缂佈呯敾閹?WORKFLOW 閹恒劏绻橀敍灞肩喘閸忓牐藟姒绘劘绻涚紒?seek / 閺嗗倸浠犻幀?seek / close-reopen 閻?CLI 閸ョ偛缍婂Λ鈧弻銉ｂ偓?- 閻╊喗鐖ｉ弰顖濈翻閸戠儤婧€閸ｃ劌褰茬拠?`key=value` 閸?`result=PASS/FAIL`閿涘苯鑻熺憰鍡欐磰 stale/serial/FailSession 缁撅附娼妴?
### 閺冦儱绻旀潏鎾冲毉
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Checks:
build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4 -> PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 閸忓牐藟 `DiagnosticsSnapshot` 閻ㄥ嫰娼▔鏇＄讣缁夋槒顓搁弫甯礉闁灝鍘?CLI 閸欘亣鍏橀棃鐘虫）韫囨鏋冮張顒€鍨界€规哎鈧?2. 鏉╃偟鐢?seek 閸︾儤娅欐稉顓炲讲鐟欏倸鐧傞崚?`illegal_pipeline_transitions` 閸?`Draining -> Seeking` 閺堝顤冮柌蹇ョ礉娴ｅ棗缍嬮崜宥嗘弓娴兼挳娈?`FailSession`閵?3. 閸掋倕鐣鹃柅鏄忕帆娴兼ê鍘涙穱婵婄槈 `FailSession` 鐠侯垰绶炴稉宥呭毉閻滀即娼▔鏇＄儲鏉烆剨绱檂fail_session_transition_ok`閿涘绱濋獮鏈电箽閻ｆ瑩娼▔鏇＄讣缁夋槒顓搁弫鎵暏娴滃骸鎮楃紒顓濈瑩妞よ鏁归弫娑栤偓?
### 婢跺嫮鎮婄紒鎾寸亯
- `PlayerCore`閿?  - 閺傛澘顤冮獮璺侯嚤閸?`illegal_session_transitions / illegal_run_transitions / illegal_pipeline_transitions`
  - 闂堢偞纭舵潻浣盒╅崚鍡樻暜鐠佲剝鏆?+ diagnostics reset + diagnostics 閺冦儱绻旀潏鎾冲毉
- CLI閿?  - 閺傛澘顤?`--seek-burst-serial-check`
  - 閺傛澘顤?`--paused-seek-serial-check`
  - 閺傛澘顤?`--close-reopen-serial-check`
  - 閻滅増婀?`--performance-log-check`閵嗕梗--software-video-decode-check` 閸氬本顒炵€电厧鍤棃鐐寸《鏉╀胶些鐠佲剝鏆?- 閺傚洦銆傞敍?  - 閺傛澘顤?`docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md`

### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?87: serial/failsession 閸ョ偛缍婃晶鐐插娑撯偓闁款喛浠涢崥?gate閿涘牓妾锋担搴㈢础鐠烘垿顥撻梽鈺嬬礆

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 娑撳閲?serial/failsession 娑撴捇銆嶉幒銏ゆ嫛瀹歌尙绮￠崣顖滄暏閿涘奔绲鹃幍褑顢戞禒宥勭贩鐠ф牔姹夊銉よ鐞涘矁鐨熼悽銊ｂ偓?- 閸︺劍瀵旂紒顓″嚡娴狅絼鑵戦敍灞兼眽瀹搞儰瑕嗙悰灞炬煙瀵繐顔愰弰鎾寸础鐠烘垶鐓囨稉鈧い鐧哥礉鐎佃壈鍤ч崶鐐茬秺 gate 娑撳秶菙鐎规哎鈧?
### 閺冦儱绻旀潏鎾冲毉
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Check:
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.pass_count=3
serial-failsession-regression-check.total_count=3
serial-failsession-regression-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 閹恒垽鎷￠張顒冮煩瀹告彃鍙挎径鍥╃埠娑撯偓閻?`key=value` 閸掋倕鐣剧€涙顔岄敍宀€宸遍崣锝呮躬閳ユ粍澧界悰灞藉弳閸欙綀浠涢崥鍫氣偓婵勨偓?2. 閼汇儰绗夐幓鎰返閼辨艾鎮庨崗銉ュ經閿涘矁鐨熼悽銊︽煙闂団偓闁插秴顦茬紒瀛樺Б娑撳娼崨鎴掓姢娑撳骸寮弫甯礉閸ョ偛缍婇懘姘拱閺勬挸鍨庨崣澶堚偓?3. 閸忓牆婀?CLI 鐏炲倽藟閼辨艾鎮?gate閿涘苯褰叉禒銉ユ躬娑撳秵鏁?`PlayerCore` 閺嶇绺鹃柅鏄忕帆閻ㄥ嫬澧犻幓鎰瑓闂勫秳缍嗛幍褑顢戝▓瀣╃稇妞嬪酣娅撻妴?
### 婢跺嫮鎮婄紒鎾寸亯
- `src/main.cpp` 閺傛澘顤冮敍?  - `runSerialFailSessionRegressionCheck(...)`
  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 閼辨艾鎮庨崨鎴掓姢閸愬懘鍎存い鍝勭碍閹笛嗩攽閿?  - `runSeekBurstSerialCheck`
  - `runPausedSeekSerialCheck`
  - `runCloseReopenSerialCheck`
- 閺傛澘顤冮懕姘値缂佹挻鐏夌€涙顔岄敍?  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`
- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md`

### 娣囶喗鏁奸弬鍥︽
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?88: 瀵搫鍩?FailSession 閸ョ偛缍婇幒銏ゆ嫛娑?codec 闁夸線鍣搁崗銉ョ┛濠у啩鎱ㄦ径?
**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- `FailSession` 娴犲秳瀵岀憰浣风贩鐠ф牜婀＄€圭偤鏁婄拠顖澬曢崣鎴礉閸ョ偛缍婄憰鍡欐磰娑撳秶菙鐎规哎鈧?- 閸︺劏藟瀵搫鍩楅幒銏ゆ嫛閺冭绱濈憴锕€褰傛禍鍡毿掗惍浣哄殠缁嬪绻橀崗?`FailSession` 閻?codec 闁夸線鍣搁崗銉ョ磽鐢潻绱檂device or resource busy`閿涘鈧?
### 閺冦儱绻旀潏鎾冲毉
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Forced FailSession check:
build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200
forced-failsession-check.runtime_failure_stop_requests=1
forced-failsession-check.runtime_failure_fail_sessions=1
forced-failsession-check.illegal_transition_total=0
forced-failsession-check.result=PASS

Serial aggregate check:
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.result=PASS
```

### 閸掑棙鐎界拋鏉跨秿
1. 閸忓牆绱╅崗銉ュ讲閹貉勬暈閸忋儳鍋ｉ敍宀€鈥樻穱?`FailSession` 鐠侯垰绶為崣顖濐潶缁嬪啿鐣剧憴锕€褰傞妴?2. 瀵搫鍩楃憴锕€褰傞崥搴ｇ彌閸楄櫕姣氶棁鎻掓倱缁捐法鈻?codec 闁夸線鍣搁崗銉╂６妫版﹫绱濈拠瀛樻鐠囥儴鐭惧鍕劃閸撳秵婀悮顐㈠帠閸掑棗甯囧ù瀣ㄢ偓?3. 娣囶喖顦查柨浣筋嚔娑斿鎮楅敍瀹岶ailSession` 鐠侯垰绶為崣顖溓旂€规俺鎯ら崚?`session=Failed / playback=Stopped`閿涘矂娼▔鏇＄讣缁夋槒顓搁弫棰佺箽閹?0閵?
### 婢跺嫮鎮婄紒鎾寸亯
- `PlayerCore::decodeVideoFrame` 婢х偛濮?`MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE` 濞村鐦▔銊ュ弳闁槒绶妴?- `main.cpp` 閺傛澘顤?`--forced-failsession-check <media_file> [sample_ms]`閿涘矁绶崙鍝勭暚閺佸瓨婧€閸ｃ劌褰茬拠?gate 鐎涙顔岄妴?- `video_codec_mutex_`閵嗕梗audio_codec_mutex_` 閺€閫涜礋 `std::recursive_mutex`閿涘苯鑻熼崥灞绢劄閺囧瓨鏌?`decodeVideoFrame/decodeAudioFrame` 闁夸胶琚崹瀣ㄢ偓?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?89: run_all_checks 閹恒儱鍙?forced-failsession 娑撯偓闁?gate

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 閺冦儱鐖舵稉鈧柨顔兼礀瑜版帟鍓奸張?`tools/run_all_checks.ps1` 閺堫亪绮拋銈嗗⒔鐞?`--forced-failsession-check`閿涘瓗ailSession 鐠侯垰绶炵憰鍡欐磰娓氭繆绂嗘禍鍝勪紣閸楁洜瀚幍褑顢戦妴?
### 閺冦儱绻旀潏鎾冲毉
```text
Command:
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 \
  -ExecutablePath "build/Debug/modern-video-player.exe" \
  -ProbeFile "juren-30s.mp4" \
  -ForcedFailSessionSampleMs 2200

Result:
[1/3] Probe exit code: 0
[2/3] Forced FailSession exit code: 0
[3/3] Regression exit code: 0
Script exit code: 0
```

### 閸掑棙鐎界拋鏉跨秿
1. `--forced-failsession-check` 瀹告彃褰茬粙鍐茬暰 PASS閿涘苯绨查幓鎰磳閸掔増澹掓径鍕倞姒涙顓诲ù浣衡柤閵?2. 娑撯偓闁款喛鍓奸張顒勬付鐟曚焦濡?forced path 鐠佸彞璐熺涵?gate閿涘矁鈧奔绗夐弰顖欎繆閹垱鈧勵劄妤犮們鈧?3. 閸欏倹鏆熺拋鎹愵吀鎼存柧绻氶幐浣告倻閸氬骸鍚嬬€圭櫢绱濇妯款吇婢跺秶鏁?`ProbeFile` 閺堚偓鐏忓繐瀵叉潻浣盒╅幋鎰拱閵?
### 婢跺嫮鎮婄紒鎾寸亯
- `tools/run_all_checks.ps1` 閺傛澘顤冮崣鍌涙殶閿?  - `ForcedFailSessionFile`
  - `ForcedFailSessionSampleMs`
- 閹笛嗩攽妞ゅ搫绨悽鍙樿⒈濮濄儲鏁兼稉杞扮瑏濮濄儻绱?  - `[1/3]` probe
  - `[2/3]` forced-failsession gate
  - `[3/3]` format regression
- gate 闁槒绶敍?  - probe 闂堢偤娴?-> 閻╁瓨甯撮柅鈧崙鐚寸幢
  - forced-failsession 闂堢偤娴?-> 閻╁瓨甯撮柅鈧崙鍝勮嫙鐠哄疇绻?regression閵?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md`閵?
### 娣囶喗鏁奸弬鍥︽
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 闂 90: OpenGL 鍘熺敓 D3D11 浜掓搷浣?stop/close 寮傚父閫€鍑?
**鏃ユ湡**: 2026-03-24
**鐘舵€?*: 宸茶В鍐?### 闂鎻忚堪
- `Release` 涓嬫墽琛?`$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 鏃讹紝鏃ュ織鏄剧ず宸插惎鐢?`OpenGL native D3D11 interop`锛屼絾 stop/close 闃舵寮傚父閫€鍑猴紝CLI 娌℃湁鎵撳嵃鏈€缁?`performance-log-check.result`銆?- 鍚屼竴杞牱鏈腑 native 鍚炲悙寮傚父鍋忎綆锛屽彧娓叉煋浜嗙害 3 甯с€?### 鏃ュ織杈撳嚭
```text
OpenGL native D3D11 interop enabled for AV_PIX_FMT_D3D11 surfaces
D3D11VA decoder bound to renderer-owned D3D11 device
[diag:audio-backpressure] ... render(out=3,...) video(path_native=3,copyback=0,...)
EXITCODE=2173
```
### 鍒嗘瀽璁板綍
1. OpenGL 鍘熺敓璺緞涓?D3D11 涓绘覆鏌撳櫒涓嶅悓锛屽畠璁?FFmpeg 纭В鍜?OpenGL 娓叉煋绾跨▼鍏变韩 renderer-owned D3D11 device/context銆?2. 浠ｇ爜妫€鏌ュ彂鐜?OpenGL 璺緞娌℃湁鍍?D3D11 涓婚摼閭ｆ牱鍚敤 `ID3D11Multithread::SetMultithreadProtected(TRUE)`銆?3. 缁х画妫€鏌?session 閲婃斁椤哄簭锛屽彂鐜?`applySessionReleaseSideEffects()` 鍏堥噴鏀?decoder/hw context锛屽啀鍏抽棴 renderer锛涜繖瀵规寔鏈?native `AVFrame` 鐨勭紦瀛樺抚鍜屾覆鏌撶嚎绋嬩笉瀹夊叏銆?### 澶勭悊缁撴灉
- 鍦?`src/render/opengl_video_renderer.cpp` 涓ˉ榻?`ID3D11Multithread` 澶氱嚎绋嬩繚鎶ゃ€?- 鍦?`src/core/player_core.cpp` 涓妸缂撳瓨 native frame 涓?renderer 鍏抽棴鍓嶇疆鍒?decoder/hw context 閲婃斁涔嬪墠銆?- 澶嶆祴鍚?OpenGL 鍘熺敓璺緞鍚炲悙鎭㈠姝ｅ父锛宻top/close 鍙ǔ瀹氬畬鎴愩€?### 澶嶆祴缁撴灉
```text
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.render_frames=47
performance-log-check.result=PASS
EXITCODE=0
```
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
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- OpenGL 瀛楀箷閾惧凡缁忓叿澶囧熀鏈?ASS/SSA 鏀寔锛屼絾 `WrapStyle / spacing / scale / rotation / x-y border / x-y shadow` 浠嶆湭瀹屾暣绌块€忓埌 GPU renderer銆?- 瑙ｆ瀽灞傜己灏戞牱寮忚瘖鏂叆鍙ｏ紝瀵艰嚧 OpenGL 瀛楀箷闂闅句互鍋氱ǔ瀹氬洖褰掋€?
### 鏃ュ織杈撳嚭
```text
subtitle-style-check.entries=5
subtitle-style-check.result=PASS
subtitle-sync-check.mismatches=0
subtitle-sync-check.result=PASS
delay-adjust-check.subtitle_loaded=true
delay-adjust-check.subtitle_entries=5
delay-adjust-check.result=PASS
opengl-diagnostics.result=PASS
```

### 鍒嗘瀽璁板綍
- 鎵╁睍 `SubtitleStyle`锛屾柊澧?`wrap_style / spacing / scale_x_percent / scale_y_percent / rotation_degrees / outline_x / outline_y / shadow_x / shadow_y`銆?- `ass_parser.cpp` 琛ラ綈 `WrapStyle`銆乻tyle 瀛楁鍜?`\q/\fsp/\fscx/\fscy/\fr/\frz/\xbord/\ybord/\xshad/\yshad`銆?- OpenGL/D3D11 瀛楀箷 renderer 琛ラ綈 DirectWrite/D2D 娑堣垂閫昏緫锛歚SetCharacterSpacing + D2D transform + x/y outline/shadow`銆?- 澧炲姞 `--subtitle-style-check`锛屽苟琛?`samples/subtitles/opengl_ass_style_validation.ass` 浣滀负鍥炲綊鏍锋湰銆?
### 楠岃瘉鍛戒护
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --opengl-diagnostics
```

## Issue 92: OpenGL/D3D11 subtitle run-level karaoke, clip and subtitle clock convergence

**Date**: 2026-03-24
**Status**: Resolved

### Problem Description
- GPU subtitle rendering still had a gap between parser/model support and actual renderer behavior for ASS karaoke and rectangular clip semantics.
- OpenGL in particular still needed subtitle-clock-driven texture invalidation and run-level D2D rendering convergence.

### Logs / Validation
```text
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
delay-adjust-check.result=PASS
d3d11-diagnostics.result=PASS
opengl-diagnostics.result=PASS
```

### Analysis Notes
- Closed subtitle clock propagation from `PlayerCore` into renderer-side animated subtitle redraw decisions.
- Completed run-level D2D subtitle rendering on both D3D11 and OpenGL.
- Added basic rectangular `iclip` rendering support through inverse clip region decomposition.
- Added a dedicated karaoke/clip regression sample and exposed the new style/run fields through `--subtitle-style-check`.

## Issue 93: ASS move/fad/fade animation converged into D3D11/OpenGL subtitle rendering

**Date**: 2026-03-24
**Status**: Resolved

### Problem Description
- The GPU subtitle chain still lacked practical ASS line animation semantics even after karaoke/clip support was added.
- Without move/fade integration, subtitle motion and opacity behavior still lagged behind mature players.

### Logs / Validation
```text
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
delay-adjust-check.result=PASS
d3d11-diagnostics.result=PASS
opengl-diagnostics.result=PASS
```

### Analysis Notes
- Added a dedicated line-level animation container on subtitle items.
- Added parser coverage for `\move`, `\fad` and `\fade`.
- Added renderer-side move interpolation and fade opacity evaluation on both D3D11 and OpenGL.
- Verified animation fields are exposed through `--subtitle-style-check` and covered by a dedicated regression sample.


## Issue 94: ASS transform, vector drawing/clip, and font fallback groundwork

**Date**: 2026-03-25
**Status**: Resolved

### Problem Description
- The next ASS convergence batch still needed `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector drawing / vector clip, and font fallback groundwork across the GPU subtitle path.
- OpenGL runtime validation started failing after the new vector clip path landed.

### Log Output
```text
OpenGL subtitle D2D draw failed: hr=-2003238890
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
OpenGL delay-adjust-check.result=PASS
D3D11 delay-adjust-check.result=PASS
```

### Analysis Notes
- `hr=-2003238890` was decoded to `D2DERR_PUSH_POP_UNBALANCED (0x88990016)`.
- The failure came from mismatched clip layer state in the renderer, not from missing D3D11/OpenGL/NV12 hardware capability.
- `draw_text_pass()` had an unmatched `PopLayer()` and `draw_box_region()` had a missing `PopLayer()`.
- The same bug pattern existed in both OpenGL and D3D11 subtitle renderers, so both were corrected together.
- Subtitle-sidecar/private font registration was added as groundwork, but full container attachment extraction is still pending.

### Files Updated
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
**Status**: Resolved

### Description
- Added idle startup window behavior for direct exe launch without media args.
- Added drag-drop media open support in SDL, D3D11, and OpenGL renderer paths.
- Added validated playback replacement when a new file is dropped onto an active playback window.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Manual GUI smoke test:
Not executed in automation during this turn.
Recommended checks:
1. Launch build\\Release\\modern-video-player.exe with no args.
2. Drag a local video file onto the idle window.
3. Drag another file during playback and confirm replacement.
```

### Analysis Notes
1. The safe place to react to a dropped file is the app loop, not the renderer or `PlayerCore`, because path validation must happen before playback is stopped.
2. OpenGL needed its file-drop handling in the internal render-thread event pump, not only in the external `handleEvents()` wrapper.
3. Idle-mode support is cheapest when implemented as a renderer-only window in `main.cpp`, instead of forcing a fake media-open path through `PlayerCore`.

### Result
- Empty-start sessions now open an idle window.
- File-drop requests now traverse renderer -> `PlayerCore` -> `VideoPlayer` -> `main.cpp`.
- Invalid drops are ignored without interrupting playback.
- Sessions started without CLI media return to the idle window after playback finishes.

### Files
- `docs/analysis/PLAYERCORE_DAY32_IDLE_WINDOW_AND_DRAG_DROP_PLAYBACK.md`
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

## Issue 100: OpenGL present pacing and AMD stutter
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Investigated user-reported OpenGL playback stutter under native D3D11 interop.
- The main structural issue was that OpenGL submit and actual display completion were decoupled, so scheduler pacing could run ahead of the real window presentation path.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Diagnostics:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

OpenGL native path:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500
Result: PASS

OpenGL copy-back path:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500
Result: PASS
```

### Analysis Notes
1. The old OpenGL path treated queued frames as already presented, unlike SDL and D3D11.
2. That mismatch made visible stutter possible without clearly failing existing scheduler counters.
3. Forcing `swap interval=0` further widened the behavior gap from the D3D11 path.
4. Removing the per-frame native interop `Flush()` reduces an avoidable synchronization point.

### Result
- OpenGL now waits for real render-thread presentation before returning from `present()`.
- OpenGL now prefers synchronized swap pacing.
- Native and copy-back OpenGL paths both remained functional in local validation.
- No `OpenGL present wait timed out` warning was observed in local regression runs.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY33_OPENGL_PRESENT_PACING_AND_AMD_STUTTER.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
## Issue 101: OpenGL runtime diagnostics export and P010/P016 copy-back upload
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed end-to-end validation for the new `renderer_opengl_*` counters in `--performance-log-check`.
- Added `p010le/p016le` direct software upload support to the OpenGL renderer.
- Removed the extra 10-bit copy-back downgrade from `p010le -> swscale -> yuv420p` on the OpenGL fallback path.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL native regression:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
Result: PASS

OpenGL copy-back regression:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
Result: PASS

OpenGL 10-bit copy-back regression:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200
Key lines:
- decoder_backend=D3D11VA
- video_copy_back_frames=72
- video_swscale_frames=0
- renderer_opengl_native_interop_active=false
- result=PASS
```

### Analysis Notes
1. The renderer diagnostics export was already coded, but it still required final-surface verification through the actual CLI that field engineers use.
2. `PlayerCore` already avoids `swscale` when the renderer advertises direct-frame support, so the real missing piece was OpenGL support for 16-bit semi-planar software frames.
3. Using 16-bit normalized GL textures lets the copy-back path keep `p010le/p016le` precision without changing the higher-level scheduler or decoder state machine.

### Result
- `renderer_opengl_native_interop_*` and `renderer_opengl_present_wait_timeouts` now show up in `--performance-log-check`.
- OpenGL now accepts `p010le/p016le` direct upload frames.
- Forced-copyback 10-bit playback now remains `copyback + direct upload` instead of `copyback + swscale to 8-bit`.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY34_OPENGL_RUNTIME_DIAGNOSTICS_AND_P010_UPLOAD.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 102: OpenGL present-mode override and gate script
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added an operator-facing OpenGL present-mode override via `MVP_OPENGL_PRESENT_MODE`.
- Exported requested/active present mode in `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to gate the main OpenGL paths in one command.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL auto present mode:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500
Key lines:
- renderer_opengl_present_mode_requested=auto
- renderer_opengl_present_mode_active=paced
- renderer_opengl_present_wait_timeouts=0
- result=PASS

OpenGL immediate present mode:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_PRESENT_MODE=immediate .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500
Key lines:
- [diag:opengl-present] requested=immediate active=immediate
- renderer_opengl_present_mode_requested=immediate
- renderer_opengl_present_mode_active=immediate
- result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. Present pacing control is most useful when the final runtime snapshot reports both the request and the real active mode; otherwise field logs remain ambiguous.
2. The OpenGL gate script is deliberately narrow: it validates the OpenGL paths directly instead of hiding them behind the broader `run_all_checks.ps1` umbrella.
3. Auto-generating the temporary 10-bit sample keeps the gate reproducible on machines that do not already carry a packaged HDR/10-bit asset.

### Result
- OpenGL present mode can now be toggled via environment variable.
- `--performance-log-check` now exposes the actual present mode state.
- OpenGL now has a reusable one-shot validation script that covers diagnostics, native, copy-back, 10-bit, and subtitle regressions.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 103: OpenGL HDR probe, quirk-table expansion, subtitle gate completion, and final gap matrix
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added HDR output capability probe fields to `--opengl-diagnostics`.
- Expanded the OpenGL quirk rule table for software and virtual GPU contexts.
- Expanded the OpenGL gate script to cover the current ASS sample suite.
- Closed the current OpenGL task table with a final gap matrix against mature players.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL diagnostics:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics
Key lines:
- opengl-diagnostics.hdr_output.probe_succeeded=true
- opengl-diagnostics.hdr_output.adapter_matched=true
- opengl-diagnostics.hdr_output.output_found=false
- opengl-diagnostics.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. The HDR probe closes the capability-detection half of display-level HDR work without pretending that HDR presentation is already implemented.
2. The quirk table is now shaped for long-term accumulation instead of one-off conditions.
3. The OpenGL gate now validates the current ASS sample suite as part of one reusable command, which is enough to mark the present subtitle sample bolster task as closed.

### Result
- The remaining OpenGL task-table items are now completed.
- What remains in the backlog is no longer task-table ambiguity, but a smaller set of known long-tail gaps: display-level HDR output, ICC/3D LUT color management, fuller libass parity, and a larger quirk corpus.

### Files
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/analysis/PLAYERCORE_DAY36_OPENGL_HDR_PROBE_QUIRK_TABLE_AND_GAP_MATRIX.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

### 2026-03-25 Update: OpenGL hotkey repeat suppression and interactive OSD
- Root cause confirmed in `src/render/opengl_video_renderer.cpp`: OpenGL hotkeys still accepted `SDL_KEYDOWN repeat`, and OSD wakeup only forced redraw while paused.
- Added repeat suppression for OpenGL hotkeys so a single press no longer expands into repeated seek/volume/fullscreen requests.
- Changed hotkey OSD wakeup to `requestRedraw()` and added initial OSD visibility on renderer startup.
- Added OpenGL mouse interaction for progress/volume bars: move to wake OSD, click/drag progress for seek preview, click/drag volume for live volume change.
- Shared the control layout between OSD drawing and hit-testing so the bottom panel geometry stays consistent.
- Validation: `cmake --build build --config Release --target modern-video-player` PASS, `run_opengl_checks.ps1` PASS.
- Manual smoke focus: `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe .\a.mp4`, then test `Space`, `Left/Right`, `Up/Down`, mouse move, progress drag, and volume drag.

## Issue 105: ASS transform transition parser/runtime support
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added parser/model support for ASS `\t(...)` transitions.
- Added transition diagnostics output to `--subtitle-style-check`.
- Applied transition-evaluated styles in both OpenGL and D3D11 subtitle renderers.
- Added a dedicated transition validation sample and folded it into the OpenGL gate.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Subtitle parser/diagnostics:
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key lines:
- subtitle-style-check.item0.animation.transition_count=1
- subtitle-style-check.item0.animation.transition0.property_names="primary_color,outline_color,outline_x,outline_y,shadow_x,shadow_y,scale_x,scale_y,rotation_z"
- subtitle-style-check.item1.animation.transition0.accel=0.45
- subtitle-style-check.item2.animation.transition0.target.has_clip=true
- subtitle-style-check.result=PASS

OpenGL runtime regression:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key line:
- delay-adjust-check.result=PASS

D3D11 runtime regression:
$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key line:
- delay-adjust-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- [7/12] OpenGL subtitle transform transition regression
- [12/12] OpenGL subtitle style regression: opengl_ass_transform_transition_validation.ass
- OpenGL gate result: PASS
```

### Analysis Notes
1. The real blocker was parser structure, not shader capability: `\t(...)` needs nested-parenthesis-safe extraction and top-level comma splitting before renderer work even matters.
2. Transition support had to be added to both OpenGL and D3D11, because subtitle semantics should not diverge by renderer backend.
3. The new sample intentionally mixes timed transform, acceleration-only transform, and nested `\clip(...)` inside `\t(...)` so the regression is not limited to the easiest case.

### Result
- The subtitle stack now covers a substantial additional ASS semantics slice beyond move/fade/karaoke/vector basics.
- `--subtitle-style-check` can now explain transition timing and target state in machine-readable form.
- OpenGL gate coverage now includes the transition sample, while D3D11 has a direct runtime regression for the same asset.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 106: OpenGL bottom-bar player chrome
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Upgraded the OpenGL path from a minimal OSD into a bottom-bar player UI.
- Added a clickable play/pause button, segmented current/total time text, and larger seek/volume interaction zones.
- Added hover-aware visibility so the bar stays up while hovered and auto-hides with fade-out during idle playback.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. The OpenGL UI gap was no longer a rendering-backend problem, but a control-chrome problem: the interaction model was still shaped like an OSD, not a player surface.
2. Time text was implemented with lightweight segmented glyphs instead of a new font dependency so the OpenGL path stays self-contained and stable in the current build.
3. Automated checks can prove the OpenGL main path still works after the UI expansion, but they cannot prove hover timing or click feel; that still requires manual GUI smoke.

### Result
- OpenGL now presents a fuller player interface with persistent bottom-bar structure, not only transient rails.
- Play/pause is directly clickable inside the OpenGL window.
- Current/total time is now visible in the bottom bar and seek preview updates that time readout while dragging.
- The bar remains visible during hover/drag and auto-hides during idle playback.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY38_OPENGL_BOTTOM_BAR_PLAYER_CHROME.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 107: Container attachment font pipeline
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added media-scoped extraction and private registration for font attachments exposed by FFmpeg container streams.
- Wired attachment font registration into `PlayerCore::open()` and cleanup into session release.
- Added `--attachment-font-check <media_file>` as a machine-readable regression command.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8
Result: PASS

Attachment sample generation:
ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -c copy -attach C:\Windows\Fonts\arial.ttf -metadata:s:t mimetype=application/x-truetype-font -metadata:s:t:0 filename=attachment-font-check.ttf .\build\tmp\attachment-font-check.mkv
Result: PASS

Attachment font check:
.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
Key lines:
- attachment-font-check.open_ok=true
- attachment-font-check.attachment_streams=1
- attachment-font-check.extracted_file_count=1
- attachment-font-check.registered_file_count=1
- attachment-font-check.invalid_attachment_stream_count=0
- attachment-font-check.result=PASS

Cleanup:
- cache directory removed after releaseMediaAttachmentFonts(...)
```

### Analysis Notes
1. The missing capability was in the media-open stage, not the ASS parser: container fonts never entered the process before this change.
2. Session-scoped registration is the correct ownership model because attachment fonts belong to the opened media, not to a subtitle sidecar directory.
3. This closes the highest-value subtitle-font gap, but embedded subtitle-track playback remains separate work.

### Result
- Container attachment fonts now participate in the subtitle font path.
- External ASS loaded after media open can use fonts bundled inside the current media container.
- The project now has a direct regression command for attachment extraction, registration, and cleanup.

### Files
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY39_ATTACHMENT_FONT_PIPELINE.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 2026-03-25 OpenGL CPU / GPU / driver optimization matrix doc update
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Consolidated the current OpenGL tuning strategy into one plan document.
- Split the strategy by CPU layer, GPU path selection, and driver/adapter rule handling.
- Added a release-oriented default strategy table for NVIDIA / AMD / Intel plus validation commands.

### Log
```text
Document added:
- docs/plans/OPENGL_CPU_GPU_DRIVER_OPTIMIZATION_MATRIX.md

Coverage:
- startup diagnostics signals
- runtime diagnostics signals
- CPU / GPU / driver layered matrix
- NVIDIA / AMD / Intel default strategy table
- vendor-specific validation commands
- generic OpenGL gate commands
```

### Analysis Notes
1. This document intentionally describes the current codebase behavior, not a hypothetical future policy.
2. Vendor differences are currently concentrated in native interop / driver rule handling, not in CPU-side thread-count hardcoding.
3. AMD remains conservative by design in the current rule table; Intel remains auto-probe first; NVIDIA remains native-first when diagnostics stay clean.

### Result
- The repository now has one stable document for OpenGL default-policy discussion and release review.
- Future quirk additions can extend this document instead of repeating the same strategy explanation in issue threads.

### Files
- `docs/plans/OPENGL_CPU_GPU_DRIVER_OPTIMIZATION_MATRIX.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 108: Embedded subtitle-track playback
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added automatic loading for supported embedded text subtitle streams during media open.
- Split subtitle ownership into external and embedded stores with `external > embedded` precedence.
- Added `--embedded-subtitle-check <media_file>` and folded embedded subtitle media into the OpenGL gate.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8
Result: PASS

Embedded ASS sample generation:
ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass -map 0:v -map 0:a? -map 1:0 -c:v copy -c:a copy -c:s ass .\build\tmp\embedded-ass-validation.mkv
Result: PASS

Embedded subtitle CLI:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
Key lines:
- embedded-subtitle-check.loaded=true
- embedded-subtitle-check.codec_name=ass
- embedded-subtitle-check.item_count=3
- embedded-subtitle-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- Embedded ASS subtitle CLI regression: PASS
- OpenGL embedded ASS subtitle playback regression: PASS
- Embedded text subtitle CLI regression: PASS
- OpenGL embedded text subtitle playback regression: PASS
- OpenGL gate result: PASS
```

### Analysis Notes
1. The correct integration point was `PlayerCore::open()`, because embedded subtitle tracks belong to the media session itself, not to a later external-subtitle action.
2. Reusing `SubtitleItem` instead of introducing a second subtitle representation keeps OpenGL and D3D11 subtitle semantics aligned.
3. The important ownership rule is `external > embedded`; otherwise loading and clearing a sidecar subtitle would produce unstable fallback behavior.
4. Validating only ASS was not enough, so the OpenGL gate now also generates and checks an embedded `mov_text` sample to cover the plain-text decode path.

### Result
- Embedded text subtitle tracks now load automatically on normal media open.
- External subtitle loading still overrides the embedded track, and clearing the external subtitle restores the embedded one.
- The OpenGL gate now exercises both embedded ASS and embedded text subtitle media instead of only sidecar subtitle files.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## 闂 99: OpenGL 鍚姩鏈熷崱姝诲疄涓洪粯璁?WASAPI 绔偣闃诲
**鏃ユ湡**: 2026-03-25
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛鍙嶉锛氫娇鐢?OpenGL 閾捐矾鎾斁瑙嗛鏃讹紝鎾斁绾?1-2 绉掑悗绋嬪簭浼氬崱姝汇€?- 鏈疆闇€瑕佸厛鍒ゆ柇杩欐槸 OpenGL render/present 姝婚攣锛岃繕鏄惎鍔ㄦ湡鍏朵粬瀛愮郴缁熼樆濉炪€?
### 鏃ュ織杈撳嚭
```text
Before fix:
2026-03-25 17:38:06.634 [INFO] OpenGL renderer initialized: window=1728x972
2026-03-25 17:38:14.635 [WARNING] Audio output init failed, continuing with video-only playback
Error: Could not open audio device: WASAPI can't find requested audio endpoint: 鎵句笉鍒板厓绱犮€?
After fix:
2026-03-25 17:59:13.711 [INFO] OpenGL renderer initialized: window=1728x972
2026-03-25 17:59:13.711 [WARNING] Audio output preflight skipped SDL_OpenAudioDevice strategy=skip-no-default-render-endpoint elapsed_ms=0 detail=skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI
performance-log-check.audio_device_open_attempted=false
performance-log-check.audio_init_latency_ms=0
performance-log-check.audio_init_strategy=skip-no-default-render-endpoint
performance-log-check.audio_init_detail=skipped SDL_OpenAudioDevice: no default or active render endpoint for WASAPI
performance-log-check.result=PASS
```

### 鍒嗘瀽璁板綍
- OpenGL renderer 鍜?context 宸茬粡鍦?`17:38:06.634` 鍒濆鍖栧畬鎴愶紝璇存槑鍗＄偣涓嶅湪 OpenGL startup銆?- 鐪熸鐨勯暱鏃堕棿绌虹獥鍙戠敓鍦?`PlayerCore::open() -> AudioPlayer::init() -> SDL_OpenAudioDevice(nullptr, ...)`銆?- 褰撳墠鏈哄櫒缂哄け榛樿 `WASAPI` render endpoint锛孲DL 鎵撳紑榛樿杈撳嚭璁惧鏃朵細鍚屾闃诲鏁扮鍚庢墠澶辫触銆?- 鐢变簬闃诲鍙戠敓鍦?open 涓荤嚎绋嬶紝UI/鎾斁鐘舵€佷竴璧峰仠浣忥紝鐢ㄦ埛鎵嶄細鎶婄幇璞¤鍒や负 OpenGL 鍗℃銆?
### 澶勭悊缁撴灉
- `AudioPlayer::init()` 鏀逛负鍏堟墽琛?Windows 榛樿 render endpoint preflight銆?- 榛樿/`wasapi` 闊抽杈撳嚭涓旀病鏈夐粯璁ゆ垨 active render endpoint 鏃讹細
  - 涓嶅啀杩涘叆 `SDL_OpenAudioDevice`
  - 鐩存帴鍥炴姤 `skip-no-default-render-endpoint`
  - 瑙嗛鏂囦欢绔嬪埢璧?`video-only fallback`
- `PlayerCore` / diagnostics / 鎾斁绫?CLI 宸叉柊澧為煶棰戝垵濮嬪寲绛栫暐鍜岃€楁椂杈撳嚭銆?- Release build銆乣--opengl-diagnostics`銆乣--performance-log-check` 鍜?`tools/run_opengl_checks.ps1` 鍏ㄩ儴閫氳繃銆?
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
## Issue 109: Embedded subtitle multi-track selection UI + CLI
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed embedded subtitle multi-track control closure across core/UI/CLI:
  - OpenGL subtitle previous/next track controls + track state overlay
  - playback arg `--subtitle-track <stream_index>`
  - diagnostics commands `--embedded-subtitle-list` and `--embedded-subtitle-select-check`
- Kept `external > embedded` ownership policy and moved selection semantics to supported embedded subtitle codecs (`supported_codec`).

### Log
```text
Build:
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && msbuild build\modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64"
Result: PASS

Embedded subtitle list:
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-text-validation.mp4
Key lines:
- embedded-subtitle-list.best_stream_index=2
- embedded-subtitle-list.result=PASS

Embedded subtitle select:
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-text-validation.mp4 2
Key lines:
- embedded-subtitle-select-check.loaded=true
- embedded-subtitle-select-check.result=PASS

Embedded best-stream check:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-text-validation.mp4
Key lines:
- embedded-subtitle-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. The remaining gap after Day40 was no longer decoding but control surface completeness (multi-track selection path closure).
2. OpenGL track state now maps to supported subtitle tracks (`supported_codec`), avoiding confusing counts when unsupported subtitle codecs exist.
3. The new list/select CLI closes machine-readable observability for embedded multi-track behavior.

### Files
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 110: Embedded bitmap subtitle path + DirectWrite custom font collection
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Extended embedded subtitle capability from text-only closure to supported-codec closure (text + bitmap).
- Landed PGS/DVD bitmap subtitle decode/model/render consumption path.
- Landed DirectWrite custom subtitle font collection integration on top of registered private fonts.
- Synced CLI diagnostics fields for bitmap and font-collection visibility.

### Log
```text
Build:
MSBuild.exe build/modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64
Result: PASS

CLI:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\embedded-ass-validation.mkv
Result: PASS

Gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Notes
1. Embedded subtitle selection/overlay policy is now aligned with `supported_codec`, avoiding the previous text-only interpretation mismatch.
2. Bitmap subtitle path is functionally closed in loader + renderer, but a wider real-media PGS/DVD sample corpus remains future regression debt.
3. Display-level HDR output bridge and ICC/3D LUT output management remain separate backlog tracks.

### Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY43_BITMAP_SUBTITLE_AND_DWRITE_COLLECTION.md`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 111: OpenGL HDR output policy + 3D LUT output baseline
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added OpenGL output-color policy controls for HDR output mode and 3D LUT loading.
- Wired output-stage 3D LUT parsing/upload/sampling into OpenGL final composition paths.
- Extended machine-readable diagnostics for OpenGL HDR bridge and LUT runtime state.
- Added dedicated regression command `--opengl-output-color-check`.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL output color check:
.\build\Release\modern-video-player.exe --opengl-output-color-check .\juren-30s.mp4 .\samples\lut\identity_2.cube 1200
Key lines:
- opengl-output-color-check.hdr_bridge_mode=auto
- opengl-output-color-check.output_lut_configured=true
- opengl-output-color-check.output_lut_active=true
- opengl-output-color-check.result=PASS

OpenGL performance log:
$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_3DLUT_FILE='.\samples\lut\identity_2.cube'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.renderer_opengl_hdr_bridge_mode=auto
- performance-log-check.renderer_opengl_output_lut_active=true
- performance-log-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. This round lands a practical output control plane (policy + LUT) but intentionally does not claim final display-HDR delivery closure.
2. The new diagnostics fields close observability for request/active state, which was the main blocker for stable HDR/LUT regression checks.
3. Full display-level HDR present still requires DXGI swapchain color-space/metadata output path, beyond this implementation batch.
4. ICC/profile-driven LUT generation and per-display dynamic binding remain future backlog.

### Files
- `src/render/opengl_video_renderer.cpp`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY44_OPENGL_HDR_OUTPUT_POLICY_AND_3DLUT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 112: OpenGL interaction freeze on mouse/keyboard/window events
**Date**: 2026-03-25
**Status**: Resolved

### Description
- User reported OpenGL playback freezing after mouse move/click, with hotkeys and maximize/minimize/fullscreen becoming unresponsive.
- Root cause traced to SDL event pumping on the OpenGL render thread instead of main-thread handling.
- Fixed by moving event pumping/fullscreen window operation to the `handleEvents()` main-thread path.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

OpenGL playback diagnostics:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Result: performance-log-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. D3D11 path already pumps SDL events on `handleEvents()` main-thread path; OpenGL path drifted into render-thread pumping and diverged behavior.
2. Interaction-triggered freezes were consistent with thread-affinity/message-pump stall patterns rather than decoder/render throughput bottlenecks.
3. Render thread should remain frame/present focused; window/event transitions should stay in main-thread event path.
4. Automated gate cannot fully replace real desktop-interaction smoke; final closure still requires short manual GUI stress pass.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY45_OPENGL_EVENT_THREAD_AFFINITY_FREEZE_FIX.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 113: Cross-platform master tasklist consolidation
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added one execution-ready master tasklist for all cross-platform work.
- Unified task IDs, phase boundaries, milestone gates, and acceptance criteria into a single plan entry.
- Synced matching analysis and records docs so future execution can be tracked from one baseline.

### Log
```text
Master plan added:
docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md

Analysis added:
docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md

Plans index updated:
docs/plans/README.md
```

### Analysis Notes
1. Existing plan docs had strong content but were fragmented by purpose; execution required repeated manual merging.
2. A master task board with stable IDs is needed for continuous tracking, review, and staged delivery.
3. Linux-first sequencing remains unchanged: strategy extraction -> Linux MVP -> subtitle/font/bitmap maturity -> HDR/ICC/LUT -> CI/package convergence.

### Validation
```text
rg -n "CROSS_PLATFORM_MASTER_TASKLIST" docs/plans docs/analysis docs/records
PASS

cmake --build build --config Release --target modern-video-player
PASS
```

### Files
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`








