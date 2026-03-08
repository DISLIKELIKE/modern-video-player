# 实现步骤文档

## 状态说明（2026-03-08）

- 本文档主要保留项目早期“从零搭建原型播放器”的教程性实现记录，适合作为 FFmpeg + SDL2 播放器的学习参考。
- 文中出现的 `video_decoder`、`audio_decoder`、单体 `playLoop` 等路径与流程，属于早期原型阶段，不再代表当前仓库的实际主链结构。
- 当前仓库已经收敛到 `VideoPlayer -> PlayerCore -> Scheduler -> core/*` 主链，并扩展了 `decoder/`、`render/`、`playlist/`、`subtitle/`、`input/` 等模块。
- 如需理解现行实现，请优先结合 `docs/ARCHITECTURE_REFACTOR_2026-03-06.md`、`docs/MPC_HC_GAP_ANALYSIS.md` 与实际代码阅读。

## 项目依赖版本

| 组件 | 版本 |
|------|------|
| FFmpeg | 8.0.1 |
| SDL2 | 2.30.11 |
| Quill | 6.0.0 |
| CMake | 3.15+ |
| C++ | C++17 |

## 项目构建步骤

本文档详细说明如何从零开始构建一个基于 FFmpeg + SDL2 的 C++17 视频播放器原型。

**说明**:
- 以下目录结构、文件名和代码片段用于讲解早期原型的演进过程。
- 它们不等同于当前仓库的完整文件布局，也不应视为当前主分支的逐文件实施清单。

## 阶段一：环境准备

### 1.1 安装依赖库

#### Ubuntu/Debian 系统

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    git \
    libavformat-dev \
    libavcodec-dev \
    libswscale-dev \
    libswresample-dev \
    libavutil-dev \
    libsdl2-dev
```

**注意**: Ubuntu/Debian 默认仓库中的 FFmpeg 版本可能低于 8.0。如需 8.0+ 版本，请从源码编译。

#### 验证安装

```bash
ffmpeg -version
sdl2-config --version
cmake --version
g++ --version
```

### 1.2 创建项目目录结构

```bash
mkdir VideoPlayer
cd VideoPlayer
mkdir -p include src docs build
```

## 阶段二：构建系统配置

### 2.1 创建 CMakeLists.txt

CMakeLists.txt 是项目的构建配置文件，定义了如何编译和链接项目。

```cmake
cmake_minimum_required(VERSION 3.15)
project(VideoPlayer VERSION 1.0.0 LANGUAGES C CXX)

# 设置 C++17 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic")

# 查找 SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# 包含头文件目录
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

# 链接库目录
link_directories(
    ${SDL2_LIBRARY_DIRS}
)

# 定义可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    src/video_player.cpp
    src/video_decoder.cpp
    src/audio_decoder.cpp
    src/display.cpp
    src/audio_player.cpp
)

# 链接库
target_link_libraries(${PROJECT_NAME}
    ${SDL2_LIBRARIES}
    avformat
    avcodec
    avutil
    swscale
    swresample
    SDL2
    pthread
    m
)
```

**关键点说明**：
- `CMAKE_CXX_STANDARD 17`: 使用 C++17 标准
- `find_package(PkgConfig)`: 查找 pkg-config 工具
- `pkg_check_modules`: 查找 SDL2 库
- `add_executable`: 定义可执行文件及其源文件
- `target_link_libraries`: 链接所需的库

## 阶段三：视频解码器实现

### 3.1 理解 FFmpeg 解码流程

FFmpeg 解码视频的基本流程：

```
打开文件 → 查找流信息 → 查找解码器 → 打开解码器 → 
读取数据包 → 发送数据包到解码器 → 接收解码后的帧 → 
格式转换 → 显示
```

### 3.2 创建视频解码器头文件

**文件**: `include/video_decoder.h`

```cpp
#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <string>
#include <memory>
#include <mutex>

namespace vp {

// 视频帧封装类
class VideoFrame {
public:
    VideoFrame();
    ~VideoFrame();
    
    bool isValid() const { return frame_ != nullptr; }
    AVFrame* get() { return frame_; }
    
