# Day7 结论：高码率场景下先不要大改播放主链，先把“音频设备失败的视频-only降级”和“回归门禁误判”收口

日期：2026-03-19  
范围：`src/core/player_core.cpp`、`include/core/player_core.h`、`src/main.cpp`、`docs/records/*`

## implementation planner

1. 先复核 `PlayerCore::open()`、`initDecoders()`、`ClockSource` 选择和各类 `--xxx-check` 的判定条件。
2. 把“音频设备初始化失败”从隐式副作用改成显式分支：
   - 视频存在时允许降级为 video-only
   - 音频-only 文件仍然直接失败
3. 把无音频输出时的主时钟从 `System` 改为 `Video`，避免纯视频推进依赖系统时钟漂移。
4. 把高码率/长时间/1080p60 回归的 demux drop 门禁改成只看 `demux_queue_drop_packets`，不再把 ignored audio packets 误判成回压丢包。
5. 补充 diagnostics 输出，让回归结果直接暴露 `audio_output_initialized / video_only_fallback / clock_source`。

## 结论

- 当前仓库已经具备 D3D11 原生零拷贝直通路径，这轮高码率不稳定的主问题不在“完全没有 zero-copy”，而在播放策略和回归判定混杂。
- 这次不需要立即做大重构。更合理的顺序是：
  - 先把“音频设备不可用”的降级语义做对
  - 再把回归门禁改成真正反映队列背压
  - 最后再看是否需要引入更重的 queue serial / frame serial / renderer capability negotiation

## 本轮落地

### 1. 显式 video-only 降级

- `PlayerCore::open()` 现在按流类型区分：
  - 有视频流时，音频设备初始化失败只记 warning，并继续播放视频
  - 没有视频流时，音频设备初始化失败仍然是致命错误
- 这和 ffplay/mpv/MPC-HC 一类播放器的处理方式一致：音频设备不可用时，视频文件尽量继续播，音频-only 资源不能伪成功。

### 2. 无音频输出时切到 Video clock

- 旧逻辑在没有音频设备时回退到 `System` clock。
- 新逻辑改成：
  - 有音频输出：`Audio`
  - 无音频输出但有视频：`Video`
  - 都没有时才是 `System`
- 这样高码率视频在 video-only 模式下由真实渲染 PTS 驱动位置推进，更接近 ffplay/mpv 的主时钟思路。

### 3. 回归门禁纠偏

- `1080p60-check`
- `high-bitrate-check`
- `long-playback-check`

以上三个检查现在都改成以 `demux_queue_drop_packets == 0` 作为 demux 背压失败条件。

- `demux_dropped_packets` 继续保留打印，但只作为总量观察值。
- `demux_ignored_packets` 和 `demux_queue_drop_packets` 会一起输出，便于立刻区分：
  - 被禁用流的包被忽略
  - 真正因为队列背压而丢包

### 4. diagnostics 直接暴露当前播放模式

- `audio_output_initialized`
- `video_only_fallback`
- `clock_source`

现在 `--performance-log-check` 和播放类稳定性检查会直接打印这些字段，避免再靠日志推断当前是音频主时钟还是 video-only 降级。

## 本轮验证

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.vcxproj `
  /t:Build /p:Configuration=Debug /p:Platform=x64 /m

.\build\Debug\modern-video-player.exe --1080p60-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000
.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000
.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000
.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 1200
```

结果：

- `1080p60-check`: PASS
- `high-bitrate-check`: PASS
- `long-playback-check`: PASS
- `performance-log-check`: PASS

共同观察：

- 本机 `WASAPI` 音频设备仍然不可用
- 但播放已稳定降级到 `video_only_fallback=true`
- `clock_source=Video`
- `demux_queue_drop_packets=0`

## 剩余风险

- `4k-playback-check` 仍然 FAIL，但失败点已经收敛到 `fallback_ok=false` 的子进程路径，不再是 demux drop 误判。
- 如果下一轮继续优化，优先级建议是：
  1. 查清 `runBackendSessionSubprocess()` 的超时/退出码问题
  2. 为 demux / packet queue / frame queue 引入更稳定的 serial 语义
  3. 再评估 `FrameQueue` 容量和 copy-back 热点是否需要结构性重构
