# DashManifestParser DASH 解析

源码: `include/streaming/dash_manifest_parser.h`, `src/streaming/dash_manifest_parser.cpp`

## 角色

解析 DASH MPD 文本，提取 representation 的带宽、分辨率和 base URL。

## 接口

| 接口 | 用途 |
|---|---|
| `DashManifestParser::parse(text, manifest)` | 解析 DASH manifest |

## 数据

| 数据 | 说明 |
|---|---|
| `DashManifest` | MPD 解析结果 |
| `DashRepresentation` | 单个 representation 的 bandwidth、width、height、base_url |

## 数据流

```mermaid
flowchart LR
    MPD[MPD 文本] --> PARSE[DashManifestParser::parse]
    PARSE --> REPS[DashRepresentation[]]
    REPS --> ABR[AdaptiveBitrateSelector]
```

## 关键约束

- 当前 parser 通过文本查找和属性提取处理 MPD，属于轻量实现。
- `BaseURL` 和 representation 属性需要在样例 MPD 中保持可解析格式。

## 注意点

- 如果后续支持复杂 MPD，需要考虑 XML parser，避免继续扩大字符串解析复杂度。
- 修改解析规则时同步 `samples/streaming/abr_local/dash/sample.mpd`。
