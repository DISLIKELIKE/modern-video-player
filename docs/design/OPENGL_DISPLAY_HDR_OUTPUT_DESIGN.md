# OpenGL 显示级 HDR 输出设计

日期：2026-03-24

## 目标

- 明确当前 OpenGL 后端从“renderer 内部 HDR aware 处理”走向“显示级 HDR 输出”的正式架构。
- 说明 Windows 下要做真正 HDR 输出时，`swapchain / metadata / ICC/3D LUT` 分别落在哪里。
- 给后续实现一个可执行的阶段拆分，而不是继续把 tone-map 当成完整 HDR 方案。

## 当前状态

当前 OpenGL 已经具备：
- `BT.601 / BT.709 / BT.2020` matrix 识别
- `PQ / HLG` 检测
- 基础 SDR tone-map
- `BT.2020 -> BT.709` gamut mapping

这仍然只是 renderer 内部变换，不等于显示级 HDR。当前还没有：
- HDR swapchain
- HDR metadata 输出
- 显示能力探测与 luminance 匹配
- ICC/profile 驱动的 SDR 色彩管理
- 3D LUT 校正链路

## 设计原则

1. 先把“场景线性空间的内部表示”固定下来，再决定 SDR/HDR 输出分叉。
2. OpenGL 自己不承担 Windows HDR present 终端职责；真正 HDR 输出应走 DXGI swapchain 桥接。
3. SDR 色彩管理和 HDR 输出要分开：
   - SDR 走 `ICC / 3D LUT`
   - HDR 走 `DXGI color space + HDR metadata`
4. 不把 OS HDR 开关、显示器 EDID、用户 tone-map 偏好继续混在渲染 shader 里硬编码。

## 推荐架构

### 阶段 A：统一内部颜色表示

- 解码后统一进入线性场景空间表示。
- 维护每帧元数据：
  - primaries
  - transfer
  - mastering/display metadata
  - content light metadata
- 这一阶段完成后，OpenGL shader 不再只做“临时 if/else 转换”，而是围绕一个明确的内部工作空间输出。

### 阶段 B：SDR 管理输出

目标：即使显示器不是 HDR，也不要只靠固定 `BT.2020 -> BT.709 + gamma`。

推荐路径：
1. 查询当前显示设备 ICC/profile。
2. 使用 `LittleCMS` 之类的颜色管理库离线生成 3D LUT。
3. 在 OpenGL 端上传 3D LUT 纹理，对 SDR 输出做最终校正。
4. 把用户 tone-map 选项放在 LUT 之前，而不是直接写死在 shader 常量里。

结果：
- SDR 显示器下的颜色管理成为正式链路。
- Windows `OpenGL` 后端不再只能靠“猜一个 709/gamma2.2”。

### 阶段 C：Windows HDR 输出桥接

真正的 HDR 输出建议采用“OpenGL 渲染 + D3D11/DXGI present bridge”。

#### 为什么不直接靠当前 SDL OpenGL 窗口

- Windows 显示级 HDR 主要依赖 `DXGI swapchain` 能力。
- `SetColorSpace1`、`SetHDRMetaData` 这些接口属于 `IDXGISwapChain3/4` 路径。
- 纯 SDL/WGL OpenGL 窗口并不天然提供这一层显示控制能力。

#### 推荐桥接方案

1. OpenGL 先把最终 HDR 场景结果渲染到可互操作的纹理。
2. 通过 `WGL_NV_DX_interop` 或共享纹理桥到 D3D11 输出纹理。
3. 使用 DXGI flip-model swapchain 做最终 present。
4. 在 DXGI 层设置：
   - `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` 或其他适当 color space
   - `SetHDRMetaData(DXGI_HDR_METADATA_TYPE_HDR10, ...)`
5. HDR metadata 来源：
   - 优先从码流/容器拿 mastering display metadata 和 content light metadata
   - 缺失时明确走 fallback 策略，不伪造“精确 metadata”

