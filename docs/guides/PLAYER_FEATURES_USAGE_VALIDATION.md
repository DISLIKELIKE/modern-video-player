# 当前功能、使用方式与验证指南

> 2026-03-24 更新：`OpenGLVideoRenderer` 已从 stub 变为可用的 opt-in GPU 后端，并已补齐基础字幕/OSD 叠加与 `D3D11VA -> OpenGL` 原生互操作。
> 当前播放器的渲染后端能力为：`SDL` 可用、`D3D11` 为默认主路径、`OpenGL` 可通过 `MVP_RENDERER_BACKEND=opengl` 显式启用。
> 本轮 `Release` 验证结果：`$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000` 输出 `renderer_backend=OpenGL`、`decoder_backend=D3D11VA`、`video_copy_back_frames=0`、`result=PASS`。
> 现阶段 `OpenGL` 仍不是成熟播放器级 GPU 后端；与 `mpv / MPC-HC` 的差距主要还在 `ASS/SSA` 完整排版、HDR/色彩管理、多平台后端策略收敛和更强的驱动兼容层。
鏈枃妗ｆ眹鎬绘埅鑷?`2026-03-08` 褰撳墠涓荤▼搴?*宸茬粡钀藉湴涓斿彲楠岃瘉**鐨勮兘鍔涳紝鍥炵瓟涓変釜闂锛?

- 鐜板湪绋嬪簭宸茬粡鏈夊摢浜涘姛鑳斤紱
- 浣犲彲浠ラ€氳繃鍝簺鏂瑰紡浣跨敤瀹冿紱
- 鐩墠搴旇濡備綍楠岃瘉杩欎簺鍔熻兘銆?

> 璇存槑锛氭湰鏂囨。鍖哄垎鈥滅敤鎴峰彲鐩存帴浣跨敤鐨勫姛鑳解€濆拰鈥滃紑鍙?楠屾敹鑳藉姏鈥濄€傚儚鎻掍欢銆佹祦濯掍綋缂撳啿銆丠LS/DASH ABR 杩欑被鑳藉姏铏界劧宸茬粡瀹炵幇骞舵湁鏈湴楠屾敹锛屼絾**涓嶇瓑浜?*宸茬粡鎴愪负瀹屾暣鐨勭粓绔敤鎴锋挱鏀惧叆鍙ｃ€?

## 1. 褰撳墠鍔熻兘鎬昏

### 1.1 鐢ㄦ埛鍙洿鎺ヤ娇鐢ㄧ殑鎾斁鍔熻兘

