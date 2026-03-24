# 开发日志

## 问题 96: OpenGL 渲染链路 M0 落地并完成本地验收

**日期**: 2026-03-24
**状态**: 已解决

### 问题描述
- 用户要求继续补齐 `OpenGL` 播放链路，并确认它与 `mpv / MPC-HC` 这类成熟播放器相比还差多少。
- 当前仓库中 `OpenGLVideoRenderer` 仍是 stub，需要把它补成可运行的最小可用后端，而不是继续停留在名义选项。

### 日志输出
```text
Release build:
cmake --build build --config Release
modern-video-player.exe build succeeded

OpenGL validation:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000
OpenGL context: vendor=NVIDIA Corporation renderer=NVIDIA GeForce GTX 1080/PCIe/SSE2 version=4.6.0 NVIDIA 560.94
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.result=PASS
```

### 分析记录
1. 旧实现只有工厂选择入口，没有窗口、上下文、shader、纹理上传与事件泵，因此本质上不具备实际播放能力。
2. 这次先落地 `OpenGL M0`，目标是让 `YUV420P / NV12` 软件帧可以稳定显示，同时保持失败自动回退；不在这一轮提前做字幕/OSD GPU 叠加。
3. 验收结果已经证明 `OpenGL` 不再是 stub，但它仍不是成熟播放器级 GPU 后端，当前定位应明确为 opt-in 过渡路径。

### 处理结果
- `OpenGLVideoRenderer` 现已具备 SDL OpenGL 上下文、GLSL 120 色彩转换、YUV420P/NV12 上传与基础键盘事件处理。
- `CMakeLists.txt` 已补 `opengl32 / OpenGL::GL` 链接依赖。
- 新增 `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`，并同步 `guides / analysis / README / records` 文档口径。

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
# 寮€鍙戞棩蹇?
## 闂 95: RC 鐗堟湰鍏冩暟鎹€丷elease 椤垫鏂囦笌瀹夎鍖呯増鏈爣璇嗚ˉ榻?
**鏃ユ湡**: 2026-03-23
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛杩介棶鈥淩elease 椤垫鏂囧湪鍝噷鈥濓紝骞惰姹傛妸绋嬪簭鍐呴儴鐗堟湰銆乄indows 鍙墽琛屾枃浠剁増鏈拰瀹夎鍖呯増鏈兘琛ユ垚 `1.0.0-rc1`銆?- 杩欒疆鐩爣涓嶆槸缁х画鏀规挱鏀鹃摼锛岃€屾槸鎶?RC 鐗堟湰鏍囪瘑浠庢枃妗ｅ眰琛ラ綈鍒?CLI銆丒XE 灞炴€с€丠TTP UA 鍜屽彂甯冨寘浜х墿灞傘€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. `V1_0_0_RC1_RELEASE_READINESS.md` 鏄唴閮ㄥ彂甯冪粨璁猴紝涓嶇瓑浜庡彲鐩存帴璐村埌 GitHub Release 椤电殑姝ｆ枃锛屽洜姝ら渶瑕佸崟鐙ˉ涓€浠?`Release Notes` 鏂囨。銆?2. 鐗堟湰鍙峰鏋滃彧瀛樺湪浜?`project(... VERSION 1.0.0)`锛岀▼搴?CLI銆乄indows 璧勬簮銆佸帇缂╁寘鍚嶅拰 HTTP `user_agent` 浼氱户缁垎瑁傦紝RC 鐜板満鎺掗殰鍜岀増鏈瘑鍒垚鏈緢楂樸€?3. Windows 鏂囦欢鐗堟湰鍜屼骇鍝佺増鏈簲璇ュ尯鍒嗭細`FileVersion` 缁存寔鍥涙鏁板€?`1.0.0.0`锛宍ProductVersion`/瀵瑰鐗堟湰鍚嶄娇鐢?`1.0.0-rc1`銆?4. 鍙戝竷鍖呭繀椤婚伩鍏嶆墦鍏ユ湰鍦拌剰鐨?`config/player_settings.ini`锛屽惁鍒欎骇鐗╀細娣峰叆寮€鍙戞満鏈湴閰嶇疆锛涙湰杞墦鍖呭凡鏄惧紡楠岃瘉璇ユ枃浠舵湭杩涘叆 ZIP銆?
### 澶勭悊缁撴灉
- 鏂板 `docs/reports/V1_0_0_RC1_RELEASE_NOTES.md`锛屼綔涓?Release 椤垫鏂囨潵婧愩€?- `CMake` 寮曞叆缁熶竴鐗堟湰婧愶紝骞剁敓鎴?`mvp_version.h` 涓?`version_info.rc`銆?- `main` 鏂板 `--version`锛沗http_stream_downloader` 鐨?`user_agent` 鏀逛负 `modern-video-player/1.0.0-rc1`銆?- 鏂板 `CPack ZIP` 鎵撳寘瑙勫垯锛屼骇鐗╁悕鍥哄畾涓?`modern-video-player-1.0.0-rc1-windows-x64.zip`锛屽苟灏?`RELEASE_NOTES.md` 鎵撳叆鍖呭唴銆?- `docs/README.md`銆乣docs/reports/README.md` 涓?`V1_0_0_RC1_RELEASE_READINESS.md` 宸茶ˉ鍏?Release Notes 鍏ュ彛銆?
### 鏈湴楠屾敹缁撴灉
- `build\Release\modern-video-player.exe --version`锛氶€氳繃銆?- Windows `FileVersionInfo`锛歚FileVersion=1.0.0.0`銆乣ProductVersion=1.0.0-rc1`銆乣ProductName=Modern Video Player`銆?- `cmake --build build --config Release --target PACKAGE`锛氶€氳繃锛岀敓鎴?`build/modern-video-player-1.0.0-rc1-windows-x64.zip`銆?- ZIP 宸茬‘璁ゅ寘鍚?`modern-video-player.exe`銆佷緷璧?DLL銆乣plugins/sample_logger_plugin.dll` 涓?`RELEASE_NOTES.md`锛屼笖涓嶅寘鍚?`config/player_settings.ini`銆?- `build\Release\modern-video-player.exe --d3d11-diagnostics`锛氶€氳繃锛宍result=PASS`銆?
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
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛鏄庣‘纭鎸?`1.0.0-rc1` 鏂瑰悜鎺ㄨ繘锛屽笇鏈涙妸 RC 鍙戝竷娓呭崟銆佸凡鐭ラ棶棰樺拰鍙戝竷璇存槑鏀跺彛鎴愪竴濂楀彲鐩存帴浣跨敤鐨勬枃妗ｃ€?- 杩欒疆鐩爣涓嶆槸缁х画鎵╁姛鑳斤紝鑰屾槸鍒ゆ柇鈥滃綋鍓嶇増鏈槸鍚﹀凡缁忚冻澶熷彂 RC鈥濓紝骞舵妸璇佹嵁涓庨檺鍒惰娓呮銆?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 杩欒疆璇佹嵁宸茬粡瓒冲鏀寔鈥滃彲鍙?RC鈥濊€屼笉鏄粎鍋滅暀鍦ㄢ€滄湰鍦板伓灏旇兘鎾€濓細Release gate銆佹牸寮忓洖褰掋€侀暱鏃舵挱鏀俱€乻eek/serial銆乫orced failsession 鍜?D3D11 diagnostics 閮芥湁鏈€鏂扮粨鏋溿€?2. 褰撳墠 RC 缁撹搴斿綋鏄庣‘鍖哄垎涓衡€滃彲鎵?`v1.0.0-rc1` 鏍囩鈥濅笌鈥滀笉鍙洿鎺ュ绉版寮忕増宸插畬鎴愨€濄€傜湡姝ｉ樆姝㈢洿鎺?GA 鐨勶紝涓嶆槸涓婚摼涓嶅彲鐢紝鑰屾槸鍏煎鐭╅樀鍜岄暱灏捐矾寰勫皻鏈畬鍏ㄥ悆閫忋€?3. 鏈€闇€瑕佹樉寮忓啓杩涘凡鐭ラ棶棰樼殑鏄?`闂 79`锛歴oftware video decode 杩愯鎬佽矾寰勪粛鏈畬鍏ㄦ敹鍙ｃ€傚畠褰撳墠涓嶉樆濉?`D3D11VA` 涓婚摼 RC锛屼絾瀹冧粛鏄寮忕増鍓嶇殑鍓╀綑椋庨櫓銆?4. 褰撳墠鏈哄櫒鐨?`AV1` profile 璇婃柇缁撴灉浠嶄负 `false`锛岃繖鍐嶆璇佹槑 RC 鍙戝竷璇存槑涓嶈兘鎶娾€滃彲鎾斁 AV1 鏂囦欢鈥濆拰鈥滄櫘閬嶅叿澶?AV1 纭В鑳藉姏鈥濇贩涓轰竴璋堛€?
### 澶勭悊缁撴灉
- 鏂板 `docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`锛?  - 杈撳嚭 RC 鍙戝竷缁撹
  - 姹囨€绘湰杞?Release 楠岃瘉璇佹嵁
  - 鏀跺彛鍙戝竷璇存槑銆佸凡鐭ラ棶棰樹笌鍙戝竷娓呭崟
- 鏇存柊 `docs/reports/README.md` 涓?`docs/README.md`锛?  - 涓?RC 姹囨€绘姤鍛婂鍔犲叆鍙?- 鏇存柊 `docs/records/VERSION.md`锛?  - 澧炲姞 `褰撳墠鍙戝竷鍊欓€? 1.0.0-rc1`
  - 澧炲姞 `鍙戝竷鐘舵€? 鍙彂甯?RC锛屼笉寤鸿鐩存帴 GA`
  - 璁板綍鏈疆 RC 缁撹涓庢渶鏂伴獙璇佽瘉鎹?- `CHANGELOG / DEVELOP_LOG` 宸插悓姝ヨ褰曟湰娆″彂甯冨噯澶囨敹鍙ｃ€?
### 鏈湴楠屾敹缁撴灉
- 褰撳墠缁撹锛歚鍙墦 v1.0.0-rc1 鏍囩`銆?- 涓嶅缓璁綋鍓嶇洿鎺ユ墦 `v1.0.0` 姝ｅ紡鐗堛€?- 鏈€鏂?Release 璇佹嵁锛?  - `run_all_checks.ps1`锛氶€氳繃
  - `FORMAT_REGRESSION_20260323_224615.md`锛歚17 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`
  - `--d3d11-diagnostics`锛氶€氳繃
  - `--performance-log-check`锛氶€氳繃
  - `--long-playback-check`锛氶€氳繃
  - `--serial-failsession-regression-check`锛氶€氳繃

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
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰鎶?D3D11 鍚庣画鎴愮啛鍖栭」涓€娆¤ˉ榻愶細decoder profile 鎺㈡祴銆乨river quirk/blacklist 鍚姩鏈熼檷绾х瓥鐣ワ紝浠ュ強鍗曠嫭鐨?`--d3d11-diagnostics` CLI銆?- 鐩爣涓嶆槸缁х画琛ヤ竴缁勪复鏃舵棩蹇楋紝鑰屾槸鎶?D3D11 鑳藉姏蹇収鍋氭垚鍙満鍣ㄨ鍙栥€佸彲鍚姩鏈熷喅绛栥€佸彲鑴辩鎾斁鍗曠嫭鎵ц鐨勫熀纭€璁炬柦銆?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 杩欒疆鍏抽敭鍙樺寲鏄妸 D3D11 鎺㈡祴浠庘€滄棩蹇楁墦鍗扳€濆崌绾ф垚鈥滅粨鏋勫寲蹇収鈥濓紝杩欐牱 renderer 鍚姩鏈熺瓥鐣ュ拰鐙珛 CLI 鍙互鍏辩敤鍚屼竴浠戒簨瀹炴簮銆?2. 褰撳墠鏈哄櫒鐨勫疄娴嬬粨鏋滃凡缁忚兘鐩存帴鍥炵瓟鐢ㄦ埛鏈€鍏冲績鐨勯棶棰橈細`H.264 / HEVC / VP9` 鏈夌‖瑙?profile锛宍AV1` 娌℃湁锛沗NV12` 鏀寔 shader sampling锛沶ative direct 鍚姩鏈熷厑璁稿紑鍚€?3. `P016` 鐨?`CheckFormatSupport` 浠嶅け璐ワ紝`VP9 profile2 10bit` 涓庢墍鏈?`AV1` profile 涔熼兘涓嶆敮鎸侊紝杩欒鏄庘€滄湁 D3D11鈥濅笉绛変簬鈥滄墍鏈?10bit / 鏂扮紪鐮?profile 閮藉彲鐢ㄢ€濓紝鐙珛 diagnostics CLI 鐨勪环鍊煎氨鏄妸杩欎簺杈圭晫鏄惧紡鍖栥€?4. quirk / blacklist 鏈哄埗褰撳墠鍏堜繚瀹堣惤涓€涓渶鏄庣‘瑙勫垯锛歚Microsoft Basic Render Driver` 鐩存帴绂佺敤 native direct锛涘悗缁嫢绉疮鍒版洿澶氶┍鍔ㄥ潙浣嶏紝鍙户缁部鍚屼竴绛栫暐琛ㄦ墿鍏呫€?
### 澶勭悊缁撴灉
- `include/render/d3d11_video_renderer.h`锛?  - 鏂板 `D3D11FormatSupportSnapshot`
  - 鏂板 `D3D11DecoderProfileSupport`
  - 鏂板 `D3D11DiagnosticsSnapshot`
  - 鏂板 `D3D11VideoRenderer::probeSystemDiagnostics()`
- `src/render/d3d11_video_renderer.cpp`锛?  - 鏂板 D3D11 probe context銆佹牸寮忔敮鎸佹煡璇€乨ecoder profile 鏋氫妇銆乻tartup policy 璇勪及鍜屽惎鍔ㄦ棩蹇楄緭鍑?  - 鏂板鏈湴 GUID 甯搁噺锛岄伩鍏嶇洿鎺ヤ緷璧栨煇浜?Windows SDK 鐜涓嬬殑 decoder profile 澶栭儴绗﹀彿
  - renderer 鍒濆鍖栭樁娈垫帴鍏?startup policy锛屽繀瑕佹椂鍦ㄦ挱鏀惧墠灏辩鐢?native direct
- `src/main.cpp`锛?  - 鏂板 `runD3D11Diagnostics()`
  - 鏂板 `--d3d11-diagnostics`
  - 杈撳嚭缁熶竴 `key=value` 鏈哄櫒鍙缁撴灉
- `CHANGELOG / VERSION / DEVELOP_LOG` 宸插悓姝ヨ褰曟湰娆?D3D11 鎴愮啛鍖栧寮恒€?
### 鏈湴楠屾敹缁撴灉
- `build\Release\modern-video-player.exe --d3d11-diagnostics`锛氶€氳繃锛宍result=PASS`銆?- 褰撳墠鏈哄櫒鐨勬渶鏂拌瘖鏂粨鏋滐細
  - `adapter_name=NVIDIA GeForce GTX 1080`
  - `driver_version=32.0.15.6094`
  - `decoder_profiles.h264_any=true`
  - `decoder_profiles.hevc_any=true`
  - `decoder_profiles.vp9_any=true`
  - `decoder_profiles.av1_any=false`
  - `native_direct.allowed=true`
  - `native_direct.disable_rule=none`
- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`锛氶€氳繃锛宍renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣video_native_output_frames=62`銆乣video_copy_back_frames=0`銆乣result=PASS`銆?
### 淇敼鏂囦欢
- include/render/d3d11_video_renderer.h
- src/render/d3d11_video_renderer.cpp
- src/main.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂 92: D3D11 鍚姩鏈熻兘鍔涙帰娴嬩笌 adapter/driver 璇婃柇鏃ュ織琛ラ綈

**鏃ユ湡**: 2026-03-23
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛瑕佹眰缁х画鎸夋垚鐔熸挱鏀惧櫒鏂瑰悜鎺ㄨ繘锛屼笅涓€姝ヨˉ榻?D3D11 鍚姩鏈熻兘鍔涙帰娴嬩笌 adapter/driver 璇婃柇鏃ュ織銆?- 鐩爣鏄湪鍒濆鍖栭樁娈电洿鎺ョ湅鍒扳€滃綋鍓嶉€傞厤鍣ㄦ槸璋併€侀┍鍔ㄧ増鏈槸澶氬皯銆乫eature level 鍒板摢銆佸叧閿牸寮忔槸鍚︽敮鎸佲€濓紝鑰屼笉鏄彧鍦ㄩ粦灞忓悗琚姩杩芥棩蹇椼€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 鍚姩鏈熸棩蹇楀凡缁忚兘鐩存帴鏆撮湶 adapter/driver/feature level 涓婁笅鏂囷紝鍚庣画鍐嶉亣鍒版満鍣ㄥ樊寮傞棶棰樻椂锛屼笉闇€瑕佸厛绛夊埌鎾斁澶辫触鍐嶅弽鎺ㄨ澶囦俊鎭€?2. 鏍煎紡鏀寔鎺㈡祴鏄剧ず褰撳墠鏈哄櫒鐨?`NV12`銆乣P010` 鍏峰 `shader_sample + decoder_output`锛岃€?`P016` 鐨?`CheckFormatSupport` 鐩存帴澶辫触锛岃繖绫荤粏鑺傛鍓嶅畬鍏ㄤ笉鍙銆?3. 杩欑被鏃ュ織鏄垚鐔熸挱鏀惧櫒鐨勯噸瑕佸熀纭€璁炬柦锛屽洜涓哄畠鎶娾€滆澶囪兘鍔涒€濅笌鈥滆繍琛屾湡琛ㄧ幇鈥濊繛鎺ヨ捣鏉ヤ簡銆?
### 澶勭悊缁撴灉
- `src/render/d3d11_video_renderer.cpp`锛?  - 鏂板 adapter/driver version/鏄惧瓨淇℃伅鏃ュ織
  - 鏂板 feature level銆乨ebug layer銆乵ultithread銆乣device3/video_device/video_context` 鏃ュ織
  - 鏂板 `NV12/P010/P016` 鐨勬牸寮忔敮鎸佹憳瑕佹棩蹇?  - 鏂板 swap chain 鍙傛暟鏃ュ織
  - `MakeWindowAssociation` 鏀逛负鏄惧紡妫€鏌ュけ璐ュ苟鍛婅
- `CHANGELOG / VERSION / DEVELOP_LOG` 宸插悓姝ヨ褰曟湰娆¤瘖鏂寮恒€?
### 鏈湴楠屾敹缁撴灉
- `Release` 鏋勫缓閫氳繃锛宍0 warnings / 0 errors`銆?- `build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 閫氳繃銆?- 鏂板鐨?`[diag:d3d11-init]` 鍥涚粍鏃ュ織绋冲畾鎵撳嵃锛屼笖鏈奖鍝嶅綋鍓嶅凡缁忔仮澶嶇殑闆舵嫹璐濈洿閲囨牱閾捐矾锛歚video_native_output_frames=62`銆乣video_copy_back_frames=0`銆乣result=PASS`銆?
### 淇敼鏂囦欢
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂 91: D3D11VA 鑷畾涔?hw_frames_ctx锛氱敵璇峰彲閲囨牱瑙ｇ爜琛ㄩ潰骞舵仮澶嶉浂鎷疯礉鐩撮噰鏍?
**鏃ユ湡**: 2026-03-23
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛杩涗竴姝ヨ姹傛妸 D3D11 璺緞鍋氭垚鍍忔垚鐔熸挱鏀惧櫒閭ｆ牱鐨勭湡瀹炲彲鐢ㄥ疄鐜帮紝鑰屼笉鏄粎渚濊禆闂 90 鐨勮繍琛屾椂 copy-back fallback銆?- 闇€瑕侀獙璇佸綋鍓嶉」鐩槸涓嶆槸鍥犱负娌℃湁鑷畾涔?`hw_frames_ctx`锛屾墠瀵艰嚧 D3D11VA 瑙ｇ爜闈㈤粯璁や笉鍙洿鎺ラ噰鏍枫€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 褰撳墠椤圭洰姝ゅ墠鍙缃簡 `hw_device_ctx`锛屾病鏈夊湪 `get_format()` 涓帴绠?`hw_frames_ctx`锛岃繖涓?FFmpeg 澶存枃浠舵彁渚涚殑 D3D11 frames 鍒嗛厤鎺ュ彛涓嶅尮閰嶃€?2. 涓€鏃﹀湪 `AVD3D11VAFramesContext::BindFlags` 涓婅拷鍔?`D3D11_BIND_SHADER_RESOURCE`锛屽悓涓€鍙版満鍣ㄤ笂绔嬪埢鎭㈠浜?`video_copy_back_frames=0` 鐨?native direct 璺緞锛岃鏄庢牴鍥犳槸甯ф睜缁戝畾鏂瑰紡锛岃€屼笉鏄‖浠剁粷瀵逛笉鏀寔銆?3. 鍥犳鎴愮啛鎾斁鍣ㄥ拰褰撳墠鏃у疄鐜扮殑宸窛锛屼富瑕佷笉鍦ㄢ€滄湁娌℃湁 D3D11鈥濓紝鑰屽湪鈥滄湁娌℃湁瀹屾暣鎺ョ D3D11VA frames allocation policy 鍜屽绾?fallback鈥濄€?
### 澶勭悊缁撴灉
- `include/core/player_core.h`锛?  - 鏂板 `configureD3D11HardwareFramesContext(...)`
  - 鏂板 `selectSoftwarePixelFormat(...)`
- `src/core/player_core.cpp`锛?  - 鍦?`get_format()` 闃舵閫氳繃 `avcodec_get_hw_frames_parameters()` 鍒涘缓鑷畾涔?`hw_frames_ctx`
  - 涓?`AVD3D11VAFramesContext::BindFlags` 杩藉姞 `D3D11_BIND_SHADER_RESOURCE`
  - 鎶?`extra_hw_frames` 棰勭畻鍙犲姞鍒?`initial_pool_size`
  - 鑻ヨ嚜瀹氫箟 frames ctx 澶辫触锛屽垯閫€鍥?decoder-owned D3D11VA surface锛涜嫢杩愯鏃跺啀澶辫触锛屽垯缁х画鐢遍棶棰?90 鐨?renderer fallback 鍏滃簳
- `CHANGELOG / VERSION / DEVELOP_LOG` 宸插悓姝ヨ褰曟湰娆″疄鐜板崌绾с€?
### 鏈湴楠屾敹缁撴灉
- `Release` / `Debug` 鏋勫缓鍧囬€氳繃锛宍0 warnings / 0 errors`銆?- `juren-30s.mp4` 鐨?`--performance-log-check 2000` 鍦?`Release` / `Debug` 涓嬪潎杈撳嚭锛?  - `Configured D3D11VA frames context for direct shader sampling: bind_flags=520`
  - `video_native_output_frames=62`
  - `video_copy_back_frames=0`
  - `result=PASS`
- 璇存槑褰撳墠鏈哄櫒涓婂凡涓嶅啀渚濊禆 copy-back锛岀湡姝ｆ仮澶嶅埌 D3D11VA -> D3D11 native direct 鐨勯浂鎷疯礉鐩撮噰鏍烽摼璺€?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂 90: D3D11 鍘熺敓鐩撮噰鏍烽粦灞忥細杩愯鏃剁鐢?native direct 骞跺洖閫€ copy-back

**鏃ユ湡**: 2026-03-23
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鐢ㄦ埛鍙嶉锛歚Release` 鏋勫缓绋嬪簭鎾斁 `juren-30s.mp4` 鏃跺彧鏈夊０闊筹紝娌℃湁鐢婚潰锛屽苟鏄庣‘瑕佹眰鎺掓煡 `D3D11` 璺緞榛戝睆銆?- 闇€瑕佺‘璁ら棶棰樺睘浜庤В鐮佸け璐ャ€佹覆鏌撳け璐ワ紝杩樻槸 D3D11VA 涓?D3D11 renderer 鐨勮仈鍔ㄥ吋瀹规€ч棶棰樸€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. `renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣render_frames > 0` 璇存槑鎾斁涓婚摼璺湭鏂紝闂涓嶅湪鏂囦欢鎺㈡祴銆佽В澶嶇敤鎴栭煶棰戦摼銆?2. 鍛婅绋冲畾鍛戒腑 `CreateShaderResourceView1 for Y plane failed`锛岃〃鏄庨粦灞忔牴鍥犲湪 D3D11 鍘熺敓鐩撮噰鏍烽樁娈靛纭В琛ㄩ潰鍒涘缓 SRV 澶辫触锛岃€屾棫閫昏緫姝ゅ墠娌℃湁鎶婅繖绫昏繍琛屾椂澶辫触杞垚鏄庣‘闄嶇骇銆?3. `video_copy_back_frames > 0` 涓?`result=PASS` 璇存槑涓€鏃﹀仠姝?native direct锛岀幇鏈?copy-back + 杞欢绾圭悊涓婁紶閾捐矾鑳藉缁х画绋冲畾鍑哄浘銆?
### 澶勭悊缁撴灉
- `src/render/d3d11_video_renderer.cpp`锛?  - 鏂板 `disableNativeDirectRendering(...)`锛岀粺涓€鍥炴敹 native SRV 鐘舵€佸苟璁板綍涓€娆℃€у憡璀︺€?  - `ensureNativeShaderResourcesLocked(...)` 鍦?Y/UV plane `CreateShaderResourceView1` 澶辫触鏃剁鐢ㄥ綋鍓嶄細璇?native direct锛屽苟杩斿洖 `false`銆?  - `bindNativeFrameLocked(...)` 鍦ㄨВ鐮佽〃闈㈡牸寮忎笉鏀寔鐩存帴閲囨牱鏃跺悓鏍风鐢?native direct銆?  - `supportsNativeFrameFormat()` 鎺ュ叆 `native_direct_rendering_disabled_` 鐘舵€侊紝纭繚鍚庣画甯ф敼璧?`PlayerCore` 鐜版湁 copy-back 璺緞銆?- `CHANGELOG / VERSION / DEVELOP_LOG` 宸插悓姝ヨ褰曟湰娆′慨澶嶃€?
### 鏈湴楠屾敹缁撴灉
- `Release` / `Debug` 鏋勫缓鍧囬€氳繃锛宍0 warnings / 0 errors`銆?- `juren-30s.mp4` 鐨?`--performance-log-check 2000` 鍦?`Release` / `Debug` 涓嬪潎閫氳繃銆?- 涓や釜鏋勫缓鍧囪兘绋冲畾鎵撳嵃 `fallback=copyback-to-software`锛屽苟淇濇寔闈為浂 `video_copy_back_frames`銆乣render_frames`锛岃鏄庨粦灞忚矾寰勫凡琚繍琛屾椂闄嶇骇鎺ョ銆?
### 淇敼鏂囦欢
- src/render/d3d11_video_renderer.cpp
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂 80: 鏂囨。涓€鑷存€цˉ榻愶細CHANGELOG 绱㈠紩淇涓庨棶棰?69 analysis 鍥炲～

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠婂ぉ涓夋鎻愪氦瀹屾垚鍚庯紝缁х画瀵圭収 records / analysis 鏃跺彂鐜颁袱澶勬枃妗ｄ竴鑷存€х己鍙ｏ細

  - `CHANGELOG` 鐨勯棶棰樻€昏〃婕忎簡 `闂 78`

  - `闂 69` 鍙湁 records 涓変欢濂楋紝娌℃湁瀵瑰簲鐨?implementation planner analysis 鏂囨。

- 杩欎袱涓棶棰樹笉浼氬奖鍝嶈繍琛屾椂琛屼负锛屼絾浼氬奖鍝嶆柊浼氳瘽鎺ョ画銆侀棶棰樼储寮曟绱㈠拰鍚庣画鏂囨。瀹¤銆?


### 鏃ュ織杈撳嚭

```text

docs/records/CHANGELOG.md 闂鎬昏〃褰撳墠瀛樺湪 77 -> 79 鐨勮烦鍙凤紝浣嗘鏂囧凡瀛樺湪鈥滈棶棰?78鈥濄€?
docs/analysis/ 鐩綍褰撳墠缂哄皯涓庘€滈棶棰?69: PlayerCore 鍋滄挱鏀跺彛銆佸寘闃熷垪鎵€鏈夋潈涓?Clock/Demuxer 璁捐鍊轰慨澶嶁€濆搴旂殑鍗曠嫭鍒嗘瀽鏂囨。銆?
```



### 鍒嗘瀽璁板綍

1. `闂 78` 灞炰簬姝ｆ枃宸插啓銆佺储寮曟紡琛ワ紝闂鏈川鏄墜宸ョ淮鎶ゆ€昏〃鏃堕仐婕忥紝涓嶆槸鍐呭缂哄け銆?
2. `闂 69` 鐨?records 鍐呭鏈韩瀹屾暣锛屼絾涓庡綋鍓?implementation planner 宸ヤ綔鏂瑰紡鐩告瘮锛岀‘瀹炵己灏戝彲鐙珛寮曠敤鐨?analysis 鏂囨。銆?
3. 鍥犳杩欒疆鏈€灏忎慨姝ｅ簲鏄細

   - 琛ョ储寮曪紝涓嶆敼鏃㈡湁闂缂栧彿鍜屾鏂囬『搴?
   - 鍥炲～涓€绡囧彧瑕嗙洊 `闂 69` 鐨勫垎鏋愭枃妗ｏ紝骞舵妸 records 涓殑寮曠敤琛ラ綈



### 澶勭悊缁撴灉

- `CHANGELOG` 闂鎬昏〃宸茶ˉ `闂 78`锛屽苟鏂板鏈鏂囨。涓€鑷存€т慨澶嶇殑 `闂 80`銆?
- 宸叉柊澧?`docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`锛屽洖濉?`闂 69` 鐨?implementation planner 鍜屽叧閿粨璁恒€?
- `CHANGELOG / VERSION / DEVELOP_LOG` 宸插悓姝ヨˉ榻愯繖娆℃枃妗ｄ竴鑷存€т慨姝ｃ€?


### 鏈湴楠屾敹缁撴灉

- 鏈噸鏂版瀯寤猴紱鏈疆浠呬慨姝ｆ枃妗ｄ竴鑷存€э紝涓嶆秹鍙婁唬鐮侀€昏緫鍙樻洿銆?
- 褰撳墠 records 涓?analysis 鐨勫搴斿叧绯诲凡琛ラ綈鍒帮細

  - `闂 69 -> PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`

  - `闂 78 -> CHANGELOG 闂鎬昏〃宸插彲鐩存帴绱㈠紩`



### 淇敼鏂囦欢

- docs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂 79: PlayerCore 杩愯鎬?software send probe 瀵圭収鏀舵暃

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 寰呰В鍐?
### 闂鎻忚堪

- 鍦ㄩ棶棰?78 宸叉妸 software decode blocker 鏀舵暃鍒扳€滈涓?`avcodec_send_packet()` 娌℃湁瀹屾垚杩斿洖鈥濆悗锛屾湰杞户缁寜 implementation planner 鍋氱湡瀹炶繍琛屾€佸鐓э紝鐩爣鏄户缁缉灏忚寖鍥淬€?
- 褰撳墠闇€瑕佸洖绛旂殑闂鏄細杩欎釜 blocker 鍒板簳鏄?FFmpeg software decode 鏈綋銆乸acket 浜ゆ帴銆乨emux read-ahead銆侀煶棰戦摼锛岃繕鏄?`PlayerCore` 杩愯鎬佽嚜韬€犳垚鐨勩€?
### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

1. 鐙珛 `send-probe` 鍦?`pre-receive + packet queue round-trip + read-ahead=512` 鍚庝粛 `result=PASS`锛岃鏄?FFmpeg software decode 鏈綋銆乸acket queue 浜ゆ帴銆乣receive->send` 椤哄簭鍜?demux 缁х画璇诲寘閮戒笉鏄牴鍥犮€?
2. `video-only` 瀵圭収涓嬩粛 `video_packet_dequeue_count=1 / video_send_packet_ok=0 / decode_video_ok=0 / render_frames=0`锛岃鏄?blocker 涓庨煶棰戣緭鍑恒€丄udio master 鏃犲叧銆?
3. `MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD=1` 涓嬶紝`PlayerCore` 杩愯鎬侀噷鐨?software `send_packet` 渚濇棫 500ms 瓒呮椂锛岃鏄庨棶棰樹篃涓嶅彧鏄€滃綋鍓?video decode 绾跨▼涓婁笅鏂団€濇湰韬€?
4. 褰撳墠鏈€纭殑缁撹鏄細blocker 宸茬粡鏀舵暃鍒?`PlayerCore` 杩愯鎬侀噷鐨?software codec context / surrounding state 宸紓锛屽悗缁簲鐩存帴瀵规瘮 `PlayerCore::initDecoders()` 浜у嚭鐨?codec ctx 瀛楁涓庣嫭绔?probe銆?
### 澶勭悊缁撴灉

