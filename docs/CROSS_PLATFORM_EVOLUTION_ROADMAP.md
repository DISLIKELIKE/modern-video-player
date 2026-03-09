# 从 Windows 演进到跨平台架构的改造路线图

更新日期：2026-03-09

## 1. 文档目标

本文档面向当前仓库 `modern-video-player`，目标不是讨论“跨平台要不要做”，而是给出一份 **贴着现有代码结构** 的、可执行的跨平台演进路线图。

本文重点回答 4 个问题：

1. 当前项目里哪些部分已经具备跨平台基础
2. 哪些地方仍然带有明显的 Windows 假设
3. 应该按什么顺序改，才不会把项目搞乱
4. 每个阶段改完后，如何判断方向是对的

---

## 2. 当前项目的现状判断

从当前代码结构看，这个项目 **不是纯 Windows 死耦合播放器**，而是已经具备一部分跨平台雏形。

### 2.1 已有的跨平台基础

- 构建系统使用 `CMake`：`CMakeLists.txt:3`
- 音频输出基于 `SDL`：`src/audio_player.cpp:20`
- 显示/窗口层大量基于 `SDL`：`src/display.cpp:452`
- 已有 renderer 工厂抽象：`src/render/renderer_factory.cpp:10`
- 已有多个 renderer 后端类型：`D3D11 / SoftwareSDL / OpenGL`
- 已有 decoder 工厂与后端排序：`src/decoder/decoder_factory.cpp:42`
- 播放控制入口已经是 facade + core 结构：`src/video_player.cpp:14`、`src/core/player_core.cpp:109`

### 2.2 仍然存在的 Windows 假设

- `CMakeLists.txt:10` 目前以 `WIN32` 为主要构建分支
- renderer 自动选择在 Windows 上直接优先 `D3D11`：`src/render/renderer_factory.cpp:11`
- `PlayerCore` 打开时直接依赖 renderer 工厂的“最佳类型”判断：`src/core/player_core.cpp:138`
- `D3D11VideoRenderer` 已经是较强主链，而 `OpenGLVideoRenderer` 目前仍是空壳：`src/render/opengl_video_renderer.cpp:5`
- `display.cpp` 目前承担了较多 SDL 窗口、渲染、事件循环、UI 控件逻辑：`src/display.cpp:452`
- decoder 策略目前更偏“Windows 硬解语义”，尚未显式建模 Linux/macOS 路径：`src/decoder/decoder_factory.cpp:42`

### 2.3 结论

当前项目最正确的跨平台演进方式不是“推倒重写”，而是：

- **保留现有 core/media 主链**
- **继续抽清平台边界**
- **先让第二平台跑通软件链**
- **最后再把硬解与高级 renderer 能力逐平台补上**

---

## 3. 跨平台演进的总原则

整个改造建议遵守 6 条原则：

1. **先做分层，再做移植**
2. **先跑通软件链，再补硬解**
3. **先支持第二平台，再谈真正跨平台**
4. **核心层不能知道平台 API 名字**
5. **策略层统一，后端层分平台**
6. **允许 Windows 先强，Linux/macOS 先能用**

最重要的一条是：

> 不要把“跨平台”理解成“每个平台一开始都要有同等高级能力”。

第一阶段真正需要的是：

- 架构可迁移
- 主链能复用
- 第二平台能稳定播放

---

## 4. 推荐的目标架构

建议把项目逐步收敛成下面 5 层：

### 4.1 `core/`：平台无关的播放核心

职责：

- 播放状态机
- seek / pause / step / screenshot 控制逻辑
- clock / scheduler / A-V sync
- frame / queue / playback decision state

当前对应：

- `src/core/player_core.cpp`
- `src/core/scheduler.cpp`
- `src/core/clock.cpp`
- `src/core/frame.cpp`
- `src/core/frame_pool.cpp`

目标要求：

- 不出现 `HWND`、`D3D11`、`DXVA2` 等平台/图形 API 细节

