# 软件运行流程图

五个核心时序：**冷启动**、**打开媒体**、**播放控制**、**EOF 结束**、**关闭退出**。

## 一、冷启动时序

```mermaid
sequenceDiagram
    autonumber
    participant OS as 操作系统
    participant MAIN as main()
    participant LOG as Logger
    participant SET as SettingsManager
    participant HK as HotkeyManager
    participant PL as PlaylistManager
    participant VP as VideoPlayer

    OS->>MAIN: 启动进程
    MAIN->>MAIN: 解析 --version / --capabilities / --xxx-check
    alt 诊断或检查命令
        MAIN->>MAIN: run*Check / run*Diagnostics
        MAIN-->>OS: exit code
    else 播放模式
        MAIN->>LOG: Logger::init()
        MAIN->>SET: loadAppSettings(config/player_settings.ini)
        MAIN->>HK: loadHotkeySettings()
        MAIN->>PL: buildPlaylistFromInputs(argv)
        MAIN->>VP: new VideoPlayer
        MAIN->>VP: apply settings + hotkeys
        MAIN->>OS: 进入播放循环
    end
```

## 二、打开媒体时序

```mermaid
sequenceDiagram
    autonumber
    participant MAIN as main loop
    participant VP as VideoPlayer
    participant PC as PlayerCore
    participant DEM as Demuxer
    participant SUB as Subtitle loaders
    participant CAP as PlatformCapabilitiesProbe
    participant STR as PlaybackStrategy
    participant REN as RendererFactory
    participant DEC as DecoderFactory
    participant AUD as AudioPlayer

    MAIN->>VP: open(path)
    VP->>PC: open(path)
    PC->>PC: close() 清理旧会话
    PC->>PC: Session=Opening / Run=Stopped / Phase=Idle
    PC->>DEM: open(path)
    DEM->>DEM: avformat_open_input + stream detect
    PC->>SUB: 注册附件字体 + 枚举/选择内嵌字幕
    PC->>CAP: detect()
    PC->>STR: buildOpenPlan(media info + capabilities + preferences)
    PC->>REN: 按 renderer candidates 创建/初始化
    PC->>DEC: 按 decoder candidates 初始化 video/audio codec
    PC->>AUD: init(sample_rate, channels)
    PC->>PC: configure queues + callbacks
    PC-->>VP: open ok
```

### 打开失败兜底

| 阶段 | 失败 | 行为 |
|---|---|---|
| Demuxer open | 文件/URL 打不开 | Session=Failed，emit FileNotFound |
| renderer candidates 为空 | 平台无可用 renderer | emit DisplayInitFailed |
| renderer init 失败 | 单个候选失败 | 尝试下一个 renderer |
| decoder init 失败 | 软/硬解不可用 | 按 decoder candidates fallback |
| audio init 失败 | SDL audio 初始化失败 | 根据媒体类型和策略进入错误处理 |

## 三、播放控制时序

```mermaid
sequenceDiagram
    autonumber
    participant USER as 用户输入
    participant REN as Renderer input
    participant PC as PlayerCore
    participant DEMW as demux_worker
    participant SCH as Scheduler
    participant AC as audio_consumer
    participant AUD as AudioPlayer

    USER->>REN: Space / click / hotkey
    PC->>REN: handleEvents + consume request

    alt play
        PC->>DEMW: startDemuxThread()
        PC->>AC: startAudioConsumer()
        PC->>SCH: start/resume()
        PC->>AUD: resume()
        PC->>PC: RunState=Running
    else pause
        PC->>SCH: pause()
        PC->>AUD: pause()
        PC->>PC: RunState=Paused
    else seek
        PC->>PC: allocate timeline serial
        PC->>SCH: flush()
        PC->>DEM: seek(timestamp)
        PC->>PC: activateTimelineSerial()
        PC->>DEMW: startDemuxThread()
    else stop
        PC->>SCH: requestStopAsync/stop
        PC->>DEMW: requestStop
        PC->>AC: requestStop
        PC->>AUD: stop
        PC->>PC: RunState=Stopped
    end
```

## 四、帧调度与呈现时序

```mermaid
sequenceDiagram
    autonumber
    participant DEMW as demux_worker
    participant VPQ as video_packet_queue
    participant APQ as audio_packet_queue
    participant SCH as Scheduler
    participant PC as PlayerCore
    participant REN as IVideoRenderer
    participant AUD as AudioPlayer

    DEMW->>VPQ: push(DemuxPacket + serial)
    DEMW->>APQ: push(DemuxPacket + serial)
    SCH->>PC: decodeVideoFrame(VideoFrame&)
    PC->>VPQ: pop packet
    PC->>PC: avcodec_send_packet / receive_frame
    SCH->>PC: renderFrame(VideoFrame&&)
    PC->>PC: drop stale serial
    PC->>PC: updateSubtitleOverlay + FilterPipeline
    PC->>REN: renderFrame(frame)
    PC->>REN: present()
    SCH->>PC: decodeAudioFrame(AudioFrame&)
    PC->>APQ: pop packet
    PC->>AUD: play(audio bytes)
```

## 五、EOF / Ended 流程

```mermaid
sequenceDiagram
    autonumber
    participant DEMW as demux_worker
    participant Q as packet/frame queues
    participant SCH as Scheduler
    participant PC as PlayerCore
    participant AUD as AudioPlayer

    DEMW->>Q: setEof(true)
    PC->>PC: setEofReached(true)
    SCH->>PC: onRenderIdle()
    PC->>Q: 检查 packet queue empty + frame queue empty
    PC->>AUD: getBufferedSeconds()
    alt 全部排空
        PC->>DEMW: requestStop()
        PC->>SCH: requestStopAsync()
        PC->>AUD: pause()
        PC->>PC: EndedReason=Eof
        PC->>PC: RunState=Ended / Phase=Idle
    else 仍有缓存
        PC-->>SCH: 等下一次 idle
    end
```

## 六、关闭 / 退出流程

```mermaid
sequenceDiagram
    autonumber
    participant MAIN as main loop
    participant VP as VideoPlayer
    participant PC as PlayerCore
    participant SCH as Scheduler
    participant REN as Renderer
    participant AUD as AudioPlayer
    participant LOG as Logger

    MAIN->>VP: close()
    VP->>PC: close()
    PC->>SCH: stop()
    PC->>PC: stopDemuxThread + stopAudioConsumer
    PC->>REN: close()
    PC->>AUD: close()
    PC->>PC: releaseDecoders / releaseScaler / releaseResampler
    MAIN->>MAIN: saveAppSettings()
    MAIN->>LOG: Logger::shutdown()
```

## 七、关键状态约束

| 状态 | 来源 | 用途 |
|---|---|---|
| `SessionState` | PlayerCore | open/close/fail 生命周期 |
| `RunState` | PlayerCore | Stopped/Starting/Running/Paused/Stopping/Ended |
| `PipelinePhase` | PlayerCore | Idle/Normal/Draining 等流水线阶段 |
| `TimelineSerial` | PlayerCore | seek/flush/close 后丢弃旧包旧帧 |
| `SchedulerControlSnapshot` | PlayerCore → Scheduler | 让 Scheduler 感知运行状态、时钟策略和 EOF 策略 |

