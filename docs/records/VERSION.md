# 项目版本记录

### 2026-03-24 更新：OpenGL 诊断快照、libass 差距清单与显示级 HDR 设计收敛
- `OpenGLVideoRenderer` 新增 `OpenGLDiagnosticsSnapshot` 与 `OpenGLVideoRenderer::probeSystemDiagnostics()`，并提供 `--opengl-diagnostics` 机器可读输出。
- OpenGL native interop 启动期策略已从零散判断收敛为 `hard blocker + quirk rule + env override` 三层决策，`force` 只允许覆盖 quirk，不覆盖真正的 hard blocker。
- 已新增 `docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`，把当前 OpenGL 与 `libass/mpv` 的差距拆到 `shaping / karaoke / vector / font fallback / layout` 级别。
- 已新增 `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`，明确 Windows 下显示级 HDR 输出应走 `DXGI swapchain + HDR metadata` 桥接，而不是继续把 tone-map 当成完整 HDR 方案。
- Release 验证通过：`--opengl-diagnostics=PASS`、`MVP_OPENGL_NATIVE_INTEROP=disable --opengl-diagnostics=PASS`、OpenGL `performance-log-check=PASS`、`subtitle-sync-check=PASS`、`delay-adjust-check=PASS`。

### 2026-03-24 更新：OpenGL M1 成熟化收敛
- `OpenGLVideoRenderer` 已从仅能播放的 `M0` 过渡后端，推进到带字幕、启动期 quirk 决策和基础 HDR/色彩处理的 `M1` 级别；当前仍保持 `opt-in`，不替代默认 `D3D11` 后端。
- `ASS/SSA` 现阶段采用 `DirectWrite + D2D offscreen -> OpenGL texture` 路径，支持多 `SubtitleItem` 排序、item/run 分段样式、定位对齐、描边、阴影、背景框与填充；仍未达到 `libass/mpv` 的完整高级特效覆盖。
- OpenGL 启动期新增 `MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`，并对 `Microsoft / GDI Generic` 软件 GL 环境直接禁用 native interop；同时输出 `[diag:opengl-native]` 适配器、驱动、decoder profile 与最终规则日志。
- GLSL/HLSL 色彩链路补齐 `BT.601 / BT.709 / BT.2020` 矩阵选择、`PQ / HLG` 检测、基础 SDR tone-map 和 `BT.2020 -> BT.709` gamut mapping，并输出 `[diag:opengl-color]`。
- Release 验证通过：`OpenGL performance-log-check=PASS`、`subtitle-sync-check=PASS`、`delay-adjust-check=PASS`。

### 2026-03-24 更新：OpenGL M0 渲染链路落地
- `OpenGLVideoRenderer` 已从 stub 变为可用后端，支持 `YUV420P / NV12` 直接帧、GLSL 120 色彩转换与基础键盘控制。
- 当前策略保持保守：默认后端仍为 `D3D11` / `SoftwareSDL` 回退，`OpenGL` 需通过 `MVP_RENDERER_BACKEND=opengl` 显式启用。
- 本地 `Release` 验证：`--performance-log-check .\juren-30s.mp4 2000` 输出 `renderer_backend=OpenGL`、`result=PASS`。
- 当前 `OpenGL` 路径定位为 M0，不等同于成熟播放器 GPU 后端；字幕/OSD 叠加、原生硬件表面互操作、进阶色彩管理仍未补齐。



鏈枃妗ｈ褰曢」鐩殑鐗堟湰鍙樻洿鍘嗗彶鍜屽綋鍓嶇姸鎬併€?


## 褰撳墠鐗堟湰淇℃伅



### 绗笁鏂瑰簱鐗堟湰



| 缁勪欢 | 鐗堟湰 | 璇存槑 |

|------|------|------|

| FFmpeg | 8.0.1 | 澶氬獟浣撳鐞嗘鏋?|

| SDL2 | 2.30.11 | 澶氬獟浣撳簱锛堟樉绀哄拰闊抽锛?|

| Quill | 6.0.0 | 寮傛 Console + Rotating 鏃ュ織 |

| CMake | 3.15+ | 鏋勫缓绯荤粺 |

| C++ 鏍囧噯 | C++17 | 缂栬瘧鏍囧噯 |



**娉ㄦ剰**: Quill 鐢?`external/quill` 鎻愪緵锛屾垨閫氳繃璁剧疆 `QUILL_ROOT` 鎸囧悜绯荤粺瀹夎鐨?v6.0.0+ 鐗堟湰銆?


### 椤圭洰鐗堟湰



- **椤圭洰鐗堟湰**: 1.0.0

- **褰撳墠鍙戝竷鍊欓€?*: 1.0.0-rc1

- **鍙戝竷鐘舵€?*: 鍙彂甯?RC锛屼笉寤鸿鐩存帴 GA

- **鏋勫缓绫诲瀷**: Release / Debug

- **鏀寔骞冲彴**: Windows, Linux, macOS

- **绋嬪簭鏄剧ず鐗堟湰**: 1.0.0-rc1锛坄--version`锛?
- **Windows 鏂囦欢鐗堟湰**: 1.0.0.0

- **鍙戝竷鍖呮枃浠跺悕**: modern-video-player-1.0.0-rc1-windows-x64.zip



### 2026-03-23 鏇存柊锛歊C 鐗堟湰鍏冩暟鎹€丷elease 椤垫鏂囦笌鍖呯増鏈爣璇嗚ˉ榻?
- 宸叉柊澧炲彲鐩存帴璐村埌 GitHub Release 椤电殑姝ｆ枃鏂囦欢锛歚docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`銆?
- `CMake` 鐜板凡寮曞叆缁熶竴鐗堟湰婧愶細鍩虹鐗堟湰 `1.0.0`锛岄鍙戝竷鍚庣紑 `rc1`锛岀粺涓€鐢熸垚绋嬪簭鍐呴儴鐗堟湰涓层€乄indows 璧勬簮鐗堟湰浠ュ強鎵撳寘鏂囦欢鍚嶃€?
- `main` 鏂板 `--version`锛涘綋鍓嶅疄娴嬭緭鍑轰负锛歚Modern Video Player 1.0.0-rc1`銆?
- Windows `VERSIONINFO` 宸茶ˉ榻愶紱褰撳墠瀹炴祴缁撴灉涓猴細
  - `FileVersion=1.0.0.0`
  - `ProductVersion=1.0.0-rc1`
  - `ProductName=Modern Video Player`

- 鏂板 `CPack ZIP` 鎵撳寘瑙勫垯锛涘綋鍓嶅疄娴嬩骇鐗╀负锛歚build/modern-video-player-1.0.0-rc1-windows-x64.zip`銆?
- 褰撳墠鍙戝竷鍖呭凡纭鍖呭惈 `modern-video-player.exe`銆佷緷璧?DLL銆乣plugins/sample_logger_plugin.dll` 涓?`RELEASE_NOTES.md`锛屼笖鏈墦鍏ユ湰鍦?`config/player_settings.ini`銆?
- `HTTP` 涓嬭浇閾捐矾鐨?`user_agent` 鐜板凡缁熶竴鏇存柊涓?`modern-video-player/1.0.0-rc1`锛屼究浜庡悗缁?RC 闃舵鏃ュ織褰掓。涓庣嚎涓婃帓闅溿€?
### 2026-03-23 鏇存柊锛?.0.0-rc1 鍙戝竷鍑嗗瀹屾垚

- 褰撳墠缁撹宸叉敹鍙ｄ负锛歚鍙墦 v1.0.0-rc1 鏍囩`锛屼絾涓嶅缓璁洿鎺ュ彂甯冩寮忕増 `1.0.0`銆?
- 鏈疆鍩轰簬 `Release` 鏋勫缓閲嶆柊鎵ц骞堕€氳繃锛?  - `tools/run_all_checks.ps1`
  - `--d3d11-diagnostics`
  - `--performance-log-check .\juren-30s.mp4 2000`
  - `--long-playback-check .\juren-30s.mp4 10000`
  - `--serial-failsession-regression-check .\juren-30s.mp4`

- 鏈€鏂版牸寮忓洖褰掓姤鍛?`docs/reports/FORMAT_REGRESSION_20260323_224615.md` 姹囨€荤粨鏋滀负锛歚17 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`銆?
- 宸叉柊澧?RC 姹囨€绘枃妗ｏ細`docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`锛岀粺涓€鏀跺彛鍙戝竷娓呭崟銆佸凡鐭ラ棶棰樸€佸彂甯冭鏄庡拰浠婂ぉ鐨勯獙璇佽瘉鎹€?
- 褰撳墠 RC 浠嶄繚鐣欎袱绫绘槑纭闄╋細
  - `闂 79` 瀵瑰簲鐨?software video decode 杩愯鎬佽矾寰勪粛鏈畬鍏ㄦ敹鍙?  - D3D11 driver / adapter quirk blacklist 瑙勫垯浠嶉渶缁х画鎵╁厖
### 2026-03-23 鏇存柊锛欴3D11 decoder profile 鎺㈡祴銆乹uirk blacklist 涓庣嫭绔?diagnostics CLI

- `D3D11VideoRenderer` 鐜板凡杈撳嚭缁撴瀯鍖?`D3D11DiagnosticsSnapshot`锛岀粺涓€鍖呭惈 adapter/driver銆乫eature level銆乣NV12/P010/P016` 鏍煎紡鏀寔銆乨ecoder profiles 涓?native direct 鍚姩鏈熺瓥鐣ャ€?
- 鏂板 decoder profile 鎺㈡祴锛屽綋鍓嶆満鍣ㄦ渶鏂板疄娴嬬粨鏋滀负锛?  - `H.264`锛氭敮鎸?  - `HEVC`锛氭敮鎸?`Main / Main10`
  - `VP9`锛氭敮鎸?`Profile0`锛屼笉鏀寔 `Profile2 10bit`
  - `AV1`锛氬綋鍓嶉€傞厤鍣ㄦ湭鏆撮湶鏀寔 profile

- 鏂板 startup-time quirk / blacklist 鍐崇瓥锛涜嫢鍛戒腑 software adapter銆佺己灏戝叧閿?D3D11 video 鎺ュ彛銆乣NV12` 涓嶆弧瓒崇洿閲囨牱瑕佹眰鎴栧懡涓鍒欒〃锛屽垯浼氬湪鎾斁鍓嶇洿鎺ュ叧闂?native direct銆?
- 棣栫増 blacklist 宸叉樉寮忚鐩?`Microsoft Basic Render Driver`锛涘綋鍓嶆満鍣ㄨ瘖鏂粨鏋滀负 `native_direct.allowed=true`銆乣disable_rule=none`銆?
- `main` 鏂板 `--d3d11-diagnostics`锛屽彲涓€娆℃€ц緭鍑烘満鍣ㄥ彲璇?`key=value` 缁撴灉锛岀敤浜庤嚜鍔ㄥ寲銆佺幇鍦烘帓闅滃拰鍚庣画椹卞姩鍏煎鎬у綊妗ｃ€?### 2026-03-23 鏇存柊锛欴3D11 鍚姩鏈熻兘鍔涙帰娴嬩笌 adapter/driver 璇婃柇鏃ュ織

- `D3D11VideoRenderer` 鍒濆鍖栭樁娈垫柊澧?`[diag:d3d11-init]` 鏃ュ織锛岀洿鎺ヨ緭鍑?adapter銆乨river version銆乫eature level銆佸叧閿?D3D11/DXGI 鎺ュ彛鍙敤鎬с€佹牸寮忔敮鎸佷綅涓?swap chain 鍙傛暟銆?
- 褰撳墠鏈哄櫒鏈€鏂板疄娴嬪凡鑳藉湪鍚姩鏃剁洿鎺ョ湅鍒帮細`adapter="NVIDIA GeForce GTX 1080"`銆乣driver_version=32.0.15.6094`銆乣feature_level=11_1`銆乣device3=true`銆乣video_device=true`銆乣NV12/P010` 鏀寔鎯呭喌锛屼互鍙?`P016{check_failed_hr=-2147467259}` 杩欑被鏍煎紡鑳藉姏宸紓銆?
- 杩欒疆涓嶆敼鍙樼幇鏈夋挱鏀捐矾寰勶紝鍙ˉ鎴愮啛鎾斁鍣ㄩ渶瑕佺殑鍚姩鏈熻瘖鏂笂涓嬫枃锛涢棶棰?90/91 鐨?native direct 涓?fallback 閾捐矾淇濇寔涓嶅彉銆?### 2026-03-23 鏇存柊锛欴3D11VA 鑷畾涔?hw_frames_ctx 鎺ュ叆鍙噰鏍疯В鐮佽〃闈?
- `PlayerCore` 涓嶅啀鍙妸 `D3D11VA` 璁惧鎸傜粰 decoder锛岃€屾槸鍦?`get_format()` 闃舵鏄惧紡鍒涘缓 `hw_frames_ctx`锛屽苟涓?`AVD3D11VAFramesContext::BindFlags` 杩藉姞 `D3D11_BIND_SHADER_RESOURCE`銆?
- 褰撳墠鏈哄櫒澶嶆祴缁撴灉琛ㄦ槑锛岃繖涓€姝ュ凡缁忚 D3D11 鍘熺敓鐩撮噰鏍风湡姝ｆ仮澶嶏細`Configured D3D11VA frames context for direct shader sampling: bind_flags=520`锛屽悓鏃?`video_native_output_frames=62`銆乣video_copy_back_frames=0`銆?
- 闂 90 鐨勮繍琛屾椂 fallback 浠嶇劧淇濈暀锛屼綔涓哄叾浠栭┍鍔?璁惧缁勫悎涓?native direct 澶辫触鏃剁殑鏈€鍚庡厹搴曘€?### 2026-03-23 鏇存柊锛欴3D11 鍘熺敓鐩撮噰鏍疯繍琛屾椂澶辫触鏀逛负鑷姩鍥為€€ copy-back

- `D3D11VideoRenderer` 鏂板杩愯鏃?native direct 鐔旀柇锛氬鏋?`CreateShaderResourceView1` 鏃犳硶涓?D3D11VA 瑙ｇ爜琛ㄩ潰鍒涘缓 Y/UV plane SRV锛屾垨瑙ｇ爜琛ㄩ潰鏍煎紡涓嶆敮鎸佺洿鎺ラ噰鏍凤紝鍒欏叧闂湰娆′細璇濈殑 native direct rendering銆?
- native direct 琚叧闂悗锛宍PlayerCore` 浼氱户缁妸鍚庣画纭В甯ц蛋 copy-back 鍒拌蒋浠跺抚锛屽啀澶嶇敤鐜版湁杞欢绾圭悊涓婁紶閾捐矾鏄剧ず锛岄伩鍏嶇户缁嚭鐜扳€滈煶棰戞甯搞€佽棰戦粦灞忊€濄€?
- `Release / Debug` 瀵?`juren-30s.mp4` 鐨?`--performance-log-check 2000` 宸插娴嬮€氳繃锛涗袱鑰呴兘瀹為檯鍛戒腑 `fallback=copyback-to-software` 鍛婅锛屽苟淇濇寔 `render_frames > 0`銆?### 2026-03-20 鏇存柊锛歅layerCore 鍓╀綑椋庨櫓鏀舵暃锛圫cheduler 缁堢増绛栫暐銆丗ailSession 瀹炲寲銆乻erial/generation 瑙傛祴寮哄寲锛?
- `SchedulerControlSnapshot` 鏂板缁撴瀯鍖栫瓥鐣ュ瓧娈碉細`clock_policy`銆乣audio_master_policy`銆乣audio_buffered_seconds`锛屽苟灏?`ended_policy` 鎵╁睍鍒?`HoldLastFrameNoClockSync`銆?
- `Scheduler` render wait 鏀逛负娑堣垂绛栫暐鍑芥暟锛坄isAudioMasterActive` / `isVideoMasterActive` / `shouldApplyClockSync`锛夛紝鍚屾椂 `Scheduler::stop()` 鏂板 self-join 淇濇姢銆?
- `FailSession` 宸蹭粠棰勭暀鍏ュ彛鎺ㄨ繘鍒板叧閿け璐ョ偣瀹炵敤璺緞锛沗handleRuntimeFailure()` 鍚屾琛ュ叆 stop/session failure 璁℃暟銆?
- diagnostics 鏂板 stale serial 涓㈠純璁℃暟锛屾槑纭?`queue generation` 鍙礋璐ｅ鍣ㄨ竟鐣屼腑鏂紝鑰屾棫鏃堕棿绾跨‖澶辨晥浠嶇敱 item-level serial 涓诲垽瀹氥€?### 2026-03-20 鏇存柊锛歅layerCore 鍓綔鐢ㄩ泦涓寲涓?runtime failure/recovery policy 鏀跺彛

- `SchedulerControlSnapshot` 宸茶ˉ鍏?`clock_source`銆乣audio_output_initialized`銆乣audio_master_sync_active` 涓?`ended_policy`锛宻cheduler 涓嶅啀鍙潬鏈€灏忔€佸拰闆舵暎鎺ㄦ柇娑堣垂涓氬姟璇箟銆?
- `PlayerCore` 宸叉妸 start/resume/pause/stop request/stop completion/seek/session release 鍓綔鐢ㄧ粺涓€鎶藉埌 `apply*SideEffects()`锛宍play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 鍏ュ彛涓嶅啀鐩存帴鍫嗙嚎绋嬪拰璁惧鍔ㄤ綔銆?
- `deferred stop` 宸插苟鍥炵粺涓€ stopping 璺緞锛況untime fatal 鐜伴€氳繃 `FailureRecoveryPolicy + handleRuntimeFailure()` 鏀跺彛鍒扮粺涓€鎭㈠鍏ュ彛锛屽悗缁彲浠ョ户缁墿瑕嗙洊鑼冨洿鑰屼笉鍐嶅鍒舵仮澶嶉€昏緫銆?
- 鏈疆涓嶆敼澶栭儴 `PlaybackState`銆乁I銆乧opy-back 鎴?SoftwareSDL锛屽彧缁х画鏀剁揣鍐呮牳鐘舵€佹満涓?worker/device 鍔ㄤ綔鐨勮竟鐣屻€?

### 2026-03-20 鏇存柊锛歅layerCore seek/flush timeline serial 鍖栫浜岄樁娈?


- `PlayerCore` 宸插紩鍏?`timeline_serial / pending_seek_serial`锛宻eek 涓嶅啀鍙潬 flush 鍜屽竷灏斾綅鍒囨崲鏃堕棿绾裤€?
- demux packet銆乨ecoded `VideoFrame / AudioFrame`銆乺ender 鍜?audio consumer 閾捐矾閮藉凡鎺ヤ笂 serial 纭け鏁堝垽瀹氥€?
- `flush` 缁х画淇濈暀锛屼絾璇箟宸蹭粠鈥滃敮涓€澶辨晥鏈哄埗鈥濋檷绾т负鈥滆緟鍔╂竻鎵€濓紱鐪熸鐨勬棫鏃堕棿绾胯竟鐣岀敱 serial 鎻愪緵銆?
- diagnostics 涓庝笓椤规鏌ュ懡浠ゅ凡鏂板 `timeline_serial / pending_seek_serial` 杈撳嚭锛屼究浜庡悗缁户缁仛 EOF/Ended 鍜?Scheduler 濂戠害鍗囩骇銆?




### 2026-03-19 鏇存柊锛氳ˉ榻?records / analysis 鏂囨。涓€鑷存€?


- `CHANGELOG` 鐨勯棶棰樻€昏〃宸茶ˉ涓?`闂 78` 绱㈠紩锛岄伩鍏嶄粖澶╄繖缁?software decode 璇婃柇璁板綍鍦ㄦ€昏〃閲屽嚭鐜?`77 -> 79` 璺冲彿銆?
- 宸插洖濉?`闂 69` 瀵瑰簲鐨?analysis 鏂囨。锛歚docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`銆?
- 鏈疆涓嶆秹鍙婁唬鐮侀€昏緫鍙樻洿锛屽彧鏀跺彛浠婂ぉ鎻愪氦鍚庣殑鏂囨。绱㈠紩鍜屽垎鏋愭矇娣€涓€鑷存€с€?


### 2026-03-19 鏇存柊锛歅layerCore 杩愯鎬?software send probe 瀵圭収宸叉妸 software decode blocker 鏀舵暃鍒?runtime context 宸紓

