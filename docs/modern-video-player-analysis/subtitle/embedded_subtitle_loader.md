# embedded_subtitle_loader 内嵌字幕加载

源码: `include/subtitle/embedded_subtitle_loader.h`, `src/subtitle/embedded_subtitle_loader.cpp`

## 角色

内嵌字幕轨道发现、选择和加载模块。它通过 FFmpeg 读取媒体中的字幕流，支持文本、ASS 和 bitmap 字幕，并按语言、forced、SDH 策略选择最佳轨道。

## 接口

| 接口 | 用途 |
|---|---|
| `listEmbeddedSubtitleTracks(media)` | 枚举媒体内嵌字幕轨道 |
| `selectBestEmbeddedSubtitleStream(tracks, policy)` | 按策略选择最佳字幕流 |
| `loadEmbeddedSubtitleTrack(media, stream_index)` | 加载指定内嵌字幕流 |
| `loadBestEmbeddedSubtitleTrack(media)` | 加载默认最佳字幕流 |
| `probeEmbeddedSubtitleLivePacketPath(media, stream_index, max_packets)` | 直播包路径探测 |

## 选择策略

| 策略 | 说明 |
|---|---|
| `preferred_languages` | 首选语言列表 |
| `EmbeddedSubtitleForcedPolicy` | forced 字幕偏好 |
| `EmbeddedSubtitleSdhPolicy` | SDH 字幕偏好 |
| codec preference | 文本、ASS、bitmap 字幕 codec 偏好 |

## 数据流

```mermaid
flowchart LR
    MEDIA[媒体文件] --> LIST[listEmbeddedSubtitleTracks]
    LIST --> TRACKS[EmbeddedSubtitleTrackInfo[]]
    TRACKS --> SELECT[selectBestEmbeddedSubtitleStream]
    SELECT --> LOAD[loadEmbeddedSubtitleTrack]
    LOAD --> ITEMS[SubtitleItem[]]
    ITEMS --> CORE[PlayerCore]
```

## 关键约束

- bitmap 字幕会转换为 `SubtitleBitmap` 写入 `SubtitleItem::bitmap_rects`。
- ASS 字幕可构造合成 ASS document 后复用 `AssParser`。
- 轨道选择要同时考虑 codec、语言、forced、SDH、默认标记等因素。

## 注意点

- PlayerCore 外挂字幕优先级高于内嵌字幕；清除外挂后可回到内嵌源。
- 修改策略时需要同步 `config/player_settings.ini` 的 `subtitle.*` 设置和 CLI 参数。
