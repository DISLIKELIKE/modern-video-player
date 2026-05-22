# subtitle_parser 字幕统一模型

源码: `include/subtitle/subtitle_parser.h`, `src/subtitle/subtitle_parser.cpp`

## 角色

字幕统一数据模型和 parser facade。定义 SRT/ASS/SSA 解析器共享的 `SubtitleItem`、样式、动画、bitmap、text run 等结构，并根据文件扩展名创建具体 parser。

## 接口

| 接口 | 用途 |
|---|---|
| `ISubtitleParser::parseFile` | 解析字幕文件 |
| `ISubtitleParser::items` | 返回解析后的字幕条目 |
| `ISubtitleParser::format` | 返回字幕格式 |
| `isSupportedSubtitleExtension` | 判断扩展名是否支持 |
| `createParserForPath` | 创建 SRT 或 ASS/SSA parser |
| `flattenSubtitleText` | 把字幕条目文本展开为普通文本 |

## 数据

| 数据 | 说明 |
|---|---|
| `SubtitleItem` | index、layer、起止时间、源路径、文本、样式、动画、bitmap、runs |
| `SubtitleStyle` | 字体、颜色、描边、阴影、位置、clip、旋转、缩放等样式 |
| `SubtitleStyleAnimation` | move、fade、transition 等动画 |
| `SubtitleBitmap` | bitmap 字幕矩形和 RGBA 数据 |

## 数据流

```mermaid
flowchart LR
    PATH[subtitle path] --> FACADE[createParserForPath]
    FACADE --> SRT[SrtParser]
    FACADE --> ASS[AssParser]
    SRT --> ITEMS[SubtitleItem[]]
    ASS --> ITEMS
    ITEMS --> CORE[PlayerCore active subtitles]
```

## 关键约束

- 解析结果统一落到 `SubtitleItem`，渲染层不直接依赖 SRT/ASS 原始格式。
- ASS 的复杂样式和动画字段保留在统一结构中，供 OpenGL/D3D11 overlay 使用。

## 注意点

- 新增字幕格式时需要补扩展名判断、parser 创建和回归检查。
- 修改 `SubtitleItem` 字段时要同步渲染 overlay、内嵌字幕加载和 timeline 辅助函数。
