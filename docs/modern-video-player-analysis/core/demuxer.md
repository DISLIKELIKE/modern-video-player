# Demuxer 解复用器

源码: `include/demuxer.h`, `src/demuxer.cpp`

## 角色

FFmpeg 输入封装。负责打开媒体、探测音视频/字幕流、读取 `AVPacket`、执行 seek，并向播放核心暴露媒体基础信息。

## 接口

| 接口 | 用途 |
|---|---|
| `open(filename)` | 创建并打开 `AVFormatContext` |
| `close()` | 释放输入上下文 |
| `readPacket(AVPacket*)` | 从容器读取下一个包 |
| `seek(timestamp)` | 按秒 seek |
| `getMediaInfo()` | 返回探测后的媒体信息 |
| `getFormatContext()` | 暴露 FFmpeg 上下文给字幕、解码等模块 |
| `isEof()` / `isOpen()` | 状态判断 |

## 数据流

```mermaid
flowchart LR
    FILE[媒体文件/URL] --> DEMUX[Demuxer::open]
    DEMUX --> INFO[MediaInfo]
    DEMUX --> PKT[AVPacket]
    PKT --> CORE[PlayerCore packet queues]
    CORE --> SEEK[Demuxer::seek]
```

## 关键约束

- `Demuxer` 持有裸 `AVFormatContext*`，生命周期由 `open/close` 管理。
- `readPacket` 和 `seek` 共享内部 mutex，避免并发读写 FFmpeg 输入上下文。
- `eof_reached_` 用于播放核心 EOF 判定，seek 或重新打开时需要重新建立 EOF 状态。

## 注意点

- 上层不应长期缓存 `AVFormatContext*` 后越过 `Demuxer::close()` 使用。
- 新增流类型时需要同步 `detectStreams()` 和 `MediaInfo` 字段。