| 绫诲埆 | 褰撳墠鑳藉姏 | 璇存槑 |
|------|----------|------|
| 鏈湴鎾斁 | 鏈湴闊宠棰戞枃浠舵挱鏀?| 鍏峰闊宠棰戣В鐮併€佺敾闈㈡樉绀恒€侀煶棰戞挱鏀句笌 A/V 鍚屾 |
| 鎾斁鍒楄〃 | 澶氭枃浠堕『搴忔挱鏀俱€佹湰鍦?`m3u8` 鎾斁鍒楄〃 | 鏀寔涓婁竴椤?涓嬩竴椤广€丒OF 鑷姩涓嬩竴椤广€佹仮澶嶄笂娆＄储寮?|
| 瀛楀箷 | 澶栨寕 `SRT` 鍔犺浇銆佽嚜鍔ㄦ帰娴嬪悓鍚嶅瓧骞曘€佸瓧骞曞紑鍏?| 鏀寔鎾斁/鏆傚仠/seek 鍚庡悓姝ヤ笌瀛楀箷鏃跺欢璋冭妭 |
| 鍩虹鎺у埗 | 鎾斁/鏆傚仠銆侀€€鍑恒€佸叏灞忋€佹嫋鍔ㄨ繘搴︽潯 seek | 閫傚悎鏈湴鏂囦欢鐨勬棩甯告挱鏀?|
| 闊抽噺涓庨潤闊?| 闊抽噺澧炲噺銆侀紶鏍囨嫋鍔ㄩ煶閲忔潯銆侀潤闊冲垏鎹?| 褰撳墠鍩轰簬 `SDL` 闊抽杈撳嚭 |
| 鎾斁閫熷害 | 鍙橀€熼檷閫?/ 鍗囬€?/ 鎭㈠ `1.0x` | 璁剧疆鍙寔涔呭寲 |
| 绮剧粏璺宠浆 | `5s / 30s` 蹇繘蹇€€銆乣1..9` 鐧惧垎姣旇烦杞?| `1..9` 瀵瑰簲 `10%..90%` |
| 绔犺妭瀵艰埅 | 涓婁竴绔?/ 涓嬩竴绔?| 渚濊禆濯掍綋绔犺妭淇℃伅 |
| A-B Repeat | 璁剧疆 A 鐐?/ B 鐐?/ 娓呴櫎 A-B 寰幆 | 閫傚悎鐭墖娈靛弽澶嶈鐪?|
| 甯ф杩?| 鏆傚仠鎬佸崟甯у悗閫€ / 鍓嶈繘 | 榛樿鎸夐敭 `,` / `.` |
| 鎴浘 | 淇濆瓨褰撳墠鐢婚潰鎴浘 | 杈撳嚭鍒?`screenshots/` 鐩綍锛屾牸寮忎负 `.ppm` |
| 璁剧疆鎸佷箙鍖?| 鍚姩鍔犺浇銆侀€€鍑轰繚瀛樸€佸け璐ュ洖閫€榛樿鍊?| 褰撳墠瑕嗙洊闊抽噺銆侀€熷害銆侀煶/瀛楀箷鏃跺欢銆佺‖瑙ｅ亸濂姐€佹挱鏀惧垪琛ㄦ仮澶嶃€佺儹閿?|
| 鐑敭绯荤粺 | 榛樿鐑敭銆佹寔涔呭寲銆佽嚜瀹氫箟銆佸啿绐佹娴嬨€佹仮澶嶉粯璁?| 鑷畾涔夊叆鍙ｄ负 `config/player_settings.ini` |
| 鏍煎紡鎺㈡祴 | 濯掍綋鎺㈡祴涓庤繍琛屾椂鑳藉姏鏌ョ湅 | 鏀寔 `--capabilities`銆乣--probe-file`銆乣--evaluate-target` |

### 1.2 骞冲彴涓庡悗绔兘鍔?

| 绫诲埆 | 褰撳墠鑳藉姏 | 璇存槑 |
|------|----------|------|
| 瑙嗛瑙ｇ爜 | 杞欢瑙ｇ爜 + `D3D11VA` 纭В | 鍙€氳繃閰嶇疆鍋忓ソ纭В锛屽け璐ュ彲鍥為€€杞В |
| 瑙嗛娓叉煋 | `SDL` 鍙敤锛宍D3D11` 鏈€灏忛摼璺彲鐢?| `D3D11` 澶辫触鍙洖閫€ `SDL` |
| 鏍煎紡瑕嗙洊 | 涓诲姏瀹瑰櫒 / 瑙嗛缂栫爜 / 闊抽缂栫爜鐭╅樀宸茶ˉ榻?| 缁撴灉鍙€氳繃 `--capabilities` 鍜屽洖褰掓姤鍛婅拷婧?|
| 鎬ц兘鐩爣 | `1080p60`銆乣4K`銆乣>80Mbps` 鏍锋湰宸叉湁鏈湴楠屾敹 | 鍏峰鎬ц兘鏃ュ織杈撳嚭涓庨檷绾ч獙璇?|

### 1.3 宸插疄鐜颁絾鍋忓紑鍙?楠屾敹鍚戠殑鑳藉姏

