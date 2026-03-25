# 鏂囨。绱㈠紩

- 2026-03-24 新增：[`reports/OPENGL_RENDERER_LOCAL_CHECK.md`](./reports/OPENGL_RENDERER_LOCAL_CHECK.md) 记录 `OpenGL` M0 渲染链路的本地验收结果。

`docs/` 鐩綍宸叉寜鐢ㄩ€旈噸鏂板垎绫伙紝寤鸿浼樺厛浠庢湰椤佃繘鍏ユ煡鎵俱€?姣忎釜鍒嗙被鐩綍涓嬩篃琛ュ厖浜嗕竴涓氨杩戠殑 `README.md`锛屾柟渚垮湪 IDE 渚ц竟鏍忔寜鏂囦欢澶规祻瑙堛€?
## 鐩綍鍒嗙被

| 鐩綍 | 閫傚悎鏌ョ湅浠€涔?| 浠ｈ〃鏂囨。 |
|------|--------------|----------|
| [`guides/README.md`](./guides/README.md) | 鐜鎼缓銆佸姛鑳戒娇鐢ㄣ€佸揩閫熶笂鎵?| `WINDOWS_SETUP.md`銆乣PLAYER_FEATURES_USAGE_VALIDATION.md` |
| [`design/README.md`](./design/README.md) | 褰撳墠鏋舵瀯銆佸巻鍙叉灦鏋勩€佹ā鍧楄璁¤崏妗?| `ARCHITECTURE_REFACTOR_2026-03-06.md`銆乣FILTERS.md` |
| [`analysis/README.md`](./analysis/README.md) | 宸窛璇勪及銆侀棶棰樺垎鏋愩€佹枃妗ｅ贰妫€ | `MPC_HC_GAP_ANALYSIS.md`銆乣DOC_AUDIT_2026-03-08.md` |
| [`reference/README.md`](./reference/README.md) | 澶栭儴鑳藉姏鍙傝€冦€佸涔犺祫鏂欍€佹棩蹇楀簱琛ュ厖璇存槑 | `PLAYER_REFERENCE_AND_FFMPEG_NOTES.md`銆乣OPEN_SOURCE_PLAYER_LEARNING_PATH.md` |
| [`plans/README.md`](./plans/README.md) | 璺嚎鍥俱€侀樁娈佃鍒掋€乀ODO 娓呭崟 | `CROSS_PLATFORM_EVOLUTION_ROADMAP.md`銆乣PHASE1_CROSS_PLATFORM_TODO.md` |
| [`workflows/README.md`](./workflows/README.md) | 鍥炲綊鎵嬪唽銆佸伐浣滄祦銆侀槄璇绘竻鍗曘€丄I 鎻愰棶鍓ф湰 | `REGRESSION_OPERATION_PLAYBOOK.md`銆乣WORKFLOW.md` |
| [`records/README.md`](./records/README.md) | 鐗堟湰璁板綍銆佷慨澶嶈褰曘€佸紑鍙戞棩蹇?| `VERSION.md`銆乣CHANGELOG.md`銆乣DEVELOP_LOG.md` |
| [`reports/README.md`](./reports/README.md) | 鏈湴楠屾敹涓庡姛鑳介獙璇佺粨鏋?| `*_LOCAL_CHECK.md` |

