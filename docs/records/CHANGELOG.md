# CHANGELOG

## 索引说明（2026-03-26 编码清理批次）

- 本轮仅清理 `records/readme` 索引范围，不批量改写历史正文。
- 最近收口条目位于文件顶部（`Issue 183` 到 `Issue 122`）。
- 历史段落若出现旧编码乱码，将在后续专题批次逐步处理。

## Issue 184: PlayerCore worker thread consolidation

**Date**: 2026-04-10

### Problem Description
- `PlayerCore` kept duplicated `std::thread` and running-flag pairs for `demux` and `audio consumer`.
- Start/stop/reap/join logic was repeated across both workers.
- Repository still contained an unused `DecoderThread` implementation that no longer matched the current playback mainline.

### Root Cause Analysis
- Thread ownership concerns were not extracted into a current mainline helper.
- Existing `DecoderThread` was a prototype-era abstraction and was not actually used by `PlayerCore`.
- As `PlayerCore` evolved, worker-loop policy stayed explicit but lifecycle plumbing remained duplicated.

### Solution
- Added `core::WorkerThread` for shared worker lifecycle ownership.
- Migrated `PlayerCore` `demux` and `audio consumer` workers to the new helper.
- Removed the unused legacy `DecoderThread` files.
- Updated planner/index/docs so this refactor round is traceable end-to-end.

### Validation
- Build:
  - `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- Runtime:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
  - key lines:
    - `performance-log-check.open_ok=true`
    - `performance-log-check.entered_playback_loop=true`
    - `performance-log-check.audio_output_initialized=true`
    - `performance-log-check.result=PASS`

### Modified Files
- `CMakeLists.txt`
- `include/core/player_core.h`
- `include/core/worker_thread.h`
- `src/core/player_core.cpp`
- `src/core/worker_thread.cpp`
- `docs/analysis/PLAYERCORE_DAY117_WORKER_THREAD_CONSOLIDATION.md`
- `docs/plans/PLAYERCORE_WORKER_THREAD_CONSOLIDATION_PLAN_2026-04-10.md`
- `docs/plans/README.md`
- `docs/reports/PLAYERCORE_WORKER_THREAD_CONSOLIDATION_LOCAL_CHECK.md`
- `docs/reports/README.md`
- `docs/analysis/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 183: Vulkan chain VK-053 Windows auto optional sdk-missing canary

**Date**: 2026-03-28

### Problem Description
- Auto-policy branch coverage had no explicit canary for sdk-missing boundary with runtime probe present.

### Root Cause Analysis
- `strict_mode_auto_prerequisites_met` should stay `false` when SDK signal is absent.
- Without dedicated canary, accidental future strict escalation under `sdk=0` could slip through.

### Solution
- Added canary:
  - `tools/run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1`
- Scenario:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=0`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Assertions:
  - `gate_mode=optional`
  - `gate_strict_mode_effective=false`
  - `gate_strict_mode_auto_prerequisites_met=false`
  - `gate_strict_mode_auto_runtime_probe_any_available=true`
  - `gate_strict_mode_auto_runtime_probe_source=native`
  - `gate_result=SKIPPED`
- Integrated into:
  - `tools/run_windows_ci_gate.ps1`
  - with Step Summary section:
    - `Windows Vulkan Gate Auto Optional SDK-Missing Canary`.

### Validation
- SDK-missing canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1` -> PASS
  - key lines:
    - `windows-vulkan-auto-optional-sdk-missing-canary.gate_mode=optional`
    - `windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_prerequisites_met=false`
    - `windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_any_available=true`
    - `windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_source=native`
    - `windows-vulkan-auto-optional-sdk-missing-canary.result=PASS`

### Modified Files
- `tools/run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1`
- `tools/run_windows_ci_gate.ps1`
- `docs/analysis/PLAYERCORE_DAY116_VK053_WINDOWS_VULKAN_AUTO_OPTIONAL_SDK_MISSING_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_SDK_MISSING_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_SDK_MISSING_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_SDK_MISSING_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 182: Vulkan chain VK-052 Windows auto strict dual-probe canary

**Date**: 2026-03-28

### Problem Description
- Auto-policy canary coverage had `none` / `native` / `swiftshader` branches.
- Combined dual-probe branch (`native+swiftshader`) lacked dedicated canary.

### Root Cause Analysis
- `strict_mode_auto_runtime_probe_source` supports `native+swiftshader`.
- No canary asserted this combined-source classification under auto strict promotion.

### Solution
- Added canary:
  - `tools/run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1`
- Scenario:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=1`
- Assertions:
  - `gate_mode=strict`
  - `gate_strict_mode_effective=true`
  - `gate_strict_mode_auto_runtime_probe_source=native+swiftshader`
  - `gate_result=PASS`
- Integrated into:
  - `tools/run_windows_ci_gate.ps1`
  - with Step Summary section:
    - `Windows Vulkan Gate Auto Strict Dual-Probe Canary`.

### Validation
- Dual-probe canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1` -> PASS
  - key lines:
    - `windows-vulkan-auto-strict-dual-probe-canary.gate_mode=strict`
    - `windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_runtime_probe_source=native+swiftshader`
    - `windows-vulkan-auto-strict-dual-probe-canary.result=PASS`

### Modified Files
- `tools/run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1`
- `tools/run_windows_ci_gate.ps1`
- `docs/analysis/PLAYERCORE_DAY115_VK052_WINDOWS_VULKAN_AUTO_STRICT_DUAL_PROBE_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_DUAL_PROBE_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_DUAL_PROBE_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_DUAL_PROBE_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 181: Vulkan chain VK-051 Windows auto optional no-probe canary

**Date**: 2026-03-28

### Problem Description
- Auto strict promotion branches (`native` / `swiftshader`) were covered.
- Auto no-probe downgrade branch still lacked dedicated canary coverage.

### Root Cause Analysis
- Existing optional-skip canary was policy `off` path, not `auto` downgrade path.
- No direct assertion existed for:
  - `strict_mode_auto_prerequisites_met=false`
  - `strict_mode_auto_runtime_probe_source=none`
  under `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`.

### Solution
- Added new canary:
  - `tools/run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1`
- Scenario:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=0`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Assertions:
  - `gate_mode=optional`
  - `gate_strict_mode_effective=false`
  - `gate_strict_mode_auto_prerequisites_met=false`
  - `gate_strict_mode_auto_runtime_probe_source=none`
  - `gate_result=SKIPPED`
- Integrated into:
  - `tools/run_windows_ci_gate.ps1`
  - with dedicated Step Summary section:
    - `Windows Vulkan Gate Auto Optional No-Probe Canary`

### Validation
- Auto no-probe canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1` -> PASS
  - key lines:
    - `windows-vulkan-auto-optional-no-probe-canary.gate_mode=optional`
    - `windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_prerequisites_met=false`
    - `windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_runtime_probe_source=none`
    - `windows-vulkan-auto-optional-no-probe-canary.result=PASS`
- Regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1` -> PASS
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1` -> PASS

### Modified Files
- `tools/run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1`
- `tools/run_windows_ci_gate.ps1`
- `docs/analysis/PLAYERCORE_DAY114_VK051_WINDOWS_VULKAN_AUTO_OPTIONAL_NO_PROBE_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_NO_PROBE_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_NO_PROBE_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_OPTIONAL_NO_PROBE_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 180: Vulkan chain VK-050 Windows auto strict native runtime probe canary

**Date**: 2026-03-28

### Problem Description
- `VK-049` covered SwiftShader branch promotion for auto strict mode.
- But native runtime-probe auto strict branch still lacked dedicated canary coverage.

### Root Cause Analysis
- Existing pass-contract canary validates strict PASS path, but it is CLI strict (`-RequireVulkanAvailable`) not auto strict.
- Native-probe source classification (`strict_mode_auto_runtime_probe_source=native`) had no direct canary assertion.

### Solution
- Added canary:
  - `tools/run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1`
- Canary scenario:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
  - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE=0`
- Assertions:
  - `gate_mode=strict`
  - `gate_strict_mode_effective=true`
  - `gate_strict_mode_auto_runtime_probe_source=native`
  - `result=PASS`
- Integrated into:
  - `tools/run_windows_ci_gate.ps1`
  - with dedicated Step Summary section.

### Validation
- Native auto strict canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1` -> PASS
  - key lines:
    - `windows-vulkan-auto-strict-native-probe-canary.gate_mode=strict`
    - `windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_runtime_probe_source=native`
    - `windows-vulkan-auto-strict-native-probe-canary.result=PASS`
- Regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1` -> PASS
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1` -> PASS

### Modified Files
- `tools/run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1`
- `tools/run_windows_ci_gate.ps1`
- `docs/analysis/PLAYERCORE_DAY113_VK050_WINDOWS_VULKAN_AUTO_STRICT_NATIVE_RUNTIME_PROBE_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_NATIVE_RUNTIME_PROBE_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_NATIVE_RUNTIME_PROBE_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_NATIVE_RUNTIME_PROBE_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 179: Vulkan chain VK-049 Windows auto strict SwiftShader runtime probe promotion

**Date**: 2026-03-28

### Problem Description
- Windows Vulkan gate already captured SwiftShader runtime-probe signals.
- But `auto` strict mode still only depended on native runtime probe:
  - `sdk && native_runtime_probe`
- This left CI strict-mode promotion disconnected from SwiftShader-only runtime-available cases.

### Root Cause Analysis
- `run_windows_vulkan_checks.ps1` strict auto prerequisite did not include `MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE`.
- No dedicated canary asserted the branch:
  - `auto + sdk=1 + native_probe=0 + swiftshader_probe=1`.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - auto strict prerequisite now:
    - `sdk && (native_runtime_probe || swiftshader_runtime_probe)`
  - updated summary basis:
    - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe_or_swiftshader_probe`
  - added summary fields:
    - `windows-vulkan-check.strict_mode_auto_runtime_probe_any_available`
    - `windows-vulkan-check.strict_mode_auto_runtime_probe_source`
- Added canary:
  - `tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1`
- Integrated canary and summary table rows into:
  - `tools/run_windows_ci_gate.ps1`

### Validation
- Static scan:
  - `rg -n "sdk_and_runtime_probe_or_swiftshader_probe|strict_mode_auto_runtime_probe_any_available|strict_mode_auto_runtime_probe_source|auto_strict_swiftshader_probe_canary" tools/run_windows_vulkan_checks.ps1 tools/run_windows_ci_gate.ps1 tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1` -> PASS
- New canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1` -> PASS
  - key lines:
    - `gate_mode=strict`
    - `gate_strict_mode_auto_runtime_probe_source=swiftshader`
    - `result=PASS`
- Regression canaries:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1` -> PASS
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1` -> PASS

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `tools/run_windows_ci_gate.ps1`
- `tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY112_VK049_WINDOWS_VULKAN_AUTO_STRICT_SWIFTSHADER_RUNTIME_PROBE_PROMOTION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_SWIFTSHADER_RUNTIME_PROBE_PROMOTION_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_SWIFTSHADER_RUNTIME_PROBE_PROMOTION_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_SWIFTSHADER_RUNTIME_PROBE_PROMOTION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 178: Forced FailSession deferred release and headless logging override

**Date**: 2026-03-28

### Problem Description
- Format-regression workflow timeout containment was already landed, but the workflow still failed at step 2:
  - `--forced-failsession-check`
- GitHub Actions log stalled after:
  - `PlayerCore applied stop-completion side effects ...`
- Local headless validation reproduced the same recovery boundary and also hit Quill console-handle exit failures.

### Root Cause Analysis
- `FailureRecoveryPolicy::FailSession` executed full stop-completion and session-release inline on worker threads.
- That recovery path crossed unsafe ownership boundaries around scheduler stop, renderer shutdown, decoder release, and demuxer close.
- The forced FailSession canary also assumed facade playback-state callbacks would settle before immediate injected failure, which was not guaranteed.

### Solution
- Added deferred fail-session state to `PlayerCore`.
- Refactored `handleRuntimeFailure(FailSession)` to:
  - arm deferred fail-session metadata
  - request stop
  - publish stopped playback state
- Moved final stop-completion follow-up into `serviceDeferredStop()`:
  - session release
  - `Stopped / Idle / Failed` closure
  - final error emission
- Hardened `runForcedFailSessionCheck()` so immediate injected failure still counts as entering the playback path.
- Added `MVP_DISABLE_QUILL_LOGGING=1` env override for local/headless validation without changing default logging behavior.

### Validation
- Build:
  - `cmake --build build --config Debug --target modern-video-player` -> PASS
- Forced FailSession:
  - `$env:MVP_DISABLE_QUILL_LOGGING='1'; $env:SDL_AUDIODRIVER='dummy'; .\build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200` -> PASS
  - key lines:
    - `forced-failsession-check.entered_playback_loop=true`
    - `forced-failsession-check.fail_session_observed=true`
    - `forced-failsession-check.stopped_after_failure=true`
    - `forced-failsession-check.result=PASS`
- Aggregate checks:
  - `$env:MVP_DISABLE_QUILL_LOGGING='1'; $env:SDL_AUDIODRIVER='dummy'; powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath 'build/Debug/modern-video-player.exe' -ProbeFile 'juren-30s.mp4' -SamplesFile 'tools/format_regression/format_samples_ci.csv' -RegressionOutputFile 'build/FORMAT_REGRESSION_CI_LOCAL_TEST.md' -ProbeTimeoutSec 120 -ForcedFailSessionTimeoutSec 240 -RegressionTimeoutSec 900` -> PASS
  - key lines:
    - `Probe exit code: 0`
    - `Forced FailSession exit code: 0`
    - `Regression exit code: 0`

### Modified Files
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/logger.cpp`
- `docs/analysis/PLAYERCORE_DAY111_FORCED_FAILSESSION_DEFERRED_RELEASE_AND_HEADLESS_LOGGING_OVERRIDE.md`
- `docs/design/FORCED_FAILSESSION_DEFERRED_RELEASE_AND_HEADLESS_LOGGING_OVERRIDE_DESIGN_2026-03-28.md`
- `docs/plans/FORCED_FAILSESSION_DEFERRED_RELEASE_AND_HEADLESS_LOGGING_OVERRIDE_PLAN_2026-03-28.md`
- `docs/reports/FORCED_FAILSESSION_DEFERRED_RELEASE_AND_HEADLESS_LOGGING_OVERRIDE_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 177: Vulkan chain VK-048 Windows PASS-contract availability detail assertion hardening

**Date**: 2026-03-28

### Problem Description
- PASS-contract canary already asserts strict PASS behavior and playback contract signals.
- But it did not assert `windows-vulkan-check.vulkan_availability_failure_detail`, so success-path classification drift risk remained.

### Root Cause Analysis
- Availability-detail assertion hardening was added incrementally to failure-branch canaries first.
- PASS-contract canary remained on legacy assertion set and lacked parity checks for `vulkan_availability_failure_detail`.

### Solution
- Updated `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - require value equals `none`
  - export `windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail`
- Updated `.github/workflows/cross-platform-gate.yml` PASS-contract canary Step Summary rows:
  - added `gate_vulkan_availability_failure_detail`.
- Synced docs/records/index chain for `VK-048`.

### Validation
- Build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk048-baseline.env"` -> PASS
- PASS-contract canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk048.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk048.env"` -> PASS
  - key lines:
    - `windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail=none`
    - `windows-vulkan-pass-contract-canary.result=PASS`
- Full Windows Vulkan canary matrix (`vk048` batch): PASS
  - key line: `ALL_VK048_CHECKS_PASS`
- Static scan:
  - `rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_pass_contract_canary|Windows Vulkan Gate PASS Contract Canary|vulkanPassCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY110_VK048_WINDOWS_VULKAN_PASS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 176: Vulkan chain VK-047 Windows unsupported-platform availability detail assertion hardening

**Date**: 2026-03-28

### Problem Description
- Unsupported-platform expected-fail canary already asserted:
  - `gate_result=FAIL`
  - `gate_failure_reason=unsupported-platform`
  - `gate_playback_check_executed=false`
- But it did not assert `windows-vulkan-check.vulkan_availability_failure_detail`, leaving unsupported-branch detail drift risk.

### Root Cause Analysis
- Availability-detail assertions were hardened in recent canary rounds, but unsupported-platform canary still had legacy assertion scope and lacked parity.
- Workflow Step Summary for this canary also did not expose availability-detail value.

### Solution
- Updated `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - require value equals `unsupported-platform`
  - export `windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail`
- Updated `.github/workflows/cross-platform-gate.yml` unsupported-platform canary Step Summary rows:
  - added `gate_vulkan_availability_failure_detail`.
- Synced docs/records/index chain for `VK-047`.

### Validation
- Build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk047-baseline.env"` -> PASS
- Unsupported-platform canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk047.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk047.env"` -> PASS
  - key lines:
    - `windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail=unsupported-platform`
    - `windows-vulkan-unsupported-platform-canary.result=PASS`
- Full Windows Vulkan canary matrix (`vk047` batch): PASS
  - key line: `ALL_VK047_CHECKS_PASS`
