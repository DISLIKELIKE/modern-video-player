# 问题修复记录

## 问题 98: OpenGL libass 差距清单、显示级 HDR 设计与 quirk diagnostics 正式化

**日期**: 2026-03-24

### 问题描述
- 用户要求把上一轮给出的三个后续建议全部落地：
  - 补 OpenGL 与 `libass/mpv` 的能力差距清单
  - 补显示级 HDR 输出设计
  - 把 OpenGL quirk 机制做成可扩展规则表和独立诊断快照
- 当前 OpenGL 虽已具备 `ASS/SSA` 基础渲染、native interop 和基础 HDR aware 处理，但对外仍缺少明确的差距口径与正式诊断入口。

### 原因分析
- `ASS/SSA` 之前虽然已能渲染，但“还差多少”仍然停留在模糊描述，没有拆到 `karaoke / vector / shaping / font fallback` 等成熟播放器真正敏感的维度。
- 现有 OpenGL HDR 处理还只是 renderer 内部色彩变换，没有把显示级 HDR 输出需要的 `swapchain / metadata / ICC/3D LUT` 结构化下来。
- OpenGL native interop 的启动期决策此前仍偏内部实现细节，缺少像 `--d3d11-diagnostics` 那样可单独执行的机器可读快照。

### 解决方案
- 在 `include/render/opengl_video_renderer.h` 与 `src/render/opengl_video_renderer.cpp` 中新增 `OpenGLDiagnosticsSnapshot`、`probeSystemDiagnostics()` 和结构化 `hard blocker + quirk rule + env override` 启动期决策。
- 在 `src/main.cpp` 中新增 `--opengl-diagnostics` CLI，一次性输出 OpenGL 上下文、`WGL_NV_DX_interop`、D3D11 基础能力、最终生效规则与 override 结果。
- 新增 `docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`，把 `libass` 差距拆为明确清单。
- 新增 `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`，明确显示级 HDR 输出应走 `DXGI HDR swapchain bridge` 路径，并定义 metadata/ICC 策略边界。
- 更新 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md` 与 records 文档。

### 本地验收
- `cmake --build build --config Release`：通过。
- `./build/Release/modern-video-player.exe --opengl-diagnostics`
  - `probe_succeeded=true`
  - `has_wgl_dx_interop=true`
  - `native_interop.allowed=true`
  - `result=PASS`
- `$env:MVP_OPENGL_NATIVE_INTEROP='disable'; ./build/Release/modern-video-player.exe --opengl-diagnostics`
  - `native_interop.env_override=disable`
  - `native_interop.allowed=false`
  - `native_interop.disable_rule=env_disable`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000`
  - `renderer_backend=OpenGL`
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `result=PASS`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass`
  - `mismatches=0`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass`
  - `subtitle_loaded=true`
  - `probe_found=true`
  - `result=PASS`

### 修改文件
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 问题 97: OpenGL ASS/SSA、driver quirk 收敛与 HDR/色彩管理补齐

**日期**: 2026-03-24

### 问题描述
- 用户要求继续把 `OpenGL` 往成熟播放器方向推进，优先补齐 `ASS/SSA`、driver quirk 收敛和 `HDR / 色彩管理`。
- 当前 `OpenGL` 虽已具备可播放链路和 `D3D11VA -> OpenGL` 原生互操作，但字幕、启动期策略和色彩处理仍偏 `M0/M1-` 过渡状态。

### 原因分析
- 旧 OpenGL 字幕路径仍偏简单叠字，不能稳定承接 `ASS/SSA` 的 item/run 样式信息。
- native interop 主要依赖运行期失败回退，缺少启动期环境策略、适配器/驱动诊断和保守 blacklist。
- 软件上传和原生 interop 两条 OpenGL 路径的色彩处理此前没有收敛到一套统一且可观测的管线。

### 解决方案
- 在 `src/render/opengl_video_renderer.cpp` 中将 OpenGL 字幕渲染升级为 `DirectWrite + D2D offscreen -> GL texture`，支持多 `SubtitleItem` 排序、item/run 分段样式、定位对齐、描边、阴影、背景框和填充。
- 为 OpenGL native interop 增加 `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`、软件 GL blacklist 和 `[diag:opengl-native]` 启动期诊断。
- 统一软件/原生 OpenGL 色彩链路：补齐 `BT.601 / 709 / 2020`、`PQ / HLG`、基础 tone-map 和 `BT.2020 -> BT.709` gamut mapping，并输出 `[diag:opengl-color]`。
- 新增 `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md` 记录 implementation planner，并更新 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`。

### 本地验收
- `cmake --build build --config Release`：通过。
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --performance-log-check ./juren-30s.mp4 2000`
  - `renderer_backend=OpenGL`
  - `decoder_backend=D3D11VA`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `render_frames=47`
  - `result=PASS`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./build/tmp_opengl_ass_validation.ass`
  - `entries=2`
  - `mismatches=0`
  - `result=PASS`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./juren-30s.mp4 ./build/tmp_opengl_ass_validation.ass`
  - `subtitle_loaded=true`
  - `subtitle_entries=2`
  - `probe_found=true`
  - `result=PASS`

### 修改文件
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY26_OPENGL_ASS_HDR_QUIRK_CONVERGENCE.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 问题 96: OpenGL 渲染链路 M0 落地并完成本地验收

**日期**: 2026-03-24

### 问题描述
- `OpenGLVideoRenderer` 之前仍是 stub，显式选择 `OpenGL` 后端时只能回退 `SoftwareSDL`。
- 用户要求把 OpenGL 链路补成可实际播放的后端，并明确它与成熟播放器 GPU 渲染链路的差距。

### 原因分析
- 旧实现缺少 SDL OpenGL 窗口、上下文、shader、纹理上传、颜色转换与事件泵，只有接口没有真正渲染路径。
- 因此显式设置 `MVP_RENDERER_BACKEND=opengl` 时，并不存在可交付的视频显示能力。

### 解决方案
- 实现 `SDL + OpenGL 2.1 compatibility context + GLSL 120` 的最小可用渲染线程。
- 支持 `YUV420P / NV12` 直接帧格式、基础热键控制、全屏切换、退出事件与失败自动回退。
- 新增 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md` 记录本地验收结果，并同步功能说明与差距文档口径。
- 保持默认后端策略不变，当前 `OpenGL` 仍为 opt-in 的 M0 过渡后端。

### 本地验收
- `Release` 构建通过。
- `MVP_RENDERER_BACKEND=opengl` + `--performance-log-check .\juren-30s.mp4 2000` 输出：
  - `performance-log-check.renderer_backend=OpenGL`
  - `performance-log-check.decoder_backend=D3D11VA`
  - `performance-log-check.result=PASS`

### 修改文件
- `CMakeLists.txt`
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md`
- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`







鏈枃妗ｈ褰曞紑鍙戣繃绋嬩腑閬囧埌鐨勯棶棰樺強鍏惰В鍐虫柟妗堛€?






---







## 闂鍒楄〃







| # | 鏃ユ湡 | 闂 | 鐘舵€?|



|---|------|------|------|



| 1 | 2026-02-17 | FFmpeg 8.0 鍏煎鎬ч棶棰?| 鉁?宸蹭慨澶?|



| 2 | 2026-02-24 | 瑙嗛娴佺储寮曚笉鍖归厤 | 鉁?宸蹭慨澶?|



| 3 | 2026-02-24 | 闊抽娴佺储寮曚笉鍖归厤 | 鉁?宸蹭慨澶?|



| 4 | 2026-02-24 | YUV 鏁版嵁娓叉煋閿欒 | 鉁?宸蹭慨澶?|



| 5 | 2026-02-24 | 浼佷笟绾?Quill 鏃ュ織閫氶亾 | 鉁?宸蹭慨澶?|



| 6 | 2026-02-25 | 澶氱嚎绋嬫挱鏀炬灦鏋勯噸鏋?| 鉁?宸插畬鎴?|



| 7 | 2026-02-25 | 闊抽鎾斁鏋舵瀯淇 | 鉁?宸蹭慨澶?|



| 9 | 2026-02-25 | VideoFrame/AudioFrame 绉诲姩璇箟缂洪櫡 | 鉁?宸蹭慨澶?|



| 10 | 2026-02-25 | 澶氳В鐮佸櫒瀹炰緥绔炰簤璇诲彇瀵艰嚧瑙ｇ爜閿欒 | 鉁?宸蹭慨澶?|



| 11 | 2026-02-27 | 骞跺彂璇诲彇 AVFormatContext 瀵艰嚧宕╂簝 | 鉁?宸蹭慨澶?|



| 12 | 2026-02-27 | 浼佷笟绾у绾跨▼鏋舵瀯閲嶆瀯 | 鉁?宸插畬鎴?|



| 15 | 2026-03-06 | 灏忓睆绐楀彛杩囧ぇ涓旀嫋鎷界缉鏀句笉绋冲畾 | 鉁?宸蹭慨澶?|



| 18 | 2026-03-07 | DASH 瑙ｆ瀽缂栬瘧澶辫触涓庢牸寮忚兘鍔涚煩闃电己澶?| 鉁?宸蹭慨澶?|



| 19 | 2026-03-07 | D3D11VA 纭В鏈€灏忛棴鐜笌杞В鍥為€€ | 鉁?宸蹭慨澶?|



| 20 | 2026-03-07 | 鎺㈡祴鍏ュ彛涓庢牸寮忓洖褰掕剼鏈惤鍦?| 鉁?宸蹭慨澶?|



| 21 | 2026-03-07 | GitHub Actions 鑷姩鏍煎紡鍥炲綊鎺ュ叆 | 鉁?宸蹭慨澶?|



| 22 | 2026-03-07 | 鎾斁鍒楄〃涓婚摼璺€佽缃寔涔呭寲涓庡揩鎹烽敭棣栫増鎺ュ叆 | 鉁?宸蹭慨澶?|



| 23 | 2026-03-07 | 绉婚櫎 Core 鍗曞厓娴嬭瘯鐩爣涓庢祴璇曟枃浠?| 鉁?宸蹭慨澶?|



| 24 | 2026-03-07 | 澶栨寕瀛楀箷鍔犺浇鍏ュ彛锛圫RT锛夋帴鍏ヤ富娴佺▼ | 鉁?宸蹭慨澶?|



| 25 | 2026-03-07 | 瀛楀箷娓叉煋鍙犲姞涓庢挱鏀炬椂搴忓悓姝ユ帴鍏?| 鉁?宸蹭慨澶?|



| 26 | 2026-03-08 | 瀛楀箷寮€鍏虫帶鍒朵笌瀛楀箷鍔犺浇寮傚父澶勭悊瀹屽杽 | 鉁?宸蹭慨澶?|



| 27 | 2026-03-08 | 蹇嵎閿厤缃寔涔呭寲鎺ュ叆锛坔otkey.*锛?| 鉁?宸蹭慨澶?|



| 28 | 2026-03-08 | 蹇嵎閿啿绐佹娴嬩笌鎭㈠榛樿鑳藉姏 | 鉁?宸蹭慨澶?|



| 29 | 2026-03-08 | M1 楠屾敹 1.4.1锛歋RT seek 鍚屾鑷鍛戒护钀藉湴 | 鉁?宸蹭慨澶?|



| 30 | 2026-03-08 | M1 楠屾敹 1.4.2锛氭挱鏀惧垪琛ㄨ繛缁挱鏀?5 鏂囦欢鑷閫氳繃 | 鉁?宸蹭慨澶?|



| 31 | 2026-03-08 | M1 楠屾敹 1.4.3锛氳缃噸鍚仮澶嶈嚜妫€閫氳繃 | 鉁?宸蹭慨澶?|



| 32 | 2026-03-08 | M2 2.1.2锛氬鍣ㄧ煩闃佃ˉ榻?mov/avi/m2ts 骞跺洖褰掗€氳繃 | 鉁?宸蹭慨澶?|



| 33 | 2026-03-08 | M2 2.1.3锛氳棰戠紪鐮佺煩闃佃ˉ榻?MPEG-2 骞跺洖褰掗€氳繃 | 鉁?宸蹭慨澶?|



| 34 | 2026-03-08 | M2 2.1.4锛氶煶棰戠紪鐮佺煩闃佃ˉ榻?E-AC3/DTS/Vorbis/PCM 骞跺洖褰掗€氳繃 | 鉁?宸蹭慨澶?|



| 35 | 2026-03-08 | M3 3.1.1锛欴ecoderFactory 鎺ュ叆鐪熷疄鍒濆鍖栨祦绋?| 鉁?宸蹭慨澶?|



| 36 | 2026-03-08 | M3 3.1.2锛欴3D11VA 鍗忓晢澶辫触杞В鍏滃簳瀹屽杽 | 鉁?宸蹭慨澶?|



| 37 | 2026-03-08 | M3 3.2.1锛欴3D11 娓叉煋鏈€灏忓彲鐢ㄩ摼璺惤鍦?| 鉁?宸蹭慨澶?|



| 38 | 2026-03-08 | M3 3.3.2锛氭覆鏌撳け璐ラ檷绾у洖褰掑叆鍙ｈˉ榻?| 鉁?宸蹭慨澶?|



| 39 | 2026-03-08 | M3 3.3.1锛歐indows 杞В/纭В涓诲姏鏍锋湰鍥炲綊閫氳繃 | 鉁?宸蹭慨澶?|



| 40 | 2026-03-08 | M4 4.1锛氱珷鑺傚鑸紙涓婁竴绔?涓嬩竴绔狅級鎺ュ叆涓庨獙鏀?| 鉁?宸蹭慨澶?|



| 41 | 2026-03-08 | M4 4.2锛欰-B Repeat锛圓/B/C锛夋帴鍏ヤ笌楠屾敹 | 鉁?宸蹭慨澶?|



| 42 | 2026-03-08 | M4 4.3锛堟埅鍥撅級 | 鉁?宸蹭慨澶?|



| 43 | 2026-03-08 | `MPC_HC_GAP_ANALYSIS` 璇勪及缁撹杩囨湡 | 鉁?宸蹭慨澶?|



| 44 | 2026-03-08 | `docs/records/VERSION.md` 鍘嗗彶璺緞鎻忚堪杩囨湡 | 鉁?宸蹭慨澶?|



| 45 | 2026-03-08 | README 涓庢灦鏋勬枃妗ｄ粛娣风敤鏃т富閾捐〃杩?| 鉁?宸蹭慨澶?|



| 46 | 2026-03-08 | 瀹炵幇鏁欑▼涓庤凯浠ｈ鍒掔己灏戝巻鍙?褰撳墠杈圭晫璇存槑 | 鉁?宸蹭慨澶?|



| 47 | 2026-03-08 | 杈呭姪璇存槑鏂囨。浠嶇己灏戝綋鍓嶅叆鍙ｄ笌鐘舵€佽竟鐣?| 鉁?宸蹭慨澶?|



| 48 | 2026-03-08 | 鏍?README 鏁呴殰鎺掗櫎涓庡巻鍙查棶棰樺綊妗ｄ粛鏈夋棫鍙ｅ緞 | 鉁?宸蹭慨澶?|



| 49 | 2026-03-08 | 缂哄皯鐙珛鐨勬枃妗ｅ贰妫€鎬昏〃 | 鉁?宸蹭慨澶?|



| 50 | 2026-03-08 | M4 4.4锛氭殏鍋滄€佸抚姝ヨ繘鎺ュ叆涓庨獙鏀?| 鉁?宸蹭慨澶?|



| 53 | 2026-03-08 | M2 2.2.4锛氳緭鍑烘挱鏀炬€ц兘鏃ュ織锛堟帀甯?闃熷垪/CPU/GPU锛?| 鉁?宸蹭慨澶?|



| 54 | 2026-03-08 | M2 2.2.1 / 2.3.2锛?080p60 绋冲畾鎾斁楠屾敹 | 鉁?宸蹭慨澶?|



| 55 | 2026-03-08 | M2 2.2.2 / 2.3.3锛?K 鎾斁涓庨檷绾ч獙鏀?| 鉁?宸蹭慨澶?|



| 56 | 2026-03-08 | M2 2.2.3锛?80Mbps 楂樼爜鐜囨牱鏈獙鏀?| 鉁?宸蹭慨澶?|



| 57 | 2026-03-18 | D3D11 鍘熺敓 GPU 娓叉煋閾捐ˉ榻?| 鉁?宸蹭慨澶?|



| 66 | 2026-03-18 | 鍏ㄥ眬鏋勫缓闃诲娓呯悊涓?ASS/SSA 鍘熺敓 D3D11 瀛楀箷閾?| 鉁?宸蹭慨澶?|



| 67 | 2026-03-18 | ASS 鏍囩瑙ｆ瀽涓?UTF-16 瀛楀箷鑼冨洿淇 | 鉁?宸蹭慨澶?|



| 68 | 2026-03-18 | MSVC warning debt 鍒嗗眰娓呯悊锛圕4819 / C4996 / C4706锛?| 鉁?宸蹭慨澶?|



| 69 | 2026-03-19 | PlayerCore 鍋滄挱鏀跺彛銆佸寘闃熷垪鎵€鏈夋潈涓?Clock/Demuxer 璁捐鍊轰慨澶?| 鉁?宸蹭慨澶?|



| 70 | 2026-03-19 | 闊抽璁惧澶辫触鏃剁殑瑙嗛-only闄嶇骇涓庡洖褰掗棬绂佺籂鍋?| 鉁?宸蹭慨澶?|



| 71 | 2026-03-19 | 4K backend session 瀛愯繘绋嬮€€鍑鸿矾寰勪慨澶?| 鉁?宸蹭慨澶?|



| 72 | 2026-03-19 | 楂樼爜鐜?4K 闃熷垪瀹归噺銆佽嚜閫傚簲鑺傛祦涓?copy-back 璇婃柇澧炲己 | 鉁?宸蹭慨澶?|



| 73 | 2026-03-19 | SoftwareSDL 鎷疯礉閾捐矾閲忓寲銆丼cheduler 閲嶅惎棰勭畻涓?renderer override | 鉁?宸蹭慨澶?|



| 74 | 2026-03-19 | Audio-master lateness 鏀剁揣涓?SoftwareSDL 鍑忔嫹璐濇湁闄愰噸鏋?| 鉁?宸蹭慨澶?|



| 75 | 2026-03-19 | 鎾ゅ洖 SoftwareSDL automatic software-first 骞惰ˉ杞В闃诲璇婃柇 | 鉁?宸蹭慨澶?|



| 76 | 2026-03-19 | Software video decode 鐪熷疄浜у抚涓撻」妫€鏌ヤ笌 blocker 瀹氫綅 | 鉁?宸蹭慨澶?|



| 77 | 2026-03-19 | Software decode 棣栧寘鍋滄粸澶嶆牳涓?SDL renderer 娉ㄩ噴涔辩爜淇 | 鉁?宸蹭慨澶?|



| 78 | 2026-03-19 | software decode 鏈€灏?send/dequeue 璁℃暟鎺ュ叆涓庨鍖呴€佸寘鍋滄粸閽夋 | 鉁?宸蹭慨澶?|



| 79 | 2026-03-19 | PlayerCore 杩愯鎬?software send probe 瀵圭収鏀舵暃 | 馃攳 宸插畾浣?|



| 80 | 2026-03-19 | 鏂囨。涓€鑷存€цˉ榻愶細CHANGELOG 绱㈠紩淇涓庨棶棰?69 analysis 鍥炲～ | 鉁?宸蹭慨澶?|



| 81 | 2026-03-20 | PlayerCore seek/flush timeline serial 鍖栫浜岄樁娈?| 鉁?宸蹭慨澶?|
| 82 | 2026-03-20 | PlayerCore EOF/Ended 缁堟€佽涔夐噸璁捐 | 鉁?宸蹭慨澶?|
| 83 | 2026-03-20 | PlayerCore queue generation 涓?Scheduler 鎺у埗蹇収鏀跺彛 | 鉁?宸蹭慨澶?|
| 84 | 2026-03-20 | PlayerCore 鍓綔鐢ㄩ泦涓寲涓?runtime failure/recovery policy 鏀跺彛 | 鉁?宸蹭慨澶?|
| 85 | 2026-03-20 | PlayerCore 鍓╀綑椋庨櫓鏀舵暃锛歋cheduler 缁堢増绛栫暐銆丗ailSession 瀹炲寲涓?serial/generation 瑙傛祴寮哄寲 | 鉁?宸蹭慨澶?|
| 86 | 2026-03-20 | 澧炶ˉ serial/failsession 鍥炲綊鎺㈤拡锛堣繛缁?seek銆佹殏鍋滄€?seek銆乧lose/reopen锛?| 鉁?宸蹭慨澶?|
| 87 | 2026-03-20 | serial/failsession 鍥炲綊澧炲姞涓€閿仛鍚?gate锛堥檷浣庢紡璺戦闄╋級 | 鉁?宸蹭慨澶?|
| 88 | 2026-03-20 | 寮哄埗 FailSession 鍥炲綊鎺㈤拡涓?codec 閿侀噸鍏ュ穿婧冧慨澶?| 鉁?宸蹭慨澶?|
| 89 | 2026-03-20 | run_all_checks 鎺ュ叆 forced-failsession 涓€閿?gate | 鉁?宸蹭慨澶?|
| 90 | 2026-03-23 | D3D11 鍘熺敓鐩撮噰鏍烽粦灞忥細杩愯鏃剁鐢?native direct 骞跺洖閫€ copy-back | 鉁?宸蹭慨澶?|
| 91 | 2026-03-23 | D3D11VA 鑷畾涔?hw_frames_ctx锛氱敵璇峰彲閲囨牱瑙ｇ爜琛ㄩ潰骞舵仮澶嶉浂鎷疯礉鐩撮噰鏍?| 鉁?宸蹭慨澶?|
| 92 | 2026-03-23 | D3D11 鍚姩鏈熻兘鍔涙帰娴嬩笌 adapter/driver 璇婃柇鏃ュ織琛ラ綈 | 鉁?宸蹭慨澶?|
| 93 | 2026-03-23 | D3D11 decoder profile 鎺㈡祴銆乹uirk blacklist 涓庣嫭绔?diagnostics CLI | 鉁?宸蹭慨澶?|
| 94 | 2026-03-23 | 1.0.0-rc1 鍙戝竷鍑嗗锛氬彂甯冩竻鍗曘€佸凡鐭ラ棶棰樹笌鍙戝竷璇存槑鏀跺彛 | 鉁?宸蹭慨澶?|
| 95 | 2026-03-23 | RC 鐗堟湰鍏冩暟鎹€丷elease 椤垫鏂囦笌瀹夎鍖呯増鏈爣璇嗚ˉ榻?| 鉁?宸蹭慨澶?|







## 闂 95: RC 鐗堟湰鍏冩暟鎹€丷elease 椤垫鏂囦笌瀹夎鍖呯増鏈爣璇嗚ˉ榻?
**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰鏄庣‘ Release 椤垫鏂囨斁鍦ㄥ摢閲岋紝骞惰姹傛妸绋嬪簭鍐呴儴鐗堟湰銆乄indows 鍙墽琛屾枃浠剁増鏈拰瀹夎鍖呯増鏈兘缁熶竴鏄剧ず涓?`rc1`銆?
- 褰撴椂宸ョ▼閲岀殑 `CMakeLists.txt` 浠嶅彧鏈?`project(... VERSION 1.0.0)`锛屼粨搴撻噷涔熸病鏈夊崟鐙殑 Release 姝ｆ枃鏂囦欢銆乣--version` CLI銆乣VERSIONINFO` 璧勬簮鍜?`CPack` 鍖呭懡鍚嶈鍒欍€?
- `HTTP` 涓嬭浇閾捐矾鐨?`user_agent` 浠嶅浐瀹氫负 `modern-video-player/1.0`锛屼笉鍒╀簬鍖哄垎 RC 鏋勫缓涓庡悗缁寮忕増/鍚庣画鍊欓€夌増銆?
### 鍘熷洜鍒嗘瀽

- 鍘熸湁鐗堟湰淇℃伅鍙仠鐣欏湪 `CMake project version`锛屾病鏈夋妸 prerelease suffix 浼犳挱鍒扮▼搴忓唴閮ㄥ瓧绗︿覆銆乄indows 璧勬簮淇℃伅鍜屽彂甯冨寘鍛藉悕銆?
- 椤圭洰姝ゅ墠缂哄皯 `VERSIONINFO` 璧勬簮涓庣嫭绔?`Release Notes` 鏂囦欢锛屽洜姝も€滃彲鍙?RC鈥濈殑鍐呴儴缁撹鍜屸€滃彲鐩存帴璐村嚭鍘荤殑鍙戝竷姝ｆ枃鈥濅箣闂翠粛鏈夌己鍙ｃ€?
- 娌℃湁鎵撳寘瑙勫垯鏃讹紝鍘嬬缉鍖呭懡鍚嶃€佸唴瀹硅竟鐣屽拰鏄惁娣峰叆鏈湴閰嶇疆涔熼兘鏃犳硶琚嚜鍔ㄩ獙璇併€?
### 瑙ｅ喅鏂规

- 鏂板 `docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`锛屼綔涓?GitHub Release 椤靛彲鐩存帴浣跨敤鐨勬鏂囥€?
- 鍦?`CMakeLists.txt` 涓紩鍏ョ粺涓€鐗堟湰婧愶紝鐢熸垚 `mvp_version.h` 涓?Windows `version_info.rc`锛岀粺涓€杈撳嚭 `1.0.0-rc1`銆?
- `main` 鏂板 `--version`锛沗http_stream_downloader.cpp` 鏀逛负澶嶇敤缁熶竴鐗堟湰澶达紝`user_agent` 鍙樹负 `modern-video-player/1.0.0-rc1`銆?
- 鏂板 `CPack ZIP` 鎵撳寘瑙勫垯涓庡畨瑁呴」锛岄獙璇佷骇鐗╂枃浠跺悕涓?`modern-video-player-1.0.0-rc1-windows-x64.zip`锛屽苟灏?`RELEASE_NOTES.md` 涓€骞舵墦鍖咃紝鍚屾椂鎺掗櫎鏈湴 `config/player_settings.ini`銆?
- 鍩轰簬 Release 鏋勫缓楠岃瘉锛歚--version`銆乄indows `FileVersionInfo`銆乣PACKAGE` 鐩爣鍜?`--d3d11-diagnostics`銆?
### 淇敼鏂囦欢

- CMakeLists.txt

- cmake/mvp_version.h.in

- cmake/version_info.rc.in

- src/main.cpp

- src/streaming/http_stream_downloader.cpp

- docs/reports/V1_0_0_RC1_RELEASE_NOTES.md

- docs/reports/V1_0_0_RC1_RELEASE_READINESS.md

- docs/reports/README.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/records/VERSION.md

## 闂 94: 1.0.0-rc1 鍙戝竷鍑嗗锛氬彂甯冩竻鍗曘€佸凡鐭ラ棶棰樹笌鍙戝竷璇存槑鏀跺彛

**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 褰撳墠鎾斁鍣ㄥ凡缁忓叿澶囦富娴佹湰鍦版挱鏀捐兘鍔涖€佺ǔ瀹?seek 涓婚摼浠ュ強 `D3D11VA + D3D11` 涓诲姏娓叉煋璺緞锛屼絾浠撳簱閲屼粛缂哄皯涓€浠介潰鍚?`RC` 鍙戝竷鐨勭粺涓€缁撹鏂囨。銆?
- 鐜版湁楠岃瘉璇佹嵁鍒嗘暎鍦?`VERSION / CHANGELOG / DEVELOP_LOG` 浠ュ強澶氫釜 `reports/*_LOCAL_CHECK.md` 涓紝缂哄皯涓€涓彲浠ョ洿鎺ュ洖绛斺€滅幇鍦ㄨ兘涓嶈兘鍙?`1.0.0-rc1`銆佸凡鐭ラ棶棰樻槸浠€涔堛€佸澶栬鎬庝箞鎻忚堪鈥濈殑鏀跺彛鍏ュ彛銆?
### 鍘熷洜鍒嗘瀽

- 杩囧幓鐨?records 鏇村亸鍚戦棶棰樹慨澶嶅拰鑳藉姏澧為噺璁板綍锛屼笉鐩存帴绛変环浜庡彂甯冭鏄庛€?
- 鍗充究杩戞湡 D3D11銆乻erial/failsession銆乫ormat regression 绛夎兘鍔涘凡缁忕户缁敹鍙ｏ紝娌℃湁鍗曠嫭鐨?RC 姹囨€绘枃妗ｏ紝浠嶇劧瀹规槗鎶娾€滃彲鍙?RC鈥濆拰鈥滃彲鍙戞寮忕増鈥濇贩涓轰竴璋堛€?
- 鍚屾椂锛屽綋鍓嶈繕鏈変竴涓繀椤绘樉寮忓憡鐭ョ殑娈嬩綑椋庨櫓锛歚闂 79` 瀵瑰簲鐨?software video decode 杩愯鎬佽矾寰勫皻鏈畬鍏ㄦ敹鍙ｃ€?
### 瑙ｅ喅鏂规

- 鏂板 RC 姹囨€绘姤鍛?`docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`锛岀粺涓€鍖呭惈锛?  - `1.0.0-rc1` 鍙戝竷缁撹
  - 鏈疆鏂板楠岃瘉璇佹嵁
  - 鏃㈡湁鑳藉姏璇佹嵁鍏ュ彛
  - 鍙戝竷璇存槑
  - 宸茬煡闂
  - 鍙戝竷娓呭崟

- 閲嶆柊鍩轰簬 `Release` 鏋勫缓鎵ц RC 鐩存帴鐩稿叧妫€鏌ワ細
  - `tools/run_all_checks.ps1`
  - `--d3d11-diagnostics`
  - `--performance-log-check .\juren-30s.mp4 2000`
  - `--long-playback-check .\juren-30s.mp4 10000`
  - `--serial-failsession-regression-check .\juren-30s.mp4`

- 鍚屾鏇存柊 `docs/README.md`銆乣docs/reports/README.md` 鍜?`docs/records/VERSION.md`锛屾妸褰撳墠 RC 鍊欓€夌姸鎬佸拰鍏ュ彛鏂囨。鏄惧紡鍖栥€?
### 淇敼鏂囦欢

- docs/reports/V1_0_0_RC1_RELEASE_READINESS.md

- docs/reports/FORMAT_REGRESSION_20260323_224615.md

- docs/reports/README.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/records/VERSION.md
## 闂 93: D3D11 decoder profile 鎺㈡祴銆乹uirk blacklist 涓庣嫭绔?diagnostics CLI

**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 鍦ㄩ棶棰?92 宸茶ˉ榻?D3D11 鍚姩鏈?adapter/driver/format 鏃ュ織鍚庯紝褰撳墠椤圭洰浠嶇己灏戞垚鐔熸挱鏀惧櫒甯歌鐨勪笁椤瑰熀纭€璁炬柦锛?  - 瑙嗛瑙ｇ爜 profile 绾у埆鐨勮兘鍔涙帰娴嬶紝鏃犳硶鐩存帴鍥炵瓟褰撳墠鏈哄櫒瀵?`H.264 / HEVC / VP9 / AV1` 鍒板簳鏀寔鍝簺 decoder profile
  - 鍚姩鏈?quirk / blacklist 绛栫暐锛屾棤娉曞湪宸茬煡涓嶇ǔ瀹氳澶囦笂鎻愬墠绂佺敤 native direct
  - 鐙珛銆佹満鍣ㄥ彲璇荤殑 D3D11 璇婃柇 CLI锛岃嚜鍔ㄥ寲鍜岄棶棰樺鐜板満鏅粛鍙兘渚濊禆鎾斁鏈熸棩蹇?
### 鍘熷洜鍒嗘瀽

- 鏃у疄鐜板彧鏈?format-level 鑳藉姏鎺㈡祴锛屾病鏈夋灇涓?`ID3D11VideoDevice::GetVideoDecoderProfile*`锛屽洜姝も€滄牸寮忚兘寤虹汗鐞嗏€濆拰鈥滆缂栫爜 profile 鑳藉惁纭В鈥濅箣闂翠粛瀛樺湪鐩插尯銆?
- native direct 鐨勫惎鍋滄鍓嶄富瑕佷緷璧栬繍琛屾椂鐔旀柇锛岀己灏戝儚 `mpv / MPC-HC` 閭ｆ牱鐨勫惎鍔ㄦ湡淇濆畧绛栫暐锛岄亣鍒?software adapter銆佺己澶卞叧閿帴鍙ｆ垨鏄庣‘榛戝悕鍗曢┍鍔ㄦ椂锛屼笉鑳藉湪鎾斁鍓嶇洿鎺ラ檷绾с€?
- 椤圭洰鐜版湁妫€鏌ュ懡浠や互鎾斁閾捐矾涓轰腑蹇冿紝缂哄皯涓€涓笉渚濊禆瀹為檯鎾斁銆佸彲涓€娆℃€ц緭鍑哄畬鏁?D3D11 鑳藉姏蹇収鐨勭嫭绔嬪叆鍙ｃ€?
### 瑙ｅ喅鏂规

- 鍦?`D3D11VideoRenderer` 涓柊澧炵粨鏋勫寲 `D3D11DiagnosticsSnapshot`锛岀粺涓€姹囨€伙細
  - adapter / driver / feature level / interface availability
  - `NV12 / P010 / P016` 鏍煎紡鏀寔浣?  - `H.264 / HEVC / VP9 / AV1` decoder profile 鏀寔鎯呭喌
  - native direct 鍚姩鏈?allow / disable policy銆佸懡涓鍒欏拰鍘熷洜

- 鏂板 decoder profile 鎺㈡祴閫昏緫锛岀洿鎺ユ灇涓?`ID3D11VideoDevice` 鏆撮湶鐨?decoder profiles锛屽苟杈撳嚭锛?  - `h264_vld_nofgt / h264_vld_fgt`
  - `hevc_main / hevc_main10`
  - `vp9_profile0 / vp9_profile2_10bit`
  - `av1_profile0 / av1_profile1 / av1_profile2 / av1_profile2_12bit / av1_profile2_12bit_420`

- 鏂板鍚姩鏈?native direct 绛栫暐鍒ゆ柇锛涜嫢鎺㈡祴澶辫触銆乻oftware adapter銆佺己澶?`ID3D11Device3 / ID3D11VideoDevice / ID3D11VideoContext`銆乣NV12` 涓嶆弧瓒?`texture2d + shader_sample + decoder_output`锛屾垨鍛戒腑 blacklist锛屽垯鍦?renderer 鍒濆鍖栭樁娈电洿鎺ュ叧闂?native direct銆?
- 棣栫増 quirk / blacklist 瑙勫垯鏄惧紡钀藉湴锛?  - `microsoft-basic-render-driver`

- `main` 鏂板 `--d3d11-diagnostics`锛屼互 `key=value` 褰㈠紡鏈哄櫒鍙杈撳嚭鏁翠釜 D3D11 鑳藉姏蹇収锛屽苟缁欏嚭 `result=PASS/FAIL`銆?
### 淇敼鏂囦欢

- include/render/d3d11_video_renderer.h

- src/render/d3d11_video_renderer.cpp

- src/main.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂 92: D3D11 鍚姩鏈熻兘鍔涙帰娴嬩笌 adapter/driver 璇婃柇鏃ュ織琛ラ綈

**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 鍦ㄩ棶棰?90 鍜岄棶棰?91 宸插厛鍚庤В鍐斥€滈粦灞忓厹搴曗€濆拰鈥淒3D11VA 鍙噰鏍峰抚姹犫€濆悗锛孌3D11 浠嶇己灏戞垚鐔熸挱鏀惧櫒甯歌鐨勫惎鍔ㄦ湡鑳藉姏鎺㈡祴涓?adapter/driver 璇婃柇鏃ュ織銆?
- 褰撳墠濡傛灉鍚庣画鍐嶉亣鍒版煇鍙版満鍣ㄧ殑鏍煎紡鍏煎銆侀┍鍔ㄥ樊寮傘€乻wap chain 鎴?feature level 闂锛屾棩蹇楁棤娉曞湪鍒濆鍖栭樁娈电洿鎺ョ粰鍑鸿冻澶熶笂涓嬫枃銆?
### 鍘熷洜鍒嗘瀽

- 鏃у疄鐜板彧鍦ㄥ垵濮嬪寲鎴愬姛鍚庤緭鍑?`Native D3D11 renderer initialized`锛屾病鏈夎褰?adapter 鍚嶇О銆乿endor/device id銆乨river version銆乫eature level銆佹牳蹇冩帴鍙ｅ彲鐢ㄦ€с€佹牸寮忔敮鎸佷綅涓?swap chain 鍙傛暟銆?
- 杩欏鑷存帓鏌ユ椂鍙兘浠庢挱鏀炬湡鐥囩姸鍙嶆帹锛岃€屼笉鑳藉儚鎴愮啛鎾斁鍣ㄩ偅鏍峰湪鍚姩鏈熷氨鍒ゆ柇鈥滃綋鍓嶈澶囨敮鎸佷粈涔堛€佷笉鏀寔浠€涔堚€濄€?
### 瑙ｅ喅鏂规

- 鍦?`D3D11VideoRenderer` 鍚姩鏃舵柊澧炵粨鏋勫寲璇婃柇鏃ュ織锛岃緭鍑猴細
  - adapter 鎻忚堪銆乿endor/device/subsystem/revision銆乨river version銆佹樉瀛?鍏变韩鍐呭瓨銆佹槸鍚?software adapter
  - feature level銆乨ebug layer銆乵ultithread protection銆乣ID3D11Device3` / `ID3D11VideoDevice` / `ID3D11VideoContext` 鍙敤鎬?  - `NV12 / P010 / P016` 鐨?`CheckFormatSupport` 缁撴灉
  - swap chain 瀹介珮銆佹牸寮忋€乥uffer count銆乻wap effect銆乤lpha mode銆乽sage

- `MakeWindowAssociation` 鏀逛负鏄惧紡妫€鏌ュけ璐ュ苟杈撳嚭璀﹀憡锛岄伩鍏嶉潤榛樹涪澶辩獥鍙ｅ叧鑱旈敊璇€?
### 淇敼鏂囦欢

