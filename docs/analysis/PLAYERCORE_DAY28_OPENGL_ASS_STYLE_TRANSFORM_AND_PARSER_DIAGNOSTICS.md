# PLAYERCORE DAY28: OpenGL ASS Style Transform And Parser Diagnostics

**日期**: 2026-03-24

## 背景
- 当前 OpenGL 播放链已经具备：
  - `D3D11VA -> OpenGL` 原生直通
  - 启动期 `OpenGL/D3D11` 诊断
  - 基础 `ASS/SSA` 文本、定位、描边、阴影、背景框
- 与成熟播放器的主要差距，已经从“播不出来”转成“ASS 语义覆盖不够”。
- 之前项目内最大的空洞不是视频显示，而是 ASS 的样式字段没有完整进入 GPU 字幕渲染。

## 本轮目标
- 扩展 `SubtitleStyle`，补齐能直接提升 OpenGL/D3D11 观感的 ASS 样式字段。
- 让 `ASS parser -> SubtitleItem/Run -> DirectWrite/D2D subtitle renderer` 形成完整闭环。
- 增加机器可读的 `--subtitle-style-check`，降低后续排障成本。

## 本轮实现
### 1. SubtitleStyle 扩展
- 在 `include/subtitle/subtitle_parser.h` 中新增：
  - `outline_x / outline_y`
  - `shadow_x / shadow_y`
  - `wrap_style`
  - `spacing`
  - `scale_x_percent / scale_y_percent`
  - `rotation_degrees`
- 同步更新 `src/subtitle/subtitle_parser.cpp` 的结构比较逻辑。

### 2. ASS/SSA 解析增强
- `src/subtitle/ass_parser.cpp` 新增解析：
  - Script Info:
    - `WrapStyle`
  - Style fields:
    - `ScaleX`
    - `ScaleY`
    - `Spacing`
    - `Angle`
  - Override tags:
    - `\q`
    - `\fsp`
    - `\fscx`
    - `\fscy`
    - `\fr`
    - `\frz`
    - `\xbord`
    - `\ybord`
    - `\xshad`
    - `\yshad`
- `\bord` / `\shad` 现在会同步更新 `x/y` 轴向字段，避免 renderer 再自行猜测。

### 3. OpenGL / D3D11 字幕渲染对齐
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`

两条渲染链本轮统一补齐：
- `WrapStyle -> DWRITE_WORD_WRAPPING`
- `Spacing -> IDWriteTextLayout1::SetCharacterSpacing`
- `ScaleX/ScaleY -> D2D transform`
- `Rotation -> D2D rotation transform`
- `X/Y outline -> 各向异性描边偏移`
- `X/Y shadow -> 轴向阴影偏移`

实现策略保持务实：
- 继续复用现有 `DirectWrite + D2D` 离屏字幕路径
- 不引入新的字幕渲染后端
- 优先把常见 ASS 样式真正显示出来，而不是一开始就追 `karaoke/vector/clip`

## 新增诊断与样本
### CLI
- `src/main.cpp`
- 新增：
  - `--subtitle-style-check <subtitle.(srt|ass|ssa)>`

输出内容为机器可读的 `key=value`：
- item 层：
  - 时间、层级、文本、分辨率、run 数量
- style 层：
  - wrap / spacing / scale / rotation / x-y outline / x-y shadow / position
- run 层：
  - range + style

### 样本
- `samples/subtitles/opengl_ass_style_validation.ass`
- 覆盖：
  - `WrapStyle`
  - `\q`
  - `\fsp`
  - `\fscx/\fscy`
  - `\frz`
  - `\xbord/\ybord`
  - `\xshad/\yshad`
  - `BorderStyle=3`
  - 多种 `\an + \pos`

## 本地验证
### 解析验证
```powershell
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
```

结果：
- `subtitle-style-check.entries=5`
- `subtitle-style-check.result=PASS`
- `subtitle-sync-check.mismatches=0`
- `subtitle-sync-check.result=PASS`

### OpenGL 链路验证
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_style_validation.ass
Remove-Item Env:MVP_RENDERER_BACKEND
```

结果：
- `delay-adjust-check.subtitle_loaded=true`
- `delay-adjust-check.subtitle_entries=5`
- `delay-adjust-check.result=PASS`

### OpenGL 启动期能力确认
```powershell
.\build\Release\modern-video-player.exe --opengl-diagnostics
```

结果：
- `opengl-diagnostics.probe_succeeded=true`
- `opengl-diagnostics.native_interop.allowed=true`
- `opengl-diagnostics.result=PASS`

## 与成熟播放器的差距收敛情况
### 已明显缩小的部分
- 常见 ASS 定位与样式字段终于不是“解析后丢失”
- OpenGL 与 D3D11 两套 GPU renderer 的字幕语义更一致
- 出现 OpenGL/ASS 问题时，不再只能靠肉眼排查，已有 `--subtitle-style-check`

### 仍存在的主要差距
- 还不是 `libass` 等价实现，未完成：
  - `karaoke`
  - `vector drawing (\p)`
  - `clip/iclip`
  - 更完整的字体回退/附件字体
  - 逐 run 颜色/复杂 drawing effect 的完全等价
- HDR/色彩管理仍停留在 renderer 内部 aware 阶段，尚未进入显示级输出管理。

## 结论
- 这一轮之后，OpenGL 链路与成熟播放器的差距，已经从“底层播放能力”进一步收敛到“高级 ASS 特效与显示级色彩管理”。
- 就当前项目架构而言，这是一轮高性价比推进：不重写后端，但明显提升了 OpenGL/D3D11 的字幕成熟度与可诊断性。
