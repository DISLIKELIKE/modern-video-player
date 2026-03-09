# 当前仓库跨平台改造任务清单

更新日期：2026-03-09

## 1. 文档定位

这份文档不是泛泛而谈的“跨平台愿景”，而是面向当前仓库 `modern-video-player` 的一份 **文件级改造任务清单**。
目标是回答三个问题：

1. 先改什么，后改什么
2. 每一阶段要动哪些文件
3. 每一阶段改完以后，如何判断没有把项目改散

这份清单默认遵循一个现实前提：

- **先保住 Windows 主链路**
- **再做出 Linux 可运行基线**
- **最后再补平台专属硬解与高级能力**

---

## 2. 当前仓库的跨平台切入点

从现有代码结构看，仓库不是“完全 Windows 写死”，而是已经有一部分跨平台基础，只是平台策略和渲染职责还没有真正收口。

### 2.1 已有的可复用基础

- `CMakeLists.txt` 已经有 `WIN32 / 非 WIN32` 分支
- `src/audio_player.cpp` 基于 SDL 音频，天然适合作为跨平台基线
- `src/render/renderer_factory.cpp` 已经有 renderer 工厂抽象
- `src/decoder/decoder_factory.cpp` 已经有 decoder backend 排序逻辑
- `src/core/player_core.cpp` 已经承担播放主链，适合继续作为统一 core
- `src/video_player.cpp` 已经是 facade 入口，适合作为 future app/service 层接口

### 2.2 目前最明显的 Windows 绑定点

- `src/core/player_core.cpp` 在 `open()` 里直接决定 renderer 选择与 fallback
- `src/render/renderer_factory.cpp` 仍然是 `_WIN32 -> D3D11，其他 -> SoftwareSDL`
- `src/decoder/decoder_factory.cpp` 目前只显式建模了 `Software` 和 `D3D11VA`
- `include/render/video_renderer.h` 把“渲染职责”和“播放器命令/事件消费”揉在一起
- `src/render/opengl_video_renderer.cpp` 还是占位实现，第二平台基线并未真正成立
- `src/display.cpp` 承担了较重的窗口/UI/事件职责，后续要收口边界
- `CMakeLists.txt` 仍然按“Windows 是主路径，其他平台只是能编”来组织

### 2.3 这意味着什么

当前仓库最合适的跨平台路径，不是“推倒重写”，而是：

1. 先把 **策略选择** 从 `PlayerCore` 中拆出来
2. 再把 **renderer 接口** 收口成纯渲染职责
3. 再让 `OpenGL/SDL` 变成真正能跑的第二平台链路
4. 最后再补 `VAAPI / VideoToolbox` 之类平台增强

---

## 3. 推荐先后顺序（一句话版）

推荐按下面顺序推进，不建议打乱：

1. **抽平台能力与默认策略层**
2. **收口 renderer 接口，去掉输入/命令耦合**
3. **做实 OpenGL/SDL 跨平台基线**
4. **改造 CMake 与 feature 开关**
5. **把策略接到 settings / 用户配置层**
6. **最后再补 Linux/macOS 硬解能力**

不建议一开始就同时做：

- D3D11 + OpenGL + VAAPI + VideoToolbox 一起上
- UI 重构、播放器状态机重构、跨平台改造三件事同时做
- 在 `PlayerCore` 还没抽策略之前直接新增一堆 backend

---

## 4. 总体任务总表

