# 开发日志

## 问题 95: RC 版本元数据、Release 页正文与安装包版本标识补齐

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户追问“Release 页正文在哪里”，并要求把程序内部版本、Windows 可执行文件版本和安装包版本都补成 `1.0.0-rc1`。
- 这轮目标不是继续改播放链，而是把 RC 版本标识从文档层补齐到 CLI、EXE 属性、HTTP UA 和发布包产物层。

### 日志输出
```text
Version CLI:
build\Release\modern-video-player.exe --version
Modern Video Player 1.0.0-rc1

Windows file version:
FileVersion=1.0.0.0
ProductVersion=1.0.0-rc1
ProductName=Modern Video Player

Package:
cmake --build build --config Release --target PACKAGE
CPack: - package: D:/VSProject/sssssssssssssss/modern-video-player/build/modern-video-player-1.0.0-rc1-windows-x64.zip generated.

Package entries:
modern-video-player-1.0.0-rc1-windows-x64/modern-video-player.exe
modern-video-player-1.0.0-rc1-windows-x64/plugins/sample_logger_plugin.dll
modern-video-player-1.0.0-rc1-windows-x64/RELEASE_NOTES.md
modern-video-player-1.0.0-rc1-windows-x64/SDL2.dll
modern-video-player-1.0.0-rc1-windows-x64/avcodec-62.dll
...
config/player_settings.ini absent

Diagnostics smoke:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.result=PASS
```

### 分析记录
1. `V1_0_0_RC1_RELEASE_READINESS.md` 是内部发布结论，不等于可直接贴到 GitHub Release 页的正文，因此需要单独补一份 `Release Notes` 文档。
2. 版本号如果只存在于 `project(... VERSION 1.0.0)`，程序 CLI、Windows 资源、压缩包名和 HTTP `user_agent` 会继续分裂，RC 现场排障和版本识别成本很高。
3. Windows 文件版本和产品版本应该区分：`FileVersion` 维持四段数值 `1.0.0.0`，`ProductVersion`/对外版本名使用 `1.0.0-rc1`。
4. 发布包必须避免打入本地脏的 `config/player_settings.ini`，否则产物会混入开发机本地配置；本轮打包已显式验证该文件未进入 ZIP。

### 处理结果
- 新增 `docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`，作为 Release 页正文来源。
- `CMake` 引入统一版本源，并生成 `mvp_version.h` 与 `version_info.rc`。
- `main` 新增 `--version`；`http_stream_downloader` 的 `user_agent` 改为 `modern-video-player/1.0.0-rc1`。
- 新增 `CPack ZIP` 打包规则，产物名固定为 `modern-video-player-1.0.0-rc1-windows-x64.zip`，并将 `RELEASE_NOTES.md` 打入包内。
- `docs/README.md`、`docs/reports/README.md` 与 `V1_0_0_RC1_RELEASE_READINESS.md` 已补入 Release Notes 入口。

### 本地验收结果
- `build\Release\modern-video-player.exe --version`：通过。
- Windows `FileVersionInfo`：`FileVersion=1.0.0.0`、`ProductVersion=1.0.0-rc1`、`ProductName=Modern Video Player`。
- `cmake --build build --config Release --target PACKAGE`：通过，生成 `build/modern-video-player-1.0.0-rc1-windows-x64.zip`。
- ZIP 已确认包含 `modern-video-player.exe`、依赖 DLL、`plugins/sample_logger_plugin.dll` 与 `RELEASE_NOTES.md`，且不包含 `config/player_settings.ini`。
- `build\Release\modern-video-player.exe --d3d11-diagnostics`：通过，`result=PASS`。

### 修改文件
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



## 问题 94: 1.0.0-rc1 发布准备：发布清单、已知问题与发布说明收口

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户明确确认按 `1.0.0-rc1` 方向推进，希望把 RC 发布清单、已知问题和发布说明收口成一套可直接使用的文档。
- 这轮目标不是继续扩功能，而是判断“当前版本是否已经足够发 RC”，并把证据与限制讲清楚。

### 日志输出
```text
Release gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200
[1/3] Probe exit code: 0
[2/3] Forced FailSession exit code: 0
[3/3] Regression exit code: 0
Format regression report: docs/reports/FORMAT_REGRESSION_20260323_224615.md
Summary: Total=17 PASS=17 PARTIAL=0 FAIL=0 SKIP=0

Release diagnostics:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.hevc_any=true
d3d11-diagnostics.decoder_profiles.vp9_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.result=PASS

Release performance smoke:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS

Release long playback smoke:
build\Release\modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000
long-playback-check.still_playing_after_window=true
long-playback-check.late_drops=0
long-playback-check.demux_dropped_packets=0
long-playback-check.result=PASS

Release serial/failsession gate:
build\Release\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.pass_count=3
serial-failsession-regression-check.total_count=3
serial-failsession-regression-check.result=PASS
```

### 分析记录
1. 这轮证据已经足够支持“可发 RC”而不是仅停留在“本地偶尔能播”：Release gate、格式回归、长时播放、seek/serial、forced failsession 和 D3D11 diagnostics 都有最新结果。
2. 当前 RC 结论应当明确区分为“可打 `v1.0.0-rc1` 标签”与“不可直接宣称正式版已完成”。真正阻止直接 GA 的，不是主链不可用，而是兼容矩阵和长尾路径尚未完全吃透。
3. 最需要显式写进已知问题的是 `问题 79`：software video decode 运行态路径仍未完全收口。它当前不阻塞 `D3D11VA` 主链 RC，但它仍是正式版前的剩余风险。
4. 当前机器的 `AV1` profile 诊断结果仍为 `false`，这再次证明 RC 发布说明不能把“可播放 AV1 文件”和“普遍具备 AV1 硬解能力”混为一谈。

### 处理结果
- 新增 `docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`：
  - 输出 RC 发布结论
  - 汇总本轮 Release 验证证据
  - 收口发布说明、已知问题与发布清单
- 更新 `docs/reports/README.md` 与 `docs/README.md`：
  - 为 RC 汇总报告增加入口
- 更新 `docs/records/VERSION.md`：
  - 增加 `当前发布候选: 1.0.0-rc1`
  - 增加 `发布状态: 可发布 RC，不建议直接 GA`
  - 记录本轮 RC 结论与最新验证证据
- `CHANGELOG / DEVELOP_LOG` 已同步记录本次发布准备收口。

### 本地验收结果
- 当前结论：`可打 v1.0.0-rc1 标签`。
- 不建议当前直接打 `v1.0.0` 正式版。
- 最新 Release 证据：
  - `run_all_checks.ps1`：通过
  - `FORMAT_REGRESSION_20260323_224615.md`：`17 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`
  - `--d3d11-diagnostics`：通过
  - `--performance-log-check`：通过
  - `--long-playback-check`：通过
  - `--serial-failsession-regression-check`：通过

### 修改文件
- docs/reports/V1_0_0_RC1_RELEASE_READINESS.md
- docs/reports/FORMAT_REGRESSION_20260323_224615.md
- docs/reports/README.md
- docs/README.md
- docs/records/CHANGELOG.md
- docs/records/DEVELOP_LOG.md
- docs/records/VERSION.md
## 问题 93: D3D11 decoder profile 探测、quirk blacklist 与独立 diagnostics CLI

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户要求把 D3D11 后续成熟化项一次补齐：decoder profile 探测、driver quirk/blacklist 启动期降级策略，以及单独的 `--d3d11-diagnostics` CLI。
- 目标不是继续补一组临时日志，而是把 D3D11 能力快照做成可机器读取、可启动期决策、可脱离播放单独执行的基础设施。

### 日志输出
```text
Release diagnostics:
build\Release\modern-video-player.exe --d3d11-diagnostics
d3d11-diagnostics.supported_platform=true
d3d11-diagnostics.probe_succeeded=true
d3d11-diagnostics.adapter_name=NVIDIA GeForce GTX 1080
d3d11-diagnostics.driver_version=32.0.15.6094
d3d11-diagnostics.format.nv12.shader_sample=true
d3d11-diagnostics.decoder_profiles.h264_any=true
d3d11-diagnostics.decoder_profiles.hevc_any=true
d3d11-diagnostics.decoder_profiles.vp9_any=true
d3d11-diagnostics.decoder_profiles.av1_any=false
d3d11-diagnostics.native_direct.allowed=true
d3d11-diagnostics.native_direct.disable_rule=none
d3d11-diagnostics.result=PASS

Release playback regression:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[diag:d3d11-init] decoder_profiles enumeration_succeeded=true enumerated_profile_count=20 h264_vld_nofgt=true h264_vld_fgt=false hevc_main=true hevc_main10=true vp9_profile0=true vp9_profile2_10bit=false av1_profile0=false av1_profile1=false av1_profile2=false av1_profile2_12bit=false av1_profile2_12bit_420=false
[diag:d3d11-init] native_direct startup_allowed=true startup_disabled=false rule=none
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 分析记录
1. 这轮关键变化是把 D3D11 探测从“日志打印”升级成“结构化快照”，这样 renderer 启动期策略和独立 CLI 可以共用同一份事实源。
2. 当前机器的实测结果已经能直接回答用户最关心的问题：`H.264 / HEVC / VP9` 有硬解 profile，`AV1` 没有；`NV12` 支持 shader sampling；native direct 启动期允许开启。
3. `P016` 的 `CheckFormatSupport` 仍失败，`VP9 profile2 10bit` 与所有 `AV1` profile 也都不支持，这说明“有 D3D11”不等于“所有 10bit / 新编码 profile 都可用”，独立 diagnostics CLI 的价值就是把这些边界显式化。
4. quirk / blacklist 机制当前先保守落一个最明确规则：`Microsoft Basic Render Driver` 直接禁用 native direct；后续若积累到更多驱动坑位，可继续沿同一策略表扩充。

### 处理结果
- `include/render/d3d11_video_renderer.h`：
  - 新增 `D3D11FormatSupportSnapshot`
  - 新增 `D3D11DecoderProfileSupport`
  - 新增 `D3D11DiagnosticsSnapshot`
  - 新增 `D3D11VideoRenderer::probeSystemDiagnostics()`
- `src/render/d3d11_video_renderer.cpp`：
  - 新增 D3D11 probe context、格式支持查询、decoder profile 枚举、startup policy 评估和启动日志输出
  - 新增本地 GUID 常量，避免直接依赖某些 Windows SDK 环境下的 decoder profile 外部符号
  - renderer 初始化阶段接入 startup policy，必要时在播放前就禁用 native direct
- `src/main.cpp`：
  - 新增 `runD3D11Diagnostics()`
  - 新增 `--d3d11-diagnostics`
  - 输出统一 `key=value` 机器可读结果
- `CHANGELOG / VERSION / DEVELOP_LOG` 已同步记录本次 D3D11 成熟化增强。

### 本地验收结果
- `build\Release\modern-video-player.exe --d3d11-diagnostics`：通过，`result=PASS`。
- 当前机器的最新诊断结果：
  - `adapter_name=NVIDIA GeForce GTX 1080`
  - `driver_version=32.0.15.6094`
  - `decoder_profiles.h264_any=true`
  - `decoder_profiles.hevc_any=true`
  - `decoder_profiles.vp9_any=true`
  - `decoder_profiles.av1_any=false`
  - `native_direct.allowed=true`
  - `native_direct.disable_rule=none`
- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`：通过，`renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`video_native_output_frames=62`、`video_copy_back_frames=0`、`result=PASS`。

### 修改文件
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- src/main.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 92: D3D11 启动期能力探测与 adapter/driver 诊断日志补齐

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户要求继续按成熟播放器方向推进，下一步补齐 D3D11 启动期能力探测与 adapter/driver 诊断日志。
- 目标是在初始化阶段直接看到“当前适配器是谁、驱动版本是多少、feature level 到哪、关键格式是否支持”，而不是只在黑屏后被动追日志。

### 日志输出
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[diag:d3d11-init] adapter="NVIDIA GeForce GTX 1080" vendor_id=0x10DE device_id=0x1B80 subsystem_id=0x145119DA revision=161 driver_version=32.0.15.6094 software_adapter=false dedicated_video_mib=8059 dedicated_system_mib=0 shared_system_mib=16313
[diag:d3d11-init] device feature_level=11_1 debug_layer=true multithread_protected=true device3=true video_device=true video_context=true
[diag:d3d11-init] format_support NV12{raw=0xFA82C320 texture2d=true shader_sample=true shader_load=true decoder_output=true} P010{raw=0x3A82C320 texture2d=true shader_sample=true shader_load=true decoder_output=true} P016{check_failed_hr=-2147467259}
[diag:d3d11-init] swap_chain width=1318 height=742 format=B8G8R8A8_UNORM buffer_count=2 sample_count=1 swap_effect=FLIP_DISCARD alpha_mode=IGNORE usage=0x20
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 分析记录
1. 启动期日志已经能直接暴露 adapter/driver/feature level 上下文，后续再遇到机器差异问题时，不需要先等到播放失败再反推设备信息。
2. 格式支持探测显示当前机器的 `NV12`、`P010` 具备 `shader_sample + decoder_output`，而 `P016` 的 `CheckFormatSupport` 直接失败，这类细节此前完全不可见。
3. 这类日志是成熟播放器的重要基础设施，因为它把“设备能力”与“运行期表现”连接起来了。

### 处理结果
- `src/render/d3d11_video_renderer.cpp`：
  - 新增 adapter/driver version/显存信息日志
  - 新增 feature level、debug layer、multithread、`device3/video_device/video_context` 日志
  - 新增 `NV12/P010/P016` 的格式支持摘要日志
  - 新增 swap chain 参数日志
  - `MakeWindowAssociation` 改为显式检查失败并告警
- `CHANGELOG / VERSION / DEVELOP_LOG` 已同步记录本次诊断增强。

### 本地验收结果
- `Release` 构建通过，`0 warnings / 0 errors`。
- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 通过。
- 新增的 `[diag:d3d11-init]` 四组日志稳定打印，且未影响当前已经恢复的零拷贝直采样链路：`video_native_output_frames=62`、`video_copy_back_frames=0`、`result=PASS`。

### 修改文件
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 91: D3D11VA 自定义 hw_frames_ctx：申请可采样解码表面并恢复零拷贝直采样

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户进一步要求把 D3D11 路径做成像成熟播放器那样的真实可用实现，而不是仅依赖问题 90 的运行时 copy-back fallback。
- 需要验证当前项目是不是因为没有自定义 `hw_frames_ctx`，才导致 D3D11VA 解码面默认不可直接采样。

### 日志输出
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS

Debug build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Debug check:
build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
Configured D3D11VA frames context for direct shader sampling: bind_flags=520 initial_pool_size=49 sw_format=nv12 size=1920x1088
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.result=PASS
```

### 分析记录
1. 当前项目此前只设置了 `hw_device_ctx`，没有在 `get_format()` 中接管 `hw_frames_ctx`，这与 FFmpeg 头文件提供的 D3D11 frames 分配接口不匹配。
2. 一旦在 `AVD3D11VAFramesContext::BindFlags` 上追加 `D3D11_BIND_SHADER_RESOURCE`，同一台机器上立刻恢复了 `video_copy_back_frames=0` 的 native direct 路径，说明根因是帧池绑定方式，而不是硬件绝对不支持。
3. 因此成熟播放器和当前旧实现的差距，主要不在“有没有 D3D11”，而在“有没有完整接管 D3D11VA frames allocation policy 和多级 fallback”。

### 处理结果
- `include/core/player_core.h`：
  - 新增 `configureD3D11HardwareFramesContext(...)`
  - 新增 `selectSoftwarePixelFormat(...)`
- `src/core/player_core.cpp`：
  - 在 `get_format()` 阶段通过 `avcodec_get_hw_frames_parameters()` 创建自定义 `hw_frames_ctx`
  - 为 `AVD3D11VAFramesContext::BindFlags` 追加 `D3D11_BIND_SHADER_RESOURCE`
  - 把 `extra_hw_frames` 预算叠加到 `initial_pool_size`
  - 若自定义 frames ctx 失败，则退回 decoder-owned D3D11VA surface；若运行时再失败，则继续由问题 90 的 renderer fallback 兜底
- `CHANGELOG / VERSION / DEVELOP_LOG` 已同步记录本次实现升级。

### 本地验收结果
- `Release` / `Debug` 构建均通过，`0 warnings / 0 errors`。
- `juren-30s.mp4` 的 `--performance-log-check 2000` 在 `Release` / `Debug` 下均输出：
  - `Configured D3D11VA frames context for direct shader sampling: bind_flags=520`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `result=PASS`
- 说明当前机器上已不再依赖 copy-back，真正恢复到 D3D11VA -> D3D11 native direct 的零拷贝直采样链路。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 90: D3D11 原生直采样黑屏：运行时禁用 native direct 并回退 copy-back

**日期**: 2026-03-23
**状态**: 已解决

### 问题描述
- 用户反馈：`Release` 构建程序播放 `juren-30s.mp4` 时只有声音，没有画面，并明确要求排查 `D3D11` 路径黑屏。
- 需要确认问题属于解码失败、渲染失败，还是 D3D11VA 与 D3D11 renderer 的联动兼容性问题。

### 日志输出
```text
Release build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Release /p:Platform=x64 /m
0 warnings / 0 errors

Debug build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Release check:
build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[WARNING] D3D11 native direct rendering disabled for current session: CreateShaderResourceView1 for Y plane failed texture_format=NV12 texture_format_id=103 array_size=44 bind_flags=512 misc_flags=0 texture_index=43 y_plane_hr=-2147024809 uv_plane_hr=0 fallback=copyback-to-software
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=3
performance-log-check.video_copy_back_frames=59
performance-log-check.render_frames=47
performance-log-check.result=PASS

Debug check:
build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
[WARNING] D3D11 native direct rendering disabled for current session: CreateShaderResourceView1 for Y plane failed texture_format=NV12 texture_format_id=103 array_size=44 bind_flags=512 misc_flags=0 texture_index=43 y_plane_hr=-2147024809 uv_plane_hr=0 fallback=copyback-to-software
performance-log-check.renderer_backend=D3D11
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=1
performance-log-check.video_copy_back_frames=62
performance-log-check.render_frames=48
performance-log-check.result=PASS
```