## 鎸夊満鏅煡鎵?
- **鍏堣窇璧锋潵**锛歚guides/WINDOWS_SETUP.md`銆乣guides/PLAYER_FEATURES_USAGE_VALIDATION.md`
- **鐞嗚В褰撳墠涓婚摼**锛歚design/ARCHITECTURE_REFACTOR_2026-03-06.md`銆乣design/ARCHITECTURE.md`
- **鐪嬫ā鍧楄璁?*锛歚design/FILTERS.md`銆乣design/LOGGING.md`銆乣design/PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md`
- **鐪嬭鍒掍笌璺嚎**锛歚plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md`銆乣plans/MPC_HC_ITERATION_PLAN.md`
- **鐪嬪樊璺濆拰鍒嗘瀽**锛歚analysis/MPC_HC_GAP_ANALYSIS.md`銆乣analysis/video-stream-index-fix.md`
- **璺戝洖褰?鏌ユ祦绋?*锛歚workflows/REGRESSION_OPERATION_PLAYBOOK.md`銆乣workflows/FORMAT_REGRESSION.md`銆乣workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`
- **鏌ュ巻鍙茶褰?*锛歚records/CHANGELOG.md`銆乣records/VERSION.md`銆乣records/DEVELOP_LOG.md`
- **鐪嬮獙鏀剁粨鏋?*锛歚reports/` 鐩綍涓嬪悇椤?`*_LOCAL_CHECK.md`

## 鎸夌洰褰曟祻瑙?
### `guides/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [WINDOWS_SETUP.md](./guides/WINDOWS_SETUP.md) | Windows 鐜閰嶇疆涓庢瀯寤哄叆鍙?|
| [PLAYER_FEATURES_USAGE_VALIDATION.md](./guides/PLAYER_FEATURES_USAGE_VALIDATION.md) | 褰撳墠鍔熻兘銆佷娇鐢ㄦ柟寮忎笌楠岃瘉鍏ュ彛鎬昏 |
| [IMPLEMENTATION.md](./guides/IMPLEMENTATION.md) | 鏃╂湡鍘熷瀷鐨勪粠闆跺疄鐜版暀绋嬶紙鍘嗗彶瀹炵幇鍩虹嚎锛?|

### `design/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [ARCHITECTURE_REFACTOR_2026-03-06.md](./design/ARCHITECTURE_REFACTOR_2026-03-06.md) | 褰撳墠涓婚摼閲嶆瀯璇存槑 |
| [ARCHITECTURE.md](./design/ARCHITECTURE.md) | 鍘嗗彶鏋舵瀯鍩虹嚎涓庤璁¤儗鏅?|
| [FILTERS.md](./design/FILTERS.md) | 婊ら暅鎺ュ彛銆佸唴缃护闀滀笌褰撳墠鎺ュ叆鏂瑰紡 |
| [LOGGING.md](./design/LOGGING.md) | 鏃ュ織绯荤粺璇存槑 |
| [PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md](./design/PLAYERCORE_PLAYBACK_STATE_MACHINE_DRAFT.md) | 鎾斁鐘舵€佹満璁捐鑽夋 |
| [WINDOWS_PLAYER_DEFAULT_STRATEGY_DRAFT.md](./design/WINDOWS_PLAYER_DEFAULT_STRATEGY_DRAFT.md) | Windows 榛樿绛栫暐寤鸿绋?|

### `analysis/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [MPC_HC_GAP_ANALYSIS.md](./analysis/MPC_HC_GAP_ANALYSIS.md) | 涓?MPC-HC 鐨勫姛鑳藉樊璺濊瘎浼?|
| [DOC_AUDIT_2026-03-08.md](./analysis/DOC_AUDIT_2026-03-08.md) | 鏈疆鏂囨。鍙ｅ緞宸℃涓庢敹鏁涚粨鏋滄€昏〃 |
| [video-stream-index-fix.md](./analysis/video-stream-index-fix.md) | 鏃╂湡娴佺储寮曢棶棰樼殑鍘嗗彶鍒嗘瀽褰掓。 |

### `reference/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [PLAYER_REFERENCE_AND_FFMPEG_NOTES.md](./reference/PLAYER_REFERENCE_AND_FFMPEG_NOTES.md) | 鎾斁鑳藉姏瀹炵幇鍙傝€冿紙鏍煎紡 / 楂樺垎楂樺抚 / 澶氶煶閬擄級 |
| [OPEN_SOURCE_PLAYER_LEARNING_PATH.md](./reference/OPEN_SOURCE_PLAYER_LEARNING_PATH.md) | 寮€婧愭挱鏀惧櫒鏁村悎瀛︿範璺緞 |
| [QUILL_LOGGING.md](./reference/QUILL_LOGGING.md) | Quill 鏃ュ織搴撻泦鎴愯ˉ鍏呰鏄?|

