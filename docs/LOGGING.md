# 日志系统说明

## 当前状态

**Quill 日志库已被禁用，项目使用 std::cout 替代。**

由于 Quill v6.x API 与项目不兼容，项目暂时禁用了 Quill 日志库，改用标准的 `std::cout` 和 `std::cerr` 进行日志输出。

## 项目依赖版本

| 组件 | 版本 |
|------|------|
| FFmpeg | 8.0.1 |
| SDL2 | 2.30.11 |
| CMake | 3.15+ |
| C++ | C++17 |

**注意**: 不需要安装 Quill 日志库。

## 日志系统架构

### 代码结构

```
include/
└── logger.h           # 日志系统封装

src/
└── logger.cpp         # 日志系统实现（使用 std::cout）
```

### 日志宏定义

项目定义了以下日志宏（在 DEBUG_MODE 下启用）：

```cpp
LOG_DEBUG(msg)           // 通用调试日志
LOG_TRACE_VIDEO(msg)    // 视频相关日志
LOG_TRACE_AUDIO(msg)    // 音频相关日志
LOG_TRACE_EVENT(msg)    // SDL 事件日志
LOG_TRACE_LOOP(msg)    // 播放循环日志
```

### 日志输出示例

```
[INFO] Starting Modern Video Player
[INFO] Opening file: test.mp4
Video decoder opened: 1920x1080, format: yuv420p
Display initialized: 1920x1080
[DEBUG] [LOOP] playLoop started, stopped=0, display=valid, shouldQuit=0
[DEBUG] [LOOP] Loop iteration 0, shouldQuit=0
[DEBUG] [VIDEO] Calling decodeFrame...
[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=1, expected=1
[DEBUG] [VIDEO] decodeFrame: success, pts=0
```

## 启用调试日志

在 CMake 配置时设置 `DEBUG_MODE` 选项：

```bash
# 启用调试日志（默认开启）
cmake -B build -DDEBUG_MODE=ON

# 禁用调试日志
cmake -B build -DDEBUG_MODE=OFF
```

### 调试日志分类

| 宏 | 说明 |
|---|---|
| `LOG_DEBUG(msg)` | 通用调试日志 |
| `LOG_TRACE_VIDEO(msg)` | 视频相关日志（解码、渲染） |
| `LOG_TRACE_AUDIO(msg)` | 音频相关日志 |
| `LOG_TRACE_EVENT(msg)` | SDL 事件日志 |
| `LOG_TRACE_LOOP(msg)` | 播放循环日志 |

## 日志系统实现

### Logger 类

项目使用简单的 Logger 类封装日志输出：

```cpp
namespace vp {

class Logger {
public:
    static void init();
    static void shutdown();
    
    static void info(const std::string& msg);
    static void warning(const std::string& msg);
    static void error(const std::string& msg);
    static void debug(const std::string& msg);
    
private:
    static bool initialized_;
};

} // namespace vp
```

### 实现代码

```cpp
void Logger::info(const std::string& msg) {
    std::cout << "[INFO] " << msg << std::endl;
}

void Logger::warning(const std::string& msg) {
    std::cout << "[WARNING] " << msg << std::endl;
}

void Logger::error(const std::string& msg) {
    std::cerr << "[ERROR] " << msg << std::endl;
}

void Logger::debug(const std::string& msg) {
    std::cout << "[DEBUG] " << msg << std::endl;
}
```

## 常见 SDL 事件类型

在调试时，以下 SDL 事件类型可能出现在日志中：

| 事件类型值 | 事件名称 |
|-----------|---------|
| 256 (0x100) | SDL_QUIT |
| 512 (0x200) | SDL_WINDOWEVENT |
| 768 (0x300) | SDL_KEYDOWN |
| 1024 (0x400) | SDL_MOUSEMOTION |
| 4352 (0x1100) | SDL_KEYUP |
| 769 (0x301) | SDL_TEXTINPUT |

## 相关文档

- [播放循环问题调试](./docs/playback-loop-exit-issue.md)
- [视频流索引问题分析](./docs/video-stream-index-fix.md)