### 分析记录
1. `renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`render_frames > 0` 说明播放主链路未断，问题不在文件探测、解复用或音频链。
2. 告警稳定命中 `CreateShaderResourceView1 for Y plane failed`，表明黑屏根因在 D3D11 原生直采样阶段对硬解表面创建 SRV 失败，而旧逻辑此前没有把这类运行时失败转成明确降级。
3. `video_copy_back_frames > 0` 且 `result=PASS` 说明一旦停止 native direct，现有 copy-back + 软件纹理上传链路能够继续稳定出图。

### 处理结果
- `src/render/d3d11_video_renderer.cpp`：
  - 新增 `disableNativeDirectRendering(...)`，统一回收 native SRV 状态并记录一次性告警。
  - `ensureNativeShaderResourcesLocked(...)` 在 Y/UV plane `CreateShaderResourceView1` 失败时禁用当前会话 native direct，并返回 `false`。
  - `bindNativeFrameLocked(...)` 在解码表面格式不支持直接采样时同样禁用 native direct。
  - `supportsNativeFrameFormat()` 接入 `native_direct_rendering_disabled_` 状态，确保后续帧改走 `PlayerCore` 现有 copy-back 路径。
- `CHANGELOG / VERSION / DEVELOP_LOG` 已同步记录本次修复。

### 本地验收结果
- `Release` / `Debug` 构建均通过，`0 warnings / 0 errors`。
- `juren-30s.mp4` 的 `--performance-log-check 2000` 在 `Release` / `Debug` 下均通过。
- 两个构建均能稳定打印 `fallback=copyback-to-software`，并保持非零 `video_copy_back_frames`、`render_frames`，说明黑屏路径已被运行时降级接管。

### 修改文件
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 80: 文档一致性补齐：CHANGELOG 索引修复与问题 69 analysis 回填

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 今天三次提交完成后，继续对照 records / analysis 时发现两处文档一致性缺口：

  - `CHANGELOG` 的问题总表漏了 `问题 78`

  - `问题 69` 只有 records 三件套，没有对应的 implementation planner analysis 文档

- 这两个问题不会影响运行时行为，但会影响新会话接续、问题索引检索和后续文档审计。



### 日志输出

```text

docs/records/CHANGELOG.md 问题总表当前存在 77 -> 79 的跳号，但正文已存在“问题 78”。

docs/analysis/ 目录当前缺少与“问题 69: PlayerCore 停播收口、包队列所有权与 Clock/Demuxer 设计债修复”对应的单独分析文档。

```



### 分析记录

1. `问题 78` 属于正文已写、索引漏补，问题本质是手工维护总表时遗漏，不是内容缺失。

2. `问题 69` 的 records 内容本身完整，但与当前 implementation planner 工作方式相比，确实缺少可独立引用的 analysis 文档。

3. 因此这轮最小修正应是：

   - 补索引，不改既有问题编号和正文顺序

   - 回填一篇只覆盖 `问题 69` 的分析文档，并把 records 中的引用补齐



### 处理结果

- `CHANGELOG` 问题总表已补 `问题 78`，并新增本次文档一致性修复的 `问题 80`。

- 已新增 `docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`，回填 `问题 69` 的 implementation planner 和关键结论。

- `CHANGELOG / VERSION / DEVELOP_LOG` 已同步补齐这次文档一致性修正。



### 本地验收结果

- 未重新构建；本轮仅修正文档一致性，不涉及代码逻辑变更。

- 当前 records 与 analysis 的对应关系已补齐到：

  - `问题 69 -> PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`

  - `问题 78 -> CHANGELOG 问题总表已可直接索引`



### 修改文件

- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 79: PlayerCore 运行态 software send probe 对照收敛

**日期**: 2026-03-19

**状态**: 待解决

### 问题描述

- 在问题 78 已把 software decode blocker 收敛到“首个 `avcodec_send_packet()` 没有完成返回”后，本轮继续按 implementation planner 做真实运行态对照，目标是继续缩小范围。

- 当前需要回答的问题是：这个 blocker 到底是 FFmpeg software decode 本体、packet 交接、demux read-ahead、音频链，还是 `PlayerCore` 运行态自身造成的。

### 日志输出

```text

.\build\Debug\modern-video-player.exe --software-video-send-probe .\juren-30s.mp4 1500

software-video-send-probe.packet_queue_push_ok=true

software-video-send-probe.packet_queue_pop_ok=true

software-video-send-probe.read_ahead_packets=512

software-video-send-probe.pre_send_receive_ret=-11

software-video-send-probe.send_ret=0

software-video-send-probe.receive_got_frame=true

software-video-send-probe.result=PASS



$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'

.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200

software-video-decode-check.audio_probe_mode=disabled

software-video-decode-check.clock_source=Video

software-video-decode-check.video_packet_dequeue_count=1

software-video-decode-check.video_send_packet_ok=0

software-video-decode-check.decode_video_ok=0

software-video-decode-check.render_frames=0

software-video-decode-check.result=FAIL



$env:MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO='1'

$env:MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD='1'

.\build\Debug\modern-video-player.exe --software-video-decode-check .\juren-30s.mp4 1200

Offthread software video send_packet probe timed out after 500ms

```



### 分析记录

1. 独立 `send-probe` 在 `pre-receive + packet queue round-trip + read-ahead=512` 后仍 `result=PASS`，说明 FFmpeg software decode 本体、packet queue 交接、`receive->send` 顺序和 demux 继续读包都不是根因。

2. `video-only` 对照下仍 `video_packet_dequeue_count=1 / video_send_packet_ok=0 / decode_video_ok=0 / render_frames=0`，说明 blocker 与音频输出、Audio master 无关。

3. `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD=1` 下，`PlayerCore` 运行态里的 software `send_packet` 依旧 500ms 超时，说明问题也不只是“当前 video decode 线程上下文”本身。

4. 当前最硬的结论是：blocker 已经收敛到 `PlayerCore` 运行态里的 software codec context / surrounding state 差异，后续应直接对比 `PlayerCore::initDecoders()` 产出的 codec ctx 字段与独立 probe。

### 处理结果

- 新增 Day15 分析文档，系统记录本轮所有 probe 对照与结论。

- 扩展 `--software-video-send-probe` 与 `--software-video-decode-check` 的结构化输出，避免后续重复走弯路。

- `PlayerCore` 已补一个仅环境变量开启的 offthread send 诊断路径，供下一轮继续钉死 runtime context 差异。

### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过，`0 warnings / 0 errors`。

- `--software-video-send-probe .\juren-30s.mp4 1500`：`result=PASS`。

- `video-only --software-video-decode-check .\juren-30s.mp4 1200`：`result=FAIL`。

- `video-only + offthread send --software-video-decode-check .\juren-30s.mp4 1200`：`result=FAIL`，并打印超时日志。

### 修改文件

- src/main.cpp

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY15_PLAYERRUNTIME_SOFTWARE_SEND_PROBE.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

## 问题 78: software decode 最小 send/dequeue 计数接入与首包送包停滞钉死

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求直接沿 software decode 首包停滞方向补最小计数：`video packet dequeue 次数 / send 成功次数 / send 返回码`。

- 这一步不应继续先动 `SoftwareSDL` 渲染侧，而应该先把 software path 首包阶段卡在哪一层钉死。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

0 warnings / 0 errors



.\build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200

performance-log-check.video_packet_dequeue_count=57

performance-log-check.video_send_packet_ok=57

performance-log-check.video_send_packet_last_ret=0

performance-log-check.result=PASS



[diag:audio-backpressure] ... dec(core v=0,a=155,v_pkt_deq=1,v_send_ok=0,v_send_ret=-2147483648,v_send_eagain=0,...) ...

```



### 分析记录

1. `juren-30s.mp4` 的 `--performance-log-check` 已经打印出 `video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`，说明新计数字段已经从 `PlayerCore` 正常透传到 CLI。

2. software decode 样本的 1 秒诊断显示 `v_pkt_deq=1`，说明 video decode 线程并不是卡在“取不到视频包”。

3. 同时 `v_send_ok=0` 且 `v_send_ret=-2147483648` 仍是未返回哨兵值，说明到诊断点为止首个 packet send 还没有形成一次完成返回。

4. 结合此前一直缺失的 `Video decode first send_packet returned` 日志，本轮可以把 blocker 再收紧一步：当前 software path 更像是卡在首个 `avcodec_send_packet(video_codec_ctx_, packet.get())` 调用本身。

5. 这也进一步证明：当前 blocker 还没走到 copy-back / swscale / display copy 阶段。



### 处理结果

- 在 `PlayerCore` 补 `video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`。

- 把新字段接到 `DiagnosticsSnapshot`、低频诊断日志、`--performance-log-check` 与 `--software-video-decode-check`。

- 新增 Day14 分析文档，记录“首个视频包已经 dequeue，但首个 send 仍未成功返回”的最新结论。



### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过，`0 warnings / 0 errors`。

- `--performance-log-check .\juren-30s.mp4 1200`：`video_packet_dequeue_count=57 / video_send_packet_ok=57 / video_send_packet_last_ret=0 / result=PASS`。

- software decode 样本运行期诊断：`v_pkt_deq=1 / v_send_ok=0 / v_send_ret=-2147483648`。



### 修改文件

- include/core/player_core.h

- src/core/player_core.cpp

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY14_VIDEO_SEND_PACKET_MIN_COUNTERS.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 77: Software decode 首包停滞复核与 SDL renderer 注释乱码修复

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求继续下一步，并顺手把代码文件里残留的注释乱码清理掉。

- 当前目标仍是 software video decode blocker：确认在保守线程配置下，soft decode 是否已经能真实产帧。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

0 warnings / 0 errors



.\build\Debug\modern-video-player.exe --software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000

Video decoder threading: backend=Software thread_count=1 thread_type=none

Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808

[diag:audio-backpressure] demux(v=163,a=501,...) pkt_q(v=162,a=256) dec(core v=0,a=245,...)

software-video-decode-check.renderer_backend=SoftwareSDL

software-video-decode-check.decoder_backend=Software

software-video-decode-check.decode_video_ok=0

software-video-decode-check.scheduler_video_decoded_frames=0

software-video-decode-check.render_frames=0

software-video-decode-check.video_frame_queue_peak_size=0

software-video-decode-check.result=FAIL

```



### 分析记录

1. `Video decoder threading: backend=Software thread_count=1 thread_type=none` 已经证明本轮复核命中了 software decode 保守线程配置。

2. 即便如此，`decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0 / video_frame_queue_peak_size=0` 仍然全部为 0，说明 blocker 不会因为把 FFmpeg 软件解码线程收紧到单线程就自动消失。

3. 结合 `demux(v=163)` 与 `pkt_q(v=162)` 可以推断：2 秒窗口里 video decode 线程只真正消费了 1 个视频包。

4. 同时日志只出现 `Video decode first send_packet start`，没有看到对应的 `returned`，因此当前更像是卡在首个视频包提交阶段，或刚提交首包后就失去后续推进。

5. `video_copy_back_frames=0 / video_swscale_frames=0` 继续说明这个 blocker 还没进入 copy-back 或 swscale 阶段。

6. 代码注释乱码方面，本轮确认 `src/render/sdl_video_renderer.cpp` 仍有 9 处真实 mojibake 注释；修复后再次扫描 `src/`、`include/` 的 `///` 与 `//` 注释行，未再命中新乱码。



### 处理结果

- 修复 `src/render/sdl_video_renderer.cpp` 的 9 处函数头注释乱码，仅改注释，不改逻辑。

- 重新执行 Debug 构建，结果为 `0 warnings / 0 errors`。

- 重新执行 `--software-video-decode-check`，当前 blocker 结论进一步收敛为：software decode 在保守线程配置下仍然不产帧，且更像是“首个视频包后停住”。

- 新增 Day13 分析文档记录本轮结论与下一步建议。



### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过，`0 warnings / 0 errors`。

- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`：结构化输出仍为 `result=FAIL`。

- 代码注释扫描：本轮未再发现新的可疑乱码命中。



### 修改文件

- src/render/sdl_video_renderer.cpp

- docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 76: Software video decode 真实产帧专项检查与 blocker 定位

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求继续按 implementation planner 单开一个“software video decode 真实产帧专项检查”，不要再靠只能证明“打开成功”的 `session-check` 间接推断。

- 当前已知 `SoftwareSDL + Software decode` 会出现 0 帧输出，但旧命令既不能直接给出结构化结论，命令自身还可能被 soft decode 的 `stop/close` 卡住。



### 日志输出

```text

& '.\build\Debug\modern-video-player.exe' --software-video-decode-check '.\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv' 2000

software-video-decode-check.open_ok=true

software-video-decode-check.entered_playback_loop=true

software-video-decode-check.renderer_backend=SoftwareSDL

software-video-decode-check.decoder_backend=Software

software-video-decode-check.audio_output_initialized=true

software-video-decode-check.clock_source=Audio

software-video-decode-check.advanced_seconds=1.89867

software-video-decode-check.demux_video_packets=163

software-video-decode-check.demux_ignored_packets=0

software-video-decode-check.demux_queue_drop_packets=0

software-video-decode-check.decode_video_ok=0

software-video-decode-check.scheduler_video_decoded_frames=0

software-video-decode-check.render_frames=0

software-video-decode-check.video_frame_queue_peak_size=0

software-video-decode-check.video_copy_back_frames=0

software-video-decode-check.video_swscale_frames=0

software-video-decode-check.real_frame_output_ok=false

software-video-decode-check.result=FAIL

```



### 分析记录

1. 新命令已经把“只是进入 playback loop”和“真实产帧”彻底拆开：本次样本里 `open_ok=true`、`entered_playback_loop=true`、`advanced_seconds=1.89867`，说明播放会话和音频主时钟都在推进。

2. 同时 `renderer_backend=SoftwareSDL`、`decoder_backend=Software` 已经证明命令命中了目标链路，而不是又悄悄退回 `D3D11VA`。

3. 但 `decode_video_ok=0`、`scheduler_video_decoded_frames=0`、`render_frames=0`、`video_frame_queue_peak_size=0` 全为 0，说明 blocker 发生在“软件视频解码产出帧”之前，连 frame queue 都没有被填过。

4. `demux_video_packets=163`、`demux_ignored_packets=0`、`demux_queue_drop_packets=0` 说明问题也不在 demux 没喂包或队列丢包。

5. `video_copy_back_frames=0`、`video_swscale_frames=0` 进一步证明这个 blocker 与 `copy-back / swscale / display copy` 无关，焦点已经收敛到当前 FFmpeg software video decode 接入链本身。

6. 旧版直接在命令尾部 `stop/close` 会被 blocker 拖挂；因此这次专项检查被改成 probe 式硬退出，这本身也是当前 blocker 的一个旁证。



### 处理结果

- `src/main.cpp` 新增 `--software-video-decode-check <media_file> [sample_ms]`。

- 命令内部强制 `SoftwareSDL + Software decode + dummy audio`，避免环境差异把结论污染掉。

- 命令通过条件被收紧为：

  - `decoder_backend=Software`

  - `decode_video_ok > 0`

  - `scheduler_video_decoded_frames > 0`

  - `render_frames > 0`

  - `video_frame_queue_peak_size > 0`

  - `video_copy_back_frames == 0`

- 命令改为 probe 式硬退出，确保即使 soft decode 路径卡死，专项检查也能稳定打印结构化 FAIL 结果。



### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过，`0 warnings / 0 errors`。

- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`：返回非零，并输出 `result=FAIL`。

- 当前 blocker 已经被单独钉死：软件路径能打开、能进播放循环、音频时钟能推进，但软件视频解码链没有形成任何有效视频帧产出。



### 修改文件

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY12_SOFTWARE_VIDEO_DECODE_REAL_FRAME_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 75: 撤回 SoftwareSDL automatic software-first 并补软解阻塞诊断

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求继续沿着“如何进一步减少或规避 `av_hwframe_transfer_data()` copy-back”推进，于是本轮先尝试把 `SoftwareSDL` fallback 改成 renderer-aware `software-first`。

- 预期收益是：如果 `SoftwareSDL` 能直接稳定消费软件解码帧，就能进一步压低当前 fallback 链只剩下的 copy-back 热点。

- 但实际验证中，`SoftwareSDL + Software decode` 会进入“音频继续推进、视频 0 帧输出”的回归状态，因此需要先收敛策略，再决定后续方向。



### 日志输出

```text

$env:SDL_AUDIODRIVER='dummy'

.\build\Debug\modern-video-player.exe --windows-backend-session-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv soft

Video decode first send_packet start: backend=Software packet_size=221427 pts=0 dts=-9223372036854775808

Video decode first send_packet returned: backend=Software ret=0 message=unknown

windows-backend-session-check.soft.decoder_backend=Software

windows-backend-session-check.result=PASS



.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=33.5958

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS

```



### 分析记录

1. “system-memory renderer 优先避免 copy-back”这个方向本身没有问题，也符合 `ffplay / mpv / MPC-HC` 一类成熟播放器的常见设计思路。

2. 但本轮临时 `software-first` 实验显示，`SoftwareSDL + Software decode` 会出现 `decode_video_ok=0 / render_frames=0 / video_frame_queue_peak_size=0`，说明当前工程的软件视频解码接入链本身不成立。

3. 进一步强制 `D3D11 + Software decode` 后，仍可复现软件解码首包进入 `send_packet` 但后续不形成有效视频输出，说明 blocker 不在 `SoftwareSDL` renderer，而在软件视频解码接入。

4. 因此当前最合理的收敛方式不是继续把 `software-first` 留在主路径，而是撤回自动切换，保住已经通过验证的 `D3D11VA copy-back` fallback。

5. 同时保留最小必要诊断，后续再单独修软件视频解码接入，而不是把 fallback 主流程继续拖进回归。



### 解决方案

- 撤回自动 renderer-aware `software-first` decoder 排序，恢复默认 `D3D11VA -> Software` 顺序。

- 保留软件视频解码低频诊断能力：

  - FFmpeg 错误码字符串

  - 首次 `send_packet` 起止探针

  - stall 上下文日志

- 继续保留上一轮已经完成的 `SoftwareSDL` fallback 有限重构：

  - `NV12 / YUV420P` 直传

  - `AVFrame` 引用复用

  - `swscale=0 / display_copy=0`



### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过。

- 默认主链验证：

  `./build/Debug/modern-video-player.exe --performance-log-check ./samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500`

  结果：`renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`video_copy_back_ratio_percent=0`、`video_swscale_ratio_percent=0`、`display_copy_ratio_percent=0`、`result=PASS`。

- `SoftwareSDL` fallback 验证：

  `$env:MVP_RENDERER_BACKEND='software'; .\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`

  结果：`renderer_backend=SoftwareSDL`、`decoder_backend=D3D11VA`、`video_copy_back_ratio_percent=33.5958`、`video_swscale_ratio_percent=0`、`display_copy_ratio_percent=0`、`result=PASS`。

- 强制 `D3D11 + Software decode` 的 `session-check` 仍只证明“能打开并进入播放循环”，不能证明“稳定产帧”；当前软件视频解码 blocker 依然存在，但已不再影响默认 fallback 主路径。



### 修改文件

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY11_SOFTWARE_DECODE_BLOCKER_AND_FALLBACK_DIRECTION.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 74: Audio-master lateness 收紧与 SoftwareSDL 减拷贝有限重构

**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户确认已手动提交上一轮结果，要求主链继续优化 `audio-master lateness / catch-up`，同时在软件回退链做有限重构，优先减少 `swscale` 与显示层深拷贝。

- 当前默认 `D3D11` 主链已确认是 zero-copy，因此这轮不做大重构，只针对 `Audio` master 时序和 `SoftwareSDL` fallback 做小范围优化。



### 日志输出

```text

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=46.4067

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.display_copy_frames=0

performance-log-check.result=PASS



$env:SDL_AUDIODRIVER='dummy'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.audio_output_initialized=true

performance-log-check.clock_source=Audio

performance-log-check.scheduler_late_drops=2

performance-log-check.scheduler_wait_events=274

performance-log-check.result=PASS

