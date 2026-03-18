# D3D11 原生渲染链交付说明（2026-03-18）

日期：2026-03-18  
范围：`CMakeLists.txt`、`include/subtitle/*`、`src/subtitle/*`、`include/render/video_renderer.h`、`include/render/d3d11_video_renderer.h`、`src/render/d3d11_video_renderer.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、`src/video_player.cpp`、`src/main.cpp`

## implementation planner

1. 先清掉与业务逻辑无关的全局构建阻塞，恢复 MSVC 全量构建基线。
2. 扩展字幕数据模型与解析器，把 ASS/SSA 的样式、层级、定位和 run 级文本样式引入主链。
3. 让 `PlayerCore` 和 `IVideoRenderer` 传递结构化字幕对象，而不是只传单条纯文本字符串。
4. 在同一块 DXGI swap chain backbuffer 上完成视频、OSD、ASS/SSA 样式字幕绘制，同时保留明确回退边界。
5. 用完整解决方案构建而不是单文件语法检查来验证交付结果。

## 当前主链

```text
Demuxer
  -> FFmpeg video decoder (D3D11VA on renderer-owned device)
  -> AV_PIX_FMT_D3D11
  -> PlayerCore native passthrough
  -> D3D11VideoRenderer (NV12 / P010 / P016 shader sampling)
  -> D2D1 / DirectWrite subtitle compositor on the same DXGI backbuffer
  -> DXGI swap chain Present
