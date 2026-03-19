# Day7 结论：4K 回归残留失败不是播放主链退化，而是 backend session probe 子进程退出策略错误

日期：2026-03-19  
范围：`src/main.cpp`、`docs/records/*`

## implementation planner

1. 先复核 `runBackendSessionSubprocess()`、`runWindowsBackendSessionCheck()` 和 `--4k-playback-check` 的 `fallback_ok` 判定。
2. 直接复跑 `--windows-backend-session-check hard/soft`，确认剩余失败到底是 timeout、崩溃，还是主链播放本身失败。
3. 把 backend probe 改成真正的一次性子进程路径，不再复用常规播放器退出收尾。
4. 重新验证 `hard/soft` 子进程退出码、`--windows-backend-check` 和 `--4k-playback-check`。

## 结论

- 上一轮剩余的 `4k-playback-check` FAIL，根因已经确认不是 4K 播放主链推进失败。
- 真正失败点在 `--windows-backend-session-check` 这条 probe 子进程路径：
  - `hard` 模式会在打印 PASS 后悬挂到超时
  - `soft` 模式会在打印 PASS 后以异常退出码结束
- 这说明问题出在 probe 进程的退出策略，而不是 `D3D11VA` / software decode 本身不能完成 4K 播放窗口验证。

## 分析结论

- `runWindowsBackendSessionCheck()` 本质上只是给父进程返回一组“是否能启动并进入播放窗口”的探针结果。
- 它不应该复用正常播放器的完整 teardown 路径，因为这个路径在 software decode + D3D11 renderer 的专用 probe 子进程里仍然可能卡死或触发异常退出。
- 更合理的做法是：
  - 让 probe 子进程打印完结构化结果
  - 显式 flush stdout/stderr
  - 直接用 Windows 进程级终止结束该子进程
- 这不会影响正常播放器运行时，只影响 `--windows-backend-session-check` 这条测试辅助命令。

## 本轮落地

- `src/main.cpp` 中的 `runWindowsBackendSessionCheck` 已改为专用的 `runWindowsBackendSessionCheckAndExit()`。
- 该路径现在会：
  - 正常完成 open/play/pump 的最小探针窗口
  - 打印 `open_ok / entered_playback_loop / renderer_backend / decoder_backend / mode_ok`
  - flush 输出
  - 在 Windows 下调用 `TerminateProcess(GetCurrentProcess(), code)` 立即退出子进程
- `main()` 中 `--windows-backend-session-check` 分支已改为直接进入这条专用退出路径。

## 本轮验证

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.vcxproj `
  /t:Build /p:Configuration=Debug /p:Platform=x64 /m

.\build\Debug\modern-video-player.exe --windows-backend-session-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv hard
.\build\Debug\modern-video-player.exe --windows-backend-session-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv soft
.\build\Debug\modern-video-player.exe --windows-backend-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv
.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000
```

结果：

- `hard` backend session: `PASS`, exit code `0`
- `soft` backend session: `PASS`, exit code `0`
- `windows-backend-check`: `PASS`
- `4k-playback-check`: `PASS`

## 影响边界

- 这是回归 harness / probe 进程层的修复，不是主播放链新的运行时依赖。
- 正常播放器播放、暂停、seek、stop 仍继续走常规 `VideoPlayer` / `PlayerCore` 生命周期。
- 后续如果继续做深层优化，优先级可以回到：
  1. `FrameQueue` 容量与背压策略
  2. copy-back / memcpy 热点占比量化
  3. scheduler serial / generation 语义补强
