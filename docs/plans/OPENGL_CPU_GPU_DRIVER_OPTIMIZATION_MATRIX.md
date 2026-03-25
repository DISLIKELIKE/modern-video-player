# OpenGL CPU / GPU / Driver Optimization Matrix

Date: 2026-03-25

## 目标
- 把 OpenGL 路径的优化思路从“遇到问题再临时调”整理成可落库的分层策略文档。
- 明确哪些优化属于 CPU 层、哪些属于 GPU 路径选择、哪些必须沉淀为 driver/adapter 规则。
- 给出一版可以直接用于发布默认值和现场排障的 NVIDIA / AMD / Intel 策略表与验证命令。

## 适用范围
- 当前文档只描述 Windows OpenGL 播放路径。
- 当前 OpenGL 主链路本质上是:
  - 解码: `D3D11VA` 或 software
  - 呈现: `OpenGL`
  - 视频上传子路径:
    - native interop: `D3D11 surface -> WGL_NV_DX_interop -> OpenGL`
    - copy-back: `D3D11 -> copyback -> OpenGL upload`
- 当前可控开关:
  - `MVP_RENDERER_BACKEND=opengl`
  - `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`
  - `MVP_OPENGL_PRESENT_MODE=auto|paced|immediate`

## 基本原则
1. 默认策略先保稳定, 再保性能。
2. CPU 优化按症状收敛, 不按厂商写死。
3. GPU 优化主要决定“native interop 还是 copy-back”以及 present 策略。
4. Driver 优化必须沉淀为规则表, 不要散落成临时 `if/else`。
5. 所有默认策略都必须能被 `--opengl-diagnostics` / `--performance-log-check` / `run_opengl_checks.ps1` 机器可读验证。

## 当前可观测信号

### 启动期诊断
- `--opengl-diagnostics`
- 重点字段:
  - `opengl-diagnostics.gl_vendor`
  - `opengl-diagnostics.gl_renderer`
  - `opengl-diagnostics.has_wgl_dx_interop`
  - `opengl-diagnostics.native_interop.allowed`
  - `opengl-diagnostics.native_interop.disable_rule`
  - `opengl-diagnostics.native_interop.hard_blocker_rule`
  - `opengl-diagnostics.native_interop.quirk_rule_name`
  - `opengl-diagnostics.d3d11.decoder_profiles.*`
  - `opengl-diagnostics.hdr_output.*`

### 运行期诊断
- `--performance-log-check <media_file> [sample_ms]`
- 重点字段:
  - `performance-log-check.renderer_backend`
  - `performance-log-check.decoder_backend`
  - `performance-log-check.video_native_output_frames`
  - `performance-log-check.video_copy_back_frames`
  - `performance-log-check.video_swscale_frames`
  - `performance-log-check.video_filter_blocked_native_frames`
  - `performance-log-check.renderer_opengl_native_interop_active`
  - `performance-log-check.renderer_opengl_native_interop_disable_events`
  - `performance-log-check.renderer_opengl_present_wait_timeouts`
  - `performance-log-check.renderer_opengl_present_mode_requested`
  - `performance-log-check.renderer_opengl_present_mode_active`
  - `performance-log-check.scheduler_late_drops`
  - `performance-log-check.result`

## 分层优化矩阵

