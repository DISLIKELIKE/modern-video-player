# Day26 实施记录：OpenGL ASS/SSA、driver quirk 与 HDR/色彩管理收敛

日期：2026-03-24  
范围：`src/render/opengl_video_renderer.cpp`

## implementation planner

1. 复用现有 `SubtitleItem / SubtitleTextRun` 语义，在 OpenGL 端补 item/run 级字幕渲染，而不是新增第二套字幕模型。
2. 把 OpenGL native interop 的决策前移到启动期，补环境变量覆盖、软件 GL blacklist 和 adapter/driver/profile 摘要日志。
3. 统一软件上传与原生互操作两条 OpenGL 色彩管线，至少补齐矩阵、transfer、gamut 感知和基础 tone-map。
4. 验证以 `Release` 为准，覆盖 OpenGL 主播放、字幕解析同步和字幕延迟联动三类检查。

## 本轮改动

### 1) ASS/SSA 渲染从简单叠字升级到 item/run 级渲染

- OpenGL 路径不再只依赖扁平文本。
- 新实现改为 `DirectWrite + D2D offscreen -> BGRA texture -> OpenGL overlay`：
  - 支持多 `SubtitleItem` 的稳定排序（`layer/index`）
  - 支持 `SubtitleTextRun` 分段样式
  - 支持定位、对齐、背景框、描边、阴影和填充
  - 叠加混合切到 premultiplied alpha，避免边缘发灰
- 这条路径的目标是和现有 `D3D11` 字幕语义保持一致，而不是在 OpenGL 里单独再造一套规则。

### 2) driver quirk 收敛到启动期策略，而不是只靠运行期回退

- OpenGL native interop 新增环境变量：`MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`。
- 启动期显式收敛软件 GL 场景：`Microsoft` vendor 或 `GDI Generic` renderer 时直接禁用 native interop。
- 启动日志新增 `[diag:opengl-native]`，直接输出：
  - GL vendor / renderer / version
  - D3D adapter / driver version
  - `H.264 / HEVC / VP9 / AV1` decoder profile 摘要
  - 最终 rule 和 `native_direct_allowed` 决策
- 当前 blacklist 仍保持保守，只覆盖最确定的不稳定条件，避免过早写死过多厂商规则。

### 3) HDR / 色彩管理先做到可观测、可降级的 M1

- `PendingVideoFrame` 现在继续携带 `color_transfer` 和 `color_primaries`，不再只看 matrix/range。
- GLSL `YUV420P` / `NV12` 路径和 native interop HLSL 路径统一接入：
  - `BT.601 / BT.709 / BT.2020` 矩阵选择
  - `PQ / HLG` 检测
  - 基础 SDR tone-map
  - `BT.2020 -> BT.709` gamut mapping
- 新增 `[diag:opengl-color]`，把 path、matrix、range、transfer、primaries、tone-map/gamut-map 状态直接打出来。
- 这还不是完整 display-managed HDR：没有 HDR swapchain、没有 OS metadata 输出、没有 ICC/3D LUT。

## 本地验证

```text
cmake --build build --config Release
结果：通过

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
结果：renderer_backend=OpenGL / decoder_backend=D3D11VA / video_native_output_frames=62 / video_copy_back_frames=0 / render_frames=47 / result=PASS

.\build\Release\modern-video-player.exe --subtitle-sync-check .\build\tmp_opengl_ass_validation.ass
结果：entries=2 / mismatches=0 / result=PASS

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\build\tmp_opengl_ass_validation.ass
结果：subtitle_loaded=true / subtitle_entries=2 / probe_found=true / result=PASS
```

## 与成熟播放器的剩余差距

- `ASS/SSA` 还不是完整 `libass` 级实现：karaoke、复杂矢量绘制、完整 shaping 和更细颗粒度的特效仍缺失。
- `HDR / 色彩管理` 还没有真正到显示设备端：缺 HDR 输出链、缺 display profile/ICC/3D LUT。
- driver quirk/blacklist 仍偏保守，需要更多现场适配器/驱动样本继续收敛。
- 当前强能力路径仍偏 Windows：字幕离屏渲染依赖 `DirectWrite/D2D`，native interop 依赖 `WGL_NV_DX_interop`。

## 结论

- 这轮之后，OpenGL 已经不再只是“能播”的过渡后端，而是具备字幕、启动期诊断和基础 HDR/色彩收敛的 `M1` 路径。
- 但它还不是 `mpv / MPC-HC` 级别的最终 GPU 后端，短板已经收敛到更具体的 `libass 完整度`、`显示级 HDR` 和 `更广的 driver quirk` 三块。