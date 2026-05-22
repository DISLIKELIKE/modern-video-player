# DEVELOP_LOG

## 索引说明（2026-03-26 编码清理批次）

- 本轮仅清理 `records/readme` 索引范围，不批量改写历史日志正文。
- 最新开发日志条目位于文件顶部（`Issue 183` 到 `Issue 122`）。
- 历史段落若出现旧编码乱码，将在后续专题批次逐步处理。

## Issue 184: PlayerCore worker thread consolidation

**Date**: 2026-04-10
**Status**: Resolved

### Description
- Added a shared `core::WorkerThread` helper for current mainline loop workers.
- Migrated `PlayerCore` `demux` and `audio consumer` worker ownership to the new helper.
- Removed the unused legacy `DecoderThread` implementation from the build.

### Log
```text
Planner:
- docs/plans/PLAYERCORE_WORKER_THREAD_CONSOLIDATION_PLAN_2026-04-10.md

Code changes:
1) include/core/worker_thread.h
2) src/core/worker_thread.cpp
3) include/core/player_core.h
4) src/core/player_core.cpp
5) CMakeLists.txt
6) remove include/core/decoder_thread.h
7) remove src/core/decoder_thread.cpp

Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

Runtime:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Result: PASS
Key lines:
- performance-log-check.open_ok=true
- performance-log-check.entered_playback_loop=true
- performance-log-check.audio_output_initialized=true
- performance-log-check.scheduler_video_restart_attempts=0
- performance-log-check.scheduler_audio_restart_attempts=0
- performance-log-check.result=PASS
```

### Notes
1. This round only consolidates `PlayerCore` worker ownership; it does not refactor `Scheduler` or renderer-side threads.
2. Queue semantics (`ThreadSafeQueue` / `FrameQueue`) were intentionally left unchanged to keep the refactor behavior-neutral.
3. Build still reports unrelated historical source-encoding warnings from `src/video_player.cpp`.

## Issue 183: Vulkan chain VK-053 Windows auto optional sdk-missing canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic sdk-missing downgrade canary for Windows Vulkan auto policy.
- Integrated canary into `run_windows_ci_gate.ps1` with Step Summary reporting.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1
   - new scenario:
     auto + sdk=0 + native_probe=1 + swiftshader_probe=0
   - key assertions:
     gate_mode=optional
     gate_strict_mode_effective=false
     gate_strict_mode_auto_prerequisites_met=false
     gate_strict_mode_auto_runtime_probe_any_available=true
     gate_strict_mode_auto_runtime_probe_source=native
     gate_result=SKIPPED
     result=PASS

2) tools/run_windows_ci_gate.ps1
   - add sdk-missing canary execution
   - add Step Summary table:
     Windows Vulkan Gate Auto Optional SDK-Missing Canary

SDK-missing canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1
Result: PASS
Key lines:
- windows-vulkan-auto-optional-sdk-missing-canary.gate_mode=optional
- windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_prerequisites_met=false
- windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_any_available=true
- windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_source=native
- windows-vulkan-auto-optional-sdk-missing-canary.result=PASS
```

### Notes
1. Auto-policy SDK prerequisite boundary is now explicitly canary-protected.
2. This round is branch-coverage hardening; production gate policy remains unchanged.
3. End-to-end strict PASS runtime evidence still depends on GitHub Windows runner execution.

## Issue 182: Vulkan chain VK-052 Windows auto strict dual-probe canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic dual-probe strict canary for Windows Vulkan auto policy.
- Integrated canary into `run_windows_ci_gate.ps1` with Step Summary reporting.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1
   - new scenario:
     auto + sdk=1 + native_probe=1 + swiftshader_probe=1
   - key assertions:
     gate_mode=strict
     gate_strict_mode_effective=true
     gate_strict_mode_auto_runtime_probe_source=native+swiftshader
     gate_result=PASS
     result=PASS

2) tools/run_windows_ci_gate.ps1
   - add dual-probe canary execution
   - add Step Summary table:
     Windows Vulkan Gate Auto Strict Dual-Probe Canary

Dual-probe canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1
Result: PASS
Key lines:
- windows-vulkan-auto-strict-dual-probe-canary.gate_mode=strict
- windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_runtime_probe_source=native+swiftshader
- windows-vulkan-auto-strict-dual-probe-canary.result=PASS
```

### Notes
1. Auto-policy runtime-probe source classification now has dedicated canary coverage for all states.
2. This round is branch-coverage hardening; production gate policy remains unchanged.
3. End-to-end strict PASS runtime evidence still depends on GitHub Windows runner execution.

## Issue 181: Vulkan chain VK-051 Windows auto optional no-probe canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic no-probe downgrade canary for Windows Vulkan auto policy.
- Integrated new canary into `run_windows_ci_gate.ps1` and Step Summary output.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1
   - new scenario:
     auto + sdk=1 + native_probe=0 + swiftshader_probe=0
   - key assertions:
     gate_mode=optional
     gate_strict_mode_effective=false
     gate_strict_mode_auto_prerequisites_met=false
     gate_strict_mode_auto_runtime_probe_source=none
     gate_result=SKIPPED
     result=PASS

2) tools/run_windows_ci_gate.ps1
   - add no-probe auto canary execution
   - add Step Summary table:
     Windows Vulkan Gate Auto Optional No-Probe Canary

No-probe canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1
Result: PASS
Key lines:
- windows-vulkan-auto-optional-no-probe-canary.gate_mode=optional
- windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_prerequisites_met=false
- windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_runtime_probe_source=none
- windows-vulkan-auto-optional-no-probe-canary.result=PASS

Regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1
Result: PASS

Regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
Result: PASS
```

### Notes
1. Windows Vulkan auto-policy canary matrix now covers no-probe downgrade and both strict promotion sources.
2. This round is branch-coverage/observability hardening; production policy logic is unchanged.
3. End-to-end strict PASS runtime evidence still depends on GitHub Windows runner execution.

## Issue 180: Vulkan chain VK-050 Windows auto strict native runtime probe canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic native runtime-probe canary coverage for Windows Vulkan auto strict policy.
- Integrated native auto strict canary into `run_windows_ci_gate.ps1` and Step Summary.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1
   - new canary scenario:
     auto + sdk=1 + native_probe=1 + swiftshader_probe=0
   - key assertions:
     gate_mode=strict
     gate_strict_mode_effective=true
     gate_strict_mode_auto_runtime_probe_source=native
     result=PASS

2) tools/run_windows_ci_gate.ps1
   - add native auto strict canary execution
   - add Step Summary table:
     Windows Vulkan Gate Auto Strict Native Probe Canary

Native canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1
Result: PASS
Key lines:
- windows-vulkan-auto-strict-native-probe-canary.gate_mode=strict
- windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_runtime_probe_source=native
- windows-vulkan-auto-strict-native-probe-canary.result=PASS

Regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
Result: PASS

Regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1
Result: PASS
```

### Notes
1. Windows Vulkan auto strict probe-source branches now have dedicated canary coverage for both native and SwiftShader paths.
2. This round is canary/observability hardening; gate behavior remains unchanged.
3. End-to-end strict PASS runtime evidence still depends on GitHub Windows runner execution.

## Issue 179: Vulkan chain VK-049 Windows auto strict SwiftShader runtime probe promotion

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Promoted Windows Vulkan auto strict mode to include SwiftShader runtime probe path.
- Added probe-source observability fields for strict auto decisions.
- Added dedicated canary and wired it into `run_windows_ci_gate.ps1`.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_checks.ps1
   - strict auto prerequisite:
     sdk && (native_runtime_probe || swiftshader_runtime_probe)
   - strict_mode_auto_basis:
     sdk_and_runtime_probe_or_swiftshader_probe
   - new summary fields:
     strict_mode_auto_runtime_probe_any_available
     strict_mode_auto_runtime_probe_source

2) tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
   - new canary:
     auto + sdk=1 + native_probe=0 + swiftshader_probe=1
   - assertions:
     mode=strict
     strict_mode_effective=true
     strict_mode_auto_runtime_probe_source=swiftshader
     result=PASS

3) tools/run_windows_ci_gate.ps1
   - baseline summary rows add strict auto probe-source fields
   - add new canary stage + Step Summary section

Static scan:
rg -n "sdk_and_runtime_probe_or_swiftshader_probe|strict_mode_auto_runtime_probe_any_available|strict_mode_auto_runtime_probe_source|auto_strict_swiftshader_probe_canary" tools/run_windows_vulkan_checks.ps1 tools/run_windows_ci_gate.ps1 tools/run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
Result: PASS

New canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1
Result: PASS
Key lines:
- windows-vulkan-auto-strict-swiftshader-probe-canary.gate_mode=strict
- windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_runtime_probe_source=swiftshader
- windows-vulkan-auto-strict-swiftshader-probe-canary.result=PASS

Regression canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1
Result: PASS

Regression canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1
Result: PASS
```

### Notes
1. This closes the policy gap where SwiftShader runtime readiness could not promote `auto` strict mode.
2. Existing explicit strict controls (CLI/env) are unchanged.
3. End-to-end strict PASS runtime evidence still depends on GitHub Windows runner execution.

## Issue 178: Forced FailSession deferred release and headless logging override

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Moved `FailSession` final teardown off worker threads and into deferred stop servicing.
- Stabilized forced FailSession canary timing for immediate injected failure.
- Added local/headless logger override `MVP_DISABLE_QUILL_LOGGING=1` so CLI validation can run without Quill console-handle dependency.

### Log
```text
Code changes:
1) include/core/player_core.h
   - add DeferredFailSessionState
   - add arm/consume/clear helpers

2) src/core/player_core.cpp
   - FailSession now arms deferred failure state and requests stop only
   - serviceDeferredStop() now performs session release + Failed-state closure
   - open()/close() clear stale deferred fail-session state

3) src/main.cpp
   - forced FailSession check now treats observed injected failure as valid playback-path entry

4) src/logger.cpp
   - add MVP_DISABLE_QUILL_LOGGING=1 env override

Build:
cmake --build build --config Debug --target modern-video-player
Result: PASS

Forced FailSession check:
$env:MVP_DISABLE_QUILL_LOGGING='1'
$env:SDL_AUDIODRIVER='dummy'
.\build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200
Result: PASS
Key lines:
- forced-failsession-check.entered_playback_loop=true
- forced-failsession-check.fail_session_observed=true
- forced-failsession-check.stopped_after_failure=true
- forced-failsession-check.runtime_failure_stop_requests=1
- forced-failsession-check.runtime_failure_fail_sessions=1
- forced-failsession-check.result=PASS

Aggregate check:
$env:MVP_DISABLE_QUILL_LOGGING='1'
$env:SDL_AUDIODRIVER='dummy'
powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath 'build/Debug/modern-video-player.exe' -ProbeFile 'juren-30s.mp4' -SamplesFile 'tools/format_regression/format_samples_ci.csv' -RegressionOutputFile 'build/FORMAT_REGRESSION_CI_LOCAL_TEST.md' -ProbeTimeoutSec 120 -ForcedFailSessionTimeoutSec 240 -RegressionTimeoutSec 900
Result: PASS
Key lines:
- Probe exit code: 0
- Forced FailSession exit code: 0
- Regression exit code: 0
```

### Notes
1. This closes the runtime bug behind the remaining format-regression workflow timeout failure point.
2. Default logging behavior is unchanged; the new logger override is opt-in and only used for headless validation.
3. GitHub Actions timeout containment remains useful, but it is no longer the only line of defense because the runtime gate now exits cleanly.

## Issue 177: Vulkan chain VK-048 Windows PASS-contract availability detail assertion hardening

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Hardened PASS-contract canary to assert success-path availability detail `none`.
- Extended workflow PASS-contract canary Step Summary with explicit availability-detail row.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_pass_contract_canary.ps1
   - parse:
     windows-vulkan-check.vulkan_availability_failure_detail
   - assert:
     gate_vulkan_availability_failure_detail=none
   - emit summary key:
     windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail

2) .github/workflows/cross-platform-gate.yml
   - PASS-contract Canary Step Summary rows add:
     gate_vulkan_availability_failure_detail

Build:
cmake --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk048-baseline.env"
Result: PASS

PASS-contract canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk048.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk048.env"
Result: PASS
Key lines:
- windows-vulkan-pass-contract-canary.actual_gate_exit_code=0
- windows-vulkan-pass-contract-canary.gate_result=PASS
- windows-vulkan-pass-contract-canary.gate_mode=strict
- windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail=none
- windows-vulkan-pass-contract-canary.gate_playback_contract_valid=true
- windows-vulkan-pass-contract-canary.gate_playback_failure_detail=none
- windows-vulkan-pass-contract-canary.validation_failure_reason=none
- windows-vulkan-pass-contract-canary.result=PASS

Full Windows Vulkan canary matrix batch:
Result: PASS
Key line:
- ALL_VK048_CHECKS_PASS

Static scan:
rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_pass_contract_canary|Windows Vulkan Gate PASS Contract Canary|vulkanPassCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1
Result: PASS
```

### Notes
1. This round is assertion/observability hardening only; PASS path behavior is unchanged.
2. Success-path availability-detail drift is now explicitly canary-protected.
3. Real strict PASS runtime proof still depends on Vulkan-ready Windows runner/workstation.

## Issue 176: Vulkan chain VK-047 Windows unsupported-platform availability detail assertion hardening

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Hardened unsupported-platform canary to assert availability detail `unsupported-platform`.
- Extended workflow unsupported-platform canary Step Summary with explicit availability-detail row.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1
   - parse:
     windows-vulkan-check.vulkan_availability_failure_detail
   - assert:
     gate_vulkan_availability_failure_detail=unsupported-platform
   - emit summary key:
     windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail

2) .github/workflows/cross-platform-gate.yml
   - Unsupported-platform Canary Step Summary rows add:
     gate_vulkan_availability_failure_detail

Build:
cmake --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk047-baseline.env"
Result: PASS

Unsupported-platform canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk047.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk047.env"
Result: PASS
Key lines:
- windows-vulkan-unsupported-platform-canary.actual_gate_exit_code=2
- windows-vulkan-unsupported-platform-canary.gate_result=FAIL
- windows-vulkan-unsupported-platform-canary.gate_failure_reason=unsupported-platform
- windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail=unsupported-platform
- windows-vulkan-unsupported-platform-canary.gate_playback_check_executed=false
- windows-vulkan-unsupported-platform-canary.validation_failure_reason=none
- windows-vulkan-unsupported-platform-canary.result=PASS

Full Windows Vulkan canary matrix batch:
Result: PASS
Key line:
- ALL_VK047_CHECKS_PASS

Static scan:
rg -n "gate_vulkan_availability_failure_detail|run_windows_vulkan_gate_unsupported_platform_canary|Windows Vulkan Gate Unsupported Platform Canary|vulkanUnsupportedPlatformCanaryExitCode" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1
Result: PASS
```

### Notes
1. This round is assertion/observability hardening only; unsupported-platform policy behavior is unchanged.
2. Unsupported-platform detail-classification drift is now explicitly canary-protected.
3. Real strict PASS runtime proof still depends on Vulkan-ready Windows runner/workstation.

## Issue 175: Vulkan chain VK-046 Windows diagnostics-contract availability detail assertion hardening

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Hardened diagnostics-contract canary to assert availability detail `diag-contract-missing-required-fields`.
- Extended workflow contract-canary Step Summary with explicit availability-detail row.

### Log
```text
Code changes:
1) tools/run_windows_vulkan_gate_contract_canary.ps1
   - parse:
     windows-vulkan-check.vulkan_availability_failure_detail
   - assert:
     gate_vulkan_availability_failure_detail=diag-contract-missing-required-fields
   - emit summary key:
     windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail

2) .github/workflows/cross-platform-gate.yml
   - Contract Canary Step Summary rows add:
     gate_vulkan_availability_failure_detail

Build:
cmake --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk046-baseline.env"
Result: PASS

Contract canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk046.env"
Result: PASS
Key lines:
- windows-vulkan-contract-canary.actual_gate_exit_code=2
- windows-vulkan-contract-canary.gate_result=FAIL
- windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
- windows-vulkan-contract-canary.gate_diag_contract_valid=false
- windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail=diag-contract-missing-required-fields
- windows-vulkan-contract-canary.validation_failure_reason=none
- windows-vulkan-contract-canary.result=PASS

Full Windows Vulkan canary matrix batch:
Result: PASS
Key line:
- ALL_VK046_CHECKS_PASS

