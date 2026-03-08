# CHAPTER_NAV_LOCAL_CHECK

- Date: 2026-03-08
- Build: `cmake --build build --config Debug --target modern-video-player`

## Sample Preparation

- Source media: `./juren-30s.mp4`
- Chapter sample: `%TEMP%/mvp_chapter_sample.mp4` (3 chapters: 0-10s, 10-20s, 20-30s)
- Generation command:

```powershell
$metaPath = Join-Path $env:TEMP 'mvp_chapters.txt'
$outPath = Join-Path $env:TEMP 'mvp_chapter_sample.mp4'
$lines = @(
  ';FFMETADATA1',
  '[CHAPTER]','TIMEBASE=1/1000','START=0','END=10000','title=Chapter 1',
  '[CHAPTER]','TIMEBASE=1/1000','START=10000','END=20000','title=Chapter 2',
  '[CHAPTER]','TIMEBASE=1/1000','START=20000','END=30000','title=Chapter 3'
)
Set-Content -Path $metaPath -Value $lines -Encoding Ascii
.\external\ffmpeg\bin\ffmpeg.exe -y -i .\juren-30s.mp4 -i $metaPath -map_metadata 1 -codec copy $outPath
```

## Command

```powershell
.\build\Debug\modern-video-player.exe --chapter-nav-check $env:TEMP\mvp_chapter_sample.mp4
```

## Output

```text
chapter-nav-check.path=C:\Users\31133\AppData\Local\Temp\mvp_chapter_sample.mp4
chapter-nav-check.open_ok=true
chapter-nav-check.chapter_count=3
chapter-nav-check.entered_playback_loop=true
chapter-nav-check.next_invoked=true
chapter-nav-check.prev_invoked=true
chapter-nav-check.before=0.426667
chapter-nav-check.after_next=10.0267
chapter-nav-check.after_prev=0.405333
chapter-nav-check.moved_forward=true
chapter-nav-check.moved_backward=true
chapter-nav-check.result=PASS
```

## Conclusion

- `4.1 章节导航（上一章/下一章）` 本地验收通过。
