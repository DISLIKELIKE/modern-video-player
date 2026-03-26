# PLAYERCORE DAY51: CP-501 ~ CP-506 Subtitle/Font Platform Closure
Date: 2026-03-26
Status: Done (Windows local validation + Linux runtime follow-up needed)

## 1. Problem Statement
- Phase-5 closure tasks (`CP-501` to `CP-506`) were partially implemented but not fully closed:
  - policy-aware embedded subtitle selection had landed, but build was broken by a CLI/settings integration regression;
  - Linux attachment-font flow still had no runtime registration path in `subtitle_font_registry`;
  - DirectWrite custom font collection was integrated in renderers, but Windows gate script coverage was incomplete.
- Phase-4 report doc still had pending placeholders and needed real local execution notes.

## 2. Root Cause
- `main.cpp` attempted to mutate `const AppSettings`, which broke compile after policy persistence wiring.
- `subtitle_font_registry` only implemented private font registration for Windows (`AddFontResourceExW`), while non-Windows returned `false`.
- Windows OpenGL gate script (`tools/run_opengl_checks.ps1`) did not yet exercise `--directwrite-font-collection-check`.

## 3. Implementation Planner Used
1. Fix current compile blocker in `main.cpp`.
2. Complete Linux attachment-font register/rebuild/cleanup flow in `src/subtitle/subtitle_font_registry.cpp`.
3. Add DirectWrite custom font collection check into Windows OpenGL gate script.
4. Build + run Phase5 CLI validations and gate script.
5. Sync phase docs + records + tasklist statuses.

## 4. Landed Changes
- Build unblock:
  - Removed invalid write to `app_settings.embedded_subtitle_selection_policy` in `src/main.cpp`.
- `CP-505` Linux attachment-font closure:
  - Added Linux `fontconfig` branch in `src/subtitle/subtitle_font_registry.cpp`:
    - register path: `FcConfigAppFontAddFile` + `FcConfigBuildFonts`
    - cleanup path: rebuild app font collection from remaining registry entries after media-session release
  - Tightened registration-state locking for media attachment registration flow.
- `CP-506` gate coverage:
  - Added `DirectWrite subtitle custom font collection` check entry to `tools/run_opengl_checks.ps1`:
    - command: `--directwrite-font-collection-check <media_file>`
    - required patterns include `factory_ok=true` and `result=PASS`.
- Phase docs and records synchronized for Phase5 closure.

## 5. Result Against CP IDs
- `CP-501`: done (language + forced/SDH policy + settings persistence + runtime apply path).
- `CP-502`: done (subtitle track catalog/current selection/switch feedback output in OpenGL overlay path).
- `CP-503`: done (CLI args + policy check command + usage/docs wiring).
- `CP-504`: done (`external > embedded` ownership and clear-external fallback check command).
- `CP-505`: done in code path (Linux attachment font extract/register/release flow now implemented via fontconfig integration).
- `CP-506`: done (DirectWrite custom collection integrated and now included in Windows gate coverage).

## 6. Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release`
  - result: PASS
- Phase5 CLI checks:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid`
  - `.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4`
  - `.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv`
  - `.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv`
  - `.\build\Release\modern-video-player.exe --settings-persistence-check`
  - result: all PASS
- Windows OpenGL gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"`
  - result: PASS (`18/18` checks, including new DirectWrite check).

## 7. Remaining Risks
- Linux runtime validation is still pending:
  - this host is Windows, so new `fontconfig` branch is compile-reviewed but not runtime-verified on Linux.
- Phase4 Linux-only check commands correctly return platform mismatch on Windows; full Linux PASS still requires Linux host execution.
