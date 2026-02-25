# 架构设计文档

## 项目依赖版本

| 组件 | 版本 |
|------|------|
| FFmpeg | 8.0.1 |
| SDL2 | 2.30.11 |
| Quill | 6.0.0 |
| CMake | 3.15+ |
| C++ | C++17 |

## 系统架构

## 1. 整体架构图

```mermaid
graph TB
    A[Main Program] --> B[VideoPlayer]
    B --> C[VideoDecoder]
    B --> D[AudioDecoder]
    B --> E[Display]
    B --> F[AudioPlayer]
    
    C --> G[FFmpeg Video Codec]
    D --> H[FFmpeg Audio Codec]
    E --> I[SDL2 Window/Renderer]
    F --> J[SDL2 Audio Device]
    
    B --> K[RenderThread]
    B --> L[VideoDecodeThread]
    B --> M[AudioDecodeThread]
    
    L --> N[VideoFrameQueue]
    M --> O[AudioFrameQueue]
    N --> K
    O --> K
    
    K --> P[SyncManager]
    
    style A fill:#e1f5ff
    style B fill:#ffe1e1
    style C fill:#e1ffe1
    style D fill:#e1ffe1
    style E fill:#fff5e1
    style F fill:#fff5e1
    style K fill:#f5e1ff
    style L fill:#e1e1ff
    style M fill:#e1e1ff
    style N fill:#ffffe1
    style O fill:#ffffe1
    style P fill:#ffe1ff
```

## 2. 模块设计

### 2.0 FrameQueue (帧队列)

**职责**:
- 线程安全的帧数据传输
- 解码线程与渲染线程之间的缓冲
- 带超时等待的 push/pop 操作

**关键接口**:
```cpp
template<typename T>
class FrameQueue {
    bool push(T&& frame);                    // 非阻塞 push
    bool pushWithWait(T&& frame, int timeout_ms);  // 带等待的 push
    bool pop(T& frame, int timeout_ms);     // 带等待的 pop
    bool tryPop(T& frame);                   // 非阻塞 pop
    void clear();                            // 清空队列
    void stop();                             // 停止队列
};
```

**特性**:
- 使用 `std::mutex` 和 `std::condition_variable` 实现线程安全
- 支持队列满/空时的超时等待，避免 CPU 忙轮询
- 默认队列大小为 10 帧

### 2.0.1 VideoDecodeThread (视频解码线程)

**职责**:
- 独立于主线程进行视频解码
- 将解码后的帧放入视频帧队列
- 支持暂停/恢复/停止控制

**关键接口**:
```cpp
class VideoDecodeThread {
    bool start(AVFormatContext* fmt_ctx, int stream_idx, FrameQueue<VideoFrame>* output_queue);
    void stop();
    void pause();
    void resume();
    void flush();
};
```

### 2.0.2 AudioDecodeThread (音频解码线程)

**职责**:
- 独立于主线程进行音频解码
- 将解码后的帧转换为 SDL 可播放格式
- 将转换后的帧放入音频帧队列

**关键接口**:
```cpp
class AudioDecodeThread {
    bool start(AVFormatContext* fmt_ctx, int stream_idx, FrameQueue<AudioFrame>* output_queue);
    void stop();
    void pause();
    void resume();
    void flush();
};
```

### 2.0.3 SyncManager (同步管理器)

**职责**:
- 管理音视频同步
- 计算帧延迟
- 决定是否跳帧或重复帧

**同步模式**:
```cpp
enum class SyncMode {
    AudioMaster,   // 音频为主时钟
    VideoMaster,  // 视频为主时钟
    Free          // 自由播放
};
```

**关键接口**:
```cpp
class SyncManager {
    void setMode(SyncMode mode);
    void updateVideoPts(double pts);
    void updateAudioPts(double pts);
    double calculateDelay(double current_pts);
    bool shouldSkipFrame(double frame_pts);
    bool shouldRepeatFrame(double frame_pts);
};
```