### 4.2 `media/`：尽量平台无关的媒体主链

职责：

- demux
- decode 抽象
- format support
- subtitle parse / timeline

当前对应：

- `src/demuxer.cpp`
- `src/decoder/decoder_factory.cpp`
- `src/media/format_support.cpp`
- `src/subtitle/*`

目标要求：

- decode 策略可表达多平台后端，而不是只隐含 Windows 硬解

### 4.3 `render/`：渲染抽象与后端实现

职责：

- 定义统一 renderer 接口
- 组织多渲染后端
- 统一 overlay / subtitle / screenshot / present 接口语义

当前对应：

- `src/render/renderer_factory.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`

目标要求：

- renderer 接口统一
- renderer 工厂不把 Windows 规则写死在 core 里

### 4.4 `platform/`：平台能力与系统适配层

职责：

- GPU / OS / display capability probe
- 平台相关路径、环境、系统调用
- 每个平台可选后端集合

当前项目里这一层还不完整，建议新增。

### 4.5 `app/ui/`：应用壳层与事件入口

职责：

- CLI / app settings
- 热键与用户交互
- 显示调试信息
- 统一把用户操作转成 core 命令

当前对应：

- `src/main.cpp`
- `src/video_player.cpp`
- `src/display.cpp`
- `src/input/*`

---

## 5. 平台路线建议

## 5.1 优先顺序

建议平台推进顺序：

1. `Windows`：继续作为主平台
2. `Linux`：作为第一个第二平台
3. `macOS`：作为第三阶段平台

原因：

- 你当前技术栈与 Linux 的兼容性最好
- Linux 是验证架构抽象是否正确的最佳第二平台
- macOS 的图形、硬解、打包链路投入更大，适合后置

## 5.2 各平台第一阶段目标

### Windows

- 保持现有主力链：`D3D11 + SDL + FFmpeg`
- 继续完善默认策略与 fallback

### Linux

- 第一阶段只要求：`Software decode + SDL/OpenGL renderer + SDL audio`
- 第二阶段再考虑 `VA-API`

### macOS

- 第一阶段只要求：`Software decode + SDL/OpenGL/Metal过渡`
- 第二阶段再考虑 `VideoToolbox`

---

## 6. 分阶段改造路线图

下面按 `Phase 0 ~ Phase 5` 给出建议路径。

## Phase 0：先把跨平台“接缝”标出来

### 目标

在不改功能的前提下，明确哪些代码属于：

- 平台无关逻辑
- 平台能力探测
- 平台后端实现

### 要做的事

1. 盘点并标注所有平台相关文件
2. 盘点所有 `#if defined(_WIN32)` / `WIN32` 分支
3. 标记哪些模块仍然隐式依赖 Windows 语义

### 重点文件

- `CMakeLists.txt:10`
- `src/render/renderer_factory.cpp:10`
- `src/decoder/decoder_factory.cpp:42`
- `src/core/player_core.cpp:138`
- `src/display.cpp:452`
- `src/plugin/plugin_manager.cpp:10`

### 产出

- 一张“平台相关代码分布图”
- 一份“必须解耦的 Windows 假设列表”

### 完成标准

- 你能明确说出：哪些文件可直接复用到 Linux，哪些不能

---

## Phase 1：先统一抽象，不急着移植第二平台

### 目标

让当前项目在架构上变成“可被第二平台承接”的状态。

### 核心思路

不是马上让 Linux 编译通过，而是先让：

- `core` 只依赖抽象
- `renderer` 与 `decoder` 的策略从 `PlayerCore` 中剥离

### 建议新增模块

建议新增：

- `include/core/playback_strategy.h`
- `src/core/playback_strategy.cpp`
- `include/platform/platform_capabilities.h`
- `src/platform/platform_capabilities.cpp`

### 这一步要做的事

1. 把 renderer / decoder 决策从 `PlayerCore::open()` 中抽走
2. 让 `PlayerCore` 只拿到一个“决策结果对象”
3. 把平台能力探测从工厂与 core 里独立出来

