# Day13 结论：software video decode 保守线程配置仍不产帧，现象进一步收敛到“首个视频包后停住”；同时补修 `SdlVideoRenderer` 注释乱码

日期：2026-03-19  
范围：`src/core/player_core.cpp`、`src/render/sdl_video_renderer.cpp`

## implementation planner

1. 在不动默认 `D3D11 + D3D11VA` 主链的前提下，先把 software video decode 线程配置收紧到 `thread_count=1 / thread_type=none`，排除当前 scheduler/decode 模型与 FFmpeg 激进多线程的交互噪声。
2. 复跑 `--software-video-decode-check`，确认 software path 是否已经从“打开成功但 0 帧输出”变成“真实产帧”。
3. 结合新增日志与运行期诊断，判断 software decode 是卡在 `receive_frame()` 之后，还是在首个视频包提交阶段就已经停住。
4. 顺手清理当前代码里确认存在的注释乱码，避免编码问题继续污染阅读和后续 diff。

## 关键结论

### 1. 保守线程配置没有解除 software decode blocker

本轮构建命令：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `0 warnings / 0 errors`

本轮专项命令：

```powershell
.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000
```

关键输出：

- `renderer_backend=SoftwareSDL`
- `decoder_backend=Software`
- `Video decoder threading: backend=Software thread_count=1 thread_type=none`
- `decode_video_ok=0`
- `scheduler_video_decoded_frames=0`
- `render_frames=0`
- `video_frame_queue_peak_size=0`
- `video_copy_back_frames=0`
- `video_swscale_frames=0`
- `result=FAIL`

结论：

- software path 的单线程保守配置已经实际生效。
- 但 software video decode 仍然没有形成任何有效视频帧输出。
- 因此当前 blocker 不能再归因为“FFmpeg software decode 开了激进线程导致 scheduler 交互异常”；至少在 `thread_count=1 / thread_type=none` 下它依旧存在。

### 2. 从日志推断，software decode 现在更像是“首个视频包后停住”

本轮 2 秒窗口内，控制台诊断日志出现了下面的关键片段：

```text
Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808
[diag:audio-backpressure] demux(v=134,a=411,...) pkt_q(v=133,a=256) dec(core v=0,a=155,...)
[diag:audio-backpressure] demux(v=163,a=501,...) pkt_q(v=162,a=256) dec(core v=0,a=245,...)
```

这里可以做一个明确推断：

- 2 秒内 demux 总共推入了 `163` 个视频包；
- 同时 `pkt_q(v=162)` 说明视频包队列里仍残留 `162` 个；
- 也就是说，video decode 线程只真正消费了 1 个视频包；
- 并且日志只出现了 `first send_packet start`，没有出现 `first send_packet returned`。

因此这轮最稳的判断是：

- 问题已经不再只是“收不到 decoded frame”；
- 更可能是 software video decode 在线程里卡在首个视频包提交阶段，或者刚提交完首包就没有再形成任何后续推进。

这条结论是基于本地运行日志做出的推断，不是直接来自单个原子计数器。

### 3. 当前 blocker 仍然与 copy-back / swscale / display copy 无关

- 本轮专项命令仍然保持：
  - `video_copy_back_frames=0`
  - `video_swscale_frames=0`
  - `render_frames=0`
- 这说明软件视频解码链甚至还没走到 copy-back 或软件显示上传那一步。
- 因而“继续减少或规避 `av_hwframe_transfer_data()` copy-back”这条主目标依旧成立，但 software decode 这个独立 blocker 仍需单独解决，不能把它和当前硬解 fallback copy-back 热点混在一起。

### 4. 代码注释乱码已继续清理到 `SdlVideoRenderer`

本轮补修：

- `src/render/sdl_video_renderer.cpp`
  - 修复 9 处函数头注释乱码，仅改中文注释，不改逻辑。

补修后再次对 `src/`、`include/` 做了基于注释行的乱码扫描：

- `/// ...`
- `// ...`

本轮扫描未再发现新的可疑乱码注释命中。

## 本轮代码与文档收敛

### 代码

- `src/render/sdl_video_renderer.cpp`
  - 修复注释乱码，不改运行时行为。

### 文档

- 新增：
  - `docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md`
- 更新：
  - `docs/records/CHANGELOG.md`
  - `docs/records/DEVELOP_LOG.md`
  - `docs/records/VERSION.md`

## 下一步建议

1. 下一轮不要再优先动 `SoftwareSDL` 渲染侧；当前更值得看的是 software decode 首包提交阶段为什么停住。
2. 继续补最小化诊断时，优先考虑把“视频包 dequeue 次数 / send 成功次数 / send 返回码”拆成独立计数，而不是继续追加大段日志。
3. 对照 `ffplay / mpv` 时，重点先核对：
   - software path `AVCodecContext` 初始化参数
   - `avcodec_send_packet()` 前后的线程与锁使用方式
   - 当前 H.264 样本首包进入 decoder 后是否还缺少 parser/drain 语义上的某个前置条件
