# Cross-platform workflow-log error fix local check

Date: 2026-03-26  
Host: Windows workstation (no local Linux runtime)

## 1. Scope
- Verify fixes for remaining workflow log compile errors (`AVFrame::duration` compatibility).

## 2. Commands and Results

### Build
```text
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Performance regression
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.renderer_backend=D3D11
performance-log-check.startup_platform=Windows
performance-log-check.result=PASS
```
Result: PASS

### Embedded subtitle live packet regression
```text
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
```
Key lines:
```text
embedded-subtitle-live-packet-check.stream_index=2
embedded-subtitle-live-packet-check.subtitle_packets_read=3
embedded-subtitle-live-packet-check.produced_output=true
embedded-subtitle-live-packet-check.result=PASS
```
Result: PASS

### Linux gate script syntax
```text
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
```
Result: PASS

## 3. Conclusion
- Workflow-log-derived compatibility fix is integrated and locally stable.
- Linux runner proof still requires remote CI execution after push.
