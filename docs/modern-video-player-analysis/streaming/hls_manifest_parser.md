# HlsManifestParser HLS 解析

源码: `include/streaming/hls_manifest_parser.h`, `src/streaming/hls_manifest_parser.cpp`

## 角色

解析 HLS master playlist 文本，提取 variant stream 的带宽、分辨率和 URI。

## 接口

| 接口 | 用途 |
|---|---|
| `HlsManifestParser::parse(text, manifest)` | 解析 HLS manifest |

## 数据

| 数据 | 说明 |
|---|---|
| `HlsManifest` | 解析后的 HLS manifest |
| `HlsVariantStream` | 单个 variant 的 bandwidth、width、height、uri |

## 数据流

```mermaid
flowchart LR
    TEXT[master.m3u8 文本] --> PARSE[HlsManifestParser::parse]
    PARSE --> VARS[HlsVariantStream[]]
    VARS --> ABR[AdaptiveBitrateSelector]
```

## 关键约束

- 解析器读取 `#EXT-X-STREAM-INF` 属性，并将下一行作为 URI。
- 带宽和分辨率解析失败时不会产生有效 variant。

## 注意点

- 当前实现是轻量 parser，不覆盖 HLS 全部规范。
- 修改字段时需要同步 `runAdaptiveBitrateCheck` 和样例 manifest。