### 当前代码的建议改造点

#### `src/core/player_core.cpp:138`

当前行为：

- 直接调用 `RendererFactory::detectBestRendererType()`

建议改成：

- 依赖 `PlaybackStrategyDecision`
- `PlayerCore` 不再自己推断平台最佳 renderer

#### `src/decoder/decoder_factory.cpp:42`

当前行为：

- 基于 codec 和 prefer_hardware 排序

建议改成：

- 输入参数增加平台能力、渲染器能力、播放模式
- 输出“候选链列表”而不是只输出单一最佳后端

#### `src/render/renderer_factory.cpp:10`

当前行为：

- Windows 默认直接优先 `D3D11`

建议改成：

- 接收平台能力与播放策略
- 允许第二平台返回 `SDL` 或 `OpenGL`

### 产出

- 一套平台无关的策略对象
- 一套更清晰的 renderer / decoder 候选链模型

### 完成标准

- `PlayerCore` 中不再直接写死平台优先级
- “自动模式” 的策略逻辑集中在单独模块中

---

## Phase 2：把 renderer 分层做实

### 目标

让 renderer 层成为真正可扩展的跨平台后端层，而不是当前“D3D11 主力 + SDL 兼容 + OpenGL 空壳”。

### 推荐目录演进

建议逐步从：

- `src/render/d3d11_video_renderer.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`

演进到：

- `src/render/common/*`
- `src/render/d3d11/*`
- `src/render/sdl/*`
- `src/render/opengl/*`

### 这一步要做的事

1. 把 renderer 公共接口单独收口
2. 把 overlay、subtitle、hotkey bridge、present 等通用语义写清楚
3. 让 `OpenGLVideoRenderer` 不再只是占位，而是具备最小可播放能力

### 重点文件

- `src/render/d3d11_video_renderer.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp:5`
- `src/render/renderer_factory.cpp`

### 推荐最小目标

`OpenGLVideoRenderer` 第一阶段至少要支持：

- init / close
- renderFrame
- present
- basic subtitle text overlay path（哪怕先走 CPU/SDL 叠字也行）
- basic input/event consumption

### 完成标准

- 在不依赖 D3D11 的情况下，项目至少有一个可用的视频 renderer 后端

---

## Phase 3：先落地 Linux 的“软件基线链”

### 目标

让 Linux 上先跑通最小可用链，而不是一上来就做硬解。

### 推荐 Linux 第一版目标链

```text
Demux -> Software decode -> SDL/OpenGL renderer -> SDL audio
```

### 这一步要做的事

1. 更新 `CMakeLists.txt`，把 Linux 构建路径变成一级公民
2. 确保 SDL2、FFmpeg 在 Linux 下的依赖路径稳定
3. 跑通窗口、音频、视频显示、seek、字幕

### 重点文件

- `CMakeLists.txt`
- `src/audio_player.cpp`
- `src/display.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/core/player_core.cpp`

### 建议实现顺序

1. 先让 `SoftwareSDL` 路径在 Linux 下能播
2. 再让 `OpenGL` 路径在 Linux 下能播
3. 最后再决定 Linux 默认选 `SDL` 还是 `OpenGL`

### 完成标准

- Linux 下可打开本地文件
- 有声音
- 有画面
- 可 seek
- 可暂停/恢复
- 可切换字幕显示

---

## Phase 4：统一平台能力探测与策略决策

### 目标

让 Windows / Linux 共享同一套“如何选 decoder / renderer”的策略层。

### 这一步要做的事

1. 明确平台能力模型
2. 明确候选链模型
3. 明确 fallback 模型

### 推荐能力模型包含

- OS 类型
- GPU 信息
- 可用 renderer 集合
- 可用 hwdec 集合
- codec/profile/bitdepth 支持情况
- subtitle/capture/hdr 需求

### 推荐候选链模型

