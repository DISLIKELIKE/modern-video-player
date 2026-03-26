# PLAYERCORE Day61: Linux workflow build error closure

Date: 2026-03-26  
Status: Done (code fix landed, Windows local checks passed, Linux runner recheck pending)

## 1. Problem
- GitHub Actions Linux lane (`cross-platform-gate`) failed in `Build Linux Release` on runs:
  - `23601824744` (push)
  - `23601841417` (workflow_dispatch)
- Main compile failures:
  - `src/subtitle/libass_probe.cpp`: `std::max(int, long long)` template deduction mismatch.
  - `src/render/opengl_video_renderer.cpp`: missing types/functions (for example `OpenGLHdrBridgeDecision`, `OpenGLPresentMode`, `trimAscii`, `resolveOutputDisplayBinding`) during Linux build.

## 2. Root Cause
1. `ASS_Event::Start`/`ASS_Event::Duration` are wider integer types on Linux libass headers; direct `std::max(0, event.Start)` mixed `int` and `long long`.
2. A large helper block in `opengl_video_renderer.cpp` was guarded by `#if defined(_WIN32)` while class-level cross-platform code still referenced some of those helpers.

## 3. Solution
- `libass_probe`:
  - Added safe clamp helper to convert libass event timestamps to `int` with bounds.
- `opengl_video_renderer`:
  - Added `#if !defined(_WIN32)` helper block (same symbol names used by cross-platform class path):
    - subtitle animation probe helpers
    - `OpenGLPresentMode` / `OpenGLHdrBridgeMode` / `OpenGLHdrBridgeDecision`
    - env parsing helpers (`readOpenGLForceInitFail`, `readOpenGLPresentMode`, `readOpenGLHdrBridgeMode`, etc.)
    - `trimAscii`, swap interval description helpers
    - `OutputDisplayBindingState` and Linux `resolveOutputDisplayBinding`

## 4. Outcome
- Windows local build and smoke checks passed after patch.
- Linux build error set identified from workflow logs is addressed in source.

## 5. Remaining Gap
- Final Linux runtime/build proof still requires remote Linux runner rerun after push.