    double pts() const { return pts_; }
    void setPts(double pts) { pts_ = pts; }
    
private:
    AVFrame* frame_;  // FFmpeg 帧对象
    double pts_;      // 显示时间戳
};

// 视频解码器类
class VideoDecoder {
public:
    VideoDecoder();
    ~VideoDecoder();
    
    // 打开解码器
    bool open(AVFormatContext* fmt_ctx, int stream_idx);
    
    // 关闭解码器
    void close();
    
    // 解码一帧
    bool decodeFrame(VideoFrame& frame);
    
    // 刷新解码器缓冲区
    void flush();
    
    // 获取视频信息
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }
    AVPixelFormat getPixelFormat() const { return pix_fmt_; }
    AVRational getTimeBase() const { return time_base_; }

private:
    AVCodecContext* codec_ctx_;  // 编解码器上下文
    SwsContext* sws_ctx_;       // 格式转换上下文
    int stream_idx_;            // 视频流索引
    
    int width_;
    int height_;
    AVPixelFormat pix_fmt_;
    AVRational time_base_;
    
    std::mutex mutex_;  // 互斥锁，保护共享资源
};

} // namespace vp
```

**设计要点**：
1. 使用 RAII 管理 FFmpeg 资源
2. 封装 AVFrame 为 VideoFrame 类
3. 使用 std::mutex 保证线程安全
4. 提供 flush() 方法用于 seek 操作

### 3.3 实现视频解码器

**文件**: `src/video_decoder.cpp`

**步骤 1: 构造函数和析构函数**

```cpp
VideoFrame::VideoFrame()
    : frame_(nullptr)
    , pts_(0.0) {
    frame_ = av_frame_alloc();  // 分配帧内存
}

VideoFrame::~VideoFrame() {
    if (frame_) {
        av_frame_free(&frame_);  // 释放帧内存
    }
}

VideoDecoder::VideoDecoder()
    : codec_ctx_(nullptr)
    , sws_ctx_(nullptr)
    , stream_idx_(-1)
    , width_(0)
    , height_(0)
    , pix_fmt_(AV_PIX_FMT_NONE) {
}

