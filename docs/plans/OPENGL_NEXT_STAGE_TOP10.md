# OpenGL 下一阶段 Top 10 任务表
日期: 2026-03-25

目标:
- 优先收敛 OpenGL 在真实机器上的可用性和稳定性。
- 再补高位深、HDR、诊断等成熟播放器常见能力。
- 每项任务都要求可验证, 不做只停留在代码层的“看起来更完整”。

## 优先级表

| P | 任务 | 目标 | 验证 |
|---|---|---|---|
| 1 | AMD native interop quirk/blacklist 自动降级 | 避免 Radeon 驱动上的 OpenGL native D3D11 interop 黑屏/卡顿/不稳定 | `--opengl-diagnostics` + AMD 实机播放 |
| 2 | OpenGL native interop 运行时诊断计数 | 让 present timeout、native interop 成功/失败次数进入机器可读输出 | `--performance-log-check` |
| 3 | P010/P016 软件上传链路 | 避免高位深视频回退后继续降成 8bit 路径 | HDR/10bit 样本 + 对照日志 |
| 4 | HDR 输出能力探测 | 区分“内部 HDR aware”与“显示级 HDR output” | `--opengl-diagnostics` |
| 5 | present pacing / vsync override | 允许现场排障切换 paced/immediate present 策略 | 环境变量 + `--performance-log-check` |
| 6 | OpenGL 压测与门禁脚本 | 把 native/copyback/10bit/字幕压力变成 PASS/FAIL 检查 | `tools/run_opengl_checks.ps1` |
| 7 | ASS/SSA 压力样本补强 | 用更复杂动画/clip/vector/karaoke 样本压字幕链 | `tools/run_opengl_checks.ps1` |
| 8 | driver/adapter 经验规则表扩展 | 让 quirk 规则可持续累计, 不再散落在临时判断里 | diagnostics 对照 |
| 9 | OpenGL HDR/色彩设计收敛 | 为后续 DXGI HDR bridge / ICC / LUT 铺链路 | 设计文档 + probe 输出 |
| 10 | OpenGL 对标成熟播放器差距清单 | 明确和 mpv / MPC-HC / VLC 的剩余差距 | 差距分析文档 |

## Progress (2026-03-25)
- Completed: 1. AMD native interop quirk/blacklist auto downgrade
- Completed: 2. OpenGL native interop runtime diagnostics counters
- Completed: 3. P010/P016 software upload path
- Completed: 4. HDR output capability probe
- Completed: 5. present pacing / vsync override
- Completed: 6. OpenGL stress / gate script
- Completed: 7. ASS/SSA stress sample bolster
- Completed: 8. driver/adapter quirk table expansion
- Completed: 9. HDR/color design convergence update
- Completed: 10. mature-player gap analysis

## 执行结论
- 这一轮任务表已经执行完。
- 后续不再是“下一阶段 Top 10 任务表”里的未完成项，而是新的长期 backlog:
  - 显示级 HDR present bridge
  - ICC / 3D LUT 输出级色彩管理
  - 更完整的 libass 语义和 attachment/font pipeline
  - 更大的 driver quirk 样本库

## Post-Top10 Follow-up (2026-03-25)
- Completed: ASS `\t(...)` transform transition parsing, diagnostics, runtime interpolation, and OpenGL gate coverage.
- Completed: container attachment font extraction, private registration, and cleanup diagnostics.
- Completed: embedded text subtitle-track playback with `external > embedded` ownership, `--embedded-subtitle-check`, and OpenGL gate coverage.
- Completed: embedded subtitle track selection baseline (`PlayerCore` multi-track catalog + OpenGL prev/next track UI + `--subtitle-track` / `--embedded-subtitle-list` / `--embedded-subtitle-select-check`).
- Completed: OpenGL output-color baseline (`MVP_OPENGL_HDR_OUTPUT_MODE` + `MVP_OPENGL_3DLUT_FILE`, output diagnostics fields, and `--opengl-output-color-check`).
- Completed: OpenGL interaction freeze stabilization by moving SDL event pumping/window fullscreen transition onto main-thread `handleEvents()` path.
- Remaining subtitle long-tail after this follow-up:
  - fuller libass shaping/layout parity
  - broader multi-track policy control (default-language preference, forced/SDH policy, persistence strategy)
  - live subtitle packet paths
  - bitmap subtitle renderer optimization (cache/reuse/upload policy and multi-rect composition tuning)
  - more advanced transform/effect semantics beyond the current transition field set
- Remaining color/HDR long-tail after this follow-up:
  - full display-level HDR present bridge (`DXGI swapchain + SetColorSpace1 + SetHDRMetaData`)
  - ICC/profile-driven LUT generation and per-display LUT selection/update strategy
