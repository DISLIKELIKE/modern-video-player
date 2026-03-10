# Windows 播放器默认策略建议稿

更新日期：2026-03-09

## 1. 文档定位

本文档是一份面向当前仓库 `modern-video-player` 的 **Windows 播放器默认策略建议稿**。

目标是提供一份可以直接转化为：

- 产品默认行为设计
- 技术实现方案
- 设置项设计
- fallback 与错误处理设计
- 版本迭代计划

本文档不追求覆盖所有极端播放器玩法，而是优先服务于：

- `Windows 10/11`
- 桌面本地播放器
- 普通用户开箱即用
- 同时保留高级用户可调空间

---

## 2. 一句话结论

默认策略建议采用：

- **默认 renderer**：`D3D11 renderer`
- **默认视频解码**：`D3D11VA native`
- **默认 fallback 顺序**：`D3D11VA native -> D3D11VA copy-back -> DXVA2 native -> DXVA2 copy-back -> software decode`
- **默认渲染 fallback**：`D3D11 renderer -> SDL renderer`
- **默认用户设置暴露**：只暴露 `自动（推荐） / 兼容模式 / 软件解码` 三档，高级页再展开底层选项

这套方案的核心原则是：

- **默认硬解优先**
- **兼容性靠自动 fallback 兜底**
- **renderer 与 decoder 联动决策**
- **普通用户看到的是简单设置，高级用户可深度控制**

---

## 3. 产品目标

## 3.1 核心目标

1. 新装即播，不要求用户理解 `D3D11VA / DXVA2 / native / copy-back`
2. `1080p60` 长时间稳定播放
3. 现代机器上尽量稳定支持 `4K / HEVC / AV1 / HDR`
4. 遇到驱动、格式、字幕、渲染链不兼容时，优先自动降级而不是直接失败
5. 高级用户仍可手动选择 renderer 和解码方式

## 3.2 非目标

1. 第一阶段不追求发烧级 renderer 调参体验
2. 第一阶段不追求像 `PotPlayer` 一样暴露大量底层解码开关
3. 第一阶段不追求多平台统一策略，本文聚焦 Windows

---

## 4. 适用用户分层

## 4.1 普通用户

用户特征：

- 只关心能播、流畅、省心
- 不理解 renderer / hwdec 的术语

默认产品策略：

- 使用 `自动（推荐）`
- 后台自动做能力探测与 fallback
- UI 不直接展示 `D3D11VA native` 这类术语

## 4.2 进阶用户

用户特征：

- 会遇到驱动、字幕、HDR、截图、滤镜兼容问题
- 愿意手动切换兼容模式

默认产品策略：

- 可切 `兼容模式`
- 可查看“当前实际使用的解码链 / 渲染链”

## 4.3 高级用户

用户特征：

- 明确知道 `D3D11VA native / copy-back / DXVA2 / software`
- 可能会主动选择某个 renderer / decoder 组合

默认产品策略：

- 在高级设置中允许手动覆盖自动策略
- 允许查看能力探测结果、失败原因、当前链路日志

---

## 5. 默认产品策略

## 5.1 默认播放策略

建议默认值：

- `播放模式`：自动（推荐）
- `视频渲染器`：自动，优先 `D3D11 renderer`
- `视频解码`：自动，优先 `D3D11VA native`
- `音视频同步策略`：默认以音频时钟为主
- `字幕`：默认开启，优先走 renderer 支持的字幕叠加路径
- `日志等级`：普通模式默认只记录关键链路决策，调试模式记录完整 fallback 过程

## 5.2 默认 fallback 策略

视频解码 fallback 建议顺序：

1. `D3D11VA native`
2. `D3D11VA copy-back`
3. `DXVA2 native`
4. `DXVA2 copy-back`
5. `software decode`

视频渲染 fallback 建议顺序：

1. `D3D11 renderer`
2. `SDL renderer`

不建议第一阶段自动在 `D3D11 renderer` 与 `OpenGL renderer` 之间频繁切换，原因是：

- 切换 GPU API 的副作用更大
- 故障定位更复杂
- 当前项目已有 `sdl_video_renderer`，更适合作为稳定兜底

## 5.3 默认策略的优先级规则

默认策略不是固定死表，而应遵循以下优先级：

1. **用户显式设置优先于自动策略**
2. **renderer 能力优先于解码偏好**
3. **兼容性优先于极限性能**
4. **连续失败优先触发降级**，单次异常不立即切换

---

## 6. 推荐设置项设计

## 6.1 面向普通用户的设置项

建议在主设置页只暴露三档：

### `自动（推荐）`

- 自动选择最佳 renderer 与硬解方式
- 出错后自动 fallback
- 推荐作为默认值

### `兼容模式`

- 优先 `copy-back` 或更保守渲染路径
- 减少 `native / zero-copy` 带来的兼容性问题
- 适合字幕、截图、复杂链路用户

### `软件解码`

- 强制禁用硬解
- 用于排障与极端兼容场景

## 6.2 面向高级用户的设置项

建议在高级页展开：

- `视频渲染器`
  - 自动
  - D3D11
  - SDL
  - OpenGL（实验/备用）