VideoDecoder::~VideoDecoder() {
    close();  // 析构时自动清理资源
}
```

**步骤 2: 打开解码器**

```cpp
bool VideoDecoder::open(AVFormatContext* fmt_ctx, int stream_idx) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!fmt_ctx || stream_idx < 0) {
        return false;
    }
    
    stream_idx_ = stream_idx;
    AVStream* stream = fmt_ctx->streams[stream_idx];
    AVCodecParameters* codecpar = stream->codecpar;
    
    // 查找解码器
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        std::cerr << "Error: Unsupported codec" << std::endl;
        return false;
    }
    
    // 分配备解码器上下文
    codec_ctx_ = avcodec_alloc_context3(codec);
    if (!codec_ctx_) {
        std::cerr << "Error: Could not allocate codec context" << std::endl;
        return false;
    }
    
    // 复制编解码器参数到上下文
    if (avcodec_parameters_to_context(codec_ctx_, codecpar) < 0) {
        std::cerr << "Error: Could not copy codec parameters" << std::endl;
        close();
        return false;
    }
    
    // 打开解码器
    if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
        std::cerr << "Error: Could not open codec" << std::endl;
        close();
        return false;
    }
    
    // 保存视频信息
    width_ = codec_ctx_->width;
    height_ = codec_ctx_->height;
    pix_fmt_ = codec_ctx_->pix_fmt;
    time_base_ = stream->time_base;
    
    return true;
}
```

**关键函数说明**：
- `avcodec_find_decoder()`: 根据 codec_id 查找解码器
- `avcodec_alloc_context3()`: 分配解码器上下文
- `avcodec_parameters_to_context()`: 复制参数
- `avcodec_open2()`: 打开解码器

**步骤 3: 解码一帧**

```cpp
bool VideoDecoder::decodeFrame(VideoFrame& frame) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!codec_ctx_) {
        return false;
    }
    
    AVPacket* packet = av_packet_alloc();
    if (!packet) {
        return false;
    }
    
    // 从文件中读取数据包
    int ret = av_read_frame(fmt_ctx, packet);
    
    if (ret < 0) {
        av_packet_free(&packet);
        return false;  // 文件结束或出错
    }
    
    // 检查是否是视频流
    if (packet->stream_index != stream_idx_) {
        av_packet_unref(packet);
        av_packet_free(&packet);
        return false;
    }
    
    // 发送数据包到解码器
    ret = avcodec_send_packet(codec_ctx_, packet);
    av_packet_unref(packet);
    av_packet_free(&packet);
    
    if (ret < 0) {
        return false;
    }
    
    // 从解码器接收解码后的帧
    ret = avcodec_receive_frame(codec_ctx_, frame.get());
    if (ret < 0) {
        return false;
    }
    
    // 计算 PTS (Presentation Time Stamp)
    if (frame.get()->pts != AV_NOPTS_VALUE) {
        frame.setPts(frame.get()->pts * av_q2d(time_base_));
    }
    
    return true;
}
```

**解码流程**：
1. `av_packet_alloc()`: 分配数据包
2. `av_read_frame()`: 从文件读取数据包
3. `avcodec_send_packet()`: 发送数据包到解码器
4. `avcodec_receive_frame()`: 接收解码后的帧

**步骤 4: 刷新解码器**

```cpp
void VideoDecoder::flush() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (codec_ctx_) {
        avcodec_flush_buffers(codec_ctx_);  // 刷新解码器缓冲区
    }
    
    if (sws_ctx_) {
        sws_freeContext(sws_ctx_);  // 释放格式转换上下文
        sws_ctx_ = nullptr;
    }
}
```

## 阶段四：显示模块实现

### 4.1 理解 SDL2 显示流程

SDL2 显示视频的基本流程：

```
创建窗口 → 创建渲染器 → 创建纹理 → 
更新纹理数据 → 渲染纹理 → 显示到屏幕
```

### 4.2 创建显示头文件

**文件**: `include/display.h`

```cpp
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <memory>

namespace vp {

class Display {
public:
    Display();
    ~Display();
    
    // 初始化显示
    bool init(int width, int height, const std::string& title);
    
    // 关闭显示
    void close();
    
    // 渲染帧
    void renderFrame(const uint8_t* data, int width, int height);
    
    // 显示到屏幕
    void present();
    
    // 清空屏幕
    void clear();
    
    // 处理事件
    void handleEvents();
    bool shouldQuit() const { return should_quit_; }
    
    // 全屏切换
    void toggleFullscreen();
    
    // 获取尺寸
    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

private:
    SDL_Window* window_;
    SDL_Renderer* renderer_;
    SDL_Texture* texture_;
    
