# Windows Setup Guide

## Windows 环境配置指南

本指南详细说明如何在 Windows 系统上配置和编译本项目。

## 前置要求

### 必需软件

1. **CMake** (3.15+)
   - 下载: https://cmake.org/download/
   - 安装时勾选 "Add CMake to the system PATH"

2. **编译器** (二选一)
   - **Visual Studio 2017+** (推荐)
     - 下载: https://visualstudio.microsoft.com/
     - 安装时选择 "使用 C++ 的桌面开发"
   - **MinGW-w64**
     - 下载: https://www.mingw-w64.org/
     - 或使用 MSYS2: https://www.msys2.org/

3. **Git** (可选，用于克隆仓库)
   - 下载: https://git-scm.com/download/win

## 方法一：使用 vcpkg (推荐)

vcpkg 是微软推出的 C++ 包管理器，使用最方便。

### 1. 安装 vcpkg

```powershell
# 克隆 vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg

# 运行启动脚本
.\bootstrap-vcpkg.bat

# 集成到 Visual Studio (可选)
.\vcpkg integrate install
```

### 2. 安装依赖库

```powershell
# 安装 SDL2 和 FFmpeg
.\vcpkg install sdl2 ffmpeg:x64-windows

# 如果使用 MinGW，使用 x64-mingw-static
# .\vcpkg install sdl2 ffmpeg:x64-mingw-static
```

### 3. 配置和编译项目

在项目根目录下：

```powershell
# 创建构建目录
mkdir build
cd build

# 配置项目（使用 vcpkg toolchain）
cmake .. -DCMAKE_TOOLCHAIN_FILE=[vcpkg路径]\scripts\buildsystems\vcpkg.cmake

# 编译项目
cmake --build . --config Release
```

### 4. 运行程序

```powershell
.\Release\modern-video-player.exe your_video.mp4
```

## 方法二：手动安装

### 1. 下载 FFmpeg

