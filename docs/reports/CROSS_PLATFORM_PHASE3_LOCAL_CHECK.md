# CROSS_PLATFORM_PHASE3_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-301 ~ CP-305

## Commands

```powershell
cmake -S . -B build
cmake --build build --config Release --target modern-video-player
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"

cmake -S . -B build_nod3d11 -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON
cmake --build build_nod3d11 --config Release --target modern-video-player
.\build_nod3d11\Release\modern-video-player.exe --d3d11-diagnostics
.\build_nod3d11\Release\modern-video-player.exe --opengl-diagnostics

cmake -S . -B build_noopengl -DENABLE_OPENGL_RENDERER=OFF -DENABLE_D3D11_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_D3D11VA=ON
cmake --build build_noopengl --config Release --target modern-video-player
.\build_noopengl\Release\modern-video-player.exe --opengl-diagnostics
.\build_noopengl\Release\modern-video-player.exe --d3d11-diagnostics
```

## Result

- Default build: PASS
- Default runtime regressions:
  - `performance-log-check.result=PASS`
  - `renderer-fallback-check.result=PASS`
  - `interaction-freeze-check.result=PASS`
  - `OpenGL gate result: PASS`
- Switch matrix builds:
  - `build_nod3d11`: PASS
  - `build_noopengl`: PASS
- Startup diagnostics new keys present in default check:
  - `performance-log-check.startup_renderer_compiled_set=...`
  - `performance-log-check.startup_renderer_runtime_set=...`
  - `performance-log-check.startup_decoder_compiled_set=...`
  - `performance-log-check.startup_decoder_runtime_set=...`

## Notes

- `build_nod3d11` runtime check falls into known software decode blocker path in this repo baseline; this is tracked separately and does not block CP-301~305 switch/guard scope.
- Linux packaging script `tools/package_linux.sh` and CPack `DEB/TGZ` config are landed, but actual Linux packaging execution is pending Linux environment validation.

## Conclusion

- CP-301~CP-305 baseline is landed for build switch control, platform guarding, startup observability, Linux dependency declarations, and Linux package path setup.