```



### 分析记录

- 这轮 `SoftwareSDL` 的关键收益不是减少 `copy-back`，而是把 copy-back 后面的 `swscale + display memcpy` 两段热点直接拿掉了。

- `Display` 现在在正 stride 的 `YUV420P/NV12` 上优先保留 `AVFrame` 引用，因此 `display_copy_frames=0` 已经说明显示层深拷贝不再是 fallback 热点。

- `NV12` 可以直接走 `SDL_UpdateNVTexture()` 后，`video_swscale_ratio_percent` 也已经降到 `0`；回退链剩余主要瓶颈只剩 `av_hwframe_transfer_data()`。

- `Audio` master 新策略下的 dummy audio 验证显示 `wait_events=274`，说明分段等待已经回到合理范围；前一次实现里出现的高频伪忙等已被最小 sleep 量子修正。

- 默认 `D3D11 + D3D11VA` 主链仍然保持 zero-copy，这轮修改没有把主链重新拖回软件路径。



### 处理结果

- `PlayerCore` 在无视频滤镜时允许 copy-back 后的软件 `NV12/YUV420P` 直接交给 `SoftwareSDL`。

- `Display` 现已支持 `NV12` SDL texture 和 `AVFrame` 引用复用，负 stride/不适配布局才回退深拷贝。

- `Scheduler::pumpRenderOnce()` 的 `Audio` master 已改成分段等待 + 动态 late-drop 阈值 + 最小 sleep 量子。

- 当前结论已经收敛：

  - 默认主链继续保持 zero-copy

  - `SoftwareSDL` 回退链下一阶段若继续优化，应优先关注 `copy-back`，而不是继续围绕 `swscale`/`display memcpy`



### 修改文件

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



---

## 问题 73: SoftwareSDL 拷贝链路量化、Scheduler 重启预算与 renderer override

**日期**: 2026-03-19

**状态**: 已解决

### 问题描述

- 用户继续追问 `Display::copyFrameData()` 和 `Scheduler` 固定重启次数是否会导致高码率/4K 输出帧不稳定。

- 当前主链已经确认是 `D3D11 + D3D11VA` zero-copy，但软件回退链没有真实统计，renderer override 也没有真正接入选择链。

- 这让第 8、10 点只能靠推断，无法在本机给出软件路径实测占比。

### 日志输出

```text

.\build\Debug\modern-video-player.exe --renderer-fallback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv

renderer-fallback-check.renderer_backend=SoftwareSDL

renderer-fallback-check.decoder_backend=D3D11VA

renderer-fallback-check.fallback_to_sdl=true

renderer-fallback-check.result=PASS



$env:MVP_RENDERER_BACKEND='software'

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200

performance-log-check.renderer_backend=SoftwareSDL

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=30.1018

performance-log-check.video_swscale_ratio_percent=18.6623

performance-log-check.display_copy_ratio_percent=21.8407

performance-log-check.display_copy_avg_ms=5.69759

performance-log-check.scheduler_video_restart_limit_hits=0

performance-log-check.result=PASS



.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_copy_back_ratio_percent=0

performance-log-check.video_swscale_ratio_percent=0

performance-log-check.display_copy_ratio_percent=0

performance-log-check.result=PASS

```



### 分析记录

- 默认 `D3D11` 主链下，`Display::copyFrameData()` 根本不在热路径，当前机器上的主链瓶颈依旧不是软件拷贝。

- 一旦切到 `SoftwareSDL`，链路会变成 `copy-back + swscale + display memcpy` 三段叠加，其中 `Display::copyFrameData()` 本机实测已占采样窗口约 `21.84%`。

- `Scheduler` 新策略下 `restart_limit_hits=0`，说明它不是这批样本的主因，但窗口预算比固定次数更安全。

- 旧的 fallback 检查之所以测不准，不是因为结论错了，而是因为 renderer 选择链之前根本没有消费 override。



### 处理结果

- `Display` / `SdlVideoRenderer` / `PlayerCore` / CLI 现在已经能完整输出软件显示链的拷贝统计。

- `Scheduler` 现已改成窗口预算制重启，并新增 limit hit 诊断。

- `RendererFactory` 现已支持 `MVP_RENDERER_BACKEND` 和 `MVP_D3D11_DRIVER_HINT=software`，`--renderer-fallback-check` 当前已恢复 PASS。

- 当前可以明确区分两条结论：

  - 默认 `D3D11` 主链：继续保留 zero-copy，不做大重构

  - `SoftwareSDL` 回退链：`Display::copyFrameData()` 已是明确热点，但还不是唯一热点



### 修改文件

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



---

## 问题 72: 高码率/4K 队列容量、自适应节流与 copy-back 诊断增强

**日期**: 2026-03-19

**状态**: 已解决

### 问题描述

- 用户要求继续围绕“高码率视频输出帧不稳定”做优化，不直接做大重构，而是先判断 `FrameQueue` 容量、背压、copy-back 和 scheduler 时序这些更接近真实瓶颈的点。

- 当前项目已经具备 D3D11 native zero-copy，但高码率/4K 相关诊断还缺 queue 峰值、push timeout、背压等待时长、copy-back/swscale 时间这些关键指标。

- 同时，无音频输出时走 `Video` master，render 等待逻辑太粗；落后时一次只丢一帧，也会放大追帧抖动。

### 日志输出

```text

.\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500

performance-log-check.renderer_backend=D3D11

performance-log-check.decoder_backend=D3D11VA

performance-log-check.video_native_output_frames=101

performance-log-check.video_copy_back_frames=0

performance-log-check.video_swscale_frames=0

performance-log-check.video_frame_queue_capacity=24

performance-log-check.video_frame_queue_peak_size=23

performance-log-check.video_frame_queue_push_timeouts=0

performance-log-check.scheduler_video_backpressure_wait_ms=1252

performance-log-check.result=PASS



.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

high-bitrate-check.advance_ratio=0.866667

high-bitrate-check.demux_queue_drop_packets=0

high-bitrate-check.result=PASS



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.advance_ratio=0.85

4k-playback-check.late_drops=0

4k-playback-check.demux_queue_drop_packets=0

4k-playback-check.result=PASS



.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000

long-playback-check.advance_ratio=0.938438

long-playback-check.demux_queue_drop_packets=0

long-playback-check.result=PASS

```



### 分析记录

- 当前 4K 主链采样里 `video_copy_back_frames=0`、`video_swscale_frames=0`，说明现在真正跑的是 native zero-copy，不是 copy-back 热点。

- 单纯把 4K `FrameQueue` 放大，会触发 FFmpeg `Static surface pool size exceeded`；因此视频队列大小必须和 `D3D11VA extra_hw_frames` 联动。

- `scheduler_video_backpressure_wait_ms` 很高并不等于“播放失败”，它更多说明 decoder 跑得比 render 快；真正要看的是 `push_timeout_count` 和 `demux_queue_drop_packets`，这轮两者都保持为 0。

- `Video` master 之前会让 24fps 样本明显超速；`pumpRenderOnce()` 一次只丢一帧也会让 4K 追帧过慢。两者都比“线程重启次数限制”更接近当前真实症状。



### 处理结果

- `FrameQueue` 现已具备 `peak_size / push_timeout_count` 统计，`PlayerCore` 会导出 frame queue capacity/peak/timeout。

- `PlayerCore::open()` 会根据媒体属性设置 frame queue 容量，并在 `D3D11VA` 打开前显式配置 `extra_hw_frames`。

- `Scheduler` 现已支持背压迟滞与 `video/audio_backpressure_wait_ms` 统计。

- `Scheduler::pumpRenderOnce()` 已改成：

  - `Video` master 下用 wall-clock pacing

  - 单次 pump 内连续丢弃过期帧直到拿到可显示帧

- 当前 4K / 高码率 / 长时回归都已恢复通过；这轮剩余工作重点不再是“有没有 zero-copy”，而是后续是否继续补 audio-master 下更细的 lateness 策略。



### 修改文件

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



---





## 问题 71: 4K backend session 子进程退出路径修复

**日期**: 2026-03-19

**状态**: 已解决

### 问题描述

- 在 video-only 降级和 demux 门禁纠偏之后，`4k-playback-check` 仍然失败，失败点集中在 `fallback_ok=false`。

- 继续拆开验证后发现：backend session 子进程本身已经打印 PASS，但 `hard` 会超时不退出，`soft` 会异常退出。

- 这说明残留问题已经不在播放主链，而在测试 probe 子进程的退出策略。

### 日志输出

```text

.\build\Debug\modern-video-player.exe --windows-backend-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv

windows-backend-check.hard.mode_ok=true

windows-backend-check.hard.exit_code=0

windows-backend-check.soft.mode_ok=true

windows-backend-check.soft.exit_code=0

windows-backend-check.result=PASS



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.fallback_ok=true

4k-playback-check.result=PASS

```



### 分析记录

- `runWindowsBackendSessionCheck()` 的职责只是向父进程报告“这个 backend 能否启动并进入播放窗口”，并不需要沿用正常播放器会话的完整生命周期回收语义。

- 旧实现的问题是：probe 结果已经打印出来了，但子进程本身没有用稳定的方式结束，导致父进程看到的是 timeout 或异常退出码，而不是 probe 结果。

- 因为 `4k-playback-check` 把 `mode_ok` 和 `exit_code==0` 一起纳入 `fallback_ok`，所以这类退出策略错误会直接表现成 4K 回归失败。



### 处理结果

- `src/main.cpp` 中的 `--windows-backend-session-check` 已改成专用退出路径：打印结果后 flush，并在 Windows 下用 `TerminateProcess(GetCurrentProcess(), code)` 立即结束 probe 子进程。

- `hard/soft` backend session 当前都能稳定返回 `exit_code=0`。

- `--windows-backend-check` 与 `--4k-playback-check` 当前已恢复 PASS，说明这轮残留失败已经收口。



### 修改文件

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_BACKEND_SESSION_EXIT_FIX.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 问题 70: 音频设备失败时的视频-only降级与回归门禁纠偏

**日期**: 2026-03-19

**状态**: 已解决

### 问题描述

- 用户确认先不做大重构，优先参考 ffmpeg/mpv/MPC-HC 的思路，把“音频设备失败时的视频-only降级”和“高码率回归误判”这一层收口。

- 当前机器上 `WASAPI` 音频设备不可用，视频文件实际上已经能靠现有主链继续播，但 `open()` 语义、clock source 和回归门禁都没有把这个状态显式表达出来。

- 这导致高码率素材检查里 `demux_dropped_packets` 主要由 ignored audio packets 组成，却仍然被当成背压失败。

### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

已成功生成。0 个警告，0 个错误



.\build\Debug\modern-video-player.exe --1080p60-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

1080p60-check.result=PASS

1080p60-check.audio_output_initialized=false

1080p60-check.video_only_fallback=true

1080p60-check.clock_source=Video

1080p60-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000

high-bitrate-check.result=PASS

high-bitrate-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --long-playback-check .\juren-30s.mp4 6000

long-playback-check.result=PASS

long-playback-check.demux_queue_drop_packets=0



.\build\Debug\modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000

4k-playback-check.result=FAIL

4k-playback-check.fallback_ok=false

```



### 分析记录

- `initDecoders()` 本身已经按 `audio_player_->isInitialized()` 控制音频解码是否启用，所以这轮不需要先做大规模重构；真正缺的是 `open()` 层面的显式策略。

- 高码率检查失败的旧根因不是视频队列真的被压爆，而是 disabled audio path 产生了大量 `demux_ignored_packets`，又被旧门禁误当成统一的 demux drop。

- video-only 场景下继续使用 `System` clock 会让播放推进和真实渲染 PTS 脱节，因此需要把无音频输出时的主时钟切到 `Video`。

- `4k-playback-check` 当前剩余失败点已经缩小到 `runBackendSessionSubprocess()` 相关的 `fallback_ok` 子进程路径，不再是这轮修复范围里的误判问题。



### 处理结果

- `PlayerCore::open()` 现已区分：

  - 视频文件音频设备失败：warning + video-only 继续播放

  - 音频-only 文件音频设备失败：直接失败

- `DiagnosticsSnapshot` 现已新增 `audio_output_initialized / video_only_fallback / clock_source`，播放类检查会直接打印当前降级模式。

- `1080p60-check`、`high-bitrate-check`、`long-playback-check` 已改为只看 `demux_queue_drop_packets`，并保留 `demux_dropped_packets` 作为总量观测值。

- 当前验证结果表明：这轮修复已经把“音频设备缺失导致的假失败”从高码率回归里剥离出去；后续如果继续优化，应优先排查 4K fallback 子进程路径和更长期的 queue serial 设计。



### 修改文件

- `include/core/player_core.h`

- `src/core/player_core.cpp`

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_AUDIO_FALLBACK_AND_REGRESSION_GATES.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 问题 69: PlayerCore 停播收口、包队列所有权与 Clock/Demuxer 设计债修复



**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求继续把上一轮审查发现的设计债直接落地修掉，重点包括：EOF/停播线程状态不完整、`PacketQueue` 原始指针所有权、`Clock` system-clock 时间跳变，以及 `Demuxer::open()` 的自锁风险。

- 这类问题不一定会立即体现为编译失败，但会在 replay / seek / EOF / close 等长期运行路径上积累成稳定性和可维护性问题。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

已成功生成。

0 个警告

0 个错误

```



### 分析记录

- EOF 自动停播发生在 scheduler render 线程内，不能直接同步 `stop()`；否则 render 线程会 join 自己。旧实现因此退化成“只改状态”的半停机路径。

- `AVPacket*` 放进通用队列后，`clear()` / 队列析构不会帮项目释放 FFmpeg 包；seek、stop、close 都会命中这条泄漏路径。

- `Clock` 的 system-clock pause/speed 没有先固化旧时间基准，纯视频播放时会出现暂停时间倒退或倍速切换跳时。

- `Demuxer::open()` 持锁调用 `close()` 是接口级自锁，虽然主流程不常复用同一实例，但设计上必须收口。



### 处理结果

- `PlayerCore` 现在具备 deferred stop + worker reap 机制，EOF、Next/Previous、Quit 等路径不再留下脏线程状态，后续 replay / restart 可安全重启。

- `PacketQueue` 已改为 `unique_ptr<AVPacket, AvPacketDeleter>`，`ThreadSafeQueue` 同步补上延迟 move push，压缩包生命周期回归 RAII。

- `Scheduler` 已新增异步停机入口并在重启前回收旧 worker，`Clock` 已修复 system-clock pause/speed 基准更新。

- `Demuxer::open()` 不再在锁内重入 `close()`；整工程 Windows `Debug` 全量重建仍保持 `0 个警告 / 0 个错误`。



### 修改文件

- include/core/player_core.h

- include/core/scheduler.h

- include/thread_safe_queue.h

- src/core/player_core.cpp

- src/core/scheduler.cpp

- src/core/clock.cpp

- src/demuxer.cpp

- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---



## 问题 68: MSVC warning debt 分层清理（C4819 / C4996 / C4706）



**日期**: 2026-03-18

**状态**: 已解决



### 问题描述

- 用户要求继续清理全局 warning debt，并优先处理 `C4819` 编码告警，以及第三方 / 本地 `C4996` 的分层治理。

- 当前 Windows CI 已不再有编译 blocker，但 warning 总量过高，会掩盖真实回归信号。

- 需要在不回退既有 D3D11/字幕改动的前提下，把本地可修 warning 和第三方 warning 隔离策略一起补齐。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

已成功生成。

0 个警告

0 个错误

```



### 分析记录

- `C4819` 的核心不是单个源文件坏掉，而是 MSVC 仍按本地代码页读取 UTF-8 源文件；这类问题应优先通过编译选项统一治理。

- 第三方 warning 不应靠修改 FFmpeg / Quill 源码解决，更稳妥的做法是把第三方头文件放到外部 warning 层，由 MSVC 的 `/external:*` 机制统一隔离。

- 本地 `C4996` 和 `C4706` 仍然应该逐个修掉，因为这些 warning 直接反映项目自身代码质量。

- 本轮落地后，全量 `Rebuild` 已达到 `0 warning / 0 error`，说明分层策略和本地修复都已生效。



### 处理结果

- `CMakeLists.txt` 已为 MSVC 目标启用 `/utf-8 /external:anglebrackets /external:W0`，本地编码告警与第三方头文件 warning 已显著收口。

- `src/logger.cpp` 已改为安全环境变量读取 helper，清理本地 `getenv` 告警。

- `src/subtitle/srt_parser.cpp` 与 `src/subtitle/ass_parser.cpp` 已补上 Windows / 非 Windows 的 `sscanf_s` / `std::sscanf` 分支。

- `src/main.cpp` 的 `signalHandler` 参数和 `src/core/player_core.cpp` 的 demux push 循环已改写，清掉本地剩余的 `C4100 / C4706`。

- 当前 Windows `Debug` 全量重建已经达到 `0 个警告 / 0 个错误`。



### 修改文件

- CMakeLists.txt

- src/logger.cpp

- src/main.cpp

- src/subtitle/srt_parser.cpp

- src/subtitle/ass_parser.cpp

- src/core/player_core.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---

## 问题 67: ASS 标签解析与 UTF-16 字幕范围修正



**日期**: 2026-03-18

**状态**: 已解决



### 问题描述

- D3D11 原生字幕链已经接入 `ASS/SSA`，但 override 标签解析对 `\fnArial`、`\rDefault` 这类紧凑写法不正确，常见样式会被静默忽略。

- `SubtitleTextRun` 的区间长度如果继续沿用 UTF-8 code point，而渲染端直接按 DirectWrite 的 UTF-16 range 使用，就会在 emoji、扩展 CJK 或其他非 BMP 字符上出现样式错位。

- 用户要求在前一批提交后继续把剩余正确性问题清掉，并给出这轮剩余补丁的提交命令。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

已成功生成。

167 个警告

0 个错误

```



### 分析记录

- 这轮复查没有发现新的高危内存泄漏点；问题集中在字幕语义正确性，而不是 COM/AVFrame 生命周期管理。

- `ASS/SSA` override 解析器此前把标签名读取成“连续数字 + 连续字母”，因此 `fn/r` 这类允许直接跟值的标签会把值首段误吞进标签名。

- `SubtitleTextRun.start/length` 目前的真正消费者是 Windows DirectWrite，而它要求的文本范围单位是 UTF-16 code unit，不是 UTF-8 code point。

- 当前全量重建已重新通过；剩余 167 个 warning 主要来自第三方头文件（FFmpeg / Quill）、项目内多处源码的 C4819 编码告警、以及少量既有的 C4996/C4100 提示，本轮未扩散成新的构建阻塞。



### 处理结果

- `src/subtitle/ass_parser.cpp` 已补上常用 ASS 标签前缀匹配，修复 `\fnArial`、`\rDefault`、`\1c&H...&` 等标签的识别路径。

- `src/subtitle/ass_parser.cpp` 与 `src/render/d3d11_video_renderer.cpp` 已统一使用 UTF-16 code unit 计算 run 长度，确保样式范围与 DirectWrite 一致；并顺手清理了 `ass_parser.cpp` 内本地 `sscanf` / 局部变量遮蔽告警。

- 两个源码文件末尾的多余空行已清理，保持本轮 diff 只包含有效改动。

- 这轮剩余补丁现在只包含“ASS 标签解析修正 + UTF-16 范围修正 + 文档同步”，可独立提交。



### 修改文件

- src/subtitle/ass_parser.cpp

- src/render/d3d11_video_renderer.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---

## 问题 66: 全局构建阻塞清理与 ASS/SSA 原生 D3D11 字幕链



**日期**: 2026-03-18

**状态**: 已解决



### 问题描述

- 当前工作树虽然已经具备原生 D3D11 视频呈现能力，但全局 `Debug|x64` 构建仍被多处头文件/源文件的编码误读阻塞，导致无法用整工程结果验证交付状态。

- D3D11 原生字幕链此前只覆盖纯文本叠加，`.ass/.ssa` 的样式、定位、层级和多 cue 同屏能力还没有进入 native renderer。

- 外挂字幕自动探测与显式加载需要覆盖 `.ass`、`.ssa`、`.srt` 三种格式，而不是停留在仅面向 SRT 的状态。



### 日志输出

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

已成功生成。

0 个警告

0 个错误

```