```cpp
struct PlaybackCandidate {
    PlatformKind platform;
    RendererKind renderer;
    DecoderMode decoder;
    bool hardware_decode;
    bool copy_back;
};
```

### Windows 候选链示例

- `D3D11 + D3D11VA native`
- `D3D11 + D3D11VA copy-back`
- `D3D11 + DXVA2 copy-back`
- `SDL + software`

### Linux 候选链示例

- `OpenGL + software`
- `SDL + software`
- 以后再加：`OpenGL + VAAPI native/copy-back`

### 完成标准

- Windows 与 Linux 的播放策略不再是两套分散逻辑
- 只是候选链不同，决策流程相同

---

## Phase 5：逐平台补硬解与高级能力

### 目标

在主链稳定、软件路径稳定之后，再逐平台补硬解和更高级渲染能力。

### Windows 继续强化

- `D3D11VA native`
- `D3D11VA copy-back`
- `DXVA2`
- HDR 路径
- 更清晰的 renderer 可观测性

### Linux 新增

- `VA-API`
- 与 `OpenGL` 或未来 Vulkan 路径的联动

### macOS 后续新增

- `VideoToolbox`
- 对应 renderer 路径

### 原则

- 新平台先做 `software`
- 新平台硬解只在主链稳定后加入
- 每个平台先进一个硬解后端，不要一次上太多

---

## 7. 建议的目录与模块调整顺序

下面给出更贴近当前仓库的实际演进建议。

## 7.1 先收口接口，不急着搬目录

第一阶段不建议立刻大规模挪文件，建议先：

- 明确 renderer 公共接口
- 明确 decoder 公共接口
- 明确 playback strategy 接口

等接口稳定后，再做目录搬迁。

## 7.2 第二阶段再逐步分目录

建议最终趋近：

```text
src/
  core/
  media/
  platform/
    windows/
    linux/
    mac/
  render/
    common/
    d3d11/
    sdl/
    opengl/
  decoder/
    common/
    windows/
    linux/
    mac/
  audio/
    sdl/
  ui/
```

注意：

- 这是方向，不是要求你一次性搬成这样
- 一次性大搬家风险很高

---

## 8. `CMake` 的具体演进建议

## 8.1 当前问题

`CMakeLists.txt:10` 目前以 `WIN32` 作为核心分支，虽然 Linux/macOS 构建说明存在，但项目主设计重心仍偏 Windows。

## 8.2 建议演进方向

### 新增选项

建议逐步加入：

- `ENABLE_D3D11_RENDERER`
- `ENABLE_OPENGL_RENDERER`
- `ENABLE_SDL_RENDERER`
- `ENABLE_D3D11VA`
- `ENABLE_DXVA2`
- `ENABLE_VAAPI`
- `ENABLE_VIDEOTOOLBOX`

### 原则

- 平台支持与功能支持分开配置
- 即便在 Windows 上，也不要默认所有平台专有项都强耦合进主逻辑

### 完成标准

- `CMake` 可以明确知道当前构建产物包含哪些 backend
- 运行时策略只在这些 backend 集合里选，不做无意义尝试

---

## 9. `display.cpp` 的演进建议

## 9.1 当前问题

`src/display.cpp` 目前承担了：

- SDL 初始化
- 窗口创建
- renderer 创建
- 纹理创建
- 控件绘制
- 鼠标交互
- 全屏控制

这对单平台阶段没问题，但对跨平台演进来说，职责偏重。

## 9.2 建议拆分方向

建议逐步拆成：

- `display_window`：窗口与事件
- `display_overlay`：控件和 overlay 绘制
- `display_presenter`：present 与 frame 提交

### 为什么要拆

- 这样 SDL UI 与 renderer 后端可以解耦
- Linux/macOS 接入时不需要把整个显示层重写

### 不是现在就要做的事

不要一开始就大拆 `display.cpp`。
建议等 `Phase 1` 的策略层与 renderer 边界稳定后再拆。

---

