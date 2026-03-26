# Phase 1 逐文件 TODO 实施单

更新日期：2026-03-09

## 0. 执行状态（2026-03-25）

- [x] `CP-101`：`platform_capabilities` 抽象与探测入口已落地
- [x] `CP-102`：`playback_strategy` 抽象与 open plan 已落地
- [x] `CP-103`：`RendererFactory` 已回收为“支持判断 + 创建”职责
- [x] `CP-104`：`DecoderFactory` 已改为 context-driven 排序并保留 software mandatory fallback
- [x] `CP-105`：`PlayerCore::open()` 已改为消费策略计划
- [x] `CP-106`：启动策略机器可读诊断（capabilities/candidates/selected/fallback）已接入 `--performance-log-check`

当前结论：Phase 1 核心抽离已完成，且 `CP-201` ~ `CP-205` 已完成职责收口与回归验证；下一步进入 `CP-301`（构建开关平台化）及后续 Linux MVP 交付链路。

## 1. 文档定位

这份文档是对 `docs/plans/CROSS_PLATFORM_REFACTOR_TASKLIST.md` 里 **Phase 1：抽平台能力与播放策略层** 的单独展开。

目标不是继续讲 roadmap，而是把这一阶段拆成：

- 具体要改哪些文件
- 每个文件里先做什么、后做什么
- 每个文件新增哪些结构/函数更合适
- 哪些内容属于本阶段范围，哪些不要提前做

一句话概括 Phase 1：

> **让 `PlayerCore` 从“自己拍板选 renderer / decoder”变成“消费策略结果的执行器”。**

---

## 2. Phase 1 的目标与边界

## 2.1 这阶段一定要达到的目标

Phase 1 结束后，至少要做到：

1. `PlayerCore::open()` 不再直接决定默认 renderer
2. `PlayerCore::open()` 不再直接把 decoder backend 选择逻辑写死在自己内部
3. 平台能力探测有单独入口，不再散落在工厂和 core 里
4. renderer / decoder 的默认顺序由 **策略层** 产出
5. Windows 当前默认行为不退化

## 2.2 这阶段明确不做的事

下面这些很重要，但 **不是 Phase 1 要做的事**：

- 不重构 `include/render/video_renderer.h`
- 不拆 `src/display.cpp`
- 不让 `OpenGLVideoRenderer` 变成完整实现
- 不引入 `VAAPI / VideoToolbox`
- 不改播放器主状态机
- 不做大规模 UI / 输入层调整

也就是说，这阶段只解决：

- 平台能力从哪里来
- 默认策略由谁决定
- `PlayerCore` 如何消费策略结果

---

## 3. Phase 1 结束后的目标结构

推荐先把打开媒体的主链收敛成下面这样：

```text
PlayerCore::open()
  -> Demuxer open
  -> collect media info
  -> PlatformCapabilities::detect()
  -> PlaybackStrategy::buildOpenPlan(...)
  -> try renderer candidates in order
  -> init audio if present
  -> try decoder backend candidates in order
  -> create queues
  -> reset clock / diagnostics
  -> enter Ready/Stopped(当前实现)
```

换句话说：

- `PlayerCore`：执行者
- `PlatformCapabilities`：探测者
- `PlaybackStrategy`：决策者
- `RendererFactory` / `DecoderFactory`：具体实例化与候选信息提供者

---

## 4. 建议新增/修改文件总览

| 类型 | 文件 | 处理方式 | 作用 |
| --- | --- | --- | --- |
| 新增 | `include/platform/platform_capabilities.h` | 新建 | 定义平台能力探测结果 |
| 新增 | `src/platform/platform_capabilities.cpp` | 新建 | 实现 compile-time + runtime 能力探测 |
| 新增 | `include/core/playback_strategy.h` | 新建 | 定义策略请求、策略结果、默认顺序规划 |
| 新增 | `src/core/playback_strategy.cpp` | 新建 | 实现 open 阶段的默认策略 |
| 修改 | `include/render/renderer_factory.h` | 改接口 | 工厂从“只会创建”进化成“还能回答支持情况/可见候选” |
| 修改 | `src/render/renderer_factory.cpp` | 改实现 | 去掉核心默认策略硬编码，转为工厂能力回答者 |
| 修改 | `include/decoder/decoder_factory.h` | 改接口 | 让 decoder 选择能接收更多上下文 |
| 修改 | `src/decoder/decoder_factory.cpp` | 改实现 | 把排序逻辑改成 context-driven |
| 修改 | `include/core/player_core.h` | 改接口 | 增加策略集成辅助结构/私有 helper |
| 修改 | `src/core/player_core.cpp` | 改实现 | `open()` 改为消费策略结果 |
| 修改 | `CMakeLists.txt` | 改构建 | 把新增文件纳入构建 |