Static scan:
rg -n "gate_vulkan_availability_failure_detail|diag-contract-missing-required-fields|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
Result: PASS
```

### Notes
1. This round is assertion/observability hardening only; policy behavior is unchanged.
2. Contract branch classification drift is now explicitly canary-protected.
3. Real strict PASS runtime proof still depends on Vulkan-ready Windows runner/workstation.

## Issue 174: Vulkan chain VK-045 Windows strict-compiled-in-disabled expected-fail canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic strict-compiled-in-disabled expected-fail canary for Windows Vulkan gate.
- Wired strict-compiled-in-disabled canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_strict_compiled_in_disabled_canary.cmd
- diagnostics path emits contract-valid disabled-build state:
  - platform=Windows
  - supported_platform=true
  - compiled_in=false
  - runtime_available=false
  - result=FAIL
  - dependency_source=disabled
  - process exit code=0
- strict mode requested via gate CLI
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate mode=strict
  - gate strict_mode_effective=true
  - gate failure_reason=vulkan-not-available-in-strict-mode
  - gate vulkan_availability_failure_detail=compiled-in-disabled
  - gate playback_check_executed=false
  - gate diag_exit_code=0
  - gate diag_dependency_source=disabled
- emits windows-vulkan-strict-compiled-in-disabled-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run strict-compiled-in-disabled canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1 ...
- parse strict-compiled-in-disabled summary and append section:
  "Windows Vulkan Gate Strict Compiled-In-Disabled Canary"
- fail-fast:
  if ($vulkanStrictCompiledInDisabledCanaryExitCode -ne 0) { throw ... }

Build:
cmake --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk046-baseline.env"
Result: PASS
Key lines:
- windows-vulkan-check.result=SKIPPED
- windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
- windows-vulkan-check.diag_dependency_source=disabled

Strict-compiled-in-disabled canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary-vk046.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-gate-vk046.env"
Result: PASS
Key lines:
- windows-vulkan-strict-compiled-in-disabled-canary.actual_gate_exit_code=2
- windows-vulkan-strict-compiled-in-disabled-canary.gate_result=FAIL
- windows-vulkan-strict-compiled-in-disabled-canary.gate_mode=strict
- windows-vulkan-strict-compiled-in-disabled-canary.gate_strict_mode_effective=true
- windows-vulkan-strict-compiled-in-disabled-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-strict-compiled-in-disabled-canary.gate_vulkan_availability_failure_detail=compiled-in-disabled
- windows-vulkan-strict-compiled-in-disabled-canary.gate_playback_check_executed=false
- windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_exit_code=0
- windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_dependency_source=disabled
- windows-vulkan-strict-compiled-in-disabled-canary.validation_failure_reason=none
- windows-vulkan-strict-compiled-in-disabled-canary.result=PASS

Canary matrix regression:
Result: PASS
Key line:
- ALL_VK046_CHECKS_PASS

Static scan:
rg -n "run_windows_vulkan_gate_strict_compiled_in_disabled_canary|vulkanStrictCompiledInDisabledCanaryExitCode|Windows Vulkan Gate Strict Compiled-In-Disabled Canary|windows-vulkan-strict-compiled-in-disabled-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic strict availability-detail `compiled-in-disabled` branch coverage only.
2. CI canary matrix now covers all strict unavailable availability details emitted by the gate.
3. Real strict PASS runtime proof still depends on Vulkan-ready Windows runner/workstation.

## Issue 173: Vulkan chain VK-044 Windows strict-diag-result-not-pass expected-fail canary

**Date**: 2026-03-28
**Status**: Resolved

### Description
- Added deterministic strict-diag-result-not-pass expected-fail canary for Windows Vulkan gate.
- Wired strict-diag-result-not-pass canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_strict_diag_result_not_pass_canary.cmd
- diagnostics path emits contract-valid and runtime-available state with FAIL result:
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=FAIL
  - dependency_source=find_package
  - process exit code=0
- strict mode requested via gate CLI
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate mode=strict
  - gate strict_mode_effective=true
  - gate failure_reason=vulkan-not-available-in-strict-mode
  - gate vulkan_availability_failure_detail=diag-result-not-pass
  - gate playback_check_executed=false
  - gate diag_exit_code=0
  - gate diag_result=FAIL
- emits windows-vulkan-strict-diag-result-not-pass-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run strict-diag-result-not-pass canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1 ...
- parse strict-diag-result-not-pass canary summary and append section:
  "Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary"
- fail-fast:
  if ($vulkanStrictDiagResultNotPassCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk044-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Canary matrix regression (VK-044 batch):
- diagnostics expected-fail canary: PASS
- playback expected-fail canary: PASS
- PASS-contract canary: PASS
- strict-unavailable canary: PASS
- strict-runtime-unavailable canary: PASS
- strict-diag-exit-nonzero canary: PASS
- strict-diag-result-not-pass canary: PASS
  - windows-vulkan-strict-diag-result-not-pass-canary.actual_gate_exit_code=2
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_result=FAIL
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_mode=strict
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_strict_mode_effective=true
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_vulkan_availability_failure_detail=diag-result-not-pass
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_playback_check_executed=false
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_exit_code=0
  - windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_result=FAIL
  - windows-vulkan-strict-diag-result-not-pass-canary.validation_failure_reason=none
  - windows-vulkan-strict-diag-result-not-pass-canary.result=PASS
- optional-skip canary: PASS
- unsupported-platform canary: PASS
- playback-semantic canary: PASS
- playback-backend semantic canary: PASS
- playback-candidates semantic canary: PASS
- playback-plan-reason semantic canary: PASS
- playback-result-not-pass canary: PASS
- playback-command-exit-nonzero canary: PASS

Static scan:
rg -n "run_windows_vulkan_gate_strict_diag_result_not_pass_canary|vulkanStrictDiagResultNotPassCanaryExitCode|Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary|windows-vulkan-strict-diag-result-not-pass-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic strict diag-result-not-pass branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now also covers strict diag-result-not-pass availability detail branch on top of existing canaries.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 172: Linux WSL gate build/playback chain stabilization

**Date**: 2026-03-28
**Status**: Resolved

### Description
- 在 Windows + WSL2 环境首次执行 Linux 全链路验证，定位并修复 Linux 构建、播放与音频 gate 的多个阻塞问题。
- 完成 Linux 构建、gate、打包三条链路的实机回归，结果全部通过。

### Log
```text
Environment:
- Host: Windows
- WSL distro: Ubuntu-24.04
- Repo: /mnt/d/C++files/VSProjects/modern-video-player/modern-video-player

Initial blocker:
- Running CMake build directory under /mnt/d/... triggered:
  - configure_file: Operation not permitted
- Mitigation:
  - switched build output to Linux filesystem:
    /home/u1133/mvp-build-linux

Code fixes landed:
1) include/streaming/http_stream_downloader.h
   - add: #include <cstdint>

2) src/render/opengl_video_renderer.cpp
   - non-Windows path:
     #undef None
     #undef Complex
   - avoid X11 macro collision with subtitle enums/tokens.

3) src/core/player_core.cpp
   - software/hardware decode path keeps valid callback wiring:
     video_codec_ctx_->get_format = &PlayerCore::selectVideoPixelFormat
     video_codec_ctx_->opaque = this
   - root cause:
     software path previously nullified get_format/opaque while FFmpeg may call
     get_format lazily near first packet send (observed CP-902 crash path).

4) src/main.cpp (CP-404)
   - result contract changed:
     from: pass_count == available_count
     to:   available_count > 0 && pass_count > 0
   - aligns executable behavior with gate script expectation.

5) .gitattributes
   - add: *.sh text eol=lf
   - prevents WSL shell script execution breakage from CRLF line endings.

Validation commands:
- cmake -S . -B /home/u1133/mvp-build-linux -DCMAKE_BUILD_TYPE=Release -DDEBUG_MODE=OFF -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_DXVA2=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_VAAPI=ON -DENABLE_VIDEOTOOLBOX=OFF
  Result: PASS

- cmake --build /home/u1133/mvp-build-linux --parallel --target modern-video-player sample_logger_plugin
  Result: PASS

- xvfb-run -a bash ./tools/run_linux_mvp_checks.sh /home/u1133/mvp-build-linux/modern-video-player samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 1800 samples/subtitles/opengl_ass_style_validation.ass build/tmp/embedded-ass-validation.mkv 120 0 samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4 samples/subtitles/opengl_ass_transform_transition_validation.ass /home/u1133/mvp-logs/linux-mvp-gate-summary-20260328.env 0
  Result: PASS
  Key lines:
  - linux-audio-backend-smoke.available_target_count=3
  - linux-audio-backend-smoke.pass_target_count=1
  - linux-audio-backend-smoke.result=PASS
  - Linux MVP gate result: PASS

- bash ./tools/package_linux.sh /home/u1133/mvp-package-build-20260328
  Result: PASS
  Generated:
  - modern-video-player_1.0.0_amd64.deb
  - modern-video-player-1.0.0-rc1-linux-x64.tar.gz
