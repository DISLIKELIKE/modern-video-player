# Settings Persistence Local Check

Generated: 2026-03-08
Task: `1.4.3 设置重启后可恢复`

## Command

```powershell
.\build\Debug\modern-video-player.exe --settings-persistence-check
```

## Output

```text
settings-persistence-check.path=C:\Users\31133\AppData\Local\Temp\modern_video_player_settings_check.ini
settings-persistence-check.volume_ok=true
settings-persistence-check.speed_ok=true
settings-persistence-check.audio_delay_ok=true
settings-persistence-check.subtitle_delay_ok=true
settings-persistence-check.decode_pref_ok=true
settings-persistence-check.resume_ok=true
settings-persistence-check.index_ok=true
settings-persistence-check.hotkey_ok=true
settings-persistence-check.result=PASS
```

## Conclusion

- `PASS`: settings round-trip across save/load preserves volume, speed, audio/subtitle delay, decode preference, resume flag, playlist index, and hotkey mapping.