---

## 5. 推荐实施顺序

建议严格按下面顺序做，不要跳步：

1. 新建 `platform_capabilities.*`
2. 新建 `playback_strategy.*`
3. 改 `renderer_factory.*`
4. 改 `decoder_factory.*`
5. 改 `player_core.h / player_core.cpp`
6. 改 `CMakeLists.txt`
7. 最后补日志与验收

原因很简单：

- 没有 `PlatformCapabilities`，策略层没输入
- 没有 `PlaybackStrategy`，`PlayerCore` 无法消费统一决策
- 先改 `PlayerCore` 很容易中间态很乱

---

## 6. 逐文件 TODO 实施单

## 6.1 `include/platform/platform_capabilities.h`（新增）

### 这个文件的职责

定义“当前平台和当前构建到底能做什么”。

它不是具体 renderer，也不是 decoder 工厂，而是一个统一能力快照。

### 建议放进去的内容

建议新增下面这些类型：

```cpp
namespace vp::platform {

enum class PlatformKind {
    Windows,
    Linux,
    MacOS,
    Unknown
};

struct RendererSupport {
    render::VideoRendererType type;
    bool compiled_in{false};
    bool runtime_available{false};
    int default_priority{0};
};

struct DecoderBackendSupport {
    decoder::DecoderBackend backend;
    bool compiled_in{false};
    bool runtime_available{false};
    bool hardware_accelerated{false};
    int default_priority{0};
};

struct PlatformCapabilities {
    PlatformKind platform{PlatformKind::Unknown};
    std::vector<RendererSupport> renderers;
    std::vector<DecoderBackendSupport> decoders;
    bool has_sdl_windowing{false};
    bool has_sdl_audio{false};
};

class PlatformCapabilitiesProbe {
public:
    static PlatformCapabilities detect();
};

} // namespace vp::platform
```

### 逐项 TODO

- [ ] 定义 `PlatformKind`
- [ ] 定义 renderer 能力描述结构
- [ ] 定义 decoder 能力描述结构
- [ ] 定义总的 `PlatformCapabilities`
- [ ] 定义统一探测入口 `PlatformCapabilitiesProbe::detect()`
- [ ] 头文件只表达抽象，不做平台 API 细节探测逻辑

### 这一步的设计要求

- 不要在头文件里引入 D3D11 头
- 不要在头文件里做任何 ifdef 逻辑展开
- 让它只成为一个“结果类型定义 + 探测入口声明”的地方

### 完成标准

- `PlaybackStrategy` 可以只依赖这个头，就拿到平台能力输入
- 后续加 Linux/macOS backend 时，不需要重写这个文件结构

---

## 6.2 `src/platform/platform_capabilities.cpp`（新增）

### 这个文件的职责

把平台能力探测集中实现掉。

当前阶段不需要把 runtime probe 做得特别复杂，**先把 compile-time + 简单 runtime 判断跑通** 即可。

### 建议实现内容

第一版建议只做这些：

- `platform`：通过 `_WIN32` / `__linux__` / `__APPLE__` 判断
- renderer：
  - Windows：`D3D11` compiled in
  - 全平台：`SoftwareSDL` compiled in
  - 若项目编译了 OpenGL 路径，则 `OpenGL` compiled in
- decoder：
  - `Software` 永远 compiled in
  - Windows 下 `D3D11VA` 视当前代码能力设为 compiled in
- runtime_available：第一版可以先等于 `compiled_in`，以后再增强

### 逐项 TODO

- [ ] 实现 `detectPlatformKind()` 私有 helper
- [ ] 实现 `buildRendererSupportList()` 私有 helper
- [ ] 实现 `buildDecoderSupportList()` 私有 helper
- [ ] 实现 `PlatformCapabilitiesProbe::detect()`
- [ ] 输出稳定的 `default_priority`
- [ ] 给日志层提供可打印的调试字符串辅助函数（可选）

### 第一版优先级建议

第一版建议默认优先级这样定：

