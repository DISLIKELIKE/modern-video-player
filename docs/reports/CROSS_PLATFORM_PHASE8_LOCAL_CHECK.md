# Cross-Platform Phase 8 Local Check
Date: 2026-03-26
Scope: `CP-801 ~ CP-805`

## Build
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
```
Result: PASS

## D3D11 Diagnostics
```powershell
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```
Key lines:
- `d3d11-diagnostics.probe_succeeded=true`
- `d3d11-diagnostics.hdr_output.probe_succeeded=true`
- `d3d11-diagnostics.hdr_output.output_found=false`
- `d3d11-diagnostics.result=PASS`

## D3D11 HDR Output Check
```powershell
.\build\Release\modern-video-player.exe --d3d11-hdr-output-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
- `d3d11-hdr-output-check.renderer_backend=D3D11`
- `d3d11-hdr-output-check.hdr_present_decision=probe-unavailable`
- `d3d11-hdr-output-check.present_count=64`
- `d3d11-hdr-output-check.result=PASS`

## Manual Cube Output Check
```powershell
.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200
```
Key lines:
- `output_lut_source=cube`
- `output_lut_active=true`
- `output_display_device_name=WinDisc`
- `result=PASS`

## Auto ICC Output Check
```powershell
.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```
Key lines:
- `output_lut_source=icc-display`
- `output_icc_profile_available=true`
- `output_icc_profile_path=C:\Windows\system32\spool\drivers\color\sRGB Color Space Profile.icm`
- `output_icc_profile_description=sRGB IEC61966-2.1`
- `result=PASS`

## OpenGL Diagnostics
```powershell
.\build\Release\modern-video-player.exe --opengl-diagnostics
```
Key lines:
- `opengl-diagnostics.output_display.binding_succeeded=true`
- `opengl-diagnostics.output_display.icc_profile_available=true`
- `opengl-diagnostics.output_display.icc_profile_source=display-auto`
- `opengl-diagnostics.result=PASS`

## OpenGL Gate
```powershell
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
```
Result: PASS (`25/25`)

## Residual Gap
- The current validation host did not expose a bindable DXGI output, so Phase 8 validation does not include a locally observed `hdr_present_active=true` result.
