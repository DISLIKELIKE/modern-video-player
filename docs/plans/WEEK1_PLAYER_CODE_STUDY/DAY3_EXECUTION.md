# Day 3 执行版（解码/渲染性能瓶颈专项）

日期：2026-03-12  
目标：定位“卡顿、掉帧、CPU/GPU开销高”的主要瓶颈点，形成可执行优化清单。

---

## 1. 今日执行安排（8 小时）

1. `09:00-09:30` 回看 Day2 结论，锁定今天只做“性能链路”，不做功能扩展。
2. `09:30-10:30` 建立基线：运行性能验收命令，记录一份“当前性能快照”。
3. `10:30-11:30` 分析视频解码链路：`decodeVideoFrame -> prepareVideoOutputFrame -> convertVideoFrameToYuv420`。
4. `11:30-12:00` 分析硬解路径：`D3D11VA` 是否发生 `av_hwframe_transfer_data` 回拷。
5. `13:30-14:30` 分析渲染链路：`copyFrameData -> SDL_UpdateYUVTexture` 的每帧复制成本。
6. `14:30-15:20` 分析调度策略：背压、短等待、迟到丢帧阈值与队列填充率关系。
7. `15:20-16:00` 分析音频链路：`AudioPlayer::play/audioCallback/getPlaybackPts` 对主时钟与稳定性的影响。
8. `16:00-17:20` 输出瓶颈地图、优先级优化清单、Day4 验证计划。

---

## 2. 今日建议命令（先采样再结论）

```powershell
# 基础性能日志（命令参数以 docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md 为准）
.\modern-video-player.exe --performance-log-check <media_file> 5000

# 1080p60 稳定性
.\modern-video-player.exe --1080p60-check <media_file> 5000

# 4K 与高码率场景
.\modern-video-player.exe --4k-playback-check <media_file> 5000
.\modern-video-player.exe --high-bitrate-check <media_file> 5000
```

---

## 3. 明日必须产出

1. 一张“性能瓶颈地图”（解复用 / 解码 / 转换 / 渲染 / 音频 / 同步）。
2. 一张“证据矩阵表”（指标、现象、代码位置、结论）。
3. 一份优化 Backlog（P0/P1/P2），每条必须可执行可验证。

---

## 4. 重点代码定位（仅性能相关）

- 视频解码与转换：
  - `src/core/player_core.cpp`  
  - `decodeVideoFrame()` / `prepareVideoOutputFrame()` / `convertVideoFrameToYuv420()`
- 硬解路径：
  - `tryConfigureD3D11HardwareDecode()` / `av_hwframe_transfer_data()`
- 渲染上传：
  - `src/display.cpp`  
  - `copyFrameData()` / `SDL_UpdateYUVTexture()`
- 调度与同步：
  - `src/core/scheduler.cpp`  
  - `videoDecoderLoop()` / `audioDecoderLoop()` / `pumpRenderOnce()`
- 音频时钟：
  - `src/audio_player.cpp`  
  - `play()` / `audioCallback()` / `getPlaybackPts()`

---

## 5. 复盘模板（直接复制）

```markdown
## 瓶颈点：

## 证据：
- 指标：
- 日志：
- 代码位置：

## 结论：

## 优化方案（只写可落地）：

## 验证方式：
```

---

## 6. Day3 验收标准

- 能说明当前瓶颈是否主要在“硬解回拷 + 纹理上传”。
- 能说明哪些卡顿来自“同步策略”而非“解码能力不足”。
- 能给出至少 5 条可执行优化项，并标出优先级与收益预期。
