# Format Regression Report

Generated: 2026-03-23 22:46:33
Tool: run_format_regression.ps1
Executable: build/Release/modern-video-player.exe
Sample List: tools/format_regression/format_samples.csv

## Summary

- Total: 17
- PASS: 17
- PARTIAL: 0
- FAIL: 0
- SKIP: 0

## Details

| Sample | Exists | Open | Container | Video | Audio | Overall | Compatibility | Notes |
|---|---|---|---|---|---|---|---|---|
| juren-30s.mp4 | Yes | PASS | PASS (mp4) | PASS (h264) | PASS (aac) | PASS | PASS | repo baseline sample; probe reason: target is feasible |
| samples/mp4/demo__hevc_aac__1920x1080__30fps__2ch.mp4 | Yes | PASS | PASS (mp4) | PASS (hevc) | PASS (aac) | PASS | PASS | mp4 + h265; probe reason: target is feasible |
| samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv | Yes | PASS | PASS (mkv) | PASS (hevc) | PASS (ac3) | PASS | PASS | mkv h265 + multi-audio; probe reason: target is feasible, prefer hardware decode, prefer D3D11 renderer, verify 6ch output path |
| samples/webm/demo__vp9_opus__1920x1080__30fps__2ch.webm | Yes | PASS | PASS (webm) | PASS (vp9) | PASS (opus) | PASS | PASS | webm vp9 + opus; probe reason: target is feasible |
| samples/flv/demo__h264_aac__1280x720__30fps__2ch.flv | Yes | PASS | PASS (flv) | PASS (h264) | PASS (aac) | PASS | PASS | flv baseline compatibility; probe reason: target is feasible |
| samples/ts/demo__h264_aac__1920x1080__25fps__2ch.ts | Yes | PASS | PASS (ts) | PASS (h264) | PASS (aac) | PASS | PASS | mpeg-ts baseline compatibility; probe reason: target is feasible |
| samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts | Yes | PASS | PASS (ts) | PASS (mpeg2video) | PASS (ac3) | PASS | PASS | mpeg2 video decoder compatibility; probe reason: target is feasible |
| samples/mkv/demo__h264_eac3__1920x1080__30fps__2ch.mkv | Yes | PASS | PASS (mkv) | PASS (h264) | PASS (eac3) | PASS | PASS | eac3 audio decoder compatibility; probe reason: target is feasible |
| samples/mkv/demo__h264_dts__1920x1080__30fps__2ch.mkv | Yes | PASS | PASS (mkv) | PASS (h264) | PASS (dts) | PASS | PASS | dts audio decoder compatibility; probe reason: target is feasible |
| samples/webm/demo__vp9_vorbis__1920x1080__30fps__2ch.webm | Yes | PASS | PASS (webm) | PASS (vp9) | PASS (vorbis) | PASS | PASS | vorbis audio decoder compatibility; probe reason: target is feasible |
| samples/mov/demo__h264_pcm_s16le__1920x1080__30fps__2ch.mov | Yes | PASS | PASS (mov) | PASS (h264) | PASS (pcm_s16le) | PASS | PASS | pcm audio decoder compatibility; probe reason: target is feasible |
| samples/mov/demo__h264_aac__1920x1080__30fps__2ch.mov | Yes | PASS | PASS (mov) | PASS (h264) | PASS (aac) | PASS | PASS | mov baseline compatibility; probe reason: target is feasible |
| samples/avi/demo__h264_mp3__1280x720__30fps__2ch.avi | Yes | PASS | PASS (avi) | PASS (h264) | PASS (mp3) | PASS | PASS | avi baseline compatibility; probe reason: target is feasible |
| samples/m2ts/demo__h264_ac3__1920x1080__30fps__2ch.m2ts | Yes | PASS | PASS (m2ts) | PASS (h264) | PASS (ac3) | PASS | PASS | m2ts baseline compatibility; probe reason: target is feasible |
| samples/mp4/demo__av1_aac__1920x1080__30fps__2ch.mp4 | Yes | PASS | PASS (mp4) | PASS (av1) | PASS (aac) | PASS | PASS | mp4 + av1; probe reason: target is feasible |
| samples/mkv/demo__av1_flac__3840x2160__60fps__8ch__ma2.mkv | Yes | PASS | PASS (mkv) | PASS (av1) | PASS (flac) | PASS | PASS | mkv av1 + multi-audio; probe reason: target is feasible, prefer hardware decode, prefer D3D11 renderer, verify 8ch output path |
| samples/mkv/demo__vp9_opus__1920x1080__30fps__2ch__ma2.mkv | Yes | PASS | PASS (mkv) | PASS (vp9) | PASS (opus) | PASS | PASS | mkv vp9 + multi-audio; probe reason: target is feasible |
