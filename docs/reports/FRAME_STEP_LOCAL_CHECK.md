# 本地验收：帧步进（暂停态）

- 日期：`2026-03-08`
- 目标：验证 `4.4 帧步进（暂停态）`
- 样本：`./juren-30s.mp4`

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --frame-step-check .\juren-30s.mp4
```

## 输出摘要

```text
frame-step-check.path=.\juren-30s.mp4
frame-step-check.open_ok=true
frame-step-check.entered_playback_loop=true
frame-step-check.paused_before_step=true
frame-step-check.forward_invoked=true
frame-step-check.backward_invoked=true
frame-step-check.paused_after_forward=true
frame-step-check.paused_after_backward=true
frame-step-check.before=0.725333
frame-step-check.after_forward=0.834168
frame-step-check.after_backward=0.792459
frame-step-check.moved_forward=true
frame-step-check.moved_backward=true
frame-step-check.result=PASS
```

## 结论

- `PASS`：暂停后可执行单帧前进/后退，步进后仍保持暂停状态。
- 本次验收同时验证了默认 `,` / `.` 热键对应的主链能力已接通到 `PlayerCore`。
