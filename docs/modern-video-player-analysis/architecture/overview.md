# 软件架构总览

Modern Video Player 是一个 C++17 / SDL2 / FFmpeg 桌面播放器，核心目标是跨平台播放、后端自动选择、字幕与诊断能力。

## 一、分层模型

```mermaid
flowchart TB
    subgraph L1[入口层 Entry / CLI]
        MAIN[main.cpp<br/>CLI + playback loop + checks]
    end

    subgraph L2[应用门面层 Facade]
        VP[VideoPlayer<br/>应用 API]
        SET[SettingsManager<br/>配置读写]
        PL[PlaylistManager<br/>播放列表]
        HK[HotkeyManager<br/>热键映射]
    end

    subgraph L3[播放核心层 core]
        PC[PlayerCore<br/>状态机 + 编排]
        STR[PlaybackStrategy<br/>后端计划]
        SCH[Scheduler<br/>解码/渲染线程]
        CLK[Clock<br/>音视频时钟]
        FQ[FrameQueue / ThreadSafeQueue<br/>generation flush]
    end

    subgraph L4[媒体 IO / 解码]
        DEM[Demuxer<br/>AVFormatContext]
        DEC[DecoderFactory<br/>HW/SW backend order]
        AUD[AudioPlayer<br/>SDL audio]
    end

    subgraph L5[渲染和输入]
        RF[RendererFactory]
        VR[IVideoRenderer]
        D3D[D3D11VideoRenderer]
        OGL[OpenGLVideoRenderer]
        SDL[SdlVideoRenderer]
        VK[VulkanVideoRenderer]
        DISP[Display<br/>legacy SDL path]
    end

    subgraph L6[功能扩展]
        SUB[Subtitle parsers/loaders]
        FIL[FilterPipeline + Registry]
        STM[Streaming parsers/downloader]
        PLG[PluginManager]
        CAP[PlatformCapabilities + FormatSupport]
    end

    subgraph L7[外部依赖 External]
        FFMPEG[(FFmpeg)]
        SDL2[(SDL2)]
        QUILL[(Quill)]
        GPU[[D3D11 / OpenGL / Vulkan]]
        OS[[OS audio / window / filesystem]]
        HTTP[[HTTP source]]
    end

    MAIN --> VP
    MAIN --> SET & PL & HK
    VP --> PC
    PC --> STR & SCH & CLK & FQ
    PC --> DEM & DEC & AUD
    PC --> RF
    RF --> VR
    VR --> D3D & OGL & SDL & VK
    PC --> SUB & FIL & CAP
    MAIN --> STM
    MAIN --> PLG
    DEM --> FFMPEG
    DEC --> FFMPEG
    AUD --> SDL2
    D3D --> GPU
    OGL --> GPU
    VK --> GPU
    SDL --> SDL2
    PC --> OS
    STM --> HTTP
    MAIN --> QUILL

    classDef facade fill:#dbeafe,stroke:#2563eb
    classDef core fill:#fef3c7,stroke:#d97706
    classDef external fill:#fee2e2,stroke:#dc2626
    class VP,SET,PL,HK facade
    class PC,STR,SCH,CLK,FQ core
    class FFMPEG,SDL2,QUILL,GPU,OS,HTTP external
```

## 二、各层职责

| 层 | 目录 | 职责 | 上对接 | 下对接 |
|---|---|---|---|---|
| 入口 | `src/main.cpp` | 解析 CLI、加载设置、组织播放循环、执行本地检查 | 用户 / CI | VideoPlayer、Settings、Playlist |
| 门面 | `include/video_player.h` | 屏蔽 PlayerCore 复杂状态，提供应用层 API | main.cpp | PlayerCore |
| 核心 | `include/core/`, `src/core/` | open/play/pause/seek/stop 状态机、调度、时钟、队列 | VideoPlayer | Demuxer、Decoder、Renderer、Audio |
| 媒体 IO | `demuxer.*`, `decoder/`, `audio_player.*` | 打开媒体、读包、解码后端选择、音频输出 | PlayerCore | FFmpeg、SDL audio、平台硬件 |
| 渲染输入 | `render/`, `display.*` | 视频呈现、窗口事件、overlay、热键请求 | PlayerCore | SDL/D3D11/OpenGL/Vulkan |
| 功能扩展 | `subtitle/`, `filters/`, `streaming/`, `plugin/` | 字幕、滤镜、流媒体、插件 | main.cpp / PlayerCore | 文件、HTTP、动态库 |
| 平台能力 | `platform/`, `media/` | 编译/运行时能力探测、格式能力评估 | PlayerCore / CLI | OS、GPU、FFmpeg |

## 三、核心持有关系

