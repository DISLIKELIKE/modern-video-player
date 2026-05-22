# AssParser ASS/SSA 解析器

源码: `include/subtitle/ass_parser.h`, `src/subtitle/ass_parser.cpp`

## 角色

解析 `.ass` / `.ssa` 字幕。负责把脚本样式、Dialogue、override tag、动画、karaoke、clip、矢量绘图等内容转换为统一 `SubtitleItem`。

## 接口

| 接口 | 用途 |
|---|---|
| `parseFile(file_path)` | 从文件解析 ASS/SSA |
| `parseText(content, source_path)` | 从字符串解析 ASS/SSA |
| `items()` | 返回字幕条目 |
| `format()` | 返回 ASS/SSA 格式 |

## 数据流

```mermaid
flowchart LR
    ASS[.ass/.ssa] --> PARSE[AssParser]
    PARSE --> STYLE[SubtitleStyle]
    PARSE --> ANIM[SubtitleStyleAnimation]
    PARSE --> RUNS[SubtitleTextRun[]]
    PARSE --> ITEMS[SubtitleItem[]]
```

## 关键能力

| 能力 | 对应数据 |
|---|---|
| 字体、颜色、描边、阴影 | `SubtitleStyle` |
| `\move`、`\fad`、`\fade`、`\t` | `SubtitleStyleAnimation` |
| karaoke | `SubtitleTextRun::karaoke_*` |
| 矢量绘图/clip | `is_vector_drawing`、`drawing_commands`、`vector_clip_commands` |
| 分辨率 | `play_res_x` / `play_res_y` |

## 关键约束

- ASS 解析结果必须兼容统一 `SubtitleItem`，渲染器不读取 ASS 原文件。
- 样式 transition 用 bitmask 表示变化字段。

## 注意点

- 修改 ASS tag 支持时，要同步字幕样式、OpenGL ASS 动画和 libass shaping 检查。
- 内嵌 ASS 字幕加载也会复用 ASS 解析路径。