| 绫诲埆 | 褰撳墠鑳藉姏 | 褰撳墠瀹氫綅 |
|------|----------|----------|
| 鎻掍欢绯荤粺 | `DLL` 鍔ㄦ€佸姞杞姐€丄PI 鐗堟湰鏍￠獙銆佸垵濮嬪寲/鍗歌浇銆佺ず渚嬫彃浠?| 宸插彲楠岃瘉锛屾殏鏃犵敤鎴风骇鎻掍欢绠＄悊 UI |
| 娴佸獟浣撳熀纭€ | 鐪熷疄 HTTP 涓嬭浇銆丠LS/DASH 娓呭崟瑙ｆ瀽銆佸垎鐗囩紦鍐?| 宸插彲鏈湴楠屾敹锛屽皻鏈舰鎴愬畬鏁存挱鏀鹃摼璺?|
| 鑷€傚簲鐮佺巼 | HLS/DASH 澶氱爜鐜囪В鏋愩€丄BR 閫夋嫨銆佸崌闄嶆。楠岃瘉 | 宸插彲鏈湴楠屾敹锛屽睘浜庡熀纭€璁炬柦鑳藉姏 |
| 婊ら暅/澧炲己鍩虹 | 鍐呯疆瑙嗛/闊抽婊ら暅涓庢敞鍐岃〃 | 绠＄嚎宸插湪浠ｇ爜涓瓨鍦紝浣嗙己灏戠ǔ瀹氱敤鎴峰叆鍙?|

## 2. 浣犵幇鍦ㄥ彲浠ユ€庝箞浣跨敤杩欎釜绋嬪簭

### 2.1 鏅€氭挱鏀炬ā寮?

#### 鏂瑰紡 1锛氭挱鏀惧崟涓湰鍦板獟浣撴枃浠?

```powershell
.\build\Debug\modern-video-player.exe .\juren-30s.mp4
```

#### 鏂瑰紡 2锛氫竴娆′紶鍏ュ涓獟浣撴枃浠讹紝浣滀负涓存椂鎾斁鍒楄〃

```powershell
.\build\Debug\modern-video-player.exe .\video1.mp4 .\video2.mkv .\video3.webm
```

#### 鏂瑰紡 3锛氬姞杞芥湰鍦?`m3u8` 鎾斁鍒楄〃鏂囦欢

```powershell
.\build\Debug\modern-video-player.exe .\playlist.m3u8
```

璇存槑锛氳繖閲岀殑 `.m3u8` 鎸?*鏈湴鎾斁鍒楄〃鏂囦欢**锛屼笉鏄畬鏁?HLS 鍦ㄧ嚎鎾斁鍏ュ彛銆?

#### 鏂瑰紡 4锛氭樉寮忔寚瀹氬鎸傚瓧骞?

```powershell
.\build\Debug\modern-video-player.exe .\movie.mkv --subtitle .\movie.srt
```

璇存槑锛氬鏋滀笉鏄惧紡鎸囧畾瀛楀箷锛岀▼搴忚繕浼氬皾璇曡嚜鍔ㄥ姞杞藉悓鍚?`.srt` 鏂囦欢銆?

### 2.2 杩愯鏃惰兘鍔?/ 璇婃柇妯″紡

#### 鏂瑰紡 5锛氭煡鐪嬪綋鍓嶈繍琛屾椂鑳藉姏鐭╅樀

```powershell
.\build\Debug\modern-video-player.exe --capabilities
```

#### 鏂瑰紡 6锛氭帰娴嬪崟涓獟浣撴枃浠?

```powershell
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

#### 鏂瑰紡 7锛氳瘎浼版煇涓洰鏍囨挱鏀惧満鏅槸鍚﹂€傚悎褰撳墠鏈哄櫒

```powershell
.\build\Debug\modern-video-player.exe --evaluate-target 3840 2160 60 6 80
```

### 2.3 閰嶇疆涓庤嚜瀹氫箟鏂瑰紡

閰嶇疆鏂囦欢锛歚config/player_settings.ini`

褰撳墠宸茶惤鍦扮殑涓昏閰嶇疆椤癸細

