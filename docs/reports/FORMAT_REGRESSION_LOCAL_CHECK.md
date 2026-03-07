# Format Regression Report

Generated: 2026-03-07 22:45:10
Tool: run_format_regression.ps1
Executable: build/Debug/modern-video-player.exe
Sample List: tools/format_regression/format_samples.csv

## Summary

- Total: 9
- PASS: 9
- PARTIAL: 0
- FAIL: 0
- SKIP: 0

## Details


| Sample                                                     | Exists | Open | Container   | Video       | Audio       | Overall | Compatibility | Notes                                                                                                                           |
| ---------------------------------------------------------- | ------ | ---- | ----------- | ----------- | ----------- | ------- | ------------- | ------------------------------------------------------------------------------------------------------------------------------- |
| juren-30s.mp4                                              | Yes    | PASS | PASS (mp4)  | PASS (h264) | PASS (aac)  | PASS    | PASS          | repo baseline sample; probe reason: target is feasible                                                                          |
| samples/mp4/demo__hevc_aac__1920x1080__30fps__2ch.mp4      | Yes    | PASS | PASS (mp4)  | PASS (hevc) | PASS (aac)  | PASS    | PASS          | mp4 + h265; probe reason: target is feasible                                                                                    |
| samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv | Yes    | PASS | PASS (mkv)  | PASS (hevc) | PASS (ac3)  | PASS    | PASS          | mkv h265 + multi-audio; probe reason: target is feasible, prefer hardware decode, prefer D3D11 renderer, verify 6ch output path |
| samples/webm/demo__vp9_opus__1920x1080__30fps__2ch.webm    | Yes    | PASS | PASS (webm) | PASS (vp9)  | PASS (opus) | PASS    | PASS          | webm vp9 + opus; probe reason: target is feasible                                                                               |
| samples/flv/demo__h264_aac__1280x720__30fps__2ch.flv       | Yes    | PASS | PASS (flv)  | PASS (h264) | PASS (aac)  | PASS    | PASS          | flv baseline compatibility; probe reason: target is feasible                                                                    |
| samples/ts/demo__h264_aac__1920x1080__25fps__2ch.ts        | Yes    | PASS | PASS (ts)   | PASS (h264) | PASS (aac)  | PASS    | PASS          | mpeg-ts baseline compatibility; probe reason: target is feasible                                                                |
| samples/mp4/demo__av1_aac__1920x1080__30fps__2ch.mp4       | Yes    | PASS | PASS (mp4)  | PASS (av1)  | PASS (aac)  | PASS    | PASS          | mp4 + av1; probe reason: target is feasible                                                                                     |
| samples/mkv/demo__av1_flac__3840x2160__60fps__8ch__ma2.mkv | Yes    | PASS | PASS (mkv)  | PASS (av1)  | PASS (flac) | PASS    | PASS          | mkv av1 + multi-audio; probe reason: target is feasible, prefer hardware decode, prefer D3D11 renderer, verify 8ch output path  |
| samples/mkv/demo__vp9_opus__1920x1080__30fps__2ch__ma2.mkv | Yes    | PASS | PASS (mkv)  | PASS (vp9)  | PASS (opus) | PASS    | PASS          | mkv vp9 + multi-audio; probe reason: target is feasible                                                                         |


