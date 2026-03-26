# CROSS PLATFORM PHASE7 LOCAL CHECK

Date: 2026-03-26  
Scope: `CP-701 ~ CP-705`

## Commands
```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --linux-vaapi-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
```

## Result Summary
- Build: PASS
- `--performance-log-check`: PASS
  - `renderer_backend=D3D11`
  - `decoder_backend=D3D11VA`
  - no runtime-failure counters increased
- `--d3d11-diagnostics`: PASS (Windows baseline unchanged)
- `--linux-vaapi-fallback-check` on this host: expected non-Linux FAIL
  - `platform=NonLinux`
  - `platform_ok=false`
  - `result=FAIL`

## Scope Notes
- This workstation is Windows-only (no Linux runtime environment), so Linux host evidence remains pending.
- Phase-7 code-path closure in this report means:
  - compile/link integration complete;
  - command surfaces and fallback logic are wired;
  - Linux runtime `PASS` still requires Linux host execution.

## CP Mapping
- `CP-701`: unified hw device factory landed.
- `CP-702`: VAAPI runtime probing integrated into platform capabilities.
- `CP-703`: `VAAPI -> Software` fallback path integrated in decoder setup flow.
- `CP-704`: OpenGL + VAAPI copy-back baseline command/gate path added.
- `CP-705`: zero-copy deferred by design after evaluation; copy-back baseline retained.