- 鏂板 Day15 鍒嗘瀽鏂囨。锛岀郴缁熻褰曟湰杞墍鏈?probe 瀵圭収涓庣粨璁恒€?
- 鎵╁睍 `--software-video-send-probe` 涓?`--software-video-decode-check` 鐨勭粨鏋勫寲杈撳嚭锛岄伩鍏嶅悗缁噸澶嶈蛋寮矾銆?
- `PlayerCore` 宸茶ˉ涓€涓粎鐜鍙橀噺寮€鍚殑 offthread send 璇婃柇璺緞锛屼緵涓嬩竴杞户缁拤姝?runtime context 宸紓銆?
### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
- `--software-video-send-probe .\juren-30s.mp4 1500`锛歚result=PASS`銆?
- `video-only --software-video-decode-check .\juren-30s.mp4 1200`锛歚result=FAIL`銆?
- `video-only + offthread send --software-video-decode-check .\juren-30s.mp4 1200`锛歚result=FAIL`锛屽苟鎵撳嵃瓒呮椂鏃ュ織銆?
### 淇敼鏂囦欢

- src/main.cpp

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY15_PLAYERRUNTIME_SOFTWARE_SEND_PROBE.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

## 闂 78: software decode 鏈€灏?send/dequeue 璁℃暟鎺ュ叆涓庨鍖呴€佸寘鍋滄粸閽夋

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰鐩存帴娌?software decode 棣栧寘鍋滄粸鏂瑰悜琛ユ渶灏忚鏁帮細`video packet dequeue 娆℃暟 / send 鎴愬姛娆℃暟 / send 杩斿洖鐮乣銆?
- 杩欎竴姝ヤ笉搴旂户缁厛鍔?`SoftwareSDL` 娓叉煋渚э紝鑰屽簲璇ュ厛鎶?software path 棣栧寘闃舵鍗″湪鍝竴灞傞拤姝汇€?


### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

1. `juren-30s.mp4` 鐨?`--performance-log-check` 宸茬粡鎵撳嵃鍑?`video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`锛岃鏄庢柊璁℃暟瀛楁宸茬粡浠?`PlayerCore` 姝ｅ父閫忎紶鍒?CLI銆?
2. software decode 鏍锋湰鐨?1 绉掕瘖鏂樉绀?`v_pkt_deq=1`锛岃鏄?video decode 绾跨▼骞朵笉鏄崱鍦ㄢ€滃彇涓嶅埌瑙嗛鍖呪€濄€?
3. 鍚屾椂 `v_send_ok=0` 涓?`v_send_ret=-2147483648` 浠嶆槸鏈繑鍥炲摠鍏靛€硷紝璇存槑鍒拌瘖鏂偣涓烘棣栦釜 packet send 杩樻病鏈夊舰鎴愪竴娆″畬鎴愯繑鍥炪€?
4. 缁撳悎姝ゅ墠涓€鐩寸己澶辩殑 `Video decode first send_packet returned` 鏃ュ織锛屾湰杞彲浠ユ妸 blocker 鍐嶆敹绱т竴姝ワ細褰撳墠 software path 鏇村儚鏄崱鍦ㄩ涓?`avcodec_send_packet(video_codec_ctx_, packet.get())` 璋冪敤鏈韩銆?
5. 杩欎篃杩涗竴姝ヨ瘉鏄庯細褰撳墠 blocker 杩樻病璧板埌 copy-back / swscale / display copy 闃舵銆?


### 澶勭悊缁撴灉

- 鍦?`PlayerCore` 琛?`video_packet_dequeue_count / video_send_packet_ok / video_send_packet_last_ret`銆?
- 鎶婃柊瀛楁鎺ュ埌 `DiagnosticsSnapshot`銆佷綆棰戣瘖鏂棩蹇椼€乣--performance-log-check` 涓?`--software-video-decode-check`銆?
- 鏂板 Day14 鍒嗘瀽鏂囨。锛岃褰曗€滈涓棰戝寘宸茬粡 dequeue锛屼絾棣栦釜 send 浠嶆湭鎴愬姛杩斿洖鈥濈殑鏈€鏂扮粨璁恒€?


### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
- `--performance-log-check .\juren-30s.mp4 1200`锛歚video_packet_dequeue_count=57 / video_send_packet_ok=57 / video_send_packet_last_ret=0 / result=PASS`銆?
- software decode 鏍锋湰杩愯鏈熻瘖鏂細`v_pkt_deq=1 / v_send_ok=0 / v_send_ret=-2147483648`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画涓嬩竴姝ワ紝骞堕『鎵嬫妸浠ｇ爜鏂囦欢閲屾畫鐣欑殑娉ㄩ噴涔辩爜娓呯悊鎺夈€?
- 褰撳墠鐩爣浠嶆槸 software video decode blocker锛氱‘璁ゅ湪淇濆畧绾跨▼閰嶇疆涓嬶紝soft decode 鏄惁宸茬粡鑳界湡瀹炰骇甯с€?


### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

1. `Video decoder threading: backend=Software thread_count=1 thread_type=none` 宸茬粡璇佹槑鏈疆澶嶆牳鍛戒腑浜?software decode 淇濆畧绾跨▼閰嶇疆銆?
2. 鍗充究濡傛锛宍decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0 / video_frame_queue_peak_size=0` 浠嶇劧鍏ㄩ儴涓?0锛岃鏄?blocker 涓嶄細鍥犱负鎶?FFmpeg 杞欢瑙ｇ爜绾跨▼鏀剁揣鍒板崟绾跨▼灏辫嚜鍔ㄦ秷澶便€?
3. 缁撳悎 `demux(v=163)` 涓?`pkt_q(v=162)` 鍙互鎺ㄦ柇锛? 绉掔獥鍙ｉ噷 video decode 绾跨▼鍙湡姝ｆ秷璐逛簡 1 涓棰戝寘銆?
4. 鍚屾椂鏃ュ織鍙嚭鐜?`Video decode first send_packet start`锛屾病鏈夌湅鍒板搴旂殑 `returned`锛屽洜姝ゅ綋鍓嶆洿鍍忔槸鍗″湪棣栦釜瑙嗛鍖呮彁浜ら樁娈碉紝鎴栧垰鎻愪氦棣栧寘鍚庡氨澶卞幓鍚庣画鎺ㄨ繘銆?
5. `video_copy_back_frames=0 / video_swscale_frames=0` 缁х画璇存槑杩欎釜 blocker 杩樻病杩涘叆 copy-back 鎴?swscale 闃舵銆?
6. 浠ｇ爜娉ㄩ噴涔辩爜鏂归潰锛屾湰杞‘璁?`src/render/sdl_video_renderer.cpp` 浠嶆湁 9 澶勭湡瀹?mojibake 娉ㄩ噴锛涗慨澶嶅悗鍐嶆鎵弿 `src/`銆乣include/` 鐨?`///` 涓?`//` 娉ㄩ噴琛岋紝鏈啀鍛戒腑鏂颁贡鐮併€?


### 澶勭悊缁撴灉

- 淇 `src/render/sdl_video_renderer.cpp` 鐨?9 澶勫嚱鏁板ご娉ㄩ噴涔辩爜锛屼粎鏀规敞閲婏紝涓嶆敼閫昏緫銆?
- 閲嶆柊鎵ц Debug 鏋勫缓锛岀粨鏋滀负 `0 warnings / 0 errors`銆?
- 閲嶆柊鎵ц `--software-video-decode-check`锛屽綋鍓?blocker 缁撹杩涗竴姝ユ敹鏁涗负锛歴oftware decode 鍦ㄤ繚瀹堢嚎绋嬮厤缃笅浠嶇劧涓嶄骇甯э紝涓旀洿鍍忔槸鈥滈涓棰戝寘鍚庡仠浣忊€濄€?
- 鏂板 Day13 鍒嗘瀽鏂囨。璁板綍鏈疆缁撹涓庝笅涓€姝ュ缓璁€?


### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`锛氱粨鏋勫寲杈撳嚭浠嶄负 `result=FAIL`銆?
- 浠ｇ爜娉ㄩ噴鎵弿锛氭湰杞湭鍐嶅彂鐜版柊鐨勫彲鐤戜贡鐮佸懡涓€?


### 淇敼鏂囦欢

- src/render/sdl_video_renderer.cpp

- docs/analysis/PLAYERCORE_DAY13_SOFTWARE_DECODE_FIRST_PACKET_STALL_AND_COMMENT_FIX.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂 76: Software video decode 鐪熷疄浜у抚涓撻」妫€鏌ヤ笌 blocker 瀹氫綅

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画鎸?implementation planner 鍗曞紑涓€涓€渟oftware video decode 鐪熷疄浜у抚涓撻」妫€鏌モ€濓紝涓嶈鍐嶉潬鍙兘璇佹槑鈥滄墦寮€鎴愬姛鈥濈殑 `session-check` 闂存帴鎺ㄦ柇銆?
- 褰撳墠宸茬煡 `SoftwareSDL + Software decode` 浼氬嚭鐜?0 甯ц緭鍑猴紝浣嗘棫鍛戒护鏃笉鑳界洿鎺ョ粰鍑虹粨鏋勫寲缁撹锛屽懡浠よ嚜韬繕鍙兘琚?soft decode 鐨?`stop/close` 鍗′綇銆?


### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

1. 鏂板懡浠ゅ凡缁忔妸鈥滃彧鏄繘鍏?playback loop鈥濆拰鈥滅湡瀹炰骇甯р€濆交搴曟媶寮€锛氭湰娆℃牱鏈噷 `open_ok=true`銆乣entered_playback_loop=true`銆乣advanced_seconds=1.89867`锛岃鏄庢挱鏀句細璇濆拰闊抽涓绘椂閽熼兘鍦ㄦ帹杩涖€?
2. 鍚屾椂 `renderer_backend=SoftwareSDL`銆乣decoder_backend=Software` 宸茬粡璇佹槑鍛戒护鍛戒腑浜嗙洰鏍囬摼璺紝鑰屼笉鏄張鎮勬倓閫€鍥?`D3D11VA`銆?
3. 浣?`decode_video_ok=0`銆乣scheduler_video_decoded_frames=0`銆乣render_frames=0`銆乣video_frame_queue_peak_size=0` 鍏ㄤ负 0锛岃鏄?blocker 鍙戠敓鍦ㄢ€滆蒋浠惰棰戣В鐮佷骇鍑哄抚鈥濅箣鍓嶏紝杩?frame queue 閮芥病鏈夎濉繃銆?
4. `demux_video_packets=163`銆乣demux_ignored_packets=0`銆乣demux_queue_drop_packets=0` 璇存槑闂涔熶笉鍦?demux 娌″杺鍖呮垨闃熷垪涓㈠寘銆?
5. `video_copy_back_frames=0`銆乣video_swscale_frames=0` 杩涗竴姝ヨ瘉鏄庤繖涓?blocker 涓?`copy-back / swscale / display copy` 鏃犲叧锛岀劍鐐瑰凡缁忔敹鏁涘埌褰撳墠 FFmpeg software video decode 鎺ュ叆閾炬湰韬€?
6. 鏃х増鐩存帴鍦ㄥ懡浠ゅ熬閮?`stop/close` 浼氳 blocker 鎷栨寕锛涘洜姝よ繖娆′笓椤规鏌ヨ鏀规垚 probe 寮忕‖閫€鍑猴紝杩欐湰韬篃鏄綋鍓?blocker 鐨勪竴涓梺璇併€?


### 澶勭悊缁撴灉

- `src/main.cpp` 鏂板 `--software-video-decode-check <media_file> [sample_ms]`銆?
- 鍛戒护鍐呴儴寮哄埗 `SoftwareSDL + Software decode + dummy audio`锛岄伩鍏嶇幆澧冨樊寮傛妸缁撹姹℃煋鎺夈€?
- 鍛戒护閫氳繃鏉′欢琚敹绱т负锛?
  - `decoder_backend=Software`

  - `decode_video_ok > 0`

  - `scheduler_video_decoded_frames > 0`

  - `render_frames > 0`

  - `video_frame_queue_peak_size > 0`

  - `video_copy_back_frames == 0`

- 鍛戒护鏀逛负 probe 寮忕‖閫€鍑猴紝纭繚鍗充娇 soft decode 璺緞鍗℃锛屼笓椤规鏌ヤ篃鑳界ǔ瀹氭墦鍗扮粨鏋勫寲 FAIL 缁撴灉銆?


### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃锛宍0 warnings / 0 errors`銆?
- `--software-video-decode-check .\samples\mkv\demo__h264_dts__1920x1080__30fps__2ch.mkv 2000`锛氳繑鍥為潪闆讹紝骞惰緭鍑?`result=FAIL`銆?
- 褰撳墠 blocker 宸茬粡琚崟鐙拤姝伙細杞欢璺緞鑳芥墦寮€銆佽兘杩涙挱鏀惧惊鐜€侀煶棰戞椂閽熻兘鎺ㄨ繘锛屼絾杞欢瑙嗛瑙ｇ爜閾炬病鏈夊舰鎴愪换浣曟湁鏁堣棰戝抚浜у嚭銆?


### 淇敼鏂囦欢

- src/main.cpp

- docs/analysis/PLAYERCORE_DAY12_SOFTWARE_VIDEO_DECODE_REAL_FRAME_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂 75: 鎾ゅ洖 SoftwareSDL automatic software-first 骞惰ˉ杞В闃诲璇婃柇

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画娌跨潃鈥滃浣曡繘涓€姝ュ噺灏戞垨瑙勯伩 `av_hwframe_transfer_data()` copy-back鈥濇帹杩涳紝浜庢槸鏈疆鍏堝皾璇曟妸 `SoftwareSDL` fallback 鏀规垚 renderer-aware `software-first`銆?
- 棰勬湡鏀剁泭鏄細濡傛灉 `SoftwareSDL` 鑳界洿鎺ョǔ瀹氭秷璐硅蒋浠惰В鐮佸抚锛屽氨鑳借繘涓€姝ュ帇浣庡綋鍓?fallback 閾惧彧鍓╀笅鐨?copy-back 鐑偣銆?
- 浣嗗疄闄呴獙璇佷腑锛宍SoftwareSDL + Software decode` 浼氳繘鍏モ€滈煶棰戠户缁帹杩涖€佽棰?0 甯ц緭鍑衡€濈殑鍥炲綊鐘舵€侊紝鍥犳闇€瑕佸厛鏀舵暃绛栫暐锛屽啀鍐冲畾鍚庣画鏂瑰悜銆?


### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

1. 鈥渟ystem-memory renderer 浼樺厛閬垮厤 copy-back鈥濊繖涓柟鍚戞湰韬病鏈夐棶棰橈紝涔熺鍚?`ffplay / mpv / MPC-HC` 涓€绫绘垚鐔熸挱鏀惧櫒鐨勫父瑙佽璁℃€濊矾銆?
2. 浣嗘湰杞复鏃?`software-first` 瀹為獙鏄剧ず锛宍SoftwareSDL + Software decode` 浼氬嚭鐜?`decode_video_ok=0 / render_frames=0 / video_frame_queue_peak_size=0`锛岃鏄庡綋鍓嶅伐绋嬬殑杞欢瑙嗛瑙ｇ爜鎺ュ叆閾炬湰韬笉鎴愮珛銆?
3. 杩涗竴姝ュ己鍒?`D3D11 + Software decode` 鍚庯紝浠嶅彲澶嶇幇杞欢瑙ｇ爜棣栧寘杩涘叆 `send_packet` 浣嗗悗缁笉褰㈡垚鏈夋晥瑙嗛杈撳嚭锛岃鏄?blocker 涓嶅湪 `SoftwareSDL` renderer锛岃€屽湪杞欢瑙嗛瑙ｇ爜鎺ュ叆銆?
4. 鍥犳褰撳墠鏈€鍚堢悊鐨勬敹鏁涙柟寮忎笉鏄户缁妸 `software-first` 鐣欏湪涓昏矾寰勶紝鑰屾槸鎾ゅ洖鑷姩鍒囨崲锛屼繚浣忓凡缁忛€氳繃楠岃瘉鐨?`D3D11VA copy-back` fallback銆?
5. 鍚屾椂淇濈暀鏈€灏忓繀瑕佽瘖鏂紝鍚庣画鍐嶅崟鐙慨杞欢瑙嗛瑙ｇ爜鎺ュ叆锛岃€屼笉鏄妸 fallback 涓绘祦绋嬬户缁嫋杩涘洖褰掋€?


### 瑙ｅ喅鏂规

- 鎾ゅ洖鑷姩 renderer-aware `software-first` decoder 鎺掑簭锛屾仮澶嶉粯璁?`D3D11VA -> Software` 椤哄簭銆?
- 淇濈暀杞欢瑙嗛瑙ｇ爜浣庨璇婃柇鑳藉姏锛?
  - FFmpeg 閿欒鐮佸瓧绗︿覆

  - 棣栨 `send_packet` 璧锋鎺㈤拡

  - stall 涓婁笅鏂囨棩蹇?
- 缁х画淇濈暀涓婁竴杞凡缁忓畬鎴愮殑 `SoftwareSDL` fallback 鏈夐檺閲嶆瀯锛?
  - `NV12 / YUV420P` 鐩翠紶

  - `AVFrame` 寮曠敤澶嶇敤

  - `swscale=0 / display_copy=0`



### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃銆?
- 榛樿涓婚摼楠岃瘉锛?
  `./build/Debug/modern-video-player.exe --performance-log-check ./samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1500`

  缁撴灉锛歚renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣video_copy_back_ratio_percent=0`銆乣video_swscale_ratio_percent=0`銆乣display_copy_ratio_percent=0`銆乣result=PASS`銆?
- `SoftwareSDL` fallback 楠岃瘉锛?
  `$env:MVP_RENDERER_BACKEND='software'; .\build\Debug\modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`

  缁撴灉锛歚renderer_backend=SoftwareSDL`銆乣decoder_backend=D3D11VA`銆乣video_copy_back_ratio_percent=33.5958`銆乣video_swscale_ratio_percent=0`銆乣display_copy_ratio_percent=0`銆乣result=PASS`銆?
- 寮哄埗 `D3D11 + Software decode` 鐨?`session-check` 浠嶅彧璇佹槑鈥滆兘鎵撳紑骞惰繘鍏ユ挱鏀惧惊鐜€濓紝涓嶈兘璇佹槑鈥滅ǔ瀹氫骇甯р€濓紱褰撳墠杞欢瑙嗛瑙ｇ爜 blocker 渚濈劧瀛樺湪锛屼絾宸蹭笉鍐嶅奖鍝嶉粯璁?fallback 涓昏矾寰勩€?


### 淇敼鏂囦欢

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY11_SOFTWARE_DECODE_BLOCKER_AND_FALLBACK_DIRECTION.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂 74: Audio-master lateness 鏀剁揣涓?SoftwareSDL 鍑忔嫹璐濇湁闄愰噸鏋?
**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛纭宸叉墜鍔ㄦ彁浜や笂涓€杞粨鏋滐紝瑕佹眰涓婚摼缁х画浼樺寲 `audio-master lateness / catch-up`锛屽悓鏃跺湪杞欢鍥為€€閾惧仛鏈夐檺閲嶆瀯锛屼紭鍏堝噺灏?`swscale` 涓庢樉绀哄眰娣辨嫹璐濄€?
- 褰撳墠榛樿 `D3D11` 涓婚摼宸茬‘璁ゆ槸 zero-copy锛屽洜姝よ繖杞笉鍋氬ぇ閲嶆瀯锛屽彧閽堝 `Audio` master 鏃跺簭鍜?`SoftwareSDL` fallback 鍋氬皬鑼冨洿浼樺寲銆?


### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

- 杩欒疆 `SoftwareSDL` 鐨勫叧閿敹鐩婁笉鏄噺灏?`copy-back`锛岃€屾槸鎶?copy-back 鍚庨潰鐨?`swscale + display memcpy` 涓ゆ鐑偣鐩存帴鎷挎帀浜嗐€?
- `Display` 鐜板湪鍦ㄦ stride 鐨?`YUV420P/NV12` 涓婁紭鍏堜繚鐣?`AVFrame` 寮曠敤锛屽洜姝?`display_copy_frames=0` 宸茬粡璇存槑鏄剧ず灞傛繁鎷疯礉涓嶅啀鏄?fallback 鐑偣銆?
- `NV12` 鍙互鐩存帴璧?`SDL_UpdateNVTexture()` 鍚庯紝`video_swscale_ratio_percent` 涔熷凡缁忛檷鍒?`0`锛涘洖閫€閾惧墿浣欎富瑕佺摱棰堝彧鍓?`av_hwframe_transfer_data()`銆?
- `Audio` master 鏂扮瓥鐣ヤ笅鐨?dummy audio 楠岃瘉鏄剧ず `wait_events=274`锛岃鏄庡垎娈电瓑寰呭凡缁忓洖鍒板悎鐞嗚寖鍥达紱鍓嶄竴娆″疄鐜伴噷鍑虹幇鐨勯珮棰戜吉蹇欑瓑宸茶鏈€灏?sleep 閲忓瓙淇銆?
- 榛樿 `D3D11 + D3D11VA` 涓婚摼浠嶇劧淇濇寔 zero-copy锛岃繖杞慨鏀规病鏈夋妸涓婚摼閲嶆柊鎷栧洖杞欢璺緞銆?


### 澶勭悊缁撴灉

- `PlayerCore` 鍦ㄦ棤瑙嗛婊ら暅鏃跺厑璁?copy-back 鍚庣殑杞欢 `NV12/YUV420P` 鐩存帴浜ょ粰 `SoftwareSDL`銆?
- `Display` 鐜板凡鏀寔 `NV12` SDL texture 鍜?`AVFrame` 寮曠敤澶嶇敤锛岃礋 stride/涓嶉€傞厤甯冨眬鎵嶅洖閫€娣辨嫹璐濄€?
- `Scheduler::pumpRenderOnce()` 鐨?`Audio` master 宸叉敼鎴愬垎娈电瓑寰?+ 鍔ㄦ€?late-drop 闃堝€?+ 鏈€灏?sleep 閲忓瓙銆?
- 褰撳墠缁撹宸茬粡鏀舵暃锛?
  - 榛樿涓婚摼缁х画淇濇寔 zero-copy

  - `SoftwareSDL` 鍥為€€閾句笅涓€闃舵鑻ョ户缁紭鍖栵紝搴斾紭鍏堝叧娉?`copy-back`锛岃€屼笉鏄户缁洿缁?`swscale`/`display memcpy`



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



---

## 闂 73: SoftwareSDL 鎷疯礉閾捐矾閲忓寲銆丼cheduler 閲嶅惎棰勭畻涓?renderer override

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪

- 鐢ㄦ埛缁х画杩介棶 `Display::copyFrameData()` 鍜?`Scheduler` 鍥哄畾閲嶅惎娆℃暟鏄惁浼氬鑷撮珮鐮佺巼/4K 杈撳嚭甯т笉绋冲畾銆?
- 褰撳墠涓婚摼宸茬粡纭鏄?`D3D11 + D3D11VA` zero-copy锛屼絾杞欢鍥為€€閾炬病鏈夌湡瀹炵粺璁★紝renderer override 涔熸病鏈夌湡姝ｆ帴鍏ラ€夋嫨閾俱€?
- 杩欒绗?8銆?0 鐐瑰彧鑳介潬鎺ㄦ柇锛屾棤娉曞湪鏈満缁欏嚭杞欢璺緞瀹炴祴鍗犳瘮銆?
### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

- 榛樿 `D3D11` 涓婚摼涓嬶紝`Display::copyFrameData()` 鏍规湰涓嶅湪鐑矾寰勶紝褰撳墠鏈哄櫒涓婄殑涓婚摼鐡堕渚濇棫涓嶆槸杞欢鎷疯礉銆?
- 涓€鏃﹀垏鍒?`SoftwareSDL`锛岄摼璺細鍙樻垚 `copy-back + swscale + display memcpy` 涓夋鍙犲姞锛屽叾涓?`Display::copyFrameData()` 鏈満瀹炴祴宸插崰閲囨牱绐楀彛绾?`21.84%`銆?
- `Scheduler` 鏂扮瓥鐣ヤ笅 `restart_limit_hits=0`锛岃鏄庡畠涓嶆槸杩欐壒鏍锋湰鐨勪富鍥狅紝浣嗙獥鍙ｉ绠楁瘮鍥哄畾娆℃暟鏇村畨鍏ㄣ€?
- 鏃х殑 fallback 妫€鏌ヤ箣鎵€浠ユ祴涓嶅噯锛屼笉鏄洜涓虹粨璁洪敊浜嗭紝鑰屾槸鍥犱负 renderer 閫夋嫨閾句箣鍓嶆牴鏈病鏈夋秷璐?override銆?


### 澶勭悊缁撴灉

- `Display` / `SdlVideoRenderer` / `PlayerCore` / CLI 鐜板湪宸茬粡鑳藉畬鏁磋緭鍑鸿蒋浠舵樉绀洪摼鐨勬嫹璐濈粺璁°€?
- `Scheduler` 鐜板凡鏀规垚绐楀彛棰勭畻鍒堕噸鍚紝骞舵柊澧?limit hit 璇婃柇銆?
- `RendererFactory` 鐜板凡鏀寔 `MVP_RENDERER_BACKEND` 鍜?`MVP_D3D11_DRIVER_HINT=software`锛宍--renderer-fallback-check` 褰撳墠宸叉仮澶?PASS銆?
- 褰撳墠鍙互鏄庣‘鍖哄垎涓ゆ潯缁撹锛?
  - 榛樿 `D3D11` 涓婚摼锛氱户缁繚鐣?zero-copy锛屼笉鍋氬ぇ閲嶆瀯

  - `SoftwareSDL` 鍥為€€閾撅細`Display::copyFrameData()` 宸叉槸鏄庣‘鐑偣锛屼絾杩樹笉鏄敮涓€鐑偣



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



---

## 闂 72: 楂樼爜鐜?4K 闃熷垪瀹归噺銆佽嚜閫傚簲鑺傛祦涓?copy-back 璇婃柇澧炲己

**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画鍥寸粫鈥滈珮鐮佺巼瑙嗛杈撳嚭甯т笉绋冲畾鈥濆仛浼樺寲锛屼笉鐩存帴鍋氬ぇ閲嶆瀯锛岃€屾槸鍏堝垽鏂?`FrameQueue` 瀹归噺銆佽儗鍘嬨€乧opy-back 鍜?scheduler 鏃跺簭杩欎簺鏇存帴杩戠湡瀹炵摱棰堢殑鐐广€?
- 褰撳墠椤圭洰宸茬粡鍏峰 D3D11 native zero-copy锛屼絾楂樼爜鐜?4K 鐩稿叧璇婃柇杩樼己 queue 宄板€笺€乸ush timeout銆佽儗鍘嬬瓑寰呮椂闀裤€乧opy-back/swscale 鏃堕棿杩欎簺鍏抽敭鎸囨爣銆?
- 鍚屾椂锛屾棤闊抽杈撳嚭鏃惰蛋 `Video` master锛宺ender 绛夊緟閫昏緫澶矖锛涜惤鍚庢椂涓€娆″彧涓竴甯э紝涔熶細鏀惧ぇ杩藉抚鎶栧姩銆?
### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

- 褰撳墠 4K 涓婚摼閲囨牱閲?`video_copy_back_frames=0`銆乣video_swscale_frames=0`锛岃鏄庣幇鍦ㄧ湡姝ｈ窇鐨勬槸 native zero-copy锛屼笉鏄?copy-back 鐑偣銆?
- 鍗曠函鎶?4K `FrameQueue` 鏀惧ぇ锛屼細瑙﹀彂 FFmpeg `Static surface pool size exceeded`锛涘洜姝よ棰戦槦鍒楀ぇ灏忓繀椤诲拰 `D3D11VA extra_hw_frames` 鑱斿姩銆?
- `scheduler_video_backpressure_wait_ms` 寰堥珮骞朵笉绛変簬鈥滄挱鏀惧け璐モ€濓紝瀹冩洿澶氳鏄?decoder 璺戝緱姣?render 蹇紱鐪熸瑕佺湅鐨勬槸 `push_timeout_count` 鍜?`demux_queue_drop_packets`锛岃繖杞袱鑰呴兘淇濇寔涓?0銆?
- `Video` master 涔嬪墠浼氳 24fps 鏍锋湰鏄庢樉瓒呴€燂紱`pumpRenderOnce()` 涓€娆″彧涓竴甯т篃浼氳 4K 杩藉抚杩囨參銆備袱鑰呴兘姣斺€滅嚎绋嬮噸鍚鏁伴檺鍒垛€濇洿鎺ヨ繎褰撳墠鐪熷疄鐥囩姸銆?


### 澶勭悊缁撴灉

- `FrameQueue` 鐜板凡鍏峰 `peak_size / push_timeout_count` 缁熻锛宍PlayerCore` 浼氬鍑?frame queue capacity/peak/timeout銆?
- `PlayerCore::open()` 浼氭牴鎹獟浣撳睘鎬ц缃?frame queue 瀹归噺锛屽苟鍦?`D3D11VA` 鎵撳紑鍓嶆樉寮忛厤缃?`extra_hw_frames`銆?
- `Scheduler` 鐜板凡鏀寔鑳屽帇杩熸粸涓?`video/audio_backpressure_wait_ms` 缁熻銆?
- `Scheduler::pumpRenderOnce()` 宸叉敼鎴愶細

  - `Video` master 涓嬬敤 wall-clock pacing

  - 鍗曟 pump 鍐呰繛缁涪寮冭繃鏈熷抚鐩村埌鎷垮埌鍙樉绀哄抚

- 褰撳墠 4K / 楂樼爜鐜?/ 闀挎椂鍥炲綊閮藉凡鎭㈠閫氳繃锛涜繖杞墿浣欏伐浣滈噸鐐逛笉鍐嶆槸鈥滄湁娌℃湁 zero-copy鈥濓紝鑰屾槸鍚庣画鏄惁缁х画琛?audio-master 涓嬫洿缁嗙殑 lateness 绛栫暐銆?


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



---





## 闂 71: 4K backend session 瀛愯繘绋嬮€€鍑鸿矾寰勪慨澶?
**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪

- 鍦?video-only 闄嶇骇鍜?demux 闂ㄧ绾犲亸涔嬪悗锛宍4k-playback-check` 浠嶇劧澶辫触锛屽け璐ョ偣闆嗕腑鍦?`fallback_ok=false`銆?
- 缁х画鎷嗗紑楠岃瘉鍚庡彂鐜帮細backend session 瀛愯繘绋嬫湰韬凡缁忔墦鍗?PASS锛屼絾 `hard` 浼氳秴鏃朵笉閫€鍑猴紝`soft` 浼氬紓甯搁€€鍑恒€?
- 杩欒鏄庢畫鐣欓棶棰樺凡缁忎笉鍦ㄦ挱鏀句富閾撅紝鑰屽湪娴嬭瘯 probe 瀛愯繘绋嬬殑閫€鍑虹瓥鐣ャ€?
### 鏃ュ織杈撳嚭

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