| 层 | 关注点 | 当前默认策略 | 失效信号 | 主动作 | 观察命令 | 优先级 |
|---|---|---|---|---|---|---|
| CPU | 避免软件颜色转换 / swscale | 优先走 native interop, 次选 copy-back, 目标保持 `video_swscale_frames=0` | `video_swscale_frames>0` | 保证 native/copy-back 至少有一条可用, 不让高位深回落到 8bit swscale | `--performance-log-check` | P0 |
| CPU | 控制 copy-back 成本 | copy-back 作为稳定兜底, 但不应变成常态主路径 | `video_copy_back_frames` 长期高且 CPU 占用明显上升 | 先查 native 是否被 quirk / hard blocker 关闭, 再看是否必须继续 copy-back | `--opengl-diagnostics` + `--performance-log-check` | P0 |
| CPU | 降低渲染线程抖动 | `present()` 等待真实显示完成, 默认 `present_mode=auto` | `renderer_opengl_present_wait_timeouts>0` 或肉眼 stutter | 保持 paced; 只在现场排障时切到 `immediate` A/B | `--performance-log-check` | P0 |
| CPU | 输入/OSD/字幕 redraw 风暴 | 抑制 hotkey repeat, hover auto-hide, 避免无意义持续重绘 | 按键卡顿、拖动卡顿、`scheduler_late_drops` 持续上升 | 保持 OSD 非常驻高频重绘; 避免为 UI 牺牲播放主路径 | GUI smoke + `--performance-log-check` | P1 |
| CPU | 高位深 copy-back 保真 | copy-back 保持 `p010le/p016le`, 避免二次降级 | 10bit 样本出现 `video_swscale_frames>0` | 保持 10bit direct upload 路径 | `run_opengl_checks.ps1` | P0 |
| GPU | native interop 可用性 | `auto`, 让已验证稳定的平台直接走 native | `renderer_opengl_native_interop_active=false` 且本应支持 | 先看 hard blocker / quirk / env override, 再决定是否 force | `--opengl-diagnostics` | P0 |
| GPU | present pacing | 默认 `auto`, 优先拿到 `active=paced` | `active=immediate` 但并非人工指定 | 先排 SDL swap interval/driver 情况, 不要先怪解码 | `--performance-log-check` | P0 |
| GPU | HDR / 色彩能力探测 | 先 probe, 不假装已经完成显示级 HDR 输出 | `hdr_output.probe_succeeded=false` 或 `output_found=false` | 把问题归类为“环境不可见”还是“实现缺失” | `--opengl-diagnostics` | P1 |
| GPU | 解码 profile 能力确认 | 启动期导出 H.264/HEVC/VP9/AV1 profile 支持情况 | 编码格式和硬解能力不匹配 | 先确认 D3D11 decoder profile, 再决定样本/机型默认策略 | `--opengl-diagnostics` | P1 |
| Driver | 启动期 blacklist / quirk 收敛 | 把已知坏组合在启动期直接降级 | `disable_rule != none` 或 `quirk_rule_matched=true` | 扩 quirk 表, 不在 runtime 临时散判 | `--opengl-diagnostics` | P0 |
| Driver | software / virtual GL context 排除 | 对 GDI Generic / SwiftShader / llvmpipe / VMware / Parallels 直接禁 native | `gl_vendor/gl_renderer` 命中软件或虚拟栈 | 保持 native 禁用, 避免“能跑但不稳” | `--opengl-diagnostics` | P0 |
| Driver | AMD Radeon 特殊处理 | 当前默认通过 quirk 关闭 native interop | `quirk_rule_name=amd-radeon-native-interop` | 发布默认保持关闭; `force` 仅用于现场验证 | `--opengl-diagnostics` + A/B 命令 | P0 |

## 当前规则表落地状态

### hard blocker
- `missing-wgl-nv-dx-interop`

### quirk / blacklist
- `software-opengl-context`
- `google-swiftshader-opengl-context`
- `mesa-llvmpipe-opengl-context`
- `vmware-virtual-gpu-opengl-context`
- `parallels-virtual-gpu-opengl-context`
- `amd-radeon-native-interop`

## 默认发布策略

### 全局默认
- `MVP_RENDERER_BACKEND=opengl`
- `MVP_OPENGL_NATIVE_INTEROP=auto`
- `MVP_OPENGL_PRESENT_MODE=auto`

### 解释
- `native interop` 默认不是“强制开”, 而是“允许已知稳定的组合走 native, 已知坏组合自动降级”。
- `present mode` 默认不是“永远 immediate”, 而是“优先 paced, 打不开再退 immediate”。
- 真正的厂商差异主要体现在 `native interop` 默认结果, 不是 CPU 线程数这种逻辑。

## 按 NVIDIA / AMD / Intel 的默认策略表

| 厂商 | OpenGL 默认策略 | Native Interop 默认 | Present 默认 | 发布建议 | 预期关键输出 |
|---|---|---|---|---|---|
| NVIDIA | 性能优先, 但仍保留 copy-back 兜底 | `auto`, 期望允许并实际启用 native | `auto`, 期望落到 `paced` | 可把 OpenGL 作为可用主链路之一 | `disable_rule=none`, `renderer_opengl_native_interop_active=true`, `video_copy_back_frames=0`, `present_mode_active=paced` |
| AMD | 稳定优先 | `auto`, 但当前规则会命中 `amd-radeon-native-interop` 并默认关闭 native | `auto`, 仍优先 paced | 发布默认不要 `force`; 先走 copy-back 稳定链路 | `quirk_rule_name=amd-radeon-native-interop`, `allowed=false`, `renderer_opengl_native_interop_active=false`, `video_copy_back_frames>0` |
| Intel | 自动探测优先 | `auto`, 当前没有单独 Intel blacklist; 成功则走 native, 失败则走 hard blocker / copy-back | `auto`, 期望落到 `paced` | 不要预设“Intel 一定差”或“一定强制 copy-back”; 先看 probe | 典型好结果是 `disable_rule=none`; 若老驱动缺 WGL interop, 则 `hard_blocker_rule=missing-wgl-nv-dx-interop` |

## 分厂商验证命令