- Windows renderer：`D3D11 > SoftwareSDL > OpenGL`
- 非 Windows renderer：`OpenGL > SoftwareSDL`
- decoder：`D3D11VA > Software`

注意：

这里的优先级只是“能力层默认倾向”，**真正顺序最终仍由 `PlaybackStrategy` 决定**。

### 完成标准

- 调用一次 `detect()` 能拿到一个自描述的能力对象
- `RendererFactory` 和 `DecoderFactory` 不再需要独立假设“当前是什么平台”

---

## 6.3 `include/core/playback_strategy.h`（新增）

### 这个文件的职责

定义“给定一次打开媒体请求，策略层应该产出什么结果”。

这个文件是 Phase 1 的核心抽象之一。

### 建议放进去的内容

建议定义下面这些类型：

```cpp
namespace vp::core {

struct PlaybackPreferences {
    bool prefer_hardware_decode{true};
    render::VideoRendererType preferred_renderer{render::VideoRendererType::Auto};
};

struct PlaybackOpenRequest {
    MediaInfo media_info;
    PlaybackPreferences preferences;
    platform::PlatformCapabilities platform_capabilities;
};

struct PlaybackOpenPlan {
    std::vector<render::VideoRendererType> renderer_candidates;
    std::vector<decoder::DecoderBackend> video_decoder_candidates;
    bool allow_hardware_decode{true};
    std::string debug_reason;
};

class PlaybackStrategy {
public:
    static PlaybackOpenPlan buildOpenPlan(const PlaybackOpenRequest& request);
};

} // namespace vp::core
```

### 逐项 TODO

- [ ] 定义 `PlaybackPreferences`
- [ ] 定义 `PlaybackOpenRequest`
- [ ] 定义 `PlaybackOpenPlan`
- [ ] 定义 `PlaybackStrategy::buildOpenPlan()`
- [ ] 让 plan 至少包含 renderer 候选和 decoder 候选
- [ ] 给 plan 增加 `debug_reason` 或 `debug_notes`

### 设计要求

- 不要把“如何真正 init renderer”写进策略层
- 不要让策略层依赖 `PlayerCore` 内部状态
- 不要让策略层直接持有 renderer/decoder 实例

### 完成标准

- `PlayerCore::open()` 只需要拿到一个 `PlaybackOpenPlan` 就能按顺序执行
- 后续把用户配置接进来时，不需要重写 plan 结构

---

## 6.4 `src/core/playback_strategy.cpp`（新增）

### 这个文件的职责

实现 Phase 1 的默认策略逻辑。

当前阶段它不是终极策略系统，而是一个 **足够简单、足够稳定、可替代现有硬编码逻辑** 的默认决策器。

### 第一版建议决策规则

#### renderer 选择规则

- 如果用户显式指定 renderer，优先把它放第一位
- 然后只保留 `platform_capabilities.renderers` 中 `runtime_available == true` 的项
- 如果媒体有视频流：
  - Windows 默认 `D3D11 -> SoftwareSDL -> OpenGL`
  - 非 Windows 默认 `OpenGL -> SoftwareSDL`
- 如果媒体没有视频流：renderer candidates 为空

#### decoder 选择规则

- 如果媒体没有视频流：视频 decoder candidates 为空
- 如果 `prefer_hardware_decode == false`：直接 `Software`
- 如果允许硬解：
  - Windows 且 codec 属于当前 `D3D11VA` 支持集：`D3D11VA -> Software`
  - 其他情况：`Software`

### 逐项 TODO

- [ ] 实现 `buildRendererCandidates()` 私有 helper
- [ ] 实现 `buildVideoDecoderCandidates()` 私有 helper
- [ ] 对 `MediaInfo` 做视频/音频存在性判断
- [ ] 对 codec 名称做最小支持判断
- [ ] 给 plan 填充 `debug_reason`
- [ ] 保证候选列表去重且顺序稳定

### 这一阶段最重要的约束

- 策略逻辑不要引用 D3D11 具体设备对象
- 策略逻辑不要引用 `PlayerCore` 的 queue/thread 成员
- 不要提前把“copy-back / native / zero-copy”细节塞进这里

### 完成标准

- 现有 `_WIN32 -> D3D11` 的默认策略能完整迁到这里
- 未来非 Windows 只需要补能力和偏好输入，不需要再改 `PlayerCore`

---

## 6.5 `include/render/renderer_factory.h`（修改）

### 当前问题

当前接口只有：

- `detectBestRendererType()`
- `create(type)`

