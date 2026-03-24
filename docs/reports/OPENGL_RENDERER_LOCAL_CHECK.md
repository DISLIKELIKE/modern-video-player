# OpenGL 渲染链路本地验收记录

**日期**: 2026-03-24

## 目标
- 把 `OpenGLVideoRenderer` 从占位实现补成可用的最小播放链路。
- 保持默认后端策略保守，不改变当前 `D3D11 -> SoftwareSDL` 主路径。
- 给后续是否继续做成熟 GPU 后端提供清晰基线。

## 本次范围
- SDL 窗口 + OpenGL 2.1 compatibility context
- GLSL 120 着色器做 `YUV420P / NV12 -> RGB` 转换
- `supportsDirectFrameFormat(AV_PIX_FMT_YUV420P/NV12)` 直通
- 基础键盘控制、全屏切换、退出事件
- init 失败自动回退 `SoftwareSDL`

## 非目标
- 字幕 GPU 叠加
- OSD / 控制条绘制
- 鼠标进度条 / 音量条交互
- D3D11 硬件表面原生互操作
- HDR / 色彩管理 / 高级后处理

## 验证命令
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Remove-Item Env:MVP_RENDERER_BACKEND
```

## 验证结果
- `performance-log-check.renderer_backend=OpenGL`
- `performance-log-check.decoder_backend=D3D11VA`
- `performance-log-check.result=PASS`
- OpenGL 上下文日志：`vendor=NVIDIA Corporation`、`renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2`、`version=4.6.0 NVIDIA 560.94`
- 当前运行环境音频设备不可用，因此 `audio_output_initialized=false`、`video_only_fallback=true`；这不影响本次 OpenGL 视频链路验收结论。

## 结论
- `OpenGLVideoRenderer` 已从 stub 变为可用的 M0 路径。
- 当前仍建议通过 `MVP_RENDERER_BACKEND=opengl` 显式启用，而不是作为默认后端。
- 与 `mpv / MPC-HC / VLC` 这类成熟播放器相比，当前差距仍主要在字幕/OSD GPU 叠加、硬件表面互操作和高级色彩管理。

## 2026-03-24 补充收敛：OpenGL 原生 D3D11 互操作稳定性修复
- 上文里“字幕 GPU 叠加 / OSD / D3D11 原生互操作”为非目标的结论已过期；当前仓库已经补齐这些能力的 M0 版本。
- 本轮额外修复了 OpenGL 原生互操作的稳定性问题：
  - 为 renderer-owned D3D11 device/context 开启 `ID3D11Multithread::SetMultithreadProtected(TRUE)`。
  - 调整 session release 顺序，先释放缓存 native frame 并关闭 renderer，再释放 decoder/hw context。

### 补充验证命令
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Remove-Item Env:MVP_RENDERER_BACKEND
```

### 补充验证结果
- `performance-log-check.renderer_backend=OpenGL`
- `performance-log-check.decoder_backend=D3D11VA`
- `performance-log-check.audio_output_initialized=true`
- `performance-log-check.video_only_fallback=false`
- `performance-log-check.video_native_output_frames=62`
- `performance-log-check.video_copy_back_frames=0`
- `performance-log-check.render_frames=47`
- `performance-log-check.result=PASS`
- 进程退出码：`0`

### 当前结论
- `OpenGLVideoRenderer` 已经不只是 M0 软件上传链路，而是具备基础字幕/OSD 叠加和 Windows `D3D11VA -> OpenGL` 原生硬件表面互操作的 opt-in GPU 后端。
- 它仍不是 `mpv / MPC-HC` 级别的最终形态，后续重点仍在 `ASS/SSA` 完整渲染、HDR/色彩管理、更多 driver quirk 收敛和跨平台后端策略统一。
