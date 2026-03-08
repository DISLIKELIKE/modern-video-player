# AB_REPEAT_LOCAL_CHECK

- Date: 2026-03-08
- Build: `cmake --build build --config Debug --target modern-video-player`

## Command

```powershell
.\build\Debug\modern-video-player.exe --ab-repeat-check .\juren-30s.mp4
```

## Output

```text
ab-repeat-check.path=.\juren-30s.mp4
ab-repeat-check.open_ok=true
ab-repeat-check.entered_playback_loop=true
ab-repeat-check.a_set=true
ab-repeat-check.b_set=true
ab-repeat-check.enabled_after_set=true
ab-repeat-check.cleared=true
ab-repeat-check.point_a=0.597333
ab-repeat-check.point_b=2.88
ab-repeat-check.loop_before=2.88
ab-repeat-check.loop_after=0.512
ab-repeat-check.loop_observed=true
ab-repeat-check.result=PASS
```

## Regression

```powershell
.\build\Debug\modern-video-player.exe --settings-persistence-check
.\build\Debug\modern-video-player.exe --chapter-nav-check $env:TEMP\mvp_chapter_sample.mp4
.\build\Debug\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
.\build\Debug\modern-video-player.exe --windows-backend-check .\juren-30s.mp4
```

All commands return `PASS`.

## Conclusion

- `4.2 A-B Repeat` 本地验收通过。