- src/render/d3d11_video_renderer.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂 91: D3D11VA 鑷畾涔?hw_frames_ctx锛氱敵璇峰彲閲囨牱瑙ｇ爜琛ㄩ潰骞舵仮澶嶉浂鎷疯礉鐩撮噰鏍?
**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 铏界劧闂 90 宸茬敤杩愯鏃?fallback 鍏滀綇浜嗛粦灞忥紝浣嗘牴鍥犱粛鍦細褰撳墠 `PlayerCore` 鍙粦瀹氫簡 `D3D11VA` 璁惧锛屾病鏈夎嚜宸辨帴绠?`hw_frames_ctx`锛屽鑷?FFmpeg/椹卞姩榛樿鍒嗛厤鍑烘潵鐨勬槸 decoder-only surface銆?
- 杩欎細璁?D3D11 renderer 鍗充娇鏀寔鍘熺敓 `AV_PIX_FMT_D3D11`锛屼篃鎷夸笉鍒板彲鐩存帴鍒涘缓 shader resource view 鐨勮В鐮佽〃闈紝闆舵嫹璐濈洿閲囨牱鏃犳硶绋冲畾鎴愮珛銆?
### 鍘熷洜鍒嗘瀽

- FFmpeg 鐨?`AVD3D11VAFramesContext` 鎻愪緵浜?`BindFlags`锛屽厑璁歌皟鐢ㄦ柟鍦?`get_format()` 闃舵鎸囧畾瑙ｇ爜甯ф睜鐨勭汗鐞嗙粦瀹氭柟寮忥紱褰撳墠椤圭洰姝ゅ墠娌℃湁璧拌繖鏉¤矾寰勶紝鍙缃簡 `hw_device_ctx`銆?
- 瀹炴祴鍦ㄥ綋鍓嶆満鍣ㄤ笂锛屽彧瑕佹妸 frames context 鐢宠涓?`D3D11_BIND_DECODER | D3D11_BIND_SHADER_RESOURCE`锛孌3D11 鍘熺敓鐩撮噰鏍峰嵆鍙仮澶嶏紝璇存槑涔嬪墠鐨勬牳蹇冪己鍙ｆ槸鈥滃抚姹犲垎閰嶇瓥鐣ヤ笉瀹屾暣鈥濓紝涓嶆槸纭欢缁濆涓嶆敮鎸?D3D11 鐩撮噰鏍枫€?
### 瑙ｅ喅鏂规

- 鍦?`PlayerCore::selectVideoPixelFormat()` 涓敼涓烘樉寮忚皟鐢?`avcodec_get_hw_frames_parameters()`锛屽垱寤哄苟鍒濆鍖栬嚜瀹氫箟 `hw_frames_ctx`銆?
- 鍦?`AVD3D11VAFramesContext::BindFlags` 涓婅拷鍔?`D3D11_BIND_SHADER_RESOURCE`锛屽苟鎶?`extra_hw_frames` 棰勭畻鍙犲姞鍒?`initial_pool_size`锛岃瑙ｇ爜甯ф睜鏃㈣兘缁?decoder 浣跨敤锛屼篃鑳借 D3D11 renderer 鐩存帴閲囨牱銆?
- 濡傛灉鑷畾涔?frames ctx 鍒濆鍖栧け璐ワ紝鍒欎粎鍥為€€鍒?decoder-owned D3D11VA surface锛屼笉涓柇纭В锛涘悓鏃堕棶棰?90 宸叉湁鐨勮繍琛屾椂 fallback 缁х画浣滀负鏈€鍚庡厹搴曘€?
### 淇敼鏂囦欢

- include/core/player_core.h

- src/core/player_core.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂 90: D3D11 鍘熺敓鐩撮噰鏍烽粦灞忥細杩愯鏃剁鐢?native direct 骞跺洖閫€ copy-back

**鏃ユ湡**: 2026-03-23

### 闂鎻忚堪

- 浣跨敤 `Release` 鏋勫缓绋嬪簭鎾斁 `juren-30s.mp4` 鏃讹紝鍑虹幇鈥滄湁澹伴煶銆佹棤鐢婚潰鈥濄€?
- `--performance-log-check` 宸叉樉绀?`renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣render_frames > 0`锛岃鏄庢挱鏀句富閾捐矾浠嶅湪杩愯锛岄粦灞忛泦涓湪 D3D11 鍘熺敓鐩撮噰鏍锋樉绀洪樁娈点€?
### 鍘熷洜鍒嗘瀽

- 鏃ч€昏緫鍙娓叉煋鍣ㄥ０鏄庢敮鎸?`AV_PIX_FMT_D3D11`锛屽氨鎸佺画鎶婄‖瑙ｅ抚鎸?native direct 璺緞閫佸叆鍍忕礌鐫€鑹插櫒锛岄粯璁ゅ亣璁捐В鐮佽〃闈㈡€昏兘鍦ㄨ繍琛屾椂鎴愬姛鍒涘缓 Y/UV plane 鐨?shader resource view銆?
- 瀹炴祴褰撳墠璁惧/椹卞姩缁勫悎涓婏紝`CreateShaderResourceView1` 瀵?`NV12` 瑙ｇ爜琛ㄩ潰杩斿洖澶辫触锛屾棩蹇椾负 `y_plane_hr=-2147024809`锛屽鑷村儚绱犵潃鑹插櫒鎷夸笉鍒板钩闈㈣祫婧愶紝浣?swap chain 浠嶇户缁?present锛屼簬鏄敤鎴风湅鍒扳€滃彧鏈夊０闊筹紝娌℃湁鐢婚潰鈥濄€?
- 杩欐槸杩愯鏃惰澶囧吋瀹规€ч棶棰橈紝涓嶆槸濯掍綋鏂囦欢鎹熷潖锛屼篃涓嶆槸 D3D11VA 瑙ｇ爜鏈韩澶辫触銆?
### 瑙ｅ喅鏂规

- 鍦?`D3D11VideoRenderer` 涓柊澧炶繍琛屾椂鐔旀柇锛氳嫢 Y/UV plane 鐨?`CreateShaderResourceView1` 澶辫触锛屾垨瑙ｇ爜琛ㄩ潰鏍煎紡涓嶆敮鎸佺洿鎺ラ噰鏍凤紝鍒欑珛鍗冲叧闂綋鍓嶄細璇濈殑 native direct rendering銆?
- native direct 琚叧闂悗锛宍supportsNativeFrameFormat()` 杩斿洖 `false`锛宍PlayerCore::prepareVideoOutputFrame()` 浼氭妸鍚庣画纭В甯ц蛋 `av_hwframe_transfer_data()` copy-back 鍒拌蒋浠跺抚锛屽啀澶嶇敤鐜版湁杞欢绾圭悊涓婁紶璺緞缁х画鏄剧ず銆?
- 鍚屾椂琛ュ厖鏄庣‘鍛婅鏃ュ織锛岃緭鍑哄け璐ュ師鍥犮€佺汗鐞嗘牸寮忋€佹暟缁勫ぇ灏忋€佺汗鐞嗙储寮曘€丠RESULT锛屽苟鏍囪 `fallback=copyback-to-software`锛屼究浜庡悗缁户缁仛椹卞姩宸紓鎺掓煡銆?
### 淇敼鏂囦欢

- src/render/d3d11_video_renderer.cpp

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md
## 闂 80: 鏂囨。涓€鑷存€цˉ榻愶細CHANGELOG 绱㈠紩淇涓庨棶棰?69 analysis 鍥炲～



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 浠婂ぉ涓夋鎻愪氦瀹屾垚鍚庯紝records / analysis 瀵圭収閲岃繕鐣欎簡涓ゅ鏂囨。涓€鑷存€х己鍙ｏ細



  - `CHANGELOG` 鐨勯棶棰樻€昏〃缂哄皯 `闂 78`锛屼絾姝ｆ枃宸茬粡瀛樺湪 `闂 78`



  - `闂 69` 宸插啓鍏?`CHANGELOG / DEVELOP_LOG / VERSION`锛屼絾娌℃湁瀵瑰簲鐨?implementation planner analysis 鏂囨。



- 杩欎袱涓棶棰橀兘涓嶅奖鍝嶄唬鐮佽繍琛岋紝浣嗕細鐩存帴褰卞搷鍚庣画鏂囨。妫€绱€侀棶棰樿拷韪拰鏂颁細璇濇帴缁川閲忋€?






### 鍘熷洜鍒嗘瀽



- `CHANGELOG` 鐨勯棶棰樻€昏〃渚濊禆鎵嬪伐缁存姢锛涘湪鍚屼竴澶╁娆¤繛缁ˉ鍐欓棶棰樿褰曟椂锛宍78` 鐨勭储寮曡琚紡鎺変簡銆?


- 涓婂崍 `fix: 鏀跺彛 PlayerCore 鍋滄挱绾跨▼涓庤繍琛屾椂璁捐鍊篳 閭ｆ鎻愪氦鍙甫浜?records 涓変欢濂楋紝娌℃湁鎶婂搴旂殑鍒嗘瀽鏂囨。涓€璧峰洖濉€?






### 瑙ｅ喅鏂规



- 琛ラ綈 `CHANGELOG` 闂鎬昏〃涓殑 `闂 78` 绱㈠紩锛屽苟鐧昏鏈 `闂 80`銆?


- 鏂板鍥炲～鍒嗘瀽鏂囨。 `docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`锛屾妸 `闂 69` 鐨?implementation planner銆佺粨璁哄拰杈圭晫鍗曠嫭娌夋穩涓嬫潵銆?


- 鍚屾鏇存柊 `闂 69` 鐨?records 鏂囦欢淇敼娓呭崟锛屼互鍙?`VERSION / DEVELOP_LOG` 涓殑鏂囨。涓€鑷存€ц褰曘€?






### 淇敼鏂囦欢



- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂 79: PlayerCore 杩愯鎬?software send probe 瀵圭収鏀舵暃



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鍦ㄩ棶棰?78 宸茬‘璁?software decode 绾跨▼鑳?dequeue 鍒伴鍖呫€佷絾棣栦釜 `avcodec_send_packet()` 娌℃湁瀹屾垚杩斿洖鍚庯紝鏈疆缁х画鎸?implementation planner 鍋氣€滅湡瀹炶繍琛屾€佷笓椤瑰鐓р€濓紝鐩爣鏄妸 blocker 鍐嶅畾姝讳竴灞傘€?


- 鐢ㄦ埛鏄庣‘瑕佹眰缁х画鍥寸粫 software decode blocker 鎺ㄨ繘锛屼笉瑕佸啀鍏堝姩 `SoftwareSDL` 娓叉煋渚с€?






### 鍘熷洜鍒嗘瀽



- 鐙珛 `--software-video-send-probe` 宸茶瘉鏄?FFmpeg software H.264 decode 鏈綋鍙敤锛屼絾瀹冭繕娌℃湁瀹屽叏瑕嗙洊 `PlayerCore` 鐪熷疄杩愯鎬佺殑鍏ㄩ儴宸紓銆?


- 鍥犳闇€瑕佺户缁€愰」鎺掗櫎锛歚receive->send` 椤哄簭銆乸acket queue 浜ゆ帴銆乨emux read-ahead銆侀煶棰戦摼銆佸綋鍓?video decode 绾跨▼涓婁笅鏂囥€?






### 瑙ｅ喅鏂规



- 鎵╁睍 `--software-video-send-probe`锛岃ˉ鍏咃細



  - `pre_send_receive_ret`



  - `packet_queue_push_ok / packet_queue_pop_ok`



  - `read_ahead_packets / read_ahead_done`



- 鎵╁睍 `--software-video-decode-check`锛?


  - 鏂板 `audio_probe_mode`



  - 鏀寔 `MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO=1` 鐨?video-only 瀵圭収



- 鍦?`PlayerCore::decodeVideoFrame()` 鏂板浠呯幆澧冨彉閲忓紑鍚殑 `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD` 璇婃柇璺緞锛岀敤浜庣‘璁?software `send_packet` 鏄惁鍙細鍗″湪褰撳墠 decode 绾跨▼銆?


- 鏈疆鍏抽敭缁撹锛?


  - 鐙珛 probe 鍦?`pre-receive + packet queue round-trip + read-ahead=512` 鍚庝粛 `send_ret=0 / result=PASS`



  - video-only software decode 浠?`video_packet_dequeue_count=1 / video_send_packet_ok=0 / result=FAIL`



  - `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD=1` 涓嬩粛鍑虹幇 `Offthread software video send_packet probe timed out after 500ms`



- 缁撹鍥犳缁х画鏀舵暃涓猴細blocker 涓嶅湪 FFmpeg software decoder 鏈綋銆乸acket queue銆乨emux read-ahead銆侀煶棰戦摼鎴栧綋鍓?video decode 绾跨▼鏈韩锛岃€屽湪 `PlayerCore` 杩愯鎬侀噷鐨?software codec context / surrounding state 宸紓銆?






### 淇敼鏂囦欢



- src/main.cpp



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY15_PLAYERRUNTIME_SOFTWARE_SEND_PROBE.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



## 闂 78: software decode 鏈€灏?send/dequeue 璁℃暟鎺ュ叆涓庨鍖呴€佸寘鍋滄粸閽夋



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鐢ㄦ埛鏄庣‘瑕佹眰涓嶈鍐嶅厛鍔?`SoftwareSDL` 娓叉煋渚э紝鑰屾槸鐩存帴娌?software decode 棣栧寘鍋滄粸鏂瑰悜琛ユ渶灏忚鏁般€?


- 褰撳墠宸茬煡 software path 浼氬嚭鐜?`decode_video_ok=0 / render_frames=0`锛屼絾浠呴潬鏃ф棩蹇楄繕涓嶈兘鎶婇棶棰樻媶鎴愨€滄病 dequeue 鍒板寘鈥濊繕鏄€渀send_packet` 鏈韩鍗′綇鈥濄€?






### 鍘熷洜鍒嗘瀽



- 鏃ц瘖鏂噷鍙湁 `decode_video_ok / decode_video_send_eagain / video_decoder_drain_signals`锛岀己灏?packet dequeue 涓?`send_packet` 鎴愬姛杩斿洖灞傞潰鐨勬渶灏忚娴嬪€笺€?


- 鍥犳鍗充娇宸茬粡鎬€鐤戦鍖呴€佸叆 decoder 闃舵鏈夐棶棰橈紝涔熸棤娉曠敤缁撴瀯鍖栨暟瀛楃洿鎺ヨ瘉鏄庛€?






### 瑙ｅ喅鏂规



- 鍦?`PlayerCore` 琛ヤ笁椤规渶灏忚鏁帮細



  - `video_packet_dequeue_count`



  - `video_send_packet_ok`



  - `video_send_packet_last_ret`



- 灏嗗畠浠€忎紶鍒帮細



  - `DiagnosticsSnapshot`



  - 浣庨閾捐矾璇婃柇鏃ュ織



  - `--performance-log-check`



  - `--software-video-decode-check`



- 鏈疆澶嶈窇 software decode 鏍锋湰鍚庯紝鍏抽敭璇婃柇宸叉敹鏁涘埌锛?


  - `v_pkt_deq=1`



  - `v_send_ok=0`



  - `v_send_ret=-2147483648`



  - `decode_video_ok=0`



  - `render_frames=0`



- 缁撹鍥犳杩涗竴姝ユ敹绱т负锛歴oftware decode 绾跨▼宸茬粡鍙栧埌棣栦釜瑙嗛鍖咃紝浣嗛涓?`avcodec_send_packet()` 娌℃湁褰㈡垚鎴愬姛杩斿洖銆?






### 淇敼鏂囦欢



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY14_VIDEO_SEND_PACKET_MIN_COUNTERS.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂 77: Software decode 棣栧寘鍋滄粸澶嶆牳涓?SDL renderer 娉ㄩ噴涔辩爜淇



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鍦?Day12 宸茬粡鎶?`SoftwareSDL + Software decode` 鐨勨€? 甯ц緭鍑衡€濆崟鐙拤姝诲悗锛屾湰杞户缁寜 implementation planner 澶嶈窇骞堕獙璇侊細淇濆畧 software decode 绾跨▼閰嶇疆鏄惁鑳借В闄?blocker銆?


- 鍚屾椂锛屼唬鐮佹枃浠堕噷杩樻畫鐣欎竴澶勭湡瀹炴敞閲婁贡鐮侊細`src/render/sdl_video_renderer.cpp` 鐨勫嚱鏁板ご娉ㄩ噴瀛樺湪鏄庢樉 mojibake锛屽奖鍝嶅悗缁槄璇诲拰 diff 瀹℃牳銆?






### 鍘熷洜鍒嗘瀽



- 鏈疆澶嶈窇鍚庯紝`Video decoder threading: backend=Software thread_count=1 thread_type=none` 宸茶瘉鏄庝繚瀹堢嚎绋嬮厤缃凡缁忓疄闄呯敓鏁堬紱浣?`decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0` 浠嶇劧鍏ㄩ儴涓?0锛岃鏄?blocker 涓嶆槸鈥滄縺杩涚嚎绋嬮厤缃鑷寸殑鍋跺彂浜や簰闂鈥濄€?


- 缁撳悎杩愯鏈熻瘖鏂棩蹇?`demux(v=163) / pkt_q(v=162)` 涓庝粎鍑虹幇 `Video decode first send_packet start` 鐨勪簨瀹烇紝鍙互鎺ㄦ柇 software decode 绾跨▼鏇村儚鏄湪棣栦釜瑙嗛鍖呮彁浜ら樁娈靛悗鍋滀綇銆?


- `src/render/sdl_video_renderer.cpp` 鐨勪贡鐮佸垯鏄崟绾紪鐮侀仐鐣欓棶棰橈紝涓嶅奖鍝嶉€昏緫锛屼絾浼氭寔缁薄鏌撲唬鐮侀槄璇诲拰瀹￠槄缁撴灉銆?






### 瑙ｅ喅鏂规



- 淇濇寔褰撳墠 software decode 淇濆畧绾跨▼閰嶇疆涓嶅洖閫€锛屽苟閲嶆柊鎵ц `--software-video-decode-check`锛屾妸 blocker 缁撹浠庘€? 甯ц緭鍑衡€濊繘涓€姝ユ敹鏁涘埌鈥滈涓棰戝寘鍚庡仠浣忊€濄€?


- 淇 `src/render/sdl_video_renderer.cpp` 鐨?9 澶勫嚱鏁板ご娉ㄩ噴涔辩爜锛屼粎鏀逛腑鏂囨敞閲婏紝涓嶆敼閫昏緫銆?


- 鍐嶆鎵弿 `src/`銆乣include/` 鐨?`///` 涓?`//` 娉ㄩ噴琛岋紝纭鏈疆鏈啀鍛戒腑鏂扮殑鍙枒涔辩爜娉ㄩ噴銆?






### 淇敼鏂囦欢



- src/render/sdl_video_renderer.cpp



- docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



## 闂 76: Software video decode 鐪熷疄浜у抚涓撻」妫€鏌ヤ笌 blocker 瀹氫綅



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 涓婁竴杞凡缁忕‘璁も€滅户缁噺灏戞垨瑙勯伩 `av_hwframe_transfer_data()` copy-back鈥濈殑涓嬩竴姝ワ紝涓嶈鐩存帴鎶?fallback 榛樿鍒囧埌 `software-first`锛岃€屽簲璇ュ厛鎶婂綋鍓嶅伐绋嬬殑 software video decode blocker 瀹氫綅娓呮銆?


- 鏃х殑 `--windows-backend-session-check soft` 鍙兘璇佹槑鈥滆兘鎵撳紑骞惰繘鍏ユ挱鏀惧惊鐜€濓紝涓嶈兘璇佹槑鈥滆蒋浠惰棰戣В鐮佺湡鐨勪骇鍑哄苟娓叉煋瑙嗛甯р€濓紱鍚屾椂涓€鏃?soft decode 鍗℃锛岀洿鎺?`stop/close` 杩樺彲鑳芥妸涓撻」鍛戒护鏈韩鎷栨寕銆?






### 鍘熷洜鍒嗘瀽



- 鐜版湁鍥炲綊鍛戒护缂哄皯瀵光€滅湡瀹炰骇甯р€濈殑纭棬妲涳紝鏃犳硶鍖哄垎鈥渁udio clock 鍦ㄦ帹杩涒€濅笌鈥渧ideo frame 鐪熷嚭鏉ヤ簡鈥濄€?


- 褰撳墠 blocker 涓嬶紝`SoftwareSDL + Software decode` 涓嶄粎浼氳〃鐜颁负 `decode_video_ok=0 / render_frames=0 / video_frame_queue_peak_size=0`锛岃繕浼氳甯歌鏀跺熬璺緞鍙樺緱涓嶅彲闈狅紱鍥犳涓撻」妫€鏌ュ懡浠や篃蹇呴』鍍?probe 涓€鏍疯嚜甯︾‖閫€鍑鸿兘鍔涖€?






### 瑙ｅ喅鏂规



- 鏂板 `--software-video-decode-check <media_file> [sample_ms]`銆?


- 鍛戒护鍐呴儴寮哄埗锛?


  - `MVP_RENDERER_BACKEND=software`



  - `SDL_AUDIODRIVER=dummy`



  - `preferHardwareDecode=false`



- 閫氳繃鏉′欢鏀剁揣涓衡€滅湡瀹炰骇甯р€濊€屼笉鏄€滃彧鎵撳紑鎴愬姛鈥濓細



  - `renderer_backend=SoftwareSDL`



  - `decoder_backend=Software`



  - `decode_video_ok > 0`



  - `scheduler_video_decoded_frames > 0`



  - `render_frames > 0`



  - `video_frame_queue_peak_size > 0`



  - `video_copy_back_frames == 0`



- 鍛戒护鏀规垚 probe 寮忕‖閫€鍑猴紝閬垮厤鍦?soft decode blocker 涓嬭 `stop/close` 鍗′綇銆?






### 淇敼鏂囦欢



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY12_SOFTWARE_VIDEO_DECODE_REAL_FRAME_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂 75: 鎾ゅ洖 SoftwareSDL automatic software-first 骞惰ˉ杞В闃诲璇婃柇



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 褰撳墠鐩爣鍘熸湰鎯崇户缁部鐫€鈥滃噺灏戞垨瑙勯伩 `av_hwframe_transfer_data()` copy-back鈥濇帹杩涳紝浜庢槸涓存椂灏濊瘯鎶?`SoftwareSDL` fallback 鏀规垚 renderer-aware `software-first`銆?


- 浣嗗疄闄呴獙璇佹樉绀猴紝涓€鏃?`SoftwareSDL + Software decode` 鑷姩鍚敤锛屾挱鏀惧櫒浼氬嚭鐜?`decode_video_ok=0 / render_frames=0`锛宍--performance-log-check` 鏃犳硶姝ｅ父鏀跺彛銆?






### 鍘熷洜鍒嗘瀽



- 浠庢挱鏀惧櫒璁捐鐞嗗康涓婄湅锛宻ystem-memory renderer 浼樺厛閬垮厤 copy-back 鐨勬柟鍚戞湰韬槸鎴愮珛鐨勶紝涔熺鍚堟垚鐔熸挱鏀惧櫒甯歌鎬濊矾銆?


- 浣嗗綋鍓嶅伐绋嬬殑 FFmpeg 杞欢瑙嗛瑙ｇ爜鎺ュ叆璺緞鏈韩瀛樺湪闃诲锛氬己鍒?`D3D11 + Software decode` 鏃朵篃鑳藉鐜拌蒋瑙ｉ摼涓嶅舰鎴愭湁鏁堣棰戜骇鍑猴紝鍥犳闂涓嶅湪 `SoftwareSDL` renderer锛岃€屽湪杞欢瑙嗛瑙ｇ爜鎺ュ叆銆?


- 鍦ㄨ繖涓墠鎻愪笅锛岀户缁粯璁ゅ惎鐢?`software-first` 鍙細鎶婂綋鍓嶇ǔ瀹氱殑 fallback 閾惧甫鍏ュ洖褰掋€?






### 瑙ｅ喅鏂规



- 鎾ゅ洖鑷姩 renderer-aware `software-first` decoder 椤哄簭璋冩暣锛屾仮澶嶉粯璁?`D3D11VA -> Software`銆?


- 淇濈暀涓婁竴杞凡缁忛獙璇侀€氳繃鐨?`NV12` 鐩翠紶銆乣AVFrame` 寮曠敤澶嶇敤鍜?`SoftwareSDL` 闆?`swscale` / 闆?`display_copy` 鏀归€犮€?


- 涓哄悗缁崟鐙慨杞欢瑙嗛瑙ｇ爜鎺ュ叆琛ュ厖浣庨璇婃柇锛?


  - FFmpeg 閿欒鐮佸瓧绗︿覆



  - 棣栨 `send_packet` 鎺㈤拡



  - stall 涓婁笅鏂囨棩蹇?






### 淇敼鏂囦欢



- include/core/player_core.h



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY11_SOFTWARE_DECODE_BLOCKER_AND_FALLBACK_DIRECTION.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







## 闂 74: Audio-master lateness 鏀剁揣涓?SoftwareSDL 鍑忔嫹璐濇湁闄愰噸鏋?


**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鐢ㄦ埛宸叉墜鍔ㄦ彁浜や笂涓€杞粨鏋滃悗锛岃姹備富閾剧户缁墦纾?`audio-master lateness / catch-up`锛屽悓鏃跺湪杞欢鍥為€€閾句笂鍋氣€滃噺灏?copy-back + swscale + 娣辨嫹璐濇鏁扳€濈殑鏈夐檺閲嶆瀯銆?


- 鏃х殑 `Scheduler::pumpRenderOnce()` 鍦?`Audio` master 涓嬩粛鐒舵槸鈥滄渶澶氱潯 5ms锛岀劧鍚庣洿鎺?render鈥濓紝瀹规槗杩囨棭鍑哄抚銆?


- `SoftwareSDL` 鍥為€€閾捐櫧鐒跺凡缁忚兘閲忓寲鐑偣锛屼絾璺緞浠嶆槸 `copy-back + swscale + display memcpy` 涓夋鍙犲姞锛岃惤鍒?4K60 鏃舵垚鏈繃楂樸€?






### 鍘熷洜鍒嗘瀽



- `Audio` master 姝ｅ悜 diff 鍙潯涓€娆′笖涓嶉噸璇讳富鏃堕挓锛屼細璁?renderer 鍦ㄩ煶棰戞椂閽熷皻鏈拷涓婃椂鎻愬墠鎻愪氦瑙嗛甯с€?


- `-250ms` 鍥哄畾 late-drop 闃堝€艰繃绮楋紝瀵?24fps/60fps 浠ュ強涓嶅悓闃熷垪濉厖搴﹂兘涓嶅鍚堢悊銆?


- `SoftwareSDL` 涔嬪墠鍙兘鍚?`IYUV` 娣辨嫹璐濆抚锛屽鑷?copy-back 涔嬪悗杩樿棰濆 `swscale`銆乣Display::copyFrameData()` 娣辨嫹璐濓紝鍐嶄氦缁?SDL 涓婁紶銆?






### 瑙ｅ喅鏂规



- `IVideoRenderer` 鏂板 `supportsDirectFrameFormat()`锛宍SdlVideoRenderer` 澹版槑鏀寔 `YUV420P/NV12`銆?


- `PlayerCore::prepareVideoOutputFrame()` 鍦ㄦ棤瑙嗛婊ら暅鏃跺厑璁?copy-back 鍚庣殑杞欢甯х洿鎺ヤ氦缁?`SoftwareSDL`锛屼笉鍐嶅己鍒?`swscale -> YUV420P`銆?


- `Display` 鏀规垚鈥滀紭鍏堜繚鐣?`AVFrame` 寮曠敤锛岃礋 stride/涓嶉€傞厤鏃舵墠娣辨嫹璐濃€濓紝骞惰ˉ `SDL_UpdateNVTexture()` 鏀寔 `NV12` 鐩翠紶銆?


- `Scheduler::pumpRenderOnce()` 鐨?`Audio` master 閫昏緫鏀规垚鍒嗘绛夊緟骞堕噸璇?clock锛屽悓鏃舵妸 late-drop 闃堝€兼敼鎴愬熀浜?`frame.duration + queue fill ratio` 鐨勫姩鎬佺獥鍙ｏ紝骞惰ˉ鏈€灏?sleep 閲忓瓙閬垮厤浼繖绛夈€?


- 宸查噸鏂版墽琛岋細榛樿 `D3D11 --performance-log-check`銆佸己鍒?`SoftwareSDL --performance-log-check`銆乣SDL_AUDIODRIVER=dummy` 涓嬬殑 `Audio` master `--performance-log-check`銆?






### 淇敼鏂囦欢



- `include/render/video_renderer.h`



- `include/render/sdl_video_renderer.h`



- `src/render/sdl_video_renderer.cpp`



- `include/display.h`



- `src/display.cpp`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `docs/analysis/PLAYERCORE_DAY10_AUDIOMASTER_AND_SOFTWARESDL_REFACTOR.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂 73: SoftwareSDL 鎷疯礉閾捐矾閲忓寲銆丼cheduler 閲嶅惎棰勭畻涓?renderer override



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鍦ㄤ笂涓€杞‘璁?D3D11 涓婚摼浠嶆槸 zero-copy 涔嬪悗锛岀敤鎴风户缁姹傚垽鏂?`Display::copyFrameData()` 涓?`Scheduler` 閲嶅惎绛栫暐鏄惁鏄珮鐮佺巼/4K 涓嶇ǔ瀹氱殑鐪熷疄鍘熷洜銆?


- 鏃у疄鐜版棤娉曢噺鍖?`SoftwareSDL` 璺緞姣忓抚 memcpy 鐨勫疄闄呮垚鏈紝`Scheduler` 涔熶粛鐒朵娇鐢ㄥ浐瀹氭鏁伴噸鍚瓥鐣ワ紝涓?renderer 閫夋嫨閾炬病鏈夌湡姝ｆ敮鎸佸己鍒?`SoftwareSDL` 閲囨牱銆?


- 杩欏鑷寸 8銆?0 鐐逛粛鐒跺彧鑳藉崐瀹氭€у垽鏂紝鏃犳硶鍍忎富閾?zero-copy 閭ｆ牱缁欏嚭纭暟鎹€?






### 鍘熷洜鍒嗘瀽



- `Display::copyFrameData()` 鐨勭粺璁℃病鏈夐€忎紶鍒?`PlayerCore` 鍜?CLI锛岃嚜鐒朵篃灏辨棤娉曠畻鍑?4K60 涓嬬殑鍗犳瘮銆?


- `Scheduler` 鍥哄畾閲嶅惎娆℃暟杩囦簬鐢熺‖锛岀煭鏃跺紓甯稿拰鎸佺画鎶栧姩鏃犳硶鍖哄垎銆?


- `RendererFactory` 涔嬪墠瀹屽叏鏃犺 `MVP_D3D11_DRIVER_HINT` / renderer override锛屽鑷?`--renderer-fallback-check` 鍜?`SoftwareSDL` 鎬ц兘閲囨牱閮戒笉鍙潬銆?






### 瑙ｅ喅鏂规



- 涓?`Display` 澧炲姞 `FrameCopyStats`锛屽苟缁忕敱 `SdlVideoRenderer -> RendererDiagnostics -> PlayerCore::DiagnosticsSnapshot` 閫忎紶鍒?`--performance-log-check`銆?


- `Scheduler` 閲嶅惎绛栫暐鏀规垚鈥?0s 绐楀彛鍐呮渶澶?4 娆?+ 100ms 鍐峰嵈鈥濓紝骞舵柊澧?`scheduler_*_restart_limit_hits`銆?


- `RendererFactory::detectBestRendererType()` 鏂板 `MVP_RENDERER_BACKEND` override锛屽苟鍦?Windows 涓嬫敮鎸?`MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`銆?


- 宸查噸鏂版墽琛岋細`MSBuild`銆乣--renderer-fallback-check`銆侀粯璁?`D3D11 --performance-log-check`銆佸己鍒?`SoftwareSDL --performance-log-check`銆?


- 鍏抽敭缁撴灉锛歚SoftwareSDL` 4K60 閲囨牱閲?`display_copy_ratio_percent=21.8407`銆乣video_copy_back_ratio_percent=30.1018`銆乣video_swscale_ratio_percent=18.6623`锛涢粯璁?`D3D11` 涓婚摼鍒欎笁鑰呴兘涓?`0`銆?






### 淇敼鏂囦欢



- `include/display.h`



- `src/display.cpp`



- `include/render/video_renderer.h`



- `include/render/sdl_video_renderer.h`



- `src/render/sdl_video_renderer.cpp`



- `src/render/renderer_factory.cpp`



- `include/core/scheduler.h`



- `src/core/scheduler.cpp`



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY9_SOFTWARE_COPY_AND_RESTART_BUDGET_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







## 闂 72: 楂樼爜鐜?4K 闃熷垪瀹归噺銆佽嚜閫傚簲鑺傛祦涓?copy-back 璇婃柇澧炲己



**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鐢ㄦ埛瑕佹眰缁х画鍥寸粫楂樼爜鐜囪棰戜笉绋冲畾杈撳嚭甯х殑闂锛岄噸鐐瑰垽鏂?`FrameQueue` 瀹归噺銆乣Scheduler` 鑳屽帇/鑺傛祦銆乣Display::copyFrameData()`銆乣av_hwframe_transfer_data()` 涓?`Scheduler` 绾跨▼绛栫暐鏄惁鏄湡姝ｇ摱棰堛€?


- 鐜版湁涓婚摼宸茬粡鍏峰 D3D11 native zero-copy锛屼絾缂哄皯 queue 宄板€笺€佽儗鍘嬬瓑寰呮椂闀裤€乧opy-back/swscale 鏃堕棿缁熻锛屽鑷撮槦鍒楁槸鍚﹁繃灏忋€乧opy-back 鏄惁鐑偣閮藉彧鑳介潬鐚溿€?


- 鍚屾椂锛宍Video` master 璺緞鐨勭瓑寰呯瓥鐣ヨ繃浜庣矖绯欙紝render loop 钀藉悗鏃朵竴娆″彧涓竴甯э紝涔熶細鏀惧ぇ楂樺垎杈ㄧ巼鏍锋湰涓嬬殑鏃跺簭鎶栧姩銆?






### 鍘熷洜鍒嗘瀽



- `FrameQueue` 涔嬪墠鍙湁褰撳墠 size/capacity锛屾病鏈?peak 涓?push timeout锛屾棤娉曠‘璁ゆ槸鍚︾湡鐨勮 frame queue 椤舵弧銆?


- `Scheduler` 鐨勫崟鐐硅儗鍘嬮槇鍊煎拰鍥哄畾灏忔绛夊緟浼氬湪楂樼爜鐜?4K 涓嬪舰鎴愨€滆涔堣繃鏃╅檺娴併€佽涔堣拷甯уお鎱⑩€濈殑涓ょ鏋佺銆?


- 鐩存帴鏀惧ぇ 4K `D3D11VA` 瑙嗛闃熷垪鍙堜細鎵撳埌 FFmpeg 鐨勫浐瀹氱‖浠跺抚姹狅紝鍥犳 frame queue 鍜?`extra_hw_frames` 蹇呴』鑱斿姩銆?


- 閲囨牱楠岃瘉琛ㄦ槑褰撳墠 4K 涓婚摼鍛戒腑鐨勪粛鐒舵槸 `D3D11VA -> D3D11` native path锛岃€屼笉鏄?copy-back / swscale锛涘洜姝?`av_hwframe_transfer_data()` 骞朵笉鏄綋鍓嶈繖鍙版満鍣ㄤ笂鐨勪富鐡堕銆?






### 瑙ｅ喅鏂规



- 涓?`FrameQueue` 澧炲姞 `peak_size / push_timeout_count / getStats / resetStats`銆?


- `PlayerCore::open()` 鐜板湪浼氭寜濯掍綋灞炴€ч厤缃棰?闊抽 frame queue 瀹归噺锛屽苟鍦?`D3D11VA` 鎵撳紑鍓嶈缃?`extra_hw_frames`锛岄伩鍏?4K native path 鎵撶垎 surface pool銆?


- `Scheduler` 鑳屽帇鏀规垚 enter/exit hysteresis锛屽苟鏂板 `video/audio_backpressure_wait_ms` 缁熻銆?


- `Scheduler::pumpRenderOnce()` 鐜板湪浼氾細



  - 鍦?`Video` master 涓嬫寜鐪熷疄 wall-clock 鍋?frame pacing



  - 鍦ㄤ竴娆?pump 鍐呰繛缁涪寮冭繃鏈熷抚锛岀洿鍒版嬁鍒板彲鏄剧ず甯?


- `--performance-log-check` 鎵╁睍杈撳嚭 copy-back/swscale 鏃堕棿銆乹ueue capacity/peak/timeout 涓庤儗鍘嬬瓑寰呮椂闀裤€?


- 宸查噸鏂版墽琛岋細



  - `--performance-log-check ...4k... 1500`



  - `--high-bitrate-check ... 3000`



  - `--4k-playback-check ... 2000`



  - `--long-playback-check .\juren-30s.mp4 6000`



  褰撳墠鍧囬€氳繃銆?






### 淇敼鏂囦欢



- `include/core/frame_queue.h`



- `include/core/player_core.h`



- `include/core/scheduler.h`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY8_QUEUE_PACING_AND_COPYBACK_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂 71: 4K backend session 瀛愯繘绋嬮€€鍑鸿矾寰勪慨澶?


**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鍦ㄤ笂涓€杞慨澶?video-only 闄嶇骇鍜?demux 闂ㄧ鍚庯紝`--4k-playback-check` 浠嶇劧澶辫触锛屼絾澶辫触鐐瑰凡鏀舵暃鍒?`fallback_ok=false`銆?


