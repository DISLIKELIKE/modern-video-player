# Cross-platform Linux workflow build error fix local check

Date: 2026-03-26  
Host: Windows workstation (no local Linux runtime)

## 1. Scope
- Verify code fixes for Linux workflow compile blockers:
  - libass event timestamp type mismatch
  - OpenGL helper/type visibility for non-Windows build path

## 2. Commands and Results

### Workflow failure inspection
```text
gh run view 23601824744 --job 68733664519 --log
gh run view 23601841417 --json status,conclusion,event,jobs
```
Result: FAIL reproduced from remote logs (`Build Linux Release`, exit code 2) and root errors identified.

### Build
```text
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### D3D11 diagnostics smoke check
```text
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Key lines:
```text
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.result=PASS
```
Result: PASS

### Performance regression
```text
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
```text
performance-log-check.renderer_backend=D3D11
performance-log-check.result=PASS
```
Result: PASS

## 3. Conclusion
- The identified Linux compile blockers are patched in source.
- Local Windows regression surface remains stable.
- Final Linux lane confirmation still requires push + GitHub Actions rerun.
