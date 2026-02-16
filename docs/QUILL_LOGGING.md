# Quill 日志库集成指南

本指南说明如何安装和使用 Quill 日志库。

## 什么是 Quill？

Quill 是一个高性能的 C++ 异步日志库，具有以下特点：

- 极高的性能（异步日志记录）
- 线程安全
- 支持多种日志级别
- 自动日志文件轮转
- 同时支持文件和控制台输出
- 跨平台支持（Windows, Linux, macOS）

## 安装 Quill

### Windows

#### 使用 vcpkg (推荐）

```powershell
vcpkg install quill:x64-windows
```

#### 手动安装

1. 从 [Quill Releases](https://github.com/odygrd/quill/releases) 下载最新版本
2. 解压到 `external/quill/` 目录
3. 在 CMakeLists.txt 中会自动找到

### Linux

```bash
# 方法1: 使用 vcpkg
vcpkg install quill:x64-linux

# 方法2: 从源码编译
git clone https://github.com/odygrd/quill.git external/quill
cd external/quill
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### macOS

```bash
# 使用 Homebrew
brew install quill

# 或从源码编译
git clone https://github.com/odygrd/quill.git external/quill
cd external/quill
mkdir build && cd build
cmake ..
make
sudo make install
```

## 项目集成

### 代码结构

```
include/
└── logger.h           # 日志系统封装

src/
└── logger.cpp         # 日志系统实现
```

### 使用方法

#### 1. 包含头文件

```cpp
#include "logger.h"
```

#### 2. 初始化日志系统

在 `main()` 函数开始时：

```cpp
int main(int argc, char* argv[]) {
    Logger::init();
    
    // ... 你的代码 ...
    
    Logger::shutdown();
    return 0;
}
```

#### 3. 记录日志

```cpp
Logger::info("This is an info message");
Logger::warning("This is a warning message");
Logger::error("This is an error message");
Logger::debug("This is a debug message");
```

### 日志输出

默认情况下，日志会：

1. **文件输出**: `video_player.log`
   - 包含 INFO、WARNING、ERROR 级别的日志
   - 自动轮转（文件过大时自动分割）

2. **控制台输出**: 标准输出
   - 包含所有级别（包括 DEBUG）的日志
   - 带颜色输出（终端支持时）

### 未安装 Quill 时的行为

如果未安装 Quill，项目会自动降级使用 `std::cout` 和 `std::cerr`：

```cpp
// 如果未定义 USE_QUILL_LOGGING
LOG_INFO(msg)    → std::cout << "[INFO] " << msg << std::endl
LOG_WARNING(msg) → std::cout << "[WARNING] " << msg << std::endl
LOG_ERROR(msg)  → std::cerr << "[ERROR] " << msg << std::endl
LOG_DEBUG(msg)  → std::cout << "[DEBUG] " << msg << std::endl
```

## 配置选项

### 修改日志文件路径

在 `src/logger.cpp` 中修改：

```cpp
auto file_handler = quill::FileHandler(
    "your_custom_log.log",  // 修改这里
    []() { return quill::RotatingFileHandlerConfig(); },
    FileHandlerNotifier{}
);
```

### 修改日志级别

#### 文件输出级别

```cpp
file_handler->set_log_level(quill::LogLevel::Info);  // 修改这里
// 可选: TraceL, DebugL, InfoL, WarningL, ErrorL, CriticalL
```

#### 控制台输出级别

```cpp
console_handler->set_log_level(quill::LogLevel::Debug);  // 修改这里
```

### 修改日志轮转策略

```cpp
auto config = quill::RotatingFileHandlerConfig();
config.set_rotation_max_file_size(100 * 1024 * 1024);  // 100MB
config.set_rotation_max_backup_files(10);               // 保留 10 个备份
```

## 性能说明

Quill 是一个异步日志库，具有以下性能优势：

- **低延迟**: 日志记录不会阻塞主线程
- **高吞吐**: 每秒可记录数百万条日志
- **批量写入**: 多条日志批量写入文件，减少 I/O 操作

## 示例输出

### 文件日志

```
2025-02-12 08:30:45.123 [INFO] [root] Starting Modern Video Player
2025-02-12 08:30:45.234 [INFO] [root] Opening file: test.mp4
2025-02-12 08:30:45.456 [INFO] [root] Video decoder opened: 1920x1080
2025-02-12 08:30:45.567 [WARNING] [root] Failed to open audio decoder
2025-02-12 08:30:45.678 [INFO] [root] Starting playback...
2025-02-12 08:30:50.789 [INFO] [root] Playback finished
```

### 控制台日志

```
[DEBUG] Initializing logger...
[INFO]  Starting Modern Video Player
[INFO]  Opening file: test.mp4
[INFO]  Video decoder opened: 1920x1080
[WARNING] Failed to open audio decoder
[INFO]  Starting playback...
[INFO]  Playback finished
[DEBUG] Shutting down logger...
```

## 故障排除

### 问题: 找不到 quill.h

**解决方案**: 确保 Quill 已正确安装并设置了路径：

```bash
cmake .. -DCMAKE_PREFIX_PATH=/path/to/quill
```

### 问题: 链接错误

**解决方案**: 确保 Quill 已编译并安装：

```bash
cd external/quill/build
sudo make install
```

### 问题: 日志文件未生成

**解决方案**: 检查是否有写入权限，或修改日志文件路径。

## 最佳实践

1. **尽早初始化**: 在 `main()` 函数开始时初始化日志系统
2. **正确关闭**: 在程序退出前调用 `Logger::shutdown()`
3. **合理使用日志级别**:
   - DEBUG: 详细的调试信息
   - INFO: 重要的运行状态
   - WARNING: 不影响运行的问题
   - ERROR: 影响运行的问题
4. **避免循环中频繁日志**: 在性能关键的代码中减少日志输出
5. **使用结构化日志**: 使用字符串格式化而不是多个参数

## 更多资源

- [Quill GitHub](https://github.com/odygrd/quill)
- [Quill 文档](https://quill.readthedocs.io/)
- [Quill 性能测试](https://github.com/odygrd/quill#benchmarks)

## 扩展功能

### 添加自定义格式化器

```cpp
class CustomFormatter : public quill::LogFormatter {
public:
    void format(quill::LogRecord const& record, std::string& out) override {
        // 自定义格式化逻辑
    }
};
```

### 添加多个文件处理器

```cpp
auto error_handler = quill::FileHandler("error.log", ...);
error_handler->set_log_level(quill::LogLevel::Error);
logger->add_handler(error_handler);
```

### 网络日志

```cpp
auto network_handler = quill::TcpHandler{"127.0.0.1", 8080};
logger->add_handler(network_handler);
```