### 鍒嗘瀽璁板綍

- `runWindowsBackendSessionCheck()` 鐨勮亴璐ｅ彧鏄悜鐖惰繘绋嬫姤鍛娾€滆繖涓?backend 鑳藉惁鍚姩骞惰繘鍏ユ挱鏀剧獥鍙ｂ€濓紝骞朵笉闇€瑕佹部鐢ㄦ甯告挱鏀惧櫒浼氳瘽鐨勫畬鏁寸敓鍛藉懆鏈熷洖鏀惰涔夈€?
- 鏃у疄鐜扮殑闂鏄細probe 缁撴灉宸茬粡鎵撳嵃鍑烘潵浜嗭紝浣嗗瓙杩涚▼鏈韩娌℃湁鐢ㄧǔ瀹氱殑鏂瑰紡缁撴潫锛屽鑷寸埗杩涚▼鐪嬪埌鐨勬槸 timeout 鎴栧紓甯搁€€鍑虹爜锛岃€屼笉鏄?probe 缁撴灉銆?
- 鍥犱负 `4k-playback-check` 鎶?`mode_ok` 鍜?`exit_code==0` 涓€璧风撼鍏?`fallback_ok`锛屾墍浠ヨ繖绫婚€€鍑虹瓥鐣ラ敊璇細鐩存帴琛ㄧ幇鎴?4K 鍥炲綊澶辫触銆?


### 澶勭悊缁撴灉

- `src/main.cpp` 涓殑 `--windows-backend-session-check` 宸叉敼鎴愪笓鐢ㄩ€€鍑鸿矾寰勶細鎵撳嵃缁撴灉鍚?flush锛屽苟鍦?Windows 涓嬬敤 `TerminateProcess(GetCurrentProcess(), code)` 绔嬪嵆缁撴潫 probe 瀛愯繘绋嬨€?
- `hard/soft` backend session 褰撳墠閮借兘绋冲畾杩斿洖 `exit_code=0`銆?
- `--windows-backend-check` 涓?`--4k-playback-check` 褰撳墠宸叉仮澶?PASS锛岃鏄庤繖杞畫鐣欏け璐ュ凡缁忔敹鍙ｃ€?


### 淇敼鏂囦欢

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_BACKEND_SESSION_EXIT_FIX.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 闂 70: 闊抽璁惧澶辫触鏃剁殑瑙嗛-only闄嶇骇涓庡洖褰掗棬绂佺籂鍋?
**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪

- 鐢ㄦ埛纭鍏堜笉鍋氬ぇ閲嶆瀯锛屼紭鍏堝弬鑰?ffmpeg/mpv/MPC-HC 鐨勬€濊矾锛屾妸鈥滈煶棰戣澶囧け璐ユ椂鐨勮棰?only闄嶇骇鈥濆拰鈥滈珮鐮佺巼鍥炲綊璇垽鈥濊繖涓€灞傛敹鍙ｃ€?
- 褰撳墠鏈哄櫒涓?`WASAPI` 闊抽璁惧涓嶅彲鐢紝瑙嗛鏂囦欢瀹為檯涓婂凡缁忚兘闈犵幇鏈変富閾剧户缁挱锛屼絾 `open()` 璇箟銆乧lock source 鍜屽洖褰掗棬绂侀兘娌℃湁鎶婅繖涓姸鎬佹樉寮忚〃杈惧嚭鏉ャ€?
- 杩欏鑷撮珮鐮佺巼绱犳潗妫€鏌ラ噷 `demux_dropped_packets` 涓昏鐢?ignored audio packets 缁勬垚锛屽嵈浠嶇劧琚綋鎴愯儗鍘嬪け璐ャ€?
### 鏃ュ織杈撳嚭

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

宸叉垚鍔熺敓鎴愩€? 涓鍛婏紝0 涓敊璇?


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



### 鍒嗘瀽璁板綍

- `initDecoders()` 鏈韩宸茬粡鎸?`audio_player_->isInitialized()` 鎺у埗闊抽瑙ｇ爜鏄惁鍚敤锛屾墍浠ヨ繖杞笉闇€瑕佸厛鍋氬ぇ瑙勬ā閲嶆瀯锛涚湡姝ｇ己鐨勬槸 `open()` 灞傞潰鐨勬樉寮忕瓥鐣ャ€?
- 楂樼爜鐜囨鏌ュけ璐ョ殑鏃ф牴鍥犱笉鏄棰戦槦鍒楃湡鐨勮鍘嬬垎锛岃€屾槸 disabled audio path 浜х敓浜嗗ぇ閲?`demux_ignored_packets`锛屽張琚棫闂ㄧ璇綋鎴愮粺涓€鐨?demux drop銆?
- video-only 鍦烘櫙涓嬬户缁娇鐢?`System` clock 浼氳鎾斁鎺ㄨ繘鍜岀湡瀹炴覆鏌?PTS 鑴辫妭锛屽洜姝ら渶瑕佹妸鏃犻煶棰戣緭鍑烘椂鐨勪富鏃堕挓鍒囧埌 `Video`銆?
- `4k-playback-check` 褰撳墠鍓╀綑澶辫触鐐瑰凡缁忕缉灏忓埌 `runBackendSessionSubprocess()` 鐩稿叧鐨?`fallback_ok` 瀛愯繘绋嬭矾寰勶紝涓嶅啀鏄繖杞慨澶嶈寖鍥撮噷鐨勮鍒ら棶棰樸€?


### 澶勭悊缁撴灉

- `PlayerCore::open()` 鐜板凡鍖哄垎锛?
  - 瑙嗛鏂囦欢闊抽璁惧澶辫触锛歸arning + video-only 缁х画鎾斁

  - 闊抽-only 鏂囦欢闊抽璁惧澶辫触锛氱洿鎺ュけ璐?
- `DiagnosticsSnapshot` 鐜板凡鏂板 `audio_output_initialized / video_only_fallback / clock_source`锛屾挱鏀剧被妫€鏌ヤ細鐩存帴鎵撳嵃褰撳墠闄嶇骇妯″紡銆?
- `1080p60-check`銆乣high-bitrate-check`銆乣long-playback-check` 宸叉敼涓哄彧鐪?`demux_queue_drop_packets`锛屽苟淇濈暀 `demux_dropped_packets` 浣滀负鎬婚噺瑙傛祴鍊笺€?
- 褰撳墠楠岃瘉缁撴灉琛ㄦ槑锛氳繖杞慨澶嶅凡缁忔妸鈥滈煶棰戣澶囩己澶卞鑷寸殑鍋囧け璐モ€濅粠楂樼爜鐜囧洖褰掗噷鍓ョ鍑哄幓锛涘悗缁鏋滅户缁紭鍖栵紝搴斾紭鍏堟帓鏌?4K fallback 瀛愯繘绋嬭矾寰勫拰鏇撮暱鏈熺殑 queue serial 璁捐銆?


### 淇敼鏂囦欢

- `include/core/player_core.h`

- `src/core/player_core.cpp`

- `src/main.cpp`

- `docs/analysis/PLAYERCORE_DAY7_AUDIO_FALLBACK_AND_REGRESSION_GATES.md`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



---

## 闂 69: PlayerCore 鍋滄挱鏀跺彛銆佸寘闃熷垪鎵€鏈夋潈涓?Clock/Demuxer 璁捐鍊轰慨澶?


**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画鎶婁笂涓€杞鏌ュ彂鐜扮殑璁捐鍊虹洿鎺ヨ惤鍦颁慨鎺夛紝閲嶇偣鍖呮嫭锛欵OF/鍋滄挱绾跨▼鐘舵€佷笉瀹屾暣銆乣PacketQueue` 鍘熷鎸囬拡鎵€鏈夋潈銆乣Clock` system-clock 鏃堕棿璺冲彉锛屼互鍙?`Demuxer::open()` 鐨勮嚜閿侀闄┿€?
- 杩欑被闂涓嶄竴瀹氫細绔嬪嵆浣撶幇涓虹紪璇戝け璐ワ紝浣嗕細鍦?replay / seek / EOF / close 绛夐暱鏈熻繍琛岃矾寰勪笂绉疮鎴愮ǔ瀹氭€у拰鍙淮鎶ゆ€ч棶棰樸€?


### 鏃ュ織杈撳嚭

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

宸叉垚鍔熺敓鎴愩€?
0 涓鍛?
0 涓敊璇?
```



### 鍒嗘瀽璁板綍

- EOF 鑷姩鍋滄挱鍙戠敓鍦?scheduler render 绾跨▼鍐咃紝涓嶈兘鐩存帴鍚屾 `stop()`锛涘惁鍒?render 绾跨▼浼?join 鑷繁銆傛棫瀹炵幇鍥犳閫€鍖栨垚鈥滃彧鏀圭姸鎬佲€濈殑鍗婂仠鏈鸿矾寰勩€?
- `AVPacket*` 鏀捐繘閫氱敤闃熷垪鍚庯紝`clear()` / 闃熷垪鏋愭瀯涓嶄細甯」鐩噴鏀?FFmpeg 鍖咃紱seek銆乻top銆乧lose 閮戒細鍛戒腑杩欐潯娉勬紡璺緞銆?
- `Clock` 鐨?system-clock pause/speed 娌℃湁鍏堝浐鍖栨棫鏃堕棿鍩哄噯锛岀函瑙嗛鎾斁鏃朵細鍑虹幇鏆傚仠鏃堕棿鍊掗€€鎴栧€嶉€熷垏鎹㈣烦鏃躲€?
- `Demuxer::open()` 鎸侀攣璋冪敤 `close()` 鏄帴鍙ｇ骇鑷攣锛岃櫧鐒朵富娴佺▼涓嶅父澶嶇敤鍚屼竴瀹炰緥锛屼絾璁捐涓婂繀椤绘敹鍙ｃ€?


### 澶勭悊缁撴灉

- `PlayerCore` 鐜板湪鍏峰 deferred stop + worker reap 鏈哄埗锛孍OF銆丯ext/Previous銆丵uit 绛夎矾寰勪笉鍐嶇暀涓嬭剰绾跨▼鐘舵€侊紝鍚庣画 replay / restart 鍙畨鍏ㄩ噸鍚€?
- `PacketQueue` 宸叉敼涓?`unique_ptr<AVPacket, AvPacketDeleter>`锛宍ThreadSafeQueue` 鍚屾琛ヤ笂寤惰繜 move push锛屽帇缂╁寘鐢熷懡鍛ㄦ湡鍥炲綊 RAII銆?
- `Scheduler` 宸叉柊澧炲紓姝ュ仠鏈哄叆鍙ｅ苟鍦ㄩ噸鍚墠鍥炴敹鏃?worker锛宍Clock` 宸蹭慨澶?system-clock pause/speed 鍩哄噯鏇存柊銆?
- `Demuxer::open()` 涓嶅啀鍦ㄩ攣鍐呴噸鍏?`close()`锛涙暣宸ョ▼ Windows `Debug` 鍏ㄩ噺閲嶅缓浠嶄繚鎸?`0 涓鍛?/ 0 涓敊璇痐銆?


### 淇敼鏂囦欢

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



## 闂 68: MSVC warning debt 鍒嗗眰娓呯悊锛圕4819 / C4996 / C4706锛?


**鏃ユ湡**: 2026-03-18

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画娓呯悊鍏ㄥ眬 warning debt锛屽苟浼樺厛澶勭悊 `C4819` 缂栫爜鍛婅锛屼互鍙婄涓夋柟 / 鏈湴 `C4996` 鐨勫垎灞傛不鐞嗐€?
- 褰撳墠 Windows CI 宸蹭笉鍐嶆湁缂栬瘧 blocker锛屼絾 warning 鎬婚噺杩囬珮锛屼細鎺╃洊鐪熷疄鍥炲綊淇″彿銆?
- 闇€瑕佸湪涓嶅洖閫€鏃㈡湁 D3D11/瀛楀箷鏀瑰姩鐨勫墠鎻愪笅锛屾妸鏈湴鍙慨 warning 鍜岀涓夋柟 warning 闅旂绛栫暐涓€璧疯ˉ榻愩€?


### 鏃ュ織杈撳嚭

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

宸叉垚鍔熺敓鎴愩€?
0 涓鍛?
0 涓敊璇?
```



### 鍒嗘瀽璁板綍

- `C4819` 鐨勬牳蹇冧笉鏄崟涓簮鏂囦欢鍧忔帀锛岃€屾槸 MSVC 浠嶆寜鏈湴浠ｇ爜椤佃鍙?UTF-8 婧愭枃浠讹紱杩欑被闂搴斾紭鍏堥€氳繃缂栬瘧閫夐」缁熶竴娌荤悊銆?
- 绗笁鏂?warning 涓嶅簲闈犱慨鏀?FFmpeg / Quill 婧愮爜瑙ｅ喅锛屾洿绋冲Ε鐨勫仛娉曟槸鎶婄涓夋柟澶存枃浠舵斁鍒板閮?warning 灞傦紝鐢?MSVC 鐨?`/external:*` 鏈哄埗缁熶竴闅旂銆?
- 鏈湴 `C4996` 鍜?`C4706` 浠嶇劧搴旇閫愪釜淇帀锛屽洜涓鸿繖浜?warning 鐩存帴鍙嶆槧椤圭洰鑷韩浠ｇ爜璐ㄩ噺銆?
- 鏈疆钀藉湴鍚庯紝鍏ㄩ噺 `Rebuild` 宸茶揪鍒?`0 warning / 0 error`锛岃鏄庡垎灞傜瓥鐣ュ拰鏈湴淇閮藉凡鐢熸晥銆?


### 澶勭悊缁撴灉

- `CMakeLists.txt` 宸蹭负 MSVC 鐩爣鍚敤 `/utf-8 /external:anglebrackets /external:W0`锛屾湰鍦扮紪鐮佸憡璀︿笌绗笁鏂瑰ご鏂囦欢 warning 宸叉樉钁楁敹鍙ｃ€?
- `src/logger.cpp` 宸叉敼涓哄畨鍏ㄧ幆澧冨彉閲忚鍙?helper锛屾竻鐞嗘湰鍦?`getenv` 鍛婅銆?
- `src/subtitle/srt_parser.cpp` 涓?`src/subtitle/ass_parser.cpp` 宸茶ˉ涓?Windows / 闈?Windows 鐨?`sscanf_s` / `std::sscanf` 鍒嗘敮銆?
- `src/main.cpp` 鐨?`signalHandler` 鍙傛暟鍜?`src/core/player_core.cpp` 鐨?demux push 寰幆宸叉敼鍐欙紝娓呮帀鏈湴鍓╀綑鐨?`C4100 / C4706`銆?
- 褰撳墠 Windows `Debug` 鍏ㄩ噺閲嶅缓宸茬粡杈惧埌 `0 涓鍛?/ 0 涓敊璇痐銆?


### 淇敼鏂囦欢

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

## 闂 67: ASS 鏍囩瑙ｆ瀽涓?UTF-16 瀛楀箷鑼冨洿淇



**鏃ユ湡**: 2026-03-18

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- D3D11 鍘熺敓瀛楀箷閾惧凡缁忔帴鍏?`ASS/SSA`锛屼絾 override 鏍囩瑙ｆ瀽瀵?`\fnArial`銆乣\rDefault` 杩欑被绱у噾鍐欐硶涓嶆纭紝甯歌鏍峰紡浼氳闈欓粯蹇界暐銆?
- `SubtitleTextRun` 鐨勫尯闂撮暱搴﹀鏋滅户缁部鐢?UTF-8 code point锛岃€屾覆鏌撶鐩存帴鎸?DirectWrite 鐨?UTF-16 range 浣跨敤锛屽氨浼氬湪 emoji銆佹墿灞?CJK 鎴栧叾浠栭潪 BMP 瀛楃涓婂嚭鐜版牱寮忛敊浣嶃€?
- 鐢ㄦ埛瑕佹眰鍦ㄥ墠涓€鎵规彁浜ゅ悗缁х画鎶婂墿浣欐纭€ч棶棰樻竻鎺夛紝骞剁粰鍑鸿繖杞墿浣欒ˉ涓佺殑鎻愪氦鍛戒护銆?


### 鏃ュ織杈撳嚭

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

宸叉垚鍔熺敓鎴愩€?
167 涓鍛?
0 涓敊璇?
```



### 鍒嗘瀽璁板綍

- 杩欒疆澶嶆煡娌℃湁鍙戠幇鏂扮殑楂樺嵄鍐呭瓨娉勬紡鐐癸紱闂闆嗕腑鍦ㄥ瓧骞曡涔夋纭€э紝鑰屼笉鏄?COM/AVFrame 鐢熷懡鍛ㄦ湡绠＄悊銆?
- `ASS/SSA` override 瑙ｆ瀽鍣ㄦ鍓嶆妸鏍囩鍚嶈鍙栨垚鈥滆繛缁暟瀛?+ 杩炵画瀛楁瘝鈥濓紝鍥犳 `fn/r` 杩欑被鍏佽鐩存帴璺熷€肩殑鏍囩浼氭妸鍊奸娈佃鍚炶繘鏍囩鍚嶃€?
- `SubtitleTextRun.start/length` 鐩墠鐨勭湡姝ｆ秷璐硅€呮槸 Windows DirectWrite锛岃€屽畠瑕佹眰鐨勬枃鏈寖鍥村崟浣嶆槸 UTF-16 code unit锛屼笉鏄?UTF-8 code point銆?
- 褰撳墠鍏ㄩ噺閲嶅缓宸查噸鏂伴€氳繃锛涘墿浣?167 涓?warning 涓昏鏉ヨ嚜绗笁鏂瑰ご鏂囦欢锛團Fmpeg / Quill锛夈€侀」鐩唴澶氬婧愮爜鐨?C4819 缂栫爜鍛婅銆佷互鍙婂皯閲忔棦鏈夌殑 C4996/C4100 鎻愮ず锛屾湰杞湭鎵╂暎鎴愭柊鐨勬瀯寤洪樆濉炪€?


### 澶勭悊缁撴灉

- `src/subtitle/ass_parser.cpp` 宸茶ˉ涓婂父鐢?ASS 鏍囩鍓嶇紑鍖归厤锛屼慨澶?`\fnArial`銆乣\rDefault`銆乣\1c&H...&` 绛夋爣绛剧殑璇嗗埆璺緞銆?
- `src/subtitle/ass_parser.cpp` 涓?`src/render/d3d11_video_renderer.cpp` 宸茬粺涓€浣跨敤 UTF-16 code unit 璁＄畻 run 闀垮害锛岀‘淇濇牱寮忚寖鍥翠笌 DirectWrite 涓€鑷达紱骞堕『鎵嬫竻鐞嗕簡 `ass_parser.cpp` 鍐呮湰鍦?`sscanf` / 灞€閮ㄥ彉閲忛伄钄藉憡璀︺€?
- 涓や釜婧愮爜鏂囦欢鏈熬鐨勫浣欑┖琛屽凡娓呯悊锛屼繚鎸佹湰杞?diff 鍙寘鍚湁鏁堟敼鍔ㄣ€?
- 杩欒疆鍓╀綑琛ヤ竵鐜板湪鍙寘鍚€淎SS 鏍囩瑙ｆ瀽淇 + UTF-16 鑼冨洿淇 + 鏂囨。鍚屾鈥濓紝鍙嫭绔嬫彁浜ゃ€?


### 淇敼鏂囦欢

- src/subtitle/ass_parser.cpp

- src/render/d3d11_video_renderer.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



---

## 闂 66: 鍏ㄥ眬鏋勫缓闃诲娓呯悊涓?ASS/SSA 鍘熺敓 D3D11 瀛楀箷閾?


**鏃ユ湡**: 2026-03-18

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 褰撳墠宸ヤ綔鏍戣櫧鐒跺凡缁忓叿澶囧師鐢?D3D11 瑙嗛鍛堢幇鑳藉姏锛屼絾鍏ㄥ眬 `Debug|x64` 鏋勫缓浠嶈澶氬澶存枃浠?婧愭枃浠剁殑缂栫爜璇闃诲锛屽鑷存棤娉曠敤鏁村伐绋嬬粨鏋滈獙璇佷氦浠樼姸鎬併€?
- D3D11 鍘熺敓瀛楀箷閾炬鍓嶅彧瑕嗙洊绾枃鏈彔鍔狅紝`.ass/.ssa` 鐨勬牱寮忋€佸畾浣嶃€佸眰绾у拰澶?cue 鍚屽睆鑳藉姏杩樻病鏈夎繘鍏?native renderer銆?
- 澶栨寕瀛楀箷鑷姩鎺㈡祴涓庢樉寮忓姞杞介渶瑕佽鐩?`.ass`銆乣.ssa`銆乣.srt` 涓夌鏍煎紡锛岃€屼笉鏄仠鐣欏湪浠呴潰鍚?SRT 鐨勭姸鎬併€?


### 鏃ュ織杈撳嚭

```text

& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m

modern-video-player.vcxproj -> D:\VSProject\sssssssssssssss\modern-video-player\build\Debug\modern-video-player.exe

宸叉垚鍔熺敓鎴愩€?
0 涓鍛?
0 涓敊璇?
```



### 鍒嗘瀽璁板綍

- 鏋勫缓闃诲鐨勬牴鍥犱笉鍦?D3D11 涓婚摼閫昏緫锛岃€屽湪澶氬甯︿腑鏂囨敞閲婄殑澶存枃浠?婧愭枃浠惰褰撳墠 MSVC/浠ｇ爜椤电粍鍚堣璇诲悗瑙﹀彂鍏ㄥ眬璇硶閿欒銆?
- 鏃у瓧骞曟ā鍨嬪彧鎵胯浇绾枃鏈紝`IVideoRenderer` 涔熷彧鎺ユ敹鍗曞瓧绗︿覆锛屽洜姝?`PlayerCore` 鏃犳硶鎶?ASS/SSA 鐨勬牱寮忋€佸眰绾у拰瀹氫綅璇箟閫佸叆 D3D11 娓叉煋鍣ㄣ€?
- 鏃堕棿绾胯В鏋愬師鏈彧闈㈠悜鍗曟潯娲诲姩瀛楀箷锛屼笉瓒充互琛ㄨ揪 ASS/SSA 甯歌鐨勫 cue 鍚屽睆鍦烘櫙銆?
- 鏈€鍚堥€傜殑淇璺緞涓嶆槸鎶?ASS/SSA 鍐嶉€€鍥?SDL锛岃€屾槸鎵╁睍瀛楀箷瀵硅薄妯″瀷鍚庣户缁蛋鍚屼竴鏉?DXGI swap chain backbuffer 鍘熺敓鍙犲姞閾俱€?


### 澶勭悊缁撴灉

- 娓呯悊浜嗗叏灞€鏋勫缓闃诲鏂囦欢涓殑缂栫爜璇闂锛屾仮澶嶅畬鏁磋В鍐虫柟妗堟瀯寤哄熀绾裤€?
- 鏂板 `AssParser`锛屽苟鎵╁睍 `SubtitleStyle / SubtitleTextRun / SubtitleItem` 浠ユ壙杞?ASS/SSA 鏍峰紡淇℃伅銆?
- `PlayerCore` 鐜板湪浼氭敹闆嗗鏉″綋鍓嶆縺娲诲瓧骞曪紝閫氳繃 `IVideoRenderer::setSubtitleItems()` 鎶婄粨鏋勫寲瀛楀箷瀵硅薄閫佸叆娓叉煋鍣ㄣ€?
- `D3D11VideoRenderer` 宸插湪鍘熺敓 D3D11/D2D 璺緞涓敮鎸?ASS/SSA 甯哥敤鏍峰紡瀛楀箷缁樺埗锛氬～鍏呫€佹弿杈广€侀槾褰便€佽儗鏅銆佸榻愩€佸畾浣嶅拰 run 绾у瓧浣撴牱寮忋€?
- `VideoPlayer` 涓?`main.cpp` 宸叉敮鎸?`.ass/.ssa/.srt` 鍔犺浇涓庤嚜鍔ㄦ帰娴嬶紱瀹屾暣 `MSBuild` 楠岃瘉閫氳繃銆?
---

## 闂 57: D3D11 鍘熺敓 GPU 娓叉煋閾捐ˉ榻?


**鏃ユ湡**: 2026-03-18

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 褰撳墠浠撳簱宸茬粡鍏峰鍘熺敓 D3D11 瑙嗛闈㈠憟鐜颁笌 D3D11VA 璁惧鍏变韩锛屼絾瀛楀箷鍦?D3D11 娓叉煋鍣ㄥ唴鍙湁鐘舵€佸啓鍏ワ紝娌℃湁鐪熸缁樺埗鍒?backbuffer銆?
- 鏂囨。浠嶄繚鐣欌€淒3D11 鍙槸 SDL 鍖呰鍣ㄢ€濈殑鏃х粨璁猴紝宸茬粡涓庡綋鍓嶄唬鐮佷簨瀹炰笉涓€鑷淬€?
- 鐢ㄦ埛鐩爣鏄嬁鍒颁竴鏉″畬鏁寸殑銆佺嫭绔嬬殑銆佸師鐢?D3D11 GPU 娓叉煋閾撅紝鍥犳闇€瑕佹妸瀛楀箷鍙犲姞涓庤鏄庢枃妗ｄ竴璧疯ˉ榻愩€?


### 鏃ュ織杈撳嚭

```

Native D3D11 renderer initialized: window=...

D3D11VA decoder bound to renderer-owned D3D11 device

```



### 鍒嗘瀽璁板綍

- 瑙嗛涓婚潰宸茬粡鍙互鐩存帴娑堣垂 `AV_PIX_FMT_D3D11`锛屽苟瑕嗙洊 `NV12 / P010 / P016` 纭欢闈㈤噰鏍凤紱鐪熸缂哄彛鏄瓧骞曞彔鍔犱粛鏈惤鍒板師鐢熼摼鍐呫€?
- 鏈€鍚堥€傜殑琛ラ綈鏂瑰紡鏄湪 renderer-owned DXGI swap chain backbuffer 涓婂紩鍏?D2D1 / DirectWrite 鏂囨湰缁樺埗锛岃€屼笉鏄€€鍥?`Display` 鎴?SDL texture 鍙犲姞銆?
- `PlayerCore` 鐨?copy-back 鍒嗘敮闇€瑕佷繚鐣欙紝浣滀负鈥滆棰戞护闀滃紑鍚?鏍煎紡涓嶆敮鎸佲€濇椂鐨勬槑纭洖閫€杈圭晫銆?
- 瀹氬悜楠岃瘉琛ㄦ槑 `src/render/d3d11_video_renderer.cpp` 璇硶缂栬瘧閫氳繃锛涙暣宸ョ▼鏋勫缓澶辫触浠嶆潵鑷巻鍙叉崯鍧忓ご鏂囦欢锛岃€屼笉鏄湰娆?D3D11 淇敼鏈韩銆?


### 澶勭悊缁撴灉

- `D3D11VideoRenderer` 鏂板 D2D1 / DirectWrite 瀛楀箷缁樺埗璧勬簮鍜屾殏鍋滄€佸嵆鏃堕噸缁樸€?
- `CMakeLists.txt` 琛ラ綈 `d2d1`銆乣dwrite` 閾炬帴渚濊禆銆?
- 鏂板 `docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md`锛屽苟缁?`PLAYERCORE_DAY4_RENDERER_ANALYSIS.md` 鍔犲巻鍙茶鏄庛€?
---



## 闂 12: 浼佷笟绾у绾跨▼鏋舵瀯閲嶆瀯



**鏃ユ湡**: 2026-02-27

**鐘舵€?*: 宸插畬鎴?


### 闂鎻忚堪

鍘熸湁鏋舵瀯瀛樺湪浠ヤ笅闂锛?
1. 缁勪欢鑱岃矗涓嶆竻鏅帮紝VideoPlayer 鎵挎媴杩囧鑱岃矗

2. 绾跨▼妯″瀷澶嶆潅锛岄毦浠ョ淮鎶?
3. 鍐呭瓨绠＄悊瀹规槗鍑洪敊锛屽鑷村弻閲嶉噴鏀剧瓑 bug



### 瑙ｅ喅鏂规

閲嶆瀯涓轰紒涓氱骇澶氱嚎绋嬫灦鏋勶紝寮曞叆浠ヤ笅鏂扮粍浠讹細



1. **Demuxer** - 瑙ｅ皝瑁呭櫒锛屽皝瑁?AVFormatContext 鐨勮鍙栨搷浣?
2. **DecoderWorker** - 瑙ｇ爜宸ヤ綔绾跨▼锛屽皝瑁呭崟涓祦鐨勮В鐮侀€昏緫

3. **ThreadSafeQueue** - 绾跨▼瀹夊叏闃熷垪妯℃澘

4. **Clock** - 鏃堕挓鍚屾鍣紝绠＄悊闊宠棰戝悓姝?


### 鏋舵瀯鍥?
```