- Static scan:
  - `rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_unsupported_platform_canary|Windows Vulkan Gate Unsupported Platform Canary|vulkanUnsupportedPlatformCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY109_VK047_WINDOWS_VULKAN_UNSUPPORTED_PLATFORM_AVAILABILITY_DETAIL_ASSERTION_HARDENING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_AVAILABILITY_DETAIL_ASSERTION_HARDENING_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_AVAILABILITY_DETAIL_ASSERTION_HARDENING_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_AVAILABILITY_DETAIL_ASSERTION_HARDENING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 175: Vulkan chain VK-046 Windows diagnostics-contract availability detail assertion hardening

**Date**: 2026-03-28

### Problem Description
- Diagnostics-contract expected-fail canary (`tools/run_windows_vulkan_gate_contract_canary.ps1`) previously asserted:
  - `failure_reason=vulkan-diagnostics-contract-broken`
  - `diag_contract_valid=false`
- But it did not assert `windows-vulkan-check.vulkan_availability_failure_detail`, leaving classification drift risk for `diag-contract-missing-required-fields`.

### Root Cause Analysis
- `run_windows_vulkan_checks.ps1` already emits availability-detail classification, but legacy contract canary was created before detail-level branch hardening rounds (`VK-033~VK-045`) and lacked parity assertions.
- CI Step Summary for contract canary also lacked this field, reducing triage visibility.

### Solution
- Updated `tools/run_windows_vulkan_gate_contract_canary.ps1`:
  - parse `windows-vulkan-check.vulkan_availability_failure_detail`
  - require value equals `diag-contract-missing-required-fields`
  - export `windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail`
- Updated `.github/workflows/cross-platform-gate.yml` contract canary Step Summary rows:
  - added `gate_vulkan_availability_failure_detail`.
- Synced docs/records/index chain for `VK-046`.

### Validation
- Build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk046-baseline.env"` -> PASS
- Contract canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk046.env"` -> PASS
  - key lines:
    - `windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail=diag-contract-missing-required-fields`
    - `windows-vulkan-contract-canary.result=PASS`
- Full Windows Vulkan canary matrix (`vk046` batch): PASS
  - key line: `ALL_VK046_CHECKS_PASS`