## 10. decoder 工厂的跨平台演进建议

## 10.1 当前问题

`src/decoder/decoder_factory.cpp:42` 目前更像是：

- 根据 codec 名称
- 再根据是否 prefer hardware
- 给出一组偏 Windows 的后端顺序

## 10.2 建议目标

让 decoder 工厂支持下面这种输入：

```cpp
DecoderSelectionInput {
    PlatformKind platform;
    PlaybackMode mode;
    RendererKind renderer;
    CodecInfo codec;
    SubtitleRequirements subtitle;
    CaptureRequirements capture;
    PlatformCapabilities capabilities;
}
```

输出：

- 候选 decoder 链表
- 每条链的原因说明

### 这样做的好处

- Windows 和 Linux 共用同一套决策框架
- 只是可选后端集合不同
- 以后接 `VAAPI`、`VideoToolbox` 很自然

---

## 11. renderer 工厂的跨平台演进建议

## 11.1 当前问题

`src/render/renderer_factory.cpp:10` 现在在 Windows 上直接返回 `D3D11`，其他平台直接回 `SoftwareSDL`。

这对于“现在能跑”是够的，但对于跨平台架构不够精细。

## 11.2 建议目标

renderer 工厂不负责“拍脑袋猜最佳值”，而是负责：

- 根据平台能力与播放策略
- 创建指定 renderer
- 返回失败原因

最佳 renderer 的决策应由 `playback_strategy` 完成。

### 建议职责划分

- `PlaybackStrategy`：决定“想用谁”
- `RendererFactory`：负责“创建谁”
- `PlayerCore`：负责“使用谁”

---

## 12. 推荐的里程碑验收标准

## M1：架构准备完成

验收标准：

- `PlayerCore` 不再写死平台优先级
- decoder / renderer 候选链由统一策略模块生成
- `OpenGLVideoRenderer` 不再是纯空壳

## M2：Linux 软件播放打通

验收标准：

- Linux 构建通过
- 本地文件可播放
- 音频、seek、字幕、暂停可用

## M3：统一策略层稳定

验收标准：

- Windows / Linux 共用同一套策略决策流程
- 只是候选链不同

## M4：第二平台硬解落地

验收标准：

- Linux `VA-API` 或 macOS `VideoToolbox` 有至少一条可用链
- fallback 逻辑仍然统一

---

## 13. 风险与避免方式

## 风险 1：过早抽象，结果没有第二平台验证

避免方式：

- 每做一层抽象，都尽快让 Linux 软件链验证一次

## 风险 2：一次性挪目录太多

避免方式：

- 先稳定接口，再调整目录

## 风险 3：一上来就做 Linux/macOS 硬解

避免方式：

- 坚持“先软件链、后硬解”

## 风险 4：跨平台改造把 Windows 主链搞坏

避免方式：

- 每个阶段都保留 Windows 回归验证

---

## 14. 建议的实施顺序摘要

如果把整份路线图压缩成最简实施顺序，建议按下面做：

1. 先抽 `playback_strategy` 和 `platform_capabilities`
2. 再让 `PlayerCore` 不直接决定 Windows 最佳 renderer
3. 再补 `OpenGLVideoRenderer` 到最小可用
4. 再让 Linux 跑通 `software + SDL/OpenGL`
5. 再统一 Windows / Linux 的候选链与 fallback 逻辑
6. 最后才补 Linux/macOS 硬解

---

## 15. 一句话结论

这个项目走向跨平台，最正确的路线不是“把 Windows 特性删掉重来”，而是：

> 保留当前 `FFmpeg + SDL2 + Core/Render/Decoder` 的主结构，先把策略层和平台能力层独立出来，再用 Linux 跑通软件基线链，最后逐平台补硬解与高级渲染能力。

这样做的好处是：

- 不破坏当前 Windows 主链
- 能逐步验证架构是否真正可迁移
- 能把“跨平台”做成工程能力，而不是一次性重构赌博
