# Modern Video Player 1.0.0-rc1

首个发布候选版已经具备主流 Windows 本地播放能力，当前建议面向 `Windows x64` 以 `D3D11 renderer + D3D11VA decode` 作为主力链路试用。

## 本版重点

- 修复 `D3D11` 黑屏问题，补齐 `D3D11VA` 可采样解码表面创建，当前主力机型上已恢复原生直采样零拷贝路径。
- 补齐 D3D11 启动期能力探测、driver/adapter 诊断日志、decoder profile 探测，以及独立 `--d3d11-diagnostics` CLI。
- 强化 seek/flush/failsession 收口，当前 `seek` 主链已明显顺滑，连续 seek、暂停态 seek、close/reopen 与 forced failsession 都已纳入回归门禁。
- Release 构建重新执行格式回归与长时播放检查，最新结果为 `17 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`。

## 已验证能力

- 主流本地容器/编码播放
- `D3D11VA + D3D11` 主路径播放
- 字幕、播放列表、章节导航、A-B Repeat、截图、帧步进、延迟调节、数字跳转
- HLS / DASH 流媒体与自适应码率基础链路
- seek、长时播放、renderer fallback、serial/failsession 回归

## 已知问题

- `software video decode` 运行态路径还有剩余收口工作，当前不阻塞 `D3D11VA` 主链 RC，但仍不建议直接宣称正式版完成。
- D3D11 driver quirk / blacklist 规则目前只有首版，后续还需要继续积累更多显卡/驱动组合的经验规则。
- 是否具备 `AV1` 硬解能力取决于当前 adapter/driver；本次 RC 不承诺所有机器都有 `AV1` 硬解。

## 建议发布物

- Git 标签：`v1.0.0-rc1`
- 对外版本名：`Modern Video Player 1.0.0-rc1`
- 安装包/压缩包名：`modern-video-player-1.0.0-rc1-windows-x64.zip`

## 建议命令

```powershell
.\build\Release\modern-video-player.exe --version
.\build\Release\modern-video-player.exe --d3d11-diagnostics
```

对应的 RC 发布准备与验证证据见：[V1_0_0_RC1_RELEASE_READINESS.md](./V1_0_0_RC1_RELEASE_READINESS.md)