| 阶段 | 核心目标 | 主要文件 | 结果物 |
| --- | --- | --- | --- |
| Phase 0 | 现状收口与边界确认 | `CMakeLists.txt`、`src/core/player_core.cpp`、`src/render/renderer_factory.cpp`、`src/decoder/decoder_factory.cpp`、`src/display.cpp` | 明确平台假设与改造边界 |
| Phase 1 | 抽平台能力与播放策略层 | `include/core/player_core.h`、`src/core/player_core.cpp`、`include/render/renderer_factory.h`、`src/render/renderer_factory.cpp`、`include/decoder/decoder_factory.h`、`src/decoder/decoder_factory.cpp`、新增 `platform/` 与 `playback_strategy` 文件 | `PlayerCore` 不再亲自做平台策略决策 |
| Phase 2 | 收口 renderer 接口 | `include/render/video_renderer.h`、`src/render/sdl_video_renderer.cpp`、`src/render/d3d11_video_renderer.cpp`、`src/render/opengl_video_renderer.cpp`、`src/display.cpp`、`src/video_player.cpp` | renderer 只负责渲染，不负责播放器命令语义 |
| Phase 3 | 做实第二平台基线 | `include/render/opengl_video_renderer.h`、`src/render/opengl_video_renderer.cpp`、`src/render/sdl_video_renderer.cpp`、`src/render/renderer_factory.cpp`、`CMakeLists.txt` | Linux/非 Windows 至少有软件可用链路 |
| Phase 4 | Build 与 capability 开关显式化 | `CMakeLists.txt`、`src/render/renderer_factory.cpp`、`src/decoder/decoder_factory.cpp` | 构建系统显式知道有哪些 backend |
| Phase 5 | 接入设置层与用户策略 | `include/config/settings_manager.h`、`src/config/settings_manager.cpp`、`include/video_player.h`、`src/video_player.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、`src/main.cpp` | 默认策略可配置，可回放问题现场 |
| Phase 6 | 平台增强：VAAPI / VideoToolbox 等 | `include/decoder/decoder_capability.h`、`src/decoder/decoder_factory.cpp`、`include/core/player_core.h`、`src/core/player_core.cpp`、新增 `platform/hw_device_factory.*`、`CMakeLists.txt` | 硬解扩展不再污染 core 主链 |
| Phase 7 | 后续深拆 display / ui / input | `src/display.cpp`、`include/display.h`、新增 `ui/`、`input/` 分层文件 | 让窗口、UI、输入成为可替换层 |

---

## 5. Phase 0：现状收口与边界确认

这一阶段不追求“功能变化”，追求的是 **先把改造边界定对**。

### 5.1 需要重点审查的文件

| 文件 | 这一步要看什么 | 结论要写清楚什么 |
| --- | --- | --- |
| `CMakeLists.txt` | 现在哪些 source / lib 仅能在 Windows 编译 | 哪些 backend 是“编译期能力”，哪些只是“运行期选择” |
| `src/core/player_core.cpp` | `open()` 里做了哪些平台决策、fallback、初始化顺序 | 哪些逻辑必须迁出 core |
| `src/render/renderer_factory.cpp` | renderer auto 选择逻辑 | 将来应该改成“按 capability + preference 选” |
| `src/decoder/decoder_factory.cpp` | decoder backend 顺序规则 | 目前规则缺哪些输入维度 |
| `include/render/video_renderer.h` | renderer 接口里哪些方法不属于渲染职责 | 哪些方法应该迁给 display / input / command 层 |
| `src/display.cpp` | SDL 窗口、事件、UI、交互混杂点 | 哪部分后续适合作为 WindowHost / UIHost |

### 5.2 本阶段建议产出

- 一张“当前平台耦合点清单”
- 一张“后续新增抽象文件名清单”
- 一个明确结论：**先抽策略，再抽 renderer 接口，不先碰 VAAPI/VideoToolbox**

### 5.3 完成标准

满足下面三点即可进入 Phase 1：

- 你已经能说清 `PlayerCore` 里哪些代码是“执行层”，哪些代码是“策略层”
- 你已经能说清 `video_renderer.h` 里哪些方法不应该继续留在 renderer
- 你已经确定第二平台基线选 `OpenGL/SDL + SDL audio + software decode`

---

## 6. Phase 1：抽平台能力与播放策略层

这是整个跨平台改造里 **最优先、最关键** 的一步。

目标不是增加新功能，而是让 `PlayerCore` 不再亲自决定：

- 用哪个 renderer
- 用哪个 decoder backend
- fallback 顺序是什么

### 6.1 新增文件

| 文件 | 作用 |
| --- | --- |
| `include/platform/platform_capabilities.h` | 定义平台能力探测结果，例如可编译 renderer、可用 decoder backend、当前平台类型 |
| `src/platform/platform_capabilities.cpp` | 统一做 compile-time + runtime capability 探测 |
| `include/core/playback_strategy.h` | 定义播放策略输入与输出，例如 `PlaybackOpenRequest`、`PlaybackStrategyResult` |
| `src/core/playback_strategy.cpp` | 根据媒体信息、用户偏好、平台能力，生成 renderer / decoder fallback 顺序 |

### 6.2 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `include/core/player_core.h` | 增加对 `PlaybackStrategyResult` 的消费入口，不再把“选 backend”写死在 core 私有流程里 |
| `src/core/player_core.cpp` | `open()` 改成“先拿 strategy 结果，再按顺序尝试初始化”，不要再直接调用 `detectBestRendererType()` |
| `include/render/renderer_factory.h` | 工厂接口从“给一个 type 创建实例”扩成“根据 capability / requested type 判断是否可创建” |
| `src/render/renderer_factory.cpp` | 去掉“Windows 固定 D3D11、其他固定 SDL”的硬编码默认策略，改由 strategy 层给候选顺序 |
| `include/decoder/decoder_factory.h` | 让 backend 排序接口接收更多上下文，例如 codec、prefer_hardware、renderer 类型、平台能力 |
| `src/decoder/decoder_factory.cpp` | 从“只看 codec + prefer_hardware”改成“看平台能力 + 用户偏好 + 媒体能力”的排序函数 |
| `CMakeLists.txt` | 把新增 `platform_capabilities.cpp`、`playback_strategy.cpp` 纳入构建 |

### 6.3 这一阶段建议的策略模型

建议让策略层至少输出下面几类信息：

- `renderer_candidates`
- `decoder_backend_candidates`
- `preferred_present_path`
- `allow_hardware_decode`
- `reason / debug_text`

也就是说，`PlayerCore` 以后应该像执行器，而不是拍脑袋做判断的人。

### 6.4 建议的提交顺序

1. 先加 `platform_capabilities.*`
2. 再加 `playback_strategy.*`
3. 再改 `renderer_factory.*`
4. 再改 `decoder_factory.*`
5. 最后改 `player_core.*` 接入策略结果

### 6.5 完成标准

- `src/core/player_core.cpp` 里不再出现“平台默认 renderer 决策”硬编码
- `PlayerCore::open()` 只负责“按策略执行初始化 + fallback”
- Windows 当前默认行为不退化
- 后续新增 Linux/macOS backend 时，不需要继续把分支硬塞进 `PlayerCore`

---

## 7. Phase 2：收口 renderer 接口

这一阶段的核心目标是：**让 renderer 重新变回 renderer**。

当前 `include/render/video_renderer.h` 里的问题不是“方法多”，而是“职责混”。
它既做渲染，又做事件轮询，又做播放器命令消费，还挂着 overlay/subtitle/hotkey 语义。
这会让每新增一个 renderer，都被迫复制一套播放器输入语义。

### 7.1 建议新增文件

| 文件 | 作用 |
| --- | --- |
| `include/core/playback_command.h` | 定义播放命令，例如 Pause、Seek、SeekDelta、Volume、Speed、Screenshot、ABRepeat |
| `include/render/render_overlay_state.h` | 定义 renderer 只需要知道的 overlay/subtitle 状态数据 |

### 7.2 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `include/render/video_renderer.h` | 删除或迁移 `consumeTogglePauseRequest()`、`consumeSeekRequest()`、`consumeVolumeChangeRequest()` 这类播放器命令接口；保留纯渲染生命周期接口 |
| `src/render/sdl_video_renderer.cpp` | 跟着新接口收口，把渲染代码和输入语义拆开 |
| `src/render/d3d11_video_renderer.cpp` | 同上，保留 backend 特有的资源管理与呈现逻辑 |
| `src/render/opengl_video_renderer.cpp` | 按新接口实现，不再被命令消费接口污染 |
| `src/display.cpp` | 接管 SDL 事件轮询、鼠标/键盘输入、UI 控件点击，并产出 `PlaybackCommand` |
| `src/video_player.cpp` | 负责把输入层产生的命令交给 `PlayerCore`，而不是继续从 renderer 反向“拉请求” |
| `include/core/player_core.h` / `src/core/player_core.cpp` | 增加统一命令入口，例如 `submitCommand()` 或 `handleCommand()` |

### 7.3 这一阶段要达到的接口状态

理想情况下，`IVideoRenderer` 最终只保留这类职责：

- `init / close`
- `renderFrame`
- `present`
- `clear`
- `setOverlayState`
- `rendererBackendName`
- 如有必要，再补 `resize / onSurfaceLost / capabilities`

而下面这些语义，不应继续留在 renderer：

- 暂停/播放切换
- seek 请求
- 音量与倍速调整
- 截图请求
- A-B Repeat
- 章节跳转
- 下一项/上一项

### 7.4 建议的提交顺序

1. 新增 `playback_command.h`
2. 新增 `render_overlay_state.h`
3. 修改 `video_renderer.h`
4. 依次改 `sdl_video_renderer.cpp`、`d3d11_video_renderer.cpp`、`opengl_video_renderer.cpp`
5. 最后改 `display.cpp`、`video_player.cpp`、`player_core.cpp`

### 7.5 完成标准

- 新增 renderer 时，只需要关心 frame upload / shader / present
- 输入命令链不再绑定某一个 renderer 实现
- `OpenGLVideoRenderer` 不再因为“还没补完播放器命令接口”而一直只能占位

---

## 8. Phase 3：做实 OpenGL/SDL 第二平台基线

这一阶段的目标非常明确：

> 先让“非 Windows 平台至少能稳定播放”成立。

不要一上来就追求 Linux 上也有最优硬解。先把软件链路跑稳，整个项目才有真正跨平台的地基。

### 8.1 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `include/render/opengl_video_renderer.h` | 补齐真正可用的 OpenGL renderer 状态定义 |
| `src/render/opengl_video_renderer.cpp` | 从占位实现改成真实初始化流程：创建 GL 上下文、纹理上传、颜色转换、present |
| `include/render/sdl_video_renderer.h` / `src/render/sdl_video_renderer.cpp` | 明确 SDL 软件渲染器是“保底 renderer”，而不是临时 demo |
| `src/render/renderer_factory.cpp` | 非 Windows 平台默认候选顺序建议改成 `OpenGL -> SoftwareSDL` |
| `src/core/player_core.cpp` | 按策略结果尝试 `OpenGL`，失败后自动 fallback 到 `SoftwareSDL` |
| `CMakeLists.txt` | 增加 OpenGL 相关依赖查找与链接，避免只靠 Windows 路径假定 |

### 8.2 这一步建议达成的最小链路

Linux / 非 Windows 最小可用链路建议定义为：

- demux：FFmpeg
- decode：software
- audio：SDL audio
- video：OpenGL，失败则 SDL software
- input/window：SDL

### 8.3 不建议在这一阶段做的事

- 不要同时补 Vulkan renderer
- 不要同时补 VAAPI 零拷贝路径
- 不要一边补 OpenGL，一边重写全部 UI 皮肤系统

### 8.4 完成标准

- `OpenGLVideoRenderer` 不再是空实现
- 非 Windows 构建能给出明确默认 renderer 策略
- 软件解码 + OpenGL/SDL 渲染 + SDL 音频 能稳定形成基线播放链

---

## 9. Phase 4：Build 与 capability 开关显式化

当前 `CMakeLists.txt` 更像“Windows 主项目 + 其他平台兼容分支”。
跨平台继续往前走，就必须让构建系统显式知道：

- 哪些 backend 被编译进来了
- 哪些 backend 只是当前平台不支持
- 哪些 backend 以后要按 option 开关控制

### 9.1 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `CMakeLists.txt` | 引入显式 option，例如 `ENABLE_D3D11_RENDERER`、`ENABLE_SDL_RENDERER`、`ENABLE_OPENGL_RENDERER`、`ENABLE_D3D11VA`、`ENABLE_VAAPI`、`ENABLE_VIDEOTOOLBOX` |
| `src/render/renderer_factory.cpp` | 根据编译期开关和 runtime capability 决定某个 renderer 是否可见 |
| `src/decoder/decoder_factory.cpp` | 同理，根据编译期开关和 capability 输出 decoder backend 候选 |

### 9.2 推荐做法

建议区分两类能力：

- **编译期能力**：这个 backend 有没有被编进程序
- **运行期能力**：当前机器/驱动/依赖是否真的可用

后续 `PlatformCapabilities` 就可以同时持有这两层信息。

### 9.3 完成标准

- 非 Windows 构建不会再去无意义地编译 Windows 专属路径
- 构建日志或启动日志能明确打印出“本次构建启用了哪些 backend”
- 后续加 `VAAPI`、`VideoToolbox` 时只需加 option 与 capability，而不是继续堆 ifdef

---

## 10. Phase 5：接入 settings / 用户策略层

跨平台播放器一旦进入“多 renderer、多 hwdec backend”的阶段，没有设置层就很难 debug 和复现。
所以这一阶段的目标不是做花哨 UI，而是让默认策略 **可配置、可覆盖、可回放问题现场**。

### 10.1 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `include/config/settings_manager.h` / `src/config/settings_manager.cpp` | 增加 renderer、hwdec、fallback policy 相关 key 的读写约定 |
| `include/video_player.h` / `src/video_player.cpp` | 增加对用户偏好设置的传递，而不是只传 `prefer_hardware_decode_` 一个布尔值 |
| `include/core/player_core.h` / `src/core/player_core.cpp` | 让 `open()` 能拿到更完整的 `PlaybackOpenOptions / PlaybackPreferences` |
| `src/main.cpp` | 加载配置，形成默认策略入口 |

### 10.2 建议最先支持的配置项

建议最小配置集先支持下面几项：

- `video.renderer = auto / d3d11 / opengl / sdl`
- `video.hwdec = auto / on / off`
- `video.hwdec.zero_copy = auto / on / off`
- `video.fallback_policy = safe / balanced / aggressive`

### 10.3 这一步的价值

这一步完成后，你就有了一个非常重要的工程能力：

- 用户说“这台机器 OpenGL 黑屏”
- 你可以让他切换 renderer
- 你可以保留日志
- 你可以复现场景
- 你可以知道问题是策略层、backend 层，还是 capability 探测层

### 10.4 完成标准

- 默认策略不再只写死在代码里
- 相同程序在不同机器上的默认行为可以通过设置和日志被解释清楚
- 后续加新 backend 不需要再给 UI 层做一堆临时补丁

---

## 11. Phase 6：平台增强——VAAPI / VideoToolbox / 更多 hwdec

等前面几步做完之后，再做这一步才是顺序正确的。

### 11.1 新增文件

| 文件 | 作用 |
| --- | --- |
| `include/platform/hw_device_factory.h` | 抽象 FFmpeg `AVHWDeviceContext` 创建逻辑 |
| `src/platform/hw_device_factory.cpp` | 按 backend 创建 D3D11VA / VAAPI / VideoToolbox 设备 |

### 11.2 必改文件

| 文件 | 要改什么 |
| --- | --- |
| `include/decoder/decoder_capability.h` | 扩展 backend 枚举，例如 `VAAPI`、`VideoToolbox`，必要时再补 `DXVA2` |
| `include/decoder/decoder_factory.h` / `src/decoder/decoder_factory.cpp` | 扩展 capability 与 backend 排序规则 |
| `include/core/player_core.h` / `src/core/player_core.cpp` | 不再在 core 里硬编码某一个 hw device 初始化过程，改调 `hw_device_factory` |
| `CMakeLists.txt` | 增加平台专属 hwaccel option 和依赖检测 |

### 11.3 正确的实现顺序

建议按下面顺序补，不要并行开太多：

1. Windows：把现有 D3D11VA 路径抽干净
2. Linux：增加 `VAAPI -> Software`
3. macOS：预留 `VideoToolbox -> Software`

### 11.4 完成标准

- 新增 hwdec backend 时，不需要再深入修改 `PlayerCore` 主流程
- 平台 hw device 初始化逻辑被收口到单独模块
- fallback 顺序可以统一由策略层生成

---

## 12. Phase 7：后续深拆 display / ui / input（非第一优先级）

`src/display.cpp` 现在承担的职责比较重，但不建议在跨平台第一阶段就大拆。
更合理的顺序是：**等策略层与 renderer 接口收口以后，再拆 display。**

### 12.1 建议后续拆分方向

| 目标文件 | 拆分方向 |
| --- | --- |
| `src/display.cpp` | 拆成 WindowHost、OverlayPresenter、InputRouter |
| `include/display.h` | 跟着拆成更窄的接口头 |
| `src/ui/skin_engine.cpp` | 保留 UI 皮肤层，不直接绑定 renderer 初始化策略 |
| `src/input/hotkey_manager.cpp`（如后续补实现） | 作为输入映射层，而不是 renderer 附属物 |

### 12.2 为什么不放前面

因为它虽然重要，但不是当前跨平台的主阻塞点。
当前真正的主阻塞点是：

- 平台策略没抽出来
- renderer 接口职责过宽
- 第二平台基线并未成立

---

## 13. 建议的提交粒度（直接照着做也可以）

如果你想把这件事真正拆成可执行开发任务，建议按下面 commit 粒度推进：

1. `platform: add platform_capabilities abstraction`
2. `core: add playback_strategy and move default selection out of PlayerCore`
3. `render: decouple renderer interface from playback commands`
4. `render: make OpenGL renderer a real cross-platform backend`
5. `build: add explicit backend options and capability guards`
6. `config: expose renderer and hwdec preferences`
7. `platform: add hw_device_factory for platform hwaccel backends`
8. `linux/macos: add VAAPI or VideoToolbox strategy path`

这个拆法的好处是：

- 每一步都能单独验证
- 每一步都能回滚
- 每一步都能做 code review
- 不会把“跨平台改造”做成一个无法合并的大分支

---

## 14. 现阶段最值得马上开工的文件顺序

如果只问一句“明天开始第一刀先砍哪几个文件”，我给你的顺序是：

1. `include/platform/platform_capabilities.h`（新增）
2. `src/platform/platform_capabilities.cpp`（新增）
3. `include/core/playback_strategy.h`（新增）
4. `src/core/playback_strategy.cpp`（新增）
5. `include/render/renderer_factory.h`
6. `src/render/renderer_factory.cpp`
7. `include/decoder/decoder_factory.h`
8. `src/decoder/decoder_factory.cpp`
9. `include/core/player_core.h`
10. `src/core/player_core.cpp`
11. `include/render/video_renderer.h`
12. `src/render/sdl_video_renderer.cpp`
13. `src/render/d3d11_video_renderer.cpp`
14. `src/render/opengl_video_renderer.cpp`
15. `src/display.cpp`
16. `CMakeLists.txt`

也就是说，**先抽策略，再收接口，再做第二平台 renderer，最后改 build 和配置层**。

---

## 15. 最终落地目标（务实版）

如果按这份清单推进，比较务实的阶段性目标应该是：

### 目标 A：Windows 架构先干净

- Windows 默认仍然 `D3D11 -> SDL fallback`
- `PlayerCore` 不再承担平台策略判断
- renderer 接口收口完毕

### 目标 B：Linux 形成可用基线

- `software decode + SDL audio + OpenGL/SDL render`
- 可以稳定打开、播放、暂停、seek
- 构建系统显式知道当前启用了哪些 backend

### 目标 C：平台增强可持续演进

- 后续加 `VAAPI / VideoToolbox` 不会再把 core 打乱
- 可以继续往真正跨平台播放器架构演进

---

## 16. 一句话结论

这仓库当前最正确的跨平台改造顺序不是“先移植 renderer”，而是：

> **先抽默认策略，再收 renderer 接口，再建立 OpenGL/SDL 第二平台基线，最后补平台专属硬解。**

如果顺序反了，后面每加一个平台，都会继续把 `PlayerCore`、`renderer_factory`、`video_renderer.h` 搅得更重。
