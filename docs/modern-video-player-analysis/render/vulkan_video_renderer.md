# VulkanVideoRenderer 渲染后端

源码: `include/render/vulkan_video_renderer.h`, `src/render/vulkan_video_renderer.cpp`

## 角色

Vulkan 渲染后端。它实现 `IVideoRenderer`、`IPlaybackInputSource` 和 `IRenderOverlaySink`，用于 Vulkan 编译开关启用时的播放、诊断和平台 gate。

## 接口

| 接口 | 用途 |
|---|---|
| `init` / `close` | 创建和释放 Vulkan 相关状态 |
| `renderFrame` / `present` / `clear` | 提交并呈现视频帧 |
| `handleEvents` / `consume*Request` | 窗口输入请求 |
| `setOverlayState` / `setSubtitleText` | 控制栏和字幕文本 |
| `supportsDirectFrameFormat` | 判断直接帧格式 |
| `rendererBackendName()` | 返回 Vulkan 后端名 |

## 数据流

```mermaid
flowchart LR
    CORE[PlayerCore] --> VK[VulkanVideoRenderer]
    VK --> STATE[VulkanState]
    VK -.input requests.-> CORE
    CORE -.overlay.-> VK
```

## 关键约束

- CMake `ENABLE_VULKAN_RENDERER` 控制是否构建该后端；Windows 会尝试 `find_package(Vulkan)` 和 `VULKAN_SDK` fallback。
- Apple 平台会强制关闭 Vulkan 后端。
- 根 CMake 要求至少一个渲染后端可用，Vulkan 只是候选之一。

## 注意点

- 仓库中有大量 Windows Vulkan gate 和 canary 文档/脚本，修改 Vulkan 行为时要同步这些回归入口。
- 当前头文件只暴露 `VulkanState` PIMPL，不应在上层依赖内部状态。
