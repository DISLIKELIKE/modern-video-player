# MPC-HC 功能差距评估（截至 2026-03-08）

## 1. 评估口径

本评估用于回答“当前项目距离 MPC-HC 级别播放器还有多少功能未实现”。

评估口径：
- 只按“端到端可用”计为已实现（主流程可触达、可运行、可被用户使用）。
- 只有类/接口/占位代码但未接入主播放链路，计为“骨架/未接入”。
- 参考基线为桌面播放器常见能力集合（MPC-HC 同类能力），不是学习型最小播放器目标。
- 本次评估同步参考本地验收报告，而不是只看类/接口是否存在。

代码核对范围（关键入口）：
- `src/main.cpp`
- `src/video_player.cpp`
- `src/core/player_core.cpp`
- `src/display.cpp`
- `src/render/*`
- `src/subtitle/*`
- `src/playlist/*`
- `src/config/*`
- `src/input/*`
- `src/streaming/*`
- `src/plugin/*`
- `docs/reports/*`

## 2. 当前已具备的可用能力

当前项目已经不再只是“基础本地播放链路”，而是具备一套可连续使用的 Windows 本地播放器主路径：
- 本地媒体打开、解封装、音视频解码与播放。
- 播放控制：播放、暂停、停止、拖拽进度 seek、快捷键 seek、数字热键 `10%~90%` 跳转、音量、静音、倍速。
- 基础交互：全屏、控制条覆盖层、字幕显示、章节导航、A-B Repeat、截图、暂停态帧步进。
- 字幕：外挂 `SRT` 加载、渲染叠加、播放/暂停/seek 同步、字幕开关、字幕延迟调节。
- 播放列表：多文件参数 / `m3u8`、上一项/下一项、EOF 自动下一项、恢复上次索引。
- 设置：音量、倍速、音轨/字幕时延、硬解偏好、恢复播放列表索引、快捷键持久化。
- 快捷键：默认键位、增强键位（章节 / A-B / 截图 / 帧步进 / 时延调节 / 数字跳转）、持久化、冲突检测、恢复默认。
- 渲染/解码：`SDL` 主路径可用，`D3D11VA` 解码初始化与失败回退、`D3D11` 渲染最小可用与自动回退 `SDL` 已落地。
- 格式能力：已有探测入口、回归脚本与本地报告，可输出 `PASS/PARTIAL/FAIL`。
- 可观测性：新增 `--performance-log-check`，可输出 renderer / decoder backend、CPU 平均占用、队列长度与调度掉帧指标。
- 高分辨率门禁：`1080p60` 连续播放窗口本地验收已通过，`4K` 样本可播放并完成 hard/soft 降级验收。

## 3. 差距总览（按 14 个模块）

| 模块 | 当前状态 | 结论 |
|---|---|---|
| 01 核心播放引擎 | 本地播放主链、调度、基础控制、章节/A-B/截图已可用，但离 MPC-HC 的成熟度仍有差距 | 部分实现 |
| 02 滤镜系统 | 管道、注册表、内置亮度/对比度/饱和度/声道平衡已实现，但缺少用户侧控制入口 | 部分实现 |
| 03 视频渲染器 | `SDL` 可用，`D3D11` 最小链路与回退已可用，`OpenGL` 仍是占位 | 部分实现 |
| 04 音频渲染器 | `SDL` 音频可用，音轨时延调节已接入主链，但均衡器/混音/设备选择仍缺失 | 部分实现 |
| 05 字幕系统 | 外挂 `SRT` 已接入播放主链并可同步显示，但内嵌字幕/字幕轨切换/ASS 仍缺失 | 部分实现 |
| 06 播放列表 | 多文件与 `m3u8`、上一项/下一项、EOF 自动下一项、恢复索引已可用，但无 UI/历史/循环模式 | 部分实现 |
| 07 解码器管理 | `DecoderFactory` 已接入真实初始化，`D3D11VA` 与软件回退可运行，结构化性能日志已可导出，但可配置性仍偏基础 | 部分实现 |
| 08 文件格式支持 | 主力容器/编码矩阵已有回归结果，`1080p60` 稳定播放、`4K` 可播放并可降级、性能日志入口已补齐，但 `>80Mbps` 高码率门禁仍未完成 | 部分实现 |
| 09 流媒体支持 | `HLS/DASH` 解析器仅轻量实现，HTTP 下载器无真实读流/缓冲/ABR | 骨架/未接入 |
| 10 皮肤系统 | 只有主题变量容器，没有资源化主题与组件级样式接入 | 骨架/未接入 |
| 11 快捷键系统 | 默认键位、增强键位、持久化、冲突检测、恢复默认均已接入，但仍缺少更细粒度的用户侧配置体验 | 部分实现 |
| 12 设置系统 | 启动加载、退出保存、运行期应用已接入主流程，但覆盖项仍偏少 | 部分实现 |
| 13 插件系统 | 仅内存注册/启停状态，无动态加载、隔离、版本兼容与生命周期 | 骨架/未接入 |
| 14 视频增强 | 算法与管道存在，但没有形成可见、可调、可持久化的产品能力 | 部分实现 |

