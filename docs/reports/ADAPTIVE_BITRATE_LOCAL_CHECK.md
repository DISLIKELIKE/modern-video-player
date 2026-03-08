# 本地验收：HLS/DASH 自适应码率

- 日期：`2026-03-08`
- 目标：验证 `7.3 HLS/DASH 自适应码率`
- HTTP 夹具：`./tools/start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`
- HLS 清单：`http://127.0.0.1:8766/abr_local/hls/master.m3u8`
- DASH 清单：`http://127.0.0.1:8766/abr_local/dash/sample.mpd`
- 带宽序列：`900000,3500000,1500000`

## 执行命令

```powershell
.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766
.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128
.\build\Debug\modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128
```

## 输出摘要（HLS）

```text
adaptive-bitrate-check.manifest_url=http://127.0.0.1:8766/abr_local/hls/master.m3u8
adaptive-bitrate-check.protocol=HLS
adaptive-bitrate-check.segment_limit=2
adaptive-bitrate-check.target_buffer_bytes=128
adaptive-bitrate-check.manifest_download_ok=true
adaptive-bitrate-check.manifest_parse_ok=true
adaptive-bitrate-check.bandwidth_samples_ok=true
adaptive-bitrate-check.bandwidth_samples=900000, 3500000, 1500000
adaptive-bitrate-check.variant_count=3
adaptive-bitrate-check.selected_variants=640x360, 1920x1080, 1280x720
adaptive-bitrate-check.selected_bandwidths=400000, 2200000, 1200000
adaptive-bitrate-check.switch_count=2
adaptive-bitrate-check.upswitch_count=1
adaptive-bitrate-check.downswitch_count=1
adaptive-bitrate-check.fallback_count=0
adaptive-bitrate-check.initializations_downloaded=0
adaptive-bitrate-check.segments_downloaded=6
adaptive-bitrate-check.buffered_bytes=1540
adaptive-bitrate-check.max_buffered_bytes=128
adaptive-bitrate-check.chunk_reads=16
adaptive-bitrate-check.variant_count_ok=true
adaptive-bitrate-check.abr_ok=true
adaptive-bitrate-check.downloads_ok=true
adaptive-bitrate-check.error=none
adaptive-bitrate-check.result=PASS
```

## 输出摘要（DASH）

```text
adaptive-bitrate-check.manifest_url=http://127.0.0.1:8766/abr_local/dash/sample.mpd
adaptive-bitrate-check.protocol=DASH
adaptive-bitrate-check.segment_limit=2
adaptive-bitrate-check.target_buffer_bytes=128
adaptive-bitrate-check.manifest_download_ok=true
adaptive-bitrate-check.manifest_parse_ok=true
adaptive-bitrate-check.bandwidth_samples_ok=true
adaptive-bitrate-check.bandwidth_samples=900000, 3500000, 1500000
adaptive-bitrate-check.variant_count=3
adaptive-bitrate-check.selected_variants=video_400k, video_2200k, video_1200k
adaptive-bitrate-check.selected_bandwidths=400000, 2200000, 1200000
adaptive-bitrate-check.switch_count=2
adaptive-bitrate-check.upswitch_count=1
adaptive-bitrate-check.downswitch_count=1
adaptive-bitrate-check.fallback_count=0
adaptive-bitrate-check.initializations_downloaded=3
adaptive-bitrate-check.segments_downloaded=6
adaptive-bitrate-check.buffered_bytes=1888
adaptive-bitrate-check.max_buffered_bytes=128
adaptive-bitrate-check.chunk_reads=19
adaptive-bitrate-check.variant_count_ok=true
adaptive-bitrate-check.abr_ok=true
adaptive-bitrate-check.downloads_ok=true
adaptive-bitrate-check.error=none
adaptive-bitrate-check.result=PASS
```

## 结论

- `PASS`：宿主已能解析 HLS master playlist 与 DASH MPD，并基于给定带宽序列完成多码率档位选择。
- HLS 与 DASH 在本次本地夹具中都完成了一次升档和一次降档，`switch_count=2`、`upswitch_count=1`、`downswitch_count=1`。
- DASH 路径额外验证了初始化分片下载，说明 `Representation` 级别的 `BaseURL / Initialization / SegmentURL` 明细已接入。
- 至此，任务清单中的 `7.3 HLS/DASH 自适应码率` 已具备可重复执行的本地回归入口。