```

### Notes
1. Linux 构建目录建议固定使用 WSL Linux 文件系统路径，避免 `/mnt/<drive>` 下的权限语义差异导致 CMake 配置失败。
2. 当前环境 Vulkan 在 Linux gate 中维持 expected-skip（`compiled_in=false`, `runtime_available=false`），不影响本轮 gate PASS。
3. 本轮已完成仓库内构建/打包临时产物清理，仅保留源码与文档改动。

## Issue 171: Vulkan chain VK-043 Windows strict-diag-exit-nonzero expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic strict-diag-exit-nonzero expected-fail canary for Windows Vulkan gate.
- Wired strict-diag-exit-nonzero canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_strict_diag_exit_nonzero_canary.cmd
- diagnostics path emits contract-valid and runtime-available state, then exits non-zero intentionally
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
  - process exit code=9
- strict mode requested via gate CLI
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate mode=strict
  - gate strict_mode_effective=true
  - gate failure_reason=vulkan-not-available-in-strict-mode
  - gate vulkan_availability_failure_detail=diag-exit-nonzero
  - gate playback_check_executed=false
  - gate diag_exit_code=9
- emits windows-vulkan-strict-diag-exit-nonzero-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run strict-diag-exit-nonzero canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1 ...
- parse strict-diag-exit-nonzero canary summary and append section:
  "Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary"
- fail-fast:
  if ($vulkanStrictDiagExitNonzeroCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk043-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Strict-runtime-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-runtime-unavailable-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-strict-runtime-unavailable-canary.result=PASS

Strict-diag-exit-nonzero canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-gate-vk043.env"
Result: PASS
Key lines:
- windows-vulkan-strict-diag-exit-nonzero-canary.actual_gate_exit_code=2
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_result=FAIL
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_mode=strict
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_strict_mode_effective=true
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_vulkan_availability_failure_detail=diag-exit-nonzero
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_playback_check_executed=false
- windows-vulkan-strict-diag-exit-nonzero-canary.gate_diag_exit_code=9
- windows-vulkan-strict-diag-exit-nonzero-canary.validation_failure_reason=none
- windows-vulkan-strict-diag-exit-nonzero-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Playback-plan-reason semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-plan-reason-semantic-canary.result=PASS

Playback-result-not-pass canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-result-not-pass-canary.result=PASS

Playback-command-exit-nonzero canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk043.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk043.env"
Result: PASS
Key line:
- windows-vulkan-playback-command-exit-nonzero-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_strict_diag_exit_nonzero_canary|vulkanStrictDiagExitNonzeroCanaryExitCode|Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary|windows-vulkan-strict-diag-exit-nonzero-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic strict diag-exit-nonzero branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now also covers strict diag-exit-nonzero availability detail branch on top of existing canaries.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 170: Vulkan chain VK-042 Windows strict-runtime-unavailable expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic strict-runtime-unavailable expected-fail canary for Windows Vulkan gate.
- Wired strict-runtime-unavailable canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_strict_runtime_unavailable_canary.cmd
- diagnostics path emits contract-valid state with runtime unavailable
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=false
  - result=FAIL
  - dependency_source=find_package
- strict mode requested via gate CLI
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate mode=strict
  - gate strict_mode_effective=true
  - gate failure_reason=vulkan-not-available-in-strict-mode
  - gate vulkan_availability_failure_detail=runtime-unavailable
  - gate playback_check_executed=false
- emits windows-vulkan-strict-runtime-unavailable-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run strict-runtime-unavailable canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 ...
- parse strict-runtime-unavailable canary summary and append section:
  "Windows Vulkan Gate Strict Runtime-Unavailable Canary"
- fail-fast:
  if ($vulkanStrictRuntimeUnavailableCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk042-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Strict-runtime-unavailable canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-runtime-unavailable-canary-gate-vk042.env"
Result: PASS
Key lines:
- windows-vulkan-strict-runtime-unavailable-canary.actual_gate_exit_code=2
- windows-vulkan-strict-runtime-unavailable-canary.gate_result=FAIL
- windows-vulkan-strict-runtime-unavailable-canary.gate_mode=strict
- windows-vulkan-strict-runtime-unavailable-canary.gate_strict_mode_effective=true
- windows-vulkan-strict-runtime-unavailable-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-strict-runtime-unavailable-canary.gate_vulkan_availability_failure_detail=runtime-unavailable
- windows-vulkan-strict-runtime-unavailable-canary.gate_playback_check_executed=false
- windows-vulkan-strict-runtime-unavailable-canary.validation_failure_reason=none
- windows-vulkan-strict-runtime-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Playback-plan-reason semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-plan-reason-semantic-canary.result=PASS

Playback-result-not-pass canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-result-not-pass-canary.result=PASS

Playback-command-exit-nonzero canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk042.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk042.env"
Result: PASS
Key line:
- windows-vulkan-playback-command-exit-nonzero-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_strict_runtime_unavailable_canary|vulkanStrictRuntimeUnavailableCanaryExitCode|Windows Vulkan Gate Strict Runtime-Unavailable Canary|windows-vulkan-strict-runtime-unavailable-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic strict runtime-unavailable branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now also covers strict runtime-unavailable availability detail branch on top of existing canaries.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 169: Vulkan chain VK-041 Windows playback-command-exit-nonzero expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-command-exit-nonzero expected-fail canary for Windows Vulkan gate.
- Wired playback-command-exit-nonzero canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_command_exit_nonzero_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid payload and exits non-zero intentionally
  - result=PASS
  - startup_selected_renderer=Vulkan
  - renderer_backend=Vulkan
  - startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL
  - startup_renderer_plan_reason=renderer-override-env
  - process exit code=7
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=command-exit-nonzero
  - gate playback_result=PASS
- emits windows-vulkan-playback-command-exit-nonzero-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-command-exit-nonzero canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 ...
- parse playback-command-exit-nonzero canary summary and append section:
  "Windows Vulkan Gate Playback Command-Exit-Nonzero Canary"
- fail-fast:
  if ($vulkanPlaybackCommandExitNonzeroCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk041-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Playback-plan-reason semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-plan-reason-semantic-canary.result=PASS

Playback-result-not-pass canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk041.env"
Result: PASS
Key line:
- windows-vulkan-playback-result-not-pass-canary.result=PASS

Playback-command-exit-nonzero canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary-vk041.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate-vk041.env"
Result: PASS
Key lines:
- windows-vulkan-playback-command-exit-nonzero-canary.actual_gate_exit_code=2
- windows-vulkan-playback-command-exit-nonzero-canary.gate_result=FAIL
- windows-vulkan-playback-command-exit-nonzero-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_failure_detail=command-exit-nonzero
- windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_result=PASS
- windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_selected_renderer=Vulkan
- windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_renderer_backend=Vulkan
- windows-vulkan-playback-command-exit-nonzero-canary.validation_failure_reason=none
- windows-vulkan-playback-command-exit-nonzero-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_command_exit_nonzero_canary|vulkanPlaybackCommandExitNonzeroCanaryExitCode|Windows Vulkan Gate Playback Command-Exit-Nonzero Canary|windows-vulkan-playback-command-exit-nonzero-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback command-exit branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now also covers command-exit-nonzero branch on top of previous diagnostics/playback contract and semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 168: Vulkan chain VK-040 Windows playback-result-not-pass expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-result-not-pass expected-fail canary for Windows Vulkan gate.
- Wired playback-result-not-pass canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_result_not_pass_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid but result mismatch state
  - result=FAIL
  - startup_selected_renderer=Vulkan
  - renderer_backend=Vulkan
  - startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL
  - startup_renderer_plan_reason=renderer-override-env
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=result-not-pass
  - gate playback_result=FAIL
- emits windows-vulkan-playback-result-not-pass-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-result-not-pass canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 ...
- parse playback-result-not-pass canary summary and append section:
  "Windows Vulkan Gate Playback Result-Not-Pass Canary"
- fail-fast:
  if ($vulkanPlaybackResultNotPassCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk040-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Playback-plan-reason semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk040.env"
Result: PASS
Key line:
- windows-vulkan-playback-plan-reason-semantic-canary.result=PASS

Playback-result-not-pass canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-summary-vk040.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-result-not-pass-canary-gate-vk040.env"
Result: PASS
Key lines:
- windows-vulkan-playback-result-not-pass-canary.actual_gate_exit_code=2
- windows-vulkan-playback-result-not-pass-canary.gate_result=FAIL
- windows-vulkan-playback-result-not-pass-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-result-not-pass-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-result-not-pass-canary.gate_playback_failure_detail=result-not-pass
- windows-vulkan-playback-result-not-pass-canary.gate_playback_result=FAIL
- windows-vulkan-playback-result-not-pass-canary.gate_playback_selected_renderer=Vulkan
- windows-vulkan-playback-result-not-pass-canary.gate_playback_renderer_backend=Vulkan
- windows-vulkan-playback-result-not-pass-canary.validation_failure_reason=none
- windows-vulkan-playback-result-not-pass-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_result_not_pass_canary|vulkanPlaybackResultNotPassCanaryExitCode|Windows Vulkan Gate Playback Result-Not-Pass Canary|windows-vulkan-playback-result-not-pass-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback `result-not-pass` branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now also covers playback-result mismatch branch on top of previous diagnostics/playback contract and semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 167: Vulkan chain VK-039 Windows playback-plan-reason semantic expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-plan-reason semantic expected-fail canary for Windows Vulkan gate.
- Wired playback-plan-reason semantic canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_plan_reason_semantic_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid but semantic mismatch state
  - startup_selected_renderer=Vulkan
  - renderer_backend=Vulkan
  - startup_renderer_candidates=Vulkan > D3D11 > SoftwareSDL
  - startup_renderer_plan_reason=strategy-default
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=plan-reason-not-renderer-override-env
- emits windows-vulkan-playback-plan-reason-semantic-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-plan-reason semantic canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 ...
- parse playback-plan-reason semantic canary summary and append section:
  "Windows Vulkan Gate Playback Plan Reason Semantic Canary"
- fail-fast:
  if ($vulkanPlaybackPlanReasonSemanticCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk039-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk039.env"
Result: PASS
Key line:
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Playback-plan-reason semantic canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary-vk039.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate-vk039.env"
Result: PASS
Key lines:
- windows-vulkan-playback-plan-reason-semantic-canary.actual_gate_exit_code=2
- windows-vulkan-playback-plan-reason-semantic-canary.gate_result=FAIL
- windows-vulkan-playback-plan-reason-semantic-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_failure_detail=plan-reason-not-renderer-override-env
- windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_selected_renderer=Vulkan
- windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_renderer_backend=Vulkan
- windows-vulkan-playback-plan-reason-semantic-canary.validation_failure_reason=none
- windows-vulkan-playback-plan-reason-semantic-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_plan_reason_semantic_canary|vulkanPlaybackPlanReasonSemanticCanaryExitCode|Windows Vulkan Gate Playback Plan Reason Semantic Canary|windows-vulkan-playback-plan-reason-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback-plan-reason semantic branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback contract expected-fail, strict PASS, strict-unavailable expected-fail, optional-skip, unsupported-platform expected-fail, selected-renderer semantic expected-fail, backend semantic expected-fail, candidates semantic expected-fail, and plan-reason semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 166: Vulkan chain VK-038 Windows playback-candidates semantic expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-candidates semantic expected-fail canary for Windows Vulkan gate.
- Wired playback-candidates semantic canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_candidates_semantic_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid but semantic mismatch state
  - startup_selected_renderer=Vulkan
  - renderer_backend=Vulkan
  - startup_renderer_candidates=D3D11 > SoftwareSDL
  - startup_renderer_plan_reason=renderer-override-env
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=candidates-missing-vulkan
- emits windows-vulkan-playback-candidates-semantic-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-candidates semantic canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 ...
- parse playback-candidates semantic canary summary and append section:
  "Windows Vulkan Gate Playback Candidates Semantic Canary"
- fail-fast:
  if ($vulkanPlaybackCandidatesSemanticCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk038-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk038.env"
Result: PASS
Key line:
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Playback-candidates semantic canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary-vk038.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate-vk038.env"
Result: PASS
Key lines:
- windows-vulkan-playback-candidates-semantic-canary.actual_gate_exit_code=2
- windows-vulkan-playback-candidates-semantic-canary.gate_result=FAIL
- windows-vulkan-playback-candidates-semantic-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-candidates-semantic-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-candidates-semantic-canary.gate_playback_failure_detail=candidates-missing-vulkan
- windows-vulkan-playback-candidates-semantic-canary.gate_playback_selected_renderer=Vulkan
- windows-vulkan-playback-candidates-semantic-canary.gate_playback_renderer_backend=Vulkan
- windows-vulkan-playback-candidates-semantic-canary.validation_failure_reason=none
- windows-vulkan-playback-candidates-semantic-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_candidates_semantic_canary|vulkanPlaybackCandidatesSemanticCanaryExitCode|Windows Vulkan Gate Playback Candidates Semantic Canary|windows-vulkan-playback-candidates-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback-candidates semantic branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback contract expected-fail, strict PASS, strict-unavailable expected-fail, optional-skip, unsupported-platform expected-fail, selected-renderer semantic expected-fail, backend semantic expected-fail, and candidate-chain semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 165: Vulkan chain VK-037 Windows playback-backend semantic expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-backend semantic expected-fail canary for Windows Vulkan gate.
- Wired playback-backend semantic canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_backend_semantic_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid but semantic mismatch state
  - startup_selected_renderer=Vulkan
  - renderer_backend=OpenGL
  - startup_renderer_plan_reason=renderer-override-env
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=renderer-backend-not-vulkan
- emits windows-vulkan-playback-backend-semantic-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-backend semantic canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 ...
- parse playback-backend semantic canary summary and append section:
  "Windows Vulkan Gate Playback Backend Semantic Canary"
- fail-fast:
  if ($vulkanPlaybackBackendSemanticCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk037-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk037.env"
Result: PASS
Key line:
- windows-vulkan-playback-semantic-canary.result=PASS

Playback-backend semantic canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-summary-vk037.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-backend-semantic-canary-gate-vk037.env"
Result: PASS
Key lines:
- windows-vulkan-playback-backend-semantic-canary.actual_gate_exit_code=2
- windows-vulkan-playback-backend-semantic-canary.gate_result=FAIL
- windows-vulkan-playback-backend-semantic-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-backend-semantic-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-backend-semantic-canary.gate_playback_failure_detail=renderer-backend-not-vulkan
- windows-vulkan-playback-backend-semantic-canary.gate_playback_selected_renderer=Vulkan
- windows-vulkan-playback-backend-semantic-canary.gate_playback_renderer_backend=OpenGL
- windows-vulkan-playback-backend-semantic-canary.validation_failure_reason=none
- windows-vulkan-playback-backend-semantic-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_backend_semantic_canary|vulkanPlaybackBackendSemanticCanaryExitCode|Windows Vulkan Gate Playback Backend Semantic Canary|windows-vulkan-playback-backend-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback-backend semantic branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback contract expected-fail, strict PASS, strict-unavailable expected-fail, optional-skip, unsupported-platform expected-fail, selected-renderer semantic expected-fail, and backend semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 164: Vulkan chain VK-036 Windows playback-semantic expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic playback-semantic expected-fail canary for Windows Vulkan gate.
- Wired playback-semantic canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_semantic_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_semantic_canary.cmd
- diagnostics path emits contract-valid and available Vulkan state
  - platform=Windows
  - supported_platform=true
  - compiled_in=true
  - runtime_available=true
  - result=PASS
  - dependency_source=find_package
- playback path emits contract-valid but semantic mismatch state
  - startup_selected_renderer=D3D11
  - renderer_backend=Vulkan
  - startup_renderer_plan_reason=selected_vulkan
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=vulkan-playback-check-failed
  - gate playback_contract_valid=true
  - gate playback_failure_detail=selected-renderer-not-vulkan
- emits windows-vulkan-playback-semantic-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-semantic canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 ...
- parse playback-semantic canary summary and append section:
  "Windows Vulkan Gate Playback Semantic Canary"
- fail-fast:
  if ($vulkanPlaybackSemanticCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk036-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk036.env"
Result: PASS
Key line:
- windows-vulkan-unsupported-platform-canary.result=PASS

Playback-semantic canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-summary-vk036.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-semantic-canary-gate-vk036.env"
Result: PASS
Key lines:
- windows-vulkan-playback-semantic-canary.actual_gate_exit_code=2
- windows-vulkan-playback-semantic-canary.gate_result=FAIL
- windows-vulkan-playback-semantic-canary.gate_failure_reason=vulkan-playback-check-failed
- windows-vulkan-playback-semantic-canary.gate_playback_contract_valid=true
- windows-vulkan-playback-semantic-canary.gate_playback_failure_detail=selected-renderer-not-vulkan
- windows-vulkan-playback-semantic-canary.gate_playback_selected_renderer=D3D11
- windows-vulkan-playback-semantic-canary.validation_failure_reason=none
- windows-vulkan-playback-semantic-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_semantic_canary|vulkanPlaybackSemanticCanaryExitCode|Windows Vulkan Gate Playback Semantic Canary|windows-vulkan-playback-semantic-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_semantic_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback-semantic branch coverage only; gate policy behavior remains unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback contract expected-fail, strict PASS, strict-unavailable expected-fail, optional-skip, unsupported-platform expected-fail, and playback-semantic expected-fail branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 163: Vulkan chain VK-035 Windows unsupported-platform expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic unsupported-platform expected-fail canary for Windows Vulkan gate.
- Wired unsupported-platform canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_unsupported_platform_canary.cmd
- diagnostics path emits contract-valid unsupported-platform state
  - platform=Linux
  - supported_platform=false
  - compiled_in=false
  - runtime_available=false
  - result=FAIL
  - dependency_source=find_package
- invokes gate in default optional mode
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate failure_reason=unsupported-platform
  - gate skip_reason empty
  - gate playback_check_executed=false
- emits windows-vulkan-unsupported-platform-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run unsupported-platform canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 ...
- parse unsupported-platform canary summary and append section:
  "Windows Vulkan Gate Unsupported Platform Canary"
- fail-fast:
  if ($vulkanUnsupportedPlatformCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk035-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk035.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk035.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk035.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk035.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk035.env"
Result: PASS
Key line:
- windows-vulkan-optional-skip-canary.result=PASS

Unsupported-platform canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-summary-vk035.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-unsupported-platform-canary-gate-vk035.env"
Result: PASS
Key lines:
- windows-vulkan-unsupported-platform-canary.actual_gate_exit_code=2
- windows-vulkan-unsupported-platform-canary.gate_result=FAIL
- windows-vulkan-unsupported-platform-canary.gate_mode=optional
- windows-vulkan-unsupported-platform-canary.gate_failure_reason=unsupported-platform
- windows-vulkan-unsupported-platform-canary.gate_skip_reason=
- windows-vulkan-unsupported-platform-canary.gate_playback_check_executed=false
- windows-vulkan-unsupported-platform-canary.validation_failure_reason=none
- windows-vulkan-unsupported-platform-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_unsupported_platform_canary|vulkanUnsupportedPlatformCanaryExitCode|Windows Vulkan Gate Unsupported Platform Canary|windows-vulkan-unsupported-platform-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic unsupported-platform branch coverage only; policy behavior unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback expected-fail, strict PASS, strict-unavailable expected-fail, optional-skip, and unsupported-platform expected-fail.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 162: Vulkan chain VK-034 Windows optional-unavailable skip canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic optional-unavailable skip canary for Windows Vulkan gate optional-policy branch.
- Wired optional-skip canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_optional_skip_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_optional_skip_canary.cmd
- diagnostics path emits contract-valid but unavailable Vulkan state
  - supported_platform=true
  - compiled_in=false
  - runtime_available=false
  - result=FAIL
  - dependency_source=find_package
- invokes gate without strict override (optional mode expected)
- validates:
  - gate exit code=0
  - gate result=SKIPPED
  - gate mode=optional
  - gate strict_mode_effective=false
  - gate skip_reason=vulkan-not-available
  - gate failure_reason empty
  - gate playback_check_executed=false
  - gate vulkan_availability_failure_detail=compiled-in-false
- emits windows-vulkan-optional-skip-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run optional-skip canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 ...
- parse optional-skip canary summary and append section:
  "Windows Vulkan Gate Optional Skip Canary"
- fail-fast:
  if ($vulkanOptionalSkipCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk034-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk034.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk034.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk034.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk034.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk034.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk034.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk034.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk034.env"
Result: PASS
Key line:
- windows-vulkan-strict-unavailable-canary.result=PASS

Optional-skip canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-summary-vk034.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-optional-skip-canary-gate-vk034.env"
Result: PASS
Key lines:
- windows-vulkan-optional-skip-canary.actual_gate_exit_code=0
- windows-vulkan-optional-skip-canary.gate_result=SKIPPED
- windows-vulkan-optional-skip-canary.gate_mode=optional
- windows-vulkan-optional-skip-canary.gate_strict_mode_effective=false
- windows-vulkan-optional-skip-canary.gate_skip_reason=vulkan-not-available
- windows-vulkan-optional-skip-canary.gate_failure_reason=
- windows-vulkan-optional-skip-canary.gate_vulkan_availability_failure_detail=compiled-in-false
- windows-vulkan-optional-skip-canary.gate_playback_check_executed=false
- windows-vulkan-optional-skip-canary.validation_failure_reason=none
- windows-vulkan-optional-skip-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_optional_skip_canary|vulkanOptionalSkipCanaryExitCode|Windows Vulkan Gate Optional Skip Canary|windows-vulkan-optional-skip-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_optional_skip_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic optional-skip branch coverage only; policy behavior unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback expected-fail, strict PASS, strict-unavailable expected-fail, and optional-unavailable SKIPPED branch.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 161: Vulkan chain VK-033 Windows strict-unavailable expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic strict-unavailable expected-fail canary for Windows Vulkan gate strict-policy branch.
- Wired strict-unavailable canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_strict_unavailable_canary.cmd
- diagnostics path emits contract-valid but unavailable Vulkan state
  - supported_platform=true
  - compiled_in=false
  - runtime_available=false
  - result=FAIL
  - dependency_source=find_package
- invokes gate in strict mode (-RequireVulkanAvailable)
- validates:
  - gate exit code=2
  - gate result=FAIL
  - gate mode=strict
  - gate failure_reason=vulkan-not-available-in-strict-mode
  - gate playback_check_executed=false
  - gate vulkan_availability_failure_detail=compiled-in-false
- emits windows-vulkan-strict-unavailable-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run strict-unavailable canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 ...
- parse strict-unavailable canary summary and append section:
  "Windows Vulkan Gate Strict Unavailable Canary"
- fail-fast:
  if ($vulkanStrictUnavailableCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk033-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk033.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk033.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary regression:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk033.env"
Result: PASS
Key line:
- windows-vulkan-pass-contract-canary.result=PASS

Strict-unavailable canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-summary-vk033.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-strict-unavailable-canary-gate-vk033.env"
Result: PASS
Key lines:
- windows-vulkan-strict-unavailable-canary.actual_gate_exit_code=2
- windows-vulkan-strict-unavailable-canary.gate_result=FAIL
- windows-vulkan-strict-unavailable-canary.gate_mode=strict
- windows-vulkan-strict-unavailable-canary.gate_strict_mode_effective=true
- windows-vulkan-strict-unavailable-canary.gate_failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-strict-unavailable-canary.gate_vulkan_availability_failure_detail=compiled-in-false
- windows-vulkan-strict-unavailable-canary.gate_playback_check_executed=false
- windows-vulkan-strict-unavailable-canary.validation_failure_reason=none
- windows-vulkan-strict-unavailable-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_strict_unavailable_canary|vulkanStrictUnavailableCanaryExitCode|Windows Vulkan Gate Strict Unavailable Canary|windows-vulkan-strict-unavailable-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic strict-unavailable branch coverage only; policy behavior unchanged.
2. CI canary matrix now covers diagnostics expected-fail, playback expected-fail, strict PASS, and strict-unavailable expected-fail.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 160: Vulkan chain VK-032 Windows PASS-contract canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic PASS-contract canary for Windows Vulkan gate strict-path success semantics.
- Wired PASS canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_pass_contract_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_pass_contract_canary.cmd
- diagnostics path emits valid Vulkan availability contract
- playback path emits complete Vulkan PASS contract fields
- invokes gate in strict mode (-RequireVulkanAvailable)
- validates:
  - gate exit code=0
  - gate result=PASS
  - gate mode=strict
  - gate playback_contract_valid=true
  - gate playback_failure_detail=none
- emits windows-vulkan-pass-contract-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run pass-contract canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 ...
- parse pass canary summary and append section:
  "Windows Vulkan Gate PASS Contract Canary"
- fail-fast:
  if ($vulkanPassCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk032-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Diagnostics expected-fail canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk032.env"
Result: PASS
Key line:
- windows-vulkan-contract-canary.result=PASS

Playback expected-fail canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk032.env"
Result: PASS
Key line:
- windows-vulkan-playback-contract-canary.result=PASS

PASS-contract canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-summary-vk032.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-pass-contract-canary-gate-vk032.env"
Result: PASS
Key lines:
- windows-vulkan-pass-contract-canary.actual_gate_exit_code=0
- windows-vulkan-pass-contract-canary.gate_result=PASS
- windows-vulkan-pass-contract-canary.gate_mode=strict
- windows-vulkan-pass-contract-canary.gate_strict_mode_effective=true
- windows-vulkan-pass-contract-canary.gate_playback_contract_valid=true
- windows-vulkan-pass-contract-canary.gate_playback_failure_detail=none
- windows-vulkan-pass-contract-canary.validation_failure_reason=none
- windows-vulkan-pass-contract-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_pass_contract_canary|vulkanPassCanaryExitCode|Windows Vulkan Gate PASS Contract Canary|windows-vulkan-pass-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_pass_contract_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic PASS branch contract coverage only; policy behavior unchanged.
2. CI now has deterministic canary guards for diagnostics expected-fail, playback expected-fail, and strict PASS branch.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 159: Vulkan chain VK-031 Windows playback-contract expected-fail canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic expected-fail canary for Windows Vulkan playback-contract branch.
- Wired playback-contract canary into workflow with Step Summary and fail-fast guard.

### Log
```text
New script:
tools/run_windows_vulkan_gate_playback_contract_canary.ps1
- creates mock executable:
  logs/mock_windows_vulkan_playback_contract_canary.cmd