### 分析记录

- 构建阻塞的根因不在 D3D11 主链逻辑，而在多处带中文注释的头文件/源文件被当前 MSVC/代码页组合误读后触发全局语法错误。

- 旧字幕模型只承载纯文本，`IVideoRenderer` 也只接收单字符串，因此 `PlayerCore` 无法把 ASS/SSA 的样式、层级和定位语义送入 D3D11 渲染器。

- 时间线解析原本只面向单条活动字幕，不足以表达 ASS/SSA 常见的多 cue 同屏场景。

- 最合适的修复路径不是把 ASS/SSA 再退回 SDL，而是扩展字幕对象模型后继续走同一条 DXGI swap chain backbuffer 原生叠加链。



### 处理结果

- 清理了全局构建阻塞文件中的编码误读问题，恢复完整解决方案构建基线。

- 新增 `AssParser`，并扩展 `SubtitleStyle / SubtitleTextRun / SubtitleItem` 以承载 ASS/SSA 样式信息。

- `PlayerCore` 现在会收集多条当前激活字幕，通过 `IVideoRenderer::setSubtitleItems()` 把结构化字幕对象送入渲染器。

- `D3D11VideoRenderer` 已在原生 D3D11/D2D 路径中支持 ASS/SSA 常用样式字幕绘制：填充、描边、阴影、背景框、对齐、定位和 run 级字体样式。

- `VideoPlayer` 与 `main.cpp` 已支持 `.ass/.ssa/.srt` 加载与自动探测；完整 `MSBuild` 验证通过。

---

## 问题 57: D3D11 原生 GPU 渲染链补齐



**日期**: 2026-03-18

**状态**: 已解决



### 问题描述

- 当前仓库已经具备原生 D3D11 视频面呈现与 D3D11VA 设备共享，但字幕在 D3D11 渲染器内只有状态写入，没有真正绘制到 backbuffer。

- 文档仍保留“D3D11 只是 SDL 包装器”的旧结论，已经与当前代码事实不一致。

- 用户目标是拿到一条完整的、独立的、原生 D3D11 GPU 渲染链，因此需要把字幕叠加与说明文档一起补齐。



### 日志输出

```

Native D3D11 renderer initialized: window=...

D3D11VA decoder bound to renderer-owned D3D11 device

```



### 分析记录

- 视频主面已经可以直接消费 `AV_PIX_FMT_D3D11`，并覆盖 `NV12 / P010 / P016` 硬件面采样；真正缺口是字幕叠加仍未落到原生链内。

- 最合适的补齐方式是在 renderer-owned DXGI swap chain backbuffer 上引入 D2D1 / DirectWrite 文本绘制，而不是退回 `Display` 或 SDL texture 叠加。

- `PlayerCore` 的 copy-back 分支需要保留，作为“视频滤镜开启/格式不支持”时的明确回退边界。

- 定向验证表明 `src/render/d3d11_video_renderer.cpp` 语法编译通过；整工程构建失败仍来自历史损坏头文件，而不是本次 D3D11 修改本身。



### 处理结果

- `D3D11VideoRenderer` 新增 D2D1 / DirectWrite 字幕绘制资源和暂停态即时重绘。

- `CMakeLists.txt` 补齐 `d2d1`、`dwrite` 链接依赖。

- 新增 `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`，并给 `PLAYERCORE_DAY4_RENDERER_ANALYSIS.md` 加历史说明。

---



## 问题 12: 企业级多线程架构重构



**日期**: 2026-02-27

**状态**: 已完成



### 问题描述

原有架构存在以下问题：

1. 组件职责不清晰，VideoPlayer 承担过多职责

2. 线程模型复杂，难以维护

3. 内存管理容易出错，导致双重释放等 bug



### 解决方案

重构为企业级多线程架构，引入以下新组件：



1. **Demuxer** - 解封装器，封装 AVFormatContext 的读取操作

2. **DecoderWorker** - 解码工作线程，封装单个流的解码逻辑

3. **ThreadSafeQueue** - 线程安全队列模板

4. **Clock** - 时钟同步器，管理音视频同步



### 架构图

```

VideoPlayer (主控制器)

    ├── Demuxer (解封装器) → PacketQueue

    ├── DecoderWorker (视频解码线程)

    ├── DecoderWorker (音频解码线程)

    ├── Display (渲染器)

    ├── AudioPlayer (音频播放)

    └── Clock (时钟同步)

```



---



## 问题 11: 并发读取 AVFormatContext 导致崩溃



**日期**: 2026-02-27

**状态**: 已解决



### 问题描述

播放视频时出现大量 FFmpeg 解码错误和访问冲突崩溃：

```

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

[aac @ ...] Number of scalefactor bands in group (53) exceeds limit (49).

0xC0000005: 写入位置 0x0000022947799000 时发生访问冲突

```

调用堆栈显示错误在 `av_read_frame(format_ctx_, packet)` 处。



### 日志输出

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Video decode thread started

Audio decode thread started

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

... (大量类似错误)

```



### 分析记录

1. 问题出现在视频解码线程和音频解码线程同时运行时

2. 两个线程各自拥有独立的 VideoDecoder/AudioDecoder 实例

3. 两个实例共享同一个 `AVFormatContext* format_ctx_`

4. 在 `decodeFrame()` 中都调用了 `av_read_frame(format_ctx_, packet)`

5. 并发读取导致 packet 数据错乱，H264 解码器读取到错误的 NAL 单元



### 根本原因

**并发调用 `av_read_frame()` 导致数据竞争**。`AVFormatContext` 不是线程安全的，两个线程同时读取会导致：

- 读取位置错乱

- Packet 数据被覆盖

- 解码器收到损坏的数据



### 解决方案

引入统一的 `PacketReaderThread` 作为唯一的 packet 读取入口：

1. `PacketReaderThread` 是唯一调用 `av_read_frame()` 的地方

2. 读取到的 packet 根据 stream_index 分发到 `PacketQueue`

3. `VideoDecodeThread` 和 `AudioDecodeThread` 从各自的 `PacketQueue` 获取 packet

4. 解码器新增 `decodePacket()` 方法接收外部传入的 packet



---



## 问题 9: VideoFrame/AudioFrame 移动语义缺陷导致崩溃



**日期**: 2026-02-25

**状态**: 已解决



### 问题描述

程序启动播放后立即崩溃，错误信息：

```

modern-video-player.exe - 应用程序错误

0x00007FFF7A80DA4C 指令引用了 0xFFFFFFFFFFFFFFFF 内存。该内存不能为 read

```



### 日志输出

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Playing video...

Video decode thread started

Audio decode thread started

(程序崩溃)

```



### 分析记录

1. 错误地址 0xFFFFFFFFFFFFFFFF = -1，表示空指针/无效指针

2. 崩溃发生在视频解码线程启动后不久

3. 代码审查发现 `VideoFrame` 和 `AudioFrame` 类缺少正确的移动语义实现



### 根本原因

`FrameQueue::pop()` 使用 `std::move` 将帧移动出队列：

```cpp

frame = std::move(queue_.front());

queue_.pop();

```



`VideoFrame` 类没有定义移动构造函数和移动赋值运算符：

- 默认的移动操作只是浅拷贝 `frame_` 指针

- 原对象析构时调用 `av_frame_free(&frame_)` 释放内存

- 目标对象的 `frame_` 变成悬空指针

- 渲染循环访问悬空指针导致崩溃



### 解决方案

为 `VideoFrame` 和 `AudioFrame` 类实现正确的移动语义：

1. 添加移动构造函数，将 `frame_` 指针转移

2. 添加移动赋值运算符，将 `frame_` 指针转移

3. 将原对象的 `frame_` 设为 nullptr，防止析构时释放内存



---



## 问题 13: Core API + Scheduler + Filter 多线程重构落地



**日期**: 2026-03-06

**状态**: 已解决



### 问题描述

- 现有播放器主流程仍以旧架构为中心，缺少规格要求的 `Core API / Scheduler / Filter` 模块化分层。

- 音视频解码线程需要在新核心中独立调度，并通过队列实现无阻塞解耦。



### 分析记录

- 新增 `core` 层：`frame/frame_queue/clock/command/scheduler/player_core`。

- 新增 `filters` 层：`video_filter/audio_filter/filter_registry/filter_pipeline/builtin filters`。

- `VideoPlayer` 增加 `USE_NEW_PLAYER_CORE` 迁移开关路径，保持旧接口不变。



### 修改文件

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



## 问题 14: 播放器架构收敛为 Core 单路径



**日期**: 2026-03-06

**状态**: 已解决



### 问题描述

- 项目同时保留旧播放链路和新核心链路，存在维护分叉与并发设计风险。



### 解决方案

- `VideoPlayer` 仅保留 `PlayerCore` 包装实现，删除旧链路分支。

- CMake 删除旧模块编译入口，统一到 `core/*` + `filters/*`。

- 清理旧头源文件，保留必要基础设施和输出模块。

- 新增架构重构文档：`docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`。



### 修改文件

- CMakeLists.txt

- include/video_player.h

- src/video_player.cpp

- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md

- 删除旧模块文件（decoder/thread/sync/packet/legacy clock/frame_queue）



---



## 问题 15: 小屏窗口过大且拖拽缩放不稳定



**日期**: 2026-03-06

**状态**: 已解决



### 问题描述

- 播放器在小屏设备上按视频原始分辨率（如 1920x1080）直接开窗，窗口初始显示过大。

- 用户拖拽窗口后，部分环境下渲染区域未稳定跟随，表现为“窗口不能正常调整”。



### 日志输出

```

Display initialized: window 1306x734 (source 1920x1080)

```



### 分析记录

- `PlayerCore::open()` 直接把视频分辨率传给 `Display::init()`，没有根据屏幕可用区做首帧窗口缩放。

- 事件处理只监听 `SDL_WINDOWEVENT_RESIZED`，未覆盖常见的 `SDL_WINDOWEVENT_SIZE_CHANGED`。

- 渲染目标矩形直接铺满窗口，缺少按源视频比例计算的目标区域。



### 解决方案

- 在 `Display::init()` 增加基于 `SDL_GetDisplayUsableBounds()` 的初始窗口尺寸计算，保持原始宽高比并限制到屏幕可用区 90%。

- 同时处理 `SDL_WINDOWEVENT_RESIZED` 和 `SDL_WINDOWEVENT_SIZE_CHANGED`，确保拖拽时窗口尺寸状态实时更新。

- 渲染时按视频宽高比计算 `dst_rect`，避免窗口变化时拉伸失真。



### 修改文件

- src/display.cpp



## 问题 16: 窗口最大化/缩放时视频画面卡住，缺少基础控制条



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 窗口最大化或拖动缩放时，视频画面容易卡住，音频仍继续播放。

- 播放器缺少进度条、音量调节和拖动进度的基础能力。



### 日志输出

```text

现象复现：窗口最大化/缩放时视频画面停止刷新，音频继续。

```



### 分析记录

- 运行期 SDL 事件处理与渲染分布在不同线程，窗口变化事件与渲染调用并发时容易出现渲染停滞。

- 播放器 UI 仅支持键盘暂停/退出/全屏，缺少基础交互控件。



### 解决方案

- 将窗口事件处理收敛到渲染路径（`Display::renderFrame`/`PlayerCore::onRenderIdle`）侧，降低缩放与最大化时的渲染阻塞风险。

- `Display` 新增控制层绘制：进度条 + 音量条。

- 新增鼠标交互：拖动进度条发起 seek、拖动音量条实时调节音量。

- `PlayerCore::pumpEvents` 新增对 seek/volume 请求的消费与执行。



### 修改文件

- include/display.h

- src/display.cpp

- src/core/player_core.cpp

- src/main.cpp



## 问题 17: 企业级 MPC-HC 架构剩余模块缺少可落地代码骨架



**日期**: 2026-03-07

**状态**: 已解决（阶段性）



### 问题描述

- `enterprise-quill-logging/tasklist.md` 中模块 02-14 仍有大量未完成项，缺少统一接口与代码落地点。

- 现有播放器主链路未抽象渲染后端，难以扩展 D3D11/OpenGL 等企业级渲染模块。



### 分析记录

- 当前工程已具备 Core + Scheduler + Filter 基础，但缺少跨模块边界定义。

- 需要先补高优先模块骨架并接入构建，再逐步填充完整能力。



### 解决方案

- 新增基础设施：`TaskQueue`、`FramePool`、`DecoderThread`。

- 新增渲染模块：`IVideoRenderer`、SDL 适配器、D3D11/OpenGL 占位实现、`RendererFactory`，并接入 `PlayerCore`。

- 新增音频增强：10 段均衡器、多流混音器，扩展 `AudioPlayer` 缓冲观测接口。

- 新增解码器管理：`IDecoder`、`DecoderCapability`、`DecoderFactory` 自动选择逻辑。

- 新增字幕/播放列表/设置/快捷键/皮肤/插件/格式支持/流媒体等企业级模块骨架。

- 新增滤镜基类与音视频滤镜链，并补充音量平衡滤镜。

- 同步更新 `tasklist.md` 勾选状态（仅勾选已落地代码项）。



### 修改文件

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



---



## 问题 18: 编译基线恢复与格式能力矩阵入口



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- `dash_manifest_parser.cpp` 在 VS2022/MSVC 编译失败，阻塞本地持续开发。

- 缺少可直接运行的能力检查入口，不利于快速验证“主力格式 + 高分高帧 + 多音道”目标。



### 解决方案

- 修复 DASH 正则 raw-string 分隔符，恢复工程可编译。

- 新增命令行能力入口：

  - `--capabilities`：输出运行时容器/编解码器能力与主力格式覆盖矩阵。

  - `--evaluate-target`：评估指定分辨率/帧率/声道/码率目标是否建议硬解与 D3D11 渲染。

- 增强播放链路基础能力：

  - `Demuxer` 使用 `av_find_best_stream` + probe 参数增强；

  - `AudioPlayer` 暴露实际输出参数；

  - `PlayerCore` 复用 `SwrContext`，按设备输出参数进行重采样。



### 修改文件

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



## 问题 19: D3D11VA 硬解最小闭环（失败回退软解）



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 需要在 Windows 下优先使用 D3D11VA 解码高负载视频，同时避免硬解失败导致无法播放。

- 硬件输出帧格式与 SDL 渲染链路不一致（GPU/NV12 vs YUV420P），需要统一输出格式。



### 解决方案

- 在 `PlayerCore::initDecoders` 中加入 D3D11VA 配置与选择逻辑。

- 当硬解 `avcodec_open2` 失败时，自动重建解码器并回退软解继续播放。

- 在视频解码路径补充硬件帧转存与像素格式规整：

  - `av_hwframe_transfer_data`（硬件帧 -> 系统内存帧）

  - `sws_scale`（非 YUV420P -> YUV420P）



### 修改文件

- include/core/player_core.h

- src/core/player_core.cpp



---



## 问题 20: `--probe-file` 与格式回归脚本



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 缺少单文件可脚本化探测入口，不便于快速定位“某个样本为什么不达标”。

- 缺少批量回归脚本，不利于单人开发迭代中持续追踪格式兼容性退化。



### 解决方案

- 新增命令：`modern-video-player.exe --probe-file <media_file>`，输出 `probe.*` 键值结果，便于脚本解析。

- 新增 `tools/format_regression/run_format_regression.ps1`：按 CSV 样本清单批量探测并输出 Markdown 报告。

- 报告默认路径：`docs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`。

- 返回码约定：`0=PASS`，`1=PARTIAL`，`2=FAIL`。



### 验证记录

- `.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4` 返回 `probe.overall=PASS`。

- `.\tools\format_regression\run_format_regression.ps1` 成功生成报告。

- `-OutputFile` 自定义路径验证通过。



### 修改文件

- src/main.cpp

- tools/format_regression/run_format_regression.ps1

- tools/format_regression/format_samples.csv

- docs/workflows/FORMAT_REGRESSION.md

- docs/README.md



---



## 问题 21: GitHub Actions 自动回归接入



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 当前格式回归仅在本地手动执行，PR 缺少自动化门禁。

- Windows CI 中依赖发现方式与本地 `external/` 目录方案不同，存在构建不一致风险。



### 解决方案

- 新增 `.github/workflows/format-regression.yml`：

  - 自动下载 `SDL2/FFmpeg` 预编译依赖并构建 `build/Debug/modern-video-player.exe`；

  - 调用 `tools/download_test_samples.ps1` 生成样本；

  - 调用 `tools/run_all_checks.ps1` 执行单文件探测 + 批量回归；

  - 上传 `docs/reports/FORMAT_REGRESSION_CI.md` 产物。

- 调整 `CMakeLists.txt`（Windows）：

  - 优先支持 `SDL2::`、`FFMPEG::`、`unofficial::ffmpeg::` 导入目标链接；

  - 继续保留 `external/SDL2` 与 `external/ffmpeg` 回退路径。

- 调整 `tools/download_test_samples.ps1`：

  - 新增 PATH 可执行解析逻辑，支持 `-FfmpegPath ffmpeg`。

- 同步更新回归文档与任务清单已完成项。



### 修改文件

- .github/workflows/format-regression.yml

- CMakeLists.txt

- tools/download_test_samples.ps1

- docs/workflows/FORMAT_REGRESSION.md

- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---



## 问题 22: M1 主链路推进（播放列表 + 设置 + 快捷键首版）



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 当前主流程只有单文件播放，`PlaylistManager`/`SettingsManager` 虽有模块但未接入。

- 关键快捷键未覆盖任务清单目标，用户交互效率不足。



### 解决方案

- 在 `main` 主流程接入播放列表：

  - 支持多文件参数构建播放列表；

  - 支持 `.m3u8` 加载；

  - 支持上一首/下一首与 EOF 自动下一首。

- 在 `main` 接入设置持久化：

  - 启动加载 `config/player_settings.ini`；

  - 设置失败回退默认；

  - 退出保存音量、速度、索引。

- 扩展渲染事件请求链路（Display -> Renderer -> PlayerCore -> VideoPlayer -> main）：

  - seek 增量请求、速度调整请求、列表切换请求、退出请求；

  - 实现任务清单首版默认键位集（`Space/Enter/Esc/Q/Left/Right/Ctrl+Left/Ctrl+Right/Up/Down/M/PageUp/PageDown/[ ]/R`）。



### 修改文件

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



