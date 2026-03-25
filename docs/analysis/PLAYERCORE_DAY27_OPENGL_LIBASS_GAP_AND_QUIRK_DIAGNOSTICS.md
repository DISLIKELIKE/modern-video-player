# Day27 实施记录：OpenGL libass 差距清单与 quirk/diagnostics 收敛

日期：2026-03-24  
范围：`src/render/opengl_video_renderer.cpp`、`src/main.cpp`

## implementation planner

1. 不把 `ASS/SSA` 讨论继续停留在“能不能显示字幕”，而是按成熟播放器实际依赖的 `shaping / karaoke / vector / font fallback / layout` 维度拆成清单。
2. 把 OpenGL native interop 的 quirk 判断从散落逻辑收敛成规则表和结构化诊断快照，并提供独立 CLI。
3. 保持当前 OpenGL 主播放链路不回退，验证 `--opengl-diagnostics`、OpenGL 播放主链和字幕回归命令仍然成立。

## 1) libass 级能力差距清单

### 1.1 当前已具备的 OpenGL 字幕能力

- 多 `SubtitleItem` 稳定排序
- `SubtitleTextRun` 分段样式应用
- 位置与对齐
- 背景框、描边、阴影、填充
- `ASS/SSA` 基础样式的离屏渲染与 OpenGL 叠加

### 1.2 与 `libass / mpv` 的关键差距拆解

| 维度 | 当前状态 | 差距等级 | 说明 |
|---|---|---|---|
| 文本 shaping / bidi / complex script | 部分 | 高 | 目前主要依赖 `DirectWrite` 基础排版，但没有明确收敛到 `libass` 级脚本兼容矩阵，也缺少回归样本集。 |
| karaoke / timed effect | 缺失 | 高 | `\k / \K / \kf / \ko` 这类逐字/逐段时序效果未实现。 |
| vector drawing | 缺失 | 高 | `\p` 绘图、复杂 path、矢量边界和相关动画未落地。 |
| clip / iclip / mask | 缺失 | 高 | 几何裁剪、反裁剪与更复杂蒙版未接入。 |
| transform / animation | 缺失 | 高 | `\t()`、旋转、缩放、时间窗动画仍未进入渲染链。 |
| border / shadow 精细语义 | 部分 | 中 | 已有基础描边/阴影，但还不是 `libass` 级细节和一致性。 |
| font fallback / attachment / font select | 部分 | 中 | 当前主要依赖系统字体和基本字体族，没有 attachment/font cache 策略。 |
| wrap / collision / line breaking 兼容性 | 部分 | 中 | 有基础布局，但未对齐 `libass` 的兼容行为。 |
| regression corpus | 缺失 | 高 | 目前缺少专门覆盖 `ASS effect tags` 的标准样本集和自动化 gate。 |

### 1.3 推荐分期

1. `P0`：先做样本清单和解析/渲染兼容矩阵，别继续用“感觉差不多”判断完成度。
2. `P1`：优先补 `shaping/font fallback/wrap`，这是日常字幕可读性的主收益区。
3. `P2`：再补 `karaoke / \t() / vector / clip`，这是接近 `libass/mpv` 的关键分水岭。
4. `P3`：最后补 attachment/font cache 和更系统的 `ASS regression corpus`。

### 1.4 当前结论

- 现在的 OpenGL 字幕链路已经超过“简单叠字”，但还不能宣称等价于 `libass`。
- 真正的成熟播放器差距已经被收敛到更清晰的几个块：`shaping`、`karaoke/animation`、`vector/clip`、`font attachment/fallback`。

## 2) quirk 机制收敛到规则表 + diagnostics snapshot

### 2.1 本轮代码收敛

- 新增 `OpenGLDiagnosticsSnapshot`，统一承载：
  - GL vendor / renderer / version
  - `WGL_NV_DX_interop` 可用性
  - 环境变量 override
  - D3D11 基础诊断快照
  - hard blocker / quirk rule / 最终有效规则
- `OpenGLVideoRenderer` 新增静态探测入口：
  - `OpenGLVideoRenderer::probeSystemDiagnostics()`
- `main` 新增：
  - `--opengl-diagnostics`

### 2.2 规则分层

- `hard blocker`：不能靠强制开关绕过的条件，例如当前上下文缺失 `WGL_NV_DX_interop`，或者底层 D3D11 诊断本身已经判定 native direct 不成立。
- `quirk rule`：保守禁用但可继续扩展归档的条件，例如软件 OpenGL 上下文。
- `env override`：
  - `disable` 永远直接关闭
  - `force` 只能覆盖 quirk，不覆盖 hard blocker

### 2.3 这样做的意义

- 播放期日志和独立 CLI 共享同一套事实源，不再是两份口径。
- 后续新增 driver/adapter 规则时，只需要往规则表扩充，而不是继续堆 if/else。
- `force` 与“真的不能开”的边界现在被显式写出来，用户排障不再需要猜。

## 本地验证

```text
cmake --build build --config Release
结果：通过

.\build\Release\modern-video-player.exe --opengl-diagnostics
结果：probe_succeeded=true / has_wgl_dx_interop=true / native_interop.allowed=true / result=PASS

$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --opengl-diagnostics
结果：env_override=disable / native_interop.allowed=false / disable_rule=env_disable / result=PASS

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
结果：renderer_backend=OpenGL / decoder_backend=D3D11VA / video_native_output_frames=62 / video_copy_back_frames=0 / result=PASS
```

## 结论

- OpenGL quirk 机制现在已经不是一段零散逻辑，而是“规则表 + 结构化快照 + 独立 CLI”的正式基础设施。
- `libass` 差距也从模糊的“还差很多”变成了可继续排期的明确清单。