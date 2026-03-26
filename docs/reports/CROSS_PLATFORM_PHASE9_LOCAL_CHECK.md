# Cross-Platform Phase 9 Local Check
Date: 2026-03-26
Scope: `CP-901 ~ CP-905`

## Build
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
```
Result: PASS

## Driver Sample Capture
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -OutputCsvPath .\docs\reference\DRIVER_QUIRK_SAMPLE_LIBRARY.csv -HostLabel local-win-gtx1080 -Notes "Local Windows validation sample after CP-801 and Phase-9 closure"
```
Key lines:
- `driver-quirk-sample.record_count=2`
- `driver-quirk-sample.record=OpenGL:NVIDIA GeForce GTX 1080:32.0.15.6094`
- `driver-quirk-sample.record=D3D11:NVIDIA GeForce GTX 1080:32.0.15.6094`

## Unified Counter Surface
```powershell
$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
- `performance-log-check.scheduler_render_wait_ms=995`
- `performance-log-check.runtime_drop_total=0`
- `performance-log-check.renderer_d3d11.present_count=64`
- `performance-log-check.renderer_d3d11.present_avg_ms=0.0717656`
- `performance-log-check.result=PASS`

## Windows Packaging Script
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir build -Configuration Release -SkipBuild
```
Key lines:
- `windows-package.file=D:\VSProject\sssssssssssssss\modern-video-player\build\modern-video-player-1.0.0-rc1-windows-x64.zip`

## Linux Gate Script
```powershell
bash -n tools/run_linux_mvp_checks.sh
```
Result:
- Could not be executed on this workstation because `bash.exe` resolves to the Windows Subsystem for Linux launcher and WSL is not installed locally.
- Script content was reviewed and synced with the Linux Phase-4 command surface and required-pattern model.

## CI Matrix / Packaging Readiness Checklist
- Windows gate workflow leg added in `.github/workflows/cross-platform-gate.yml`.
- Linux gate workflow leg added in `.github/workflows/cross-platform-gate.yml`.
- Windows package helper added: `tools/package_windows.ps1`
- Linux package helper retained and reused: `tools/package_linux.sh`
- Artifact upload configured for:
  - Windows ZIP
  - Linux `DEB/TGZ`
  - driver sample CSV
  - logs

## Residual Gap
- Linux gate and Linux packaging were not executed end-to-end locally on this Windows workstation.
- CI workflow is authored and reviewed but not runnable from the local machine.