- Static scan:
  - `rg -n "gate_vulkan_availability_failure_detail|diag-contract-missing-required-fields|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY108_VK046_WINDOWS_VULKAN_DIAGNOSTICS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_AVAILABILITY_DETAIL_ASSERTION_HARDENING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 174: Vulkan chain VK-045 Windows strict-compiled-in-disabled expected-fail canary

**Date**: 2026-03-28

### Problem Description
- Strict unavailable expected-fail canaries already covered availability detail branches:
  - `compiled-in-false`
  - `runtime-unavailable`
  - `diag-exit-nonzero`
  - `diag-result-not-pass`
- Branch `compiled-in-disabled` still lacked deterministic canary coverage.

### Root Cause Analysis
- `tools/run_windows_vulkan_checks.ps1` distinguishes `compiled-in-false` vs `compiled-in-disabled` by `vulkan-diagnostics.dependency_source`, but no dedicated canary exercised and asserted the `disabled` path.
- CI could not detect drift where disabled-build branch is misclassified.

### Solution
- Added `tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1`:
  - builds deterministic mock executable under `logs/`
  - diagnostics contract-valid output with:
    - `supported_platform=true`
    - `compiled_in=false`
    - `runtime_available=false`
    - `dependency_source=disabled`
    - `result=FAIL`
  - runs gate in strict mode (`-RequireVulkanAvailable`)
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate mode = `strict`
    - failure reason = `vulkan-not-available-in-strict-mode`
    - availability failure detail = `compiled-in-disabled`
    - playback check executed = `false`
    - diag exit code = `0`
    - diag dependency source = `disabled`
  - publishes machine-readable `windows-vulkan-strict-compiled-in-disabled-canary.*`
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added strict-compiled-in-disabled canary command/log capture
  - added Step Summary section:
    - `Windows Vulkan Gate Strict Compiled-In-Disabled Canary`
  - added fail-fast guard:
    - `if ($vulkanStrictCompiledInDisabledCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-045`.

### Validation
- Build:
  - `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk046-baseline.env"` -> PASS
  - key lines:
    - `windows-vulkan-check.result=SKIPPED`
    - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled`
    - `windows-vulkan-check.diag_dependency_source=disabled`
- Strict-compiled-in-disabled canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1 ... -SummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-gate-vk046.env"` -> PASS
  - key lines:
    - `windows-vulkan-strict-compiled-in-disabled-canary.actual_gate_exit_code=2`
    - `windows-vulkan-strict-compiled-in-disabled-canary.gate_vulkan_availability_failure_detail=compiled-in-disabled`
    - `windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_dependency_source=disabled`
    - `windows-vulkan-strict-compiled-in-disabled-canary.result=PASS`
- Full Windows Vulkan canary matrix (`vk046` batch): PASS
  - key line: `ALL_VK046_CHECKS_PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_strict_compiled_in_disabled_canary|vulkanStrictCompiledInDisabledCanaryExitCode|Windows Vulkan Gate Strict Compiled-In-Disabled Canary|windows-vulkan-strict-compiled-in-disabled-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY107_VK045_WINDOWS_VULKAN_STRICT_COMPILED_IN_DISABLED_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_COMPILED_IN_DISABLED_EXPECTED_FAIL_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_COMPILED_IN_DISABLED_EXPECTED_FAIL_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_COMPILED_IN_DISABLED_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 173: Vulkan chain VK-044 Windows strict-diag-result-not-pass expected-fail canary

**Date**: 2026-03-28

### Problem Description
- Windows Vulkan gate 已覆盖 strict unavailable expected-fail 分支：
  - `compiled-in-false`
  - `runtime-unavailable`
  - `diag-exit-nonzero`
- strict unavailable 分支中 `diag-result-not-pass` 仍缺少确定性 canary，CI 不能直接锁定该分类回归。

### Root Cause Analysis
- `tools/run_windows_vulkan_checks.ps1` 虽已实现 `diag-result-not-pass` 分类逻辑，但无独立脚本在 strict 模式下构造并断言该分支。
- 若未来分支判定顺序或分类条件调整，`diag-result-not-pass` 可能被其他分支吞没而不被现有 canary 发现。

### Solution
- Added `tools/run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=true`
    - `vulkan-diagnostics.runtime_available=true`
    - `vulkan-diagnostics.result=FAIL`
  - diagnostics command exits zero (`exit /b 0`) to isolate `diag-result-not-pass` branch.
  - invokes gate in strict mode (`-RequireVulkanAvailable`).
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate mode = `strict`
    - failure reason = `vulkan-not-available-in-strict-mode`
    - availability failure detail = `diag-result-not-pass`
    - playback check executed = `false`
    - diag exit code = `0`
    - diag result = `FAIL`
  - publishes machine-readable `windows-vulkan-strict-diag-result-not-pass-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added strict-diag-result-not-pass canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary`
  - added fail-fast guard:
    - `if ($vulkanStrictDiagResultNotPassCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-044`.

### Validation
- Build:
  - `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk044-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Full canary regression (`VK-044` batch):
  - diagnostics expected-fail canary -> PASS
  - playback contract expected-fail canary -> PASS
  - PASS-contract canary -> PASS
  - strict-unavailable canary -> PASS
  - strict-runtime-unavailable canary -> PASS
  - strict-diag-exit-nonzero canary -> PASS
  - strict-diag-result-not-pass canary -> PASS
    - `windows-vulkan-strict-diag-result-not-pass-canary.actual_gate_exit_code=2`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_result=FAIL`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_mode=strict`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_vulkan_availability_failure_detail=diag-result-not-pass`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_playback_check_executed=false`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_exit_code=0`
    - `windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_result=FAIL`
    - `windows-vulkan-strict-diag-result-not-pass-canary.result=PASS`
  - optional-skip canary -> PASS
  - unsupported-platform canary -> PASS
  - playback-semantic canary -> PASS
  - playback-backend semantic canary -> PASS
  - playback-candidates semantic canary -> PASS
  - playback-plan-reason semantic canary -> PASS
  - playback-result-not-pass canary -> PASS
  - playback-command-exit-nonzero canary -> PASS
- Static scan:
  - `rg -n "run_windows_vulkan_gate_strict_diag_result_not_pass_canary|vulkanStrictDiagResultNotPassCanaryExitCode|Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary|windows-vulkan-strict-diag-result-not-pass-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY106_VK044_WINDOWS_VULKAN_STRICT_DIAG_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_DESIGN_2026-03-28.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_PLAN_2026-03-28.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 172: Linux WSL gate build/playback chain stabilization

**Date**: 2026-03-28

### Problem Description
- 在 Windows + WSL2 (`Ubuntu-24.04`) 首次执行 Linux 构建时，若把构建目录放在 `/mnt/d/...` 下，CMake 在 `configure_file` 阶段多次报错 `Operation not permitted`，无法完成配置。
- 切换到 Linux 文件系统构建后，暴露出 Linux 专属编译与运行问题：
  - `include/streaming/http_stream_downloader.h` 缺少 `uint8_t` 头文件声明；
  - OpenGL 渲染实现受到 X11 宏污染（`None`/`Complex`）影响；
  - Linux gate 的 CP-902 在 `avcodec_send_packet` 路径触发崩溃；
  - CP-404（Linux audio backend smoke）在 WSL 环境下因“并非全部后端可通过”而被错误判定失败。

### Root Cause Analysis
- WSL 的 DrvFs 挂载目录（`/mnt/d`）在当前环境下不适合作为 CMake 配置输出目录，`configure_file` 写入出现权限语义冲突。
- Linux 编译链比 Windows 更严格地暴露出头文件显式依赖与宏命名污染问题。
- `PlayerCore` 在软件解码路径把 `AVCodecContext::get_format` 清空为 `nullptr`，而 FFmpeg 在部分构建下会在首包发送时延迟触发该回调，导致不安全调用路径。
- CP-404 代码侧成功条件与 gate 脚本契约不一致：脚本要求“至少一个可用后端通过”，代码却要求“所有可用后端都通过”。

### Solution
- 将 Linux 构建目录固定到 WSL Linux 文件系统（例如 `/home/u1133/mvp-build-linux`）。
- 修复 Linux 编译与运行稳定性：
  - `include/streaming/http_stream_downloader.h` 增加 `#include <cstdint>`；
  - `src/render/opengl_video_renderer.cpp` 在非 Windows 路径显式 `#undef None` / `#undef Complex`；
  - `src/core/player_core.cpp` 保持软件/硬件路径都使用有效 `get_format` 回调，并设置 `opaque=this`；
  - `src/main.cpp` 将 CP-404 判定修正为 `available_count > 0 && pass_count > 0`。
- 新增 `.gitattributes` 规则 `*.sh text eol=lf`，避免 WSL 下脚本被 CRLF 破坏。

### Validation
- Linux configure/build（WSL）：
  - `cmake -S . -B /home/u1133/mvp-build-linux -DCMAKE_BUILD_TYPE=Release -DDEBUG_MODE=OFF -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_DXVA2=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_VAAPI=ON -DENABLE_VIDEOTOOLBOX=OFF` -> PASS
  - `cmake --build /home/u1133/mvp-build-linux --parallel --target modern-video-player sample_logger_plugin` -> PASS
- Linux gate（WSL/Xvfb）：
  - `xvfb-run -a bash ./tools/run_linux_mvp_checks.sh /home/u1133/mvp-build-linux/modern-video-player ... /home/u1133/mvp-logs/linux-mvp-gate-summary-20260328.env 0` -> PASS
  - 关键结果：
    - `linux-audio-backend-smoke.available_target_count=3`
    - `linux-audio-backend-smoke.pass_target_count=1`
    - `linux-audio-backend-smoke.result=PASS`
    - `Linux MVP gate result: PASS`
- Linux packaging（WSL）：
  - `bash ./tools/package_linux.sh /home/u1133/mvp-package-build-20260328` -> PASS
  - 产物：
    - `modern-video-player_1.0.0_amd64.deb`
    - `modern-video-player-1.0.0-rc1-linux-x64.tar.gz`

### Modified Files
- `.gitattributes`
- `include/streaming/http_stream_downloader.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 171: Vulkan chain VK-043 Windows strict-diag-exit-nonzero expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate already had strict unavailable expected-fail coverage for availability detail `compiled-in-false` and `runtime-unavailable`.
- Strict unavailable branch for availability detail `diag-exit-nonzero` still lacked deterministic CI protection.

### Root Cause Analysis
- Existing strict canaries did not construct a deterministic scenario where diagnostics contract is valid but diagnostics process exits non-zero.
- CI had no stable assertion on `windows-vulkan-check.vulkan_availability_failure_detail=diag-exit-nonzero`.
- Without this canary, branch ordering/classification around diagnostics exit handling could drift without immediate gate signal.

### Solution
- Added `tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=true`
    - `vulkan-diagnostics.runtime_available=true`
    - `vulkan-diagnostics.result=PASS`
  - exits diagnostics command non-zero intentionally (`exit /b 9`).
  - invokes gate in strict mode (`-RequireVulkanAvailable`).
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate mode = `strict`
    - failure reason = `vulkan-not-available-in-strict-mode`
    - availability failure detail = `diag-exit-nonzero`
    - playback check executed = `false`
    - diag exit code = `9`
  - publishes machine-readable `windows-vulkan-strict-diag-exit-nonzero-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added strict-diag-exit-nonzero canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary`
  - added fail-fast guard:
    - `if ($vulkanStrictDiagExitNonzeroCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-043`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk043-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Strict-runtime-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-runtime-unavailable-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-strict-runtime-unavailable-canary.result=PASS`
- Strict-diag-exit-nonzero canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-gate-vk043.env` -> PASS
  - key lines:
    - `windows-vulkan-strict-diag-exit-nonzero-canary.actual_gate_exit_code=2`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_result=FAIL`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_mode=strict`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_vulkan_availability_failure_detail=diag-exit-nonzero`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_playback_check_executed=false`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.gate_diag_exit_code=9`
    - `windows-vulkan-strict-diag-exit-nonzero-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Playback-plan-reason semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-plan-reason-semantic-canary.result=PASS`
- Playback-result-not-pass canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-result-not-pass-canary.result=PASS`
- Playback-command-exit-nonzero canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk043.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk043.env` -> PASS
  - key line: `windows-vulkan-playback-command-exit-nonzero-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_strict_diag_exit_nonzero_canary|vulkanStrictDiagExitNonzeroCanaryExitCode|Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary|windows-vulkan-strict-diag-exit-nonzero-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY105_VK043_WINDOWS_VULKAN_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 170: Vulkan chain VK-042 Windows strict-runtime-unavailable expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate already had strict unavailable expected-fail coverage for availability detail `compiled-in-false`.
- Strict unavailable branch for availability detail `runtime-unavailable` still lacked deterministic CI protection.

### Root Cause Analysis
- Existing strict unavailable canary asserts failure classification only for compiled-in-missing path.
- CI had no stable mock scenario where diagnostics remains contract-valid with `compiled_in=true` but `runtime_available=false`.
- Without branch-specific canary, availability-detail classification could drift without immediate CI signal.

### Solution
- Added `tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=true`
    - `vulkan-diagnostics.runtime_available=false`
    - `vulkan-diagnostics.result=FAIL`
  - invokes gate in strict mode (`-RequireVulkanAvailable`).
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate mode = `strict`
    - failure reason = `vulkan-not-available-in-strict-mode`
    - availability failure detail = `runtime-unavailable`
    - playback check executed = `false`
  - publishes machine-readable `windows-vulkan-strict-runtime-unavailable-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added strict-runtime-unavailable canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Strict Runtime-Unavailable Canary`
  - added fail-fast guard:
    - `if ($vulkanStrictRuntimeUnavailableCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-042`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk042-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Strict-runtime-unavailable canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-runtime-unavailable-canary-gate-vk042.env` -> PASS
  - key lines:
    - `windows-vulkan-strict-runtime-unavailable-canary.actual_gate_exit_code=2`
    - `windows-vulkan-strict-runtime-unavailable-canary.gate_result=FAIL`
    - `windows-vulkan-strict-runtime-unavailable-canary.gate_mode=strict`
    - `windows-vulkan-strict-runtime-unavailable-canary.gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-strict-runtime-unavailable-canary.gate_vulkan_availability_failure_detail=runtime-unavailable`
    - `windows-vulkan-strict-runtime-unavailable-canary.gate_playback_check_executed=false`
    - `windows-vulkan-strict-runtime-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Playback-plan-reason semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-plan-reason-semantic-canary.result=PASS`
- Playback-result-not-pass canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-result-not-pass-canary.result=PASS`
- Playback-command-exit-nonzero canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk042.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk042.env` -> PASS
  - key line: `windows-vulkan-playback-command-exit-nonzero-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_strict_runtime_unavailable_canary|vulkanStrictRuntimeUnavailableCanaryExitCode|Windows Vulkan Gate Strict Runtime-Unavailable Canary|windows-vulkan-strict-runtime-unavailable-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY104_VK042_WINDOWS_VULKAN_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 169: Vulkan chain VK-041 Windows playback-command-exit-nonzero expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, unsupported-platform failure, and five playback semantic/result branches.
- Playback command-exit branch (`playback_failure_detail=command-exit-nonzero`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing playback canaries (`VK-036`~`VK-040`) focus on renderer/backend/candidates/plan-reason/result mismatch, but do not cover non-zero process exit after contract-valid output.
- CI had no stable mock scenario where playback contract remains valid while command exit code is non-zero.
- This branch is evaluated before `result-not-pass` and downstream semantic checks; drift could bypass those canaries.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid PASS payload and exits non-zero intentionally.
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `command-exit-nonzero`
    - playback result = `PASS`
  - publishes machine-readable `windows-vulkan-playback-command-exit-nonzero-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-command-exit-nonzero canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Command-Exit-Nonzero Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackCommandExitNonzeroCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-041`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk041-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Playback-plan-reason semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-plan-reason-semantic-canary.result=PASS`
- Playback-result-not-pass canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk041.env` -> PASS
  - key line: `windows-vulkan-playback-result-not-pass-canary.result=PASS`
- Playback-command-exit-nonzero canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk041.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk041.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-command-exit-nonzero-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-command-exit-nonzero-canary.gate_result=FAIL`
    - `windows-vulkan-playback-command-exit-nonzero-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_failure_detail=command-exit-nonzero`
    - `windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_result=PASS`
    - `windows-vulkan-playback-command-exit-nonzero-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_command_exit_nonzero_canary|vulkanPlaybackCommandExitNonzeroCanaryExitCode|Windows Vulkan Gate Playback Command-Exit-Nonzero Canary|windows-vulkan-playback-command-exit-nonzero-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY103_VK041_WINDOWS_VULKAN_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 168: Vulkan chain VK-040 Windows playback-result-not-pass expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, unsupported-platform failure, and four playback semantic failures.
- Playback result failure branch (`playback_failure_detail=result-not-pass`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing semantic canaries (`VK-036`~`VK-039`) only covered renderer/backend/candidates/plan-reason mismatches after playback result check.
- CI had no stable mock scenario where playback contract stays valid but `performance-log-check.result` is not `PASS`.
- This branch is evaluated before downstream semantic checks, so drift could bypass existing branch-specific canaries.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid output with intentional result mismatch:
    - `performance-log-check.result=FAIL`
    - `performance-log-check.startup_selected_renderer=Vulkan`
    - `performance-log-check.renderer_backend=Vulkan`
    - `performance-log-check.startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL`
    - `performance-log-check.startup_renderer_plan_reason=renderer-override-env`
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `result-not-pass`
    - playback result = `FAIL`
  - publishes machine-readable `windows-vulkan-playback-result-not-pass-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-result-not-pass canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Result-Not-Pass Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackResultNotPassCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-040`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk040-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Playback-plan-reason semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk040.env` -> PASS
  - key line: `windows-vulkan-playback-plan-reason-semantic-canary.result=PASS`
- Playback-result-not-pass canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk040.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk040.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-result-not-pass-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-result-not-pass-canary.gate_result=FAIL`
    - `windows-vulkan-playback-result-not-pass-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-result-not-pass-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-result-not-pass-canary.gate_playback_failure_detail=result-not-pass`
    - `windows-vulkan-playback-result-not-pass-canary.gate_playback_result=FAIL`
    - `windows-vulkan-playback-result-not-pass-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_result_not_pass_canary|vulkanPlaybackResultNotPassCanaryExitCode|Windows Vulkan Gate Playback Result-Not-Pass Canary|windows-vulkan-playback-result-not-pass-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY102_VK040_WINDOWS_VULKAN_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 167: Vulkan chain VK-039 Windows playback-plan-reason semantic expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, unsupported-platform failure, selected-renderer semantic failure, backend semantic failure, and candidates semantic failure.
- Plan-reason semantic failure branch (`playback_failure_detail=plan-reason-not-renderer-override-env`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing semantic canaries (`VK-036`, `VK-037`, `VK-038`) cover selected renderer, renderer backend, and candidate-chain mismatches only.
- CI had no stable mock scenario where playback contract remains valid but plan reason is not `renderer-override-env`.
- Real runner behavior is not deterministic for this specific branch.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid output with plan-reason semantic mismatch:
    - `performance-log-check.startup_selected_renderer=Vulkan`
    - `performance-log-check.renderer_backend=Vulkan`
    - `performance-log-check.startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL`
    - `performance-log-check.startup_renderer_plan_reason=strategy-default`
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `plan-reason-not-renderer-override-env`
  - publishes machine-readable `windows-vulkan-playback-plan-reason-semantic-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-plan-reason semantic canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Plan Reason Semantic Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackPlanReasonSemanticCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-039`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk039-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk039.env` -> PASS
  - key line: `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Playback-plan-reason semantic canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk039.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk039.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-plan-reason-semantic-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-plan-reason-semantic-canary.gate_result=FAIL`
    - `windows-vulkan-playback-plan-reason-semantic-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_failure_detail=plan-reason-not-renderer-override-env`
    - `windows-vulkan-playback-plan-reason-semantic-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_plan_reason_semantic_canary|vulkanPlaybackPlanReasonSemanticCanaryExitCode|Windows Vulkan Gate Playback Plan Reason Semantic Canary|windows-vulkan-playback-plan-reason-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY101_VK039_WINDOWS_VULKAN_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 166: Vulkan chain VK-038 Windows playback-candidates semantic expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, unsupported-platform failure, selected-renderer semantic failure, and backend semantic failure.
- Candidate-chain semantic failure branch (`playback_failure_detail=candidates-missing-vulkan`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing semantic canaries (`VK-036`, `VK-037`) cover selected renderer and renderer backend mismatches only.
- CI had no stable mock scenario where playback contract remains valid but startup candidate chain omits Vulkan.
- Real runner behavior is not deterministic for this specific branch.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid output with candidate-chain semantic mismatch:
    - `performance-log-check.startup_selected_renderer=Vulkan`
    - `performance-log-check.renderer_backend=Vulkan`
    - `performance-log-check.startup_renderer_candidates=D3D11 > SoftwareSDL`
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `candidates-missing-vulkan`
  - publishes machine-readable `windows-vulkan-playback-candidates-semantic-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-candidates semantic canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Candidates Semantic Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackCandidatesSemanticCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-038`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk038-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk038.env` -> PASS
  - key line: `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Playback-candidates semantic canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk038.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk038.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-candidates-semantic-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-candidates-semantic-canary.gate_result=FAIL`
    - `windows-vulkan-playback-candidates-semantic-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-candidates-semantic-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-candidates-semantic-canary.gate_playback_failure_detail=candidates-missing-vulkan`
    - `windows-vulkan-playback-candidates-semantic-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_candidates_semantic_canary|vulkanPlaybackCandidatesSemanticCanaryExitCode|Windows Vulkan Gate Playback Candidates Semantic Canary|windows-vulkan-playback-candidates-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY100_VK038_WINDOWS_VULKAN_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 165: Vulkan chain VK-037 Windows playback-backend semantic expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, unsupported-platform failure, and selected-renderer semantic failure.
- Backend semantic failure branch (`playback_failure_detail=renderer-backend-not-vulkan`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing semantic canary (`VK-036`) only covers `startup_selected_renderer != Vulkan`.
- CI had no stable mock scenario where playback contract remains valid but backend semantic mismatches Vulkan expectation.
- Real runner behavior is not deterministic for this specific branch.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid output with backend semantic mismatch:
    - `performance-log-check.startup_selected_renderer=Vulkan`
    - `performance-log-check.renderer_backend=OpenGL`
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `renderer-backend-not-vulkan`
  - publishes machine-readable `windows-vulkan-playback-backend-semantic-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-backend semantic canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Backend Semantic Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackBackendSemanticCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-037`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk037-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk037.env` -> PASS
  - key line: `windows-vulkan-playback-semantic-canary.result=PASS`
- Playback-backend semantic canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk037.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk037.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-backend-semantic-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-backend-semantic-canary.gate_result=FAIL`
    - `windows-vulkan-playback-backend-semantic-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-backend-semantic-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-backend-semantic-canary.gate_playback_failure_detail=renderer-backend-not-vulkan`
    - `windows-vulkan-playback-backend-semantic-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_backend_semantic_canary|vulkanPlaybackBackendSemanticCanaryExitCode|Windows Vulkan Gate Playback Backend Semantic Canary|windows-vulkan-playback-backend-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY99_VK037_WINDOWS_VULKAN_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 164: Vulkan chain VK-036 Windows playback-semantic expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, optional-unavailable skip, and unsupported-platform failure.
- Playback semantic failure branch (`failure_reason=vulkan-playback-check-failed`) still lacked deterministic CI protection.

### Root Cause Analysis
- Existing playback canary (`VK-031`) validates missing-field contract failure only.
- CI had no stable mock scenario where playback contract remains valid but selected renderer semantics violate Vulkan expectations.
- Real runner behavior is not deterministic for this exact branch.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_semantic_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics PASS contract with Vulkan available.
  - emits playback contract-valid output with semantic mismatch:
    - `performance-log-check.startup_selected_renderer=D3D11`
    - `performance-log-check.renderer_backend=Vulkan`
  - invokes gate under optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `vulkan-playback-check-failed`
    - playback contract valid = `true`
    - playback failure detail = `selected-renderer-not-vulkan`
  - publishes machine-readable `windows-vulkan-playback-semantic-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-semantic canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Semantic Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackSemanticCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-036`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk036-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk036.env` -> PASS
  - key line: `windows-vulkan-unsupported-platform-canary.result=PASS`
- Playback-semantic canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-summary-vk036.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-semantic-canary-gate-vk036.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-semantic-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-semantic-canary.gate_result=FAIL`
    - `windows-vulkan-playback-semantic-canary.gate_failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-playback-semantic-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-playback-semantic-canary.gate_playback_failure_detail=selected-renderer-not-vulkan`
    - `windows-vulkan-playback-semantic-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_semantic_canary|vulkanPlaybackSemanticCanaryExitCode|Windows Vulkan Gate Playback Semantic Canary|windows-vulkan-playback-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_semantic_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_semantic_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY98_VK036_WINDOWS_VULKAN_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 163: Vulkan chain VK-035 Windows unsupported-platform expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, strict-unavailable failure, and optional-unavailable skip.
- Unsupported-platform failure branch (`failure_reason=unsupported-platform`) still lacked deterministic CI protection.

### Root Cause Analysis
- CI had no stable mock-based scenario that can force:
  - diagnostics contract-valid output
  - unsupported-platform decision branch (`supported_platform=false`)
  - expected gate FAIL contract without playback check execution
- Regular Windows runner behavior cannot naturally exercise unsupported-platform path.

### Solution
- Added `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `platform=Linux`
    - `supported_platform=false`
    - `compiled_in=false`
    - `runtime_available=false`
    - `result=FAIL`
    - `dependency_source=find_package`
  - invokes gate under default optional mode.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - failure reason = `unsupported-platform`
    - skip reason empty
    - playback check executed = `false`
  - publishes machine-readable `windows-vulkan-unsupported-platform-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added unsupported-platform canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Unsupported Platform Canary`
  - added fail-fast guard:
    - `if ($vulkanUnsupportedPlatformCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-035`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk035-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk035.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk035.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk035.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk035.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk035.env` -> PASS
  - key line: `windows-vulkan-optional-skip-canary.result=PASS`
- Unsupported-platform canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk035.env -GateSummaryOutputPath logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk035.env` -> PASS
  - key lines:
    - `windows-vulkan-unsupported-platform-canary.actual_gate_exit_code=2`
    - `windows-vulkan-unsupported-platform-canary.gate_result=FAIL`
    - `windows-vulkan-unsupported-platform-canary.gate_failure_reason=unsupported-platform`
    - `windows-vulkan-unsupported-platform-canary.gate_skip_reason=`
    - `windows-vulkan-unsupported-platform-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_unsupported_platform_canary|vulkanUnsupportedPlatformCanaryExitCode|Windows Vulkan Gate Unsupported Platform Canary|windows-vulkan-unsupported-platform-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY97_VK035_WINDOWS_VULKAN_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 162: Vulkan chain VK-034 Windows optional-unavailable skip canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures, strict PASS, and strict-unavailable failure.
- Optional-unavailable skip branch (`result=SKIPPED`) still lacked deterministic CI protection.

### Root Cause Analysis
- CI had no stable mock-based scenario that can force:
  - contract-valid diagnostics output
  - Vulkan unavailable probe
  - optional-mode skip semantics (`skip_reason=vulkan-not-available`)
- Real runner Vulkan variability makes this branch non-deterministic in regular runs.

### Solution
- Added `tools/run_windows_vulkan_gate_optional_skip_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `supported_platform=true`
    - `compiled_in=false`
    - `runtime_available=false`
    - `result=FAIL`
    - `dependency_source=find_package`
  - invokes gate without strict override.
  - validates:
    - gate exit code = `0`
    - gate result = `SKIPPED`
    - gate mode = `optional`
    - strict mode effective = `false`
    - skip reason = `vulkan-not-available`
    - failure reason empty
    - playback check executed = `false`
    - availability detail = `compiled-in-false`
  - publishes machine-readable `windows-vulkan-optional-skip-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added optional-skip canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Optional Skip Canary`
  - added fail-fast guard:
    - `if ($vulkanOptionalSkipCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-034`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk034-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk034.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk034.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk034.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk034.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk034.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk034.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk034.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk034.env` -> PASS
  - key line: `windows-vulkan-strict-unavailable-canary.result=PASS`
- Optional-skip canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-summary-vk034.env -GateSummaryOutputPath logs/windows-vulkan-gate-optional-skip-canary-gate-vk034.env` -> PASS
  - key lines:
    - `windows-vulkan-optional-skip-canary.actual_gate_exit_code=0`
    - `windows-vulkan-optional-skip-canary.gate_result=SKIPPED`
    - `windows-vulkan-optional-skip-canary.gate_mode=optional`
    - `windows-vulkan-optional-skip-canary.gate_skip_reason=vulkan-not-available`
    - `windows-vulkan-optional-skip-canary.gate_vulkan_availability_failure_detail=compiled-in-false`
    - `windows-vulkan-optional-skip-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_optional_skip_canary|vulkanOptionalSkipCanaryExitCode|Windows Vulkan Gate Optional Skip Canary|windows-vulkan-optional-skip-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_optional_skip_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_optional_skip_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY96_VK034_WINDOWS_VULKAN_OPTIONAL_UNAVAILABLE_SKIP_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 161: Vulkan chain VK-033 Windows strict-unavailable expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canary coverage for diagnostics/playback contract failures and strict PASS contract.
- Strict policy unavailable branch (`vulkan-not-available-in-strict-mode`) lacked deterministic CI protection.

### Root Cause Analysis
- CI had no stable mock-based scenario that can force:
  - contract-valid diagnostics output
  - Vulkan unavailable probe (`compiled_in/runtime_available=false`)
  - strict-mode expected failure behavior
- Real runner Vulkan variability makes this branch non-deterministic in regular runs.

### Solution
- Added `tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits diagnostics contract-valid output with:
    - `supported_platform=true`
    - `compiled_in=false`
    - `runtime_available=false`
    - `result=FAIL`
    - `dependency_source=find_package`
  - invokes gate with strict switch (`-RequireVulkanAvailable`).
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate mode = `strict`
    - failure reason = `vulkan-not-available-in-strict-mode`
    - playback check executed = `false`
    - availability detail = `compiled-in-false`
  - publishes machine-readable `windows-vulkan-strict-unavailable-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added strict-unavailable canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Strict Unavailable Canary`
  - added fail-fast guard:
    - `if ($vulkanStrictUnavailableCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-033`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk033-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk033.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk033.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk033.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk033.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary regression:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk033.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk033.env` -> PASS
  - key line: `windows-vulkan-pass-contract-canary.result=PASS`
- Strict-unavailable canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk033.env -GateSummaryOutputPath logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk033.env` -> PASS
  - key lines:
    - `windows-vulkan-strict-unavailable-canary.actual_gate_exit_code=2`
    - `windows-vulkan-strict-unavailable-canary.gate_result=FAIL`
    - `windows-vulkan-strict-unavailable-canary.gate_mode=strict`
    - `windows-vulkan-strict-unavailable-canary.gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-strict-unavailable-canary.gate_vulkan_availability_failure_detail=compiled-in-false`
    - `windows-vulkan-strict-unavailable-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_strict_unavailable_canary|vulkanStrictUnavailableCanaryExitCode|Windows Vulkan Gate Strict Unavailable Canary|windows-vulkan-strict-unavailable-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY95_VK033_WINDOWS_VULKAN_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 160: Vulkan chain VK-032 Windows PASS-contract canary

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate had deterministic canaries for expected-fail branches only.
- PASS branch contract lacked deterministic CI protection.

### Root Cause Analysis
- CI had no stable mock-based scenario that can force strict-path PASS with valid diagnostics + playback contracts.
- Real runner Vulkan availability is variable, so regular gate runs cannot reliably verify PASS semantics.

### Solution
- Added `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits valid diagnostics contract:
    - `supported_platform=true`
    - `compiled_in=true`
    - `runtime_available=true`
    - `result=PASS`
  - emits complete playback PASS contract:
    - `performance-log-check.result=PASS`
    - `startup_selected_renderer=Vulkan`
    - `renderer_backend=Vulkan`
    - `startup_renderer_plan_reason=renderer-override-env`
  - invokes gate with strict switch and validates:
    - gate exit code = `0`
    - gate result = `PASS`
    - gate mode = `strict`
    - playback contract valid = `true`
    - playback failure detail = `none`
  - publishes machine-readable `windows-vulkan-pass-contract-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added PASS-contract canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate PASS Contract Canary`
  - added fail-fast guard:
    - `if ($vulkanPassCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-032`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk032-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Diagnostics expected-fail canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk032.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk032.env` -> PASS
  - key line: `windows-vulkan-contract-canary.result=PASS`
- Playback expected-fail canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk032.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk032.env` -> PASS
  - key line: `windows-vulkan-playback-contract-canary.result=PASS`
- PASS-contract canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-summary-vk032.env -GateSummaryOutputPath logs/windows-vulkan-gate-pass-contract-canary-gate-vk032.env` -> PASS
  - key lines:
    - `windows-vulkan-pass-contract-canary.actual_gate_exit_code=0`
    - `windows-vulkan-pass-contract-canary.gate_result=PASS`
    - `windows-vulkan-pass-contract-canary.gate_mode=strict`
    - `windows-vulkan-pass-contract-canary.gate_playback_contract_valid=true`
    - `windows-vulkan-pass-contract-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_pass_contract_canary|vulkanPassCanaryExitCode|Windows Vulkan Gate PASS Contract Canary|windows-vulkan-pass-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY94_VK032_WINDOWS_VULKAN_PASS_CONTRACT_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 159: Vulkan chain VK-031 Windows playback-contract expected-fail canary

**Date**: 2026-03-27

### Problem Description
- Existing deterministic canary (`VK-029`) covered diagnostics-contract failure path only.
- Playback-contract failure path (`VK-026`) still lacked deterministic CI canary protection.

### Root Cause Analysis
- CI had no stable mock-based scenario that can force:
  - `failure_reason=vulkan-playback-contract-broken`
  - `playback_contract_valid=false`
- Playback-contract branch is difficult to reliably hit with real runner variability alone.

### Solution
- Added `tools/run_windows_vulkan_gate_playback_contract_canary.ps1`:
  - builds deterministic mock executable under `logs/`.
  - emits valid diagnostics contract to pass availability gate.
  - emits intentionally incomplete playback output to trigger playback-contract-broken.
  - validates:
    - gate exit code = `2`
    - gate result = `FAIL`
    - gate failure reason = `vulkan-playback-contract-broken`
    - playback contract valid = `false`
    - required missing fields are reported.
  - publishes machine-readable `windows-vulkan-playback-contract-canary.*`.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added playback-contract canary command and log capture.
  - added summary parsing + `GITHUB_STEP_SUMMARY` section:
    - `Windows Vulkan Gate Playback Contract Canary`
  - added fail-fast guard:
    - `if ($vulkanPlaybackCanaryExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-031`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk031-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Playback-contract canary:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-summary-vk031.env -GateSummaryOutputPath logs/windows-vulkan-gate-playback-contract-canary-gate-vk031.env` -> PASS
  - key lines:
    - `windows-vulkan-playback-contract-canary.actual_gate_exit_code=2`
    - `windows-vulkan-playback-contract-canary.gate_summary_file_present=true`
    - `windows-vulkan-playback-contract-canary.gate_result=FAIL`
    - `windows-vulkan-playback-contract-canary.gate_failure_reason=vulkan-playback-contract-broken`
    - `windows-vulkan-playback-contract-canary.gate_playback_contract_valid=false`
    - `windows-vulkan-playback-contract-canary.gate_playback_failure_detail=contract-missing-required-fields`
    - `windows-vulkan-playback-contract-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_playback_contract_canary|vulkanPlaybackCanaryExitCode|Windows Vulkan Gate Playback Contract Canary|windows-vulkan-playback-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_playback_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY93_VK031_WINDOWS_VULKAN_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 158: Vulkan chain VK-030 Windows gate contract-canary Step Summary observability

**Date**: 2026-03-27

### Problem Description
- `VK-029` added deterministic Windows Vulkan gate contract canary, but canary result visibility remained artifact/log-only.
- CI reviewer could not directly read canary status from GitHub Step Summary panel.

### Root Cause Analysis
- Workflow lacked Step Summary rendering logic for `windows-vulkan-gate-contract-canary-summary.env`.
- Canary summary payload did not explicitly expose whether child gate summary file existed.

### Solution
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate step:
  - parse `logs/windows-vulkan-gate-contract-canary-summary.env`.
  - append `Windows Vulkan Gate Contract Canary` table to `GITHUB_STEP_SUMMARY`.
  - add fallback message when canary summary env is missing.
  - key rows:
    - `result`
    - `expected_gate_exit_code`
    - `actual_gate_exit_code`
    - `gate_summary_file_present`
    - `gate_result`
    - `gate_failure_reason`
    - `gate_diag_contract_valid`
    - `validation_failure_reason`
- Updated `tools/run_windows_vulkan_gate_contract_canary.ps1`:
  - added field `windows-vulkan-contract-canary.gate_summary_file_present`.
  - added explicit validation branch for missing gate summary file.
- Synced docs/records/index chain for `VK-030`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk030-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Canary run:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk030.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk030.env` -> PASS
  - key lines:
    - `windows-vulkan-contract-canary.actual_gate_exit_code=2`
    - `windows-vulkan-contract-canary.gate_summary_file_present=true`
    - `windows-vulkan-contract-canary.gate_result=FAIL`
    - `windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken`
    - `windows-vulkan-contract-canary.result=PASS`
- Local Step Summary preview:
  - generated `logs/windows-vulkan-canary-step-summary-preview-vk030.md`
  - key lines:
    - `| result | PASS |`
    - `| actual_gate_exit_code | 2 |`
    - `| gate_summary_file_present | true |`
    - `| gate_failure_reason | vulkan-diagnostics-contract-broken |`
- Static scan:
  - `rg -n "Windows Vulkan Gate Contract Canary|gate_summary_file_present|vulkanCanarySummaryPath|vulkanCanaryExitCode|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY92_VK030_WINDOWS_VULKAN_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 157: Vulkan chain VK-029 Windows gate expected-fail contract canary

**Date**: 2026-03-27

### Problem Description
- `VK-028` ensured real Vulkan gate failures propagate in CI, but there was no deterministic canary for gate failure-contract behavior.
- Hardware/runtime availability on runners is variable, so normal gate execution does not always exercise failure-contract branches.

### Root Cause Analysis
- CI lacked a stable expected-fail sub-check validating:
  - gate failure exit code contract
  - failure reason and diagnostics-contract fields in summary output

### Solution
- Added `tools/run_windows_vulkan_gate_contract_canary.ps1`:
  - runs `run_windows_vulkan_checks.ps1` against `cmd.exe` diagnostics path to trigger deterministic diagnostics-contract failure.
  - validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken`
    - `windows-vulkan-check.diag_contract_valid=false`
  - publishes machine-readable `windows-vulkan-contract-canary.*` fields.
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate:
  - added canary command execution.
  - captures `$vulkanCanaryExitCode = $LASTEXITCODE`.
  - fail-fast throws on non-zero canary exit.
- Synced docs/records/index chain for `VK-029`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk029-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Canary run:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-contract-canary-summary-vk029.env -GateSummaryOutputPath logs/windows-vulkan-gate-contract-canary-gate-vk029.env` -> PASS
  - key lines:
    - `windows-vulkan-contract-canary.actual_gate_exit_code=2`
    - `windows-vulkan-contract-canary.gate_result=FAIL`
    - `windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken`
    - `windows-vulkan-contract-canary.gate_diag_contract_valid=false`
    - `windows-vulkan-contract-canary.result=PASS`
- Static scan:
  - `rg -n "run_windows_vulkan_gate_contract_canary|vulkanCanaryExitCode|windows-vulkan-gate-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_gate_contract_canary.ps1`
- `docs/analysis/PLAYERCORE_DAY91_VK029_WINDOWS_VULKAN_GATE_EXPECTED_FAIL_CONTRACT_CANARY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 156: Vulkan chain VK-028 Windows gate exit-code propagation hardening

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate command in workflow uses PowerShell pipeline logging (`... | Tee-Object`).
- Non-zero native-command exit code stays in `$LASTEXITCODE`, but step does not fail automatically unless explicitly checked.
- This creates a gate integrity risk: Vulkan gate can report `FAIL` while CI step still continues.

### Root Cause Analysis
- Workflow Windows gate step lacked explicit exit-code propagation check after running `run_windows_vulkan_checks.ps1`.
- Pipeline logging behavior masked non-zero exit at step-result level.

### Solution
- Updated `.github/workflows/cross-platform-gate.yml` Windows gate step:
  - capture `$vulkanGateExitCode = $LASTEXITCODE` immediately after Vulkan gate pipeline command.
  - keep existing `VK-027` Step Summary rendering logic unchanged.
  - add fail-fast guard before downstream checks:
    - `if ($vulkanGateExitCode -ne 0) { throw ... }`
- Synced docs/records/index chain for `VK-028`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk028-baseline.env` -> PASS
  - key line: `windows-vulkan-check.result=SKIPPED`
- Legacy behavior reproduction (without guard):
  - strict gate output reported `windows-vulkan-check.result=FAIL`
  - `legacy-last=2`
  - outer process exit stayed `0` (failure swallowed)
- Guarded behavior reproduction:
  - strict gate output reported `windows-vulkan-check.result=FAIL`
  - explicit throw emitted: `Windows Vulkan gate failed with exit code 2`
  - outer process exit became non-zero (`1`)
- Static scan:
  - `rg -n "vulkanGateExitCode|Windows Vulkan gate failed with exit code|windows-vulkan-gate-summary.env" .github/workflows/cross-platform-gate.yml` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY90_VK028_WINDOWS_VULKAN_GATE_EXIT_CODE_PROPAGATION_HARDENING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 155: Vulkan chain VK-027 Windows CI step summary observability

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate already exported machine-readable `.env` summary artifacts, but CI page lacked direct at-a-glance gate status.
- Diagnosis still required downloading artifacts/logs instead of reading Step Summary.

### Root Cause Analysis
- Workflow Windows gate step did not parse Vulkan summary env into `GITHUB_STEP_SUMMARY`.
- No fallback message existed when summary env was missing.

### Solution
- Updated `.github/workflows/cross-platform-gate.yml` (Windows gate step):
  - parse `logs/windows-vulkan-gate-summary.env` into key/value map.
  - append Markdown summary table to `GITHUB_STEP_SUMMARY`.
  - include key fields:
    - `result`, `mode`, `strict_mode_effective`, `strict_mode_policy`
    - `runner_vulkan_sdk_available`, `runner_vulkan_runtime_probe_available`, `runner_vulkan_runtime_probe_detail`
    - `diag_contract_valid`, `playback_contract_valid`
    - `failure_reason`, `vulkan_availability_failure_detail`, `playback_failure_detail`
  - add fallback summary message when env file is missing.
- Synced docs/records/index chain for `VK-027`.

### Validation
- `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline env generation:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk027-baseline.env` -> PASS
- Local workflow-equivalent summary preview:
  - generated `logs/windows-vulkan-step-summary-preview-vk027.md`
  - key lines:
    - `| result | SKIPPED |`
    - `| mode | optional |`
    - `| strict_mode_effective | false |`
    - `| diag_contract_valid | true |`
    - `| playback_contract_valid | n/a |`
    - `| vulkan_availability_failure_detail | compiled-in-disabled |`
- Static scan:
  - `rg -n "Windows Vulkan Gate Summary|GITHUB_STEP_SUMMARY|windows-vulkan-gate-summary.env|runner_vulkan_runtime_probe_detail|playback_contract_valid|vulkan_availability_failure_detail" .github/workflows/cross-platform-gate.yml` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY89_VK027_WINDOWS_VULKAN_CI_STEP_SUMMARY_OBSERVABILITY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 154: Vulkan chain VK-026 Windows playback contract validation

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate already validated diagnostics contract (`VK-025`), but playback output still lacked required-key contract checks.
- Malformed playback outputs could be reported as generic playback failures without contract-level diagnosis.

### Root Cause Analysis
- `run_windows_vulkan_checks.ps1` parsed playback fields with defaults and did not assert required `performance-log-check.*` keys.
- No machine-readable playback contract validity/missing-fields outputs were available.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added playback required-key validation.
  - added summary fields:
    - `windows-vulkan-check.playback_contract_valid`
    - `windows-vulkan-check.playback_missing_required_fields`
    - `windows-vulkan-check.playback_failure_detail`
  - added playback contract-broken failure path:
    - `failure_reason=vulkan-playback-contract-broken`
    - `playback_failure_detail=contract-missing-required-fields`
  - non-playback paths now emit explicit `n/a`/`not-executed` markers for playback contract fields.
- Synced docs/records/index chain for `VK-026`.

### Validation
- Static scan:
  - `rg -n "playback_contract_valid|playback_missing_required_fields|playback_failure_detail|vulkan-playback-contract-broken|contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1` -> PASS
- `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk026-baseline.env` -> PASS
  - key lines:
    - `windows-vulkan-check.playback_check_executed=false`
    - `windows-vulkan-check.playback_contract_valid=n/a`
    - `windows-vulkan-check.playback_missing_required_fields=n/a`
    - `windows-vulkan-check.playback_failure_detail=not-executed`
- Playback contract-broken simulation:
  - mock executable: `logs/mock_windows_vulkan_gate_player.cmd`
  - run:
    - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "logs/mock_windows_vulkan_gate_player.cmd" ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk026-playback-contract-broken.env` -> expected FAIL (exit 2)
  - key lines:
    - `windows-vulkan-check.playback_check_executed=true`
    - `windows-vulkan-check.playback_contract_valid=false`
    - `windows-vulkan-check.playback_missing_required_fields=performance-log-check.startup_selected_renderer,performance-log-check.renderer_backend,performance-log-check.startup_renderer_candidates,performance-log-check.startup_renderer_plan_reason`
    - `windows-vulkan-check.playback_failure_detail=contract-missing-required-fields`
    - `windows-vulkan-check.failure_reason=vulkan-playback-contract-broken`
- Auto policy compatibility spot-check:
  - `auto + sdk=1 + runtime_probe=0` -> `SKIPPED` (exit 0)
  - `auto + sdk=1 + runtime_probe=1` -> strict expected `FAIL` (exit 2 on current host)

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY88_VK026_WINDOWS_VULKAN_PLAYBACK_CONTRACT_VALIDATION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 153: Vulkan chain VK-025 Windows diagnostics contract validation

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan gate used default parsing for diagnostics output and had no required-key contract validation.
- Diagnostics contract regressions could be misclassified as generic availability issues.

### Root Cause Analysis
- `run_windows_vulkan_checks.ps1` did not assert required `--vulkan-diagnostics` keys.
- No machine-readable contract-valid/missing-fields outputs were available.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added required diagnostics key validation.
  - added summary fields:
    - `windows-vulkan-check.diag_contract_valid`
    - `windows-vulkan-check.diag_missing_required_fields`
  - added contract-broken failure path:
    - `failure_reason=vulkan-diagnostics-contract-broken`
    - `vulkan_availability_failure_detail=diag-contract-missing-required-fields`
- Synced docs/records/index chain for `VK-025`.

### Validation
- Static scan:
  - `rg -n "diag_contract_valid|diag_missing_required_fields|vulkan-diagnostics-contract-broken|diag-contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1` -> PASS
- `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk025-baseline.env` -> PASS
  - key lines:
    - `windows-vulkan-check.diag_contract_valid=true`
    - `windows-vulkan-check.diag_missing_required_fields=none`
    - `windows-vulkan-check.result=SKIPPED`
- Contract-broken simulation:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "C:/Windows/System32/cmd.exe" ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk025-contract-broken.env` -> expected FAIL (exit 2)
  - key lines:
    - `windows-vulkan-check.diag_contract_valid=false`
    - `windows-vulkan-check.diag_missing_required_fields=vulkan-diagnostics.platform,vulkan-diagnostics.supported_platform,vulkan-diagnostics.compiled_in,vulkan-diagnostics.runtime_available,vulkan-diagnostics.result`
    - `windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken`
    - `windows-vulkan-check.vulkan_availability_failure_detail=diag-contract-missing-required-fields`
- Compatibility spot-check:
  - `auto + sdk=1 + runtime_probe=0` -> `SKIPPED` (exit 0)
  - `auto + sdk=1 + runtime_probe=1` -> strict expected `FAIL` (exit 2 on current host)

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY87_VK025_WINDOWS_VULKAN_DIAGNOSTICS_CONTRACT_VALIDATION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 152: Vulkan chain VK-024 Windows availability failure detail classification

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan strict failures still carried a coarse reason (`vulkan-not-available-in-strict-mode`).
- CI diagnosis lacked a dedicated machine-readable classification for availability-stage failure source.

### Root Cause Analysis
- Gate summary had no field for availability probe pass/failure detail.
- Existing fields exposed raw diag/runtime flags but not a normalized root-cause classification.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added:
    - `windows-vulkan-check.vulkan_availability_probe_passed`
    - `windows-vulkan-check.vulkan_availability_failure_detail`
  - classification covers:
    - `unsupported-platform`
    - `none`
    - `compiled-in-disabled`
    - `compiled-in-false`
    - `runtime-unavailable`
    - `diag-exit-nonzero`
    - `diag-result-not-pass`
    - `unknown`
  - retained existing `result/failure_reason/skip_reason` contract.
- Synced docs/records/index chain for `VK-024`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing -> forcing OFF.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Static scan:
  - `rg -n "vulkan_availability_probe_passed|vulkan_availability_failure_detail|compiled-in-disabled|diag-result-not-pass" tools/run_windows_vulkan_checks.ps1` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk024-baseline.env` -> PASS
  - key lines:
    - `windows-vulkan-check.vulkan_availability_probe_passed=false`
    - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled`
    - `windows-vulkan-check.result=SKIPPED`
- Matrix check (behavior unchanged):
  - `auto + sdk=1 + runtime_probe=0` -> PASS (exit 0, optional, `SKIPPED`)
  - `auto + sdk=1 + runtime_probe=1` -> expected FAIL (exit 2, strict, current host)

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY86_VK024_WINDOWS_VULKAN_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 151: Vulkan chain VK-023 Windows runtime probe detail observability

**Date**: 2026-03-27

### Problem Description
- `VK-022` already added runtime probe availability signal, but gate summary still exposed only boolean state.
- CI diagnosis still required manual workflow log inspection to understand runtime probe failure reason.

### Root Cause Analysis
- Runtime probe detail existed only as workflow step log text.
- `run_windows_vulkan_checks.ps1` had no summary field for runtime probe detail.

### Solution
- Updated `.github/workflows/cross-platform-gate.yml`:
  - export `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL=<detail>` together with runtime probe availability.
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - read `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL`.
  - add summary field:
    - `windows-vulkan-check.runner_vulkan_runtime_probe_detail`
  - normalize empty detail to `unknown`.
- Kept strict policy behavior unchanged (`VK-022` compatibility).
- Synced docs/records/index chain for `VK-023`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing -> forcing OFF.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Static scan:
  - `rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL|runner_vulkan_runtime_probe_detail" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1` -> PASS
- Baseline gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk023-baseline.env` -> PASS
  - key lines:
    - `windows-vulkan-check.runner_vulkan_runtime_probe_detail=unknown`
    - `windows-vulkan-check.result=SKIPPED`
- Matrix check (behavior unchanged):
  - `auto + sdk=1 + runtime_probe=0 + detail=vulkaninfo-missing` -> PASS (exit 0, optional, `SKIPPED`)
  - `auto + sdk=1 + runtime_probe=1 + detail=vulkaninfo-path` -> expected FAIL (exit 2, strict, current host)

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY85_VK023_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL_OBSERVABILITY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 150: Vulkan chain VK-022 Windows auto strict runtime probe guard

**Date**: 2026-03-27

### Problem Description
- `VK-019` auto strict promotion only depended on SDK signal.
- On runners with SDK present but runtime/device not usable, strict mode could be promoted too early and create false strict failures.

### Root Cause Analysis
- `run_windows_vulkan_checks.ps1` treated `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1` as sufficient for auto strict.
- Workflow lacked a dedicated runtime probe signal for policy gating.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - auto strict now requires both:
    - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE`
    - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE`
  - added machine-readable summary fields:
    - `windows-vulkan-check.runner_vulkan_runtime_probe_available`
    - `windows-vulkan-check.strict_mode_auto_basis`
    - `windows-vulkan-check.strict_mode_auto_prerequisites_met`
- Updated `.github/workflows/cross-platform-gate.yml`:
  - Windows Vulkan SDK step now performs runtime probe via `vulkaninfo --summary` (SDK bin first, PATH fallback).
  - exports `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1|0`.
- Synced docs/records/index chain for `VK-022`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing -> forcing OFF.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on current host
  - key lines:
    - `vulkan-diagnostics.compiled_in=false`
    - `vulkan-diagnostics.runtime_available=false`
    - `vulkan-diagnostics.result=FAIL`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk022-auto-sdk1-runtime0.env` with `auto + sdk=1 + runtime_probe=0` -> PASS
  - key lines:
    - `windows-vulkan-check.strict_mode_auto_prerequisites_met=false`
    - `windows-vulkan-check.mode=optional`
    - `windows-vulkan-check.result=SKIPPED`
- Same command with `auto + sdk=1 + runtime_probe=1` -> expected FAIL (exit 2 on current host)
  - key lines:
    - `windows-vulkan-check.strict_mode_auto_prerequisites_met=true`
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.result=FAIL`
- Static scan:
  - `rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE|strict_mode_auto_basis|strict_mode_auto_prerequisites_met|sdk_and_runtime_probe|vulkaninfo" tools/run_windows_vulkan_checks.ps1 .github/workflows/cross-platform-gate.yml` -> PASS

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY84_VK022_WINDOWS_VULKAN_AUTO_STRICT_RUNTIME_PROBE_GUARD.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 149: Vulkan chain VK-021 Windows dependency-source observability and find_package hardening

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan path had SDK fallback closure, but CMake still lacked an explicit completeness contract for `find_package(Vulkan)`.
- Diagnostics could not show whether build dependency came from `find_package`, SDK fallback, or downgrade path.

### Root Cause Analysis
- Dependency decision relied on `Vulkan_FOUND` with no explicit complete/incomplete metadata distinction.
- No machine-readable build dependency-source field existed in Vulkan diagnostics or Windows gate summary.

### Solution
- Updated `CMakeLists.txt`:
  - added completeness guard for Windows `find_package(Vulkan)` metadata.
  - incomplete package metadata now falls back to `VULKAN_SDK` probe.
  - fallback miss still forces OFF (safe downgrade preserved).
  - added compile-time source marker:
    - `MVP_VULKAN_DEPENDENCY_SOURCE=find_package|vulkan_sdk_fallback|disabled`
- Updated `src/main.cpp`:
  - `--vulkan-diagnostics` now prints:
    - `vulkan-diagnostics.dependency_source`
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - summary now prints:
    - `windows-vulkan-check.diag_dependency_source`
- Synced docs/records/index chain for `VK-021`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing -> forcing OFF.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on current host
  - key lines:
    - `vulkan-diagnostics.dependency_source=disabled`
    - `vulkan-diagnostics.compiled_in=false`
    - `vulkan-diagnostics.result=FAIL`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-vk021.env` -> PASS
  - key lines:
    - `windows-vulkan-check.diag_dependency_source=disabled`
    - `windows-vulkan-check.skip_reason=vulkan-not-available`
    - `windows-vulkan-check.result=SKIPPED`
- Static scan:
  - `rg -n "vulkan-diagnostics\.dependency_source|diag_dependency_source|MVP_VULKAN_DEPENDENCY_SOURCE|find_package\(Vulkan\) returned incomplete package info" ...` -> PASS

### Modified Files
- `CMakeLists.txt`
- `src/main.cpp`
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY83_VK021_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 148: Vulkan chain VK-020 Windows CMake SDK fallback and CMake prefix-path closure

**Date**: 2026-03-27

### Problem Description
- Windows lane already had Vulkan SDK provisioning and auto strict policy, but CMake resolution still relied mainly on `find_package(Vulkan)`.
- When package discovery fails despite SDK presence, Vulkan can remain `compiled_in=false`, and Windows Vulkan gate keeps `SKIPPED`.

### Root Cause Analysis
- No explicit Windows SDK include/lib fallback existed in CMake after `find_package(Vulkan)` miss.
- Workflow exported `VULKAN_SDK` but did not inject SDK root into `CMAKE_PREFIX_PATH`.

### Solution
- Updated `CMakeLists.txt`:
  - after `find_package(Vulkan)` miss, probe `VULKAN_SDK` fallback:
    - `${VULKAN_SDK}/Include/vulkan/vulkan.h`
    - `${VULKAN_SDK}/Lib/vulkan-1.lib`
  - complete fallback appends include/lib and keeps Vulkan enabled.
  - incomplete fallback warns and forces OFF (safe downgrade unchanged).
- Updated `.github/workflows/cross-platform-gate.yml`:
  - when SDK is detected, prepend `CMAKE_PREFIX_PATH` with SDK root.
- Synced docs/records/index chain for `VK-020`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: `find_package(Vulkan)` missing -> forcing OFF.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on current host
  - key lines:
    - `vulkan-diagnostics.platform=Windows`
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=false`
    - `vulkan-diagnostics.result=FAIL`
- `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary.env` -> PASS
  - key lines:
    - `windows-vulkan-check.compiled_in=false`
    - `windows-vulkan-check.skip_reason=vulkan-not-available`
    - `windows-vulkan-check.result=SKIPPED`

### Modified Files
- `CMakeLists.txt`
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY82_VK020_WINDOWS_VULKAN_CMAKE_SDK_FALLBACK_AND_PREFIX_PATH.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 147: Vulkan chain VK-019 Windows Vulkan auto strict policy promotion

**Date**: 2026-03-27

### Problem Description
- `VK-018` introduced SDK provisioning and SDK availability signal, but strict enforcement still relied on explicit manual policy.
- Without auto policy promotion, CI could remain optional and keep returning `SKIPPED` despite runner-side SDK availability signal.

### Root Cause Analysis
- `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS` had static on/off semantics in practice.
- Workflow default was not auto-promoted by SDK readiness signal.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added policy normalization and `auto` semantics:
    - `auto` -> strict when `MVP_WINDOWS_VULKAN_SDK_AVAILABLE` is truthy
    - `on` -> strict
    - `off` -> optional
  - retained CLI strict override (`-RequireVulkanAvailable`)
  - added machine-readable fields:
    - `windows-vulkan-check.strict_mode_policy`
    - `windows-vulkan-check.strict_mode_cli_requested`
    - `windows-vulkan-check.strict_mode_effective`
- Updated `.github/workflows/cross-platform-gate.yml`:
  - promoted default policy to:
    - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS: "auto"`
- Synced docs/records/index chain for `VK-019`.

### Validation
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- Policy matrix checks:
  - `auto + sdk=0` -> PASS, `mode=optional`, `strict_mode_effective=false`, `result=SKIPPED`
  - `auto + sdk=1` -> expected FAIL (exit 2), `mode=strict`, `strict_mode_effective=true`, `result=FAIL`
  - `off + sdk=1` -> PASS, `mode=optional`, `strict_mode_effective=false`, `result=SKIPPED`
- Summary files generated and validated for all matrix paths.

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY81_VK019_WINDOWS_VULKAN_AUTO_STRICT_POLICY_PROMOTION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 146: Vulkan chain VK-018 Windows Vulkan SDK provisioning and CI observability

**Date**: 2026-03-27

### Problem Description
- `VK-017` completed Windows Vulkan gate strict/summary contract, but CI still lacked a Windows Vulkan SDK provisioning attempt.
- Gate outputs also lacked SDK-context fields, making root-cause diagnosis for `compiled_in=false` less explicit.

### Root Cause Analysis
- Workflow Windows lane had no Vulkan SDK install/probe stage.
- `run_windows_vulkan_checks.ps1` summary did not include SDK presence/path/runner availability context.

### Solution
- Updated `.github/workflows/cross-platform-gate.yml`:
  - added Windows-only optional SDK provisioning step:
    - attempts `choco install vulkan-sdk`
    - probes `C:\\VulkanSDK`
    - exports `VULKAN_SDK` and appends `${VULKAN_SDK}\\Bin` to path when detected
    - exports `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1|0`
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added summary fields:
    - `windows-vulkan-check.vulkan_sdk_present`
    - `windows-vulkan-check.vulkan_sdk_path`
    - `windows-vulkan-check.runner_vulkan_sdk_available`
  - no behavior regression for result decision contract.
- Synced docs/records/index chain for `VK-018`.

### Validation
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing, forcing OFF.
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\\tools\\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary.env` -> PASS
  - key lines:
    - `windows-vulkan-check.vulkan_sdk_present=false`
    - `windows-vulkan-check.vulkan_sdk_path=`
    - `windows-vulkan-check.runner_vulkan_sdk_available=false`
    - `windows-vulkan-check.result=SKIPPED`
  - summary file generated and validated.

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY80_VK018_WINDOWS_VULKAN_SDK_PROVISIONING_AND_CI_OBSERVABILITY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 145: Vulkan chain VK-017 Windows Vulkan gate strict policy and summary artifact

**Date**: 2026-03-27

### Problem Description
- `VK-016` added Windows Vulkan gate command and CI hook, but operational closure still lacked:
  - persisted summary artifact output for Windows Vulkan gate stage
  - environment-driven strict policy control for CI/runtime escalation

### Root Cause Analysis
- `tools/run_windows_vulkan_checks.ps1` only emitted stdout lines and had no summary-file output parameter.
- Strict mode depended on CLI switch only, not CI env policy.
- Workflow Windows lane did not yet persist dedicated Vulkan gate summary/log files.

### Solution
- Updated `tools/run_windows_vulkan_checks.ps1`:
  - added `-SummaryOutputPath` parameter
  - added env strict policy hook:
    - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS`
  - added output field:
    - `windows-vulkan-check.strict_mode_env_requested`
  - summary file now persists full `windows-vulkan-check.*` snapshot.
- Updated `.github/workflows/cross-platform-gate.yml`:
  - added job env default:
    - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS: "0"`
  - Windows gate step now:
    - creates `logs/`
    - runs Vulkan check with `-SummaryOutputPath logs/windows-vulkan-gate-summary.env`
    - tees output to `logs/windows-vulkan-gate.log`

### Validation
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning on current host: Vulkan package missing, forcing OFF.
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\\tools\\run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary.env` -> PASS
  - key lines:
    - `windows-vulkan-check.mode=optional`
    - `windows-vulkan-check.strict_mode_env_requested=false`
    - `windows-vulkan-check.result=SKIPPED`
  - summary file generated and validated.
- `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=1` with same command -> expected FAIL (exit code `2`)
  - key lines:
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.strict_mode_env_requested=true`
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.result=FAIL`
  - strict summary file generated.

### Modified Files
- `tools/run_windows_vulkan_checks.ps1`
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY79_VK017_WINDOWS_VULKAN_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 144: Vulkan chain VK-016 Windows Vulkan gate and CI integration

**Date**: 2026-03-27

### Problem Description
- Windows Vulkan feature had enablement + diagnostics baseline, but Windows gate/CI still lacked a dedicated Vulkan check stage.
- There was no machine-readable Windows Vulkan gate output contract and no optional/strict policy for unavailable Vulkan hosts.

### Root Cause Analysis
- `.github/workflows/cross-platform-gate.yml` Windows lane only executed D3D11/OpenGL checks.
- No dedicated Windows Vulkan check script existed under `tools/`.

### Solution
- Added `tools/run_windows_vulkan_checks.ps1`:
  - probes `--vulkan-diagnostics`
  - emits machine-readable `windows-vulkan-check.*` fields
  - supports optional mode (default) and strict mode (`-RequireVulkanAvailable`)
  - when Vulkan is available, executes `MVP_RENDERER_BACKEND=vulkan --performance-log-check` and validates renderer selection/output contract
- Updated `.github/workflows/cross-platform-gate.yml`:
  - Windows configure now explicitly sets `-DENABLE_VULKAN_RENDERER=ON`
  - Windows gate stage now calls `run_windows_vulkan_checks.ps1`
- Synced docs/records/index chain for this round.

### Validation
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - expected warning: missing Vulkan package on current host, forcing OFF.
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\\tools\\run_windows_vulkan_checks.ps1 ...` -> PASS (exit 0, optional mode)
  - key lines:
    - `windows-vulkan-check.supported_platform=true`
    - `windows-vulkan-check.skip_reason=vulkan-not-available`
    - `windows-vulkan-check.result=SKIPPED`
- `powershell -ExecutionPolicy Bypass -File .\\tools\\run_windows_vulkan_checks.ps1 ... -RequireVulkanAvailable` -> expected FAIL (exit 2)
  - key lines:
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.result=FAIL`

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `tools/run_windows_vulkan_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY78_VK016_WINDOWS_VULKAN_GATE_AND_CI_INTEGRATION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 143: Vulkan chain VK-015 Windows link/runtime probe closure

**Date**: 2026-03-27

### Problem Description
- `Issue 142` opened Windows Vulkan enablement, but two closure gaps remained:
  - Windows final target link stage did not include `PLATFORM_EXTRA_LIBRARIES`, so discovered Vulkan link inputs might not reach linker stage.
  - Vulkan capability publication used static `runtime_available=true` when compiled, not a runtime probe.

### Root Cause Analysis
- `CMakeLists.txt` (Windows branch) gathered Vulkan libs/includes but omitted `PLATFORM_EXTRA_LIBRARIES` in `target_link_libraries(${PROJECT_NAME} ...)`.
- `src/platform/platform_capabilities.cpp` published Vulkan support with compile-time runtime assumption.

### Solution
- Updated `CMakeLists.txt`:
  - add `${PLATFORM_EXTRA_LIBRARIES}` to Windows final `target_link_libraries` for `modern-video-player`.
- Updated `src/platform/platform_capabilities.cpp`:
  - added `probeVulkanRuntimeAvailability(...)` using:
    - `vkEnumerateInstanceExtensionProperties`
    - `vkCreateInstance`
    - `vkEnumeratePhysicalDevices`
  - switched Vulkan support publication to probe-driven `runtime_available`.
- Synced docs/records/index chain for this follow-up round:
  - new analysis/design/plan/report docs for Windows link/runtime probe closure.
  - updated `analysis/design/plans/reports` README indexes.
  - updated `VERSION.md`, `CHANGELOG.md`, `DEVELOP_LOG.md`.

### Validation
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - warning on current host: missing Vulkan package, forcing OFF (expected).
- `& "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin\\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on current host
  - key lines:
    - `vulkan-diagnostics.platform=Windows`
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=false`
    - `vulkan-diagnostics.runtime_available=false`
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check ...` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS

### Modified Files
- `CMakeLists.txt`
- `src/platform/platform_capabilities.cpp`
- `docs/analysis/PLAYERCORE_DAY77_VK015_WINDOWS_VULKAN_LINK_AND_RUNTIME_PROBE_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 142: Vulkan chain VK-015 Windows enablement implementation

**Date**: 2026-03-27

### Problem Description
- Vulkan backend was hard-disabled on Windows by CMake platform guards.
- `--vulkan-diagnostics` treated Linux as the only supported platform.
- Result: Windows could not enter Vulkan compile/runtime contract and diagnostics remained Linux-only.

### Root Cause Analysis
- `CMakeLists.txt` forced `ENABLE_VULKAN_RENDERER=OFF` when `WIN32`.
- Windows build path had no Vulkan dependency-detection branch.
- `runVulkanDiagnostics()` hardcoded `supported_platform=(Linux)`.

### Solution
- Updated `CMakeLists.txt`:
  - allow `ENABLE_VULKAN_RENDERER` on Windows and Linux (non-Apple).
  - keep Apple forced OFF.
  - add Windows Vulkan dependency detection:
    - `find_package(Vulkan QUIET)`
    - on missing dependency, warn and force OFF (graceful downgrade).
- Updated `src/main.cpp`:
  - extend diagnostics contract:
    - `supported_platform = (Linux || Windows)`.

### Validation
- `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - key warning confirms graceful downgrade on current host:
    - `ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing ... forcing OFF`
- `cmake --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on current host with corrected platform contract
  - key lines:
    - `vulkan-diagnostics.platform=Windows`
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=false`
    - `vulkan-diagnostics.result=FAIL`
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check ...` -> PASS
  - key lines confirm fallback observability:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `CMakeLists.txt`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY76_VK015_WINDOWS_VULKAN_ENABLEMENT_SCOPE_AND_IMPLEMENTATION_PLANNER.md`
- `docs/design/CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 141: Vulkan chain VK-014 documentation and release closure

**Date**: 2026-03-27

### Problem Description
- After `VK-013` regression execution, the Vulkan chain still required final release-level synchronization across records and index entrances.
- Without a dedicated closure step, completion evidence remained fragmented across per-task artifacts.

### Root Cause Analysis
- `VK-001` ~ `VK-013` produced a large set of documents, but the chain-level closure (`VK-014`) had not yet synchronized all fixed record/index targets in one pass.

### Solution
- Added `VK-014` closure documents:
  - `docs/analysis/PLAYERCORE_DAY75_VK014_DOCUMENTATION_AND_RELEASE_CLOSURE.md`
  - `docs/design/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_DESIGN_2026-03-27.md`
  - `docs/plans/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_PLAN_2026-03-27.md`
  - `docs/reports/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_LOCAL_CHECK.md`
- Synced fixed records:
  - `docs/records/VERSION.md`
  - `docs/records/CHANGELOG.md`
  - `docs/records/DEVELOP_LOG.md`
- Synced index entrances:
  - `docs/analysis/README.md`
  - `docs/design/README.md`
  - `docs/plans/README.md`
  - `docs/reports/README.md`

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `rg -n "DAY75_VK014|DAY74_VK013|REGRESSION_MATRIX_EXECUTION|DOCUMENTATION_AND_RELEASE_CLOSURE" docs/analysis/README.md docs/design/README.md docs/plans/README.md docs/reports/README.md docs/records/VERSION.md docs/records/CHANGELOG.md docs/records/DEVELOP_LOG.md` -> PASS
- `git status --short` -> expected modified/untracked set only, no commit/push performed

### Modified Files
- `docs/analysis/PLAYERCORE_DAY75_VK014_DOCUMENTATION_AND_RELEASE_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 140: Vulkan chain VK-013 regression matrix execution

**Date**: 2026-03-27

### Problem Description
- Vulkan chain was integrated through `VK-012`, but this stage still lacked one consolidated regression matrix proof across open/play/pause/seek/subtitle/fallback paths.

### Root Cause Analysis
- Prior iterations focused on implementation and CI wiring.
- Stage-level runtime evidence was not yet archived in a dedicated report.

### Solution
- Executed and archived regression matrix commands:
  - build baseline
  - `--performance-log-check`
  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--subtitle-style-check`
  - `--subtitle-sync-check`
  - `--renderer-fallback-check`
  - `MVP_RENDERER_BACKEND=vulkan --performance-log-check`
  - `--vulkan-diagnostics`
- Added/updated `VK-013` docs:
  - `docs/analysis/PLAYERCORE_DAY74_VK013_REGRESSION_MATRIX_EXECUTION.md`
  - `docs/design/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_DESIGN_2026-03-27.md`
  - `docs/plans/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_PLAN_2026-03-27.md`
  - `docs/reports/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_LOCAL_CHECK.md`

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --seek-burst-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 6` -> first run FAIL, immediate rerun PASS
- `.\build\Release\modern-video-player.exe --paused-seek-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 4` -> PASS
- `.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass` -> PASS
- `.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check ...` -> PASS with fallback observability
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on Windows host (`supported_platform=false`)

### Modified Files
- `docs/analysis/PLAYERCORE_DAY74_VK013_REGRESSION_MATRIX_EXECUTION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 139: Vulkan chain VK-012 GitHub Actions Linux Vulkan lane

**Date**: 2026-03-27

### Problem Description
- `VK-011` Linux gate script already supports Vulkan checks, but GitHub Actions Linux lane was not yet enforcing Vulkan-ready build/check path.
- CI could still pass without strict Vulkan check execution.

### Root Cause Analysis
- Linux workflow dependency list lacked explicit Vulkan packages.
- Linux configure command did not pin `ENABLE_VULKAN_RENDERER=ON`.
- Linux gate call did not pass strict Vulkan requirement parameter (`REQUIRE_VULKAN_CHECKS`).

### Solution
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - dependency install adds:
    - `libvulkan-dev`
    - `mesa-vulkan-drivers`
  - configure command adds:
    - `-DENABLE_VULKAN_RENDERER=ON`
  - Linux gate invocation appends arg `11=1`:
    - strict Vulkan check requirement enabled via `VK-011` script contract.

### Validation
- `rg -n "libvulkan-dev|mesa-vulkan-drivers|ENABLE_VULKAN_RENDERER=ON|logs/linux-mvp-gate-summary.env|Run Linux gate|Install Linux dependencies|Configure Linux build" .github/workflows/cross-platform-gate.yml` -> PASS
- `& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh` -> PASS
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on Windows host
  - key lines:
    - `vulkan-diagnostics.supported_platform=false`
    - `vulkan-diagnostics.result=FAIL`

### Modified Files
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY73_VK012_GITHUB_ACTIONS_LINUX_VULKAN_LANE.md`
- `docs/design/CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 138: Vulkan chain VK-011 Linux gate Vulkan checks

**Date**: 2026-03-27

### Problem Description
- Linux gate script did not include a Vulkan stage after `VK-010` diagnostics command landed.
- Gate pipeline lacked machine-readable Vulkan availability/check state for later CI integration.

### Root Cause Analysis
- `tools/run_linux_mvp_checks.sh` had no Vulkan probe/check branch.
- Existing optional/strict behavior covered subtitle checks only (`CP-507`/`CP-508`), not Vulkan checks.

### Solution
- Added Vulkan probe and conditional check integration in `tools/run_linux_mvp_checks.sh`:
  - new strict hook:
    - arg `11`: `REQUIRE_VULKAN_CHECKS`
    - env fallback: `MVP_REQUIRE_VULKAN_CHECKS`
  - new probe function:
    - `probe_vulkan_check_availability`
    - probes `--vulkan-diagnostics`, parses capability fields and exit code
  - new gate stage:
    - check id `vk010_vulkan_diagnostics`
    - runs only when Vulkan probe indicates availability
    - validates candidate chain / plan reason / selected renderer / fallback target / result
- Added machine-readable report fields:
  - `gate.require_vulkan_checks`
  - `gate.vulkan_probe_exit_code`
  - `gate.vulkan_supported_platform`
  - `gate.vulkan_compiled_in`
  - `gate.vulkan_runtime_available`
  - `gate.has_vk010`
  - `gate.vulkan_skip_reason`
  - `check.vk010_vulkan_diagnostics.*`

### Validation
- `rg -n "REQUIRE_VULKAN_CHECKS|probe_vulkan_check_availability|gate.has_vk010|vk010_vulkan_diagnostics|vulkan_skip_reason|--vulkan-diagnostics" tools/run_linux_mvp_checks.sh` -> PASS
- `& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh` -> PASS
- `& "C:\Program Files\Git\bin\bash.exe" tools/run_linux_mvp_checks.sh` -> expected FAIL on non-Linux host
  - key line:
    - `This gate script only supports Linux.`
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on Windows host
  - key lines:
    - `vulkan-diagnostics.supported_platform=false`
    - `vulkan-diagnostics.result=FAIL`

### Modified Files
- `tools/run_linux_mvp_checks.sh`
- `docs/analysis/PLAYERCORE_DAY72_VK011_LINUX_GATE_VULKAN_CHECKS.md`
- `docs/design/CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 137: Vulkan chain VK-010 Vulkan diagnostics CLI

**Date**: 2026-03-27

### Problem Description
- Vulkan implementation reached startup/fallback integration (`VK-009`), but there was no dedicated diagnostics command for direct machine-readable probing.
- Next tasks (`VK-011` Linux gate / `VK-012` CI lane) require a stable Vulkan diagnostics CLI contract.

### Root Cause Analysis
- `src/main.cpp` only exposed diagnostics command paths for D3D11/OpenGL.
- Vulkan observability existed only indirectly through startup/performance outputs.

### Solution
- Added `runVulkanDiagnostics()` in `src/main.cpp`.
- Added CLI contract wiring:
  - usage text: `--vulkan-diagnostics`
  - command dispatch branch in `main`
- Reused existing strategy/capability abstractions (no new deep probe module):
  - `platform::PlatformCapabilitiesProbe::detect()`
  - `core::PlaybackStrategy::buildOpenPlan(...)`
  - `render::RendererFactory::isSupported(...)`
- Exported machine-readable fields:
  - `vulkan-diagnostics.supported_platform`
  - `vulkan-diagnostics.compiled_in`
  - `vulkan-diagnostics.runtime_available`
  - `vulkan-diagnostics.requested_renderer_override`
  - `vulkan-diagnostics.startup_renderer_candidates`
  - `vulkan-diagnostics.startup_renderer_plan_reason`
  - `vulkan-diagnostics.selected_renderer`
  - `vulkan-diagnostics.fallback_target`
  - `vulkan-diagnostics.result`

### Validation
- `rg -n "runVulkanDiagnostics|--vulkan-diagnostics|vulkan-diagnostics\\.|rendererCandidateChainToString" src/main.cpp` -> PASS
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on Windows host
  - key lines:
    - `vulkan-diagnostics.supported_platform=false`
    - `vulkan-diagnostics.result=FAIL`
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --vulkan-diagnostics` -> expected FAIL on Windows host with fallback observability
  - key lines:
    - `vulkan-diagnostics.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `vulkan-diagnostics.startup_renderer_plan_reason=renderer-override-env`
    - `vulkan-diagnostics.selected_renderer=D3D11`
    - `vulkan-diagnostics.fallback_target=D3D11`

### Modified Files
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY71_VK010_VULKAN_DIAGNOSTICS_CLI.md`
- `docs/design/CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/VERSION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 136: Vulkan chain VK-009 fallback chain and startup policy

**Date**: 2026-03-27

### Problem Description
- Linux Vulkan fallback chain behavior depended mainly on priority sorting side effects.
- Startup strategy output lacked machine-readable policy reason fields, reducing observability.

### Root Cause Analysis
- No explicit Linux Vulkan fallback-chain normalization in `PlaybackStrategy`.
- `DiagnosticsSnapshot` exposed startup candidates/fallback reasons but not planner reason metadata.

### Solution
- Added Linux Vulkan chain normalization in `PlaybackStrategy`:
  - when platform is Linux and first candidate is Vulkan, normalize prefix to:
    - `Vulkan -> OpenGL -> SoftwareSDL` (runtime-available subset only)
  - append renderer reason tag:
    - `linux-vulkan-fallback-chain`
- Added startup plan-reason observability fields:
  - `startup_renderer_plan_reason`
  - `startup_decoder_plan_reason`
- Exported both fields in `--performance-log-check` output.

### Validation
- `rg -n "normalizeLinuxVulkanFallbackChain|linux-vulkan-fallback-chain|startup_renderer_plan_reason|startup_decoder_plan_reason" src/core/playback_strategy.cpp include/core/player_core.h src/core/player_core.cpp src/main.cpp` -> PASS
  - key lines confirm Linux chain normalization and machine-readable output wiring.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `performance-log-check.startup_renderer_plan_reason=platform-default-order`
    - `performance-log-check.startup_decoder_plan_reason=hardware-first`
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `performance-log-check.startup_renderer_plan_reason=renderer-override-env`
    - `performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `src/core/playback_strategy.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY70_VK009_FALLBACK_CHAIN_AND_STARTUP_POLICY.md`
- `docs/design/CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_LOCAL_CHECK.md`

## Issue 135: Vulkan chain VK-008 sync and present pacing

**Date**: 2026-03-27

### Problem Description
- Vulkan path had basic sync objects but lacked pacing controls and robust wait behavior.
- In-flight frame wait used an unbounded fence wait, with potential long-stall behavior.
- Swapchain present mode had no explicit operator override contract.

### Root Cause Analysis
- Missing timeout/recovery branch in frame wait logic.
- Missing present-mode parse/select/fallback contract and runtime mode observability.

### Solution
- Added present-mode override contract:
  - env `MVP_VULKAN_PRESENT_MODE`
  - accepted values: `auto|fifo|mailbox|immediate|fifo_relaxed`
  - invalid values warn and fall back to `auto`
- Added present-mode selection/fallback flow in swapchain creation:
  - `auto`: mailbox -> fifo -> immediate -> first supported
  - explicit mode tries exact-match first, then safe fallback policy
  - startup/swapchain logs include requested/active mode and exact-match flag
- Added sync hardening:
  - fence wait timeout (`250ms`)
  - timeout recovery via `vkDeviceWaitIdle` and swapchain recreate request
- Added pacing observability counters/logs:
  - frame submitted/presented
  - fence wait total/max/timeout counts
  - periodic pacing snapshot logs

### Validation
- `rg -n "MVP_VULKAN_PRESENT_MODE|present_mode_requested|present_mode_active|kFrameFenceWaitTimeoutNs|fence_wait_timeout|Vulkan pacing stats|choosePresentMode\\(" src/render/vulkan_video_renderer.cpp` -> PASS
  - key lines confirm present-mode parser/selection and wait-timeout pacing paths.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; $env:MVP_VULKAN_PRESENT_MODE='immediate'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `src/render/vulkan_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY69_VK008_SYNC_AND_PRESENT_PACING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_LOCAL_CHECK.md`

## Issue 134: Vulkan chain VK-007 frame upload YUV path

**Date**: 2026-03-27

### Problem Description
- Vulkan backend could clear/present (`VK-006`) but could not upload and display decoded video frames.
- `present()` still used stale clear-only recording call, and Vulkan direct-frame format contract was incomplete.

### Root Cause Analysis
- Frame upload payload path ended at `renderFrame()` buffer staging and was not wired into present submit path.
- `supportsDirectFrameFormat()` was declared in Vulkan header but lacked cpp implementation.

### Solution
- Completed `present()` upload integration:
  - snapshot RGBA payload metadata under mutex
  - ensure host-visible staging buffer capacity
  - `vkMapMemory -> memcpy -> vkUnmapMemory`
  - invoke `recordPresentCommandBuffer(..., has_frame_upload, width, height, stride)`
- Implemented `VulkanVideoRenderer::supportsDirectFrameFormat()`:
  - accepts `AV_PIX_FMT_YUV420P` and `AV_PIX_FMT_NV12`
- Removed stale clear-only record call usage from Vulkan present path.

### Validation
- `rg -n "recordClearCommandBuffer|recordPresentCommandBuffer|supportsDirectFrameFormat\\(" src/render/vulkan_video_renderer.cpp include/render/vulkan_video_renderer.h` -> PASS
  - key lines confirm `recordPresentCommandBuffer` call path and `supportsDirectFrameFormat` implementation.
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `src/render/vulkan_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY68_VK007_FRAME_UPLOAD_YUV_PATH.md`
- `docs/design/CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_LOCAL_CHECK.md`

## Issue 133: Vulkan chain VK-006 clear/present and swapchain recreate

**Date**: 2026-03-27

### Problem Description
- `VK-005` initialized Vulkan objects but had no visible render output path.
- Swapchain could become stale on resize without recreate handling.

### Root Cause Analysis
- Missing frame-level acquire/submit/present synchronization and clear command recording.
- Missing SDL window-resize and Vulkan out-of-date/suboptimal recreate flow.

### Solution
- Added Vulkan frame resources:
  - command pool/command buffer
  - image-available / render-finished semaphores
  - in-flight fence
- Implemented minimal present loop:
  - wait in-flight
  - acquire image
  - record clear command (`vkCmdClearColorImage`)
  - submit + present
- Added swapchain recreate handling:
  - trigger on SDL resize/minimize/maximize/restore events
  - trigger on `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR`
  - deferred recreate when drawable size is zero
- `clear()` now routes to `present()` for observable clear output.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `src/render/vulkan_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY67_VK006_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE.md`
- `docs/design/CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_LOCAL_CHECK.md`

## Issue 132: Vulkan chain VK-005 instance/surface/device/swapchain init

**Date**: 2026-03-27

### Problem Description
- Vulkan backend had only skeleton wiring and could not initialize SDL/Vulkan runtime resources.
- `VK-006` (clear/present/recreate) was blocked without a valid swapchain lifecycle baseline.

### Root Cause Analysis
- Missing Vulkan runtime bring-up path (`instance -> surface -> device -> swapchain`).
- Missing deterministic teardown contract for partially initialized Vulkan resources.

### Solution
- Implemented Vulkan runtime initialization in `src/render/vulkan_video_renderer.cpp`:
  - SDL subsystem init (`VIDEO|AUDIO`) + owned-subsystem tracking
  - SDL Vulkan window creation
  - SDL-required Vulkan instance extension query
  - `vkCreateInstance` + `SDL_Vulkan_CreateSurface`
  - physical device suitability selection (graphics/present + `VK_KHR_swapchain` + surface support)
  - logical device/queue creation
  - swapchain creation and image acquisition
- Implemented deterministic `close()` release order:
  - `vkDeviceWaitIdle`
  - `vkDestroySwapchainKHR`
  - `vkDestroyDevice`
  - `vkDestroySurfaceKHR`
  - `vkDestroyInstance`
  - `SDL_DestroyWindow`
  - `SDL_QuitSubSystem(owned_flags)`
- Added baseline event handling for quit and drag-drop open-file request.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `include/render/vulkan_video_renderer.h`
- `src/render/vulkan_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY66_VK005_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT.md`
- `docs/design/CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_DESIGN_2026-03-27.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_PLAN_2026-03-27.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_LOCAL_CHECK.md`

## Issue 131: Vulkan chain VK-004 renderer skeleton and factory wiring

**Date**: 2026-03-26

### Problem Description
- Vulkan backend had no concrete renderer implementation unit, so strategy/factory could not wire a runtime target.

### Root Cause Analysis
- Missing enum/factory/capability/strategy integration for Vulkan renderer candidate.
- No placeholder backend existed for staged bring-up.

### Solution
- Added `VideoRendererType::Vulkan`.
- Added `VulkanVideoRenderer` skeleton:
  - `include/render/vulkan_video_renderer.h`
  - `src/render/vulkan_video_renderer.cpp`
- Wired Vulkan in:
  - `src/render/renderer_factory.cpp` (name/create)
  - `src/platform/platform_capabilities.cpp` (renderer support + priority)
  - `src/core/playback_strategy.cpp` (`vulkan|vk` env parse)
  - `src/main.cpp` renderer type display helper
- Skeleton behavior currently returns `init=false` intentionally to preserve staged fallback flow until `VK-005`.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
  - key lines:
    - `startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL`
    - `startup_renderer_fallback_reason=fallback-after-renderer-failure`

### Modified Files
- `include/render/video_renderer.h`
- `include/render/vulkan_video_renderer.h`
- `src/render/vulkan_video_renderer.cpp`
- `src/render/renderer_factory.cpp`
- `src/platform/platform_capabilities.cpp`
- `src/core/playback_strategy.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY65_VK004_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING.md`
- `docs/design/CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_LOCAL_CHECK.md`

## Issue 130: Vulkan chain VK-003 CMake switches and dependency detect

**Date**: 2026-03-26

### Problem Description
- Build system had no Vulkan renderer switch, dependency probe, or compile macro contract.

### Root Cause Analysis
- Existing feature matrix did not include Vulkan renderer path.
- No downgrade behavior existed for missing Vulkan package on Linux hosts.

### Solution
- Added CMake switch `ENABLE_VULKAN_RENDERER`:
  - Linux default `ON`
  - non-Linux default `OFF`
- Added platform guard + downgrade:
  - non-Linux forced OFF with warning
  - Linux `pkg-config vulkan` missing -> forced OFF with warning
- Added compile and source propagation:
  - `MVP_HAVE_VULKAN_RENDERER`
  - conditional inclusion of `src/render/vulkan_video_renderer.cpp`
- Extended renderer-presence guard to include Vulkan switch.

### Validation
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build-vk-on-win -G "Visual Studio 17 2022" -A x64 -DENABLE_VULKAN_RENDERER=ON` -> PASS
  - key lines:
    - warning: `ENABLE_VULKAN_RENDERER requires Linux/Unix non-Apple; forcing OFF`
    - feature switches include `VULKAN_RENDERER=OFF`
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS

### Modified Files
- `CMakeLists.txt`
- `docs/analysis/PLAYERCORE_DAY64_VK003_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT.md`
- `docs/design/CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_LOCAL_CHECK.md`

## Issue 129: Vulkan chain VK-002 architecture and strategy integration

**Date**: 2026-03-26

### Problem Description
- Vulkan task execution required an architecture-level integration contract before deeper implementation.

### Root Cause Analysis
- Without explicit boundary, Vulkan implementation could leak policy logic back into `PlayerCore`.

### Solution
- Frozen architecture integration via existing abstractions:
  - `PlatformCapabilities`
  - `PlaybackStrategy`
  - `RendererFactory`
  - `PlayerCore` plan-consume only
- Frozen Linux-first fallback contract:
  - `Vulkan -> OpenGL -> SoftwareSDL`

### Validation
- `rg -n "enum class VideoRendererType|VideoRendererType::|RendererFactory::create|RendererFactory::isSupported|MVP_RENDERER_BACKEND|renderer_candidates" include src CMakeLists.txt -S` -> PASS
- `$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS

### Modified Files
- `docs/analysis/PLAYERCORE_DAY63_VK002_ARCHITECTURE_AND_STRATEGY_INTEGRATION.md`
- `docs/design/CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_LOCAL_CHECK.md`

## Issue 128: Vulkan chain VK-001 scope and acceptance freeze

**Date**: 2026-03-26

### Problem Description
- Vulkan链路尚未开始实现，当前仓库没有 Vulkan renderer/build/diagnostics/gate contract。
- 如果不先冻结范围与验收标准，后续 `VK-002 ~ VK-014` 会发生目标漂移，导致实现和验收标准不一致。

### Root Cause Analysis
- 现有跨平台任务板已完成到 Linux/OpenGL/VAAPI 收口，但没有 Vulkan 子任务的仓库内冻结文档链路。
- 架构入口点（`PlaybackStrategy` / `RendererFactory` / Linux gate / CI lane）明确存在，但缺少 Vulkan 任务的统一 DoD。

### Solution
- 完成 `VK-001` 范围冻结，新增并同步以下文档：
  - analysis: `docs/analysis/PLAYERCORE_DAY62_VK001_SCOPE_AND_ACCEPTANCE_FREEZE.md`
  - design: `docs/design/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_DESIGN_2026-03-26.md`
  - plan: `docs/plans/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_PLAN_2026-03-26.md`
  - report: `docs/reports/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_LOCAL_CHECK.md`
- 冻结执行边界：
  - Linux-first, macOS deferred
  - fallback target: `Vulkan -> OpenGL -> SoftwareSDL`
  - diagnostics target: `--vulkan-diagnostics`
  - 顺序执行 `VK-001 ~ VK-014`，代码改动从 `VK-003` 开始

### Validation
- `rg -n "Vulkan|vulkan|ENABLE_VULKAN_RENDERER|MVP_HAVE_VULKAN_RENDERER|--vulkan-diagnostics|vk" include src tools .github/workflows CMakeLists.txt -S` -> PASS (no existing Vulkan path in baseline)
- `& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS

### Modified Files
- `docs/analysis/PLAYERCORE_DAY62_VK001_SCOPE_AND_ACCEPTANCE_FREEZE.md`
- `docs/design/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 127: Linux workflow Build Linux Release compile blocker closure

**Date**: 2026-03-26

### Problem Description
- GitHub Actions Linux lane failed in `Build Linux Release` while Windows lane passed.
- Failing runs included:
  - `23601824744` (`push`)
  - `23601841417` (`workflow_dispatch`)

### Root Cause Analysis
- `src/subtitle/libass_probe.cpp` used `std::max(0, event.Start)` and `std::max(0, event.Duration)`, which caused template deduction conflict on Linux (`int` vs `long long`).
- `src/render/opengl_video_renderer.cpp` had cross-platform class code referencing helper symbols that were only defined inside a Windows-only helper block.

### Solution
- `libass_probe`:
  - Added clamp helper to safely convert libass event timestamps to `int`.
- `opengl_video_renderer`:
  - Added Linux-visible (`#if !defined(_WIN32)`) helper/type block outside Windows-only section, including:
    - subtitle animated-run detection helpers
    - OpenGL present/HDR enums and env parsers
    - string trimming and swap-interval helpers
    - output display binding state + Linux resolver

### Validation
- `gh run view 23601824744 --job 68733664519 --log` -> failure root errors confirmed.
- `gh run view 23601841417 --json status,conclusion,event,jobs` -> Linux `Build Linux Release` failure confirmed.
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS

### Modified Files
- `src/subtitle/libass_probe.cpp`
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY61_LINUX_WORKFLOW_BUILD_ERROR_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 126: Workflow log remaining FFmpeg duration compatibility closure

**Date**: 2026-03-26

### Problem Description
- The `log` file still reported Linux compile failures after previous compatibility fixes.
- Remaining hard failure was old-FFmpeg `AVFrame` duration field mismatch:
  - `AVFrame has no member duration; did you mean pkt_duration`

### Root Cause Analysis
- `PlayerCore` decode output timing still read `frame->duration` directly.
- Existing FFmpeg compatibility layer covered channel-layout API drift but not frame-duration field drift.

### Solution
- Extended `include/media/ffmpeg_channel_layout_compat.h` with `frameDuration(const AVFrame*)`.
- Added compile-time branch:
  - newer FFmpeg -> `frame->duration`
  - older FFmpeg -> `frame->pkt_duration`
- Replaced direct frame duration access in `src/core/player_core.cpp` video/audio decode timing paths.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS

### Modified Files
- `include/media/ffmpeg_channel_layout_compat.h`
- `src/core/player_core.cpp`
- `docs/analysis/PLAYERCORE_DAY60_LOG_WORKFLOW_ERROR_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 125: Linux CI compatibility stabilization (FFmpeg/libass + gate workflow determinism)

**Date**: 2026-03-26

### Problem Description
- Cross-platform task matrix was marked complete for Windows/Linux (`CP-001 ~ CP-905`), but CI gate still failed on Linux and Windows lanes.
- Linux compile failed on FFmpeg channel-layout API drift and `libass` header path mismatch.
- Windows CI gate/packaging could fail when probe media fixture and plugin build output were absent in CI workspace.

### Root Cause Analysis
- Audio channel layout usage mixed old/new FFmpeg field assumptions directly in runtime code.
- `libass` include path was Linux-distribution dependent (`ass/ass.h` vs `libass/ass.h`).
- OpenGL color enum used `None`, which has macro collision risk on Linux stacks.
- CI workflow built only `modern-video-player` but packaging/install path also required `sample_logger_plugin`, and probe media presence was assumed.

### Solution
- Added `include/media/ffmpeg_channel_layout_compat.h` and migrated channel-layout access in:
  - `src/core/player_core.cpp`
  - `src/main.cpp`
  - `src/demuxer.cpp`
- Updated `PlayerCore` resampler state/initialization to support old/new swresample interfaces:
  - modern path: `swr_alloc_set_opts2`
  - legacy path: `swr_alloc_set_opts`
- Fixed Linux compile blockers:
  - `src/subtitle/libass_probe.cpp` include fallback (`ass/ass.h` first)
  - `src/render/opengl_video_renderer.cpp` enum rename (`None` -> `Disabled`)
- Hardened CI workflow `.github/workflows/cross-platform-gate.yml`:
  - generate probe media fixture when missing (Windows + Linux lanes)
  - build `sample_logger_plugin` together with `modern-video-player`

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)

### Modified Files
- `include/media/ffmpeg_channel_layout_compat.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/demuxer.cpp`
- `src/subtitle/libass_probe.cpp`
- `src/render/opengl_video_renderer.cpp`
- `.github/workflows/cross-platform-gate.yml`
- `docs/analysis/PLAYERCORE_DAY59_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION.md`
- `docs/design/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 124: Linux gate reporting/artifact closure for CI evidence reuse

**Date**: 2026-03-26

### Problem Description
- Linux gate checks were strict, but evidence output was mainly console text.
- CI lacked a dedicated machine-readable Linux gate summary artifact for downstream review/automation.

### Root Cause Analysis
- `tools/run_linux_mvp_checks.sh` had no report-file output contract.
- Linux CI gate step did not tee gate logs to dedicated artifact files.

### Solution
- Added Linux gate report contract in `tools/run_linux_mvp_checks.sh`:
  - arg #10 / env `MVP_LINUX_GATE_REPORT_FILE`
  - per-check fields `check.<id>.*` and global `gate.*`
  - explicit fail result/reason recording via unified fail path
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - enforce `set -euo pipefail`
  - tee output to `logs/linux-mvp-gate.log`
  - write summary to `logs/linux-mvp-gate-summary.env`
  - upload `logs/*.env` artifacts
- Synced master tasklist/docs indexes and round analysis/design/plan/report documents.

### Validation
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `.\build\Release\modern-video-player.exe --d3d11-diagnostics` -> PASS

### Modified Files
- `tools/run_linux_mvp_checks.sh`
- `.github/workflows/cross-platform-gate.yml`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY58_LINUX_GATE_REPORTING_AND_CI_ARTIFACT_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_GATE_REPORTING_AND_ARTIFACT_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_GATE_REPORTING_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_GATE_REPORTING_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 123: Linux gate strict optional checks closure for CP-507/CP-508 in CI

**Date**: 2026-03-26

### Problem Description
- Linux gate script accepted `CP-507` and `CP-508` as optional checks, but `CP-508` depended on a non-versioned fixture file (`build/tmp/embedded-ass-validation.mkv`).
- In CI Linux runs, missing fixture could silently skip `CP-508`, reducing deterministic subtitle backlog coverage.

### Root Cause Analysis
- No embedded ASS fixture auto-generation path existed in `tools/run_linux_mvp_checks.sh`.
- CI Linux dependency setup did not install `ffmpeg` binary for fixture generation.
- No strict mode existed to force optional checks as required in CI.

### Solution
- Updated `tools/run_linux_mvp_checks.sh`:
  - added `ensure_embedded_ass_sample(...)` with `ffmpeg`-based fixture generation
  - added strict optional-check switch (`REQUIRE_OPTIONAL_CHECKS`, arg #7 or `MVP_REQUIRE_OPTIONAL_CHECKS`)
  - added generation input args #8/#9 (base media + ASS subtitle)
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - install `ffmpeg`
  - invoke Linux gate in strict mode with explicit CP-508 fixture paths
- Synced tasklist/docs indexes and round docs for analysis/design/plan/report.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120` -> PASS
- `C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh` -> PASS (syntax check)
- `C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh` -> expected FAIL on Windows host (`This gate script only supports Linux.`)


### Modified Files
- `tools/run_linux_mvp_checks.sh`
- `.github/workflows/cross-platform-gate.yml`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY57_LINUX_GATE_STRICT_OPTIONAL_CHECKS_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/plans/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 120: CP-501 ~ CP-506 subtitle/font platform closure

**Date**: 2026-03-26

### Problem Description
- Phase-5 tasks (`CP-501`~`CP-506`) were partially landed but not fully closed:
  - embedded subtitle policy flow had integration regression in `main.cpp`;
  - Linux attachment-font runtime path still lacked registration/cleanup closure;
  - DirectWrite custom font collection check was not included in Windows gate coverage.
- Phase-4 local report still had pending placeholders.

### Root Cause Analysis
- `main.cpp` attempted to mutate `const AppSettings`, causing compile failure after policy merge wiring.
- `subtitle_font_registry` only implemented private-font registration on Windows; non-Windows branch always returned failure.
- `tools/run_opengl_checks.ps1` had no explicit `--directwrite-font-collection-check` stage.

### Solution
- Fixed compile blocker in `src/main.cpp` by removing invalid write to `const app_settings`.
- Completed Linux fontconfig closure in `src/subtitle/subtitle_font_registry.cpp`:
  - add-file registration (`FcConfigAppFontAddFile`) + font rebuild (`FcConfigBuildFonts`);
  - release-time app-font collection rebuild from remaining registry files.
- Added DirectWrite custom font collection gate stage in `tools/run_opengl_checks.ps1`.
- Synced Phase5 docs (`analysis/design/report/plans`) and updated Phase5 statuses in master tasklist.
- Updated `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md` with actual local execution results.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid` -> PASS
- `.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --settings-persistence-check` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"` -> `OpenGL gate result: PASS` (`18/18`)


### Modified Files
- `src/main.cpp`
- `src/subtitle/subtitle_font_registry.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/CROSS_PLATFORM_PHASE5_SUBTITLE_FONT_CLOSURE_PLAN_2026-03-26.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY51_CP501_CP506_SUBTITLE_FONT_PLATFORM_CLOSURE.md`
- `docs/analysis/README.md`
- `docs/design/CROSS_PLATFORM_SUBTITLE_FONT_PLATFORM_CLOSURE_DESIGN_2026-03-26.md`
- `docs/design/README.md`
- `docs/reports/CROSS_PLATFORM_PHASE5_LOCAL_CHECK.md`
- `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 119: CP-401 ~ CP-406 Linux MVP playback closure and gate commands

**Date**: 2026-03-26

### Problem Description
- Phase-4 Linux MVP tasks (`CP-401`~`CP-406`) were still open and not connected as a complete machine-readable gate.
- Only partial `linux-software-audio-check` scaffolding existed; command usage and CLI dispatch were incomplete.
- No deterministic OpenGL init fail injection existed for fallback-chain verification.

### Root Cause Analysis
- Existing checks were mostly Windows/OpenGL focused and did not provide a Linux MVP closure command set.
- Fallback verification relied on incidental failures rather than a controlled injection mechanism.
- Linux gate script baseline was missing.

### Solution
- Added complete Phase-4 command set in `src/main.cpp`:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added command usage lines and CLI argument dispatch for all above commands.
- Added OpenGL force-fail injection in `src/render/opengl_video_renderer.cpp`:
  - `MVP_OPENGL_FORCE_INIT_FAIL` (truthy values force init failure).
- Added Linux gate script:
  - `tools/run_linux_mvp_checks.sh`
- Updated Phase4 status and command baseline in task planning docs.

### Validation
- `cmake -S . -B build` -> PASS
- `cmake --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
- Linux-only Phase4 commands are implemented and dispatchable, but full PASS verification remains pending Linux host execution.


### Modified Files
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `tools/run_linux_mvp_checks.sh`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY50_CP401_CP406_LINUX_MVP_PLAYBACK_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_LINUX_MVP_PLAYBACK_GATE_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE4_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 118: CP-301 ~ CP-305 build switch platformization and Linux packaging baseline

**Date**: 2026-03-26

### Problem Description
- Cross-platform backend composition still lacked explicit CMake feature switches.
- Platform-specific source boundaries were not fully controlled by build-time backend switches.
- Startup diagnostics did not explicitly separate compiled backend set from runtime-available set.
- Linux dependency closure and packaging baseline path were not codified in build/packaging config.

### Root Cause Analysis
- Capability compile macros were hardcoded and not directly driven by user-visible build options.
- Renderer source inclusion and diagnostics command paths were not fully switch-aware.
- Linux package generator/dependency baseline had no standardized CPack path.

### Solution
- Added explicit CMake switches:
  - `ENABLE_D3D11_RENDERER`, `ENABLE_OPENGL_RENDERER`, `ENABLE_SDL_RENDERER`
  - `ENABLE_D3D11VA`, `ENABLE_DXVA2`, `ENABLE_VAAPI`, `ENABLE_VIDEOTOOLBOX`
- Added platform force-off rules and effective `MVP_HAVE_*` compile definitions.
- Made renderer source inclusion/link dependencies switch-aware and platform-guarded.
- Extended startup diagnostics with:
  - `startup_renderer_compiled_set`
  - `startup_renderer_runtime_set`
  - `startup_decoder_compiled_set`
  - `startup_decoder_runtime_set`
- Added Linux dependency closure requirements (`libass/fontconfig/freetype/OpenGL`) and Linux package baseline:
  - CPack generator: `DEB;TGZ`
  - helper script: `tools/package_linux.sh`

### Validation
- Default configure/build:
  - `cmake -S . -B build` -> PASS
  - `cmake --build build --config Release --target modern-video-player` -> PASS
- Default runtime checks:
  - `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
  - `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
  - `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS`
- Switch matrix builds:
  - `build_nod3d11` (`ENABLE_D3D11_RENDERER=OFF`, `ENABLE_D3D11VA=OFF`) build PASS
  - `build_noopengl` (`ENABLE_OPENGL_RENDERER=OFF`) build PASS
- Command behavior under switch matrix:
  - `build_nod3d11 --d3d11-diagnostics` reports unsupported without linker/runtime crash.
  - `build_noopengl --opengl-diagnostics` reports unsupported without linker/runtime crash.


### Modified Files
- `CMakeLists.txt`
- `include/decoder/decoder_capability.h`
- `src/decoder/decoder_factory.cpp`
- `src/platform/platform_capabilities.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `tools/package_linux.sh`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/analysis/PLAYERCORE_DAY49_CP301_CP305_BUILD_SWITCH_GUARD_AND_PACKAGING_BASELINE.md`
- `docs/design/CROSS_PLATFORM_BUILD_SWITCH_AND_PACKAGING_BASELINE_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE3_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 117: CP-201 ~ CP-205 renderer/input/overlay responsibility cleanup and interaction freeze gate

**Date**: 2026-03-26

### Problem Description
- Renderer abstraction still exposed input and overlay responsibilities mixed with rendering concerns.
- Event-pump thread affinity constraints were implicit and could regress into interaction freeze/deadlock behavior.
- No machine-readable dedicated check existed for interaction freeze scenarios (mouse/hotkey/window-event stress).

### Root Cause Analysis
- `IVideoRenderer` historically accumulated non-render responsibilities.
- `PlayerCore` and idle startup window consumed renderer command APIs directly, coupling control flow to renderer internals.
- Event handling call paths lacked explicit guardrails for non-owner thread invocation.

### Solution
- Introduced role interfaces:
  - `input::IPlaybackInputSource`
  - `render::IRenderOverlaySink`
- Shrank `IVideoRenderer` to rendering-focused contract only.
- Refactored `PlayerCore` and idle-window path to consume input/overlay roles via interface cross-cast.
- Added main-thread guard handling for event pumping in `PlayerCore`, `Display`, `D3D11VideoRenderer`, and `OpenGLVideoRenderer`.
- Added machine-readable `--interaction-freeze-check` and integrated it into `tools/run_opengl_checks.ps1`.

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS
- `.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS`


### Modified Files
- `include/input/playback_input_source.h`
- `include/render/render_overlay_sink.h`
- `include/render/video_renderer.h`
- `include/render/sdl_video_renderer.h`
- `include/render/d3d11_video_renderer.h`
- `include/render/opengl_video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `include/display.h`
- `src/display.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/analysis/PLAYERCORE_DAY48_CP201_CP205_RENDERER_DISPLAY_INPUT_CLEANUP.md`
- `docs/design/CROSS_PLATFORM_RENDER_INPUT_OVERLAY_BOUNDARY_DESIGN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE2_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 116: CP-101 ~ CP-106 cross-platform startup strategy extraction

**Date**: 2026-03-25

### Problem Description
- `PlayerCore` startup path still mixed platform policy and execution logic.
- `RendererFactory`/`DecoderFactory` still held default policy responsibilities.
- Startup strategy lacked machine-readable diagnostics (capabilities/candidates/selected/fallback reason).

### Root Cause Analysis
- No unified capability model for platform/backend availability.
- No dedicated strategy object for open-time planning.
- Decoder selection inputs were too narrow to evolve safely across platforms.

### Solution
- Added `platform_capabilities` abstraction and probe (`CP-101`).
- Added `playback_strategy` abstraction and open plan generation (`CP-102`).
- Refactored `RendererFactory` to support-check + creation only (`CP-103`).
- Refactored `DecoderFactory` to context-driven ordering with mandatory software fallback (`CP-104`).
- Refactored `PlayerCore::open()`/decoder init to consume startup plan (`CP-105`).
- Added machine-readable startup strategy diagnostics exported by `--performance-log-check` (`CP-106`).

### Validation
- `C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4` -> PASS


### Modified Files
- `include/platform/platform_capabilities.h`
- `src/platform/platform_capabilities.cpp`
- `include/core/playback_strategy.h`
- `src/core/playback_strategy.cpp`
- `include/render/renderer_factory.h`
- `src/render/renderer_factory.cpp`
- `include/decoder/decoder_factory.h`
- `src/decoder/decoder_factory.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `CMakeLists.txt`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/PHASE1_CROSS_PLATFORM_TODO.md`
- `docs/analysis/PLAYERCORE_DAY47_CP101_CP106_STRATEGY_AND_CAPABILITY_EXTRACTION.md`
- `docs/design/CROSS_PLATFORM_STARTUP_STRATEGY_AND_CAPABILITIES_DESIGN_2026-03-25.md`
- `docs/reports/CROSS_PLATFORM_STRATEGY_LOCAL_CHECK.md`
- `docs/analysis/README.md`
- `docs/design/README.md`
- `docs/reports/README.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 115: Cross-platform Phase 8 ICC output LUT and per-display diagnostics closure

**Date**: 2026-03-26

### Problem Description
- Phase 8 still lacked ICC/profile-driven LUT generation, per-display output binding, and runtime diagnostics closure.
- OpenGL output color regression still validated only the manual `.cube` path.
- `CP-801` remained open, but the runtime/output model required by the rest of Phase 8 was still missing.

### Root Cause Analysis
- Output LUT initialization was static and only consumed `MVP_OPENGL_3DLUT_FILE`.
- There was no display-binding model for the SDL window's current display / monitor ICC profile.
- Diagnostics could not explain which display, ICC profile, or LUT source was active during playback.

### Solution
- Added reusable output-color helper with `.cube` parsing and ICC matrix/TRC profile -> sampled 3D LUT generation.
- Added OpenGL runtime display binding and Windows ICC profile discovery.
- Added output LUT source priority:
  - manual `.cube`
  - manual ICC profile
  - auto ICC from current display
  - none
- Added new CLI / env surfaces:
  - `MVP_OPENGL_ICC_PROFILE_FILE`
  - `MVP_OPENGL_AUTO_ICC`
  - `--opengl-icc-profile <icc_profile_file>`
  - `--opengl-auto-icc`
  - `--opengl-output-color-icc-check <media_file> [sample_ms]`
- Extended diagnostics and OpenGL gate coverage with display / ICC / LUT runtime fields.
- Marked `CP-802 ~ CP-805` done in the master tasklist; kept `CP-801` open.

### Validation
- `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-diagnostics` -> PASS
- `$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path; powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'` -> `OpenGL gate result: PASS` (`25/25`)


### Modified Files
- `include/render/output_color_profile.h`
- `src/render/output_color_profile.cpp`
- `src/render/opengl_video_renderer.cpp`
- `include/render/opengl_video_renderer.h`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY53_CP802_CP805_ICC_OUTPUT_BINDING_AND_OUTPUT_DIAGNOSTICS.md`
- `docs/design/CROSS_PLATFORM_OUTPUT_COLOR_PROFILE_BINDING_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE8_OUTPUT_COLOR_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE8_LOCAL_CHECK.md`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 114: Cross-platform Phase 6 bitmap subtitle pipeline closure

**Date**: 2026-03-26

### Problem Description
- Phase 6 bitmap subtitle baseline existed, but packet-level timeline modeling, cache reuse, and dedicated regression coverage were still incomplete.
- Real PGS samples could surface invalid display-window metadata, causing pathological subtitle duration expansion.
- Existing diagnostics could not expose rect-level and multi-rect bitmap subtitle behavior.

### Root Cause Analysis
- Bitmap subtitle loader still modeled each rect as an independent `SubtitleItem`, which lost packet/event grouping.
- Renderer bitmap branches recreated premultiplied payloads and D2D bitmap resources inline without reuse.
- Phase 6 had no dedicated CLI / gate path for PGS/DVD bitmap subtitles and multi-rect stress.
- Bitmap timeline fallback trusted `end_display_time` too eagerly.

### Solution
- Switched bitmap subtitle modeling to packet-level `SubtitleItem.bitmap_rects` aggregation.
- Extended `EmbeddedSubtitleLoadResult` with `bitmap_rect_count`, `bitmap_multi_rect_item_count`, and `bitmap_max_rects_per_item`.
- Added bitmap display-window sanity fallback for invalid / oversized `start_display_time` / `end_display_time`.
- Added renderer bitmap cache/reuse in OpenGL and D3D11 subtitle D2D paths.
- Added CLI:
  - `--bitmap-subtitle-check <media_file> [stream_index]`
  - `--bitmap-subtitle-stress-check`
- Extended `tools/run_opengl_checks.ps1` with DVD / PGS / stress bitmap regressions.

### Validation
- `& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check` -> PASS
- `$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path; powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'` -> `OpenGL gate result: PASS` (`23/23`)


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/analysis/PLAYERCORE_DAY52_CP601_CP605_BITMAP_SUBTITLE_PIPELINE_CLOSURE.md`
- `docs/design/CROSS_PLATFORM_BITMAP_SUBTITLE_PIPELINE_DESIGN_2026-03-26.md`
- `docs/plans/CROSS_PLATFORM_PHASE6_BITMAP_SUBTITLE_PLAN_2026-03-26.md`
- `docs/reports/CROSS_PLATFORM_PHASE6_LOCAL_CHECK.md`
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 112: OpenGL interaction freeze on mouse/keyboard/window events

**Date**: 2026-03-25

### Problem Description
- OpenGL playback could freeze after mouse move/click interaction.
- After freeze, hotkeys and window operations (maximize/minimize/fullscreen) became unresponsive.
- This looked like a hard UI stall instead of normal performance backpressure.

### Root Cause Analysis
- OpenGL renderer pumped SDL events on the render thread (`renderLoop -> pumpEvents()`), while the control loop and window lifecycle lived on the main thread.
- Under Windows event-heavy interaction, this thread-affinity split can stall the message path and produce apparent deadlock behavior.

### Solution
- Moved OpenGL SDL event pumping to `handleEvents()` (main-thread call path via `PlayerCore::pumpEvents()`).
- Removed `pumpEvents()` from the render thread loop.
- Moved fullscreen toggle execution (`SDL_SetWindowFullscreen`) to `handleEvents()` after event pumping.
- Kept render thread focused on frame processing/present and request-driven redraw.

### Validation
- `cmake --build build --config Release --target modern-video-player` (VS CMake path) -> PASS
- `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY45_OPENGL_EVENT_THREAD_AFFINITY_FREEZE_FIX.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 111: OpenGL HDR output policy + 3D LUT output baseline

**Date**: 2026-03-25

### Problem Description
- OpenGL still lacked an explicit output-color control plane for HDR-mode policy and post-output LUT handling.
- HDR/SDR output behavior could not be explicitly selected for playback diagnostics beyond internal tone-map logic.
- There was no dedicated machine-readable regression command to verify OpenGL output-stage HDR/LUT wiring.

### Root Cause Analysis
- Existing OpenGL HDR logic primarily lived inside per-frame color conversion/tone-map decisions.
- Renderer diagnostics did not expose enough output-stage policy signals to distinguish request vs active state.
- ICC/LUT backlog had design docs, but runtime plumbing for a practical LUT path was still missing.

### Solution
- Added OpenGL HDR output mode policy controls:
  - env: `MVP_OPENGL_HDR_OUTPUT_MODE=auto|off|force`
  - CLI: `--opengl-hdr-output-mode <auto|off|force>`
- Added OpenGL output 3D LUT controls:
  - env: `MVP_OPENGL_3DLUT_FILE=<cube_lut_file>`
  - CLI: `--opengl-3dlut <cube_lut_file>`
- Implemented `.cube` parser + OpenGL 3D LUT texture upload and output-stage sampling.
- Extended diagnostics chain (`RendererDiagnostics` -> `DiagnosticsSnapshot` -> `--performance-log-check`) with:
  - `renderer_opengl_hdr_bridge_*`
  - `renderer_opengl_output_lut_*`
- Added machine-readable regression command:
  - `--opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`

### Validation
- `cmake --build build --config Release --target modern-video-player` -> PASS
- `.\build\Release\modern-video-player.exe --opengl-output-color-check .\juren-30s.mp4 .\samples\lut\identity_2.cube 1200` -> PASS
- `$env:MVP_RENDERER_BACKEND='opengl'; $env:MVP_OPENGL_3DLUT_FILE='.\samples\lut\identity_2.cube'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/render/opengl_video_renderer.cpp`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY44_OPENGL_HDR_OUTPUT_POLICY_AND_3DLUT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 110: Embedded bitmap subtitle path and DirectWrite subtitle font collection

**Date**: 2026-03-25

### Problem Description
- Embedded subtitle multi-track switching and text playback were available, but bitmap subtitle codecs (PGS/DVD) were still outside the rendering path.
- Private subtitle fonts were registered, but renderers still lacked an explicit DirectWrite custom font collection binding.
- CLI diagnostics could not clearly expose bitmap-track coverage and custom font collection readiness.

### Root Cause Analysis
- Embedded subtitle policy and documentation still partially treated track selection as text-only.
- The embedded loader lacked a bitmap decode branch from `AVSubtitleRect` to renderer model payload.
- OpenGL/D3D11 subtitle loops handled text-only rendering paths.
- Font registration and DirectWrite collection construction were not connected end-to-end.

### Solution
- Extended embedded subtitle selection policy to supported codecs (`supported_codec`) instead of text-only.
- Added embedded bitmap subtitle decode path for:
  - `AV_CODEC_ID_HDMV_PGS_SUBTITLE`
  - `AV_CODEC_ID_DVD_SUBTITLE`
- Added `SubtitleBitmap` model fields and renderer bitmap branches in both OpenGL and D3D11 subtitle renderers.
- Added DirectWrite custom subtitle font collection builder from registered private fonts and integrated it into both subtitle renderers.
- Added/extended diagnostics:
  - `--directwrite-font-collection-check <media_file>`
  - `embedded-subtitle-check.bitmap_codec`
  - `embedded-subtitle-check.bitmap_item_count`
  - `embedded-subtitle-list.supported_bitmap_track_count`
  - `embedded-subtitle-select-check.bitmap_codec`
  - `embedded-subtitle-select-check.bitmap_item_count`

### Validation
- `MSBuild.exe build/modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2` -> PASS
- `.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\embedded-ass-validation.mkv` -> PASS
- `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"` -> `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY43_BITMAP_SUBTITLE_AND_DWRITE_COLLECTION.md`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

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
- OpenGL present pacing had been fixed, but鐜板満鎺掗殰 still needed a supported way to switch between paced and immediate present behavior.
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
## Issue 109: Embedded subtitle multi-track selection UI + CLI closure

**Date**: 2026-03-25

### Problem Description
- Embedded subtitle auto-load was already available, but multi-track selection was still not closed as a player feature.
- OpenGL bottom bar lacked subtitle-track switching controls.
- CLI lacked machine-readable list/select checks for embedded subtitle streams.

### Root Cause Analysis
- `PlayerCore` had embedded subtitle loading capability, but cross-layer control surface (renderer interaction + playback CLI + regression CLI) was incomplete.
- OpenGL control layout had placeholder fields for subtitle-track buttons, but draw/hit-test/request-consume links were not fully implemented.
- Existing diagnostics only covered "best-stream auto-load", not explicit stream listing/selection.

### Solution
- Completed OpenGL subtitle-track controls:
  - previous/next subtitle track buttons
  - subtitle track state text (`current / total`)
  - request consumption and PlayerCore event pump linkage
- Added renderer state push:
  - `setSubtitleTrackState(int current_ordinal, int track_count)`
- Added playback CLI:
  - `--subtitle-track <stream_index>`
- Added diagnostic CLI:
  - `--embedded-subtitle-list <media_file>`
  - `--embedded-subtitle-select-check <media_file> <stream_index>`
- Refined `PlayerCore` overlay-state count to supported selectable subtitle tracks (`supported_codec`) instead of raw subtitle stream count.

### Validation
- Build:
  - `cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && msbuild build\modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64"`
  - Result: PASS
- CLI:
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-text-validation.mp4`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2`
  - `.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-text-validation.mp4 2`
  - Results: PASS
- Gate:
  - `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"`
  - Result: `OpenGL gate result: PASS` (`16/16 PASS`)


### Modified Files
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 113: Cross-platform master tasklist consolidation for direct execution
**Date**: 2026-03-25

### Problem Description
- Cross-platform tasks were split across multiple plan documents, making direct execution inefficient.
- There was no single document containing full task IDs, execution order, milestone gates, and acceptance criteria.

### Root Cause Analysis
- Existing docs were designed for different scopes (roadmap / refactor list / phase TODO), not a unified execution board.
- Recent progress changed baseline assumptions and required a consolidated up-to-date master list.

### Solution
- Added `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md` with:
  - phase-based task matrix
  - `CP-xxx` task IDs
  - `DONE/NEXT/LATER` status
  - milestones (`M1..M5`) and acceptance criteria
- Added analysis companion:
  - `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- Rebuilt `docs/plans/README.md` as clean index and set master tasklist as default execution entry.

### Validation
- `rg -n "CROSS_PLATFORM_MASTER_TASKLIST" docs/plans docs/analysis docs/records` -> PASS
- `cmake --build build --config Release --target modern-video-player` -> PASS


### Modified Files
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`