    int width_;
    int height_;
    bool should_quit_;
    bool fullscreen_;
    bool initialized_;
};

} // namespace vp
```

### 4.3 实现显示模块

**文件**: `src/display.cpp`

**步骤 1: 初始化**

```cpp
bool Display::init(int width, int height, const std::string& title) {
    if (initialized_) {
        close();
    }
    
    width_ = width;
    height_ = height;
    
    // 创建窗口
    window_ = SDL_CreateWindow(
        title.c_str(),
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
    );
    
    if (!window_) {
        std::cerr << "Error: Could not create SDL window" << std::endl;
        return false;
    }
    
    // 创建渲染器
    renderer_ = SDL_CreateRenderer(window_, -1, 
        SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    
    if (!renderer_) {
        std::cerr << "Error: Could not create SDL renderer" << std::endl;
        SDL_DestroyWindow(window_);
        return false;
    }
    
    // 创建 YUV 纹理
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_IYUV,
        SDL_TEXTUREACCESS_STREAMING, width, height);
    
    if (!texture_) {
        std::cerr << "Error: Could not create SDL texture" << std::endl;
        SDL_DestroyRenderer(renderer_);
        SDL_DestroyWindow(window_);
        return false;
    }
    
    initialized_ = true;
    return true;
}
```

**步骤 2: 渲染帧**

```cpp
void Display::renderFrame(const uint8_t* data, int width, int height) {
    if (!renderer_ || !texture_ || !data) {
        return;
    }
    
    // 清空渲染器
    SDL_RenderClear(renderer_);
    
    // 更新 YUV 纹理
    // YUV420 格式: Y 平面 + U 平面 + V 平面
    int ret = SDL_UpdateYUVTexture(
        texture_,
        nullptr,
        data,                    // Y 平面
        width,                   // Y 平面行大小
        data + width * height,   // U 平面 (偏移)
        width / 2,               // U 平面行大小
        data + width * height * 5 / 4,  // V 平面 (偏移)
        width / 2                // V 平面行大小
    );
    
    if (ret < 0) {
        std::cerr << "Error: Could not update YUV texture" << std::endl;
        return;
    }
    
    // 复制纹理到渲染器
    SDL_Rect dst_rect = {0, 0, width_, height_};
    SDL_RenderCopy(renderer_, texture_, nullptr, &dst_rect);
}
```

**YUV420 格式说明**：
- Y 平面: 亮度信息，大小为 width × height
- U 平面: 色度信息，大小为 (width/2) × (height/2)
- V 平面: 色度信息，大小为 (width/2) × (height/2)

**步骤 3: 显示**

```cpp
void Display::present() {
    if (renderer_) {
        SDL_RenderPresent(renderer_);  // 将渲染内容显示到屏幕
    }
}
```

## 阶段五：音频解码器实现

### 5.1 理解音频解码

音频解码流程与视频类似，但需要格式转换：

```
读取数据包 → 发送到解码器 → 接收 PCM 帧 → 
格式转换 → 发送到音频设备 → 播放
```

### 5.2 创建音频解码器

**文件**: `include/audio_decoder.h`

```cpp
#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}

#include <string>
#include <memory>
#include <mutex>

namespace vp {

class AudioFrame {
public:
    AudioFrame();
    ~AudioFrame();
    
    bool isValid() const { return frame_ != nullptr; }
    AVFrame* get() { return frame_; }
    
    double pts() const { return pts_; }
    void setPts(double pts) { pts_ = pts; }
    
private:
    AVFrame* frame_;
    double pts_;
};

class AudioDecoder {
public:
    AudioDecoder();
    ~AudioDecoder();
    
    bool open(AVFormatContext* fmt_ctx, int stream_idx);
    void close();
    bool decodeFrame(AudioFrame& frame);
    void flush();
    
    int getSampleRate() const { return sample_rate_; }
    int getChannels() const { return channels_; }
    AVSampleFormat getSampleFormat() const { return sample_fmt_; }
    
private:
    AVCodecContext* codec_ctx_;
    SwrContext* swr_ctx_;
    int stream_idx_;
    
    int sample_rate_;
    int channels_;
    AVSampleFormat sample_fmt_;
    
    std::mutex mutex_;
};

} // namespace vp
```

## 阶段六：音频播放器实现

### 6.1 SDL2 音频播放

SDL2 使用回调函数播放音频：

```cpp
void audioCallback(void* userdata, uint8_t* stream, int len) {
    // 从队列中获取音频数据并填充到 stream
}
```

### 6.2 创建音频播放器

**文件**: `include/audio_player.h`

```cpp
#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include <queue>
#include <mutex>

namespace vp {

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();
    
    bool init(int sample_rate, int channels);
    void close();
    
    void play(const std::vector<uint8_t>& data);
    void pause();
    void resume();
    void stop();
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }

private:
    static void audioCallback(void* userdata, uint8_t* stream, int len);
    
    SDL_AudioDeviceID audio_device_;
    SDL_AudioSpec audio_spec_;
    
    int sample_rate_;
    int channels_;
    float volume_;
    
    std::queue<std::vector<uint8_t>> audio_queue_;
    std::mutex queue_mutex_;
    
    bool initialized_;
};

} // namespace vp
```

## 阶段七：主播放器实现

### 7.1 播放器架构

主播放器负责协调各个模块：

```
VideoPlayer
├── VideoDecoder (视频解码)
├── AudioDecoder (音频解码)
├── Display (显示)
├── AudioPlayer (音频播放)
└── Play Thread (播放线程，实现音视频同步)
```

### 7.2 创建主播放器

**文件**: `include/video_player.h`

```cpp
#pragma once