这说明工厂同时承担了：

- 默认策略判断
- 实例创建

Phase 1 里要做的不是把工厂变复杂，而是把它 **收回到“工厂本职”**。

### 建议改成的职责

建议让它回答下面几类问题：

- 某种 renderer 当前是否可创建
- 某种 renderer 的名字是什么
- 根据 type 创建实例

### 建议新增/调整接口

```cpp
class RendererFactory {
public:
    static bool isSupported(render::VideoRendererType type,
                            const platform::PlatformCapabilities& caps);
    static const char* rendererName(render::VideoRendererType type);
    static VideoRendererPtr create(render::VideoRendererType type);
};
```

### 逐项 TODO

- [ ] 删除或废弃 `detectBestRendererType()` 的核心职责
- [ ] 新增 `isSupported(type, caps)`
- [ ] 新增 `rendererName(type)`
- [ ] 保留 `create(type)`
- [ ] 头文件引入 `platform_capabilities.h`

### 这一步不要做什么

- 不要在工厂里重新写一套完整 fallback 逻辑
- 不要让工厂反过来依赖 `PlayerCore`

### 完成标准

- `RendererFactory` 不再是“默认策略中心”
- `PlayerCore` 可以通过 `PlaybackOpenPlan` + `RendererFactory::create()` 完成初始化

---

## 6.6 `src/render/renderer_factory.cpp`（修改）

### 当前问题

当前核心问题是：

```cpp
#if defined(_WIN32)
return VideoRendererType::D3D11;
#else
return VideoRendererType::SoftwareSDL;
#endif
```

这其实是策略层逻辑，不应该继续放在这里。

### 逐项 TODO

- [ ] 删除 `detectBestRendererType()` 中的核心平台默认策略
- [ ] 如短期兼容需要，可保留一个过渡实现，但内部调用策略/能力层，而不是写死 ifdef
- [ ] 实现 `isSupported(type, caps)`
- [ ] 让 `create(type)` 只负责实例创建，不负责排序
- [ ] 统一 renderer 名称输出，便于日志打印

### 第一版实现建议

- `SoftwareSDL`：默认认为支持
- `D3D11`：仅在 Windows + compiled in 时支持
- `OpenGL`：仅在 compiled in 时支持

### 完成标准

- 工厂文件里不再承担跨平台默认策略
- 后续策略变化只需要改 `playback_strategy.cpp`

---

## 6.7 `include/decoder/decoder_factory.h`（修改）

### 当前问题

当前接口只有：

- `probeCapabilities()`
- `selectBackendOrder(codec_name, prefer_hardware)`
- `selectBestBackend(...)`

问题在于选择输入过少，只看：

- codec 名称
- prefer_hardware

这不足以支撑跨平台默认策略。

### 建议新增上下文对象

```cpp
struct DecoderSelectionContext {
    std::string codec_name;
    bool prefer_hardware{true};
    render::VideoRendererType renderer_type{render::VideoRendererType::Auto};
    platform::PlatformCapabilities platform_capabilities;
};
```

### 建议调整接口

```cpp
class DecoderFactory {
public:
    static std::vector<DecoderCapability> probeCapabilities(
        const platform::PlatformCapabilities& caps);
    static std::vector<DecoderBackend> selectBackendOrder(
        const DecoderSelectionContext& context);
    static DecoderBackend selectBestBackend(
        const DecoderSelectionContext& context);
    static const char* backendName(DecoderBackend backend);
};
```

### 逐项 TODO

- [ ] 定义 `DecoderSelectionContext`
- [ ] 修改 `probeCapabilities()` 入参
- [ ] 修改 `selectBackendOrder()` 入参
- [ ] 修改 `selectBestBackend()` 入参
- [ ] 头文件纳入 `platform_capabilities.h`

### 完成标准

- decoder 选择不再只靠 codec 名称和一个 bool
- 后面接入 renderer 兼容性、平台能力时，不需要再推翻接口

---

## 6.8 `src/decoder/decoder_factory.cpp`（修改）

### 当前问题

当前实现只有：

- `Software`
- Windows 下 `D3D11VA`

并且排序逻辑非常轻：

- 支持 codec 就入候选
- `prefer_hardware=false` 就排掉硬解
- 最后按 priority 排

这对于 Phase 1 来说已经够做基础，但要把输入收口成 context-driven。

### 逐项 TODO