1. 访问 [gyan.dev](https://www.gyan.dev/ffmpeg/builds/)
2. 下载最新版本的 "ffmpeg-git-full.7z"
3. 解压到项目目录的 `external/ffmpeg/`

目录结构应该是：
```
modern-video-player/
├── external/
│   └── ffmpeg/
│       ├── include/           # 头文件
│       ├── lib/               # 库文件 (.lib, .a)
│       └── bin/               # DLL 文件
├── include/
├── src/
└── ...
```

### 2. 下载 SDL2

1. 访问 [SDL2 GitHub Releases](https://github.com/libsdl-org/SDL/releases)
2. 下载最新版本的 `SDL2-devel-VC.zip` (Visual Studio) 或 `SDL2-devel-mingw.zip` (MinGW)
3. 解压到项目目录的 `external/SDL2/`

目录结构应该是：
```
modern-video-player/
├── external/
│   ├── SDL2/
│   │   ├── include/
│   │   ├── lib/
│   │   └── bin/              # 包含 SDL2.dll
│   └── ffmpeg/
│       ├── include/
│       ├── lib/
│       └── bin/
├── include/
├── src/
└── ...
```

### 3. 配置 CMake

创建构建目录并配置：

```powershell
mkdir build
cd build

# 如果 FFmpeg 和 SDL2 在标准位置
cmake ..

# 如果需要指定路径
cmake .. -DSDL2_DIR=..\external\SDL2 -DFFMPEG_DIR=..\external\ffmpeg

# 编译
cmake --build . --config Release
```

### 4. 运行程序

```powershell
# 确保 SDL2.dll 在可执行文件目录中
# （构建脚本应该会自动复制）
.\Release\modern-video-player.exe your_video.mp4
```

## 方法三：使用预编译的 FFmpeg 共享版本

如果你遇到了 FFmpeg 静态库的链接问题，可以使用共享版本（DLL）。

### 1. 下载共享版本

1. 访问 [gyan.dev](https://www.gyan.dev/ffmpeg/builds/)
2. 下载 `ffmpeg-git-full.7z`
3. 解压后，将 `bin/` 目录下的所有 DLL 复制到项目输出目录

### 2. 修改 CMakeLists.txt (可选)

如果需要，可以在 CMakeLists.txt 中添加：

```cmake
# Windows 下使用共享库
if(WIN32)
    add_definitions(-DFFMPEG_SHARED)
endif()
```

## 常见问题

### 1. SDL2.dll 未找到

**错误信息**:
```
无法启动此程序，因为计算机中丢失 SDL2.dll
```

**解决方案**:
- 确保 SDL2.dll 在可执行文件同一目录
- 或将 SDL2.dll 添加到系统 PATH

### 2. FFmpeg 库未找到

**错误信息**:
```
LINK : fatal error LNK1104: 无法打开文件 "avformat.lib"
```

**解决方案**:
- 检查 FFMPEG_DIR 是否正确
- 确保库文件 (.lib) 在 `ffmpeg/lib/` 目录
- 对于 64 位编译，确保使用的是 64 位版本的 FFmpeg

### 3. CMake 找不到 SDL2

**错误信息**:
```
Could not find SDL2
```

**解决方案**:
- 设置环境变量: `SDL2_DIR=你的SDL2路径`
- 或在 CMake 时指定: `cmake .. -DSDL2_DIR=path/to/SDL2`

### 4. 编译器不匹配

**错误信息**:
```
fatal error C1083: 无法打开包括文件: "SDL.h"
```

**解决方案**:
- 确保使用 64 位编译器生成 64 位库
- Visual Studio: x64 Native Tools Command Prompt
- MinGW: 使用 64 位版本的 MinGW-w64

### 5. C++17 支持问题

**错误信息**:
```
error: 'std::optional' is not a member of 'std'
```

**解决方案**:
- 更新 Visual Studio 到 2017 或更高版本
- 或在 CMakeLists.txt 中明确指定 C++17 标准

## Visual Studio 集成

### 使用 Visual Studio 打开项目

```powershell
cd build
cmake .. -G "Visual Studio 17 2022" -A x64
```

然后在 Visual Studio 中打开 `modern-video-player.sln`

### Visual Studio Code 集成

1. 安装 C/C++ 扩展
2. 安装 CMake Tools 扩展
3. 打开项目文件夹
4. 使用 CMake Tools 插件配置和构建

## 调试配置

### 在 Visual Studio 中调试

1. 设置启动项目为 `modern-video-player`
2. 右键项目 → 属性 → 调试
3. 设置 "命令行参数" 为 `your_video.mp4`
4. 设置 "工作目录" 为 `$(ProjectDir)`

### 在 VS Code 中调试

创建 `.vscode/launch.json`:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/Debug/modern-video-player.exe",
            "args": ["your_video.mp4"],
            "cwd": "${workspaceFolder}",
            "environment": []
        }
    ]
}
```

## 性能优化

### 1. Release 编译

```powershell
cmake --build . --config Release
```

### 2. 启用优化选项

在 CMakeLists.txt 中添加：

```cmake
if(MSVC)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /GL")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} /LTCG")
else()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -march=native")
endif()
```

## 清理构建

```powershell
# 删除构建目录
cd ..
Remove-Item -Recurse -Force build

# 或在构建目录中
cd build
cmake --build . --target clean
```

## 下一步

安装完成后，您可以：

1. 阅读 [IMPLEMENTATION.md](IMPLEMENTATION.md) 了解实现细节
2. 阅读 [ARCHITECTURE.md](ARCHITECTURE.md) 了解架构设计
3. 运行示例视频文件测试功能

## 获取帮助

如果遇到问题，可以：

1. 查看 GitHub Issues
2. 查看 FFmpeg 官方文档: https://ffmpeg.org/documentation.html
3. 查看 SDL2 官方文档: https://wiki.libsdl.org/
4. 在项目中提出 Issue