```ini
player.volume_percent=100
player.playback_speed=1.0
player.audio_delay_ms=0
player.subtitle_delay_ms=0
decoder.prefer_hardware_decode=true
player.resume_last_playlist=true
player.last_playlist_index=0
hotkey.restore_defaults=false
hotkey.play_pause=SPACE
hotkey.seek_backward=LEFT
hotkey.seek_forward=RIGHT
...
```

璇存槑锛?

- `player.*` 鐢ㄤ簬鎸佷箙鍖栬繍琛屽弬鏁帮紱
- `decoder.prefer_hardware_decode` 鎺у埗鏄惁浼樺厛灏濊瘯纭В锛?
- `hotkey.*` 鍙鐩栭粯璁ら敭浣嶏紱
- 濡傞渶鎭㈠榛樿鐑敭锛屽彲灏?`hotkey.restore_defaults=true` 鍚庨噸鍚▼搴忋€?

### 2.4 浜や簰鏂瑰紡锛堥粯璁ょ儹閿?/ 榧犳爣锛?

| 鏂瑰紡 | 褰撳墠浣滅敤 |
|------|----------|
| `Space` | 鎾斁 / 鏆傚仠 |
| `Enter` / `F` / `Alt+Enter` | 鍒囨崲鍏ㄥ睆 |
| `Esc` | 閫€鍑哄叏灞忥紱绐楀彛鎬佷笅閫€鍑烘挱鏀惧櫒 |
| `Q` | 閫€鍑烘挱鏀惧櫒 |
| `Left / Right` | `-5s / +5s` |
| `Ctrl+Left / Ctrl+Right` | `-30s / +30s` |
| `Up / Down / +/-` | 闊抽噺澧炲噺 |
| `M` | 闈欓煶 |
| `[` / `]` / `R` | 闄嶉€?/ 鍗囬€?/ 鎭㈠ `1.0x` |
| `PageUp / PageDown` | 涓婁竴椤?/ 涓嬩竴椤?|
| `Home / End` | 涓婁竴绔?/ 涓嬩竴绔?|
| `A / B / C` | 璁剧疆 A 鐐?/ B 鐐?/ 娓呴櫎 A-B 閲嶅 |
| `S` | 鎴浘 |
| `,` / `.` | 鏆傚仠鎬佸崟甯у悗閫€ / 鍓嶈繘 |
| `J / K` | 瀛楀箷鏃跺欢 `-100ms / +100ms` |
| `Ctrl+J / Ctrl+K` | 闊抽鏃跺欢 `-100ms / +100ms` |
| `1..9` | 璺宠浆鍒?`10%..90%` |
| `V` | 瀛楀箷寮€鍏?|
| 榧犳爣鎷栧姩杩涘害鏉?| seek |
| 榧犳爣鎷栧姩闊抽噺鏉?| 璋冩暣闊抽噺 |

### 2.5 浣滀负寮€鍙?楠屾敹宸ュ叿浣跨敤

褰撳墠绋嬪簭闄や簡鈥滄甯告挱鏀锯€濓紝杩樺彲浠ョ洿鎺ヤ綔涓轰互涓嬪伐鍏蜂娇鐢細

