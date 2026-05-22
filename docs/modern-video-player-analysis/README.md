# Modern Video Player 文档索引

按目录分组的轻量文档。每篇文档基于 `include/` + `src/` + `CMakeLists.txt` 中可验证的结构，只记录源码中已经存在的事实。

## 架构总览（含图）

| 文档 | 说明 |
|---|---|
| [architecture/README.md](architecture/README.md) | 架构文档索引 |
| [architecture/overview.md](architecture/overview.md) | 分层模型 + 模块依赖 + 单例 / 多实例 |
| [architecture/dataflow.md](architecture/dataflow.md) | 播放 / 字幕 / 设置 / 流媒体 / 诊断数据流 |
| [architecture/ui-layer.md](architecture/ui-layer.md) | SDL 窗口 UI + 输入事件 + overlay 分层 |
| [architecture/runtime-flow.md](architecture/runtime-flow.md) | 启动 / 打开媒体 / seek / EOF / 关闭时序 |

## 启动入口 / 门面 / 核心

| 文档 | 源文件 | 说明 |
|---|---|---|
| [core/main.md](core/main.md) | `src/main.cpp` | CLI 入口、播放主循环、诊断/回归检查命令 |
| [core/video_player.md](core/video_player.md) | `include/video_player.h`, `src/video_player.cpp` | 面向应用层的播放门面 |
| [core/player_core.md](core/player_core.md) | `include/core/player_core.h`, `src/core/player_core.cpp` | 播放核心编排器 |
| [core/demuxer.md](core/demuxer.md) | `include/demuxer.h`, `src/demuxer.cpp` | FFmpeg 输入打开、探测、读包、seek |
| [core/audio_player.md](core/audio_player.md) | `include/audio_player.h`, `src/audio_player.cpp` | SDL 音频设备、音频队列、音量/静音 |
| [core/scheduler.md](core/scheduler.md) | `include/core/scheduler.h`, `src/core/scheduler.cpp` | 视频解码、音频解码、渲染线程调度 |
| [core/frame_queue.md](core/frame_queue.md) | `include/core/frame_queue.h`, `include/thread_safe_queue.h` | 帧队列 / 包队列 / generation flush |
| [core/logger.md](core/logger.md) | `include/logger.h`, `src/logger.cpp` | Quill / std::cout 日志封装 |

## Render（渲染与窗口）

| 文档 | 源文件 | 说明 |
|---|---|---|
| [render/video_renderer.md](render/video_renderer.md) | `include/render/video_renderer.h` | 渲染后端统一接口 |
| [render/renderer_factory.md](render/renderer_factory.md) | `include/render/renderer_factory.h`, `src/render/renderer_factory.cpp` | 后端创建与可用性判断 |
| [render/d3d11_video_renderer.md](render/d3d11_video_renderer.md) | `include/render/d3d11_video_renderer.h`, `src/render/d3d11_video_renderer.cpp` | Windows D3D11 渲染链路 |
| [render/opengl_video_renderer.md](render/opengl_video_renderer.md) | `include/render/opengl_video_renderer.h`, `src/render/opengl_video_renderer.cpp` | OpenGL 渲染链路 |
| [render/sdl_video_renderer.md](render/sdl_video_renderer.md) | `include/render/sdl_video_renderer.h`, `src/render/sdl_video_renderer.cpp` | SDL 软件渲染后端 |
| [render/vulkan_video_renderer.md](render/vulkan_video_renderer.md) | `include/render/vulkan_video_renderer.h`, `src/render/vulkan_video_renderer.cpp` | Vulkan 渲染后端 |
| [render/output_color_profile.md](render/output_color_profile.md) | `include/render/output_color_profile.h`, `src/render/output_color_profile.cpp` | ICC / 3D LUT 读取与输出色彩配置 |
| [render/display_legacy_sdl.md](render/display_legacy_sdl.md) | `include/display.h`, `src/display.cpp` | 旧 SDL Display 输入/渲染实现 |

## Subtitle（字幕）

| 文档 | 源文件 | 说明 |
|---|---|---|
| [subtitle/subtitle_parser.md](subtitle/subtitle_parser.md) | `include/subtitle/subtitle_parser.h`, `src/subtitle/subtitle_parser.cpp` | 字幕统一模型与 parser facade |
| [subtitle/srt_parser.md](subtitle/srt_parser.md) | `include/subtitle/srt_parser.h`, `src/subtitle/srt_parser.cpp` | SRT 解析 |
| [subtitle/ass_parser.md](subtitle/ass_parser.md) | `include/subtitle/ass_parser.h`, `src/subtitle/ass_parser.cpp` | ASS/SSA 样式、动画、位图字幕解析 |
| [subtitle/embedded_subtitle_loader.md](subtitle/embedded_subtitle_loader.md) | `include/subtitle/embedded_subtitle_loader.h`, `src/subtitle/embedded_subtitle_loader.cpp` | 内嵌字幕轨枚举、选择、加载 |
| [subtitle/subtitle_timeline.md](subtitle/subtitle_timeline.md) | `include/subtitle/subtitle_timeline.h`, `src/subtitle/subtitle_timeline.cpp` | 按播放时间查找活跃字幕 |
| [subtitle/subtitle_font_registry.md](subtitle/subtitle_font_registry.md) | `include/subtitle/subtitle_font_registry.h`, `src/subtitle/subtitle_font_registry.cpp` | 字体附件提取与注册 |
| [subtitle/libass_probe.md](subtitle/libass_probe.md) | `include/subtitle/libass_probe.h`, `src/subtitle/libass_probe.cpp` | libass shaping 能力探测 |

