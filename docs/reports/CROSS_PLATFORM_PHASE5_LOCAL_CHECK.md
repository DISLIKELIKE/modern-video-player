# CROSS_PLATFORM_PHASE5_LOCAL_CHECK

Date: 2026-03-26
Platform: Windows (local)
Scope: CP-501 ~ CP-506

## Commands

```powershell
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release

.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid
.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4
.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --settings-persistence-check

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"
```

## Result

- Build: PASS
- `embedded-subtitle-policy-check.result=PASS`
- `subtitle-ownership-check.result=PASS`
- `attachment-font-check.result=PASS`
- `directwrite-font-collection-check.result=PASS`
- `settings-persistence-check.result=PASS`
- OpenGL gate script: PASS (`18/18` checks)

## Key Signals

- `embedded-subtitle-policy-check.selected_loaded=true`
- `subtitle-ownership-check.external_precedence_ok=true`
- `subtitle-ownership-check.fallback_to_embedded_ok=true`
- `attachment-font-check.registered_file_count=1`
- `directwrite-font-collection-check.collection_created=true`
- `directwrite-font-collection-check.factory3_available=true`

## Notes

- Local host has no active WASAPI default endpoint; audio preflight warning is expected and non-blocking for this scope.
- Linux attachment-font runtime behavior (`fontconfig` branch) still requires Linux-host execution for full cross-platform runtime closure.

## Conclusion

- `CP-501 ~ CP-506` implementation is build- and gate-closed on local Windows validation path.
- Remaining cross-platform risk is Linux runtime verification of the new `fontconfig` path.