- diagnostics path emits valid Vulkan availability contract
- playback path intentionally omits required keys to trigger playback-contract-broken
- validates:
  - gate exit code=2
  - gate failure_reason=vulkan-playback-contract-broken
  - gate playback_contract_valid=false
  - required missing fields are reported
- emits windows-vulkan-playback-contract-canary.* output

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- run playback-contract canary:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 ...
- parse playback canary summary and append section:
  "Windows Vulkan Gate Playback Contract Canary"
- fail-fast:
  if ($vulkanPlaybackCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk031-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Playback-contract canary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-summary-vk031.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-playback-contract-canary-gate-vk031.env"
Result: PASS
Key lines:
- windows-vulkan-playback-contract-canary.actual_gate_exit_code=2
- windows-vulkan-playback-contract-canary.gate_summary_file_present=true
- windows-vulkan-playback-contract-canary.gate_result=FAIL
- windows-vulkan-playback-contract-canary.gate_failure_reason=vulkan-playback-contract-broken
- windows-vulkan-playback-contract-canary.gate_playback_contract_valid=false
- windows-vulkan-playback-contract-canary.gate_playback_failure_detail=contract-missing-required-fields
- windows-vulkan-playback-contract-canary.validation_failure_reason=none
- windows-vulkan-playback-contract-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_playback_contract_canary|vulkanPlaybackCanaryExitCode|Windows Vulkan Gate Playback Contract Canary|windows-vulkan-playback-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_playback_contract_canary.ps1
Result: PASS
```

### Notes
1. This round adds deterministic playback-contract canary coverage only; policy behavior unchanged.
2. CI now has deterministic canary guards for both diagnostics-contract and playback-contract branches.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 158: Vulkan chain VK-030 Windows gate contract-canary Step Summary observability

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added CI Step Summary publication for Windows Vulkan contract-canary outputs.
- Extended canary summary payload with gate-summary file presence signal.

### Log
```text
Workflow change:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- parse logs/windows-vulkan-gate-contract-canary-summary.env
- append "Windows Vulkan Gate Contract Canary" markdown table to GITHUB_STEP_SUMMARY
- fallback message when canary summary env is missing
- keep fail-fast:
  if ($vulkanCanaryExitCode -ne 0) { throw ... }

Canary script change:
tools/run_windows_vulkan_gate_contract_canary.ps1
- added output:
  windows-vulkan-contract-canary.gate_summary_file_present=true|false
- added validation branch:
  gate-summary-file-missing

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk030-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Canary run:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk030.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk030.env"
Result: PASS
Key lines:
- windows-vulkan-contract-canary.actual_gate_exit_code=2
- windows-vulkan-contract-canary.gate_summary_file_present=true
- windows-vulkan-contract-canary.gate_result=FAIL
- windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
- windows-vulkan-contract-canary.gate_diag_contract_valid=false
- windows-vulkan-contract-canary.result=PASS

Local Step Summary preview:
generated logs/windows-vulkan-canary-step-summary-preview-vk030.md
Key lines:
- | result | PASS |
- | actual_gate_exit_code | 2 |
- | gate_summary_file_present | true |
- | gate_failure_reason | vulkan-diagnostics-contract-broken |

Static scan:
rg -n "Windows Vulkan Gate Contract Canary|gate_summary_file_present|vulkanCanarySummaryPath|vulkanCanaryExitCode|run_windows_vulkan_gate_contract_canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
Result: PASS
```

### Notes
1. This round is observability-only for canary outputs; no policy behavior changes.
2. Contract-canary fail-fast semantics remain unchanged.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 157: Vulkan chain VK-029 Windows gate expected-fail contract canary

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added deterministic expected-fail contract canary for Windows Vulkan gate.
- Wired canary into CI Windows gate with explicit non-zero propagation check.

### Log
```text
New canary script:
tools/run_windows_vulkan_gate_contract_canary.ps1
- invokes run_windows_vulkan_checks.ps1 with cmd.exe diagnostics path
- validates deterministic failure contract:
  - gate exit code = 2
  - windows-vulkan-check.result=FAIL
  - windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken
  - windows-vulkan-check.diag_contract_valid=false
- emits windows-vulkan-contract-canary.* summary fields

Workflow integration:
.github/workflows/cross-platform-gate.yml (Run Windows gate)
- executes:
  powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 ...
- captures:
  $vulkanCanaryExitCode = $LASTEXITCODE
- fail-fast:
  if ($vulkanCanaryExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk029-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Canary run:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-contract-canary-summary-vk029.env" -GateSummaryOutputPath "logs/windows-vulkan-gate-contract-canary-gate-vk029.env"
Result: PASS
Key lines:
- windows-vulkan-contract-canary.actual_gate_exit_code=2
- windows-vulkan-contract-canary.gate_result=FAIL
- windows-vulkan-contract-canary.gate_failure_reason=vulkan-diagnostics-contract-broken
- windows-vulkan-contract-canary.gate_diag_contract_valid=false
- windows-vulkan-contract-canary.result=PASS

Static scan:
rg -n "run_windows_vulkan_gate_contract_canary|vulkanCanaryExitCode|windows-vulkan-gate-contract-canary" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_gate_contract_canary.ps1
Result: PASS
```

### Notes
1. This round adds contract canary coverage and does not change strict/optional policy semantics.
2. Canary uses deterministic expected-fail path, so it remains stable across runner hardware differences.
3. Strict PASS-path for real Vulkan playback still depends on Vulkan-ready Windows runner/workstation.

## Issue 156: Vulkan chain VK-028 Windows gate exit-code propagation hardening

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Hardened Windows Vulkan CI gate so non-zero gate exit code is no longer swallowed by PowerShell pipeline logging.
- Preserved `VK-027` Step Summary rendering behavior while enforcing fail-fast on Vulkan gate failure.

### Log
```text
Workflow change:
.github/workflows/cross-platform-gate.yml
- after Vulkan gate pipeline command:
  $vulkanGateExitCode = $LASTEXITCODE
- after Step Summary rendering:
  if ($vulkanGateExitCode -ne 0) { throw ... }

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk028-baseline.env"
Result: PASS
Key line:
- windows-vulkan-check.result=SKIPPED

Legacy behavior reproduction (no guard):
powershell -NoProfile -Command '$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS="1"; powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... 2>&1 | Tee-Object ...; Write-Host "legacy-last=$LASTEXITCODE"'
Observed:
- windows-vulkan-check.result=FAIL
- legacy-last=2
- outer process exit=0

Guarded behavior reproduction:
powershell -NoProfile -Command '$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS="1"; powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 ... 2>&1 | Tee-Object ...; $vulkanGateExitCode = $LASTEXITCODE; if ($vulkanGateExitCode -ne 0) { throw (''Windows Vulkan gate failed with exit code {0}'' -f $vulkanGateExitCode) }'
Observed:
- windows-vulkan-check.result=FAIL
- Windows Vulkan gate failed with exit code 2
- outer process exit=1

Static scan:
rg -n "vulkanGateExitCode|Windows Vulkan gate failed with exit code|windows-vulkan-gate-summary.env" .github/workflows/cross-platform-gate.yml
Result: PASS
```

### Notes
1. This round hardens gate semantics; strict/optional policy behavior is unchanged.
2. Step Summary remains generated before fail-fast throw.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 155: Vulkan chain VK-027 Windows CI step summary observability

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added Windows Vulkan gate key-signal publication to GitHub Actions Step Summary.
- Added fallback summary message when Vulkan summary env file is missing.

### Log
```text
Workflow change:
.github/workflows/cross-platform-gate.yml (Windows gate step)
- parse logs/windows-vulkan-gate-summary.env
- append Step Summary table with key fields:
  result/mode/strict policy + SDK/runtime probe + contract validity + failure details
- add missing-file fallback message

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline env generation:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk027-baseline.env"
Result: PASS

Local parser preview:
logs/windows-vulkan-step-summary-preview-vk027.md
Key lines:
- | result | SKIPPED |
- | mode | optional |
- | strict_mode_effective | false |
- | diag_contract_valid | true |
- | playback_contract_valid | n/a |
- | vulkan_availability_failure_detail | compiled-in-disabled |

Static scan:
rg -n "Windows Vulkan Gate Summary|GITHUB_STEP_SUMMARY|windows-vulkan-gate-summary.env|runner_vulkan_runtime_probe_detail|playback_contract_valid|vulkan_availability_failure_detail" .github/workflows/cross-platform-gate.yml
Result: PASS
```

### Notes
1. This round is observability-only and does not change gate decision policy.
2. CI triage can read Vulkan gate key signals directly from Step Summary.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 154: Vulkan chain VK-026 Windows playback contract validation

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added required-key contract validation for playback-check output in Windows Vulkan gate.
- Added machine-readable playback contract fields and playback contract-broken failure path.

### Log
```text
Static path scan:
rg -n "playback_contract_valid|playback_missing_required_fields|playback_failure_detail|vulkan-playback-contract-broken|contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk026-baseline.env"
Key lines:
- windows-vulkan-check.playback_check_executed=false
- windows-vulkan-check.playback_contract_valid=n/a
- windows-vulkan-check.playback_missing_required_fields=n/a
- windows-vulkan-check.playback_failure_detail=not-executed
- windows-vulkan-check.result=SKIPPED

Playback contract-broken simulation:
mock executable: logs/mock_windows_vulkan_gate_player.cmd
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "logs/mock_windows_vulkan_gate_player.cmd" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk026-playback-contract-broken.env"
Result: expected FAIL (exit 2)
Key lines:
- windows-vulkan-check.playback_check_executed=true
- windows-vulkan-check.playback_contract_valid=false
- windows-vulkan-check.playback_missing_required_fields=performance-log-check.startup_selected_renderer,performance-log-check.renderer_backend,performance-log-check.startup_renderer_candidates,performance-log-check.startup_renderer_plan_reason
- windows-vulkan-check.playback_failure_detail=contract-missing-required-fields
- windows-vulkan-check.failure_reason=vulkan-playback-contract-broken
- windows-vulkan-check.result=FAIL

Compatibility spot-check:
1) auto + sdk=1 + runtime_probe=0 -> result=SKIPPED (exit 0)
2) auto + sdk=1 + runtime_probe=1 -> result=FAIL (exit 2, expected on current host)
```

### Notes
1. This round hardens playback contract observability; availability/strict policy logic remains unchanged.
2. Gate now separates playback contract regressions from generic playback semantic failures.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 153: Vulkan chain VK-025 Windows diagnostics contract validation

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added required-key contract validation for `--vulkan-diagnostics` output in Windows Vulkan gate.
- Added machine-readable diagnostics contract observability fields and contract-broken failure path.

### Log
```text
Static path scan:
rg -n "diag_contract_valid|diag_missing_required_fields|vulkan-diagnostics-contract-broken|diag-contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk025-baseline.env"
Key lines:
- windows-vulkan-check.diag_contract_valid=true
- windows-vulkan-check.diag_missing_required_fields=none
- windows-vulkan-check.result=SKIPPED

Contract-broken simulation:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "C:/Windows/System32/cmd.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk025-contract-broken.env"
Result: expected FAIL (exit 2)
Key lines:
- windows-vulkan-check.diag_contract_valid=false
- windows-vulkan-check.diag_missing_required_fields=vulkan-diagnostics.platform,vulkan-diagnostics.supported_platform,vulkan-diagnostics.compiled_in,vulkan-diagnostics.runtime_available,vulkan-diagnostics.result
- windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken
- windows-vulkan-check.vulkan_availability_failure_detail=diag-contract-missing-required-fields
- windows-vulkan-check.result=FAIL

Compatibility spot-check:
1) auto + sdk=1 + runtime_probe=0 -> result=SKIPPED (exit 0)
2) auto + sdk=1 + runtime_probe=1 -> result=FAIL (exit 2, expected on current host)
```

### Notes
1. This round adds diagnostics-contract safety and observability, not policy behavior changes.
2. Gate now fails fast for malformed diagnostics output to avoid silent misclassification.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 152: Vulkan chain VK-024 Windows availability failure detail classification

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added machine-readable availability-stage failure classification fields to Windows Vulkan gate.
- Preserved strict/optional behavior and legacy result/failure contracts.

### Log
```text
Static path scan:
rg -n "vulkan_availability_probe_passed|vulkan_availability_failure_detail|compiled-in-disabled|diag-result-not-pass" tools/run_windows_vulkan_checks.ps1
Result: PASS

Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk024-baseline.env"
Key lines:
- windows-vulkan-check.vulkan_availability_probe_passed=false
- windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
- windows-vulkan-check.result=SKIPPED

Policy matrix:
1) auto + sdk=1 + runtime_probe=0
- strict_mode_auto_prerequisites_met=false
- mode=optional
- vulkan_availability_failure_detail=compiled-in-disabled
- result=SKIPPED
- exit=0

2) auto + sdk=1 + runtime_probe=1
- strict_mode_auto_prerequisites_met=true
- mode=strict
- vulkan_availability_failure_detail=compiled-in-disabled
- failure_reason=vulkan-not-available-in-strict-mode
- result=FAIL
- exit=2 (expected on current host)
```

### Notes
1. This round improves classification observability only; it does not change decision behavior.
2. Availability failure detail now provides stable machine-readable triage signal for CI artifacts.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 151: Vulkan chain VK-023 Windows runtime probe detail observability

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added machine-readable runtime probe detail publication for Windows Vulkan CI/gate diagnostics.
- Preserved `VK-022` strict-policy behavior while improving root-cause observability.

### Log
```text
Static path scan:
rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL|runner_vulkan_runtime_probe_detail" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
Result: PASS

Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Baseline gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk023-baseline.env"
Key lines:
- windows-vulkan-check.runner_vulkan_runtime_probe_detail=unknown
- windows-vulkan-check.result=SKIPPED

Policy matrix:
1) auto + sdk=1 + runtime_probe=0 + detail=vulkaninfo-missing
- strict_mode_auto_prerequisites_met=false
- mode=optional
- runner_vulkan_runtime_probe_detail=vulkaninfo-missing
- result=SKIPPED
- exit=0