## Streaming（流媒体）

| 文档 | 源文件 | 说明 |
|---|---|---|
| [streaming/http_stream_downloader.md](streaming/http_stream_downloader.md) | `include/streaming/http_stream_downloader.h`, `src/streaming/http_stream_downloader.cpp` | HTTP 分块下载器 |
| [streaming/hls_manifest_parser.md](streaming/hls_manifest_parser.md) | `include/streaming/hls_manifest_parser.h`, `src/streaming/hls_manifest_parser.cpp` | HLS master playlist 解析 |
| [streaming/dash_manifest_parser.md](streaming/dash_manifest_parser.md) | `include/streaming/dash_manifest_parser.h`, `src/streaming/dash_manifest_parser.cpp` | DASH MPD 解析 |
| [streaming/adaptive_bitrate_selector.md](streaming/adaptive_bitrate_selector.md) | `include/streaming/adaptive_bitrate_selector.h`, `src/streaming/adaptive_bitrate_selector.cpp` | ABR 码率选择 |

## Filters（滤镜）

| 文档 | 源文件 | 说明 |
|---|---|---|
| [filters/filter_pipeline.md](filters/filter_pipeline.md) | `include/filters/filter_pipeline.h`, `src/filters/filter_pipeline.cpp` | 视频 / 音频滤镜流水线 |
| [filters/filter_registry.md](filters/filter_registry.md) | `include/filters/filter_registry.h`, `src/filters/filter_registry.cpp` | 滤镜工厂注册表 |
| [filters/builtin_filters.md](filters/builtin_filters.md) | `include/filters/builtin_filters.h`, `src/filters/*.cpp` | 亮度、对比度、饱和度、音量平衡 |

## Support（配置、输入、平台、插件）

| 文档 | 源文件 | 说明 |
|---|---|---|
| [support/config_settings_manager.md](support/config_settings_manager.md) | `include/config/settings_manager.h`, `src/config/settings_manager.cpp` | INI 风格设置读写 |
| [support/hotkey_manager.md](support/hotkey_manager.md) | `include/input/hotkey_manager.h`, `src/input/hotkey_manager.cpp` | 热键动作映射 |
| [support/playlist_manager.md](support/playlist_manager.md) | `include/playlist/playlist_manager.h`, `src/playlist/playlist_manager.cpp` | 播放列表与当前索引 |
| [support/format_support.md](support/format_support.md) | `include/media/format_support.h`, `src/media/format_support.cpp` | 容器/编码/播放目标能力评估 |
| [support/platform_capabilities.md](support/platform_capabilities.md) | `include/platform/*.h`, `src/platform/*.cpp` | 平台能力与硬件设备探测 |
| [support/decoder_factory.md](support/decoder_factory.md) | `include/decoder/*.h`, `src/decoder/decoder_factory.cpp` | 解码后端能力和优先级选择 |
| [support/plugin_manager.md](support/plugin_manager.md) | `include/plugin/*.h`, `src/plugin/*.cpp` | 动态插件加载与滤镜注册 |
| [support/skin_engine.md](support/skin_engine.md) | `include/ui/skin_engine.h`, `src/ui/skin_engine.cpp` | UI 皮肤占位模块 |
| [support/audio_tools.md](support/audio_tools.md) | `include/audio/*.h`, `src/audio/*.cpp` | 音频均衡器与混音器 |

## 同名 / 多实现 注意

| 名称 | 出现位置 | 选用建议 |
|---|---|---|
| `Display` | `display.{h,cpp}` | 旧 SDL 窗口 + 输入层；新链路优先走 `IVideoRenderer` 后端 |
| `SdlVideoRenderer` | `render/sdl_video_renderer.*` | 当前软件渲染后端，接口与 D3D11/OpenGL/Vulkan 对齐 |
| `RendererDiagnostics` | `render/video_renderer.h` | 各渲染器统一诊断输出，D3D11/OpenGL 另有扩展快照 |
| `PlayerAction` | `input/hotkey_manager.h` + 各 renderer 输入实现 | 改热键时需同步配置读写和 renderer consume 路由 |

## 关联资料

- 工程入口: `CMakeLists.txt`
- 主程序入口: `src/main.cpp`
- 公共头文件: `include/`
- 实现文件: `src/`
- 默认配置: `config/player_settings.ini`, `config/logging.conf`
- 样例与回归数据: `samples/`, `tools/format_regression/`

## 现场日志关键字速查

| 模块 | 文件 | 关键字 |
|---|---|---|
| 启动策略 | `src/core/player_core.cpp` | `Startup strategy`, `renderer_candidates`, `decoder_candidates` |
| 解复用 | `src/core/player_core.cpp`, `src/demuxer.cpp` | `Demux thread`, `demux reached EOF`, `readPacket` |
| 解码 | `src/core/player_core.cpp` | `Video decode`, `Audio decode`, `stale`, `send_packet` |
| 调度 | `src/core/scheduler.cpp` | `Scheduler`, `late_drop`, `backpressure`, `restart` |
| 渲染 | `src/render/*.cpp` | `renderer`, `present`, `diagnostics`, `fallback` |
| 音频 | `src/audio_player.cpp` | `Audio init`, `WASAPI`, `SDL audio` |
| 字幕 | `src/subtitle/*.cpp` | `Embedded subtitle`, `Subtitle attachment font`, `libass` |
| 设置 | `src/main.cpp`, `src/config/settings_manager.cpp` | `player.volume`, `hotkey.*`, `settings-persistence-check` |

