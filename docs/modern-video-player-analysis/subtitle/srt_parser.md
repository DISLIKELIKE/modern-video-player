# SrtParser SRT 解析器

源码: `include/subtitle/srt_parser.h`, `src/subtitle/srt_parser.cpp`

## 角色

解析 `.srt` 文本字幕。支持从文件或字符串内容解析，输出统一 `SubtitleItem` 列表。

## 接口

| 接口 | 用途 |
|---|---|
| `parseFile(file_path)` | 从文件解析 SRT |
| `parseText(content, source_path)` | 从字符串解析 SRT |
| `items()` | 返回字幕条目 |
| `format()` | 返回 `SubtitleFormat::Srt` |

## 数据流

```mermaid
flowchart LR
    FILE[.srt] --> PARSE[parseFile / parseText]
    PARSE --> STREAM[parseStream]
    STREAM --> TC[parseTimecode]
    STREAM --> ITEMS[SubtitleItem[]]
```

## 关键约束

- SRT 时间码由 `parseTimecode` 转换为秒。
- `source_path` 会写入 `SubtitleItem`，用于后续 active source 和诊断。

## 注意点

- SRT 解析结果主要包含纯文本和时间，不承载 ASS 的复杂样式语义。
- 修改容错规则时要检查 `--subtitle-sync-check`。
