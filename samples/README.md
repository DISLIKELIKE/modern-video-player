# Sample Media Layout

This folder stores local regression media samples. By default, media files are ignored by git.

## Directory layout

- `samples/mp4/`
- `samples/mkv/`
- `samples/webm/`
- `samples/flv/`
- `samples/ts/`

## File naming convention

Use this template:

`<tag>__<video_codec>_<audio_codec>__<width>x<height>__<fps>fps__<channels>ch[__maN].<ext>`

Examples:

- `demo__hevc_aac__1920x1080__30fps__2ch.mp4`
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

## Bootstrap sample pack

To download and prepare a local sample pack automatically:

`.\tools\download_test_samples.ps1`

The script will:

- download one public base clip into `samples/source/`
- generate all regression targets listed in `format_samples.csv`