## 问题 23: 移除 Core 单元测试目标与测试文件



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 需求要求移除两个 Core 测试及相关文件。

- `CMakeLists.txt` 中仍存在测试目标与开关，直接删除文件会留下构建残留。



### 分析记录

1. 已确认待移除测试文件为 `tests/core_frame_queue_tests.cpp` 与 `tests/core_clock_tests.cpp`。

2. 已确认构建脚本中存在 `BUILD_CORE_TESTS`、`core_frame_queue_tests`、`core_clock_tests` 与 `core_tests` 聚合目标引用。

3. 需要同步清理 CMake 配置与文档记录，保证仓库状态一致。



### 解决方案

- 清理 `CMakeLists.txt` 中的两个测试目标、聚合目标与测试开关。

- 删除两个测试文件。

- 同步更新 `CHANGELOG.md`、`VERSION.md`、`DEVELOP_LOG.md`。



### 修改文件

- CMakeLists.txt

- tests/core_frame_queue_tests.cpp（删除）

- tests/core_clock_tests.cpp（删除）

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 24: 外挂字幕加载入口（SRT）接入主流程



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 任务清单 `1.1.1` 需要提供外挂字幕加载入口。

- 现有代码虽然有 `subtitle::SrtParser`，但主流程没有任何参数入口或播放器加载接口。



### 分析记录

1. 字幕解析模块已具备（`include/subtitle/srt_parser.h` + `src/subtitle/srt_parser.cpp`）。

2. `main` 参数解析仅支持媒体输入、能力查询和 probe，缺失字幕入口。

3. `VideoPlayer` 不持有外挂字幕状态，无法在播放会话中追踪已加载字幕。



### 解决方案

- `VideoPlayer` 新增外挂字幕加载能力（仅 SRT，含容错）：

  - `loadExternalSubtitle()` / `clearExternalSubtitle()`；

  - 记录已加载字幕路径和条目数。

- `main` 新增 `--subtitle <file.srt>` 入口，并保留播放列表参数能力。

- 若未显式传 `--subtitle`，自动尝试加载同名 `.srt` 文件。

- 任务清单 `1.1.1` 标记完成。



### 修改文件

- include/video_player.h

- src/video_player.cpp

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 25: 字幕渲染叠加与播放时序同步接入



**日期**: 2026-03-07

**状态**: 已解决



### 问题描述

- 任务清单 `1.1.2` 要求字幕渲染叠加，但当前渲染接口只有画面与控制层，没有字幕通道。

- 任务清单 `1.1.3` 要求字幕与播放/暂停/seek 同步，但播放核心未按时钟驱动字幕更新。



### 分析记录

1. `VideoPlayer` 已具备外挂字幕加载能力（问题 24），但字幕数据未进入渲染链路。

2. 渲染层抽象缺失字幕接口，SDL/D3D11/OpenGL 后端无法统一接收字幕文本。

3. 播放核心需要在渲染帧与空闲事件都更新字幕，才能覆盖暂停态和 seek 后静止画面。

4. 字幕更新实现中存在锁内调用渲染接口风险，需要调整锁粒度。



### 解决方案

- 渲染抽象扩展：

  - `IVideoRenderer` 增加 `setSubtitleText()`；

  - SDL 后端转发到 `Display`；

  - D3D11/OpenGL 增加兼容桩实现。

- `Display` 新增字幕叠加层：

  - 新增字幕状态与互斥保护；

  - 在每帧渲染中叠加字幕面板与文本；

  - 支持多行与超长文本处理。

  - 当前字模为轻量实现，非 ASCII 字符降级显示。

- `PlayerCore` 新增字幕时间轴逻辑：

  - 引入外挂字幕轨道状态管理；

  - 在 `renderFrame()` 与 `onRenderIdle()` 中调用 `updateSubtitleOverlay()`；

  - 依据当前播放时间选择活跃字幕，支持播放/暂停/seek 同步。

- 修复并发细节：

  - 将渲染调用移出字幕互斥锁，避免锁内回调渲染层。

- 更新任务清单：

  - `1.1.2`、`1.1.3` 标记完成。



### 修改文件

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



## 问题 26: 字幕开关控制与字幕加载异常处理完善



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.1.4` 要求“字幕开关与异常处理”，但当前缺少运行时字幕开关能力。

- 字幕加载流程在文件系统异常场景下缺少专门容错，稳定性不足。



### 分析记录

1. 现有输入事件链路（`Display -> Renderer -> PlayerCore`）可扩展请求型控制，适合加入字幕开关。

2. 字幕时间轴更新已在 `PlayerCore` 内闭环，新增“开关状态”即可复用现有同步逻辑。

3. `VideoPlayer::loadExternalSubtitle()` 使用默认 `filesystem` 检查方式，存在异常传播风险。



### 解决方案

- 新增字幕开关控制：

  - 在 `Display` 中新增字幕开关请求并绑定 `V` 键；

  - 在渲染器接口新增 `consumeToggleSubtitleRequest()`；

  - 在 `PlayerCore` 中新增 `setSubtitleEnabled()/toggleSubtitleEnabled()/isSubtitleEnabled()`；

  - 关闭字幕时主动清空渲染文本，开启字幕后按当前时间重新选取字幕。

- 加强异常处理：

  - 字幕路径检查改用 `std::error_code`；

  - 捕获字幕解析异常并写告警日志；

  - 加载失败保持主流程可播放，不保留脏字幕状态。

- 同步任务状态：

  - `1.1.4` 标记完成。



### 修改文件

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



## 问题 27: 快捷键配置持久化接入（hotkey.*）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.3.2` 需要支持键位配置持久化。

- 当前快捷键逻辑直接写在 `Display` 的 `SDL_KEYDOWN` 分支，无法从配置加载并在重启后保持。



### 分析记录

1. 项目已有 `HotkeyManager`，但默认键位与主流程实际键位不一致，且未接入播放链。

2. `SettingsManager` 已能读写 `player_settings.ini`，可复用为 `hotkey.*` 持久化通道。

3. 需要打通 `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 的热键映射透传链路。



### 解决方案

- 扩展 `HotkeyManager`：

  - 对齐首版默认动作与键位；

  - 新增动作键名和键码序列化/反序列化能力。

- 事件链路接入：

  - `Display` 改为通过 `HotkeyManager` 解析键位动作；

  - 渲染器接口新增 `setHotkeyManager()`；

  - `PlayerCore` 与 `VideoPlayer` 增加热键管理 API 并透传到 SDL 渲染器。

- 持久化接入：

  - `main` 新增 `hotkey.*` 配置加载与回写；

  - 非法配置自动降级默认并记录告警；

  - 更新 `config/player_settings.ini` 默认样例。

- 任务状态同步：

  - `1.3.2` 标记完成。



### 修改文件

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



## 问题 28: 快捷键冲突检测与恢复默认能力



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.3.3` 需要支持“键位冲突检测与恢复默认”。

- 当前已具备 `hotkey.*` 持久化，但缺少冲突治理，重复键位会导致动作语义不清晰。



### 分析记录

1. `HotkeyManager` 已有动作与键位映射，适合扩展冲突扫描接口。

2. 配置加载流程在 `main` 中集中处理，适合统一加入冲突检测与恢复策略。

3. 需要一个可显式触发恢复默认的配置通道，便于用户自救。



### 解决方案

- 在 `HotkeyManager` 增加：

  - `resetToDefaults()`；

  - `findConflicts()` / `hasConflicts()`。

- 在热键加载流程增加冲突治理：

  - 加载配置后先构建候选映射；

  - 检测到冲突则记录详细日志并自动回退默认键位；

  - 非法 token 保留默认并给出告警。

- 增加恢复默认配置开关：

  - `hotkey.restore_defaults=true` 时启动自动恢复默认；

  - 恢复后自动置回 `false`。

- 任务清单同步：

  - `1.3.3` 标记完成。



### 修改文件

- include/input/hotkey_manager.h

- src/input/hotkey_manager.cpp

- src/main.cpp

- config/player_settings.ini

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 29: M1 验收 1.4.1（SRT seek 同步自检命令落地）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.4.1` 需要验证 SRT 字幕在 seek 后仍与播放时间同步。

- 当前缺少可重复的本地验收命令，无法稳定做回归。



### 分析记录

1. `PlayerCore::updateSubtitleOverlay()` 已具备缓存索引 + 二分定位逻辑，但无法脱离播放 UI 独立验证。

2. 需要把字幕时间轴匹配算法提取为可复用函数，并提供命令行自检入口。



### 解决方案

- 新增 `subtitle::resolveActiveSubtitleIndex(...)` 并由 `PlayerCore` 统一复用。

- 新增 `--subtitle-sync-check <subtitle.srt>`：

  - 有序时间轴检查；

  - seek 跳转检查；

  - 输出 mismatch 统计与 PASS/FAIL。

- 新增样例文件 `samples/subtitle/subtitle_seek_sync_sample.srt` 与本地报告 `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`。

- 更新任务清单，勾选 `1.4.1`。



### 修改文件

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



## 问题 30: M1 验收 1.4.2（播放列表连续播放 5 文件自检）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.4.2` 需要验证播放列表连续播放 5 文件。

- 缺少可重复执行的自动化验收入口。



### 分析记录

1. 主流程已支持 EOF 自动切换下一条目，但运行结果未结构化输出。

2. 现有 `--probe-file` 已具备稳定的媒体可打开检测能力，可复用为验收前置检查。



### 解决方案

- 新增 `--playlist-flow-check`：

  - 构建播放列表并要求至少 5 条目；

  - 检查前 5 条的可打开状态；

  - 按 EOF 自动切换逻辑验证顺序覆盖 `0,1,2,3,4`；

  - 输出 `playlist-flow-check.result=PASS/FAIL`。

- 新增本地报告：`docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`。

- 任务清单 `1.4.2` 标记完成。



### 修改文件

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md

- samples/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 31: M1 验收 1.4.3（设置重启恢复自检）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `1.4.3` 需要验证设置在重启后可恢复。

- 当前缺少可重复的命令行验收入口。



### 分析记录

1. `loadAppSettings/saveAppSettings` 已覆盖播放器核心设置和热键持久化。

2. 需要新增独立命令将“写入->重载->比对”过程结构化输出。



### 解决方案

- 新增 `--settings-persistence-check [settings_file]`。

- 使用测试配置执行 round-trip：

  - `volume`

  - `playback_speed`

  - `resume_last_playlist`

  - `last_playlist_index`

  - `toggle_subtitle` 热键

- 输出每个字段的 `*_ok` 结果与总结果。

- 默认使用临时路径，避免污染实际配置。

- 任务清单 `1.4.3` 标记完成。



### 修改文件

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 32: M2 2.1.2（容器矩阵补齐 mov/avi/m2ts）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `2.1.2` 需要覆盖 `mp4/mkv/mov/avi/webm/flv/ts/m2ts` 容器。

- 当前回归样本缺失 `mov/avi/m2ts`。



### 分析记录

1. `FormatSupport` 已声明容器支持，但缺少对应回归样本无法形成验收闭环。

2. 样本生成脚本仅生成 5 类容器，需同步扩展。



### 解决方案

- 扩展 `format_samples.csv` 新增 `mov/avi/m2ts` 三条样本。

- 扩展 `download_test_samples.ps1` 生成三类新样本。

- 更新样本目录规则与文档。

- 运行 `run_format_regression.ps1` 产出最新本地报告。

- 任务清单 `2.1.2` 标记完成。



### 本地验收结果

- `FORMAT_REGRESSION_LOCAL_CHECK.md`：

  - Total=12

  - PASS=12

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



### 修改文件

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



## 问题 33: M2 2.1.3（视频编码矩阵补齐 MPEG-2）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `2.1.3` 需要覆盖 `H.264/H.265/VP9/AV1/MPEG-2` 视频编码。

- 回归样本尚缺 `MPEG-2`，无法完成完整验收。



### 分析记录

1. 现有矩阵包含 h264/hevc/vp9/av1。

2. `FormatSupport` 已包含 `mpeg2video`，缺口在样本和回归链路。



### 解决方案

- 新增 `mpeg2video` 回归样本条目（ts 容器，ac3 音频）。

- 扩展 `download_test_samples.ps1` 自动生成 MPEG-2 样本。

- 运行回归并更新本地报告。

- 任务清单 `2.1.3` 标记完成。



### 本地验收结果

- `FORMAT_REGRESSION_LOCAL_CHECK.md`：

  - Total=13

  - PASS=13

  - PARTIAL=0

  - FAIL=0

  - SKIP=0

- 新增样本结果：

  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts` -> `PASS (mpeg2video)`



### 修改文件

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 34: M2 2.1.4（音频编码矩阵补齐 E-AC3/DTS/Vorbis/PCM）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `2.1.4` 需要覆盖 `AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`。

- 样本矩阵缺少 `E-AC3/DTS/Vorbis/PCM`。



### 分析记录

1. 现有样本已覆盖 aac/mp3/ac3/flac/opus。

2. DTS 编码器 `dca` 在当前 FFmpeg 中属于实验特性，需要 `-strict -2`。

3. 兼容性比对需处理 `dts/dca`、`hevc/h265`、`pcm/pcm_*` 等价关系。



### 解决方案

- 扩展回归样本与生成脚本，新增四类音频样本。

- 在 `download_test_samples.ps1` 为 DTS 命令增加 `-strict -2`。

- 在 `run_format_regression.ps1` 增加等价编码名比较函数。

- 更新本地回归报告并确认全量通过。

- 任务清单 `2.1.4` 标记完成。



### 本地验收结果

- `FORMAT_REGRESSION_LOCAL_CHECK.md`：

  - Total=17

  - PASS=17

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



### 修改文件

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- tools/format_regression/run_format_regression.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 35: M3 3.1.1（DecoderFactory 接入真实初始化流程）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `3.1.1` 要求 `DecoderFactory` 接入真实解码初始化流程。

- 现状是 `PlayerCore` 主要走内嵌分支逻辑，`DecoderFactory` 只在硬解配置局部被动参与，未形成统一候选链路。



### 分析记录

1. `DecoderFactory` 已有能力探测与优先级，但缺少“后端候选序列”接口。

2. `PlayerCore::initDecoders` 需要按候选序列逐个尝试，以实现统一初始化与回退。

3. `3.1.3` 已提供“是否偏好硬解”配置，需要保留并复用。



### 解决方案

- `DecoderFactory` 新增 `selectBackendOrder(codec_name, prefer_hardware)`：

  - 生成按优先级排序的解码后端候选序列；

  - 保证软件解码兜底在候选链路中。

- `PlayerCore::initDecoders` 接入候选序列：

  - 逐个后端尝试初始化与 `avcodec_open2`；

  - 失败自动切换下一个候选；

  - 成功后统一记录最终后端日志。

- `tryConfigureD3D11HardwareDecode` 去除内部后端选择判断，改为纯 D3D11 配置职责，选择策略由 `DecoderFactory` 统一决定。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。



### 修改文件

- include/decoder/decoder_factory.h

- src/decoder/decoder_factory.cpp

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 36: M3 3.1.2（D3D11VA 初始化失败回退软解兜底）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `3.1.2` 要求 D3D11VA 初始化失败时必须可靠回退到软解。

- 现有链路在像素格式协商回退场景下仅打印日志，后端状态未显式切换为软解，存在状态不一致风险。



### 分析记录

1. `PlayerCore::selectVideoPixelFormat` 在未命中 D3D11VA 像素格式时会返回软件像素格式。

2. 该路径此前未同步更新 `video_decoder_backend_`，可能导致后续硬件绑定清理条件判断不准确。



### 解决方案

- 在 `selectVideoPixelFormat` 中补充显式降级：

  - 将 `video_hw_pixel_fmt_` 重置为 `AV_PIX_FMT_NONE`；

  - 将 `video_decoder_backend_` 设置为 `Software`。

- 在 `initDecoders` 的后端尝试链路中新增降级日志：

  - 当 D3D11VA 初始化过程中被协商为软件路径时，记录“降级为软解”提示。

- 同步更新任务清单 `3.1.2` 为完成。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --capabilities` 通过（主力矩阵 `PASS`）。



### 修改文件

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 37: M3 3.2.1（D3D11 渲染最小可用链路）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `3.2.1` 要求 D3D11 渲染最小可用（`init/upload/present`）。

- 现有 `D3D11VideoRenderer` 为桩实现，`init` 固定失败，无法形成可用渲染后端。



### 分析记录

1. 现有 `Display` 已具备稳定的 owner-thread 渲染与帧上传链路，可复用于 D3D11 最小闭环。

2. 需要可观测当前 SDL renderer 后端，以判断是否真正启用了 `direct3d11`。

3. 若实际后端不是 D3D11，应在 `D3D11VideoRenderer::init` 失败并交给上层触发 `3.2.2` 自动回退。



### 解决方案

- `Display` 新增渲染驱动控制与观测接口：

  - `setPreferredRendererDriver()`：允许在创建 renderer 前设置偏好驱动；

  - `currentRendererDriver()` / `isUsingRendererDriver()`：返回当前实际 renderer 后端。

- `D3D11VideoRenderer` 改为最小可用实现：

  - `init`：创建 `Display`，指定 `direct3d11` 偏好，完成窗口/渲染初始化；

  - `renderFrame/present/clear`：接通帧上传与呈现；

  - 事件与控制请求接口统一透传 `Display`。

- `init` 阶段增加后端校验：

  - 若实际 renderer 后端非 `direct3d11/d3d11`，主动返回失败并记录日志，触发上层 SDL 回退。

- 任务清单 `3.2.1` 标记完成。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。



### 修改文件

- include/display.h

- src/display.cpp

- include/render/d3d11_video_renderer.h

- src/render/d3d11_video_renderer.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 38: M3 3.3.2（渲染失败降级不中断播放）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `3.3.2` 要求渲染失败时可自动降级且不中断播放。

- 需要一个可重复执行的本地验收入口来稳定验证 D3D11 失败后的 SDL 回退链路。



### 分析记录

1. `3.2.1/3.2.2` 已具备 D3D11 初始化与 SDL 回退机制，但缺少自动化验收命令。

2. 需要在不依赖人工操作的前提下，强制制造 D3D11 renderer 初始化失败场景。



### 解决方案

- 新增渲染/解码后端可观测接口（renderer backend / decoder backend）。

- 新增命令 `--renderer-fallback-check <media_file>`：

  - 临时注入 `MVP_D3D11_DRIVER_HINT=software`，强制 D3D11 渲染初始化失败；

  - 通过播放器主链路验证是否自动回退到 `SoftwareSDL`；

  - 输出结构化字段与 `PASS/FAIL`。

- 新增本地报告：`docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`。

- 更新任务清单，标记 `3.3.2` 完成。



### 本地验收结果

- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`

  - `open_ok=true`

  - `renderer_backend=SoftwareSDL`

  - `entered_playback_loop=true`

  - `fallback_to_sdl=true`

  - `result=PASS`



### 修改文件

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



## 问题 39: M3 3.3.1（Windows 软解+硬解主力样本可播）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `3.3.1` 要求在 Windows 下验证硬解（D3D11VA）和软解（Software）主力样本均可播。

- 原聚合命令在同进程顺序执行双会话时，出现停止阶段卡住，导致命令超时。



### 日志输出

```text

