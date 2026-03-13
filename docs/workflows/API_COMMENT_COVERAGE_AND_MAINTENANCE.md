# 已注释模块清单与维护准则

本文档记录当前这轮中文 `///` API 注释覆盖范围，并给出后续增量维护规则，避免新代码继续回到“接口无语义、线程边界不清、公共 API 注释缺失”的状态。

## 当前覆盖范围

### 核心播放链路

- `include/video_player.h`
- `include/core/player_core.h`
- `include/core/scheduler.h`
- `include/core/clock.h`
- `include/core/frame.h`
- `include/core/frame_queue.h`
- `include/demuxer.h`
- `include/display.h`
- `include/audio_player.h`
- `src/video_player.cpp`
- `src/core/player_core.cpp`
- `src/core/scheduler.cpp`
- `src/core/clock.cpp`
- `src/demuxer.cpp`
- `src/display.cpp`
- `src/audio_player.cpp`

### 渲染与输入

- `include/render/video_renderer.h`
- `include/render/renderer_factory.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/input/hotkey_manager.h`
- `src/render/renderer_factory.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/input/hotkey_manager.cpp`

### 队列、线程与解码能力

- `include/thread_safe_queue.h`
- `include/core/command.h`
- `include/core/decoder_thread.h`
- `include/core/frame_pool.h`
- `include/core/task_queue.h`
- `include/decoder/decoder_capability.h`
- `include/decoder/decoder_factory.h`
- `src/core/decoder_thread.cpp`
- `src/core/frame_pool.cpp`
- `src/decoder/decoder_factory.cpp`

### 滤镜体系

- `include/filters/filter_base.h`
- `include/filters/audio_filter.h`
- `include/filters/video_filter.h`
- `include/filters/audio_filter_chain.h`
- `include/filters/video_filter_chain.h`
- `include/filters/filter_registry.h`
- `include/filters/filter_pipeline.h`
- `include/filters/builtin_filters.h`
- `src/filters/audio_filter_chain.cpp`
- `src/filters/video_filter_chain.cpp`
- `src/filters/filter_registry.cpp`
- `src/filters/filter_pipeline.cpp`
- `src/filters/builtin_filters.cpp`
- `src/filters/brightness_filter.cpp`
- `src/filters/contrast_filter.cpp`
- `src/filters/saturation_filter.cpp`
- `src/filters/volume_balance_filter.cpp`

### 字幕、流媒体与播放列表

- `include/subtitle/subtitle_parser.h`
- `include/subtitle/srt_parser.h`
- `include/subtitle/subtitle_timeline.h`
- `include/streaming/adaptive_bitrate_selector.h`
- `include/streaming/dash_manifest_parser.h`
- `include/streaming/hls_manifest_parser.h`
- `include/streaming/http_stream_downloader.h`
- `include/playlist/playlist_manager.h`
- `src/subtitle/srt_parser.cpp`
- `src/subtitle/subtitle_timeline.cpp`
- `src/streaming/adaptive_bitrate_selector.cpp`
- `src/streaming/dash_manifest_parser.cpp`
- `src/streaming/hls_manifest_parser.cpp`
- `src/streaming/http_stream_downloader.cpp`
- `src/playlist/playlist_manager.cpp`

### 配置、能力、插件与 UI

- `include/config/settings_manager.h`
- `include/media/format_support.h`
- `include/plugin/plugin_api.h`
- `include/plugin/plugin_manager.h`
- `include/ui/skin_engine.h`
- `include/logger.h`
- `src/config/settings_manager.cpp`
- `src/media/format_support.cpp`
- `src/plugin/plugin_manager.cpp`
- `src/plugin/sample_logger_plugin.cpp`
- `src/ui/skin_engine.cpp`
- `src/logger.cpp`
- `src/main.cpp`（仅关键 CLI / 启动 / 检查入口）

## 已采用的注释层级

### 第一层：接口职责注释

适用对象：

- 类 / 结构体 / 枚举
- 公共方法
- 线程入口
- 回调入口

要求：

- 使用 `///`
- 先写“做什么”
- 不复述代码实现

### 第二层：参数化注释

适用对象：

- 参数语义不直观的函数
- 跨线程函数
- 带副作用的生命周期函数
- 返回值有明确成功/失败语义的函数

要求：

- 使用 `@param`、`@return`、`@note`
- 只写高价值信息
- 避免把每个参数都机械注释一遍

当前已强化参数化注释的重点模块：

- `VideoPlayer`
- `PlayerCore`
- `Scheduler`
- `Display`
- `AudioPlayer`
- `Demuxer`
- `HttpStreamDownloader`

## 后续维护准则

### 1. 新增公共头文件时

必须同步完成：

- 类注释
- 生命周期函数注释
- 线程/回调接口注释
- 参数/返回语义注释

最低验收标准：新增头文件中的公共 API 不允许裸奔。

### 2. 修改已有公共接口签名时

以下任一变化都必须同步改注释：

- 参数新增 / 删除 / 改名
- 返回值语义改变
- 线程模型改变
- 所有权转移方式改变
- flush / reset / 延迟执行 / 异步执行行为改变

### 3. 修改实现但不改签名时

如果发生以下变化，也要更新注释：

- 函数原本同步执行，改成异步排队
- 原本立即生效，改成“设置请求、稍后消费”
- 原本零副作用，新增清队列/重置状态/写文件
- 原本运行在调用线程，改到回调线程/渲染线程/工作线程

### 4. 注释优先级规则

维护时按下面优先级补：

1. `include/*` 公共接口
2. `src/*` 生命周期入口
3. 线程入口 / 回调入口
4. 队列 / flush / 时钟 / seek / screenshot / subtitle 等高误解区域
5. 复杂实现块的少量 `//` 注释

## 代码评审清单

提交前至少检查以下问题：

- 新增公共函数是否有 `///`
- 线程边界是否写清楚
- 时间单位是否明确到“秒 / 毫秒 / 比例”
- 返回 `bool` 的函数是否写清成功语义
- 指针/引用参数是否写清输入/输出语义
- 是否存在“注释仍描述旧行为”的情况
- 是否出现大量复述代码的无效注释

## 推荐维护动作

### 小改动

- 直接在同次改动中同步更新注释
- 不要把“以后再补注释”留成口头承诺

### 大改动

建议按以下顺序：

1. 先改头文件注释
2. 再改实现入口注释
3. 最后检查文档中是否要补充规范或清单

### 架构调整

如果线程模型、渲染链路、解码链路有明显变化：

- 更新 `docs/workflows/CHINESE_API_COMMENT_GUIDELINES.md`
- 更新本清单中对应模块的覆盖说明
- 需要时新增专项说明文档

## 不纳入强制范围的文件

以下文件不要求“每个函数都带 `///`”：

- 纯局部工具函数密集的 `.cpp`
- `main.cpp` 中大量一次性 CLI 小工具函数
- 明显只在单文件内部使用、且语义非常直白的辅助函数

但即便如此，以下入口仍建议保留注释：

- `main()`
- 重要 CLI 分发函数
- 重要验证/回归入口
- 跨模块行为编排函数

## 配套文档

- 规范入口：`docs/workflows/CHINESE_API_COMMENT_GUIDELINES.md`
- 工作流入口：`docs/workflows/README.md`
- 开发流程规范：`docs/workflows/WORKFLOW.md`