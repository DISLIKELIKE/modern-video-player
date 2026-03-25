# OpenGL 渲染链路本地验收记录

**日期**: 2026-03-24

## 当前定位
- 当前 `OpenGL` 后端定位为 Windows 下的 `opt-in M1` GPU 路径。
- 默认后端仍保持 `D3D11`，`OpenGL` 继续通过 `MVP_RENDERER_BACKEND=opengl` 显式启用。
- native interop 启动期策略已正式化为：`hard blocker + quirk rule + env override`。

## 当前已具备能力
- `SDL + OpenGL 2.1 compatibility context + GLSL 120` 视频链路。
- `YUV420P / NV12` 软件帧上传与显示。
- Windows `D3D11VA -> OpenGL` 原生硬件表面互操作。
- OpenGL 启动期诊断与策略：
  - `OpenGLDiagnosticsSnapshot`
  - `OpenGLVideoRenderer::probeSystemDiagnostics()`
  - `--opengl-diagnostics`
  - `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`
  - `hard blocker + quirk rule + env override`
- `ASS/SSA` 字幕 item/run 级渲染：
  - `DirectWrite + D2D offscreen -> GL texture`
  - `layer/index` 稳定排序
  - 分段样式、定位/对齐、背景框、描边、阴影、填充
- 软件路径与 native interop 路径统一的色彩处理：
  - `BT.601 / BT.709 / BT.2020` 矩阵
  - `PQ / HLG` 检测
  - 基础 SDR tone-map
  - `BT.2020 -> BT.709` gamut mapping
  - `[diag:opengl-color]` 诊断日志

## 新增诊断命令
```powershell
.\build\Release\modern-video-player.exe --opengl-diagnostics

$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --opengl-diagnostics
Remove-Item Env:MVP_OPENGL_NATIVE_INTEROP
```

## 新增诊断结果
- 默认探测：
  - `opengl-diagnostics.probe_succeeded=true`
  - `opengl-diagnostics.gl_vendor=NVIDIA Corporation`
  - `opengl-diagnostics.gl_renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2`
  - `opengl-diagnostics.has_wgl_dx_interop=true`
  - `opengl-diagnostics.native_interop.allowed=true`
  - `opengl-diagnostics.native_interop.hard_blocker_matched=false`
  - `opengl-diagnostics.native_interop.quirk_rule_matched=false`
  - `opengl-diagnostics.result=PASS`
- 显式禁用互操作：
  - `opengl-diagnostics.native_interop.env_override=disable`
  - `opengl-diagnostics.native_interop.allowed=false`
  - `opengl-diagnostics.native_interop.disable_rule=env_disable`
  - `opengl-diagnostics.result=PASS`

## 播放与字幕回归命令
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Remove-Item Env:MVP_RENDERER_BACKEND

.\build\Release\modern-video-player.exe --subtitle-sync-check .\build\tmp_opengl_ass_validation.ass

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\build\tmp_opengl_ass_validation.ass
Remove-Item Env:MVP_RENDERER_BACKEND
```

## 播放与字幕回归结果
- OpenGL 播放链路：
  - `performance-log-check.renderer_backend=OpenGL`
  - `performance-log-check.decoder_backend=D3D11VA`
  - `performance-log-check.video_native_output_frames=62`
  - `performance-log-check.video_copy_back_frames=0`
  - `performance-log-check.result=PASS`
- 字幕解析同步：
  - `subtitle-sync-check.entries=2`
  - `subtitle-sync-check.mismatches=0`
  - `subtitle-sync-check.result=PASS`
- 字幕联动与延迟调整：
  - `delay-adjust-check.subtitle_loaded=true`
  - `delay-adjust-check.probe_found=true`
  - `delay-adjust-check.result=PASS`

## 与成熟播放器的剩余差距
- `libass` 级字幕能力仍未补齐：`shaping / karaoke / vector / clip / attachment / font fallback` 还缺正式实现和样本 gate。
- 显示级 HDR 输出仍未实现：当前只有 renderer 内部 HDR aware 处理，没有 `DXGI HDR swapchain / metadata / ICC/3D LUT` 输出链。
- driver quirk 规则表已正式化，但仍需更多 adapter/driver 样本继续扩表。

## 本轮新增参考文档
- [PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md](/d:/VSProject/sssssssssssssss/modern-video-player/docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md)
- [OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md](/d:/VSProject/sssssssssssssss/modern-video-player/docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md)

## 结论
- OpenGL 后端现在已经具备正式的独立诊断入口和更清晰的后续路线。
- 这轮之后，OpenGL 的主要剩余工作已被压缩到两类明确问题：
  - `libass` 级字幕兼容度
  - 显示级 HDR 输出

## 2026-03-24 ASS 样式扩展回归
### 命令
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
Remove-Item Env:MVP_RENDERER_BACKEND

.\build\Release\modern-video-player.exe --opengl-diagnostics
```