windows-backend-check.command=... 

... The filename, directory name, or volume label syntax is incorrect.

windows-backend-check.result=FAIL

```



### 分析记录

1. 同进程连续跑 hard/soft 会话时，第二轮存在回收链路不稳定。

2. `std::system` + 重定向在当前路径组合下存在 shell 解析不稳定。

3. 需要将会话隔离并改为更稳定的子进程创建方式。



### 解决方案

- 新增 `--windows-backend-session-check <media_file> <hard|soft>`，每次只验证一个会话并输出结构化字段。

- 将 `--windows-backend-check` 改为父进程拉起两个子进程（hard、soft）并汇总结果。

- Windows 下用 `CreateProcess` + 文件句柄重定向采集子进程输出，避免 shell 语法问题。

- 增加本地报告 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 hard` 通过（PASS）。

- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 soft` 通过（PASS）。

- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4` 通过（PASS）。

- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4` 通过（PASS）。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（PASS）。



### 任务清单同步

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `3.3.1` 标记完成。

  - `3.3.3` 标记完成。



### 修改文件

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 40: M4 4.1（章节导航：上一章/下一章）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `4.1` 要求提供章节导航能力（上一章/下一章）。

- 当前播放主链路没有章节请求动作和跳章入口，无法从键盘直接跳章。



### 分析记录

1. 章节元数据来自 `AVFormatContext::chapters`，需要在 `Demuxer` 开档阶段提取并保存在媒体信息中。

2. 输入事件链路需新增章节动作透传：`Display -> Renderer -> PlayerCore -> VideoPlayer`。

3. 需要新增命令行自检入口，避免章节导航仅依赖手工播放验证。



### 解决方案

- `Demuxer` 新增章节模型：

  - 增加 `ChapterInfo` 结构与 `MediaInfo::chapters`；

  - 解析 `AVChapter` 的起止时间和标题，并按起始时间排序。

- 快捷键与请求链路扩展：

  - `HotkeyManager` 新增 `PreviousChapter` / `NextChapter`；

  - 默认键位绑定 `HOME` / `END`；

  - `Display`、`IVideoRenderer` 及各渲染器实现新增章节请求消费接口。

- `PlayerCore` 增加章节跳转逻辑：

  - `rebuildChapterPoints()` 在 `open()` 后构建章节时间点；

  - 新增 `seekToNextChapter()` / `seekToPreviousChapter()`；

  - `pumpEvents()` 消费章节请求并触发 seek。

- `VideoPlayer` 增加章节 API 包装：

  - `seekToNextChapter()` / `seekToPreviousChapter()` / `chapterCount()`。

- `main` 新增 `--chapter-nav-check <media_file>`：

  - 自动执行播放、下一章、上一章流程；

  - 输出 `chapter-nav-check.*` 字段与 `PASS/FAIL`。

- 任务清单同步：

  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.1` 完成。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。



### 修改文件

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



## 问题 41: M4 4.2（A-B Repeat）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `4.2` 要求支持 A-B Repeat。

- 当前主流程缺少 A/B/C 区间循环控制，无法设置 A 点、B 点并重复播放区间。



### 分析记录

1. 热键与事件请求链路已具备可扩展模式，可复用为 A-B Repeat 请求透传。

2. `PlayerCore` 已有 seek 与播放位置状态，适合新增区间状态并在播放中触发回跳。

3. 需要新增命令行自检入口，确保区间循环可稳定回归。



### 解决方案

- 新增热键动作与默认绑定：

  - `SetABRepeatStart`（`A`）；

  - `SetABRepeatEnd`（`B`）；

  - `ClearABRepeat`（`C`）。

- 扩展链路：

  - `Display` 增加 A-B Repeat 请求标记/消费；

  - `Renderer` 抽象与 SDL/D3D11/OpenGL 实现新增对应透传接口。

- `PlayerCore` 增加 A-B Repeat 控制能力：

  - `setABRepeatStart()`、`setABRepeatEnd()`、`clearABRepeat()`；

  - `isABRepeatEnabled()`、`abRepeatStart()`、`abRepeatEnd()`；

  - `handleABRepeatLoop()` 在播放中检测到达 B 点后自动 seek 回 A 点。

- `VideoPlayer` 增加 A-B Repeat API 包装。

- `main` 新增 `--ab-repeat-check <media_file>` 验收命令与帮助信息。

- 修复回归检查冲突：

  - `--settings-persistence-check` 测试键位由 `b` 改为 `x`，避免与新默认 `B` 热键冲突。

- 任务清单同步：

  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.2` 完成。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 通过（`PASS`）。



### 修改文件

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



## 问题 42: M4 4.3（截图）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `4.3` 需要支持截图。

- 当前截图链路仍处于 WIP：热键、主流程与 `--screenshot-check` 已接入，但暂停态请求缺少“最后显示帧”缓存，导致暂停时无法稳定保存当前画面。



### 分析记录

1. `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 的请求链路已经具备，只差播放器核心对“最近一帧”的保留能力。

2. 现有实现只在渲染线程处理截图请求；一旦进入暂停态，调度器停止继续送帧，截图请求就拿不到新的图像数据。

3. 要让截图成为可用功能，除了补齐缓存外，还需要把自检改成覆盖“暂停态截图”，否则无法证明根因已修复。



### 解决方案

- `PlayerCore` 新增最近一次渲染帧缓存：

  - `updateLastRenderedFrame()` 在每次呈现后缓存当前帧；

  - `captureScreenshotFromCachedFrame()` 支持在暂停态直接截图；

  - `clearLastRenderedFrame()` 在重新打开/关闭媒体时清理缓存。

- `requestScreenshot()` 调整为：

  - 播放中：维持异步请求；

  - 非播放态（重点是暂停态）：直接从缓存帧落盘。

- `--screenshot-check` 调整为“先播放 -> 暂停 -> 请求截图 -> 校验输出文件”，确保本次修复有针对性验证。

- 文档与任务清单同步：

  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 标记 `4.3` 完成；

  - 新增本地报告 `docs/reports/SCREENSHOT_LOCAL_CHECK.md`；

  - 更新 `README.md` / `README_ZH.md` 的快捷键说明。



### 本地验收结果

- `cmake --build build --config Debug --target modern-video-player` 通过。

- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --settings-persistence-check` 通过（`PASS`）。

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 通过（`PASS`）。



### 修改文件

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



## 问题 43: `MPC_HC_GAP_ANALYSIS` 评估结论过期



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 仍停留在 2026-03-07 之前的评估口径。

- 文档把字幕、快捷键、设置、播放列表、解码器管理等能力写成“未接入/骨架”，与当前代码和本地验收报告不一致。



### 分析记录

1. `M1`、`M3` 与 `M4 4.1/4.2/4.3` 已陆续落地，但差距评估文档没有同步刷新。

2. 继续沿用旧结论会误导后续优先级判断，特别是会把已完成项继续列入 `P0/P1`。

3. 差距评估文档需要同时参考代码入口和 `docs/reports/*` 验收报告，而不是只看类/接口是否存在。



### 解决方案

- 将 `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 全文更新到 2026-03-08 版本。

- 重写模块总览、模块统计、剩余差距清单与建议里程碑。

- 把字幕、播放列表、设置、快捷键、解码器管理、截图等已落地能力从“骨架/未接入”纠正为“部分实现”。

- 新增“验收与报告证据”章节，明确评估依据包含本地回归报告。

- 更新文档索引，补一条本次差距评估对齐说明。



### 本地校对结果

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 标题日期已更新为 `2026-03-08`。

- 模块统计更新为：`部分实现 11 / 14`、`骨架/未接入 3 / 14`。

- `P0/P1/P2` 剩余项已与当前任务清单和验收报告保持一致。



### 修改文件

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 问题 44: `docs/records/VERSION.md` 历史路径描述过期



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `docs/records/VERSION.md` 的早期阶段描述仍保留旧版 decoder/thread/test 文件级表述，看起来像当前仓库仍在使用这些路径。

- 这会影响按文档遍历仓库时的进度判断，也容易把历史实现和当前主链结构混为一谈。



### 分析记录

1. `2026-03-06` 架构收敛后，播放器主链已经切到 `PlayerCore + Scheduler + core/*`，但版本文档的历史章节没有同步去歧义。

2. “阶段一：基础播放器 (当前阶段)” 与“下一步计划”等措辞已经不再符合 `2026-03-08` 的实际状态。

3. 历史章节应该保留能力演进和问题修复记录，但不应再把已删除文件当作当前仓库结构说明。



### 解决方案

- 将阶段一改写为“历史起点”，并补充旧版 `decoder / playLoop` 架构已并入当前主链的说明。

- 将 `video_decoder` / `audio_decoder`、`VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 等旧路径改写为能力级历史表述。

- 将“下一步计划”、`USE_NEW_PLAYER_CORE`、临时 `tests/core_*` 的相关描述调整为历史记录口径。

- 在 `docs/records/VERSION.md` 的文档更新日志中补记本次清理。



### 本地校对结果

- `docs/records/VERSION.md` 不再出现“当前阶段”“下一步计划”等易误导当前状态的旧口径。

- 早期章节对旧版 decoder/thread 的表述已改为历史说明，并指向当前 `core/*` 主链。

- 本次变更限定在文档层，未扩散到代码或构建配置。



### 修改文件

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 45: README 与架构文档仍混用旧主链表述



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 根目录 `README.md` / `README_ZH.md` 仍把 `video_decoder`、`audio_decoder` 等旧文件写成项目当前结构。

- `docs/design/ARCHITECTURE.md` 同时混用了“当前实现”和旧模块命名，读者容易误判现行主链仍依赖这些历史模块。



### 分析记录

1. `2026-03-06` 架构收敛后，当前播放器主链已经稳定在 `VideoPlayer + PlayerCore + Scheduler + core/*`。

2. `README` 更偏向“快速理解仓库”的入口文档，因此目录树和架构示意应优先反映现状，而不是保留已删除路径。

3. `docs/design/ARCHITECTURE.md` 作为背景文档可以保留历史内容，但必须显式标注哪些章节属于历史实现。



### 解决方案

- 重写 `README.md` / `README_ZH.md` 的项目结构树和架构示意，改为当前主链口径。

- 在 `docs/design/ARCHITECTURE.md` 顶部增加状态说明，并将旧 decoder/thread/sync 章节统一标成“历史实现”。

- 将文档中的日志示例改为当前 `Quill` 宏接口。

- 更新 `docs/README.md` 的架构文档索引说明，区分历史基线和当前重构说明。



### 本地校对结果

- `README.md` / `README_ZH.md` 已不再把 `video_decoder` / `audio_decoder` 写为当前目录结构。

- `docs/design/ARCHITECTURE.md` 已明确声明历史章节边界，并不再把旧多线程链路标为“当前实现”。

- 本次改动仍限定在文档层，未涉及代码和构建配置。



### 修改文件

- README.md

- README_ZH.md

- docs/design/ARCHITECTURE.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 46: 实现教程与迭代计划缺少历史/当前边界说明



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `docs/guides/IMPLEMENTATION.md` 保留的是早期原型的从零实现教程，但没有明确声明其中 `video_decoder`、`audio_decoder`、单体 `playLoop` 等内容属于历史实现。

- `docs/plans/MPC_HC_ITERATION_PLAN.md` 记录的是 `2026-03-07` 的计划基线，但没有提示其中部分能力已在后续提交中落地。



### 分析记录

1. 这两份文档都还有使用价值：前者偏教程，后者偏规划；问题在于缺少和“当前代码状态”的显式分界。

2. 经过前几轮清理后，`README`、`VERSION`、`ARCHITECTURE` 已经区分历史与当前，如果这两份文档不跟进，读者仍会在入口层产生混淆。

3. 最合适的做法不是重写全文，而是在文档顶部补充状态说明，并把索引描述统一到同一口径。



### 解决方案

- 为 `docs/guides/IMPLEMENTATION.md` 增加“状态说明（2026-03-08）”，明确其为早期原型教程，并指向当前主链参考文档。

- 为 `docs/plans/MPC_HC_ITERATION_PLAN.md` 增加“状态说明（2026-03-08）”，明确其为 `2026-03-07` 的计划快照，并列出当前进度参考入口。

- 更新 `docs/README.md`、`README.md`、`README_ZH.md` 的文档说明，使历史教程、计划快照、当前主链说明三者边界一致。



### 本地校对结果

- `docs/guides/IMPLEMENTATION.md` 已不再被表述为当前仓库的逐文件实施清单。

- `docs/plans/MPC_HC_ITERATION_PLAN.md` 已明确为计划快照，并指向当前进度来源。

- 本次改动仍限定在文档层，未涉及代码、构建和任务清单状态修改。



### 修改文件

- README.md

- README_ZH.md

- docs/guides/IMPLEMENTATION.md

- docs/plans/MPC_HC_ITERATION_PLAN.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 47: 辅助说明文档仍缺少当前入口与状态边界



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- `docs/design/FILTERS.md`、`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`、`docs/guides/WINDOWS_SETUP.md` 仍偏向“静态说明”，缺少与当前代码主链、依赖探测方式和文档适用范围的衔接说明。

- `docs/guides/WINDOWS_SETUP.md` 还保留了旧的 `SDL2_DIR` / `FFMPEG_DIR` 配置示例，与当前 `CMakeLists.txt` 的手动依赖回退路径不完全一致。



### 分析记录

1. 经过前几轮清理后，入口文档和核心设计文档已经区分了“历史基线”和“当前主链”，但辅助说明文档还没有完全跟上。

2. `FILTERS.md` 的主要问题不是错误，而是缺少“当前默认主流程入口是 `FilterPipeline`”这一层解释。

3. `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 更像专题参考笔记，需要提醒读者不要把其中的目标项直接等同于当前未完成项。

4. `WINDOWS_SETUP.md` 则需要与现有 `CMakeLists.txt` 的依赖探测顺序保持一致，否则会影响实际搭建体验。



### 解决方案

- 为 `docs/design/FILTERS.md` 增加状态说明，并补齐当前内置音频滤镜 `volume_balance` 与链路说明。

- 为 `docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 增加状态说明，明确其是专题参考而不是全量进度面板。

- 为 `docs/guides/WINDOWS_SETUP.md` 增加状态说明，修正 Quill 说明、手动安装示例和共享库使用说明，使之与当前 `CMakeLists.txt` 对齐。

- 更新 `docs/README.md` 索引，将 `FILTERS.md` 纳入可发现入口，并追加本轮文档整理记录。



### 本地校对结果

- `docs/design/FILTERS.md` 已明确当前生效的滤镜主链与预留组件边界。

- `docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 已明确为专题参考文档，并指向当前总进度来源。

- `docs/guides/WINDOWS_SETUP.md` 已不再建议使用与当前仓库不一致的 `SDL2_DIR` / `FFMPEG_DIR` 传参示例。

- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。



### 修改文件

- docs/design/FILTERS.md

- docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md

- docs/guides/WINDOWS_SETUP.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 48: 根 README 故障排除与历史问题归档仍有旧口径



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 根目录 `README.md` / `README_ZH.md` 的 Windows 故障排除仍提示使用 `FFMPEG_DIR` 传参，这与当前 `CMakeLists.txt` 的自动探测 / `external/ffmpeg` 回退逻辑不一致。

- `docs/analysis/video-stream-index-fix.md` 记录的是早期原型阶段的问题，但缺少“历史归档”标识，文中的 `playLoop` 与 `src/video_decoder.cpp` 易被误读为现行实现。



### 分析记录

1. 前几轮已经清理了 `WINDOWS_SETUP.md` 的构建入口，但根 README 的简版故障排除还残留旧说明。

2. `video-stream-index-fix.md` 作为问题分析样本仍然有价值，适合保留；重点是让读者明确它属于早期阶段。

3. 这两类问题都属于“入口文档和历史归档之间边界不清”，适合一起收尾。



### 解决方案

- 将根 README 的 FFmpeg 故障排除改为当前 `vcpkg toolchain / external/ffmpeg` 口径。

- 为 `docs/analysis/video-stream-index-fix.md` 增加状态说明，标记其为 `2026-02-24` 早期原型问题分析归档。

- 在 `docs/README.md` 中为该历史文档补充索引，并记录本轮更新。



### 本地校对结果

- `README.md` / `README_ZH.md` 不再建议使用与当前主链不一致的 `FFMPEG_DIR` 传参方式。

- `docs/analysis/video-stream-index-fix.md` 已明确为历史归档，不再暗示 `playLoop` / `video_decoder.cpp` 属于当前仓库结构。

- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。



### 修改文件

- README.md

- README_ZH.md

- docs/analysis/video-stream-index-fix.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 49: 缺少独立的文档巡检总表



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 本轮已经连续完成多批文档清理，但结果仍然分散在多个提交和日志条目里。

- 如果后续需要继续维护文档口径，缺少一份“一眼看懂本轮清理范围和结论”的总表。



### 分析记录

1. `CHANGELOG.md` 和 `DEVELOP_LOG.md` 能记录过程，但对后续维护者来说不够聚合。

2. `docs/README.md` 提供了索引入口，但没有一份专门解释“本轮文档巡检结论”的汇总报告。

3. 增加单独报告文档，可以把“当前主链基线、已收敛文档、保留历史内容、后续建议”固定下来，减少重复解释成本。



### 解决方案

- 新增 `docs/analysis/DOC_AUDIT_2026-03-08.md`，作为本轮文档巡检总表。

- 报告中集中记录：巡检范围、关键词方法、当前主链基线、已完成对齐项、保留但合理的历史内容、后续维护建议、关联提交。

- 在 `docs/README.md` 中将该报告纳入索引，并追加更新历史说明。



### 本地校对结果

- `docs/analysis/DOC_AUDIT_2026-03-08.md` 已可独立说明本轮文档清理工作的范围与结论。

- `docs/README.md` 已可直接定位到该巡检报告。

- 本次改动仍限定在文档层，未涉及代码、构建脚本和任务清单状态修改。



### 修改文件

- docs/analysis/DOC_AUDIT_2026-03-08.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 问题 50: M4 4.4（帧步进）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `4.4` 要求支持暂停态逐帧查看画面。

- 当前仓库缺少暂停态帧步进入口，无法像 MPC-HC 一样在暂停后逐帧前后检查画面。



### 分析记录

1. 输入层当前只有播放、seek、音量、章节、A-B Repeat、截图等动作，没有帧步进动作和默认键位。

2. `PlayerCore` 暂停时会冻结调度器，因此不能直接复用常规渲染循环，需要一条暂停态的定向刷新路径。

3. 初版实现联调时发现，音频消费线程在暂停态仍会用旧 `playback_pts` 回写 `position_`，会把步进结果覆盖掉，需要同步收口。



### 解决方案

- 新增 `step_frame_backward` / `step_frame_forward` 热键动作，默认绑定 `,` / `.`。

- 在 `Display`、渲染器接口、`PlayerCore`、`VideoPlayer` 之间打通帧步进请求与 API。

- 采用“暂停态 seek + 首帧刷新”的方式实现前后单帧步进，并在步进后维持暂停状态。

- 调整音频消费线程：仅在 `PlaybackState::Playing` 时才用音频播放位置回写主时间轴。

- 新增 `--frame-step-check .\\juren-30s.mp4` 自检命令，并记录本地验收结果。



### 本地校对结果

- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`：`PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`：`PASS`

- 默认热键文档已补充 `, / .` 的暂停态帧步进说明。



### 修改文件

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



## ?? 51: M4 4.5???/???????



**??**: 2026-03-08

**??**: ???



### ????

- ???? `4.5` ???????? / ???????

- ?????? `J/K` ? `Ctrl+J/K` ?????????????? MPC-HC ??????????



### ????

1. ??????????????????????????????????????????

2. ??????????????????????????????????????????????????

3. ???????????????????????????????????????????????



### ????

- ?? `SubtitleDelayDown` / `SubtitleDelayUp` ??????? `J / K`??? `Ctrl` ???????????

- ? `PlayerCore` ????/??????? getter/setter?

  - ???? `updateSubtitleOverlay` ???????????????

  - ??????? PTS???????????????????????????

- ?? `AppSettings`?`config/player_settings.ini` ? `--settings-persistence-check`?????????/???

- ?? `--delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt` ???????? README / ???? / ???? / ?????



### ??????

- `build/Debug/modern-video-player.exe --settings-persistence-check`?`PASS`

- `build/Debug/modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt`?`PASS`

- ????????? `J / K` ? `Ctrl+J / Ctrl+K` ????????



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



## ?? 52: M4 4.6????? `1..9` ???



**??**: 2026-03-08

**??**: ???



### ????

- `4.6` ????????? `1..9` ???????

- ????A-B Repeat???????????????????????? MPC-HC ???????????????????????



### ????

1. ??????????????????????????????????? `1..9` ????????????????????

2. `Display` ? `PlayerCore` ?????????????? seek ???????????????? `seek_ratio_` ?????????

3. ???????????????????????? 10% / 90% ???????????????????



### ????

- ?? `SeekTo10Percent` ~ `SeekTo90Percent` ????????? `1..9`?

- ? `Display` ????????????? `0.1` ~ `0.9` ???? seek ???

- ?? `--numeric-seek-check .\juren-30s.mp4` ???????? README / ???? / ???? / ???



### ??????

- `build/Debug/modern-video-player.exe --numeric-seek-check .\juren-30s.mp4`?`PASS`

- ????????? `1..9` ?? `10%..90%` ????



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



## 问题 53: M2 2.2.4：输出播放性能日志（掉帧/队列/CPU/GPU）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `2.2.4` 需要一个统一的性能日志入口，用于观察高分辨率/高码率样本的掉帧、队列和资源占用情况。

- 当前播放器主链已经具备较多内部统计，但外部没有稳定的结构化验收输出，无法直接沉淀为本地报告。



### 分析记录

1. `PlayerCore` 已经维护了 demux、decode、render 等多组原子计数，`Scheduler` 也有掉帧与解码统计，但缺少统一快照接口。

2. 现有命令行自检更偏向功能验证，不适合做 `1080p60 / 4K / 高码率` 样本的性能对比留档。

3. GPU 直接占用率采样跨平台成本较高，因此本轮先输出当前激活的解码 backend / 渲染 backend，作为 GPU 路径观测信息。



### 解决方案

- 新增 `core::DiagnosticsSnapshot`，在 `PlayerCore` 中收敛 demux / decode / render / scheduler / queue 指标。

- 在 `VideoPlayer` 中增加 `getInfo()` / `getDiagnosticsSnapshot()` 透传接口。

- 在 `main` 中新增 `--performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200` 自检命令，输出 CPU 平均占用、逻辑核心数、backend、掉帧与队列指标。

- 新增 `docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`，并同步任务清单、差距评估、版本文档与变更记录。



### 本地校对结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`：`PASS`

- 当前样本输出包含 `renderer_backend=D3D11`、`decoder_backend=D3D11VA`、`cpu_avg_percent≈100%`、`scheduler_late_drops=0` 等关键指标。



### 修改文件

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



## 问题 54: M2 2.2.1 / 2.3.2：1080p60 稳定播放验收



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 需要为 `1080p60` 稳定播放建立明确的本地验收入口，避免只凭性能日志或人工观察判断。

- 当前样本包没有显式的 `1080p60` 稳定性样本生成说明，复现实验路径不够清晰。



### 分析记录

1. `collectFileProbeReport()` 已经能给出宽高、FPS、推荐 backend 等元信息，适合做稳定性验收前置条件。

2. `DiagnosticsSnapshot` 已可提供 `scheduler_late_drops` / `demux_dropped_packets` 等关键稳定性指标。

3. 还需要补一个“连续播放窗口内时间推进”的判断，才能把 `1080p60` 验收从观测日志提升为明确门禁。



### 解决方案

- 新增 `--1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`，检查 `probe`、时间推进、late drop 与 demux drop。

- 在 `tools/download_test_samples.ps1` 增加 `1080p60 AAC 2ch` 样本生成路径，并在 `samples/README.md` 标记其用于 `2.2.1 / 2.3.2`。

- 新增 `docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。



### 本地校对结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`：`PASS`

- 当前样本输出包含 `advance_ratio=0.994133`、`late_drops=0`、`demux_dropped_packets=0`、`decoder_backend=D3D11VA`。



### 修改文件

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



## 问题 55: M2 2.2.2 / 2.3.3：4K 播放与降级验收



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 需要把 `4K` 样本播放与“失败时可降级”收敛成一个统一的本地验收入口。

- 现有能力分散在性能日志与 Windows 后端回退检查中，不利于直接对应任务清单 `2.2.2 / 2.3.3`。



### 分析记录

1. `collectFileProbeReport()` 已能确认样本是否为 `3840x2160`，适合做 4K 门禁前置判断。

2. `runBackendSessionSubprocess()` 已能稳定验证 hard / soft 两个后端模式，无需重新实现降级流程。

3. 还需要一个主进程连续播放窗口，确保 `4K` 样本不是“只打开不推进”，而是真正进入播放状态。



### 解决方案

- 新增 `--4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`。

- 主进程检查 `probe`、时间推进、`late_drop` 与当前 backend；子进程检查 hard / soft 模式都能进入播放。

- 新增 `docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。



### 本地校对结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`：`PASS`

- 当前样本输出包含 `advance_ratio=0.968167`、`late_drops=0`、`hard.decoder_backend=D3D11VA`、`soft.decoder_backend=Software`。



### 修改文件

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md





---



## 问题 56: M2 2.2.3：>80Mbps 高码率样本验收



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 需要一个真正超过 `80Mbps` 的回归样本和对应验收入口，才能完成 `2.2.3`。

- 当前样本集合虽然覆盖分辨率与格式，但码率普遍偏低，无法作为高码率门禁。



### 分析记录

1. 现有 `collectFileProbeReport()` 没有直接输出码率，但 FFmpeg 的 `AVFormatContext::bit_rate` 可直接复用。

2. 高码率任务的核心不在分辨率，而在于样本真实码率、连续播放窗口推进和掉帧/丢包情况。

3. 基于当前 `D3D11VA + D3D11` 主链，`100Mbps` 级别的 `1080p60` H.264 样本已能稳定进入播放链路，适合作为本地回归基线。



### 解决方案

- 新增 `collectFormatBitrateBitsPerSecond()` 与 `--high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`。

- 在 `tools/download_test_samples.ps1` 增加 `100Mbps` 样本生成路径，并在 `samples/README.md` 标注用途。

- 新增 `docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`，同步任务清单、差距评估与版本记录。



### 本地校对结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`：`PASS`

- 当前样本输出包含 `format_bitrate_bps=102829290`、`advance_ratio=0.988444`、`late_drops=0`、`demux_dropped_packets=0`。



### 修改文件

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



## 问题 57: 发布门禁 6.5（长时播放稳定性）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `6.5` 需要一个可重复执行的本地入口，验证长时播放窗口内无 crash 且仍能持续推进。



### 分析记录

1. 现有 `1080p60` / `4K` / `>80Mbps` 验收命令都以短窗口为主，不能直接作为“长时播放无 crash”的证明。

2. `VideoPlayer` 已提供 `play()` / `isPlaying()` / `getCurrentTime()` / `getDiagnosticsSnapshot()`，可以复用为 smoke 验收闭环。

3. 只要同时验证播放态保持、时间推进与 `late_drop` / demux drop，即可形成一个足够轻量的发布门禁。



### 解决方案

- 新增 `--long-playback-check .\juren-30s.mp4 10000`，要求采样窗口不少于 `5000ms`。

- 命令输出 `probe_duration`、`renderer_backend`、`decoder_backend`、`advance_ratio`、`late_drops`、`demux_dropped_packets` 等结构化字段。

- 新增 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`，并同步任务清单、差距评估、版本记录与变更记录。



### 本地验收结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`：`PASS`

- 当前输出包含 `probe_duration=30.03`、`entered_playback_loop=true`、`still_playing_after_window=true`、`advance_ratio=0.996267`、`late_drops=0`、`demux_dropped_packets=0`、`renderer_backend=D3D11`、`decoder_backend=D3D11VA`。



### 修改文件

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---



## 问题 58: 7.1 插件系统（动态加载与生命周期闭环）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `7.1` 需要一个真正可运行的插件宿主，而不只是内存中的元数据占位。



### 分析记录

1. 现有 `PluginManager` 只能做静态描述符注册与启停标记，无法覆盖 `DLL` 动态加载、版本兼容和卸载清理场景。

2. 代码库已经有 `FilterRegistry` 这类天然扩展点，适合作为首个插件闭环的宿主能力。

3. 如果没有示例 `DLL` 和命令行验收入口，就无法证明插件系统已从“骨架”进入“可运行”。



### 解决方案

- 新增 `include/plugin/plugin_api.h`，定义插件宿主接口与导出符号。

- 重写 `PluginManager`，支持按路径加载插件、校验 `API` 版本、执行 `initialize/shutdown` 生命周期，并跟踪插件注册的滤镜工厂用于卸载清理。

- 新增 `sample_logger_plugin` 示例插件与 `--plugin-check` 验收命令；插件会注册 `sample_identity` 视频滤镜，供宿主验证注册/卸载闭环。



### 本地验收结果

- `cmake --build build --config Debug`：通过。

- `build/Debug/modern-video-player.exe --plugin-check`：`PASS`

- 当前输出包含 `loaded_count=1`、`plugin_ids=sample_logger_plugin@0.1.0`、`sample_video_filter_registered=true`、`sample_video_filter_unloaded=true`、`errors=none`。



### 修改文件

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



## 问题 59: 7.2 流媒体（真实 HTTP 分片与缓冲）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `7.2` 需要一个真正可跑的 HTTP 分片下载与缓冲入口，而不是停留在清单解析骨架。



### 分析记录

1. `HttpStreamDownloader` 之前没有真实读流实现，`readChunk()` 永远返回空数组。

2. 当前代码已经具备 HLS 清单解析器，适合以 HLS 媒体清单作为首个流媒体 smoke 入口。

3. 在受限网络环境下，最稳定的验收方式是在本机起一个小型 HTTP 夹具服务，避免依赖外部站点。



### 解决方案

- 重写 `HttpStreamDownloader`，基于 FFmpeg `avio` 支持真实 HTTP 打开、分块读取、内部缓冲、EOF 状态与错误透传。

- 新增 `--streaming-buffer-check`，下载 HLS 清单、解析相对 URL、抓取前 3 个分片并验证缓冲字节数。

- 新增 `tools/start_streaming_fixture_server.ps1` 与 `samples/streaming/hls_local/*` 夹具，用于本机 HTTP 回归。



### 本地验收结果

- `cmake --build build --config Debug`：通过。

- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`：`PASS`

- 当前输出包含 `manifest_download_ok=true`、`manifest_parse_ok=true`、`segments_downloaded=3`、`buffered_bytes=621`、`buffer_ok=true`、`error=none`。



### 修改文件

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





## 问题 60: 7.3 HLS/DASH 自适应码率



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `7.3` 需要一个可重复执行的 HLS/DASH 多码率解析与自适应码率本地验收入口。



### 分析记录

1. 现有 `HlsManifestParser` 只能读取媒体播放列表，无法处理 `master playlist`。

2. 现有 `DashManifestParser` 只知道 `Representation` 带宽，无法拿到初始化分片和媒体分片 URL。

3. 最稳妥的验收方式仍是在本机 HTTP 夹具下，分别验证 HLS/DASH 的档位选择与升降档路径。



### 解决方案

- 扩展 HLS/DASH 解析器，补齐 variant / representation / `BaseURL` / 初始化分片 / 媒体分片明细。

- 新增 `AdaptiveBitrateSelector`，并在 `main` 中提供 `--adaptive-bitrate-check` 命令。

- 新增 `samples/streaming/abr_local/{hls,dash}` 样本与报告，验证 HLS/DASH 在给定带宽序列下的升码率与降码率切换。



### 本地验收结果

- `cmake --build build --config Debug`：通过。

- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`：`PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`：`PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`：`PASS`

- 当前输出包含 `switch_count=2`、`upswitch_count=1`、`downswitch_count=1`，说明 HLS/DASH 两条路径都完成了一次升档和一次降档。



### 修改文件

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

## 问题 61: 建立里程碑标签（v0.2.0-rc1 / v0.2.0）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 任务清单 `0.4` 尚未完成，仓库缺少 `v0.2.0-rc1` 与 `v0.2.0` 的里程碑标签。



### 分析记录

1. 当前仓库 `git tag --list` 为空，说明还没有任何正式里程碑标记。

2. `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 仍写着“当前仅剩版本标签操作未执行”，需要在标签建立后立即回写。

3. 里程碑标签属于仓库状态的一部分，除了创建标签本身，还需要同步任务状态与版本记录。



### 解决方案

- 建立 `v0.2.0-rc1` 与 `v0.2.0` 两个里程碑标签。

- 同步更新版本记录、变更记录、开发日志、差距评估和任务清单。

- 基于 `v0.2.0-rc1` 已可稳定建立，执行约束 `5.3 每个里程碑结束前必须可打 RC 标签` 可判定为满足。



### 本地验收结果

- `git tag --list`：创建前为空，创建后包含 `v0.2.0-rc1` 与 `v0.2.0`。

- 任务清单 `5.3` 已可随 `0.4` 一并勾选。

- 标签建立后可通过 `git show <tag> --no-patch --stat` 验证指向提交。



### 修改文件

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 问题 62: 执行守则口径同步（5.1 / 5.2）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 守则项 `5.1 / 5.2` 需要根据当前仓库与交付节奏做一次口径同步，避免任务清单与实际执行状态脱节。



### 分析记录

1. 本轮主要任务按“发布门禁 → 插件系统 → 流媒体缓冲 → ABR → 里程碑标签 → 守则口径”的顺序串行推进，没有出现超过 2 个主任务并行推进的证据。

2. `5.2` 的判断条件是“每周五只做收敛”，这要求持续性的周节奏记录，而不仅是一轮收口结果。

3. 因此 `5.1` 可以勾选，`5.2` 仍应保持待完成。



### 解决方案

- 勾选 `5.1 WIP 限制：同时进行任务不超过 2 个`。

- 保留 `5.2 每周五只做收敛（修复、回归、文档）` 为待完成，并在版本/变更记录中写明原因。



### 本地验收结果

- 任务清单已更新为：`5.1` 完成、`5.2` 待完成、`5.3` 完成。

- 当前仓库没有新增代码改动，本次只同步过程约束口径。



### 修改文件

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 问题 63: 落地 5.2 周五收敛日执行手册



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 用户要求继续处理守则项口径，并把 `5.2 每周五只做收敛（修复、回归、文档）` 落成一份可执行的流程约束文档与节奏说明。



### 分析记录

1. `5.2` 当前仍不应直接勾选，因为它要求的是跨周、重复发生的过程证据。

2. 但在未勾选之前，仍然可以先把“如何执行周五收敛日”文档化，降低后续执行口径漂移。

3. 最合理的落地方式，是新增一份类似操作手册的文档，把周节奏、允许事项、禁止事项、执行顺序、输出物与勾选依据全部写清楚。



### 解决方案

- 新增 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`，将 `5.2` 固化为可执行流程。

- 在 `docs/README.md` 增加入口，并同步版本记录与变更记录。

- 保持任务清单 `5.2` 未勾选，待后续跨周执行形成证据后再更新状态。



### 本地验收结果

- 新文档已覆盖周一到周五的节奏分工。

- 新文档已列出收敛日允许事项、禁止事项、最短执行路径与输出物。

- 当前仓库没有新增代码改动，本次只新增流程文档并同步文档索引与记录。



### 修改文件

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 问题 64: 补齐 5.2 留痕模板（daily_board / 周报）



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 用户要求继续把 `5.2` 的“留痕模板”补到 `daily_board / 周报` 里，使周五收敛的执行证据有固定载体。



### 分析记录

1. `5.2` 现在已经有原则和流程，但还没有真正面向执行的填写模板。

2. 当日看板和每周汇总属于两个不同粒度：前者记录“周五当天做了什么”，后者记录“这一周是否满足 5.2”。

3. 因此需要同时补齐这两种模板，才能在后续跨周证据累积时保持口径一致。



### 解决方案

- 在 `.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 中，为 Day 5 / Day 10 增加收敛日记录卡。

- 新增 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`，作为每周周报与收敛留痕模板。

- 同步 `tasklist / WEEKLY_CONVERGENCE_PLAYBOOK / README / 版本记录`，把入口和用途写清楚。



### 本地验收结果

- 现在每个周五都可以在 `daily_board.md` 直接填写范围冻结、回归命令、blocker 结论和阶段结论。

- 现在可以基于 `weekly_report_template.md` 复制生成每周周报实例，用于判断是否满足 `5.2`。

- 当前仓库没有新增代码改动，本次只补齐模板与文档入口。



### 修改文件

- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md

- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 问题 65: 汇总当前功能、使用方式与验证入口



**日期**: 2026-03-08

**状态**: 已解决



### 问题描述

- 用户要求列出程序当前的所有功能、可用使用方式，以及目前应该如何验证项目现有功能，并将这些信息记录到文档中。



### 分析记录

1. 当前项目的功能已经不再只是“基础播放”，还包括字幕、播放列表、设置、快捷键、章节、A-B、截图、帧步进、格式探测、Windows 后端、插件与流媒体验收能力。

2. 这些能力目前分散在 `main.cpp` 帮助输出、任务清单、本地验收报告和若干文档中，缺少面向阅读者的一页式总览。

3. 最合适的做法是新增一份“功能 / 使用 / 验证”总览文档，并同步索引与根 README 入口。



### 解决方案

- 新增 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`，把当前主程序能力分成“用户可直接使用”和“开发/验收能力”两层来整理。

- 在文档中给出普通播放方式、能力探测方式、配置方式、默认交互方式和功能验证表。

- 同步更新 `docs/README.md` 与根 `README.md`，保证入口可见。



### 本地验收结果

- 新文档已覆盖当前功能列表、普通使用方式、诊断方式、配置方式与专项验证入口。

- 新文档已明确流媒体、插件、滤镜增强等能力的当前边界，避免误判为完整用户功能。

- 当前仓库没有新增代码改动，本次只新增说明文档并同步索引。



### 修改文件

- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md

- docs/README.md

- README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md







## 问题 69: 播放链诊断分层与 decoder drain / scheduler 容错补强



**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求继续围绕高码率播放稳定性，评估 `FrameQueue` 背压、decoder `receive/send` 时序、`Display::copyFrameData()`、`av_hwframe_transfer_data()` 与 `Scheduler` 一次重启限制等潜在问题，并在确认后落地优化。



### 分析记录

1. 当前源码已具备条件成立时的 `D3D11` native render path，因此旧分析文档里“主链一定经过 copy-back + SDL upload”的结论已经过时，运行时更需要知道“当前到底命中了 native / copy-back / swscale 哪条路径”。

2. `decodeVideoFrame()` / `decodeAudioFrame()` 采用 receive-first 思路本身没问题，但此前 packet queue EOF 后没有给 codec 发送 `nullptr` drain，而且 send 后只尝试一次 receive，不够接近成熟播放器常见的 drain/feed 语义。

3. `demux_dropped_packets_` 之前没有细分“非目标流被忽略”与“目标流入队失败”，会误导对背压和吞吐瓶颈的判断。

4. `Scheduler` 之前只保护了解码线程，render thread 没有同样的异常保护；同时没有结构化导出背压与 restart 指标。

5. 本轮设计参考了 `ffplay` 常见的 decoder drain 模式，以及 `mpv / MPC-HC` 常见的“先把路径与原因量出来，再调参数”的诊断思路。



### 解决方案

- 重写 `decodeVideoFrame()` / `decodeAudioFrame()`：改成持续 `receive -> send -> receive` 状态机，并在 packet EOF 后向 codec 发送 `nullptr` 完成 drain。

- 为 `DiagnosticsSnapshot` 增加 demux drop 分类、decoder `send_packet(EAGAIN)`、drain 次数，以及 `native/copy-back/swscale/filter-blocked` 视频路径计数。

- 为 `SchedulerStats` 增加 video/audio 背压事件与 video/audio/render restart 统计，并把 render thread 纳入 `runProtectedLoop()`。

- 扩展 `--performance-log-check` 输出，让后续 4K/高码率分析能直接看见当前是否命中 native path、是否真的出现 queue drop、是否频繁背压。



### 本地验收结果

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`：通过。

- `build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`：通过；当前环境音频初始化失败，但新诊断字段已成功导出，且可见 `demux_dropped_packets` 在本次样本中全部属于 `demux_ignored_packets`，不是 `queue drop`。



### 修改文件

- include/core/scheduler.h

- src/core/scheduler.cpp

- include/core/player_core.h

- src/core/player_core.cpp

- src/main.cpp

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 问题 70: PlayerCore 状态机重设计第一阶段



**日期**: 2026-03-19

**状态**: 已解决



### 问题描述

- 用户要求先落地播放器内核状态机重设计第一阶段：保留对外 `PlaybackState` 兼容，只在 `PlayerCore` 内部拆出会话态、运行态和流水线态，并收口散点状态改写。

- 本轮明确不做 timeline serial、EOF -> Ended 语义重写，也不提前升级 `Scheduler` 契约。



### 日志输出

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 分析记录

1. 当前 `PlaybackState` 之前同时承载了 UI 播放态、会话态和流水线过程态，`deferred stop` 还额外游离在统一状态机之外。

2. `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 直接写 `state_`，导致状态迁移日志、非法迁移保护和后续 serial 化都没有稳定落点。

3. `Scheduler` 当前仍只有 `running_ / paused_`；本轮最合理的边界，是先让 `PlayerCore` 成为状态权威，再把更细的控制快照留给下一阶段。



### 解决方案

- 在 `PlayerCore` 内部新增 `SessionState / RunState / PipelinePhase` 与 `CoreStateSnapshot`。

- 新增统一 transition helper 与 `publishPlaybackStateFromInternalState()`，让对外 `PlaybackState` 成为内部状态的投影。

- 将 `eof_reached / pending_seek / deferred_stop_pending` 纳入统一快照管理，并在状态迁移时输出结构化日志。

- 保持现有线程起停与 `Scheduler` 调用方式基本不变，避免第一阶段范围失控。



### 本地验收结果

- Debug 构建通过。

- `PlayerCore` 相关代码中，散点 `state_.store/exchange` 已收口到统一 publish 入口。

- 新增分析文档：`docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md`。



### 修改文件

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 问题 81: PlayerCore seek/flush timeline serial 化第二阶段



**日期**: 2026-03-20

**状态**: 已解决



### 问题描述

- 用户要求继续执行播放器内核状态机重设计第二阶段，范围限定在 seek/flush timeline serial 化，不动 copy-back、SoftwareSDL、UI 层和外部 `PlaybackState`。

- 本轮的核心目标，是把 seek/stop/deferred stop 相关路径里对旧时间线数据的处理，从“flush 软清理”升级为“serial 硬失效”。



### 日志输出

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 分析记录

1. 第一阶段已经把 `SessionState / RunState / PipelinePhase` 和统一 transition 入口落到了 `PlayerCore`，因此第二阶段具备稳定的 serial 收口点。

2. seek 旧实现的问题，不是 flush 不够多，而是 packet/frame 缺少时间线身份；即使做了 `pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop() + 二次 flush`，旧数据仍然可能晚到。

3. `ThreadSafeQueue` / `FrameQueue` 本体都没有 generation/serial，audio consumer 线程也不会在 seek 时自然退出，render 路径之前同样没有 serial gate，所以必须把“废弃规则”放到 packet/frame 本身和消费边界上。

4. 这轮因此选择 item-level serial，而不是先把 queue 容器做成播放器专用 epoch 容器；这样改动更聚焦，也更符合“第二阶段先做 seek/flush serial 化”的范围控制。



### 解决方案

- 新增 `TimelineSerial`，并让 `DemuxPacket`、`VideoFrame`、`AudioFrame` 都携带 serial。

- 在 `PlayerCore` 内部新增 `timeline_serial / pending_seek_serial` 及统一 helper，禁止在 `seek / stop / open / deferred stop` 各处零散自增 serial。

- `open` 成功后建立首个 serial；`seek` 开始时先分配 `pending_seek_serial`，让接受边界先切走，seek 成功后再激活；`stop / requestDeferredStop` 会立即推进 serial。

- demux 线程在启动时捕获 active serial，decode/render/audio consumer 全链路按 serial 丢弃 stale 数据；`flush` 继续保留，但降级为辅助清扫。

- diagnostics 和专项检查命令新增 `timeline_serial / pending_seek_serial` 输出，便于后续继续做 EOF/Ended 与 Scheduler 契约阶段。



### 本地验收结果

- Debug 构建通过。

- packet/frame 到 render/audio consumer 的链路已经具备 serial 丢弃边界。

- 新增分析文档：`docs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md`。



### 修改文件

- include/core/frame.h

- src/core/frame.cpp

- include/core/player_core.h

- src/core/player_core.cpp

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## ?? 82: PlayerCore EOF/Ended ????????



**??**: 2026-03-20

**??**: ???



### ????

- ??????????????????????????????????? copy-back?SoftwareSDL?UI ???? `PlaybackState`?

- ????? EOF ? stop/deferred-stop ?????????????????????? `Ended`???? `close/reopen` serial ????? scheduler stale render ?????



### ????

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### ????

1. ???????? packet/frame ? seek/stop ????? serial ???? EOF ?????????? `Stopped`???????????? stop ??????

2. ?? demux EOF?packet queue EOF ? frame queue ???????????????????????????????????? ended ?????

3. `close()` ???? `stop()` ???????? timeline ????????? scheduler ???? render callback ????????? stale serial ??????????



### ????

- ? `PlayerCore` ????? `EndedReason`??? `ended_reason` ?? `CoreStateSnapshot`?`DiagnosticsSnapshot`?????????????

- ?? `onRenderIdle()`?? demux EOF ???? `PipelinePhase::Draining`???? packet queue EOF + frame queue ?? + `audio_player_->getBufferedSeconds() <= 0.001`????? `RunState::Ended`?

- ??? EOF ? `deferred stop` ?? `Stopped`?`play()` ? `Ended` ???? `seek(0.0)`?`seek()` ? `Ended` ????? `Stopped`?

- ? `close()` ??? stop ???? serial ?????? worker ??????? stale serial ????

- ? scheduler render callback ???? `bool`????? render/present ?????? `rendered_frames` ???????

- ?????? `docs/analysis/PLAYERCORE_DAY18_EOF_ENDED_AND_TERMINAL_STATE_REDESIGN.md`?????????????????



### ??????

- Debug ?????

- EOF ???????? `Ended`???????? `Stopped`?

- `play()` ? `Ended` ??????????

- close/reopen serial ??? scheduler ??????????????



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

## ?? 83: PlayerCore queue generation ? Scheduler ????????

**??**: 2026-03-20
**??**: ???

### ????
- ????????????????????? UI ??????? flush ??? scheduler ???????????
- ???????? queue ? `clear()/flush()` ???????????? scheduler ? `Seeking / Flushing / Stopping / Ended` ????? decode/render ???

### ????
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
```

### ????
1. ??????? packet/frame ? serial ????????? queue ?????????????????????????????????? `push()/pop()` ? flush ?????????
2. ??????? `Ended` ???????? scheduler ??????? `RunState / PipelinePhase / accepted timeline serial`??? seek/flush/stopping ?????????????
3. ?????????????????? `Ended` ???????? queue generation ? scheduler ???????????????????????

### ????
- ? `ThreadSafeQueue` ? `FrameQueue` ?? generation??? `clear()/flush()` ?? generation????? `push()/pop()` ????? generation ???????????????
- ? `scheduler.h` ??? `SchedulerControlSnapshot`?? `Scheduler` ???? `run_state / pipeline_phase / accepted_timeline_serial`?
- `PlayerCore` ?? `makeSchedulerControlSnapshot()` ??????? `setControlSnapshotProvider()` ??? scheduler?
- ??/?? decode loop ????????????????render loop ????????????????? accepted serial?????????
- `DiagnosticsSnapshot`??? diagnostics ????????? queue generation ????????? flush ?????
- ?????? `docs/analysis/PLAYERCORE_DAY19_QUEUE_GENERATION_AND_SCHEDULER_CONTROL_REDESIGN.md`?

### ??????
- Debug ???????? `0 warnings / 0 errors`?
- queue flush ?????? generation ????????????????
- scheduler ?????????????? `Seeking / Flushing / Stopping / Ended` ?????????????
- diagnostics / ???????????? packet/frame queue generation?

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

## 问题 84: PlayerCore 副作用集中化与 runtime failure/recovery policy 收口

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- timeline serial 与 queue generation 已经把 seek/flush 的数据边界硬化，但 `PlayerCore` 入口里仍混着线程、设备、队列和时钟副作用。
- `SchedulerControlSnapshot` 还只有最小态，scheduler 对 `clock_source`、audio-master 与 ended policy 的依赖仍然没有完全结构化。
- runtime fatal 点如果继续分散处理，会把 stopping / session release / error emit 再次拆散。

### 日志输出
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 分析记录
1. 这轮的重点不是继续改 timeline serial，而是把“状态迁移之后该做什么动作”收口成统一副作用模型。
2. `deferred stop` 应该保留为异步 stop completion 机制，但不应继续作为旁路业务状态来源。
3. scheduler 已有 `run_state / pipeline_phase / accepted_timeline_serial`，继续扩成 `clock_source + audio master + ended policy` 结构，能避免再回到零散布尔位模式。
4. runtime failure 必须先统一 recovery policy，再逐步扩覆盖点；否则每个新 fatal 点都会复制一遍 stop/release/error 分支。

### 处理结果
- `PlayerCore` 已新增一组统一 side-effects helpers，并把 `play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 的核心副作用迁入这些 helpers。
- `SchedulerControlSnapshot` 已新增 `clock_source`、`audio_output_initialized`、`audio_master_sync_active`、`ended_policy`，scheduler 已接入这些字段。
- 新增 `FailureRecoveryPolicy` 与 `handleRuntimeFailure()`，并把 decode/resample 关键 fatal 点接入统一恢复入口。
- 已新增 analysis：`docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md`。

### 本地验收结果
- Debug 构建通过。
- 这轮未改 UI 层、copy-back、SoftwareSDL 和外部 `PlaybackState`。
- 当前剩余风险已明确收敛到：更完整的 `SchedulerControlSnapshot`、`FailSession` 真正启用时的线程安全边界，以及更细的 ended/audio-master policy。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 85: PlayerCore 剩余风险收敛：Scheduler 终版策略、FailSession 实化与 serial/generation 观测强化

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- 用户要求继续消化剩余风险：`SchedulerControlSnapshot` 终版策略化、`FailSession` 实用化、以及 generation 与 serial 职责边界强化。

### 日志输出
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 分析记录
1. scheduler 需要明确消费策略枚举，而不是继续依赖隐式布尔组合。
2. `FailSession` 需要覆盖会话级不可恢复错误，并确保 stop/join 在 worker 上下文可安全执行。
3. generation 只解决容器边界中断；serial 才是旧时间线硬失效主判定，需通过 stale drop 计数直观观察。

### 处理结果
- 扩展 `SchedulerControlSnapshot`：新增 `clock_policy`、`audio_master_policy`、`audio_buffered_seconds`，并扩展 ended policy。
- `Scheduler` render wait 改为策略驱动；`Scheduler::stop()` 补 self-join 保护。
- `handleRuntimeFailure()` 收口增强并补统计；关键不可恢复错误点切到 `FailSession`。
- 新增 stale serial drop 计数并接入 diagnostics 与检查命令输出。
- 新增 analysis：`docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md`。

### 本地验收结果
- Debug 构建通过。
- 本轮未改 UI 层、外部 `PlaybackState`、copy-back、SoftwareSDL。

### 修改文件
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 问题 86: serial/failsession 回归检查补齐（连续 seek、暂停态 seek、close-reopen）

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- 用户要求继续按 WORKFLOW 推进，优先补齐连续 seek / 暂停态 seek / close-reopen 的 CLI 回归检查。
- 目标是输出机器可读 `key=value` 和 `result=PASS/FAIL`，并覆盖 stale/serial/FailSession 约束。

### 日志输出
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Checks:
build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4 -> PASS
```

### 分析记录
1. 先补 `DiagnosticsSnapshot` 的非法迁移计数，避免 CLI 只能靠日志文本判定。
2. 连续 seek 场景中可观察到 `illegal_pipeline_transitions` 在 `Draining -> Seeking` 有增量，但当前未伴随 `FailSession`。
3. 判定逻辑优先保证 `FailSession` 路径不出现非法跳转（`fail_session_transition_ok`），并保留非法迁移计数用于后续专项收敛。

### 处理结果
- `PlayerCore`：
  - 新增并导出 `illegal_session_transitions / illegal_run_transitions / illegal_pipeline_transitions`
  - 非法迁移分支计数 + diagnostics reset + diagnostics 日志输出
- CLI：
  - 新增 `--seek-burst-serial-check`
  - 新增 `--paused-seek-serial-check`
  - 新增 `--close-reopen-serial-check`
  - 现有 `--performance-log-check`、`--software-video-decode-check` 同步导出非法迁移计数
- 文档：
  - 新增 `docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md`

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 问题 87: serial/failsession 回归增加一键聚合 gate（降低漏跑风险）

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- 三个 serial/failsession 专项探针已经可用，但执行仍依赖人工串行调用。
- 在持续迭代中，人工串行方式容易漏跑某一项，导致回归 gate 不稳定。

### 日志输出
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Check:
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.pass_count=3
serial-failsession-regression-check.total_count=3
serial-failsession-regression-check.result=PASS
```

### 分析记录
1. 探针本身已具备统一的 `key=value` 判定字段，缺口在“执行入口聚合”。
2. 若不提供聚合入口，调用方需重复维护三条命令与参数，回归脚本易分叉。
3. 先在 CLI 层补聚合 gate，可以在不改 `PlayerCore` 核心逻辑的前提下降低执行残余风险。

### 处理结果
- `src/main.cpp` 新增：
  - `runSerialFailSessionRegressionCheck(...)`
  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 聚合命令内部顺序执行：
  - `runSeekBurstSerialCheck`
  - `runPausedSeekSerialCheck`
  - `runCloseReopenSerialCheck`
- 新增聚合结果字段：
  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`
- 新增分析文档：`docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md`

### 修改文件
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 问题 88: 强制 FailSession 回归探针与 codec 锁重入崩溃修复

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- `FailSession` 仍主要依赖真实错误触发，回归覆盖不稳定。
- 在补强制探针时，触发了解码线程进入 `FailSession` 的 codec 锁重入异常（`device or resource busy`）。

### 日志输出
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Forced FailSession check:
build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200
forced-failsession-check.runtime_failure_stop_requests=1
forced-failsession-check.runtime_failure_fail_sessions=1
forced-failsession-check.illegal_transition_total=0
forced-failsession-check.result=PASS

Serial aggregate check:
build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4
serial-failsession-regression-check.result=PASS
```

### 分析记录
1. 先引入可控注入点，确保 `FailSession` 路径可被稳定触发。
2. 强制触发后立即暴露同线程 codec 锁重入问题，说明该路径此前未被充分压测。
3. 修复锁语义后，`FailSession` 路径可稳定落到 `session=Failed / playback=Stopped`，非法迁移计数保持 0。

### 处理结果
- `PlayerCore::decodeVideoFrame` 增加 `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE` 测试注入逻辑。
- `main.cpp` 新增 `--forced-failsession-check <media_file> [sample_ms]`，输出完整机器可读 gate 字段。
- `video_codec_mutex_`、`audio_codec_mutex_` 改为 `std::recursive_mutex`，并同步更新 `decodeVideoFrame/decodeAudioFrame` 锁类型。
- 新增分析文档：`docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md`。

### 修改文件
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 问题 89: run_all_checks 接入 forced-failsession 一键 gate

**日期**: 2026-03-20
**状态**: 已解决

### 问题描述
- 日常一键回归脚本 `tools/run_all_checks.ps1` 未默认执行 `--forced-failsession-check`，FailSession 路径覆盖依赖人工单独执行。

### 日志输出
```text
Command:
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 \
  -ExecutablePath "build/Debug/modern-video-player.exe" \
  -ProbeFile "juren-30s.mp4" \
  -ForcedFailSessionSampleMs 2200

Result:
[1/3] Probe exit code: 0
[2/3] Forced FailSession exit code: 0
[3/3] Regression exit code: 0
Script exit code: 0
```

### 分析记录
1. `--forced-failsession-check` 已可稳定 PASS，应提升到批处理默认流程。
2. 一键脚本需要把 forced path 设为硬 gate，而不是信息性步骤。
3. 参数设计应保持向后兼容，默认复用 `ProbeFile` 最小化迁移成本。

### 处理结果
- `tools/run_all_checks.ps1` 新增参数：
  - `ForcedFailSessionFile`
  - `ForcedFailSessionSampleMs`
- 执行顺序由两步改为三步：
  - `[1/3]` probe
  - `[2/3]` forced-failsession gate
  - `[3/3]` format regression
- gate 逻辑：
  - probe 非零 -> 直接退出；
  - forced-failsession 非零 -> 直接退出并跳过 regression。
- 新增分析文档：`docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md`。

### 修改文件
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