- [ ] 修改 `probeCapabilities(caps)`，从 `PlatformCapabilities` 读取可见 backend
- [ ] 修改 `selectBackendOrder(context)`，从 context 中读取 codec / hardware preference / renderer type
- [ ] 先保留当前 backend 集合不扩容，只重构接口与排序输入
- [ ] 保证最终顺序一定包含 `Software` 兜底
- [ ] 输出更清楚的日志友好顺序

### Phase 1 推荐保留的简化策略

Phase 1 不建议过度聪明，推荐维持：

- Windows + `prefer_hardware` + codec 支持：`D3D11VA -> Software`
- 其他情况：`Software`

### 完成标准

- `DecoderFactory` 成为“候选顺序生成器”，而不是平台假设散落点
- 后续补 `VAAPI` 时，只需要扩 `caps + context + capability`

---

## 6.9 `include/core/player_core.h`（修改）

### 当前问题

当前 `PlayerCore` 直接依赖：

- `RendererFactory::detectBestRendererType()`
- decoder 选择逻辑的隐式结果

这会让它继续变成“执行 + 决策混合体”。

### 这一步要做什么

让 `PlayerCore` 增加几个私有 helper，专门消费策略结果。

### 建议新增内容

可以考虑新增下面这些私有 helper：

```cpp
bool initRendererFromPlan(const core::PlaybackOpenPlan& plan,
                          const render::VideoRendererConfig& config);

bool initVideoDecoderFromPlan(const core::PlaybackOpenPlan& plan,
                              const MediaInfo& info);
```

如果你不想一开始拆太多，也可以先只加：

- `bool initRendererWithFallbackOrder(...)`
- `decoder::DecoderBackend selectVideoDecoderBackend(...)`

### 逐项 TODO

- [ ] 头文件引入 `core/playback_strategy.h`
- [ ] 增加 plan 消费 helper 声明
- [ ] 不把 `PlatformCapabilities` 持久化为复杂全局状态，先以 open 流程内局部对象为主
- [ ] 如有必要，增加一个 `last_open_plan_` 仅用于日志/debug（可选）

### 完成标准

- `open()` 可以明显变短
- `PlayerCore` 私有方法更像“执行计划”，而不是“现场决策”

---

## 6.10 `src/core/player_core.cpp`（修改）

### 这是 Phase 1 最核心的改造点

当前 `open()` 最大的问题是：

- 自己决定默认 renderer
- 自己做 renderer fallback
- 自己内含 decoder 策略语义

Phase 1 的目标是把这些挪到：

- `PlatformCapabilities`
- `PlaybackStrategy`
- `RendererFactory`
- `DecoderFactory`

`PlayerCore` 自己只负责“按结果执行”。

### 建议把 `open()` 改成下面这个顺序

```text
1. close old session
2. open demuxer
3. read media info
4. detect platform capabilities
5. build PlaybackOpenRequest
6. build PlaybackOpenPlan
7. init renderer by plan.renderer_candidates
8. init audio if present
9. init decoders by plan.video_decoder_candidates
10. create queues
11. reset flags / clock / diagnostics
12. return success
```

### 逐项 TODO

- [ ] 在 `open()` 里调用 `PlatformCapabilitiesProbe::detect()`
- [ ] 构造 `PlaybackPreferences`
- [ ] 构造 `PlaybackOpenRequest`
- [ ] 调用 `PlaybackStrategy::buildOpenPlan()`
- [ ] 用 plan 的 renderer 候选顺序替代 `detectBestRendererType()`
- [ ] 用 plan 的 decoder backend 候选顺序替代隐式 backend 选择
- [ ] 为 plan 的执行过程补日志：候选顺序、失败点、最终命中项
- [ ] 让 renderer fallback 从“硬编码一个 SDL 兜底”变成“遍历候选列表”
- [ ] 初始化失败时保留现有错误码语义

### 推荐新增的私有 helper

建议至少拆出这两个 helper：

#### `initRendererFromPlan(...)`

职责：

- 遍历 `plan.renderer_candidates`
- 对每一个候选执行：`RendererFactory::create(type)` + `init(config)`
- 成功即返回
- 全部失败则报 `DisplayInitFailed`

#### `resolveVideoDecoderBackendFromPlan(...)`

职责：

- 遍历 `plan.video_decoder_candidates`
- 结合 `initDecoders()` 当前实际能力，选一个最终 backend
- 记录命中与失败原因

如果你愿意更进一步，也可以把现有 `initDecoders()` 再拆成：