### 结果
- `subtitle-style-check.entries=5`
- `subtitle-style-check.result=PASS`
- `subtitle-sync-check.mismatches=0`
- `subtitle-sync-check.result=PASS`
- `delay-adjust-check.subtitle_loaded=true`
- `delay-adjust-check.subtitle_entries=5`
- `delay-adjust-check.result=PASS`
- `opengl-diagnostics.probe_succeeded=true`
- `opengl-diagnostics.native_interop.allowed=true`
- `opengl-diagnostics.result=PASS`

### 本轮确认
- OpenGL/D3D11 字幕渲染已经消费新的 ASS 样式字段：
  - `WrapStyle/\q`
  - `Spacing/\fsp`
  - `ScaleX/\fscx`
  - `ScaleY/\fscy`
  - `Angle/\fr/\frz`
  - `X/Y border`
  - `X/Y shadow`
- 样式诊断命令和回归样本已进入仓库：
  - `--subtitle-style-check`
  - `samples/subtitles/opengl_ass_style_validation.ass`

## 2026-03-24 GPU Subtitle Karaoke / Clip Regression

### Commands
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_karaoke_clip_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
```

### Results
- `subtitle-style-check.result=PASS` on `opengl_ass_style_validation.ass`
- `subtitle-style-check.result=PASS` on `opengl_ass_karaoke_clip_validation.ass`
- `subtitle-sync-check.result=PASS`
- `delay-adjust-check.result=PASS`
- `d3d11-diagnostics.result=PASS`
- `opengl-diagnostics.result=PASS`

### Notes
- Verified parser/model output for `secondary_rgba`, `has_clip`, `inverse_clip`, clip bounds, karaoke mode and karaoke timing.
- Verified OpenGL native interop probe remains healthy on the local NVIDIA GTX 1080 test machine after the subtitle renderer changes.
- Current `iclip` coverage is limited to rectangular clip regions; vector clip rendering is still pending.

## 2026-03-24 ASS Move / Fade Regression

### Commands
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_animation_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_animation_validation.ass
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_karaoke_clip_validation.ass
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
```

### Results
- `subtitle-style-check.result=PASS` on `opengl_ass_animation_validation.ass`
- `subtitle-sync-check.result=PASS` on `opengl_ass_animation_validation.ass`
- `subtitle-style-check.result=PASS` on `opengl_ass_karaoke_clip_validation.ass`
- `delay-adjust-check.result=PASS`
- `d3d11-diagnostics.result=PASS`
- `opengl-diagnostics.result=PASS`

### Notes
- Verified parser output for `animation.has_move`, `move_has_timing`, move coordinates, fade mode and fade timing.
- Verified the existing OpenGL native interop path remains healthy after adding animation-driven subtitle invalidation.
- This round does not yet cover `t(...)`, `org`, `fax/fay`, `frx/fry`, vector drawing or vector clip.


## 2026-03-25 ASS Transform / Vector / Font Fallback Regression

### Commands
```powershell
./build/Release/modern-video-player.exe --subtitle-style-check ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
./build/Release/modern-video-player.exe --subtitle-sync-check ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
$env:MVP_RENDERER_BACKEND='opengl'
./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
$env:MVP_RENDERER_BACKEND='d3d11'
./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_transform_vector_font_validation.ass
```

### Results
- `subtitle-style-check.result=PASS`
- `subtitle-sync-check.result=PASS`
- OpenGL `delay-adjust-check.result=PASS`
- D3D11 `delay-adjust-check.result=PASS`

### Notes
- Verified parser/model output for `source_path`, drawing mode, drawing commands, rotation origin, projected rotation, shear, and vector clip payload.
- Verified the OpenGL runtime regression was `D2DERR_PUSH_POP_UNBALANCED (0x88990016)` caused by mismatched clip layer push/pop calls, not by missing device capability.
- Verified the same push/pop balance fix was applied to both OpenGL and D3D11 subtitle renderers.
- Current font fallback coverage is subtitle-sidecar/private font registration plus family-chain fallback; full container attachment extraction remains future work.

## 2026-03-25 OpenGL runtime diagnostics and 10-bit copy-back regression

### Commands
```powershell
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200
```

