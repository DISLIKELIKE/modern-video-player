# PlaylistManager 播放列表

源码: `include/playlist/playlist_manager.h`, `src/playlist/playlist_manager.cpp`

## 角色

播放列表数据结构和 `.m3u8` 读写工具。用于命令行多媒体输入、拖放媒体、播放列表顺序播放和本地回归检查。

## 接口

| 接口 | 用途 |
|---|---|
| `addItem` / `removeItem` / `clear` | 管理条目 |
| `empty` / `size` / `items` | 查询列表 |
| `sortByTitle` / `sortByDuration` | 简单排序 |
| `loadM3U8(file_path)` / `saveM3U8(file_path)` | 读写 M3U8 |

## 数据

| 数据 | 说明 |
|---|---|
| `PlaylistItem` | 播放条目，包含 URI、标题、时长等信息 |
| `items_` | 当前播放列表条目数组 |

## 数据流

```mermaid
flowchart LR
    INPUT[命令行/拖放/M3U8] --> PL[PlaylistManager]
    PL --> MAIN[main.cpp 播放循环]
    MAIN --> VP[VideoPlayer::open]
    VP -.next/previous request.-> MAIN
```

## 关键约束

- `main.cpp` 负责当前索引、上一项/下一项和退出时位置保存；`PlaylistManager` 只保存列表数据。
- `.m3u8` 文件可作为播放输入，也可由工具函数保存。

## 注意点

- 修改播放列表条目字段时需要同步 M3U8 读写和 `buildPlaylistFromInputs()`。
- `runPlaylistFlowCheck` 依赖列表顺序和打开结果。
