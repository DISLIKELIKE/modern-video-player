# RENDER_FALLBACK_LOCAL_CHECK

Date: 2026-03-08
Platform: Windows (local)
Build: Debug

## Command

```powershell
.\build\Debug\modern-video-player.exe --renderer-fallback-check juren-30s.mp4
```

## Result

- renderer-fallback-check.open_ok=true
- renderer-fallback-check.renderer_backend=SoftwareSDL
- renderer-fallback-check.decoder_backend=D3D11VA
- renderer-fallback-check.entered_playback_loop=true
- renderer-fallback-check.fallback_to_sdl=true
- renderer-fallback-check.result=PASS

## Conclusion

- Forced D3D11 renderer init failure can be downgraded to SoftwareSDL automatically.
- Playback loop can continue after fallback.