### Results
- Native OpenGL regression:
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=57`
  - `video_copy_back_frames=0`
  - `video_swscale_frames=0`
  - `renderer_opengl_native_interop_active=true`
  - `renderer_opengl_present_wait_timeouts=0`
  - `result=PASS`
- Forced OpenGL copy-back regression on `juren-30s.mp4`:
  - `video_native_output_frames=0`
  - `video_copy_back_frames=52`
  - `renderer_opengl_native_interop_active=false`
  - `renderer_opengl_present_wait_timeouts=0`
  - `result=PASS`
- Forced OpenGL copy-back regression on generated 10-bit HEVC sample:
  - `decoder_backend=D3D11VA`
  - `video_copy_back_frames=72`
  - `video_swscale_frames=0`
  - OpenGL color log: `path=software format=p010le`
  - `result=PASS`

### Notes
- Confirmed the runtime `renderer_opengl_*` counters are visible in `--performance-log-check`.
- Confirmed OpenGL now directly uploads `p010le` after D3D11 copy-back instead of forcing an 8-bit `swscale`.
- The temporary validation sample lives under `build\tmp\opengl-p010-validation.mp4` and is not part of the repository payload.

## 2026-03-25 OpenGL present override and gate regression

### Commands
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_PRESENT_MODE='immediate'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- Auto mode:
  - `renderer_opengl_present_mode_requested=auto`
  - `renderer_opengl_present_mode_active=paced`
  - `renderer_opengl_present_wait_timeouts=0`
  - `result=PASS`
- Immediate mode:
  - `[diag:opengl-present] requested=immediate active=immediate`
  - `renderer_opengl_present_mode_requested=immediate`
  - `renderer_opengl_present_mode_active=immediate`
  - `result=PASS`
- Gate script:
  - `OpenGL gate result: PASS`

### Notes
- The present-mode override is designed for现场排障, not as a user-facing playback preference yet.
- The gate script auto-generates `build\tmp\opengl-p010-validation.mp4` when no external 10-bit sample is supplied.

## 2026-03-25 OpenGL HDR probe, quirk expansion, and final gate regression

### Commands
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --opengl-diagnostics

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- `opengl-diagnostics.result=PASS`
- `opengl-diagnostics.hdr_output.probe_succeeded=true`
- Current shell result: `opengl-diagnostics.hdr_output.output_found=false`
- Current shell note: `opengl-diagnostics.hdr_output.probe_error=matched adapter has no DXGI outputs`
- `OpenGL gate result: PASS`

### Notes
- In this environment the DXGI adapter can be matched, but no desktop output is exposed to the probe, so display-level HDR state remains unavailable in this session.
- This is still a valid probe result and is now explicitly machine-readable.
- The OpenGL gate now covers 10 checks, including the current ASS subtitle sample suite.

## 2026-03-25 OpenGL hotkey and OSD interaction regression

### Commands
```powershell
cmake --build build --config Release --target modern-video-player

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- Release build: PASS
- OpenGL gate: `OpenGL gate result: PASS`

### Notes
- The OpenGL renderer now suppresses `SDL_KEYDOWN repeat` for hotkey actions, which avoids repeated seek/volume storms from one physical key press.
- Hotkey-triggered OSD wakeup now requests an immediate redraw instead of waiting for a paused-only repaint path.
- OpenGL now exposes the bottom control panel through startup wakeup plus mouse move/click/drag handling for progress and volume.
- The automated gate does not synthesize SDL keyboard or mouse input, so a short manual GUI smoke is still recommended after packaging:
  - `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe .\a.mp4`
  - Verify `Space`, `Left/Right`, `Up/Down`, mouse move, progress drag, and volume drag.

## 2026-03-25 ASS transform transition regression

### Commands
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass

$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- `subtitle-style-check.result=PASS`
- OpenGL `delay-adjust-check.result=PASS`
- D3D11 `delay-adjust-check.result=PASS`
- OpenGL gate: `OpenGL gate result: PASS`

### Notes
- Verified parser output for transition timing, accel, property masks, property names, and target style payloads.
- Verified `\t(...)` payloads with nested `\clip(...)` arguments now parse correctly instead of being truncated by naive parenthesis scanning.
- Verified transition evaluation is active in both OpenGL and D3D11 subtitle paths, not only in parser diagnostics.
- The OpenGL gate now includes a dedicated transition regression step and also includes the transition sample in the ASS sample suite.

## 2026-03-25 OpenGL bottom-bar player chrome regression

### Commands
```powershell
cmake --build build --config Release --target modern-video-player

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- Release build: PASS
- OpenGL gate: `OpenGL gate result: PASS`

### Notes
- OpenGL bottom controls now use a formal bottom-bar layout instead of the previous thin OSD rails.
- Added a clickable play/pause button, segmented current/total time text, and enlarged hit zones for seek and volume interaction.
- The OpenGL bar now stays visible while hovered or dragged and fades out after idle playback.
- Automated checks do not synthesize pointer interaction, so manual GUI smoke is still recommended:
  - `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe .\a.mp4`
  - Verify play/pause button click, progress drag, volume drag, bottom-bar hover keep-visible, and idle auto-hide timing.

## 2026-03-25 Embedded subtitle-track regression

### Commands
```powershell
ffmpeg -y `
  -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 `
  -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass `
  -map 0:v -map 0:a? -map 1:0 `
  -c:v copy -c:a copy -c:s ass `
  .\build\tmp\embedded-ass-validation.mkv

.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv

powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

### Results
- Generated embedded ASS sample PASS
- `embedded-subtitle-check.result=PASS` on `embedded-ass-validation.mkv`
- Gate-generated embedded text subtitle sample (`mov_text`) PASS
- OpenGL embedded ASS playback regression PASS
- OpenGL embedded text subtitle playback regression PASS
- OpenGL gate: `OpenGL gate result: PASS`

### Notes
- Embedded subtitle auto-load currently targets supported text subtitle codecs and reuses the same subtitle timeline model as external subtitles.
- `PlayerCore` now keeps separate embedded/external subtitle stores with `external > embedded` precedence, so clearing a sidecar subtitle falls back to the embedded track.
- The OpenGL gate now covers 16 checks, including embedded ASS and embedded `mov_text` playback.