#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>

namespace vp {

class VideoDecoder;
class AudioDecoder;
class Display;
class AudioPlayer;

class VideoPlayer {
public:
    VideoPlayer();
    ~VideoPlayer();

    bool open(const std::string& filename);
    void close();
    
    void play();
    void pause();
    void stop();
    void seek(double timestamp);
    
    bool isPlaying() const { return playing_.load(); }
    double getDuration() const { return duration_; }
    double getCurrentTime() const { return current_time_.load(); }
    
    void setVolume(float volume);
    float getVolume() const { return volume_; }

private:
    void playLoop();
    
    std::unique_ptr<VideoDecoder> video_decoder_;
    std::unique_ptr<AudioDecoder> audio_decoder_;
    std::unique_ptr<Display> display_;
    std::unique_ptr<AudioPlayer> audio_player_;
    
    std::atomic<bool> playing_;
    std::atomic<bool> paused_;
    std::atomic<bool> stopped_;
    
    double duration_;
    std::atomic<double> current_time_;
    float volume_;
    
    std::thread play_thread_;
    std::mutex mutex_;
    std::condition_variable cv_;
};

} // namespace vp
```

### 7.3 实现播放循环

**文件**: `src/video_player.cpp`

```cpp
void VideoPlayer::playLoop() {
    VideoFrame video_frame;
    
    auto start_time = Clock::now();
    
    while (!stopped_.load() && display_ && !display_->shouldQuit()) {
        if (paused_.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            start_time = Clock::now() - 
                         std::chrono::duration<double>(current_time_.load());
            continue;
        }
        
        // 计算当前播放时间
        auto current = Clock::now();
        double elapsed = std::chrono::duration<double>(current - start_time).count();
        current_time_ = elapsed;
        
        // 解码并显示视频帧
        if (video_decoder_) {
            if (video_decoder_->decodeFrame(video_frame) && video_frame.isValid()) {
                display_->handleEvents();
                
                AVFrame* frame = video_frame.get();
                display_->renderFrame(frame->data[0], frame->width, frame->height);
                display_->present();
                
                // 根据帧的 PTS 控制播放速度
                double delay = video_frame.pts() - elapsed;
                if (delay > 0.01) {
                    std::this_thread::sleep_for(std::chrono::duration<double>(delay));
                }
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
```

## 阶段八：主程序

### 8.1 创建主程序

**文件**: `src/main.cpp`

```cpp
#include "video_player.h"
#include <iostream>
#include <csignal>
#include <memory>

using namespace vp;

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage: " << argv[0] << " <video_file>" << std::endl;
        return 1;
    }
    
    // 创建播放器
    auto player = std::make_unique<VideoPlayer>();
    
    // 打开视频文件
    if (!player->open(argv[1])) {
        std::cerr << "Error: Could not open video file" << std::endl;
        return 1;
    }
    
    // 播放视频
    player->play();
    
    // 等待播放结束
    while (player->isPlaying()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    // 清理
    player->stop();
    player->close();
    
    return 0;
}
```

## 阶段九：编译和运行

### 9.1 编译项目

```bash
cd build
cmake ..
make
```

### 9.2 运行播放器

```bash
./VideoPlayer your_video.mp4
```

## 总结

本实现涵盖了：

1. **环境准备**: 安装 FFmpeg 和 SDL2
2. **构建系统**: CMake 配置
3. **视频解码**: FFmpeg 解码流程
4. **显示模块**: SDL2 渲染
5. **音频解码**: FFmpeg 音频处理
6. **音频播放**: SDL2 音频回调
7. **主播放器**: 模块协调
8. **主程序**: 入口函数

通过这个项目，您可以深入学习：
- FFmpeg 的编解码 API
- SDL2 的显示和音频 API
- C++17 新特性应用
- 多线程编程
- 音视频同步算法