### `plans/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [CROSS_PLATFORM_EVOLUTION_ROADMAP.md](./plans/CROSS_PLATFORM_EVOLUTION_ROADMAP.md) | 浠?Windows 婕旇繘鍒拌法骞冲彴鏋舵瀯鐨勮矾绾垮浘 |
| [CROSS_PLATFORM_REFACTOR_TASKLIST.md](./plans/CROSS_PLATFORM_REFACTOR_TASKLIST.md) | 褰撳墠浠撳簱璺ㄥ钩鍙版敼閫犱换鍔℃竻鍗?|
| [MPC_HC_ITERATION_PLAN.md](./plans/MPC_HC_ITERATION_PLAN.md) | MPC-HC 瀵归綈杩唬璁″垝锛堝揩鐓э級 |
| [PHASE1_CROSS_PLATFORM_TODO.md](./plans/PHASE1_CROSS_PLATFORM_TODO.md) | Phase 1 閫愭枃浠?TODO 瀹炴柦鍗?|

### `workflows/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [WORKFLOW.md](./workflows/WORKFLOW.md) | 宸ヤ綔娴佺▼瑙勮寖涓庢枃妗ｅ悓姝ヨ姹?|
| [REGRESSION_OPERATION_PLAYBOOK.md](./workflows/REGRESSION_OPERATION_PLAYBOOK.md) | 鏍锋湰鍑嗗涓庡洖褰掓搷浣滄墜鍐?|
| [FORMAT_REGRESSION.md](./workflows/FORMAT_REGRESSION.md) | 鏍煎紡鍥炲綊鑴氭湰涓庢姤鍛婅鏄?|
| [WEEKLY_CONVERGENCE_PLAYBOOK.md](./workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md) | 鍛ㄨ妭濂忎笌鍛ㄤ簲鏀舵暃鎵嬪唽 |
| [SOURCE_FILE_READING_CHECKLIST.md](./workflows/SOURCE_FILE_READING_CHECKLIST.md) | 鎸夋枃浠跺悕灞曞紑鐨勯槄璇绘竻鍗?|
| [MODULE_BASED_ANALYSIS_SCRIPTS.md](./workflows/MODULE_BASED_ANALYSIS_SCRIPTS.md) | 鎸夋ā鍧楃殑杩炵画鎻愰棶鍓ф湰 |
| [AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md](./workflows/AI_SOURCE_ANALYSIS_PROMPT_PLAYBOOK.md) | AI 婧愮爜鍒嗘瀽鎻愰棶鏂规 |
| [AI_FUNCTION_LEVEL_PROMPT_CHECKLIST.md](./workflows/AI_FUNCTION_LEVEL_PROMPT_CHECKLIST.md) | AI 閫愬嚱鏁版彁闂ā鏉挎竻鍗?|

### `records/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [VERSION.md](./records/VERSION.md) | 椤圭洰鐗堟湰璁板綍涓庝緷璧栬鏄?|
| [CHANGELOG.md](./records/CHANGELOG.md) | 闂淇璁板綍 |
| [DEVELOP_LOG.md](./records/DEVELOP_LOG.md) | 寮€鍙戞棩蹇?|

### `reports/`