- 杩涗竴姝ュ璺戝彂鐜帮細`--windows-backend-session-check hard` 浼氬湪鎵撳嵃 `PASS` 鍚庡崱鍒扮埗杩涚▼瓒呮椂锛宍soft` 浼氬湪鎵撳嵃 `PASS` 鍚庝互寮傚父閫€鍑虹爜缁撴潫銆?


- 杩欎娇寰?4K 鍥炲綊浠嶇劧琚?backend probe 瀛愯繘绋嬭鍒ゆ嫋鎴?FAIL銆?






### 鍘熷洜鍒嗘瀽



- `runWindowsBackendSessionCheck()` 鏈川鏄笓渚涚埗杩涚▼娑堣垂鐨?probe 瀛愯繘绋嬶紝涓嶆槸鐢ㄦ埛鎬侀暱鏈熻繍琛屾挱鏀惧櫒浼氳瘽銆?


- 鏃у疄鐜伴敊璇湴澶嶇敤浜嗗父瑙勯€€鍑哄亣璁撅紝瀵艰嚧杩欐潯 probe 璺緞鍦ㄤ笉鍚?backend 缁勫悎涓嬪嚭鐜扳€滅粨鏋滃凡鎵撳嵃銆佽繘绋嬫湭姝ｅ父缁撴潫鈥濈殑闂銆?


- 鍥犱负 `runBackendSessionSubprocess()` 鍚屾椂瑕佹眰 `mode_ok=true` 涓?`exit_code==0`锛屾墍浠ヨ繖绉嶈秴鏃?寮傚父閫€鍑轰細鐩存帴鎶?`fallback_ok` 鎷夋垚 false銆?






### 瑙ｅ喅鏂规



- 灏?`runWindowsBackendSessionCheck` 鏀逛负涓撶敤鐨?`runWindowsBackendSessionCheckAndExit()`銆?


- 璇ュ懡浠ゅ湪鎵撳嵃缁撴瀯鍖栫粨鏋滃悗浼氭樉寮?flush `stdout/stderr`锛屽苟鍦?Windows 涓嬬洿鎺ヨ皟鐢?`TerminateProcess(GetCurrentProcess(), code)` 閫€鍑哄瓙杩涚▼銆?


- `main()` 鐨?`--windows-backend-session-check` 鍒嗘敮宸插垏鍒拌繖鏉′笓鐢ㄩ€€鍑鸿矾寰勩€?


- 宸查噸鏂版墽琛岋細



  - `--windows-backend-session-check ... hard`



  - `--windows-backend-session-check ... soft`



  - `--windows-backend-check ...`



  - `--4k-playback-check ... 2000`



  褰撳墠鍧囬€氳繃銆?






### 淇敼鏂囦欢



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY7_BACKEND_SESSION_EXIT_FIX.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



## 闂 70: 闊抽璁惧澶辫触鏃剁殑瑙嗛-only闄嶇骇涓庡洖褰掗棬绂佺籂鍋?


**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 褰撳墠鏈哄櫒涓?`WASAPI` 闊抽璁惧鍒濆鍖栧け璐ユ椂锛宍PlayerCore::open()` 铏界劧浼氱户缁蛋涓嬪幓锛屼絾浠嶇劧浼氬彂鍑?`AudioInitFailed` 閿欒锛屽鑷粹€滈潪鑷村懡鑳藉姏闄嶇骇鈥濆拰鈥滅湡姝ｆ墦寮€澶辫触鈥濊涔夋贩鍦ㄤ竴璧枫€?


- 楂樼爜鐜囧満鏅殑鍥炲綊妫€鏌ヤ粛鐒舵妸 `demux_dropped_packets` 褰撴垚缁熶竴澶辫触闂ㄧ锛涘湪 video-only 闄嶇骇鏃讹紝琚鐢ㄩ煶棰戞祦鐨勫寘浼氱疮璁¤繘 `demux_ignored_packets`锛屼粠鑰屾妸鈥滈鏈熷拷鐣モ€濊鍒ゆ垚鈥滈槦鍒楄儗鍘嬩涪鍖呪€濄€?


- 鏃犻煶棰戣緭鍑烘椂锛屾棫閫昏緫鎶婁富鏃堕挓閫€鍥?`System`锛岃繖瀵圭函瑙嗛鎺ㄨ繘涓嶅绋冲畾锛屼篃涓嶅埄浜庤瘖鏂?video-only 鍦烘櫙銆?






### 鍘熷洜鍒嗘瀽



- `initDecoders()` 宸茬粡鐢?`audio_player_->isInitialized()` 鎺у埗闊抽瑙ｇ爜鏄惁鍚敤锛屼絾 `open()` 灞傜己灏戞樉寮忕殑鈥滆棰戝彲缁х画鎾?/ 闊抽-only 蹇呴』澶辫触鈥濆垎鏀紝鎵€浠ヨ涓轰緷璧栧壇浣滅敤鑰屼笉鏄瓥鐣ャ€?


- `demux_dropped_packets` 鍚屾椂娣峰悎浜?ignored 鍜?queue-drop 涓ょ被璇箟锛岄€傚悎鍋氭€婚噺瑙傛祴锛屼笉閫傚悎鐩存帴褰撻珮鐮佺巼绋冲畾鎬ч棬绂併€?


- 娌℃湁鎶婂綋鍓嶆挱鏀炬ā寮忕洿鎺ユ毚闇插埌 diagnostics锛屽鑷存瘡娆￠兘瑕佸洖鐪嬫棩蹇楁墠鑳界‘璁ゆ槸鍚﹀彂鐢熶簡 video-only 闄嶇骇銆?






### 瑙ｅ喅鏂规



- 璋冩暣 `PlayerCore::open()`锛?


  - 鏈夎棰戞祦鏃讹紝闊抽璁惧鍒濆鍖栧け璐ュ彧璁?warning锛屽苟缁х画浠?video-only 妯″紡鎵撳紑



  - 娌℃湁瑙嗛娴佹椂锛岄煶棰戣澶囧垵濮嬪寲澶辫触浠嶇洿鎺ヨ繑鍥炲け璐?


- 璋冩暣涓绘椂閽熼€夋嫨锛?


  - 鏈夐煶棰戣緭鍑烘椂浣跨敤 `Audio`



  - 鏃犻煶棰戣緭鍑轰絾鏈夎棰戞祦鏃朵娇鐢?`Video`



  - 鍙湁閮戒笉鍙敤鏃舵墠鍥為€€ `System`



- 鎵╁睍 `DiagnosticsSnapshot` 涓?CLI 杈撳嚭锛屾柊澧?`audio_output_initialized / video_only_fallback / clock_source`銆?


- 灏?`1080p60-check`銆乣high-bitrate-check`銆乣long-playback-check` 鐨?demux 闂ㄧ鏀逛负 `demux_queue_drop_packets == 0`锛屽苟琛ュ厖鎵撳嵃 `demux_ignored_packets / demux_queue_drop_packets`銆?


- 宸查噸鏂版墽琛?`MSBuild`銆乣--1080p60-check`銆乣--high-bitrate-check`銆乣--long-playback-check`銆乣--performance-log-check`锛涘綋鍓嶉€氳繃銆俙--4k-playback-check` 浠嶅墿 `fallback_ok` 瀛愯繘绋嬭矾寰勫緟鍚庣画澶勭悊銆?






### 淇敼鏂囦欢



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/main.cpp`



- `docs/analysis/PLAYERCORE_DAY7_AUDIO_FALLBACK_AND_REGRESSION_GATES.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- docs/records/VERSION.md







## 闂 69: PlayerCore 鍋滄挱鏀跺彛銆佸寘闃熷垪鎵€鏈夋潈涓?Clock/Demuxer 璁捐鍊轰慨澶?






**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 鍦ㄤ笂涓€杞鏌ヤ腑鍙戠幇锛宍PlayerCore` 鐨?EOF 鑷姩鍋滄挱鍜岄儴鍒?UI 鍋滄挱璺緞鍙慨鏀逛簡鎾斁鐘舵€侊紝娌℃湁瀹屾暣鏀跺彛 demux/audio/scheduler 绾跨▼锛屽瓨鍦ㄢ€滅姸鎬佸凡鍋溿€佺嚎绋嬫湭娓呪€濈殑鍙噸鍚闄┿€?


- `PacketQueue` 浠嶄互 `AVPacket*` 鍘熷鎸囬拡鎵胯浇鎵€鏈夋潈锛宍flush/clear/reset` 璺緞浼氶仐鐣欐湭閲婃斁鍘嬬缉鍖呫€?


- `Clock` 鍦ㄧ郴缁熸椂閽熻矾寰勪笂鐨?`pause()` / `setSpeed()` 鍩哄噯鏇存柊涓嶆纭紝绾棰戞垨 system-clock 璺緞浼氬嚭鐜版椂闂磋烦鍙橈紱`Demuxer::open()` 鎸侀攣璋冪敤 `close()` 杩樺瓨鍦ㄨ嚜閿侀闄┿€?






### 鍘熷洜鍒嗘瀽



- EOF 鑷姩鍋滄挱鍙戠敓鍦?scheduler 鐨?render 绾跨▼鍐咃紝涓嶈兘鐩存帴璧板悓姝?`stop()`锛屽惁鍒欎細瑙﹀彂绾跨▼鑷?join锛涙棫瀹炵幇鍥犳閫€鍖栨垚鈥滃彧鏀?`state_`鈥濄€?


- `ThreadSafeQueue<AVPacket*>` 鍙鐞嗘寚閽堟惉杩愶紝涓嶇鐞?FFmpeg 鍖呯敓鍛藉懆鏈燂紝瀵艰嚧 `clear()` 涓庨槦鍒楁瀽鏋勪笉浼氶噴鏀鹃槦鍒椾腑鍓╀綑鍖呫€?


- `Clock::pause()` 鍏堝啓 `paused_` 鍐嶈褰撳墠鏃堕棿锛宍Clock::setSpeed()` 涔熸病鏈夊厛鍥哄寲鏃у熀鍑嗭紝瀵艰嚧 system clock 杩炵画鎬ц鐮村潖銆?


- `Demuxer::open()` 鍦ㄥ凡鎸佹湁 `mutex_` 鏃跺啀娆¤皟鐢ㄥ悓鏍蜂細涓婇攣鐨?`close()`锛屾帴鍙ｅ鐢ㄦ椂浼氬崱姝汇€?






### 瑙ｅ喅鏂规



- 涓?`PlayerCore` 澧炲姞 deferred stop 鏀跺彛璺緞锛欵OF 鍦?render 绾跨▼鍐呭彧鍙戝嚭寮傛鍋滄満璇锋眰骞舵爣璁板緟娓呯悊锛屽悗缁敱瀹夊叏绾跨▼鎵ц鐪熷疄 `stop/join/flush`锛屽悓鏃朵慨澶?`next/previous/quit` 璇锋眰鐩存帴璧板畬鏁?`stop()`銆?


- 鎶?`PacketQueue` 鏀逛负 `ThreadSafeQueue<unique_ptr<AVPacket, AvPacketDeleter>>`锛屽苟璁?`ThreadSafeQueue::push()` 鏀寔寤惰繜 move锛屽交搴曟妸 FFmpeg 鍖呯敓鍛藉懆鏈熸敹鍥炲埌 RAII銆?


- 涓?`Scheduler` 澧炲姞寮傛鍋滄満鍏ュ彛骞跺湪閲嶆柊 `start()` 鍓嶅洖鏀跺凡閫€鍑?worker锛岄伩鍏?EOF 鍚庝笅涓€娆″惎鍔ㄨЕ鍙?`std::terminate`銆?


- 淇 `Clock` 鐨?pause/speed 鍩哄噯鏇存柊閫昏緫锛屼繚璇?system-clock 璺緞鐨勬殏鍋滃拰鍙橀€熸椂闂磋繛缁紱`Demuxer::open()` 鏀逛负鍦ㄥ悓涓€閿佸煙鍐呯洿鎺ュ叧闂棫杈撳叆锛屼笉鍐嶉噸鍏?`close()`銆?


- 宸查噸鏂版墽琛?`MSBuild` 鍏ㄩ噺閲嶅缓锛歚& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`锛屽綋鍓嶇粨鏋滀负 `0 涓鍛?/ 0 涓敊璇痐銆?






### 淇敼鏂囦欢



- `include/core/player_core.h`



- `include/core/scheduler.h`



- `include/thread_safe_queue.h`



- `src/core/player_core.cpp`



- `src/core/scheduler.cpp`



- `src/core/clock.cpp`



- `src/demuxer.cpp`



- `docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂 68: MSVC warning debt 鍒嗗眰娓呯悊锛圕4819 / C4996 / C4706锛?






**鏃ユ湡**: 2026-03-18







### 闂鎻忚堪



- GitHub Actions 鐨?Windows `Debug` 鏋勫缓铏界劧宸茬粡鑳介€氳繃涓绘祦绋嬬紪璇戯紝浣嗛暱鏈熸畫鐣欏ぇ閲?warning锛屽奖鍝?CI 鍙鎬у拰鍚庣画鍥炲綊瀹氫綅銆?


- `C4819` 涓昏鏉ヨ嚜 MSVC 浠ユ湰鍦颁唬鐮侀〉璇诲彇 UTF-8 婧愭枃浠讹紱`C4996` 鍚屾椂鍑虹幇鍦ㄧ涓夋柟澶存枃浠跺拰椤圭洰鏈湴浠ｇ爜锛涘彟鏈夊皯閲忔湰鍦?`C4100 / C4706` 鎻愮ず銆?


- 闇€瑕佸厛鎸夆€滅紪鐮佸憡璀︺€佺涓夋柟 warning銆佹湰鍦?warning鈥濅笁灞傛媶寮€娌荤悊锛岄伩鍏嶇畝鍗曞叏灞€闈欓粯鎶婄湡瀹為棶棰樹竴璧峰悶鎺夈€?






### 鍘熷洜鍒嗘瀽



- 鏈湴婧愮爜涓瓨鍦ㄤ腑鏂囨敞閲婂拰 UTF-8 鍐呭锛屼絾 MSVC 榛樿骞朵笉鎸?UTF-8 瑙ｉ噴婧愭枃浠讹紝瑙﹀彂 `C4819`銆?


- FFmpeg / Quill 澶存枃浠跺睘浜庣涓夋柟渚濊禆锛屼笉搴旇瑕佹眰椤圭洰閫氳繃鏀圭涓夋柟婧愮爜鏉ユ秷闄?warning锛屾洿鍚堥€傜殑鍔炴硶鏄妸瀹冧滑闅旂鍒板閮ㄥご鏂囦欢 warning 灞傘€?


- 鏈湴 `logger / subtitle / player_core / main` 涓粛淇濈暀浜嗚嫢骞插彲鐩存帴淇帀鐨勫畨鍏ㄥ嚱鏁颁笌琛ㄨ揪寮?warning銆?






### 瑙ｅ喅鏂规



- 鍦?`CMakeLists.txt` 涓负 MSVC 鐩爣澧炲姞 `/utf-8 /external:anglebrackets /external:W0`锛岃鏈湴婧愮爜鎸?UTF-8 缂栬瘧锛屽苟鎶婄涓夋柟 angle-bracket 澶存枃浠?warning 涓嬮檷鍒板閮ㄥ眰澶勭悊銆?


- 鍦?`src/logger.cpp` 涓柊澧炲畨鍏ㄧ幆澧冨彉閲忚鍙?helper锛岀敤 `_dupenv_s` 鏇挎崲 Windows 涓嬬殑 `std::getenv` 鐢ㄦ硶銆?


- 鍦?`src/subtitle/srt_parser.cpp` 鍜?`src/subtitle/ass_parser.cpp` 涓噰鐢ㄢ€淲indows 璧?`sscanf_s`锛屽叾浠栧钩鍙颁繚鐣?`std::sscanf`鈥濈殑璺ㄥ钩鍙拌В鏋愬垎鏀紝娓呯悊鏈湴 `C4996` 鑰屼笉鐮村潖鍙Щ妞嶆€с€?


- 鍦?`src/main.cpp` 涓妸淇″彿澶勭悊鍙傛暟鏍囪涓?`[[maybe_unused]]`锛涘湪 `src/core/player_core.cpp` 涓妸 demux push 閲嶈瘯閫昏緫鏀瑰啓涓烘樉寮忚祴鍊硷紝鍘绘帀鏉′欢琛ㄨ揪寮忓唴璧嬪€煎鑷寸殑 `C4706`銆?


- 閲嶆柊鎵ц `MSBuild` 鍏ㄩ噺閲嶅缓锛歚& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`锛屽綋鍓嶇粨鏋滀负 `0 涓鍛?/ 0 涓敊璇痐銆?






### 淇敼鏂囦欢



- `CMakeLists.txt`



- `src/logger.cpp`



- `src/main.cpp`



- `src/subtitle/srt_parser.cpp`



- `src/subtitle/ass_parser.cpp`



- `src/core/player_core.cpp`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂 67: ASS 鏍囩瑙ｆ瀽涓?UTF-16 瀛楀箷鑼冨洿淇







**鏃ユ湡**: 2026-03-18







### 闂鎻忚堪



- `ASS/SSA` override block 涓殑 `\fnArial`銆乣\rDefault` 涔嬬被绱у噾鍐欐硶浼氳閿欒璇嗗埆鎴愭爣绛惧悕 `fnArial`銆乣rDefault`锛屽鑷村父瑙佹牱寮忔爣绛惧け鏁堛€?


- `SubtitleTextRun.start/length` 涔嬪墠鎸?UTF-8 code point 璁℃暟锛屼絾 D3D11 瀛楀箷娓叉煋鏈€缁堢洿鎺ユ妸瀹冧滑浼犵粰 DirectWrite 鐨?`DWRITE_TEXT_RANGE`锛岄亣鍒?emoji 鎴栭潪 BMP 瀛楃鏃朵細浜х敓鑼冨洿閿欎綅銆?


- 闇€瑕佸湪鍓嶄竴杞?D3D11 鍘熺敓瀛楀箷閾炬彁浜ゅ悗锛屽啀鍋氫竴娆℃暣宸ョ▼鏋勫缓澶嶆牳骞舵妸褰撳墠缁撴灉鍚屾鍒拌褰曟枃妗ｃ€?






### 鍘熷洜鍒嗘瀽



- 鏃цВ鏋愰€昏緫鍦ㄨ鍙?override 鏍囩鍚嶆椂浼氭妸杩炵画瀛楁瘝鏁翠綋鍚炴帀锛岀己灏戝甯哥敤 ASS 鏍囩鍓嶇紑鐨勬樉寮忓尮閰嶃€?


- 瀛楀箷瑙ｆ瀽闃舵鍜屾覆鏌撻樁娈典娇鐢ㄤ簡涓嶅悓鐨勬枃鏈暱搴﹁涔夛細鍓嶈€呭亸鍚?UTF-8 code point锛屽悗鑰呭疄闄呴渶瑕?UTF-16 code unit銆?


- 杩欑被闂涓嶄細鐩存帴閫犳垚宕╂簝鎴栨硠婕忥紝浣嗕細鐮村潖 ASS/SSA 鏍峰紡瀛楀箷鍦ㄥ師鐢?D3D11 閾句腑鐨勮涔夋纭€с€?






### 瑙ｅ喅鏂规



- 鍦?`src/subtitle/ass_parser.cpp` 涓姞鍏ュ父鐢?ASS 鏍囩鐨勬樉寮忓墠缂€鍖归厤锛屾寜鏈€闀挎爣绛句紭鍏堣瘑鍒?`alpha / bord / shad / pos / fn / fs / an / 1c / 1a / c / a / b / i / u / s / r`锛屾湭鍛戒腑鏃跺啀鍥為€€鍒版棫鐨勫鏉捐В鏋愰€昏緫銆?


- 灏?`ASS/SSA` 鏂囨湰 run 闀垮害鍜岀函鏂囨湰瀛楀箷 fallback 璺緞缁熶竴鏀逛负鎸?UTF-16 code unit 璁℃暟锛屼娇 `SubtitleTextRun` 涓?DirectWrite `DWRITE_TEXT_RANGE` 璇箟涓€鑷淬€俙ass_parser.cpp` 鍐呴『鎵嬫竻鎺変簡鏈湴 `sscanf` / 灞€閮ㄥ彉閲忛伄钄藉憡璀︺€?


- 閲嶆柊鎵ц `MSBuild` 鍏ㄩ噺閲嶅缓锛歚& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`锛屽綋鍓嶇粨鏋滀负 `167 涓鍛?/ 0 涓敊璇痐銆?






### 淇敼鏂囦欢



- `src/subtitle/ass_parser.cpp`



- `src/render/d3d11_video_renderer.cpp`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---



## 闂 66: 鍏ㄥ眬鏋勫缓闃诲娓呯悊涓?ASS/SSA 鍘熺敓 D3D11 瀛楀箷閾?






**鏃ユ湡**: 2026-03-18







### 闂鎻忚堪



- 鍏ㄥ眬 `Debug|x64` 鏋勫缓鏇捐澶氬澶存枃浠?婧愭枃浠剁殑缂栫爜璇闃诲锛屽鑷粹€淒3D11 鍘熺敓閾句唬鐮佸凡鏀瑰ソ锛屼絾鏃犳硶鏁翠綋楠岃瘉鈥濄€?


- D3D11 鍘熺敓瀛楀箷閾炬鍓嶅彧瑕嗙洊绾枃鏈彔鍔狅紝`.ass/.ssa` 鐨勬牱寮忋€佸畾浣嶅拰澶氭潯鍚屾椂婵€娲诲瓧骞曡繕娌℃湁杩涘叆 native renderer銆?


- 澶栨寕瀛楀箷鑷姩鎺㈡祴涓庢樉寮忓姞杞介渶瑕佽鐩?`.ass`銆乣.ssa`銆乣.srt` 涓夌鏍煎紡锛岃€屼笉鏄彧闈㈠悜 SRT銆?






### 鍘熷洜鍒嗘瀽



- 閮ㄥ垎甯︿腑鏂囨敞閲婄殑澶存枃浠跺拰婧愭枃浠跺湪褰撳墠 MSVC/浠ｇ爜椤电粍鍚堜笅琚璇伙紝瑙﹀彂鍏ㄥ眬璇硶/鏍囪鍖栭敊璇紝鏋勬垚涓庝笟鍔￠€昏緫鏃犲叧鐨勬瀯寤洪樆濉炪€?


- 鏃у瓧骞曟ā鍨嬪彧鏈夌函鏂囨湰璇箟锛宍IVideoRenderer` 鎺ュ彛涔熷彧鎺ユ敹鍗曞瓧绗︿覆锛屽鑷?`PlayerCore` 鏃犳硶鎶?ASS/SSA 鐨勬牱寮忋€佸眰绾у拰瀹氫綅淇℃伅浼犵粰 D3D11 娓叉煋鍣ㄣ€?


- 鏃堕棿绾夸笂鍘熸湰鍙В鏋愬崟鏉℃椿鍔ㄥ瓧骞曪紝涓嶈冻浠ヨ〃杈?ASS/SSA 甯歌鐨勫 cue 鍚屽睆鍦烘櫙銆?






### 瑙ｅ喅鏂规



- 灏嗗彈褰卞搷鐨勫ご鏂囦欢鍜屾簮鏂囦欢鏀瑰啓涓?ASCII-safe 褰㈠紡锛屾仮澶?`MSBuild` 鍏ㄩ噺鏋勫缓鍩虹嚎銆?


- 鎵╁睍瀛楀箷鏁版嵁妯″瀷锛屾柊澧?`SubtitleStyle`銆乣SubtitleTextRun`銆乣SubtitleItem.layer/play_res/runs`锛屽苟璁╄В鏋愬伐鍘傛敮鎸?`.ass/.ssa/.srt`銆?


- 鏂板 `AssParser`锛岃В鏋?`Script Info / Styles / Events`锛岃鐩栧父鐢?`\b \i \u \s \fs \fn \an \a \pos \c/\1c \alpha/\1a \bord \shad \r` 鏍囩銆?


- 璁?`PlayerCore` 璁＄畻澶氭潯褰撳墠婵€娲诲瓧骞曪紝骞堕€氳繃 `IVideoRenderer::setSubtitleItems()` 鎶婄粨鏋勫寲瀛楀箷瀵硅薄鐩存帴閫佸叆 `D3D11VideoRenderer`銆?


- 鍦?`D3D11VideoRenderer` 鍚屼竴鍧?swap chain backbuffer 涓婂畬鎴?ASS/SSA 鏂囨湰濉厖銆佹弿杈广€侀槾褰便€佽儗鏅銆佸榻愬拰瀹氫綅缁樺埗锛涢潪 D3D11 娓叉煋鍣ㄩ粯璁ら€€鍖栦负绾枃鏈樉绀恒€?


- 鏇存柊 `main.cpp` 鐨勮嚜鍔ㄥ鎸傚瓧骞曟帰娴嬮『搴忎负 `.ass -> .ssa -> .srt`锛屽苟鍦ㄦ暣宸ョ▼绾у埆閲嶆柊楠岃瘉鏋勫缓閫氳繃銆?






### 淇敼鏂囦欢



- `CMakeLists.txt`



- `include/subtitle/subtitle_parser.h`



- `include/subtitle/ass_parser.h`



- `include/subtitle/srt_parser.h`



- `include/subtitle/subtitle_timeline.h`



- `src/subtitle/subtitle_parser.cpp`



- `src/subtitle/ass_parser.cpp`



- `src/subtitle/srt_parser.cpp`



- `src/subtitle/subtitle_timeline.cpp`



- `include/render/video_renderer.h`



- `include/render/d3d11_video_renderer.h`



- `src/render/d3d11_video_renderer.cpp`



- `include/core/player_core.h`



- `src/core/player_core.cpp`



- `src/video_player.cpp`



- `src/main.cpp`



- `include/config/settings_manager.h`



- `include/media/format_support.h`



- `include/playlist/playlist_manager.h`



- `include/plugin/plugin_api.h`



- `include/plugin/plugin_manager.h`



- `include/filters/filter_registry.h`



- `include/filters/video_filter_chain.h`



- `include/filters/audio_filter_chain.h`



- `include/streaming/http_stream_downloader.h`



- `include/streaming/hls_manifest_parser.h`



- `include/streaming/dash_manifest_parser.h`



- `include/streaming/adaptive_bitrate_selector.h`



- `include/display.h`



- `include/render/sdl_video_renderer.h`



- `include/render/opengl_video_renderer.h`



- `include/core/frame_queue.h`



- `src/streaming/adaptive_bitrate_selector.cpp`



- `src/render/renderer_factory.cpp`



- `src/ui/skin_engine.cpp`



- `src/core/scheduler.cpp`



- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



---



## 闂 57: D3D11 鍘熺敓 GPU 娓叉煋閾捐ˉ榻?






**鏃ユ湡**: 2026-03-18







### 闂鎻忚堪



- `D3D11VideoRenderer` 宸茬粡鍏峰鐙珛鐨?D3D11 瑙嗛鍛堢幇鑳藉姏锛屼絾瀛楀箷浠嶇劧鍙繚瀛樻枃鏈姸鎬侊紝娌℃湁鐪熸缁樺埗鍒板悓涓€鍧?swap chain backbuffer 涓娿€?


- 鏃у垎鏋愭枃妗ｄ粛鎶?D3D11 renderer 鎻忚堪涓?SDL 鍖呰鍣紝宸茬粡涓嶇鍚堝綋鍓嶄粨搴撶姸鎬併€?






### 鍘熷洜鍒嗘瀽



- 鍘熺敓瑙嗛涓婚潰涓?D3D11VA device sharing 宸茬粡钀藉湴锛屽墿浣欑己鍙ｉ泦涓湪瀛楀箷鍙犲姞杩欎竴鍧楁渶鍚庣殑 UI/overlay 鍚堟垚璺緞銆?


- 濡傛灉瀛楀箷浠嶄緷璧?SDL `Display` 鎴栬蒋浠剁汗鐞嗛摼锛屾暣鏉℃覆鏌撻摼灏变笉鑳界О涓衡€滃畬鏁淬€佺嫭绔嬨€佸師鐢?D3D11 GPU 閾捐矾鈥濄€?






### 瑙ｅ喅鏂规



- 鍦?`D3D11VideoRenderer` 鍐呮柊澧?D2D1 / DirectWrite 璧勬簮锛岀洿鎺ュ DXGI swap chain backbuffer 杩涜瀛楀箷鏂囨湰缁樺埗銆?


- 淇濈暀鐜版湁 D3D11 瑙嗛闈㈤噰鏍疯矾寰勶紝骞跺皢瀛楀箷缁樺埗涓插埌鍚屼竴甯?present 鍓嶃€?


- 瀵规殏鍋滄€佸瓧骞曞彉鍖栧鍔犲嵆鏃堕噸缁橈紝纭繚 seek / frame-step 鍚庡瓧骞曚笌瑙嗛鐘舵€佷竴鑷淬€?


- 鍚屾鏇存柊璁捐鏂囨。鍜屾棫鍒嗘瀽鏂囨。鐨勫巻鍙茶鏄庯紝閬垮厤鍚庣画缁х画娌跨敤杩囨湡缁撹銆?






### 淇敼鏂囦欢



- `src/render/d3d11_video_renderer.cpp`



- `src/render/renderer_factory.cpp`



- `CMakeLists.txt`



- `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`



- `docs/analysis/PLAYERCORE_DAY4_RENDERER_ANALYSIS.md`



- `docs/records/DEVELOP_LOG.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`



---







## 闂 12: 浼佷笟绾у绾跨▼鏋舵瀯閲嶆瀯







**鏃ユ湡**: 2026-02-27







### 闂鎻忚堪







鍘熸湁鏋舵瀯瀛樺湪浠ヤ笅闂锛?


1. 缁勪欢鑱岃矗涓嶆竻鏅帮紝VideoPlayer 鎵挎媴杩囧鑱岃矗



2. 绾跨▼妯″瀷澶嶆潅锛岄毦浠ョ淮鎶?


3. 鍐呭瓨绠＄悊瀹规槗鍑洪敊锛屽鑷村弻閲嶉噴鏀剧瓑 bug







### 瑙ｅ喅鏂规







閲嶆瀯涓轰紒涓氱骇澶氱嚎绋嬫灦鏋勶細







```



鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?


鈹?                   VideoPlayer (涓绘帶鍒跺櫒)                    鈹?


鈹溾攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?


鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?     鈹?


鈹? 鈹?Demuxer      鈹? 鈹?DecoderWorker鈹? 鈹?Display      鈹?     鈹?