- `视频解码`
  - 自动
  - D3D11VA native
  - D3D11VA copy-back
  - DXVA2 native
  - DXVA2 copy-back
  - Software

- `失败处理`
  - 自动 fallback 开 / 关
  - 失败阈值
  - 是否记住本次播放会话中的坏链路

- `调试信息`
  - 显示当前 renderer
  - 显示当前 decoder
  - 显示是否走 native/copy-back/software
  - 显示当前像素格式 / HDR 状态 / 帧丢弃统计

---

## 7. 技术默认链路建议

## 7.1 默认主链

当前项目的推荐默认主链：

```text
Demuxer -> DecoderFactory -> D3D11VA native -> D3D11 renderer -> Screen
```

映射到当前仓库建议对应：

- demux：`src/demuxer.cpp`
- decode selection：`src/decoder/decoder_factory.cpp`
- playback control：`src/core/player_core.cpp`
- rendering：`src/render/d3d11_video_renderer.cpp`
- presentation bridge：`src/display.cpp`

## 7.2 兼容主链

```text
Demuxer -> DecoderFactory -> D3D11VA copy-back -> D3D11 renderer -> Screen
```

适用场景：

- 需要复杂字幕叠加
- 需要更稳的截图路径
- `native` 出现黑屏、绿屏、边缘异常

## 7.3 保守主链

```text
Demuxer -> DecoderFactory -> Software decode -> SDL renderer -> Screen
```

适用场景：

- 驱动兼容性差
- 硬解连续失败
- 需要快速确认问题是否来自 GPU 链路

---

## 8. 能力探测策略

## 8.1 启动期探测

播放器启动时应缓存以下能力信息：

- 操作系统版本
- GPU 适配器信息
- D3D11 设备创建是否成功
- DXVA2 可用性
- 支持的硬解 codec / profile / pixel format
- HDR 相关能力
- 显示器色彩与位深信息

## 8.2 打开媒体时探测

打开媒体后再补充探测：

- codec 类型：`H.264 / HEVC / AV1 / VP9 ...`
- bit depth：`8bit / 10bit / 12bit`
- 色度：`4:2:0 / 4:2:2 / 4:4:4`
- HDR metadata
- 分辨率、帧率、码率
- 字幕/截图/滤镜链是否要求系统内存帧

## 8.3 探测结果如何参与决策

建议先做 **候选链生成**，再做过滤，而不是写死一串 `if/else`。

伪代码建议：

```cpp
candidates = [
  D3D11VA_NATIVE,
  D3D11VA_COPYBACK,
  DXVA2_NATIVE,
  DXVA2_COPYBACK,
  SOFTWARE,
];

filter_by_os(candidates);
filter_by_renderer_support(candidates);
filter_by_codec_profile_bitdepth(candidates);
filter_by_subtitle_and_capture_requirements(candidates);
sort_by_policy(candidates);
```

排序建议：

- 自动模式：`性能优先，但保留安全白名单`
- 兼容模式：`copy-back 优先`
- 软件模式：直接只保留 `software`

---

## 9. renderer 与 decoder 的联动规则

这是整套默认策略中最关键的部分。

## 9.1 联动原则

推荐规则：

- 如果当前 renderer 是 `D3D11 renderer`，优先允许 `D3D11VA native`
- 如果字幕/截图/软件处理链要求系统内存帧，优先 `copy-back`
- 如果 `D3D11 renderer` 创建失败，则渲染 fallback 到 `SDL renderer`
- 如果 renderer 已退到 `SDL renderer`，解码优先切 `software` 或 `copy-back`

## 9.2 不建议的做法

不建议：

- 只根据 codec 支持情况决定硬解，不看 renderer 能否接住
- 默认强开 `zero-copy`
- 不区分 `native` 与 `copy-back`
- 一旦失败就永久禁用全部硬解

---

## 10. fallback 设计建议

## 10.1 何时触发 fallback

建议把失败分成三类：

### 初始化失败

例如：

- D3D11 设备创建失败
- decoder open 失败
- media type negotiation 失败

处理：

- 直接切下一个候选链

### 运行时连续失败

例如：

- 连续多帧解码失败
- renderer 连续 present 失败
- 硬解表面无法正确转换

处理：

- 达到阈值后切下一候选链

### 单次异常

例如：

- 单次迟到帧
- 单帧渲染超时

处理：

- 记录日志，不立即降级

## 10.2 建议失败阈值

建议默认：

- 初始化阶段：`1 次即切`
- 运行时：`连续 3 次关键失败` 才触发 fallback

## 10.3 会话内坏链路记忆

建议在当前播放会话中记住：

- 某个文件
- 某个 codec/profile/bitdepth
- 某个 renderer + decoder 组合

如果已经被判定为坏链路，在本次会话内不再反复尝试。

---

## 11. 状态与配置模型建议

## 11.1 建议的用户配置模型

