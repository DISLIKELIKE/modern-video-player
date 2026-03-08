# Sample Media Layout

This folder stores local regression media samples. By default, media files are ignored by git.

## Directory layout

- `samples/mp4/`
- `samples/mkv/`
- `samples/webm/`
- `samples/flv/`
- `samples/ts/`
- `samples/mov/`
- `samples/avi/`
- `samples/m2ts/`
- `samples/subtitle/`
- `samples/streaming/`

## File naming convention

Use this template:

`<tag>__<video_codec>_<audio_codec>__<width>x<height>__<fps>fps__<channels>ch[__maN].<ext>`

Examples:

- `demo__hevc_aac__1920x1080__30fps__2ch.mp4`
- `demo__h264_aac__1920x1080__60fps__2ch.mp4`
- `stress100m__h264_aac__1920x1080__60fps__2ch.mp4`
- `demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv`
- `demo__av1_flac__3840x2160__60fps__8ch__ma2.mkv`

Field notes:

- `tag`: short source label, e.g. `demo`, `cameraA`, `prodline`.
- `video_codec`: `h264`, `hevc`, `vp9`, `av1`, etc.
- `audio_codec`: `aac`, `ac3`, `opus`, `flac`, etc.
- `maN` (optional): multi-audio-track count, e.g. `ma2`.

## How to register new samples

1. Put media file under the matching container folder.
2. Add one row to `tools/format_regression/format_samples.csv`.
3. Run:
   - `.\tools\format_regression\run_format_regression.ps1`
   - or `.\tools\run_all_checks.ps1`

## Subtitle sync sample

- `samples/subtitle/subtitle_seek_sync_sample.srt` is used by:
  - `.\build\Debug\modern-video-player.exe --subtitle-sync-check samples\subtitle\subtitle_seek_sync_sample.srt`

## Playlist flow check sample set

- 5-file continuous playlist acceptance (`1.4.2`) can be checked by:
  - `.\build\Debug\modern-video-player.exe --playlist-flow-check <5 media files...>`
- Example command is recorded in:
  - `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`

## Bootstrap sample pack

To download and prepare a local sample pack automatically:

`.\tools\download_test_samples.ps1`

The script will:

- download one public base clip into `samples/source/`
- generate all regression targets listed in `format_samples.csv`
- generate an extra `1080p60` stability sample at `samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4` (used by `--1080p60-check`)
- generate a `>80Mbps` stress sample at `samples/mp4/stress100m__h264_aac__1920x1080__60fps__2ch.mp4` (used by `--high-bitrate-check`)


## Streaming fixture sample set

- `samples/streaming/hls_local/sample.m3u8` with `segment000.ts..segment002.ts` is used by:
  - `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`
  - `.\build\Debug\modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`
- This fixture is intentionally tiny and byte-oriented; it validates real HTTP segment download and buffering, not media decoding.
- `samples/streaming/abr_local/hls/*` and `samples/streaming/abr_local/dash/*` are used by:
  - `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`
  - `.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`
  - `.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`
- These ABR fixtures validate HLS master playlist / DASH MPD parsing, bitrate selection, and per-variant segment download; they do not validate decode or playback integration.