2) auto + sdk=1 + runtime_probe=1 + detail=vulkaninfo-path
- strict_mode_auto_prerequisites_met=true
- mode=strict
- runner_vulkan_runtime_probe_detail=vulkaninfo-path
- result=FAIL
- exit=2 (expected on current host)
```

### Notes
1. This round is observability-only; strict policy logic remains unchanged.
2. Summary artifacts now carry enough signal to classify runtime probe failure reasons directly.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/workstation.

## Issue 150: Vulkan chain VK-022 Windows auto strict runtime probe guard

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Hardened Windows Vulkan `auto` strict promotion with runtime-probe guard to prevent SDK-only false strict escalation.
- Added runtime-probe observability fields in Vulkan gate summary and workflow runtime probe signal export.

### Log
```text
Static path scan:
rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE|strict_mode_auto_basis|strict_mode_auto_prerequisites_met|sdk_and_runtime_probe|vulkaninfo" tools/run_windows_vulkan_checks.ps1 .github/workflows/cross-platform-gate.yml
Result: PASS

Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL

Policy matrix:
1) auto + sdk=1 + runtime_probe=0
- strict_mode_auto_prerequisites_met=false
- mode=optional
- result=SKIPPED
- exit=0

2) auto + sdk=1 + runtime_probe=1
- strict_mode_auto_prerequisites_met=true
- mode=strict
- failure_reason=vulkan-not-available-in-strict-mode
- result=FAIL
- exit=2 (expected on current host)
```

### Notes
1. `auto` policy now aligns with true runner readiness (SDK + runtime probe), not SDK-only inference.
2. Safe downgrade behavior on non-Vulkan hosts remains unchanged.
3. Strict PASS-path still requires a Vulkan-ready Windows runner/workstation.

## Issue 149: Vulkan chain VK-021 Windows dependency-source observability and find_package hardening

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Hardened Windows Vulkan dependency-resolution decision with complete/incomplete package metadata handling.
- Added machine-readable dependency-source observability in both Vulkan diagnostics and Windows gate summary.

### Log
```text
Static path scan:
rg -n "vulkan-diagnostics\.dependency_source|diag_dependency_source|MVP_VULKAN_DEPENDENCY_SOURCE|find_package\(Vulkan\) returned incomplete package info" CMakeLists.txt src/main.cpp tools/run_windows_vulkan_checks.ps1
Result: PASS

Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.dependency_source=disabled
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL

Windows Vulkan gate check:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk021.env"
Result: PASS
Key lines:
- windows-vulkan-check.diag_dependency_source=disabled
- windows-vulkan-check.skip_reason=vulkan-not-available
- windows-vulkan-check.result=SKIPPED
```

### Notes
1. This round improves dependency-resolution observability and robustness, not runtime strict PASS proof.
2. Safe downgrade contract remains unchanged for non-Vulkan hosts.
3. Strict runtime PASS still requires Vulkan-ready Windows runner/runtime.

## Issue 148: Vulkan chain VK-020 Windows CMake SDK fallback and CMake prefix-path closure

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Hardened Windows Vulkan dependency resolution with explicit SDK fallback in CMake.
- Added workflow `CMAKE_PREFIX_PATH` injection when Vulkan SDK is detected.

### Log
```text
Static path scan:
rg -n "VULKAN_SDK fallback|CMAKE_PREFIX_PATH|VULKAN_SDK_ROOT|MVP_WINDOWS_VULKAN_SDK_AVAILABLE|find_package\(Vulkan\) failed and VULKAN_SDK fallback is incomplete" CMakeLists.txt .github/workflows/cross-platform-gate.yml
Result: PASS

Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.platform=Windows
- vulkan-diagnostics.supported_platform=true
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL

Windows Vulkan gate check:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
Result: PASS
Key lines:
- windows-vulkan-check.strict_mode_policy=off
- windows-vulkan-check.compiled_in=false
- windows-vulkan-check.runtime_available=false
- windows-vulkan-check.skip_reason=vulkan-not-available
- windows-vulkan-check.result=SKIPPED
```

### Notes
1. This round closes Windows Vulkan CMake dependency-resolution robustness, not runtime strict PASS proof.
2. Safe downgrade behavior is preserved for non-Vulkan hosts.
3. Runtime strict PASS still requires a Vulkan-ready Windows runner.

## Issue 147: Vulkan chain VK-019 Windows Vulkan auto strict policy promotion

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Promoted Windows Vulkan strict policy to auto mode based on runner SDK readiness signal.
- Added strict policy/effective-state machine-readable fields for diagnostics and CI traceability.

### Log
```text
Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Policy matrix:
1) auto + sdk=0
- mode=optional
- strict_mode_effective=false
- result=SKIPPED
- exit=0

2) auto + sdk=1
- mode=strict
- strict_mode_effective=true
- failure_reason=vulkan-not-available-in-strict-mode
- result=FAIL
- exit=2 (expected on current host)

3) off + sdk=1
- mode=optional
- strict_mode_effective=false
- result=SKIPPED
- exit=0
```

### Notes
1. Workflow default policy now uses `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`.
2. Auto mode ensures strict promotion only when runner SDK availability signal is true.
3. Strict PASS-path still depends on Vulkan-ready Windows runner/runtime.

## Issue 146: Vulkan chain VK-018 Windows Vulkan SDK provisioning and CI observability

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added optional Windows Vulkan SDK provisioning stage into CI workflow.
- Extended Windows Vulkan gate summary output with SDK observability fields.

### Log
```text
Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Windows Vulkan check summary:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
Result: PASS
Key lines:
- windows-vulkan-check.vulkan_sdk_present=false
- windows-vulkan-check.vulkan_sdk_path=
- windows-vulkan-check.runner_vulkan_sdk_available=false
- windows-vulkan-check.result=SKIPPED
```

### Notes
1. Workflow now attempts Windows Vulkan SDK provisioning and exports runner SDK availability signal.
2. Gate summary now includes SDK context fields for easier diagnosis of compiled/runtime Vulkan state.
3. PASS-path Vulkan runtime proof remains dependent on Vulkan-ready Windows runner.

## Issue 145: Vulkan chain VK-017 Windows Vulkan gate strict policy and summary artifact

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Extended Windows Vulkan gate script with persisted summary artifact output.
- Added env-driven strict policy hook and integrated summary/log capture in CI Windows lane.

### Log
```text
Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Optional mode with summary output:
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
Result: PASS (exit code 0)
Key lines:
- windows-vulkan-check.mode=optional
- windows-vulkan-check.strict_mode_env_requested=false
- windows-vulkan-check.result=SKIPPED

Env strict mode with summary output:
MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=1 + same command
Result: expected FAIL (exit code 2)
Key lines:
- windows-vulkan-check.mode=strict
- windows-vulkan-check.strict_mode_env_requested=true
- windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-check.result=FAIL

Summary outputs:
- logs/windows-vulkan-gate-summary.env generated
- logs/windows-vulkan-gate-summary-strict.env generated
```

### Notes
1. Windows Vulkan gate now supports both stdout machine-readable lines and persisted summary artifact files.
2. Strict policy can be toggled via environment (`MVP_REQUIRE_WINDOWS_VULKAN_CHECKS`) without script argument changes.
3. Strict PASS-path verification still needs Vulkan-ready Windows host/runner.

## Issue 144: Vulkan chain VK-016 Windows Vulkan gate and CI integration

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added a dedicated Windows Vulkan gate command with machine-readable output and optional/strict mode policy.
- Integrated Windows Vulkan gate command into GitHub Actions Windows lane.

### Log
```text
Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Windows Vulkan check (optional mode):
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200
Result: PASS (exit code 0)
Key lines:
- windows-vulkan-check.supported_platform=true
- windows-vulkan-check.compiled_in=false
- windows-vulkan-check.runtime_available=false
- windows-vulkan-check.skip_reason=vulkan-not-available
- windows-vulkan-check.result=SKIPPED

Windows Vulkan check (strict mode):
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -RequireVulkanAvailable
Result: expected FAIL (exit code 2)
Key lines:
- windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
- windows-vulkan-check.result=FAIL
```

### Notes
1. Windows Vulkan gate contract is now machine-readable and CI-integrated.
2. Optional mode keeps current non-Vulkan hosts non-blocking; strict mode is ready for enforced environments.
3. PASS-path runtime proof still requires Windows host/runner with Vulkan dependency/runtime available.

## Issue 143: Vulkan chain VK-015 Windows link/runtime probe closure

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Closed Windows Vulkan enablement gaps in link-stage dependency wiring and capability runtime truth publication.
- Kept diagnostics/fallback contracts stable while making runtime availability probe-driven.

### Log
```text
Configure with Vulkan ON:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.platform=Windows
- vulkan-diagnostics.supported_platform=true
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL

Vulkan override fallback observability:
$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200; Remove-Item Env:MVP_RENDERER_BACKEND
Result: PASS
Key lines:
- performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
- performance-log-check.result=PASS

Windows baseline diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS
```

### Notes
1. Windows final link stage now consumes Vulkan-resolved platform libraries when available.
2. Vulkan runtime availability publication is now probe-based instead of static.
3. Current host still lacks Vulkan package; true `compiled_in=true` runtime path needs validation on Vulkan-equipped Windows host.

## Issue 142: Vulkan chain VK-015 Windows enablement implementation

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Enabled Windows-side Vulkan build contract with safe dependency downgrade behavior.
- Extended Vulkan diagnostics supported-platform contract from Linux-only to Windows+Linux.

### Log
```text
Configure with Vulkan ON:
cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON
Result: PASS
Key warning:
- ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF

Build:
cmake --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.platform=Windows
- vulkan-diagnostics.supported_platform=true
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL

Vulkan override fallback observability:
$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200; Remove-Item Env:MVP_RENDERER_BACKEND
Result: PASS
Key lines:
- performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
```

### Notes
1. Current workstation lacks Vulkan SDK/runtime package, so Windows Vulkan remains compile-disabled by designed fallback.
2. Platform contract for Vulkan diagnostics is now correct on Windows.
3. Next step is validation on a Windows host with Vulkan package installed (`compiled_in=true` path).

## Issue 141: Vulkan chain VK-014 documentation and release closure

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Completed final documentation and release closure for Vulkan chain.
- Synchronized fixed records and index entrances after `VK-013` matrix report.

### Log
```text
Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Index/record path scan:
rg -n "DAY75_VK014|DAY74_VK013|REGRESSION_MATRIX_EXECUTION|DOCUMENTATION_AND_RELEASE_CLOSURE" docs/analysis/README.md docs/design/README.md docs/plans/README.md docs/reports/README.md docs/records/VERSION.md docs/records/CHANGELOG.md docs/records/DEVELOP_LOG.md
Result: PASS

Workspace status check:
git status --short
Result: PASS (expected modified/untracked working set, no commit/push)
```

### Notes
1. `VK-014` closes local documentation/release synchronization for Vulkan chain.
2. Linux Vulkan runtime PASS evidence remains runner-dependent.
3. No commit/push executed in this round.

## Issue 140: Vulkan chain VK-013 regression matrix execution

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Executed Vulkan-stage regression matrix for open/play/pause/seek/subtitle/fallback coverage.
- Archived results into dedicated `VK-013` local-check report.

### Log
```text
Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Open/play baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

Seek burst serial:
.\build\Release\modern-video-player.exe --seek-burst-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 6
Result: first run FAIL, immediate rerun PASS

Paused seek serial:
.\build\Release\modern-video-player.exe --paused-seek-serial-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 4
Result: PASS

Subtitle style:
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_style_validation.ass
Result: PASS

Subtitle sync:
.\build\Release\modern-video-player.exe --subtitle-sync-check .\samples\subtitles\opengl_ass_style_validation.ass
Result: PASS

Renderer fallback:
.\build\Release\modern-video-player.exe --renderer-fallback-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4
Result: PASS

Vulkan override fallback observability:
$env:MVP_RENDERER_BACKEND='vulkan'; .\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200; Remove-Item Env:MVP_RENDERER_BACKEND
Result: PASS

Vulkan diagnostics:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.supported_platform=false
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL
Result: expected FAIL on Windows host
```

### Notes
1. Matrix coverage for `VK-013` is archived and linked.
2. Seek-burst first-run FAIL with rerun PASS is retained as residual nondeterministic risk.
3. Next step in sequence is `VK-014` documentation/release closure.

## Issue 139: Vulkan chain VK-012 GitHub Actions Linux Vulkan lane

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Wired Vulkan build/check enforcement into GitHub Actions Linux lane.
- Connected workflow lane to strict Vulkan gate mode introduced in `VK-011`.

### Log
```text
Workflow field scan:
rg -n "libvulkan-dev|mesa-vulkan-drivers|ENABLE_VULKAN_RENDERER=ON|logs/linux-mvp-gate-summary.env|Run Linux gate|Install Linux dependencies|Configure Linux build" .github/workflows/cross-platform-gate.yml
Result: PASS

Linux gate script syntax:
& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics baseline:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.supported_platform=false
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL
```

### Notes
1. Local Windows host validates workflow/script/build consistency only; CI Linux runner still needed for runtime proof.
2. Linux lane now installs Vulkan packages and explicitly enables Vulkan renderer at configure time.
3. Next step in sequence is `VK-013` (regression matrix execution).

## Issue 138: Vulkan chain VK-011 Linux gate Vulkan checks

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added Vulkan diagnostics integration stage into Linux gate script.
- Added capability-aware probe/skip/fail policy and machine-readable report fields for Vulkan gate.

### Log
```text
Script path scan:
rg -n "REQUIRE_VULKAN_CHECKS|probe_vulkan_check_availability|gate.has_vk010|vk010_vulkan_diagnostics|vulkan_skip_reason|--vulkan-diagnostics" tools/run_linux_mvp_checks.sh
Result: PASS

Script syntax:
& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh
Result: PASS

Non-Linux dispatch guard:
& "C:\Program Files\Git\bin\bash.exe" tools/run_linux_mvp_checks.sh
Output: This gate script only supports Linux.
Result: expected FAIL on Windows host

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Vulkan diagnostics baseline:
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.supported_platform=false
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.result=FAIL
```

### Notes
1. Windows host cannot execute Linux gate runtime checks; only syntax/dispatch validation is available locally.
2. Vulkan gate stage is now integrated with capability-aware skip behavior and strict-mode hook.
3. Next step in sequence is `VK-012` (GitHub Actions Linux Vulkan lane).

## Issue 137: Vulkan chain VK-010 Vulkan diagnostics CLI

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added dedicated `--vulkan-diagnostics` machine-readable command path.
- Reused startup strategy/capability projection to expose Vulkan compile/runtime/fallback observability.

### Log
```text
Code-path scan:
rg -n "runVulkanDiagnostics|--vulkan-diagnostics|vulkan-diagnostics\.|rendererCandidateChainToString" src/main.cpp
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_renderer_plan_reason=platform-default-order
- performance-log-check.startup_decoder_plan_reason=hardware-first
- performance-log-check.result=PASS

Vulkan diagnostics (default env, Windows host):
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.supported_platform=false
- vulkan-diagnostics.compiled_in=false
- vulkan-diagnostics.runtime_available=false
- vulkan-diagnostics.selected_renderer=D3D11
- vulkan-diagnostics.result=FAIL

Vulkan override observability (Windows host):
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --vulkan-diagnostics
Key lines:
- vulkan-diagnostics.requested_renderer_override=vulkan
- vulkan-diagnostics.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- vulkan-diagnostics.startup_renderer_plan_reason=renderer-override-env
- vulkan-diagnostics.selected_renderer=D3D11
- vulkan-diagnostics.fallback_target=D3D11
- vulkan-diagnostics.result=FAIL
```

### Notes
1. Non-Linux host result is expected `FAIL`; machine-readable unsupported signal is intentional.
2. Linux host/runner execution remains required for `vulkan-diagnostics.result=PASS`.
3. Next step in sequence is `VK-011` (Linux gate Vulkan checks).

## Issue 136: Vulkan chain VK-009 fallback chain and startup policy

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added explicit Linux Vulkan fallback-chain normalization in startup strategy.
- Added machine-readable startup plan-reason fields in diagnostics/performance output.

### Log
```text
Policy/observability code-path scan:
rg -n "normalizeLinuxVulkanFallbackChain|linux-vulkan-fallback-chain|startup_renderer_plan_reason|startup_decoder_plan_reason" src/core/playback_strategy.cpp include/core/player_core.h src/core/player_core.cpp src/main.cpp
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- performance-log-check.startup_renderer_plan_reason=platform-default-order
- performance-log-check.startup_decoder_plan_reason=hardware-first
- result=PASS

