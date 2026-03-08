# WINDOWS_BACKEND_LOCAL_CHECK

Date: 2026-03-08
Platform: Windows (local)
Build: Debug

## Commands

```powershell
.\build\Debug\modern-video-player.exe --windows-backend-session-check juren-30s.mp4 hard
.\build\Debug\modern-video-player.exe --windows-backend-session-check juren-30s.mp4 soft
.\build\Debug\modern-video-player.exe --windows-backend-check juren-30s.mp4
```

## Result

- windows-backend-check.hard.open_ok=true
- windows-backend-check.hard.entered_playback_loop=true
- windows-backend-check.hard.renderer_backend=D3D11
- windows-backend-check.hard.decoder_backend=D3D11VA
- windows-backend-check.hard.mode_ok=true
- windows-backend-check.hard.exit_code=0
- windows-backend-check.soft.open_ok=true
- windows-backend-check.soft.entered_playback_loop=true
- windows-backend-check.soft.renderer_backend=D3D11
- windows-backend-check.soft.decoder_backend=Software
- windows-backend-check.soft.mode_ok=true
- windows-backend-check.soft.exit_code=0
- windows-backend-check.result=PASS

## Conclusion

- On Windows, both hardware decode (D3D11VA) and software decode (Software) checks pass.
- The aggregated check command exits reliably without the previous dual-session hang.
