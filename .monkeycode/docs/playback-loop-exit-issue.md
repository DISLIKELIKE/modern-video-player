# 播放循环立即退出问题分析

## 问题现象

运行视频播放器时，程序正常初始化但立即退出，无法播放视频：

```
[DEBUG] playLoop started, stopped=0, display=valid, shouldQuit=0
```

调试信息显示循环条件满足，但循环立即退出。

## 问题分析

### 可能原因

1. **SDL 窗口事件处理问题**
   - `handleEvents()` 在循环第一次迭代时可能检测到 `SDL_QUIT` 事件
   - 窗口创建后可能立即收到关闭信号

2. **解码器问题**
   - `decodeFrame()` 第一次调用就失败
   - 但调试日志未显示 decodeFrame 失败消息，说明可能没有进入该分支

3. **时序问题**
   - `handleEvents()` 在解码之前被调用，可能在窗口完全初始化前就检测到事件

### 代码流程分析

```cpp
while (!stopped_.load() && display_ && !display_->shouldQuit()) {
    // ...
    if (video_decoder_ && !seeking_.load()) {
        if (video_decoder_->decodeFrame(video_frame) && video_frame.isValid()) {
            video_pts = video_frame.pts();
            display_->handleEvents();  // 事件处理在解码成功后
            // ...
        } else {
            // decodeFrame 失败时会输出日志
        }
    }
    // ...
}
```

循环退出的唯一原因是 `shouldQuit()` 变为 `true`。

## 解决方案

### 方案一：在循环开始前处理初始事件

在进入播放循环前，先清空事件队列中的残留事件：

```cpp
void VideoPlayer::playLoop() {
    // 清空 SDL 事件队列中的初始事件
    if (display_) {
        SDL_PumpEvents();
        SDL_FlushEvent(SDL_QUIT);
        SDL_FlushEvent(SDL_WINDOWEVENT);
    }
    
    // ... 原有代码
}
```

### 方案二：延迟窗口关闭检测

在窗口创建后添加短暂延迟，确保窗口完全初始化：

```cpp
bool Display::init(int width, int height, const std::string& title) {
    // ... 原有初始化代码
    
    // 添加短暂延迟确保窗口稳定
    SDL_Delay(100);
    
    // 清空事件队列
    SDL_PumpEvents();
    SDL_FlushEvents(SDL_FIRSTEVENT, SDL_LASTEVENT);
    
    return true;
}
```

### 方案三：修改事件处理逻辑（推荐）

在 `handleEvents()` 中忽略窗口刚创建时的 `SDL_QUIT` 事件：

```cpp
void Display::handleEvents() {
    static auto init_time = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        // 忽略初始化期间的事件
        if (std::chrono::steady_clock::now() < init_time) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT) {
                continue;
            }
        }
        
        switch (event.type) {
            case SDL_QUIT:
                should_quit_ = true;
                break;
            // ... 其他事件处理
        }
    }
}
```

### 方案四：在循环开始处清空事件队列

修改 `playLoop()` 在每次循环开始时先处理事件：

```cpp
void VideoPlayer::playLoop() {
    // ... 初始化代码
    
    // 首次清空事件队列
    display_->handleEvents();
    
    while (!stopped_.load() && display_ && !display_->shouldQuit()) {
        display_->handleEvents();  // 先处理事件
        
        // ... 解码和渲染代码
    }
}
```

## 调试步骤

1. 添加更详细的日志，确定循环退出的具体原因：

```cpp
while (!stopped_.load() && display_ && !display_->shouldQuit()) {
    std::cerr << "[DEBUG] Loop iteration, shouldQuit=" << display_->shouldQuit() << std::endl;
    // ...
}
std::cerr << "[DEBUG] Loop exited, stopped=" << stopped_.load() 
          << ", display=" << (display_ ? "valid" : "null")
          << ", shouldQuit=" << (display_ ? display_->shouldQuit() : false) << std::endl;
```

2. 在 `handleEvents()` 中记录接收到的事件类型：

```cpp
void Display::handleEvents() {
    SDL_Event event;
    
    while (SDL_PollEvent(&event)) {
        std::cerr << "[DEBUG] Received event type: " << event.type << std::endl;
        // ...
    }
}
```

## 推荐修复步骤

1. 首先添加更详细的调试日志，确认是哪个条件导致循环退出
2. 根据调试结果选择合适的解决方案
3. 测试修复后的播放功能
4. 移除调试日志

## 相关文件

- `src/video_player.cpp` - 播放循环实现
- `src/display.cpp` - SDL 事件处理
- `include/video_player.h` - VideoPlayer 类定义
