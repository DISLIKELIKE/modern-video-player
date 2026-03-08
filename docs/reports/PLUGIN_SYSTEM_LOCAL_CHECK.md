# 本地验收：插件系统

- 日期：`2026-03-08`
- 目标：验证 `7.1 插件系统`
- 宿主命令：`./build/Debug/modern-video-player.exe --plugin-check`
- 插件目录：`./build/Debug/plugins`
- 示例插件：`sample_logger_plugin.dll`

## 执行命令

```powershell
.\build\Debug\modern-video-player.exe --plugin-check
```

## 输出摘要

```text
plugin-check.path=D:/C++files/VSProjects/modern-video-player/modern-video-player/build/Debug/plugins
plugin-check.loaded_count=1
plugin-check.plugin_ids=sample_logger_plugin@0.1.0
plugin-check.video_filters_before=0
plugin-check.video_filters_loaded=1
plugin-check.video_filters_unloaded=0
plugin-check.audio_filters_before=0
plugin-check.audio_filters_loaded=0
plugin-check.audio_filters_unloaded=0
plugin-check.sample_plugin_loaded=true
plugin-check.sample_video_filter_registered=true
plugin-check.sample_video_filter_unloaded=true
plugin-check.errors=none
plugin-check.result=PASS
```

## 结论

- `PASS`：宿主可从默认 `plugins` 目录动态加载示例 `DLL`，并完成 `API` 版本校验与生命周期回调。
- 示例插件注册的 `sample_identity` 视频滤镜在加载后可见、卸载后已被清理，说明扩展点闭环成立。
- 至此，任务清单中的 `7.1 插件系统` 已从“骨架”进入“可运行的最小闭环”。