```

## 本次补齐内容

### 1. 全局构建阻塞清理

- 清理了多处头文件/源文件在当前 MSVC + 代码页组合下的误读问题，避免“代码已完成但整工程无法验证”的假阻塞。
- 这批文件主要是注释和文本层面的 ASCII-safe 重写，不改变模块边界和功能设计。
- 已处理的全局阻塞文件包括：
  - 头文件：`include/config/settings_manager.h`、`include/media/format_support.h`、`include/playlist/playlist_manager.h`、`include/plugin/plugin_api.h`、`include/plugin/plugin_manager.h`、`include/filters/filter_registry.h`、`include/filters/video_filter_chain.h`、`include/filters/audio_filter_chain.h`、`include/streaming/http_stream_downloader.h`、`include/streaming/hls_manifest_parser.h`、`include/streaming/dash_manifest_parser.h`、`include/streaming/adaptive_bitrate_selector.h`、`include/display.h`、`include/render/sdl_video_renderer.h`、`include/render/opengl_video_renderer.h`、`include/core/frame_queue.h`
  - 源文件：`src/streaming/adaptive_bitrate_selector.cpp`、`src/render/renderer_factory.cpp`、`src/ui/skin_engine.cpp`、`src/core/scheduler.cpp`

### 2. 字幕模型与解析链扩展

- `subtitle::SubtitleItem` 现在承载完整字幕对象，而不只是时间区间和纯文本：
  - `layer`
  - `play_res_x / play_res_y`
  - `style`
  - `runs`
  - `raw_text`
- `subtitle::SubtitleStyle` 覆盖当前原生 D3D11 路径需要的样式字段：字体、字号、粗斜体、下划线、删除线、主色、描边色、背景色、对齐、边距、边框样式、描边、阴影、绝对定位。
- `subtitle::SubtitleTextRun` 用于表达单条字幕内的 run 级字体样式变化，供 D3D11 renderer 直接消费。
- `subtitle::createParserForPath()` 现在统一分发 `.ass`、`.ssa`、`.srt`。

### 3. ASS/SSA 解析能力

- 新增 `AssParser`，解析：
  - `[Script Info]` 中的 `PlayResX / PlayResY`
  - `[V4+ Styles]` / `[V4 Styles]`
  - `[Events]`
- 当前支持的常用 ASS/SSA override 标签包括：
  - `\b`、`\i`、`\u`、`\s`
  - `\fs`、`\fn`
  - `\an`、`\a`
  - `\pos`
  - `\c` / `\1c`
  - `\alpha` / `\1a`
  - `\bord`
  - `\shad`
  - `\r`
- `subtitle_timeline.cpp` 现在支持一次返回多条当前激活字幕索引，覆盖 ASS/SSA 常见的同屏多 cue 场景。

### 4. PlayerCore 与 renderer 接口打通

- `PlayerCore::updateSubtitleOverlay()` 不再只推送一条当前字幕，而是收集所有当前激活 cue，并通过 `video_renderer_->setSubtitleItems(active_items)` 送入渲染器。
- `IVideoRenderer` 新增默认实现 `setSubtitleItems(...)`：
  - 非 D3D11 渲染器仍可回退为纯文本聚合显示。
  - D3D11 渲染器消费结构化字幕对象，保持原生样式信息不丢失。
- `VideoPlayer::loadExternalSubtitle()` 与 `main.cpp` 已支持 `.ass`、`.ssa`、`.srt`，自动外挂字幕探测顺序更新为 `.ass -> .ssa -> .srt`。

### 5. 原生 D3D11 ASS/SSA 字幕绘制

- `D3D11VideoRenderer` 现在在同一块 swap chain backbuffer 上完成字幕合成，不再退回 `Display::setSubtitleText()` 的 SDL 叠加路径。
- 当前原生 D3D11 字幕绘制能力包括：
  - 多条活动字幕稳定排序与同屏显示（按 `layer`、`index`）
  - 文本填充色
  - 近似描边
  - 阴影
  - 盒式背景（`border_style == 3`）
  - 对齐
  - `\pos` 绝对定位
  - 字体族、字号、粗体、斜体、下划线、删除线
  - 暂停态字幕变化即时重绘
- 视频面、控制条、字幕叠加和 `Present` 都在原生 D3D11 / D2D 路径内完成。

## 当前能力结论

- 当前仓库已经具备一条完整、独立、原生的 Windows D3D11 GPU 渲染链：
  - 解码设备与渲染设备同源
  - `AV_PIX_FMT_D3D11` 硬件帧可直接进入 renderer
  - 视频采样、OSD、ASS/SSA 样式字幕、swap chain `Present` 在同一原生图形链内完成
- 外挂字幕链已经从“仅 SRT / 仅纯文本”推进到“.ass/.ssa/.srt + 原生 D3D11 样式字幕绘制”。
- 全局构建阻塞已清理，当前不是“局部代码可编译、整工程不可交付”的状态，而是完整解决方案可构建通过。

## 回退边界

以下场景仍会显式回退，这属于当前设计的一部分，而不是缺陷：

- 启用了视频滤镜：`PlayerCore` 会把硬件帧转换为软件帧，再交给现有滤镜链处理。
- 硬件面格式不在 `NV12 / P010 / P016` 范围内：D3D11 renderer 拒绝直接采样，交由上层回退。
- D3D11 renderer 初始化失败：`RendererFactory` 仍可降级到 `SoftwareSDL`。
- 非 D3D11 渲染器：通过 `IVideoRenderer::setSubtitleItems()` 默认实现回退为纯文本显示。

## 当前限制

- 这不是完整的 libass 兼容实现。当前没有覆盖完整 ASS 脚本引擎，例如：
  - `\move`、`\org`、`\fad` / `\fade`
  - `\t` 动态变换
  - `\clip` / `\iclip`
  - `\blur` / `\be`
  - `\p` 矢量绘图
  - karaoke / drawing / animation 等高级效果
- run 级字体样式已生效，但 run 级颜色当前没有逐 run 独立绘制，主色仍以 cue 级样式为主。
- `SubtitleTextRun` 的位置映射按 UTF-8 code point 计数，面向常见 Latin/CJK/BMP 文本足够稳定，但不等价于完整的复杂文本 shaping 引擎。
- 目前只完成了编译/构建级验证，尚未补充真实 `.ass` 样本的运行时视觉验收报告。

## 验证情况

- 定向语法编译已覆盖本次关键文件：
  - `src/subtitle/ass_parser.cpp`
  - `src/core/player_core.cpp`
  - `src/video_player.cpp`
  - `src/render/d3d11_video_renderer.cpp`
  - `src/main.cpp`
  - `src/core/scheduler.cpp`
  - `src/display.cpp`
  - `src/render/renderer_factory.cpp`
  - `src/ui/skin_engine.cpp`
  - `src/streaming/adaptive_bitrate_selector.cpp`
- 完整解决方案验证命令：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m
```

- 当前结果：
  - `modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe`
  - `已成功生成。`
  - `0 个警告`
  - `0 个错误`

## 与历史文档的关系

- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md` 记录的是 2026-03-14 的旧实现结论，历史说明仍然保留。
- `问题 57` 记录的是“原生 D3D11 主链与原生字幕叠加的第一阶段补齐”。
- 本文当前状态同时吸收了 `问题 66` 的后续收口结果：全局构建阻塞清理、ASS/SSA 解析链打通、原生 D3D11 样式字幕链补齐。