- 鏈湴鑳藉姏鎺㈡祴宸ュ叿锛歚--capabilities`銆乣--probe-file`銆乣--evaluate-target`
- 浜や簰鍔熻兘楠屾敹宸ュ叿锛歚--chapter-nav-check`銆乣--ab-repeat-check`銆乣--frame-step-check`銆乣--delay-adjust-check`銆乣--numeric-seek-check`銆乣--screenshot-check`
- 鎾斁绋冲畾鎬?/ 鎬ц兘楠屾敹宸ュ叿锛歚--performance-log-check`銆乣--1080p60-check`銆乣--4k-playback-check`銆乣--high-bitrate-check`銆乣--long-playback-check`
- 鍚庣 / 骞冲彴楠屾敹宸ュ叿锛歚--renderer-fallback-check`銆乣--windows-backend-check`
- 鎵╁睍涓庢祦濯掍綋鍩虹璁炬柦楠屾敹宸ュ叿锛歚--plugin-check`銆乣--streaming-buffer-check`銆乣--adaptive-bitrate-check`

## 3. 鐩墠鎬庝箞楠岃瘉杩欎釜椤圭洰鐜版湁鐨勫姛鑳?

### 3.1 鏈€鐭獙璇佽矾寰勶紙鎺ㄨ崘锛?

```powershell
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe' `
  build\modern-video-player.sln `
  /t:modern-video-player `
  /p:Configuration=Debug `
  /p:Platform=x64

powershell -ExecutionPolicy Bypass -File .\tools\download_test_samples.ps1
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1
.\build\Debug\modern-video-player.exe --capabilities
.\build\Debug\modern-video-player.exe --probe-file .\juren-30s.mp4 --json
```

## 3.2 鍔熻兘鍒嗛」楠岃瘉娓呭崟

涓嬭〃鍒楀嚭鈥滃綋鍓嶄富瑕佸姛鑳?-> 鎺ㄨ崘楠岃瘉鍛戒护 -> 瀵瑰簲鎶ュ憡鈥濈殑鏄犲皠銆傚叿浣撴牱鏈矾寰勩€佺鍙ｅ拰杈撳嚭鎽樿锛屼紭鍏堜互瀵瑰簲鎶ュ憡鏂囦欢涓哄噯銆?

| 鍔熻兘 | 鎺ㄨ崘楠岃瘉鍛戒护 | 瀵瑰簲鎶ュ憡 |
|------|--------------|----------|
| 澶栨寕瀛楀箷鍔犺浇涓庡悓姝?| `--subtitle-sync-check <subtitle.srt>` | `docs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md` |
| 鎾斁鍒楄〃杩炵画鎾斁 | `--playlist-flow-check <media1> <media2> ...` | `docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md` |
| 璁剧疆鎸佷箙鍖?| `--settings-persistence-check [settings_file]` | `docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md` |
| 娓叉煋澶辫触鍥為€€ | `--renderer-fallback-check <media_file>` | `docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md` |
| Windows 鍚庣涓庣‖瑙ｅ洖閫€ | `--windows-backend-check <media_file>` | `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md` |
| 绔犺妭瀵艰埅 | `--chapter-nav-check <media_file>` | `docs/reports/CHAPTER_NAV_LOCAL_CHECK.md` |
| A-B Repeat | `--ab-repeat-check <media_file>` | `docs/reports/AB_REPEAT_LOCAL_CHECK.md` |
| 甯ф杩?| `--frame-step-check <media_file>` | `docs/reports/FRAME_STEP_LOCAL_CHECK.md` |
| 闊抽 / 瀛楀箷鏃跺欢璋冭妭 | `--delay-adjust-check <media_file> <subtitle.srt>` | `docs/reports/DELAY_ADJUST_LOCAL_CHECK.md` |
| `1..9` 鐧惧垎姣旇烦杞?| `--numeric-seek-check <media_file>` | `docs/reports/NUMERIC_SEEK_LOCAL_CHECK.md` |
| 鎴浘 | `--screenshot-check <media_file>` | `docs/reports/SCREENSHOT_LOCAL_CHECK.md` |
| 鎾斁鎬ц兘鏃ュ織 | `--performance-log-check <media_file> [sample_ms]` | `docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md` |
| `1080p60` 绋冲畾鎬?| `--1080p60-check <media_file> [sample_ms]` | `docs/reports/1080P60_STABILITY_LOCAL_CHECK.md` |
| `4K` 鎾斁涓庨檷绾?| `--4k-playback-check <media_file> [sample_ms]` | `docs/reports/4K_PLAYBACK_LOCAL_CHECK.md` |
| `>80Mbps` 楂樼爜鐜?| `--high-bitrate-check <media_file> [sample_ms]` | `docs/reports/HIGH_BITRATE_LOCAL_CHECK.md` |
| 闀挎椂鎾斁绋冲畾鎬?| `--long-playback-check <media_file> [sample_ms]` | `docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md` |
| 鎻掍欢绯荤粺 | `--plugin-check [plugin_dir_or_file]` | `docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md` |
| 娴佸獟浣?HTTP 鍒嗙墖涓庣紦鍐?| `--streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]` | `docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md` |
| HLS/DASH 鑷€傚簲鐮佺巼 | `--adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]` | `docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md` |
| 涓诲姏鏍煎紡鐭╅樀 | `tools/format_regression/run_format_regression.ps1` | `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md` |

