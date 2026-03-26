# Cross-platform Linux gate strict optional local check

Date: 2026-03-26  
Host: Windows 10/11 workstation (no WSL/Linux runtime)

## 1. Scope
- Validate this round Linux gate hardening changes:
  - `tools/run_linux_mvp_checks.sh` strict optional checks + CP-508 fixture generation contract
  - `.github/workflows/cross-platform-gate.yml` Linux lane strict invocation contract
- Run local build and key runtime regressions to ensure no side effects.

## 2. Commands and Results

### Build
```text
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
```
Result: PASS

### Shared regression sanity
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Result: PASS (`performance-log-check.result=PASS`)

```text
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
```
Result: PASS (`embedded-subtitle-live-packet-check.result=PASS`)

```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Result: PASS (`d3d11-diagnostics.result=PASS`)

### Linux shell/static check
```text
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
```
Result: PASS (script syntax valid)

### Linux runtime check status
- Runtime dispatch check:
```text
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
```
Result: expected FAIL on non-Linux host (`This gate script only supports Linux.`)
- Full Linux gate runtime PASS evidence still requires Linux host/CI.

## 3. Conclusion
- Build and Windows-side shared regressions remain stable after Linux gate hardening changes.
- Linux runtime/script execution evidence still requires:
  - CI Linux runner execution, or
  - a real Linux host.
