# Day6 结论：先收口 `deferred stop`、`PacketQueue` 所有权、`Clock` 时间基准与 `Demuxer::open()` 自锁，再继续往播放状态机深挖

日期：2026-03-19  
范围：`include/core/player_core.h`、`include/core/scheduler.h`、`include/thread_safe_queue.h`、`src/core/player_core.cpp`、`src/core/scheduler.cpp`、`src/core/clock.cpp`、`src/demuxer.cpp`

## implementation planner

1. 先把 EOF 自动停播与 UI 停播路径的线程收口做完整，避免 render 线程内同步 `stop()` 导致自 `join`。
2. 把 `PacketQueue` 从原始 `AVPacket*` 改为 RAII 所有权模型，确保 `stop/seek/close/clear` 不再遗留压缩包生命周期漏洞。
3. 给 `Scheduler` 增加异步停机与旧 worker 回收能力，避免 replay / restart 时命中线程句柄残留。
4. 修正 `Clock` 的 system-clock `pause()/setSpeed()` 基准连续性，以及 `Demuxer::open()` 锁内重入 `close()` 的设计债。
5. 以 Windows Debug 全量重建验证这轮收口不引入新的 warning / error。

## 关键结论

### 1. 旧的 EOF/停播路径最大问题不是“状态没改”，而是“状态改了但线程没收”

- EOF 自动停播发生在 scheduler 的 render 线程内，旧实现不能直接同步 `stop()`，否则会触发线程自 `join`。
- 结果就是旧逻辑只能把 `state_` 改成 `Stopped`，但 demux / audio consumer / scheduler worker 还可能留在后台。
- 这种“半停机”对单次播放未必立刻可见，但 replay、seek、next/previous、quit 时会积累成稳定性风险。

### 2. `PacketQueue` 的原始指针所有权是实打实的运行时债

- 把 `AVPacket*` 塞进通用队列后，`clear()`、队列析构和异常退出路径都不会自动释放 FFmpeg 包。
- 这类问题不会表现成编译报错，但 stop / seek / close 命中频率很高，属于长期运行一定会踩的资源管理缺口。

### 3. `Clock` 与 `Demuxer` 的问题属于“低频但高代价”的基础设施错误

- `Clock::pause()` 和 `Clock::setSpeed()` 如果不先固化旧基准，纯视频或 system-clock 路径就会出现时间跳变。
- `Demuxer::open()` 在持锁状态下复用 `close()` 属于接口级自锁，一旦未来复用同一实例就会卡死。
- 这类问题不会被单条 happy path 掩盖，但会持续污染后续所有状态机和调度优化工作。

## 本轮落地

### 1. `PlayerCore` 引入 `deferred stop` + worker reap

- EOF 在 render 线程内不再直接同步 `stop()`，而是先发异步停机请求。
- 后续由安全线程执行真实的 `stop/join/flush`，避免线程自锁。
- `next/previous/quit` 等路径也统一回到完整停机收口，而不是只切状态位。

### 2. `PacketQueue` 回归 RAII 所有权

- `PacketQueue` 改为 `ThreadSafeQueue<std::unique_ptr<AVPacket, AvPacketDeleter>>`。
- `ThreadSafeQueue::push()` 同步补了延迟 move 能力，确保异步 handoff 仍然可用。
- 这使得剩余压缩包能随着队列清理和析构自动释放。

### 3. `Scheduler` / `Clock` / `Demuxer` 的基础设施债同步收口

- `Scheduler` 增加异步停机入口，并在重新 `start()` 前回收已经退出的旧 worker。
- `Clock` 修复了 system-clock 路径的暂停与变速时间基准更新逻辑。
- `Demuxer::open()` 改成在同一锁域里直接关闭旧输入，不再锁内重入 `close()`。

## 验证结果

命令：

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m
```

结果：

- `0 个警告`
- `0 个错误`

## 当前边界

- 这轮解决的是“停播收口与运行时设计债”，不是完整状态机重设计。
- `PlaybackState` 仍然只有 `Stopped / Playing / Paused` 三态；后续如果继续向 ffplay / mpv / MPC-HC 一类实现靠拢，仍应把：
  - 会话生命周期
  - seek / flush / drain
  - EOF / ended / closing
  - worker 线程状态
  明确拆成独立内核状态层。