Vulkan override fallback observability on Windows:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- performance-log-check.startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_renderer_plan_reason=renderer-override-env
- performance-log-check.startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. Startup policy observability now includes strategy reason metadata, not only candidates/fallback result.
2. Linux-specific Vulkan fallback-chain runtime proof still requires Linux host/runner.
3. Next step in sequence is `VK-010` (Vulkan diagnostics CLI).

## Issue 135: Vulkan chain VK-008 sync and present pacing

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Added Vulkan present-mode request/selection/fallback path for pacing policy control.
- Hardened in-flight fence wait with timeout and recovery to avoid unbounded blocking.
- Added sync/pacing counters and periodic runtime snapshot logging.

### Log
```text
Sync/pacing code-path scan:
rg -n "MVP_VULKAN_PRESENT_MODE|present_mode_requested|present_mode_active|kFrameFenceWaitTimeoutNs|fence_wait_timeout|Vulkan pacing stats|choosePresentMode\(" src/render/vulkan_video_renderer.cpp
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=none
- result=PASS

Vulkan override + present-mode override fallback observability on Windows:
$env:MVP_RENDERER_BACKEND='vulkan'
$env:MVP_VULKAN_PRESENT_MODE='immediate'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. Windows baseline regressions remain stable after Vulkan sync/pacing code changes.
2. Linux runner validation is still required for real Vulkan runtime pacing behavior.
3. Next step in sequence is `VK-009` (fallback chain and startup policy closure).

## Issue 134: Vulkan chain VK-007 frame upload YUV path

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Completed Vulkan baseline frame upload chain for decoded YUV frames.
- Wired upload payload into present-stage command path and completed Vulkan direct-format contract.

### Log
```text
Code path scan:
rg -n "recordClearCommandBuffer|recordPresentCommandBuffer|supportsDirectFrameFormat\(" src/render/vulkan_video_renderer.cpp include/render/vulkan_video_renderer.h
Result: PASS

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=none
- result=PASS

Vulkan override fallback observability on Windows:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. Windows regression checks remain stable after Vulkan path code updates.
2. Linux host/runner execution is still required for real Vulkan upload/display runtime proof.
3. Next step in sequence is `VK-008` (sync and present pacing).

## Issue 133: Vulkan chain VK-006 clear/present and swapchain recreate

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Implemented Vulkan minimal clear/present frame loop and swapchain recreate baseline.
- Added frame sync objects and resize-triggered swapchain rebuild flow.

### Log
```text
Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=none
- result=PASS

Vulkan override fallback observability on Windows:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. Existing Windows runtime paths remain stable.
2. Vulkan clear/present/recreate behavior still needs Linux host/runner runtime proof.
3. `VK-007` will build frame upload path on top of this baseline.

## Issue 132: Vulkan chain VK-005 instance/surface/device/swapchain init

**Date**: 2026-03-27
**Status**: Resolved

### Description
- Implemented Vulkan runtime initialization baseline in `VulkanVideoRenderer`.
- Added full init/close lifecycle for SDL/Vulkan core objects and baseline event handling.

### Log
```text
Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Performance baseline:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=none
- result=PASS

Vulkan override fallback observability on Windows:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. Windows host validates no regression in existing paths.
2. Vulkan code path is Linux-first and requires Linux host/runner for compile/runtime proof.
3. `VK-006` will continue from this lifecycle baseline with clear/present/swapchain recreate.

## Issue 131: Vulkan chain VK-004 renderer skeleton and factory wiring

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Added Vulkan renderer skeleton backend and wired it through enum/factory/capability/strategy.
- Kept staged behavior conservative: skeleton `init()` returns `false`, forcing observable fallback.

### Log
```text
Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

Vulkan override fallback:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- startup_renderer_candidates=Vulkan -> D3D11 -> SoftwareSDL -> OpenGL
- startup_renderer_fallback_reason=fallback-after-renderer-failure
- result=PASS
```

### Notes
1. `VK-004` closes compile/wiring baseline only.
2. Real Vulkan init chain starts at `VK-005`.

## Issue 130: Vulkan chain VK-003 CMake switches and dependency detect

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Added Vulkan renderer build switch and compile macro contract.
- Added unsupported-host and missing-dependency downgrade behavior.

### Log
```text
Configure (forced-on on Windows host):
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build-vk-on-win -G "Visual Studio 17 2022" -A x64 -DENABLE_VULKAN_RENDERER=ON
Key lines:
- ENABLE_VULKAN_RENDERER requires Linux/Unix non-Apple; forcing OFF
- Feature switches: ... VULKAN_RENDERER=OFF ...

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS
```

### Notes
1. Switch path is deterministic and safe on unsupported hosts.
2. Linux dependency probe path is ready for runner-side Vulkan package verification.

## Issue 129: Vulkan chain VK-002 architecture and strategy integration

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Finalized Vulkan architecture integration through existing strategy/factory contracts.
- Kept `PlayerCore` policy-neutral, fallback observable.

### Log
```text
Integration scan:
rg -n "enum class VideoRendererType|VideoRendererType::|RendererFactory::create|RendererFactory::isSupported|MVP_RENDERER_BACKEND|renderer_candidates" include src CMakeLists.txt -S
Result: PASS

Runtime observability:
$env:MVP_RENDERER_BACKEND='vulkan'
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS
```

### Notes
1. Architecture boundary is stable for next implementation steps.
2. No `PlayerCore` policy fork introduced.

## Issue 128: Vulkan chain VK-001 scope and acceptance freeze

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Started Vulkan implementation sequence with `VK-001` scope/DoD freeze before code changes.
- Confirmed repository baseline has no existing Vulkan backend/switch/diagnostics path.
- Synced full documentation chain (`analysis/design/plan/report` + indexes + records).

### Log
```text
Vulkan baseline probe:
rg -n "Vulkan|vulkan|ENABLE_VULKAN_RENDERER|MVP_HAVE_VULKAN_RENDERER|--vulkan-diagnostics|vk" include src tools .github/workflows CMakeLists.txt -S
Result: PASS (no existing Vulkan path found)

Build:
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS
```

### Notes
1. `VK-001` is documentation/contract freeze only; no code implementation started.
2. `VK-002` (architecture and strategy integration design) is the next execution step.
3. Code changes remain queued from `VK-003` onward after confirmation.

## Issue 127: Linux workflow Build Linux Release compile blocker closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Parsed failed Linux workflow runs and fixed current compile blockers in subtitle/OpenGL paths.
- Kept Windows path behavior unchanged and added non-Windows helper visibility where class code depends on shared symbols.

### Log
```text
Workflow failure inspection:
gh run view 23601824744 --job 68733664519 --log
gh run view 23601841417 --json status,conclusion,event,jobs
Result: Linux Build Linux Release failure reproduced and root causes identified.

Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

Performance regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS
```

### Notes
1. Linux runner evidence still requires push + GitHub Actions rerun.
2. Existing encoding warnings in unrelated files remain out of this patch scope.

## Issue 126: Workflow log FFmpeg duration compatibility closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Parsed `log` workflow error output and closed remaining FFmpeg-old compile compatibility gap.
- Added `AVFrame` duration compatibility fallback (`duration` / `pkt_duration`) and updated decode timing reads.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS
```

### Notes
1. This closes the unresolved compile error subset from the provided workflow `log` file.
2. Final Linux runtime gate PASS still requires remote CI run after push.
## Issue 125: Linux CI compatibility stabilization (FFmpeg/libass + workflow determinism)

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Linux CI compatibility regressions found after previous cross-platform closure.
- Landed FFmpeg channel-layout compatibility migration, Linux compile blocker fixes, and workflow-level deterministic gate inputs.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player sample_logger_plugin
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)
```

### Notes
1. `CP-101 ~ CP-106` remain complete; this round is Linux CI compatibility/stability closure, not scope expansion.
2. macOS items `CP-1001 ~ CP-1005` stay deferred by scope decision.
3. Final Linux runtime PASS evidence still requires CI/Linux runner execution after push.
## Issue 124: Linux gate reporting/artifact closure for CI evidence reuse

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Continued Linux cross-platform closure by adding reusable Linux gate evidence artifacts.
- Added machine-readable report output in Linux gate script and CI log/report archival wiring.

### Log
```text
Linux gate script syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)

Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS
```