- `initVideoDecoder(DecoderBackend backend)`
- `initAudioDecoder()`

但这属于“Phase 1 可做可不做的增强”，不是硬要求。

### 这一步要特别注意的边界

- 不要顺手重构 `pumpEvents()`
- 不要顺手改 `seek()`
- 不要顺手改状态机
- 不要顺手改 `video_renderer.h`

### 完成标准

- `open()` 不再直接调用 `RendererFactory::detectBestRendererType()`
- renderer fallback 由 plan 驱动，而不是硬编码 if-else
- decoder backend 顺序由 plan/context 产出
- 现有 Windows 默认行为不退化

---

## 6.11 `CMakeLists.txt`（修改）

### 这一步只做最小接线

Phase 1 里，`CMakeLists.txt` 不需要大改 feature option，只需要把新增文件纳入构建。

### 逐项 TODO

- [ ] 将 `src/core/playback_strategy.cpp` 加入 `add_executable(...)`
- [ ] 将 `src/platform/platform_capabilities.cpp` 加入 `add_executable(...)`
- [ ] 如果新增了对应头文件所在目录，不需要额外改 include path（当前 `include/` 已统一纳入）

### 这一步不要做什么

- 不要在 Phase 1 就引入一堆 `ENABLE_XXX` option
- 不要在 Phase 1 就重新设计全部 CMake 平台开关

### 完成标准

- 新增策略层和平台能力层可被当前工程正常编译链接

---

## 7. Phase 1 推荐提交拆分

建议按下面 commit 粒度推进：

### Commit 1

- 新增 `include/platform/platform_capabilities.h`
- 新增 `src/platform/platform_capabilities.cpp`

提交意图：先把平台能力抽象立起来。

### Commit 2

- 新增 `include/core/playback_strategy.h`
- 新增 `src/core/playback_strategy.cpp`

提交意图：先把策略输入输出对象立起来。

### Commit 3

- 修改 `include/render/renderer_factory.h`
- 修改 `src/render/renderer_factory.cpp`

提交意图：让 renderer 工厂回到“实例化 + 支持判断”职责。

### Commit 4

- 修改 `include/decoder/decoder_factory.h`
- 修改 `src/decoder/decoder_factory.cpp`

提交意图：让 decoder 选择变成 context-driven。

### Commit 5

- 修改 `include/core/player_core.h`
- 修改 `src/core/player_core.cpp`
- 修改 `CMakeLists.txt`

提交意图：让 `PlayerCore` 正式消费策略结果。

---

## 8. Phase 1 的日志与验收建议

虽然这阶段不一定要加完整测试，但至少要补一套日志验收。

### 8.1 启动日志至少要能看到

建议在 `open()` 里打印：

- 平台类型
- renderer 候选顺序
- decoder 候选顺序
- 最终命中的 renderer
- 最终命中的 decoder backend
- 每个失败候选的失败原因

### 8.2 最低验收场景

Phase 1 至少验证下面 4 个场景：

1. Windows + 视频文件：仍然优先尝试 `D3D11`
2. Windows + `D3D11` 初始化失败：仍然可退到 `SoftwareSDL`
3. 纯音频文件：不应强行走 video renderer 候选逻辑
4. `prefer_hardware_decode=false`：video decoder 顺序直接退回 `Software`

### 8.3 通过标准

只要满足这几点，就说明 Phase 1 基本成功：

- 默认策略不再散落在 `PlayerCore` 和工厂文件里
- 新增第二平台时，不需要再把 ifdef 继续塞进 `open()`
- 当前 Windows 主链行为保持一致

---

## 9. Phase 1 明确不该碰的文件

为了防止范围失控，我建议 Phase 1 **显式不碰** 这些文件：

- `include/render/video_renderer.h`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/display.cpp`
- `src/video_player.cpp`
- `src/main.cpp`

原因不是这些文件不重要，而是它们属于：

- Phase 2 的 renderer 接口收口
- Phase 3 的第二平台 renderer 做实
- Phase 5 的配置与应用层接入

---

## 10. 一句话执行建议

如果你明天就开始动手，我建议你按这句执行：

> **先把 `PlatformCapabilities` 和 `PlaybackOpenPlan` 两个对象建出来，再让 `PlayerCore::open()` 从“自己决策”改成“执行计划”。**

只要这一步做对，后面的 renderer 收口、跨平台扩 backend、设置项接入，都会顺很多。
