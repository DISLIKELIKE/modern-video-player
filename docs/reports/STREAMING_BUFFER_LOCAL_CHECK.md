# 本地验收：流媒体 HTTP 分片与缓冲

- 日期：`2026-03-08`
- 目标：验证 `7.2 流媒体（真实 HTTP 分片与缓冲）`
- HTTP 夹具：`./tools/start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`
- 播放列表地址：`http://127.0.0.1:8765/sample.m3u8`

## 执行命令

```powershell
.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765
.\build\Debug\modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128
```

## 输出摘要

```text
streaming-buffer-check.playlist_url=http://127.0.0.1:8765/sample.m3u8
streaming-buffer-check.segment_limit=3
streaming-buffer-check.target_buffer_bytes=128
streaming-buffer-check.manifest_download_ok=true
streaming-buffer-check.manifest_parse_ok=true
streaming-buffer-check.manifest_segments=3
streaming-buffer-check.requested_segments=3
streaming-buffer-check.segments_downloaded=3
streaming-buffer-check.buffered_bytes=621
streaming-buffer-check.max_segment_buffer=128
streaming-buffer-check.chunk_reads=6
streaming-buffer-check.buffer_ok=true
streaming-buffer-check.resolved_segments=http://127.0.0.1:8765/segment000.ts, http://127.0.0.1:8765/segment001.ts, http://127.0.0.1:8765/segment002.ts
streaming-buffer-check.error=none
streaming-buffer-check.result=PASS
```

## 结论

- `PASS`：宿主已能通过真实 HTTP 拉取 HLS 媒体清单，并顺序抓取前 3 个分片。
- 分片缓冲在本次 smoke 中累计达到 `621` 字节，且单次分块缓冲达到目标阈值 `128` 字节。
- 至此，任务清单中的 `7.2 流媒体（真实 HTTP 分片与缓冲）` 已具备可重复执行的本地回归入口。