VideoPlayer (涓绘帶鍒跺櫒)

    鈹溾攢鈹€ Demuxer (瑙ｅ皝瑁呭櫒) 鈫?PacketQueue

    鈹溾攢鈹€ DecoderWorker (瑙嗛瑙ｇ爜绾跨▼)

    鈹溾攢鈹€ DecoderWorker (闊抽瑙ｇ爜绾跨▼)

    鈹溾攢鈹€ Display (娓叉煋鍣?

    鈹溾攢鈹€ AudioPlayer (闊抽鎾斁)

    鈹斺攢鈹€ Clock (鏃堕挓鍚屾)

```



---



## 闂 11: 骞跺彂璇诲彇 AVFormatContext 瀵艰嚧宕╂簝



**鏃ユ湡**: 2026-02-27

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

鎾斁瑙嗛鏃跺嚭鐜板ぇ閲?FFmpeg 瑙ｇ爜閿欒鍜岃闂啿绐佸穿婧冿細

```

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

[aac @ ...] Number of scalefactor bands in group (53) exceeds limit (49).

0xC0000005: 鍐欏叆浣嶇疆 0x0000022947799000 鏃跺彂鐢熻闂啿绐?
```

璋冪敤鍫嗘爤鏄剧ず閿欒鍦?`av_read_frame(format_ctx_, packet)` 澶勩€?


### 鏃ュ織杈撳嚭

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Video decode thread started

Audio decode thread started

[h264 @ ...] Invalid NAL unit size (-299142208 > 12338).

[h264 @ ...] missing picture in access unit with size 12342

... (澶ч噺绫讳技閿欒)

```



### 鍒嗘瀽璁板綍

1. 闂鍑虹幇鍦ㄨ棰戣В鐮佺嚎绋嬪拰闊抽瑙ｇ爜绾跨▼鍚屾椂杩愯鏃?
2. 涓や釜绾跨▼鍚勮嚜鎷ユ湁鐙珛鐨?VideoDecoder/AudioDecoder 瀹炰緥

3. 涓や釜瀹炰緥鍏变韩鍚屼竴涓?`AVFormatContext* format_ctx_`

4. 鍦?`decodeFrame()` 涓兘璋冪敤浜?`av_read_frame(format_ctx_, packet)`

5. 骞跺彂璇诲彇瀵艰嚧 packet 鏁版嵁閿欎贡锛孒264 瑙ｇ爜鍣ㄨ鍙栧埌閿欒鐨?NAL 鍗曞厓



### 鏍规湰鍘熷洜

**骞跺彂璋冪敤 `av_read_frame()` 瀵艰嚧鏁版嵁绔炰簤**銆俙AVFormatContext` 涓嶆槸绾跨▼瀹夊叏鐨勶紝涓や釜绾跨▼鍚屾椂璇诲彇浼氬鑷达細

- 璇诲彇浣嶇疆閿欎贡

- Packet 鏁版嵁琚鐩?
- 瑙ｇ爜鍣ㄦ敹鍒版崯鍧忕殑鏁版嵁



### 瑙ｅ喅鏂规

寮曞叆缁熶竴鐨?`PacketReaderThread` 浣滀负鍞竴鐨?packet 璇诲彇鍏ュ彛锛?
1. `PacketReaderThread` 鏄敮涓€璋冪敤 `av_read_frame()` 鐨勫湴鏂?
2. 璇诲彇鍒扮殑 packet 鏍规嵁 stream_index 鍒嗗彂鍒?`PacketQueue`

3. `VideoDecodeThread` 鍜?`AudioDecodeThread` 浠庡悇鑷殑 `PacketQueue` 鑾峰彇 packet

4. 瑙ｇ爜鍣ㄦ柊澧?`decodePacket()` 鏂规硶鎺ユ敹澶栭儴浼犲叆鐨?packet



---



## 闂 9: VideoFrame/AudioFrame 绉诲姩璇箟缂洪櫡瀵艰嚧宕╂簝



**鏃ユ湡**: 2026-02-25

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

绋嬪簭鍚姩鎾斁鍚庣珛鍗冲穿婧冿紝閿欒淇℃伅锛?
```

modern-video-player.exe - 搴旂敤绋嬪簭閿欒

0x00007FFF7A80DA4C 鎸囦护寮曠敤浜?0xFFFFFFFFFFFFFFFF 鍐呭瓨銆傝鍐呭瓨涓嶈兘涓?read

```



### 鏃ュ織杈撳嚭

```

Video decoder opened: 1920x1080, format: yuv420p

Audio decoder opened: 48000Hz, 2 channels, fltp

Playing video...

Video decode thread started

Audio decode thread started

(绋嬪簭宕╂簝)

```



### 鍒嗘瀽璁板綍

1. 閿欒鍦板潃 0xFFFFFFFFFFFFFFFF = -1锛岃〃绀虹┖鎸囬拡/鏃犳晥鎸囬拡

2. 宕╂簝鍙戠敓鍦ㄨ棰戣В鐮佺嚎绋嬪惎鍔ㄥ悗涓嶄箙

3. 浠ｇ爜瀹℃煡鍙戠幇 `VideoFrame` 鍜?`AudioFrame` 绫荤己灏戞纭殑绉诲姩璇箟瀹炵幇



### 鏍规湰鍘熷洜

`FrameQueue::pop()` 浣跨敤 `std::move` 灏嗗抚绉诲姩鍑洪槦鍒楋細

```cpp

frame = std::move(queue_.front());

queue_.pop();

```



`VideoFrame` 绫绘病鏈夊畾涔夌Щ鍔ㄦ瀯閫犲嚱鏁板拰绉诲姩璧嬪€艰繍绠楃锛?
- 榛樿鐨勭Щ鍔ㄦ搷浣滃彧鏄祬鎷疯礉 `frame_` 鎸囬拡

- 鍘熷璞℃瀽鏋勬椂璋冪敤 `av_frame_free(&frame_)` 閲婃斁鍐呭瓨

- 鐩爣瀵硅薄鐨?`frame_` 鍙樻垚鎮┖鎸囬拡

- 娓叉煋寰幆璁块棶鎮┖鎸囬拡瀵艰嚧宕╂簝



### 瑙ｅ喅鏂规

涓?`VideoFrame` 鍜?`AudioFrame` 绫诲疄鐜版纭殑绉诲姩璇箟锛?
1. 娣诲姞绉诲姩鏋勯€犲嚱鏁帮紝灏?`frame_` 鎸囬拡杞Щ

2. 娣诲姞绉诲姩璧嬪€艰繍绠楃锛屽皢 `frame_` 鎸囬拡杞Щ

3. 灏嗗師瀵硅薄鐨?`frame_` 璁句负 nullptr锛岄槻姝㈡瀽鏋勬椂閲婃斁鍐呭瓨



---



## 闂 13: Core API + Scheduler + Filter 澶氱嚎绋嬮噸鏋勮惤鍦?


**鏃ユ湡**: 2026-03-06

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐜版湁鎾斁鍣ㄤ富娴佺▼浠嶄互鏃ф灦鏋勪负涓績锛岀己灏戣鏍艰姹傜殑 `Core API / Scheduler / Filter` 妯″潡鍖栧垎灞傘€?
- 闊宠棰戣В鐮佺嚎绋嬮渶瑕佸湪鏂版牳蹇冧腑鐙珛璋冨害锛屽苟閫氳繃闃熷垪瀹炵幇鏃犻樆濉炶В鑰︺€?


### 鍒嗘瀽璁板綍

- 鏂板 `core` 灞傦細`frame/frame_queue/clock/command/scheduler/player_core`銆?
- 鏂板 `filters` 灞傦細`video_filter/audio_filter/filter_registry/filter_pipeline/builtin filters`銆?
- `VideoPlayer` 澧炲姞 `USE_NEW_PLAYER_CORE` 杩佺Щ寮€鍏宠矾寰勶紝淇濇寔鏃ф帴鍙ｄ笉鍙樸€?


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



## 闂 14: 鎾斁鍣ㄦ灦鏋勬敹鏁涗负 Core 鍗曡矾寰?


**鏃ユ湡**: 2026-03-06

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 椤圭洰鍚屾椂淇濈暀鏃ф挱鏀鹃摼璺拰鏂版牳蹇冮摼璺紝瀛樺湪缁存姢鍒嗗弶涓庡苟鍙戣璁￠闄┿€?


### 瑙ｅ喅鏂规

- `VideoPlayer` 浠呬繚鐣?`PlayerCore` 鍖呰瀹炵幇锛屽垹闄ゆ棫閾捐矾鍒嗘敮銆?
- CMake 鍒犻櫎鏃фā鍧楃紪璇戝叆鍙ｏ紝缁熶竴鍒?`core/*` + `filters/*`銆?
- 娓呯悊鏃уご婧愭枃浠讹紝淇濈暀蹇呰鍩虹璁炬柦鍜岃緭鍑烘ā鍧椼€?
- 鏂板鏋舵瀯閲嶆瀯鏂囨。锛歚docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`銆?


### 淇敼鏂囦欢

- CMakeLists.txt

- include/video_player.h

- src/video_player.cpp

- docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md

- 鍒犻櫎鏃фā鍧楁枃浠讹紙decoder/thread/sync/packet/legacy clock/frame_queue锛?


---



## 闂 15: 灏忓睆绐楀彛杩囧ぇ涓旀嫋鎷界缉鏀句笉绋冲畾



**鏃ユ湡**: 2026-03-06

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鎾斁鍣ㄥ湪灏忓睆璁惧涓婃寜瑙嗛鍘熷鍒嗚鲸鐜囷紙濡?1920x1080锛夌洿鎺ュ紑绐楋紝绐楀彛鍒濆鏄剧ず杩囧ぇ銆?
- 鐢ㄦ埛鎷栨嫿绐楀彛鍚庯紝閮ㄥ垎鐜涓嬫覆鏌撳尯鍩熸湭绋冲畾璺熼殢锛岃〃鐜颁负鈥滅獥鍙ｄ笉鑳芥甯歌皟鏁粹€濄€?


### 鏃ュ織杈撳嚭

```

Display initialized: window 1306x734 (source 1920x1080)

```



### 鍒嗘瀽璁板綍

- `PlayerCore::open()` 鐩存帴鎶婅棰戝垎杈ㄧ巼浼犵粰 `Display::init()`锛屾病鏈夋牴鎹睆骞曞彲鐢ㄥ尯鍋氶甯х獥鍙ｇ缉鏀俱€?
- 浜嬩欢澶勭悊鍙洃鍚?`SDL_WINDOWEVENT_RESIZED`锛屾湭瑕嗙洊甯歌鐨?`SDL_WINDOWEVENT_SIZE_CHANGED`銆?
- 娓叉煋鐩爣鐭╁舰鐩存帴閾烘弧绐楀彛锛岀己灏戞寜婧愯棰戞瘮渚嬭绠楃殑鐩爣鍖哄煙銆?


### 瑙ｅ喅鏂规

- 鍦?`Display::init()` 澧炲姞鍩轰簬 `SDL_GetDisplayUsableBounds()` 鐨勫垵濮嬬獥鍙ｅ昂瀵歌绠楋紝淇濇寔鍘熷瀹介珮姣斿苟闄愬埗鍒板睆骞曞彲鐢ㄥ尯 90%銆?
- 鍚屾椂澶勭悊 `SDL_WINDOWEVENT_RESIZED` 鍜?`SDL_WINDOWEVENT_SIZE_CHANGED`锛岀‘淇濇嫋鎷芥椂绐楀彛灏哄鐘舵€佸疄鏃舵洿鏂般€?
- 娓叉煋鏃舵寜瑙嗛瀹介珮姣旇绠?`dst_rect`锛岄伩鍏嶇獥鍙ｅ彉鍖栨椂鎷変几澶辩湡銆?


### 淇敼鏂囦欢

- src/display.cpp



## 闂 16: 绐楀彛鏈€澶у寲/缂╂斁鏃惰棰戠敾闈㈠崱浣忥紝缂哄皯鍩虹鎺у埗鏉?


**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 绐楀彛鏈€澶у寲鎴栨嫋鍔ㄧ缉鏀炬椂锛岃棰戠敾闈㈠鏄撳崱浣忥紝闊抽浠嶇户缁挱鏀俱€?
- 鎾斁鍣ㄧ己灏戣繘搴︽潯銆侀煶閲忚皟鑺傚拰鎷栧姩杩涘害鐨勫熀纭€鑳藉姏銆?


### 鏃ュ織杈撳嚭

```text

鐜拌薄澶嶇幇锛氱獥鍙ｆ渶澶у寲/缂╂斁鏃惰棰戠敾闈㈠仠姝㈠埛鏂帮紝闊抽缁х画銆?
```



### 鍒嗘瀽璁板綍

- 杩愯鏈?SDL 浜嬩欢澶勭悊涓庢覆鏌撳垎甯冨湪涓嶅悓绾跨▼锛岀獥鍙ｅ彉鍖栦簨浠朵笌娓叉煋璋冪敤骞跺彂鏃跺鏄撳嚭鐜版覆鏌撳仠婊炪€?
- 鎾斁鍣?UI 浠呮敮鎸侀敭鐩樻殏鍋?閫€鍑?鍏ㄥ睆锛岀己灏戝熀纭€浜や簰鎺т欢銆?


### 瑙ｅ喅鏂规

- 灏嗙獥鍙ｄ簨浠跺鐞嗘敹鏁涘埌娓叉煋璺緞锛坄Display::renderFrame`/`PlayerCore::onRenderIdle`锛変晶锛岄檷浣庣缉鏀句笌鏈€澶у寲鏃剁殑娓叉煋闃诲椋庨櫓銆?
- `Display` 鏂板鎺у埗灞傜粯鍒讹細杩涘害鏉?+ 闊抽噺鏉°€?
- 鏂板榧犳爣浜や簰锛氭嫋鍔ㄨ繘搴︽潯鍙戣捣 seek銆佹嫋鍔ㄩ煶閲忔潯瀹炴椂璋冭妭闊抽噺銆?
- `PlayerCore::pumpEvents` 鏂板瀵?seek/volume 璇锋眰鐨勬秷璐逛笌鎵ц銆?


### 淇敼鏂囦欢

- include/display.h

- src/display.cpp

- src/core/player_core.cpp

- src/main.cpp



## 闂 17: 浼佷笟绾?MPC-HC 鏋舵瀯鍓╀綑妯″潡缂哄皯鍙惤鍦颁唬鐮侀鏋?


**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐筹紙闃舵鎬э級



### 闂鎻忚堪

- `enterprise-quill-logging/tasklist.md` 涓ā鍧?02-14 浠嶆湁澶ч噺鏈畬鎴愰」锛岀己灏戠粺涓€鎺ュ彛涓庝唬鐮佽惤鍦扮偣銆?
- 鐜版湁鎾斁鍣ㄤ富閾捐矾鏈娊璞℃覆鏌撳悗绔紝闅句互鎵╁睍 D3D11/OpenGL 绛変紒涓氱骇娓叉煋妯″潡銆?


### 鍒嗘瀽璁板綍

- 褰撳墠宸ョ▼宸插叿澶?Core + Scheduler + Filter 鍩虹锛屼絾缂哄皯璺ㄦā鍧楄竟鐣屽畾涔夈€?
- 闇€瑕佸厛琛ラ珮浼樺厛妯″潡楠ㄦ灦骞舵帴鍏ユ瀯寤猴紝鍐嶉€愭濉厖瀹屾暣鑳藉姏銆?


### 瑙ｅ喅鏂规

- 鏂板鍩虹璁炬柦锛歚TaskQueue`銆乣FramePool`銆乣DecoderThread`銆?
- 鏂板娓叉煋妯″潡锛歚IVideoRenderer`銆丼DL 閫傞厤鍣ㄣ€丏3D11/OpenGL 鍗犱綅瀹炵幇銆乣RendererFactory`锛屽苟鎺ュ叆 `PlayerCore`銆?
- 鏂板闊抽澧炲己锛?0 娈靛潎琛″櫒銆佸娴佹贩闊冲櫒锛屾墿灞?`AudioPlayer` 缂撳啿瑙傛祴鎺ュ彛銆?
- 鏂板瑙ｇ爜鍣ㄧ鐞嗭細`IDecoder`銆乣DecoderCapability`銆乣DecoderFactory` 鑷姩閫夋嫨閫昏緫銆?
- 鏂板瀛楀箷/鎾斁鍒楄〃/璁剧疆/蹇嵎閿?鐨偆/鎻掍欢/鏍煎紡鏀寔/娴佸獟浣撶瓑浼佷笟绾фā鍧楅鏋躲€?
- 鏂板婊ら暅鍩虹被涓庨煶瑙嗛婊ら暅閾撅紝骞惰ˉ鍏呴煶閲忓钩琛℃护闀溿€?
- 鍚屾鏇存柊 `tasklist.md` 鍕鹃€夌姸鎬侊紙浠呭嬀閫夊凡钀藉湴浠ｇ爜椤癸級銆?


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



---



## 闂 18: 缂栬瘧鍩虹嚎鎭㈠涓庢牸寮忚兘鍔涚煩闃靛叆鍙?


**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `dash_manifest_parser.cpp` 鍦?VS2022/MSVC 缂栬瘧澶辫触锛岄樆濉炴湰鍦版寔缁紑鍙戙€?
- 缂哄皯鍙洿鎺ヨ繍琛岀殑鑳藉姏妫€鏌ュ叆鍙ｏ紝涓嶅埄浜庡揩閫熼獙璇佲€滀富鍔涙牸寮?+ 楂樺垎楂樺抚 + 澶氶煶閬撯€濈洰鏍囥€?


### 瑙ｅ喅鏂规

- 淇 DASH 姝ｅ垯 raw-string 鍒嗛殧绗︼紝鎭㈠宸ョ▼鍙紪璇戙€?
- 鏂板鍛戒护琛岃兘鍔涘叆鍙ｏ細

  - `--capabilities`锛氳緭鍑鸿繍琛屾椂瀹瑰櫒/缂栬В鐮佸櫒鑳藉姏涓庝富鍔涙牸寮忚鐩栫煩闃点€?
  - `--evaluate-target`锛氳瘎浼版寚瀹氬垎杈ㄧ巼/甯х巼/澹伴亾/鐮佺巼鐩爣鏄惁寤鸿纭В涓?D3D11 娓叉煋銆?
- 澧炲己鎾斁閾捐矾鍩虹鑳藉姏锛?
  - `Demuxer` 浣跨敤 `av_find_best_stream` + probe 鍙傛暟澧炲己锛?
  - `AudioPlayer` 鏆撮湶瀹為檯杈撳嚭鍙傛暟锛?
  - `PlayerCore` 澶嶇敤 `SwrContext`锛屾寜璁惧杈撳嚭鍙傛暟杩涜閲嶉噰鏍枫€?


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



## 闂 19: D3D11VA 纭В鏈€灏忛棴鐜紙澶辫触鍥為€€杞В锛?


**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 闇€瑕佸湪 Windows 涓嬩紭鍏堜娇鐢?D3D11VA 瑙ｇ爜楂樿礋杞借棰戯紝鍚屾椂閬垮厤纭В澶辫触瀵艰嚧鏃犳硶鎾斁銆?
- 纭欢杈撳嚭甯ф牸寮忎笌 SDL 娓叉煋閾捐矾涓嶄竴鑷达紙GPU/NV12 vs YUV420P锛夛紝闇€瑕佺粺涓€杈撳嚭鏍煎紡銆?


### 瑙ｅ喅鏂规

- 鍦?`PlayerCore::initDecoders` 涓姞鍏?D3D11VA 閰嶇疆涓庨€夋嫨閫昏緫銆?
- 褰撶‖瑙?`avcodec_open2` 澶辫触鏃讹紝鑷姩閲嶅缓瑙ｇ爜鍣ㄥ苟鍥為€€杞В缁х画鎾斁銆?
- 鍦ㄨ棰戣В鐮佽矾寰勮ˉ鍏呯‖浠跺抚杞瓨涓庡儚绱犳牸寮忚鏁达細

  - `av_hwframe_transfer_data`锛堢‖浠跺抚 -> 绯荤粺鍐呭瓨甯э級

  - `sws_scale`锛堥潪 YUV420P -> YUV420P锛?


### 淇敼鏂囦欢

- include/core/player_core.h

- src/core/player_core.cpp



---



## 闂 20: `--probe-file` 涓庢牸寮忓洖褰掕剼鏈?


**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 缂哄皯鍗曟枃浠跺彲鑴氭湰鍖栨帰娴嬪叆鍙ｏ紝涓嶄究浜庡揩閫熷畾浣嶁€滄煇涓牱鏈负浠€涔堜笉杈炬爣鈥濄€?
- 缂哄皯鎵归噺鍥炲綊鑴氭湰锛屼笉鍒╀簬鍗曚汉寮€鍙戣凯浠ｄ腑鎸佺画杩借釜鏍煎紡鍏煎鎬ч€€鍖栥€?


### 瑙ｅ喅鏂规

- 鏂板鍛戒护锛歚modern-video-player.exe --probe-file <media_file>`锛岃緭鍑?`probe.*` 閿€肩粨鏋滐紝渚夸簬鑴氭湰瑙ｆ瀽銆?
- 鏂板 `tools/format_regression/run_format_regression.ps1`锛氭寜 CSV 鏍锋湰娓呭崟鎵归噺鎺㈡祴骞惰緭鍑?Markdown 鎶ュ憡銆?
- 鎶ュ憡榛樿璺緞锛歚docs/reports/FORMAT_REGRESSION_yyyyMMdd_HHmmss.md`銆?
- 杩斿洖鐮佺害瀹氾細`0=PASS`锛宍1=PARTIAL`锛宍2=FAIL`銆?


### 楠岃瘉璁板綍

- `.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4` 杩斿洖 `probe.overall=PASS`銆?
- `.\tools\format_regression\run_format_regression.ps1` 鎴愬姛鐢熸垚鎶ュ憡銆?
- `-OutputFile` 鑷畾涔夎矾寰勯獙璇侀€氳繃銆?


### 淇敼鏂囦欢

- src/main.cpp

- tools/format_regression/run_format_regression.ps1

- tools/format_regression/format_samples.csv

- docs/workflows/FORMAT_REGRESSION.md

- docs/README.md



---



## 闂 21: GitHub Actions 鑷姩鍥炲綊鎺ュ叆



**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 褰撳墠鏍煎紡鍥炲綊浠呭湪鏈湴鎵嬪姩鎵ц锛孭R 缂哄皯鑷姩鍖栭棬绂併€?
- Windows CI 涓緷璧栧彂鐜版柟寮忎笌鏈湴 `external/` 鐩綍鏂规涓嶅悓锛屽瓨鍦ㄦ瀯寤轰笉涓€鑷撮闄┿€?


### 瑙ｅ喅鏂规

- 鏂板 `.github/workflows/format-regression.yml`锛?
  - 鑷姩涓嬭浇 `SDL2/FFmpeg` 棰勭紪璇戜緷璧栧苟鏋勫缓 `build/Debug/modern-video-player.exe`锛?
  - 璋冪敤 `tools/download_test_samples.ps1` 鐢熸垚鏍锋湰锛?
  - 璋冪敤 `tools/run_all_checks.ps1` 鎵ц鍗曟枃浠舵帰娴?+ 鎵归噺鍥炲綊锛?
  - 涓婁紶 `docs/reports/FORMAT_REGRESSION_CI.md` 浜х墿銆?
- 璋冩暣 `CMakeLists.txt`锛圵indows锛夛細

  - 浼樺厛鏀寔 `SDL2::`銆乣FFMPEG::`銆乣unofficial::ffmpeg::` 瀵煎叆鐩爣閾炬帴锛?
  - 缁х画淇濈暀 `external/SDL2` 涓?`external/ffmpeg` 鍥為€€璺緞銆?
- 璋冩暣 `tools/download_test_samples.ps1`锛?
  - 鏂板 PATH 鍙墽琛岃В鏋愰€昏緫锛屾敮鎸?`-FfmpegPath ffmpeg`銆?
- 鍚屾鏇存柊鍥炲綊鏂囨。涓庝换鍔℃竻鍗曞凡瀹屾垚椤广€?


### 淇敼鏂囦欢

- .github/workflows/format-regression.yml

- CMakeLists.txt

- tools/download_test_samples.ps1

- docs/workflows/FORMAT_REGRESSION.md

- docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



---



## 闂 22: M1 涓婚摼璺帹杩涳紙鎾斁鍒楄〃 + 璁剧疆 + 蹇嵎閿鐗堬級



**鏃ユ湡**: 2026-03-07

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 褰撳墠涓绘祦绋嬪彧鏈夊崟鏂囦欢鎾斁锛宍PlaylistManager`/`SettingsManager` 铏芥湁妯″潡浣嗘湭鎺ュ叆銆?
- 鍏抽敭蹇嵎閿湭瑕嗙洊浠诲姟娓呭崟鐩爣锛岀敤鎴蜂氦浜掓晥鐜囦笉瓒炽€?


### 瑙ｅ喅鏂规

- 鍦?`main` 涓绘祦绋嬫帴鍏ユ挱鏀惧垪琛細

  - 鏀寔澶氭枃浠跺弬鏁版瀯寤烘挱鏀惧垪琛紱

  - 鏀寔 `.m3u8` 鍔犺浇锛?
  - 鏀寔涓婁竴棣?涓嬩竴棣栦笌 EOF 鑷姩涓嬩竴棣栥€?
- 鍦?`main` 鎺ュ叆璁剧疆鎸佷箙鍖栵細

  - 鍚姩鍔犺浇 `config/player_settings.ini`锛?
  - 璁剧疆澶辫触鍥為€€榛樿锛?
  - 閫€鍑轰繚瀛橀煶閲忋€侀€熷害銆佺储寮曘€?
- 鎵╁睍娓叉煋浜嬩欢璇锋眰閾捐矾锛圖isplay -> Renderer -> PlayerCore -> VideoPlayer -> main锛夛細

  - seek 澧為噺璇锋眰銆侀€熷害璋冩暣璇锋眰銆佸垪琛ㄥ垏鎹㈣姹傘€侀€€鍑鸿姹傦紱

  - 瀹炵幇浠诲姟娓呭崟棣栫増榛樿閿綅闆嗭紙`Space/Enter/Esc/Q/Left/Right/Ctrl+Left/Ctrl+Right/Up/Down/M/PageUp/PageDown/[ ]/R`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 闇€姹傝姹傜Щ闄や袱涓?Core 娴嬭瘯鍙婄浉鍏虫枃浠躲€?
- `CMakeLists.txt` 涓粛瀛樺湪娴嬭瘯鐩爣涓庡紑鍏筹紝鐩存帴鍒犻櫎鏂囦欢浼氱暀涓嬫瀯寤烘畫鐣欍€?


### 鍒嗘瀽璁板綍

1. 宸茬‘璁ゅ緟绉婚櫎娴嬭瘯鏂囦欢涓?`tests/core_frame_queue_tests.cpp` 涓?`tests/core_clock_tests.cpp`銆?
2. 宸茬‘璁ゆ瀯寤鸿剼鏈腑瀛樺湪 `BUILD_CORE_TESTS`銆乣core_frame_queue_tests`銆乣core_clock_tests` 涓?`core_tests` 鑱氬悎鐩爣寮曠敤銆?
3. 闇€瑕佸悓姝ユ竻鐞?CMake 閰嶇疆涓庢枃妗ｈ褰曪紝淇濊瘉浠撳簱鐘舵€佷竴鑷淬€?


### 瑙ｅ喅鏂规

- 娓呯悊 `CMakeLists.txt` 涓殑涓や釜娴嬭瘯鐩爣銆佽仛鍚堢洰鏍囦笌娴嬭瘯寮€鍏炽€?
- 鍒犻櫎涓や釜娴嬭瘯鏂囦欢銆?
- 鍚屾鏇存柊 `CHANGELOG.md`銆乣VERSION.md`銆乣DEVELOP_LOG.md`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.1.1` 闇€瑕佹彁渚涘鎸傚瓧骞曞姞杞藉叆鍙ｃ€?
- 鐜版湁浠ｇ爜铏界劧鏈?`subtitle::SrtParser`锛屼絾涓绘祦绋嬫病鏈変换浣曞弬鏁板叆鍙ｆ垨鎾斁鍣ㄥ姞杞芥帴鍙ｃ€?


### 鍒嗘瀽璁板綍

1. 瀛楀箷瑙ｆ瀽妯″潡宸插叿澶囷紙`include/subtitle/srt_parser.h` + `src/subtitle/srt_parser.cpp`锛夈€?
2. `main` 鍙傛暟瑙ｆ瀽浠呮敮鎸佸獟浣撹緭鍏ャ€佽兘鍔涙煡璇㈠拰 probe锛岀己澶卞瓧骞曞叆鍙ｃ€?
3. `VideoPlayer` 涓嶆寔鏈夊鎸傚瓧骞曠姸鎬侊紝鏃犳硶鍦ㄦ挱鏀句細璇濅腑杩借釜宸插姞杞藉瓧骞曘€?


### 瑙ｅ喅鏂规

- `VideoPlayer` 鏂板澶栨寕瀛楀箷鍔犺浇鑳藉姏锛堜粎 SRT锛屽惈瀹归敊锛夛細

  - `loadExternalSubtitle()` / `clearExternalSubtitle()`锛?
  - 璁板綍宸插姞杞藉瓧骞曡矾寰勫拰鏉＄洰鏁般€?
- `main` 鏂板 `--subtitle <file.srt>` 鍏ュ彛锛屽苟淇濈暀鎾斁鍒楄〃鍙傛暟鑳藉姏銆?
- 鑻ユ湭鏄惧紡浼?`--subtitle`锛岃嚜鍔ㄥ皾璇曞姞杞藉悓鍚?`.srt` 鏂囦欢銆?
- 浠诲姟娓呭崟 `1.1.1` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.1.2` 瑕佹眰瀛楀箷娓叉煋鍙犲姞锛屼絾褰撳墠娓叉煋鎺ュ彛鍙湁鐢婚潰涓庢帶鍒跺眰锛屾病鏈夊瓧骞曢€氶亾銆?
- 浠诲姟娓呭崟 `1.1.3` 瑕佹眰瀛楀箷涓庢挱鏀?鏆傚仠/seek 鍚屾锛屼絾鎾斁鏍稿績鏈寜鏃堕挓椹卞姩瀛楀箷鏇存柊銆?


### 鍒嗘瀽璁板綍

1. `VideoPlayer` 宸插叿澶囧鎸傚瓧骞曞姞杞借兘鍔涳紙闂 24锛夛紝浣嗗瓧骞曟暟鎹湭杩涘叆娓叉煋閾捐矾銆?
2. 娓叉煋灞傛娊璞＄己澶卞瓧骞曟帴鍙ｏ紝SDL/D3D11/OpenGL 鍚庣鏃犳硶缁熶竴鎺ユ敹瀛楀箷鏂囨湰銆?
3. 鎾斁鏍稿績闇€瑕佸湪娓叉煋甯т笌绌洪棽浜嬩欢閮芥洿鏂板瓧骞曪紝鎵嶈兘瑕嗙洊鏆傚仠鎬佸拰 seek 鍚庨潤姝㈢敾闈€?
4. 瀛楀箷鏇存柊瀹炵幇涓瓨鍦ㄩ攣鍐呰皟鐢ㄦ覆鏌撴帴鍙ｉ闄╋紝闇€瑕佽皟鏁撮攣绮掑害銆?


### 瑙ｅ喅鏂规

- 娓叉煋鎶借薄鎵╁睍锛?
  - `IVideoRenderer` 澧炲姞 `setSubtitleText()`锛?
  - SDL 鍚庣杞彂鍒?`Display`锛?
  - D3D11/OpenGL 澧炲姞鍏煎妗╁疄鐜般€?
- `Display` 鏂板瀛楀箷鍙犲姞灞傦細

  - 鏂板瀛楀箷鐘舵€佷笌浜掓枼淇濇姢锛?
  - 鍦ㄦ瘡甯ф覆鏌撲腑鍙犲姞瀛楀箷闈㈡澘涓庢枃鏈紱

  - 鏀寔澶氳涓庤秴闀挎枃鏈鐞嗐€?
  - 褰撳墠瀛楁ā涓鸿交閲忓疄鐜帮紝闈?ASCII 瀛楃闄嶇骇鏄剧ず銆?
- `PlayerCore` 鏂板瀛楀箷鏃堕棿杞撮€昏緫锛?
  - 寮曞叆澶栨寕瀛楀箷杞ㄩ亾鐘舵€佺鐞嗭紱

  - 鍦?`renderFrame()` 涓?`onRenderIdle()` 涓皟鐢?`updateSubtitleOverlay()`锛?
  - 渚濇嵁褰撳墠鎾斁鏃堕棿閫夋嫨娲昏穬瀛楀箷锛屾敮鎸佹挱鏀?鏆傚仠/seek 鍚屾銆?
- 淇骞跺彂缁嗚妭锛?
  - 灏嗘覆鏌撹皟鐢ㄧЩ鍑哄瓧骞曚簰鏂ラ攣锛岄伩鍏嶉攣鍐呭洖璋冩覆鏌撳眰銆?
- 鏇存柊浠诲姟娓呭崟锛?
  - `1.1.2`銆乣1.1.3` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.1.4` 瑕佹眰鈥滃瓧骞曞紑鍏充笌寮傚父澶勭悊鈥濓紝浣嗗綋鍓嶇己灏戣繍琛屾椂瀛楀箷寮€鍏宠兘鍔涖€?
- 瀛楀箷鍔犺浇娴佺▼鍦ㄦ枃浠剁郴缁熷紓甯稿満鏅笅缂哄皯涓撻棬瀹归敊锛岀ǔ瀹氭€т笉瓒炽€?


### 鍒嗘瀽璁板綍

1. 鐜版湁杈撳叆浜嬩欢閾捐矾锛坄Display -> Renderer -> PlayerCore`锛夊彲鎵╁睍璇锋眰鍨嬫帶鍒讹紝閫傚悎鍔犲叆瀛楀箷寮€鍏炽€?
2. 瀛楀箷鏃堕棿杞存洿鏂板凡鍦?`PlayerCore` 鍐呴棴鐜紝鏂板鈥滃紑鍏崇姸鎬佲€濆嵆鍙鐢ㄧ幇鏈夊悓姝ラ€昏緫銆?
3. `VideoPlayer::loadExternalSubtitle()` 浣跨敤榛樿 `filesystem` 妫€鏌ユ柟寮忥紝瀛樺湪寮傚父浼犳挱椋庨櫓銆?


### 瑙ｅ喅鏂规

- 鏂板瀛楀箷寮€鍏虫帶鍒讹細

  - 鍦?`Display` 涓柊澧炲瓧骞曞紑鍏宠姹傚苟缁戝畾 `V` 閿紱

  - 鍦ㄦ覆鏌撳櫒鎺ュ彛鏂板 `consumeToggleSubtitleRequest()`锛?
  - 鍦?`PlayerCore` 涓柊澧?`setSubtitleEnabled()/toggleSubtitleEnabled()/isSubtitleEnabled()`锛?
  - 鍏抽棴瀛楀箷鏃朵富鍔ㄦ竻绌烘覆鏌撴枃鏈紝寮€鍚瓧骞曞悗鎸夊綋鍓嶆椂闂撮噸鏂伴€夊彇瀛楀箷銆?
- 鍔犲己寮傚父澶勭悊锛?
  - 瀛楀箷璺緞妫€鏌ユ敼鐢?`std::error_code`锛?
  - 鎹曡幏瀛楀箷瑙ｆ瀽寮傚父骞跺啓鍛婅鏃ュ織锛?
  - 鍔犺浇澶辫触淇濇寔涓绘祦绋嬪彲鎾斁锛屼笉淇濈暀鑴忓瓧骞曠姸鎬併€?
- 鍚屾浠诲姟鐘舵€侊細

  - `1.1.4` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.3.2` 闇€瑕佹敮鎸侀敭浣嶉厤缃寔涔呭寲銆?
- 褰撳墠蹇嵎閿€昏緫鐩存帴鍐欏湪 `Display` 鐨?`SDL_KEYDOWN` 鍒嗘敮锛屾棤娉曚粠閰嶇疆鍔犺浇骞跺湪閲嶅惎鍚庝繚鎸併€?


### 鍒嗘瀽璁板綍

1. 椤圭洰宸叉湁 `HotkeyManager`锛屼絾榛樿閿綅涓庝富娴佺▼瀹為檯閿綅涓嶄竴鑷达紝涓旀湭鎺ュ叆鎾斁閾俱€?
2. `SettingsManager` 宸茶兘璇诲啓 `player_settings.ini`锛屽彲澶嶇敤涓?`hotkey.*` 鎸佷箙鍖栭€氶亾銆?
3. 闇€瑕佹墦閫?`Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 鐨勭儹閿槧灏勯€忎紶閾捐矾銆?


### 瑙ｅ喅鏂规

- 鎵╁睍 `HotkeyManager`锛?
  - 瀵归綈棣栫増榛樿鍔ㄤ綔涓庨敭浣嶏紱

  - 鏂板鍔ㄤ綔閿悕鍜岄敭鐮佸簭鍒楀寲/鍙嶅簭鍒楀寲鑳藉姏銆?
- 浜嬩欢閾捐矾鎺ュ叆锛?
  - `Display` 鏀逛负閫氳繃 `HotkeyManager` 瑙ｆ瀽閿綅鍔ㄤ綔锛?
  - 娓叉煋鍣ㄦ帴鍙ｆ柊澧?`setHotkeyManager()`锛?
  - `PlayerCore` 涓?`VideoPlayer` 澧炲姞鐑敭绠＄悊 API 骞堕€忎紶鍒?SDL 娓叉煋鍣ㄣ€?
- 鎸佷箙鍖栨帴鍏ワ細

  - `main` 鏂板 `hotkey.*` 閰嶇疆鍔犺浇涓庡洖鍐欙紱

  - 闈炴硶閰嶇疆鑷姩闄嶇骇榛樿骞惰褰曞憡璀︼紱

  - 鏇存柊 `config/player_settings.ini` 榛樿鏍蜂緥銆?
- 浠诲姟鐘舵€佸悓姝ワ細

  - `1.3.2` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.3.3` 闇€瑕佹敮鎸佲€滈敭浣嶅啿绐佹娴嬩笌鎭㈠榛樿鈥濄€?
- 褰撳墠宸插叿澶?`hotkey.*` 鎸佷箙鍖栵紝浣嗙己灏戝啿绐佹不鐞嗭紝閲嶅閿綅浼氬鑷村姩浣滆涔変笉娓呮櫚銆?


### 鍒嗘瀽璁板綍

1. `HotkeyManager` 宸叉湁鍔ㄤ綔涓庨敭浣嶆槧灏勶紝閫傚悎鎵╁睍鍐茬獊鎵弿鎺ュ彛銆?
2. 閰嶇疆鍔犺浇娴佺▼鍦?`main` 涓泦涓鐞嗭紝閫傚悎缁熶竴鍔犲叆鍐茬獊妫€娴嬩笌鎭㈠绛栫暐銆?
3. 闇€瑕佷竴涓彲鏄惧紡瑙﹀彂鎭㈠榛樿鐨勯厤缃€氶亾锛屼究浜庣敤鎴疯嚜鏁戙€?


### 瑙ｅ喅鏂规

- 鍦?`HotkeyManager` 澧炲姞锛?
  - `resetToDefaults()`锛?
  - `findConflicts()` / `hasConflicts()`銆?
- 鍦ㄧ儹閿姞杞芥祦绋嬪鍔犲啿绐佹不鐞嗭細

  - 鍔犺浇閰嶇疆鍚庡厛鏋勫缓鍊欓€夋槧灏勶紱

  - 妫€娴嬪埌鍐茬獊鍒欒褰曡缁嗘棩蹇楀苟鑷姩鍥為€€榛樿閿綅锛?
  - 闈炴硶 token 淇濈暀榛樿骞剁粰鍑哄憡璀︺€?
- 澧炲姞鎭㈠榛樿閰嶇疆寮€鍏筹細

  - `hotkey.restore_defaults=true` 鏃跺惎鍔ㄨ嚜鍔ㄦ仮澶嶉粯璁わ紱

  - 鎭㈠鍚庤嚜鍔ㄧ疆鍥?`false`銆?
- 浠诲姟娓呭崟鍚屾锛?
  - `1.3.3` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.4.1` 闇€瑕侀獙璇?SRT 瀛楀箷鍦?seek 鍚庝粛涓庢挱鏀炬椂闂村悓姝ャ€?
- 褰撳墠缂哄皯鍙噸澶嶇殑鏈湴楠屾敹鍛戒护锛屾棤娉曠ǔ瀹氬仛鍥炲綊銆?


### 鍒嗘瀽璁板綍

1. `PlayerCore::updateSubtitleOverlay()` 宸插叿澶囩紦瀛樼储寮?+ 浜屽垎瀹氫綅閫昏緫锛屼絾鏃犳硶鑴辩鎾斁 UI 鐙珛楠岃瘉銆?
2. 闇€瑕佹妸瀛楀箷鏃堕棿杞村尮閰嶇畻娉曟彁鍙栦负鍙鐢ㄥ嚱鏁帮紝骞舵彁渚涘懡浠よ鑷鍏ュ彛銆?


### 瑙ｅ喅鏂规

- 鏂板 `subtitle::resolveActiveSubtitleIndex(...)` 骞剁敱 `PlayerCore` 缁熶竴澶嶇敤銆?
- 鏂板 `--subtitle-sync-check <subtitle.srt>`锛?
  - 鏈夊簭鏃堕棿杞存鏌ワ紱

  - seek 璺宠浆妫€鏌ワ紱

  - 杈撳嚭 mismatch 缁熻涓?PASS/FAIL銆?
- 鏂板鏍蜂緥鏂囦欢 `samples/subtitle/subtitle_seek_sync_sample.srt` 涓庢湰鍦版姤鍛?`docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`銆?
- 鏇存柊浠诲姟娓呭崟锛屽嬀閫?`1.4.1`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.4.2` 闇€瑕侀獙璇佹挱鏀惧垪琛ㄨ繛缁挱鏀?5 鏂囦欢銆?
- 缂哄皯鍙噸澶嶆墽琛岀殑鑷姩鍖栭獙鏀跺叆鍙ｃ€?


### 鍒嗘瀽璁板綍

1. 涓绘祦绋嬪凡鏀寔 EOF 鑷姩鍒囨崲涓嬩竴鏉＄洰锛屼絾杩愯缁撴灉鏈粨鏋勫寲杈撳嚭銆?
2. 鐜版湁 `--probe-file` 宸插叿澶囩ǔ瀹氱殑濯掍綋鍙墦寮€妫€娴嬭兘鍔涳紝鍙鐢ㄤ负楠屾敹鍓嶇疆妫€鏌ャ€?


### 瑙ｅ喅鏂规

- 鏂板 `--playlist-flow-check`锛?
  - 鏋勫缓鎾斁鍒楄〃骞惰姹傝嚦灏?5 鏉＄洰锛?
  - 妫€鏌ュ墠 5 鏉＄殑鍙墦寮€鐘舵€侊紱

  - 鎸?EOF 鑷姩鍒囨崲閫昏緫楠岃瘉椤哄簭瑕嗙洊 `0,1,2,3,4`锛?
  - 杈撳嚭 `playlist-flow-check.result=PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`銆?
- 浠诲姟娓呭崟 `1.4.2` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `1.4.3` 闇€瑕侀獙璇佽缃湪閲嶅惎鍚庡彲鎭㈠銆?
- 褰撳墠缂哄皯鍙噸澶嶇殑鍛戒护琛岄獙鏀跺叆鍙ｃ€?


### 鍒嗘瀽璁板綍

1. `loadAppSettings/saveAppSettings` 宸茶鐩栨挱鏀惧櫒鏍稿績璁剧疆鍜岀儹閿寔涔呭寲銆?
2. 闇€瑕佹柊澧炵嫭绔嬪懡浠ゅ皢鈥滃啓鍏?>閲嶈浇->姣斿鈥濊繃绋嬬粨鏋勫寲杈撳嚭銆?


### 瑙ｅ喅鏂规

- 鏂板 `--settings-persistence-check [settings_file]`銆?
- 浣跨敤娴嬭瘯閰嶇疆鎵ц round-trip锛?
  - `volume`

  - `playback_speed`

  - `resume_last_playlist`

  - `last_playlist_index`

  - `toggle_subtitle` 鐑敭

- 杈撳嚭姣忎釜瀛楁鐨?`*_ok` 缁撴灉涓庢€荤粨鏋溿€?
- 榛樿浣跨敤涓存椂璺緞锛岄伩鍏嶆薄鏌撳疄闄呴厤缃€?
- 浠诲姟娓呭崟 `1.4.3` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `2.1.2` 闇€瑕佽鐩?`mp4/mkv/mov/avi/webm/flv/ts/m2ts` 瀹瑰櫒銆?
- 褰撳墠鍥炲綊鏍锋湰缂哄け `mov/avi/m2ts`銆?


### 鍒嗘瀽璁板綍

1. `FormatSupport` 宸插０鏄庡鍣ㄦ敮鎸侊紝浣嗙己灏戝搴斿洖褰掓牱鏈棤娉曞舰鎴愰獙鏀堕棴鐜€?
2. 鏍锋湰鐢熸垚鑴氭湰浠呯敓鎴?5 绫诲鍣紝闇€鍚屾鎵╁睍銆?


### 瑙ｅ喅鏂规

- 鎵╁睍 `format_samples.csv` 鏂板 `mov/avi/m2ts` 涓夋潯鏍锋湰銆?
- 鎵╁睍 `download_test_samples.ps1` 鐢熸垚涓夌被鏂版牱鏈€?
- 鏇存柊鏍锋湰鐩綍瑙勫垯涓庢枃妗ｃ€?
- 杩愯 `run_format_regression.ps1` 浜у嚭鏈€鏂版湰鍦版姤鍛娿€?
- 浠诲姟娓呭崟 `2.1.2` 鏍囪瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `FORMAT_REGRESSION_LOCAL_CHECK.md`锛?
  - Total=12

  - PASS=12

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `2.1.3` 闇€瑕佽鐩?`H.264/H.265/VP9/AV1/MPEG-2` 瑙嗛缂栫爜銆?
- 鍥炲綊鏍锋湰灏氱己 `MPEG-2`锛屾棤娉曞畬鎴愬畬鏁撮獙鏀躲€?


### 鍒嗘瀽璁板綍

1. 鐜版湁鐭╅樀鍖呭惈 h264/hevc/vp9/av1銆?
2. `FormatSupport` 宸插寘鍚?`mpeg2video`锛岀己鍙ｅ湪鏍锋湰鍜屽洖褰掗摼璺€?


### 瑙ｅ喅鏂规

- 鏂板 `mpeg2video` 鍥炲綊鏍锋湰鏉＄洰锛坱s 瀹瑰櫒锛宎c3 闊抽锛夈€?
- 鎵╁睍 `download_test_samples.ps1` 鑷姩鐢熸垚 MPEG-2 鏍锋湰銆?
- 杩愯鍥炲綊骞舵洿鏂版湰鍦版姤鍛娿€?
- 浠诲姟娓呭崟 `2.1.3` 鏍囪瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `FORMAT_REGRESSION_LOCAL_CHECK.md`锛?
  - Total=13

  - PASS=13

  - PARTIAL=0

  - FAIL=0

  - SKIP=0

- 鏂板鏍锋湰缁撴灉锛?
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts` -> `PASS (mpeg2video)`



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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `2.1.4` 闇€瑕佽鐩?`AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`銆?
- 鏍锋湰鐭╅樀缂哄皯 `E-AC3/DTS/Vorbis/PCM`銆?


### 鍒嗘瀽璁板綍

1. 鐜版湁鏍锋湰宸茶鐩?aac/mp3/ac3/flac/opus銆?
2. DTS 缂栫爜鍣?`dca` 鍦ㄥ綋鍓?FFmpeg 涓睘浜庡疄楠岀壒鎬э紝闇€瑕?`-strict -2`銆?
3. 鍏煎鎬ф瘮瀵归渶澶勭悊 `dts/dca`銆乣hevc/h265`銆乣pcm/pcm_*` 绛変环鍏崇郴銆?


### 瑙ｅ喅鏂规

- 鎵╁睍鍥炲綊鏍锋湰涓庣敓鎴愯剼鏈紝鏂板鍥涚被闊抽鏍锋湰銆?
- 鍦?`download_test_samples.ps1` 涓?DTS 鍛戒护澧炲姞 `-strict -2`銆?
- 鍦?`run_format_regression.ps1` 澧炲姞绛変环缂栫爜鍚嶆瘮杈冨嚱鏁般€?
- 鏇存柊鏈湴鍥炲綊鎶ュ憡骞剁‘璁ゅ叏閲忛€氳繃銆?
- 浠诲姟娓呭崟 `2.1.4` 鏍囪瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `FORMAT_REGRESSION_LOCAL_CHECK.md`锛?
  - Total=17

  - PASS=17

  - PARTIAL=0

  - FAIL=0

  - SKIP=0



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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `3.1.1` 瑕佹眰 `DecoderFactory` 鎺ュ叆鐪熷疄瑙ｇ爜鍒濆鍖栨祦绋嬨€?
- 鐜扮姸鏄?`PlayerCore` 涓昏璧板唴宓屽垎鏀€昏緫锛宍DecoderFactory` 鍙湪纭В閰嶇疆灞€閮ㄨ鍔ㄥ弬涓庯紝鏈舰鎴愮粺涓€鍊欓€夐摼璺€?


### 鍒嗘瀽璁板綍

1. `DecoderFactory` 宸叉湁鑳藉姏鎺㈡祴涓庝紭鍏堢骇锛屼絾缂哄皯鈥滃悗绔€欓€夊簭鍒椻€濇帴鍙ｃ€?
2. `PlayerCore::initDecoders` 闇€瑕佹寜鍊欓€夊簭鍒楅€愪釜灏濊瘯锛屼互瀹炵幇缁熶竴鍒濆鍖栦笌鍥為€€銆?
3. `3.1.3` 宸叉彁渚涒€滄槸鍚﹀亸濂界‖瑙ｂ€濋厤缃紝闇€瑕佷繚鐣欏苟澶嶇敤銆?


### 瑙ｅ喅鏂规

- `DecoderFactory` 鏂板 `selectBackendOrder(codec_name, prefer_hardware)`锛?
  - 鐢熸垚鎸変紭鍏堢骇鎺掑簭鐨勮В鐮佸悗绔€欓€夊簭鍒楋紱

  - 淇濊瘉杞欢瑙ｇ爜鍏滃簳鍦ㄥ€欓€夐摼璺腑銆?
- `PlayerCore::initDecoders` 鎺ュ叆鍊欓€夊簭鍒楋細

  - 閫愪釜鍚庣灏濊瘯鍒濆鍖栦笌 `avcodec_open2`锛?
  - 澶辫触鑷姩鍒囨崲涓嬩竴涓€欓€夛紱

  - 鎴愬姛鍚庣粺涓€璁板綍鏈€缁堝悗绔棩蹇椼€?
- `tryConfigureD3D11HardwareDecode` 鍘婚櫎鍐呴儴鍚庣閫夋嫨鍒ゆ柇锛屾敼涓虹函 D3D11 閰嶇疆鑱岃矗锛岄€夋嫨绛栫暐鐢?`DecoderFactory` 缁熶竴鍐冲畾銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `3.1.2` 瑕佹眰 D3D11VA 鍒濆鍖栧け璐ユ椂蹇呴』鍙潬鍥為€€鍒拌蒋瑙ｃ€?
- 鐜版湁閾捐矾鍦ㄥ儚绱犳牸寮忓崗鍟嗗洖閫€鍦烘櫙涓嬩粎鎵撳嵃鏃ュ織锛屽悗绔姸鎬佹湭鏄惧紡鍒囨崲涓鸿蒋瑙ｏ紝瀛樺湪鐘舵€佷笉涓€鑷撮闄┿€?


### 鍒嗘瀽璁板綍

1. `PlayerCore::selectVideoPixelFormat` 鍦ㄦ湭鍛戒腑 D3D11VA 鍍忕礌鏍煎紡鏃朵細杩斿洖杞欢鍍忕礌鏍煎紡銆?
2. 璇ヨ矾寰勬鍓嶆湭鍚屾鏇存柊 `video_decoder_backend_`锛屽彲鑳藉鑷村悗缁‖浠剁粦瀹氭竻鐞嗘潯浠跺垽鏂笉鍑嗙‘銆?


### 瑙ｅ喅鏂规

- 鍦?`selectVideoPixelFormat` 涓ˉ鍏呮樉寮忛檷绾э細

  - 灏?`video_hw_pixel_fmt_` 閲嶇疆涓?`AV_PIX_FMT_NONE`锛?
  - 灏?`video_decoder_backend_` 璁剧疆涓?`Software`銆?
- 鍦?`initDecoders` 鐨勫悗绔皾璇曢摼璺腑鏂板闄嶇骇鏃ュ織锛?
  - 褰?D3D11VA 鍒濆鍖栬繃绋嬩腑琚崗鍟嗕负杞欢璺緞鏃讹紝璁板綍鈥滈檷绾т负杞В鈥濇彁绀恒€?
- 鍚屾鏇存柊浠诲姟娓呭崟 `3.1.2` 涓哄畬鎴愩€?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --capabilities` 閫氳繃锛堜富鍔涚煩闃?`PASS`锛夈€?


### 淇敼鏂囦欢

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 闂 37: M3 3.2.1锛圖3D11 娓叉煋鏈€灏忓彲鐢ㄩ摼璺級



**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `3.2.1` 瑕佹眰 D3D11 娓叉煋鏈€灏忓彲鐢紙`init/upload/present`锛夈€?
- 鐜版湁 `D3D11VideoRenderer` 涓烘々瀹炵幇锛宍init` 鍥哄畾澶辫触锛屾棤娉曞舰鎴愬彲鐢ㄦ覆鏌撳悗绔€?


### 鍒嗘瀽璁板綍

1. 鐜版湁 `Display` 宸插叿澶囩ǔ瀹氱殑 owner-thread 娓叉煋涓庡抚涓婁紶閾捐矾锛屽彲澶嶇敤浜?D3D11 鏈€灏忛棴鐜€?
2. 闇€瑕佸彲瑙傛祴褰撳墠 SDL renderer 鍚庣锛屼互鍒ゆ柇鏄惁鐪熸鍚敤浜?`direct3d11`銆?
3. 鑻ュ疄闄呭悗绔笉鏄?D3D11锛屽簲鍦?`D3D11VideoRenderer::init` 澶辫触骞朵氦缁欎笂灞傝Е鍙?`3.2.2` 鑷姩鍥為€€銆?


### 瑙ｅ喅鏂规

- `Display` 鏂板娓叉煋椹卞姩鎺у埗涓庤娴嬫帴鍙ｏ細

  - `setPreferredRendererDriver()`锛氬厑璁稿湪鍒涘缓 renderer 鍓嶈缃亸濂介┍鍔紱

  - `currentRendererDriver()` / `isUsingRendererDriver()`锛氳繑鍥炲綋鍓嶅疄闄?renderer 鍚庣銆?
- `D3D11VideoRenderer` 鏀逛负鏈€灏忓彲鐢ㄥ疄鐜帮細

  - `init`锛氬垱寤?`Display`锛屾寚瀹?`direct3d11` 鍋忓ソ锛屽畬鎴愮獥鍙?娓叉煋鍒濆鍖栵紱

  - `renderFrame/present/clear`锛氭帴閫氬抚涓婁紶涓庡憟鐜帮紱

  - 浜嬩欢涓庢帶鍒惰姹傛帴鍙ｇ粺涓€閫忎紶 `Display`銆?
- `init` 闃舵澧炲姞鍚庣鏍￠獙锛?
  - 鑻ュ疄闄?renderer 鍚庣闈?`direct3d11/d3d11`锛屼富鍔ㄨ繑鍥炲け璐ュ苟璁板綍鏃ュ織锛岃Е鍙戜笂灞?SDL 鍥為€€銆?
- 浠诲姟娓呭崟 `3.2.1` 鏍囪瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `3.3.2` 瑕佹眰娓叉煋澶辫触鏃跺彲鑷姩闄嶇骇涓斾笉涓柇鎾斁銆?
- 闇€瑕佷竴涓彲閲嶅鎵ц鐨勬湰鍦伴獙鏀跺叆鍙ｆ潵绋冲畾楠岃瘉 D3D11 澶辫触鍚庣殑 SDL 鍥為€€閾捐矾銆?


### 鍒嗘瀽璁板綍

1. `3.2.1/3.2.2` 宸插叿澶?D3D11 鍒濆鍖栦笌 SDL 鍥為€€鏈哄埗锛屼絾缂哄皯鑷姩鍖栭獙鏀跺懡浠ゃ€?
2. 闇€瑕佸湪涓嶄緷璧栦汉宸ユ搷浣滅殑鍓嶆彁涓嬶紝寮哄埗鍒堕€?D3D11 renderer 鍒濆鍖栧け璐ュ満鏅€?


### 瑙ｅ喅鏂规

- 鏂板娓叉煋/瑙ｇ爜鍚庣鍙娴嬫帴鍙ｏ紙renderer backend / decoder backend锛夈€?
- 鏂板鍛戒护 `--renderer-fallback-check <media_file>`锛?
  - 涓存椂娉ㄥ叆 `MVP_D3D11_DRIVER_HINT=software`锛屽己鍒?D3D11 娓叉煋鍒濆鍖栧け璐ワ紱

  - 閫氳繃鎾斁鍣ㄤ富閾捐矾楠岃瘉鏄惁鑷姩鍥為€€鍒?`SoftwareSDL`锛?
  - 杈撳嚭缁撴瀯鍖栧瓧娈典笌 `PASS/FAIL`銆?
- 鏂板鏈湴鎶ュ憡锛歚docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`銆?
- 鏇存柊浠诲姟娓呭崟锛屾爣璁?`3.3.2` 瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`

  - `open_ok=true`

  - `renderer_backend=SoftwareSDL`

  - `entered_playback_loop=true`

  - `fallback_to_sdl=true`

  - `result=PASS`



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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `3.3.1` 瑕佹眰鍦?Windows 涓嬮獙璇佺‖瑙ｏ紙D3D11VA锛夊拰杞В锛圫oftware锛変富鍔涙牱鏈潎鍙挱銆?
- 鍘熻仛鍚堝懡浠ゅ湪鍚岃繘绋嬮『搴忔墽琛屽弻浼氳瘽鏃讹紝鍑虹幇鍋滄闃舵鍗′綇锛屽鑷村懡浠よ秴鏃躲€?


### 鏃ュ織杈撳嚭

```text

windows-backend-check.command=... 

... The filename, directory name, or volume label syntax is incorrect.

windows-backend-check.result=FAIL

```



### 鍒嗘瀽璁板綍

1. 鍚岃繘绋嬭繛缁窇 hard/soft 浼氳瘽鏃讹紝绗簩杞瓨鍦ㄥ洖鏀堕摼璺笉绋冲畾銆?
2. `std::system` + 閲嶅畾鍚戝湪褰撳墠璺緞缁勫悎涓嬪瓨鍦?shell 瑙ｆ瀽涓嶇ǔ瀹氥€?
3. 闇€瑕佸皢浼氳瘽闅旂骞舵敼涓烘洿绋冲畾鐨勫瓙杩涚▼鍒涘缓鏂瑰紡銆?


### 瑙ｅ喅鏂规

- 鏂板 `--windows-backend-session-check <media_file> <hard|soft>`锛屾瘡娆″彧楠岃瘉涓€涓細璇濆苟杈撳嚭缁撴瀯鍖栧瓧娈点€?
- 灏?`--windows-backend-check` 鏀逛负鐖惰繘绋嬫媺璧蜂袱涓瓙杩涚▼锛坔ard銆乻oft锛夊苟姹囨€荤粨鏋溿€?
- Windows 涓嬬敤 `CreateProcess` + 鏂囦欢鍙ユ焺閲嶅畾鍚戦噰闆嗗瓙杩涚▼杈撳嚭锛岄伩鍏?shell 璇硶闂銆?
- 澧炲姞鏈湴鎶ュ憡 `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 hard` 閫氳繃锛圥ASS锛夈€?
- `build/Debug/modern-video-player.exe --windows-backend-session-check juren-30s.mp4 soft` 閫氳繃锛圥ASS锛夈€?
- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4` 閫氳繃锛圥ASS锛夈€?
- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4` 閫氳繃锛圥ASS锛夈€?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛圥ASS锛夈€?


### 浠诲姟娓呭崟鍚屾

- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `3.3.1` 鏍囪瀹屾垚銆?
  - `3.3.3` 鏍囪瀹屾垚銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `4.1` 瑕佹眰鎻愪緵绔犺妭瀵艰埅鑳藉姏锛堜笂涓€绔?涓嬩竴绔狅級銆?
- 褰撳墠鎾斁涓婚摼璺病鏈夌珷鑺傝姹傚姩浣滃拰璺崇珷鍏ュ彛锛屾棤娉曚粠閿洏鐩存帴璺崇珷銆?


### 鍒嗘瀽璁板綍

1. 绔犺妭鍏冩暟鎹潵鑷?`AVFormatContext::chapters`锛岄渶瑕佸湪 `Demuxer` 寮€妗ｉ樁娈垫彁鍙栧苟淇濆瓨鍦ㄥ獟浣撲俊鎭腑銆?
2. 杈撳叆浜嬩欢閾捐矾闇€鏂板绔犺妭鍔ㄤ綔閫忎紶锛歚Display -> Renderer -> PlayerCore -> VideoPlayer`銆?
3. 闇€瑕佹柊澧炲懡浠よ鑷鍏ュ彛锛岄伩鍏嶇珷鑺傚鑸粎渚濊禆鎵嬪伐鎾斁楠岃瘉銆?


### 瑙ｅ喅鏂规

- `Demuxer` 鏂板绔犺妭妯″瀷锛?
  - 澧炲姞 `ChapterInfo` 缁撴瀯涓?`MediaInfo::chapters`锛?
  - 瑙ｆ瀽 `AVChapter` 鐨勮捣姝㈡椂闂村拰鏍囬锛屽苟鎸夎捣濮嬫椂闂存帓搴忋€?
- 蹇嵎閿笌璇锋眰閾捐矾鎵╁睍锛?
  - `HotkeyManager` 鏂板 `PreviousChapter` / `NextChapter`锛?
  - 榛樿閿綅缁戝畾 `HOME` / `END`锛?
  - `Display`銆乣IVideoRenderer` 鍙婂悇娓叉煋鍣ㄥ疄鐜版柊澧炵珷鑺傝姹傛秷璐规帴鍙ｃ€?
- `PlayerCore` 澧炲姞绔犺妭璺宠浆閫昏緫锛?
  - `rebuildChapterPoints()` 鍦?`open()` 鍚庢瀯寤虹珷鑺傛椂闂寸偣锛?
  - 鏂板 `seekToNextChapter()` / `seekToPreviousChapter()`锛?
  - `pumpEvents()` 娑堣垂绔犺妭璇锋眰骞惰Е鍙?seek銆?
- `VideoPlayer` 澧炲姞绔犺妭 API 鍖呰锛?
  - `seekToNextChapter()` / `seekToPreviousChapter()` / `chapterCount()`銆?
- `main` 鏂板 `--chapter-nav-check <media_file>`锛?
  - 鑷姩鎵ц鎾斁銆佷笅涓€绔犮€佷笂涓€绔犳祦绋嬶紱

  - 杈撳嚭 `chapter-nav-check.*` 瀛楁涓?`PASS/FAIL`銆?
- 浠诲姟娓呭崟鍚屾锛?
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 鏍囪 `4.1` 瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `4.2` 瑕佹眰鏀寔 A-B Repeat銆?
- 褰撳墠涓绘祦绋嬬己灏?A/B/C 鍖洪棿寰幆鎺у埗锛屾棤娉曡缃?A 鐐广€丅 鐐瑰苟閲嶅鎾斁鍖洪棿銆?


### 鍒嗘瀽璁板綍

1. 鐑敭涓庝簨浠惰姹傞摼璺凡鍏峰鍙墿灞曟ā寮忥紝鍙鐢ㄤ负 A-B Repeat 璇锋眰閫忎紶銆?
2. `PlayerCore` 宸叉湁 seek 涓庢挱鏀句綅缃姸鎬侊紝閫傚悎鏂板鍖洪棿鐘舵€佸苟鍦ㄦ挱鏀句腑瑙﹀彂鍥炶烦銆?
3. 闇€瑕佹柊澧炲懡浠よ鑷鍏ュ彛锛岀‘淇濆尯闂村惊鐜彲绋冲畾鍥炲綊銆?


### 瑙ｅ喅鏂规

- 鏂板鐑敭鍔ㄤ綔涓庨粯璁ょ粦瀹氾細

  - `SetABRepeatStart`锛坄A`锛夛紱

  - `SetABRepeatEnd`锛坄B`锛夛紱

  - `ClearABRepeat`锛坄C`锛夈€?
- 鎵╁睍閾捐矾锛?
  - `Display` 澧炲姞 A-B Repeat 璇锋眰鏍囪/娑堣垂锛?
  - `Renderer` 鎶借薄涓?SDL/D3D11/OpenGL 瀹炵幇鏂板瀵瑰簲閫忎紶鎺ュ彛銆?
- `PlayerCore` 澧炲姞 A-B Repeat 鎺у埗鑳藉姏锛?
  - `setABRepeatStart()`銆乣setABRepeatEnd()`銆乣clearABRepeat()`锛?
  - `isABRepeatEnabled()`銆乣abRepeatStart()`銆乣abRepeatEnd()`锛?
  - `handleABRepeatLoop()` 鍦ㄦ挱鏀句腑妫€娴嬪埌杈?B 鐐瑰悗鑷姩 seek 鍥?A 鐐广€?
- `VideoPlayer` 澧炲姞 A-B Repeat API 鍖呰銆?
- `main` 鏂板 `--ab-repeat-check <media_file>` 楠屾敹鍛戒护涓庡府鍔╀俊鎭€?
- 淇鍥炲綊妫€鏌ュ啿绐侊細

  - `--settings-persistence-check` 娴嬭瘯閿綅鐢?`b` 鏀逛负 `x`锛岄伩鍏嶄笌鏂伴粯璁?`B` 鐑敭鍐茬獊銆?
- 浠诲姟娓呭崟鍚屾锛?
  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 鏍囪 `4.2` 瀹屾垚銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `4.3` 闇€瑕佹敮鎸佹埅鍥俱€?
- 褰撳墠鎴浘閾捐矾浠嶅浜?WIP锛氱儹閿€佷富娴佺▼涓?`--screenshot-check` 宸叉帴鍏ワ紝浣嗘殏鍋滄€佽姹傜己灏戔€滄渶鍚庢樉绀哄抚鈥濈紦瀛橈紝瀵艰嚧鏆傚仠鏃舵棤娉曠ǔ瀹氫繚瀛樺綋鍓嶇敾闈€?


### 鍒嗘瀽璁板綍

1. `Display -> Renderer -> PlayerCore -> VideoPlayer -> main` 鐨勮姹傞摼璺凡缁忓叿澶囷紝鍙樊鎾斁鍣ㄦ牳蹇冨鈥滄渶杩戜竴甯р€濈殑淇濈暀鑳藉姏銆?
2. 鐜版湁瀹炵幇鍙湪娓叉煋绾跨▼澶勭悊鎴浘璇锋眰锛涗竴鏃﹁繘鍏ユ殏鍋滄€侊紝璋冨害鍣ㄥ仠姝㈢户缁€佸抚锛屾埅鍥捐姹傚氨鎷夸笉鍒版柊鐨勫浘鍍忔暟鎹€?
3. 瑕佽鎴浘鎴愪负鍙敤鍔熻兘锛岄櫎浜嗚ˉ榻愮紦瀛樺锛岃繕闇€瑕佹妸鑷鏀规垚瑕嗙洊鈥滄殏鍋滄€佹埅鍥锯€濓紝鍚﹀垯鏃犳硶璇佹槑鏍瑰洜宸蹭慨澶嶃€?


### 瑙ｅ喅鏂规

- `PlayerCore` 鏂板鏈€杩戜竴娆℃覆鏌撳抚缂撳瓨锛?
  - `updateLastRenderedFrame()` 鍦ㄦ瘡娆″憟鐜板悗缂撳瓨褰撳墠甯э紱

  - `captureScreenshotFromCachedFrame()` 鏀寔鍦ㄦ殏鍋滄€佺洿鎺ユ埅鍥撅紱

  - `clearLastRenderedFrame()` 鍦ㄩ噸鏂版墦寮€/鍏抽棴濯掍綋鏃舵竻鐞嗙紦瀛樸€?
- `requestScreenshot()` 璋冩暣涓猴細

  - 鎾斁涓細缁存寔寮傛璇锋眰锛?
  - 闈炴挱鏀炬€侊紙閲嶇偣鏄殏鍋滄€侊級锛氱洿鎺ヤ粠缂撳瓨甯ц惤鐩樸€?
- `--screenshot-check` 璋冩暣涓衡€滃厛鎾斁 -> 鏆傚仠 -> 璇锋眰鎴浘 -> 鏍￠獙杈撳嚭鏂囦欢鈥濓紝纭繚鏈淇鏈夐拡瀵规€ч獙璇併€?
- 鏂囨。涓庝换鍔℃竻鍗曞悓姝ワ細

  - `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md` 鏍囪 `4.3` 瀹屾垚锛?
  - 鏂板鏈湴鎶ュ憡 `docs/reports/SCREENSHOT_LOCAL_CHECK.md`锛?
  - 鏇存柊 `README.md` / `README_ZH.md` 鐨勫揩鎹烽敭璇存槑銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug --target modern-video-player` 閫氳繃銆?
- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --settings-persistence-check` 閫氳繃锛坄PASS`锛夈€?
- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4` 閫氳繃锛坄PASS`锛夈€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 浠嶅仠鐣欏湪 2026-03-07 涔嬪墠鐨勮瘎浼板彛寰勩€?
- 鏂囨。鎶婂瓧骞曘€佸揩鎹烽敭銆佽缃€佹挱鏀惧垪琛ㄣ€佽В鐮佸櫒绠＄悊绛夎兘鍔涘啓鎴愨€滄湭鎺ュ叆/楠ㄦ灦鈥濓紝涓庡綋鍓嶄唬鐮佸拰鏈湴楠屾敹鎶ュ憡涓嶄竴鑷淬€?


### 鍒嗘瀽璁板綍

1. `M1`銆乣M3` 涓?`M4 4.1/4.2/4.3` 宸查檰缁惤鍦帮紝浣嗗樊璺濊瘎浼版枃妗ｆ病鏈夊悓姝ュ埛鏂般€?
2. 缁х画娌跨敤鏃х粨璁轰細璇鍚庣画浼樺厛绾у垽鏂紝鐗瑰埆鏄細鎶婂凡瀹屾垚椤圭户缁垪鍏?`P0/P1`銆?
3. 宸窛璇勪及鏂囨。闇€瑕佸悓鏃跺弬鑰冧唬鐮佸叆鍙ｅ拰 `docs/reports/*` 楠屾敹鎶ュ憡锛岃€屼笉鏄彧鐪嬬被/鎺ュ彛鏄惁瀛樺湪銆?


### 瑙ｅ喅鏂规

- 灏?`docs/analysis/MPC_HC_GAP_ANALYSIS.md` 鍏ㄦ枃鏇存柊鍒?2026-03-08 鐗堟湰銆?
- 閲嶅啓妯″潡鎬昏銆佹ā鍧楃粺璁°€佸墿浣欏樊璺濇竻鍗曚笌寤鸿閲岀▼纰戙€?
- 鎶婂瓧骞曘€佹挱鏀惧垪琛ㄣ€佽缃€佸揩鎹烽敭銆佽В鐮佸櫒绠＄悊銆佹埅鍥剧瓑宸茶惤鍦拌兘鍔涗粠鈥滈鏋?鏈帴鍏モ€濈籂姝ｄ负鈥滈儴鍒嗗疄鐜扳€濄€?
- 鏂板鈥滈獙鏀朵笌鎶ュ憡璇佹嵁鈥濈珷鑺傦紝鏄庣‘璇勪及渚濇嵁鍖呭惈鏈湴鍥炲綊鎶ュ憡銆?
- 鏇存柊鏂囨。绱㈠紩锛岃ˉ涓€鏉℃湰娆″樊璺濊瘎浼板榻愯鏄庛€?


### 鏈湴鏍″缁撴灉

- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 鏍囬鏃ユ湡宸叉洿鏂颁负 `2026-03-08`銆?
- 妯″潡缁熻鏇存柊涓猴細`閮ㄥ垎瀹炵幇 11 / 14`銆乣楠ㄦ灦/鏈帴鍏?3 / 14`銆?
- `P0/P1/P2` 鍓╀綑椤瑰凡涓庡綋鍓嶄换鍔℃竻鍗曞拰楠屾敹鎶ュ憡淇濇寔涓€鑷淬€?


### 淇敼鏂囦欢

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



---



## 闂 44: `docs/records/VERSION.md` 鍘嗗彶璺緞鎻忚堪杩囨湡



**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `docs/records/VERSION.md` 鐨勬棭鏈熼樁娈垫弿杩颁粛淇濈暀鏃х増 decoder/thread/test 鏂囦欢绾ц〃杩帮紝鐪嬭捣鏉ュ儚褰撳墠浠撳簱浠嶅湪浣跨敤杩欎簺璺緞銆?
- 杩欎細褰卞搷鎸夋枃妗ｉ亶鍘嗕粨搴撴椂鐨勮繘搴﹀垽鏂紝涔熷鏄撴妸鍘嗗彶瀹炵幇鍜屽綋鍓嶄富閾剧粨鏋勬贩涓轰竴璋堛€?


### 鍒嗘瀽璁板綍

1. `2026-03-06` 鏋舵瀯鏀舵暃鍚庯紝鎾斁鍣ㄤ富閾惧凡缁忓垏鍒?`PlayerCore + Scheduler + core/*`锛屼絾鐗堟湰鏂囨。鐨勫巻鍙茬珷鑺傛病鏈夊悓姝ュ幓姝т箟銆?
2. 鈥滈樁娈典竴锛氬熀纭€鎾斁鍣?(褰撳墠闃舵)鈥?涓庘€滀笅涓€姝ヨ鍒掆€濈瓑鎺緸宸茬粡涓嶅啀绗﹀悎 `2026-03-08` 鐨勫疄闄呯姸鎬併€?
3. 鍘嗗彶绔犺妭搴旇淇濈暀鑳藉姏婕旇繘鍜岄棶棰樹慨澶嶈褰曪紝浣嗕笉搴斿啀鎶婂凡鍒犻櫎鏂囦欢褰撲綔褰撳墠浠撳簱缁撴瀯璇存槑銆?


### 瑙ｅ喅鏂规

- 灏嗛樁娈典竴鏀瑰啓涓衡€滃巻鍙茶捣鐐光€濓紝骞惰ˉ鍏呮棫鐗?`decoder / playLoop` 鏋舵瀯宸插苟鍏ュ綋鍓嶄富閾剧殑璇存槑銆?
- 灏?`video_decoder` / `audio_decoder`銆乣VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 绛夋棫璺緞鏀瑰啓涓鸿兘鍔涚骇鍘嗗彶琛ㄨ堪銆?
- 灏嗏€滀笅涓€姝ヨ鍒掆€濄€乣USE_NEW_PLAYER_CORE`銆佷复鏃?`tests/core_*` 鐨勭浉鍏虫弿杩拌皟鏁翠负鍘嗗彶璁板綍鍙ｅ緞銆?
- 鍦?`docs/records/VERSION.md` 鐨勬枃妗ｆ洿鏂版棩蹇椾腑琛ヨ鏈娓呯悊銆?


### 鏈湴鏍″缁撴灉

- `docs/records/VERSION.md` 涓嶅啀鍑虹幇鈥滃綋鍓嶉樁娈碘€濃€滀笅涓€姝ヨ鍒掆€濈瓑鏄撹瀵煎綋鍓嶇姸鎬佺殑鏃у彛寰勩€?
- 鏃╂湡绔犺妭瀵规棫鐗?decoder/thread 鐨勮〃杩板凡鏀逛负鍘嗗彶璇存槑锛屽苟鎸囧悜褰撳墠 `core/*` 涓婚摼銆?
- 鏈鍙樻洿闄愬畾鍦ㄦ枃妗ｅ眰锛屾湭鎵╂暎鍒颁唬鐮佹垨鏋勫缓閰嶇疆銆?


### 淇敼鏂囦欢

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 闂 45: README 涓庢灦鏋勬枃妗ｄ粛娣风敤鏃т富閾捐〃杩?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鏍圭洰褰?`README.md` / `README_ZH.md` 浠嶆妸 `video_decoder`銆乣audio_decoder` 绛夋棫鏂囦欢鍐欐垚椤圭洰褰撳墠缁撴瀯銆?
- `docs/design/ARCHITECTURE.md` 鍚屾椂娣风敤浜嗏€滃綋鍓嶅疄鐜扳€濆拰鏃фā鍧楀懡鍚嶏紝璇昏€呭鏄撹鍒ょ幇琛屼富閾句粛渚濊禆杩欎簺鍘嗗彶妯″潡銆?


### 鍒嗘瀽璁板綍

1. `2026-03-06` 鏋舵瀯鏀舵暃鍚庯紝褰撳墠鎾斁鍣ㄤ富閾惧凡缁忕ǔ瀹氬湪 `VideoPlayer + PlayerCore + Scheduler + core/*`銆?
2. `README` 鏇村亸鍚戔€滃揩閫熺悊瑙ｄ粨搴撯€濈殑鍏ュ彛鏂囨。锛屽洜姝ょ洰褰曟爲鍜屾灦鏋勭ず鎰忓簲浼樺厛鍙嶆槧鐜扮姸锛岃€屼笉鏄繚鐣欏凡鍒犻櫎璺緞銆?
3. `docs/design/ARCHITECTURE.md` 浣滀负鑳屾櫙鏂囨。鍙互淇濈暀鍘嗗彶鍐呭锛屼絾蹇呴』鏄惧紡鏍囨敞鍝簺绔犺妭灞炰簬鍘嗗彶瀹炵幇銆?


### 瑙ｅ喅鏂规

- 閲嶅啓 `README.md` / `README_ZH.md` 鐨勯」鐩粨鏋勬爲鍜屾灦鏋勭ず鎰忥紝鏀逛负褰撳墠涓婚摼鍙ｅ緞銆?
- 鍦?`docs/design/ARCHITECTURE.md` 椤堕儴澧炲姞鐘舵€佽鏄庯紝骞跺皢鏃?decoder/thread/sync 绔犺妭缁熶竴鏍囨垚鈥滃巻鍙插疄鐜扳€濄€?
- 灏嗘枃妗ｄ腑鐨勬棩蹇楃ず渚嬫敼涓哄綋鍓?`Quill` 瀹忔帴鍙ｃ€?
- 鏇存柊 `docs/README.md` 鐨勬灦鏋勬枃妗ｇ储寮曡鏄庯紝鍖哄垎鍘嗗彶鍩虹嚎鍜屽綋鍓嶉噸鏋勮鏄庛€?


### 鏈湴鏍″缁撴灉

- `README.md` / `README_ZH.md` 宸蹭笉鍐嶆妸 `video_decoder` / `audio_decoder` 鍐欎负褰撳墠鐩綍缁撴瀯銆?
- `docs/design/ARCHITECTURE.md` 宸叉槑纭０鏄庡巻鍙茬珷鑺傝竟鐣岋紝骞朵笉鍐嶆妸鏃у绾跨▼閾捐矾鏍囦负鈥滃綋鍓嶅疄鐜扳€濄€?
- 鏈鏀瑰姩浠嶉檺瀹氬湪鏂囨。灞傦紝鏈秹鍙婁唬鐮佸拰鏋勫缓閰嶇疆銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `docs/guides/IMPLEMENTATION.md` 淇濈暀鐨勬槸鏃╂湡鍘熷瀷鐨勪粠闆跺疄鐜版暀绋嬶紝浣嗘病鏈夋槑纭０鏄庡叾涓?`video_decoder`銆乣audio_decoder`銆佸崟浣?`playLoop` 绛夊唴瀹瑰睘浜庡巻鍙插疄鐜般€?
- `docs/plans/MPC_HC_ITERATION_PLAN.md` 璁板綍鐨勬槸 `2026-03-07` 鐨勮鍒掑熀绾匡紝浣嗘病鏈夋彁绀哄叾涓儴鍒嗚兘鍔涘凡鍦ㄥ悗缁彁浜や腑钀藉湴銆?


### 鍒嗘瀽璁板綍

1. 杩欎袱浠芥枃妗ｉ兘杩樻湁浣跨敤浠峰€硷細鍓嶈€呭亸鏁欑▼锛屽悗鑰呭亸瑙勫垝锛涢棶棰樺湪浜庣己灏戝拰鈥滃綋鍓嶄唬鐮佺姸鎬佲€濈殑鏄惧紡鍒嗙晫銆?
2. 缁忚繃鍓嶅嚑杞竻鐞嗗悗锛宍README`銆乣VERSION`銆乣ARCHITECTURE` 宸茬粡鍖哄垎鍘嗗彶涓庡綋鍓嶏紝濡傛灉杩欎袱浠芥枃妗ｄ笉璺熻繘锛岃鑰呬粛浼氬湪鍏ュ彛灞備骇鐢熸贩娣嗐€?
3. 鏈€鍚堥€傜殑鍋氭硶涓嶆槸閲嶅啓鍏ㄦ枃锛岃€屾槸鍦ㄦ枃妗ｉ《閮ㄨˉ鍏呯姸鎬佽鏄庯紝骞舵妸绱㈠紩鎻忚堪缁熶竴鍒板悓涓€鍙ｅ緞銆?


### 瑙ｅ喅鏂规

- 涓?`docs/guides/IMPLEMENTATION.md` 澧炲姞鈥滅姸鎬佽鏄庯紙2026-03-08锛夆€濓紝鏄庣‘鍏朵负鏃╂湡鍘熷瀷鏁欑▼锛屽苟鎸囧悜褰撳墠涓婚摼鍙傝€冩枃妗ｃ€?
- 涓?`docs/plans/MPC_HC_ITERATION_PLAN.md` 澧炲姞鈥滅姸鎬佽鏄庯紙2026-03-08锛夆€濓紝鏄庣‘鍏朵负 `2026-03-07` 鐨勮鍒掑揩鐓э紝骞跺垪鍑哄綋鍓嶈繘搴﹀弬鑰冨叆鍙ｃ€?
- 鏇存柊 `docs/README.md`銆乣README.md`銆乣README_ZH.md` 鐨勬枃妗ｈ鏄庯紝浣垮巻鍙叉暀绋嬨€佽鍒掑揩鐓с€佸綋鍓嶄富閾捐鏄庝笁鑰呰竟鐣屼竴鑷淬€?


### 鏈湴鏍″缁撴灉

- `docs/guides/IMPLEMENTATION.md` 宸蹭笉鍐嶈琛ㄨ堪涓哄綋鍓嶄粨搴撶殑閫愭枃浠跺疄鏂芥竻鍗曘€?
- `docs/plans/MPC_HC_ITERATION_PLAN.md` 宸叉槑纭负璁″垝蹇収锛屽苟鎸囧悜褰撳墠杩涘害鏉ユ簮銆?
- 鏈鏀瑰姩浠嶉檺瀹氬湪鏂囨。灞傦紝鏈秹鍙婁唬鐮併€佹瀯寤哄拰浠诲姟娓呭崟鐘舵€佷慨鏀广€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- `docs/design/FILTERS.md`銆乣docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`銆乣docs/guides/WINDOWS_SETUP.md` 浠嶅亸鍚戔€滈潤鎬佽鏄庘€濓紝缂哄皯涓庡綋鍓嶄唬鐮佷富閾俱€佷緷璧栨帰娴嬫柟寮忓拰鏂囨。閫傜敤鑼冨洿鐨勮鎺ヨ鏄庛€?
- `docs/guides/WINDOWS_SETUP.md` 杩樹繚鐣欎簡鏃х殑 `SDL2_DIR` / `FFMPEG_DIR` 閰嶇疆绀轰緥锛屼笌褰撳墠 `CMakeLists.txt` 鐨勬墜鍔ㄤ緷璧栧洖閫€璺緞涓嶅畬鍏ㄤ竴鑷淬€?


### 鍒嗘瀽璁板綍

1. 缁忚繃鍓嶅嚑杞竻鐞嗗悗锛屽叆鍙ｆ枃妗ｅ拰鏍稿績璁捐鏂囨。宸茬粡鍖哄垎浜嗏€滃巻鍙插熀绾库€濆拰鈥滃綋鍓嶄富閾锯€濓紝浣嗚緟鍔╄鏄庢枃妗ｈ繕娌℃湁瀹屽叏璺熶笂銆?
2. `FILTERS.md` 鐨勪富瑕侀棶棰樹笉鏄敊璇紝鑰屾槸缂哄皯鈥滃綋鍓嶉粯璁や富娴佺▼鍏ュ彛鏄?`FilterPipeline`鈥濊繖涓€灞傝В閲娿€?
3. `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 鏇村儚涓撻鍙傝€冪瑪璁帮紝闇€瑕佹彁閱掕鑰呬笉瑕佹妸鍏朵腑鐨勭洰鏍囬」鐩存帴绛夊悓浜庡綋鍓嶆湭瀹屾垚椤广€?
4. `WINDOWS_SETUP.md` 鍒欓渶瑕佷笌鐜版湁 `CMakeLists.txt` 鐨勪緷璧栨帰娴嬮『搴忎繚鎸佷竴鑷达紝鍚﹀垯浼氬奖鍝嶅疄闄呮惌寤轰綋楠屻€?


### 瑙ｅ喅鏂规

- 涓?`docs/design/FILTERS.md` 澧炲姞鐘舵€佽鏄庯紝骞惰ˉ榻愬綋鍓嶅唴缃煶棰戞护闀?`volume_balance` 涓庨摼璺鏄庛€?
- 涓?`docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 澧炲姞鐘舵€佽鏄庯紝鏄庣‘鍏舵槸涓撻鍙傝€冭€屼笉鏄叏閲忚繘搴﹂潰鏉裤€?
- 涓?`docs/guides/WINDOWS_SETUP.md` 澧炲姞鐘舵€佽鏄庯紝淇 Quill 璇存槑銆佹墜鍔ㄥ畨瑁呯ず渚嬪拰鍏变韩搴撲娇鐢ㄨ鏄庯紝浣夸箣涓庡綋鍓?`CMakeLists.txt` 瀵归綈銆?
- 鏇存柊 `docs/README.md` 绱㈠紩锛屽皢 `FILTERS.md` 绾冲叆鍙彂鐜板叆鍙ｏ紝骞惰拷鍔犳湰杞枃妗ｆ暣鐞嗚褰曘€?


### 鏈湴鏍″缁撴灉

- `docs/design/FILTERS.md` 宸叉槑纭綋鍓嶇敓鏁堢殑婊ら暅涓婚摼涓庨鐣欑粍浠惰竟鐣屻€?
- `docs/reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md` 宸叉槑纭负涓撻鍙傝€冩枃妗ｏ紝骞舵寚鍚戝綋鍓嶆€昏繘搴︽潵婧愩€?
- `docs/guides/WINDOWS_SETUP.md` 宸蹭笉鍐嶅缓璁娇鐢ㄤ笌褰撳墠浠撳簱涓嶄竴鑷寸殑 `SDL2_DIR` / `FFMPEG_DIR` 浼犲弬绀轰緥銆?
- 鏈鏀瑰姩浠嶉檺瀹氬湪鏂囨。灞傦紝鏈秹鍙婁唬鐮併€佹瀯寤鸿剼鏈拰浠诲姟娓呭崟鐘舵€佷慨鏀广€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鏍圭洰褰?`README.md` / `README_ZH.md` 鐨?Windows 鏁呴殰鎺掗櫎浠嶆彁绀轰娇鐢?`FFMPEG_DIR` 浼犲弬锛岃繖涓庡綋鍓?`CMakeLists.txt` 鐨勮嚜鍔ㄦ帰娴?/ `external/ffmpeg` 鍥為€€閫昏緫涓嶄竴鑷淬€?
- `docs/analysis/video-stream-index-fix.md` 璁板綍鐨勬槸鏃╂湡鍘熷瀷闃舵鐨勯棶棰橈紝浣嗙己灏戔€滃巻鍙插綊妗ｂ€濇爣璇嗭紝鏂囦腑鐨?`playLoop` 涓?`src/video_decoder.cpp` 鏄撹璇涓虹幇琛屽疄鐜般€?


### 鍒嗘瀽璁板綍

1. 鍓嶅嚑杞凡缁忔竻鐞嗕簡 `WINDOWS_SETUP.md` 鐨勬瀯寤哄叆鍙ｏ紝浣嗘牴 README 鐨勭畝鐗堟晠闅滄帓闄よ繕娈嬬暀鏃ц鏄庛€?
2. `video-stream-index-fix.md` 浣滀负闂鍒嗘瀽鏍锋湰浠嶇劧鏈変环鍊硷紝閫傚悎淇濈暀锛涢噸鐐规槸璁╄鑰呮槑纭畠灞炰簬鏃╂湡闃舵銆?
3. 杩欎袱绫婚棶棰橀兘灞炰簬鈥滃叆鍙ｆ枃妗ｅ拰鍘嗗彶褰掓。涔嬮棿杈圭晫涓嶆竻鈥濓紝閫傚悎涓€璧锋敹灏俱€?


### 瑙ｅ喅鏂规

- 灏嗘牴 README 鐨?FFmpeg 鏁呴殰鎺掗櫎鏀逛负褰撳墠 `vcpkg toolchain / external/ffmpeg` 鍙ｅ緞銆?
- 涓?`docs/analysis/video-stream-index-fix.md` 澧炲姞鐘舵€佽鏄庯紝鏍囪鍏朵负 `2026-02-24` 鏃╂湡鍘熷瀷闂鍒嗘瀽褰掓。銆?
- 鍦?`docs/README.md` 涓负璇ュ巻鍙叉枃妗ｈˉ鍏呯储寮曪紝骞惰褰曟湰杞洿鏂般€?


### 鏈湴鏍″缁撴灉

- `README.md` / `README_ZH.md` 涓嶅啀寤鸿浣跨敤涓庡綋鍓嶄富閾句笉涓€鑷寸殑 `FFMPEG_DIR` 浼犲弬鏂瑰紡銆?
- `docs/analysis/video-stream-index-fix.md` 宸叉槑纭负鍘嗗彶褰掓。锛屼笉鍐嶆殫绀?`playLoop` / `video_decoder.cpp` 灞炰簬褰撳墠浠撳簱缁撴瀯銆?
- 鏈鏀瑰姩浠嶉檺瀹氬湪鏂囨。灞傦紝鏈秹鍙婁唬鐮併€佹瀯寤鸿剼鏈拰浠诲姟娓呭崟鐘舵€佷慨鏀广€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鏈疆宸茬粡杩炵画瀹屾垚澶氭壒鏂囨。娓呯悊锛屼絾缁撴灉浠嶇劧鍒嗘暎鍦ㄥ涓彁浜ゅ拰鏃ュ織鏉＄洰閲屻€?
- 濡傛灉鍚庣画闇€瑕佺户缁淮鎶ゆ枃妗ｅ彛寰勶紝缂哄皯涓€浠解€滀竴鐪肩湅鎳傛湰杞竻鐞嗚寖鍥村拰缁撹鈥濈殑鎬昏〃銆?


### 鍒嗘瀽璁板綍

1. `CHANGELOG.md` 鍜?`DEVELOP_LOG.md` 鑳借褰曡繃绋嬶紝浣嗗鍚庣画缁存姢鑰呮潵璇翠笉澶熻仛鍚堛€?
2. `docs/README.md` 鎻愪緵浜嗙储寮曞叆鍙ｏ紝浣嗘病鏈変竴浠戒笓闂ㄨВ閲娾€滄湰杞枃妗ｅ贰妫€缁撹鈥濈殑姹囨€绘姤鍛娿€?
3. 澧炲姞鍗曠嫭鎶ュ憡鏂囨。锛屽彲浠ユ妸鈥滃綋鍓嶄富閾惧熀绾裤€佸凡鏀舵暃鏂囨。銆佷繚鐣欏巻鍙插唴瀹广€佸悗缁缓璁€濆浐瀹氫笅鏉ワ紝鍑忓皯閲嶅瑙ｉ噴鎴愭湰銆?


### 瑙ｅ喅鏂规

- 鏂板 `docs/analysis/DOC_AUDIT_2026-03-08.md`锛屼綔涓烘湰杞枃妗ｅ贰妫€鎬昏〃銆?
- 鎶ュ憡涓泦涓褰曪細宸℃鑼冨洿銆佸叧閿瘝鏂规硶銆佸綋鍓嶄富閾惧熀绾裤€佸凡瀹屾垚瀵归綈椤广€佷繚鐣欎絾鍚堢悊鐨勫巻鍙插唴瀹广€佸悗缁淮鎶ゅ缓璁€佸叧鑱旀彁浜ゃ€?
- 鍦?`docs/README.md` 涓皢璇ユ姤鍛婄撼鍏ョ储寮曪紝骞惰拷鍔犳洿鏂板巻鍙茶鏄庛€?


### 鏈湴鏍″缁撴灉

- `docs/analysis/DOC_AUDIT_2026-03-08.md` 宸插彲鐙珛璇存槑鏈疆鏂囨。娓呯悊宸ヤ綔鐨勮寖鍥翠笌缁撹銆?
- `docs/README.md` 宸插彲鐩存帴瀹氫綅鍒拌宸℃鎶ュ憡銆?
- 鏈鏀瑰姩浠嶉檺瀹氬湪鏂囨。灞傦紝鏈秹鍙婁唬鐮併€佹瀯寤鸿剼鏈拰浠诲姟娓呭崟鐘舵€佷慨鏀广€?


### 淇敼鏂囦欢

- docs/analysis/DOC_AUDIT_2026-03-08.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md



---



## 闂 50: M4 4.4锛堝抚姝ヨ繘锛?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `4.4` 瑕佹眰鏀寔鏆傚仠鎬侀€愬抚鏌ョ湅鐢婚潰銆?
- 褰撳墠浠撳簱缂哄皯鏆傚仠鎬佸抚姝ヨ繘鍏ュ彛锛屾棤娉曞儚 MPC-HC 涓€鏍峰湪鏆傚仠鍚庨€愬抚鍓嶅悗妫€鏌ョ敾闈€?


### 鍒嗘瀽璁板綍

1. 杈撳叆灞傚綋鍓嶅彧鏈夋挱鏀俱€乻eek銆侀煶閲忋€佺珷鑺傘€丄-B Repeat銆佹埅鍥剧瓑鍔ㄤ綔锛屾病鏈夊抚姝ヨ繘鍔ㄤ綔鍜岄粯璁ら敭浣嶃€?
2. `PlayerCore` 鏆傚仠鏃朵細鍐荤粨璋冨害鍣紝鍥犳涓嶈兘鐩存帴澶嶇敤甯歌娓叉煋寰幆锛岄渶瑕佷竴鏉℃殏鍋滄€佺殑瀹氬悜鍒锋柊璺緞銆?
3. 鍒濈増瀹炵幇鑱旇皟鏃跺彂鐜帮紝闊抽娑堣垂绾跨▼鍦ㄦ殏鍋滄€佷粛浼氱敤鏃?`playback_pts` 鍥炲啓 `position_`锛屼細鎶婃杩涚粨鏋滆鐩栨帀锛岄渶瑕佸悓姝ユ敹鍙ｃ€?


### 瑙ｅ喅鏂规

- 鏂板 `step_frame_backward` / `step_frame_forward` 鐑敭鍔ㄤ綔锛岄粯璁ょ粦瀹?`,` / `.`銆?
- 鍦?`Display`銆佹覆鏌撳櫒鎺ュ彛銆乣PlayerCore`銆乣VideoPlayer` 涔嬮棿鎵撻€氬抚姝ヨ繘璇锋眰涓?API銆?
- 閲囩敤鈥滄殏鍋滄€?seek + 棣栧抚鍒锋柊鈥濈殑鏂瑰紡瀹炵幇鍓嶅悗鍗曞抚姝ヨ繘锛屽苟鍦ㄦ杩涘悗缁存寔鏆傚仠鐘舵€併€?
- 璋冩暣闊抽娑堣垂绾跨▼锛氫粎鍦?`PlaybackState::Playing` 鏃舵墠鐢ㄩ煶棰戞挱鏀句綅缃洖鍐欎富鏃堕棿杞淬€?
- 鏂板 `--frame-step-check .\\juren-30s.mp4` 鑷鍛戒护锛屽苟璁板綍鏈湴楠屾敹缁撴灉銆?


### 鏈湴鏍″缁撴灉

- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`锛歚PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`锛歚PASS`

- 榛樿鐑敭鏂囨。宸茶ˉ鍏?`, / .` 鐨勬殏鍋滄€佸抚姝ヨ繘璇存槑銆?


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



## 闂 53: M2 2.2.4锛氳緭鍑烘挱鏀炬€ц兘鏃ュ織锛堟帀甯?闃熷垪/CPU/GPU锛?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `2.2.4` 闇€瑕佷竴涓粺涓€鐨勬€ц兘鏃ュ織鍏ュ彛锛岀敤浜庤瀵熼珮鍒嗚鲸鐜?楂樼爜鐜囨牱鏈殑鎺夊抚銆侀槦鍒楀拰璧勬簮鍗犵敤鎯呭喌銆?
- 褰撳墠鎾斁鍣ㄤ富閾惧凡缁忓叿澶囪緝澶氬唴閮ㄧ粺璁★紝浣嗗閮ㄦ病鏈夌ǔ瀹氱殑缁撴瀯鍖栭獙鏀惰緭鍑猴紝鏃犳硶鐩存帴娌夋穩涓烘湰鍦版姤鍛娿€?


### 鍒嗘瀽璁板綍

1. `PlayerCore` 宸茬粡缁存姢浜?demux銆乨ecode銆乺ender 绛夊缁勫師瀛愯鏁帮紝`Scheduler` 涔熸湁鎺夊抚涓庤В鐮佺粺璁★紝浣嗙己灏戠粺涓€蹇収鎺ュ彛銆?
2. 鐜版湁鍛戒护琛岃嚜妫€鏇村亸鍚戝姛鑳介獙璇侊紝涓嶉€傚悎鍋?`1080p60 / 4K / 楂樼爜鐜嘸 鏍锋湰鐨勬€ц兘瀵规瘮鐣欐。銆?
3. GPU 鐩存帴鍗犵敤鐜囬噰鏍疯法骞冲彴鎴愭湰杈冮珮锛屽洜姝ゆ湰杞厛杈撳嚭褰撳墠婵€娲荤殑瑙ｇ爜 backend / 娓叉煋 backend锛屼綔涓?GPU 璺緞瑙傛祴淇℃伅銆?


### 瑙ｅ喅鏂规

- 鏂板 `core::DiagnosticsSnapshot`锛屽湪 `PlayerCore` 涓敹鏁?demux / decode / render / scheduler / queue 鎸囨爣銆?
- 鍦?`VideoPlayer` 涓鍔?`getInfo()` / `getDiagnosticsSnapshot()` 閫忎紶鎺ュ彛銆?
- 鍦?`main` 涓柊澧?`--performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200` 鑷鍛戒护锛岃緭鍑?CPU 骞冲潎鍗犵敤銆侀€昏緫鏍稿績鏁般€乥ackend銆佹帀甯т笌闃熷垪鎸囨爣銆?
- 鏂板 `docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`锛屽苟鍚屾浠诲姟娓呭崟銆佸樊璺濊瘎浼般€佺増鏈枃妗ｄ笌鍙樻洿璁板綍銆?


### 鏈湴鏍″缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`锛歚PASS`

- 褰撳墠鏍锋湰杈撳嚭鍖呭惈 `renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆乣cpu_avg_percent鈮?00%`銆乣scheduler_late_drops=0` 绛夊叧閿寚鏍囥€?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 闇€瑕佷负 `1080p60` 绋冲畾鎾斁寤虹珛鏄庣‘鐨勬湰鍦伴獙鏀跺叆鍙ｏ紝閬垮厤鍙嚟鎬ц兘鏃ュ織鎴栦汉宸ヨ瀵熷垽鏂€?
- 褰撳墠鏍锋湰鍖呮病鏈夋樉寮忕殑 `1080p60` 绋冲畾鎬ф牱鏈敓鎴愯鏄庯紝澶嶇幇瀹為獙璺緞涓嶅娓呮櫚銆?


### 鍒嗘瀽璁板綍

1. `collectFileProbeReport()` 宸茬粡鑳界粰鍑哄楂樸€丗PS銆佹帹鑽?backend 绛夊厓淇℃伅锛岄€傚悎鍋氱ǔ瀹氭€ч獙鏀跺墠缃潯浠躲€?
2. `DiagnosticsSnapshot` 宸插彲鎻愪緵 `scheduler_late_drops` / `demux_dropped_packets` 绛夊叧閿ǔ瀹氭€ф寚鏍囥€?
3. 杩橀渶瑕佽ˉ涓€涓€滆繛缁挱鏀剧獥鍙ｅ唴鏃堕棿鎺ㄨ繘鈥濈殑鍒ゆ柇锛屾墠鑳芥妸 `1080p60` 楠屾敹浠庤娴嬫棩蹇楁彁鍗囦负鏄庣‘闂ㄧ銆?


### 瑙ｅ喅鏂规

- 鏂板 `--1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`锛屾鏌?`probe`銆佹椂闂存帹杩涖€乴ate drop 涓?demux drop銆?
- 鍦?`tools/download_test_samples.ps1` 澧炲姞 `1080p60 AAC 2ch` 鏍锋湰鐢熸垚璺緞锛屽苟鍦?`samples/README.md` 鏍囪鍏剁敤浜?`2.2.1 / 2.3.2`銆?
- 鏂板 `docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`锛屽悓姝ヤ换鍔℃竻鍗曘€佸樊璺濊瘎浼颁笌鐗堟湰璁板綍銆?


### 鏈湴鏍″缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`锛歚PASS`

- 褰撳墠鏍锋湰杈撳嚭鍖呭惈 `advance_ratio=0.994133`銆乣late_drops=0`銆乣demux_dropped_packets=0`銆乣decoder_backend=D3D11VA`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 闇€瑕佹妸 `4K` 鏍锋湰鎾斁涓庘€滃け璐ユ椂鍙檷绾р€濇敹鏁涙垚涓€涓粺涓€鐨勬湰鍦伴獙鏀跺叆鍙ｃ€?
- 鐜版湁鑳藉姏鍒嗘暎鍦ㄦ€ц兘鏃ュ織涓?Windows 鍚庣鍥為€€妫€鏌ヤ腑锛屼笉鍒╀簬鐩存帴瀵瑰簲浠诲姟娓呭崟 `2.2.2 / 2.3.3`銆?


### 鍒嗘瀽璁板綍

1. `collectFileProbeReport()` 宸茶兘纭鏍锋湰鏄惁涓?`3840x2160`锛岄€傚悎鍋?4K 闂ㄧ鍓嶇疆鍒ゆ柇銆?
2. `runBackendSessionSubprocess()` 宸茶兘绋冲畾楠岃瘉 hard / soft 涓や釜鍚庣妯″紡锛屾棤闇€閲嶆柊瀹炵幇闄嶇骇娴佺▼銆?
3. 杩橀渶瑕佷竴涓富杩涚▼杩炵画鎾斁绐楀彛锛岀‘淇?`4K` 鏍锋湰涓嶆槸鈥滃彧鎵撳紑涓嶆帹杩涒€濓紝鑰屾槸鐪熸杩涘叆鎾斁鐘舵€併€?


### 瑙ｅ喅鏂规

- 鏂板 `--4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`銆?
- 涓昏繘绋嬫鏌?`probe`銆佹椂闂存帹杩涖€乣late_drop` 涓庡綋鍓?backend锛涘瓙杩涚▼妫€鏌?hard / soft 妯″紡閮借兘杩涘叆鎾斁銆?
- 鏂板 `docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`锛屽悓姝ヤ换鍔℃竻鍗曘€佸樊璺濊瘎浼颁笌鐗堟湰璁板綍銆?


### 鏈湴鏍″缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`锛歚PASS`

- 褰撳墠鏍锋湰杈撳嚭鍖呭惈 `advance_ratio=0.968167`銆乣late_drops=0`銆乣hard.decoder_backend=D3D11VA`銆乣soft.decoder_backend=Software`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 闇€瑕佷竴涓湡姝ｈ秴杩?`80Mbps` 鐨勫洖褰掓牱鏈拰瀵瑰簲楠屾敹鍏ュ彛锛屾墠鑳藉畬鎴?`2.2.3`銆?
- 褰撳墠鏍锋湰闆嗗悎铏界劧瑕嗙洊鍒嗚鲸鐜囦笌鏍煎紡锛屼絾鐮佺巼鏅亶鍋忎綆锛屾棤娉曚綔涓洪珮鐮佺巼闂ㄧ銆?


### 鍒嗘瀽璁板綍

1. 鐜版湁 `collectFileProbeReport()` 娌℃湁鐩存帴杈撳嚭鐮佺巼锛屼絾 FFmpeg 鐨?`AVFormatContext::bit_rate` 鍙洿鎺ュ鐢ㄣ€?
2. 楂樼爜鐜囦换鍔＄殑鏍稿績涓嶅湪鍒嗚鲸鐜囷紝鑰屽湪浜庢牱鏈湡瀹炵爜鐜囥€佽繛缁挱鏀剧獥鍙ｆ帹杩涘拰鎺夊抚/涓㈠寘鎯呭喌銆?
3. 鍩轰簬褰撳墠 `D3D11VA + D3D11` 涓婚摼锛宍100Mbps` 绾у埆鐨?`1080p60` H.264 鏍锋湰宸茶兘绋冲畾杩涘叆鎾斁閾捐矾锛岄€傚悎浣滀负鏈湴鍥炲綊鍩虹嚎銆?


### 瑙ｅ喅鏂规

- 鏂板 `collectFormatBitrateBitsPerSecond()` 涓?`--high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`銆?
- 鍦?`tools/download_test_samples.ps1` 澧炲姞 `100Mbps` 鏍锋湰鐢熸垚璺緞锛屽苟鍦?`samples/README.md` 鏍囨敞鐢ㄩ€斻€?
- 鏂板 `docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`锛屽悓姝ヤ换鍔℃竻鍗曘€佸樊璺濊瘎浼颁笌鐗堟湰璁板綍銆?


### 鏈湴鏍″缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`锛歚PASS`

- 褰撳墠鏍锋湰杈撳嚭鍖呭惈 `format_bitrate_bps=102829290`銆乣advance_ratio=0.988444`銆乣late_drops=0`銆乣demux_dropped_packets=0`銆?


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



## 闂 57: 鍙戝竷闂ㄧ 6.5锛堥暱鏃舵挱鏀剧ǔ瀹氭€э級



**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `6.5` 闇€瑕佷竴涓彲閲嶅鎵ц鐨勬湰鍦板叆鍙ｏ紝楠岃瘉闀挎椂鎾斁绐楀彛鍐呮棤 crash 涓斾粛鑳芥寔缁帹杩涖€?


### 鍒嗘瀽璁板綍

1. 鐜版湁 `1080p60` / `4K` / `>80Mbps` 楠屾敹鍛戒护閮戒互鐭獥鍙ｄ负涓伙紝涓嶈兘鐩存帴浣滀负鈥滈暱鏃舵挱鏀炬棤 crash鈥濈殑璇佹槑銆?
2. `VideoPlayer` 宸叉彁渚?`play()` / `isPlaying()` / `getCurrentTime()` / `getDiagnosticsSnapshot()`锛屽彲浠ュ鐢ㄤ负 smoke 楠屾敹闂幆銆?
3. 鍙鍚屾椂楠岃瘉鎾斁鎬佷繚鎸併€佹椂闂存帹杩涗笌 `late_drop` / demux drop锛屽嵆鍙舰鎴愪竴涓冻澶熻交閲忕殑鍙戝竷闂ㄧ銆?


### 瑙ｅ喅鏂规

- 鏂板 `--long-playback-check .\juren-30s.mp4 10000`锛岃姹傞噰鏍风獥鍙ｄ笉灏戜簬 `5000ms`銆?
- 鍛戒护杈撳嚭 `probe_duration`銆乣renderer_backend`銆乣decoder_backend`銆乣advance_ratio`銆乣late_drops`銆乣demux_dropped_packets` 绛夌粨鏋勫寲瀛楁銆?
- 鏂板 `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`锛屽苟鍚屾浠诲姟娓呭崟銆佸樊璺濊瘎浼般€佺増鏈褰曚笌鍙樻洿璁板綍銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`锛歚PASS`

- 褰撳墠杈撳嚭鍖呭惈 `probe_duration=30.03`銆乣entered_playback_loop=true`銆乣still_playing_after_window=true`銆乣advance_ratio=0.996267`銆乣late_drops=0`銆乣demux_dropped_packets=0`銆乣renderer_backend=D3D11`銆乣decoder_backend=D3D11VA`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `7.1` 闇€瑕佷竴涓湡姝ｅ彲杩愯鐨勬彃浠跺涓伙紝鑰屼笉鍙槸鍐呭瓨涓殑鍏冩暟鎹崰浣嶃€?


### 鍒嗘瀽璁板綍

1. 鐜版湁 `PluginManager` 鍙兘鍋氶潤鎬佹弿杩扮娉ㄥ唽涓庡惎鍋滄爣璁帮紝鏃犳硶瑕嗙洊 `DLL` 鍔ㄦ€佸姞杞姐€佺増鏈吋瀹瑰拰鍗歌浇娓呯悊鍦烘櫙銆?
2. 浠ｇ爜搴撳凡缁忔湁 `FilterRegistry` 杩欑被澶╃劧鎵╁睍鐐癸紝閫傚悎浣滀负棣栦釜鎻掍欢闂幆鐨勫涓昏兘鍔涖€?
3. 濡傛灉娌℃湁绀轰緥 `DLL` 鍜屽懡浠よ楠屾敹鍏ュ彛锛屽氨鏃犳硶璇佹槑鎻掍欢绯荤粺宸蹭粠鈥滈鏋垛€濊繘鍏モ€滃彲杩愯鈥濄€?


### 瑙ｅ喅鏂规

- 鏂板 `include/plugin/plugin_api.h`锛屽畾涔夋彃浠跺涓绘帴鍙ｄ笌瀵煎嚭绗﹀彿銆?
- 閲嶅啓 `PluginManager`锛屾敮鎸佹寜璺緞鍔犺浇鎻掍欢銆佹牎楠?`API` 鐗堟湰銆佹墽琛?`initialize/shutdown` 鐢熷懡鍛ㄦ湡锛屽苟璺熻釜鎻掍欢娉ㄥ唽鐨勬护闀滃伐鍘傜敤浜庡嵏杞芥竻鐞嗐€?
- 鏂板 `sample_logger_plugin` 绀轰緥鎻掍欢涓?`--plugin-check` 楠屾敹鍛戒护锛涙彃浠朵細娉ㄥ唽 `sample_identity` 瑙嗛婊ら暅锛屼緵瀹夸富楠岃瘉娉ㄥ唽/鍗歌浇闂幆銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `build/Debug/modern-video-player.exe --plugin-check`锛歚PASS`

- 褰撳墠杈撳嚭鍖呭惈 `loaded_count=1`銆乣plugin_ids=sample_logger_plugin@0.1.0`銆乣sample_video_filter_registered=true`銆乣sample_video_filter_unloaded=true`銆乣errors=none`銆?


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

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `7.2` 闇€瑕佷竴涓湡姝ｅ彲璺戠殑 HTTP 鍒嗙墖涓嬭浇涓庣紦鍐插叆鍙ｏ紝鑰屼笉鏄仠鐣欏湪娓呭崟瑙ｆ瀽楠ㄦ灦銆?


### 鍒嗘瀽璁板綍

1. `HttpStreamDownloader` 涔嬪墠娌℃湁鐪熷疄璇绘祦瀹炵幇锛宍readChunk()` 姘歌繙杩斿洖绌烘暟缁勩€?
2. 褰撳墠浠ｇ爜宸茬粡鍏峰 HLS 娓呭崟瑙ｆ瀽鍣紝閫傚悎浠?HLS 濯掍綋娓呭崟浣滀负棣栦釜娴佸獟浣?smoke 鍏ュ彛銆?
3. 鍦ㄥ彈闄愮綉缁滅幆澧冧笅锛屾渶绋冲畾鐨勯獙鏀舵柟寮忔槸鍦ㄦ湰鏈鸿捣涓€涓皬鍨?HTTP 澶瑰叿鏈嶅姟锛岄伩鍏嶄緷璧栧閮ㄧ珯鐐广€?


### 瑙ｅ喅鏂规

- 閲嶅啓 `HttpStreamDownloader`锛屽熀浜?FFmpeg `avio` 鏀寔鐪熷疄 HTTP 鎵撳紑銆佸垎鍧楄鍙栥€佸唴閮ㄧ紦鍐层€丒OF 鐘舵€佷笌閿欒閫忎紶銆?
- 鏂板 `--streaming-buffer-check`锛屼笅杞?HLS 娓呭崟銆佽В鏋愮浉瀵?URL銆佹姄鍙栧墠 3 涓垎鐗囧苟楠岃瘉缂撳啿瀛楄妭鏁般€?
- 鏂板 `tools/start_streaming_fixture_server.ps1` 涓?`samples/streaming/hls_local/*` 澶瑰叿锛岀敤浜庢湰鏈?HTTP 鍥炲綊銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`锛歚PASS`

- 褰撳墠杈撳嚭鍖呭惈 `manifest_download_ok=true`銆乣manifest_parse_ok=true`銆乣segments_downloaded=3`銆乣buffered_bytes=621`銆乣buffer_ok=true`銆乣error=none`銆?


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





## 闂 60: 7.3 HLS/DASH 鑷€傚簲鐮佺巼



**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `7.3` 闇€瑕佷竴涓彲閲嶅鎵ц鐨?HLS/DASH 澶氱爜鐜囪В鏋愪笌鑷€傚簲鐮佺巼鏈湴楠屾敹鍏ュ彛銆?


### 鍒嗘瀽璁板綍

1. 鐜版湁 `HlsManifestParser` 鍙兘璇诲彇濯掍綋鎾斁鍒楄〃锛屾棤娉曞鐞?`master playlist`銆?
2. 鐜版湁 `DashManifestParser` 鍙煡閬?`Representation` 甯﹀锛屾棤娉曟嬁鍒板垵濮嬪寲鍒嗙墖鍜屽獟浣撳垎鐗?URL銆?
3. 鏈€绋冲Ε鐨勯獙鏀舵柟寮忎粛鏄湪鏈満 HTTP 澶瑰叿涓嬶紝鍒嗗埆楠岃瘉 HLS/DASH 鐨勬。浣嶉€夋嫨涓庡崌闄嶆。璺緞銆?


### 瑙ｅ喅鏂规

- 鎵╁睍 HLS/DASH 瑙ｆ瀽鍣紝琛ラ綈 variant / representation / `BaseURL` / 鍒濆鍖栧垎鐗?/ 濯掍綋鍒嗙墖鏄庣粏銆?
- 鏂板 `AdaptiveBitrateSelector`锛屽苟鍦?`main` 涓彁渚?`--adaptive-bitrate-check` 鍛戒护銆?
- 鏂板 `samples/streaming/abr_local/{hls,dash}` 鏍锋湰涓庢姤鍛婏紝楠岃瘉 HLS/DASH 鍦ㄧ粰瀹氬甫瀹藉簭鍒椾笅鐨勫崌鐮佺巼涓庨檷鐮佺巼鍒囨崲銆?


### 鏈湴楠屾敹缁撴灉

- `cmake --build build --config Debug`锛氶€氳繃銆?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`锛歚PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`锛歚PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`锛歚PASS`

- 褰撳墠杈撳嚭鍖呭惈 `switch_count=2`銆乣upswitch_count=1`銆乣downswitch_count=1`锛岃鏄?HLS/DASH 涓ゆ潯璺緞閮藉畬鎴愪簡涓€娆″崌妗ｅ拰涓€娆￠檷妗ｃ€?


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

## 闂 61: 寤虹珛閲岀▼纰戞爣绛撅紙v0.2.0-rc1 / v0.2.0锛?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 浠诲姟娓呭崟 `0.4` 灏氭湭瀹屾垚锛屼粨搴撶己灏?`v0.2.0-rc1` 涓?`v0.2.0` 鐨勯噷绋嬬鏍囩銆?


### 鍒嗘瀽璁板綍

1. 褰撳墠浠撳簱 `git tag --list` 涓虹┖锛岃鏄庤繕娌℃湁浠讳綍姝ｅ紡閲岀▼纰戞爣璁般€?
2. `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 浠嶅啓鐫€鈥滃綋鍓嶄粎鍓╃増鏈爣绛炬搷浣滄湭鎵ц鈥濓紝闇€瑕佸湪鏍囩寤虹珛鍚庣珛鍗冲洖鍐欍€?
3. 閲岀▼纰戞爣绛惧睘浜庝粨搴撶姸鎬佺殑涓€閮ㄥ垎锛岄櫎浜嗗垱寤烘爣绛炬湰韬紝杩橀渶瑕佸悓姝ヤ换鍔＄姸鎬佷笌鐗堟湰璁板綍銆?


### 瑙ｅ喅鏂规

- 寤虹珛 `v0.2.0-rc1` 涓?`v0.2.0` 涓や釜閲岀▼纰戞爣绛俱€?
- 鍚屾鏇存柊鐗堟湰璁板綍銆佸彉鏇磋褰曘€佸紑鍙戞棩蹇椼€佸樊璺濊瘎浼板拰浠诲姟娓呭崟銆?
- 鍩轰簬 `v0.2.0-rc1` 宸插彲绋冲畾寤虹珛锛屾墽琛岀害鏉?`5.3 姣忎釜閲岀▼纰戠粨鏉熷墠蹇呴』鍙墦 RC 鏍囩` 鍙垽瀹氫负婊¤冻銆?


### 鏈湴楠屾敹缁撴灉

- `git tag --list`锛氬垱寤哄墠涓虹┖锛屽垱寤哄悗鍖呭惈 `v0.2.0-rc1` 涓?`v0.2.0`銆?
- 浠诲姟娓呭崟 `5.3` 宸插彲闅?`0.4` 涓€骞跺嬀閫夈€?
- 鏍囩寤虹珛鍚庡彲閫氳繃 `git show <tag> --no-patch --stat` 楠岃瘉鎸囧悜鎻愪氦銆?


### 淇敼鏂囦欢

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 闂 62: 鎵ц瀹堝垯鍙ｅ緞鍚屾锛?.1 / 5.2锛?


**鏃ユ湡**: 2026-03-08

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 瀹堝垯椤?`5.1 / 5.2` 闇€瑕佹牴鎹綋鍓嶄粨搴撲笌浜や粯鑺傚鍋氫竴娆″彛寰勫悓姝ワ紝閬垮厤浠诲姟娓呭崟涓庡疄闄呮墽琛岀姸鎬佽劚鑺傘€?


### 鍒嗘瀽璁板綍

1. 鏈疆涓昏浠诲姟鎸夆€滃彂甯冮棬绂?鈫?鎻掍欢绯荤粺 鈫?娴佸獟浣撶紦鍐?鈫?ABR 鈫?閲岀▼纰戞爣绛?鈫?瀹堝垯鍙ｅ緞鈥濈殑椤哄簭涓茶鎺ㄨ繘锛屾病鏈夊嚭鐜拌秴杩?2 涓富浠诲姟骞惰鎺ㄨ繘鐨勮瘉鎹€?
2. `5.2` 鐨勫垽鏂潯浠舵槸鈥滄瘡鍛ㄤ簲鍙仛鏀舵暃鈥濓紝杩欒姹傛寔缁€х殑鍛ㄨ妭濂忚褰曪紝鑰屼笉浠呮槸涓€杞敹鍙ｇ粨鏋溿€?
3. 鍥犳 `5.1` 鍙互鍕鹃€夛紝`5.2` 浠嶅簲淇濇寔寰呭畬鎴愩€?


### 瑙ｅ喅鏂规

- 鍕鹃€?`5.1 WIP 闄愬埗锛氬悓鏃惰繘琛屼换鍔′笉瓒呰繃 2 涓猔銆?
- 淇濈暀 `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 涓哄緟瀹屾垚锛屽苟鍦ㄧ増鏈?鍙樻洿璁板綍涓啓鏄庡師鍥犮€?


### 鏈湴楠屾敹缁撴灉

- 浠诲姟娓呭崟宸叉洿鏂颁负锛歚5.1` 瀹屾垚銆乣5.2` 寰呭畬鎴愩€乣5.3` 瀹屾垚銆?
- 褰撳墠浠撳簱娌℃湁鏂板浠ｇ爜鏀瑰姩锛屾湰娆″彧鍚屾杩囩▼绾︽潫鍙ｅ緞銆?


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

- 鐢ㄦ埛瑕佹眰缁х画澶勭悊瀹堝垯椤瑰彛寰勶紝骞舵妸 `5.2 姣忓懆浜斿彧鍋氭敹鏁涳紙淇銆佸洖褰掋€佹枃妗ｏ級` 钀芥垚涓€浠藉彲鎵ц鐨勬祦绋嬬害鏉熸枃妗ｄ笌鑺傚璇存槑銆?


### 鍒嗘瀽璁板綍

1. `5.2` 褰撳墠浠嶄笉搴旂洿鎺ュ嬀閫夛紝鍥犱负瀹冭姹傜殑鏄法鍛ㄣ€侀噸澶嶅彂鐢熺殑杩囩▼璇佹嵁銆?
2. 浣嗗湪鏈嬀閫変箣鍓嶏紝浠嶇劧鍙互鍏堟妸鈥滃浣曟墽琛屽懆浜旀敹鏁涙棩鈥濇枃妗ｅ寲锛岄檷浣庡悗缁墽琛屽彛寰勬紓绉汇€?
3. 鏈€鍚堢悊鐨勮惤鍦版柟寮忥紝鏄柊澧炰竴浠界被浼兼搷浣滄墜鍐岀殑鏂囨。锛屾妸鍛ㄨ妭濂忋€佸厑璁镐簨椤广€佺姝簨椤广€佹墽琛岄『搴忋€佽緭鍑虹墿涓庡嬀閫変緷鎹叏閮ㄥ啓娓呮銆?


### 瑙ｅ喅鏂规

- 鏂板 `docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`锛屽皢 `5.2` 鍥哄寲涓哄彲鎵ц娴佺▼銆?
- 鍦?`docs/README.md` 澧炲姞鍏ュ彛锛屽苟鍚屾鐗堟湰璁板綍涓庡彉鏇磋褰曘€?
- 淇濇寔浠诲姟娓呭崟 `5.2` 鏈嬀閫夛紝寰呭悗缁法鍛ㄦ墽琛屽舰鎴愯瘉鎹悗鍐嶆洿鏂扮姸鎬併€?


### 鏈湴楠屾敹缁撴灉

- 鏂版枃妗ｅ凡瑕嗙洊鍛ㄤ竴鍒板懆浜旂殑鑺傚鍒嗗伐銆?
- 鏂版枃妗ｅ凡鍒楀嚭鏀舵暃鏃ュ厑璁镐簨椤广€佺姝簨椤广€佹渶鐭墽琛岃矾寰勪笌杈撳嚭鐗┿€?
- 褰撳墠浠撳簱娌℃湁鏂板浠ｇ爜鏀瑰姩锛屾湰娆″彧鏂板娴佺▼鏂囨。骞跺悓姝ユ枃妗ｇ储寮曚笌璁板綍銆?


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

- 鐢ㄦ埛瑕佹眰缁х画鎶?`5.2` 鐨勨€滅暀鐥曟ā鏉库€濊ˉ鍒?`daily_board / 鍛ㄦ姤` 閲岋紝浣垮懆浜旀敹鏁涚殑鎵ц璇佹嵁鏈夊浐瀹氳浇浣撱€?


### 鍒嗘瀽璁板綍

1. `5.2` 鐜板湪宸茬粡鏈夊師鍒欏拰娴佺▼锛屼絾杩樻病鏈夌湡姝ｉ潰鍚戞墽琛岀殑濉啓妯℃澘銆?
2. 褰撴棩鐪嬫澘鍜屾瘡鍛ㄦ眹鎬诲睘浜庝袱涓笉鍚岀矑搴︼細鍓嶈€呰褰曗€滃懆浜斿綋澶╁仛浜嗕粈涔堚€濓紝鍚庤€呰褰曗€滆繖涓€鍛ㄦ槸鍚︽弧瓒?5.2鈥濄€?
3. 鍥犳闇€瑕佸悓鏃惰ˉ榻愯繖涓ょ妯℃澘锛屾墠鑳藉湪鍚庣画璺ㄥ懆璇佹嵁绱Н鏃朵繚鎸佸彛寰勪竴鑷淬€?


### 瑙ｅ喅鏂规

- 鍦?`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md` 涓紝涓?Day 5 / Day 10 澧炲姞鏀舵暃鏃ヨ褰曞崱銆?
- 鏂板 `.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`锛屼綔涓烘瘡鍛ㄥ懆鎶ヤ笌鏀舵暃鐣欑棔妯℃澘銆?
- 鍚屾 `tasklist / WEEKLY_CONVERGENCE_PLAYBOOK / README / 鐗堟湰璁板綍`锛屾妸鍏ュ彛鍜岀敤閫斿啓娓呮銆?


### 鏈湴楠屾敹缁撴灉

- 鐜板湪姣忎釜鍛ㄤ簲閮藉彲浠ュ湪 `daily_board.md` 鐩存帴濉啓鑼冨洿鍐荤粨銆佸洖褰掑懡浠ゃ€乥locker 缁撹鍜岄樁娈电粨璁恒€?
- 鐜板湪鍙互鍩轰簬 `weekly_report_template.md` 澶嶅埗鐢熸垚姣忓懆鍛ㄦ姤瀹炰緥锛岀敤浜庡垽鏂槸鍚︽弧瓒?`5.2`銆?
- 褰撳墠浠撳簱娌℃湁鏂板浠ｇ爜鏀瑰姩锛屾湰娆″彧琛ラ綈妯℃澘涓庢枃妗ｅ叆鍙ｃ€?


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

- 鐢ㄦ埛瑕佹眰鍒楀嚭绋嬪簭褰撳墠鐨勬墍鏈夊姛鑳姐€佸彲鐢ㄤ娇鐢ㄦ柟寮忥紝浠ュ強鐩墠搴旇濡備綍楠岃瘉椤圭洰鐜版湁鍔熻兘锛屽苟灏嗚繖浜涗俊鎭褰曞埌鏂囨。涓€?


### 鍒嗘瀽璁板綍

1. 褰撳墠椤圭洰鐨勫姛鑳藉凡缁忎笉鍐嶅彧鏄€滃熀纭€鎾斁鈥濓紝杩樺寘鎷瓧骞曘€佹挱鏀惧垪琛ㄣ€佽缃€佸揩鎹烽敭銆佺珷鑺傘€丄-B銆佹埅鍥俱€佸抚姝ヨ繘銆佹牸寮忔帰娴嬨€乄indows 鍚庣銆佹彃浠朵笌娴佸獟浣撻獙鏀惰兘鍔涖€?
2. 杩欎簺鑳藉姏鐩墠鍒嗘暎鍦?`main.cpp` 甯姪杈撳嚭銆佷换鍔℃竻鍗曘€佹湰鍦伴獙鏀舵姤鍛婂拰鑻ュ共鏂囨。涓紝缂哄皯闈㈠悜闃呰鑰呯殑涓€椤靛紡鎬昏銆?
3. 鏈€鍚堥€傜殑鍋氭硶鏄柊澧炰竴浠解€滃姛鑳?/ 浣跨敤 / 楠岃瘉鈥濇€昏鏂囨。锛屽苟鍚屾绱㈠紩涓庢牴 README 鍏ュ彛銆?


### 瑙ｅ喅鏂规

- 鏂板 `docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`锛屾妸褰撳墠涓荤▼搴忚兘鍔涘垎鎴愨€滅敤鎴峰彲鐩存帴浣跨敤鈥濆拰鈥滃紑鍙?楠屾敹鑳藉姏鈥濅袱灞傛潵鏁寸悊銆?
- 鍦ㄦ枃妗ｄ腑缁欏嚭鏅€氭挱鏀炬柟寮忋€佽兘鍔涙帰娴嬫柟寮忋€侀厤缃柟寮忋€侀粯璁や氦浜掓柟寮忓拰鍔熻兘楠岃瘉琛ㄣ€?
- 鍚屾鏇存柊 `docs/README.md` 涓庢牴 `README.md`锛屼繚璇佸叆鍙ｅ彲瑙併€?


### 鏈湴楠屾敹缁撴灉

- 鏂版枃妗ｅ凡瑕嗙洊褰撳墠鍔熻兘鍒楄〃銆佹櫘閫氫娇鐢ㄦ柟寮忋€佽瘖鏂柟寮忋€侀厤缃柟寮忎笌涓撻」楠岃瘉鍏ュ彛銆?
- 鏂版枃妗ｅ凡鏄庣‘娴佸獟浣撱€佹彃浠躲€佹护闀滃寮虹瓑鑳藉姏鐨勫綋鍓嶈竟鐣岋紝閬垮厤璇垽涓哄畬鏁寸敤鎴峰姛鑳姐€?
- 褰撳墠浠撳簱娌℃湁鏂板浠ｇ爜鏀瑰姩锛屾湰娆″彧鏂板璇存槑鏂囨。骞跺悓姝ョ储寮曘€?


### 淇敼鏂囦欢

- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md

- docs/README.md

- README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md







## 闂 69: 鎾斁閾捐瘖鏂垎灞備笌 decoder drain / scheduler 瀹归敊琛ュ己



**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰缁х画鍥寸粫楂樼爜鐜囨挱鏀剧ǔ瀹氭€э紝璇勪及 `FrameQueue` 鑳屽帇銆乨ecoder `receive/send` 鏃跺簭銆乣Display::copyFrameData()`銆乣av_hwframe_transfer_data()` 涓?`Scheduler` 涓€娆￠噸鍚檺鍒剁瓑娼滃湪闂锛屽苟鍦ㄧ‘璁ゅ悗钀藉湴浼樺寲銆?


### 鍒嗘瀽璁板綍

1. 褰撳墠婧愮爜宸插叿澶囨潯浠舵垚绔嬫椂鐨?`D3D11` native render path锛屽洜姝ゆ棫鍒嗘瀽鏂囨。閲屸€滀富閾句竴瀹氱粡杩?copy-back + SDL upload鈥濈殑缁撹宸茬粡杩囨椂锛岃繍琛屾椂鏇撮渶瑕佺煡閬撯€滃綋鍓嶅埌搴曞懡涓簡 native / copy-back / swscale 鍝潯璺緞鈥濄€?
2. `decodeVideoFrame()` / `decodeAudioFrame()` 閲囩敤 receive-first 鎬濊矾鏈韩娌￠棶棰橈紝浣嗘鍓?packet queue EOF 鍚庢病鏈夌粰 codec 鍙戦€?`nullptr` drain锛岃€屼笖 send 鍚庡彧灏濊瘯涓€娆?receive锛屼笉澶熸帴杩戞垚鐔熸挱鏀惧櫒甯歌鐨?drain/feed 璇箟銆?
3. `demux_dropped_packets_` 涔嬪墠娌℃湁缁嗗垎鈥滈潪鐩爣娴佽蹇界暐鈥濅笌鈥滅洰鏍囨祦鍏ラ槦澶辫触鈥濓紝浼氳瀵煎鑳屽帇鍜屽悶鍚愮摱棰堢殑鍒ゆ柇銆?
4. `Scheduler` 涔嬪墠鍙繚鎶や簡瑙ｇ爜绾跨▼锛宺ender thread 娌℃湁鍚屾牱鐨勫紓甯镐繚鎶わ紱鍚屾椂娌℃湁缁撴瀯鍖栧鍑鸿儗鍘嬩笌 restart 鎸囨爣銆?
5. 鏈疆璁捐鍙傝€冧簡 `ffplay` 甯歌鐨?decoder drain 妯″紡锛屼互鍙?`mpv / MPC-HC` 甯歌鐨勨€滃厛鎶婅矾寰勪笌鍘熷洜閲忓嚭鏉ワ紝鍐嶈皟鍙傛暟鈥濈殑璇婃柇鎬濊矾銆?


### 瑙ｅ喅鏂规

- 閲嶅啓 `decodeVideoFrame()` / `decodeAudioFrame()`锛氭敼鎴愭寔缁?`receive -> send -> receive` 鐘舵€佹満锛屽苟鍦?packet EOF 鍚庡悜 codec 鍙戦€?`nullptr` 瀹屾垚 drain銆?
- 涓?`DiagnosticsSnapshot` 澧炲姞 demux drop 鍒嗙被銆乨ecoder `send_packet(EAGAIN)`銆乨rain 娆℃暟锛屼互鍙?`native/copy-back/swscale/filter-blocked` 瑙嗛璺緞璁℃暟銆?
- 涓?`SchedulerStats` 澧炲姞 video/audio 鑳屽帇浜嬩欢涓?video/audio/render restart 缁熻锛屽苟鎶?render thread 绾冲叆 `runProtectedLoop()`銆?
- 鎵╁睍 `--performance-log-check` 杈撳嚭锛岃鍚庣画 4K/楂樼爜鐜囧垎鏋愯兘鐩存帴鐪嬭褰撳墠鏄惁鍛戒腑 native path銆佹槸鍚︾湡鐨勫嚭鐜?queue drop銆佹槸鍚﹂绻佽儗鍘嬨€?


### 鏈湴楠屾敹缁撴灉

- `MSBuild build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`锛氶€氳繃銆?
- `build\Debug\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`锛氶€氳繃锛涘綋鍓嶇幆澧冮煶棰戝垵濮嬪寲澶辫触锛屼絾鏂拌瘖鏂瓧娈靛凡鎴愬姛瀵煎嚭锛屼笖鍙 `demux_dropped_packets` 鍦ㄦ湰娆℃牱鏈腑鍏ㄩ儴灞炰簬 `demux_ignored_packets`锛屼笉鏄?`queue drop`銆?


### 淇敼鏂囦欢

- include/core/scheduler.h

- src/core/scheduler.cpp

- include/core/player_core.h

- src/core/player_core.cpp

- src/main.cpp

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂 70: PlayerCore 鐘舵€佹満閲嶈璁＄涓€闃舵



**鏃ユ湡**: 2026-03-19

**鐘舵€?*: 宸茶В鍐?


### 闂鎻忚堪

- 鐢ㄦ埛瑕佹眰鍏堣惤鍦版挱鏀惧櫒鍐呮牳鐘舵€佹満閲嶈璁＄涓€闃舵锛氫繚鐣欏澶?`PlaybackState` 鍏煎锛屽彧鍦?`PlayerCore` 鍐呴儴鎷嗗嚭浼氳瘽鎬併€佽繍琛屾€佸拰娴佹按绾挎€侊紝骞舵敹鍙ｆ暎鐐圭姸鎬佹敼鍐欍€?
- 鏈疆鏄庣‘涓嶅仛 timeline serial銆丒OF -> Ended 璇箟閲嶅啓锛屼篃涓嶆彁鍓嶅崌绾?`Scheduler` 濂戠害銆?


### 鏃ュ織杈撳嚭

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 鍒嗘瀽璁板綍

1. 褰撳墠 `PlaybackState` 涔嬪墠鍚屾椂鎵胯浇浜?UI 鎾斁鎬併€佷細璇濇€佸拰娴佹按绾胯繃绋嬫€侊紝`deferred stop` 杩橀澶栨父绂诲湪缁熶竴鐘舵€佹満涔嬪銆?
2. `open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 鐩存帴鍐?`state_`锛屽鑷寸姸鎬佽縼绉绘棩蹇椼€侀潪娉曡縼绉讳繚鎶ゅ拰鍚庣画 serial 鍖栭兘娌℃湁绋冲畾钀界偣銆?
3. `Scheduler` 褰撳墠浠嶅彧鏈?`running_ / paused_`锛涙湰杞渶鍚堢悊鐨勮竟鐣岋紝鏄厛璁?`PlayerCore` 鎴愪负鐘舵€佹潈濞侊紝鍐嶆妸鏇寸粏鐨勬帶鍒跺揩鐓х暀缁欎笅涓€闃舵銆?


### 瑙ｅ喅鏂规

- 鍦?`PlayerCore` 鍐呴儴鏂板 `SessionState / RunState / PipelinePhase` 涓?`CoreStateSnapshot`銆?
- 鏂板缁熶竴 transition helper 涓?`publishPlaybackStateFromInternalState()`锛岃瀵瑰 `PlaybackState` 鎴愪负鍐呴儴鐘舵€佺殑鎶曞奖銆?
- 灏?`eof_reached / pending_seek / deferred_stop_pending` 绾冲叆缁熶竴蹇収绠＄悊锛屽苟鍦ㄧ姸鎬佽縼绉绘椂杈撳嚭缁撴瀯鍖栨棩蹇椼€?
- 淇濇寔鐜版湁绾跨▼璧峰仠涓?`Scheduler` 璋冪敤鏂瑰紡鍩烘湰涓嶅彉锛岄伩鍏嶇涓€闃舵鑼冨洿澶辨帶銆?


### 鏈湴楠屾敹缁撴灉

- Debug 鏋勫缓閫氳繃銆?
- `PlayerCore` 鐩稿叧浠ｇ爜涓紝鏁ｇ偣 `state_.store/exchange` 宸叉敹鍙ｅ埌缁熶竴 publish 鍏ュ彛銆?
- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md`銆?


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

- 鐢ㄦ埛瑕佹眰缁х画鎵ц鎾斁鍣ㄥ唴鏍哥姸鎬佹満閲嶈璁＄浜岄樁娈碉紝鑼冨洿闄愬畾鍦?seek/flush timeline serial 鍖栵紝涓嶅姩 copy-back銆丼oftwareSDL銆乁I 灞傚拰澶栭儴 `PlaybackState`銆?
- 鏈疆鐨勬牳蹇冪洰鏍囷紝鏄妸 seek/stop/deferred stop 鐩稿叧璺緞閲屽鏃ф椂闂寸嚎鏁版嵁鐨勫鐞嗭紝浠庘€渇lush 杞竻鐞嗏€濆崌绾т负鈥渟erial 纭け鏁堚€濄€?


### 鏃ュ織杈撳嚭

```text

Debug build passed:

C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m

```



### 鍒嗘瀽璁板綍

1. 绗竴闃舵宸茬粡鎶?`SessionState / RunState / PipelinePhase` 鍜岀粺涓€ transition 鍏ュ彛钀藉埌浜?`PlayerCore`锛屽洜姝ょ浜岄樁娈靛叿澶囩ǔ瀹氱殑 serial 鏀跺彛鐐广€?
2. seek 鏃у疄鐜扮殑闂锛屼笉鏄?flush 涓嶅澶氾紝鑰屾槸 packet/frame 缂哄皯鏃堕棿绾胯韩浠斤紱鍗充娇鍋氫簡 `pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop() + 浜屾 flush`锛屾棫鏁版嵁浠嶇劧鍙兘鏅氬埌銆?
3. `ThreadSafeQueue` / `FrameQueue` 鏈綋閮芥病鏈?generation/serial锛宎udio consumer 绾跨▼涔熶笉浼氬湪 seek 鏃惰嚜鐒堕€€鍑猴紝render 璺緞涔嬪墠鍚屾牱娌℃湁 serial gate锛屾墍浠ュ繀椤绘妸鈥滃簾寮冭鍒欌€濇斁鍒?packet/frame 鏈韩鍜屾秷璐硅竟鐣屼笂銆?
4. 杩欒疆鍥犳閫夋嫨 item-level serial锛岃€屼笉鏄厛鎶?queue 瀹瑰櫒鍋氭垚鎾斁鍣ㄤ笓鐢?epoch 瀹瑰櫒锛涜繖鏍锋敼鍔ㄦ洿鑱氱劍锛屼篃鏇寸鍚堚€滅浜岄樁娈靛厛鍋?seek/flush serial 鍖栤€濈殑鑼冨洿鎺у埗銆?


### 瑙ｅ喅鏂规

- 鏂板 `TimelineSerial`锛屽苟璁?`DemuxPacket`銆乣VideoFrame`銆乣AudioFrame` 閮芥惡甯?serial銆?
- 鍦?`PlayerCore` 鍐呴儴鏂板 `timeline_serial / pending_seek_serial` 鍙婄粺涓€ helper锛岀姝㈠湪 `seek / stop / open / deferred stop` 鍚勫闆舵暎鑷 serial銆?
- `open` 鎴愬姛鍚庡缓绔嬮涓?serial锛沗seek` 寮€濮嬫椂鍏堝垎閰?`pending_seek_serial`锛岃鎺ュ彈杈圭晫鍏堝垏璧帮紝seek 鎴愬姛鍚庡啀婵€娲伙紱`stop / requestDeferredStop` 浼氱珛鍗虫帹杩?serial銆?
- demux 绾跨▼鍦ㄥ惎鍔ㄦ椂鎹曡幏 active serial锛宒ecode/render/audio consumer 鍏ㄩ摼璺寜 serial 涓㈠純 stale 鏁版嵁锛沗flush` 缁х画淇濈暀锛屼絾闄嶇骇涓鸿緟鍔╂竻鎵€?
- diagnostics 鍜屼笓椤规鏌ュ懡浠ゆ柊澧?`timeline_serial / pending_seek_serial` 杈撳嚭锛屼究浜庡悗缁户缁仛 EOF/Ended 涓?Scheduler 濂戠害闃舵銆?


### 鏈湴楠屾敹缁撴灉

- Debug 鏋勫缓閫氳繃銆?
- packet/frame 鍒?render/audio consumer 鐨勯摼璺凡缁忓叿澶?serial 涓㈠純杈圭晫銆?
- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY17_TIMELINE_SERIALIZATION_REDESIGN.md`銆?


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

## 闂 84: PlayerCore 鍓綔鐢ㄩ泦涓寲涓?runtime failure/recovery policy 鏀跺彛

**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- timeline serial 涓?queue generation 宸茬粡鎶?seek/flush 鐨勬暟鎹竟鐣岀‖鍖栵紝浣?`PlayerCore` 鍏ュ彛閲屼粛娣风潃绾跨▼銆佽澶囥€侀槦鍒楀拰鏃堕挓鍓綔鐢ㄣ€?- `SchedulerControlSnapshot` 杩樺彧鏈夋渶灏忔€侊紝scheduler 瀵?`clock_source`銆乤udio-master 涓?ended policy 鐨勪緷璧栦粛鐒舵病鏈夊畬鍏ㄧ粨鏋勫寲銆?- runtime fatal 鐐瑰鏋滅户缁垎鏁ｅ鐞嗭紝浼氭妸 stopping / session release / error emit 鍐嶆鎷嗘暎銆?
### 鏃ュ織杈撳嚭
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 鍒嗘瀽璁板綍
1. 杩欒疆鐨勯噸鐐逛笉鏄户缁敼 timeline serial锛岃€屾槸鎶娾€滅姸鎬佽縼绉讳箣鍚庤鍋氫粈涔堝姩浣溾€濇敹鍙ｆ垚缁熶竴鍓綔鐢ㄦā鍨嬨€?2. `deferred stop` 搴旇淇濈暀涓哄紓姝?stop completion 鏈哄埗锛屼絾涓嶅簲缁х画浣滀负鏃佽矾涓氬姟鐘舵€佹潵婧愩€?3. scheduler 宸叉湁 `run_state / pipeline_phase / accepted_timeline_serial`锛岀户缁墿鎴?`clock_source + audio master + ended policy` 缁撴瀯锛岃兘閬垮厤鍐嶅洖鍒伴浂鏁ｅ竷灏斾綅妯″紡銆?4. runtime failure 蹇呴』鍏堢粺涓€ recovery policy锛屽啀閫愭鎵╄鐩栫偣锛涘惁鍒欐瘡涓柊 fatal 鐐归兘浼氬鍒朵竴閬?stop/release/error 鍒嗘敮銆?
### 澶勭悊缁撴灉
- `PlayerCore` 宸叉柊澧炰竴缁勭粺涓€ side-effects helpers锛屽苟鎶?`play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 鐨勬牳蹇冨壇浣滅敤杩佸叆杩欎簺 helpers銆?- `SchedulerControlSnapshot` 宸叉柊澧?`clock_source`銆乣audio_output_initialized`銆乣audio_master_sync_active`銆乣ended_policy`锛宻cheduler 宸叉帴鍏ヨ繖浜涘瓧娈点€?- 鏂板 `FailureRecoveryPolicy` 涓?`handleRuntimeFailure()`锛屽苟鎶?decode/resample 鍏抽敭 fatal 鐐规帴鍏ョ粺涓€鎭㈠鍏ュ彛銆?- 宸叉柊澧?analysis锛歚docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md`銆?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓閫氳繃銆?- 杩欒疆鏈敼 UI 灞傘€乧opy-back銆丼oftwareSDL 鍜屽閮?`PlaybackState`銆?- 褰撳墠鍓╀綑椋庨櫓宸叉槑纭敹鏁涘埌锛氭洿瀹屾暣鐨?`SchedulerControlSnapshot`銆乣FailSession` 鐪熸鍚敤鏃剁殑绾跨▼瀹夊叏杈圭晫锛屼互鍙婃洿缁嗙殑 ended/audio-master policy銆?
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
- 鐢ㄦ埛瑕佹眰缁х画娑堝寲鍓╀綑椋庨櫓锛歚SchedulerControlSnapshot` 缁堢増绛栫暐鍖栥€乣FailSession` 瀹炵敤鍖栥€佷互鍙?generation 涓?serial 鑱岃矗杈圭晫寮哄寲銆?
### 鏃ュ織杈撳嚭
```text
Debug build passed:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors
```

### 鍒嗘瀽璁板綍
1. scheduler 闇€瑕佹槑纭秷璐圭瓥鐣ユ灇涓撅紝鑰屼笉鏄户缁緷璧栭殣寮忓竷灏旂粍鍚堛€?2. `FailSession` 闇€瑕佽鐩栦細璇濈骇涓嶅彲鎭㈠閿欒锛屽苟纭繚 stop/join 鍦?worker 涓婁笅鏂囧彲瀹夊叏鎵ц銆?3. generation 鍙В鍐冲鍣ㄨ竟鐣屼腑鏂紱serial 鎵嶆槸鏃ф椂闂寸嚎纭け鏁堜富鍒ゅ畾锛岄渶閫氳繃 stale drop 璁℃暟鐩磋瑙傚療銆?
### 澶勭悊缁撴灉
- 鎵╁睍 `SchedulerControlSnapshot`锛氭柊澧?`clock_policy`銆乣audio_master_policy`銆乣audio_buffered_seconds`锛屽苟鎵╁睍 ended policy銆?- `Scheduler` render wait 鏀逛负绛栫暐椹卞姩锛沗Scheduler::stop()` 琛?self-join 淇濇姢銆?- `handleRuntimeFailure()` 鏀跺彛澧炲己骞惰ˉ缁熻锛涘叧閿笉鍙仮澶嶉敊璇偣鍒囧埌 `FailSession`銆?- 鏂板 stale serial drop 璁℃暟骞舵帴鍏?diagnostics 涓庢鏌ュ懡浠よ緭鍑恒€?- 鏂板 analysis锛歚docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md`銆?
### 鏈湴楠屾敹缁撴灉
- Debug 鏋勫缓閫氳繃銆?- 鏈疆鏈敼 UI 灞傘€佸閮?`PlaybackState`銆乧opy-back銆丼oftwareSDL銆?
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
- 鐢ㄦ埛瑕佹眰缁х画鎸?WORKFLOW 鎺ㄨ繘锛屼紭鍏堣ˉ榻愯繛缁?seek / 鏆傚仠鎬?seek / close-reopen 鐨?CLI 鍥炲綊妫€鏌ャ€?- 鐩爣鏄緭鍑烘満鍣ㄥ彲璇?`key=value` 鍜?`result=PASS/FAIL`锛屽苟瑕嗙洊 stale/serial/FailSession 绾︽潫銆?
### 鏃ュ織杈撳嚭
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m
0 warnings / 0 errors

Checks:
build\Debug\modern-video-player.exe --seek-burst-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --paused-seek-serial-check .\juren-30s.mp4 -> PASS
build\Debug\modern-video-player.exe --close-reopen-serial-check .\juren-30s.mp4 -> PASS
```

### 鍒嗘瀽璁板綍
1. 鍏堣ˉ `DiagnosticsSnapshot` 鐨勯潪娉曡縼绉昏鏁帮紝閬垮厤 CLI 鍙兘闈犳棩蹇楁枃鏈垽瀹氥€?2. 杩炵画 seek 鍦烘櫙涓彲瑙傚療鍒?`illegal_pipeline_transitions` 鍦?`Draining -> Seeking` 鏈夊閲忥紝浣嗗綋鍓嶆湭浼撮殢 `FailSession`銆?3. 鍒ゅ畾閫昏緫浼樺厛淇濊瘉 `FailSession` 璺緞涓嶅嚭鐜伴潪娉曡烦杞紙`fail_session_transition_ok`锛夛紝骞朵繚鐣欓潪娉曡縼绉昏鏁扮敤浜庡悗缁笓椤规敹鏁涖€?
### 澶勭悊缁撴灉
- `PlayerCore`锛?  - 鏂板骞跺鍑?`illegal_session_transitions / illegal_run_transitions / illegal_pipeline_transitions`
  - 闈炴硶杩佺Щ鍒嗘敮璁℃暟 + diagnostics reset + diagnostics 鏃ュ織杈撳嚭
- CLI锛?  - 鏂板 `--seek-burst-serial-check`
  - 鏂板 `--paused-seek-serial-check`
  - 鏂板 `--close-reopen-serial-check`
  - 鐜版湁 `--performance-log-check`銆乣--software-video-decode-check` 鍚屾瀵煎嚭闈炴硶杩佺Щ璁℃暟
- 鏂囨。锛?  - 鏂板 `docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md`

### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 87: serial/failsession 鍥炲綊澧炲姞涓€閿仛鍚?gate锛堥檷浣庢紡璺戦闄╋級

**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 涓変釜 serial/failsession 涓撻」鎺㈤拡宸茬粡鍙敤锛屼絾鎵ц浠嶄緷璧栦汉宸ヤ覆琛岃皟鐢ㄣ€?- 鍦ㄦ寔缁凯浠ｄ腑锛屼汉宸ヤ覆琛屾柟寮忓鏄撴紡璺戞煇涓€椤癸紝瀵艰嚧鍥炲綊 gate 涓嶇ǔ瀹氥€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 鎺㈤拡鏈韩宸插叿澶囩粺涓€鐨?`key=value` 鍒ゅ畾瀛楁锛岀己鍙ｅ湪鈥滄墽琛屽叆鍙ｈ仛鍚堚€濄€?2. 鑻ヤ笉鎻愪緵鑱氬悎鍏ュ彛锛岃皟鐢ㄦ柟闇€閲嶅缁存姢涓夋潯鍛戒护涓庡弬鏁帮紝鍥炲綊鑴氭湰鏄撳垎鍙夈€?3. 鍏堝湪 CLI 灞傝ˉ鑱氬悎 gate锛屽彲浠ュ湪涓嶆敼 `PlayerCore` 鏍稿績閫昏緫鐨勫墠鎻愪笅闄嶄綆鎵ц娈嬩綑椋庨櫓銆?
### 澶勭悊缁撴灉
- `src/main.cpp` 鏂板锛?  - `runSerialFailSessionRegressionCheck(...)`
  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 鑱氬悎鍛戒护鍐呴儴椤哄簭鎵ц锛?  - `runSeekBurstSerialCheck`
  - `runPausedSeekSerialCheck`
  - `runCloseReopenSerialCheck`
- 鏂板鑱氬悎缁撴灉瀛楁锛?  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`
- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md`

### 淇敼鏂囦欢
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 88: 寮哄埗 FailSession 鍥炲綊鎺㈤拡涓?codec 閿侀噸鍏ュ穿婧冧慨澶?
**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- `FailSession` 浠嶄富瑕佷緷璧栫湡瀹為敊璇Е鍙戯紝鍥炲綊瑕嗙洊涓嶇ǔ瀹氥€?- 鍦ㄨˉ寮哄埗鎺㈤拡鏃讹紝瑙﹀彂浜嗚В鐮佺嚎绋嬭繘鍏?`FailSession` 鐨?codec 閿侀噸鍏ュ紓甯革紙`device or resource busy`锛夈€?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. 鍏堝紩鍏ュ彲鎺ф敞鍏ョ偣锛岀‘淇?`FailSession` 璺緞鍙绋冲畾瑙﹀彂銆?2. 寮哄埗瑙﹀彂鍚庣珛鍗虫毚闇插悓绾跨▼ codec 閿侀噸鍏ラ棶棰橈紝璇存槑璇ヨ矾寰勬鍓嶆湭琚厖鍒嗗帇娴嬨€?3. 淇閿佽涔夊悗锛宍FailSession` 璺緞鍙ǔ瀹氳惤鍒?`session=Failed / playback=Stopped`锛岄潪娉曡縼绉昏鏁颁繚鎸?0銆?
### 澶勭悊缁撴灉
- `PlayerCore::decodeVideoFrame` 澧炲姞 `MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE` 娴嬭瘯娉ㄥ叆閫昏緫銆?- `main.cpp` 鏂板 `--forced-failsession-check <media_file> [sample_ms]`锛岃緭鍑哄畬鏁存満鍣ㄥ彲璇?gate 瀛楁銆?- `video_codec_mutex_`銆乣audio_codec_mutex_` 鏀逛负 `std::recursive_mutex`锛屽苟鍚屾鏇存柊 `decodeVideoFrame/decodeAudioFrame` 閿佺被鍨嬨€?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md`銆?
### 淇敼鏂囦欢
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂 89: run_all_checks 鎺ュ叆 forced-failsession 涓€閿?gate

**鏃ユ湡**: 2026-03-20
**鐘舵€?*: 宸茶В鍐?
### 闂鎻忚堪
- 鏃ュ父涓€閿洖褰掕剼鏈?`tools/run_all_checks.ps1` 鏈粯璁ゆ墽琛?`--forced-failsession-check`锛孎ailSession 璺緞瑕嗙洊渚濊禆浜哄伐鍗曠嫭鎵ц銆?
### 鏃ュ織杈撳嚭
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

### 鍒嗘瀽璁板綍
1. `--forced-failsession-check` 宸插彲绋冲畾 PASS锛屽簲鎻愬崌鍒版壒澶勭悊榛樿娴佺▼銆?2. 涓€閿剼鏈渶瑕佹妸 forced path 璁句负纭?gate锛岃€屼笉鏄俊鎭€ф楠ゃ€?3. 鍙傛暟璁捐搴斾繚鎸佸悜鍚庡吋瀹癸紝榛樿澶嶇敤 `ProbeFile` 鏈€灏忓寲杩佺Щ鎴愭湰銆?
### 澶勭悊缁撴灉
- `tools/run_all_checks.ps1` 鏂板鍙傛暟锛?  - `ForcedFailSessionFile`
  - `ForcedFailSessionSampleMs`
- 鎵ц椤哄簭鐢变袱姝ユ敼涓轰笁姝ワ細
  - `[1/3]` probe
  - `[2/3]` forced-failsession gate
  - `[3/3]` format regression
- gate 閫昏緫锛?  - probe 闈為浂 -> 鐩存帴閫€鍑猴紱
  - forced-failsession 闈為浂 -> 鐩存帴閫€鍑哄苟璺宠繃 regression銆?- 鏂板鍒嗘瀽鏂囨。锛歚docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md`銆?
### 淇敼鏂囦欢
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 问题 90: OpenGL 原生 D3D11 互操作 stop/close 异常退出

**日期**: 2026-03-24
**状态**: 已解决
### 问题描述
- `Release` 下执行 `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 时，日志显示已启用 `OpenGL native D3D11 interop`，但 stop/close 阶段异常退出，CLI 没有打印最终 `performance-log-check.result`。
- 同一轮样本中 native 吞吐异常偏低，只渲染了约 3 帧。
### 日志输出
```text
OpenGL native D3D11 interop enabled for AV_PIX_FMT_D3D11 surfaces
D3D11VA decoder bound to renderer-owned D3D11 device
[diag:audio-backpressure] ... render(out=3,...) video(path_native=3,copyback=0,...)
EXITCODE=2173
```
### 分析记录
1. OpenGL 原生路径与 D3D11 主渲染器不同，它让 FFmpeg 硬解和 OpenGL 渲染线程共享 renderer-owned D3D11 device/context。
2. 代码检查发现 OpenGL 路径没有像 D3D11 主链那样启用 `ID3D11Multithread::SetMultithreadProtected(TRUE)`。
3. 继续检查 session 释放顺序，发现 `applySessionReleaseSideEffects()` 先释放 decoder/hw context，再关闭 renderer；这对持有 native `AVFrame` 的缓存帧和渲染线程不安全。
### 处理结果
- 在 `src/render/opengl_video_renderer.cpp` 中补齐 `ID3D11Multithread` 多线程保护。
- 在 `src/core/player_core.cpp` 中把缓存 native frame 与 renderer 关闭前置到 decoder/hw context 释放之前。
- 复测后 OpenGL 原生路径吞吐恢复正常，stop/close 可稳定完成。
### 复测结果
```text
performance-log-check.renderer_backend=OpenGL
performance-log-check.decoder_backend=D3D11VA
performance-log-check.video_native_output_frames=62
performance-log-check.video_copy_back_frames=0
performance-log-check.render_frames=47
performance-log-check.result=PASS
EXITCODE=0
```
### 修改文件
- src/render/opengl_video_renderer.cpp
- src/core/player_core.cpp
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
