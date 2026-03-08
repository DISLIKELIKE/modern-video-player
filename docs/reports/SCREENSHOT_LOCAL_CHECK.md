# SCREENSHOT_LOCAL_CHECK

- Date: 2026-03-08
- Build: `cmake --build build --config Debug --target modern-video-player`

## Command

```powershell
.\build\Debug\modern-video-player.exe --screenshot-check .\juren-30s.mp4
```

## Output

```text
screenshot-check.path=.\juren-30s.mp4
screenshot-check.open_ok=true
screenshot-check.entered_playback_loop=true
screenshot-check.paused_before_request=true
screenshot-check.request_ok=true
screenshot-check.captured=true
screenshot-check.file_exists=true
screenshot-check.output=D:\C++files\VSProjects\modern-video-player\modern-video-player\screenshots\screenshot_20260308_160117_208.ppm
screenshot-check.result=PASS
```

## Regression

```powershell
.\build\Debug\modern-video-player.exe --settings-persistence-check
.\build\Debug\modern-video-player.exe --ab-repeat-check .\juren-30s.mp4
```

All commands return `PASS`.

## Conclusion

- `4.3 截图` 本地验收通过。
- 暂停态下可以稳定保存当前画面，不再依赖继续送帧。