| 鏂囨。 | 璇存槑 |
|------|------|
| [V1_0_0_RC1_RELEASE_NOTES.md](./reports/V1_0_0_RC1_RELEASE_NOTES.md) | 1.0.0-rc1 鍙洿鎺ョ矘璐村埌 Release 椤电殑姝ｆ枃 |
| [V1_0_0_RC1_RELEASE_READINESS.md](./reports/V1_0_0_RC1_RELEASE_READINESS.md) | 1.0.0-rc1 鍙戝竷鍑嗗銆佸凡鐭ラ棶棰樹笌鍙戝竷璇存槑 |
| [DELAY_ADJUST_LOCAL_CHECK.md](./reports/DELAY_ADJUST_LOCAL_CHECK.md) | 闊宠建 / 瀛楀箷寤惰繜璋冭妭鏈湴楠屾敹璁板綍 |
| [NUMERIC_SEEK_LOCAL_CHECK.md](./reports/NUMERIC_SEEK_LOCAL_CHECK.md) | 鏁板瓧鐑敭 `1..9` 璺宠浆鏈湴楠屾敹璁板綍 |
| [PERFORMANCE_LOG_LOCAL_CHECK.md](./reports/PERFORMANCE_LOG_LOCAL_CHECK.md) | 鎾斁鎬ц兘鏃ュ織鏈湴楠屾敹璁板綍 |
| [1080P60_STABILITY_LOCAL_CHECK.md](./reports/1080P60_STABILITY_LOCAL_CHECK.md) | 1080p60 绋冲畾鎾斁鏈湴楠屾敹璁板綍 |
| [4K_PLAYBACK_LOCAL_CHECK.md](./reports/4K_PLAYBACK_LOCAL_CHECK.md) | 4K 鎾斁涓庨檷绾ф湰鍦伴獙鏀惰褰?|
| [HIGH_BITRATE_LOCAL_CHECK.md](./reports/HIGH_BITRATE_LOCAL_CHECK.md) | >80Mbps 楂樼爜鐜囨牱鏈湰鍦伴獙鏀惰褰?|
| [LONG_PLAYBACK_LOCAL_CHECK.md](./reports/LONG_PLAYBACK_LOCAL_CHECK.md) | 闀挎椂鎾斁绋冲畾鎬ф湰鍦伴獙鏀惰褰?|
| [PLUGIN_SYSTEM_LOCAL_CHECK.md](./reports/PLUGIN_SYSTEM_LOCAL_CHECK.md) | 鎻掍欢绯荤粺鏈湴楠屾敹璁板綍 |
| [STREAMING_BUFFER_LOCAL_CHECK.md](./reports/STREAMING_BUFFER_LOCAL_CHECK.md) | 娴佸獟浣?HTTP 鍒嗙墖涓庣紦鍐叉湰鍦伴獙鏀惰褰?|
| [ADAPTIVE_BITRATE_LOCAL_CHECK.md](./reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md) | HLS / DASH 鑷€傚簲鐮佺巼鏈湴楠屾敹璁板綍 |

## 鍏宠仈鍏ュ彛

- 椤圭洰鎬昏锛歔../README.md](../README.md)
- 鍥炲綊鏍锋湰鐩綍锛歔../samples/README.md](../samples/README.md)
- 鍛ㄦ姤妯℃澘锛歔../.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md](../.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md)
- 鍛ㄧ湅鏉匡細[../.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md](../.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md)
- 鏍煎紡鍥炲綊宸ヤ綔娴侊細[../.github/workflows/format-regression.yml](../.github/workflows/format-regression.yml)
- 杩愯鍙傛暟绀轰緥锛歔../config/player_settings.ini](../config/player_settings.ini)

## 鏈鏁寸悊

- 2026-03-10锛氭寜鐢ㄩ€斿皢鏂囨。鎷嗗垎涓?`guides`銆乣design`銆乣analysis`銆乣reference`銆乣plans`銆乣workflows`銆乣records`銆乣reports` 鍏被銆?- 鎵€鏈変粨搴撳唴涓昏鏂囨。鍏ュ彛宸插悓姝ュ埌鏂拌矾寰勶紝渚夸簬鍦?IDE 涓寜鐩綍蹇€熸祻瑙堛€?