### 2.1 VideoPlayer (主播放器)

**职责**:
- 管理所有子模块
- 控制播放流程
- 实现音视频同步
- 处理用户输入

**关键接口**:
```cpp
class VideoPlayer {
    bool open(const std::string& filename);
    void play();
    void pause();
    void stop();
    void seek(double timestamp);
    void setVolume(float volume);
};
```

**状态管理**:
- `playing_`: 是否正在播放
- `paused_`: 是否暂停
- `stopped_`: 是否停止
- `current_time_`: 当前播放时间

**线程模型**:
- 主线程: 处理用户输入
- 播放线程: 音视频解码和渲染

### 2.2 VideoDecoder (视频解码器)

**职责**:
- 打开视频流
- 解码视频帧
- 格式转换
- 管理解码器资源

**FFmpeg 解码流程**:

```mermaid
sequenceDiagram
    participant App as Application
    participant Decoder as VideoDecoder
    participant FFmpeg as FFmpeg
    participant Codec as Codec
    
    App->>Decoder: decodeFrame()
    Decoder->>FFmpeg: av_read_frame()
    FFmpeg-->>Decoder: AVPacket
    Decoder->>Codec: avcodec_send_packet()
    Decoder->>Codec: avcodec_receive_frame()
    Codec-->>Decoder: AVFrame
    Decoder-->>App: VideoFrame
```

**关键数据结构**:
- `AVFormatContext`: 媒体文件上下文
- `AVCodecContext`: 编解码器上下文
- `AVPacket`: 压缩数据包
- `AVFrame`: 解码后的原始帧
- `SwsContext`: 格式转换上下文

**线程安全**:
- 使用 `std::mutex` 保护共享资源
- RAII 管理 FFmpeg 资源

### 2.3 AudioDecoder (音频解码器)

**职责**:
- 打开音频流
- 解码音频帧
- 格式转换
- 管理解码器资源

**音频格式转换**:

```
原始格式 (多种) → SwrContext → 目标格式 (S16)
```

**关键数据结构**:
- `AVCodecContext`: 音频编解码器上下文
- `SwrContext`: 音频格式转换上下文

### 2.4 Display (显示模块)

**职责**:
- 创建和管理窗口
- 渲染视频帧
- 处理窗口事件
- 全屏切换

**SDL2 渲染流程**:

```mermaid
sequenceDiagram
    participant App as Application
    participant Display as Display
    participant SDL as SDL2
    
    App->>Display: init()
    Display->>SDL: SDL_CreateWindow()
    Display->>SDL: SDL_CreateRenderer()
    Display->>SDL: SDL_CreateTexture()
    
    App->>Display: renderFrame()
    Display->>SDL: SDL_UpdateYUVTexture()
    Display->>SDL: SDL_RenderCopy()
    Display->>SDL: SDL_RenderPresent()
    
    App->>Display: close()
    Display->>SDL: SDL_DestroyTexture()
    Display->>SDL: SDL_DestroyRenderer()
    Display->>SDL: SDL_DestroyWindow()
```

**YUV 渲染**:

```cpp
SDL_UpdateYUVTexture(texture_, nullptr,
    y_plane, y_stride,
    u_plane, u_stride,
    v_plane, v_stride
);
```

### 2.5 AudioPlayer (音频播放器)

**职责**:
- 打开音频设备
- 播放音频数据
- 音量控制
- 队列管理

**SDL2 音频流程**:

```mermaid
sequenceDiagram
    participant App as Application
    participant AudioPlayer as AudioPlayer
    participant SDL as SDL2 Audio
    
    App->>AudioPlayer: init()
    AudioPlayer->>SDL: SDL_OpenAudioDevice()
    
    App->>AudioPlayer: play(data)
    AudioPlayer->>AudioPlayer: push to queue
    
    Note over SDL: Audio Callback
    SDL->>AudioPlayer: audioCallback()
    AudioPlayer->>AudioPlayer: pop from queue
    AudioPlayer->>SDL: fill stream
    SDL->>SDL: play audio
    
    App->>AudioPlayer: close()
    AudioPlayer->>SDL: SDL_CloseAudioDevice()
```

