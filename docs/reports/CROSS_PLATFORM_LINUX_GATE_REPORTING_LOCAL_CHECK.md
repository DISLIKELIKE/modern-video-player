# Cross-platform Linux gate reporting local check

Date: 2026-03-26  
Host: Windows 10/11 workstation (no WSL/Linux runtime)

## 1. Scope
- Validate Linux gate reporting/artifact changes:
  - script-level report contract (`arg #10` / `MVP_LINUX_GATE_REPORT_FILE`)
  - CI Linux log/report artifact wiring

## 2. Commands and Results

### Script syntax check
```text
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
```
Result: PASS

### Runtime dispatch check on Windows host
```text
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
```
Result: expected FAIL (`This gate script only supports Linux.`)

### Build/regression sanity
```text
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Result: PASS

## 3. Conclusion
- Linux gate reporting contract and CI artifact wiring are landed.
- Full Linux gate PASS and generated report-file evidence still requires Linux host/CI execution.