### NVIDIA 默认策略验证
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
Remove-Item Env:MVP_OPENGL_NATIVE_INTEROP -ErrorAction SilentlyContinue
Remove-Item Env:MVP_OPENGL_PRESENT_MODE -ErrorAction SilentlyContinue
.\build\Release\modern-video-player.exe --opengl-diagnostics

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

建议关注:
- baseline 期望 `opengl-diagnostics.native_interop.allowed=true`
- baseline 期望 `performance-log-check.renderer_opengl_native_interop_active=true`
- baseline 期望 `video_copy_back_frames=0`
- copy-back A/B 期望 `renderer_opengl_native_interop_active=false` 且 `result=PASS`

### AMD 默认策略验证
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
Remove-Item Env:MVP_OPENGL_NATIVE_INTEROP -ErrorAction SilentlyContinue
Remove-Item Env:MVP_OPENGL_PRESENT_MODE -ErrorAction SilentlyContinue
.\build\Release\modern-video-player.exe --opengl-diagnostics

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='force'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

建议关注:
- 默认诊断期望命中 `quirk_rule_name=amd-radeon-native-interop`
- 默认播放期望 `renderer_opengl_native_interop_active=false`
- 默认播放期望 `video_copy_back_frames>0`
- `force` 只用于现场验证:
  - 若 `force` 仍稳定且明显收益, 才考虑为该机型追加更细规则
  - 若 `force` 复现黑屏 / 卡顿 / disable event, 保持当前默认降级

### Intel 默认策略验证
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
Remove-Item Env:MVP_OPENGL_NATIVE_INTEROP -ErrorAction SilentlyContinue
Remove-Item Env:MVP_OPENGL_PRESENT_MODE -ErrorAction SilentlyContinue
.\build\Release\modern-video-player.exe --opengl-diagnostics

$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800

$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

建议关注:
- 新驱动正常时, 期望 `disable_rule=none`
- 若缺 `WGL_NV_DX_interop`, 期望看到 `hard_blocker_rule=missing-wgl-nv-dx-interop`
- 即使 auto 已走 copy-back, 显式 `disable` 也应保持 `result=PASS`

## 通用回归命令

### 1. 启动期能力总览
```powershell
.\build\Release\modern-video-player.exe --opengl-diagnostics
```

### 2. OpenGL 主链路回归
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

### 3. 强制 copy-back 回归
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_NATIVE_INTEROP='disable'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

### 4. immediate present A/B
```powershell
$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_PRESENT_MODE='immediate'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
```

### 5. 一键门禁
```powershell
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
```

## 现场排障判读表

| 现象 | 先看什么 | 常见结论 | 推荐动作 |
|---|---|---|---|
| 有声音无画面 | `opengl-diagnostics.native_interop.*` | native interop 被 blocker/quirk 命中, 或 surface 共享失败 | 先 `disable` 做 copy-back A/B |
| 只有 AMD 卡顿 | `quirk_rule_name`, `present_mode_active`, `disable_events` | 很可能正是当前已知 AMD native interop 风险区 | 发布默认维持 auto, 不要全局 force |
| 10bit 样本变卡且色彩不对 | `video_swscale_frames`, `path=software format=p010le` 日志 | 10bit 没保住, 走了不该走的 swscale | 先回归 10bit copy-back 检查 |
| 快捷键或 OSD 操作后掉帧 | `scheduler_late_drops`, GUI 行为 | 更像 redraw / interaction 压力, 不是 decode 崩 | 保持 OSD 自动隐藏, 不要强制常亮高频重绘 |
| 某机型 probe 正常但实播抖动 | `present_wait_timeouts`, native/copy-back A/B | 可能是 driver pacing 或 native interop 稳定性问题 | 做 `disable` 和 `immediate` 两组 A/B |

## 发布建议

### 现在可以作为默认策略写死的
- `present mode = auto`
- `native interop = auto`
- 对 software / virtual GL context 维持 native 禁用
- 对 AMD Radeon 维持 native interop 默认禁用

### 现在不要写死的
- 不要按 CPU 型号做硬编码策略
- 不要对 Intel 一刀切强制 copy-back
- 不要把 `immediate` 当成默认 present 模式
- 不要把 `force native interop` 作为面向用户的默认配置

## 后续需要继续沉淀的点
1. Intel 老驱动 / UHD / Iris 系列的实机 quirk 样本库。
2. NVIDIA 不同驱动分支在 `immediate` / `paced` 下的抖动画像。
3. AMD 在更细粒度 `device_id` / `subsystem_id` 上的可放行白名单, 而不是长期全量保守禁用。
4. 低端 CPU + 复杂 ASS 场景下的字幕/OSD redraw 压力画像。
5. 把最终稳定策略收敛到安装包默认值与发布说明里。