**音频回调机制**:

```cpp
void audioCallback(void* userdata, uint8_t* stream, int len) {
    // 从队列中获取音频数据
    // 填充到 stream 中
    // SDL 自动播放
}
```

## 3. 数据流

### 3.1 视频数据流

```
视频文件 → AVPacket → VideoDecoder → AVFrame → Display → 屏幕显示
```

### 3.2 音频数据流

```
音频文件 → AVPacket → AudioDecoder → AVFrame → AudioPlayer → 扬声器
```

### 3.3 完整播放流程

```mermaid
graph LR
    A[视频文件] --> B[avformat_open_input]
    B --> C[avformat_find_stream_info]
    C --> D[VideoDecoder & AudioDecoder]
    D --> E[Play Thread]
    E --> F[Decode Video Frame]
    E --> G[Decode Audio Frame]
    F --> H[Display]
    G --> I[AudioPlayer]
    H --> J[SDL Render]
    I --> K[SDL Audio Play]
```

## 4. 线程模型

### 4.1 多线程版本 (当前实现)

```
主线程:
└── 处理用户输入

视频解码线程:
└── 解码视频帧 → 视频帧队列

音频解码线程:
└── 解码音频帧 → 音频帧队列

渲染线程 (主循环):
├── 从视频帧队列取帧
├── 渲染视频
├── 从音频帧队列取帧
├── 播放音频
└── 控制音视频同步
```

**多线程架构图**:

```mermaid
graph TB
    subgraph DecodeThreads["解码线程"]
        L1[VideoDecodeThread]
        L2[AudioDecodeThread]
    end
    
    subgraph Queues["帧队列"]
        Q1[VideoFrameQueue]
        Q2[AudioFrameQueue]
    end
    
    subgraph RenderThread["渲染线程"]
        R1[renderLoop]
        R2[SyncManager]
    end
    
    L1 --> Q1
    L2 --> Q2
    Q1 --> R1
    Q2 --> R1
    R1 --> R2
    
    style DecodeThreads fill:#e1e1ff
    style Queues fill:#ffffe1
    style RenderThread fill:#f5e1ff
```

**优点**:
- 音视频同步更准确
- 性能更好，解码不阻塞渲染
- 支持预解码机制

**实现细节**:
- FrameQueue 使用条件变量实现阻塞等待，避免 CPU 忙轮询
- 解码线程和渲染线程通过帧队列解耦
- SyncManager 统一管理音视频同步

## 5. 音视频同步

### 5.1 同步策略

当前使用基于 SyncManager 的多模式同步：

```cpp
enum class SyncMode {
    AudioMaster,   // 音频为主时钟
    VideoMaster,  // 视频为主时钟
    Free          // 自由播放
};
```

**默认模式**: AudioMaster - 以音频时钟为基准

### 5.2 同步实现

```cpp
double SyncManager::calculateDelay(double current_pts) {
    double diff = current_pts - master_clock_.load();
    
    if (std::abs(diff) < sync_threshold_) {
        return 0.0;  // 同步，无需等待
    }
    
    return diff;  // 等待或加速
}

bool SyncManager::shouldSkipFrame(double frame_pts) {
    double diff = frame_pts - master_clock_.load();
    return diff > sync_threshold_ * 2;  // 落后太多，跳帧
}

bool SyncManager::shouldRepeatFrame(double frame_pts) {
    double diff = master_clock_.load() - frame_pts;
    return diff > sync_threshold_ * 2;  // 领先太多，重复帧
}
```

## 6. 错误处理

### 6.1 错误类型

1. **文件错误**: 文件不存在、格式不支持
2. **解码错误**: 编解码器打开失败、解码失败
3. **显示错误**: 窗口创建失败、渲染失败
4. **音频错误**: 音频设备打开失败