- `--software-video-send-probe` 鐜板凡琛ラ綈 `pre_send_receive_ret`銆乣packet queue round-trip` 涓?`read-ahead` 瀵圭収锛沗juren-30s.mp4` 鏈€鏂板疄娴嬩负 `packet_queue_push_ok=true / packet_queue_pop_ok=true / read_ahead_packets=512 / send_ret=0 / receive_got_frame=true / result=PASS`銆?
- `--software-video-decode-check` 鏂板 `audio_probe_mode`锛屽苟鏀寔 `MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO=1` 鐨?video-only 瀵圭収锛涙渶鏂板疄娴嬫樉绀哄嵆浣?`clock_source=Video`锛屼粛鐒舵槸 `video_packet_dequeue_count=1 / video_send_packet_ok=0 / decode_video_ok=0 / render_frames=0 / result=FAIL`銆?
- `PlayerCore::decodeVideoFrame()` 鏂板浠呯幆澧冨彉閲忓紑鍚殑 `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD` 璇婃柇璺緞锛涘湪 `video-only` 涓嬪疄娴嬩粛鍑虹幇 `Offthread software video send_packet probe timed out after 500ms`銆?
- 缁撹鏇存柊锛氬綋鍓?blocker 宸叉帓闄?FFmpeg software decoder 鏈綋銆乣receive->send` 椤哄簭銆乸acket queue 浜ゆ帴銆乨emux read-ahead銆侀煶棰戦摼浠ュ強褰撳墠 decode 绾跨▼鏈韩锛涘悗缁簲鐩存帴瀵规瘮 `PlayerCore::initDecoders()` 浜у嚭鐨?software codec ctx 涓庣嫭绔?probe 鐨勫瓧娈靛樊寮傘€?
### 2026-03-19 鏇存柊锛氬凡琛?software decode 鏈€灏?send/dequeue 璁℃暟锛宐locker 鏀舵暃鍒扳€滈涓?packet send 鏈畬鎴愯繑鍥炩€?


- `PlayerCore` 鏂板涓夐」鏈€灏忚瘖鏂細

  - `video_packet_dequeue_count`

  - `video_send_packet_ok`

  - `video_send_packet_last_ret`

- 瀹冧滑宸茬粡鎺ュ叆 `DiagnosticsSnapshot`銆佷綆棰戦摼璺棩蹇椼€乣--performance-log-check` 涓?`--software-video-decode-check`銆?
- 姝ｅ父涓婚摼瀵圭収鏍锋湰 `juren-30s.mp4` 鐨勬渶鏂扮粨鏋滀负锛?
  - `video_packet_dequeue_count=57`

  - `video_send_packet_ok=57`

  - `video_send_packet_last_ret=0`

  - `result=PASS`

- software decode 鏍锋湰鐨勬渶鏂颁綆棰戣瘖鏂负锛?
  - `v_pkt_deq=1`

  - `v_send_ok=0`

  - `v_send_ret=-2147483648`

- 缁撹鏇存柊锛歴oftware decode 绾跨▼宸茬粡鍙栧埌棣栦釜瑙嗛鍖咃紝浣嗛涓?`avcodec_send_packet()` 鍒拌瘖鏂偣浠嶆湭褰㈡垚鎴愬姛杩斿洖锛涘綋鍓?blocker 宸茶繘涓€姝ユ敹鏁涘埌 decoder 棣栧寘閫佸叆闃舵銆?




### 2026-03-19 鏇存柊锛歴oftware decode 淇濆畧绾跨▼閰嶇疆澶嶈窇鍚庝粛鍗″湪棣栦釜瑙嗛鍖咃紝`SdlVideoRenderer` 娉ㄩ噴涔辩爜宸叉竻鐞?


- `Video decoder threading` 鐜板凡纭 software path 杩愯鍦?`thread_count=1 / thread_type=none`锛岃鏄庤繖杞鏍哥‘瀹炲懡涓簡鈥滀繚瀹堢嚎绋嬮厤缃€濈洰鏍囩姸鎬併€?
- 閲嶆柊鎵ц `--software-video-decode-check` 鍚庯紝缁撴灉浠嶇劧鏄細`renderer_backend=SoftwareSDL`銆乣decoder_backend=Software`銆乣decode_video_ok=0`銆乣scheduler_video_decoded_frames=0`銆乣render_frames=0`銆乣video_frame_queue_peak_size=0`銆乣result=FAIL`銆?
- 缁撳悎鎺у埗鍙拌瘖鏂棩蹇楅噷 `demux(v=163) / pkt_q(v=162)` 涓庝粎鍑虹幇 `Video decode first send_packet start` 鐨勭幇璞★紝鍙互鎺ㄦ柇 software decode 绾跨▼褰撳墠鏇村儚鏄湪棣栦釜瑙嗛鍖呮彁浜ら樁娈靛悗鍋滀綇锛岃€屼笉鏄崱鍦?copy-back / swscale / display copy銆?
- `src/render/sdl_video_renderer.cpp` 宸茶ˉ淇?9 澶勫嚱鏁板ご娉ㄩ噴涔辩爜锛涙湰杞啀娆℃壂鎻?`src/`銆乣include/` 鐨?`///` 涓?`//` 娉ㄩ噴琛岋紝鏈啀鍙戠幇鏂扮殑鍙枒涔辩爜鍛戒腑銆?




### 2026-03-19 鏇存柊锛氭柊澧?`--software-video-decode-check`锛屾妸 software video decode blocker 鍗曠嫭閽夋



- 鏂板 `--software-video-decode-check <media_file> [sample_ms]`锛屼笓闂ㄥ洖绛斺€渟oftware video decode 鏄惁鐪熺殑浜у抚鈥濓紝涓嶅啀缁х画澶嶇敤鍙兘璇佹槑鈥滀細璇濆凡鍚姩鈥濈殑 `session-check`銆?
- 鍛戒护鍐呴儴寮哄埗 `MVP_RENDERER_BACKEND=software`銆乣SDL_AUDIODRIVER=dummy` 鍜?`preferHardwareDecode=false`锛屼繚璇侀獙璇侀摼璺ǔ瀹氬懡涓?`SoftwareSDL + Software decode`銆?
- 鍛戒护閫氳繃鏉′欢琚敹绱т负鈥滅湡瀹炰骇甯р€濊€屼笉鏄€滃彧鎵撳紑鎴愬姛鈥濓細蹇呴』鍚屾椂婊¤冻 `decode_video_ok > 0`銆乣scheduler_video_decoded_frames > 0`銆乣render_frames > 0`銆乣video_frame_queue_peak_size > 0`锛屽苟纭 `video_copy_back_frames == 0`銆?
- 鍛戒护鏈韩鏀规垚 probe 寮忕‖閫€鍑猴紝閬垮厤琚綋鍓?soft decode blocker 涓嬬殑 `stop/close` 鎸傛銆?
- 鏈満鏈€鏂伴獙璇佺粨鏋滐細

  - `open_ok=true / entered_playback_loop=true / renderer_backend=SoftwareSDL / decoder_backend=Software`

  - `demux_video_packets=163 / demux_queue_drop_packets=0`

  - `decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0 / video_frame_queue_peak_size=0`

  - `video_copy_back_frames=0 / video_swscale_frames=0 / result=FAIL`

- 缁撹鏇存柊锛歜locker 宸蹭粠鈥渇allback 鏄惁杩樺湪 copy-back鈥濊繘涓€姝ユ敹鏁涗负鈥滃綋鍓嶅伐绋嬬殑杞欢瑙嗛瑙ｇ爜鎺ュ叆閾炬牴鏈病鏈夊舰鎴愯棰戝抚浜у嚭鈥濓紱鍦ㄥ畠淇ソ涔嬪墠锛屼笉搴旈噸鏂伴粯璁ゅ惎鐢?`software-first`銆?




### 2026-03-19 鏇存柊锛氭挙鍥?`SoftwareSDL` automatic software-first锛屼繚鐣?copy-back fallback 骞惰ˉ杞В闃诲璇婃柇



- 鏈疆楠岃瘉纭锛氳櫧鐒垛€渟ystem-memory renderer 浼樺厛閬垮厤 copy-back鈥濊繖涓柟鍚戜笌鎴愮啛鎾斁鍣ㄦ€濊矾涓€鑷达紝浣嗗綋鍓嶅伐绋嬬殑 FFmpeg 杞欢瑙嗛瑙ｇ爜鎺ュ叆浠嶄笉绋冲畾锛屼笉鑳界洿鎺ユ妸 `SoftwareSDL` 榛樿鍒囧埌 `software-first`銆?
- 宸叉挙鍥炶嚜鍔?renderer-aware `software-first` decoder 閲嶆柊鎺掑簭锛屾仮澶?`D3D11VA -> Software` 榛樿椤哄簭锛岄伩鍏嶆妸 fallback 涓绘祦绋嬪甫杩?0 甯ц緭鍑哄洖褰掋€?
- 宸茶ˉ鍏呰蒋浠惰棰戣В鐮佷綆棰戣瘖鏂細棣栦釜 `send_packet` 鎺㈤拡銆丗Fmpeg 閿欒鐮佸瓧绗︿覆銆乻tall 涓婁笅鏂囨棩蹇楋紝鍚庣画鍙户缁崟鐙畾浣嶈蒋浠惰棰戣В鐮?blocker銆?
- 褰撳墠閲嶆柊楠岃瘉缁撴灉锛?
  - 榛樿 `D3D11 + D3D11VA` 涓婚摼浠嶄繚鎸?`video_copy_back_ratio_percent=0 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - `SoftwareSDL` fallback 宸叉仮澶嶄负 `D3D11VA copy-back` 璺緞锛屽綋鍓嶆牱鏈疄娴?`video_copy_back_ratio_percent=33.5958 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - 寮哄埗 `D3D11 + Software decode` 鏃朵粛鍙鐜拌蒋浠惰棰戣В鐮侀摼闃诲锛屽洜姝?`software-first` 鏆備笉閫傚悎閲嶆柊榛樿鍚敤





### 2026-03-19 鏇存柊锛欰udio-master lateness 鏀剁揣涓?SoftwareSDL 鍑忔嫹璐濇湁闄愰噸鏋?


- `IVideoRenderer` 鐜板凡鏀寔 `supportsDirectFrameFormat()`锛沗SdlVideoRenderer` 浼氱洿鎺ュ０鏄庢敮鎸?`YUV420P / NV12`銆?
- `PlayerCore::prepareVideoOutputFrame()` 鍦ㄦ棤瑙嗛婊ら暅鏃跺厑璁?copy-back 鍚庣殑杞欢甯х洿鎺ヤ氦缁?`SoftwareSDL`锛屼笉鍐嶅己鍒?`swscale -> YUV420P`銆?
- `Display` 鐜板凡鏀寔 `SDL_PIXELFORMAT_NV12 + SDL_UpdateNVTexture()`锛屽苟浼樺厛鎸佹湁 `AVFrame` 寮曠敤锛涙 stride 鐨?`YUV420P/NV12` 璺緞鍙伩鍏嶆樉绀哄眰娣辨嫹璐濄€?
- `Scheduler::pumpRenderOnce()` 鐨?`Audio` master 閫昏緫宸叉敼鎴愬垎娈电瓑寰呫€佸姩鎬?late-drop 闃堝€煎拰鏈€灏?sleep 閲忓瓙锛岄伩鍏嶁€滃彧鐫′竴娆″氨鎻愬墠 render鈥濅互鍙婁吉蹇欑瓑銆?
- 鏈満楠岃瘉缁撴灉锛?
  - 榛樿 `D3D11 + D3D11VA` 涓婚摼浠嶄负 `video_copy_back_ratio_percent=0 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - 寮哄埗 `SoftwareSDL` 鍚庯紝`video_swscale_ratio_percent=0`銆乣display_copy_ratio_percent=0`锛岀儹鐐瑰凡鏀舵暃鍒?`video_copy_back_ratio_percent=46.4067`

  - `SDL_AUDIODRIVER=dummy` 涓?`Audio` master 璺緞宸茶窇閫氾紝`scheduler_wait_events=274`锛屼笉鍐嶅嚭鐜板紓甯搁珮棰戝繖绛?


### 2026-03-19 鏇存柊锛歋oftwareSDL 鎷疯礉閾捐矾閲忓寲銆丼cheduler 閲嶅惎棰勭畻涓?renderer override



- `Display::copyFrameData()` 鐜板凡鍏峰 `frames / bytes / time_us_total / time_us_max` 缁熻锛屽苟閫氳繃 `RendererDiagnostics + DiagnosticsSnapshot + --performance-log-check` 杈撳嚭杞欢鏄剧ず閾剧殑鐪熷疄鎴愭湰銆?
- `Scheduler` worker 閲嶅惎绛栫暐宸蹭粠鍥哄畾娆℃暟鏀规垚鈥?0s 绐楀彛鍐呮渶澶?4 娆?+ 100ms 鍐峰嵈鈥濓紝骞舵柊澧?`scheduler_*_restart_limit_hits` 璇婃柇銆?
- `RendererFactory` 鏂板 `MVP_RENDERER_BACKEND` override锛屽苟鍦?Windows 涓嬫敮鎸?`MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`锛宍--renderer-fallback-check` 褰撳墠宸叉仮澶嶉€氳繃銆?
- 鏈満 4K60 寮哄埗 `SoftwareSDL` 閲囨牱鏄剧ず锛歚video_copy_back_ratio_percent=30.1018`銆乣video_swscale_ratio_percent=18.6623`銆乣display_copy_ratio_percent=21.8407`锛涜鏄庤蒋浠跺洖閫€閾惧凡缁忔槸 copy-back銆乻wscale銆乨isplay memcpy 鐨勫彔鍔犵儹鐐广€?
- 榛樿 `D3D11 + D3D11VA` 涓婚摼閲嶆柊楠岃瘉鍚庝粛淇濇寔 `video_copy_back_ratio_percent=0`銆乣video_swscale_ratio_percent=0`銆乣display_copy_ratio_percent=0`锛寊ero-copy 缁撹涓嶅彉銆?


### 2026-03-19 鏇存柊锛氶珮鐮佺巼/4K 闃熷垪瀹归噺銆佽嚜閫傚簲鑺傛祦涓?copy-back 璇婃柇澧炲己



- `FrameQueue` 宸叉柊澧?`peak_size / push_timeout_count` 缁熻锛宍DiagnosticsSnapshot` 涓?`--performance-log-check` 浼氬悓姝ヨ緭鍑?frame queue 鐨?`capacity / peak / timeout`銆?
- `PlayerCore::open()` 鐜板湪鎸夊獟浣撳睘鎬ч厤缃棰?闊抽 frame queue 瀹归噺锛屽苟鍦?`D3D11VA` 鎵撳紑鍓嶈缃?`extra_hw_frames`锛岄伩鍏?4K native path 鎵撶垎 FFmpeg 鐨勭‖浠跺抚姹犮€?
- `Scheduler` 宸叉妸 video/audio 鑳屽帇鏀逛负杩熸粸闃堝€硷紝骞舵柊澧?`video/audio_backpressure_wait_ms` 缁熻锛沗pumpRenderOnce()` 鍚屾椂淇浜?`Video` master 鐨?wall-clock pacing 鍜?late-frame catch-up銆?
- 鏈€鏂?4K 鎬ц兘閲囨牱鏄剧ず锛歚renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣video_native_output_frames=101`銆乣video_copy_back_frames=0`銆乣video_swscale_frames=0`锛岃鏄庡綋鍓嶄富閾句粛浠?native zero-copy 涓轰富锛屼笉鏄?copy-back 鐑偣銆?
- 宸查噸鏂伴獙璇侊細`MSBuild`銆乣--performance-log-check`銆乣--high-bitrate-check`銆乣--4k-playback-check`銆乣--long-playback-check` 褰撳墠鍧囬€氳繃銆?


### 2026-03-19 鏇存柊锛?K backend session 瀛愯繘绋嬮€€鍑鸿矾寰勪慨澶?


- `--windows-backend-session-check` 宸蹭粠鈥滃鐢ㄥ父瑙勬挱鏀惧櫒閫€鍑烘敹灏锯€濇敼涓衡€滀竴娆℃€?probe 瀛愯繘绋嬧€濊矾寰勶細鎵撳嵃缁撴瀯鍖栫粨鏋滃悗鏄惧紡 flush锛屽苟鍦?Windows 涓嬬洿鎺?`TerminateProcess(GetCurrentProcess(), code)` 閫€鍑恒€?
- 杩欐淇閽堝鐨勬槸鍥炲綊 harness锛屼笉鏄富鎾斁閾捐繍琛屾椂閫昏緫锛涚洰鏍囨槸娑堥櫎 `hard` 妯″紡鎵撳嵃 PASS 鍚庤秴鏃躲€乣soft` 妯″紡鎵撳嵃 PASS 鍚庡紓甯搁€€鍑虹殑娈嬬暀澶辫触銆?
- 宸查噸鏂伴獙璇侊細`hard/soft --windows-backend-session-check` 閫€鍑虹爜鍧囦负 `0`锛宍--windows-backend-check` 涓?`--4k-playback-check` 褰撳墠鍧囧凡鎭㈠閫氳繃銆?


### 2026-03-19 鏇存柊锛氶煶棰戣澶囧け璐ユ椂鐨勮棰?only闄嶇骇涓庡洖褰掗棬绂佺籂鍋?


- `PlayerCore::open()` 鐜板湪鎶婇煶棰戣澶囧け璐ュ垎鎴愪袱绫伙細瑙嗛鏂囦欢浼氶檷绾т负 `video-only` 缁х画鎾斁锛岄煶棰?only 鏂囦欢浠嶇劧鐩存帴澶辫触锛岄伩鍏嶁€滄墦寮€鎴愬姛浣嗘病鏈変换浣曞彲鎾斁杈撳嚭鈥濈殑浼垚鍔熴€?
- 鏃犻煶棰戣緭鍑轰絾鏈夎棰戞祦鏃讹紝涓绘椂閽熶粠 `System` 鏀逛负 `Video`锛岃浣嶇疆鎺ㄨ繘璺熼殢瀹為檯娓叉煋 PTS锛岃€屼笉鏄郴缁熸椂閽熶及绠椼€?
- `DiagnosticsSnapshot` 鏂板 `audio_output_initialized / video_only_fallback / clock_source`锛宍--performance-log-check`銆乣--1080p60-check`銆乣--4k-playback-check`銆乣--high-bitrate-check`銆乣--long-playback-check` 浼氬悓姝ヨ緭鍑鸿繖浜涚姸鎬併€?
- `1080p60/high-bitrate/long-playback` 涓変釜鍥炲綊闂ㄧ鐜板凡鏀逛负鍙湅 `demux_queue_drop_packets`锛屼笉鍐嶆妸闊抽绂佺敤鍦烘櫙涓嬬殑 `demux_ignored_packets` 璇垽鎴愰珮鐮佺巼鍥炲帇澶辫触銆?
- 宸查噸鏂版墽琛岋細

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`

  浠ュ強 `--1080p60-check`銆乣--high-bitrate-check`銆乣--long-playback-check`銆乣--performance-log-check`锛屽綋鍓嶅潎閫氳繃锛沗--4k-playback-check` 浠嶅彧鍓?`fallback_ok` 瀛愯繘绋嬭矾寰勫緟缁х画鎺掓煡銆?




### 2026-03-19 鏇存柊锛氭挱鏀鹃摼璇婃柇鍒嗗眰涓?decoder drain / scheduler 瀹归敊琛ュ己



- `decodeVideoFrame()` / `decodeAudioFrame()` 宸叉敼涓烘寔缁?`receive -> send -> receive` 鐘舵€佹満锛屽苟鍦?packet queue EOF 鍚庡 codec 鍙戦€?`nullptr` drain锛岄伩鍏嶆妸鈥滄殏鏃舵棤杈撳嚭鈥濆拰鈥滅湡姝ｅけ璐モ€濇贩鍦ㄤ竴璧枫€?
- `DiagnosticsSnapshot` 鐜板凡鍖哄垎 `demux_ignored_packets / demux_queue_drop_packets`锛屽苟鏂板 decoder `send_packet(EAGAIN)`銆乨rain 娆℃暟銆乣native/copy-back/swscale/filter-blocked` 瑙嗛璺緞璁℃暟銆?
- `Scheduler` 宸叉柊澧?video/audio 鑳屽帇浜嬩欢涓?video/audio/render restart 缁熻锛宺ender thread 涔熺撼鍏ュ彈淇濇姢 worker锛沗--performance-log-check` 浼氬悓姝ヨ緭鍑鸿繖浜涙柊鎸囨爣銆?




### 2026-03-19 鏇存柊锛歅layerCore 鍋滄挱鏀跺彛涓庤繍琛屾椂璁捐鍊轰慨澶?


