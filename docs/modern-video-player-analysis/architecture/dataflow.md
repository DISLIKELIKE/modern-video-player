# 数据流图

围绕“一次媒体播放从输入到画面/声音输出”的核心数据流，外加字幕、流媒体、配置、诊断四条次要流。

## 一、主播放数据流（文件 / URL → 解复用 → 解码 → 输出）

```mermaid
flowchart TB
    USER((用户 / CLI))
    FS[(媒体文件)]
    URL[(HTTP/HLS/DASH URL)]
    GPU[[GPU / window]]
    SPK[[Audio device]]

    USER --> MAIN[main.cpp]
    MAIN --> SET[SettingsManager<br/>loadAppSettings]
    MAIN --> PL[PlaylistManager]
    MAIN --> VP[VideoPlayer]
    VP --> PC[PlayerCore]

    PC --> DEM[Demuxer<br/>AVFormatContext]
    DEM --> FS
    DEM --> URL
    PC --> CAP[PlatformCapabilitiesProbe]
    PC --> STR[PlaybackStrategy<br/>renderer/decoder candidates]
    STR --> RF[RendererFactory]
    STR --> DF[DecoderFactory]

    RF --> REN[IVideoRenderer<br/>D3D11/OpenGL/SDL/Vulkan]
    DF --> VDEC[Video decoder]
    DF --> ADEC[Audio decoder]

    PC --> DW[demux_worker]
    DW --> VPQ[video_packet_queue]
    DW --> APQ[audio_packet_queue]

    VPQ --> SCH[Scheduler]
    APQ --> SCH
    SCH -->|decodeVideoFrame| VDEC
    SCH -->|decodeAudioFrame| ADEC
    VDEC --> VQ[FrameQueue<VideoFrame>]
    ADEC --> AQ[FrameQueue<AudioFrame>]

    VQ -->|render callback| PC
    PC --> FIL[FilterPipeline<br/>software frame only]
    FIL --> REN
    REN --> GPU

    AQ --> AC[audio_consumer_worker]
    AC --> AUD[AudioPlayer]
    AUD --> SPK

    REN -.events.-> PC
    PC -.position/state.-> VP
```

### 关键节点说明

| 阶段 | 触发条件 | 关键模块 |
|---|---|---|
| 1. 输入归一 | CLI 参数、拖拽文件或播放列表项 | `main.cpp`, `PlaylistManager` |
| 2. 打开媒体 | `VideoPlayer::open` | `PlayerCore::open`, `Demuxer::open` |
| 3. 后端计划 | 读取媒体信息和平台能力 | `PlaybackStrategy`, `RendererFactory`, `DecoderFactory` |
| 4. 解复用 | play/start 后 demux worker 循环读包 | `Demuxer`, packet queues |
| 5. 解码调度 | Scheduler 三线程处理视频/音频/渲染 | `Scheduler`, `FrameQueue` |
| 6. 呈现 | PlayerCore render callback | `FilterPipeline`, `IVideoRenderer` |
| 7. 音频输出 | audio consumer 拉取 AudioFrame | `AudioPlayer`, SDL audio callback |

## 二、字幕数据流

```mermaid
flowchart TB
    EXT[外部字幕文件]
    MEDIA[媒体内嵌字幕轨]
    ATT[媒体附件字体]

    EXT --> VP[VideoPlayer::loadExternalSubtitle]
    VP --> SP[SubtitleParser]
    SP --> SRT[SrtParser]
    SP --> ASS[AssParser]

    MEDIA --> DEM[Demuxer / AVFormatContext]
    DEM --> EST[EmbeddedSubtitleTrackInfo]
    EST --> SEL[selectBestEmbeddedSubtitleStream]
    SEL --> ESL[EmbeddedSubtitleLoader]

    ATT --> FONT[ensureMediaAttachmentFontsRegistered]

    SRT --> ITEMS[vector<SubtitleItem>]
    ASS --> ITEMS
    ESL --> ITEMS
    FONT --> ITEMS

    ITEMS --> PC[PlayerCore subtitle cache]
    PC --> TL[SubtitleTimeline / scan]
    TL --> OVL[IRenderOverlaySink]
    OVL --> REN[Renderer subtitle overlay]
```

### 字幕选择规则

| 来源 | 入口 | 生效条件 |
|---|---|---|
| 外部字幕 | `VideoPlayer::loadExternalSubtitle` | 文件扩展名可识别，解析成功 |
| 内嵌字幕 | `PlayerCore::open` | preferred stream 可用，或 selection policy 选出支持轨 |
| 字体附件 | `ensureMediaAttachmentFontsRegistered` | 媒体含 font attachment 且可提取/注册 |

## 三、流媒体辅助数据流

```mermaid
flowchart LR
    CLI[CLI check / URL input]
    HTTP[HttpStreamDownloader]
    HLS[HlsManifestParser]
    DASH[DashManifestParser]
    ABR[AdaptiveBitrateSelector]
    CAND[variant candidates]
    DL[segment download]

    CLI --> HTTP
    HTTP --> HLS
    HTTP --> DASH
    HLS --> CAND
    DASH --> CAND
    CAND --> ABR
    ABR --> DL
```

流媒体模块当前主要服务 CLI 检查和输入候选构造；实际媒体播放仍由 FFmpeg 打开最终 URL 或本地路径。

## 四、设置与热键数据流

```mermaid
flowchart TB
    INI[config/player_settings.ini]
    SM[SettingsManager]
    APP[AppSettings]
    HK[HotkeyManager]
    VP[VideoPlayer]
    PC[PlayerCore]
    REN[IPlaybackInputSource]

    INI --> SM
    SM --> APP
    SM --> HK
    APP --> VP
    HK --> VP
    VP --> PC
    PC --> REN
    REN --> PC
    VP --> SAVE[saveAppSettings]
    SAVE --> SM
    SM --> INI
```

| 配置域 | 读取位置 | 写入位置 |
|---|---|---|
| `player.volume` | `loadAppSettings` | `saveAppSettings` |
| `player.playback_speed` | `loadAppSettings` | `saveAppSettings` |
| `player.audio_delay_ms` / `subtitle_delay_ms` | `loadAppSettings` | `saveAppSettings` |
| `player.prefer_hardware_decode` | `loadAppSettings` | `saveAppSettings` |
| `hotkey.*` | `loadHotkeySettings` | `syncHotkeySettings` |

## 五、诊断 / 回归检查数据流

```mermaid
flowchart TB
    CLI[main.cpp CLI option]
    RUN[run*Check / run*Diagnostics]
    VP[VideoPlayer]
    PC[PlayerCore]
    DIAG[DiagnosticsSnapshot]
    OUT[stdout / stderr]
    LOG[Logger]

    CLI --> RUN
    RUN --> VP
    VP --> PC
    PC --> DIAG
    RUN --> OUT
    PC --> LOG
```

`main.cpp` 内部有大量 `--xxx-check` 入口，用于 CI/本地回归。它们复用生产播放器链路，但把结果输出为 `PASS/FAIL` 或 key/value。