```mermaid
graph LR
    MAIN[main.cpp] -->|栈对象| VP[VideoPlayer]
    MAIN -->|栈对象| SET[SettingsManager]
    MAIN -->|栈对象| PL[PlaylistManager]
    MAIN -->|栈对象| HK[HotkeyManager]

    VP -->|unique_ptr| PC[PlayerCore]

    PC -->|unique_ptr| DEM[Demuxer]
    PC -->|unique_ptr| AUD[AudioPlayer]
    PC -->|value| SCH[Scheduler]
    PC -->|value| CLK[Clock]
    PC -->|value| VQ[FrameQueue<VideoFrame>]
    PC -->|value| AQ[FrameQueue<AudioFrame>]
    PC -->|unique_ptr| REN[IVideoRenderer]
    PC -->|value| FIL[FilterPipeline]

    SCH -.callbacks.-> PC
    SCH -.queues.-> VQ & AQ
    REN -.input source.-> PC
    REN -.overlay sink.-> PC
```

## 四、单例 vs 实例

| 类 / 模块 | 类型 | 原因 |
|---|---|---|
| `VideoPlayer` | 应用实例 | main 中按播放会话使用，持有一个 PlayerCore |
| `PlayerCore` | 实例 | 绑定一个当前媒体会话和一组运行状态 |
| `Scheduler` | PlayerCore 值成员 | 与 PlayerCore 生命周期一致，线程回调闭包捕获 PlayerCore |
| `Demuxer` | PlayerCore 内 unique_ptr | 每次 open 重新创建，绑定当前 AVFormatContext |
| `AudioPlayer` | PlayerCore 内 unique_ptr | 根据媒体音频参数初始化 SDL audio |
| `IVideoRenderer` | PlayerCore 内 unique_ptr | 根据 PlaybackStrategy 候选链选择后端 |
| `PluginManager` | main/检查流程实例 | 动态插件生命周期由调用方显式管理 |
| `Logger` | 静态接口 / 内部状态 | `Logger::init/shutdown/log` 封装全局日志系统 |

## 五、目录与命名空间

| 命名空间 | 目录 | 说明 |
|---|---|---|
| `vp` | 根 include/src | VideoPlayer、Demuxer、Display、AudioPlayer、Logger |
| `vp::core` | `include/core`, `src/core` | 播放状态机、调度、帧结构、队列 |
| `vp::render` | `include/render`, `src/render` | 渲染器接口与后端 |
| `vp::subtitle` | `include/subtitle`, `src/subtitle` | 字幕统一模型和解析器 |
| `vp::filters` | `include/filters`, `src/filters` | 滤镜接口、注册表、流水线 |
| `vp::streaming` | `include/streaming`, `src/streaming` | HLS/DASH/HTTP 辅助能力 |
| `vp::platform` | `include/platform`, `src/platform` | 平台和硬件设备能力 |
| `vp::media` | `include/media`, `src/media` | 格式能力与 FFmpeg 兼容辅助 |

## 六、跨模块强约束

| 约束 | 涉及模块 | 描述 |
|---|---|---|
| timeline serial | PlayerCore / Scheduler / queues | seek/flush/close 会切换 serial；解码、渲染、包队列必须丢弃旧 serial 数据 |
| renderer/input 双接口 | Renderer 后端 / PlayerCore | D3D11/OpenGL/SDL/Vulkan 同时实现 `IVideoRenderer` 与 `IPlaybackInputSource`，事件必须由 PlayerCore 主事件线程 pump |
| overlay 顺序 | PlayerCore::renderFrame / renderer | 先更新字幕与 overlay，再执行滤镜，最后 render + present |
| EOF 判断 | Demuxer / packet queues / frame queues / AudioPlayer | EOF 只有在包队列、帧队列和音频设备缓存都排空后才进入 Ended |
| 设置键名 | main.cpp / SettingsManager / HotkeyManager | `player.*` 与 `hotkey.*` 字段由手写字符串约定，改名需全局搜索 |

## 七、外部依赖

| 依赖 | 形式 | 用途 |
|---|---|---|
| FFmpeg | 链接 | demux、decode、subtitle packet、scale/resample |
| SDL2 | 链接 | 窗口、事件、软件渲染、音频输出 |
| Quill | 可选链接/header | 结构化日志；缺失时走 std::cout fallback |
| D3D11 / DXGI / DWrite / D2D | Windows 链接 | D3D11 渲染、字幕/字体、HDR/adapter 诊断 |
| OpenGL | 链接 | OpenGL 渲染后端 |
| Vulkan SDK/runtime | 可选 | Vulkan 渲染后端与诊断 |
| libass / fontconfig / freetype | Linux 链接 | 字幕 shaping、字体查找 |

## 八、配置文件

| 文件 | 路径 | 内容 |
|---|---|---|
| 播放设置 | `config/player_settings.ini` | 音量、倍速、硬解偏好、热键、字幕策略 |
| 日志设置 | `config/logging.conf` | 日志级别、路径、轮转 |
| CMake 选项 | `CMakeLists.txt` | renderer/decoder 编译开关、依赖探测 |
| 样例媒体 | `samples/` | 本地检查用 mp4/hls/dash/subtitle 样例 |