### Notes
1. Linux gate now supports a structured report output file (`MVP_LINUX_GATE_REPORT_FILE` / arg #10).
2. Linux CI lane now archives both raw log and summary report artifacts.
3. Full Linux runtime evidence still requires Linux host/CI run.

## Issue 123: Linux gate strict optional checks closure for CP-507/CP-508 in CI

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Continued Linux cross-platform closure work by hardening Linux gate determinism for subtitle backlog checks.
- Added CP-508 fixture auto-generation and strict optional-check mode into Linux gate script.
- Updated CI Linux lane to install `ffmpeg` and enforce strict mode execution.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS

Linux shell syntax check:
C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh
Result: PASS (syntax check)

Linux gate runtime dispatch check:
C:\Program Files\Git\bin\bash.exe tools/run_linux_mvp_checks.sh
Result: expected FAIL on Windows host (`This gate script only supports Linux.`)
```

### Notes
1. Linux runtime proof still requires real Linux host or CI run results (syntax check alone is not runtime evidence).
2. CI input contract is now strict enough to prevent CP-507/CP-508 silent skips.

## Issue 122: CP-507 and CP-508 Linux subtitle backlog closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed remaining Linux subtitle backlog items in the cross-platform tasklist:
  - `CP-507` (`libass` shaping/layout probe baseline)
  - `CP-508` (embedded subtitle live packet probe baseline)
- Added CLI contracts and Linux gate-script integration points.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Regression:
.\build\Release\modern-video-player.exe --performance-log-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Result: PASS

D3D11 diagnostics:
.\build\Release\modern-video-player.exe --d3d11-diagnostics
Result: PASS

CP-507 command:
.\build\Release\modern-video-player.exe --libass-shaping-check .\samples\subtitles\opengl_ass_style_validation.ass
Result: FAIL on this host (expected, platform=NonLinux)

CP-508 command:
.\build\Release\modern-video-player.exe --embedded-subtitle-live-packet-check .\build\tmp\embedded-ass-validation.mkv -1 120
Result: PASS
```

### Notes
1. Linux runtime evidence is still required for `--libass-shaping-check` and full Linux gate script execution.
2. Packet-level probe output for embedded ASS stream was validated locally (`stream_index=2`, `subtitle_packets_read=3`, `produced_output=true`).
# 寮€鍙戞棩蹇?
## Issue 120: CP-501 ~ CP-506 subtitle/font platform closure

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Phase-5 tasks `CP-501` to `CP-506` end-to-end.
- Fixed policy integration compile blocker in `main.cpp`.
- Added Linux attachment-font registration/rebuild flow in `subtitle_font_registry` using `fontconfig`.
- Added DirectWrite custom collection check coverage into `tools/run_opengl_checks.ps1`.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release
Result: PASS

Phase5 CLI checks:
.\build\Release\modern-video-player.exe --embedded-subtitle-policy-check .\build\tmp\embedded-ass-validation.mkv eng,chi prefer avoid
.\build\Release\modern-video-player.exe --subtitle-ownership-check .\build\tmp\embedded-text-validation.mp4
.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\attachment-font-check.mkv
.\build\Release\modern-video-player.exe --settings-persistence-check
Result: PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"
Result: OpenGL gate result: PASS (18/18)
```

### Analysis Notes
1. `CP-505` Linux path is now implemented in code, but this Windows host cannot provide runtime validation for the new `fontconfig` branch.
2. `CP-506` gate maturation is now complete on Windows path (`--directwrite-font-collection-check` included in scripted gate).
3. Phase-4 report placeholders were replaced by actual local execution results and platform-expectation notes.

## Issue 119: CP-401 ~ CP-406 Linux MVP playback closure and gate commands

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Closed Phase-4 Linux MVP playback tasks `CP-401` to `CP-406`.
- Added the full Linux MVP command surface and Linux gate baseline script.
- Added controlled OpenGL init failure injection so fallback-chain validation is deterministic.

### Log Output
```text
Build:
cmake -S . -B build
cmake --build build --config Release --target modern-video-player
configure/build succeeded

Windows-side validation of shared runtime gates:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
performance-log-check.result=PASS

.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
renderer-fallback-check.result=PASS

.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
interaction-freeze-check.result=PASS
```

### Analysis
1. Phase-4 closure needed explicit Linux-only CLI commands; otherwise the task list could say done while automation still had no stable command contract.
2. Fallback verification without a deterministic failure trigger is not a real gate, because it depends on incidental runtime conditions.
3. The Linux gate script had to exist even before a Linux host was available locally, otherwise CI could not assume one shared Phase-4 entry point.
4. This workstation can validate command wiring and shared playback checks, but cannot provide the missing Linux runtime evidence by itself.

### Result
- Added Phase-4 commands in `src/main.cpp`:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added `MVP_OPENGL_FORCE_INIT_FAIL` handling in `src/render/opengl_video_renderer.cpp` to force OpenGL init failure for fallback validation.
- Added `tools/run_linux_mvp_checks.sh` as the Phase-4 Linux MVP gate script baseline.
- Updated Phase-4 analysis/design/report docs and marked `CP-401 ~ CP-406` done in the master task list.

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
**Status**: Resolved

### Description
- Completed Phase-3 baseline tasks `CP-301` to `CP-305`.
- Added explicit backend feature switches and platform guard enforcement in CMake.
- Added startup diagnostics compiled/runtime set fields and Linux packaging baseline (`DEB/TGZ` + helper script).

### Log
```text
Default configure/build:
cmake -S . -B build
cmake --build build --config Release --target modern-video-player
Result: PASS

Default runtime regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: PASS
Key lines:
- performance-log-check.startup_renderer_compiled_set=D3D11, SoftwareSDL, OpenGL
- performance-log-check.startup_renderer_runtime_set=D3D11, SoftwareSDL, OpenGL
- performance-log-check.startup_decoder_compiled_set=D3D11VA, Software
- performance-log-check.startup_decoder_runtime_set=D3D11VA, Software

Switch matrix build check:
cmake -S . -B build_nod3d11 -DENABLE_D3D11_RENDERER=OFF -DENABLE_D3D11VA=OFF -DENABLE_OPENGL_RENDERER=ON -DENABLE_SDL_RENDERER=ON
cmake --build build_nod3d11 --config Release --target modern-video-player
Result: PASS

cmake -S . -B build_noopengl -DENABLE_OPENGL_RENDERER=OFF -DENABLE_D3D11_RENDERER=ON -DENABLE_SDL_RENDERER=ON -DENABLE_D3D11VA=ON
cmake --build build_noopengl --config Release --target modern-video-player
Result: PASS

Command guard behavior:
.\build_nod3d11\Release\modern-video-player.exe --d3d11-diagnostics
-> supported_platform=false (expected fallback path)
.\build_noopengl\Release\modern-video-player.exe --opengl-diagnostics
-> supported_platform=false (expected fallback path)
```

### Notes
1. Backend source/link composition is now explicit and switch-controlled; no more implicit always-on backend compilation.
2. Linux dependency and package baseline are codified in CMake + script, but Linux native package execution is still pending Linux environment verification.
3. `build_nod3d11` software-decode runtime path still exposes an existing blocker unrelated to CP-301~305 scope.

## Issue 117: CP-201 ~ CP-205 renderer/input/overlay cleanup and interaction freeze regression gate

**Date**: 2026-03-26
**Status**: Resolved

### Description
- Completed Phase-2 cleanup tasks `CP-201` to `CP-205`.
- Split renderer/input/overlay responsibilities into explicit interfaces and rewired `PlayerCore`/idle window paths to consume role interfaces.
- Added main-thread event-pump guards and landed machine-readable interaction freeze stress check with OpenGL gate integration.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Runtime regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.renderer_backend=D3D11
- performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.result=PASS

Renderer fallback:
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
Key lines:
- renderer-fallback-check.renderer_backend=SoftwareSDL
- renderer-fallback-check.fallback_to_sdl=true
- renderer-fallback-check.result=PASS

Interaction freeze stress:
.\build\Release\modern-video-player.exe --interaction-freeze-check .\juren-30s.mp4 1800
Key lines:
- interaction-freeze-check.injected_events_total=251
- interaction-freeze-check.render_progress_ok=true
- interaction-freeze-check.illegal_transition_ok=true
- interaction-freeze-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- [5/17] OpenGL interaction freeze stress
- OpenGL gate result: PASS
```

### Notes
1. `PlayerCore` now treats input polling and overlay updates as independent role interfaces rather than renderer-control methods.
2. Event-pump affinity violations are now fail-safe (warn-once + ignore), reducing deadlock/freeze regression risk.
3. Phase-2 cleanup baseline is closed; next execution target is `CP-301+` build/dependency platformization.

## Issue 116: CP-101 ~ CP-106 startup strategy/capability extraction

**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed Phase-1 startup strategy extraction tasks `CP-101` to `CP-106`.
- Introduced platform capability probe + playback strategy planning and moved `PlayerCore` open path to plan execution.
- Added machine-readable startup strategy diagnostics export.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player
Result: PASS

Startup diagnostics regression:
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.startup_platform=Windows
- performance-log-check.startup_renderer_candidates=D3D11 -> SoftwareSDL -> OpenGL
- performance-log-check.startup_decoder_candidates=D3D11VA -> Software
- performance-log-check.startup_selected_renderer=D3D11
- performance-log-check.startup_selected_decoder=D3D11VA
- performance-log-check.startup_renderer_fallback_reason=none
- performance-log-check.startup_decoder_fallback_reason=none
- performance-log-check.result=PASS

Renderer fallback regression:
.\build\Release\modern-video-player.exe --renderer-fallback-check .\juren-30s.mp4
Key lines:
- renderer-fallback-check.renderer_backend=SoftwareSDL
- renderer-fallback-check.fallback_to_sdl=true
- renderer-fallback-check.result=PASS
```

### Notes
1. `RendererFactory` no longer owns default backend policy; strategy ordering now lives in `PlaybackStrategy`.
2. `DecoderFactory` ordering now consumes context + capabilities and always keeps software fallback.
3. This closes Phase-1 strategy extraction baseline while preserving Windows default behavior.

## Issue 115: Cross-platform Phase 8 ICC output LUT and per-display diagnostics closure
**Date**: 2026-03-26
**Status**: Resolved (`CP-802 ~ CP-805`)

### Description
- Closed the Phase 8 runtime color-management gaps except for the real DXGI HDR present bridge (`CP-801`).
- Added ICC/profile-driven 3D LUT generation, per-display output binding, runtime reload, and machine-readable diagnostics.
- Added dedicated auto ICC regression coverage in the OpenGL gate.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

Manual output color:
.\build\Release\modern-video-player.exe --opengl-output-color-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\lut\identity_2.cube 1200
Key lines:
- output_lut_source=cube
- output_lut_active=true
- output_display_device_name=WinDisc
- result=PASS

Auto ICC output color:
.\build\Release\modern-video-player.exe --opengl-output-color-icc-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 1200
Key lines:
- output_lut_source=icc-display
- output_icc_profile_available=true
- output_icc_profile_path=C:\Windows\system32\spool\drivers\color\sRGB Color Space Profile.icm
- output_icc_profile_description=sRGB IEC61966-2.1
- result=PASS

OpenGL diagnostics:
.\build\Release\modern-video-player.exe --opengl-diagnostics
Key lines:
- output_display.binding_succeeded=true
- output_display.icc_profile_available=true
- output_display.icc_profile_source=display-auto
- result=PASS

OpenGL gate:
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
Result: OpenGL gate result: PASS (25/25)
```

### Analysis Notes
1. Day44 already had the policy surface; the real missing work was runtime binding and observability.
2. The current display binding needed to be modeled first, otherwise auto ICC and future HDR present work would stay static and misleading.
3. Manual `.cube` override remains first priority so the new ICC path does not break existing workflows.
4. `CP-801` remains open because a real DXGI HDR present bridge is still a separate rendering/present integration task, not a diagnostics task.

### Files
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
**Status**: Resolved

### Description
- Closed `CP-601 ~ CP-605` by finishing packet-level bitmap subtitle modeling, cache reuse, and dedicated regression coverage.
- Added real DVD / PGS bitmap subtitle validation plus synthetic multi-rect stress validation.
- Fixed the real PGS sample case where invalid display-window metadata inflated bitmap subtitle duration.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

Bitmap subtitle CLI:
.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-dvd-validation.mkv
Key lines:
- codec_name=dvd_subtitle
- bitmap_item_count=1
- bitmap_rect_count=1
- mismatches=0
- result=PASS

.\build\Release\modern-video-player.exe --bitmap-subtitle-check .\build\tmp\embedded-pgs-validation.mkv
Key lines:
- codec_name=hdmv_pgs_subtitle
- bitmap_item_count=1
- bitmap_rect_count=1
- ordered_checks=18
- mismatches=0
- result=PASS

Synthetic stress:
.\build\Release\modern-video-player.exe --bitmap-subtitle-stress-check
Key lines:
- multi_rect_item_count=2
- cache_reuse_candidate_count=2
- max_active_item_count=3
- max_active_rect_count=5
- result=PASS

OpenGL gate:
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath 'build/Release/modern-video-player.exe' -ProbeFile 'samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4'
Result: OpenGL gate result: PASS (23/23)
```

### Analysis Notes
1. The existing bitmap subtitle baseline was real but incomplete; Phase 6 closure required modeling, cache, and validation to converge together.
2. Packet-level aggregation removes the old one-rect-per-item mismatch and makes multi-rect composition measurable.
3. Real PGS samples can surface invalid display timing metadata; fallback hardening is necessary for stable runtime behavior.
4. Linux host runtime evidence for bitmap subtitle playback is still needed before claiming full cross-platform parity.

### Files
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

## Issue 92: OpenGL/D3D11 subtitle run-level karaoke, clip and subtitle clock convergence

**Date**: 2026-03-24
**Status**: Resolved

### Problem Description
- GPU subtitle rendering still had a gap between parser/model support and actual renderer behavior for ASS karaoke and rectangular clip semantics.
- OpenGL in particular still needed subtitle-clock-driven texture invalidation and run-level D2D rendering convergence.

### Logs / Validation
```text
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
delay-adjust-check.result=PASS
d3d11-diagnostics.result=PASS
opengl-diagnostics.result=PASS
```

### Analysis Notes
- Closed subtitle clock propagation from `PlayerCore` into renderer-side animated subtitle redraw decisions.
- Completed run-level D2D subtitle rendering on both D3D11 and OpenGL.
- Added basic rectangular `iclip` rendering support through inverse clip region decomposition.
- Added a dedicated karaoke/clip regression sample and exposed the new style/run fields through `--subtitle-style-check`.

## Issue 93: ASS move/fad/fade animation converged into D3D11/OpenGL subtitle rendering

**Date**: 2026-03-24
**Status**: Resolved

### Problem Description
- The GPU subtitle chain still lacked practical ASS line animation semantics even after karaoke/clip support was added.
- Without move/fade integration, subtitle motion and opacity behavior still lagged behind mature players.

### Logs / Validation
```text
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
delay-adjust-check.result=PASS
d3d11-diagnostics.result=PASS
opengl-diagnostics.result=PASS
```

### Analysis Notes
- Added a dedicated line-level animation container on subtitle items.
- Added parser coverage for `\move`, `\fad` and `\fade`.
- Added renderer-side move interpolation and fade opacity evaluation on both D3D11 and OpenGL.
- Verified animation fields are exposed through `--subtitle-style-check` and covered by a dedicated regression sample.


## Issue 94: ASS transform, vector drawing/clip, and font fallback groundwork

**Date**: 2026-03-25
**Status**: Resolved

### Problem Description
- The next ASS convergence batch still needed `\org`, `\fax`, `\fay`, `\frx`, `\fry`, vector drawing / vector clip, and font fallback groundwork across the GPU subtitle path.
- OpenGL runtime validation started failing after the new vector clip path landed.

### Log Output
```text
OpenGL subtitle D2D draw failed: hr=-2003238890
subtitle-style-check.result=PASS
subtitle-sync-check.result=PASS
OpenGL delay-adjust-check.result=PASS
D3D11 delay-adjust-check.result=PASS
```

### Analysis Notes
- `hr=-2003238890` was decoded to `D2DERR_PUSH_POP_UNBALANCED (0x88990016)`.
- The failure came from mismatched clip layer state in the renderer, not from missing D3D11/OpenGL/NV12 hardware capability.
- `draw_text_pass()` had an unmatched `PopLayer()` and `draw_box_region()` had a missing `PopLayer()`.
- The same bug pattern existed in both OpenGL and D3D11 subtitle renderers, so both were corrected together.
- Subtitle-sidecar/private font registration was added as groundwork, but full container attachment extraction is still pending.

### Files Updated
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
**Status**: Resolved

### Description
- Added idle startup window behavior for direct exe launch without media args.
- Added drag-drop media open support in SDL, D3D11, and OpenGL renderer paths.
- Added validated playback replacement when a new file is dropped onto an active playback window.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Manual GUI smoke test:
Not executed in automation during this turn.
Recommended checks:
1. Launch build\\Release\\modern-video-player.exe with no args.
2. Drag a local video file onto the idle window.
3. Drag another file during playback and confirm replacement.
```

### Analysis Notes
1. The safe place to react to a dropped file is the app loop, not the renderer or `PlayerCore`, because path validation must happen before playback is stopped.
2. OpenGL needed its file-drop handling in the internal render-thread event pump, not only in the external `handleEvents()` wrapper.
3. Idle-mode support is cheapest when implemented as a renderer-only window in `main.cpp`, instead of forcing a fake media-open path through `PlayerCore`.

### Result
- Empty-start sessions now open an idle window.
- File-drop requests now traverse renderer -> `PlayerCore` -> `VideoPlayer` -> `main.cpp`.
- Invalid drops are ignored without interrupting playback.
- Sessions started without CLI media return to the idle window after playback finishes.

### Files
- `docs/analysis/PLAYERCORE_DAY32_IDLE_WINDOW_AND_DRAG_DROP_PLAYBACK.md`
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

## Issue 100: OpenGL present pacing and AMD stutter
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Investigated user-reported OpenGL playback stutter under native D3D11 interop.
- The main structural issue was that OpenGL submit and actual display completion were decoupled, so scheduler pacing could run ahead of the real window presentation path.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Diagnostics:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics
Result: PASS

OpenGL native path:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500
Result: PASS

OpenGL copy-back path:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2500
Result: PASS
```

### Analysis Notes
1. The old OpenGL path treated queued frames as already presented, unlike SDL and D3D11.
2. That mismatch made visible stutter possible without clearly failing existing scheduler counters.
3. Forcing `swap interval=0` further widened the behavior gap from the D3D11 path.
4. Removing the per-frame native interop `Flush()` reduces an avoidable synchronization point.

### Result
- OpenGL now waits for real render-thread presentation before returning from `present()`.
- OpenGL now prefers synchronized swap pacing.
- Native and copy-back OpenGL paths both remained functional in local validation.
- No `OpenGL present wait timed out` warning was observed in local regression runs.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY33_OPENGL_PRESENT_PACING_AND_AMD_STUTTER.md`
- `docs/records/CHANGELOG.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`
## Issue 101: OpenGL runtime diagnostics export and P010/P016 copy-back upload
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed end-to-end validation for the new `renderer_opengl_*` counters in `--performance-log-check`.
- Added `p010le/p016le` direct software upload support to the OpenGL renderer.
- Removed the extra 10-bit copy-back downgrade from `p010le -> swscale -> yuv420p` on the OpenGL fallback path.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL native regression:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
Result: PASS

OpenGL copy-back regression:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1800
Result: PASS

OpenGL 10-bit copy-back regression:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_NATIVE_INTEROP=disable .\build\Release\modern-video-player.exe --performance-log-check .\build\tmp\opengl-p010-validation.mp4 2200
Key lines:
- decoder_backend=D3D11VA
- video_copy_back_frames=72
- video_swscale_frames=0
- renderer_opengl_native_interop_active=false
- result=PASS
```

### Analysis Notes
1. The renderer diagnostics export was already coded, but it still required final-surface verification through the actual CLI that field engineers use.
2. `PlayerCore` already avoids `swscale` when the renderer advertises direct-frame support, so the real missing piece was OpenGL support for 16-bit semi-planar software frames.
3. Using 16-bit normalized GL textures lets the copy-back path keep `p010le/p016le` precision without changing the higher-level scheduler or decoder state machine.

### Result
- `renderer_opengl_native_interop_*` and `renderer_opengl_present_wait_timeouts` now show up in `--performance-log-check`.
- OpenGL now accepts `p010le/p016le` direct upload frames.
- Forced-copyback 10-bit playback now remains `copyback + direct upload` instead of `copyback + swscale to 8-bit`.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY34_OPENGL_RUNTIME_DIAGNOSTICS_AND_P010_UPLOAD.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 102: OpenGL present-mode override and gate script
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added an operator-facing OpenGL present-mode override via `MVP_OPENGL_PRESENT_MODE`.
- Exported requested/active present mode in `--performance-log-check`.
- Added `tools/run_opengl_checks.ps1` to gate the main OpenGL paths in one command.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL auto present mode:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500
Key lines:
- renderer_opengl_present_mode_requested=auto
- renderer_opengl_present_mode_active=paced
- renderer_opengl_present_wait_timeouts=0
- result=PASS

OpenGL immediate present mode:
MVP_RENDERER_BACKEND=opengl MVP_OPENGL_PRESENT_MODE=immediate .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1500
Key lines:
- [diag:opengl-present] requested=immediate active=immediate
- renderer_opengl_present_mode_requested=immediate
- renderer_opengl_present_mode_active=immediate
- result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. Present pacing control is most useful when the final runtime snapshot reports both the request and the real active mode; otherwise field logs remain ambiguous.
2. The OpenGL gate script is deliberately narrow: it validates the OpenGL paths directly instead of hiding them behind the broader `run_all_checks.ps1` umbrella.
3. Auto-generating the temporary 10-bit sample keeps the gate reproducible on machines that do not already carry a packaged HDR/10-bit asset.

### Result
- OpenGL present mode can now be toggled via environment variable.
- `--performance-log-check` now exposes the actual present mode state.
- OpenGL now has a reusable one-shot validation script that covers diagnostics, native, copy-back, 10-bit, and subtitle regressions.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 103: OpenGL HDR probe, quirk-table expansion, subtitle gate completion, and final gap matrix
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added HDR output capability probe fields to `--opengl-diagnostics`.
- Expanded the OpenGL quirk rule table for software and virtual GPU contexts.
- Expanded the OpenGL gate script to cover the current ASS sample suite.
- Closed the current OpenGL task table with a final gap matrix against mature players.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL diagnostics:
MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe --opengl-diagnostics
Key lines:
- opengl-diagnostics.hdr_output.probe_succeeded=true
- opengl-diagnostics.hdr_output.adapter_matched=true
- opengl-diagnostics.hdr_output.output_found=false
- opengl-diagnostics.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. The HDR probe closes the capability-detection half of display-level HDR work without pretending that HDR presentation is already implemented.
2. The quirk table is now shaped for long-term accumulation instead of one-off conditions.
3. The OpenGL gate now validates the current ASS sample suite as part of one reusable command, which is enough to mark the present subtitle sample bolster task as closed.

### Result
- The remaining OpenGL task-table items are now completed.
- What remains in the backlog is no longer task-table ambiguity, but a smaller set of known long-tail gaps: display-level HDR output, ICC/3D LUT color management, fuller libass parity, and a larger quirk corpus.

### Files
- `include/render/opengl_video_renderer.h`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `tools/run_opengl_checks.ps1`
- `docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`
- `docs/analysis/PLAYERCORE_DAY36_OPENGL_HDR_PROBE_QUIRK_TABLE_AND_GAP_MATRIX.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

### 2026-03-25 Update: OpenGL hotkey repeat suppression and interactive OSD
- Root cause confirmed in `src/render/opengl_video_renderer.cpp`: OpenGL hotkeys still accepted `SDL_KEYDOWN repeat`, and OSD wakeup only forced redraw while paused.
- Added repeat suppression for OpenGL hotkeys so a single press no longer expands into repeated seek/volume/fullscreen requests.
- Changed hotkey OSD wakeup to `requestRedraw()` and added initial OSD visibility on renderer startup.
- Added OpenGL mouse interaction for progress/volume bars: move to wake OSD, click/drag progress for seek preview, click/drag volume for live volume change.
- Shared the control layout between OSD drawing and hit-testing so the bottom panel geometry stays consistent.
- Validation: `cmake --build build --config Release --target modern-video-player` PASS, `run_opengl_checks.ps1` PASS.
- Manual smoke focus: `MVP_RENDERER_BACKEND=opengl .\build\Release\modern-video-player.exe .\a.mp4`, then test `Space`, `Left/Right`, `Up/Down`, mouse move, progress drag, and volume drag.

## Issue 105: ASS transform transition parser/runtime support
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added parser/model support for ASS `\t(...)` transitions.
- Added transition diagnostics output to `--subtitle-style-check`.
- Applied transition-evaluated styles in both OpenGL and D3D11 subtitle renderers.
- Added a dedicated transition validation sample and folded it into the OpenGL gate.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

Subtitle parser/diagnostics:
.\build\Release\modern-video-player.exe --subtitle-style-check .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key lines:
- subtitle-style-check.item0.animation.transition_count=1
- subtitle-style-check.item0.animation.transition0.property_names="primary_color,outline_color,outline_x,outline_y,shadow_x,shadow_y,scale_x,scale_y,rotation_z"
- subtitle-style-check.item1.animation.transition0.accel=0.45
- subtitle-style-check.item2.animation.transition0.target.has_clip=true
- subtitle-style-check.result=PASS

OpenGL runtime regression:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key line:
- delay-adjust-check.result=PASS

D3D11 runtime regression:
$env:MVP_RENDERER_BACKEND='d3d11'
.\build\Release\modern-video-player.exe --delay-adjust-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 .\samples\subtitles\opengl_ass_transform_transition_validation.ass
Key line:
- delay-adjust-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- [7/12] OpenGL subtitle transform transition regression
- [12/12] OpenGL subtitle style regression: opengl_ass_transform_transition_validation.ass
- OpenGL gate result: PASS
```

### Analysis Notes
1. The real blocker was parser structure, not shader capability: `\t(...)` needs nested-parenthesis-safe extraction and top-level comma splitting before renderer work even matters.
2. Transition support had to be added to both OpenGL and D3D11, because subtitle semantics should not diverge by renderer backend.
3. The new sample intentionally mixes timed transform, acceleration-only transform, and nested `\clip(...)` inside `\t(...)` so the regression is not limited to the easiest case.

### Result
- The subtitle stack now covers a substantial additional ASS semantics slice beyond move/fade/karaoke/vector basics.
- `--subtitle-style-check` can now explain transition timing and target state in machine-readable form.
- OpenGL gate coverage now includes the transition sample, while D3D11 has a direct runtime regression for the same asset.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 106: OpenGL bottom-bar player chrome
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Upgraded the OpenGL path from a minimal OSD into a bottom-bar player UI.
- Added a clickable play/pause button, segmented current/total time text, and larger seek/volume interaction zones.
- Added hover-aware visibility so the bar stays up while hovered and auto-hides with fade-out during idle playback.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS
```

### Analysis Notes
1. The OpenGL UI gap was no longer a rendering-backend problem, but a control-chrome problem: the interaction model was still shaped like an OSD, not a player surface.
2. Time text was implemented with lightweight segmented glyphs instead of a new font dependency so the OpenGL path stays self-contained and stable in the current build.
3. Automated checks can prove the OpenGL main path still works after the UI expansion, but they cannot prove hover timing or click feel; that still requires manual GUI smoke.

### Result
- OpenGL now presents a fuller player interface with persistent bottom-bar structure, not only transient rails.
- Play/pause is directly clickable inside the OpenGL window.
- Current/total time is now visible in the bottom bar and seek preview updates that time readout while dragging.
- The bar remains visible during hover/drag and auto-hides during idle playback.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY38_OPENGL_BOTTOM_BAR_PLAYER_CHROME.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 107: Container attachment font pipeline
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added media-scoped extraction and private registration for font attachments exposed by FFmpeg container streams.
- Wired attachment font registration into `PlayerCore::open()` and cleanup into session release.
- Added `--attachment-font-check <media_file>` as a machine-readable regression command.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8
Result: PASS

Attachment sample generation:
ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -c copy -attach C:\Windows\Fonts\arial.ttf -metadata:s:t mimetype=application/x-truetype-font -metadata:s:t:0 filename=attachment-font-check.ttf .\build\tmp\attachment-font-check.mkv
Result: PASS

Attachment font check:
.\build\Release\modern-video-player.exe --attachment-font-check .\build\tmp\attachment-font-check.mkv
Key lines:
- attachment-font-check.open_ok=true
- attachment-font-check.attachment_streams=1
- attachment-font-check.extracted_file_count=1
- attachment-font-check.registered_file_count=1
- attachment-font-check.invalid_attachment_stream_count=0
- attachment-font-check.result=PASS

Cleanup:
- cache directory removed after releaseMediaAttachmentFonts(...)
```

### Analysis Notes
1. The missing capability was in the media-open stage, not the ASS parser: container fonts never entered the process before this change.
2. Session-scoped registration is the correct ownership model because attachment fonts belong to the opened media, not to a subtitle sidecar directory.
3. This closes the highest-value subtitle-font gap, but embedded subtitle-track playback remains separate work.

### Result
- Container attachment fonts now participate in the subtitle font path.
- External ASS loaded after media open can use fonts bundled inside the current media container.
- The project now has a direct regression command for attachment extraction, registration, and cleanup.

### Files
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY39_ATTACHMENT_FONT_PIPELINE.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## 2026-03-25 OpenGL CPU / GPU / driver optimization matrix doc update
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Consolidated the current OpenGL tuning strategy into one plan document.
- Split the strategy by CPU layer, GPU path selection, and driver/adapter rule handling.
- Added a release-oriented default strategy table for NVIDIA / AMD / Intel plus validation commands.

### Log
```text
Document added:
- docs/plans/OPENGL_CPU_GPU_DRIVER_OPTIMIZATION_MATRIX.md

Coverage:
- startup diagnostics signals
- runtime diagnostics signals
- CPU / GPU / driver layered matrix
- NVIDIA / AMD / Intel default strategy table
- vendor-specific validation commands
- generic OpenGL gate commands
```

### Analysis Notes
1. This document intentionally describes the current codebase behavior, not a hypothetical future policy.
2. Vendor differences are currently concentrated in native interop / driver rule handling, not in CPU-side thread-count hardcoding.
3. AMD remains conservative by design in the current rule table; Intel remains auto-probe first; NVIDIA remains native-first when diagnostics stay clean.

### Result
- The repository now has one stable document for OpenGL default-policy discussion and release review.
- Future quirk additions can extend this document instead of repeating the same strategy explanation in issue threads.

### Files
- `docs/plans/OPENGL_CPU_GPU_DRIVER_OPTIMIZATION_MATRIX.md`
- `docs/records/DEVELOP_LOG.md`
- `docs/records/VERSION.md`

## Issue 108: Embedded subtitle-track playback
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added automatic loading for supported embedded text subtitle streams during media open.
- Split subtitle ownership into external and embedded stores with `external > embedded` precedence.
- Added `--embedded-subtitle-check <media_file>` and folded embedded subtitle media into the OpenGL gate.

### Log
```text
Build:
C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe --build build --config Release --target modern-video-player -j 8
Result: PASS

Embedded ASS sample generation:
ffmpeg -y -i .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 -i .\samples\subtitles\opengl_ass_transform_transition_validation.ass -map 0:v -map 0:a? -map 1:0 -c:v copy -c:a copy -c:s ass .\build\tmp\embedded-ass-validation.mkv
Result: PASS

Embedded subtitle CLI:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
Key lines:
- embedded-subtitle-check.loaded=true
- embedded-subtitle-check.codec_name=ass
- embedded-subtitle-check.item_count=3
- embedded-subtitle-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key lines:
- Embedded ASS subtitle CLI regression: PASS
- OpenGL embedded ASS subtitle playback regression: PASS
- Embedded text subtitle CLI regression: PASS
- OpenGL embedded text subtitle playback regression: PASS
- OpenGL gate result: PASS
```

### Analysis Notes
1. The correct integration point was `PlayerCore::open()`, because embedded subtitle tracks belong to the media session itself, not to a later external-subtitle action.
2. Reusing `SubtitleItem` instead of introducing a second subtitle representation keeps OpenGL and D3D11 subtitle semantics aligned.
3. The important ownership rule is `external > embedded`; otherwise loading and clearing a sidecar subtitle would produce unstable fallback behavior.
4. Validating only ASS was not enough, so the OpenGL gate now also generates and checks an embedded `mov_text` sample to cover the plain-text decode path.

### Result
- Embedded text subtitle tracks now load automatically on normal media open.
- External subtitle loading still overrides the embedded track, and clearing the external subtitle restores the embedded one.
- The OpenGL gate now exercises both embedded ASS and embedded text subtitle media instead of only sidecar subtitle files.

### Files
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
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`
## Issue 109: Embedded subtitle multi-track selection UI + CLI
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Completed embedded subtitle multi-track control closure across core/UI/CLI:
  - OpenGL subtitle previous/next track controls + track state overlay
  - playback arg `--subtitle-track <stream_index>`
  - diagnostics commands `--embedded-subtitle-list` and `--embedded-subtitle-select-check`
- Kept `external > embedded` ownership policy and moved selection semantics to supported embedded subtitle codecs (`supported_codec`).

### Log
```text
Build:
cmd /c "call ""C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat"" -arch=x64 && msbuild build\modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64"
Result: PASS

Embedded subtitle list:
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-text-validation.mp4
Key lines:
- embedded-subtitle-list.best_stream_index=2
- embedded-subtitle-list.result=PASS

Embedded subtitle select:
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-text-validation.mp4 2
Key lines:
- embedded-subtitle-select-check.loaded=true
- embedded-subtitle-select-check.result=PASS

Embedded best-stream check:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-text-validation.mp4
Key lines:
- embedded-subtitle-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Key line:
- OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. The remaining gap after Day40 was no longer decoding but control surface completeness (multi-track selection path closure).
2. OpenGL track state now maps to supported subtitle tracks (`supported_codec`), avoiding confusing counts when unsupported subtitle codecs exist.
3. The new list/select CLI closes machine-readable observability for embedded multi-track behavior.

### Files
- `src/core/player_core.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 110: Embedded bitmap subtitle path + DirectWrite custom font collection
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Extended embedded subtitle capability from text-only closure to supported-codec closure (text + bitmap).
- Landed PGS/DVD bitmap subtitle decode/model/render consumption path.
- Landed DirectWrite custom subtitle font collection integration on top of registered private fonts.
- Synced CLI diagnostics fields for bitmap and font-collection visibility.

### Log
```text
Build:
MSBuild.exe build/modern-video-player.sln /m /p:Configuration=Release /p:Platform=x64
Result: PASS

CLI:
.\build\Release\modern-video-player.exe --embedded-subtitle-check .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-list .\build\tmp\embedded-ass-validation.mkv
.\build\Release\modern-video-player.exe --embedded-subtitle-select-check .\build\tmp\embedded-ass-validation.mkv 2
.\build\Release\modern-video-player.exe --directwrite-font-collection-check .\build\tmp\embedded-ass-validation.mkv
Result: PASS

Gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Notes
1. Embedded subtitle selection/overlay policy is now aligned with `supported_codec`, avoiding the previous text-only interpretation mismatch.
2. Bitmap subtitle path is functionally closed in loader + renderer, but a wider real-media PGS/DVD sample corpus remains future regression debt.
3. Display-level HDR output bridge and ICC/3D LUT output management remain separate backlog tracks.

### Files
- `include/subtitle/subtitle_parser.h`
- `src/subtitle/subtitle_parser.cpp`
- `include/subtitle/embedded_subtitle_loader.h`
- `src/subtitle/embedded_subtitle_loader.cpp`
- `include/subtitle/subtitle_font_registry.h`
- `src/subtitle/subtitle_font_registry.cpp`
- `src/render/opengl_video_renderer.cpp`
- `src/render/d3d11_video_renderer.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY43_BITMAP_SUBTITLE_AND_DWRITE_COLLECTION.md`
- `docs/analysis/PLAYERCORE_DAY42_EMBEDDED_SUBTITLE_TRACK_SELECTION_UI_AND_CLI.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 111: OpenGL HDR output policy + 3D LUT output baseline
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added OpenGL output-color policy controls for HDR output mode and 3D LUT loading.
- Wired output-stage 3D LUT parsing/upload/sampling into OpenGL final composition paths.
- Extended machine-readable diagnostics for OpenGL HDR bridge and LUT runtime state.
- Added dedicated regression command `--opengl-output-color-check`.

### Log
```text
Build:
cmake --build build --config Release --target modern-video-player
Result: PASS

OpenGL output color check:
.\build\Release\modern-video-player.exe --opengl-output-color-check .\juren-30s.mp4 .\samples\lut\identity_2.cube 1200
Key lines:
- opengl-output-color-check.hdr_bridge_mode=auto
- opengl-output-color-check.output_lut_configured=true
- opengl-output-color-check.output_lut_active=true
- opengl-output-color-check.result=PASS

OpenGL performance log:
$env:MVP_RENDERER_BACKEND='opengl'
$env:MVP_OPENGL_3DLUT_FILE='.\samples\lut\identity_2.cube'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Key lines:
- performance-log-check.renderer_opengl_hdr_bridge_mode=auto
- performance-log-check.renderer_opengl_output_lut_active=true
- performance-log-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. This round lands a practical output control plane (policy + LUT) but intentionally does not claim final display-HDR delivery closure.
2. The new diagnostics fields close observability for request/active state, which was the main blocker for stable HDR/LUT regression checks.
3. Full display-level HDR present still requires DXGI swapchain color-space/metadata output path, beyond this implementation batch.
4. ICC/profile-driven LUT generation and per-display dynamic binding remain future backlog.

### Files
- `src/render/opengl_video_renderer.cpp`
- `include/render/video_renderer.h`
- `include/core/player_core.h`
- `src/core/player_core.cpp`
- `src/main.cpp`
- `docs/analysis/PLAYERCORE_DAY44_OPENGL_HDR_OUTPUT_POLICY_AND_3DLUT.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/plans/OPENGL_NEXT_STAGE_TOP10.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 112: OpenGL interaction freeze on mouse/keyboard/window events
**Date**: 2026-03-25
**Status**: Resolved

### Description
- User reported OpenGL playback freezing after mouse move/click, with hotkeys and maximize/minimize/fullscreen becoming unresponsive.
- Root cause traced to SDL event pumping on the OpenGL render thread instead of main-thread handling.
- Fixed by moving event pumping/fullscreen window operation to the `handleEvents()` main-thread path.

### Log
```text
Build:
& 'C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' --build build --config Release --target modern-video-player
Result: PASS

OpenGL playback diagnostics:
$env:MVP_RENDERER_BACKEND='opengl'
.\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200
Result: performance-log-check.result=PASS

OpenGL gate:
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "juren-30s.mp4"
Result: OpenGL gate result: PASS (16/16)
```

### Analysis Notes
1. D3D11 path already pumps SDL events on `handleEvents()` main-thread path; OpenGL path drifted into render-thread pumping and diverged behavior.
2. Interaction-triggered freezes were consistent with thread-affinity/message-pump stall patterns rather than decoder/render throughput bottlenecks.
3. Render thread should remain frame/present focused; window/event transitions should stay in main-thread event path.
4. Automated gate cannot fully replace real desktop-interaction smoke; final closure still requires short manual GUI stress pass.

### Files
- `src/render/opengl_video_renderer.cpp`
- `docs/analysis/PLAYERCORE_DAY45_OPENGL_EVENT_THREAD_AFFINITY_FREEZE_FIX.md`
- `docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`

## Issue 113: Cross-platform master tasklist consolidation
**Date**: 2026-03-25
**Status**: Resolved

### Description
- Added one execution-ready master tasklist for all cross-platform work.
- Unified task IDs, phase boundaries, milestone gates, and acceptance criteria into a single plan entry.
- Synced matching analysis and records docs so future execution can be tracked from one baseline.

### Log
```text
Master plan added:
docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md

Analysis added:
docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md

Plans index updated:
docs/plans/README.md
```

### Analysis Notes
1. Existing plan docs had strong content but were fragmented by purpose; execution required repeated manual merging.
2. A master task board with stable IDs is needed for continuous tracking, review, and staged delivery.
3. Linux-first sequencing remains unchanged: strategy extraction -> Linux MVP -> subtitle/font/bitmap maturity -> HDR/ICC/LUT -> CI/package convergence.

### Validation
```text
rg -n "CROSS_PLATFORM_MASTER_TASKLIST" docs/plans docs/analysis docs/records
PASS

cmake --build build --config Release --target modern-video-player
PASS
```

### Files
- `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`
- `docs/plans/README.md`
- `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`
- `docs/records/CHANGELOG.md`
- `docs/records/VERSION.md`
- `docs/records/DEVELOP_LOG.md`








