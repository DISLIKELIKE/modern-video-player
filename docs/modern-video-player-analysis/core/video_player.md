# VideoPlayer 播放门面

源码: `include/video_player.h`, `src/video_player.cpp`

## 角色

面向应用层的播放器门面。它隐藏 `core::PlayerCore` 的复杂状态机和线程调度，对外提供打开、播放、暂停、seek、章节、AB 循环、截图、字幕、音量、倍速、诊断等接口。

## 接口

| 接口 | 用途 |
|---|---|
| `open(filename)` / `close()` | 打开或关闭媒体 |
| `play()` / `pause()` / `stop()` | 控制播放状态 |
| `seek(timestamp)` / `seekToNextChapter()` / `seekToPreviousChapter()` | 时间和章节跳转 |
| `setVolume` / `setPlaybackSpeed` / `setAudioDelay` / `setSubtitleDelay` | 运行时参数调整 |
| `loadExternalSubtitle` / `clearExternalSubtitle` | 外挂字幕加载和清理 |
| `embeddedSubtitleTracks` / `selectEmbeddedSubtitleStream` / `cycleEmbeddedSubtitleTrack` | 内嵌字幕轨道管理 |
| `getDiagnosticsSnapshot()` | 导出核心、调度、渲染、音频等诊断快照 |

## 持有成员

| 成员 | 类型 | 说明 |
|---|---|---|
| `core_player_` | `unique_ptr<core::PlayerCore>` | 实际播放器核心 |
| `playing_` / `paused_` | `atomic<bool>` | 门面层快速状态缓存 |
| `current_time_` | `atomic<double>` | 当前播放时间缓存 |
| `volume_` / `playback_speed_` | 标量 | 默认音量和倍速 |
| `subtitle_path_` / `subtitle_items_` | 字幕缓存 | 外挂字幕解析后的门面层状态 |

## 数据流

```mermaid
flowchart LR
    APP[main.cpp / 回归检查] --> VP[VideoPlayer]
    VP --> CORE[core::PlayerCore]
    CORE --> VP
    VP --> APP
```

## 关键约束

- 绝大多数行为由 `PlayerCore` 完成，`VideoPlayer` 主要做参数转发、状态缓存和字幕门面处理。
- 内嵌字幕相关接口在头文件中直接判空转发，`core_player_ == nullptr` 时返回空值或失败。

## 注意点

- 修改播放器公共 API 时，需要同时检查 `src/main.cpp` 中大量 `run*Check` 对该门面的调用。
- 门面层不直接操作 FFmpeg、SDL 或渲染器资源。
