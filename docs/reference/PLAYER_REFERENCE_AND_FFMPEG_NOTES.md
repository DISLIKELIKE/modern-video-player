# 播放能力实现参考（格式 / 高分高帧 / 多音道）

更新日期：2026-03-08

## 状态说明（2026-03-08）

- 本文档聚焦“格式支持 / 高分高帧 / 多音道”相关实现参考，不是当前项目全部功能的总进度面板。
- 其中“主力能力目标（建议）”更适合作为工程目标基线阅读，不应直接等同于“尚未完成项”列表。
- 截至 `2026-03-08`，与本主题相邻的多项能力已在其他文档中持续同步，例如 D3D11VA 回退、D3D11 渲染、章节导航、A-B Repeat、截图等；当前总进度请优先参考 `docs/analysis/MPC_HC_GAP_ANALYSIS.md`、`docs/records/VERSION.md` 与 `docs/records/CHANGELOG.md`。

## 1. 本轮已落地内容

### 1.1 格式与能力探测

- 新增运行时能力查询：`FormatSupport::queryRuntimeCapabilities()`
- 新增编解码“可能支持”判断：
  - `isVideoCodecLikelySupported()`
  - `isAudioCodecLikelySupported()`
- 新增播放目标评估：`evaluatePlaybackTarget()`，用于判定高分辨率/高帧率目标是否建议硬解与 D3D11 渲染

对应代码：
- `include/media/format_support.h`
- `src/media/format_support.cpp`

### 1.2 解复用稳健性增强

- 打开输入时增加探测参数（`probesize`、`analyzeduration`、`+genpts`）
- 流选择改为 `av_find_best_stream`，并保留兜底扫描
- 补充分辨率、帧率、采样率、声道日志输出

对应代码：
- `include/demuxer.h`
- `src/demuxer.cpp`

### 1.3 多音道与音频输出一致性

- `AudioPlayer` 公开实际输出参数（采样率/声道/格式）
- SDL 音频设备允许频率与声道协商，保持输出格式为 `S16`
- `PlayerCore` 改为按“实际输出参数”进行重采样
- `SwrContext` 改为复用（参数变化时重建），不再每帧创建销毁

对应代码：
- `include/audio_player.h`
- `src/audio_player.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`

### 1.4 高分高帧解码基础

- 视频解码器启用线程参数（自动线程 + frame/slice 线程能力）
- 音频重采样路径在 seek 后重置，避免时间轴漂移和延迟积累
- Windows 下新增 D3D11VA 硬解最小闭环：优先尝试硬解，失败自动回退软解
- 对硬件帧/非 YUV420 帧增加 `hwframe_transfer + swscale` 转换，统一输出可渲染的 `YUV420P`

## 2. 主力能力目标（建议）

### 2.1 格式覆盖目标

容器优先级：
- `mp4` `mkv` `mov` `avi` `webm` `ts` `m2ts` `flv`

视频编码优先级：
- `H.264/AVC` `H.265/HEVC` `VP9` `AV1` `MPEG-2`

音频编码优先级：
- `AAC` `MP3` `AC3` `E-AC3` `DTS` `FLAC` `Opus` `Vorbis` `PCM`

### 2.2 性能目标

- `1080p60` 长时间稳定播放
- `4K` 可播放（优先 `4K30` 稳定，再推进 `4K60`）
- 多音道（`5.1/7.1`）输出链路稳定，音频时钟不漂移

## 3. FFmpeg 借鉴点（可直接对照实现）

### 3.1 流选择与解复用

- 使用 `av_find_best_stream` 选择主视频/主音频流，而非“第一个流即选中”
- 对复杂封装文件提高探测阈值，减少“无流信息/时间戳异常”问题

### 3.2 运行时能力枚举

- 用 `av_demuxer_iterate` 枚举容器能力
- 用 `av_codec_iterate` + `av_codec_is_decoder` 枚举解码器能力

### 3.3 高分高帧稳定性

- 设置 `AVCodecContext::thread_count`、`thread_type`
- 高码率场景优先硬解与 D3D11 路径，保留软解回退

### 3.4 多音道音频链路

- 解码输出与设备输出解耦，通过 `libswresample` 做统一重采样/重排
- 重采样器复用，避免每帧频繁初始化导致抖动

## 4. 可参考的开源项目

1. FFmpeg（含 ffplay）
   - 仓库：https://github.com/FFmpeg/FFmpeg
   - 借鉴点：`fftools/ffplay.c` 的播放主链、同步、队列与容错
2. mpv
   - 仓库：https://github.com/mpv-player/mpv
   - 借鉴点：高质量时钟同步、复杂格式兼容、硬解与渲染后端抽象
3. VLC
   - 仓库：https://github.com/videolan/vlc
   - 借鉴点：跨平台模块化架构、解封装/解码/输出分层
4. MPC-BE
   - 仓库：https://github.com/Aleksoid1978/MPC-BE
   - 借鉴点：Windows 本地播放器体验、快捷键与渲染链路组织
5. IINA
   - 仓库：https://github.com/iina/iina
   - 借鉴点：播放器内核能力向桌面交互层的工程化封装

## 5. 官方文档入口（建议固定保存）

- FFmpeg 文档入口：https://ffmpeg.org/documentation.html
- FFmpeg Doxygen 示例入口：https://ffmpeg.org/doxygen/trunk/examples.html
- `av_find_best_stream` 参考：https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html
- `av_codec_iterate` 参考：https://ffmpeg.org/doxygen/trunk/group__lavc__core.html

## 6. 命令行快速验证（已实现）

在构建输出目录运行：

```powershell
# 输出运行时容器/编解码器能力与主力格式覆盖矩阵
.\modern-video-player.exe --capabilities

# 评估播放目标（示例：4K60 + 6 声道 + 80Mbps）
.\modern-video-player.exe --evaluate-target 3840 2160 60 6 80
```