```cpp
enum class PlaybackMode {
    Auto,
    Compatibility,
    SoftwareOnly,
};

enum class RendererKind {
    Auto,
    D3D11,
    SDL,
    OpenGL,
};

enum class DecoderMode {
    Auto,
    D3D11VA_Native,
    D3D11VA_CopyBack,
    DXVA2_Native,
    DXVA2_CopyBack,
    Software,
};
```

## 11.2 建议的运行时决策状态

```cpp
struct PlaybackStrategyDecision {
    RendererKind requested_renderer;
    RendererKind actual_renderer;
    DecoderMode requested_decoder;
    DecoderMode actual_decoder;
    bool hardware_decode;
    bool copy_back;
    bool fallback_happened;
    std::string fallback_reason;
};
```

这个对象建议在以下模块之间传递：

- `video_player`
- `player_core`
- `decoder_factory`
- `renderer_factory`
- `display`

---

## 12. UI 与可观测性建议

## 12.1 普通设置页

只显示：

- 播放模式：自动 / 兼容 / 软件解码

## 12.2 高级设置页

显示：

- 当前 renderer
- 当前 decoder
- 当前实际链路：`D3D11VA native` / `copy-back` / `software`
- 当前视频格式摘要：codec / resolution / fps / HDR

## 12.3 调试信息面板

建议支持一键显示：

- 设备名
- 当前 decoder 链
- 当前 renderer 链
- pixel format
- 是否硬解
- 是否 copy-back
- fallback 是否发生
- 最近一次 fallback 原因

---

## 13. 日志设计建议

建议所有自动决策都打关键日志，至少包括：

- 用户请求的播放模式
- 候选链列表
- 被过滤掉的链及原因
- 实际启用的链
- fallback 发生时的上一链、下一链、失败原因

建议日志示例：

```text
[strategy] mode=auto codec=hevc bitdepth=10 renderer_candidates=[d3d11,sdl]
[strategy] decoder_candidates=[d3d11va_native,d3d11va_copyback,dxva2_native,software]
[strategy] reject=d3d11va_native reason=subtitle_path_requires_system_memory
[strategy] select decoder=d3d11va_copyback renderer=d3d11
[strategy] fallback from=d3d11va_copyback to=software reason=consecutive_render_failures
```

---

## 14. 与当前仓库的映射建议

## 14.1 建议承载模块

- 策略决策：`src/core/player_core.cpp`
- 解码候选选择：`src/decoder/decoder_factory.cpp`
- 渲染候选选择：`src/render/renderer_factory.cpp`
- 实际展示与状态反馈：`src/display.cpp`
- 用户配置：`src/config/*`

## 14.2 建议新增的抽象层

建议新增一个独立的“播放策略决策层”，例如：

- `include/core/playback_strategy.h`
- `src/core/playback_strategy.cpp`

职责：

- 汇总能力探测结果
- 生成候选链
- 执行过滤与排序
- 输出 `PlaybackStrategyDecision`

这样可以避免：

- `player_core` 里塞满策略分支
- `decoder_factory` 与 `renderer_factory` 各自做一半决策
- fallback 逻辑散落在多个模块里

---

## 15. 验收标准建议

## 15.1 功能验收

至少验证以下场景：

- `H.264 1080p`
- `HEVC 4K 10bit`
- `AV1 1080p / 4K`
- 带字幕文件
- seek 高频操作
- 暂停 / 恢复 / 切全屏
- 截图

## 15.2 策略验收

至少验证以下行为：

- 默认能选择 `D3D11VA native + D3D11 renderer`
- `native` 失败能自动退到 `copy-back`
- 硬解全部失败能退到软件解码
- `D3D11 renderer` 创建失败能退到 `SDL renderer`
- 用户手动选择的软件解码不会被自动策略覆盖

## 15.3 观测验收

必须能在日志或调试面板里回答：

- 当前实际使用了什么 decoder
- 当前实际使用了什么 renderer
- 是否发生过 fallback
- fallback 的具体原因是什么

---

## 16. 分阶段实施建议

## Phase 1：先做可用默认链

目标：

- `D3D11 renderer + D3D11VA native`
- 失败退 `software + SDL`
- UI 先只给三档模式

## Phase 2：补齐中间兼容层

目标：

- 加入 `copy-back`
- 加入 `DXVA2` fallback
- 加入更清晰的日志与决策对象

## Phase 3：补齐高级可观测性

目标：

- 调试面板
- 当前链路显示
- 会话内坏链路记忆

## Phase 4：高级用户控制

目标：

- 高级页手动 renderer / decoder 选择
- 更多设备能力展示
- 更细的兼容模式策略

---

## 17. 最终建议稿摘要

如果要把本文压缩成产品/技术一句话方案，建议写成：

> Windows 默认播放策略采用 `D3D11 renderer + D3D11VA native` 作为主链，以 `copy-back`、`DXVA2` 和 `software decode` 作为逐级 fallback；普通用户只暴露“自动 / 兼容 / 软件”三档设置，高级页再开放底层解码与渲染选项；策略决策应由独立模块统一生成候选链、过滤、排序、降级，并提供完整的链路日志和调试可观测性。

这套方案兼顾：

- 开箱即用
- 现代 Windows 性能
- 可维护的技术设计
- 可扩展的高级设置