- `PlayerCore` 宸茶ˉ涓?deferred stop / worker reap 璺緞锛孍OF 鑷姩鍋滄挱銆丯ext/Previous銆丵uit 绛夎矾寰勪笉鍐嶅彧鏀圭姸鎬佽€岄仐鐣?demux/audio/scheduler 鑴忕嚎绋嬨€?
- `PacketQueue` 宸蹭粠鍘熷 `AVPacket*` 鍒囧埌 RAII `unique_ptr` 鎵€鏈夋潈妯″瀷锛宻eek / stop / close 杩囩▼涓殑鍓╀綑鍘嬬缉鍖呬細闅忛槦鍒楁竻鐞嗚嚜鍔ㄩ噴鏀俱€?
- `Scheduler` 宸叉敮鎸佸紓姝ュ仠鏈哄苟鍦ㄩ噸鍚墠鍥炴敹宸查€€鍑?worker锛沗Clock` 宸蹭慨澶?system-clock 鐨?pause/speed 鏃堕棿鍩哄噯杩炵画鎬э紱`Demuxer::open()` 涔熷凡鍘绘帀閿佸唴閲嶅叆 `close()` 鐨勬閿侀闄┿€?
- 宸查噸鏂版墽琛屾暣宸ョ▼楠岃瘉鍛戒护

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  褰撳墠缁撴灉涓?0 涓鍛?/ 0 涓敊璇€?


### 2026-03-18 鏇存柊锛歁SVC warning debt 鍒嗗眰娓呯悊



- Windows MSVC 鐩爣宸插惎鐢?`/utf-8 /external:anglebrackets /external:W0`锛屾湰鍦?UTF-8 婧愭枃浠朵笉鍐嶈Е鍙?`C4819`锛岀涓夋柟 angle-bracket 澶存枃浠?warning 涔熻闅旂鍒板閮ㄥ眰銆?
- 鏈湴 `logger.cpp` 鐨?`getenv`銆佸瓧骞曡В鏋愬櫒鐨?`sscanf`銆乣main.cpp` 鐨勬湭浣跨敤鍙傛暟锛屼互鍙?`player_core.cpp` 鐨勬潯浠惰祴鍊?warning 鍧囧凡娓呯悊銆?
- 宸查噸鏂版墽琛屾暣宸ョ▼楠岃瘉鍛戒护

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  褰撳墠缁撴灉涓?`0 涓鍛?/ 0 涓敊璇痐銆?
- 杩欒疆鍙樻洿涓嶆墿灞曟挱鏀惧櫒鍔熻兘闈紝鐩爣浠呬负鎭㈠ Windows CI 鐨勪綆鍣０鏋勫缓鍩虹嚎銆?


### 2026-03-18 鏇存柊锛欰SS 鏍囩瑙ｆ瀽涓?UTF-16 鑼冨洿淇



- `ASS/SSA` override 瑙ｆ瀽宸蹭慨澶?`\fnArial`銆乣\rDefault` 绛夌揣鍑戝啓娉曠殑鏍囩璇嗗埆閿欒锛屽父瑙佸瓧浣撳垏鎹㈠拰鏍峰紡閲嶇疆鐜板湪浼氭寜棰勬湡杩涘叆鍘熺敓 D3D11 瀛楀箷閾俱€?
- `SubtitleTextRun.start/length` 涓?`D3D11VideoRenderer` 鐨?DirectWrite `DWRITE_TEXT_RANGE` 宸茬粺涓€涓?UTF-16 code unit 璇箟锛宔moji銆佹墿灞曞瓧绗﹀拰鍏朵粬闈?BMP 鏂囨湰鐨勬牱寮忚寖鍥翠笉鍐嶉敊浣嶃€?
- 宸查噸鏂版墽琛屾暣宸ョ▼楠岃瘉鍛戒护

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  褰撳墠缁撴灉涓?`167 涓鍛?/ 0 涓敊璇痐锛泈arning 涓昏鏉ヨ嚜绗笁鏂瑰ご鏂囦欢锛團Fmpeg / Quill锛夈€侀」鐩唴澶氬婧愮爜鐨?C4819 缂栫爜鍛婅锛屼互鍙婂皯閲忔棦鏈夌殑 C4996 / C4100 鎻愮ず銆?
- 鏈疆鍓╀綑琛ヤ竵涓嶅啀鎵╁睍鍔熻兘闈紝鍙敹鏁涘師鐢?D3D11 瀛楀箷閾剧殑璇箟姝ｇ‘鎬т笌浜や粯璁板綍銆?
### 2026-03-18 鏇存柊锛欴3D11 鍘熺敓娓叉煋閾句笌 ASS/SSA 瀛楀箷閾炬敹鍙?


- Windows 榛樿 `D3D11` renderer 鐜板湪鍏峰鐙珛鐨?`window + device + swap chain + native video + native subtitle overlay` 涓婚摼銆?
- 鍦ㄦ湭鍚敤瑙嗛婊ら暅涓旀牸寮忓彈鏀寔鏃讹紝`PlayerCore` 浼氫繚鐣?`AV_PIX_FMT_D3D11` 鍘熺敓纭欢甯х洿閫氬埌 renderer锛屽苟鍚屾椂鍚戞覆鏌撳櫒鎺ㄩ€佸鏉″綋鍓嶆縺娲诲瓧骞曞璞°€?
- 澶栨寕瀛楀箷閾惧凡浠庘€滀粎 SRT / 绾枃鏈€濇帹杩涘埌鈥?ass/.ssa/.srt + 缁撴瀯鍖栧瓧骞曞璞?+ 鍘熺敓 D3D11 鏍峰紡缁樺埗鈥濓紱`main.cpp` 鑷姩鎺㈡祴椤哄簭涓?`.ass -> .ssa -> .srt`銆?
- `D3D11VideoRenderer` 鐜板湪鍦ㄥ悓涓€鍧?swap chain backbuffer 涓婂畬鎴?ASS/SSA 甯哥敤鏍峰紡瀛楀箷缁樺埗锛岃鐩栧～鍏呫€佹弿杈广€侀槾褰便€佽儗鏅銆佸榻愩€佸畾浣嶅拰 run 绾у瓧浣撴牱寮忥紱闈?D3D11 娓叉煋鍣ㄩ粯璁ら€€鍖栦负绾枃鏈樉绀恒€?
- 宸叉竻鐞嗗叏灞€鏋勫缓闃诲鏂囦欢涓殑缂栫爜璇闂锛屽畬鏁磋В鍐虫柟妗堥獙璇佸懡浠?
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m`

  褰撳墠缁撴灉涓?`0 涓鍛?/ 0 涓敊璇痐銆?
- 褰撳墠闄愬埗浠嶇劧瀛樺湪锛氳繖涓嶆槸瀹屾暣鐨?libass 绛変环瀹炵幇锛屽皻鏈ˉ鍏呯湡瀹?`.ass` 鏍锋湰鐨勮繍琛屾椂瑙嗚楠屾敹锛涜瑙?`docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md` 涓?`docs/records/CHANGELOG.md` 鐨勯棶棰?66銆?
---

## 闃舵涓€锛氬熀纭€鎾斁鍣紙鍘嗗彶璧风偣锛?


### 寮€濮嬫棩鏈?
2026-02-17



### 闃舵鐩爣

瀹炵幇鍩虹鐨勮棰戞挱鏀惧姛鑳斤紝鍖呮嫭锛?
- 瑙嗛瑙ｇ爜鍜屾樉绀?
- 闊抽瑙ｇ爜鍜屾挱鏀?
- 鍩烘湰鎾斁鎺у埗



### 瀹屾垚鐘舵€?
- [x] 椤圭洰缁撴瀯鎼缓

- [x] FFmpeg 8.0.1 闆嗘垚

- [x] SDL2 2.30.11 闆嗘垚

- [x] 鏃ュ織绯荤粺锛圦uill 鍙岄€氶亾 + Fallback锛?
- [x] 瑙嗛瑙ｇ爜妯″潡

- [x] 闊抽瑙ｇ爜妯″潡

- [x] 瑙嗛鏄剧ず妯″潡

- [x] 闊抽鎾斁妯″潡

- [x] 涓绘挱鏀惧櫒閫昏緫

- [x] 鐗堟湰鍏煎鎬т慨澶?
- [x] 瑙嗛娴佺储寮曚笉鍖归厤闂淇



**璇存槑**:

- 鏈妭璁板綍鐨勬槸 2026-02-17 ~ 2026-02-25 鐨勬棭鏈熷疄鐜板熀绾裤€?
- 褰撴椂浣跨敤鐨勬槸鏃х増 decoder / playLoop 鏋舵瀯锛涚浉鍏虫枃浠跺凡鍦?2026-03-06 鏋舵瀯鏀舵暃鍚庣Щ闄ゆ垨骞跺叆 `PlayerCore + Scheduler + core/*` 涓婚摼銆?
- 涓洪伩鍏嶈瀵硷紝涓嬭堪鍘嗗彶璇存槑淇濈暀鈥滆兘鍔涗笌闂鈥濇湰韬紝涓嶅啀鎶婂凡鍒犻櫎鏂囦欢褰撲綔褰撳墠浠撳簱缁撴瀯璇存槑銆?


### 鐗堟湰鍏煎鎬т慨鏀?


#### FFmpeg 8.0.1 鍏煎鎬?
- 淇 `codec_ctx_->avctx->priv_data` 鍦?FFmpeg 8.0 涓笉鍙敤鐨勯棶棰?
- 璇ヤ慨澶嶅彂鐢熷湪鏃х増鐙珛瑙嗛/闊抽瑙ｇ爜鍣ㄥ疄鐜颁腑锛?
  - 鍦ㄨВ鐮佸櫒瀵硅薄鍐呮樉寮忎繚瀛?`format context`锛?
  - 涓嶅啀渚濊禆浠?`codec context` 鍙嶅悜璇诲彇鍐呴儴瀛楁銆?
- 鐩稿叧瀹炵幇鍚庢潵宸插苟鍏ョ幇琛岃В灏佽/瑙ｇ爜涓婚摼锛屾棫鏂囦欢鍚嶅凡涓嶅啀淇濈暀銆?
#### 鏃ュ織绯荤粺鏇存柊锛堜紒涓氱骇 Quill 绠￠亾锛?
- 閲嶆柊鍚敤 Quill锛屾瀯寤?ConsoleSink + RotatingFileSink 寮傛鏃ュ織閫氶亾锛屽苟淇濇寔 stdout/stderr 澶囨彺銆?
- `config/logging.conf` 涓?`MVP_LOG_*` 鐜鍙橀噺鏀寔杩愯鏃惰皟鏁?`log_dir`銆佽疆杞ぇ灏?鏁伴噺鍙婃棩蹇楃瓑绾с€?
- 鍒濆鍖栧け璐ユ垨鐩綍涓嶅彲鍐欐椂鑷姩鍛婅骞堕檷绾э紱瀹忔帴鍙ｄ笌 `DEBUG_MODE` 琛屼负淇濇寔鍏煎銆?
- 淇敼鏂囦欢锛歚include/logger.h`銆乣src/logger.cpp`銆乣config/logging.conf`銆乣docs/design/LOGGING.md`銆乣docs/records/CHANGELOG.md`銆乣docs/records/VERSION.md`

- 缂栬瘧杈撳嚭绀轰緥锛歚Quill: enabled`锛岃繍琛屾湡鏃ュ織鍐欏叆 `logs/modern-video-player.log`



#### 澶氱嚎绋嬫挱鏀炬灦鏋勯噸鏋?(2026-02-25)

- 瀹炵幇鐙珛瑙嗛瑙ｇ爜绾跨▼锛圴ideoDecodeThread锛?
- 瀹炵幇鐙珛闊抽瑙ｇ爜绾跨▼锛圓udioDecodeThread锛?
- 瀹炵幇绾跨▼瀹夊叏甯ч槦鍒楋紙FrameQueue锛夛紝鏀寔鏉′欢鍙橀噺绛夊緟

- 瀹炵幇闊宠棰戝悓姝ョ鐞嗗櫒锛圫yncManager锛夛紝鏀寔 AudioMaster/VideoMaster/Free 涓夌妯″紡

- 閲嶆瀯 VideoPlayer锛屼粠鍗曠嚎绋?playLoop 鏀逛负澶氱嚎绋?renderLoop

- 杩欎簺鑳藉姏鍚庢潵娌夋穩涓哄綋鍓嶄粨搴撲腑鐨?`core` 妯″潡锛?
  - `include/core/frame_queue.h`

  - `include/core/decoder_thread.h`

  - `include/core/clock.h`

  - `include/core/scheduler.h`

  - `include/core/player_core.h`

- 鏃х増绾跨▼/鍚屾鏂囦欢鍚嶄粎浠ｈ〃鏃╂湡瀹炵幇闃舵锛岀幇宸蹭笉鍐嶄綔涓哄綋鍓嶆枃浠剁粨鏋勫瓨鍦ㄣ€?
#### 闊抽鎾斁鏋舵瀯淇 (2026-02-25)

- 淇闊抽鎾斁鏂柇缁画鐨勯棶棰?
- AudioDecodeThread 瑙ｇ爜鍚庣洿鎺ヨ皟鐢?AudioPlayer::play() 灏嗘暟鎹斁鍏?SDL 闃熷垪

- 纭繚 SDL 闊抽鍥炶皟鑳芥寔缁幏鍙栨暟鎹?
- 鐩稿叧閫昏緫鍚庢潵宸插苟鍏ュ綋鍓嶉煶棰戞秷璐圭嚎绋嬨€乣AudioPlayer` 涓?`PlayerCore` 涓婚摼锛屼笉鍐嶅搴旂嫭绔嬫棫鏂囦欢鍚嶃€?
#### Frame 绉诲姩璇箟淇 (2026-02-25)

- 淇 VideoFrame 鍜?AudioFrame 绫荤己灏戠Щ鍔ㄨ涔夊鑷寸殑宕╂簝闂

- 涓?VideoFrame 娣诲姞绉诲姩鏋勯€犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃

- 涓?AudioFrame 娣诲姞绉诲姩鏋勯€犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃

- 鏄惧紡鍒犻櫎鎷疯礉鏋勯€犲嚱鏁板拰鎷疯礉璧嬪€艰繍绠楃锛岄槻姝㈡祬鎷疯礉

- 褰撳墠绛変环鑳藉姏浣嶄簬锛?
  - `include/core/frame.h`

  - `src/core/frame.cpp`

#### 澶氳В鐮佸櫒瀹炰緥绔炰簤璇诲彇淇 (2026-02-25)

- 淇鎾斁鏃跺涓В鐮佸櫒绔炰簤璇诲彇鍚屼竴 AVFormatContext 瀵艰嚧鐨勮В鐮侀敊璇?
- 鍦ㄦ棭鏈熷疄鐜颁腑閫氳繃閲嶇疆鏃х増瑙ｇ爜鍣ㄧ姸鎬侀伩鍏嶇珵浜夛紱鍚庣画宸叉敼涓虹粺涓€ `Demuxer + PlayerCore + Scheduler` 涓婚摼

- 淇敼鏂囦欢锛?
  - `src/video_player.cpp`

### 宸茬煡闂

- 闊宠棰戝悓姝ヤ娇鐢ㄧ畝鍗曠殑鏃堕棿璁＄畻锛岄珮閫熸垨浣庨€熸挱鏀炬椂鍙兘鏈夊悓姝ラ棶棰?
- 浠呮敮鎸?YUV420P 鏍煎紡鐨勮棰?
- seek 鍔熻兘闇€瑕佽繘涓€姝ュ畬鍠?


### 闂淇璁板綍



#### 瑙嗛娴佺储寮曚笉鍖归厤闂 (2026-02-24)



**闂鎻忚堪**:

- 鎾斁 mp4 鏂囦欢鏃讹紝瑙嗛鏃犳硶姝ｅ父鏄剧ず

- 鏃ュ織鏄剧ず `stream_index=0, expected=1` 鍙嶅鍑虹幇

- 绋嬪簭寰幆 48 娆℃墠鑳借鍒版纭殑瑙嗛甯?


**闂鍘熷洜**:

- MP4 鏂囦欢鐨勬祦椤哄簭鏄煶棰戞祦(绱㈠紩0)鍦ㄨ棰戞祦(绱㈠紩1)涔嬪墠

- `av_read_frame()` 杩斿洖鐨勫寘鍙兘鏄换鎰忔祦鐨勶紙閫氬父鏄涓€涓祦 - 闊抽娴侊級

- 鍘熶唬鐮侀亣鍒颁笉鍖归厤鐨勬祦鏃剁洿鎺ヨ繑鍥?false锛屽鑷磋棰戝抚鏃犳硶瑙ｇ爜



**瑙ｅ喅鏂规**:

- 淇敼鏃х増瑙嗛瑙ｇ爜寰幆鐨勮鍖呴€昏緫

- 灏嗛亣鍒颁笉鍖归厤娴佹椂杩斿洖 false锛屾敼涓?continue 璺宠繃璇ュ寘

- 寰幆璇诲彇鐩村埌鎵惧埌姝ｇ‘娴佺储寮曠殑鍖?


**淇敼鏂囦欢**:

- 鏃х増瑙嗛瑙ｇ爜鍣ㄥ疄鐜帮紙璇ユ枃浠跺悗缁凡鍦ㄦ灦鏋勬敹鏁涗腑绉婚櫎锛?


**鏃ュ織绀轰緥**:

```

[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=0, expected=1

[DEBUG] [VIDEO] decodeFrame: packet stream mismatch, skipping

... (閲嶅 48 娆?

[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=1, expected=1

[DEBUG] [VIDEO] decodeFrame: success, pts=0

```



#### 闊抽娴佺储寮曚笉鍖归厤闂 (2026-02-24)



**闂鎻忚堪**:

- 涓庤棰戞祦绱㈠紩鐩稿悓鐨勯棶棰橈紝闊抽鏃犳硶姝ｅ父瑙ｇ爜



**闂鍘熷洜**:

- 鍚屾牱鐨勯棶棰橈細闊抽鍖呭彲鑳戒笉鏄涓€涓璇诲彇鐨勬祦



**瑙ｅ喅鏂规**:

- 淇敼鏃х増闊抽瑙ｇ爜寰幆鐨勮鍖呴€昏緫

- 搴旂敤涓庤棰戣В鐮佸櫒鐩稿悓鐨勪慨澶?


**淇敼鏂囦欢**:

- 鏃х増闊抽瑙ｇ爜鍣ㄥ疄鐜帮紙璇ユ枃浠跺悗缁凡鍦ㄦ灦鏋勬敹鏁涗腑绉婚櫎锛?


#### YUV 鏁版嵁娓叉煋閿欒 (2026-02-24)



**闂鎻忚堪**:

- 瑙ｇ爜鎴愬姛鍚庣▼搴忕珛鍗抽€€鍑猴紝娌℃湁鐢婚潰鏄剧ず



**闂鍘熷洜**:

- `renderFrame` 鍑芥暟浣跨敤閿欒鐨?YUV 鏁版嵁

- 鍘熸潵浼犻€掔殑鏄?`frame->data[0]`锛堝彧鏄?Y 骞抽潰鎸囬拡锛?
- 鐒跺悗閿欒鍦板亣璁?Y/U/V 鏄繛缁瓨鍌ㄧ殑



**瑙ｅ喅鏂规**:

1. 浼犻€掓暣涓?AVFrame 鎸囬拡鑰屼笉鏄?`data[0]`

2. 姝ｇ‘浣跨敤 Y/U/V 骞抽潰鐨勬暟鎹拰琛屽ぇ灏?


**淇敼鏂囦欢**:

- `src/display.cpp`

- `src/video_player.cpp`



**浠ｇ爜鍙樻洿**:

```cpp

// video_player.cpp

display_->renderFrame((const uint8_t*)frame, frame->width, frame->height);



// display.cpp

AVFrame* frame = (AVFrame*)data;

SDL_UpdateYUVTexture(

    texture_, nullptr,

    frame->data[0], frame->linesize[0],

    frame->data[1], frame->linesize[1],

    frame->data[2], frame->linesize[2]

);

```



### 闃舵鍚庣画鐩爣锛堝巻鍙茶褰曪級

- 鍚庣画鐩爣宸插湪 2026-03-06 涔嬪悗鐨勭珷鑺備腑閫愭钀藉湴锛屽寘鎷細

  - 缁熶竴 `PlayerCore + Scheduler + Filters` 鏋舵瀯锛?
  - 澶栨寕瀛楀箷銆佹挱鏀惧垪琛ㄣ€佽缃笌蹇嵎閿帴鍏ワ紱

  - D3D11VA / D3D11 鏈€灏忓彲鐢ㄩ摼璺紱

  - 绔犺妭瀵艰埅銆丄-B Repeat銆佹埅鍥俱€?


---



## 鐗堟湰鍘嗗彶



### v1.0.0 (绗竴闃舵)

- 鍒濆鐗堟湰

- 鍩轰簬 FFmpeg + SDL2 + Quill 鏋勫缓

- 鏀寔鍩烘湰鐨勮棰戞挱鏀惧姛鑳?


---



## 渚濊禆搴撳畨瑁呰鏄?


### Windows



#### FFmpeg 8.0.1

```powershell

# 浣跨敤 vcpkg

vcpkg install ffmpeg:x64-windows



# 鎴栨墜鍔ㄤ笅杞?
# 璁块棶 https://www.gyan.dev/ffmpeg/builds/

# 涓嬭浇 ffmpeg-8.0.1-full_build.7z

```



#### SDL2 2.30.11

```powershell

# 浣跨敤 vcpkg

vcpkg install sdl2:x64-windows



# 鎴栨墜鍔ㄤ笅杞?
# 璁块棶 https://github.com/libsdl-org/SDL/releases

# 涓嬭浇 SDL2-devel-2.30.11-VC.zip

```



**娉ㄦ剰**: 鑻ユ湭鍗曠嫭瀹夎 Quill锛屽彲灏嗘簮鐮佽В鍘嬪埌 `external/quill`锛涗篃鍙互閫氳繃鍖呯鐞嗗櫒瀹夎鍚庤缃?`QUILL_ROOT`銆?


### Linux



```bash

# 缂栬瘧 FFmpeg 8.0.1

./configure --prefix=/usr/local

make -j$(nproc)

sudo make install



# SDL2 閫氬父閫氳繃鍖呯鐞嗗櫒瀹夎

sudo apt install libsdl2-dev

```



### macOS



```bash

# 浣跨敤 Homebrew

brew install ffmpeg

brew install sdl2

```



**娉ㄦ剰**: Quill 涓?header-only 搴擄紝鍙€氳繃 `brew` 瀹夎鎴栫洿鎺ヤ笅杞芥簮鐮佸悗閰嶇疆 `QUILL_ROOT`銆?


---



## 缂栬瘧鍛戒护



### Windows (Visual Studio)

```powershell

mkdir build

cd build

cmake .. -G "Visual Studio 17 2022" -A x64

cmake --build . --config Release

```



### Linux / macOS

```bash

mkdir build

cd build

cmake ..

make -j$(nproc)

```



---



## 鏂囨。鏇存柊鏃ュ織



| 鏃ユ湡 | 鏇存柊鍐呭 |

|------|----------|

| 2026-02-17 | 鍒涘缓鐗堟湰璁板綍鏂囨。锛岃褰曠涓€闃舵瀹屾垚鎯呭喌 |

| 2026-02-24 | 鏇存柊鏃ュ織绯荤粺涓轰紒涓氱骇 Quill 绠￠亾锛岃褰曡棰戞祦绱㈠紩闂淇 |

| 2026-02-24 | 璁板綍闊抽娴佺储寮曢棶棰樺拰 YUV 娓叉煋闂淇 |

| 2026-02-24 | 鍒涘缓 CHANGELOG.md 闂淇璁板綍鏂囨。 |

| 2026-02-25 | 璁板綍澶氱嚎绋嬫挱鏀炬灦鏋勯噸鏋勫拰闊抽鎾斁鏋舵瀯淇 |

| 2026-02-25 | 璁板綍 Frame 绉诲姩璇箟淇 |

| 2026-02-25 | 璁板綍澶氳В鐮佸櫒瀹炰緥绔炰簤璇诲彇淇 |

| 2026-03-06 | 淇灏忓睆绐楀彛杩囧ぇ涓庣獥鍙ｇ缉鏀句簨浠跺吋瀹归棶棰?|

| 2026-03-07 | 鎺ュ叆 GitHub Actions 鑷姩鏍煎紡鍥炲綊涓?CI 鍏煎鏀归€?|

| 2026-03-07 | 鎺ュ叆鎾斁鍒楄〃涓婚摼璺€佽缃寔涔呭寲涓庡揩鎹烽敭棣栫増 |

| 2026-03-08 | 娓呯悊鍘嗗彶绔犺妭涓殑鏃ц矾寰勬弿杩帮紝閬垮厤灏嗗凡绉婚櫎鏂囦欢璇啓涓哄綋鍓嶄粨搴撶粨鏋?|

| 2026-03-18 | 鍚屾 D3D11 鍘熺敓娓叉煋閾炬渶缁堢姸鎬侊紝璁板綍 ASS/SSA 鍘熺敓瀛楀箷閾炬敹鍙ｄ笌鍏ㄥ眬鏋勫缓闃诲娓呯悊 |



---



## 2026-03-06 鏇存柊



### Core API / Scheduler / Filter 閲嶆瀯

- 鏂板 `core` 瀛愭ā鍧楋細`PlayerCore`銆乣Scheduler`銆乣FrameQueue`銆乣Clock`銆乣Command`銆乣Frame`銆?
- 鏂板 `filters` 瀛愭ā鍧楋細Filter 鎺ュ彛銆乣FilterRegistry`銆乣FilterPipeline`銆佸唴缃寒搴?瀵规瘮搴?楗卞拰搴︽护闀溿€?
- 鍒濇湡寮曞叆 `VideoPlayer -> PlayerCore` 閫傞厤灞傦紝闅忓悗鍦ㄥ悓鏃ュ畬鎴愭灦鏋勬敹鏁涘苟绉婚櫎鏃ц矾寰勫垎鍙夈€?


### 娴嬭瘯涓庢瀯寤?
- 褰撴椂鏇惧紩鍏ヤ复鏃舵牳蹇冩祴璇曟枃浠讹紱杩欎簺娴嬭瘯鏂囦欢宸插湪 2026-03-07 娓呯悊銆?
- `CMakeLists.txt` 鍦ㄨ闃舵寮€濮嬬撼鍏?`src/core/*` 涓?`src/filters/*` 涓荤紪璇戦」銆?


---



## 2026-03-06 鏋舵瀯鏀舵暃鏇存柊



### 缁熶竴鏍稿績鏋舵瀯

- 鍘婚櫎鏃ф挱鏀鹃摼璺紝瀹炵幇缁熶竴涓?`VideoPlayer + PlayerCore + Scheduler + Filters`銆?
- 鍒犻櫎鏃?decoder/thread/sync/packet/legacy clock 鐩稿叧鏂囦欢銆?
- `CMakeLists.txt` 浠呬繚鐣欐柊鏋舵瀯缂栬瘧椤广€?


### 鏂囨。

- 鏂板 `docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`锛屽寘鍚噸鏋勭洰鏍囥€佸垹闄ゆā鍧椼€佷繚鐣欐枃浠舵竻鍗曘€?


---



## 2026-03-06 绐楀彛浣撻獙淇



### 灏忓睆鑷€傚簲涓庣缉鏀剧ǔ瀹氭€?
- `Display::init()` 鍚姩鏃舵寜灞忓箷鍙敤鍖哄煙鑷€傚簲鍒濆绐楀彛灏哄锛堥檺鍒跺埌 90%锛屼繚鎸佽棰戞瘮渚嬶級銆?
- 澧炲姞 `SDL_WINDOWEVENT_SIZE_CHANGED` 澶勭悊锛屽吋瀹逛笉鍚屽钩鍙?椹卞姩涓嬬殑绐楀彛灏哄鍙樺寲浜嬩欢銆?
- 娓叉煋闃舵鎸夋簮瑙嗛姣斾緥璁＄畻鐩爣鍖哄煙锛岄伩鍏嶇獥鍙ｆ嫋鎷藉悗鐢婚潰鎷変几銆?


### 淇敼鏂囦欢

- `src/display.cpp`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



## 2026-03-07 鏇存柊



### 鎾斁绋冲畾鎬т笌鍩虹浜や簰澧炲己

- 淇绐楀彛鏈€澶у寲/缂╂斁鏃惰棰戠敾闈㈠彲鑳藉崱浣忕殑闂锛堥煶棰戜笉涓柇锛夈€?
- 鏄剧ず灞傛柊澧炲熀纭€鎺у埗鏉★紙杩涘害鏉°€侀煶閲忔潯锛夈€?
- 鏀寔榧犳爣鎷栧姩杩涘害鏉¤繘琛?seek銆?
- 鏀寔榧犳爣鎷栧姩闊抽噺鏉¤繘琛屽疄鏃堕煶閲忚皟鑺傘€?
- 琛ュ厖闊抽噺蹇嵎閿細`+/-` 涓?`鈫?鈫揱銆?


### 淇敼鏂囦欢

- include/display.h

- src/display.cpp

- src/core/player_core.cpp

- src/main.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



## 2026-03-07 鏇存柊锛堜紒涓氱骇妯″潡鎺ㄨ繘锛?


### MPC-HC 鏋舵瀯妯″潡鎵╁睍

- 鏂板浼佷笟绾у熀纭€璁炬柦妯″潡锛歚TaskQueue`銆乣FramePool`銆乣DecoderThread`銆?
- 鏂板娓叉煋鎶借薄灞傦細`IVideoRenderer`銆乣SdlVideoRenderer`銆乣D3D11/OpenGL` 鍗犱綅瀹炵幇銆乣RendererFactory`銆?
- `PlayerCore` 浠庣洿鎺ヤ緷璧?`Display` 璋冩暣涓轰緷璧栨覆鏌撴娊璞℃帴鍙ｃ€?
- 鏂板闊抽澧炲己妯″潡锛?0 娈靛潎琛″櫒銆佹贩闊冲櫒锛沗AudioPlayer` 澧炲姞缂撳啿瑙傛祴鎺ュ彛銆?
- 鏂板瑙ｇ爜鍣ㄧ鐞嗘ā鍧楋細鑳藉姏鎺㈡祴涓庡悗绔嚜鍔ㄩ€夋嫨銆?
- 鏂板瀛楀箷锛圫RT锛夈€佹挱鏀惧垪琛紙M3U8锛夈€佽缃€佸揩鎹烽敭銆佺毊鑲ゃ€佹彃浠躲€佹牸寮忔敮鎸併€佹祦濯掍綋瑙ｆ瀽绛夋ā鍧楅鏋躲€?
- 鏂板婊ら暅鍩虹被涓庨煶瑙嗛婊ら暅閾撅紝琛ュ厖闊抽噺/骞宠　闊抽婊ら暅銆?
- 鍚屾鏇存柊 `.monkeycode/specs/enterprise-quill-logging/tasklist.md` 宸插疄鐜伴」銆?


### 淇敼鏂囦欢

- CMakeLists.txt

- include/core/*

- src/core/*

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

- include/filters/*

- src/filters/*

- .monkeycode/specs/enterprise-quill-logging/tasklist.md

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



## 2026-03-07 鏇存柊锛堟牸寮忚兘鍔涚煩闃典笌楂樺垎楂樺抚璇勪及锛?


### 鑳藉姏妫€鏌ュ叆鍙?
- 鏂板 `--capabilities` 鍛戒护锛氳緭鍑鸿繍琛屾椂瀹瑰櫒/瑙嗛瑙ｇ爜鍣?闊抽瑙ｇ爜鍣ㄨ兘鍔涳紝骞剁粰鍑轰富鍔涙牸寮忚鐩栫煩闃碉紙PASS/PARTIAL/FAIL锛夈€?
- 鏂板 `--evaluate-target` 鍛戒护锛氭寜 `瀹?楂?FPS/澹伴亾/鐮佺巼` 璇勪及瀹炴椂鎾斁鍙鎬т笌纭В/D3D11寤鸿銆?


### 绋冲仴鎬у寮?
- 淇 `src/streaming/dash_manifest_parser.cpp` 鍦?MSVC 涓嬬殑 raw-string 姝ｅ垯缂栬瘧闂锛屾仮澶嶆瀯寤哄熀绾裤€?
- `Demuxer` 浣跨敤 `av_find_best_stream`锛屽苟寮曞叆 `probesize`/`analyzeduration`/`+genpts` 閫夐」澧炲己澶嶆潅濯掍綋鎺㈡祴銆?
- `AudioPlayer` 鏆撮湶瀹為檯杈撳嚭鍙傛暟锛沗PlayerCore` 澶嶇敤閲嶉噰鏍峰櫒骞舵寜璁惧杈撳嚭鍙傛暟鍋氬闊抽亾閲嶉噰鏍枫€?


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



## 2026-03-07 鏇存柊锛圖3D11VA 纭В鍥為€€閾捐矾锛?


### 瑙ｇ爜鍚庣

- `PlayerCore` 澧炲姞 D3D11VA 纭В灏濊瘯閫昏緫锛圵indows锛夈€?
- 纭В鍒濆鍖栧け璐ユ椂鑷姩鍥為€€杞В锛屼繚璇佸彲鎾斁鎬т紭鍏堛€?


### 瑙嗛杈撳嚭瑙勬暣

- 瀵圭‖浠跺抚澧炲姞 `av_hwframe_transfer_data`锛?
- 瀵归潪 `YUV420P` 甯у鍔?`sws_scale` 杞崲锛岀粺涓€涓?`YUV420P` 缁?SDL 娓叉煋銆?


### 淇敼鏂囦欢

- include/core/player_core.h

- src/core/player_core.cpp



## 2026-03-07 鏇存柊锛堝崟鏂囦欢鎺㈡祴涓庢牸寮忓洖褰掕嚜鍔ㄥ寲锛?


### 鏂板鍙墽琛屽叆鍙?
- 鏂板 `--probe-file <media_file>` 鍛戒护锛氳緭鍑?`probe.*` 鏈哄櫒鍙瀛楁锛屽寘鍚鍣?瑙嗛/闊抽鐘舵€併€佸垎杈ㄧ巼銆佸抚鐜囥€佸０閬撲笌寤鸿淇℃伅銆?


### 鍥炲綊宸ュ叿閾?
- 鏂板 `tools/format_regression/run_format_regression.ps1`锛氭寜鏍锋湰娓呭崟鑷姩鎵归噺鎵ц鎺㈡祴銆?
- 鏂板 `tools/format_regression/format_samples.csv`锛氱淮鎶ゆ牸寮忔牱鏈笌棰勬湡缂栫爜淇℃伅銆?
- 鏂板 `docs/workflows/FORMAT_REGRESSION.md`锛氳褰曡剼鏈弬鏁般€佽緭鍑鸿矾寰勫拰浣跨敤鏂瑰紡銆?
- 鎶ュ憡杈撳嚭锛歚docs/reports/FORMAT_REGRESSION_*.md`銆?
- 杩斿洖鐮佽涔夛細`0=鍏ㄩ儴PASS`锛宍1=瀛樺湪PARTIAL`锛宍2=瀛樺湪FAIL`銆?


### 淇敼鏂囦欢

- src/main.cpp

- tools/format_regression/run_format_regression.ps1

- tools/format_regression/format_samples.csv

- docs/workflows/FORMAT_REGRESSION.md

- docs/README.md



## 2026-03-07 鏇存柊锛圙itHub Actions 鑷姩鍥炲綊锛?


### CI 宸ヤ綔娴?
- 鏂板 `.github/workflows/format-regression.yml`锛?
  - PR / `main` / `master` 鑷姩瑙﹀彂锛?
  - 鑷姩涓嬭浇 `SDL2/FFmpeg` 棰勭紪璇戜緷璧栧悗鎵ц Windows 鏋勫缓锛?
  - 鏍锋湰鐢熸垚 + `run_all_checks.ps1` 鑷姩鍥炲綊锛?
  - 涓婁紶 `docs/reports/FORMAT_REGRESSION_CI.md` 浜х墿銆?


### 鏋勫缓涓庤剼鏈吋瀹规€?
- `CMakeLists.txt`锛圵indows锛変紭鍏堟敮鎸?`SDL2::`銆乣FFMPEG::`銆乣unofficial::ffmpeg::` 瀵煎叆鐩爣锛屼繚鐣?`external/` 鍥為€€璺緞銆?
- `tools/download_test_samples.ps1` 鏀寔閫氳繃 PATH 瑙ｆ瀽 `ffmpeg` 鍛戒护鍚嶏紝渚夸簬 CI 鐩存帴璋冪敤銆?


### 浠诲姟娓呭崟鍚屾

- 鏇存柊 `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `0.3`锛圥0/P1/P2 鑼冨洿鍐荤粨锛夋爣璁板畬鎴愶紱

  - `2.1.1`锛堟牸寮忕煩闃靛畾涔夊喕缁擄級鏍囪瀹屾垚锛?
  - `2.1.5`锛圥ASS/PARTIAL/FAIL 缁撴灉琛級鏍囪瀹屾垚锛?
  - `2.3.1`锛堟牸寮忕煩闃电粨鏋滃彲杩芥函锛夋爣璁板畬鎴愩€?


### 淇敼鏂囦欢

- .github/workflows/format-regression.yml

- CMakeLists.txt

- tools/download_test_samples.ps1

- docs/workflows/FORMAT_REGRESSION.md

- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



## 2026-03-07 鏇存柊锛堟挱鏀惧垪琛?+ 璁剧疆 + 蹇嵎閿鐗堬級



### 涓绘祦绋嬭兘鍔?
- `main` 鎺ュ叆 `PlaylistManager`锛?
  - 鏀寔澶氭枃浠跺弬鏁版挱鏀惧垪琛紱

  - 鏀寔 `.m3u8` 瀵煎叆锛?
  - 鏀寔 `PageUp/PageDown` 涓婁竴棣?涓嬩竴棣栵紱

  - EOF 鑷姩鍒囨崲涓嬩竴椤广€?
- `main` 鎺ュ叆 `SettingsManager`锛?
  - 鍚姩鍔犺浇 `config/player_settings.ini`锛?
  - 澶辫触鍥為€€榛樿鍊硷紱

  - 閫€鍑轰繚瀛橀煶閲忋€侀€熷害涓庡垪琛ㄧ储寮曘€?


### 浜や簰澧炲己

- 蹇嵎閿鐗堥粯璁ら敭浣嶅叏閮ㄦ帴鍏ワ細

  - `Space` 鎾斁/鏆傚仠锛?
  - `Enter/Alt+Enter/F` 鍏ㄥ睆锛?
  - `Esc/Q` 閫€鍑洪€昏緫锛堝叏灞忎紭鍏堥€€鍏ㄥ睆锛夛紱

  - `Left/Right` 涓?`Ctrl+Left/Ctrl+Right` 鐩稿 seek锛?
  - `Up/Down/+/-/M` 闊抽噺涓庨潤闊筹紱

  - `[`/`]` 鍙橀€熶笌 `R` 鎭㈠ 1.0x銆?
- 娓叉煋浜嬩欢璇锋眰閾捐矾鎵╁睍鍒?`PlayerCore` 涓?`VideoPlayer`锛屾敮鎸佷富娴佺▼娑堣垂鈥滀笂涓€棣?涓嬩竴棣?閫€鍑衡€濇帶鍒惰姹傘€?


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



## 2026-03-07 鏇存柊锛堢Щ闄?Core 鍗曞厓娴嬭瘯鐩爣锛?


### 鏋勫缓閰嶇疆璋冩暣

- 绉婚櫎 `BUILD_CORE_TESTS` 閫夐」锛岄伩鍏嶄繚鐣欐棤鏁堟祴璇曞紑鍏炽€?
- 绉婚櫎 `core_frame_queue_tests`銆乣core_clock_tests` 鍜?`core_tests` 鑱氬悎鐩爣銆?


### 鏂囦欢娓呯悊

- 鍒犻櫎 `tests/core_frame_queue_tests.cpp`銆?
- 鍒犻櫎 `tests/core_clock_tests.cpp`銆?


### 淇敼鏂囦欢

- CMakeLists.txt

- tests/core_frame_queue_tests.cpp锛堝垹闄わ級

- tests/core_clock_tests.cpp锛堝垹闄わ級

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-07 鏇存柊锛堝鎸傚瓧骞曞姞杞藉叆鍙ｏ級



### 瀛楀箷鍏ュ彛鑳藉姏

- 涓绘祦绋嬫柊澧炲鎸傚瓧骞曞弬鏁帮細`--subtitle <file.srt>`銆?
- 鏈紶 `--subtitle` 鏃讹紝鑷姩灏濊瘯鍔犺浇涓庡綋鍓嶅獟浣撳悓鍚?`.srt` 鏂囦欢銆?
- `VideoPlayer` 鏂板澶栨寕瀛楀箷鍔犺浇涓庢竻鐞嗘帴鍙ｏ紝鏀寔 SRT 瑙ｆ瀽骞惰褰曟潯鐩暟銆?


### 淇敼鏂囦欢

- include/video_player.h

- src/video_player.cpp

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-07 鏇存柊锛堝瓧骞曟覆鏌撳彔鍔犱笌鏃跺簭鍚屾锛?


### 瀛楀箷娓叉煋鑳藉姏

- 娓叉煋鎺ュ彛鏂板瀛楀箷鏂囨湰閫氶亾锛歚IVideoRenderer::setSubtitleText()`銆?
- SDL 娓叉煋閾捐矾鏂板瀛楀箷鍙犲姞灞傦紝鏀寔澶氳鏄剧ず銆佽秴闀挎埅鏂€佸簳鏉夸笌闃村奖銆?
- D3D11/OpenGL 娓叉煋鍣ㄨˉ榻愬瓧骞曟帴鍙ｆ々瀹炵幇锛屼繚璇佸鍚庣缂栬瘧涓€鑷存€с€?
- 褰撳墠瀛楀箷瀛楁ā涓鸿交閲忓疄鐜帮紝闈?ASCII 瀛楃浼氶檷绾ф樉绀恒€?


### 瀛楀箷鏃堕棿杞村悓姝?
- `PlayerCore` 鏂板澶栨寕瀛楀箷杞ㄩ亾绠＄悊涓庢椿璺冪储寮曠紦瀛樸€?
- 娓叉煋甯ц矾寰勪笌绌洪棽璺緞鍧囨寜褰撳墠鎾斁鏃堕棿鏇存柊瀛楀箷锛岃鐩栨挱鏀?鏆傚仠/seek銆?
- 淇瀛楀箷鏇存柊涓殑閿佺矑搴﹂棶棰橈紝閬垮厤閿佸唴璋冪敤娓叉煋鎺ュ彛銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `1.1.2 瀛楀箷娓叉煋鍙犲姞` 鏍囪瀹屾垚锛?
  - `1.1.3 鎾斁/鏆傚仠/seek 鍚屾` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堝瓧骞曞紑鍏充笌寮傚父澶勭悊锛?


### 瀛楀箷寮€鍏宠兘鍔?
- 鏂板杩愯鏃跺瓧骞曞紑鍏抽摼璺紝鏀寔鍦ㄦ挱鏀句腑鎸?`V` 鍒囨崲瀛楀箷鏄剧ず寮€/鍏炽€?
- 瀛楀箷鍏抽棴鏃剁珛鍗虫竻绌哄彔鍔犲眰锛涢噸鏂板紑鍚悗鎸夊綋鍓嶆挱鏀炬椂闂存仮澶嶅瓧骞曞悓姝ャ€?
- 瀛楀箷寮€鍏崇姸鎬佸湪鎾斁浼氳瘽鍐呬繚鎸佷竴鑷达紝璺ㄥ獟浣撳垏鎹㈠彲缁ф壙褰撳墠鏄剧ず鍋忓ソ銆?


### 寮傚父澶勭悊澧炲己

- 澶栨寕瀛楀箷鍔犺浇璺緞妫€鏌ユ敼涓?`std::error_code`锛岄伩鍏嶆枃浠剁郴缁熷紓甯镐紶鎾€?
- 澧炲姞瀛楀箷瑙ｆ瀽寮傚父鎹曡幏涓庢棩蹇楅檷绾у鐞嗭紝纭繚鎾斁涓婚摼涓嶄腑鏂€?
- 鍔犺浇澶辫触鏃朵繚鎸佺姸鎬佷竴鑷达細娓呯┖鏃у瓧骞曞苟缁х画鎾斁濯掍綋銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `1.1.4 瀛楀箷寮€鍏充笌寮傚父澶勭悊` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堝揩鎹烽敭閰嶇疆鎸佷箙鍖栵級



### 鎸佷箙鍖栬兘鍔?
- 鏂板 `hotkey.*` 閰嶇疆椤癸紝瑕嗙洊棣栫増蹇嵎閿姩浣溿€?
- 鍚姩鏃惰鍙?`config/player_settings.ini` 骞跺簲鐢ㄩ敭浣嶆槧灏勩€?
- 閫€鍑烘椂灏嗗綋鍓嶉敭浣嶅洖鍐欓厤缃枃浠讹紝淇濊瘉閲嶅惎鍚庝繚鎸佷竴鑷淬€?


### 浜や簰閾捐矾璋冩暣

- `Display` 鏀逛负閫氳繃 `HotkeyManager` 澶勭悊閿綅鏄犲皠锛屼笉鍐嶅浐瀹氬啓姝讳富閿€笺€?
- `Renderer` / `PlayerCore` / `VideoPlayer` 澧炲姞鐑敭绠＄悊閫忎紶鎺ュ彛銆?
- 淇濈暀 `Esc` 涓?`Enter` 鐨勫吋瀹硅涓猴紝闄嶄綆榛樿浣跨敤涔犳儻鍥炲綊椋庨櫓銆?


### 寮傚父涓庡吋瀹?
- 瀵归潪娉?`hotkey.*` 閰嶇疆杩涜瀹归敊闄嶇骇锛堜繚鐣欓粯璁ゅ苟璁板綍鍛婅锛夈€?
- 鏇存柊 `config/player_settings.ini` 榛樿鏍蜂緥锛岃ˉ榻愭墍鏈夊揩鎹烽敭椤广€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `1.3.2 鏀寔閿綅閰嶇疆鎸佷箙鍖朻 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堝揩鎹烽敭鍐茬獊妫€娴嬩笌鎭㈠榛樿锛?


### 鍐茬獊妫€娴嬭兘鍔?
- `HotkeyManager` 鏂板閿綅鍐茬獊妫€娴嬫帴鍙ｏ細

  - `findConflicts()`锛氳繑鍥炲啿绐佸姩浣滃锛?
  - `hasConflicts()`锛氬揩閫熷垽鏂槸鍚﹀瓨鍦ㄥ啿绐併€?
- 鍚姩鍔犺浇鐑敭閰嶇疆鏃讹紝鑷姩妫€娴嬮噸澶嶉敭浣嶅苟杈撳嚭鍐茬獊鏃ュ織銆?


### 鎭㈠榛樿鑳藉姏

- `HotkeyManager` 鏂板 `resetToDefaults()`锛岀粺涓€鍥為€€榛樿閿綅銆?
- 鏂板閰嶇疆寮€鍏?`hotkey.restore_defaults`锛?
  - 璁剧疆涓?`true` 鍚庯紝涓嬩竴娆″惎鍔ㄨ嚜鍔ㄦ仮澶嶉粯璁ら敭浣嶏紱

  - 鎭㈠瀹屾垚鍚庤嚜鍔ㄥ洖鍐欎负 `false`锛岄伩鍏嶉噸澶嶈Е鍙戙€?
- 鍙戠幇鍐茬獊鏃惰嚜鍔ㄦ仮澶嶉粯璁ら敭浣嶏紝闃叉杩愯鏈熷姩浣滄涔夈€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `1.3.3 鏀寔閿綅鍐茬獊妫€娴嬩笌鎭㈠榛樿` 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- include/input/hotkey_manager.h

- src/input/hotkey_manager.cpp

- src/main.cpp

- config/player_settings.ini

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堝瓧骞?seek 鍚屾楠屾敹锛?


### 楠屾敹涓庡彲鍥炲綊鑳藉姏

- 鏂板 `--subtitle-sync-check <subtitle.srt>` 鍛戒护锛岀敤浜庤嚜鍔ㄩ獙璇佸瓧骞曟椂闂磋酱鍖归厤銆?
- 妫€鏌ヨ鐩栦袱绫诲満鏅細

  - 椤哄簭鎾斁鏃堕棿杞达紙ordered锛夛紱

  - 闈為『搴?seek 璺宠浆锛坰eek锛夈€?
- 杈撳嚭 `mismatches` 涓?`PASS/FAIL`锛岀敤浜?M1 楠屾敹 `1.4.1`銆?


### 浠ｇ爜缁撴瀯璋冩暣

- 鎻愬彇瀛楀箷鏃堕棿杞村尮閰嶅叕鍏卞嚱鏁帮細

  - `include/subtitle/subtitle_timeline.h`

  - `src/subtitle/subtitle_timeline.cpp`

- `PlayerCore` 澶嶇敤缁熶竴鍖归厤鍑芥暟锛岄伩鍏嶈繍琛岃矾寰勪笌楠屾敹璺緞绠楁硶鍒嗗弶銆?


### 鏍蜂緥涓庢姤鍛?
- 鏂板鏍蜂緥瀛楀箷锛歚samples/subtitle/subtitle_seek_sync_sample.srt`

- 鏂板鏈湴楠岃瘉鎶ュ憡锛歚docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚1.4.1` 宸插畬鎴愩€?


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



## 2026-03-08 鏇存柊锛堟挱鏀惧垪琛?5 鏂囦欢楠屾敹锛?


### 楠屾敹鑳藉姏

- 鏂板 `--playlist-flow-check` 鍛戒护鐢ㄤ簬 `1.4.2` 鑷姩楠屾敹銆?
- 楠屾敹鍖呭惈锛?
  - 鑷冲皯 5 鏉℃挱鏀惧垪琛ㄨ緭鍏ユ牎楠岋紱

  - 鍓?5 鏉″獟浣撳彲鎵撳紑妫€鏌ワ紱

  - EOF 鑷姩鍒囨崲椤哄簭瑕嗙洊 `0 -> 1 -> 2 -> 3 -> 4`銆?


### 杈撳嚭涓庢姤鍛?
- 鍛戒护杈撳嚭 `playlist-flow-check.*` 瀛楁锛屽寘鍚?`PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚1.4.2` 宸插畬鎴愩€?


### 淇敼鏂囦欢

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md

- samples/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堣缃噸鍚仮澶嶉獙鏀讹級



### 楠屾敹鑳藉姏

- 鏂板 `--settings-persistence-check [settings_file]`锛岀敤浜?`1.4.3` 鑷姩楠屾敹銆?
- 楠屾敹娴佺▼锛氬啓鍏ヨ缃?-> 閲嶈浇璁剧疆 -> 瀛楁涓€鑷存€ф瘮瀵广€?


### 鏍￠獙瑕嗙洊瀛楁

- `player.volume_percent`

- `player.playback_speed`

- `player.resume_last_playlist`

- `player.last_playlist_index`

- `hotkey.toggle_subtitle`



### 杈撳嚭涓庢姤鍛?
- 鍛戒护杈撳嚭 `settings-persistence-check.*` 瀛楁鍜?`PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚1.4.3` 宸插畬鎴愩€?


### 淇敼鏂囦欢

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堝鍣ㄧ煩闃佃ˉ榻愶級



### 瑕嗙洊鑼冨洿

- 瀹屾垚 `2.1.2` 瀹瑰櫒楠屾敹瑕嗙洊锛歚mp4/mkv/mov/avi/webm/flv/ts/m2ts`銆?


### 鍥炲綊鏍锋湰鎵╁睍

- `format_samples.csv` 鏂板涓夋潯鏍锋湰锛?
  - `samples/mov/demo__h264_aac__1920x1080__30fps__2ch.mov`

  - `samples/avi/demo__h264_mp3__1280x720__30fps__2ch.avi`

  - `samples/m2ts/demo__h264_ac3__1920x1080__30fps__2ch.m2ts`



### 鑷姩鐢熸垚鑴氭湰鎵╁睍

- `download_test_samples.ps1` 鏂板 `mov/avi/m2ts` 鐩綍涓庣敓鎴愭祦绋嬨€?


### 鏈湴鍥炲綊缁撴灉

- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=12, PASS=12, PARTIAL=0, FAIL=0, SKIP=0



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚2.1.2` 宸插畬鎴愩€?


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



## 2026-03-08 鏇存柊锛堣棰戠紪鐮佺煩闃佃ˉ榻愶級



### 瑕嗙洊鑼冨洿

- 瀹屾垚 `2.1.3` 瑙嗛缂栫爜楠屾敹瑕嗙洊锛歚H.264/H.265/VP9/AV1/MPEG-2`銆?


### 鍥炲綊鏍锋湰鎵╁睍

- `format_samples.csv` 鏂板锛?
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`



### 鑷姩鐢熸垚鑴氭湰鎵╁睍

- `download_test_samples.ps1` 鏂板 MPEG-2 瑙嗛鏍锋湰鐢熸垚娴佺▼锛坄mpeg2video + ac3 + ts`锛夈€?


### 鏈湴鍥炲綊缁撴灉

- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=13, PASS=13, PARTIAL=0, FAIL=0, SKIP=0



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚2.1.3` 宸插畬鎴愩€?


### 淇敼鏂囦欢

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堥煶棰戠紪鐮佺煩闃佃ˉ榻愶級



### 瑕嗙洊鑼冨洿

- 瀹屾垚 `2.1.4` 闊抽缂栫爜楠屾敹瑕嗙洊锛歚AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`銆?


### 鍥炲綊鏍锋湰鎵╁睍

- 鏂板 4 鏉￠煶棰戠紪鐮佹牱鏈細

  - `samples/mkv/demo__h264_eac3__1920x1080__30fps__2ch.mkv`

  - `samples/mkv/demo__h264_dts__1920x1080__30fps__2ch.mkv`

  - `samples/webm/demo__vp9_vorbis__1920x1080__30fps__2ch.webm`

  - `samples/mov/demo__h264_pcm_s16le__1920x1080__30fps__2ch.mov`



### 鑴氭湰澧炲己

- `download_test_samples.ps1` 澧炲姞 E-AC3/DTS/Vorbis/PCM 鏍锋湰鐢熸垚銆?
- DTS (`dca`) 缂栫爜娣诲姞 `-strict -2`銆?
- `run_format_regression.ps1` 澧炲姞缂栫爜鍚嶇瓑浠峰尮閰嶏細

  - `dts` <-> `dca`

  - `hevc` <-> `h265`

  - `pcm` <-> `pcm_*`



### 鏈湴鍥炲綊缁撴灉

- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=17, PASS=17, PARTIAL=0, FAIL=0, SKIP=0



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛歚2.1.4` 宸插畬鎴愩€?


### 淇敼鏂囦欢

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- tools/format_regression/run_format_regression.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛圖ecoderFactory 鍒濆鍖栨祦绋嬫帴鍏ワ級



### 瑙ｇ爜鍒濆鍖栭摼璺粺涓€

- `DecoderFactory` 鏂板 `selectBackendOrder(codec_name, prefer_hardware)`锛岃緭鍑哄悗绔€欓€夊簭鍒楀苟淇濈暀杞欢瑙ｇ爜鍏滃簳銆?
- `PlayerCore::initDecoders` 鎺ュ叆鍊欓€夊簭鍒楋紝鎸夐『搴忓皾璇曞悗绔垵濮嬪寲涓?`avcodec_open2`锛屽け璐ヨ嚜鍔ㄥ洖閫€涓嬩竴涓€欓€夈€?
- `tryConfigureD3D11HardwareDecode` 璋冩暣涓虹函 D3D11 閰嶇疆鍑芥暟锛屽悗绔瓥鐣ョ粺涓€鐢?`DecoderFactory` 鍐冲畾銆?


### 閰嶇疆鍏煎

- 淇濇寔 `decoder.prefer_hardware_decode` 閰嶇疆椤圭敓鏁堬細

  - `true`锛氫紭鍏堢‖瑙ｅ€欓€夛紝鍐嶅洖閫€杞В锛?
  - `false`锛氱洿鎺ヨ蛋杞欢瑙ｇ爜鍊欓€夈€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `3.1.1 DecoderFactory 鎺ュ叆鐪熷疄鍒濆鍖栨祦绋媊 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- include/decoder/decoder_factory.h

- src/decoder/decoder_factory.cpp

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛圖3D11VA 鍗忓晢澶辫触杞В鍏滃簳锛?


### 鍥為€€閾捐矾澧炲己

- 鍦?`PlayerCore::selectVideoPixelFormat` 涓紝褰?D3D11VA 鐩爣鍍忕礌鏍煎紡鏈瑙ｇ爜鍣ㄦ彁渚涙椂锛屾樉寮忛檷绾у埌杞欢鍚庣锛?
  - `video_hw_pixel_fmt_` 閲嶇疆涓?`AV_PIX_FMT_NONE`锛?
  - `video_decoder_backend_` 缃负 `Software`銆?
- 鍦ㄨВ鐮佸垵濮嬪寲娴佺▼涓ˉ鍏呭崗鍟嗛檷绾ф棩蹇楋紝渚夸簬瀹氫綅鈥滅‖瑙ｅ垵濮嬪寲鎴愬姛浣嗗崗鍟嗛樁娈靛洖閫€鈥濈殑鍦烘櫙銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `3.1.2 D3D11VA 鍒濆鍖栦笌澶辫触鍥為€€杞В` 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛圖3D11 娓叉煋鏈€灏忓彲鐢ㄩ摼璺級



### 娓叉煋鍚庣鑳藉姏琛ラ綈

- `D3D11VideoRenderer` 瀹屾垚鏈€灏忓彲鐢ㄥ疄鐜帮細

  - `init`锛氬垱寤烘覆鏌撻摼璺紱

  - `renderFrame`锛氭帴鍏ュ抚涓婁紶锛?
  - `present`锛氭帴鍏ュ憟鐜帮紱

  - 浜嬩欢涓庝氦浜掕姹傞€忎紶銆?
- `Display` 鏂板娓叉煋鍚庣鍙娴嬭兘鍔涳細

  - 鍙缃?renderer 椹卞姩鍋忓ソ锛?
  - 鍙煡璇㈠綋鍓嶅疄闄?renderer 鍚庣鍚嶇О銆?


### D3D11 鍚庣鏍￠獙涓庡洖閫€鍗忓悓

- `D3D11VideoRenderer::init` 鍦ㄨ姹?`direct3d11` 鍚庯紝浼氭牎楠屽疄闄?SDL renderer 鍚庣锛?
  - 鍛戒腑 `direct3d11/d3d11`锛氬垵濮嬪寲鎴愬姛锛?
  - 鏈懡涓細鍒濆鍖栧け璐ュ苟鐢变笂灞傝Е鍙?`SoftwareSDL` 鍥為€€閾捐矾锛堜笌 `3.2.2` 鍗忓悓锛夈€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `3.2.1 D3D11 娓叉煋鏈€灏忓彲鐢紙init/upload/present锛塦 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- include/display.h

- src/display.cpp

- include/render/d3d11_video_renderer.h

- src/render/d3d11_video_renderer.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堟覆鏌撳け璐ヨ嚜鍔ㄩ檷绾у洖褰掑叆鍙ｏ級



### 鏂板鍥炲綊鍛戒护

- 鏂板 `--renderer-fallback-check <media_file>`锛?
  - 寮哄埗鏋勯€?D3D11 娓叉煋鍒濆鍖栧け璐ュ満鏅紱

  - 楠岃瘉鏄惁鑷姩鍥為€€ `SoftwareSDL`锛?
  - 杈撳嚭 `renderer-fallback-check.*` 瀛楁鍜?`PASS/FAIL`銆?


### 鍙娴嬭兘鍔涜ˉ榻?
- 娓叉煋鎺ュ彛鏂板鍚庣鍚嶇О鏌ヨ锛屾挱鏀惧櫒瀵瑰鏂板褰撳墠娓叉煋鍚庣/瑙ｇ爜鍚庣鏌ヨ鎺ュ彛锛岀敤浜庡懡浠ゅ寲楠屾敹杈撳嚭銆?


### 鏈湴鎶ュ憡

- 鏂板 `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`锛岃褰曟湰鍦板洖褰掑懡浠や笌缁撴灉銆?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`锛?
  - `3.3.2 娓叉煋澶辫触闄嶇骇涓嶄腑鏂挱鏀綻 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛圵indows 杞В/纭В涓诲姏鏍锋湰鍥炲綊锛?


### 鍥炲綊鑳藉姏琛ラ綈

- 鏂板 `--windows-backend-session-check <media_file> <hard|soft>`锛氬崟妯″紡浼氳瘽妫€鏌ヤ笌缁撴瀯鍖栬緭鍑恒€?
- 鏂板骞剁ǔ瀹?`--windows-backend-check <media_file>`锛氳嚜鍔ㄨ仛鍚堢‖瑙?杞В缁撴灉骞惰緭鍑?`PASS/FAIL`銆?


### 绋冲畾鎬т慨澶?
- `--windows-backend-check` 浠庡悓杩涚▼鍙屼細璇濇敼涓虹埗杩涚▼鎷夎捣涓や釜瀛愯繘绋嬶紙hard/soft锛夊苟姹囨€伙紝瑙勯伩浼氳瘽鍥炴敹鍗℃銆?
- Windows 瀛愯繘绋嬫墽琛屾敼涓?`CreateProcess`锛岄伩鍏?shell 閲嶅畾鍚戣娉曞樊寮傚鑷村け璐ャ€?


### 鏈湴楠岃瘉

- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `3.3.1 Windows 涓嬭蒋瑙?纭В涓诲姏鏍锋湰鍙挱` 鏍囪瀹屾垚銆?
  - `3.3.3 鍥炲綊鎶ュ憡瀹屾暣` 鏍囪瀹屾垚銆?


### 鏂板鎶ュ憡

- `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`



## 2026-03-08 鏇存柊锛堢珷鑺傚鑸細涓婁竴绔?涓嬩竴绔狅級



### 浜や簰鑳藉姏澧炲己

- 鏂板绔犺妭瀵艰埅蹇嵎閿細

  - `HOME`锛氳烦杞笂涓€绔狅紱

  - `END`锛氳烦杞笅涓€绔犮€?
- 绔犺妭璇锋眰閾捐矾宸叉墦閫氾細

  - `Display -> Renderer -> PlayerCore -> VideoPlayer`銆?


### 濯掍綋淇℃伅鑳藉姏澧炲己

- `Demuxer` 鏂板绔犺妭鍏冩暟鎹В鏋愶細

  - `ChapterInfo`锛坰tart/end/title锛夛紱

  - `MediaInfo::chapters`銆?
- `PlayerCore` 鍦ㄦ墦寮€濯掍綋鍚庢瀯寤虹珷鑺傛椂闂寸偣锛屽苟鎻愪緵锛?
  - `seekToNextChapter()`锛?
  - `seekToPreviousChapter()`锛?
  - `chapterCount()`銆?


### 楠屾敹鑳藉姏琛ラ綈

- 鏂板 `--chapter-nav-check <media_file>`锛?
  - 鑷姩鎵ц鎾斁銆佷笅涓€绔犮€佷笂涓€绔犲苟鏍￠獙浣嶇疆鍙樺寲锛?
  - 杈撳嚭 `chapter-nav-check.*` 涓?`PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`銆?


### 鏈湴楠岃瘉

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.1 绔犺妭瀵艰埅锛堜笂涓€绔?涓嬩竴绔狅級` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛圓-B Repeat锛?


### 浜や簰鑳藉姏澧炲己

- 鏂板 A-B Repeat 榛樿鐑敭锛?
  - `A`锛氳缃?A 鐐癸紱

  - `B`锛氳缃?B 鐐瑰苟鍚敤寰幆锛?
  - `C`锛氭竻闄?A-B Repeat銆?
- 甯姪淇℃伅琛ュ厖 `A/B/C` 閿綅璇存槑銆?


### 鎾斁鏍稿績鑳藉姏澧炲己

- `PlayerCore` 鏂板 A-B Repeat 鐘舵€佺鐞嗕笌鏌ヨ锛?
  - `setABRepeatStart()`锛?
  - `setABRepeatEnd()`锛?
  - `clearABRepeat()`锛?
  - `isABRepeatEnabled()`锛?
  - `abRepeatStart()` / `abRepeatEnd()`銆?
- 鏂板鎾斁寰幆妫€娴?`handleABRepeatLoop()`锛?
  - 褰撴挱鏀句綅缃埌杈?B 鐐规椂鑷姩 seek 鍥?A 鐐广€?


### 楠屾敹鑳藉姏琛ラ綈

- 鏂板 `--ab-repeat-check <media_file>`锛?
  - 鑷姩鎵ц鈥滆缃?A 鐐?-> 璁剧疆 B 鐐?-> 瑙傚療寰幆 -> 娓呴櫎鈥濓紱

  - 杈撳嚭 `ab-repeat-check.*` 涓?`PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/AB_REPEAT_LOCAL_CHECK.md`銆?


### 鍥炲綊淇

- 淇 `--settings-persistence-check` 涓庢柊榛樿鐑敭鍐茬獊锛?
  - 娴嬭瘯閿綅鐢?`b` 璋冩暣涓?`x`锛屾仮澶嶅洖褰掔ǔ瀹氭€с€?


### 鏈湴楠岃瘉

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`锛歚PASS`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.2 A-B Repeat` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堟埅鍥撅級



### 浜や簰鑳藉姏澧炲己

- 鏂板绋冲畾鍙敤鐨勬埅鍥捐兘鍔涳細

  - `S` 瑙﹀彂鎴浘锛?
  - 杈撳嚭鏂囦欢鍐欏叆 `screenshots/` 鐩綍锛?
  - 鍛藉悕鏍煎紡涓?`screenshot_YYYYMMDD_HHMMSS_mmm.ppm`銆?


### 鎴浘閾捐矾鏀舵暃

- `PlayerCore` 鏂板鏈€杩戞覆鏌撳抚缂撳瓨锛岀敤浜庡湪鏆傚仠鎬佺洿鎺ヤ繚瀛樺綋鍓嶇敾闈€?
- `requestScreenshot()` 鍖哄垎涓ょ璺緞锛?
  - 鎾斁涓細寮傛绛夊緟褰撳墠/涓嬩竴甯ц惤鐩橈紱

  - 鏆傚仠鎬侊細鐩存帴浠庣紦瀛樺抚钀界洏銆?
- `main` 涓殑 `--screenshot-check` 鏀逛负瑕嗙洊鏆傚仠鎬佹埅鍥惧満鏅€?


### 鏈湴楠岃瘉

- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`锛歚PASS`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.3 鎴浘` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堝抚姝ヨ繘锛?


### 浜や簰鑳藉姏澧炲己

- 鏂板鏆傚仠鎬佸抚姝ヨ繘锛?
  - `,` 鍚庨€€涓€甯э紱

  - `.` 鍓嶈繘涓€甯э紱

  - 姝ヨ繘鍚庝粛淇濇寔鏆傚仠鐘舵€併€?


### 涓婚摼瀹炵幇

- `PlayerCore` 鏂板 `stepFrameBackward()` / `stepFrameForward()`锛岄噰鐢ㄢ€滄殏鍋滄€?seek + 棣栧抚鍒锋柊鈥濇敹鏁涘崟甯ф杩涖€?
- 姝ヨ繘鏃跺鐢ㄦ渶杩戞覆鏌撳抚鏃堕暱涓庡獟浣?FPS 浼扮畻姝ラ暱锛屽苟鍦?seek 鍚庝富鍔ㄦ覆鏌撶洰鏍囨椂闂寸偣鐨勯涓棰戝抚銆?
- 闊抽娑堣垂绾跨▼鍦ㄦ殏鍋滄€佷笉鍐嶇敤鏃?`playback_pts` 鍥炲啓褰撳墠浣嶇疆锛岄伩鍏嶅抚姝ヨ繘鍚庝綅缃闊抽鏃堕挓鍥炴媺銆?
- `main` 鏂板 `--frame-step-check <media_file>` 鑷鍛戒护銆?


### 鏈湴楠岃瘉

- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.4 甯ф杩涳紙鏆傚仠鎬侊級` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堝樊璺濊瘎浼版枃妗ｅ榻愶級



### 鏂囨。鍩虹嚎鍒锋柊

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 鏇存柊涓烘埅鑷?`2026-03-08` 鐨勫疄鐜扮姸鎬併€?
- 璇勪及渚濇嵁浠庘€滃彧鐪嬫帴鍙?楠ㄦ灦鈥濆崌绾т负鈥滀唬鐮佸叆鍙?+ 鏈湴楠屾敹鎶ュ憡鈥濊仈鍚堝垽鏂€?


### 缁撹淇

- 灏嗕互涓嬫ā鍧椾粠鏃х増鐨勨€滈鏋?鏈帴鍏モ€濈籂姝ｄ负鈥滈儴鍒嗗疄鐜扳€濓細

  - 瀛楀箷绯荤粺锛?
  - 鎾斁鍒楄〃锛?
  - 瑙ｇ爜鍣ㄧ鐞嗭紱

  - 蹇嵎閿郴缁燂紱

  - 璁剧疆绯荤粺銆?
- 妯″潡缁熻鏇存柊涓猴細

  - `杈惧埌 MPC-HC 绛夌骇: 0 / 14`

  - `閮ㄥ垎瀹炵幇: 11 / 14`

  - `楠ㄦ灦/鏈帴鍏? 3 / 14`



### 鍓╀綑閲嶇偣鏇存柊

- `P0` 鑱氱劍杞负锛?
  - `1080p60 / 4K / 楂樼爜鐜嘸 绋冲畾鎬т笌鎬ц兘鏃ュ織锛?
  - 澶氶煶杞?瀛楀箷杞?鏃跺欢璋冭妭锛?
  - `OpenGL` 鍚庣绛栫暐鏀舵暃锛?
  - 鍊嶉€熼煶棰戠瓥鐣ョ户缁畬鍠勩€?
- `P1` 鑱氱劍锛氬叏閲忓揩鎹烽敭銆佺敾骞?缂╂斁/鏃嬭浆銆佹护闀滅敤鎴峰叆鍙ｃ€?
- `P2` 鑱氱劍锛氭彃浠躲€佹祦濯掍綋銆佺毊鑲ょ郴缁熶骇鍝佸寲銆?


### 淇敼鏂囦欢

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 鏇存柊锛堢増鏈枃妗ｅ巻鍙叉钀芥竻鐞嗭級



### 鍘嗗彶绔犺妭鍘绘涔?
- 灏嗏€滈樁娈典竴锛氬熀纭€鎾斁鍣紙褰撳墠闃舵锛夆€濊皟鏁翠负鈥滈樁娈典竴锛氬熀纭€鎾斁鍣紙鍘嗗彶璧风偣锛夆€濓紝鏄庣‘璇ユ璁板綍鐨勬槸 `2026-02-17 ~ 2026-02-25` 鐨勬棭鏈熷疄鐜板熀绾裤€?
- 涓洪樁娈典竴琛ュ厖璇存槑锛氭棫鐗?`decoder / playLoop` 璺緞宸插湪 `2026-03-06` 鏋舵瀯鏀舵暃鍚庡苟鍏?`PlayerCore + Scheduler + core/*` 涓婚摼銆?


### 鏃ц矾寰勮〃杩版竻娲?
- 灏嗘棭鏈?`video_decoder` / `audio_decoder` 浠ュ強 `VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 鐨勬枃浠剁骇琛ㄨ堪鏀瑰啓涓鸿兘鍔涚骇鍘嗗彶璁板綍銆?
- 灏嗏€滀笅涓€姝ヨ鍒掆€濇敼鍐欎负鈥滈樁娈靛悗缁洰鏍囷紙鍘嗗彶璁板綍锛夆€濓紝閬垮厤涓庡綋鍓嶈凯浠ｈ繘搴︾珷鑺傛贩娣嗐€?
- 灏?`USE_NEW_PLAYER_CORE` 鍜屼复鏃?`tests/core_*` 鐨勫巻鍙叉弿杩版敼鍐欎负鈥滄灦鏋勬敹鏁?/ 鍚庣画娓呯悊鈥濆彛寰勩€?


### 淇敼鏂囦欢

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 ?????/???????



### ????

- ?? `J / K` ???????`- / +100ms`??

- ?? `Ctrl+J / Ctrl+K` ???????`- / +100ms`??

- ????/??????????

  - `player.audio_delay_ms`

  - `player.subtitle_delay_ms`



### ?????

- ?????????`--delay-adjust-check <media_file> <subtitle.srt>`?

- ???????

  - `build/Debug/modern-video-player.exe --settings-persistence-check`

  - `build/Debug/modern-video-player.exe --delay-adjust-check .\juren-30s.mp4 .\samples\subtitle\subtitle_seek_sync_sample.srt`

- ?????`docs/reports/DELAY_ADJUST_LOCAL_CHECK.md`



### ??????

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.5 ????/??????` ?????



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



## 2026-03-08 ??????????



### ????

- ?????? `1..9`???????? `10%..90%` ???

- ????????????`seek_to_10_percent` ~ `seek_to_90_percent`?

- `Display` ????????? seek ????????????????



### ?????

- ?????????`--numeric-seek-check <media_file>`?

- ???????

  - `build/Debug/modern-video-player.exe --numeric-seek-check .\juren-30s.mp4`

- ?????`docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md`



### ??????

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.6 ???????A/B/C/S,./J/K/1..9?` ?????



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





## 2026-03-08 鏇存柊锛堟挱鏀炬€ц兘鏃ュ織楠屾敹锛?


### 鍙娴嬫€цˉ榻?
- 鏂板 `core::DiagnosticsSnapshot`锛岀粺涓€瀵煎嚭瑙ｅ皝瑁呭寘璁℃暟銆侀噸璇?涓㈠寘銆佽В鐮佸抚鏁般€佹覆鏌撳抚鏁般€侀煶棰戞彁浜ゅ抚鏁般€佽皟搴﹀櫒鎺夊抚涓庨槦鍒楅暱搴︺€?
- `VideoPlayer` 鏂板 `getInfo()` / `getDiagnosticsSnapshot()` 閫忎紶鎺ュ彛锛屼緵鍛戒护琛岄獙鏀跺拰鍚庣画璇婃柇澶嶇敤銆?
- `main` 鏂板 `--performance-log-check <media_file> [sample_ms]`锛岃緭鍑猴細

  - renderer / decoder backend锛堜綔涓哄綋鍓?GPU 璺緞鏍囪瘑锛夛紱

  - 杩涚▼骞冲潎 CPU 鍗犵敤涓庨€昏緫鏍稿績鏁帮紱

  - demux / decode / render / scheduler / queue 鍏抽敭鎸囨爣銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.4 杈撳嚭鎬ц兘鏃ュ織锛堟帀甯?闃熷垪/CPU/GPU锛塦 鏍囪瀹屾垚銆?


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





## 2026-03-08 鏇存柊锛?080p60 绋冲畾鎾斁楠屾敹锛?


### 绋冲畾鎬ч棬绂佽ˉ榻?
- `main` 鏂板 `--1080p60-check <media_file> [sample_ms]`锛屽鐢?`probe + diagnostics` 鍙ｅ緞楠岃瘉锛?
  - `1920x1080 @ 60fps` 鏍锋湰鏄惁鍖归厤鐩爣锛?
  - 杩炵画鎾斁绐楀彛鍐呮椂闂存槸鍚︽寔缁帹杩涳紱

  - 璋冨害鍣ㄦ槸鍚﹀嚭鐜?`late_drop`锛?
  - 瑙ｅ皝瑁呮槸鍚﹀彂鐢熶涪鍖呫€?
- 鏂板 `samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4` 鐨勬牱鏈敓鎴愯剼鏈叆鍙ｏ紝鐢ㄤ簬鏈湴绋冲畾鎬у洖褰掋€?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.1 1080p60 闀挎椂绋冲畾` 鏍囪瀹屾垚銆?
  - `2.3.2 1080p60 绋冲畾鎾斁杈炬爣` 鏍囪瀹屾垚銆?


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





## 2026-03-08 鏇存柊锛?K 鎾斁涓庨檷绾ч獙鏀讹級



### 4K 闂ㄧ琛ラ綈

- `main` 鏂板 `--4k-playback-check <media_file> [sample_ms]`锛岃仈鍚堥獙璇侊細

  - `4K` 鏍锋湰鍦ㄨ繛缁挱鏀剧獥鍙ｅ唴鏄惁鎸佺画鎺ㄨ繘锛?
  - 褰撳墠鎾斁閾捐矾鏄惁鏃?`late_drop`锛?
  - hard / soft 涓や釜瀛愯繘绋嬩細璇濇槸鍚﹀潎鍙繘鍏ユ挱鏀撅紝璇佹槑纭В澶辫触鏃跺彲闄嶇骇銆?
- 楠屾敹閫昏緫澶嶇敤浜嗗凡鏈?`runBackendSessionSubprocess()`锛岄伩鍏嶉噸澶嶅疄鐜?Windows 鍚庣鍥為€€鏍￠獙銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.2 4K 鍙挱鏀撅紙浼樺厛绋冲畾锛屽啀鎻愭€ц兘锛塦 鏍囪瀹屾垚銆?
  - `2.3.3 4K 鎾斁鍙敤骞跺彲闄嶇骇` 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md





## 2026-03-08 鏇存柊锛堥珮鐮佺巼鏍锋湰楠屾敹锛?


### 楂樼爜鐜囬棬绂佽ˉ榻?
- `main` 鏂板 `--high-bitrate-check <media_file> [sample_ms]`锛屽墠缃鍙栨牸寮忕爜鐜囧苟瑕佹眰 `>= 80Mbps`銆?
- 杩炵画鎾斁绐楀彛鍚屾椂妫€鏌ワ細鏃堕棿鎺ㄨ繘銆乣late_drop`銆乨emux 涓㈠寘涓庡疄闄?decoder / renderer backend銆?
- `tools/download_test_samples.ps1` 鏂板 `stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 鐢熸垚鍏ュ彛锛岀敤浜庨珮鐮佺巼鍥炲綊銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.3 澶х爜鐜囨牱鏈紙>80Mbps锛夊彲鎾斁` 鏍囪瀹屾垚銆?


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





## 2026-03-08 鏇存柊锛堥暱鏃舵挱鏀剧ǔ瀹氭€ч獙鏀讹級



- `main` 鏂板 `--long-playback-check <media_file> [sample_ms]`锛岃姹傞噰鏍风獥鍙ｈ嚦灏?`5000ms`锛屽苟杈撳嚭 `probe`銆乥ackend銆佹椂闂存帹杩涗笌鎺夊抚/涓㈠寘闂ㄧ瀛楁銆?
- 杩炵画鎾斁绐楀彛鍚屾椂妫€鏌ワ細`open_ok`銆佽繘鍏ユ挱鏀惧惊鐜€佺獥鍙ｇ粨鏉熷悗浠嶅浜庢挱鏀炬€併€乣advance_ratio` 杈炬爣銆乣scheduler_late_drops=0` 涓?`demux_dropped_packets=0`銆?
- 鏂板 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`锛屽苟鍚屾鍙戝竷闂ㄧ `6.1 ~ 6.6` 涓哄凡瀹屾垚銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `6.1 鍔熻兘锛歁1/M2 蹇呴』鍏ㄩ儴閫氳繃` 鏍囪瀹屾垚銆?
  - `6.2 鏍煎紡锛氫富鍔涙牸寮忕煩闃垫湁缁撴灉涓斿彲瑙ｉ噴` 鏍囪瀹屾垚銆?
  - `6.3 鍒嗚鲸鐜囷細1080p60 绋冲畾锛?K 鍙敤骞跺彲闄嶇骇` 鏍囪瀹屾垚銆?
  - `6.4 浜や簰锛氶粯璁ゅ揩鎹烽敭瀹屾暣鍙敤` 鏍囪瀹屾垚銆?
  - `6.5 绋冲畾鎬э細闀挎椂鎾斁鏃?crash` 鏍囪瀹屾垚銆?
  - `6.6 鍙娴嬶細鍏抽敭鏃ュ織瀹屾暣` 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 2026-03-08 鏇存柊锛堟彃浠剁郴缁燂級



- 鏂板 `include/plugin/plugin_api.h` 涓庨噸鍐?`PluginManager`锛岃ˉ榻?`DLL` 鍔ㄦ€佸姞杞姐€乣API` 鐗堟湰鏍￠獙銆乣load/unload` 鐢熷懡鍛ㄦ湡涓庡紓甯镐繚鎶ゃ€?
- `PluginManager` 鐜板彲浣滀负鎻掍欢瀹夸富娉ㄥ唽澶栭儴瑙嗛/闊抽婊ら暅宸ュ巶锛沗FilterRegistry` 鍚屾琛ュ厖娉ㄩ攢鎺ュ彛锛屾敮鎸佸嵏杞芥椂娓呯悊鎵╁睍鐐广€?
- 鏂板 `sample_logger_plugin` 绀轰緥 `DLL` 涓?`--plugin-check [plugin_dir_or_file]` 楠屾敹鍛戒护锛岄獙璇佹彃浠跺姞杞姐€佹护闀滄敞鍐屼笌鍗歌浇娓呯悊闂幆銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --plugin-check`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.1 鎻掍欢绯荤粺` 鏍囪瀹屾垚銆?


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



## 2026-03-08 鏇存柊锛堟祦濯掍綋 HTTP 鍒嗙墖涓庣紦鍐诧級



- 閲嶅啓 `HttpStreamDownloader`锛氬熀浜?FFmpeg `avio` 鏀寔鐪熷疄 HTTP 鎵撳紑銆佸垎鍧楄鍙栥€佸唴閮ㄧ紦鍐层€丒OF 鐘舵€佷笌閿欒閫忎紶銆?
- `main` 鏂板 `--streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]`锛屽彲涓嬭浇 HLS 濯掍綋娓呭崟銆佽В鏋愮浉瀵瑰垎鐗?URL锛屽苟楠岃瘉鍒嗙墖缂撳啿闂幆銆?
- 鏂板鏈湴澶瑰叿 `samples/streaming/hls_local/*` 涓?`tools/start_streaming_fixture_server.ps1`锛岀敤浜庡湪鏈満鍚姩 HTTP 闈欐€佹湇鍔″苟澶嶇幇瀹為獙銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.2 娴佸獟浣擄紙鐪熷疄 HTTP 鍒嗙墖涓庣紦鍐诧級` 鏍囪瀹屾垚銆?


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





## 2026-03-08 鏇存柊锛圚LS/DASH 鑷€傚簲鐮佺巼锛?


- 鎵╁睍 `HlsManifestParser`锛氳瘑鍒?`#EXT-X-STREAM-INF` master playlist銆佸鐮佺巼 variant 涓庡獟浣撴挱鏀惧垪琛ㄣ€?
- 鎵╁睍 `DashManifestParser`锛氳В鏋?`Representation` 甯﹀銆乣BaseURL`銆乣Initialization` 涓?`SegmentURL` 鏄庣粏銆?
- 鏂板 `AdaptiveBitrateSelector`锛屾寜浼扮畻甯﹀閫夋嫨鏈€鍚堥€傜爜鐜囷紝骞跺湪 `main` 涓彁渚?`--adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]` 鏈湴楠屾敹鍏ュ彛銆?
- 鏂板 `samples/streaming/abr_local/{hls,dash}` 澶瑰叿锛屽苟鎵╁睍 `tools/start_streaming_fixture_server.ps1` 鏀寔 `mpd/m4s/mp4` 鍐呭绫诲瀷銆?


### 鏈湴楠岃瘉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`锛歚PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`锛歚PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`锛歚PASS`

- 楠屾敹鎶ュ憡锛歚docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.3 HLS/DASH 鑷€傚簲鐮佺巼` 鏍囪瀹屾垚銆?


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

## 2026-03-08 鏇存柊锛堝缓绔嬮噷绋嬬鏍囩锛?


- 瀹屾垚浠诲姟娓呭崟 `0.4`锛屽缓绔嬮噷绋嬬鏍囩 `v0.2.0-rc1` 涓?`v0.2.0`銆?
- `v0.2.0-rc1` 鐢ㄤ簬鍐荤粨鍙戝竷闂ㄧ宸叉敹鍙ｇ殑鍊欓€夊揩鐓э紱`v0.2.0` 鐢ㄤ簬鏍囪褰撳墠涓荤嚎閲岀▼纰戝畬鎴愮偣銆?
- 鍚屾鍒锋柊宸窛璇勪及涓庝换鍔℃竻鍗曠姸鎬侊紝閬垮厤鏂囨。浠嶄繚鐣欌€滀粎宸爣绛炬搷浣溾€濈殑鏃у彛寰勩€?
- 鐢变簬閲岀▼纰戝凡鍏峰鍙拷婧殑 `RC` 鏍囩锛屼换鍔℃竻鍗曚腑鐨?`5.3 姣忎釜閲岀▼纰戠粨鏉熷墠蹇呴』鍙墦 RC 鏍囩` 涔熷悓姝ユ弧瓒炽€?


### 鏍囩璇存槑

- `v0.2.0-rc1`锛氬熀浜庡彂甯冮棬绂侀棴鐜悗鐨勭ǔ瀹氬揩鐓у缓绔嬨€?
- `v0.2.0`锛氬熀浜庡綋鍓?`main` 鐨勯噷绋嬬鏀跺彛蹇収寤虹珛銆?


### 鏈湴楠岃瘉

- `git tag --list`锛氱‘璁ゆ爣绛惧凡瀛樺湪銆?
- `git show v0.2.0-rc1 --no-patch --stat`

- `git show v0.2.0 --no-patch --stat`



### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `0.4 寤虹珛閲岀▼纰戞爣绛撅紙v0.2.0-rc1 / v0.2.0锛塦 鏍囪瀹屾垚銆?
  - `5.3 姣忎釜閲岀▼纰戠粨鏉熷墠蹇呴』鍙墦 RC 鏍囩` 鏍囪瀹屾垚銆?


### 淇敼鏂囦欢

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-08 鏇存柊锛堟墽琛屽畧鍒欏彛寰勫悓姝ワ級



- 缁撳悎鏈疆鎻愪氦涓庝换鍔℃帹杩涢『搴忥紝`5.1 WIP 闄愬埗锛氬悓鏃惰繘琛屼换鍔′笉瓒呰繃 2 涓猔 鍙垽瀹氫负婊¤冻锛屽苟鍚屾鍕鹃€夈€?
- `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 灞炰簬鎸夊懆鎵ц鑺傚绾︽潫锛屽綋鍓嶄粨搴撲腑灏氱己鎸佺画鎬х殑鍛ㄧ淮搴﹁瘉鎹紝鏆備繚鎸佹湭鍕鹃€夈€?


### 鍙ｅ緞璇存槑

- `5.1` 鐨勫垽鏂緷鎹槸鏈疆浠诲姟鎸夊崟涓荤嚎涓茶鎺ㄨ繘锛氬彂甯冮棬绂併€佹彃浠剁郴缁熴€佹祦濯掍綋缂撳啿銆丄BR銆侀噷绋嬬鏍囩涓庡畧鍒欏彛寰勪緷娆℃敹鍙ｃ€?
- `5.2` 闇€瑕佽法鍛ㄣ€侀噸澶嶆墽琛岀殑杩囩▼璇佹嵁锛屼笉閫傚悎浠呭嚟褰撳墠涓€娆℃敹鍙ｇ粨鏋滅洿鎺ュ垽瀹氬畬鎴愩€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `5.1 WIP 闄愬埗锛氬悓鏃惰繘琛屼换鍔′笉瓒呰繃 2 涓猔 鏍囪瀹屾垚銆?
  - `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 淇濇寔寰呭畬鎴愩€?


### 淇敼鏂囦欢

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 闂 63: 钀藉湴 5.2 鍛ㄤ簲鏀舵暃鏃ユ墽琛屾墜鍐?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 瀹堝垯椤?`5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 鐩墠鍙湁鍘熷垯鎻忚堪锛岀己灏戝彲閲嶅鎵ц鐨勬祦绋嬨€佽竟鐣屽拰杈撳嚭鐗╁畾涔夈€?


### 鍒嗘瀽璁板綍

1. `docs/plans/MPC_HC_ITERATION_PLAN.md` 鍙粰鍑衡€滄瘡鍛ㄤ簲鍥哄畾鍋氭敹鏁涙棩鈥濈殑鑺傚寤鸿锛屼絾娌℃湁钀藉疄鎴愰€愭鎿嶄綔鎵嬪唽銆?
2. `docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md` 宸茶鐩栤€滃浣曞仛鍥炲綊鈥濓紝浣嗚繕娌℃湁鍥炵瓟鈥滃懆浜斿綋澶╁厑璁稿仛浠€涔堛€佺姝㈠仛浠€涔堛€佷粈涔堟椂鍊欏彲浠ュ緱鍑?RC 缁撹鈥濄€?
3. 鍥犳褰撳墠鏈€鍚堥€傜殑鍔ㄤ綔鏄厛鎶?`5.2` 钀芥垚娴佺▼鏂囨。锛屽啀绛夊緟鍚庣画璺ㄥ懆鎵ц璇佹嵁鍐冲畾鏄惁鍕鹃€夈€?


### 瑙ｅ喅鏂规

- 鏂板 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`锛屾槑纭懆鑺傚銆佸懆浜斿厑璁?绂佹浜嬮」銆佹墽琛岄『搴忋€佹帹鑽愬懡浠ゃ€佽緭鍑虹墿涓庡嬀閫夊彛寰勩€?
- 鏇存柊 `docs/README.md` 鍏ュ彛锛岀‘淇濊娴佺▼鏂囨。鍙洿鎺ユ绱€?
- 鍚屾鐗堟湰/鍙樻洿/寮€鍙戣褰曪紝鏄庣‘ `5.2` 宸叉湁鎵ц鎵嬪唽锛屼絾浠诲姟娓呭崟浠嶄繚鎸佸緟瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- 鏂版枃妗ｅ凡瑕嗙洊鈥滃懆涓€鑷冲懆鍥涘紑鍙戙€佸懆浜斿彧鍋氭敹鏁涒€濈殑鑺傚璇存槑銆?
- 鏂版枃妗ｅ凡鏄庣‘鈥滃厑璁镐簨椤?/ 绂佹浜嬮」 / 杈撳嚭鐗?/ RC 鍑嗗缁撹鈥濈殑鎵ц杈圭晫銆?
- 浠诲姟娓呭崟 `5.2` 鏈浠嶆湭鍕鹃€夛紝寰呭悗缁舰鎴愯法鍛ㄦ墽琛岃瘉鎹悗鍐嶅洖鍐欍€?


### 淇敼鏂囦欢

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂 64: 琛ラ綈 5.2 鐣欑棔妯℃澘锛坉aily_board / 鍛ㄦ姤锛?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `5.2` 宸叉湁娴佺▼鎵嬪唽锛屼絾杩樼己灏戝彲鐩存帴濉啓鐨勨€滄棩鐪嬫澘璁板綍鍗?+ 鍛ㄦ姤妯℃澘鈥濓紝涓嶅埄浜庡悗缁矇娣€璺ㄥ懆鎵ц璇佹嵁銆?


### 鍒嗘瀽璁板綍

1. 浠呮湁 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`锛岃繕涓嶈冻浠ユ敮鎾戞瘡鍛ㄥ疄闄呮墽琛屾椂鐨勪綆鎴愭湰鐣欑棔銆?
2. `daily_board.md` 褰撳墠鍙湁浠诲姟瀹夋帓锛屾病鏈変负涓や釜鍛ㄤ簲棰勭暀鍥哄畾濉啓浣嶇疆銆?
3. 鍥犳闇€瑕佸悓鏃惰ˉ榻愨€滃懆浜斿綋鏃ヨ褰曞崱鈥濆拰鈥滄瘡鍛ㄦ眹鎬绘ā鏉库€濅袱涓浇浣撱€?


### 瑙ｅ喅鏂规

- 鏇存柊 `.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`锛屼负 Day 5 / Day 10 澧炲姞鏀舵暃鏃ヨ褰曞崱銆?
- 鏂板 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`锛屼綔涓?`5.2` 鐨勫懆鎶ョ暀鐥曟ā鏉裤€?
- 鍚屾 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`銆乣docs/README.md` 涓庣浉鍏宠褰曟枃妗ｏ紝鏄庣‘杩欎袱涓ā鏉跨殑鍏ュ彛鍜岀敤閫斻€?


### 鏈湴楠屾敹缁撴灉

- `daily_board.md` 宸插彲鐩存帴濉啓鍛ㄤ簲鐨勮寖鍥村喕缁撱€佸洖褰掑懡浠ゃ€乥locker 缁撹銆佹枃妗ｅ悓姝ュ拰闃舵缁撹銆?
- `weekly_report_template.md` 宸插彲鐢ㄤ簬澶嶅埗鐢熸垚姣忓懆鍛ㄦ姤瀹炰緥锛屽苟娌夋穩 `5.2` 鐨勮法鍛ㄨ瘉鎹€?
- 浠诲姟娓呭崟 `5.2` 鏈浠嶆湭鍕鹃€夛紝浠呰ˉ榻愮暀鐥曟ā鏉裤€?


### 淇敼鏂囦欢

- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md

- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂 65: 姹囨€诲綋鍓嶅姛鑳姐€佷娇鐢ㄦ柟寮忎笌楠岃瘉鍏ュ彛



**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 褰撳墠浠撳簱宸茬粡鍏峰杈冨鎾斁鍣ㄨ兘鍔涗笌楠屾敹鍛戒护锛屼絾鍔熻兘娓呭崟銆佷娇鐢ㄦ柟寮忓拰楠岃瘉鍏ュ彛鍒嗘暎鍦?`README`銆乣reports`銆乣tasklist` 涓?`main.cpp` 甯姪杈撳嚭涓紝缂哄皯涓€浠界粺涓€鎬昏銆?


### 鍒嗘瀽璁板綍

1. 鐢ㄦ埛鍙洿鎺ヤ娇鐢ㄧ殑鍔熻兘锛屼笌寮€鍙?楠屾敹鍚戝懡浠ゆ贩鏉傚湪澶氫釜鏂囨。鍜屾簮鐮佸府鍔╄緭鍑轰腑锛屼笉鍒╀簬蹇€熺悊瑙ｂ€滅▼搴忕幇鍦ㄥ埌搴曡兘鍋氫粈涔堚€濄€?
2. 鏍?`README.md` 鍙繚鐣欎簡杈冩棭鏈熺殑鍔熻兘涓庝娇鐢ㄧず渚嬶紝鏃犳硶瀹屾暣瑕嗙洊褰撳墠涓荤嚎鑳藉姏銆?
3. 鍥犳闇€瑕佹柊澧炰竴浠芥€昏鏂囨。锛屾妸鈥滃姛鑳姐€佺敤娉曘€侀獙璇佲€濋泦涓暣鐞嗭紝骞跺悓姝ュ叆鍙ｇ储寮曘€?


### 瑙ｅ喅鏂规

- 鏂板 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`锛岀郴缁熸暣鐞嗗綋鍓嶅姛鑳姐€佷娇鐢ㄦ柟寮忋€侀厤缃柟寮忋€侀獙璇佸懡浠や笌鎶ュ憡鏄犲皠銆?
- 鏇存柊 `docs/README.md`锛屼负璇ユ€昏鏂囨。澧炲姞鍏ュ彛锛屽苟鍦ㄦ洿鏂板巻鍙蹭腑璁拌处銆?
- 鏇存柊鏍?`README.md`锛岃ˉ鍏呭埌褰撳墠鎬昏鏂囨。鐨勫叆鍙ｏ紝閬垮厤浣跨敤璇存槑缁х画鍋滅暀鍦ㄦ棫鍙ｅ緞銆?


### 鏈湴楠屾敹缁撴灉

- 鏂版枃妗ｅ凡瑕嗙洊鈥滅敤鎴峰彲鐩存帴浣跨敤鍔熻兘 / 寮€鍙戦獙鏀惰兘鍔?/ 浣跨敤鏂瑰紡 / 楠岃瘉娴佺▼ / 褰撳墠杈圭晫鈥濄€?
- 鏂囨。涓凡缁欏嚭 `--capabilities`銆乣--probe-file`銆乣--evaluate-target` 浠ュ強涓撻」楠屾敹鍛戒护鐨勭粺涓€鍏ュ彛銆?
- 褰撳墠浠撳簱娌℃湁鏂板浠ｇ爜鏀瑰姩锛屾湰娆″彧琛ラ綈鍔熻兘涓庝娇鐢ㄨ鏄庢枃妗ｃ€?


### 淇敼鏂囦欢

- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md

- docs/README.md

- README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md







## 闂 70: PlayerCore 鐘舵€佹満閲嶈璁＄涓€闃舵



**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰鍏堝仛鎾斁鍣ㄥ唴鏍哥姸鎬佹満閲嶈璁＄涓€闃舵锛屼笉鍔?serial銆佷笉鍔?copy-back/SoftwareSDL锛屼笉鏀瑰閮?`PlaybackState` 鎺ュ彛銆?
- 鐩爣鏄妸 UI 鎾斁鎬佸拰鍐呮牳浼氳瘽/娴佹按绾挎€佹媶寮€锛屽苟鎶婃暎钀藉湪 `PlayerCore` 鍚勫叆鍙ｉ噷鐨勭姸鎬佹敼鍐欐敹鍙ｅ埌缁熶竴 transition 鍏ュ彛銆?


### 鍒嗘瀽璁板綍

1. 褰撳墠 `PlayerCore` 鐨?`open / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 閮戒細鐩存帴鍐?`state_`锛岀姸鎬佸彉鍖栨病鏈夊崟涓€鍏ュ彛銆?
2. `deferred stop` 涔嬪墠鏄嫭绔嬪竷灏旇涔夛紝瀵艰嚧杩愯鎬佸拰 deferred stop 鏃佽矾鎬佸垎瑁傘€?
3. `Scheduler` 鐩墠鍙湁 `running_ / paused_`锛岀涓€闃舵涓嶅疁杩囨棭濉炲叆鏇村涓氬姟璇箟锛屽簲璇ュ厛鐢?`PlayerCore` 缁熶竴缁存姢浼氳瘽鎬併€佽繍琛屾€佸拰娴佹按绾挎€併€?


### 瑙ｅ喅鏂规

- 鍦?`PlayerCore` 鍐呴儴鏂板 `SessionState / RunState / PipelinePhase` 涓?`CoreStateSnapshot`銆?
- 鏂板 `transitionSessionState / transitionRunState / transitionPipelinePhase / publishPlaybackStateFromInternalState`锛屽苟鍔犲叆闈炴硶杩佺Щ淇濇姢鍜岀姸鎬佽縼绉绘棩蹇椼€?
- 灏?`eof_reached / pending_seek / deferred_stop_pending` 鏀跺洖鍒扮粺涓€蹇収绠＄悊銆?
- 鎶?`open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 鏀逛负鍙€氳繃 transition helper 鍙樻洿鐘舵€侊紱瀵瑰 `PlaybackState` 浠嶄繚鎸佸吋瀹广€?
- 鏂板鍒嗘瀽鏂囨。 `docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md`锛岃褰曠涓€闃舵杈圭晫銆丼cheduler 澶勭悊缁撹鍜屼笅涓€闃舵 serial 鍖栧缓璁€?


### 鏈湴楠屾敹缁撴灉

- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃銆?


### 淇敼鏂囦欢

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂 81: PlayerCore seek/flush timeline serial 鍖栫浜岄樁娈?


**鏃ユ湡**: 2026-03-20

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画鍋氭挱鏀惧櫒鍐呮牳鐘舵€佹満閲嶈璁＄浜岄樁娈碉紝浣嗘槑纭笉鍔?copy-back銆丼oftwareSDL銆乁I 灞傚拰澶栭儴 `PlaybackState`锛屽彧澶勭悊 seek/flush 鐨?timeline serial 鍖栥€?
- 鐩爣鏄 seek/stop/deferred stop 涔嬪悗鐨勬棫 packet銆佹棫瑙嗛甯с€佹棫闊抽甯у嵆浣挎櫄鍒帮紝涔熷繀椤诲洜涓?serial 涓嶅尮閰嶈€岃纭涪寮冦€?


### 鍒嗘瀽璁板綍

1. `ThreadSafeQueue` 鍜?`FrameQueue` 褰撳墠閮藉彧鏈?stop/clear/flush 璇箟锛屾病鏈?generation/serial锛涙棫鏃堕棿绾挎暟鎹鍓嶅彧鑳介潬鈥滆鍙婃椂娓呯┖鈥濊€屼笉鏄€滆鏄庣‘鍒ゅ簾鈥濄€?
2. seek 涔嬪墠涓昏渚濊禆 `scheduler pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop()`锛宎udio consumer 绾跨▼鏈韩涓嶅甫鏃堕棿绾垮垽瀹氾紝render 璺緞涔熸病鏈?serial 闃茬嚎銆?
3. 绗簩闃舵鏈€绋冲Ε鐨勮惤鐐癸紝鏄繚鎸?queue 瀹瑰櫒閫氱敤锛屾敼涓鸿姣忎釜 packet/frame 鑷甫 serial锛屽苟鐢?`PlayerCore` 闆嗕腑鍒嗛厤鍜屾縺娲?serial銆?


### 瑙ｅ喅鏂规

- 鏂板 `TimelineSerial` 涓?`DemuxPacket`锛屽苟涓?`VideoFrame / AudioFrame` 澧炲姞 `serial` 瀛楁銆?
- 鍦?`PlayerCore` 鍐呴儴鏂板 `timeline_serial / pending_seek_serial` 鍙婄粺涓€ helper锛歚allocateNextTimelineSerial / activateTimelineSerial / setPendingSeekSerial / isAcceptedTimelineSerial`銆?
- `open` 鎴愬姛鏃跺缓绔嬮涓?serial锛沗seek` 鍏堝垏鍒?pending serial锛屽啀鍦ㄦ垚鍔熷悗婵€娲伙紱`stop / requestDeferredStop` 浼氱珛鍗虫帹杩?serial锛岀‘淇濇棫 worker 鏅氬埌鏃跺彧鑳戒骇鍑哄簾鏁版嵁銆?
- demux 绾跨▼鍚姩鏃舵崟鑾?serial锛宒ecode/render/audio consumer 鍏ㄩ摼璺仛 stale serial 涓㈠純锛沝iagnostics 鍜屼笓椤规鏌ュ懡浠ゅ鍑?serial 瑙傛祴瀛楁銆?


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



## ?? 82: PlayerCore EOF/Ended ????????



**??**: 2026-03-20

**??**: ???



### ????

- ???????????????????????????????????? copy-back?SoftwareSDL?UI ???? `PlaybackState` ???

- ???????????? EOF ??? stop ???????????????? `Ended`????? `close/reopen` serial ??? scheduler stale render ?????



### ????

1. ??????? seek/stop ????????? serial ???????????????????????? `Ended`??????????????

2. ? EOF ???????? `Stopped`???????????????? stop????????????????????????????????ended reason ???????? stop ?????

3. ?? packet/frame queue ?????????? EOF ?????????????????????????????????? audio device drain ????????

4. ????????????????? `close()` ? stop ??????? serial??? scheduler ? stale frame ???????????????????????



### ????

- ? `PlayerCore` ?????? `EndedReason`??????? `CoreStateSnapshot`?`DiagnosticsSnapshot` ????????

- ? `onRenderIdle()` ? EOF ???? `PipelinePhase::Draining -> RunState::Ended`??????? `getBufferedSeconds()` ?????????

- `play()` ? `Ended` ???? `seek(0.0)` ??????`seek()` ? `Ended` ????? `Stopped`??????????

- `close()` ? `stop()` ???????? `timeline_serial`??? close/reopen ????????????

- ?? scheduler render callback ? `bool` ??????????????? `rendered_frames`??? stale frame ???????

- ?????? `docs/analysis/PLAYERCORE_DAY18_EOF_ENDED_AND_TERMINAL_STATE_REDESIGN.md`??? EOF/Ended ?????????????



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

## ?? 83: PlayerCore queue generation ? Scheduler ????????

**??**: 2026-03-20
**??**: ???

### ????
- ????????????????????? UI ??? `PlaybackState`????? queue flush ??? scheduler ??????
- ????? `clear()/flush()` ??????????????????????? scheduler ? `Seeking / Flushing / Stopping / Ended` ??????????

### ????
1. item-level serial ?????? packet/frame ??????? queue ????? generation????? producer/consumer ????????????
2. scheduler ???? `running_ / paused_`???? `Seeking / Flushing / Stopping / Ended` ??????? decode/render??? render wait ????????????????????
3. ?????????? queue ? scheduler ???????????????????????????? callback ?????

### ????
- ? `ThreadSafeQueue` ? `FrameQueue` ?? generation??????? `push()/pop()` ? generation ??????????
- ? `scheduler.h` ??? `SchedulerControlSnapshot`???? `run_state / pipeline_phase / accepted_timeline_serial` ????????????
- `PlayerCore` ?? `makeSchedulerControlSnapshot()` ????????? scheduler?decode/render loop ?????????????????
- render loop ? pop ?? clock wait ?????? accepted timeline serial????? seek/flush/stopping ???????
- `DiagnosticsSnapshot`??? diagnostics ????????? queue generation ???
- ?????? `docs/analysis/PLAYERCORE_DAY19_QUEUE_GENERATION_AND_SCHEDULER_CONTROL_REDESIGN.md`???????????????

### ??????
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`????`0 warnings / 0 errors`?

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

## 闂 84: PlayerCore 鍓綔鐢ㄩ泦涓寲涓?runtime failure/recovery policy 鏀跺彛

**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- `PlayerCore` 鐨勫閮ㄥ叆鍙ｅ凡缁忓畬鎴愮姸鎬佽縼绉绘敹鍙ｏ紝浣嗙嚎绋嬨€佽澶囥€侀槦鍒楀拰鏃堕挓鍓綔鐢ㄤ粛鏁ｈ惤鍦ㄥ涓叆鍙ｅ嚱鏁伴噷銆?- `SchedulerControlSnapshot` 杩樼己 `clock_source`銆乤udio-master 绾︽潫鍜?ended policy锛宻cheduler 浠嶈鑷鎺ㄦ柇涓氬姟璇箟銆?- decode/resample/output 鐨?fatal 鐐圭己灏戠粺涓€ recovery policy锛屽鏄撻噸鏂伴暱鍑哄垎鏁ｇ殑閿欒澶勭悊銆?
### 鍒嗘瀽璁板綍
1. 鐘舵€佽縼绉婚泦涓箣鍚庯紝涓嬩竴灞傚繀椤荤户缁泦涓殑鏄?side effects锛涘惁鍒欏叆鍙ｅ嚱鏁颁粛鐒朵細鎴愪负鈥滅姸鎬佹満 + worker 绠＄悊鍣ㄢ€濈殑娣峰悎浣撱€?2. `deferred stop` 鐨勬湰璐ㄦ槸寮傛 stop completion锛岃€屼笉鏄柊鐨勪笟鍔＄姸鎬侊紱鍥犳 request/completion side effects 蹇呴』鍏辩敤涓€濂?helper銆?3. scheduler 宸茬粡鎷ユ湁 `run_state / pipeline_phase / accepted_timeline_serial`锛屾湰杞户缁妸 `clock_source`銆乤udio-master 鍜?ended policy 缁撴瀯鍖栵紝鑳介伩鍏嶅啀鍔犳柊鐨勯浂鏁ｅ竷灏斾綅銆?4. runtime failure 鍏堢粺涓€鎴?`FailureRecoveryPolicy` 鍚庯紝鍚庣画鎵╁ぇ fatal 鐐硅鐩栬寖鍥存椂涓嶉渶瑕侀噸鏂版媶鎭㈠璺緞銆?
### 澶勭悊缁撴灉
- 鏂板 `applyStartPlaybackSideEffects / applyResumePlaybackSideEffects / applyPausePlaybackSideEffects / applyStopRequestSideEffects / applyStopCompletionSideEffects / applySeekSideEffects / applySessionReleaseSideEffects`銆?- `requestDeferredStop()` 涓?`serviceDeferredStop()` 宸插鐢?stop request/completion side effects锛宒eferred stop 涓嶅啀缁存姢鐙珛鍋滄満閫昏緫銆?- `SchedulerControlSnapshot` 宸叉柊澧?`clock_source`銆乣audio_output_initialized`銆乣audio_master_sync_active`銆乣ended_policy`锛宻cheduler render wait 閫昏緫宸叉帴鍏ヨ繖浜涘瓧娈点€?- 鏂板 `FailureRecoveryPolicy` 涓?`handleRuntimeFailure()`锛屽苟鎶婅棰?闊抽 decode-resample 閾句笂鐨勫叧閿?fatal 鐐圭粺涓€鏀跺彛鍒拌鍏ュ彛銆?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?- 鏈敼 UI 灞傚拰澶栭儴 `PlaybackState`锛屾湰杞彧鏀跺彛鍐呮牳鍓綔鐢ㄥ拰 runtime recovery 绛栫暐銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md`銆?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂 85: PlayerCore 鍓╀綑椋庨櫓鏀舵暃锛歋cheduler 缁堢増绛栫暐銆丗ailSession 瀹炲寲涓?serial/generation 瑙傛祴寮哄寲

**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- `SchedulerControlSnapshot` 闇€瑕佺户缁粨鏋勫寲 ended policy / clock policy / audio-master 绾︽潫銆?- `FailSession` 闇€瑕佷粠缁熶竴鍏ュ彛鎺ㄨ繘鍒板叧閿繍琛屾椂閿欒鐨勫疄闄呰鐩栥€?- queue generation 涓?item-level serial 闇€瑕佹洿鐩磋鐨勫彲瑙傛祴杈圭晫銆?
### 鍒嗘瀽璁板綍
1. 鑻?scheduler 浠嶄緷璧栧竷灏旀嫾璇箟锛岀瓥鐣ユ剰鍥句細缁х画鍒嗘暎銆?2. 鑻?`FailSession` 鍙仠鐣欏湪鍏ュ彛灞傦紝閬囧埌涓嶅彲鎭㈠閿欒鏃舵仮澶嶈矾寰勪粛涓嶄竴鑷淬€?3. generation 瑙ｅ喅鐨勬槸瀹瑰櫒鍞ら啋鍜岃竟鐣屼腑鏂紝涓嶆浛浠?serial 鐨勬椂闂寸嚎鍒ゅ畾銆?
### 澶勭悊缁撴灉
- `SchedulerControlSnapshot` 宸叉柊澧?`clock_policy`銆乣audio_master_policy`銆乣audio_buffered_seconds`锛屽苟鎵╁睍 ended policy銆?- `Scheduler` 宸叉敼涓虹瓥鐣ラ┍鍔?render wait锛沗Scheduler::stop()` 澧炲姞 self-join 淇濇姢銆?- `FailSession` 宸茶鐩栧叧閿笉鍙仮澶嶉敊璇偣骞惰惤鍒扮湡瀹炶矾寰勩€?- `DiagnosticsSnapshot` 鏂板 stale serial drop 璁℃暟涓?runtime failure 璁℃暟锛屽苟鎺ュ叆妫€鏌ュ懡浠よ緭鍑恒€?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md`銆?
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
## 闂 86: serial/failsession 鍥炲綊妫€鏌ヨˉ榻愶紙杩炵画 seek銆佹殏鍋滄€?seek銆乧lose-reopen锛?
**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 闇€瑕佽ˉ鍏呬笓鐢?CLI 鍥炲綊鎺㈤拡锛岄獙璇?serial 杈圭晫涓?FailSession 绾︽潫鍦ㄥ叧閿満鏅笅鏃犲洖褰掋€?
### 鍒嗘瀽璁板綍
1. 鐜版湁 performance/software 妫€鏌ュ亸閾捐矾鍋ュ悍锛屼笉鐩存帴瑕嗙洊涓夌被鐘舵€佽竟鐣屻€?2. 闈炴硶鐘舵€佽縼绉绘鍓嶅彧鍦ㄦ棩蹇楀彲瑙侊紝涓嶅埄浜庢満鍣?gate銆?3. 鍥炲綊鍛戒护闇€缁熶竴 `key=value` 涓?`result=PASS/FAIL`锛屼究浜庤嚜鍔ㄥ寲娑堣垂銆?
### 澶勭悊缁撴灉
- `DiagnosticsSnapshot` 鏂板 `illegal_session_transitions / illegal_run_transitions / illegal_pipeline_transitions`锛屽苟瀹屾垚璁℃暟涓庨噸缃帴鍏ャ€?- 鏂板 CLI 鍛戒护锛?  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--close-reopen-serial-check`
- `--performance-log-check` 涓?`--software-video-decode-check` 澧炲姞闈炴硶杩佺Щ璁℃暟瀛楁杈撳嚭銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md`銆?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?- 鏂板 3 涓鏌ュ懡浠ゅ湪 `juren-30s.mp4` 涓婂潎杩斿洖 `PASS`銆?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 87: serial/failsession 鍥炲綊澧炲姞涓€閿仛鍚?gate锛堥檷浣庢紡璺戦闄╋級

- `main` 鏂板鑱氬悎鍛戒护锛?  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 鑱氬悎鍛戒护鍐呴儴椤哄簭鎵ц锛?  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--close-reopen-serial-check`
- 鏂板鏈哄櫒鍙鑱氬悎杈撳嚭锛?  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`
- 鏈湴楠岃瘉锛?  - Debug 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`锛歚PASS`锛坄pass_count=3/3`锛夈€?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md`銆?
### 淇敼鏂囦欢
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 88: 寮哄埗 FailSession 鍥炲綊鎺㈤拡涓?codec 閿侀噸鍏ュ穿婧冧慨澶?
- 鏂板 `--forced-failsession-check <media_file> [sample_ms]`锛岄€氳繃娴嬭瘯娉ㄥ叆绋冲畾瑕嗙洊 `FailSession` 璺緞锛屽苟杈撳嚭鏈哄櫒鍙 `key=value` + `result=PASS/FAIL`銆?- `PlayerCore::decodeVideoFrame` 澧炲姞 `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE` 娴嬭瘯娉ㄥ叆寮€鍏筹紝鐢ㄤ簬寮哄埗杩涘叆 `FailureRecoveryPolicy::FailSession`銆?- 淇 `FailSession` 浠庤В鐮佺嚎绋嬭繘鍏ユ椂鐨?codec 閿侀噸鍏ュ穿婧冿細
  - `video_codec_mutex_`銆乣audio_codec_mutex_` 璋冩暣涓?`std::recursive_mutex`锛?  - `decodeVideoFrame/decodeAudioFrame` 鐨?`lock_guard` 绫诲瀷鍚屾璋冩暣銆?- 鏈湴楠岃瘉锛?  - Debug 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?  - `build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`锛歚PASS`銆?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`锛歚PASS`銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md`銆?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 89: run_all_checks 鎺ュ叆 forced-failsession 涓€閿?gate

- `tools/run_all_checks.ps1` 鏂板鍙傛暟锛?  - `ForcedFailSessionFile`锛堥粯璁ゅ鐢?`ProbeFile`锛?  - `ForcedFailSessionSampleMs`锛堥粯璁?`2200`锛?- 鍥炲綊娴佺▼浠?2 姝ユ墿涓?3 姝ワ細
  1. `[1/3]` `--probe-file --json`
  2. `[2/3]` `--forced-failsession-check`
  3. `[3/3]` `run_format_regression.ps1`
- gate 瑙勫垯澧炲己锛?  - probe 澶辫触鐩存帴閫€鍑猴紱
  - forced-failsession 澶辫触鐩存帴閫€鍑哄苟璺宠繃 format regression銆?- 鏈湴楠岃瘉锛?  - `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Debug/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200`
  - `probe/forced-failsession/regression` 閫€鍑虹爜鍧囦负 `0`锛岃剼鏈€婚€€鍑虹爜 `0`銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md`銆?
### 淇敼鏂囦欢
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 问题 90: OpenGL 原生 D3D11 互操作停止期异常退出与低吞吐修复
**日期**: 2026-03-24
**状态**: 已解决
### 问题描述
- `MVP_RENDERER_BACKEND=opengl` 下已进入 `D3D11VA -> OpenGL` 原生互操作路径，但 `--performance-log-check .\juren-30s.mp4 2000` 在 stop/close 阶段异常退出，未输出最终 `result`。
- 同时出现原生路径吞吐异常偏低：`video_native_output_frames=62` 之前只能稳定渲染约 3 帧，表现为 stop 前卡顿和退出不稳定。
### 分析记录
1. OpenGL 原生路径使用 renderer-owned D3D11 device，FFmpeg 硬解线程与 OpenGL 渲染线程共享同一 D3D11 immediate context。
2. 该设备创建时未开启 `ID3D11Multithread::SetMultithreadProtected(TRUE)`，导致并发访问不安全，出现 native 路径卡顿、stop 阶段异常退出。
3. `PlayerCore::applySessionReleaseSideEffects()` 原先先释放 decoder/hw context，再关闭 renderer；这会让仍持有 `AV_PIX_FMT_D3D11` 帧引用的缓存帧/渲染线程处于危险释放顺序。
### 处理结果
- `src/render/opengl_video_renderer.cpp`
  - 为 OpenGL 原生互操作使用的 D3D11 设备补齐 `ID3D11Multithread` 多线程保护。
  - 保持 `D3D11VA -> D3D11 shader color convert -> WGL_NV_DX_interop -> OpenGL draw` 原生路径继续工作。
- `src/core/player_core.cpp`
  - 调整 session release 顺序：先释放缓存 native frame 并关闭 renderer，再释放 decoder/hw context。
- Release 本地验证：
  - `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`
  - 结果：`renderer_backend=OpenGL`、`decoder_backend=D3D11VA`、`video_native_output_frames=62`、`video_copy_back_frames=0`、`render_frames=47`、`result=PASS`、进程退出码 `0`。
### 修改文件
- src/render/opengl_video_renderer.cpp
- src/core/player_core.cpp
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-24 OpenGL ASS 样式链路增强
- 字幕模型新增：`wrap_style`、`spacing`、`scale_x_percent`、`scale_y_percent`、`rotation_degrees`、`outline_x/y`、`shadow_x/y`。
- `ASS parser` 新增 `WrapStyle`、style 字段与 `\q/\fsp/\fscx/\fscy/\fr/\frz/\xbord/\ybord/\xshad/\yshad` 支持。
- OpenGL/D3D11 GPU 字幕渲染已消费上述字段：`DirectWrite character spacing + D2D transform + x/y outline/shadow`。
- 新增 `--subtitle-style-check` 样式诊断 CLI。
- 新增回归样本：`samples/subtitles/opengl_ass_style_validation.ass`。
- 本地验证通过：`subtitle-style-check`、`subtitle-sync-check`、`opengl delay-adjust-check`、`opengl-diagnostics` 全部 `PASS`。

## 2026-03-24 Subtitle Capability Update
- Added ASS run-level subtitle support for `secondary color`, rectangular `clip`, basic rectangular `iclip`, and karaoke timing (`k/kf/ko/K`) across both D3D11 and OpenGL subtitle paths.
- Added renderer subtitle clock propagation so animated subtitle content can invalidate and redraw correctly while paused or during clock changes.
- Extended subtitle diagnostics output and added `samples/subtitles/opengl_ass_karaoke_clip_validation.ass`.
- Local validation: `subtitle-style-check`, `subtitle-sync-check`, `delay-adjust-check`, `d3d11-diagnostics` and `opengl-diagnostics` all passed in Release.

## 2026-03-24 ASS Animation Update
- Added line-level ASS animation support for `move`, `fad` and `fade` across both D3D11 and OpenGL subtitle rendering paths.
- Added `SubtitleStyleAnimation` / `SubtitleFadeMode` and subtitle-clock-driven animation evaluation.
- Extended `--subtitle-style-check` with move/fade fields and added `samples/subtitles/opengl_ass_animation_validation.ass`.
- Local validation: Release build plus `subtitle-style-check`, `subtitle-sync-check`, `delay-adjust-check`, `d3d11-diagnostics` and `opengl-diagnostics` all passed.


## 2026-03-25 ASS Transform / Vector / Font Fallback Update
- Added subtitle parser/model coverage for `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector clip payload, pure ASS drawing items, and subtitle source path reporting.
- Added D3D11/OpenGL subtitle rendering for affine transform, vector drawing, vector clip, and subtitle-sidecar/private font fallback families.
- Fixed the D2D layer push/pop balance regression that produced `OpenGL subtitle D2D draw failed: hr=-2003238890` during validation.
- Validation: Release build plus `subtitle-style-check`, `subtitle-sync-check`, OpenGL `delay-adjust-check`, and D3D11 `delay-adjust-check` all passed.

### 2026-03-25 Update: idle startup window and drag-drop playback
- Added direct exe idle-window startup with no media arguments.
- Added `consumeOpenFileRequest()` across renderer / player-core / video-player layers.
- Added playback-time drag-drop replacement with path validation in `main.cpp`.
- Idle-start sessions now return to the idle window after playback completes.
- Validation: `cmake --build build --config Release --target modern-video-player` passed.
- Follow-up manual test still recommended for GUI drag-drop behavior.

### 2026-03-25 Update: OpenGL present pacing and stutter fix
- OpenGL `present()` now waits for real render-thread display completion instead of returning immediately after async queue submit.
- Added OpenGL submission/presentation IDs to align scheduler pacing with actual display completion.
- OpenGL now prefers `swap interval=1` and falls back to immediate presents only when needed.
- Removed the per-frame native interop `ID3D11DeviceContext::Flush()` call.
- Validation: Release build, `--opengl-diagnostics`, OpenGL native `--performance-log-check`, and OpenGL copy-back `--performance-log-check` all passed.
### 2026-03-25 Update: OpenGL runtime diagnostics export and 10-bit copy-back upload
- Confirmed `--performance-log-check` exports `renderer_opengl_native_interop_active`, `renderer_opengl_native_interop_startup_disabled`, `renderer_opengl_native_interop_disable_rule`, `renderer_opengl_native_interop_frames`, `renderer_opengl_native_interop_disable_events`, and `renderer_opengl_present_wait_timeouts`.
- Added OpenGL direct software upload support for `AV_PIX_FMT_P010LE` and `AV_PIX_FMT_P016LE`.
- Added 16-bit normalized software color coefficients for the OpenGL semi-planar upload path.
- Validation: OpenGL native regression PASS, OpenGL forced-copyback regression PASS, and forced-copyback 10-bit HEVC regression PASS with `video_swscale_frames=0`.

### 2026-03-25 Update: OpenGL present override and gate script
- Added `MVP_OPENGL_PRESENT_MODE=auto|paced|immediate`.
- Added `renderer_opengl_present_mode_requested` and `renderer_opengl_present_mode_active` to `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to validate OpenGL diagnostics, native playback, copy-back playback, immediate present mode, 10-bit copy-back, and subtitle delay regression in one command.
- Validation: auto present PASS, immediate present PASS, OpenGL gate script PASS.

### 2026-03-25 Update: OpenGL HDR probe, quirk expansion, subtitle gate, and final gap matrix
- Added `opengl-diagnostics.hdr_output.*` display-output capability probe fields.
- Expanded the OpenGL quirk rule table for SwiftShader, llvmpipe, VMware, and Parallels software/virtual contexts, and added finer rule keys.
- Expanded `tools/run_opengl_checks.ps1` to a 10-step OpenGL validation gate including ASS subtitle sample regressions.
- Added a final backend-level OpenGL gap matrix against mature players.
- Validation: `--opengl-diagnostics` PASS and `run_opengl_checks.ps1` PASS.

### 2026-03-25 Update: OpenGL hotkey and control OSD interaction
- Suppressed OpenGL `SDL_KEYDOWN repeat` handling to reduce hotkey-triggered seek/volume request storms.
- Changed OpenGL hotkey OSD wakeup to unconditional redraw so progress/volume feedback is visible during active playback.
- Added OpenGL mouse-driven control interaction for progress and volume bars, including seek preview and drag updates.
- Added initial OSD wakeup on OpenGL renderer startup.
- Validation: Release build PASS and `run_opengl_checks.ps1` PASS.

### 2026-03-25 Update: ASS transform transition support
- Added subtitle parser/model support for ASS `\t(...)` transitions, including nested-parenthesis-safe parsing and top-level comma splitting.
- Added OpenGL and D3D11 runtime interpolation for transition-driven font scale, color, outline, shadow, spacing, rotation, shear, and rotation-origin fields.
- Extended `--subtitle-style-check` to export `transition_count`, per-transition timing/accel/property names, and transition target style fields.
- Added `samples/subtitles/opengl_ass_transform_transition_validation.ass` and folded it into `tools/run_opengl_checks.ps1`.
- Validation: Release build PASS, `subtitle-style-check` PASS, OpenGL `delay-adjust-check` PASS, D3D11 `delay-adjust-check` PASS, OpenGL gate PASS.

### 2026-03-25 Update: OpenGL bottom-bar player chrome
- Expanded the OpenGL OSD into a bottom-bar player interface with a dedicated play/pause button, time text region, progress rail, and volume rail.
- Added geometry-based OpenGL helpers for button shapes and segmented time text, avoiding a new font dependency on the OpenGL path.
- Added hover-aware panel visibility so the bar remains visible while hovered or dragged and auto-hides with fade-out during idle playback.
- Kept seek preview and volume drag on the new control layout, and wired play/pause button clicks into the existing request pipeline.
- Validation: Release build PASS and OpenGL gate PASS; manual GUI smoke still recommended for hover timing and hit-testing feel.

### 2026-03-25 Update: Container attachment font pipeline
- Added media-scoped subtitle attachment font extraction and private registration from FFmpeg `AVMEDIA_TYPE_ATTACHMENT` streams.
- Added temp-cache-based attachment font session management with cleanup on playback/session release.
- Wired `PlayerCore::open()` / `applySessionReleaseSideEffects()` to register and release container font attachments automatically.
- Added `--attachment-font-check <media_file>` for machine-readable validation of attachment extraction, registration, and cleanup.
- Validation: Release build PASS, generated MKV-with-font-attachment sample PASS, `attachment-font-check.result=PASS`, cache cleanup PASS.

### 2026-03-25 Update: OpenGL CPU / GPU / driver optimization matrix
- Added `docs/plans/OPENGL_CPU_GPU_DRIVER_OPTIMIZATION_MATRIX.md`.
- Consolidated the current OpenGL default strategy by CPU layer, GPU path, and driver/adapter rule handling.
- Added a release-facing vendor strategy table for NVIDIA / AMD / Intel plus vendor-specific validation commands.
- This is a documentation consolidation update; it does not change runtime behavior by itself.

### 2026-03-25 Update: Embedded subtitle-track playback
- Added automatic loading of supported embedded text subtitle streams during media open.
- Added separate embedded/external subtitle ownership in `PlayerCore` with `external > embedded` selection and fallback when clearing sidecar subtitles.
- Added in-memory `AssParser::parseText(...)` / `SrtParser::parseText(...)`, plus `--embedded-subtitle-check <media_file>`.
- Expanded `tools/run_opengl_checks.ps1` to validate generated embedded ASS and embedded `mov_text` samples through both CLI checks and OpenGL playback.
- Validation: Release build PASS, embedded ASS check PASS, embedded text check PASS, OpenGL gate PASS.