鈹? 鈹?(瑙ｅ皝瑁呭櫒)    鈹? 鈹?(瑙ｇ爜宸ヤ綔绾跨▼)鈹? 鈹?(娓叉煋鍣?     鈹?     鈹?


鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?     鈹?


鈹?        鈹?                鈹?                鈹?              鈹?


鈹?        鈻?                鈻?                鈻?              鈹?


鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹屸攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?     鈹?


鈹? 鈹侾acketQueue   鈹? 鈹?Clock        鈹? 鈹?AudioPlayer  鈹?     鈹?


鈹? 鈹?(鍖呴槦鍒?     鈹? 鈹?(鏃堕挓鍚屾)   鈹? 鈹?(闊抽鎾斁)   鈹?     鈹?


鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹? 鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?     鈹?


鈹斺攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?


```







### 鏂板缁勪欢







1. **Demuxer (瑙ｅ皝瑁呭櫒)**



   - 灏佽 AVFormatContext 鐨勮鍙栨搷浣?


   - 鎻愪緵缁熶竴鐨?packet 璇诲彇鎺ュ彛



   - 鏀寔 seek 鎿嶄綔







2. **DecoderWorker (瑙ｇ爜宸ヤ綔绾跨▼)**



   - 灏佽鍗曚釜娴佺殑瑙ｇ爜閫昏緫



   - 浠?PacketQueue 鑾峰彇 packet锛岃В鐮佸悗閫氳繃鍥炶皟杈撳嚭



   - 鏀寔鏆傚仠/鎭㈠/flush







3. **ThreadSafeQueue (绾跨▼瀹夊叏闃熷垪)**



   - 閫氱敤鐨勭嚎绋嬪畨鍏ㄩ槦鍒楁ā鏉?


   - 鏀寔闃诲鍜岄潪闃诲鎿嶄綔



   - 鏀寔 EOF 淇″彿浼犻€?






4. **Clock (鏃堕挓鍚屾)**



   - 绠＄悊涓绘椂閽?


   - 璁＄畻闊宠棰戝悓姝ュ欢杩?


   - 鏀寔澶氱鍚屾妯″紡







### 淇敼鏂囦欢







- 鏂板 `include/demuxer.h`, `src/demuxer.cpp`



- 鏂板 `include/decoder_worker.h`, `src/decoder_worker.cpp`



- 鏂板 `include/thread_safe_queue.h`



- 鏂板 `include/clock.h`, `src/clock.cpp`



- 閲嶆瀯 `include/video_player.h`, `src/video_player.cpp`



- 淇敼 `CMakeLists.txt`



- 淇 `src/packet_reader.cpp` 鍙岄噸閲婃斁 bug







---







## 闂 11: 骞跺彂璇诲彇 AVFormatContext 瀵艰嚧宕╂簝







**鏃ユ湡**: 2026-02-27







### 闂鎻忚堪







鎾斁瑙嗛鏃跺嚭鐜板ぇ閲?FFmpeg 瑙ｇ爜閿欒鍜岃闂啿绐佸穿婧冿細



```



[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).



[h264 @ ...] missing picture in access unit with size 12342



0xC0000005: 鍐欏叆浣嶇疆 0x... 鏃跺彂鐢熻闂啿绐?


```







### 鍘熷洜鍒嗘瀽







瑙嗛瑙ｇ爜绾跨▼ (`VideoDecodeThread`) 鍜岄煶棰戣В鐮佺嚎绋?(`AudioDecodeThread`) 鍚勮嚜鎷ユ湁鐙珛鐨勮В鐮佸櫒瀹炰緥锛屼絾鍏变韩鍚屼竴涓?`AVFormatContext`銆備袱涓嚎绋嬪苟鍙戣皟鐢?`av_read_frame(format_ctx_, packet)` 瀵艰嚧鏁版嵁绔炰簤锛岃鍙栧埌鐨?packet 鏁版嵁閿欎贡锛屽紩鍙?H264 瑙ｇ爜閿欒鍜屽唴瀛樿闂啿绐併€?






### 瑙ｅ喅鏂规







寮曞叆缁熶竴鐨?`PacketReaderThread` 浣滀负鍞竴鐨?packet 璇诲彇鍏ュ彛锛?






1. **鏂板 PacketReaderThread 绫?*



   - 浣滀负鍞竴鐨?`av_read_frame()` 璋冪敤鐐?


   - 鏍规嵁 stream_index 灏?packet 鍒嗗彂鍒板搴旂殑 PacketQueue







2. **鏂板 PacketRef 鍜?PacketQueue 绫?*



   - `PacketRef`: 鍖呰 AVPacket 鐨勬櫤鑳界粨鏋勪綋锛屾敮鎸佺Щ鍔ㄨ涔?


   - `PacketQueue`: 绾跨▼瀹夊叏鐨?packet 闃熷垪锛屾敮鎸侀樆濉炵瓑寰?






3. **淇敼瑙ｇ爜鍣ㄦ帴鍙?*



   - `VideoDecoder` 鍜?`AudioDecoder` 鏂板 `decodePacket()` 鏂规硶



   - 鎺ユ敹澶栭儴浼犲叆鐨?packet锛岃€岄潪鍐呴儴璇诲彇







4. **閲嶆瀯瑙ｇ爜绾跨▼**



   - `VideoDecodeThread` 鍜?`AudioDecodeThread` 浠?`PacketQueue` 鑾峰彇 packet



   - 涓嶅啀鐩存帴璋冪敤 `av_read_frame()`







### 淇敼鏂囦欢







- 鏂板 `include/packet_reader.h`



- 鏂板 `src/packet_reader.cpp`



- 淇敼 `include/video_decoder.h`



- 淇敼 `src/video_decoder.cpp`



- 淇敼 `include/audio_decoder.h`



- 淇敼 `src/audio_decoder.cpp`



- 淇敼 `include/video_decode_thread.h`



- 淇敼 `src/video_decode_thread.cpp`



- 淇敼 `include/audio_decode_thread.h`



- 淇敼 `src/audio_decode_thread.cpp`



- 淇敼 `include/video_player.h`



- 淇敼 `src/video_player.cpp`



- 淇敼 `CMakeLists.txt`







---







## 闂 10: 澶氳В鐮佸櫒瀹炰緥绔炰簤璇诲彇瀵艰嚧瑙ｇ爜閿欒







## 闂 1: FFmpeg 8.0 鍏煎鎬ч棶棰?






**鏃ユ湡**: 2026-02-17







### 闂鎻忚堪







缂栬瘧鏃舵姤閿欙紝`codec_ctx_->avctx->priv_data` 鍦?FFmpeg 8.0 涓笉鍙敤銆?






### 鍘熷洜







FFmpeg 8.0 鏇存敼浜?API锛岀Щ闄や簡瀵?`avctx->priv_data` 鐨勭洿鎺ヨ闂€?






### 瑙ｅ喅鏂规







淇敼 `video_decoder.cpp` 鍜?`audio_decoder.cpp`锛?


- 鍦ㄨВ鐮佸櫒绫讳腑娣诲姞 `format_ctx_` 鎴愬憳鍙橀噺



- 鐩存帴浣跨敤浼犲叆鐨?format context 鑰岄潪浠?codec context 鑾峰彇







### 淇敼鏂囦欢







- `include/video_decoder.h`



- `include/audio_decoder.h`



- `src/video_decoder.cpp`



- `src/audio_decoder.cpp`







---







## 闂 2: 瑙嗛娴佺储寮曚笉鍖归厤







**鏃ユ湡**: 2026-02-24







### 闂鎻忚堪







鎾斁 mp4 鏂囦欢鏃讹紝瑙嗛鏃犳硶姝ｅ父鏄剧ず銆傛棩蹇楁樉绀猴細



```



decodeFrame: read packet, stream_index=0, expected=1



decodeFrame: packet stream mismatch, skipping



```



绋嬪簭寰幆 48 娆℃墠鑳借鍒版纭殑瑙嗛甯с€?






### 鍘熷洜







MP4 鏂囦欢鐨勬祦椤哄簭鏄細闊抽娴?绱㈠紩 0) 鍦ㄥ墠锛岃棰戞祦(绱㈠紩 1) 鍦ㄥ悗銆俙av_read_frame()` 杩斿洖鐨勫寘鍙兘鏄换鎰忔祦鐨勶紙閫氬父鏄涓€涓祦 - 闊抽娴侊級銆傚師浠ｇ爜閬囧埌涓嶅尮閰嶇殑娴佹椂鐩存帴杩斿洖 false锛屽鑷磋棰戝抚鏃犳硶瑙ｇ爜銆?






### 瑙ｅ喅鏂规







淇敼 `src/video_decoder.cpp` 鐨?`decodeFrame()` 鏂规硶锛?


- 灏嗛亣鍒颁笉鍖归厤娴佹椂杩斿洖 false锛屾敼涓?continue 璺宠繃璇ュ寘



- 寰幆璇诲彇鐩村埌鎵惧埌姝ｇ‘娴佺储寮曠殑鍖?






### 淇敼鏂囦欢







- `src/video_decoder.cpp`







### 浠ｇ爜鍙樻洿







```cpp



// 淇敼鍓?


if (packet->stream_index != stream_idx_) {



    av_packet_unref(packet);



    av_packet_free(&packet);



    return false;



}







// 淇敼鍚?


while (true) {



    ret = av_read_frame(format_ctx_, packet);



    // ...



    if (packet->stream_index != stream_idx_) {



        av_packet_unref(packet);



        continue;  // 缁х画寰幆璇诲彇



    }



    break;  // 鎵惧埌姝ｇ‘鐨勬祦



}



```







---







## 闂 3: 闊抽娴佺储寮曚笉鍖归厤







**鏃ユ湡**: 2026-02-24







### 闂鎻忚堪







涓庤棰戞祦绱㈠紩鐩稿悓鐨勯棶棰橈紝浣嗗嚭鐜板湪闊抽瑙ｇ爜鍣ㄤ腑銆?






### 鍘熷洜







鍚屾牱鐨勯棶棰橈細闊抽鍖呭彲鑳戒笉鏄涓€涓璇诲彇鐨勬祦銆?






### 瑙ｅ喅鏂规







淇敼 `src/audio_decoder.cpp` 鐨?`decodeFrame()` 鏂规硶锛屽簲鐢ㄤ笌瑙嗛瑙ｇ爜鍣ㄧ浉鍚岀殑淇銆?






### 淇敼鏂囦欢







- `src/audio_decoder.cpp`







---







## 闂 4: YUV 鏁版嵁娓叉煋閿欒







**鏃ユ湡**: 2026-02-24







### 闂鎻忚堪







瑙ｇ爜鎴愬姛鍚庣▼搴忕珛鍗抽€€鍑猴紝娌℃湁鐢婚潰鏄剧ず銆?






### 鍘熷洜







`renderFrame` 鍑芥暟浣跨敤閿欒鐨?YUV 鏁版嵁锛?


- 鍘熸潵浼犻€掔殑鏄?`frame->data[0]`锛堝彧鏄?Y 骞抽潰鎸囬拡锛?


- 鐒跺悗閿欒鍦板亣璁?Y/U/V 鏄繛缁瓨鍌ㄧ殑







瀹為檯涓?AVFrame 涓?Y/U/V 鏄垎寮€瀛樺偍鐨勶紝浣跨敤 `linesize` 鏉ヨ绠楁瘡琛岀殑姝ラ暱銆?






### 瑙ｅ喅鏂规







1. 浼犻€掓暣涓?AVFrame 鎸囬拡鑰屼笉鏄?`data[0]`



2. 姝ｇ‘浣跨敤 Y/U/V 骞抽潰鐨勬暟鎹拰琛屽ぇ灏?






### 淇敼鏂囦欢







- `src/display.cpp`



- `src/video_player.cpp`







### 浠ｇ爜鍙樻洿







```cpp



// video_player.cpp - 淇敼鍓?


display_->renderFrame(frame->data[0], frame->width, frame->height);







// video_player.cpp - 淇敼鍚?


display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);







// display.cpp - renderFrame 鍑芥暟



// 淇敼鍓?


int ret = SDL_UpdateYUVTexture(



    texture_, nullptr,



    data, width,



    data + width * height, width / 2,



    data + width * height * 5 / 4, width / 2



);







// 淇敼鍚?


AVFrame* frame = (AVFrame*)data;



int ret = SDL_UpdateYUVTexture(



    texture_, nullptr,



    frame->data[0], frame->linesize[0],



    frame->data[1], frame->linesize[1],



    frame->data[2], frame->linesize[2]



);



```







---







## 闂 5: 浼佷笟绾?Quill 鏃ュ織閫氶亾







**鏃ユ湡**: 2026-02-24







### 闂鎻忚堪







- 鏃ф棩蹇楃郴缁熷彧浣跨敤 `std::cout/std::cerr`锛屾棤娉曟弧瓒充紒涓氳褰曘€佸紓姝ュ啓鐩樹笌杞浆闇€姹傘€?


- 鏃犺繍琛屾椂閰嶇疆閫氶亾锛屾棤娉曟牴鎹幆澧冭皟鏁存棩蹇楃洰褰曘€佹枃浠跺ぇ灏忎笌绛夌骇闃堝€笺€?


- 缂轰箯鍋ュ．鎬э細鐩綍涓嶅彲鍐欐垨 Quill 鍒濆鍖栧け璐ユ椂娌℃湁鏄庣‘鍛婅涓庨檷绾ч€昏緫銆?






### 鍘熷洜鍒嗘瀽







- 涓鸿閬?Quill v6.x API 鍙樻洿鏇句复鏃剁鐢?Quill锛屽紩璧峰姛鑳藉€掗€€銆?


- Logger 閫昏緫闆嗕腑鍦ㄥご鏂囦欢瀹忓唴锛屾墿灞曠偣鏈夐檺锛屾柊澧為厤缃笌闄嶇骇璺緞鍥伴毦銆?






### 瑙ｅ喅鏂规







- 閲嶆柊鍚敤 Quill锛屾瀯寤哄紓姝?Backend + ConsoleSink + RotatingFileSink 鍙岄€氶亾锛涙棩蹇楁寜鐓?`[time][level][thread][logger][category] message` 缁熶竴鏍煎紡杈撳嚭銆?


- 鏂板 `LoggingConfigLoader`锛岃В鏋?`config/logging.conf` 鍙?`MVP_LOG_*` 鐜鍙橀噺锛岄潪娉曞€艰嚜鍔ㄧ籂姝ｅ苟杈撳嚭 `LOG_WARNING`銆?


- 褰?`USE_QUILL_LOGGING` 鏈畾涔夈€佺洰褰曚笉鍙啓鎴?Quill 鎶涘嚭寮傚父鏃讹紝鑷姩闄嶇骇鍒?stdout/stderr锛屽苟淇濈暀鏃у畯琛屼负銆?


- 鍚屾鏇存柊鏂囨。锛圠OGGING.md銆乂ERSION.md銆丆HANGELOG.md锛夊苟鎻愪緵榛樿閰嶇疆鏂囦欢銆?






### 淇敼鏂囦欢







- `include/logger.h`



- `src/logger.cpp`



- `config/logging.conf`



- `docs/design/LOGGING.md`



- `docs/records/CHANGELOG.md`



- `docs/records/VERSION.md`







---







## 闂 6: 澶氱嚎绋嬫挱鏀炬灦鏋勯噸鏋?






**鏃ユ湡**: 2026-02-25







### 闂鎻忚堪







- 鍘熸湁鏋舵瀯涓哄崟绾跨▼ playLoop锛岃В鐮佸拰娓叉煋鍦ㄥ悓涓€绾跨▼



- 瑙嗛瑙ｇ爜浼氶樆濉炴覆鏌撶嚎绋嬶紝瀵艰嚧鐢婚潰鍗￠】



- 闊宠棰戝悓姝ュ疄鐜板洶闅?


- 闃熷垪婊℃椂 CPU 蹇欒疆璇㈠鑷村崰鐢ㄨ繃楂?






### 瑙ｅ喅鏂规







1. **鏂板 FrameQueue 妯℃澘绫?*



   - 瀹炵幇绾跨▼瀹夊叏鐨勫抚闃熷垪



   - 浣跨敤 condition_variable 瀹炵幇闃诲绛夊緟锛岄伩鍏?CPU 蹇欒疆璇?


   - 鏀寔 push/pop/clear/stop 鎿嶄綔







2. **鏂板 VideoDecodeThread 鍜?AudioDecodeThread**



   - 鐙珛鐨勮棰?闊抽瑙ｇ爜绾跨▼



   - 瑙ｇ爜鍚庣殑甯ч€氳繃 FrameQueue 浼犻€掔粰娓叉煋绾跨▼



   - 鏀寔 pause/resume/flush 鎺у埗







3. **鏂板 SyncManager 鍚屾绠＄悊鍣?*



   - 鏀寔 AudioMaster/VideoMaster/Free 涓夌鍚屾妯″紡



   - 瀹炵幇甯у欢杩熻绠?


   - 瀹炵幇璺冲抚/閲嶅甯х瓥鐣?






4. **閲嶆瀯 VideoPlayer**



   - 浠庡崟绾跨▼ playLoop 鏀逛负澶氱嚎绋?renderLoop



   - 娣诲姞 setSyncMode 鏂规硶鏀寔鍚屾妯″紡鍒囨崲



   - 淇 AudioPlayer::play 绛惧悕涓嶅尮閰嶉棶棰?






### 淇敼鏂囦欢







- 鏂板 `include/frame_queue.h`



- 鏂板 `include/video_decode_thread.h`



- 鏂板 `include/audio_decode_thread.h`



- 鏂板 `include/sync_manager.h`



- 鏂板 `src/video_decode_thread.cpp`



- 鏂板 `src/audio_decode_thread.cpp`



- 鏂板 `src/sync_manager.cpp`



- 淇敼 `include/video_player.h`



- 淇敼 `include/audio_decoder.h`



- 淇敼 `src/video_player.cpp`



- 淇敼 `CMakeLists.txt`







---







## 闂 7: 闊抽鎾斁鏋舵瀯淇







**鏃ユ湡**: 2026-02-25







### 闂鎻忚堪







- AudioDecodeThread 瑙ｇ爜鍚庣殑闊抽閫氳繃 FrameQueue 浼犻€掔粰 renderLoop



- renderLoop 閫愬抚璋冪敤 AudioPlayer::play()锛屽鑷撮煶棰戞柇鏂画缁?


- SDL 鍥炶皟鏈哄埗闇€瑕佹寔缁殑鏁版嵁娴侊紝褰撳墠瀹炵幇鏃犳硶婊¤冻







### 瑙ｅ喅鏂规







1. 淇敼 AudioDecodeThread::start() 鏂规硶锛屽鍔?AudioPlayer* 鍙傛暟



2. 瑙ｇ爜绾跨▼瑙ｇ爜瀹屾垚鍚庯紝鐩存帴璋冪敤 audio_player_->play() 灏嗘暟鎹斁鍏?SDL 闃熷垪



3. 绉婚櫎 renderLoop 涓殑闊抽鎾斁浠ｇ爜锛岀敱瑙ｇ爜绾跨▼鐩存帴澶勭悊







### 淇敼鏂囦欢







- `include/audio_decode_thread.h`



- `src/audio_decode_thread.cpp`



- `src/video_player.cpp`







---







## 寰呰В鍐崇殑闂







### 闂 8: 纭欢鍔犻€熻В鐮佹敮鎸?






**鐘舵€?*: 寰呭疄鐜?






闇€瑕佹坊鍔?CUDA/D3D11VA 纭欢鍔犻€熻В鐮佹敮鎸侊紝鎻愬崌瑙ｇ爜鎬ц兘銆?






---







## 闂 9: VideoFrame/AudioFrame 绉诲姩璇箟缂洪櫡瀵艰嚧宕╂簝







**鏃ユ湡**: 2026-02-25







### 闂鎻忚堪







绋嬪簭鍚姩鎾斁鍚庣珛鍗冲穿婧冿紝閿欒淇℃伅锛?


```



modern-video-player.exe - 搴旂敤绋嬪簭閿欒



0x00007FFF7A80DA4C 鎸囦护寮曠敤浜?0xFFFFFFFFFFFFFFFF 鍐呭瓨銆傝鍐呭瓨涓嶈兘涓?read



```







### 鍘熷洜鍒嗘瀽







`VideoFrame` 鍜?`AudioFrame` 绫荤己灏戞纭殑绉诲姩璇箟瀹炵幇銆?






鍦?`FrameQueue::pop()` 涓娇鐢?`std::move` 灏嗗抚绉诲姩鍑洪槦鍒楋細



```cpp



frame = std::move(queue_.front());



queue_.pop();



```







鐢变簬娌℃湁瀹氫箟绉诲姩鏋勯€犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃锛岄粯璁ょ殑绉诲姩鎿嶄綔鍙槸娴呮嫹璐?`frame_` 鎸囬拡銆傚綋鍘熷璞℃瀽鏋勬椂璋冪敤 `av_frame_free(&frame_)` 閲婃斁鍐呭瓨锛岀洰鏍囧璞＄殑 `frame_` 鍙樻垚鎮┖鎸囬拡銆傛覆鏌撳惊鐜闂鎮┖鎸囬拡鏃跺鑷村穿婧冦€?






### 瑙ｅ喅鏂规







1. 涓?`VideoFrame` 绫绘坊鍔犵Щ鍔ㄦ瀯閫犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃



2. 涓?`AudioFrame` 绫绘坊鍔犵Щ鍔ㄦ瀯閫犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃



3. 鏄惧紡鍒犻櫎鎷疯礉鏋勯€犲嚱鏁板拰鎷疯礉璧嬪€艰繍绠楃



4. 绉诲姩鏃跺皢鍘熷璞＄殑 `frame_` 鎸囬拡缃负 nullptr锛岄槻姝㈡瀽鏋勬椂閲婃斁浠嶈浣跨敤鐨勫唴瀛?






### 淇敼鏂囦欢







- `include/video_decoder.h`



- `src/video_decoder.cpp`



- `include/audio_decoder.h`



- `src/audio_decoder.cpp`







### 浠ｇ爜鍙樻洿







```cpp



// video_decoder.h - 娣诲姞绉诲姩璇箟澹版槑



VideoFrame(VideoFrame&& other) noexcept;



VideoFrame& operator=(VideoFrame&& other) noexcept;



VideoFrame(const VideoFrame&) = delete;



VideoFrame& operator=(const VideoFrame&) = delete;







// video_decoder.cpp - 瀹炵幇绉诲姩鏋勯€犲嚱鏁?


VideoFrame::VideoFrame(VideoFrame&& other) noexcept



    : frame_(other.frame_)



    , pts_(other.pts_) {



    other.frame_ = nullptr;



}







// audio_decoder.h/cpp - 绫讳技瀹炵幇



```







---







## 闂 10: 澶氳В鐮佸櫒瀹炰緥绔炰簤璇诲彇瀵艰嚧瑙ｇ爜閿欒







**鏃ユ湡**: 2026-02-25







### 闂鎻忚堪







鎾斁瑙嗛鏃跺嚭鐜板ぇ閲?FFmpeg 瑙ｇ爜閿欒锛?


```



[h264 @ ...] Invalid NAL unit size (0 > 1045).



[h264 @ ...] missing picture in access unit with size 1049



[aac @ ...] channel element 0.0 duplicate



[mov,mp4,m4a,3gp,3g2,mj2 @ ...] DTS 2625751 < 2632415 out of order



```







### 鍘熷洜鍒嗘瀽







鍦?`VideoPlayer::open()` 涓垱寤轰簡 `video_decoder_` 鍜?`audio_decoder_` 瑙ｇ爜鍣ㄥ疄渚嬬敤浜庤幏鍙栬棰戜俊鎭€傜劧鍚庡湪 `play()` -> `initDecodeThreads()` 涓張鍒涘缓浜?`VideoDecodeThread` 鍜?`AudioDecodeThread`锛岃繖涓や釜绫诲唴閮ㄥ張鍚勮嚜鍒涘缓浜嗘柊鐨勮В鐮佸櫒瀹炰緥銆?






涓や釜瑙ｇ爜鍣ㄥ悓鏃朵粠鍚屼竴涓?`AVFormatContext` 璇诲彇 packet锛岄€犳垚鏁版嵁绔炰簤锛屽鑷磋В鐮侀敊璇拰鏁版嵁鎹熷潖銆?






### 瑙ｅ喅鏂规







鍦?`play()` 鏂规硶涓紝璋冪敤 `initDecodeThreads()` 涔嬪墠锛屽厛鍏抽棴骞堕噴鏀?`video_decoder_` 鍜?`audio_decoder_`锛?






```cpp



void VideoPlayer::play() {



    // ...



    playing_.store(true);



    



    video_decoder_.reset();



    audio_decoder_.reset();



    



    if (!initDecodeThreads(format_ctx_, video_stream_idx_, audio_stream_idx_)) {



        // ...



    }



}



```







### 淇敼鏂囦欢







- `src/video_player.cpp`







---







## 鐩稿叧鏂囨。







- [VERSION.md](./VERSION.md) - 鐗堟湰璁板綍



- [ARCHITECTURE.md](../design/ARCHITECTURE.md) - 鏋舵瀯璁捐



- [WINDOWS_SETUP.md](../guides/WINDOWS_SETUP.md) - Windows 閰嶇疆鎸囧崡



- [LOGGING.md](../design/LOGGING.md) - 鏃ュ織绯荤粺璇存槑







---







## 闂 13: Core API + Scheduler + Filter 澶氱嚎绋嬮噸鏋勮惤鍦?






**鏃ユ湡**: 2026-03-06







### 闂鎻忚堪



- 闇€瑕佹寜瑙勬牸寮曞叆 Core API銆丼cheduler 鍜?Filter 鎻掍欢妗嗘灦锛屽苟淇濇寔 `VideoPlayer` 澶栭儴鎺ュ彛绋冲畾銆?






### 鍘熷洜鍒嗘瀽



- 鏃ф灦鏋勪互 `VideoPlayer` 鑱氬悎澶ч儴鍒嗚亴璐ｏ紝缂哄皯鍙紨杩涚殑鏍稿績灞備笌璋冨害灞傘€?


- 缂哄皯 `USE_NEW_PLAYER_CORE` 鍙楁帶杩佺Щ璺緞涓嬬殑鏂版牳蹇冨疄鐜般€?






### 瑙ｅ喅鏂规



- 鏂板 `core` 妯″潡锛歚PlayerCore`銆乣Scheduler`銆乣FrameQueue`銆乣Clock`銆乣Command`銆乣Frame`銆?


- 鏂板 `filters` 妯″潡锛氳繃婊ゅ櫒鎺ュ彛銆佹敞鍐屼腑蹇冦€佸鐞嗙閬撱€佷寒搴?瀵规瘮搴?楗卞拰搴﹀唴缃护闀溿€?


- `VideoPlayer` 鏀归€犱负鍙岃矾寰勶細`USE_NEW_PLAYER_CORE=ON` 璧版柊鏍稿績锛孫FF 淇濇寔鏃у疄鐜般€?


- 鏂板 `tests/core_frame_queue_tests.cpp`銆乣tests/core_clock_tests.cpp`銆?






### 淇敼鏂囦欢



- include/core/frame.h



- include/core/frame_queue.h



- include/core/clock.h



- include/core/command.h



- include/core/scheduler.h



- include/core/player_core.h



- src/core/frame.cpp



- src/core/clock.cpp



- src/core/scheduler.cpp



- src/core/player_core.cpp



- include/filters/video_filter.h



- include/filters/audio_filter.h



- include/filters/filter_registry.h



- include/filters/filter_pipeline.h



- include/filters/builtin_filters.h



- src/filters/filter_registry.cpp



- src/filters/filter_pipeline.cpp



- src/filters/brightness_filter.cpp



- src/filters/contrast_filter.cpp



- src/filters/saturation_filter.cpp



- src/filters/builtin_filters.cpp



- include/video_player.h



- src/video_player.cpp



- CMakeLists.txt



- tests/core_frame_queue_tests.cpp



- tests/core_clock_tests.cpp







---







## 闂 14: 鏋舵瀯鏀舵暃涓?Core 鍗曡矾寰?






**鏃ユ湡**: 2026-03-06







### 闂鎻忚堪



- 鍘嗗彶涓婂苟瀛樼殑鏂版棫鎾斁閾捐矾澧炲姞浜嗙淮鎶ゆ垚鏈拰琛屼负涓嶄竴鑷撮闄┿€?






### 鍘熷洜鍒嗘瀽



- 鏃ч摼璺拰鏂伴摼璺叡瀛樺鑷存帓闅滆矾寰勫鏉傦紝涓旀棫閾捐矾瀛樺湪缁撴瀯鎬у苟鍙戦殣鎮ｃ€?






### 瑙ｅ喅鏂规



- 鍒犻櫎鏃ч摼璺ā鍧楋紝缁熶竴鍒?`VideoPlayer -> PlayerCore -> Scheduler -> Queue -> Output`銆?


- 鏋勫缓绯荤粺鏀逛负浠呯紪璇戞柊鏍稿績妯″潡銆?


- 鏂板閲嶆瀯鏂囨。璇存槑淇濈暀鏂囦欢涓庤亴璐ｈ竟鐣屻€?






### 淇敼鏂囦欢



- CMakeLists.txt



- include/video_player.h



- src/video_player.cpp



- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md



- 鏃фā鍧楀ご婧愭枃浠跺垹闄わ紙瑙?DEVELOP_LOG 闂 14锛?






---







## 闂 15: 灏忓睆绐楀彛杩囧ぇ涓旀嫋鎷界缉鏀句笉绋冲畾







**鏃ユ湡**: 2026-03-06







### 闂鎻忚堪



- 灏忓睆璁惧鎾斁楂樺垎杈ㄧ巼瑙嗛鏃讹紝绐楀彛鍒濆灏哄杩囧ぇ锛屽奖鍝嶆搷浣溿€?


- 绐楀彛鎷栨嫿鍚庨儴鍒嗗満鏅笅娓叉煋鍖哄煙鏈強鏃舵洿鏂帮紝鐢ㄦ埛鎰熺煡涓衡€滅獥鍙ｄ笉鑳借皟鏁粹€濄€?






### 鍘熷洜鍒嗘瀽



- `Display::init()` 鐩存帴浣跨敤瑙嗛鍘熷鍒嗚鲸鐜囧垱寤虹獥鍙ｏ紝鏈寜灞忓箷鍙敤鍖哄煙鍋氶灞忕缉鏀俱€?


- 浜嬩欢澶勭悊浠呯洃鍚?`SDL_WINDOWEVENT_RESIZED`锛屾湭瑕嗙洊 `SDL_WINDOWEVENT_SIZE_CHANGED`銆?


- 娓叉煋鐩爣鍖哄煙鐩存帴浣跨敤绐楀彛瀹介珮锛岀己灏戞寜瑙嗛姣斾緥璁＄畻鐨勭洰鏍囩煩褰€?






### 瑙ｅ喅鏂规



- 鍚姩鏃堕€氳繃 `SDL_GetDisplayUsableBounds()` 璁＄畻鍙敤灞忓箷鍖哄煙锛屽皢鍒濆绐楀彛闄愬埗鍦ㄥ彲鐢ㄥ尯 90% 鍐呭苟淇濇寔瑙嗛姣斾緥銆?


- 鍚屾椂澶勭悊 `SDL_WINDOWEVENT_RESIZED` 涓?`SDL_WINDOWEVENT_SIZE_CHANGED`锛岀‘淇濈獥鍙ｅ昂瀵稿彉鍖栧疄鏃剁敓鏁堛€?


- 鎸夋簮瑙嗛姣斾緥璁＄畻 `SDL_RenderCopy` 鐨勭洰鏍囩煩褰紝閬垮厤鎷栨嫿鍚庣敾闈㈡媺浼搞€?






### 淇敼鏂囦欢



- src/display.cpp



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







## 闂 16: 鏈€澶у寲/缂╂斁鏃剁敾闈㈠崱浣忥紝骞惰ˉ鍏呭熀纭€浜や簰鎺у埗







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 鎾斁鏃舵渶澶у寲绐楀彛鎴栨嫋鍔ㄧ缉鏀剧獥鍙ｏ紝瑙嗛鐢婚潰鍙兘鍗′綇锛岄煶棰戠户缁挱鏀俱€?


- 缂哄皯杩涘害鏉°€侀煶閲忚皟鑺傘€佹嫋鍔ㄨ繘搴︽潯绛夊熀纭€浜や簰鑳藉姏銆?






### 鍘熷洜鍒嗘瀽



- SDL 绐楀彛浜嬩欢澶勭悊涓庢覆鏌撹皟鐢ㄥ垎鏁ｅ湪涓嶅悓绾跨▼璺緞锛岀獥鍙ｅ昂瀵稿彉鍖栨椂鏇村鏄撹Е鍙戠敾闈㈠埛鏂板仠婊炪€?


- 鏄剧ず灞傛病鏈夋帶鍒舵潯涓庨紶鏍囦氦浜掕姹備笂鎶ヨ兘鍔涖€?






### 瑙ｅ喅鏂规



- 璋冩暣浜嬩欢澶勭悊璺緞锛氬湪娓叉煋/绌洪棽娓叉煋璺緞涓疆璇?SDL 浜嬩欢锛屼富绾跨▼浠呮秷璐逛氦浜掕姹傘€?


- 涓?Display 娣诲姞鎺у埗灞傜粯鍒讹細杩涘害鏉°€侀煶閲忔潯銆佹殏鍋滅姸鎬佹彁绀恒€?


- 鏂板榧犳爣浜や簰锛氭嫋鍔ㄨ繘搴︽潯瑙﹀彂 seek锛屾嫋鍔ㄩ煶閲忔潯璋冭妭闊抽噺銆?


- PlayerCore 澧炲姞瀵?seek/闊抽噺璇锋眰鐨勬秷璐规墽琛屻€?






### 淇敼鏂囦欢



- include/display.h



- src/display.cpp



- src/core/player_core.cpp



- src/main.cpp



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







## 闂 17: 浼佷笟绾?MPC-HC 妯″潡楠ㄦ灦钀藉湴锛堥樁娈典簩/涓夋帹杩涳級







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 浼佷笟绾фā鍧楄鍒掑凡瀹氫箟锛屼絾澶氭暟妯″潡缂哄皯浠ｇ爜鍏ュ彛锛屾棤娉曠户缁苟琛屽紑鍙戙€?






### 鍘熷洜鍒嗘瀽



- 鏃у疄鐜颁互鏍稿績鎾斁閾捐矾涓轰富锛屾ā鍧楄竟鐣屼笉瀹屾暣锛岄毦浠ュ垎宸ユ帹杩涖€?






### 瑙ｅ喅鏂规



- 鏂板骞舵帴鍏ヤ紒涓氱骇鍩虹璁炬柦鍜屾ā鍧楅鏋讹細浠诲姟闃熷垪銆佸抚姹犮€佽В鐮佺嚎绋嬪熀绫汇€?


- 寮曞叆娓叉煋鎶借薄灞傦紙`IVideoRenderer` + `RendererFactory`锛夛紝骞惰 `PlayerCore` 鍒囨崲鍒版娊璞℃帴鍙ｃ€?


- 澧炲姞闊抽鍧囪　鍣?娣烽煶鍣ㄣ€佽В鐮佸櫒宸ュ巶銆佸瓧骞?SRT 瑙ｆ瀽銆佹挱鏀惧垪琛ㄣ€佽缃?蹇嵎閿€佺毊鑲ゃ€佹彃浠躲€佹牸寮忎笌娴佸獟浣撹В鏋愭ā鍧椼€?


- 瀹屽杽婊ら暅鍩虹被涓庨煶瑙嗛婊ら暅閾撅紝琛ラ綈闊抽噺骞宠　婊ら暅銆?


- 鍚屾鏇存柊 tasklist 瀵瑰簲宸插疄鐜伴」銆?






### 淇敼鏂囦欢



- CMakeLists.txt



- include/core/task_queue.h



- include/core/frame_pool.h



- src/core/frame_pool.cpp



- include/core/decoder_thread.h



- src/core/decoder_thread.cpp



- include/render/*



- src/render/*



- include/audio/*



- src/audio/*



- include/decoder/*



- src/decoder/*



- include/subtitle/*



- src/subtitle/*



- include/playlist/*



- src/playlist/*



- include/config/*



- src/config/*



- include/input/*



- src/input/*



- include/media/*



- src/media/*



- include/streaming/*



- src/streaming/*



- include/ui/*



- src/ui/*



- include/plugin/*



- src/plugin/*



- include/filters/filter_base.h



- include/filters/video_filter_chain.h



- src/filters/video_filter_chain.cpp



- include/filters/audio_filter_chain.h



- src/filters/audio_filter_chain.cpp



- include/filters/audio_filter.h



- include/filters/video_filter.h



- include/filters/builtin_filters.h



- src/filters/volume_balance_filter.cpp



- src/filters/builtin_filters.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/audio_player.h



- src/audio_player.cpp



- .monkeycode/specs/enterprise-quill-logging/tasklist.md



- docs/records/DEVELOP_LOG.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md







---







## 闂 18: DASH 瑙ｆ瀽缂栬瘧澶辫触涓庢牸寮忚兘鍔涚煩闃电己澶?






**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- `src/streaming/dash_manifest_parser.cpp` 鍦?MSVC 涓嬬紪璇戝け璐ワ紝闃诲鍏ㄩ噺鏋勫缓銆?


- 缂哄皯涓€涓彲鐩存帴澶嶇敤鐨勨€滆繍琛屾椂鏍煎紡鑳藉姏鐭╅樀鈥濆叆鍙ｏ紝涓嶅埄浜庡崟浜鸿凯浠ｄ腑蹇€熼獙璇佹牸寮忚鐩栥€?






### 鍘熷洜鍒嗘瀽



- 鍘熷瀛楃涓叉鍒欎娇鐢ㄤ簡榛樿鍒嗛殧绗︼紝琛ㄨ揪寮忎腑鍑虹幇 `)"` 瑙﹀彂鎻愬墠缁堟锛屽鑷磋娉曢敊璇€?


- 鐜版湁鏍煎紡鏀寔妯″潡铏芥湁鍩虹鎺ュ彛锛屼絾缂哄皯缁熶竴 CLI 妫€鏌ュ叆鍙ｄ笌涓诲姏鏍煎紡瑕嗙洊杈撳嚭銆?






### 瑙ｅ喅鏂规



- 淇 DASH 姝ｅ垯锛氭敼涓鸿嚜瀹氫箟 raw-string 鍒嗛殧绗︼紝鎭㈠ MSVC 缂栬瘧閫氳繃銆?


- 鎵╁睍 `FormatSupport`锛?


  - 澧炲姞杩愯鏃跺鍣?缂栬В鐮佸櫒鏋氫妇锛坄av_demuxer_iterate` / `av_codec_iterate`锛?


  - 澧炲姞鎾斁鐩爣璇勪及锛堥珮鍒嗚鲸鐜?楂樺抚鐜?澶氶煶閬擄級



- 鏀归€?`main`锛屾柊澧炲懡浠わ細



  - `--capabilities`



  - `--evaluate-target <width> <height> <fps> <audio_channels> <video_bitrate_mbps>`



- 澧炲己 `Demuxer` 涓?`PlayerCore` 闊抽閾捐矾绋冲仴鎬э紙澶氶煶閬撹緭鍑哄弬鏁板榻愩€侀噸閲囨牱鍣ㄥ鐢ㄧ瓑锛夈€?






### 淇敼鏂囦欢



- src/streaming/dash_manifest_parser.cpp



- include/media/format_support.h



- src/media/format_support.cpp



- include/demuxer.h



- src/demuxer.cpp



- include/audio_player.h



- src/audio_player.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md



- docs/README.md







---







## 闂 19: D3D11VA 纭В鏈€灏忛棴鐜笌杞В鍥為€€







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 闇€瑕佸湪 Windows 涓嬩紭鍏堝埄鐢?D3D11VA 纭В楂樺垎杈ㄧ巼/楂樺抚鐜囪棰戯紝骞剁‘淇濆け璐ユ椂鍙嚜鍔ㄥ洖閫€杞В銆?


- 纭欢瑙ｇ爜杈撳嚭閫氬父鏄?GPU 甯ф垨 `NV12`锛岀幇鏈?SDL 娓叉煋閾捐矾瑕佹眰 `YUV420P`锛屽瓨鍦ㄦ牸寮忎笉鍖归厤椋庨櫓銆?






### 瑙ｅ喅鏂规



- `PlayerCore` 澧炲姞 D3D11VA 灏濊瘯閫昏緫锛?


  - 妫€娴?codec 鐨?D3D11VA HW config锛?


  - 鍒涘缓 `AV_HWDEVICE_TYPE_D3D11VA` 璁惧涓婁笅鏂囷紱



  - 缁戝畾 `get_format` 鍥炶皟閫夋嫨纭欢鍍忕礌鏍煎紡銆?


- 鑻?`avcodec_open2` 鍦ㄧ‖瑙ｈ矾寰勫け璐ワ紝鑷姩閲嶅缓瑙ｇ爜涓婁笅鏂囧苟鍥為€€鍒拌蒋瑙ｃ€?


- 鏂板瑙嗛甯ц緭鍑鸿鏁撮摼璺細



  - 纭欢甯у厛 `av_hwframe_transfer_data` 杞埌绯荤粺鍐呭瓨锛?


  - 闈?`YUV420P` 甯х粺涓€缁?`sws_scale` 杞负 `YUV420P` 鍐嶈繘鍏ユ覆鏌撱€?






### 淇敼鏂囦欢



- include/core/player_core.h



- src/core/player_core.cpp







---







## 闂 20: 鎺㈡祴鍏ュ彛涓庢牸寮忓洖褰掕剼鏈惤鍦?






**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 闇€瑕佹妸鏍煎紡瑕嗙洊楠岃瘉浠庘€滄墜宸ユ墦寮€瑙嗛瑙傚療鈥濆崌绾т负鈥滃彲閲嶅鐨勫懡浠よ鍥炲綊鈥濄€?


- 鐜版湁鑳藉姏鍏ュ彛鍙湁鎬讳綋鑳藉姏璇勪及锛岀己灏戝崟鏂囦欢鎺㈡祴鍜屾壒閲忔牱鏈姤鍛娿€?






### 瑙ｅ喅鏂规



- 鍦?`main` 涓柊澧?`--probe-file <media_file>`锛氳緭鍑?`probe.*` 鏈哄櫒鍙瀛楁锛屽寘鍚鍣?瑙嗛/闊抽鐘舵€併€佸垎杈ㄧ巼銆佸抚鐜囥€佸０閬撲笌寤鸿淇℃伅銆?


- 鏂板 `tools/format_regression/run_format_regression.ps1`锛?


  - 璇诲彇 `tools/format_regression/format_samples.csv`锛?


  - 閫愪釜璋冪敤 `--probe-file`锛?


  - 鐢熸垚 `docs/reports/FORMAT_REGRESSION_*.md` 鎶ュ憡锛?


  - 杩斿洖鐮佽涔夛細`0=鍏ㄩ儴PASS`锛宍1=瀛樺湪PARTIAL`锛宍2=瀛樺湪FAIL`銆?


- 琛ュ厖 `docs/workflows/FORMAT_REGRESSION.md` 涓庢枃妗ｇ储寮曪紝渚夸簬鍦?VS2022/PowerShell 涓嬬洿鎺ユ墽琛屻€?






### 淇敼鏂囦欢



- src/main.cpp



- tools/format_regression/run_format_regression.ps1



- tools/format_regression/format_samples.csv



- docs/workflows/FORMAT_REGRESSION.md



- docs/README.md







---







## 闂 21: GitHub Actions 鑷姩鏍煎紡鍥炲綊鎺ュ叆







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 鍥炲綊閾捐矾铏藉彲鍦ㄦ湰鍦拌繍琛岋紝浣嗙己灏?PR/涓诲垎鏀嚜鍔ㄦ墽琛岋紝鏃犳硶鍦ㄦ彁浜ら樁娈靛強鏃舵嫤鎴牸寮忛€€鍖栥€?


- Windows CI 鐜涓庢湰鍦颁緷璧栧彂鐜版柟寮忎笉涓€鑷达紝闇€琛ラ綈鏋勫缓涓庤剼鏈吋瀹规€с€?






### 瑙ｅ喅鏂规



- 鏂板宸ヤ綔娴?`.github/workflows/format-regression.yml`锛?


  - 鍦?`windows-latest` 涓嬭浇 `SDL2/FFmpeg` 棰勭紪璇戝寘骞舵瀯寤?`Debug`锛?


  - 鎵ц `download_test_samples.ps1` 涓?`run_all_checks.ps1`锛?


  - 涓婁紶 `docs/reports/FORMAT_REGRESSION_CI.md` 鎶ュ憡浜х墿銆?


- 璋冩暣 `CMakeLists.txt`锛?


  - Windows 涓嬩紭鍏堣瘑鍒?`SDL2::`銆乣FFMPEG::` 涓?`unofficial::ffmpeg::` 瀵煎叆鐩爣锛?


  - 淇濈暀 `external/` 鐩綍鍥為€€閫昏緫锛屽吋瀹规湰鍦版棦鏈夋瀯寤恒€?


- 璋冩暣 `download_test_samples.ps1`锛?


  - `-FfmpegPath` 鏀寔 PATH 鍛戒护鍚嶏紙濡?`ffmpeg`锛夛紝渚夸簬 CI 鐩存帴璋冪敤銆?


- 鏇存柊鍥炲綊鏂囨。涓庝换鍔℃竻鍗曠姸鎬侊紝琛ラ綈鑷姩鍥炲綊鍏ュ彛璇存槑銆?






### 淇敼鏂囦欢



- .github/workflows/format-regression.yml



- CMakeLists.txt



- tools/download_test_samples.ps1



- docs/workflows/FORMAT_REGRESSION.md



- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md



- docs/README.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂 22: 鎾斁鍒楄〃涓婚摼璺€佽缃寔涔呭寲涓庡揩鎹烽敭棣栫増鎺ュ叆







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 鎾斁鍣ㄤ富娴佺▼浠呮敮鎸佸崟鏂囦欢锛屼笉鏀寔涓婁竴棣?涓嬩竴棣栦笌 EOF 鑷姩鍒囨崲銆?


- 璁剧疆妯″潡鏈帴鍏ヨ繍琛岄摼璺紝鍚姩/閫€鍑烘椂鏃犳硶鎭㈠闊抽噺鍜屾挱鏀鹃€熷害銆?


- 榛樿蹇嵎閿己澶卞叧閿兘鍔涳紙鐩稿 seek銆佸彉閫熴€侀潤闊炽€佹挱鏀惧垪琛ㄥ垏鎹級銆?






### 瑙ｅ喅鏂规



- 涓婚摼璺帴鍏?`PlaylistManager`锛?


  - 鏀寔鍛戒护琛屼紶鍏ュ涓獟浣撴枃浠讹紱



  - 鏀寔浼犲叆 `.m3u8` 浣滀负鎾斁鍒楄〃锛?


  - 鏀寔 `PageUp/PageDown` 涓婁竴棣?涓嬩竴棣栵紱



  - EOF 鑷姩鍒囨崲涓嬩竴椤广€?


- 涓婚摼璺帴鍏?`SettingsManager`锛?


  - 鍚姩鏃跺姞杞?`config/player_settings.ini`锛?


  - 缂哄け鎴栬В鏋愬け璐ユ椂鍥為€€榛樿鍊硷紙闊抽噺 100%銆侀€熷害 1.0x銆佹仮澶嶄笂娆＄储寮曪級锛?


  - 閫€鍑烘椂淇濆瓨褰撳墠闊抽噺銆佹挱鏀鹃€熷害鍜屾挱鏀惧垪琛ㄧ储寮曘€?


- 鎵╁睍 SDL 浜嬩欢鍒版挱鏀惧櫒鎺у埗閾捐矾锛?


  - `Left/Right` seek 卤5 绉掞紱



  - `Ctrl+Left/Ctrl+Right` seek 卤30 绉掞紱



  - `[`/`]` 璋冮€燂紝`R` 鎭㈠ 1.0x锛?


  - `M` 闈欓煶/鎭㈠锛?


  - `Enter/Alt+Enter/F` 鍏ㄥ睆鍒囨崲锛?


  - `Esc` 鍏ㄥ睆鎬侀€€鍏ㄥ睆锛岀獥鍙ｆ€侀€€鍑恒€?






### 淇敼鏂囦欢



- src/main.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/config/settings_manager.h



- src/config/settings_manager.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂 23: 绉婚櫎 Core 鍗曞厓娴嬭瘯鐩爣涓庢祴璇曟枃浠?






**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 褰撳墠浠撳簱淇濈暀浜嗕袱涓?Core 鐩稿叧娴嬭瘯鐩爣涓庢祴璇曟枃浠讹紝浣嗘湰娆￠渶姹傝姹傜Щ闄よ繖涓ら」娴嬭瘯鍐呭骞跺垹闄ゆ枃浠躲€?


- 鑻ヤ粎鍒犻櫎娴嬭瘯婧愮爜鑰屼笉娓呯悊鏋勫缓鑴氭湰锛屼細瀵艰嚧鏋勫缓閰嶇疆瀛樺湪鎮寕璺緞椋庨櫓銆?






### 瑙ｅ喅鏂规



- 浠?`CMakeLists.txt` 绉婚櫎锛?


  - `BUILD_CORE_TESTS` 閫夐」锛?


  - `core_frame_queue_tests`銆乣core_clock_tests` 涓や釜娴嬭瘯鐩爣锛?


  - `core_tests` 鑱氬悎鐩爣銆?


- 鍒犻櫎娴嬭瘯鏂囦欢锛?


  - `tests/core_frame_queue_tests.cpp`



  - `tests/core_clock_tests.cpp`



- 鍚屾鏇存柊鍙樻洿鏂囨。锛岀‘淇濊褰曚笌褰撳墠浠撳簱鐘舵€佷竴鑷淬€?






### 淇敼鏂囦欢



- CMakeLists.txt



- tests/core_frame_queue_tests.cpp锛堝垹闄わ級



- tests/core_clock_tests.cpp锛堝垹闄わ級



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 24: 澶栨寕瀛楀箷鍔犺浇鍏ュ彛锛圫RT锛夋帴鍏ヤ富娴佺▼







**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.1.1` 瑕佹眰鏀寔澶栨寕瀛楀箷鍔犺浇鍏ュ彛锛屼絾褰撳墠涓绘祦绋嬪彧鏈夎棰?闊抽鎾斁閾捐矾锛屾湭鎻愪緵澶栭儴瀛楀箷鏂囦欢鍏ュ彛銆?


- 椤圭洰宸插瓨鍦?`subtitle::SrtParser`锛屼絾鏈帴鍏?`VideoPlayer` 涓庡懡浠よ鍙傛暟銆?






### 瑙ｅ喅鏂规



- 鍦?`VideoPlayer` 澧炲姞澶栨寕瀛楀箷鍔犺浇鎺ュ彛锛?


  - `loadExternalSubtitle()` / `clearExternalSubtitle()`锛?


  - 鏀寔 `.srt` 鏂囦欢瑙ｆ瀽涓庡閿欐棩蹇楋紱



  - 鏆撮湶宸插姞杞藉瓧骞曡矾寰勪笌鏉＄洰鏁伴噺锛屼究浜庡悗缁覆鏌撴帴鍏ャ€?


- 鍦?`main` 澧炲姞鍛戒护琛屽叆鍙ｏ細



  - 鏂板 `--subtitle <file.srt>`锛?


  - 淇濇寔鐜版湁鎾斁鍒楄〃鍙傛暟閫昏緫锛?


  - 鏈樉寮忎紶鍙傛椂锛岃嚜鍔ㄥ皾璇曞姞杞戒笌濯掍綋鍚屽悕鐨?`.srt`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.1.1 澶栨寕瀛楀箷鍔犺浇鍏ュ彛` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 25: 瀛楀箷娓叉煋鍙犲姞涓庢挱鏀炬椂搴忓悓姝ユ帴鍏?






**鏃ユ湡**: 2026-03-07







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.1.2` 瑕佹眰瀛楀箷鍙覆鏌撳彔鍔犲埌鐢婚潰锛屼絾鐜版湁娓叉煋鎺ュ彛娌℃湁瀛楀箷鏂囨湰閫氶亾銆?


- 浠诲姟娓呭崟 `1.1.3` 瑕佹眰瀛楀箷涓庢挱鏀?鏆傚仠/seek 鍚屾锛屼絾涓绘挱鏀炬椂閽熼摼璺病鏈夊瓧骞曟椂闂磋酱鏇存柊閫昏緫銆?






### 瑙ｅ喅鏂规



- 鎵╁睍娓叉煋鎶借薄锛?


  - 鍦?`IVideoRenderer` 澧炲姞 `setSubtitleText()`锛?


  - SDL 娓叉煋鍣ㄨ浆鍙戝瓧骞曟枃鏈埌 `Display`锛?


  - D3D11/OpenGL 鍏堟彁渚涘吋瀹规々瀹炵幇锛屼繚鎸佹帴鍙ｄ竴鑷淬€?


- 鍦?`Display` 澧炲姞瀛楀箷鍙犲姞灞傦細



  - 鏂板瀛楀箷鐘舵€佸瓨鍌ㄤ笌绾跨▼瀹夊叏鏇存柊锛?


  - 鍦ㄨ棰戝抚娓叉煋鍚庛€佹帶鍒舵潯娓叉煋鍓嶇粯鍒跺瓧骞曢潰鏉匡紱



  - 鏀寔澶氳瀛楀箷銆佽秴闀挎埅鏂笌鍩虹鍙鎬ф牱寮忥紙闃村奖+鍗婇€忔槑搴曟澘锛夈€?


  - 褰撳墠浣跨敤杞婚噺瀛楁ā娓叉煋锛岄潪 ASCII 瀛楃浼氶檷绾ф樉绀恒€?


- 鍦?`PlayerCore` 澧炲姞瀛楀箷鏃堕棿杞撮┍鍔細



  - 鏂板澶栨寕瀛楀箷杞ㄩ亾鐘舵€佷笌绱㈠紩缂撳瓨锛?


  - 娓叉煋甯ц矾寰勪笌绌洪棽浜嬩欢璺緞鍧囪皟鐢?`updateSubtitleOverlay()`锛?


  - 鍩轰簬褰撳墠鎾斁鏃堕棿閫夋嫨娲昏穬瀛楀箷锛岃鐩栨挱鏀俱€佹殏鍋滀笌 seek 鍦烘櫙锛?


  - 淇閿佸唴璋冪敤娓叉煋鎺ュ彛鐨勯棶棰橈紝閬垮厤鍦ㄥ瓧骞曚簰鏂ラ攣鍐呰Е鍙戞覆鏌撳洖璋冦€?


- 璋冩暣 `VideoPlayer::open()` 瀛楀箷鐘舵€佸鐞嗭紝娑堥櫎鈥滃厛娓呯┖鍐嶅垽鏂姞杞解€濈殑鐭涚浘閫昏緫銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.1.2`銆乣1.1.3` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 26: 瀛楀箷寮€鍏虫帶鍒朵笌瀛楀箷鍔犺浇寮傚父澶勭悊瀹屽杽







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.1.4` 瑕佹眰瀛楀箷寮€鍏充笌寮傚父澶勭悊锛屼絾褰撳墠瀛楀箷浠呮敮鎸佲€滃姞杞藉悗鏄剧ず鈥濓紝缂哄皯杩愯鏃跺紑鍏炽€?


- 澶栨寕瀛楀箷鍔犺浇璺緞鍦ㄦ枃浠剁郴缁熷紓甯稿満鏅笅瀹归敊涓嶈冻锛屽奖鍝嶇ǔ瀹氭€ч鏈熴€?






### 瑙ｅ喅鏂规



- 澧炲姞瀛楀箷寮€鍏虫帶鍒堕摼璺紙鎸夐敭 `V`锛夛細



  - `Display` 鏂板瀛楀箷寮€鍏宠姹傦紱



  - `Renderer` 鎶借薄鏂板 `consumeToggleSubtitleRequest()`锛?


  - `PlayerCore` 鏂板瀛楀箷鏄剧ず鐘舵€佺鐞嗕笌鍒囨崲鎺ュ彛锛?


  - 鍏抽棴瀛楀箷鏃剁珛鍗虫竻绌哄彔鍔犲眰锛屽紑鍚椂鎸夊綋鍓嶆挱鏀炬椂闂存仮澶嶅悓姝ャ€?


- 澧炲己澶栨寕瀛楀箷寮傚父澶勭悊锛?


  - `VideoPlayer::loadExternalSubtitle()` 鏀逛负浣跨敤 `std::error_code` 璺緞妫€鏌ワ紱



  - 鎹曡幏瑙ｆ瀽鍣ㄥ紓甯稿苟闄嶇骇涓哄憡璀︽棩蹇楋紝涓嶄腑鏂挱鏀句富娴佺▼锛?


  - 淇濇寔鈥滃姞杞藉け璐ヨ嚜鍔ㄦ竻绌烘棫瀛楀箷鈥濈殑鐘舵€佷竴鑷存€с€?


- 鏇存柊甯姪淇℃伅锛岃ˉ鍏?`V - Toggle subtitles on/off`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.1.4` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 27: 蹇嵎閿厤缃寔涔呭寲鎺ュ叆锛坔otkey.*锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.3.2` 瑕佹眰鏀寔閿綅閰嶇疆鎸佷箙鍖栵紝浣嗗綋鍓嶅揩鎹烽敭閫昏緫鍥哄畾鍐欐鍦?`Display` 浜嬩欢鍒嗘敮涓€?


- `HotkeyManager` 浠呮湁楠ㄦ灦瀹炵幇锛屾湭鎺ュ叆涓绘挱鏀鹃摼锛屾棤娉曢€氳繃閰嶇疆鏂囦欢淇濇寔鑷畾涔夐敭浣嶃€?






### 瑙ｅ喅鏂规



- 鎵╁睍 `HotkeyManager`锛?


  - 瀵归綈褰撳墠棣栫増蹇嵎閿粯璁ゆ槧灏勶紙鎾斁銆乻eek銆侀煶閲忋€侀潤闊炽€佸彉閫熴€佸垏闆嗐€佸瓧骞曞紑鍏炽€佸叏灞忋€侀€€鍑猴級锛?


  - 澧炲姞閰嶇疆搴忓垪鍖栬兘鍔涳細`actionConfigKey`銆乣keyCodeToToken`銆乣keyCodeFromToken`銆?


- 灏嗗揩鎹烽敭鏄犲皠鎺ュ叆娓叉煋杈撳叆閾撅細



  - `Display` 浜嬩欢澶勭悊鏀逛负鐢?`HotkeyManager` 椹卞姩锛?


  - 淇濈暀 `Esc` 涓?`Enter` 鐨勫吋瀹硅涓猴紱



  - `Renderer`/`PlayerCore`/`VideoPlayer` 澧炲姞鐑敭绠＄悊閫忎紶鎺ュ彛銆?


- 鍦?`main` 鐨勮缃姞杞?淇濆瓨娴佺▼涓帴鍏?`hotkey.*`锛?


  - 鍚姩璇诲彇骞跺簲鐢?`player_settings.ini` 鐨?`hotkey.*`锛?


  - 闈炴硶閿綅閰嶇疆闄嶇骇涓洪粯璁ゅ苟璁板綍鍛婅锛?


  - 閫€鍑烘椂灏嗗綋鍓嶉敭浣嶅洖鍐欓厤缃紝瀹炵幇鎸佷箙鍖栥€?


- 鏇存柊榛樿閰嶇疆鏍蜂緥 `config/player_settings.ini`锛岃ˉ榻愬叏閮?`hotkey.*` 椤广€?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.3.2` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 28: 蹇嵎閿啿绐佹娴嬩笌鎭㈠榛樿鑳藉姏







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.3.3` 瑕佹眰鏀寔閿綅鍐茬獊妫€娴嬩笌鎭㈠榛樿銆?


- 鐜版湁 `hotkey.*` 鎸佷箙鍖栧凡鍙伐浣滐紝浣嗛噸澶嶉敭浣嶉厤缃細浜х敓鍔ㄤ綔鍐茬獊锛屼笖缂哄皯鈥滀竴閿洖鍒伴粯璁も€濊兘鍔涖€?






### 瑙ｅ喅鏂规



- 鎵╁睍 `HotkeyManager`锛?


  - 鏂板 `findConflicts()` / `hasConflicts()`锛岀敤浜庢娴嬮噸澶嶉敭浣嶇粦瀹氾紱



  - 鏂板 `resetToDefaults()`锛岀粺涓€鎭㈠榛樿閿綅鏄犲皠銆?


- 鍦ㄧ儹閿厤缃姞杞芥祦绋嬩腑鍔犲叆鍐茬獊娌荤悊锛?


  - 鍚姩鏃跺厛搴旂敤閰嶇疆鍒板€欓€夋槧灏勶紝鍐嶆墽琛屽啿绐佹娴嬶紱



  - 鑻ュ彂鐜板啿绐侊紝璁板綍鍐茬獊鍔ㄤ綔涓庨敭浣嶆棩蹇楋紝鑷姩鍥為€€榛樿閿綅锛?


  - 瀵归潪娉?token 淇濈暀榛樿骞惰緭鍑哄憡璀︺€?


- 鏂板鎭㈠榛樿寮€鍏筹細



  - 鍦?`player_settings.ini` 澧炲姞 `hotkey.restore_defaults`锛?


  - 璁剧疆涓?`true` 鍚庝笅娆″惎鍔ㄨ嚜鍔ㄦ仮澶嶉粯璁ゅ苟鍥炲啓涓?`false`銆?


- 鏇存柊甯姪杈撳嚭锛岃ˉ鍏呮仮澶嶉粯璁よ鏄庛€?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.3.3` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- src/main.cpp



- config/player_settings.ini



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 29: M1 楠屾敹 1.4.1锛圫RT seek 鍚屾鑷鍛戒护钀藉湴锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.4.1` 瑕佹眰鈥渀SRT 瀛楀箷鍙敤涓?seek 鍚庡悓姝鈥濓紝鐜版湁瀹炵幇缂哄皯鍙噸澶嶆墽琛岀殑楠屾敹鍏ュ彛銆?


- 鑻ュ彧渚濊禆浜哄伐鎾斁瑙傚療锛屽洖褰掓垚鏈珮涓旈毦浠ョǔ瀹氬鐜般€?






### 鍘熷洜鍒嗘瀽



- 瀛楀箷鏃堕棿杞村尮閰嶉€昏緫浠呭瓨鍦ㄤ簬 `PlayerCore` 鍐呴儴锛屾棤娉曞崟鐙獙璇佲€滈『搴忔挱鏀?+ seek 璺宠浆鈥濅袱绫诲満鏅€?






### 瑙ｅ喅鏂规



- 鎻愬彇鍏叡鏃堕棿杞村嚱鏁?`subtitle::resolveActiveSubtitleIndex(...)`锛屽苟鐢?`PlayerCore` 澶嶇敤銆?


- 鍦?`main` 澧炲姞 `--subtitle-sync-check <subtitle.srt>` 鍛戒护锛?


  - 椤哄簭鏃堕棿杞存鏌ワ紙ordered锛夛紱



  - 闈為『搴?seek 璺宠浆妫€鏌ワ紙seek锛夛紱



  - 杈撳嚭 `mismatches` 涓?`PASS/FAIL`銆?


- 鏂板鏍蜂緥瀛楀箷 `samples/subtitle/subtitle_seek_sync_sample.srt` 鍜屾湰鍦版姤鍛?`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`銆?


- 浠诲姟娓呭崟 `1.4.1` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- include/subtitle/subtitle_timeline.h



- src/subtitle/subtitle_timeline.cpp



- src/core/player_core.cpp



- src/main.cpp



- CMakeLists.txt



- samples/subtitle/subtitle_seek_sync_sample.srt



- samples/README.md



- docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 30: M1 楠屾敹 1.4.2锛堟挱鏀惧垪琛ㄨ繛缁挱鏀?5 鏂囦欢鑷锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.4.2` 瑕佹眰鈥滄挱鏀惧垪琛ㄨ繛缁挱鏀?5 鏂囦欢閫氳繃鈥濓紝浣嗙己灏戝彲閲嶅鎵ц鐨勯獙鏀跺懡浠ゃ€?


- 浠呴潬鎵嬪伐鐐瑰嚮楠岃瘉锛屽洖褰掓晥鐜囦綆涓旂粨鏋滀笉绋冲畾銆?






### 鍘熷洜鍒嗘瀽



- 褰撳墠涓绘祦绋嬪叿澶?EOF 鑷姩鍒囨崲閫昏緫锛屼絾娌℃湁缁撴瀯鍖栬緭鍑哄彲鐢ㄤ簬蹇€熼獙鏀躲€?






### 瑙ｅ喅鏂规



- 鍦?`main` 鏂板 `--playlist-flow-check` 鍛戒护锛?


  - 璇诲彇杈撳叆骞舵瀯寤烘挱鏀惧垪琛紱



  - 鏍￠獙鑷冲皯 5 鏉＄洰锛?


  - 瀵瑰墠 5 鏉℃墽琛屽彲鎵撳紑鎺㈡祴锛坄--probe-file` 鍚屾簮閫昏緫锛夛紱



  - 妯℃嫙 EOF 鑷姩鍒囨崲椤哄簭锛岄獙璇?`0,1,2,3,4` 杩炵画瑕嗙洊锛?


  - 杈撳嚭 `PASS/FAIL` 涓庡け璐ョ储寮曘€?


- 鏂板鏈湴鎶ュ憡 `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.4.2` 瀹屾垚銆?






### 淇敼鏂囦欢



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md



- samples/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 31: M1 楠屾敹 1.4.3锛堣缃噸鍚仮澶嶈嚜妫€锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `1.4.3` 瑕佹眰鈥滆缃噸鍚悗鍙仮澶嶁€濓紝浣嗙己灏戝彲閲嶅鎵ц鐨勯獙鏀跺叆鍙ｃ€?


- 浠呮墜宸ラ噸鍚獙璇侀毦浠ヨ鐩栧叧閿瓧娈碉紙闊抽噺銆侀€熷害銆佹仮澶嶆爣蹇椼€佹挱鏀惧垪琛ㄧ储寮曘€佸揩鎹烽敭锛夈€?






### 鍘熷洜鍒嗘瀽



- 涓绘祦绋嬪凡鏈?`loadAppSettings/saveAppSettings`锛屼絾娌℃湁鐙珛鍛戒护琛岃緭鍑虹敤浜庡洖褰掗獙鏀躲€?






### 瑙ｅ喅鏂规



- 鍦?`main` 鏂板 `--settings-persistence-check [settings_file]` 鍛戒护锛?


  - 鍐欏叆涓€缁勬祴璇曡缃紱



  - 閲嶆柊鍔犺浇骞堕€愰」鏍￠獙 volume/speed/resume/index/hotkey锛?


  - 杈撳嚭 `settings-persistence-check.result=PASS/FAIL`銆?


- 榛樿浣跨敤绯荤粺涓存椂鐩綍杩涜妫€鏌ワ紝涓嶆薄鏌撻」鐩唴 `config/player_settings.ini`銆?


- 鏂板鏈湴鎶ュ憡 `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`1.4.3` 瀹屾垚銆?






### 淇敼鏂囦欢



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 32: M2 2.1.2锛堝鍣ㄧ煩闃佃ˉ榻?mov/avi/m2ts锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.1.2` 鐩爣瑕佹眰瀹瑰櫒瑕嗙洊 `mp4/mkv/mov/avi/webm/flv/ts/m2ts`銆?


- 鐜版湁鏍煎紡鍥炲綊鏍锋湰浠呰鐩栦簡 `mp4/mkv/webm/flv/ts`锛岀己灏?`mov/avi/m2ts` 瀹炴祴闂幆銆?






### 鍘熷洜鍒嗘瀽



- 鍥炲綊鏍锋湰鍒楄〃涓庤嚜鍔ㄧ敓鎴愯剼鏈湭鍖呭惈 `mov/avi/m2ts` 杈撳嚭锛屽鑷村鍣ㄧ煩闃佃鐩栦笉瀹屾暣銆?






### 瑙ｅ喅鏂规



- 鎵╁睍鏍锋湰鐭╅樀 `format_samples.csv`锛屾柊澧烇細



  - `mov`锛坔264 + aac锛?


  - `avi`锛坔264 + mp3锛?


  - `m2ts`锛坔264 + ac3锛?


- 鎵╁睍 `tools/download_test_samples.ps1`锛?


  - 鏂板 `samples/mov`銆乣samples/avi`銆乣samples/m2ts` 鐩綍鐢熸垚锛?


  - 鏂板涓夌被瀹瑰櫒鏍锋湰鐢熸垚鍛戒护銆?


- 鏇存柊鏍锋湰鐩綍鏂囨。涓庡拷鐣ヨ鍒欙紝琛ラ綈 `.gitkeep`銆?


- 鎵ц鏈湴鍥炲綊骞舵洿鏂版姤鍛婏細`docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`銆?


- 浠诲姟娓呭崟 `2.1.2` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- samples/.gitignore



- samples/mov/.gitkeep



- samples/avi/.gitkeep



- samples/m2ts/.gitkeep



- samples/README.md



- docs/workflows/FORMAT_REGRESSION.md



- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 33: M2 2.1.3锛堣棰戠紪鐮佺煩闃佃ˉ榻?MPEG-2锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.1.3` 鐩爣涓鸿棰戠紪鐮佹敮鎸?`H.264/H.265/VP9/AV1/MPEG-2`銆?


- 鐜版湁鍥炲綊鏍锋湰宸茶鐩栧墠鍥涢」锛屼絾缂哄皯 `MPEG-2` 瑙嗛缂栫爜鏍锋湰锛岄獙鏀堕棴鐜笉瀹屾暣銆?






### 鍘熷洜鍒嗘瀽



- `format_samples.csv` 涓?`download_test_samples.ps1` 鏈寘鍚?`mpeg2video` 鏍锋湰銆?






### 瑙ｅ喅鏂规



- 鍦ㄥ洖褰掓牱鏈煩闃典腑鏂板 `mpeg2video` 鏉＄洰锛?


  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`



- 鍦ㄦ牱鏈敓鎴愯剼鏈腑鏂板 MPEG-2 鏍锋湰鐢熸垚娴佺▼锛?


  - 瑙嗛缂栫爜 `mpeg2video`锛?


  - 闊抽缂栫爜 `ac3`锛?


  - 瀹瑰櫒 `mpegts`銆?


- 杩愯鏈湴鏍煎紡鍥炲綊骞舵洿鏂版姤鍛娿€?


- 浠诲姟娓呭崟 `2.1.3` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 34: M2 2.1.4锛堥煶棰戠紪鐮佺煩闃佃ˉ榻?E-AC3/DTS/Vorbis/PCM锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.1.4` 鐩爣瑕佹眰闊抽缂栫爜瑕嗙洊 `AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`銆?


- 鐜版湁鍥炲綊鏍锋湰缂哄皯 `E-AC3/DTS/Vorbis/PCM` 瀹炴祴锛岄獙鏀堕棴鐜笉瀹屾暣銆?






### 鍘熷洜鍒嗘瀽



- 鍥炲綊鏍锋湰娓呭崟鍜岃嚜鍔ㄦ牱鏈敓鎴愯剼鏈湭瑕嗙洊涓婅堪鍥涚被闊抽缂栫爜銆?






### 瑙ｅ喅鏂规



- 鎵╁睍 `format_samples.csv` 鏂板鍥涙潯鏍锋湰锛?


  - `h264 + eac3 (mkv)`



  - `h264 + dts (mkv)`



  - `vp9 + vorbis (webm)`



  - `h264 + pcm_s16le (mov)`



- 鎵╁睍 `download_test_samples.ps1` 鐢熸垚娴佺▼骞朵慨澶?DTS 缂栫爜锛?


  - `dca` 缂栫爜浣跨敤 `-strict -2` 閫氳繃瀹為獙鐗规€ч檺鍒躲€?


- 鎵╁睍鍥炲綊鑴氭湰鍏煎绛変环缂栫爜鍚嶏細



  - `dts` <-> `dca`



  - `hevc` <-> `h265`



  - `pcm` <-> `pcm_*`



- 杩愯鏈湴鍥炲綊骞舵洿鏂版姤鍛娿€?


- 浠诲姟娓呭崟 `2.1.4` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- tools/format_regression/format_samples.csv



- tools/download_test_samples.ps1



- tools/format_regression/run_format_regression.ps1



- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 35: M3 3.1.1锛圖ecoderFactory 鎺ュ叆鐪熷疄鍒濆鍖栨祦绋嬶級







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `3.1.1` 瑕佹眰 `DecoderFactory` 鎺ュ叆鐪熷疄瑙ｇ爜鍒濆鍖栨祦绋嬨€?


- 鐜版湁閾捐矾涓紝`DecoderFactory` 鏈舰鎴愮粺涓€鐨勨€滃€欓€夊悗绔?-> 閫愪釜灏濊瘯 -> 澶辫触鍥為€€鈥濅富娴佺▼銆?






### 鍘熷洜鍒嗘瀽



- `DecoderFactory` 浠呮彁渚涒€滄渶浣冲悗绔€濋€夋嫨锛岀己灏戝€欓€夊簭鍒楁帴鍙ｃ€?


- `PlayerCore::initDecoders` 鐨勫垵濮嬪寲涓庡洖閫€閫昏緫鑰﹀悎鍦ㄥ眬閮ㄦ潯浠跺垎鏀腑锛屼笉鍒╀簬缁熶竴鎵╁睍銆?






### 瑙ｅ喅鏂规



- `DecoderFactory` 鏂板 `selectBackendOrder(codec_name, prefer_hardware)`锛岃緭鍑烘寜浼樺厛绾ф帓搴忕殑鍚庣鍊欓€夊簭鍒楋紝骞朵繚鐣欒蒋浠惰В鐮佸厹搴曘€?


- `PlayerCore::initDecoders` 鏀逛负鎸夊€欓€夊簭鍒楅€愪釜灏濊瘯鍒濆鍖栵細



  - 瀵规瘡涓€欓€夊悗绔噸寤哄苟閰嶇疆 `AVCodecContext`锛?


  - 鍚庣閰嶇疆澶辫触鎴?`avcodec_open2` 澶辫触鏃惰嚜鍔ㄥ垏鎹笅涓€涓€欓€夛紱



  - 鎴愬姛鍚庣粺涓€璁板綍鏈€缁堣В鐮佸悗绔€?


- `tryConfigureD3D11HardwareDecode` 璋冩暣涓虹函 D3D11 閰嶇疆鑱岃矗锛屼笉鍐嶅湪鍑芥暟鍐呭仛鍚庣绛栫暐鍐崇瓥銆?


- 浠诲姟娓呭崟 `3.1.1` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- include/decoder/decoder_factory.h



- src/decoder/decoder_factory.cpp



- src/core/player_core.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 36: M3 3.1.2锛圖3D11VA 鍒濆鍖栧け璐ュ洖閫€杞В鍏滃簳锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `3.1.2` 瑕佹眰 D3D11VA 鍒濆鍖栧け璐ユ椂鍙潬鍥為€€杞В銆?


- 鐜版湁閫昏緫鍦ㄥ儚绱犳牸寮忓崗鍟嗗け璐ュ満鏅笅浠呮棩蹇楁彁绀猴紝鏈樉寮忔洿鏂板悗绔姸鎬侊紝瀛樺湪鐘舵€佷笉涓€鑷撮闄┿€?






### 鍘熷洜鍒嗘瀽



- `selectVideoPixelFormat` 鍦ㄥ崗鍟嗕笉鍒?D3D11VA 鏍煎紡鏃朵細杩斿洖杞欢鏍煎紡锛?


- 浣嗘鍓嶆病鏈夊悓姝ュ垏鎹?`video_decoder_backend_` 涓庣‖浠跺儚绱犳牸寮忕姸鎬併€?






### 瑙ｅ喅鏂规



- 鍦?`PlayerCore::selectVideoPixelFormat` 涓ˉ鍏呮樉寮忚蒋瑙ｉ檷绾э細



  - `video_hw_pixel_fmt_ = AV_PIX_FMT_NONE`锛?


  - `video_decoder_backend_ = Software`銆?


- 鍦?`initDecoders` 鍚庣灏濊瘯閾捐矾涓ˉ鍏呪€淒3D11VA 鍗忓晢闃舵闄嶇骇杞В鈥濇棩蹇椼€?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`3.1.2` 瀹屾垚銆?






### 淇敼鏂囦欢



- src/core/player_core.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 37: M3 3.2.1锛圖3D11 娓叉煋鏈€灏忓彲鐢ㄩ摼璺級







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `3.2.1` 瑕佹眰 D3D11 娓叉煋鍏峰鏈€灏忓彲鐢ㄨ兘鍔涳紙`init/upload/present`锛夈€?


- 鐜版湁 `D3D11VideoRenderer` 涓烘々瀹炵幇锛屾棤娉曞垵濮嬪寲銆佹棤娉曟秷璐瑰抚锛屼篃鏃犳硶鍛堢幇銆?






### 鍘熷洜鍒嗘瀽



- 娓叉煋鍚庣鎺ュ彛宸插畾涔夛紝浣?D3D11 鍚庣鏈帴鍏ュ疄闄呮覆鏌撻摼璺紱



- 缂哄皯鈥滃綋鍓?SDL renderer 瀹為檯鍚庣鈥濈殑鍙娴嬭兘鍔涳紝鏃犳硶鍒ゅ畾鏄惁鐪熺殑璺戝湪 D3D11銆?






### 瑙ｅ喅鏂规



- 鍦?`Display` 涓柊澧烇細



  - 娓叉煋椹卞姩鍋忓ソ璁剧疆锛坄setPreferredRendererDriver`锛夛紱



  - 褰撳墠娓叉煋鍚庣瑙傛祴锛坄currentRendererDriver` / `isUsingRendererDriver`锛夈€?


- 瀹屾暣瀹炵幇 `D3D11VideoRenderer` 鏈€灏忓姛鑳斤細



  - `init` 鏃惰姹?`direct3d11` 椹卞姩骞跺垱寤烘覆鏌撻摼璺紱



  - 鎺ラ€?`renderFrame`锛堜笂浼狅級銆乣present`锛堝憟鐜帮級銆乣clear`銆佷簨浠朵笌鎺у埗璇锋眰閫忎紶銆?


- 鍒濆鍖栧悗鏍￠獙瀹為檯鍚庣锛?


  - 鑻ラ潪 `direct3d11/d3d11`锛宍init` 澶辫触骞惰褰曟棩蹇楋紝浜ょ敱涓婂眰鍥為€€ `SoftwareSDL`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`3.2.1` 瀹屾垚銆?






### 淇敼鏂囦欢



- include/display.h



- src/display.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 38: M3 3.3.2锛堟覆鏌撳け璐ラ檷绾т笉涓柇鎾斁锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `3.3.2` 瑕佹眰娓叉煋澶辫触鏃惰嚜鍔ㄩ檷绾т笖涓嶄腑鏂挱鏀俱€?


- 鐜版湁瀹炵幇缂哄皯鍙噸澶嶆墽琛岀殑鑷姩鍖栭獙鏀跺叆鍙ｏ紝闅句互绋冲畾楠岃瘉鍥為€€閾捐矾銆?






### 鍘熷洜鍒嗘瀽



- D3D11 鍒濆鍖栧け璐ュ満鏅鍓嶄富瑕佷緷璧栦汉宸ヨЕ鍙戯紝鏃犳硶浣滀负绋冲畾鍥炲綊椤广€?


- 缂哄皯娓叉煋鍚庣/瑙ｇ爜鍚庣鐨勮繍琛屾椂鍙娴嬪瓧娈碉紝涓嶄究浜庡懡浠ゅ寲楠岃瘉銆?






### 瑙ｅ喅鏂规



- 鎵╁睍娓叉煋鎺ュ彛锛屽鍔犲悗绔悕绉版煡璇紙`rendererBackendName`锛夈€?


- 鎵╁睍鎾斁鍣ㄥ澶栨帴鍙ｏ紝鏆撮湶褰撳墠娓叉煋鍚庣鍜岃В鐮佸悗绔悕绉般€?


- 鏂板鍛戒护 `--renderer-fallback-check <media_file>`锛?


  - 閫氳繃鐜鍙橀噺 `MVP_D3D11_DRIVER_HINT=software` 寮哄埗 D3D11 娓叉煋鍒濆鍖栧け璐ワ紱



  - 楠岃瘉涓婚摼璺槸鍚﹁嚜鍔ㄥ洖閫€鍒?`SoftwareSDL` 骞惰兘杩涘叆鎾斁寰幆锛?


  - 杈撳嚭 `renderer-fallback-check.*` 瀛楁鍜?`PASS/FAIL`銆?


- 鏂板鏈湴鍥炲綊鎶ュ憡 `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`銆?


- 浠诲姟娓呭崟 `3.3.2` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 39: M3 3.3.1锛圵indows 杞В+纭В涓诲姏鏍锋湰鍙挱锛?


**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `3.3.1` 瑕佹眰 Windows 涓嬬‖瑙ｄ笌杞В涓诲姏鏍锋湰鍧囧彲绋冲畾杩涘叆鎾斁閾捐矾銆?


- 鐜版湁妫€鏌ヨ矾寰勭己灏戠粺涓€鑱氬悎鍛戒护锛屼笖鍚岃繘绋嬭繛缁細璇濆湪閮ㄥ垎鍦烘櫙瀛樺湪鍋滄闃舵鍗℃椋庨櫓銆?






### 鍘熷洜鍒嗘瀽



- 涔嬪墠鐨?`--windows-backend-check` 鍦ㄥ悓杩涚▼椤哄簭鎵ц纭В+杞В锛屼細瑙﹀彂浜屾浼氳瘽璧勬簮鍥炴敹涓嶇ǔ瀹氥€?


- 缂哄皯鍙鐢ㄧ殑浼氳瘽绾ц瘖鏂緭鍑猴紝涓嶅埄浜庡洖褰掓姤鍛婅嚜鍔ㄥ寲閲囬泦銆?






### 瑙ｅ喅鏂规



- 鏂板浼氳瘽绾у懡浠?`--windows-backend-session-check <media_file> <hard|soft>`锛岃緭鍑虹粨鏋勫寲瀛楁骞惰繑鍥炴ā寮忕粨鏋溿€?


- 灏嗚仛鍚堝懡浠?`--windows-backend-check <media_file>` 鏀逛负鐖惰繘绋嬫媺璧蜂袱涓瓙杩涚▼锛坔ard/soft锛夊苟姹囨€荤粨鏋滐紝闅旂浼氳瘽鐘舵€併€?


- 鍦?Windows 涓嬩娇鐢?`CreateProcess` 閲嶅畾鍚戣緭鍑猴紝閬垮厤 shell 閲嶅畾鍚戣В鏋愪笉绋冲畾銆?


- 鏂板鏈湴鍥炲綊鎶ュ憡 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`銆?


- 瀹屾垚浠诲姟娓呭崟鍚屾锛歚3.3.1`銆乣3.3.3` 鏍囪瀹屾垚銆?






### 淇敼鏂囦欢



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 40: M4 4.1锛堢珷鑺傚鑸細涓婁竴绔?涓嬩竴绔狅級







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `4.1` 闇€瑕佹敮鎸佺珷鑺傚鑸紙涓婁竴绔?涓嬩竴绔狅級銆?


- 褰撳墠鎾斁閾捐矾缂哄皯绔犺妭鍏冩暟鎹秷璐逛笌绔犺妭璺宠浆鍏ュ彛锛屾棤娉曢€氳繃蹇嵎閿洿鎺ヨ烦绔犮€?






### 鍘熷洜鍒嗘瀽



- `Demuxer` 铏借兘璇诲彇濯掍綋鍩烘湰淇℃伅锛屼絾鏈彁鍙?`AVChapter` 鏁版嵁銆?


- 杈撳叆閾捐矾缂哄皯绔犺妭鍔ㄤ綔璇锋眰锛坄Display -> Renderer -> PlayerCore`锛夈€?


- 涓绘祦绋嬬己灏戝彲閲嶅鎵ц鐨勭珷鑺傚鑸獙鏀跺懡浠ゃ€?






### 瑙ｅ喅鏂规



- 鍦?`Demuxer` 涓В鏋愮珷鑺傚厓鏁版嵁锛屾柊澧?`ChapterInfo` 涓?`MediaInfo::chapters`銆?


- 鏂板绔犺妭瀵艰埅鍔ㄤ綔涓庤姹傞摼璺細



  - `HotkeyManager` 澧炲姞 `PreviousChapter/NextChapter`锛?


  - 榛樿閿綅缁戝畾 `HOME/END`锛?


  - `Display`銆佹覆鏌撳櫒鎺ュ彛銆乣PlayerCore`銆乣VideoPlayer` 鍏ㄩ摼璺€忎紶绔犺妭璇锋眰銆?


- 鍦?`PlayerCore` 涓柊澧炵珷鑺傝烦杞兘鍔涳細



  - 鎵撳紑濯掍綋鏃舵瀯寤虹珷鑺傛椂闂寸偣锛?


  - `seekToNextChapter()` / `seekToPreviousChapter()` 鎵ц璺崇珷銆?


- 鍦?`main` 鏂板 `--chapter-nav-check <media_file>` 鑷鍛戒护锛屽苟鏇存柊甯姪杈撳嚭銆?


- 鏂板鏈湴鎶ュ憡 `docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`锛岃褰曠珷鑺傛牱鏈笌 PASS 缁撴灉銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`4.1` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/demuxer.h



- src/demuxer.cpp



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/CHAPTER_NAV_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 41: M4 4.2锛圓-B Repeat锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `4.2` 闇€瑕佹敮鎸?A-B Repeat銆?


- 褰撳墠鎾斁鍣ㄧ己灏?A/B/C 蹇嵎閿姩浣滀笌寰幆鍖洪棿鎺у埗閫昏緫锛屾棤娉曞湪鎾斁涓繘琛屽尯闂撮噸澶嶃€?






### 鍘熷洜鍒嗘瀽



- 杈撳叆閾捐矾鏈畾涔?A-B Repeat 璇锋眰鍔ㄤ綔銆?


- `PlayerCore` 缂哄皯 A 鐐?B 鐐圭姸鎬佺鐞嗕笌寰幆瑙﹀彂閫昏緫銆?


- 缂哄皯鍙噸澶嶆墽琛岀殑 A-B Repeat 楠屾敹鍛戒护銆?






### 瑙ｅ喅鏂规



- 鎵╁睍鐑敭鍔ㄤ綔锛?


  - 鏂板 `SetABRepeatStart` / `SetABRepeatEnd` / `ClearABRepeat`锛?


  - 榛樿閿綅缁戝畾 `A/B/C`銆?


- 鎵╁睍璇锋眰閾捐矾锛?


  - `Display` 澧炲姞 A-B Repeat 璇锋眰鏍囪涓庢秷璐规帴鍙ｏ紱



  - `IVideoRenderer` 涓庡悇娓叉煋鍣ㄥ疄鐜板鍔犻€忎紶鎺ュ彛銆?


- `PlayerCore` 鏂板 A-B Repeat 鐘舵€佷笌鎺у埗锛?


  - `setABRepeatStart()` 璁剧疆 A 鐐瑰苟娓呯┖鏃?B 鐐癸紱



  - `setABRepeatEnd()` 璁剧疆 B 鐐瑰苟鍚敤寰幆锛?


  - `clearABRepeat()` 娓呴櫎寰幆锛?


  - `handleABRepeatLoop()` 鍦ㄦ挱鏀句腑妫€娴嬪埌杈?B 鐐瑰悗鑷姩 seek 鍥?A 鐐广€?


- `VideoPlayer` 鏆撮湶 A-B Repeat API 渚涗富娴佺▼涓庨獙鏀跺懡浠よ皟鐢ㄣ€?


- 鏂板 `--ab-repeat-check <media_file>` 鍛戒护锛岃緭鍑?`ab-repeat-check.*` 瀛楁鍜?`PASS/FAIL`銆?


- 淇鍥炲綊妫€鏌ュ啿绐侊細



  - `--settings-persistence-check` 鐨勬祴璇曢敭浣嶇敱 `b` 璋冩暣涓?`x`锛岄伩鍏嶄笌鏂伴粯璁ょ儹閿啿绐併€?


- 鏂板鏈湴鎶ュ憡 `docs/reports/AB_REPEAT_LOCAL_CHECK.md`銆?


- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`4.2` 宸插畬鎴愩€?






### 淇敼鏂囦欢



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/display.h



- src/display.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- include/render/d3d11_video_renderer.h



- src/render/d3d11_video_renderer.cpp



- include/render/opengl_video_renderer.h



- src/render/opengl_video_renderer.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/AB_REPEAT_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 42: M4 4.3锛堟埅鍥撅級







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `4.3` 闇€瑕佹敮鎸佹埅鍥俱€?


- 褰撳墠瀹炵幇铏界劧宸茬粡鎺ュ叆鎴浘鐑敭鍜?`--screenshot-check`锛屼絾鏆傚仠鎬佹病鏈夌紦瀛樻渶杩戜竴甯э紝鎴浘璇锋眰鏃犳硶绋冲畾淇濆瓨褰撳墠鐢婚潰銆?






### 鍘熷洜鍒嗘瀽



- 鎴浘钀界洏閫昏緫鍙粦瀹氬湪娓叉煋绾跨▼鐨勬柊甯у鐞嗚矾寰勪笂銆?


- 鎾斁鏆傚仠鍚庯紝璋冨害鍣ㄤ笉鍐嶇户缁€佸抚锛屽鑷存殏鍋滄€佹埅鍥炬病鏈夊彲娑堣垂鐨勫浘鍍忔暟鎹簮銆?






### 瑙ｅ喅鏂规



- `PlayerCore` 鏂板鏈€杩戞覆鏌撳抚缂撳瓨锛屽苟鏀寔浠庣紦瀛樺抚鐩存帴钀界洏鎴浘銆?


- `requestScreenshot()` 璋冩暣涓猴細鎾斁涓紓姝ユ帓闃燂紝鏆傚仠鎬佺洿鎺ヤ娇鐢ㄧ紦瀛樺抚淇濆瓨銆?


- `--screenshot-check` 鍗囩骇涓烘殏鍋滄€佹埅鍥鹃獙鏀讹紝瑕嗙洊杩欐淇鐨勬牳蹇冨満鏅€?


- 鏇存柊蹇嵎閿枃妗ｏ紝琛ュ厖 `S` 鎴浘銆佺珷鑺傚鑸€丄-B Repeat銆佸瓧骞曞紑鍏崇瓑鐜版湁鑳藉姏璇存槑銆?






### 淇敼鏂囦欢



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- README.md



- README_ZH.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/SCREENSHOT_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 43: `MPC_HC_GAP_ANALYSIS` 璇勪及缁撹杩囨湡







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 浠嶄繚鐣欐棫缁撹锛屾妸澶氶」宸叉帴鍏ヤ富娴佺▼鐨勮兘鍔涘啓鎴愨€滈鏋?鏈帴鍏モ€濄€?


- 杩欎細褰卞搷鍚庣画杩唬浼樺厛绾у垽鏂紝涔熶細璁╂枃妗ｈ鑰呭褰撳墠瀹炵幇杩涘害褰㈡垚閿欒璁ょ煡銆?






### 鍘熷洜鍒嗘瀽



- 閲岀▼纰戞枃妗ｃ€佸紑鍙戞棩蹇楀拰鏈湴鎶ュ憡鎸佺画鏇存柊锛屼絾宸窛璇勪及鏂囨。娌℃湁鍚屾缁存姢銆?


- 鏃ц瘎浼颁富瑕佷緷鎹€滀唬鐮侀鏋舵槸鍚﹀瓨鍦ㄢ€濓紝娌℃湁鍚告敹鍚庣画 `docs/reports/*` 楠屾敹缁撴灉銆?






### 瑙ｅ喅鏂规



- 灏?`docs/analysis/MPC_HC_GAP_ANALYSIS.md` 鏇存柊鍒?2026-03-08 鍙ｅ緞銆?


- 閲嶅啓鈥滃綋鍓嶅凡鍏峰鑳藉姏鈥濃€滃樊璺濇€昏鈥濃€滃叧閿湭瀹炵幇鍔熻兘娓呭崟鈥濃€滀唬鐮佸眰璇佹嵁鎽樿鈥濃€滃缓璁噷绋嬬鈥濄€?


- 鏂板鈥滈獙鏀朵笌鎶ュ憡璇佹嵁鈥濈珷鑺傦紝鎶婂瓧骞曘€佹挱鏀惧垪琛ㄣ€佽缃€佸揩鎹烽敭銆丏3D11/鍥為€€銆佺珷鑺傚鑸€丄-B Repeat銆佹埅鍥俱€佹牸寮忕煩闃电殑鏈湴鎶ュ憡绾冲叆璇勪及渚濇嵁銆?


- 鏇存柊 `docs/README.md`锛岃ˉ鍏呮湰娆℃枃妗ｅ榻愯鏄庛€?






### 淇敼鏂囦欢



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 44: `docs/records/VERSION.md` 鍘嗗彶璺緞鎻忚堪杩囨湡







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `docs/records/VERSION.md` 鐨勨€滈樁娈典竴鈥濆巻鍙茬珷鑺備粛鎶婃棭鏈?decoder/thread/test 鏂囦欢鍚嶅啓鎴愬綋鍓嶄粨搴撶粨鏋勮鏄庛€?


- 鍦ㄥ熀浜庢枃妗ｉ亶鍘嗕粨搴撴椂锛屽鏄撴妸宸茬Щ闄ょ殑鏃у疄鐜颁笌褰撳墠 `PlayerCore + Scheduler + core/*` 涓婚摼娣锋穯銆?






### 鍘熷洜鍒嗘瀽



- `2026-03-06` 涔嬪悗鏋舵瀯宸茬粡鏀舵暃锛屼絾鐗堟湰鏂囨。淇濈暀浜嗘棭鏈熸枃浠剁骇鎻忚堪銆?


- 鍚庣画鍔熻兘鎸佺画杩藉姞鏂拌褰曪紝鏈洖澶存竻鐞嗏€滃綋鍓嶉樁娈碘€濃€滀笅涓€姝ヨ鍒掆€濊繖绫诲凡杩囨椂鍙ｅ緞銆?






### 瑙ｅ喅鏂规



- 灏嗛樁娈典竴鏍囬鏀逛负鈥滃巻鍙茶捣鐐光€濓紝骞惰ˉ鍏呰繖鏄棭鏈熷疄鐜板熀绾跨殑璇存槑銆?


- 灏?`video_decoder` / `audio_decoder`銆乣VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 绛夋棫璺緞鏀瑰啓涓鸿兘鍔涚骇鍘嗗彶璁板綍锛屽苟鏄犲皠鍒板綋鍓?`core/*` 涓婚摼銆?


- 灏嗏€滀笅涓€姝ヨ鍒掆€濄€乣USE_NEW_PLAYER_CORE`銆佷复鏃?`tests/core_*` 绛夎〃杩版敼鍐欎负鍘嗗彶璇存槑锛岄伩鍏嶈瀵煎綋鍓嶈繘搴﹀垽鏂€?


- 鍚屾琛ュ啓鐗堟湰鏂囨。鐨勬洿鏂版棩蹇楁潯鐩€?






### 淇敼鏂囦欢



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂 45: README 涓庢灦鏋勬枃妗ｄ粛娣风敤鏃т富閾捐〃杩?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `README.md`銆乣README_ZH.md` 鐨勯」鐩粨鏋勫拰鏋舵瀯绀烘剰浠嶅睍绀?`video_decoder` / `audio_decoder` 绛夋棫璺緞銆?


- `docs/design/ARCHITECTURE.md` 涔熷惈鏈夆€滃綋鍓嶅疄鐜扳€濆瓧鏍峰拰鏃фā鍧楀懡鍚嶏紝瀹规槗涓庣幇琛?`PlayerCore + Scheduler + core/*` 涓婚摼娣锋穯銆?






### 鍘熷洜鍒嗘瀽



- 鏍圭洰褰?README 鐨勭洰褰曟爲鍜屾灦鏋勫浘鏉ヨ嚜鏃╂湡鍗曚綋/鏃у绾跨▼瀹炵幇闃舵锛屽悗缁湭闅忛噸鏋勫悓姝ュ埛鏂般€?


- `docs/design/ARCHITECTURE.md` 淇濈暀浜嗗ぇ閲忓巻鍙茶璁″唴瀹癸紝浣嗙己灏戞竻鏅扮殑鈥滃巻鍙?褰撳墠鈥濊竟鐣屾彁绀恒€?






### 瑙ｅ喅鏂规



- 鏇存柊 `README.md` 涓?`README_ZH.md` 鐨勯」鐩粨鏋勩€佹灦鏋勭ず鎰忓拰鏂囨。閾炬帴锛岀粺涓€鎸囧悜褰撳墠涓婚摼銆?


- 鍦?`docs/design/ARCHITECTURE.md` 椤堕儴澧炲姞鐘舵€佽鏄庯紝骞跺皢鏃фā鍧楃珷鑺傛樉寮忔爣璁颁负鈥滃巻鍙插疄鐜扳€濄€?


- 灏嗘棩蹇楃ず渚嬩粠 `spdlog` 鏀逛负褰撳墠椤圭洰浣跨敤鐨?`Quill` 瀹忔帴鍙ｃ€?


- 鏇存柊 `docs/README.md`锛屽尯鍒嗗巻鍙叉灦鏋勫熀绾夸笌褰撳墠閲嶆瀯璇存槑銆?






### 淇敼鏂囦欢



- README.md



- README_ZH.md



- docs/design/ARCHITECTURE.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂 46: 瀹炵幇鏁欑▼涓庤凯浠ｈ鍒掔己灏戝巻鍙?褰撳墠杈圭晫璇存槑







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `docs/guides/IMPLEMENTATION.md` 浠嶄互鏃╂湡 `video_decoder/audio_decoder/playLoop` 鍘熷瀷璺緞璁茶В瀹炵幇姝ラ锛屽鏄撹璇涓哄綋鍓嶄粨搴撶殑閫愭枃浠跺紑鍙戞寚鍗椼€?


- `docs/plans/MPC_HC_ITERATION_PLAN.md` 鏄?`2026-03-07` 鐨勮鍒掑揩鐓э紝浣嗘湭鏄庣‘璇存槑閮ㄥ垎璁″垝椤瑰凡鍦?`2026-03-08` 鍓嶅悗钀藉湴銆?






### 鍘熷洜鍒嗘瀽



- 杩欎袱浠芥枃妗ｆ湰韬粛鏈夊弬鑰冧环鍊硷紝浣嗙己灏戔€滃巻鍙叉暀绋?/ 璁″垝蹇収 / 褰撳墠杩涘害鈥濅箣闂寸殑杈圭晫璇存槑銆?


- 褰撶敤鎴锋寜鏂囨。閬嶅巻浠撳簱鏃讹紝浼氭妸鏁欑▼绀轰緥鍜岃鍒掓枃瀛楄褰撲綔鐜拌缁撴瀯涓庡綋鍓嶅緟鍔炪€?






### 瑙ｅ喅鏂规



- 涓?`docs/guides/IMPLEMENTATION.md` 澧炲姞鐘舵€佽鏄庯紝鏄庣‘鍏跺睘浜庢棭鏈熷師鍨嬫暀绋嬶紝骞舵寚鍚戝綋鍓嶄富閾惧弬鑰冩枃妗ｃ€?


- 涓?`docs/plans/MPC_HC_ITERATION_PLAN.md` 澧炲姞鐘舵€佽鏄庯紝鏄庣‘鍏跺睘浜?`2026-03-07` 鐨勮鍒掑揩鐓э紝骞惰ˉ鍏呭綋鍓嶈繘搴﹀弬鑰冨叆鍙ｃ€?


- 鏇存柊 `docs/README.md`銆乣README.md`銆乣README_ZH.md` 鐨勬枃妗ｈ鏄庯紝缁熶竴鍖哄垎鍘嗗彶鏁欑▼銆佽鍒掑揩鐓т笌褰撳墠瀹炵幇璇存槑銆?






### 淇敼鏂囦欢



- README.md



- README_ZH.md



- docs/guides/IMPLEMENTATION.md



- docs/plans/MPC_HC_ITERATION_PLAN.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂 47: 杈呭姪璇存槑鏂囨。浠嶇己灏戝綋鍓嶅叆鍙ｄ笌鐘舵€佽竟鐣?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `docs/design/FILTERS.md`銆乣docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`銆乣docs/guides/WINDOWS_SETUP.md` 铏界劧鍐呭浠嶆湁鍙傝€冧环鍊硷紝浣嗙己灏戝鈥滃綋鍓嶄富娴佺▼鍏ュ彛鈥濆拰鈥滄枃妗ｉ€傜敤鑼冨洿鈥濈殑鏄庣‘璇存槑銆?


- 鍏朵腑 `docs/guides/WINDOWS_SETUP.md` 杩樹繚鐣欎簡涓庡綋鍓?`CMakeLists.txt` 涓嶅畬鍏ㄤ竴鑷寸殑鎵嬪姩閰嶇疆绀轰緥锛屽鏄撹瀵?Windows 鏋勫缓璺緞銆?






### 鍘熷洜鍒嗘瀽



- 杩欏嚑浠芥枃妗ｅ垎鍒湇鍔′簬婊ら暅銆佽兘鍔涘弬鑰冦€乄indows 鐜閰嶇疆锛岄暱鏈熻凯浠ｅ悗鍐呭娌℃湁缁熶竴琛ラ綈鈥滃綋鍓嶅彛寰勨€濊鏄庛€?


- 鏂囨。璇昏€呭鏋滅洿鎺ユ寜鏃ф弿杩版墽琛岋紝鍙兘浼氭妸鍙傝€冩€у唴瀹硅褰撴垚褰撳墠鍞竴鍏ュ彛锛屾垨娌跨敤杩囨椂鐨勫弬鏁颁紶閫掓柟寮忋€?






### 瑙ｅ喅鏂规



- 涓?`docs/design/FILTERS.md` 澧炲姞鐘舵€佽鏄庯紝鍖哄垎褰撳墠鐢熸晥鐨?`FilterPipeline` 涓婚摼涓庨鐣欓摼寮忓皝瑁呫€?


- 涓?`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 澧炲姞閫傜敤鑼冨洿璇存槑锛屽苟灏嗘€昏繘搴﹀弬鑰冨叆鍙ｆ寚鍚戝樊璺濊瘎浼般€佺増鏈褰曞拰鍙樻洿鏃ュ織銆?


- 涓?`docs/guides/WINDOWS_SETUP.md` 澧炲姞褰撳墠渚濊禆鎺㈡祴椤哄簭璇存槑锛岀Щ闄よ瀵兼€х殑 `SDL2_DIR` / `FFMPEG_DIR` 浼犲弬绀轰緥锛屾敼涓轰笌鐜版湁 `CMakeLists.txt` 涓€鑷寸殑浠撳簱鍐?`external/*` 鍥為€€鍙ｅ緞銆?


- 鏇存柊 `docs/README.md` 绱㈠紩涓庢湰杞枃妗ｆ洿鏂拌褰曘€?






### 淇敼鏂囦欢



- docs/design/FILTERS.md



- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md



- docs/guides/WINDOWS_SETUP.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂 48: 鏍?README 鏁呴殰鎺掗櫎涓庡巻鍙查棶棰樺綊妗ｄ粛鏈夋棫鍙ｅ緞







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 鏍圭洰褰?`README.md` / `README_ZH.md` 鐨?Windows 鏁呴殰鎺掗櫎浠嶅缓璁娇鐢?`FFMPEG_DIR` 浼犲弬锛屽拰褰撳墠 `CMakeLists.txt` 鐨勪緷璧栨帰娴嬫柟寮忎笉涓€鑷淬€?


- `docs/analysis/video-stream-index-fix.md` 鏄棭鏈熷師鍨嬮樁娈电殑闂鍒嗘瀽锛屼絾缂哄皯鈥滃巻鍙插綊妗ｂ€濊鏄庯紝瀹规槗璁╄鑰呰浠ヤ负鍏朵腑鐨?`playLoop` / `video_decoder.cpp` 浠嶅睘褰撳墠涓婚摼銆?






### 鍘熷洜鍒嗘瀽



- README 鐨勬晠闅滄帓闄ゆ钀戒繚鐣欎簡鏃ф墜鍔ㄤ緷璧栫敤娉曪紝娌℃湁璺熼殢 Windows 鏋勫缓鍏ュ彛涓€璧锋洿鏂般€?


- 鍘嗗彶闂鍒嗘瀽鏂囨。鍐呭鏈韩鏈変环鍊硷紝浣嗛渶瑕佹樉寮忚鏄庡叾閫傜敤闃舵銆?






### 瑙ｅ喅鏂规



- 灏?README 涓?FFmpeg 鏁呴殰鎺掗櫎鏀逛负褰撳墠鎺ㄨ崘鍙ｅ緞锛氫紭鍏堣鏄?`vcpkg` toolchain锛屾墜鍔ㄥ畨瑁呭垯浣跨敤浠撳簱鍐?`external/ffmpeg/` 鍥為€€甯冨眬銆?


- 涓?`docs/analysis/video-stream-index-fix.md` 澧炲姞鐘舵€佽鏄庯紝鏍囪涓烘棭鏈熷師鍨嬮棶棰樺垎鏋愬綊妗ｃ€?


- 鍦?`docs/README.md` 涓ˉ鍏呰鍘嗗彶鍒嗘瀽鏂囨。鐨勭储寮曚笌鏈疆鏇存柊璁板綍銆?






### 淇敼鏂囦欢



- README.md



- README_ZH.md



- docs/analysis/video-stream-index-fix.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md







---







## 闂 49: 缂哄皯鐙珛鐨勬枃妗ｅ贰妫€鎬昏〃







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 鍓嶅嚑杞枃妗ｆ暣鐞嗗凡缁忓畬鎴愶紝浣嗗贰妫€缁撴灉鍒嗘暎鍦ㄥ璇濄€佹棩蹇楀拰澶氫釜鎻愪氦涓紝缂哄皯涓€浠界嫭绔嬬殑鎬昏〃鏂囨。銆?


- 鍚庣画缁存姢鑰呭鏋滄兂蹇€熶簡瑙ｂ€滃摢浜涙枃妗ｅ凡鏀舵暃銆佸摢浜涘唴瀹瑰睘浜庡巻鍙蹭繚鐣欍€佷负浠€涔堣淇濈暀鈥濓紝闇€瑕佹潵鍥炵炕闃呭涓枃浠躲€?






### 鍘熷洜鍒嗘瀽



- 鐜版湁 `CHANGELOG.md` / `DEVELOP_LOG.md` 閫傚悎璁板綍杩囩▼锛屼絾涓嶉€傚悎浣滀负闈㈠悜鍚庣画缁存姢鐨勬憳瑕佹姤鍛娿€?


- `docs/README.md` 铏界劧鏄储寮曞叆鍙ｏ紝浣嗘病鏈夌嫭绔嬫壙杞芥湰杞贰妫€缁撹鐨勪笓棰樻枃妗ｃ€?






### 瑙ｅ喅鏂规



- 鏂板 `docs/analysis/DOC_AUDIT_2026-03-08.md`锛岄泦涓綊妗ｆ湰杞枃妗ｅ贰妫€鐨勮寖鍥淬€佹柟娉曘€佸凡瀹屾垚瀵归綈椤广€佷繚鐣欏巻鍙插唴瀹广€佸悗缁淮鎶ゅ缓璁笌鍏宠仈鎻愪氦銆?


- 鏇存柊 `docs/README.md`锛屾妸璇ユ姤鍛婂姞鍏ョ储寮曪紝骞惰ˉ涓€鏉℃湰杞洿鏂拌褰曘€?






### 淇敼鏂囦欢



- docs/analysis/DOC_AUDIT_2026-03-08.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md











---







## 闂 50: M4 4.4锛氭殏鍋滄€佸抚姝ヨ繘鎺ュ叆涓庨獙鏀?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `4.4` 瑕佹眰鏀寔鏆傚仠鎬佸抚姝ヨ繘銆?


- 褰撳墠鎾斁鍣ㄨ櫧鐒跺凡鏈夋殏鍋溿€佹埅鍥俱€佺珷鑺傚鑸拰 A-B Repeat锛屼絾缂哄皯鍙洿鎺ラ€愬抚妫€鏌ョ敾闈㈢殑浜や簰鍏ュ彛銆?






### 鍘熷洜鍒嗘瀽



- 杈撳叆灞傛病鏈夊崟鐙殑甯ф杩涘姩浣滐紝涔熸病鏈夊搴旂殑榛樿閿綅銆?


- `PlayerCore` 鐨勬殏鍋滄€佸彧浼氬喕缁撹皟搴﹀櫒锛岀己灏戔€渟eek 鍚庡埛鏂扮洰鏍囧抚鈥濈殑鍗曞抚娓叉煋璺緞銆?


- 闊抽娑堣垂绾跨▼鍦ㄦ殏鍋滄€佷粛浼氫緷鎹棫 `playback_pts` 鍥炲啓浣嶇疆锛屽鑷村崟甯ф杩涘悗鐨勬椂闂寸偣鍙兘琚煶棰戞椂閽熻鐩栥€?






### 瑙ｅ喅鏂规



- 涓虹儹閿郴缁熸柊澧?`step_frame_backward` / `step_frame_forward` 鍔ㄤ綔锛岄粯璁ょ粦瀹?`,` / `.`銆?


- 鍦?`Display -> Renderer -> PlayerCore` 閾捐矾鏂板甯ф杩涜姹傞€氶亾銆?


- `PlayerCore` 鏂板鏆傚仠鎬佸抚姝ヨ繘鑳藉姏锛?


  - 浼扮畻鍗曞抚姝ラ暱锛?


  - 閫氳繃 seek 鍒锋柊闊宠棰戠姸鎬侊紱



  - 涓诲姩娓叉煋鐩爣鏃堕棿鐐圭殑棣栧抚骞朵繚鎸佹殏鍋溿€?


- 鏀剁揣闊抽娑堣垂绾跨▼鐨勪綅缃洖鍐欐潯浠讹紝閬垮厤鏆傚仠鎬佽鐩栨杩涚粨鏋溿€?


- 鍦?`main` 鏂板 `--frame-step-check <media_file>` 楠屾敹鍛戒护锛屽苟鍚屾 README / 鐗堟湰鏂囨。 / 宸窛璇勪及 / 浠诲姟娓呭崟銆?






### 淇敼鏂囦欢



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- include/display.h



- src/display.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- include/video_player.h



- src/video_player.cpp



- src/main.cpp



- README.md



- README_ZH.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/reports/FRAME_STEP_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## ?? 51: M4 4.5???/??????????







**??**: 2026-03-08







### ????



- ???? `4.5` ????????????????



- ??????????????????????????????????????????????????







### ????



- ?????????????????????? `J/K` ? `Ctrl+J/K` ?????????



- `PlayerCore` ????????????????????????????? PTS????????/??????????



- ?????????????????/???????????????????







### ????



- ??????????? `J / K` ??? `- / +100ms`??? `Ctrl` ?????? `- / +100ms`?



- ? `Display -> Renderer -> PlayerCore -> VideoPlayer` ??????/??????? API?



- ?????????????????????????? PTS ?????????????????????????



- ???????? `player.audio_delay_ms` / `player.subtitle_delay_ms`???? `--settings-persistence-check`?



- ?? `--delay-adjust-check <media_file> <subtitle.srt>` ????????????







### ????



- include/core/player_core.h



- include/video_player.h



- include/display.h



- include/render/video_renderer.h



- include/render/sdl_video_renderer.h



- include/render/d3d11_video_renderer.h



- include/render/opengl_video_renderer.h



- include/input/hotkey_manager.h



- src/core/player_core.cpp



- src/video_player.cpp



- src/display.cpp



- src/render/sdl_video_renderer.cpp



- src/render/d3d11_video_renderer.cpp



- src/render/opengl_video_renderer.cpp



- src/input/hotkey_manager.cpp



- src/main.cpp



- config/player_settings.ini



- README.md



- README_ZH.md



- docs/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md



- docs/reports/DELAY_ADJUST_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## ?? 52: M4 4.6????? `1..9` ????







**??**: 2026-03-08







### ????



- `4.6` ???????????? `A/B/C/S,./J/K/1..9`?



- ? `4.5` ???????????????????????? `1..9` ?????????







### ????



- ??????? `1..9` ???????????????????????



- ?????????????????? seek ???????????????????



- ??????????? `1..9` ???????????????????







### ????



- ? `HotkeyManager` ??? `seek_to_10_percent` ~ `seek_to_90_percent` ?????????? `1..9`?



- `Display` ????????????? `seek_ratio_` ????? `PlayerCore::pumpEvents()` ???? seek ???



- ? `main` ?? `--numeric-seek-check <media_file>` ???????? README????????????????







### ????



- include/input/hotkey_manager.h



- src/input/hotkey_manager.cpp



- src/display.cpp



- src/main.cpp



- README.md



- README_ZH.md



- docs/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂 53: M2 2.2.4锛氳緭鍑烘挱鏀炬€ц兘鏃ュ織锛堟帀甯?闃熷垪/CPU/GPU锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.2.4` 瑕佹眰杈撳嚭鎾斁鎬ц兘鏃ュ織锛岀敤浜庤瘎浼伴珮鍒嗚鲸鐜囥€侀珮鐮佺巼鏍锋湰鐨勬帀甯с€侀槦鍒椾笌璧勬簮鍗犵敤琛ㄧ幇銆?


- 褰撳墠涓婚摼铏界劧鍐呴儴宸茬粡绱浜嗚В灏佽銆佽В鐮併€佹覆鏌撳拰璋冨害缁熻锛屼絾缂哄皯涓€涓彲澶嶇敤銆佸彲瀵规瘮銆佸彲鐩存帴楠屾敹鐨勭粺涓€杈撳嚭鍏ュ彛銆?






### 鍘熷洜鍒嗘瀽



- `PlayerCore` 鍐呯殑璇婃柇璁℃暟鍣ㄥ拰 `Scheduler` 缁熻鍒嗘暎鍦ㄥ唴閮ㄥ疄鐜颁腑锛屽閮ㄨ皟鐢ㄦ柟鏃犳硶涓€娆℃€ц幏鍙栧畬鏁村揩鐓с€?


- 鍛戒护琛岃嚜妫€鍏ュ彛灏氭湭瑕嗙洊鎬ц兘瑙傛祴鍦烘櫙锛屽鑷?`1080p60 / 4K / 楂樼爜鐜嘸 鏍锋湰鍙兘渚濊禆闆舵暎鏃ュ織锛岄毦浠ュ舰鎴愮ǔ瀹氶棬绂併€?


- GPU 鍒╃敤鐜囪法骞冲彴鐩存帴閲囨牱鎴愭湰杈冮珮锛屽洜姝ゆ洿閫傚悎鍏堣緭鍑哄綋鍓嶆縺娲荤殑瑙ｇ爜/娓叉煋 backend锛屼綔涓?GPU 璺緞鏍囪瘑銆?






### 瑙ｅ喅鏂规



- 鍦?`PlayerCore` 涓柊澧?`DiagnosticsSnapshot`锛岀粺涓€瀵煎嚭 demux銆乨ecode銆乺ender銆乻cheduler 涓庨槦鍒楁寚鏍囥€?


- 鍦?`VideoPlayer` 涓€忎紶 `getInfo()` / `getDiagnosticsSnapshot()`锛岄伩鍏嶉獙鏀堕€昏緫鐩存帴鑰﹀悎鍐呴儴瀹炵幇銆?


- 鍦?`main` 涓柊澧?`--performance-log-check <media_file> [sample_ms]`锛?


  - 閲囨牱鎾斁鏈熼棿鐨勫钩鍧?CPU 鍗犵敤锛?


  - 杈撳嚭 renderer / decoder backend锛?


  - 杈撳嚭鎺夊抚銆侀槦鍒椼€佽В鐮佸抚鏁般€佹覆鏌撳抚鏁扮瓑缁撴瀯鍖栨寚鏍囥€?


- 鍚屾琛ラ綈浠诲姟娓呭崟銆侀獙鏀舵姤鍛娿€佸樊璺濊瘎浼般€佺増鏈褰曚笌寮€鍙戞棩蹇椼€?






### 淇敼鏂囦欢



- include/core/player_core.h



- include/video_player.h



- src/core/player_core.cpp



- src/video_player.cpp



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂 54: M2 2.2.1 / 2.3.2锛?080p60 绋冲畾鎾斁楠屾敹







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.2.1` 涓?`2.3.2` 闇€瑕佺‘璁?`1080p60` 鏍锋湰鑳藉杩炵画绋冲畾鎾斁銆?


- 褰撳墠涓婚摼铏界劧宸叉湁鎬ц兘鏃ュ織鍏ュ彛锛屼絾缂哄皯涓€涓洿鎺ラ潰鍚?`1080p60` 闂ㄧ鐨勭ǔ瀹氭€ч獙鏀跺懡浠ゅ拰閰嶅鏍锋湰鍏ュ彛銆?






### 鍘熷洜鍒嗘瀽



- 鐜版湁 `--performance-log-check` 鏇村亸鍚戣娴嬫寚鏍囧鍑猴紝涓嶇洿鎺ュ垽鏂椂闂存帹杩涖€佽繛缁挱鏀剧獥鍙ｄ笌鎺夊抚闂ㄧ鏄惁杈炬爣銆?


- 浠撳簱鐜版湁鏍锋湰闆嗕腑鍦?`1080p30` 涓?`4K60`锛岀己灏戞槑纭殑 `1080p60` 绋冲畾鎬ф牱鏈敓鎴愬叆鍙ｃ€?






### 瑙ｅ喅鏂规



- 鍦?`main` 涓柊澧?`--1080p60-check <media_file> [sample_ms]`锛岃仈鍚?`collectFileProbeReport()` 涓?`DiagnosticsSnapshot` 杈撳嚭绋冲畾鎬ч棬绂佺粨鏋溿€?


- 楠屾敹閫昏緫閲嶇偣妫€鏌ワ細



  - 鏍锋湰鏄惁涓?`1920x1080 @ 60fps`锛?


  - `5s` 杩炵画鎾斁绐楀彛鍐呮椂闂存槸鍚︾ǔ瀹氭帹杩涳紱



  - `scheduler_late_drops` 涓?`demux_dropped_packets` 鏄惁涓?`0`銆?


- 鍦?`tools/download_test_samples.ps1` 涓ˉ鍏?`1080p60 AAC 2ch` 鏍锋湰鐢熸垚锛屽苟鍦?`samples/README.md` 涓褰曠敤閫斻€?


- 鍚屾琛ラ綈浠诲姟娓呭崟銆佹姤鍛娿€佸樊璺濊瘎浼般€佺増鏈枃妗ｄ笌寮€鍙戞棩蹇椼€?






### 淇敼鏂囦欢



- src/main.cpp



- tools/download_test_samples.ps1



- samples/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/1080P60_STABILITY_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂 55: M2 2.2.2 / 2.3.3锛?K 鎾斁涓庨檷绾ч獙鏀?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.2.2` 涓?`2.3.3` 瑕佹眰纭 `4K` 鏍锋湰鍙互鎾斁锛屽苟涓斿湪纭В涓嶅彲鐢ㄦ椂鑳藉闄嶇骇鍒拌蒋瑙ｇ户缁挱鏀俱€?


- 褰撳墠浠撳簱宸叉湁鎬ц兘鏃ュ織鍏ュ彛鍜?Windows 鍚庣鍥為€€鏍￠獙锛屼絾缂哄皯涓€涓洿鎺ラ潰鍚?`4K` 闂ㄧ鐨勮仛鍚堥獙鏀跺懡浠ゃ€?






### 鍘熷洜鍒嗘瀽



- `--performance-log-check` 鍙互璇存槑 4K 鏍锋湰鑳借繘鍏ユ挱鏀鹃摼璺紝浣嗕笉鐩存帴瑕嗙洊鈥滆蒋瑙ｉ檷绾ф槸鍚︽垚鍔熲€濄€?


- `--windows-backend-check` 鍙獙璇?hard / soft 涓や釜鍚庣妯″紡锛屼絾涓嶆惡甯?`4K` 杩炵画鎾斁绐楀彛鍜屾椂闂存帹杩涢棬绂併€?






### 瑙ｅ喅鏂规



- 鍦?`main` 涓柊澧?`--4k-playback-check <media_file> [sample_ms]`锛屼富杩涚▼楠岃瘉 `4K` 鏍锋湰鐪熷疄鎺ㄨ繘涓?`late_drop`锛屽瓙杩涚▼澶嶇敤 hard / soft backend session 楠岃瘉鍙檷绾с€?


- 杈撳嚭 probe 瀹介珮/FPS銆佹椂闂存帹杩涙瘮鐜囥€佸綋鍓?backend銆乭ard / soft 浼氳瘽缁撴灉绛夌粨鏋勫寲瀛楁銆?


- 鍚屾琛ラ綈浠诲姟娓呭崟銆佹姤鍛娿€佸樊璺濊瘎浼般€佺増鏈褰曚笌寮€鍙戞棩蹇椼€?






### 淇敼鏂囦欢



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂 56: M2 2.2.3锛?80Mbps 楂樼爜鐜囨牱鏈獙鏀?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `2.2.3` 瑕佹眰鑷冲皯楠岃瘉涓€涓?`>80Mbps` 鏍锋湰鑳藉鎾斁銆?


- 褰撳墠浠撳簱铏藉凡瀹屾垚 `1080p60`銆乣4K` 涓庢€ц兘鏃ュ織闂ㄧ锛屼絾缂哄皯鏄庣‘鐨勯珮鐮佺巼鏍锋湰鍜屼笓鐢ㄩ獙鏀跺叆鍙ｃ€?






### 鍘熷洜鍒嗘瀽



- 鐜版湁鏍锋湰鏅亶鍙湁 `3~4Mbps` 绾у埆锛屾棤娉曡瘉鏄庢挱鏀惧櫒鍦ㄩ珮鐮佺巼鍦烘櫙涓嬬殑瑙ｅ皝瑁呫€佽В鐮佷笌娓叉煋閾捐矾绋冲畾鎬с€?


- 鐜版湁楠屾敹鍛戒护鏈鈥滄牸寮忕爜鐜囨槸鍚﹁秴杩?80Mbps鈥濆仛鍓嶇疆鍒ゆ柇銆?






### 瑙ｅ喅鏂规



- 鏂板 `--high-bitrate-check <media_file> [sample_ms]`锛屽厛璇诲彇鏍煎紡鐮佺巼锛屽啀鎵ц杩炵画鎾斁绐楀彛鏍￠獙銆?


- 鍦?`tools/download_test_samples.ps1` 涓柊澧?`stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 鐢熸垚鍏ュ彛锛屼繚璇佹湰鍦板彲澶嶇幇瀹為獙鏍锋湰銆?


- 鍚屾琛ラ綈浠诲姟娓呭崟銆佹姤鍛娿€佸樊璺濊瘎浼般€佺増鏈褰曚笌寮€鍙戞棩蹇椼€?






### 淇敼鏂囦欢



- src/main.cpp



- tools/download_test_samples.ps1



- samples/README.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/HIGH_BITRATE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md











---







## 闂 57: 鍙戝竷闂ㄧ 6.5锛氶暱鏃舵挱鏀剧ǔ瀹氭€ч獙鏀?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `6.5` 瑕佹眰纭鎾斁鍣ㄥ湪鎸佺画鎾斁绐楀彛鍐呮棤 crash 涓旇兘鎸佺画鎺ㄨ繘銆?






### 鍘熷洜鍒嗘瀽



- 鐜版湁 `1080p60`銆乣4K`銆侀珮鐮佺巼涓庢€ц兘鏃ュ織闂ㄧ瑕嗙洊浜嗙煭绐楀彛绋冲畾鎬т笌鍙娴嬫€э紝浣嗙己灏戜竴涓洿鎺ラ潰鍚戔€滈暱鏃舵挱鏀炬棤 crash鈥濈殑鍥哄畾 smoke 鍛戒护銆?


- 鍙戝竷闂ㄧ `6.1 ~ 6.6` 鐨勬渶鍚庣己鍙ｆ槸绋冲畾鎬ц瘉鎹紝缂哄皯鍗曠嫭鎶ュ憡灏辨棤娉曟敹鍙?DoD銆?






### 瑙ｅ喅鏂规



- 鍦?`main` 涓柊澧?`--long-playback-check <media_file> [sample_ms]`锛岃姹傛渶鐭噰鏍风獥鍙?`5000ms`锛屽苟杈撳嚭 `open_ok`銆佹槸鍚﹁繘鍏ユ挱鏀惧惊鐜€佺獥鍙ｇ粨鏉熷悗鏄惁浠嶅湪鎾斁銆佹椂闂存帹杩涙瘮鐜囥€乣late_drop`銆乨emux 涓㈠寘涓?backend 淇℃伅銆?


- 鏂板 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`锛岃褰?`./juren-30s.mp4` 涓?`10000ms` 杩炵画鎾斁 smoke 缁撴灉锛屽苟鍚屾浠诲姟娓呭崟銆佸樊璺濊瘎浼般€佺増鏈褰曚笌寮€鍙戞棩蹇椼€?






### 淇敼鏂囦欢



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂 58: 7.1 鎻掍欢绯荤粺锛堝姩鎬佸姞杞戒笌鐢熷懡鍛ㄦ湡闂幆锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `7.1` 闇€瑕佹妸鐜版湁浠呮敮鎸佸唴瀛樻敞鍐?鍚仠鐘舵€佺殑鎻掍欢楠ㄦ灦锛岃ˉ鎴愬彲瀹為檯鍔犺浇鍜岄獙鏀剁殑鎻掍欢绯荤粺銆?






### 鍘熷洜鍒嗘瀽



- `PluginManager` 涔嬪墠鍙淮鎶ゅ厓鏁版嵁鍒楄〃锛屾病鏈?`DLL` 鍔ㄦ€佸姞杞姐€佺増鏈吋瀹规牎楠屻€佺敓鍛藉懆鏈熷洖璋冨拰鍗歌浇娓呯悊鑳藉姏銆?


- `FilterRegistry` 缂哄皯娉ㄩ攢鎺ュ彛锛屽鑷村嵆浣挎彃浠惰兘娉ㄥ唽婊ら暅锛屼篃鏃犳硶鍦ㄥ嵏杞芥椂瀹夊叏鍥炴敹鎵╁睍鐐广€?






### 瑙ｅ喅鏂规



- 鏂板 `include/plugin/plugin_api.h`锛屽畾涔夋彃浠跺涓绘帴鍙ｃ€乣API` 鐗堟湰甯搁噺鍜屽鍑虹鍙风害瀹氥€?


- 閲嶅啓 `PluginManager`锛氭敮鎸佹寜鏂囦欢/鐩綍鍔犺浇鎻掍欢銆佹牎楠?`API` 鐗堟湰銆佽皟鐢?`initialize/shutdown`銆佹崟鑾锋彃浠跺紓甯革紝骞跺湪鍗歌浇鏃舵敞閿€鎻掍欢娉ㄥ唽鐨勬护闀滃伐鍘傘€?


- 鏂板 `sample_logger_plugin` 绀轰緥 `DLL` 涓?`--plugin-check [plugin_dir_or_file]` 鍛戒护锛岄獙璇?`sample_identity` 瑙嗛婊ら暅鐨勬敞鍐屼笌鍗歌浇娓呯悊闂幆銆?






### 淇敼鏂囦欢



- CMakeLists.txt



- include/plugin/plugin_api.h



- include/plugin/plugin_manager.h



- include/filters/filter_registry.h



- src/plugin/plugin_manager.cpp



- src/plugin/sample_logger_plugin.cpp



- src/filters/filter_registry.cpp



- src/main.cpp



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂 59: 7.2 娴佸獟浣擄紙鐪熷疄 HTTP 鍒嗙墖涓庣紦鍐诧級







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `7.2` 瑕佹眰鎶婃祦濯掍綋鑳藉姏浠庘€滆В鏋愬櫒楠ㄦ灦鈥濇帹杩涘埌鐪熷疄 HTTP 鍒嗙墖涓嬭浇涓庣紦鍐查棴鐜€?






### 鍘熷洜鍒嗘瀽



- `HttpStreamDownloader` 涔嬪墠鍙繚瀛?URL锛屼笉鍋氫换浣曠湡瀹炵綉缁滆鍙栵紝涔熸病鏈夊唴閮ㄧ紦鍐蹭笌 EOF/閿欒鐘舵€併€?


- 鐜版湁 HLS/DASH 瑙ｆ瀽鍣ㄥ彧鑳藉鐞嗘枃鏈紝缂哄皯涓€濂楀彲閲嶅鎵ц鐨勬湰鍦?HTTP 澶瑰叿鏉ラ獙璇佸垎鐗囦笅杞介摼璺€?






### 瑙ｅ喅鏂规



- 閲嶅啓 `HttpStreamDownloader`锛屽熀浜?FFmpeg `avio` 鏀寔鐪熷疄 HTTP 鎵撳紑銆佸垎鍧楄鍙栥€佸唴閮ㄧ紦鍐层€丒OF 鐘舵€佷笌閿欒閫忎紶銆?


- 鍦?`main` 涓柊澧?`--streaming-buffer-check`锛屼笅杞?HLS 濯掍綋娓呭崟銆佽В鏋愬苟鎶撳彇鍓?N 涓垎鐗囷紝楠岃瘉缂撳啿瀛楄妭鏁颁笌涓嬭浇缁撴灉銆?


- 鏂板 `samples/streaming/hls_local/*` 鏈湴澶瑰叿涓?`tools/start_streaming_fixture_server.ps1`锛岄€氳繃鏈満 HTTP 鏈嶅姟澶嶇幇瀹為獙銆?






### 淇敼鏂囦欢



- include/streaming/http_stream_downloader.h



- src/streaming/http_stream_downloader.cpp



- src/main.cpp



- tools/start_streaming_fixture_server.ps1



- samples/README.md



- samples/streaming/hls_local/sample.m3u8



- samples/streaming/hls_local/segment000.ts



- samples/streaming/hls_local/segment001.ts



- samples/streaming/hls_local/segment002.ts



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---











---







## 闂 60: 7.3 HLS/DASH 鑷€傚簲鐮佺巼







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `7.3` 瑕佹眰鎶婃祦濯掍綋鑳藉姏浠庘€滃浐瀹氭竻鍗?smoke鈥濇帹杩涘埌 HLS/DASH 澶氱爜鐜囪В鏋愩€佹。浣嶉€夋嫨涓庡彲閲嶅鍥炲綊銆?






### 鍘熷洜鍒嗘瀽



- `HlsManifestParser` 鍙兘璇诲彇濯掍綋鎾斁鍒楄〃锛屾棤娉曡瘑鍒?`master playlist` 鐨?variant 淇℃伅銆?


- `DashManifestParser` 涔嬪墠鍙彁鍙?`Representation` 甯﹀锛岀己灏?`BaseURL`銆佸垵濮嬪寲鍒嗙墖涓庡獟浣撳垎鐗囨槑缁嗐€?


- 涓荤▼搴忕己灏戠粺涓€鐨?ABR 閫夋嫨閫昏緫鍜屾湰鍦伴獙鏀跺叆鍙ｏ紝鏃犳硶楠岃瘉鍗囩爜鐜?闄嶇爜鐜囧垏鎹㈣矾寰勩€?






### 瑙ｅ喅鏂规



- 鎵╁睍 HLS/DASH 瑙ｆ瀽鍣紝琛ラ綈澶氱爜鐜囨竻鍗曘€佽〃绀洪泦涓庡垎鐗囨槑缁嗐€?


- 鏂板 `AdaptiveBitrateSelector`锛屾寜浼扮畻甯﹀閫夋嫨鏈€鍖归厤鐨勬。浣嶏紝骞跺湪 `main` 涓鍔?`--adaptive-bitrate-check`銆?


- 鏂板 `samples/streaming/abr_local/{hls,dash}` 澶瑰叿锛屽鐢ㄦ湰鍦?HTTP 鏈嶅姟楠岃瘉 HLS/DASH 鐨勫崌闄嶆。涓庡垎鐗囦笅杞姐€?






### 淇敼鏂囦欢



- include/streaming/hls_manifest_parser.h



- src/streaming/hls_manifest_parser.cpp



- include/streaming/dash_manifest_parser.h



- src/streaming/dash_manifest_parser.cpp



- include/streaming/adaptive_bitrate_selector.h



- src/streaming/adaptive_bitrate_selector.cpp



- src/main.cpp



- CMakeLists.txt



- tools/start_streaming_fixture_server.ps1



- samples/README.md



- samples/streaming/abr_local/hls/master.m3u8



- samples/streaming/abr_local/hls/low/index.m3u8



- samples/streaming/abr_local/hls/low/segment000.ts



- samples/streaming/abr_local/hls/low/segment001.ts



- samples/streaming/abr_local/hls/medium/index.m3u8



- samples/streaming/abr_local/hls/medium/segment000.ts



- samples/streaming/abr_local/hls/medium/segment001.ts



- samples/streaming/abr_local/hls/high/index.m3u8



- samples/streaming/abr_local/hls/high/segment000.ts



- samples/streaming/abr_local/hls/high/segment001.ts



- samples/streaming/abr_local/dash/sample.mpd



- samples/streaming/abr_local/dash/low/init.mp4



- samples/streaming/abr_local/dash/low/segment000.m4s



- samples/streaming/abr_local/dash/low/segment001.m4s



- samples/streaming/abr_local/dash/medium/init.mp4



- samples/streaming/abr_local/dash/medium/segment000.m4s



- samples/streaming/abr_local/dash/medium/segment001.m4s



- samples/streaming/abr_local/dash/high/init.mp4



- samples/streaming/abr_local/dash/high/segment000.m4s



- samples/streaming/abr_local/dash/high/segment001.m4s



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- docs/README.md



- docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---







## 闂 61: 寤虹珛閲岀▼纰戞爣绛撅紙v0.2.0-rc1 / v0.2.0锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟 `0.4` 瑕佹眰涓哄綋鍓嶉樁娈靛缓绔?`v0.2.0-rc1` 涓?`v0.2.0` 閲岀▼纰戞爣绛撅紝浣嗕粨搴撲腑姝ゅ墠娌℃湁浠讳綍 Git 鏍囩銆?






### 鍘熷洜鍒嗘瀽



- 鍙戝竷闂ㄧ鍜岄樁娈垫€ц兘鍔涘凡缁忔敹鍙ｏ紝浣嗙増鏈噷绋嬬缂哄皯鍙拷婧殑 Git 鏍囪銆?


- 宸窛璇勪及涓庝换鍔℃竻鍗曚粛淇濈暀鈥滃彧宸爣绛炬搷浣溾€濈殑鏃у彛寰勶紝闇€瑕佷笌瀹為檯浠撳簱鐘舵€佸悓姝ャ€?






### 瑙ｅ喅鏂规



- 鍦ㄤ富绾跨ǔ瀹氬揩鐓т笂寤虹珛 `v0.2.0-rc1` 涓?`v0.2.0` 涓や釜閲岀▼纰戞爣绛俱€?


- 鍚屾鏇存柊 `VERSION / DEVELOP_LOG / MPC_HC_GAP_ANALYSIS / tasklist`锛岃褰曟爣绛惧凡寤虹珛銆?


- 鍩轰簬 `v0.2.0-rc1` 宸叉垚鍔熷缓绔嬭繖涓€浜嬪疄锛屽悓姝ュ嬀閫夋墽琛岀害鏉?`5.3 姣忎釜閲岀▼纰戠粨鏉熷墠蹇呴』鍙墦 RC 鏍囩`銆?






### 淇敼鏂囦欢



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md



- docs/analysis/MPC_HC_GAP_ANALYSIS.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---







## 闂 62: 鎵ц瀹堝垯鍙ｅ緞鍚屾锛?.1 / 5.2锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟涓殑鎵ц瀹堝垯 `5.1 / 5.2` 浠嶆湭鏇存柊锛屼絾褰撳墠浠撳簱鐘舵€佸凡缁忚冻浠ュ垽鏂叾涓竴閮ㄥ垎绾︽潫鏄惁婊¤冻銆?






### 鍘熷洜鍒嗘瀽



- `5.1` 鍏虫敞鐨勬槸鏈疆骞惰宸ヤ綔閲忔帶鍒讹紝鑳戒粠瀹為檯浠诲姟鎺ㄨ繘椤哄簭涓緱鍒拌瘉鎹€?


- `5.2` 鍏虫敞鐨勬槸鎸夊懆鑺傚鎵ц鈥滃彧鍋氭敹鏁涒€濓紝闇€瑕佽法鍛ㄣ€侀噸澶嶆€х殑杩囩▼璇佹嵁锛屼笉鑳戒粎鍑竴娆′氦浠樻敹鍙ｇ洿鎺ュ嬀閫夈€?






### 瑙ｅ喅鏂规



- 鍕鹃€?`5.1 WIP 闄愬埗锛氬悓鏃惰繘琛屼换鍔′笉瓒呰繃 2 涓猔銆?


- 淇濈暀 `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 涓哄緟瀹屾垚锛屽苟鍦ㄦ枃妗ｄ腑鏄庣‘鍘熷洜銆?






### 淇敼鏂囦欢



- docs/records/VERSION.md



- docs/records/CHANGELOG.md



- docs/records/DEVELOP_LOG.md



- docs/README.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md







---







## 闂 63: 钀藉湴 5.2 鍛ㄤ簲鏀舵暃鏃ユ墽琛屾墜鍐?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 浠诲姟娓呭崟涓殑 `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 浠嶅仠鐣欏湪鍘熷垯鍙ｅ緞锛岀己灏戝彲鎵ц鐨勫懆鑺傚涓庢敹鏁涙棩绾︽潫銆?






### 鍘熷洜鍒嗘瀽



- 鐜版湁鏂囨。宸茬粡瑕嗙洊鍥炲綊鍛戒护鍜岄樁娈佃鍒掞紝浣嗚繕娌℃湁鎶娾€滃懆浜斿厑璁镐粈涔堛€佺姝粈涔堛€佺粨鏉熸椂瑕佷骇鍑轰粈涔堚€濆啓鎴愬浐瀹氭祦绋嬨€?


- 濡傛灉娌℃湁杩欏眰娴佺▼绾︽潫锛屽嵆浣垮綋鍓嶅彛寰勬纭紝鍚庣画涔熷緢闅剧ǔ瀹氱Н绱法鍛ㄦ墽琛岃瘉鎹€?






### 瑙ｅ喅鏂规



- 鏂板 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`锛屾妸 `5.2` 鍥哄寲涓哄彲鐩存帴鎵ц鐨勫懆鑺傚璇存槑涓庡懆浜旀敹鏁涙墜鍐屻€?


- 鏇存柊 `docs/README.md` 涓庤褰曟枃妗ｏ紝鏄庣‘ `5.2` 鐜伴樁娈靛畬鎴愮殑鏄€滄祦绋嬭惤鍦扳€濓紝鑰屼笉鏄€滀换鍔″嬀閫夊畬鎴愨€濄€?






### 淇敼鏂囦欢



- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂 64: 琛ラ綈 5.2 鐣欑棔妯℃澘锛坉aily_board / 鍛ㄦ姤锛?






**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- `5.2` 鐨勬墽琛屾墜鍐屽凡缁忚惤鍦帮紝浣?`daily_board` 鍜屽懆鎶ュ眰闈粛缂哄皯鍥哄畾妯℃澘锛屽鑷村悗缁法鍛ㄨ瘉鎹笉鏄撶粺涓€鐣欏瓨銆?






### 鍘熷洜鍒嗘瀽



- 鍙湁娴佺▼璇存槑锛屾病鏈変綆鎴愭湰銆佸浐瀹氭牸寮忕殑濉啓妯℃澘锛屾墽琛屾椂瀹规槗鍙ｅ緞婕傜Щ銆?


- `5.2` 鏄惁鍕鹃€夊彇鍐充簬璺ㄥ懆璇佹嵁锛屽洜姝ゆā鏉挎湰韬篃鏄畧鍒欒惤鍦扮殑涓€閮ㄥ垎銆?






### 瑙ｅ喅鏂规



- 缁?`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 鐨勪袱涓懆浜旇ˉ涓婃敹鏁涙棩璁板綍鍗°€?


- 鏂板 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md` 浣滀负姣忓懆鏀舵暃/鍛ㄦ姤妯℃澘銆?


- 鍚屾鏇存柊 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md` 鍜?`docs/README.md` 鐨勫叆鍙ｈ鏄庛€?






### 淇敼鏂囦欢



- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md



- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md



- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md



- docs/README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂 65: 姹囨€诲綋鍓嶅姛鑳姐€佷娇鐢ㄦ柟寮忎笌楠岃瘉鍏ュ彛







**鏃ユ湡**: 2026-03-08







### 闂鎻忚堪



- 闇€瑕佹妸绋嬪簭褰撳墠宸茬粡瀹炵幇鐨勫姛鑳姐€佸彲鐢ㄧ殑浣跨敤鏂瑰紡锛屼互鍙婄幇鏈夐獙璇佽矾寰勯泦涓啓鎴愪竴浠藉彲鏌ユ枃妗ｏ紝閬垮厤淇℃伅鏁ｈ惤銆?






### 鍘熷洜鍒嗘瀽



- 褰撳墠鑳藉姏宸茬粡妯法鈥滄甯告挱鏀俱€佽瘖鏂懡浠ゃ€佷笓椤归獙鏀躲€佹彃浠?娴佸獟浣撳熀纭€璁炬柦鈥濓紝浣嗗叆鍙ｅ垎鏁ｅ湪澶氫釜鏂囨。涓庢簮鐮佸府鍔╄緭鍑轰腑銆?


- 濡傛灉娌℃湁缁熶竴鎬昏锛屽悗缁户缁淮鎶ゆ枃妗ｆ椂瀹规槗閬楁紡鈥滃姛鑳解€濆拰鈥滈獙璇佲€濅箣闂寸殑瀵瑰簲鍏崇郴銆?






### 瑙ｅ喅鏂规



- 鏂板 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`锛岀粺涓€璁板綍褰撳墠鍔熻兘銆佷娇鐢ㄦ柟寮忋€侀厤缃叆鍙ｃ€佷笓椤归獙鏀跺懡浠や笌鎶ュ憡鏄犲皠銆?


- 鏇存柊 `docs/README.md` 鍜屾牴 `README.md`锛屽鍔犺鎬昏鏂囨。鐨勫叆鍙ｃ€?






### 淇敼鏂囦欢



- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md



- docs/README.md



- README.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md















---







## 闂 69: 鎾斁閾捐瘖鏂垎灞備笌 decoder drain / scheduler 瀹归敊琛ュ己







**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- 楂樼爜鐜囨挱鏀剧ǔ瀹氭€ф帓鏌ラ渶瑕佹槑纭尯鍒嗏€滅湡姝ｇ殑鑳屽帇/鍏ラ槦澶辫触鈥濆拰鈥滈潪鐩爣娴佸寘琚拷鐣モ€濓紝骞惰ˉ榻?decoder drain銆乶ative path 鍛戒腑鐜囦笌 scheduler 瀹归敊杈圭晫鐨勫彲瑙傛祴鎬с€?


- 鏃у疄鐜颁腑 packet queue EOF 鍚庢病鏈夊悜 codec 鍙戦€?`nullptr` drain锛屼笖 send 鍚庡彧鍋氫竴娆?receive锛屽鏄撴妸鈥滄殏鏃舵棤杈撳嚭鈥濆拰鈥滅湡姝ｅけ璐モ€濇贩鍦ㄤ竴璧枫€?






### 鍘熷洜鍒嗘瀽



- 璇婃柇蹇収姝ゅ墠鍙湁 `demux_dropped_packets` 鎬婚噺锛屾病鏈夌粏鍒?drop 鍘熷洜銆?


- scheduler 鍙繚鎶や簡瑙ｇ爜绾跨▼锛宺ender thread 娌℃湁鍚屾牱鐨勫紓甯镐繚鎶わ紱restart 娆℃暟鍜岃儗鍘嬮鐜囦篃娌℃湁缁撴瀯鍖栧鍑恒€?


- 鍗充娇涓婚摼宸插叿澶囨潯浠跺紡 D3D11 native path锛岃繍琛屾椂涔熺己灏?`native / copy-back / swscale` 璺緞璁℃暟锛岄毦浠ョ洿鎺ラ獙璇佸綋鍓嶇儹鐐广€?






### 瑙ｅ喅鏂规



- 閲嶅啓 video/audio decoder 鐨?drain/feed 寰幆锛屽苟鍦?packet EOF 鍚庡 codec 鍙戦€?`nullptr` 瑙﹀彂 drain銆?


- 鍦?`PlayerCore` 涓鍔?demux drop 鍒嗙被銆乨ecoder `send_packet(EAGAIN)`銆乨rain 娆℃暟鍜岃棰戣緭鍑鸿矾寰勮鏁般€?


- 鍦?`Scheduler` 涓鍔犺儗鍘嬩笌 restart 缁熻锛屽苟鎶?render thread 绾冲叆 `runProtectedLoop()`锛況estart 涓婇檺鏀惧涓烘湁闄愮殑澶氭灏濊瘯銆?


- 鎵╁睍 `--performance-log-check` 杈撳嚭锛屽鍑烘柊鐨勭粨鏋勫寲璇婃柇瀛楁銆?






### 淇敼鏂囦欢



- include/core/scheduler.h



- src/core/scheduler.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---







## 闂 70: PlayerCore 鐘舵€佹満閲嶈璁＄涓€闃舵







**鏃ユ湡**: 2026-03-19







### 闂鎻忚堪



- `PlayerCore` 涔嬪墠鍙湁瀵瑰 `PlaybackState::Stopped / Playing / Paused` 涓夋€侊紝浣嗗唴鏍稿疄闄呰繕闅愬惈浜嗕細璇濇€併€佽繍琛屾€併€佹祦姘寸嚎杩囩▼鎬佸拰 deferred stop 鏃佽矾璇箟銆?


- `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 鐩存帴鏁ｇ偣鏀?`state_`锛屽鑷寸姸鎬佸彉鍖栨病鏈夌粺涓€鍏ュ彛锛屼篃缂哄皯闈炴硶杩佺Щ淇濇姢涓庣粺涓€鏃ュ織銆?






### 鍘熷洜鍒嗘瀽



- UI 鎾斁鎬佸拰鍐呮牳杩愯璇箟闀挎湡娣峰湪涓€涓灇涓鹃噷锛屽鑷?`PlaybackState` 琚揩鎵胯浇杩囧鍚箟銆?


- `deferred stop` 涔嬪墠鏄嫭绔嬪竷灏斾綅锛屼笉鍦ㄧ粺涓€鐘舵€佹満鍐咃紝鐘舵€佽瀵熻鍚屾椂鎷兼帴 `state_` 鍜屾梺璺爣蹇椼€?


- `Scheduler` 褰撳墠鍙悊瑙?`running_ / paused_`锛屽洜姝ょ涓€闃舵搴斿厛鎶婁笟鍔＄姸鎬佹潈濞佹潵婧愭敹鍥?`PlayerCore`锛岃€屼笉鏄彁鍓嶆敼 `Scheduler` 濂戠害銆?






### 瑙ｅ喅鏂规



- 鍦?`PlayerCore` 鍐呴儴鏂板 `SessionState / RunState / PipelinePhase` 鍜?`CoreStateSnapshot`锛屾妸浼氳瘽鎬併€佽繍琛屾€併€佹祦姘寸嚎鎬佹媶寮€寤烘ā銆?


- 鏂板 `transitionSessionState / transitionRunState / transitionPipelinePhase / publishPlaybackStateFromInternalState`锛屾敹鍙ｅ澶?`PlaybackState` 鍙樻洿銆?


- 鎶?`eof_reached / pending_seek / deferred_stop_pending` 绾冲叆缁熶竴蹇収绠＄悊锛屽苟涓虹姸鎬佽縼绉昏緭鍑烘棩蹇楀拰闈炴硶杩佺Щ warning銆?


- 鏈疆淇濇寔澶栭儴 `PlaybackState` 鎺ュ彛鍏煎锛屼笉寮曞叆 timeline serial锛屼笉鎻愬墠鎶?EOF 鏀规垚 `Ended`銆?






### 鏈湴楠屾敹缁撴灉



- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃銆?






### 淇敼鏂囦欢



- include/core/player_core.h



- src/core/player_core.cpp



- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md







---







## 闂 81: PlayerCore seek/flush timeline serial 鍖栫浜岄樁娈?






**鏃ユ湡**: 2026-03-20







### 闂鎻忚堪



- 绗竴闃舵瀹屾垚鍚庯紝`PlayerCore` 宸茬粡鎶?UI 鎾斁鎬佸拰鍐呮牳鐘舵€佹媶灞傦紝浣?seek/flush 浠嶄富瑕佷緷璧?`scheduler pause -> stopDemuxThread -> flushPipelines -> avcodec_flush_buffers() -> audio_player_->stop() -> 浜屾 flush` 杩欑被杞竻鐞嗚矾寰勩€?


- `ThreadSafeQueue` 鍙湁 `stop/start/eof/clear`锛宍FrameQueue` 鍙湁 `flush()`锛宲acket/frame 閮芥病鏈?timeline serial锛屽鑷存棫鏃堕棿绾挎暟鎹嵆浣跨┛杩?seek/stop 杈圭晫锛屼篃鍙兘闈犫€滅宸ц娓呯┖鈥濇潵澶辨晥銆?


- audio consumer 绾跨▼鍜?render 璺緞鍦ㄨ繖涔嬪墠閮芥病鏈?serial 闃茬嚎锛岃繛缁?seek銆佹殏鍋滄€?seek銆乴ate worker 鏀跺熬鏃朵粛鍙兘鍑虹幇鏃ф畫闊炽€佹棫娈嬪抚鎴栦吉 EOF銆?






### 鍘熷洜鍒嗘瀽



- seek/flush 涔嬪墠鍙湁鐗╃悊娓呮壂锛屾病鏈夆€滆繖鏉℃暟鎹睘浜庡摢鏉℃椂闂寸嚎鈥濈殑鏄惧紡韬唤锛宒ecoder銆乺enderer銆乤udio consumer 鏃犳硶纭垽瀹氭暟鎹槸鍚﹁繃鏈熴€?


- 鏃?demux 宸ヤ綔銆佹棫 codec 鍐呯紦瀛樺抚銆佹棫 frame queue 鏁版嵁鍜屾棫 audio submit 閮界己灏戠粺涓€鐨勬椂闂寸嚎杈圭晫銆?


- `flush` 鏈韩鍙兘灏介噺闄嶄綆娉勬紡姒傜巼锛屼笉鑳藉畾涔夌粷瀵圭殑搴熷純瑙勫垯銆?






### 瑙ｅ喅鏂规



- 鏂板 `TimelineSerial`锛屽苟璁?packet/frame 閮芥樉寮忔惡甯?serial锛歚DemuxPacket { PacketPtr packet; TimelineSerial serial; }`銆乣VideoFrame::serial`銆乣AudioFrame::serial`銆?


- 鍦?`PlayerCore` 鍐呴儴鏂板 `timeline_serial / pending_seek_serial`锛屼互鍙婄粺涓€鐨?`allocateNextTimelineSerial / activateTimelineSerial / setPendingSeekSerial / isAcceptedTimelineSerial` 鍏ュ彛銆?


- `open` 鎴愬姛鏃跺缓绔嬮涓?serial锛沗seek` 鍏堝垎閰?`pending_seek_serial`锛宻eek 鎴愬姛鍚庡啀婵€娲伙紱`stop / requestDeferredStop` 鐩存帴鎺ㄨ繘 serial锛屼娇鏃?worker 鏅氬埌鏃朵篃鍙兘浜у嚭搴熸暟鎹€?


- demux 绾跨▼鍚姩鏃舵崟鑾峰綋鍓?serial锛岄伩鍏嶆棫 demux 宸ヤ綔琚鏍囨垚鏂版椂闂寸嚎銆?


- `decodeVideoFrame / decodeAudioFrame / renderFrame / renderPausedFrameAtOrAfter / audio consumer` 鍏ㄩ摼璺帴鍏?serial 鍒ゅ畾锛屾棫 serial 鐩存帴涓㈠純锛沗flush` 淇濈暀锛屼絾闄嶇骇涓鸿緟鍔╂竻鎵€?


- `DiagnosticsSnapshot`銆佺姸鎬佹棩蹇楀拰涓撻」妫€鏌ュ懡浠ゆ柊澧?`timeline_serial / pending_seek_serial` 瑙傛祴瀛楁銆?






### 鏈湴楠屾敹缁撴灉



- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃銆?






### 淇敼鏂囦欢



- include/core/frame.h



- src/core/frame.cpp



- include/core/player_core.h



- src/core/player_core.cpp



- src/main.cpp



- docs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md



- docs/records/CHANGELOG.md



- docs/records/VERSION.md



- docs/records/DEVELOP_LOG.md











---



---



## ?? 82: PlayerCore EOF/Ended ????????



**??**: 2026-03-20



### ????

- ????????seek/stop ???????? serial ???? EOF ?????????????????? stop/deferred-stop ??????? `Stopped`???????? `Ended` ???

- ??????????????`close()` ? `stop()` ???????????????scheduler ???? render callback ??? `rendered_frames`??? stale frame ?????????



### ????

- ??? `PlaybackState` ?????????? stop????? EOF ??????????????????????

- EOF ??????? demux/queue ?????????????????????????

- ???????? serial ????? `close/reopen` ????? stop ??????? scheduler ???????????????????? present??



### ????

- ? `PlayerCore` ?????? `EndedReason`?????? `CoreStateSnapshot`?`DiagnosticsSnapshot` ?????????? `PlaybackState` ?????

- ?? `onRenderIdle()` ? EOF ?????? `PipelinePhase::Draining`??? packet queue EOF + frame queue ?? + ????????????? `RunState::Ended`???? `deferred stop` ?? `Stopped`?

- `play()` ? `Ended` ???? `seek(0.0)` ????`seek()` ? `Ended` ????? `Stopped`???? ended ??/???????????

- `close()` ? `stop()` ??????? timeline serial???? worker ??????? stale serial ?????

- ?? scheduler render callback ? `bool` ?????????? render/present ????? `rendered_frames` ? `last_render_wall_tp_`?



### ??????

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`????



### ????

- include/core/player_core.h

- include/core/scheduler.h

- src/core/player_core.cpp

- src/core/scheduler.cpp

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY18_EOF_ENDED_AND_TERMINAL_STATE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

---

## ?? 83: PlayerCore queue generation ? Scheduler ????????

**??**: 2026-03-20

### ????
- ????????? item-level serial ? EOF/Ended ?????? queue ????? `clear()/flush()` ?????????????? generation/epoch ???????? producer/consumer?
- `Scheduler` ????? `running_ / paused_`?? `Seeking / Flushing / Stopping / Ended` ???????????render loop ? clock wait ??????????????

### ????
- item-level serial ?????????????????????? queue ???? generation???????? `push()/pop()` ?????????????
- scheduler ?????? `RunState / PipelinePhase / accepted timeline serial`?????? callback ???????????/???????????????

### ????
- ? `ThreadSafeQueue` ? `FrameQueue` ?? generation?`clear()/flush()` ?? generation????? `push()/pop()` ????? generation ???????????
- ? `scheduler.h` ????? `SchedulerControlSnapshot`???? `run_state / pipeline_phase / accepted_timeline_serial`?????? scheduler ??????????
- `PlayerCore` ?? `makeSchedulerControlSnapshot()` ???????? `setControlSnapshotProvider()` ??? scheduler?
- ??/?? decode loop ???????? run/phase ??????render loop ? pop ????? clock wait ??????????? accepted serial????? callback ???????????
- `DiagnosticsSnapshot`??? diagnostics ????????? packet/frame queue generation ????????? flush ???

### ??????
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`????

### ????
- include/thread_safe_queue.h
- include/core/frame_queue.h
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY19_QUEUE_GENERATION_AND_SCHEDULER_CONTROL_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

---

## 闂 84: PlayerCore 鍓綔鐢ㄩ泦涓寲涓?runtime failure/recovery policy 鏀跺彛

**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- timeline serial 鍜?queue generation 宸茬粡鎶?seek/flush 杈圭晫纭寲锛屼絾 `play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 鍏ュ彛閲屼粛鐒舵贩鐫€绾跨▼銆佽澶囥€侀槦鍒楀拰鏃堕挓鍓綔鐢ㄣ€?- `SchedulerControlSnapshot` 涔嬪墠鍙鐩?`run_state / pipeline_phase / accepted_timeline_serial`锛宻cheduler 浠嶉渶瑕佷粠澶栭儴璇鐚?`clock_source`銆乤udio-master 鏄惁鐪熺殑鏈夋晥锛屼互鍙?`Ended` 鏃舵槸鍚﹀厑璁镐繚鏈€鍚庝竴甯с€?- decode/resample/output 鐨?fatal 鐐逛粛瀹规槗闀垮嚭鍚勮嚜鐨?`emitError + return false` 璺緞锛屾仮澶嶇瓥鐣ョ己灏戠粺涓€鍏ュ彛銆?
### 鍘熷洜鍒嗘瀽
- 绗竴闃舵鏀跺彛鐨勬槸鈥滅姸鎬佽縼绉诲啓鍏ョ偣鈥濓紝浣嗚繕娌℃湁鎶娾€滅姸鎬佽縼绉昏Е鍙戠殑鍔ㄤ綔鈥濇娊绂绘垚缁熶竴鍓綔鐢ㄦā鍨嬨€?- `deferred stop` 鐨勬湰璐ㄦ槸寮傛瀹屾垚 stop锛岃€屼笉鏄彟涓€濂楀仠鏈轰笟鍔¤涔夛紱濡傛灉 request/completion 閫昏緫涓嶇粺涓€锛屽悗缁緢瀹规槗鍐嶆鍒嗗弶銆?- scheduler 鑻ョ户缁潬闆舵暎甯冨皵浣嶆嫾涓氬姟璇箟锛屽氨浼氭妸 `clock_source`銆乤udio-master 涓?ended policy 缁х画鏁ｈ惤鍦?loop 鍐呴儴銆?- runtime failure 鑻ヤ笉鍏堢粺涓€鎴?policy锛屽搴旂殑 recovery path 浼氶殢鐫€鏇村 fatal 鐐圭户缁墿鏁ｃ€?
### 瑙ｅ喅鏂规
- 鎵?`SchedulerControlSnapshot`锛氭柊澧?`clock_source`銆乣audio_output_initialized`銆乣audio_master_sync_active`銆乣ended_policy`锛屽苟璁?scheduler 鐩存帴娑堣垂杩欎簺缁撴瀯鍖栫害鏉熴€?- 鍦?`PlayerCore` 鏂板缁熶竴 helper锛歚applyStartPlaybackSideEffects`銆乣applyResumePlaybackSideEffects`銆乣applyPausePlaybackSideEffects`銆乣applyStopRequestSideEffects`銆乣applyStopCompletionSideEffects`銆乣applySeekSideEffects`銆乣applySessionReleaseSideEffects`銆?- `requestDeferredStop()` 涓?`serviceDeferredStop()` 鏀逛负澶嶇敤鍚屼竴濂?stop-request / stop-completion helpers锛屾妸 deferred stop 骞跺洖缁熶竴 stopping 璺緞銆?- 鏂板 `FailureRecoveryPolicy` 鍜?`handleRuntimeFailure()`锛屾妸 decode/resample/output 鍏抽敭 fatal 鐐圭粺涓€鏀跺彛鍒?`EmitOnly / StopPlayback / FailSession` 绛栫暐鍏ュ彛銆?
### 鏈湴楠屾敹缁撴灉
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂 85: PlayerCore 鍓╀綑椋庨櫓鏀舵暃锛歋cheduler 缁堢増绛栫暐銆丗ailSession 瀹炲寲涓?serial/generation 瑙傛祴寮哄寲

**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- `SchedulerControlSnapshot` 浠嶆湭褰㈡垚缁堢増绛栫暐琛ㄨ揪锛宑lock/audio-master/ended 璇箟杩樻湁闅愬紡鎺ㄥ銆?- `FailSession` 铏芥湁缁熶竴鍏ュ彛锛屼絾鍏抽敭澶辫触鐐瑰皻鏈繘鍏ュ疄闄呰鐩栥€?- queue generation 涓?item-level serial 鐨勮亴璐ｈ竟鐣岀己灏戝彲瑙傛祴浣愯瘉銆?
### 鍘熷洜鍒嗘瀽
- scheduler 涔嬪墠浠嶄富瑕佹秷璐?`clock_source + bool`锛屽鑷寸瓥鐣ユ剰鍥句笉澶熸樉寮忋€?- `FailSession` 璋冪敤鐐逛笉瓒筹紝鎭㈠璺緞鍦ㄧ湡瀹為珮椋庨櫓閿欒涓婁粛鍋忓悜 `StopPlayback`銆?- diagnostics 缂哄皯 stale serial 涓㈠純璁℃暟锛岄毦浠ョ洿鎺ヨ瘉鏄庣‖澶辨晥涓诲垽瀹氭潵鑷?serial銆?
### 瑙ｅ喅鏂规
- 鎵?`SchedulerControlSnapshot`锛氭柊澧?`SchedulerClockPolicy`銆乣SchedulerAudioMasterPolicy`銆乣audio_buffered_seconds`锛屽苟鎵╁睍 `SchedulerEndedPolicy`銆?- `Scheduler` 澧炲姞绛栫暐娑堣垂鍑芥暟 `isAudioMasterActive / isVideoMasterActive / shouldApplyClockSync`锛屽苟琛?`Scheduler::stop()` self-join 闃茬嚎銆?- `handleRuntimeFailure()` 鏀跺彛澧炲己锛歚StopPlayback`/`FailSession` 鍧囩粺涓€璧?stop request side effects锛沗FailSession` 瑕嗙洊鍏抽敭涓嶅彲鎭㈠澶辫触鐐广€?- 鏂板 stale serial 涓㈠純璁℃暟骞舵帴鍏?`DiagnosticsSnapshot`銆佷綆棰戞棩蹇椼€乣--performance-log-check` 涓?`--software-video-decode-check`銆?
### 鏈湴楠屾敹缁撴灉
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
### 淇敼鏂囦欢
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂 86: 澧炶ˉ serial/failsession 鍥炲綊鎺㈤拡锛堣繛缁?seek銆佹殏鍋滄€?seek銆乧lose/reopen锛?
**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- 闇€瑕佷负 `FailSession + timeline serial` 鏀舵暃闃舵琛ュ厖鏈哄櫒鍙鍥炲綊妫€鏌ワ紝瑕嗙洊锛?  1. 杩炵画 seek
  2. 鏆傚仠鎬?seek
  3. close/reopen
- 鐜版湁妫€鏌ラ」铏界劧鑳借緭鍑?diagnostics锛屼絾灏氭湭鎶婅繖涓夌被杈圭晫鍦烘櫙鑱氬悎鎴愬彲鐩存帴 gate 鐨?`key=value + result=PASS/FAIL`銆?
### 鍘熷洜鍒嗘瀽
- 鐜版湁 `--performance-log-check` / `--software-video-decode-check` 鍋忓悜閾捐矾鍋ュ悍蹇収锛屼笉鐩存帴琛ㄨ揪鈥滆竟鐣屽墠鍚?stale 澧為噺 + serial 杩佺Щ + FailSession 闈炴硶璺宠浆绾︽潫鈥濄€?- `PlayerCore` 鍐呴儴闈炴硶杩佺Щ姝ゅ墠浠呮棩蹇楀憡璀︼紝娌℃湁 diagnostics 璁℃暟瀛楁锛孋LI 涓嶆槗鏈哄櫒鍒ゅ畾銆?
### 瑙ｅ喅鏂规
- 鍦?`DiagnosticsSnapshot` 澧炲姞骞舵槧灏勯潪娉曡縼绉昏鏁帮細
  - `illegal_session_transitions`
  - `illegal_run_transitions`
  - `illegal_pipeline_transitions`
- 鍦?`PlayerCore` 鐨?`transitionSessionState / transitionRunState / transitionPipelinePhase` 閲屽闈炴硶杩佺Щ璁℃暟锛屽苟鍦?diagnostics reset/鏃ュ織涓帴鍏ャ€?- 鍦?`src/main.cpp` 鏂板涓変釜 CLI 鍥炲綊鍛戒护锛?  - `--seek-burst-serial-check <media_file> [seek_count]`
  - `--paused-seek-serial-check <media_file> [seek_count]`
  - `--close-reopen-serial-check <media_file> [sample_ms]`
- 姣忎釜鍛戒护杈撳嚭缁熶竴 `key=value`锛屽苟缁欏嚭 `result=PASS/FAIL`锛涘垽瀹氭牳蹇冭鐩栵細
  - serial 杩佺Щ鏄惁鎸佺画鎺ㄨ繘
  - stale 鏄惁浠呭湪杈圭晫绐楀彛鍑虹幇銆佺ǔ瀹氱獥鍙ｆ槸鍚︽敹鏁?  - `FailSession` 瑙﹀彂鏃舵槸鍚﹀瓨鍦ㄩ潪娉曡縼绉伙紙`fail_session_transition_ok`锛?- 鍚屾鎶婇潪娉曡縼绉昏鏁板鍑哄埌锛?  - `--performance-log-check`
  - `--software-video-decode-check`

### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓锛?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缁撴灉锛氶€氳繃锛宍0 warnings / 0 errors`
- 鏂板妫€鏌ュ懡浠わ紙鏍锋湰锛歚juren-30s.mp4`锛夛細
  - `build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4`锛歚PASS`
  - `build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4`锛歚PASS`
  - `build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4`锛歚PASS`

### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂 87: serial/failsession 鍥炲綊澧炲姞涓€閿仛鍚?gate锛堥檷浣庢紡璺戦闄╋級

**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- 宸叉湁 `--seek-burst-serial-check`銆乣--paused-seek-serial-check`銆乣--close-reopen-serial-check` 涓変釜鎺㈤拡锛屼絾鎵ц鏃朵粛闇€浜哄伐涓茶璋冪敤銆?- 鍦ㄩ珮棰戣凯浠ｉ樁娈碉紝浜哄伐涓茶鎵ц瀛樺湪婕忚窇鏌愪竴椤圭殑椋庨櫓锛屼笉鍒╀簬绋冲畾 gate銆?
### 鍘熷洜鍒嗘瀽
- 涓変釜鎺㈤拡鐨勫垽瀹氬彛寰勫凡缁忕粺涓€涓?`key=value + result=PASS/FAIL`锛屼絾缂哄皯缁熶竴鑱氬悎鍏ュ彛銆?- 鑻ユ病鏈夎仛鍚堝叆鍙ｏ紝璋冪敤鏂归渶瑕佽嚜琛岀淮鎶ら『搴忋€佸弬鏁板拰鎬荤粨鏋滐紝瀹规槗鍑虹幇鑴氭湰涓嶄竴鑷淬€?
### 瑙ｅ喅鏂规
- 鍦?`src/main.cpp` 鏂板鑱氬悎鍛戒护锛?  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 鑱氬悎鍛戒护鍐呴儴椤哄簭鎵ц涓夋潯鐜版湁鎺㈤拡锛屽苟杈撳嚭缁熶竴鎬荤粨鏋滃瓧娈碉細
  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`

### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓锛?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缁撴灉锛氶€氳繃锛宍0 warnings / 0 errors`
- 鑱氬悎鍛戒护鏍锋湰楠岃瘉锛坄juren-30s.mp4`锛夛細
  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`
  - 缁撴灉锛歚serial-failsession-regression-check.pass_count=3`銆乣serial-failsession-regression-check.total_count=3`銆乣serial-failsession-regression-check.result=PASS`

### 淇敼鏂囦欢
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂 88: 寮哄埗 FailSession 鍥炲綊鎺㈤拡涓?codec 閿侀噸鍏ュ穿婧冧慨澶?
**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- 鐜版湁 serial/failsession 鍥炲綊涓昏瑕嗙洊姝ｅ父閾捐矾鍜岃竟鐣屽垏鎹紝浣?`FailSession` 浠嶄緷璧栫湡瀹炲紓甯歌Е鍙戯紝缂哄皯绋冲畾鍙噸澶嶇殑寮哄埗瑕嗙洊銆?- 鍦ㄦ柊澧炲己鍒惰矾寰勬帰閽堣繃绋嬩腑锛屾毚闇插嚭 `FailSession` 浠庤В鐮佺嚎绋嬭繘鍏ユ椂鐨勮祫婧愬洖鏀跺紓甯革細`device or resource busy`銆?
### 鍘熷洜鍒嗘瀽
- 缂哄皯鈥滄祴璇曚笓鐢ㄣ€佸彲鎺цЕ鍙戔€濈殑 `FailSession` 娉ㄥ叆鐐癸紝瀵艰嚧璇ヨ矾寰勫緢闅剧ǔ瀹?gate銆?- `FailSession` 閲婃斁璧勬簮鏃朵細杩涘叆 decoder 閲婃斁閫昏緫锛岃€岃璺緞鍙兘涓庤В鐮佺嚎绋嬪凡鎸佹湁鐨?codec 閿佸彂鐢熷悓绾跨▼閲嶅叆鍐茬獊銆?
### 瑙ｅ喅鏂规
- `PlayerCore::decodeVideoFrame` 澧炲姞娴嬭瘯娉ㄥ叆寮€鍏筹細
  - `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE=1` 鏃跺湪瑙嗛瑙ｇ爜杈圭晫瑙﹀彂涓€娆?`FailureRecoveryPolicy::FailSession`銆?- `main` 鏂板涓撻」鍛戒护锛?  - `--forced-failsession-check <media_file> [sample_ms]`
  - 杈撳嚭 `runtime_failure_*`銆乣illegal_*` 鍜?`result=PASS/FAIL`銆?- 淇 codec 閿侀噸鍏ュ紓甯革細
  - `video_codec_mutex_`銆乣audio_codec_mutex_` 鏀逛负 `std::recursive_mutex`锛?  - `decodeVideoFrame/decodeAudioFrame` 鐨?`lock_guard` 鍚屾鏀逛负 `std::recursive_mutex` 鐗堟湰銆?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓锛?  - `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`
  - 缁撴灉锛氶€氳繃锛宍0 warnings / 0 errors`
- 寮哄埗 FailSession 鎺㈤拡锛?  - `build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`
  - 缁撴灉锛歚runtime_failure_stop_requests=1`銆乣runtime_failure_fail_sessions=1`銆乣illegal_transition_total=0`銆乣result=PASS`
- serial 鑱氬悎澶嶆祴锛?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`
  - 缁撴灉锛歚result=PASS`

### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
---

## 闂 89: run_all_checks 鎺ュ叆 forced-failsession 涓€閿?gate

**鏃ユ湡**: 2026-03-20

### 闂鎻忚堪
- `tools/run_all_checks.ps1` 鍘熸祦绋嬩粎瑕嗙洊 `probe + format regression`锛屾湭榛樿瑕嗙洊 `FailSession` 鎭㈠閾捐矾銆?- 杩欎細瀵艰嚧鈥滄棩甯镐竴閿洖褰掑彧楠岃瘉甯歌閾捐矾锛岄仐婕忓け璐ユ仮澶嶈矾寰勨€濈殑娈嬩綑椋庨櫓銆?
### 鍘熷洜鍒嗘瀽
- `--forced-failsession-check` 宸插瓨鍦ㄤ笖绋冲畾锛屼絾鏈撼鍏ユ壒澶勭悊鑴氭湰榛樿娴佺▼銆?- 缂哄皯纭?gate 浼氫娇 FailSession 鍥炲綊鎵ц渚濊禆浜哄伐鑷锛岄暱鏈熷鏄撴紡璺戙€?
### 瑙ｅ喅鏂规
- 鎵╁睍 `tools/run_all_checks.ps1` 鍙傛暟锛?  - `ForcedFailSessionFile`锛堥粯璁ょ┖锛屽洖钀藉鐢?`ProbeFile`锛?  - `ForcedFailSessionSampleMs`锛堥粯璁?`2200`锛?- 灏嗘墽琛屾祦绋嬪崌绾т负涓夋锛?  1. `[1/3]` `--probe-file --json`
  2. `[2/3]` `--forced-failsession-check`
  3. `[3/3]` `run_format_regression.ps1`
- gate 瑙勫垯锛?  - probe 闈為浂锛氱洿鎺ラ€€鍑猴紱
  - forced-failsession 闈為浂锛氱洿鎺ラ€€鍑哄苟璺宠繃 format regression銆?
### 鏈湴楠屾敹缁撴灉
- 鎵ц鍛戒护锛?  - `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Debug/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200`
- 缁撴灉锛?  - `probe exit code = 0`
  - `forced-failsession exit code = 0`
  - `regression exit code = 0`
  - 鑴氭湰鎬婚€€鍑虹爜锛歚0`

### 淇敼鏂囦欢
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 问题 90: OpenGL 原生 D3D11 互操作停止期异常退出与低吞吐修复

**日期**: 2026-03-24

### 问题描述
- `OpenGL` 原生硬解路径已经启动，但 `--performance-log-check` 在 stop/close 阶段异常退出，导致没有最终 `PASS/FAIL` 输出。
- 原生路径吞吐明显异常，`juren-30s.mp4` 在 2 秒采样窗口内只能跑出约 3 帧，无法达到与 D3D11 主链接近的稳定度。

### 原因分析
- OpenGL 原生互操作链路里，FFmpeg 的 `D3D11VA` 解码与 OpenGL 渲染线程共享同一个 renderer-owned D3D11 device/context。
- 该 D3D11 immediate context 未开启 `ID3D11Multithread::SetMultithreadProtected(TRUE)`，导致跨线程访问设备时出现不稳定和低吞吐。
- `PlayerCore` 关闭 session 时先释放 decoder/hw context、后关闭 renderer，native `AVFrame` 缓存和渲染线程的释放顺序不安全。

### 解决方案
- 在 `src/render/opengl_video_renderer.cpp` 中为 renderer-owned D3D11 context 启用 `ID3D11Multithread` 多线程保护。
- 在 `src/core/player_core.cpp` 中调整 session release 顺序：先清理缓存 native frame 并关闭 renderer，再释放 decoder/hw context。
- 保留当前成熟播放器风格的互操作实现：`D3D11VA decode surface -> D3D11 shader convert -> WGL_NV_DX_interop -> OpenGL draw`。

### 本地验证
- Release 构建通过：`cmake --build build --config Release`
- 命令：`$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`
- 结果：`performance-log-check.renderer_backend=OpenGL`
- 结果：`performance-log-check.decoder_backend=D3D11VA`
- 结果：`performance-log-check.video_native_output_frames=62`
- 结果：`performance-log-check.video_copy_back_frames=0`
- 结果：`performance-log-check.render_frames=47`
- 结果：`performance-log-check.result=PASS`
- 结果：进程退出码 `0`

### 修改文件
- src/render/opengl_video_renderer.cpp
- src/core/player_core.cpp
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 问题 91: OpenGL/D3D11 字幕链补齐 ASS 样式变换与样式诊断

**日期**: 2026-03-24

### 问题描述
- OpenGL 与 D3D11 播放链已经能稳定播放，但 `ASS/SSA` 样式语义仍明显弱于成熟播放器。
- 之前 `ASS parser` 只覆盖少量 tag，很多常见样式虽然在字幕文件里存在，但进入播放器后并没有真正作用到 GPU 字幕渲染。
- 缺少机器可读的样式检查入口，导致 OpenGL 字幕问题排查仍偏依赖肉眼观察。

### 原因分析
- `SubtitleStyle` 数据模型不够完整，缺少：`wrap_style`、`spacing`、`scale_x/scale_y`、`rotation`、`x/y border`、`x/y shadow`。
- `src/subtitle/ass_parser.cpp` 未解析 `WrapStyle`、`ScaleX/ScaleY`、`Spacing`、`Angle` 以及 `\q/\fsp/\fscx/\fscy/\fr/\xbord/\ybord/\xshad/\yshad`。
- OpenGL / D3D11 的 D2D/DirectWrite 字幕路径也没有消费这些字段。

### 解决方案
- 扩展 `SubtitleStyle` 和比较逻辑，增加 ASS 样式变换字段。
- 增强 `ASS parser`，把脚本级、style 级和 override 级常见 ASS 样式映射到统一字幕模型。
- 同步更新 `src/render/opengl_video_renderer.cpp` 与 `src/render/d3d11_video_renderer.cpp`，让 GPU 字幕链真正消费这些字段。
- 新增 `--subtitle-style-check`，输出 item/run/style 全量 `key=value` 诊断信息。
- 新增样本 `samples/subtitles/opengl_ass_style_validation.ass`。

### 本地验证
- `./build/Release/modern-video-player.exe --subtitle-style-check ./samples/subtitles/opengl_ass_style_validation.ass`
- `./build/Release/modern-video-player.exe --subtitle-sync-check ./samples/subtitles/opengl_ass_style_validation.ass`
- `$env:MVP_RENDERER_BACKEND='opengl'; ./build/Release/modern-video-player.exe --delay-adjust-check ./samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 ./samples/subtitles/opengl_ass_style_validation.ass`
- `./build/Release/modern-video-player.exe --opengl-diagnostics`
- 结果：`subtitle-style-check.result=PASS`、`subtitle-sync-check.result=PASS`、`delay-adjust-check.result=PASS`、`opengl-diagnostics.result=PASS`

### 修改文件
- include/subtitle/subtitle_parser.h
- src/subtitle/subtitle_parser.cpp
- src/subtitle/ass_parser.cpp
- src/subtitle/srt_parser.cpp
- src/render/d3d11_video_renderer.cpp
- src/render/opengl_video_renderer.cpp
- src/main.cpp
- samples/subtitles/opengl_ass_style_validation.ass
- docs/analysis/PLAYERCORE_DAY28_OPENGL_ASS_STYLE_TRANSFORM_AND_PARSER_DIAGNOSTICS.md
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## Issue 92: OpenGL/D3D11 subtitle run-level karaoke, clip and subtitle clock convergence

**Date**: 2026-03-24

### Problem Description
- The second ASS capability batch was only partially wired in: parser/model changes existed, but renderer-side run-level coloring, karaoke timing, clip rendering and paused redraw behavior were not fully closed.
- OpenGL still rendered subtitles mostly as item-level text, which left a visible gap against mature players for `secondary color`, `clip`, `iclip` and karaoke sweep behavior.

### Root Cause Analysis
- Subtitle timing data was not fully propagated from `PlayerCore` into renderer-side subtitle texture invalidation / redraw decisions.
- The OpenGL subtitle D2D path had not yet been ported to the same run-level brush/effect logic already drafted for D3D11.
- `iclip` had parser coverage but no rendering coverage.

### Solution
- Added `setSubtitleClock()` plumbing from `PlayerCore` into `IVideoRenderer`, `D3D11VideoRenderer` and `OpenGLVideoRenderer`.
- Completed ASS parser support for `SecondaryColour`, `2c/2a/3c/3a/4c/4a`, `clip`, `iclip`, `k/kf/ko` and uppercase `K`.
- Completed run-level D2D subtitle rendering for D3D11/OpenGL with per-run fill/outline/shadow brushes and karaoke sweep highlight overlays.
- Added rectangular `clip` and basic rectangular `iclip` rendering by decomposing the inverse clip area into visible outer regions.
- Extended `--subtitle-style-check` output and added `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`.

### Modified Files
- `include/render/video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/subtitle/subtitle_parser.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/subtitle/srt_parser.cpp`
- `src/subtitle/subtitle_parser.cpp`
- `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`
- `docs/analysis/PLAYERCORE_DAY29_OPENGL_KARAOKE_CLIP_AND_SUBTITLE_CLOCK.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 93: ASS move/fad/fade animation converged into D3D11/OpenGL subtitle rendering

**Date**: 2026-03-24

### Problem Description
- The GPU subtitle path already supported style transforms, karaoke and clip, but still lacked a usable subset of ASS line animation semantics.
- `\move`, `\fad` and `\fade` were still a visible gap against mature players because subtitles remained spatially static and fully opaque once displayed.

### Root Cause Analysis
- The subtitle model only held style/run data and had no dedicated line-level animation container.
- Renderer-side animated subtitle detection only covered karaoke timing, so even if move/fade were parsed they would not trigger correct redraw behavior.
- D3D11/OpenGL brush generation had no item-level opacity modulation stage.

### Solution
- Added `SubtitleStyleAnimation` / `SubtitleFadeMode` and stored line-level animation on `SubtitleItem.animation`.
- Added ASS parser support for `\move`, `\fad` and `\fade`.
- Added subtitle-clock-based move interpolation and fade opacity evaluation in both D3D11 and OpenGL subtitle renderers.
- Extended animated subtitle detection to include move/fade and extended `--subtitle-style-check` output accordingly.
- Added `samples/subtitles/opengl_ass_animation_validation.ass`.

### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_animation_validation.ass`
- `docs/analysis/PLAYERCORE_DAY30_ASS_MOVE_FADE_GPU_SUBTITLE_ANIMATION.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`


## Issue 94: ASS transform, vector drawing/clip, and font fallback groundwork converged into GPU subtitle rendering

**Date**: 2026-03-25

### Problem Description
- The subtitle pipeline had already covered style expansion, karaoke, clip, and move/fade, but the next practical ASS batch was still missing: `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector drawing, vector clip, and font fallback groundwork.
- During OpenGL runtime validation of this batch, the new vector clip path also exposed an internal Direct2D state-stack regression.

### Root Cause Analysis
- The subtitle model/parser could not yet carry transform-origin, projected-rotation, shear, vector drawing, and vector clip data through to the GPU renderers.
- The renderer-side subtitle path lacked geometry generation and masking logic for ASS drawing and vector clip semantics.
- The new clip-layer integration introduced mismatched `PushLayer()/PopLayer()` usage, which produced `D2DERR_PUSH_POP_UNBALANCED (0x88990016)` during `EndDraw()`.

### Solution
- Extended the subtitle model and ASS parser with transform, drawing, vector clip, and source-path fields.
- Added affine transform, vector drawing, and vector clip rendering to both D3D11 and OpenGL subtitle renderers.
- Added subtitle-sidecar/private font registration and fallback family-chain groundwork through `subtitle_font_registry`.
- Fixed the D2D layer-stack regression by removing the stray pre-emptive `PopLayer()` and restoring the missing `PopLayer()` in the subtitle box path across both renderers.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `build\Release\modern-video-player.exe --subtitle-style-check samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `build\Release\modern-video-player.exe --subtitle-sync-check samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `MVP_RENDERER_BACKEND=opengl build\Release\modern-video-player.exe --delay-adjust-check samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- `MVP_RENDERER_BACKEND=d3d11 build\Release\modern-video-player.exe --delay-adjust-check samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 samples\subtitles\opengl_ass_transform_vector_font_validation.ass`
- Results: all PASS

### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_transform_vector_font_validation.ass`
- `docs/analysis/PLAYERCORE_DAY31_ASS_TRANSFORM_VECTOR_FONT_FALLBACK.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 99: Idle window startup and drag-drop playback
**Date**: 2026-03-25

### Problem Description
- Launching the exe without media arguments exited immediately instead of opening a usable player window.
- There was no drag-and-drop media open path for SDL, D3D11, or OpenGL playback windows.
- Dropping a new file during playback could not switch media because the app layer never received an open-file request.

### Root Cause
- The startup path required at least one CLI media input.
- Renderer event pumps exposed quit/seek/navigation requests but not file-drop requests.
- `PlayerCore` stopped playback for next/previous/quit, but had no buffered handoff for a validated media replacement flow.

### Solution
- Added idle-window mode for empty-start sessions.
- Added `consumeOpenFileRequest()` across the renderer interface and implementations.
- Buffered file-drop requests in `PlayerCore` and exposed them through `VideoPlayer`.
- Updated `main.cpp` to validate dropped paths before replacing playback and to return to idle mode for sessions launched without CLI media.

### Local Validation
- `cmake --build build --config Release --target modern-video-player`: PASS
- Manual GUI smoke test still pending for this turn.

### Modified Files
- `src/main.cpp`
- `src/display.cpp`
- `src/render/sdl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/core/player_core.cpp`
- `src/video_player.cpp`
- `include/render/video_renderer.h`
- `include/display.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/player_core.h`
- `include/video_player.h`
- `docs/analysis/PLAYERCORE_DAY32_IDLE_WINDOW_AND_DRAG_DROP_PLAYBACK.md`

## Issue 100: OpenGL playback stutter caused by async present pacing
**Date**: 2026-03-25

### Problem Description
- OpenGL playback could visibly stutter on the user's AMD machine even when decoder, queue, and native-output counters still looked healthy.
- The existing diagnostics mostly reflected submit-side progress, not the moment a frame was actually shown by the OpenGL render thread.

### Root Cause Analysis
- OpenGL `present()` returned immediately after queueing work to the renderer thread, so `PlayerCore` advanced the video clock before on-screen presentation actually completed.
- A single pending-frame slot allowed silent replacement when the renderer thread lagged behind the scheduler.
- OpenGL forced `swap interval=0`, which made display pacing less stable than the D3D11 path.
- The native interop path also executed a per-frame `ID3D11DeviceContext::Flush()`.

### Solution
- Made OpenGL `present()` wait for the submitted frame to be displayed.
- Added submission/presentation IDs to synchronize scheduler pacing with actual OpenGL presentation.
- Changed OpenGL to prefer `swap interval=1`, with fallback to `0` only when needed.
- Removed the per-frame native interop `Flush()` call.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500`
- Results: build PASS, diagnostics PASS, OpenGL native path PASS, OpenGL copy-back path PASS

### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY33_OPENGL_PRESENT_PACING_AND_AMD_STUTTER.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
## Issue 101: OpenGL runtime diagnostics export and P010/P016 copy-back upload

**Date**: 2026-03-25

### Problem Description
- The new OpenGL runtime counters had been wired into the renderer, but still needed end-to-end verification through `--performance-log-check`.
- OpenGL software upload only accepted `yuv420p/nv12`, so 10-bit copy-back frames could still fall through an extra `swscale -> yuv420p` downgrade when native interop was disabled.

### Root Cause Analysis
- The machine-readable reporting path spans renderer diagnostics, `PlayerCore` diagnostics snapshot, and `main.cpp`, so the new keys had to be verified on the final CLI output instead of assuming the plumbing was complete.
- The OpenGL software upload path had no 16-bit semi-planar texture allocation/upload support.
- Software color coefficients only modeled 8-bit normalized sampling.

### Solution
- Verified and retained the `renderer_opengl_native_interop_*` / `renderer_opengl_present_wait_timeouts` exports in `--performance-log-check`.
- Added `AV_PIX_FMT_P010LE` and `AV_PIX_FMT_P016LE` direct upload support to the OpenGL renderer.
- Added 16-bit GL texture allocation/upload and 16-bit normalized color coefficient handling for the semi-planar software path.
- Extended the OpenGL color diagnostics to print the software input format during validation.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200`
- Results: all PASS; 10-bit forced copy-back path reported `video_copy_back_frames=72` and `video_swscale_frames=0`

### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY34_OPENGL_RUNTIME_DIAGNOSTICS_AND_P010_UPLOAD.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 102: OpenGL present-mode override and gate script

**Date**: 2026-03-25

### Problem Description
- OpenGL present pacing had been fixed, but现场排障 still needed a supported way to switch between paced and immediate present behavior.
- The OpenGL regression commands for diagnostics/native/copy-back/10-bit/subtitle were still scattered and manual.

### Root Cause Analysis
- Swap interval selection was still effectively hard-coded in renderer startup.
- Diagnostics snapshots did not expose the requested and active OpenGL present mode.
- There was no single OpenGL-focused gate script comparable to the project's other validation helpers.

### Solution
- Added `MVP_OPENGL_PRESENT_MODE=auto|paced|immediate`.
- Added `[diag:opengl-present] requested=... active=...` startup logging.
- Exported `renderer_opengl_present_mode_requested` and `renderer_opengl_present_mode_active` in `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to run OpenGL diagnostics, native playback, copy-back playback, immediate present mode, 10-bit copy-back, and subtitle delay regression in one command.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500`
- `MVP_RENDERER_BACKEND=opengl MVP_OPENGL_PRESENT_MODE=immediate .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: all PASS; gate script reported `OpenGL gate result: PASS`

### Modified Files
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY35_OPENGL_PRESENT_OVERRIDE_AND_GATE_SCRIPT.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 103: OpenGL HDR probe, quirk-table expansion, subtitle gate completion, and final gap matrix

**Date**: 2026-03-25

### Problem Description
- The OpenGL next-stage task table still had unfinished items around HDR output capability probing, quirk-table growth, subtitle sample gating, and a final mature-player gap summary.

### Root Cause Analysis
- Diagnostics previously stopped at adapter/decoder capability and did not reach the display-output layer.
- The quirk mechanism existed, but the rule table still covered only a very small set of known conditions.
- Subtitle sample validation was still spread across manual commands instead of a single OpenGL-focused gate.
- The remaining OpenGL gap against mature players was known informally, but not yet closed into one final backend-level matrix.

### Solution
- Added `opengl-diagnostics.hdr_output.*` fields to probe DXGI output color space and luminance state.
- Expanded the OpenGL quirk table with `device_id` / `subsystem_id` match capacity and additional software / virtual GPU context rules.
- Expanded `tools/run_opengl_checks.ps1` into a 10-step OpenGL gate including the ASS subtitle sample suite.
- Added a final OpenGL mature-player gap matrix and aligned the HDR design document with the implemented probe surface.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `opengl-diagnostics.result=PASS`, `opengl-diagnostics.hdr_output.probe_succeeded=true`, `OpenGL gate result: PASS`

### Modified Files
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/analysis/PLAYERCORE_DAY36_OPENGL_HDR_PROBE_QUIRK_TABLE_AND_GAP_MATRIX.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 104: OpenGL hotkey stall and missing control OSD

**Date**: 2026-03-25

### Problem Description
- The OpenGL playback path could stutter or appear frozen after hotkeys, especially on seek/volume-related input.
- The OpenGL window did not visibly expose the progress bar and volume bar unless a paused redraw happened, so the UI looked like it had no controls.

### Root Cause Analysis
- The OpenGL renderer consumed every `SDL_KEYDOWN`, including auto-repeat events, which could accumulate repeated control requests on one physical key press.
- Hotkey-triggered OSD visibility only called `requestRedrawIfPaused()`, so normal playback often did not repaint the OSD immediately.
- Unlike the D3D11 path, the OpenGL path had no mouse-driven progress/volume interaction loop wired into SDL events.

### Solution
- Suppressed repeated `SDL_KEYDOWN` handling in the OpenGL path to avoid hotkey request storms.
- Changed OpenGL hotkey OSD wakeup from paused-only redraw to unconditional `requestRedraw()`.
- Added startup OSD visibility, mouse-move OSD wakeup, and left-button progress/volume drag handling with seek preview and live volume updates.
- Refactored OpenGL control layout calculation so rendering and hit-testing use the same geometry.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `OpenGL gate result: PASS`
- Manual GUI smoke is still recommended for actual hotkey feel and control dragging.

### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 105: ASS transform transition parser/runtime support

**Date**: 2026-03-25

### Problem Description
- The OpenGL Top 10 table was already closed, but a visible libass parity gap still remained: `\t(...)` style transitions were not parsed or executed.
- `--subtitle-style-check` could not expose machine-readable transition fields, so transition regressions were hard to verify.
- OpenGL and D3D11 subtitle paths still rendered these subtitles as static styles instead of time-varying transforms/colors/scales.

### Root Cause Analysis
- The ASS override parser still assumed simple argument extraction and did not handle nested parentheses or top-level comma splitting required by `\t(...)`.
- Subtitle runtime style resolution only consumed the base item/run style plus existing move/fade state; it had no transition interpolation stage.
- The OpenGL gate had no dedicated transition sample, so this semantic gap could survive without a targeted regression.

### Solution
- Added `SubtitleStyleTransition` plus transition property masks to the subtitle model and parser.
- Added nested-parenthesis matching and top-level comma splitting so `\t(...)` bodies parse correctly, including embedded `\clip(...)`.
- Added runtime style interpolation for supported transition fields across both OpenGL and D3D11 subtitle renderers.
- Extended `--subtitle-style-check` with `transition_count`, per-transition timing/accel/property names, and target-style dumps.
- Added `samples/subtitles/opengl_ass_transform_transition_validation.ass` and wired it into `tools/run_opengl_checks.ps1`.
- Fixed the MSVC `std::clamp` ambiguity in transition byte interpolation during final integration.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `$env:MVP_RENDERER_BACKEND='d3d11'; .\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `subtitle-style-check.result=PASS`, OpenGL `delay-adjust-check.result=PASS`, D3D11 `delay-adjust-check.result=PASS`, `OpenGL gate result: PASS`

### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `src/subtitle/ass_parser.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `samples/subtitles/opengl_ass_transform_transition_validation.ass`
- `tools/run_opengl_checks.ps1`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/analysis/PLAYERCORE_DAY37_OPENGL_ASS_TRANSFORM_TRANSITION.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 106: OpenGL bottom-bar player chrome

**Date**: 2026-03-25

### Problem Description
- The OpenGL path only exposed a lightweight OSD with progress and volume rails, not a complete player-style control bar.
- There was no clickable play/pause button, no time text, and no hover-aware keep-visible/auto-hide behavior.
- The result still felt like a diagnostics overlay instead of a mature player UI.

### Root Cause Analysis
- `ControlLayout` only modeled `progress_track` and `volume_track`.
- `drawOsdOverlay()` only painted flat bars plus a center pause badge.
- Mouse handling only understood seek/volume drag and had no concept of play-button hit-testing or panel hover state.

### Solution
- Expanded the OpenGL control layout to include a bottom bar, play/pause button, time text region, and larger progress/volume hit boxes.
- Added geometry-based OpenGL UI drawing helpers for circles, triangles, and lightweight segmented time text rendering.
- Reworked OpenGL OSD drawing into a full bottom player chrome with top progress rail, play/pause button, current/total time text, and volume rail.
- Added hover state tracking so the bar stays visible while hovered and fades out automatically after idle playback.
- Wired play/pause button clicks into the existing request path and kept seek/volume drag behavior on the expanded layout.

### Validation
- `cmake --build build --config Release --target modern-video-player`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, `OpenGL gate result: PASS`
- Manual GUI smoke is still recommended for hover timing, hit testing, and visual spacing.

### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY38_OPENGL_BOTTOM_BAR_PLAYER_CHROME.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 107: Container attachment font pipeline

**Date**: 2026-03-25

### Problem Description
- Subtitle font registration only covered sidecar font folders near external subtitle files.
- Container font attachments were ignored, so ASS/SSA subtitles that depended on MKV-attached fonts could still fall back to the wrong system font.
- There was no dedicated CLI regression for attachment extraction, registration, and cleanup.

### Root Cause Analysis
- `subtitle_font_registry` only scanned file-system directories and never consumed `AVMEDIA_TYPE_ATTACHMENT` streams from FFmpeg.
- `PlayerCore::open()` did not create any media-scoped subtitle font session after opening the demuxer.
- Because there was no attachment-specific diagnostics command, this gap could survive without a direct machine-readable test.

### Solution
- Extended `SubtitleFontRegistrationSummary` with attachment-specific extraction and registration fields.
- Added media-scoped attachment APIs in `subtitle_font_registry` to extract font payloads from container attachments into a temp cache and register them as private fonts.
- Wired `PlayerCore::open()` to register attachment fonts immediately after `demuxer_->open(...)`, and wired session release to unregister fonts and delete the cache.
- Added `--attachment-font-check <media_file>` for machine-readable validation.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8`
- `ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -c copy -attach C:\Windows\Fonts\arial.ttf -metadata:s:t mimetype=application/x-truetype-font -metadata:s:t:0 filename=attachment-font-check.ttf .\build\tmp\attachment-font-check.mkv`
- `.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv`
- Results: build PASS, `attachment-font-check.result=PASS`, extracted cache directory cleanup PASS

### Modified Files
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY39_ATTACHMENT_FONT_PIPELINE.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 108: Embedded subtitle-track playback

**Date**: 2026-03-25

### Problem Description
- After the attachment-font pipeline landed, a major remaining player gap was that muxed subtitle streams inside media containers still did not play automatically.
- Files with embedded ASS/SSA or text subtitles opened without subtitle output unless the user manually loaded a sidecar file.
- There was no dedicated machine-readable regression for embedded subtitle discovery, selection, and playback.

### Root Cause Analysis
- `PlayerCore` only owned one subtitle timeline fed from external subtitle parsing, so embedded subtitles had no first-class storage or fallback policy.
- `AssParser` and `SrtParser` did not expose a simple in-memory text entry point, which made reuse from demuxed container packets awkward.
- The project had no loader that scanned FFmpeg subtitle streams, selected a supported text track, and converted it into the existing `SubtitleItem` model.

### Solution
- Added `AssParser::parseText(...)` and `SrtParser::parseText(...)` so reconstructed container subtitle text can reuse the existing subtitle pipeline.
- Added `subtitle::loadBestEmbeddedSubtitleTrack(...)` with support for ASS/SSA plus plain-text subtitle codecs such as `subrip`, `text`, `mov_text`, and `webvtt`.
- Split `PlayerCore` subtitle ownership into external and embedded stores, with active selection rule `external > embedded`.
- Added `--embedded-subtitle-check <media_file>` for machine-readable validation.
- Expanded `tools/run_opengl_checks.ps1` to auto-generate embedded ASS and embedded `mov_text` samples and verify both CLI load and OpenGL playback.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8`
- `ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass -map 0:v -map 0:a? -map 1:0 -c:v copy -c:a copy -c:s ass .\build\tmp\embedded-ass-validation.mkv`
- `.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
- Results: build PASS, embedded ASS check PASS, embedded text check PASS, `OpenGL gate result: PASS`

### Modified Files
- `include/subtitle/ass_parser.h`
- `src/subtitle/ass_parser.cpp`
- `include/subtitle/srt_parser.h`
- `src/subtitle/srt_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY40_EMBEDDED_SUBTITLE_TRACK_PLAYBACK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