模块统计：
- 达到 MPC-HC 等级：`0 / 14`
- 部分实现：`11 / 14`
- 骨架或未接入：`3 / 14`

粗略结论：
- 如果只看“Windows 本地主播放链”，当前对齐度约 `60%` 左右。
- 如果按“MPC-HC 全量桌面播放器能力”评估，当前整体对齐度约 `45%~55%`。
- 尚未完成或未收敛的能力主要集中在：`>80Mbps` 高码率稳定性、轨道切换、完整渲染后端、流媒体、插件、皮肤。

## 4. 关键未实现功能清单

### P0（直接影响主力可用性/发布门禁）

- 大码率样本（`>80Mbps`）可播放并完成稳定性门禁。
- 多音轨切换、字幕轨切换；音轨延迟/字幕延迟调节已接入。
- 外挂字幕从 `SRT` 扩展到更常见桌面播放器能力（至少明确 `ASS/SSA` 策略）。
- 渲染后端策略收敛：
  - 要么完成 `OpenGL` 真正可用；
  - 要么明确当前阶段只支持 `SDL + D3D11`。
- 倍速策略继续完善：当前已有倍速控制闭环，但缺少更成熟的音频时伸/音高策略。

### P1（补齐体验，接近 MPC-HC 常用操作）

- 缩放/画幅比切换、旋转、循环模式、播放历史。
- 渲染器/解码器/音频输出设备可选项。
- 滤镜参数用户入口与持久化。

### P2（平台化与扩展能力）

- 插件系统：动态加载、版本兼容、生命周期、错误隔离。
- 流媒体：真实 HTTP 分片下载、缓冲管理、自适应码率。
- 皮肤系统：主题资源化与组件级样式覆盖。

## 5. 代码层证据摘要

以下文件可以直接说明当前“已打通 vs 仍是骨架”的边界：
- 播放主链：`src/core/player_core.cpp`
- 主程序接入播放列表/设置/快捷键：`src/main.cpp`
- 外挂字幕接入：`src/video_player.cpp`
- 字幕时间线：`src/subtitle/srt_parser.cpp`、`src/subtitle/subtitle_timeline.cpp`
- `SDL` 渲染主路径：`src/render/sdl_video_renderer.cpp`
- `D3D11` 最小可用与回退：`src/render/d3d11_video_renderer.cpp`
- `OpenGL` 仍为占位：`src/render/opengl_video_renderer.cpp`
- 解码后端选择与硬解回退：`src/decoder/decoder_factory.cpp`
- 设置系统：`src/config/settings_manager.cpp`
- 快捷键系统：`src/input/hotkey_manager.cpp`
- 播放列表：`src/playlist/playlist_manager.cpp`
- 流媒体下载仍未实现：`src/streaming/http_stream_downloader.cpp`
- 插件系统仍为骨架：`src/plugin/plugin_manager.cpp`
- 皮肤系统仍为骨架：`src/ui/skin_engine.cpp`

## 6. 验收与报告证据

以下本地报告表明多个原先被评估为“未接入”的模块现在已经具备端到端能力：
- 字幕同步：`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`
- 播放列表流程：`docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`
- 设置持久化：`docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`
- 渲染失败回退：`docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`
- Windows 软解/硬解会话：`docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`
- 章节导航：`docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`
- A-B Repeat：`docs/reports/AB_REPEAT_LOCAL_CHECK.md`
- 截图：`docs/reports/SCREENSHOT_LOCAL_CHECK.md`
- 帧步进：`docs/reports/FRAME_STEP_LOCAL_CHECK.md`
- 音轨/字幕时延调节：`docs/reports/DELAY_ADJUST_LOCAL_CHECK.md`
- 数字热键跳转：`docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md`
- 性能日志：`docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`
- 1080p60 稳定播放：`docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`
- 4K 播放与降级：`docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`
- 格式矩阵：`docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

## 7. 建议里程碑（面向落地）

里程碑 1（先“收敛当前主线”）：
- `M4` 交互增强项（章节 / A-B / 截图 / 帧步进 / 时延调节 / 数字热键跳转）已收口，后续转入发布门禁与轨道能力补齐。

里程碑 2（补“发布门禁”）：
- 完成 `M2 2.2`：高码率样本稳定性门禁；`1080p60`、`4K` 与性能日志验收入口已补齐。

里程碑 3（定平台策略）：
- 明确 `OpenGL` 是继续落地，还是阶段性降级为非目标后端。

里程碑 4（扩展能力）：
- 插件系统、流媒体、皮肤系统进入真正产品化阶段。

