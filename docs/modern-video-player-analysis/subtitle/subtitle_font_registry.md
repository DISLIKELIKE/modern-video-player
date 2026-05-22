# subtitle_font_registry 字幕字体注册

源码: `include/subtitle/subtitle_font_registry.h`, `src/subtitle/subtitle_font_registry.cpp`

## 角色

字幕字体发现和注册模块。它从字幕文件附近目录或媒体附件中收集字体，注册到系统/应用字体集合，并提供 Windows DirectWrite 字体集合构建能力。

## 接口

| 接口 | 用途 |
|---|---|
| `ensureSubtitleFontsRegistered(subtitle_source_path)` | 注册外挂字幕附近字体 |
| `ensureMediaAttachmentFontsRegistered(media_source_path, format_ctx)` | 提取并注册媒体附件字体 |
| `releaseMediaAttachmentFonts(media_source_path)` | 释放媒体附件字体注册 |
| `buildSubtitleFontFallbackFamilies(preferred_family_utf8)` | 构建 fallback 字体族 |
| `buildDirectWriteSubtitleFontCollection(...)` | Windows DirectWrite 字体集合 |

## 数据流

```mermaid
flowchart LR
    SUB[字幕路径] --> DIR[候选目录]
    MEDIA[媒体附件] --> CACHE[附件字体缓存]
    DIR --> REG[注册字体]
    CACHE --> REG
    REG --> SUMMARY[SubtitleFontRegistrationSummary]
```

## 关键约束

- 字体文件按扩展名和 MIME 类型识别。
- 媒体附件字体会写入缓存目录，并可按媒体源释放。
- Windows 和 Linux 字体注册路径不同，代码中包含 DirectWrite 和 Fontconfig 分支。

## 注意点

- 字体注册影响 ASS/SSA 字幕渲染一致性。
- 修改缓存命名或目录时，要保证同一媒体重复打开不会产生冲突文件名。