### 阶段 D：显示探测与模式决策

输出策略要看显示设备，而不是只看内容是 HDR 还是 SDR。

启动期应探测：
- `IDXGIOutput6::GetDesc1()` 暴露的输出 color space
- `MinLuminance / MaxLuminance / MaxFullFrameLuminance`
- OS HDR 开关状态
- 用户首选策略：
  - `passthrough hdr`
  - `tone-map to sdr`
  - `tone-map to hdr display target`

推荐形成机器可读的 display snapshot，后续可做单独 `--opengl-hdr-diagnostics`。

## swapchain 设计

### 当前建议

- SDR present：保持当前 SDL/OpenGL swap buffers 路径。
- HDR present：新增 D3D11/DXGI present bridge，不复用普通 SDL swap path。

### 关键点

- 使用 `flip discard / flip sequential` 模式的 DXGI swapchain。
- HDR path 优先考虑 `FP16` 或 `P010` 级输出中间面。
- OpenGL 侧继续负责内容组合与 shader 处理，DXGI 侧负责最终显示控制。

## metadata 设计

### 必要字段

- mastering display primaries
- mastering display white point
- mastering min/max luminance
- MaxCLL / MaxFALL
- transfer 标识：PQ / HLG

### 策略

- 有 metadata：原样透传并参与输出决策。
- 无 metadata：
  - 不伪装成完整 HDR10 mastering 信息
  - 使用 conservative fallback
  - 日志里明确标识 `metadata=missing_fallback`

## ICC / 3D LUT 设计

### SDR

- SDR 显示器或用户强制 SDR 输出时：
  - 从 ICC/profile 生成 3D LUT
  - 作为最后一级输出校正

### HDR

- HDR 输出模式下 ICC 不应与 HDR metadata 控制混成一条隐式链。
- 可保留 SDR UI/OSD 的单独映射策略，但视频主体以 HDR display path 为主。

## 分期建议

1. `M2`：显示探测 snapshot + CLI，明确当前机器的 HDR present 条件。
2. `M3`：内部线性工作空间 + metadata 管线整理。
3. `M4`：SDR ICC/3D LUT 输出。
4. `M5`：DXGI HDR swapchain bridge + HDR metadata 输出。
5. `M6`：HDR/SDR 混合 OSD、截图、窗口拖动和多显示器切换收敛。

## 风险与边界

- 真 HDR 输出会让 OpenGL 后端变成混合图形栈：OpenGL 渲染、D3D11/DXGI present，这比当前链路复杂得多。
- 多显示器和窗口跨屏移动时，HDR 状态与 ICC/profile 可能实时变化，必须有重建策略。
- OSD、字幕、截图和缩略图不能默认假设“最终输出永远是 SDR gamma 空间”。

## 结论

- 当前 OpenGL 已具备“内容级 HDR aware 处理”，但距离“显示级 HDR 输出”还差完整的 DXGI present bridge、metadata 管线和 ICC/3D LUT 策略。
- 真正可发布的 HDR 输出方案，不应继续堆在现有 shader 分支上，而应按 `内部工作空间 -> SDR 管理输出 / HDR DXGI 输出` 两条终端路径收敛。
## 2026-03-25 Probe Alignment Update

The design document is no longer purely forward-looking. The current implementation now exports display-output probe fields through `--opengl-diagnostics` under `opengl-diagnostics.hdr_output.*`.

Current probe surface:
- matched DXGI adapter index
- output found / attached-to-desktop
- `IDXGIOutput6` availability
- current DXGI output color space
- `advanced_color_active`
- `hdr_active`
- `bits_per_color`
- `min_luminance_nits`
- `max_luminance_nits`
- `max_full_frame_luminance_nits`

Meaning:
- The renderer can now explicitly distinguish “content is HDR-aware internally” from “the current execution environment exposes display-level HDR output state”.
- This does not yet implement HDR presentation. It only closes the capability-detection half of the design.
