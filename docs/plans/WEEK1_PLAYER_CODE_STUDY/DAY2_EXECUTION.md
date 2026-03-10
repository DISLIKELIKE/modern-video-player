# Day 2 执行版（Seek / Flush / 状态切换专项）

日期：2026-03-11  
目标：把 Day1 的 P0 问题打穿，形成“可解释 + 可复现 + 可验证”的结论。

---

## 1. 今日执行安排（8 小时）

1. `09:00-09:30` 回看 Day1 产物，明确今天只做 P0，不扩散。
2. `09:30-10:30` 专看 `PlayerCore::seek()`，梳理状态迁移和线程停启顺序。
3. `10:30-11:20` 专看 `flushPipelines()` + `avcodec_flush_buffers()` + resampler reset。
4. `11:20-12:00` 专看 `startDemuxThread()/stopDemuxThread()` 的 stop/start 语义。
5. `13:30-14:20` 专看 `Scheduler::pumpRenderOnce()` 的时钟差值处理和阈值。
6. `14:20-15:10` 专看 `AudioPlayer::getPlaybackPts()` 如何驱动主时钟。
7. `15:10-16:00` 画“seek 前后”时序图，标出清空点、恢复点、回调点。
8. `16:00-17:20` 输出 Day2 结论文档（每个 P0 问题给出：结论、证据、未决项）。

---

## 2. 明日必须产出

1. 一张 `seek` 时序图（含暂停、flush、复位、恢复）。
2. 一张“线程状态切换表”（Playing / Paused / Stopped）。
3. 一份 P0 问题闭环表（至少 5 条，必须附代码位置）。

---

## 3. Day2 推荐阅读顺序（严格）

1. `PlayerCore::seek()`  
2. `flushPipelines()`  
3. `stopDemuxThread()` / `startDemuxThread()`  
4. `Scheduler::pause/resume/pumpRenderOnce()`  
5. `AudioPlayer::getPlaybackPts()` + `startAudioConsumer()`

不要跳到字幕、滤镜、播放列表；这些留到 Day3/Day4。

---

## 4. 复盘模板（直接复制）

```markdown
## 问题：

## 结论（1-2 句）：

## 证据（文件:行号）：
- 

## 风险/边界：

## 明天是否继续追踪：
```

---

## 5. Day2 验收标准

- 你能解释 `seek` 顺序每一步“为什么不能换顺序”。
- 你能解释 `pumpRenderOnce()` 为什么既要等待也要丢帧。
- 你能解释“音频主时钟”与 `position_` 的一致性来源。
- 你能用 5 分钟白板讲清楚 Day1 的 5 个 P0 问题。