## 3.3 杈撳嚭涓庣棔杩逛綅缃?

- 鎴浘杈撳嚭锛歚screenshots/`
- 鎸佷箙鍖栭厤缃細`config/player_settings.ini`
- 鏈湴楠屾敹鎶ュ憡锛歚docs/reports/`
- 鏍煎紡鍥炲綊鑴氭湰涓庢搷浣滄墜鍐岋細`docs/workflows/FORMAT_REGRESSION.md`銆乣docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md`
- 姣忓懆鏀舵暃鐣欑棔锛歚.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`銆乣.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`

## 4. 褰撳墠杈圭晫涓庢湭瀹屾垚鍔熻兘

浠ヤ笅鑳藉姏**涓嶈璇涓哄凡缁忔槸瀹屾暣缁堢鐢ㄦ埛鍔熻兘**锛?

- 娴佸獟浣擄細褰撳墠宸茬粡鍏峰鐪熷疄 HTTP 涓嬭浇銆丠LS/DASH 娓呭崟瑙ｆ瀽銆佸垎鐗囩紦鍐蹭笌 ABR 楠岃瘉锛屼絾**鐪熸鎾斁閾捐矾鎺ュ叆浠嶅緟琛ラ綈**銆?
- 鎻掍欢绯荤粺锛氬綋鍓嶅凡缁忓叿澶?`DLL` 鍔ㄦ€佸姞杞藉拰鐢熷懡鍛ㄦ湡绠＄悊锛屼絾杩樻病鏈夌敤鎴风骇閰嶇疆鐣岄潰銆佹彃浠跺垎鍙戜笌闅旂绛栫暐銆?
- 鐨偆绯荤粺锛氬綋鍓嶄粛鏄鏋?/ 鏈帴鍏ョ姸鎬併€?
- 瀛楀箷绯荤粺锛氬綋鍓嶅彲鐢ㄧ殑鏄鎸?`SRT`锛涘唴宓屽瓧骞曘€佸瓧骞曡建鍒囨崲銆乣ASS/SSA` 绛夎兘鍔涘皻鏈ˉ榻愩€?
- 娓叉煋鍚庣锛歚SDL` 涓?`D3D11` 宸叉湁鍙敤璺緞锛宍OpenGL` 浠嶄笉鏄綋鍓嶄富璺緞銆?
- 婊ら暅澧炲己锛氬唴缃棰?闊抽澧炲己鍩虹璁炬柦宸插瓨鍦紝浣嗘殏鏈舰鎴愮ǔ瀹氥€佸彲瑙併€佸彲璋冪殑鐢ㄦ埛鍔熻兘鍏ュ彛銆?

## 5. 鎺ㄨ崘闃呰

- 鍏ュ彛绱㈠紩锛歚docs/README.md`
- 鍥炲綊鎵嬪唽锛歚docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md`
- 鏍煎紡鍥炲綊璇存槑锛歚docs/workflows/FORMAT_REGRESSION.md`
- 褰撳墠宸窛璇勪及锛歚docs/analysis/MPC_HC_GAP_ANALYSIS.md`
- 鏍规枃妗ｆ瑙堬細`README.md`