### 6.2 错误处理策略

```cpp
bool open(const std::string& filename) {
    if (avformat_open_input(...) != 0) {
        std::cerr << "Error: Could not open file" << std::endl;
        return false;
    }
    
    if (!video_decoder_->open(...)) {
        close();  // 清理已分配的资源
        return false;
    }
    
    return true;
}
```

## 7. 资源管理

### 7.1 RAII 模式

使用 C++ 的 RAII (Resource Acquisition Is Initialization) 管理资源：

```cpp
class VideoDecoder {
    ~VideoDecoder() {
        close();  // 析构时自动清理
    }
    
    void close() {
        if (codec_ctx_) {
            avcodec_free_context(&codec_ctx_);
        }
        if (sws_ctx_) {
            sws_freeContext(sws_ctx_);
        }
    }
};
```

### 7.2 智能指针

使用 `std::unique_ptr` 管理模块生命周期：

```cpp
class VideoPlayer {
    std::unique_ptr<VideoDecoder> video_decoder_;
    std::unique_ptr<Display> display_;
};
```

## 8. 性能优化

### 8.1 帧队列

使用队列缓存解码后的帧：

```cpp
std::queue<VideoFrame> video_queue_;
std::mutex queue_mutex_;
```

### 8.2 格式转换缓存

复用格式转换上下文：

```cpp
if (!sws_ctx_) {
    sws_ctx_ = sws_getContext(...);
}
```

### 8.3 硬件加速

使用硬件解码器：

```cpp
AVCodec* codec = avcodec_find_decoder_by_name("h264_cuvid");
```

## 9. 扩展功能

### 9.1 播放列表

```cpp
class Playlist {
    std::vector<std::string> files_;
    int current_index_;
    
    void next();
    void previous();
};
```

### 9.2 字幕支持

```cpp
class SubtitleDecoder {
    bool decodeSubtitle(SubtitleFrame& frame);
    void renderSubtitle(Display& display);
};
```

### 9.3 倍速播放

```cpp
void setPlaybackSpeed(double speed) {
    playback_speed_ = speed;
    // 调整延迟计算
    double delay = (pts - current_time_) / speed;
}
```

## 10. 测试策略

### 10.1 单元测试

测试各个模块的功能：

```cpp
TEST(VideoDecoder, OpenAndDecode) {
    VideoDecoder decoder;
    EXPECT_TRUE(decoder.open(fmt_ctx, stream_idx));
    
    VideoFrame frame;
    EXPECT_TRUE(decoder.decodeFrame(frame));
}
```

### 10.2 集成测试

测试整体播放流程：

```cpp
TEST(VideoPlayer, PlayVideo) {
    VideoPlayer player;
    EXPECT_TRUE(player.open("test.mp4"));
    
    player.play();
    EXPECT_TRUE(player.isPlaying());
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    player.stop();
}
```

## 11. 部署

### 11.1 Linux

```bash
# 编译
mkdir build && cd build
cmake ..
make

# 安装
sudo make install
```

### 11.2 打包

```bash
# 创建 AppImage
mkdir -p AppDir/usr/bin
cp VideoPlayer AppDir/usr/bin/
linuxdeploy-x86_64.AppImage --appdir AppDir --output appimage
```

## 12. 维护

### 12.1 日志系统

```cpp
#include <spdlog/spdlog.h>

spdlog::info("Playing video: {}", filename);
spdlog::error("Failed to open codec");
```

### 12.2 配置管理

```cpp
struct Config {
    int volume = 100;
    bool fullscreen = false;
    std::string theme = "dark";
};

Config loadConfig();
void saveConfig(const Config& config);
```

## 总结

本架构设计采用模块化、面向对象的方式，充分利用 C++17 新特性，实现了清晰、高效的视频播放器。通过良好的架构设计，便于后续功能扩展和性能优化。
