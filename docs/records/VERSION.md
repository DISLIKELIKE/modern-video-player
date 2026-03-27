# VERSION

## 索引说明（2026-03-26 编码清理批次）

- 本轮仅清理 `records/readme` 索引范围，不批量改写历史版本正文。
- 最新版本更新记录位于文件顶部，按时间倒序排列。
- 历史段落若出现旧编码乱码，将在后续专题批次逐步处理。

### 2026-03-27 Update: Vulkan chain VK-043 Windows strict-diag-exit-nonzero expected-fail canary
- Landed deterministic strict-diag-exit-nonzero expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid and reports:
      - `compiled_in=true`
      - `runtime_available=true`
      - `result=PASS`
    - diagnostics command exits non-zero intentionally (`diag_exit_code=9`)
    - strict mode executes expected unavailable failure branch without playback check
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.vulkan_availability_failure_detail=diag-exit-nonzero`
    - `windows-vulkan-check.playback_check_executed=false`
    - `windows-vulkan-check.diag_exit_code=9`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes strict-diag-exit-nonzero canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary`
  - fail-fast enforcement:
    - checks `$vulkanStrictDiagExitNonzeroCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY105_VK043_WINDOWS_VULKAN_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_DIAG_EXIT_NONZERO_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary (`compiled-in-false`): PASS
  - strict-runtime-unavailable canary: PASS
  - strict-diag-exit-nonzero canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_mode=strict`
    - `gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `gate_vulkan_availability_failure_detail=diag-exit-nonzero`
    - `gate_playback_check_executed=false`
    - `gate_diag_exit_code=9`
    - `result=PASS`
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS (`result=PASS`)
  - playback-plan-reason semantic canary: PASS (`result=PASS`)
  - playback-result-not-pass canary: PASS (`result=PASS`)
  - playback-command-exit-nonzero canary: PASS (`result=PASS`)
  - workflow/static scan for strict-diag-exit-nonzero canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-042 Windows strict-runtime-unavailable expected-fail canary
- Landed deterministic strict-runtime-unavailable expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid and reports:
      - `compiled_in=true`
      - `runtime_available=false`
      - `result=FAIL`
    - strict mode executes expected unavailable failure branch without playback check
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.vulkan_availability_failure_detail=runtime-unavailable`
    - `windows-vulkan-check.playback_check_executed=false`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes strict-runtime-unavailable canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Strict Runtime-Unavailable Canary`
  - fail-fast enforcement:
    - checks `$vulkanStrictRuntimeUnavailableCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY104_VK042_WINDOWS_VULKAN_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_RUNTIME_UNAVAILABLE_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary (`compiled-in-false`): PASS
  - strict-runtime-unavailable canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_mode=strict`
    - `gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `gate_vulkan_availability_failure_detail=runtime-unavailable`
    - `gate_playback_check_executed=false`
    - `result=PASS`
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS (`result=PASS`)
  - playback-plan-reason semantic canary: PASS (`result=PASS`)
  - playback-result-not-pass canary: PASS (`result=PASS`)
  - playback-command-exit-nonzero canary: PASS (`result=PASS`)
  - workflow/static scan for strict-runtime-unavailable canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-041 Windows playback-command-exit-nonzero expected-fail canary
- Landed deterministic playback-command-exit-nonzero expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid and reports PASS fields, then exits non-zero intentionally
    - optional mode executes expected playback failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=command-exit-nonzero`
    - `windows-vulkan-check.playback_result=PASS`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-command-exit-nonzero canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Command-Exit-Nonzero Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackCommandExitNonzeroCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY103_VK041_WINDOWS_VULKAN_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_COMMAND_EXIT_NONZERO_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS (`result=PASS`)
  - playback-plan-reason semantic canary: PASS (`result=PASS`)
  - playback-result-not-pass canary: PASS (`result=PASS`)
  - playback-command-exit-nonzero canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=command-exit-nonzero`
    - `gate_playback_result=PASS`
    - `result=PASS`
  - workflow/static scan for playback-command-exit-nonzero canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-040 Windows playback-result-not-pass expected-fail canary
- Landed deterministic playback-result-not-pass expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_result_not_pass_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid but intentionally reports `performance-log-check.result=FAIL`
    - optional mode executes expected playback failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=result-not-pass`
    - `windows-vulkan-check.playback_result=FAIL`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-result-not-pass canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Result-Not-Pass Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackResultNotPassCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY102_VK040_WINDOWS_VULKAN_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_RESULT_NOT_PASS_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS (`result=PASS`)
  - playback-plan-reason semantic canary: PASS (`result=PASS`)
  - playback-result-not-pass canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=result-not-pass`
    - `gate_playback_result=FAIL`
    - `result=PASS`
  - workflow/static scan for playback-result-not-pass canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-039 Windows playback-plan-reason semantic expected-fail canary
- Landed deterministic playback-plan-reason semantic expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid but intentionally violates plan-reason semantic (`startup_renderer_plan_reason` is not `renderer-override-env`)
    - optional mode executes expected playback semantic failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=plan-reason-not-renderer-override-env`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-plan-reason semantic canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Plan Reason Semantic Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackPlanReasonSemanticCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY101_VK039_WINDOWS_VULKAN_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_PLAN_REASON_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS (`result=PASS`)
  - playback-plan-reason semantic canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=plan-reason-not-renderer-override-env`
    - `result=PASS`
  - workflow/static scan for playback-plan-reason semantic canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-038 Windows playback-candidates semantic expected-fail canary
- Landed deterministic playback-candidates semantic expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid but intentionally violates candidate-chain semantic (`startup_renderer_candidates` does not include Vulkan)
    - optional mode executes expected playback semantic failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=candidates-missing-vulkan`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-candidates semantic canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Candidates Semantic Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackCandidatesSemanticCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY100_VK038_WINDOWS_VULKAN_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CANDIDATES_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS (`result=PASS`)
  - playback-candidates semantic canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=candidates-missing-vulkan`
    - `result=PASS`
  - workflow/static scan for playback-candidates semantic canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-037 Windows playback-backend semantic expected-fail canary
- Landed deterministic playback-backend semantic expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_backend_semantic_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid but intentionally violates backend semantic (`renderer_backend=OpenGL`)
    - optional mode executes expected playback semantic failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=renderer-backend-not-vulkan`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-backend semantic canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Backend Semantic Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackBackendSemanticCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY99_VK037_WINDOWS_VULKAN_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_BACKEND_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS (`result=PASS`)
  - playback-backend semantic canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=renderer-backend-not-vulkan`
    - `result=PASS`
  - workflow/static scan for playback-backend semantic canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-036 Windows playback-semantic expected-fail canary
- Landed deterministic playback-semantic expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_semantic_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid (`compiled_in=true`, `runtime_available=true`)
    - playback contract remains valid but intentionally violates renderer semantic (`startup_selected_renderer=D3D11`)
    - optional mode executes expected playback semantic failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-check-failed`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=selected-renderer-not-vulkan`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-semantic canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Semantic Canary`
  - fail-fast enforcement:
    - checks `$vulkanPlaybackSemanticCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY98_VK036_WINDOWS_VULKAN_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_SEMANTIC_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS (`result=PASS`)
  - playback-semantic canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-check-failed`
    - `gate_playback_contract_valid=true`
    - `gate_playback_failure_detail=selected-renderer-not-vulkan`
    - `result=PASS`
  - workflow/static scan for playback-semantic canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-035 Windows unsupported-platform expected-fail canary
- Landed deterministic unsupported-platform expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_unsupported_platform_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid
    - platform support is explicitly disabled (`supported_platform=false`)
    - optional mode executes expected unsupported-platform failure branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=unsupported-platform`
    - `windows-vulkan-check.playback_check_executed=false`
    - `windows-vulkan-check.skip_reason` empty
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes unsupported-platform canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Unsupported Platform Canary`
  - fail-fast enforcement:
    - checks `$vulkanUnsupportedPlatformCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY97_VK035_WINDOWS_VULKAN_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_UNSUPPORTED_PLATFORM_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS (`result=PASS`)
  - unsupported-platform canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=unsupported-platform`
    - `gate_skip_reason` empty
    - `result=PASS`
  - workflow/static scan for unsupported-platform canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-034 Windows optional-unavailable skip canary
- Landed deterministic optional-unavailable skip canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_optional_skip_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid
    - availability probe is unavailable (`compiled_in=false`, `runtime_available=false`)
    - strict override is not enabled (optional mode expected)
  - canary validates:
    - gate exit code `0`
    - `windows-vulkan-check.result=SKIPPED`
    - `windows-vulkan-check.mode=optional`
    - `windows-vulkan-check.skip_reason=vulkan-not-available`
    - `windows-vulkan-check.failure_reason` empty
    - `windows-vulkan-check.playback_check_executed=false`
    - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-false`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes optional-skip canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Optional Skip Canary`
  - fail-fast enforcement:
    - checks `$vulkanOptionalSkipCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY96_VK034_WINDOWS_VULKAN_OPTIONAL_UNAVAILABLE_SKIP_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_OPTIONAL_UNAVAILABLE_SKIP_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS (`result=PASS`)
  - optional-skip canary: PASS
    - `actual_gate_exit_code=0`
    - `gate_result=SKIPPED`
    - `gate_mode=optional`
    - `gate_skip_reason=vulkan-not-available`
    - `gate_vulkan_availability_failure_detail=compiled-in-false`
    - `result=PASS`
  - workflow/static scan for optional-skip canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-033 Windows strict-unavailable expected-fail canary
- Landed deterministic strict-unavailable expected-fail canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_strict_unavailable_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics contract remains valid
    - availability probe is unavailable (`compiled_in=false`, `runtime_available=false`)
    - strict policy is enforced via CLI (`-RequireVulkanAvailable`)
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode`
    - `windows-vulkan-check.playback_check_executed=false`
    - `windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-false`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes strict-unavailable canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Strict Unavailable Canary`
  - fail-fast enforcement:
    - checks `$vulkanStrictUnavailableCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY95_VK033_WINDOWS_VULKAN_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_STRICT_UNAVAILABLE_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS (`result=PASS`)
  - strict-unavailable canary: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_mode=strict`
    - `gate_failure_reason=vulkan-not-available-in-strict-mode`
    - `gate_vulkan_availability_failure_detail=compiled-in-false`
    - `result=PASS`
  - workflow/static scan for strict-unavailable canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-032 Windows PASS-contract canary
- Landed deterministic PASS-contract canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_pass_contract_canary.ps1`
  - script builds deterministic mock executable scenario:
    - diagnostics path emits valid Vulkan availability contract (`compiled_in/runtime_available=true`)
    - playback path emits complete Vulkan PASS contract fields
  - canary validates:
    - gate exit code `0`
    - `windows-vulkan-check.result=PASS`
    - `windows-vulkan-check.mode=strict`
    - `windows-vulkan-check.playback_contract_valid=true`
    - `windows-vulkan-check.playback_failure_detail=none`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes PASS-contract canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate PASS Contract Canary`
  - fail-fast enforcement:
    - checks `$vulkanPassCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY94_VK032_WINDOWS_VULKAN_PASS_CONTRACT_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PASS_CONTRACT_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - diagnostics expected-fail canary: PASS (`result=PASS`)
  - playback expected-fail canary: PASS (`result=PASS`)
  - PASS-contract canary: PASS
    - `actual_gate_exit_code=0`
    - `gate_result=PASS`
    - `gate_mode=strict`
    - `gate_playback_contract_valid=true`
    - `result=PASS`
  - workflow/static scan for PASS canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-031 Windows playback-contract expected-fail canary
- Landed deterministic playback-contract canary coverage for Windows Vulkan gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_playback_contract_canary.ps1`
  - script builds a deterministic mock executable scenario:
    - diagnostics path emits valid Vulkan availability contract (`compiled_in/runtime_available=true`)
    - playback path intentionally omits required keys to trigger playback-contract-broken branch
  - canary validates:
    - gate exit code `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-playback-contract-broken`
    - `windows-vulkan-check.playback_contract_valid=false`
    - required missing keys are reported
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now executes playback-contract canary.
  - publishes Step Summary section:
    - `Windows Vulkan Gate Playback Contract Canary`
  - fail-fast enforcement retained:
    - checks `$vulkanPlaybackCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY93_VK031_WINDOWS_VULKAN_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_EXPECTED_FAIL_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate command: PASS (`result=SKIPPED` on current host)
  - playback-contract canary command: PASS
    - `actual_gate_exit_code=2`
    - `gate_summary_file_present=true`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-playback-contract-broken`
    - `gate_playback_contract_valid=false`
    - `gate_playback_failure_detail=contract-missing-required-fields`
    - `result=PASS`
  - workflow/static scan for playback canary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-030 Windows gate contract-canary Step Summary observability
- Landed CI summary observability for Windows Vulkan contract canary:
  - `.github/workflows/cross-platform-gate.yml` now parses:
    - `logs/windows-vulkan-gate-contract-canary-summary.env`
  - appends dedicated section to `GITHUB_STEP_SUMMARY`:
    - `Windows Vulkan Gate Contract Canary`
  - includes key rows:
    - `result`
    - `expected_gate_exit_code`
    - `actual_gate_exit_code`
    - `gate_summary_file_present`
    - `gate_result`
    - `gate_failure_reason`
    - `gate_diag_contract_valid`
    - `validation_failure_reason`
  - fallback message added when canary summary env is missing.
- Extended canary script summary payload:
  - `tools/run_windows_vulkan_gate_contract_canary.ps1` now emits:
    - `windows-vulkan-contract-canary.gate_summary_file_present=true|false`
- Compatibility:
  - canary fail-fast semantics remain unchanged:
    - workflow still checks `$vulkanCanaryExitCode` and throws on non-zero.
- Round docs:
  - analysis: `PLAYERCORE_DAY92_VK030_WINDOWS_VULKAN_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_CONTRACT_CANARY_STEP_SUMMARY_OBSERVABILITY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate summary generation: PASS (`result=SKIPPED` on current host)
  - canary command: PASS (`result=PASS`, `actual_gate_exit_code=2`, `gate_summary_file_present=true`)
  - local Step Summary preview:
    - `logs/windows-vulkan-canary-step-summary-preview-vk030.md`
    - key rows rendered as expected
  - static scan for canary summary wiring: PASS
- Remaining:
  - strict PASS runtime proof for real Vulkan playback still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-029 Windows gate expected-fail contract canary
- Landed deterministic contract-canary check for Windows Vulkan CI gate:
  - added new script:
    - `tools/run_windows_vulkan_gate_contract_canary.ps1`
  - canary intentionally runs gate against `cmd.exe` diagnostics path and validates:
    - child gate exit code is `2`
    - `windows-vulkan-check.result=FAIL`
    - `windows-vulkan-check.failure_reason=vulkan-diagnostics-contract-broken`
    - `windows-vulkan-check.diag_contract_valid=false`
  - canary publishes machine-readable fields:
    - `windows-vulkan-contract-canary.*`
- Workflow integration:
  - `.github/workflows/cross-platform-gate.yml` Windows gate now runs canary command:
    - writes logs and canary summary env under `logs/`
    - captures `$vulkanCanaryExitCode = $LASTEXITCODE`
    - fail-fast throws on non-zero canary exit
- Effect:
  - CI continuously guards Vulkan gate failure-contract semantics independent of real Vulkan hardware/runtime availability.
- Round docs:
  - analysis: `PLAYERCORE_DAY91_VK029_WINDOWS_VULKAN_GATE_EXPECTED_FAIL_CONTRACT_CANARY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXPECTED_FAIL_CONTRACT_CANARY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline Windows Vulkan gate: PASS (`result=SKIPPED` on current host)
  - canary run: PASS
    - `actual_gate_exit_code=2`
    - `gate_result=FAIL`
    - `gate_failure_reason=vulkan-diagnostics-contract-broken`
    - `result=PASS`
  - static scan for workflow/script canary wiring: PASS
- Remaining:
  - strict PASS runtime proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-028 Windows gate exit-code propagation hardening
- Landed CI gate-semantics hardening for Windows Vulkan lane:
  - `.github/workflows/cross-platform-gate.yml` now captures:
    - `$vulkanGateExitCode = $LASTEXITCODE`
    - immediately after `run_windows_vulkan_checks.ps1` pipeline (`... | Tee-Object`).
  - keeps `VK-027` Step Summary rendering behavior unchanged.
  - adds explicit fail-fast guard before downstream checks:
    - `if ($vulkanGateExitCode -ne 0) { throw ... }`
- Effect:
  - non-zero Windows Vulkan gate result can no longer be swallowed by PowerShell pipeline behavior.
  - CI step now blocks deterministically on Vulkan gate `FAIL`.
- Round docs:
  - analysis: `PLAYERCORE_DAY90_VK028_WINDOWS_VULKAN_GATE_EXIT_CODE_PROPAGATION_HARDENING.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_EXIT_CODE_PROPAGATION_HARDENING_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate summary generation: PASS (`result=SKIPPED` on current host)
  - legacy pipeline reproduction:
    - gate `result=FAIL`, but outer process exit remained `0` (failure swallowed)
  - guarded pipeline reproduction:
    - gate `result=FAIL` + explicit throw, outer process exit becomes non-zero (failure propagated)
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-027 Windows CI step summary observability
- Landed CI observability enhancement for Windows Vulkan gate:
  - `.github/workflows/cross-platform-gate.yml` Windows gate step now parses:
    - `logs/windows-vulkan-gate-summary.env`
  - appends compact table to `GITHUB_STEP_SUMMARY` with key Vulkan gate signals.
- Step Summary key fields include:
  - `result`, `mode`, `strict_mode_effective`, `strict_mode_policy`
  - `runner_vulkan_sdk_available`, `runner_vulkan_runtime_probe_available`, `runner_vulkan_runtime_probe_detail`
  - `diag_contract_valid`, `playback_contract_valid`
  - `failure_reason`, `vulkan_availability_failure_detail`, `playback_failure_detail`
- Missing summary fallback:
  - workflow writes explicit message when env summary file is absent.
- Round docs:
  - analysis: `PLAYERCORE_DAY89_VK027_WINDOWS_VULKAN_CI_STEP_SUMMARY_OBSERVABILITY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_CI_STEP_SUMMARY_OBSERVABILITY_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline summary env generation: PASS
  - local parser preview output:
    - `logs/windows-vulkan-step-summary-preview-vk027.md`
    - key fields rendered as expected (`result=SKIPPED`, `mode=optional`, etc.)
  - workflow static scan for Step Summary logic: PASS
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-026 Windows playback contract validation
- Landed Windows Vulkan playback-stage contract hardening:
  - `tools/run_windows_vulkan_checks.ps1` now validates required playback keys when playback check executes.
  - added summary fields:
    - `windows-vulkan-check.playback_contract_valid`
    - `windows-vulkan-check.playback_missing_required_fields`
    - `windows-vulkan-check.playback_failure_detail`
  - playback contract-broken path now fails with:
    - `failure_reason=vulkan-playback-contract-broken`
    - `playback_failure_detail=contract-missing-required-fields`
- Compatibility:
  - non-playback paths now explicitly emit:
    - `playback_contract_valid=n/a`
    - `playback_missing_required_fields=n/a`
    - `playback_failure_detail=not-executed`
  - strict/optional availability behavior remains unchanged.
- Round docs:
  - analysis: `PLAYERCORE_DAY88_VK026_WINDOWS_VULKAN_PLAYBACK_CONTRACT_VALIDATION.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_PLAYBACK_CONTRACT_VALIDATION_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate output includes playback contract fields as `n/a`/`not-executed`
  - playback contract-broken simulation via mock executable expected FAIL:
    - `playback_contract_valid=false`
    - `failure_reason=vulkan-playback-contract-broken`
    - `playback_failure_detail=contract-missing-required-fields`
  - auto policy compatibility unchanged:
    - `auto + sdk=1 + runtime_probe=0` -> `SKIPPED`
    - `auto + sdk=1 + runtime_probe=1` -> strict expected `FAIL` on current host
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-025 Windows diagnostics contract validation
- Landed Windows Vulkan gate diagnostics-contract hardening:
  - `tools/run_windows_vulkan_checks.ps1` now validates required diagnostics keys:
    - `vulkan-diagnostics.platform`
    - `vulkan-diagnostics.supported_platform`
    - `vulkan-diagnostics.compiled_in`
    - `vulkan-diagnostics.runtime_available`
    - `vulkan-diagnostics.result`
  - added summary fields:
    - `windows-vulkan-check.diag_contract_valid`
    - `windows-vulkan-check.diag_missing_required_fields`
  - contract-broken path now fails with:
    - `failure_reason=vulkan-diagnostics-contract-broken`
    - `vulkan_availability_failure_detail=diag-contract-missing-required-fields`
- Compatibility:
  - valid-diagnostics strict/optional behavior remains unchanged.
  - existing result/failure/skip fields remain backward compatible.
- Round docs:
  - analysis: `PLAYERCORE_DAY87_VK025_WINDOWS_VULKAN_DIAGNOSTICS_CONTRACT_VALIDATION.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_DIAGNOSTICS_CONTRACT_VALIDATION_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate:
    - `diag_contract_valid=true`
    - `diag_missing_required_fields=none`
    - `result=SKIPPED` (current host expected)
  - contract-broken simulation (`ExecutablePath=cmd.exe`) expected FAIL:
    - `diag_contract_valid=false`
    - `failure_reason=vulkan-diagnostics-contract-broken`
    - `vulkan_availability_failure_detail=diag-contract-missing-required-fields`
  - auto policy compatibility remains unchanged:
    - `auto + sdk=1 + runtime_probe=0` -> `SKIPPED`
    - `auto + sdk=1 + runtime_probe=1` -> strict expected `FAIL` on current host
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-024 Windows availability failure detail classification
- Landed Windows Vulkan gate observability refinement:
  - `tools/run_windows_vulkan_checks.ps1` adds:
    - `windows-vulkan-check.vulkan_availability_probe_passed`
    - `windows-vulkan-check.vulkan_availability_failure_detail`
  - availability failure detail is now classified (e.g. `compiled-in-disabled`, `runtime-unavailable`, `diag-exit-nonzero`).
- Compatibility:
  - existing `result/failure_reason/skip_reason` semantics remain unchanged.
  - strict policy behavior from `VK-022` remains unchanged.
- Round docs:
  - analysis: `PLAYERCORE_DAY86_VK024_WINDOWS_VULKAN_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_AVAILABILITY_FAILURE_DETAIL_CLASSIFICATION_LOCAL_CHECK.md`
- Local validation:
  - configure with Vulkan ON: PASS (expected host warning: Vulkan missing -> forcing OFF)
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - baseline gate output includes:
    - `vulkan_availability_probe_passed=false`
    - `vulkan_availability_failure_detail=compiled-in-disabled`
  - policy matrix behavior unchanged:
    - `auto + sdk=1 + runtime_probe=0` -> optional / `SKIPPED` (exit 0)
    - `auto + sdk=1 + runtime_probe=1` -> strict / expected `FAIL` (exit 2 on current host)
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-023 Windows runtime probe detail observability
- Landed Windows Vulkan runtime-probe observability follow-up:
  - `.github/workflows/cross-platform-gate.yml` now exports:
    - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL=<detail>`
  - `tools/run_windows_vulkan_checks.ps1` now publishes:
    - `windows-vulkan-check.runner_vulkan_runtime_probe_detail`
  - when detail env is missing, script normalizes to `unknown`.
- Policy behavior compatibility:
  - `VK-022` auto strict guard (`sdk + runtime_probe`) remains unchanged.
- Round docs:
  - analysis: `PLAYERCORE_DAY85_VK023_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL_OBSERVABILITY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_RUNTIME_PROBE_DETAIL_OBSERVABILITY_LOCAL_CHECK.md`
- Local validation:
  - configure with Vulkan ON: PASS (expected host warning: Vulkan missing -> forcing OFF)
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - policy matrix remains unchanged:
    - `auto + sdk=1 + runtime_probe=0` -> optional / `SKIPPED` (exit 0)
    - `auto + sdk=1 + runtime_probe=1` -> strict / expected `FAIL` (exit 2 on current host)
  - new detail field verified:
    - baseline (no env): `runner_vulkan_runtime_probe_detail=unknown`
    - simulated cases: `vulkaninfo-missing`, `vulkaninfo-path`
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-022 Windows auto strict runtime probe guard
- Landed Windows Vulkan strict auto policy hardening follow-up:
  - `run_windows_vulkan_checks.ps1` now promotes `auto` to strict only when both are true:
    - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
    - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
  - keeps CLI/env explicit strict override behavior unchanged.
- Added machine-readable policy observability fields:
  - `windows-vulkan-check.runner_vulkan_runtime_probe_available`
  - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe`
  - `windows-vulkan-check.strict_mode_auto_prerequisites_met`
- Workflow runtime probe publication extension:
  - `.github/workflows/cross-platform-gate.yml` Windows Vulkan SDK step now probes `vulkaninfo --summary`.
  - exports `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1|0`.
- Round docs:
  - analysis: `PLAYERCORE_DAY84_VK022_WINDOWS_VULKAN_AUTO_STRICT_RUNTIME_PROBE_GUARD.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_RUNTIME_PROBE_GUARD_LOCAL_CHECK.md`
- Local validation:
  - configure with Vulkan ON: PASS (expected host warning: Vulkan missing -> forcing OFF)
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - `--vulkan-diagnostics`: expected FAIL on current host (`compiled_in=false`, `runtime_available=false`)
  - policy matrix:
    - `auto + sdk=1 + runtime_probe=0` -> optional / `SKIPPED` (exit 0)
    - `auto + sdk=1 + runtime_probe=1` -> strict / expected `FAIL` (exit 2 on current host)
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-021 Windows dependency-source observability and find_package hardening
- Landed Windows Vulkan dependency-resolution hardening follow-up:
  - `CMakeLists.txt` now validates `find_package(Vulkan)` completeness before acceptance.
  - if package metadata is incomplete, fallback to `VULKAN_SDK` probe.
  - publish build-time source marker:
    - `MVP_VULKAN_DEPENDENCY_SOURCE=find_package|vulkan_sdk_fallback|disabled`
- Diagnostics and gate observability extension:
  - `--vulkan-diagnostics` adds:
    - `vulkan-diagnostics.dependency_source`
  - `run_windows_vulkan_checks.ps1` adds:
    - `windows-vulkan-check.diag_dependency_source`
- Round docs:
  - analysis: `PLAYERCORE_DAY83_VK021_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_VULKAN_DEPENDENCY_SOURCE_OBSERVABILITY_AND_FIND_PACKAGE_HARDENING_LOCAL_CHECK.md`
- Local validation:
  - configure with Vulkan ON: PASS (expected host warning: Vulkan missing -> forcing OFF)
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - `--vulkan-diagnostics`: expected FAIL on current host, with `dependency_source=disabled`
  - `run_windows_vulkan_checks.ps1`: PASS with `diag_dependency_source=disabled` and `result=SKIPPED`
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-020 Windows CMake SDK fallback and CMake prefix-path closure
- Landed Windows Vulkan dependency-resolution closure follow-up:
  - `CMakeLists.txt` adds Windows `VULKAN_SDK` explicit fallback when `find_package(Vulkan)` fails:
    - requires `${VULKAN_SDK}/Include/vulkan/vulkan.h`
    - requires `${VULKAN_SDK}/Lib/vulkan-1.lib`
    - complete fallback keeps Vulkan enabled; incomplete fallback warns and forces OFF.
  - `.github/workflows/cross-platform-gate.yml` now prepends `CMAKE_PREFIX_PATH` with detected Vulkan SDK root.
- Round docs:
  - analysis: `PLAYERCORE_DAY82_VK020_WINDOWS_VULKAN_CMAKE_SDK_FALLBACK_AND_PREFIX_PATH.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_CMAKE_VULKAN_SDK_FALLBACK_AND_PREFIX_PATH_LOCAL_CHECK.md`
- Local validation:
  - configure with Vulkan ON: PASS (current host expected warning: Vulkan package missing -> forcing OFF)
  - Release build (`modern-video-player` + `sample_logger_plugin`): PASS
  - `--vulkan-diagnostics`: expected FAIL on current host (`supported_platform=true`, `compiled_in=false`)
  - `run_windows_vulkan_checks.ps1`: PASS (`result=SKIPPED`)
- Remaining:
  - strict PASS proof still depends on Vulkan-ready Windows runner/runtime.

### 2026-03-27 Update: Vulkan chain VK-019 Windows Vulkan auto strict policy promotion
- Landed Windows Vulkan policy promotion follow-up:
  - `run_windows_vulkan_checks.ps1` now supports policy env with `auto` semantics:
    - `auto`: strict when `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
    - `on`: strict
    - `off`: optional
  - CLI switch `-RequireVulkanAvailable` remains strict override.
- Added machine-readable strict-state fields:
  - `windows-vulkan-check.strict_mode_policy`
  - `windows-vulkan-check.strict_mode_cli_requested`
  - `windows-vulkan-check.strict_mode_effective`
- CI workflow default promoted:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto`
- Round docs:
  - analysis: `PLAYERCORE_DAY81_VK019_WINDOWS_VULKAN_AUTO_STRICT_POLICY_PROMOTION.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_AUTO_STRICT_POLICY_PROMOTION_LOCAL_CHECK.md`
- Local validation (policy matrix):
  - auto + sdk=0 => optional/`SKIPPED` (exit 0)
  - auto + sdk=1 => strict/expected `FAIL` (exit 2 on current host)
  - off + sdk=1 => optional/`SKIPPED` (exit 0)
- Remaining:
  - strict PASS-path still requires Vulkan-ready Windows runner with runtime availability.

### 2026-03-27 Update: Vulkan chain VK-018 Windows Vulkan SDK provisioning and CI observability
- Landed Windows Vulkan CI provisioning/observability follow-up:
  - workflow adds optional Windows Vulkan SDK provisioning stage:
    - tries `choco install vulkan-sdk`
    - probes `C:\\VulkanSDK` and exports `VULKAN_SDK` when found
    - exports `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1|0`
  - Vulkan gate summary extended with SDK context fields:
    - `windows-vulkan-check.vulkan_sdk_present`
    - `windows-vulkan-check.vulkan_sdk_path`
    - `windows-vulkan-check.runner_vulkan_sdk_available`
- Round docs:
  - analysis: `PLAYERCORE_DAY80_VK018_WINDOWS_VULKAN_SDK_PROVISIONING_AND_CI_OBSERVABILITY.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_SDK_PROVISIONING_AND_CI_OBSERVABILITY_LOCAL_CHECK.md`
- Local validation:
  - `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` PASS (expected warning on current host)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `run_windows_vulkan_checks.ps1` summary output includes new SDK fields and remains deterministic (`result=SKIPPED` on current host)
- Remaining:
  - Runtime PASS proof still depends on Vulkan-ready Windows runner (SDK + runtime/device).

### 2026-03-27 Update: Vulkan chain VK-017 Windows Vulkan gate strict policy and summary artifact
- Landed Windows Vulkan gate operational follow-up:
  - added summary output parameter in script:
    - `run_windows_vulkan_checks.ps1 -SummaryOutputPath <path>`
  - added env-driven strict policy hook:
    - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=1`
  - added output field:
    - `windows-vulkan-check.strict_mode_env_requested`
- CI integration refinements:
  - workflow job env now includes default:
    - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=0`
  - Windows gate writes:
    - `logs/windows-vulkan-gate-summary.env`
    - `logs/windows-vulkan-gate.log`
  - both are covered by existing artifact upload patterns.
- Round docs:
  - analysis: `PLAYERCORE_DAY79_VK017_WINDOWS_VULKAN_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_STRICT_POLICY_AND_SUMMARY_ARTIFACT_LOCAL_CHECK.md`
- Local validation:
  - `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` PASS (expected warning on host without Vulkan package)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - optional mode + summary file output PASS (`result=SKIPPED`)
  - env strict mode + summary file output expected FAIL (`result=FAIL`, exit code `2`)
- Remaining:
  - strict PASS-path still requires Vulkan-ready Windows host/runner.

### 2026-03-27 Update: Vulkan chain VK-016 Windows Vulkan gate and CI integration
- Landed Windows Vulkan gate/CI integration follow-up:
  - added new command wrapper script: `tools/run_windows_vulkan_checks.ps1`
  - added machine-readable output contract: `windows-vulkan-check.*`
  - added optional/strict mode policy (`-RequireVulkanAvailable`)
- CI integration updates:
  - Windows configure is now explicit with `-DENABLE_VULKAN_RENDERER=ON`
  - Windows gate now executes Vulkan check script before OpenGL checks
- Round docs:
  - analysis: `PLAYERCORE_DAY78_VK016_WINDOWS_VULKAN_GATE_AND_CI_INTEGRATION.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_GATE_AND_CI_INTEGRATION_LOCAL_CHECK.md`
- Local validation:
  - `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` PASS (current host shows expected Vulkan missing warning)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `run_windows_vulkan_checks.ps1` optional mode => `windows-vulkan-check.result=SKIPPED`
  - `run_windows_vulkan_checks.ps1 -RequireVulkanAvailable` strict mode => expected FAIL with exit code `2`
- Remaining:
  - CI/runtime proof of `windows-vulkan-check.result=PASS` requires a Windows host/runner with Vulkan dependency/runtime available.

### 2026-03-27 Update: Vulkan chain VK-015 Windows link/runtime probe closure
- Landed follow-up closure for Windows Vulkan enablement:
  - fixed Windows link closure by wiring `${PLATFORM_EXTRA_LIBRARIES}` into final target link stage.
  - added probe-driven Vulkan runtime availability publication in `PlatformCapabilitiesProbe`.
- Runtime truth contract update:
  - Vulkan capability no longer assumes runtime availability when compiled.
  - probe path checks loader/instance/device availability before publishing `runtime_available=true`.
- Round docs:
  - analysis: `PLAYERCORE_DAY77_VK015_WINDOWS_VULKAN_LINK_AND_RUNTIME_PROBE_CLOSURE.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_LINK_AND_RUNTIME_PROBE_LOCAL_CHECK.md`
- Local validation:
  - `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` PASS (expected downgrade warning on host without Vulkan package)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--vulkan-diagnostics` keeps truthful Windows output (`supported_platform=true`, `compiled_in=false`, `runtime_available=false`)
  - `MVP_RENDERER_BACKEND=vulkan --performance-log-check` fallback observability PASS
  - `--d3d11-diagnostics` PASS
- Remaining:
  - validate `compiled_in=true` + runtime-available Vulkan path on a Windows machine with Vulkan SDK/runtime package installed.

### 2026-03-27 Update: Vulkan chain VK-015 Windows enablement implementation
- Landed Windows Vulkan enablement baseline:
  - CMake now allows `ENABLE_VULKAN_RENDERER` on Windows.
  - Windows dependency detection added via `find_package(Vulkan)`.
  - missing Vulkan package now degrades safely with warning and forced OFF.
- Vulkan diagnostics platform contract updated:
  - `--vulkan-diagnostics` now treats Windows and Linux as supported platform set.
  - current host output: `supported_platform=true`, `compiled_in=false` (expected due missing Vulkan package).
- Startup override observability retained:
  - `MVP_RENDERER_BACKEND=vulkan` still shows deterministic fallback on Windows.
- Round docs:
  - analysis: `PLAYERCORE_DAY76_VK015_WINDOWS_VULKAN_ENABLEMENT_SCOPE_AND_IMPLEMENTATION_PLANNER.md`
  - design: `CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_WINDOWS_ENABLEMENT_LOCAL_CHECK.md`
- Local validation:
  - `cmake -S . -B build -DENABLE_VULKAN_RENDERER=ON` PASS (graceful downgrade warning observed)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--vulkan-diagnostics` reflects Windows platform support contract PASS
  - `MVP_RENDERER_BACKEND=vulkan --performance-log-check` fallback observability PASS
- Remaining:
  - validate on a Windows host with Vulkan SDK/runtime installed for `compiled_in=true` runtime path proof.

### 2026-03-27 Update: Vulkan chain VK-014 documentation and release closure
- Landed final closure sync for Vulkan chain (`VK-001` ~ `VK-014`):
  - completed `VK-014` analysis/design/plan/report documents
  - synced record chain:
    - `VERSION.md`
    - `CHANGELOG.md`
    - `DEVELOP_LOG.md`
  - synced index chain:
    - `analysis/README.md`
    - `design/README.md`
    - `plans/README.md`
    - `reports/README.md`
- Round docs:
  - analysis: `PLAYERCORE_DAY75_VK014_DOCUMENTATION_AND_RELEASE_CLOSURE.md`
  - design: `CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_DOCUMENTATION_AND_RELEASE_CLOSURE_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - index/record path scan PASS
- Remaining:
  - Linux Vulkan runtime PASS evidence still requires real Linux runner execution.

### 2026-03-27 Update: Vulkan chain VK-013 regression matrix execution
- Landed stage-level runtime regression closure report for Vulkan chain:
  - open/play: PASS
  - pause/seek: PASS (seek-burst observed first-run FAIL, rerun PASS)
  - subtitle style/sync: PASS
  - fallback and override observability: PASS
  - Vulkan diagnostics on Windows host: expected FAIL (`supported_platform=false`)
- Round docs:
  - analysis: `PLAYERCORE_DAY74_VK013_REGRESSION_MATRIX_EXECUTION.md`
  - design: `CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_REGRESSION_MATRIX_EXECUTION_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - regression matrix command set executed and archived
- Remaining:
  - Linux Vulkan runtime PASS proof still requires Linux host/runner.

### 2026-03-27 Update: Vulkan chain VK-012 GitHub Actions Linux Vulkan lane
- Landed CI Linux lane Vulkan wiring in `.github/workflows/cross-platform-gate.yml`:
  - added Linux Vulkan dependencies:
    - `libvulkan-dev`
    - `mesa-vulkan-drivers`
  - explicit Linux configure switch:
    - `-DENABLE_VULKAN_RENDERER=ON`
  - strict Vulkan gate requirement:
    - Linux gate arg `11=1` (`REQUIRE_VULKAN_CHECKS`)
- CI outcome expectation:
  - Linux lane now fails early if Vulkan check cannot run/pass.
  - gate summary artifact keeps machine-readable Vulkan check fields from `VK-011`.
- Round docs:
  - analysis: `PLAYERCORE_DAY73_VK012_GITHUB_ACTIONS_LINUX_VULKAN_LANE.md`
  - design: `CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_GITHUB_ACTIONS_LINUX_VULKAN_LANE_LOCAL_CHECK.md`
- Local validation:
  - workflow field scan PASS
  - `bash -n tools/run_linux_mvp_checks.sh` PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--vulkan-diagnostics` expected FAIL on Windows host (`supported_platform=false`)
- Remaining:
  - real Linux runner proof for strict Vulkan lane is required via GitHub Actions run.

### 2026-03-27 Update: Vulkan chain VK-011 Linux gate Vulkan checks
- Landed Linux gate Vulkan check integration in `tools/run_linux_mvp_checks.sh`:
  - added Vulkan availability probe (`--vulkan-diagnostics`)
  - added conditional gate stage `vk010_vulkan_diagnostics`
  - added machine-readable report fields for Vulkan probe/check metadata
- Added strict-mode hook for Vulkan gate requirement:
  - arg `11`: `REQUIRE_VULKAN_CHECKS`
  - env fallback: `MVP_REQUIRE_VULKAN_CHECKS`
- Gate behavior:
  - run Vulkan check when Vulkan is supported+compiled+runtime-available
  - otherwise emit `SKIPPED` with machine-readable skip reason
  - fail-fast when strict Vulkan mode is enabled and Vulkan probe is unavailable
- Round docs:
  - analysis: `PLAYERCORE_DAY72_VK011_LINUX_GATE_VULKAN_CHECKS.md`
  - design: `CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_LINUX_GATE_VULKAN_CHECKS_LOCAL_CHECK.md`
- Local validation:
  - script path scan PASS
  - `bash -n tools/run_linux_mvp_checks.sh` PASS
  - non-Linux dispatch expected FAIL (`This gate script only supports Linux.`)
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--vulkan-diagnostics` expected FAIL on Windows host (`supported_platform=false`)
- Remaining:
  - Linux host/runner proof for `vk010_vulkan_diagnostics` PASS is required.

### 2026-03-27 Update: Vulkan chain VK-010 Vulkan diagnostics CLI
- Landed dedicated Vulkan diagnostics CLI:
  - new command `--vulkan-diagnostics`
  - machine-readable output prefix: `vulkan-diagnostics.*`
- Reused existing startup strategy/capability abstractions:
  - `PlatformCapabilitiesProbe`
  - `PlaybackStrategy::buildOpenPlan`
  - `RendererFactory::isSupported`
- Exported key fields:
  - `supported_platform`
  - `compiled_in`
  - `runtime_available`
  - `requested_renderer_override`
  - `startup_renderer_candidates`
  - `startup_renderer_plan_reason`
  - `selected_renderer`
  - `fallback_target`
  - `result`
- Round docs:
  - analysis: `PLAYERCORE_DAY71_VK010_VULKAN_DIAGNOSTICS_CLI.md`
  - design: `CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_VULKAN_DIAGNOSTICS_CLI_LOCAL_CHECK.md`
- Local validation:
  - code-path scan PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS
  - `--vulkan-diagnostics` expected FAIL on Windows host (`supported_platform=false`)
  - `MVP_RENDERER_BACKEND=vulkan --vulkan-diagnostics` fallback observability PASS on Windows host
- Remaining:
  - Linux host/runner proof is required for `vulkan-diagnostics.result=PASS`.

### 2026-03-27 Update: Vulkan chain VK-009 fallback chain and startup policy
- Landed explicit Linux Vulkan fallback-chain policy in `PlaybackStrategy`:
  - when Linux startup candidate head is Vulkan, normalize prefix to:
    - `Vulkan -> OpenGL -> SoftwareSDL` (runtime-available subset)
  - strategy reason tag: `linux-vulkan-fallback-chain`
- Landed machine-readable startup policy observability extension:
  - `startup_renderer_plan_reason`
  - `startup_decoder_plan_reason`
  - both exported by `--performance-log-check`
- Round docs:
  - analysis: `PLAYERCORE_DAY70_VK009_FALLBACK_CHAIN_AND_STARTUP_POLICY.md`
  - design: `CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_FALLBACK_CHAIN_AND_STARTUP_POLICY_LOCAL_CHECK.md`
- Local validation:
  - policy/observability code-path scan PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS (new plan-reason fields present)
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS on Windows host
- Remaining:
  - Linux Vulkan runtime fallback chain proof still requires Linux host/runner.

### 2026-03-27 Update: Vulkan chain VK-008 sync and present pacing
- Landed Vulkan sync/pacing hardening baseline:
  - present-mode request contract via env `MVP_VULKAN_PRESENT_MODE`
  - selection/fallback policy (`auto|fifo|mailbox|immediate|fifo_relaxed`)
  - requested/active present-mode observability in startup/swapchain logs
- Landed frame-wait robustness:
  - in-flight fence wait timeout budget (`250ms`)
  - timeout recovery path (`vkDeviceWaitIdle` + swapchain recreate request)
- Landed pacing counters/logging:
  - submitted/presented frame counters
  - fence wait total/max/timeout counters
  - periodic pacing snapshot logs
- Round docs:
  - analysis: `PLAYERCORE_DAY69_VK008_SYNC_AND_PRESENT_PACING.md`
  - design: `CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_SYNC_AND_PRESENT_PACING_LOCAL_CHECK.md`
- Local validation:
  - sync/pacing code-path scan PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS
  - `MVP_RENDERER_BACKEND=vulkan` + `MVP_VULKAN_PRESENT_MODE=immediate` fallback observability PASS on Windows host
- Remaining:
  - Linux Vulkan runtime pacing verification still requires Linux host/runner.

### 2026-03-27 Update: Vulkan chain VK-007 frame upload YUV path
- Landed baseline Vulkan CPU frame upload path:
  - direct frame format contract for `YUV420P` / `NV12`
  - `swscale` conversion to RGBA in `renderFrame()`
  - present-stage staging upload (`map/copy/unmap`) and optional `vkCmdCopyBufferToImage`
- Unified Vulkan present command path:
  - `recordPresentCommandBuffer(...)` now serves both clear-only and frame-upload cases
  - stale `recordClearCommandBuffer` call path removed
- Round docs:
  - analysis: `PLAYERCORE_DAY68_VK007_FRAME_UPLOAD_YUV_PATH.md`
  - design: `CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_FRAME_UPLOAD_YUV_PATH_LOCAL_CHECK.md`
- Local validation:
  - code-path scan (`recordPresentCommandBuffer` + `supportsDirectFrameFormat`) PASS
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS on Windows host
- Remaining:
  - Linux Vulkan compile/runtime verification still requires Linux host/runner.

### 2026-03-27 Update: Vulkan chain VK-006 clear/present and swapchain recreate
- Landed minimal Vulkan visible render loop:
  - frame sync resources (`command pool/buffer`, acquire/present semaphores, in-flight fence)
  - `acquire -> clear -> submit -> present` frame path
  - `clear()` wired to present path for observable initial/idle clear
- Landed swapchain recreate behavior:
  - SDL resize/minimize/maximize/restore event triggers
  - `VK_ERROR_OUT_OF_DATE_KHR` / `VK_SUBOPTIMAL_KHR` handling
  - deferred recreate while drawable size is zero
- Round docs:
  - analysis: `PLAYERCORE_DAY67_VK006_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE.md`
  - design: `CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_CLEAR_PRESENT_AND_SWAPCHAIN_RECREATE_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS on Windows host
- Remaining:
  - Linux Vulkan runtime verification still requires Linux host/runner.

### 2026-03-27 Update: Vulkan chain VK-005 instance/surface/device/swapchain init
- Landed Vulkan initialization lifecycle baseline in renderer backend:
  - SDL subsystem bring-up (`VIDEO|AUDIO`) with owned-subsystem tracking
  - SDL Vulkan window creation
  - Vulkan instance creation with SDL-required instance extensions
  - surface creation (`SDL_Vulkan_CreateSurface`)
  - physical/logical device selection and queue binding
  - swapchain creation and image acquisition
- Landed deterministic teardown order:
  - `vkDeviceWaitIdle -> swapchain -> device -> surface -> instance -> window -> owned SDL subsystems`
- Added baseline event handling for quit and drag-drop open-file request.
- Round docs:
  - analysis: `PLAYERCORE_DAY66_VK005_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT.md`
  - design: `CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_DESIGN_2026-03-27.md`
  - plan: `CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_PLAN_2026-03-27.md`
  - report: `CROSS_PLATFORM_VULKAN_INSTANCE_SURFACE_DEVICE_SWAPCHAIN_INIT_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--opengl-diagnostics` PASS
  - `--performance-log-check` PASS
  - `MVP_RENDERER_BACKEND=vulkan` fallback observability PASS on Windows host
- Remaining:
  - Linux Vulkan compile/runtime evidence still requires Linux host/runner.

### 2026-03-26 Update: Vulkan chain VK-004 renderer skeleton and factory wiring
- Landed Vulkan backend skeleton and renderer-path wiring:
  - `VideoRendererType::Vulkan`
  - `RendererFactory` Vulkan name/create branch
  - `PlatformCapabilities` Vulkan support publication (Linux-first priority)
  - `PlaybackStrategy` env parser support (`MVP_RENDERER_BACKEND=vulkan|vk`)
- Added Vulkan skeleton backend files:
  - `include/render/vulkan_video_renderer.h`
  - `src/render/vulkan_video_renderer.cpp`
- Skeleton stage behavior:
  - `VulkanVideoRenderer::init()` intentionally returns `false` (real init starts at `VK-005`)
  - fallback observability validated via `--performance-log-check`
- Round docs:
  - analysis: `PLAYERCORE_DAY65_VK004_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING.md`
  - design: `CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_VULKAN_VULKAN_RENDERER_SKELETON_AND_FACTORY_WIRING_LOCAL_CHECK.md`

### 2026-03-26 Update: Vulkan chain VK-003 CMake switches and dependency detect
- Added `ENABLE_VULKAN_RENDERER` switch and compile macro `MVP_HAVE_VULKAN_RENDERER`.
- Linux-first dependency behavior:
  - Linux: Vulkan switch default `ON`
  - non-Linux: force `OFF` with warning
  - Linux missing `pkg-config vulkan`: force `OFF` with warning
- Conditional Vulkan source compilation wired into player target.
- Round docs:
  - analysis: `PLAYERCORE_DAY64_VK003_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT.md`
  - design: `CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_VULKAN_CMAKE_SWITCHES_AND_DEPENDENCY_DETECT_LOCAL_CHECK.md`

### 2026-03-26 Update: Vulkan chain VK-002 architecture and strategy integration
- Frozen and landed architecture integration contract:
  - Vulkan enters existing strategy/factory capability path
  - `PlayerCore` remains policy-neutral
  - fallback observability remains on startup diagnostics fields
- Round docs:
  - analysis: `PLAYERCORE_DAY63_VK002_ARCHITECTURE_AND_STRATEGY_INTEGRATION.md`
  - design: `CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_VULKAN_ARCHITECTURE_AND_STRATEGY_INTEGRATION_LOCAL_CHECK.md`

### 2026-03-26 Update: Vulkan chain VK-001 scope and acceptance freeze
- Started Vulkan execution track with Linux-first boundary and acceptance freeze (`VK-001`).
- Frozen goals:
  - Linux x64 first-class Vulkan renderer path
  - observable fallback target: `Vulkan -> OpenGL -> SoftwareSDL`
  - machine-readable diagnostics target: `--vulkan-diagnostics`
- Frozen non-goals:
  - macOS renderer work remains deferred
  - no decoder architecture rewrite in this wave
- Added round documents:
  - analysis: `PLAYERCORE_DAY62_VK001_SCOPE_AND_ACCEPTANCE_FREEZE.md`
  - design: `CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_VULKAN_SCOPE_AND_ACCEPTANCE_FREEZE_LOCAL_CHECK.md`
- Local validation:
  - Vulkan baseline probe (`rg`) confirms no existing Vulkan path before implementation.
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS.
  - `--d3d11-diagnostics` PASS.
  - `--opengl-diagnostics` PASS.
- Next:
  - move to `VK-002` architecture design after confirmation, then start code from `VK-003`.

### 2026-03-26 Update: Linux workflow Build Linux Release compile blocker closure
- Closed Linux compile failures observed in workflow runs `23601824744` and `23601841417`:
  - `libass_probe`: mixed `int`/`long long` in timestamp clamp path
  - `opengl_video_renderer`: non-Windows helper/type visibility gap
- Code updates:
  - `src/subtitle/libass_probe.cpp`
  - `src/render/opengl_video_renderer.cpp`
- Synced docs/records for this round:
  - analysis: `PLAYERCORE_DAY61_LINUX_WORKFLOW_BUILD_ERROR_CLOSURE.md`
  - design: `CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_LINUX_WORKFLOW_BUILD_ERROR_FIX_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--d3d11-diagnostics` PASS
  - `--performance-log-check` PASS
- Remaining:
  - Linux runner proof still requires push + rerun of `cross-platform-gate`.

### 2026-03-26 Update: Workflow-log FFmpeg duration compatibility closure
- Closed remaining workflow-log Linux compile error:
  - old FFmpeg `AVFrame` lacks `duration` field (`pkt_duration` fallback required)
- Added compatibility accessor in `include/media/ffmpeg_channel_layout_compat.h`:
  - `frameDuration(const AVFrame* frame)`
- Replaced direct `frame->duration` usage in `src/core/player_core.cpp` decode timing paths.
- Synced docs/records for this round:
  - analysis: `PLAYERCORE_DAY60_LOG_WORKFLOW_ERROR_CLOSURE.md`
  - design: `CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_LOG_WORKFLOW_ERROR_FIX_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--performance-log-check` PASS
  - `--embedded-subtitle-live-packet-check` PASS
  - Linux gate script syntax check PASS
- Linux runner evidence still requires push + CI execution.
### 2026-03-26 Update: Linux CI compatibility stabilization (FFmpeg/libass + workflow gate determinism)
- Added FFmpeg channel-layout compatibility layer:
  - `include/media/ffmpeg_channel_layout_compat.h`
  - migrated channel-layout usage in `player_core`, `main`, and `demuxer`
- Updated `PlayerCore` audio resampler compatibility path:
  - state moved to layout-mask/channel-count cache fields
  - modern swresample path uses `swr_alloc_set_opts2`
  - legacy swresample path keeps `swr_alloc_set_opts`
- Fixed Linux-specific build blockers:
  - `libass` header compatibility (`ass/ass.h` / `libass/ass.h`)
  - OpenGL enum token rename (`ColorGamutMode::Disabled`) to avoid macro collision risk
- Hardened CI workflow `.github/workflows/cross-platform-gate.yml`:
  - generate probe media fixture automatically when missing (Windows/Linux)
  - build `sample_logger_plugin` alongside `modern-video-player`
- Synced docs/records for this round:
  - analysis: `PLAYERCORE_DAY59_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION.md`
  - design: `CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_LINUX_CI_COMPATIBILITY_AND_GATE_STABILIZATION_LOCAL_CHECK.md`
- Local validation:
  - Release build (`modern-video-player` + `sample_logger_plugin`) PASS
  - `--performance-log-check` PASS
  - `--embedded-subtitle-live-packet-check` PASS
  - `--d3d11-diagnostics` PASS
  - Linux gate script syntax check PASS
  - Linux gate script runtime dispatch expected non-Linux FAIL on Windows host
- Linux CI green evidence still requires push + runner execution.
### 2026-03-26 Update: Linux gate reporting/artifact closure
- Added machine-readable Linux gate report contract in `tools/run_linux_mvp_checks.sh`:
  - arg #10 / env `MVP_LINUX_GATE_REPORT_FILE`
  - per-check output `check.<id>.*`
  - gate-level output `gate.*`, including explicit fail reason/result
- Added stable check IDs in Linux gate execution path for CI parser stability.
- Updated `.github/workflows/cross-platform-gate.yml` Linux lane:
  - `set -euo pipefail`
  - tee Linux gate output to `logs/linux-mvp-gate.log`
  - write Linux gate summary to `logs/linux-mvp-gate-summary.env`
  - upload `logs/*.env` artifacts
- Synced tasklist/docs indexes and round documents:
  - analysis: `PLAYERCORE_DAY58_LINUX_GATE_REPORTING_AND_CI_ARTIFACT_CLOSURE.md`
  - design: `CROSS_PLATFORM_LINUX_GATE_REPORTING_AND_ARTIFACT_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_LINUX_GATE_REPORTING_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_LINUX_GATE_REPORTING_LOCAL_CHECK.md`
- Local validation:
  - Linux gate script syntax check PASS
  - Linux gate runtime dispatch expected non-Linux FAIL on Windows host
  - Release build PASS
  - `--performance-log-check` PASS
  - `--embedded-subtitle-live-packet-check` PASS
  - `--d3d11-diagnostics` PASS
- Full Linux runtime PASS evidence still requires Linux host/CI execution.

### 2026-03-26 Update: Linux gate strict optional checks closure (`CP-507` / `CP-508` coverage hardening)
- Hardened Linux gate script `tools/run_linux_mvp_checks.sh` for deterministic subtitle backlog coverage:
  - auto-generate CP-508 embedded ASS fixture when absent (via `ffmpeg`)
  - strict optional-check mode (`REQUIRE_OPTIONAL_CHECKS` arg or `MVP_REQUIRE_OPTIONAL_CHECKS` env)
  - explicit generation inputs for base media + ASS subtitle path
- Updated CI workflow `.github/workflows/cross-platform-gate.yml` Linux lane:
  - install `ffmpeg`
  - run Linux gate in strict mode so `CP-507` / `CP-508` cannot be silently skipped
- Updated tasklist/docs indexes and added round documents:
  - analysis: `PLAYERCORE_DAY57_LINUX_GATE_STRICT_OPTIONAL_CHECKS_CLOSURE.md`
  - design: `CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_DESIGN_2026-03-26.md`
  - plan: `CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_CHECKS_PLAN_2026-03-26.md`
  - report: `CROSS_PLATFORM_LINUX_GATE_STRICT_OPTIONAL_LOCAL_CHECK.md`
- Local validation:
  - Release build PASS
  - `--performance-log-check` PASS
  - `--embedded-subtitle-live-packet-check` PASS
  - Linux gate script syntax check PASS (`C:\Program Files\Git\bin\bash.exe -n tools/run_linux_mvp_checks.sh`)
  - Linux gate script runtime dispatch expected FAIL on Windows host (`This gate script only supports Linux.`)
- Full Linux runtime gate execution still requires Linux host/CI.

### 2026-03-26 Update: CP-507 and CP-508 Linux subtitle backlog closure
- Landed `CP-507` Linux libass shaping/layout probe baseline:
  - new module `subtitle/libass_probe`
  - command `--libass-shaping-check <subtitle.(ass|ssa)>`
  - machine-readable fields for libass init/track/events/render output
- Landed `CP-508` embedded subtitle live packet probe baseline:
  - new loader API `probeEmbeddedSubtitleLivePacketPath(...)`
  - command `--embedded-subtitle-live-packet-check <media_file> [stream_index] [max_packets]`
  - packet counters, timestamp monotonicity, decode output diagnostics
- Linux gate script updated with optional CP-507/CP-508 stages:
  - `tools/run_linux_mvp_checks.sh`
- Updated `CROSS_PLATFORM_MASTER_TASKLIST.md`: `CP-507` and `CP-508` moved to `DONE`.
- Local validation:
  - Release build PASS
  - `--performance-log-check` PASS
  - `--d3d11-diagnostics` PASS
  - `--embedded-subtitle-live-packet-check` PASS
  - `--libass-shaping-check` returns expected non-Linux FAIL on this Windows host.
## 历史版本记录（旧档保留）

### 2026-03-26 Update: CP-801 and CP-901 ~ CP-905 HDR present / observability / CI closure
- Landed the real D3D11 HDR present/runtime diagnostics closure:
  - D3D11 DXGI HDR output probe surface
  - HDR-aware swapchain/color-space decisions
  - `--d3d11-hdr-output-check`
  - D3D11 present timing fields in `--performance-log-check`
- Landed Phase 9 closure assets:
  - `tools/collect_driver_quirk_sample.ps1`
  - `docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv`
  - upgraded `tools/run_linux_mvp_checks.sh`
  - `.github/workflows/cross-platform-gate.yml`
  - `tools/package_windows.ps1`
- Added/updated docs:
  - `docs/analysis/PLAYERCORE_DAY54_CP801_CP901_CP905_HDR_PRESENT_AND_CI_CLOSURE.md`
  - `docs/design/CROSS_PLATFORM_HDR_PRESENT_OBSERVABILITY_AND_CI_DESIGN_2026-03-26.md`
  - `docs/plans/CROSS_PLATFORM_PHASE9_OBSERVABILITY_CI_PLAN_2026-03-26.md`
  - `docs/reports/CROSS_PLATFORM_PHASE8_LOCAL_CHECK.md`
  - `docs/reports/CROSS_PLATFORM_PHASE9_LOCAL_CHECK.md`
- Updated `CROSS_PLATFORM_MASTER_TASKLIST.md` status for `CP-801` and `CP-901~CP-905` to `DONE`.
- Local validation: Release build PASS, `--d3d11-diagnostics` PASS, `--d3d11-hdr-output-check` PASS, D3D11 `--performance-log-check` PASS, driver sample collection PASS, Windows ZIP packaging PASS.

### 2026-03-26 Update: CP-501 ~ CP-506 subtitle/font platform closure
- Landed embedded subtitle policy closure with policy merge checks and ownership hardening (`external > embedded` with embedded fallback).
- Closed Phase-5 font paths:
  - Linux fontconfig app-font register/rebuild/cleanup flow in `subtitle_font_registry`
  - Windows DirectWrite custom collection check integrated into `tools/run_opengl_checks.ps1`
- Added/updated docs:
  - `docs/analysis/PLAYERCORE_DAY51_CP501_CP506_SUBTITLE_FONT_PLATFORM_CLOSURE.md`
  - `docs/design/CROSS_PLATFORM_SUBTITLE_FONT_PLATFORM_CLOSURE_DESIGN_2026-03-26.md`
  - `docs/reports/CROSS_PLATFORM_PHASE5_LOCAL_CHECK.md`
  - `docs/plans/CROSS_PLATFORM_PHASE5_SUBTITLE_FONT_CLOSURE_PLAN_2026-03-26.md`
- Updated `CROSS_PLATFORM_MASTER_TASKLIST.md` status for `CP-501~CP-506` to `DONE`.
- Local validation: Release build PASS, Phase-5 CLI checks PASS, OpenGL gate PASS (`18/18`, includes DirectWrite check).

### 2026-03-26 Update: CP-401 ~ CP-406 Linux MVP playback closure
- Added the Phase-4 Linux MVP gate command set:
  - `--linux-software-audio-check`
  - `--linux-opengl-playback-check`
  - `--linux-opengl-fallback-check`
  - `--linux-audio-backend-smoke`
  - `--core-playback-behavior-check`
  - `--ui-interaction-check`
- Added `MVP_OPENGL_FORCE_INIT_FAIL=1` so `OpenGL -> SoftwareSDL` fallback validation is deterministic.
- Added `tools/run_linux_mvp_checks.sh` as the Linux MVP gate baseline.
- Updated `CROSS_PLATFORM_MASTER_TASKLIST.md` status for `CP-401~CP-406` to `DONE`.
- Local validation: configure/build PASS; shared playback checks (`performance-log-check`, `renderer-fallback-check`, `interaction-freeze-check`) PASS. Linux host runtime execution remains pending on a Linux machine.

### 2026-03-26 Update: CP-301 ~ CP-305 build switch platformization and Linux packaging baseline
- Added explicit backend feature switches in `CMakeLists.txt`:
  - `ENABLE_D3D11_RENDERER`, `ENABLE_OPENGL_RENDERER`, `ENABLE_SDL_RENDERER`
  - `ENABLE_D3D11VA`, `ENABLE_DXVA2`, `ENABLE_VAAPI`, `ENABLE_VIDEOTOOLBOX`
- Added platform force-off rules and switch-aware renderer/decoder source boundaries.
- Extended startup diagnostics with compiled/runtime backend sets for renderer and decoder.
- Added Linux dependency closure/package baseline:
  - CPack generators `DEB;TGZ`
  - `tools/package_linux.sh`
- Local validation: default build PASS, switch-matrix builds PASS, switch-aware diagnostics commands PASS.

### 2026-03-26 Update: CP-201 ~ CP-205 renderer/input/overlay boundary closure
- Split renderer-adjacent responsibilities into explicit role interfaces:
  - `input::IPlaybackInputSource`
  - `render::IRenderOverlaySink`
- Shrunk `IVideoRenderer` back to rendering-focused responsibilities and rewired `PlayerCore` / idle window paths to the new roles.
- Added owner-thread guardrails for event pumping across `PlayerCore`, `Display`, `D3D11`, and `OpenGL` paths.
- Added `--interaction-freeze-check` and integrated it into `tools/run_opengl_checks.ps1`.
- Local validation: Release build PASS, `performance-log-check` PASS, `renderer-fallback-check` PASS, `interaction-freeze-check` PASS, OpenGL gate PASS.

### 2026-03-25 Update: CP-101 ~ CP-106 startup strategy/capability extraction
- Added `platform_capabilities` to unify platform/backend capability probing.
- Added `playback_strategy` planning so `PlayerCore::open()` consumes an explicit startup plan instead of embedding policy logic.
- Moved default policy responsibilities out of `RendererFactory` / `DecoderFactory` and into strategy planning.
- Extended `--performance-log-check` with machine-readable startup strategy fields: capabilities, candidates, selected path, and fallback reason.
- Local validation: Release build PASS, `performance-log-check` PASS, `renderer-fallback-check` PASS.

### 2026-03-26 Update: Cross-platform Phase 8 ICC/profile-driven output LUT closure (`CP-802 ~ CP-805`)
- Added reusable output color helper:
  - `.cube` parser moved into shared output-color helper
  - ICC matrix/TRC profile -> sampled 3D LUT generation
- Added OpenGL display/output binding state with:
  - display index / display name / display device name
  - auto ICC resolution from current display on Windows
  - render-thread LUT reload on display/output binding changes
- Added new env / CLI surfaces:
  - `MVP_OPENGL_ICC_PROFILE_FILE`
  - `MVP_OPENGL_AUTO_ICC`
  - `--opengl-icc-profile <icc_profile_file>`
  - `--opengl-auto-icc`
  - `--opengl-output-color-icc-check <media_file> [sample_ms]`
- Extended diagnostics:
  - `renderer_opengl_output_lut_source`
  - `renderer_opengl_output_lut_reload_count`
  - `renderer_opengl_output_display_*`
  - `renderer_opengl_output_icc_profile_*`
  - `renderer_opengl_output_binding_error`
  - `opengl-diagnostics.output_display.*`
- Expanded `tools/run_opengl_checks.ps1` to `25` checks with manual cube and auto ICC output-color regressions.
- Validation: Release build PASS, manual output-color check PASS, auto ICC output-color check PASS, OpenGL diagnostics PASS, OpenGL gate PASS (`25/25`).
- Scope note: at the time of this Phase-8 partial closure, `CP-801` still remained open; it is closed by the later 2026-03-26 HDR present update above.

### 2026-03-26 Update: Cross-platform Phase 6 bitmap subtitle pipeline closure (`CP-601 ~ CP-605`)
- Upgraded bitmap subtitle modeling from single-rect items to packet-level items with `bitmap_rects` aggregation.
- Added bitmap loader statistics:
  - `bitmap_rect_count`
  - `bitmap_multi_rect_item_count`
  - `bitmap_max_rects_per_item`
- Hardened bitmap subtitle timing fallback against invalid / oversized display-window metadata from real PGS samples.
- Added renderer-side bitmap cache/reuse for OpenGL and D3D11 subtitle D2D paths.
- Added machine-readable bitmap regression commands:
  - `--bitmap-subtitle-check <media_file> [stream_index]`
  - `--bitmap-subtitle-stress-check`
- Expanded `tools/run_opengl_checks.ps1` to `23` checks with DVD / PGS bitmap CLI + playback regression and multi-rect stress coverage.
- Validation: Release build PASS, DVD bitmap check PASS, PGS bitmap check PASS, bitmap stress check PASS, OpenGL gate PASS (`23/23`).

### 2026-03-24 鏇存柊锛歄penGL 璇婃柇蹇収銆乴ibass 宸窛娓呭崟涓庢樉绀虹骇 HDR 璁捐鏀舵暃
- `OpenGLVideoRenderer` 鏂板 `OpenGLDiagnosticsSnapshot` 涓?`OpenGLVideoRenderer::probeSystemDiagnostics()`锛屽苟鎻愪緵 `--opengl-diagnostics` 鏈哄櫒鍙杈撳嚭銆?- OpenGL native interop 鍚姩鏈熺瓥鐣ュ凡浠庨浂鏁ｅ垽鏂敹鏁涗负 `hard blocker + quirk rule + env override` 涓夊眰鍐崇瓥锛宍force` 鍙厑璁歌鐩?quirk锛屼笉瑕嗙洊鐪熸鐨?hard blocker銆?- 宸叉柊澧?`docs/analysis/PLAYERCORE_DAY27_OPENGL_LIBASS_GAP_AND_QUIRK_DIAGNOSTICS.md`锛屾妸褰撳墠 OpenGL 涓?`libass/mpv` 鐨勫樊璺濇媶鍒?`shaping / karaoke / vector / font fallback / layout` 绾у埆銆?- 宸叉柊澧?`docs/design/OPENGL_DISPLAY_HDR_OUTPUT_DESIGN.md`锛屾槑纭?Windows 涓嬫樉绀虹骇 HDR 杈撳嚭搴旇蛋 `DXGI swapchain + HDR metadata` 妗ユ帴锛岃€屼笉鏄户缁妸 tone-map 褰撴垚瀹屾暣 HDR 鏂规銆?- Release 楠岃瘉閫氳繃锛歚--opengl-diagnostics=PASS`銆乣MVP_OPENGL_NATIVE_INTEROP=disable --opengl-diagnostics=PASS`銆丱penGL `performance-log-check=PASS`銆乣subtitle-sync-check=PASS`銆乣delay-adjust-check=PASS`銆?
### 2026-03-24 鏇存柊锛歄penGL M1 鎴愮啛鍖栨敹鏁?- `OpenGLVideoRenderer` 宸蹭粠浠呰兘鎾斁鐨?`M0` 杩囨浮鍚庣锛屾帹杩涘埌甯﹀瓧骞曘€佸惎鍔ㄦ湡 quirk 鍐崇瓥鍜屽熀纭€ HDR/鑹插僵澶勭悊鐨?`M1` 绾у埆锛涘綋鍓嶄粛淇濇寔 `opt-in`锛屼笉鏇夸唬榛樿 `D3D11` 鍚庣銆?- `ASS/SSA` 鐜伴樁娈甸噰鐢?`DirectWrite + D2D offscreen -> OpenGL texture` 璺緞锛屾敮鎸佸 `SubtitleItem` 鎺掑簭銆乮tem/run 鍒嗘鏍峰紡銆佸畾浣嶅榻愩€佹弿杈广€侀槾褰便€佽儗鏅涓庡～鍏咃紱浠嶆湭杈惧埌 `libass/mpv` 鐨勫畬鏁撮珮绾х壒鏁堣鐩栥€?- OpenGL 鍚姩鏈熸柊澧?`MVP_OPENGL_NATIVE_INTEROP=auto|disable|force`锛屽苟瀵?`Microsoft / GDI Generic` 杞欢 GL 鐜鐩存帴绂佺敤 native interop锛涘悓鏃惰緭鍑?`[diag:opengl-native]` 閫傞厤鍣ㄣ€侀┍鍔ㄣ€乨ecoder profile 涓庢渶缁堣鍒欐棩蹇椼€?- GLSL/HLSL 鑹插僵閾捐矾琛ラ綈 `BT.601 / BT.709 / BT.2020` 鐭╅樀閫夋嫨銆乣PQ / HLG` 妫€娴嬨€佸熀纭€ SDR tone-map 鍜?`BT.2020 -> BT.709` gamut mapping锛屽苟杈撳嚭 `[diag:opengl-color]`銆?- Release 楠岃瘉閫氳繃锛歚OpenGL performance-log-check=PASS`銆乣subtitle-sync-check=PASS`銆乣delay-adjust-check=PASS`銆?
### 2026-03-24 鏇存柊锛歄penGL M0 娓叉煋閾捐矾钀藉湴
- `OpenGLVideoRenderer` 宸蹭粠 stub 鍙樹负鍙敤鍚庣锛屾敮鎸?`YUV420P / NV12` 鐩存帴甯с€丟LSL 120 鑹插僵杞崲涓庡熀纭€閿洏鎺у埗銆?- 褰撳墠绛栫暐淇濇寔淇濆畧锛氶粯璁ゅ悗绔粛涓?`D3D11` / `SoftwareSDL` 鍥為€€锛宍OpenGL` 闇€閫氳繃 `MVP_RENDERER_BACKEND=opengl` 鏄惧紡鍚敤銆?- 鏈湴 `Release` 楠岃瘉锛歚--performance-log-check .\juren-30s.mp4 2000` 杈撳嚭 `renderer_backend=OpenGL`銆乣result=PASS`銆?- 褰撳墠 `OpenGL` 璺緞瀹氫綅涓?M0锛屼笉绛夊悓浜庢垚鐔熸挱鏀惧櫒 GPU 鍚庣锛涘瓧骞?OSD 鍙犲姞銆佸師鐢熺‖浠惰〃闈簰鎿嶄綔銆佽繘闃惰壊褰╃鐞嗕粛鏈ˉ榻愩€?


閺堫剚鏋冨锝堫唶瑜版洟銆嶉惄顔炬畱閻楀牊婀伴崣妯绘纯閸樺棗褰堕崪灞界秼閸撳秶濮搁幀浣碘偓?


## 瑜版挸澧犻悧鍫熸拱娣団剝浼?


### 缁楊兛绗侀弬鐟扮氨閻楀牊婀?


| 缂佸嫪娆?| 閻楀牊婀?| 鐠囧瓨妲?|

|------|------|------|

| FFmpeg | 8.0.1 | 婢舵艾鐛熸担鎾愁槱閻炲棙顢嬮弸?|

| SDL2 | 2.30.11 | 婢舵艾鐛熸担鎾崇氨閿涘牊妯夌粈鍝勬嫲闂婃娊顣堕敍?|

| Quill | 6.0.0 | 瀵倹顒?Console + Rotating 閺冦儱绻?|

| CMake | 3.15+ | 閺嬪嫬缂撶化鑽ょ埠 |

| C++ 閺嶅洤鍣?| C++17 | 缂傛牞鐦ч弽鍥у櫙 |



**濞夈劍鍓?*: Quill 閻?`external/quill` 閹绘劒绶甸敍灞惧灗闁俺绻冪拋鍓х枂 `QUILL_ROOT` 閹稿洤鎮滅化鑽ょ埠鐎瑰顥婇惃?v6.0.0+ 閻楀牊婀伴妴?


### 妞ゅ湱娲伴悧鍫熸拱



- **妞ゅ湱娲伴悧鍫熸拱**: 1.0.0

- **瑜版挸澧犻崣鎴濈閸婃瑩鈧?*: 1.0.0-rc1

- **閸欐垵绔烽悩鑸碘偓?*: 閸欘垰褰傜敮?RC閿涘奔绗夊楦款唴閻╁瓨甯?GA

- **閺嬪嫬缂撶猾璇茬€?*: Release / Debug

- **閺€顖涘瘮楠炲啿褰?*: Windows, Linux, macOS

- **缁嬪绨弰鍓с仛閻楀牊婀?*: 1.0.0-rc1閿涘潉--version`閿?
- **Windows 閺傚洣娆㈤悧鍫熸拱**: 1.0.0.0

- **閸欐垵绔烽崠鍛瀮娴犺泛鎮?*: modern-video-player-1.0.0-rc1-windows-x64.zip



### 2026-03-23 閺囧瓨鏌婇敍姝奀 閻楀牊婀伴崗鍐╂殶閹诡喓鈧阜elease 妞ゅ灚顒滈弬鍥︾瑢閸栧懐澧楅張顒佺垼鐠囧棜藟姒?
- 瀹稿弶鏌婃晶鐐插讲閻╁瓨甯寸拹鏉戝煂 GitHub Release 妞ょ數娈戝锝嗘瀮閺傚洣娆㈤敍姝歞ocs/reports/V1_0_0_RC1_RELEASE_NOTES.md`閵?
- `CMake` 閻滄澘鍑″鏇炲弳缂佺喍绔撮悧鍫熸拱濠ф劧绱伴崺铏诡攨閻楀牊婀?`1.0.0`閿涘矂顣╅崣鎴濈閸氬海绱?`rc1`閿涘瞼绮烘稉鈧悽鐔稿灇缁嬪绨崘鍛村劥閻楀牊婀版稉灞傗偓涔刬ndows 鐠у嫭绨悧鍫熸拱娴犮儱寮烽幍鎾冲瘶閺傚洣娆㈤崥宥冣偓?
- `main` 閺傛澘顤?`--version`閿涙稑缍嬮崜宥呯杽濞村绶崙杞拌礋閿涙瓪Modern Video Player 1.0.0-rc1`閵?
- Windows `VERSIONINFO` 瀹歌尪藟姒绘劧绱辫ぐ鎾冲鐎圭偞绁寸紒鎾寸亯娑撶尨绱?  - `FileVersion=1.0.0.0`
  - `ProductVersion=1.0.0-rc1`
  - `ProductName=Modern Video Player`

- 閺傛澘顤?`CPack ZIP` 閹垫挸瀵樼憴鍕灟閿涙稑缍嬮崜宥呯杽濞村楠囬悧鈺€璐熼敍姝歜uild/modern-video-player-1.0.0-rc1-windows-x64.zip`閵?
- 瑜版挸澧犻崣鎴濈閸栧懎鍑＄涵顔款吇閸栧懎鎯?`modern-video-player.exe`閵嗕椒绶风挧?DLL閵嗕梗plugins/sample_logger_plugin.dll` 娑?`RELEASE_NOTES.md`閿涘奔绗栭張顏呭ⅵ閸忋儲婀伴崷?`config/player_settings.ini`閵?
- `HTTP` 娑撳娴囬柧鎹愮熅閻?`user_agent` 閻滄澘鍑＄紒鐔剁閺囧瓨鏌婃稉?`modern-video-player/1.0.0-rc1`閿涘奔绌舵禍搴℃倵缂?RC 闂冭埖顔岄弮銉ョ箶瑜版帗銆傛稉搴ｅ殠娑撳﹥甯撻梾婧库偓?
### 2026-03-23 閺囧瓨鏌婇敍?.0.0-rc1 閸欐垵绔烽崙鍡楊槵鐎瑰本鍨?
- 瑜版挸澧犵紒鎾诡啈瀹稿弶鏁归崣锝勮礋閿涙瓪閸欘垱澧?v1.0.0-rc1 閺嶅洨顒穈閿涘奔绲炬稉宥呯紦鐠侇喚娲块幒銉ュ絺鐢啯顒滃蹇曞 `1.0.0`閵?
- 閺堫剝鐤嗛崺杞扮艾 `Release` 閺嬪嫬缂撻柌宥嗘煀閹笛嗩攽楠炲爼鈧俺绻冮敍?  - `tools/run_all_checks.ps1`
  - `--d3d11-diagnostics`
  - `--performance-log-check .\juren-30s.mp4 2000`
  - `--long-playback-check .\juren-30s.mp4 10000`
  - `--serial-failsession-regression-check .\juren-30s.mp4`

- 閺堚偓閺傜増鐗稿蹇撴礀瑜版帗濮ら崨?`docs/reports/FORMAT_REGRESSION_20260323_224615.md` 濮瑰洦鈧崵绮ㄩ弸婊€璐熼敍姝?7 PASS / 0 PARTIAL / 0 FAIL / 0 SKIP`閵?
- 瀹稿弶鏌婃晶?RC 濮瑰洦鈧粯鏋冨锝忕窗`docs/reports/V1_0_0_RC1_RELEASE_READINESS.md`閿涘瞼绮烘稉鈧弨璺哄經閸欐垵绔峰〒鍛礋閵嗕礁鍑￠惌銉╂６妫版ǜ鈧礁褰傜敮鍐嚛閺勫骸鎷版禒濠傘亯閻ㄥ嫰鐛欑拠浣界槈閹诡喓鈧?
- 瑜版挸澧?RC 娴犲秳绻氶悾娆庤⒈缁粯妲戠涵顕€顥撻梽鈺嬬窗
  - `闂傤噣顣?79` 鐎电懓绨查惃?software video decode 鏉╂劘顢戦幀浣界熅瀵板嫪绮涢張顏勭暚閸忋劍鏁归崣?  - D3D11 driver / adapter quirk blacklist 鐟欏嫬鍨禒宥夋付缂佈呯敾閹碘晛鍘?### 2026-03-23 閺囧瓨鏌婇敍娆?D11 decoder profile 閹恒垺绁撮妴涔箄irk blacklist 娑撳海瀚粩?diagnostics CLI

- `D3D11VideoRenderer` 閻滄澘鍑℃潏鎾冲毉缂佹挻鐎崠?`D3D11DiagnosticsSnapshot`閿涘瞼绮烘稉鈧崠鍛儓 adapter/driver閵嗕公eature level閵嗕梗NV12/P010/P016` 閺嶇厧绱￠弨顖涘瘮閵嗕龚ecoder profiles 娑?native direct 閸氼垰濮╅張鐔虹摜閻ｃ儯鈧?
- 閺傛澘顤?decoder profile 閹恒垺绁撮敍灞界秼閸撳秵婧€閸ｃ劍娓堕弬鏉跨杽濞村绮ㄩ弸婊€璐熼敍?  - `H.264`閿涙碍鏁幐?  - `HEVC`閿涙碍鏁幐?`Main / Main10`
  - `VP9`閿涙碍鏁幐?`Profile0`閿涘奔绗夐弨顖涘瘮 `Profile2 10bit`
  - `AV1`閿涙艾缍嬮崜宥夆偓鍌炲帳閸ｃ劍婀弳鎾苟閺€顖涘瘮 profile

- 閺傛澘顤?startup-time quirk / blacklist 閸愬磭鐡ラ敍娑滃閸涙垝鑵?software adapter閵嗕胶宸辩亸鎴濆彠闁?D3D11 video 閹恒儱褰涢妴涔V12` 娑撳秵寮х搾宕囨纯闁插洦鐗辩憰浣圭湴閹存牕鎳℃稉顓☆潐閸掓瑨銆冮敍灞藉灟娴兼艾婀幘顓熸杹閸撳秶娲块幒銉ュ彠闂?native direct閵?
- 妫ｆ牜澧?blacklist 瀹稿弶妯夊蹇氼洬閻?`Microsoft Basic Render Driver`閿涙稑缍嬮崜宥嗘簚閸ｃ劏鐦栭弬顓犵波閺嬫粈璐?`native_direct.allowed=true`閵嗕梗disable_rule=none`閵?
- `main` 閺傛澘顤?`--d3d11-diagnostics`閿涘苯褰叉稉鈧▎鈩冣偓褑绶崙鐑樻簚閸ｃ劌褰茬拠?`key=value` 缂佹挻鐏夐敍宀€鏁ゆ禍搴ゅ殰閸斻劌瀵查妴浣哄箛閸︾儤甯撻梾婊冩嫲閸氬海鐢绘す鍗炲З閸忕厧顔愰幀褍缍婂锝冣偓?### 2026-03-23 閺囧瓨鏌婇敍娆?D11 閸氼垰濮╅張鐔诲厴閸旀稒甯板ù瀣╃瑢 adapter/driver 鐠囧﹥鏌囬弮銉ョ箶

- `D3D11VideoRenderer` 閸掓繂顫愰崠鏍▉濞堝灚鏌婃晶?`[diag:d3d11-init]` 閺冦儱绻旈敍宀€娲块幒銉ㄧ翻閸?adapter閵嗕龚river version閵嗕公eature level閵嗕礁鍙ч柨?D3D11/DXGI 閹恒儱褰涢崣顖滄暏閹佲偓浣圭壐瀵繑鏁幐浣风秴娑?swap chain 閸欏倹鏆熼妴?
- 瑜版挸澧犻張鍝勬珤閺堚偓閺傛澘鐤勫ù瀣嚒閼宠棄婀崥顖氬З閺冨墎娲块幒銉ф箙閸掑府绱癭adapter="NVIDIA GeForce GTX 1080"`閵嗕梗driver_version=32.0.15.6094`閵嗕梗feature_level=11_1`閵嗕梗device3=true`閵嗕梗video_device=true`閵嗕梗NV12/P010` 閺€顖涘瘮閹懎鍠岄敍灞间簰閸?`P016{check_failed_hr=-2147467259}` 鏉╂瑧琚弽鐓庣础閼宠棄濮忓顔肩磽閵?
- 鏉╂瑨鐤嗘稉宥嗘暭閸欐骞囬張澶嬫尡閺€鎹愮熅瀵板嫸绱濋崣顏囁夐幋鎰暃閹绢厽鏂侀崳銊╂付鐟曚胶娈戦崥顖氬З閺堢喕鐦栭弬顓濈瑐娑撳鏋冮敍娑㈡６妫?90/91 閻?native direct 娑?fallback 闁炬崘鐭炬穱婵囧瘮娑撳秴褰夐妴?### 2026-03-23 閺囧瓨鏌婇敍娆?D11VA 閼奉亜鐣炬稊?hw_frames_ctx 閹恒儱鍙嗛崣顖炲櫚閺嶇柉袙閻浇銆冮棃?
- `PlayerCore` 娑撳秴鍟€閸欘亝濡?`D3D11VA` 鐠佹儳顦幐鍌滅舶 decoder閿涘矁鈧本妲搁崷?`get_format()` 闂冭埖顔岄弰鎯х础閸掓稑缂?`hw_frames_ctx`閿涘苯鑻熸稉?`AVD3D11VAFramesContext::BindFlags` 鏉╄棄濮?`D3D11_BIND_SHADER_RESOURCE`閵?
- 瑜版挸澧犻張鍝勬珤婢跺秵绁寸紒鎾寸亯鐞涖劍妲戦敍宀冪箹娑撯偓濮濄儱鍑＄紒蹇氼唨 D3D11 閸樼喓鏁撻惄鎾櫚閺嶉婀″锝嗕划婢跺稄绱癭Configured D3D11VA frames context for direct shader sampling: bind_flags=520`閿涘苯鎮撻弮?`video_native_output_frames=62`閵嗕梗video_copy_back_frames=0`閵?
- 闂傤噣顣?90 閻ㄥ嫯绻嶇悰灞炬 fallback 娴犲秶鍔ф穱婵堟殌閿涘奔缍旀稉鍝勫従娴犳牠鈹嶉崝?鐠佹儳顦紒鍕値娑?native direct 婢惰精瑙﹂弮鍓佹畱閺堚偓閸氬骸鍘规惔鏇樷偓?### 2026-03-23 閺囧瓨鏌婇敍娆?D11 閸樼喓鏁撻惄鎾櫚閺嶇柉绻嶇悰灞炬婢惰精瑙﹂弨閫涜礋閼奉亜濮╅崶鐐衡偓鈧?copy-back

- `D3D11VideoRenderer` 閺傛澘顤冩潻鎰攽閺?native direct 閻旀梹鏌囬敍姘洤閺?`CreateShaderResourceView1` 閺冪姵纭舵稉?D3D11VA 鐟欙絿鐖滅悰銊╂桨閸掓稑缂?Y/UV plane SRV閿涘本鍨ㄧ憴锝囩垳鐞涖劑娼伴弽鐓庣础娑撳秵鏁幐浣烘纯閹恒儵鍣伴弽鍑ょ礉閸掓瑥鍙ч梻顓熸拱濞嗏€茬窗鐠囨繄娈?native direct rendering閵?
- native direct 鐞氼偄鍙ч梻顓炴倵閿涘畭PlayerCore` 娴兼氨鎴风紒顓熷Ω閸氬海鐢荤涵顒冃掔敮褑铔?copy-back 閸掓媽钂嬫禒璺烘姎閿涘苯鍟€婢跺秶鏁ら悳鐗堟箒鏉烆垯娆㈢痪鍦倞娑撳﹣绱堕柧鎹愮熅閺勫墽銇氶敍宀勪缉閸忓秶鎴风紒顓炲毉閻滄壋鈧粓鐓舵０鎴烆劀鐢悶鈧浇顫嬫０鎴︾拨鐏炲繆鈧縿鈧?
- `Release / Debug` 鐎?`juren-30s.mp4` 閻?`--performance-log-check 2000` 瀹告彃顦插ù瀣偓姘崇箖閿涙稐琚遍懓鍛村厴鐎圭偤妾崨鎴掕厬 `fallback=copyback-to-software` 閸涘﹨顒熼敍灞借嫙娣囨繃瀵?`render_frames > 0`閵?### 2026-03-20 閺囧瓨鏌婇敍姝卨ayerCore 閸撯晙缍戞搴ㄦ珦閺€鑸垫殐閿涘湯cheduler 缂佸牏澧楃粵鏍殣閵嗕笚ailSession 鐎圭偛瀵查妴涔籩rial/generation 鐟欏倹绁村鍝勫閿?
- `SchedulerControlSnapshot` 閺傛澘顤冪紒鎾寸€崠鏍摜閻ｃ儱鐡у▓纰夌窗`clock_policy`閵嗕梗audio_master_policy`閵嗕梗audio_buffered_seconds`閿涘苯鑻熺亸?`ended_policy` 閹碘晛鐫嶉崚?`HoldLastFrameNoClockSync`閵?
- `Scheduler` render wait 閺€閫涜礋濞戝牐鍨傜粵鏍殣閸戣姤鏆熼敍鍧刬sAudioMasterActive` / `isVideoMasterActive` / `shouldApplyClockSync`閿涘绱濋崥灞炬 `Scheduler::stop()` 閺傛澘顤?self-join 娣囨繃濮㈤妴?
- `FailSession` 瀹歌弓绮犳０鍕殌閸忋儱褰涢幒銊ㄧ箻閸掓澘鍙ч柨顔笺亼鐠愩儳鍋ｇ€圭偟鏁ょ捄顖氱窞閿涙矖handleRuntimeFailure()` 閸氬本顒炵悰銉ュ弳 stop/session failure 鐠佲剝鏆熼妴?
- diagnostics 閺傛澘顤?stale serial 娑撱垹绱旂拋鈩冩殶閿涘本妲戠涵?`queue generation` 閸欘亣绀嬬拹锝咁啇閸ｃ劏绔熼悾灞艰厬閺傤叏绱濋懓灞炬＋閺冨爼妫跨痪璺ㄢ€栨径杈ㄦ櫏娴犲秶鏁?item-level serial 娑撹鍨界€规哎鈧?### 2026-03-20 閺囧瓨鏌婇敍姝卨ayerCore 閸擃垯缍旈悽銊╂肠娑擃厼瀵叉稉?runtime failure/recovery policy 閺€璺哄經

- `SchedulerControlSnapshot` 瀹歌尪藟閸?`clock_source`閵嗕梗audio_output_initialized`閵嗕梗audio_master_sync_active` 娑?`ended_policy`閿涘cheduler 娑撳秴鍟€閸欘亪娼張鈧亸蹇斺偓浣告嫲闂嗚埖鏆庨幒銊︽焽濞戝牐鍨傛稉姘鐠囶厺绠熼妴?
- `PlayerCore` 瀹稿弶濡?start/resume/pause/stop request/stop completion/seek/session release 閸擃垯缍旈悽銊х埠娑撯偓閹惰棄鍩?`apply*SideEffects()`閿涘畭play / pause / stop / seek / close / requestDeferredStop / serviceDeferredStop` 閸忋儱褰涙稉宥呭晙閻╁瓨甯撮崼鍡欏殠缁嬪鎷扮拋鎯ь槵閸斻劋缍旈妴?
- `deferred stop` 瀹告彃鑻熼崶鐐电埠娑撯偓 stopping 鐠侯垰绶為敍娉乽ntime fatal 閻滀即鈧俺绻?`FailureRecoveryPolicy + handleRuntimeFailure()` 閺€璺哄經閸掓壆绮烘稉鈧幁銏狀槻閸忋儱褰涢敍灞芥倵缂侇厼褰叉禒銉ф埛缂侇厽澧跨憰鍡欐磰閼煎啫娲块懓灞肩瑝閸愬秴顦查崚鑸典划婢跺秹鈧槒绶妴?
- 閺堫剝鐤嗘稉宥嗘暭婢舵牠鍎?`PlaybackState`閵嗕箒I閵嗕恭opy-back 閹?SoftwareSDL閿涘苯褰х紒褏鐢婚弨鍓佹彛閸愬懏鐗抽悩鑸碘偓浣规簚娑?worker/device 閸斻劋缍旈惃鍕珶閻ｅ被鈧?

### 2026-03-20 閺囧瓨鏌婇敍姝卨ayerCore seek/flush timeline serial 閸栨牜顑囨禍宀勬▉濞?


- `PlayerCore` 瀹告彃绱╅崗?`timeline_serial / pending_seek_serial`閿涘eek 娑撳秴鍟€閸欘亪娼?flush 閸滃苯绔风亸鏂剧秴閸掑洦宕查弮鍫曟？缁捐￥鈧?
- demux packet閵嗕龚ecoded `VideoFrame / AudioFrame`閵嗕购ender 閸?audio consumer 闁炬崘鐭鹃柈钘夊嚒閹恒儰绗?serial 绾剙銇戦弫鍫濆灲鐎规哎鈧?
- `flush` 缂佈呯敾娣囨繄鏆€閿涘奔绲剧拠顓濈疅瀹歌弓绮犻垾婊冩暜娑撯偓婢惰鲸鏅ラ張鍝勫煑閳ユ繈妾风痪褌璐熼垾婊嗙窡閸斺晜绔婚幍顐熲偓婵撶幢閻喐顒滈惃鍕＋閺冨爼妫跨痪鑳珶閻ｅ瞼鏁?serial 閹绘劒绶甸妴?
- diagnostics 娑撳簼绗撴い瑙勵梾閺屻儱鎳℃禒銈呭嚒閺傛澘顤?`timeline_serial / pending_seek_serial` 鏉堟挸鍤敍灞肩┒娴滃骸鎮楃紒顓犳埛缂侇厼浠?EOF/Ended 閸?Scheduler 婵傛垹瀹抽崡鍥╅獓閵?




### 2026-03-19 閺囧瓨鏌婇敍姘乘夋?records / analysis 閺傚洦銆傛稉鈧懛瀛樷偓?


- `CHANGELOG` 閻ㄥ嫰妫舵０妯烩偓鏄忋€冨鑼端夋稉?`闂傤噣顣?78` 缁便垹绱╅敍宀勪缉閸忓秳绮栨径鈺勭箹缂?software decode 鐠囧﹥鏌囩拋鏉跨秿閸︺劍鈧槒銆冮柌灞藉毉閻?`77 -> 79` 鐠哄啿褰块妴?
- 瀹告彃娲栨繅?`闂傤噣顣?69` 鐎电懓绨查惃?analysis 閺傚洦銆傞敍姝歞ocs/analysis/PLAYERCORE_DAY6_DEFERRED_STOP_AND_RUNTIME_DEBT_FIX.md`閵?
- 閺堫剝鐤嗘稉宥嗙Ч閸欏﹣鍞惍渚€鈧槒绶崣妯绘纯閿涘苯褰ч弨璺哄經娴犲﹤銇夐幓鎰唉閸氬海娈戦弬鍥ㄣ€傜槐銏犵穿閸滃苯鍨庨弸鎰焽濞ｂ偓娑撯偓閼峰瓨鈧佲偓?


### 2026-03-19 閺囧瓨鏌婇敍姝卨ayerCore 鏉╂劘顢戦幀?software send probe 鐎靛湱鍙庡鍙夊Ω software decode blocker 閺€鑸垫殐閸?runtime context 瀹割喖绱?
- `--software-video-send-probe` 閻滄澘鍑＄悰銉╃秷 `pre_send_receive_ret`閵嗕梗packet queue round-trip` 娑?`read-ahead` 鐎靛湱鍙庨敍娌梛uren-30s.mp4` 閺堚偓閺傛澘鐤勫ù瀣╄礋 `packet_queue_push_ok=true / packet_queue_pop_ok=true / read_ahead_packets=512 / send_ret=0 / receive_got_frame=true / result=PASS`閵?
- `--software-video-decode-check` 閺傛澘顤?`audio_probe_mode`閿涘苯鑻熼弨顖涘瘮 `MVP_SOFTWARE_DECODE_CHECK_DISABLE_AUDIO=1` 閻?video-only 鐎靛湱鍙庨敍娑欐付閺傛澘鐤勫ù瀣▔缁€鍝勫祮娴?`clock_source=Video`閿涘奔绮涢悞鑸垫Ц `video_packet_dequeue_count=1 / video_send_packet_ok=0 / decode_video_ok=0 / render_frames=0 / result=FAIL`閵?
- `PlayerCore::decodeVideoFrame()` 閺傛澘顤冩禒鍛箚婢у啫褰夐柌蹇撶磻閸氼垳娈?`MVP_SOFTWARE_VIDEO_SEND_OFFTHREAD` 鐠囧﹥鏌囩捄顖氱窞閿涙稑婀?`video-only` 娑撳鐤勫ù瀣╃矝閸戣櫣骞?`Offthread software video send_packet probe timed out after 500ms`閵?
- 缂佹捁顔戦弴瀛樻煀閿涙艾缍嬮崜?blocker 瀹稿弶甯撻梽?FFmpeg software decoder 閺堫兛缍嬮妴涔eceive->send` 妞ゅ搫绨妴涔竌cket queue 娴溿倖甯撮妴涔╡mux read-ahead閵嗕線鐓舵０鎴︽懠娴犮儱寮疯ぐ鎾冲 decode 缁捐法鈻奸張顒冮煩閿涙稑鎮楃紒顓炵安閻╁瓨甯寸€佃鐦?`PlayerCore::initDecoders()` 娴溠冨毉閻?software codec ctx 娑撳海瀚粩?probe 閻ㄥ嫬鐡у▓闈涙▕瀵倶鈧?
### 2026-03-19 閺囧瓨鏌婇敍姘嚒鐞?software decode 閺堚偓鐏?send/dequeue 鐠佲剝鏆熼敍瀹恖ocker 閺€鑸垫殐閸掓壋鈧粓顩绘稉?packet send 閺堫亜鐣幋鎰箲閸ョ偐鈧?


- `PlayerCore` 閺傛澘顤冩稉澶愩€嶉張鈧亸蹇氱槚閺傤叏绱?
  - `video_packet_dequeue_count`

  - `video_send_packet_ok`

  - `video_send_packet_last_ret`

- 鐎瑰啩婊戝鑼病閹恒儱鍙?`DiagnosticsSnapshot`閵嗕椒缍嗘０鎴︽懠鐠侯垱妫╄箛妞尖偓涔?-performance-log-check` 娑?`--software-video-decode-check`閵?
- 濮濓絽鐖舵稉濠氭懠鐎靛湱鍙庨弽閿嬫拱 `juren-30s.mp4` 閻ㄥ嫭娓堕弬鎵波閺嬫粈璐熼敍?
  - `video_packet_dequeue_count=57`

  - `video_send_packet_ok=57`

  - `video_send_packet_last_ret=0`

  - `result=PASS`

- software decode 閺嶉攱婀伴惃鍕付閺傞缍嗘０鎴ｇ槚閺傤厺璐熼敍?
  - `v_pkt_deq=1`

  - `v_send_ok=0`

  - `v_send_ret=-2147483648`

- 缂佹捁顔戦弴瀛樻煀閿涙oftware decode 缁捐法鈻煎鑼病閸欐牕鍩屾＃鏍﹂嚋鐟欏棝顣堕崠鍜冪礉娴ｅ棝顩绘稉?`avcodec_send_packet()` 閸掓媽鐦栭弬顓犲仯娴犲秵婀ぐ銏″灇閹存劕濮涙潻鏂挎礀閿涙稑缍嬮崜?blocker 瀹歌尪绻樻稉鈧銉︽暪閺佹稑鍩?decoder 妫ｆ牕瀵橀柅浣稿弳闂冭埖顔岄妴?




### 2026-03-19 閺囧瓨鏌婇敍姝磑ftware decode 娣囨繂鐣х痪璺ㄢ柤闁板秶鐤嗘径宥堢獓閸氬簼绮涢崡鈥虫躬妫ｆ牔閲滅憴鍡涱暥閸栧拑绱漙SdlVideoRenderer` 濞夈劑鍣存稊杈╃垳瀹稿弶绔婚悶?


- `Video decoder threading` 閻滄澘鍑＄涵顔款吇 software path 鏉╂劘顢戦崷?`thread_count=1 / thread_type=none`閿涘矁顕╅弰搴ょ箹鏉烆喖顦查弽鍝モ€樼€圭偛鎳℃稉顓濈啊閳ユ粈绻氱€瑰牏鍤庣粙瀣帳缂冾喒鈧繄娲伴弽鍥╁Ц閹降鈧?
- 闁插秵鏌婇幍褑顢?`--software-video-decode-check` 閸氬函绱濈紒鎾寸亯娴犲秶鍔ч弰顖ょ窗`renderer_backend=SoftwareSDL`閵嗕梗decoder_backend=Software`閵嗕梗decode_video_ok=0`閵嗕梗scheduler_video_decoded_frames=0`閵嗕梗render_frames=0`閵嗕梗video_frame_queue_peak_size=0`閵嗕梗result=FAIL`閵?
- 缂佹挸鎮庨幒褍鍩楅崣鎷岀槚閺傤厽妫╄箛妤呭櫡 `demux(v=163) / pkt_q(v=162)` 娑撳簼绮庨崙铏瑰箛 `Video decode first send_packet start` 閻ㄥ嫮骞囩挒鈽呯礉閸欘垯浜掗幒銊︽焽 software decode 缁捐法鈻艰ぐ鎾冲閺囨潙鍎氶弰顖氭躬妫ｆ牔閲滅憴鍡涱暥閸栧懏褰佹禍銈夋▉濞堥潧鎮楅崑婊€缍囬敍宀冣偓灞肩瑝閺勵垰宕遍崷?copy-back / swscale / display copy閵?
- `src/render/sdl_video_renderer.cpp` 瀹歌尪藟娣?9 婢跺嫬鍤遍弫鏉裤仈濞夈劑鍣存稊杈╃垳閿涙稒婀版潪顔煎晙濞嗏剝澹傞幓?`src/`閵嗕梗include/` 閻?`///` 娑?`//` 濞夈劑鍣寸悰宀嬬礉閺堫亜鍟€閸欐垹骞囬弬鎵畱閸欘垳鏋掓稊杈╃垳閸涙垝鑵戦妴?




### 2026-03-19 閺囧瓨鏌婇敍姘煀婢?`--software-video-decode-check`閿涘本濡?software video decode blocker 閸楁洜瀚柦澶嬵劥



- 閺傛澘顤?`--software-video-decode-check <media_file> [sample_ms]`閿涘奔绗撻梻銊ユ礀缁涙柡鈧笩oftware video decode 閺勵垰鎯侀惇鐔烘畱娴溠冩姎閳ユ繐绱濇稉宥呭晙缂佈呯敾婢跺秶鏁ら崣顏囧厴鐠囦焦妲戦垾婊€绱扮拠婵嗗嚒閸氼垰濮╅垾婵堟畱 `session-check`閵?
- 閸涙垝鎶ら崘鍛村劥瀵搫鍩?`MVP_RENDERER_BACKEND=software`閵嗕梗SDL_AUDIODRIVER=dummy` 閸?`preferHardwareDecode=false`閿涘奔绻氱拠渚€鐛欑拠渚€鎽肩捄顖溓旂€规艾鎳℃稉?`SoftwareSDL + Software decode`閵?
- 閸涙垝鎶ら柅姘崇箖閺夆€叉鐞氼偅鏁圭槐褌璐熼垾婊呮埂鐎圭偘楠囩敮褉鈧繆鈧奔绗夐弰顖椻偓婊冨涧閹垫挸绱戦幋鎰閳ユ繐绱拌箛鍛淬€忛崥灞炬濠娐ゅ喕 `decode_video_ok > 0`閵嗕梗scheduler_video_decoded_frames > 0`閵嗕梗render_frames > 0`閵嗕梗video_frame_queue_peak_size > 0`閿涘苯鑻熺涵顔款吇 `video_copy_back_frames == 0`閵?
- 閸涙垝鎶ら張顒冮煩閺€瑙勫灇 probe 瀵繒鈥栭柅鈧崙鐚寸礉闁灝鍘ょ悮顐㈢秼閸?soft decode blocker 娑撳娈?`stop/close` 閹稿倹顒撮妴?
- 閺堫剚婧€閺堚偓閺備即鐛欑拠浣虹波閺嬫粣绱?
  - `open_ok=true / entered_playback_loop=true / renderer_backend=SoftwareSDL / decoder_backend=Software`

  - `demux_video_packets=163 / demux_queue_drop_packets=0`

  - `decode_video_ok=0 / scheduler_video_decoded_frames=0 / render_frames=0 / video_frame_queue_peak_size=0`

  - `video_copy_back_frames=0 / video_swscale_frames=0 / result=FAIL`

- 缂佹捁顔戦弴瀛樻煀閿涙瓬locker 瀹歌弓绮犻垾娓嘺llback 閺勵垰鎯佹潻妯烘躬 copy-back閳ユ繆绻樻稉鈧銉︽暪閺佹稐璐熼垾婊冪秼閸撳秴浼愮粙瀣畱鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖滈幒銉ュ弳闁剧偓鐗撮張顒佺梾閺堝鑸伴幋鎰潒妫版垵鎶氭禍褍鍤垾婵撶幢閸︺劌鐣犳穱顔笺偨娑斿澧犻敍灞肩瑝鎼存棃鍣搁弬浼寸帛鐠併倕鎯庨悽?`software-first`閵?




### 2026-03-19 閺囧瓨鏌婇敍姘寵閸?`SoftwareSDL` automatic software-first閿涘奔绻氶悾?copy-back fallback 楠炴儼藟鏉烆垵袙闂冭顢ｇ拠濠冩焽



- 閺堫剝鐤嗘宀冪槈绾喛顓婚敍姘虫閻掑灈鈧笩ystem-memory renderer 娴兼ê鍘涢柆鍨帳 copy-back閳ユ繆绻栨稉顏呮煙閸氭垳绗岄幋鎰暃閹绢厽鏂侀崳銊︹偓婵婄熅娑撯偓閼疯揪绱濇担鍡楃秼閸撳秴浼愮粙瀣畱 FFmpeg 鏉烆垯娆㈢憴鍡涱暥鐟欙絿鐖滈幒銉ュ弳娴犲秳绗夌粙鍐茬暰閿涘奔绗夐懗鐣屾纯閹恒儲濡?`SoftwareSDL` 姒涙顓婚崚鍥у煂 `software-first`閵?
- 瀹稿弶鎸欓崶鐐跺殰閸?renderer-aware `software-first` decoder 闁插秵鏌婇幒鎺戠碍閿涘本浠径?`D3D11VA -> Software` 姒涙顓绘い鍝勭碍閿涘矂浼╅崗宥嗗Ω fallback 娑撶粯绁︾粙瀣敨鏉?0 鐢嗙翻閸戝搫娲栬ぐ鎺嬧偓?
- 瀹歌尪藟閸忓懓钂嬫禒鎯邦潒妫版垼袙閻椒缍嗘０鎴ｇ槚閺傤叏绱版＃鏍﹂嚋 `send_packet` 閹恒垽鎷￠妴涓桭mpeg 闁挎瑨顕ら惍浣哥摟缁楋缚瑕嗛妴涔籺all 娑撳﹣绗呴弬鍥ㄦ）韫囨绱濋崥搴ｇ敾閸欘垳鎴风紒顓炲礋閻欘剙鐣炬担宥堣拫娴犳儼顫嬫０鎴Ｐ掗惍?blocker閵?
- 瑜版挸澧犻柌宥嗘煀妤犲矁鐦夌紒鎾寸亯閿?
  - 姒涙顓?`D3D11 + D3D11VA` 娑撳鎽兼禒宥勭箽閹?`video_copy_back_ratio_percent=0 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - `SoftwareSDL` fallback 瀹稿弶浠径宥勮礋 `D3D11VA copy-back` 鐠侯垰绶為敍灞界秼閸撳秵鐗遍張顒€鐤勫ù?`video_copy_back_ratio_percent=33.5958 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - 瀵搫鍩?`D3D11 + Software decode` 閺冩湹绮涢崣顖氼槻閻滄媽钂嬫禒鎯邦潒妫版垼袙閻線鎽奸梼璇差敚閿涘苯娲滃?`software-first` 閺嗗倷绗夐柅鍌氭値闁插秵鏌婃妯款吇閸氼垳鏁?




### 2026-03-19 閺囧瓨鏌婇敍娆皍dio-master lateness 閺€鍓佹彛娑?SoftwareSDL 閸戝繑瀚圭拹婵囨箒闂勬劙鍣搁弸?


- `IVideoRenderer` 閻滄澘鍑￠弨顖涘瘮 `supportsDirectFrameFormat()`閿涙矖SdlVideoRenderer` 娴兼氨娲块幒銉ワ紣閺勫孩鏁幐?`YUV420P / NV12`閵?
- `PlayerCore::prepareVideoOutputFrame()` 閸︺劍妫ょ憴鍡涱暥濠娿倝鏆呴弮璺哄帒鐠?copy-back 閸氬海娈戞潪顖欐鐢呮纯閹恒儰姘︾紒?`SoftwareSDL`閿涘奔绗夐崘宥呭繁閸?`swscale -> YUV420P`閵?
- `Display` 閻滄澘鍑￠弨顖涘瘮 `SDL_PIXELFORMAT_NV12 + SDL_UpdateNVTexture()`閿涘苯鑻熸导妯哄帥閹镐焦婀?`AVFrame` 瀵洜鏁ら敍娑欘劀 stride 閻?`YUV420P/NV12` 鐠侯垰绶為崣顖炰缉閸忓秵妯夌粈鍝勭湴濞ｈ鲸瀚圭拹婵勨偓?
- `Scheduler::pumpRenderOnce()` 閻?`Audio` master 闁槒绶鍙夋暭閹存劕鍨庡▓鐢电搼瀵板懌鈧礁濮╅幀?late-drop 闂冨牆鈧厧鎷伴張鈧亸?sleep 闁插繐鐡欓敍宀勪缉閸忓秮鈧粌褰ч惈鈥茬濞嗏€虫皑閹绘劕澧?render閳ユ繀浜掗崣濠佸悏韫囨瑧鐡戦妴?
- 閺堫剚婧€妤犲矁鐦夌紒鎾寸亯閿?
  - 姒涙顓?`D3D11 + D3D11VA` 娑撳鎽兼禒宥勮礋 `video_copy_back_ratio_percent=0 / video_swscale_ratio_percent=0 / display_copy_ratio_percent=0`

  - 瀵搫鍩?`SoftwareSDL` 閸氬函绱漙video_swscale_ratio_percent=0`閵嗕梗display_copy_ratio_percent=0`閿涘瞼鍎归悙鐟板嚒閺€鑸垫殐閸?`video_copy_back_ratio_percent=46.4067`

  - `SDL_AUDIODRIVER=dummy` 娑?`Audio` master 鐠侯垰绶炲鑼剁獓闁熬绱漙scheduler_wait_events=274`閿涘奔绗夐崘宥呭毉閻滄澘绱撶敮鎼佺彯妫版垵绻栫粵?


### 2026-03-19 閺囧瓨鏌婇敍姝媜ftwareSDL 閹风柉绀夐柧鎹愮熅闁插繐瀵查妴涓糲heduler 闁插秴鎯庢０鍕暬娑?renderer override



- `Display::copyFrameData()` 閻滄澘鍑￠崗宄邦槵 `frames / bytes / time_us_total / time_us_max` 缂佺喕顓搁敍灞借嫙闁俺绻?`RendererDiagnostics + DiagnosticsSnapshot + --performance-log-check` 鏉堟挸鍤潪顖欐閺勫墽銇氶柧鍓ф畱閻喎鐤勯幋鎰拱閵?
- `Scheduler` worker 闁插秴鎯庣粵鏍殣瀹歌弓绮犻崶鍝勭暰濞嗏剝鏆熼弨瑙勫灇閳?0s 缁愭褰涢崘鍛付婢?4 濞?+ 100ms 閸愬嘲宓堥垾婵撶礉楠炶埖鏌婃晶?`scheduler_*_restart_limit_hits` 鐠囧﹥鏌囬妴?
- `RendererFactory` 閺傛澘顤?`MVP_RENDERER_BACKEND` override閿涘苯鑻熼崷?Windows 娑撳鏁幐?`MVP_D3D11_DRIVER_HINT=software -> SoftwareSDL`閿涘畭--renderer-fallback-check` 瑜版挸澧犲鍙変划婢跺秹鈧俺绻冮妴?
- 閺堫剚婧€ 4K60 瀵搫鍩?`SoftwareSDL` 闁插洦鐗遍弰鍓с仛閿涙瓪video_copy_back_ratio_percent=30.1018`閵嗕梗video_swscale_ratio_percent=18.6623`閵嗕梗display_copy_ratio_percent=21.8407`閿涙稖顕╅弰搴よ拫娴犺泛娲栭柅鈧柧鎯у嚒缂佸繑妲?copy-back閵嗕够wscale閵嗕龚isplay memcpy 閻ㄥ嫬褰旈崝鐘靛劰閻愬箍鈧?
- 姒涙顓?`D3D11 + D3D11VA` 娑撳鎽奸柌宥嗘煀妤犲矁鐦夐崥搴濈矝娣囨繃瀵?`video_copy_back_ratio_percent=0`閵嗕梗video_swscale_ratio_percent=0`閵嗕梗display_copy_ratio_percent=0`閿涘瘖ero-copy 缂佹捁顔戞稉宥呭綁閵?


### 2026-03-19 閺囧瓨鏌婇敍姘剁彯閻胶宸?4K 闂冪喎鍨€瑰綊鍣洪妴浣藉殰闁倸绨查懞鍌涚ウ娑?copy-back 鐠囧﹥鏌囨晶鐐插繁



- `FrameQueue` 瀹稿弶鏌婃晶?`peak_size / push_timeout_count` 缂佺喕顓搁敍瀹岲iagnosticsSnapshot` 娑?`--performance-log-check` 娴兼艾鎮撳銉ㄧ翻閸?frame queue 閻?`capacity / peak / timeout`閵?
- `PlayerCore::open()` 閻滄澘婀幐澶婄崯娴ｆ挸鐫橀幀褔鍘ょ純顔款潒妫?闂婃娊顣?frame queue 鐎瑰綊鍣洪敍灞借嫙閸?`D3D11VA` 閹垫挸绱戦崜宥堫啎缂?`extra_hw_frames`閿涘矂浼╅崗?4K native path 閹垫挾鍨?FFmpeg 閻ㄥ嫮鈥栨禒璺烘姎濮圭姰鈧?
- `Scheduler` 瀹稿弶濡?video/audio 閼冲苯甯囬弨閫涜礋鏉╃喐绮搁梼鍫濃偓纭风礉楠炶埖鏌婃晶?`video/audio_backpressure_wait_ms` 缂佺喕顓搁敍娌梡umpRenderOnce()` 閸氬本妞傛穱顔筋劀娴?`Video` master 閻?wall-clock pacing 閸?late-frame catch-up閵?
- 閺堚偓閺?4K 閹嗗厴闁插洦鐗遍弰鍓с仛閿涙瓪renderer_backend=D3D11`閵嗕梗decoder_backend=D3D11VA`閵嗕梗video_native_output_frames=101`閵嗕梗video_copy_back_frames=0`閵嗕梗video_swscale_frames=0`閿涘矁顕╅弰搴＄秼閸撳秳瀵岄柧鍙ョ矝娴?native zero-copy 娑撹桨瀵岄敍灞肩瑝閺?copy-back 閻戭厾鍋ｉ妴?
- 瀹告煡鍣搁弬浼寸崣鐠囦緤绱癭MSBuild`閵嗕梗--performance-log-check`閵嗕梗--high-bitrate-check`閵嗕梗--4k-playback-check`閵嗕梗--long-playback-check` 瑜版挸澧犻崸鍥偓姘崇箖閵?


### 2026-03-19 閺囧瓨鏌婇敍?K backend session 鐎涙劘绻樼粙瀣偓鈧崙楦跨熅瀵板嫪鎱ㄦ径?


- `--windows-backend-session-check` 瀹歌弓绮犻垾婊冾槻閻劌鐖剁憴鍕尡閺€鎯ф珤闁偓閸戠儤鏁圭亸閿偓婵囨暭娑撹　鈧粈绔村▎鈩冣偓?probe 鐎涙劘绻樼粙瀣р偓婵婄熅瀵板嫸绱伴幍鎾冲祪缂佹挻鐎崠鏍波閺嬫粌鎮楅弰鎯х础 flush閿涘苯鑻熼崷?Windows 娑撳娲块幒?`TerminateProcess(GetCurrentProcess(), code)` 闁偓閸戞亽鈧?
- 鏉╂瑦顐兼穱顔碱槻闁藉牆顕惃鍕Ц閸ョ偛缍?harness閿涘奔绗夐弰顖欏瘜閹绢厽鏂侀柧鎹愮箥鐞涘本妞傞柅鏄忕帆閿涙稓娲伴弽鍥ㄦЦ濞戝牓娅?`hard` 濡€崇础閹垫挸宓?PASS 閸氬氦绉撮弮韬测偓涔oft` 濡€崇础閹垫挸宓?PASS 閸氬骸绱撶敮鎼佲偓鈧崙铏规畱濞堝鏆€婢惰精瑙﹂妴?
- 瀹告煡鍣搁弬浼寸崣鐠囦緤绱癭hard/soft --windows-backend-session-check` 闁偓閸戣櫣鐖滈崸鍥﹁礋 `0`閿涘畭--windows-backend-check` 娑?`--4k-playback-check` 瑜版挸澧犻崸鍥у嚒閹垹顦查柅姘崇箖閵?


### 2026-03-19 閺囧瓨鏌婇敍姘剁叾妫版垼顔曟径鍥с亼鐠愩儲妞傞惃鍕潒妫?only闂勫秶楠囨稉搴℃礀瑜版帡妫粋浣虹眰閸?


- `PlayerCore::open()` 閻滄澘婀幎濠囩叾妫版垼顔曟径鍥с亼鐠愩儱鍨庨幋鎰⒈缁紮绱扮憴鍡涱暥閺傚洣娆㈡导姘舵缁狙傝礋 `video-only` 缂佈呯敾閹绢厽鏂侀敍宀勭叾妫?only 閺傚洣娆㈡禒宥囧姧閻╁瓨甯存径杈Е閿涘矂浼╅崗宥佲偓婊勫ⅵ瀵偓閹存劕濮涙担鍡樼梾閺堝鎹㈡担鏇炲讲閹绢厽鏂佹潏鎾冲毉閳ユ繄娈戞导顏呭灇閸旂喆鈧?
- 閺冪娀鐓舵０鎴ｇ翻閸戣桨绲鹃張澶庮潒妫版垶绁﹂弮璁圭礉娑撶粯妞傞柦鐔剁矤 `System` 閺€閫涜礋 `Video`閿涘矁顔€娴ｅ秶鐤嗛幒銊ㄧ箻鐠虹喖娈㈢€圭偤妾〒鍙夌厠 PTS閿涘矁鈧奔绗夐弰顖滈兇缂佺喐妞傞柦鐔跺強缁犳ぜ鈧?
- `DiagnosticsSnapshot` 閺傛澘顤?`audio_output_initialized / video_only_fallback / clock_source`閿涘畭--performance-log-check`閵嗕梗--1080p60-check`閵嗕梗--4k-playback-check`閵嗕梗--high-bitrate-check`閵嗕梗--long-playback-check` 娴兼艾鎮撳銉ㄧ翻閸戦缚绻栨禍娑氬Ц閹降鈧?
- `1080p60/high-bitrate/long-playback` 娑撳閲滈崶鐐茬秺闂傘劎顩﹂悳鏉垮嚒閺€閫涜礋閸欘亞婀?`demux_queue_drop_packets`閿涘奔绗夐崘宥嗗Ω闂婃娊顣剁粋浣烘暏閸︾儤娅欐稉瀣畱 `demux_ignored_packets` 鐠囶垰鍨介幋鎰扮彯閻胶宸奸崶鐐插竾婢惰精瑙﹂妴?
- 瀹告煡鍣搁弬鐗堝⒔鐞涘矉绱?
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`

  娴犮儱寮?`--1080p60-check`閵嗕梗--high-bitrate-check`閵嗕梗--long-playback-check`閵嗕梗--performance-log-check`閿涘苯缍嬮崜宥呮綆闁俺绻冮敍娌?-4k-playback-check` 娴犲秴褰ч崜?`fallback_ok` 鐎涙劘绻樼粙瀣熅瀵板嫬绶熺紒褏鐢婚幒鎺撶叀閵?




### 2026-03-19 閺囧瓨鏌婇敍姘尡閺€楣冩懠鐠囧﹥鏌囬崚鍡楃湴娑?decoder drain / scheduler 鐎瑰綊鏁婄悰銉ュ繁



- `decodeVideoFrame()` / `decodeAudioFrame()` 瀹稿弶鏁兼稉鐑樺瘮缂?`receive -> send -> receive` 閻樿埖鈧焦婧€閿涘苯鑻熼崷?packet queue EOF 閸氬骸顕?codec 閸欐垿鈧?`nullptr` drain閿涘矂浼╅崗宥嗗Ω閳ユ粍娈忛弮鑸垫￥鏉堟挸鍤垾婵嗘嫲閳ユ粎婀″锝呫亼鐠愩儮鈧繃璐╅崷銊ょ鐠ф灚鈧?
- `DiagnosticsSnapshot` 閻滄澘鍑￠崠鍝勫瀻 `demux_ignored_packets / demux_queue_drop_packets`閿涘苯鑻熼弬鏉款杻 decoder `send_packet(EAGAIN)`閵嗕龚rain 濞嗏剝鏆熼妴涔ative/copy-back/swscale/filter-blocked` 鐟欏棝顣剁捄顖氱窞鐠佲剝鏆熼妴?
- `Scheduler` 瀹稿弶鏌婃晶?video/audio 閼冲苯甯囨禍瀣╂娑?video/audio/render restart 缂佺喕顓搁敍瀹篹nder thread 娑旂喓鎾奸崗銉ュ綀娣囨繃濮?worker閿涙矖--performance-log-check` 娴兼艾鎮撳銉ㄧ翻閸戦缚绻栨禍娑欐煀閹稿洦鐖ｉ妴?




### 2026-03-19 閺囧瓨鏌婇敍姝卨ayerCore 閸嬫粍鎸遍弨璺哄經娑撳氦绻嶇悰灞炬鐠佹崘顓搁崐杞版叏婢?


- `PlayerCore` 瀹歌尪藟娑?deferred stop / worker reap 鐠侯垰绶為敍瀛峅F 閼奉亜濮╅崑婊勬尡閵嗕腐ext/Previous閵嗕傅uit 缁涘鐭惧鍕瑝閸愬秴褰ч弨鍦Ц閹浇鈧矂浠愰悾?demux/audio/scheduler 閼村繒鍤庣粙瀣ㄢ偓?
- `PacketQueue` 瀹歌弓绮犻崢鐔奉潗 `AVPacket*` 閸掑洤鍩?RAII `unique_ptr` 閹碘偓閺堝娼堝Ο鈥崇€烽敍瀹籩ek / stop / close 鏉╁洨鈻兼稉顓犳畱閸撯晙缍戦崢瀣級閸栧懍绱伴梾蹇涙Е閸掓绔婚悶鍡氬殰閸斻劑鍣撮弨淇扁偓?
- `Scheduler` 瀹稿弶鏁幐浣哥磽濮濄儱浠犻張鍝勮嫙閸︺劑鍣搁崥顖氬閸ョ偞鏁瑰鏌モ偓鈧崙?worker閿涙矖Clock` 瀹歌弓鎱ㄦ径?system-clock 閻?pause/speed 閺冨爼妫块崺鍝勫櫙鏉╃偟鐢婚幀褝绱盽Demuxer::open()` 娑旂喎鍑￠崢缁樺竴闁夸礁鍞撮柌宥呭弳 `close()` 閻ㄥ嫭顒撮柨渚€顥撻梽鈹库偓?
- 瀹告煡鍣搁弬鐗堝⒔鐞涘本鏆ｅ銉р柤妤犲矁鐦夐崨鎴掓姢

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  瑜版挸澧犵紒鎾寸亯娑?0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠顖樷偓?


### 2026-03-18 閺囧瓨鏌婇敍姝丼VC warning debt 閸掑棗鐪板〒鍛倞



- Windows MSVC 閻╊喗鐖ｅ鎻掓儙閻?`/utf-8 /external:anglebrackets /external:W0`閿涘本婀伴崷?UTF-8 濠ф劖鏋冩禒鏈电瑝閸愬秷袝閸?`C4819`閿涘瞼顑囨稉澶嬫煙 angle-bracket 婢跺瓨鏋冩禒?warning 娑旂喕顫﹂梾鏃傤瀲閸掓澘顦婚柈銊ョ湴閵?
- 閺堫剙婀?`logger.cpp` 閻?`getenv`閵嗕礁鐡ч獮鏇⌒掗弸鎰珤閻?`sscanf`閵嗕梗main.cpp` 閻ㄥ嫭婀担璺ㄦ暏閸欏倹鏆熼敍灞间簰閸?`player_core.cpp` 閻ㄥ嫭娼禒鎯扮ゴ閸?warning 閸у洤鍑″〒鍛倞閵?
- 瀹告煡鍣搁弬鐗堝⒔鐞涘本鏆ｅ銉р柤妤犲矁鐦夐崨鎴掓姢

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  瑜版挸澧犵紒鎾寸亯娑?`0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?
- 鏉╂瑨鐤嗛崣妯绘纯娑撳秵澧跨仦鏇熸尡閺€鎯ф珤閸旂喕鍏橀棃顫礉閻╊喗鐖ｆ禒鍛礋閹垹顦?Windows CI 閻ㄥ嫪缍嗛崳顏勶紣閺嬪嫬缂撻崺铏瑰殠閵?


### 2026-03-18 閺囧瓨鏌婇敍娆癝S 閺嶅洨顒风憴锝嗙€芥稉?UTF-16 閼煎啫娲挎穱顔筋劀



- `ASS/SSA` override 鐟欙絾鐎藉韫叏婢?`\fnArial`閵嗕梗\rDefault` 缁涘鎻ｉ崙鎴濆晸濞夋洜娈戦弽鍥╊劮鐠囧棗鍩嗛柨娆掝嚖閿涘苯鐖剁憴浣哥摟娴ｆ挸鍨忛幑銏犳嫲閺嶅嘲绱￠柌宥囩枂閻滄澘婀导姘瘻妫板嫭婀℃潻娑樺弳閸樼喓鏁?D3D11 鐎涙绠烽柧淇扁偓?
- `SubtitleTextRun.start/length` 娑?`D3D11VideoRenderer` 閻?DirectWrite `DWRITE_TEXT_RANGE` 瀹歌尙绮烘稉鈧稉?UTF-16 code unit 鐠囶厺绠熼敍瀹攎oji閵嗕焦澧跨仦鏇炵摟缁楋箑鎷伴崗鏈电铂闂?BMP 閺傚洦婀伴惃鍕壉瀵繗瀵栭崶缈犵瑝閸愬秹鏁婃担宥冣偓?
- 瀹告煡鍣搁弬鐗堝⒔鐞涘本鏆ｅ銉р柤妤犲矁鐦夐崨鎴掓姢

  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.vcxproj /t:Rebuild /p:Configuration=Debug /p:Platform=x64 /m`

  瑜版挸澧犵紒鎾寸亯娑?`167 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰敍娉坅rning 娑撴槒顩﹂弶銉ㄥ殰缁楊兛绗侀弬鐟般仈閺傚洣娆㈤敍鍦楩mpeg / Quill閿涘鈧線銆嶉惄顔煎敶婢舵艾顦╁┃鎰垳閻?C4819 缂傛牜鐖滈崨濠咁劅閿涘奔浜掗崣濠傜毌闁插繑妫﹂張澶屾畱 C4996 / C4100 閹绘劗銇氶妴?
- 閺堫剝鐤嗛崜鈺€缍戠悰銉ょ娑撳秴鍟€閹碘晛鐫嶉崝鐔诲厴闂堫澁绱濋崣顏呮暪閺佹稑甯悽?D3D11 鐎涙绠烽柧鍓ф畱鐠囶厺绠熷锝団€橀幀褌绗屾禍銈勭帛鐠佹澘缍嶉妴?
### 2026-03-18 閺囧瓨鏌婇敍娆?D11 閸樼喓鏁撳〒鍙夌厠闁惧彞绗?ASS/SSA 鐎涙绠烽柧鐐暪閸?


- Windows 姒涙顓?`D3D11` renderer 閻滄澘婀崗宄邦槵閻欘剛鐝涢惃?`window + device + swap chain + native video + native subtitle overlay` 娑撳鎽奸妴?
- 閸︺劍婀崥顖滄暏鐟欏棝顣跺銈夋殔娑撴梹鐗稿蹇撳綀閺€顖涘瘮閺冭绱漙PlayerCore` 娴兼矮绻氶悾?`AV_PIX_FMT_D3D11` 閸樼喓鏁撶涵顑挎鐢呮纯闁艾鍩?renderer閿涘苯鑻熼崥灞炬閸氭垶瑕嗛弻鎾虫珤閹恒劑鈧礁顦块弶鈥崇秼閸撳秵绺哄ú璇茬摟楠炴洖顕挒掳鈧?
- 婢舵牗瀵曠€涙绠烽柧鎯у嚒娴犲簶鈧粈绮?SRT / 缁绢垱鏋冮張顑解偓婵囧腹鏉╂稑鍩岄垾?ass/.ssa/.srt + 缂佹挻鐎崠鏍х摟楠炴洖顕挒?+ 閸樼喓鏁?D3D11 閺嶅嘲绱＄紒妯哄煑閳ユ繐绱盽main.cpp` 閼奉亜濮╅幒銏＄ゴ妞ゅ搫绨稉?`.ass -> .ssa -> .srt`閵?
- `D3D11VideoRenderer` 閻滄澘婀崷銊ユ倱娑撯偓閸?swap chain backbuffer 娑撳﹤鐣幋?ASS/SSA 鐢摜鏁ら弽宄扮础鐎涙绠风紒妯哄煑閿涘矁顩惄鏍э綖閸忓懌鈧焦寮挎潏骞库偓渚€妲捐ぐ渚库偓浣藉剹閺咁垱顢嬮妴浣割嚠姒绘劑鈧礁鐣炬担宥呮嫲 run 缁狙冪摟娴ｆ挻鐗卞蹇ョ幢闂?D3D11 濞撳弶鐓嬮崳銊╃帛鐠併倝鈧偓閸栨牔璐熺痪顖涙瀮閺堫剚妯夌粈鎭掆偓?
- 瀹稿弶绔婚悶鍡楀弿鐏炩偓閺嬪嫬缂撻梼璇差敚閺傚洣娆㈡稉顓犳畱缂傛牜鐖滅拠顖濐嚢闂傤噣顣介敍灞界暚閺佺袙閸愯櫕鏌熷鍫ョ崣鐠囦礁鎳℃禒?
  `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" build\modern-video-player.sln /t:modern-video-player /p:Configuration=Debug /p:Platform=x64 /m`

  瑜版挸澧犵紒鎾寸亯娑?`0 娑擃亣顒熼崨?/ 0 娑擃亪鏁婄拠鐥愰妴?
- 瑜版挸澧犻梽鎰煑娴犲秶鍔х€涙ê婀敍姘崇箹娑撳秵妲哥€瑰本鏆ｉ惃?libass 缁涘鐜€圭偟骞囬敍灞界毣閺堫亣藟閸忓懐婀＄€?`.ass` 閺嶉攱婀伴惃鍕箥鐞涘本妞傜憴鍡氼潕妤犲本鏁归敍娑滎嚊鐟?`docs/design/D3D11_NATIVE_RENDER_CHAIN_2026-03-18.md` 娑?`docs/records/CHANGELOG.md` 閻ㄥ嫰妫舵０?66閵?
---

## 闂冭埖顔屾稉鈧敍姘唨绾偓閹绢厽鏂侀崳顭掔礄閸樺棗褰剁挧椋庡仯閿?


### 瀵偓婵妫╅張?
2026-02-17



### 闂冭埖顔岄惄顔界垼

鐎圭偟骞囬崺铏诡攨閻ㄥ嫯顫嬫０鎴炴尡閺€鎯у閼虫枻绱濋崠鍛閿?
- 鐟欏棝顣剁憴锝囩垳閸滃本妯夌粈?
- 闂婃娊顣剁憴锝囩垳閸滃本鎸遍弨?
- 閸╃儤婀伴幘顓熸杹閹貉冨煑



### 鐎瑰本鍨氶悩鑸碘偓?
- [x] 妞ゅ湱娲扮紒鎾寸€幖顓炵紦

- [x] FFmpeg 8.0.1 闂嗗棙鍨?
- [x] SDL2 2.30.11 闂嗗棙鍨?
- [x] 閺冦儱绻旂化鑽ょ埠閿涘湨uill 閸欏矂鈧岸浜?+ Fallback閿?
- [x] 鐟欏棝顣剁憴锝囩垳濡€虫健

- [x] 闂婃娊顣剁憴锝囩垳濡€虫健

- [x] 鐟欏棝顣堕弰鍓с仛濡€虫健

- [x] 闂婃娊顣堕幘顓熸杹濡€虫健

- [x] 娑撶粯鎸遍弨鎯ф珤闁槒绶?
- [x] 閻楀牊婀伴崗鐓庮啇閹傛叏婢?
- [x] 鐟欏棝顣跺ù浣哄偍瀵洑绗夐崠褰掑帳闂傤噣顣芥穱顔碱槻



**鐠囧瓨妲?*:

- 閺堫剝濡拋鏉跨秿閻ㄥ嫭妲?2026-02-17 ~ 2026-02-25 閻ㄥ嫭妫張鐔风杽閻滄澘鐔€缁捐￥鈧?
- 瑜版挻妞傛担璺ㄦ暏閻ㄥ嫭妲搁弮褏澧?decoder / playLoop 閺嬭埖鐎敍娑氭祲閸忚櫕鏋冩禒璺哄嚒閸?2026-03-06 閺嬭埖鐎弨鑸垫殐閸氬海些闂勩倖鍨ㄩ獮璺哄弳 `PlayerCore + Scheduler + core/*` 娑撳鎽奸妴?
- 娑撴椽浼╅崗宥堫嚖鐎电》绱濇稉瀣牚閸樺棗褰剁拠瀛樻娣囨繄鏆€閳ユ粏鍏橀崝娑楃瑢闂傤噣顣介垾婵囨拱闊偓绱濇稉宥呭晙閹跺﹤鍑￠崚鐘绘珟閺傚洣娆㈣ぐ鎾茬稊瑜版挸澧犳禒鎾崇氨缂佹挻鐎拠瀛樻閵?


### 閻楀牊婀伴崗鐓庮啇閹傛叏閺€?


#### FFmpeg 8.0.1 閸忕厧顔愰幀?
- 娣囶喖顦?`codec_ctx_->avctx->priv_data` 閸?FFmpeg 8.0 娑擃厺绗夐崣顖滄暏閻ㄥ嫰妫舵０?
- 鐠囥儰鎱ㄦ径宥呭絺閻㈢喎婀弮褏澧楅悪顒傜彌鐟欏棝顣?闂婃娊顣剁憴锝囩垳閸ｃ劌鐤勯悳棰佽厬閿?
  - 閸︺劏袙閻礁娅掔€电钖勯崘鍛▔瀵繋绻氱€?`format context`閿?
  - 娑撳秴鍟€娓氭繆绂嗘禒?`codec context` 閸欏秴鎮滅拠璇插絿閸愬懘鍎寸€涙顔岄妴?
- 閻╃鍙х€圭偟骞囬崥搴㈡降瀹告彃鑻熼崗銉у箛鐞涘矁袙鐏忎浇顥?鐟欙絿鐖滄稉濠氭懠閿涘本妫弬鍥︽閸氬秴鍑℃稉宥呭晙娣囨繄鏆€閵?
#### 閺冦儱绻旂化鑽ょ埠閺囧瓨鏌婇敍鍫滅磼娑撴氨楠?Quill 缁狅繝浜鹃敍?
- 闁插秵鏌婇崥顖滄暏 Quill閿涘本鐎?ConsoleSink + RotatingFileSink 瀵倹顒為弮銉ョ箶闁岸浜鹃敍灞借嫙娣囨繃瀵?stdout/stderr 婢跺洦褰洪妴?
- `config/logging.conf` 娑?`MVP_LOG_*` 閻滎垰顣ㄩ崣姗€鍣洪弨顖涘瘮鏉╂劘顢戦弮鎯扮殶閺?`log_dir`閵嗕浇鐤嗘潪顒€銇囩亸?閺佷即鍣洪崣濠冩）韫囨鐡戠痪褋鈧?
- 閸掓繂顫愰崠鏍с亼鐠愩儲鍨ㄩ惄顔肩秿娑撳秴褰查崘娆愭閼奉亜濮╅崨濠咁劅楠炲爼妾风痪褝绱辩€瑰繑甯撮崣锝勭瑢 `DEBUG_MODE` 鐞涘奔璐熸穱婵囧瘮閸忕厧顔愰妴?
- 娣囶喗鏁奸弬鍥︽閿涙瓪include/logger.h`閵嗕梗src/logger.cpp`閵嗕梗config/logging.conf`閵嗕梗docs/design/LOGGING.md`閵嗕梗docs/records/CHANGELOG.md`閵嗕梗docs/records/VERSION.md`

- 缂傛牞鐦ф潏鎾冲毉缁€杞扮伐閿涙瓪Quill: enabled`閿涘矁绻嶇悰灞炬埂閺冦儱绻旈崘娆忓弳 `logs/modern-video-player.log`



#### 婢舵氨鍤庣粙瀣尡閺€鐐仸閺嬪嫰鍣搁弸?(2026-02-25)

- 鐎圭偟骞囬悪顒傜彌鐟欏棝顣剁憴锝囩垳缁捐法鈻奸敍鍦磇deoDecodeThread閿?
- 鐎圭偟骞囬悪顒傜彌闂婃娊顣剁憴锝囩垳缁捐法鈻奸敍鍦搖dioDecodeThread閿?
- 鐎圭偟骞囩痪璺ㄢ柤鐎瑰鍙忕敮褔妲﹂崚妤嬬礄FrameQueue閿涘绱濋弨顖涘瘮閺夆€叉閸欐﹢鍣虹粵澶婄窡

- 鐎圭偟骞囬棅瀹狀潒妫版垵鎮撳銉ь吀閻炲棗娅掗敍鍦珁ncManager閿涘绱濋弨顖涘瘮 AudioMaster/VideoMaster/Free 娑撳顫掑Ο鈥崇础

- 闁插秵鐎?VideoPlayer閿涘奔绮犻崡鏇犲殠缁?playLoop 閺€閫涜礋婢舵氨鍤庣粙?renderLoop

- 鏉╂瑤绨洪懗钘夊閸氬孩娼靛▽澶嬬┅娑撳搫缍嬮崜宥勭波鎼存挷鑵戦惃?`core` 濡€虫健閿?
  - `include/core/frame_queue.h`

  - `include/core/decoder_thread.h`

  - `include/core/clock.h`

  - `include/core/scheduler.h`

  - `include/core/player_core.h`

- 閺冄呭缁捐法鈻?閸氬本顒為弬鍥︽閸氬秳绮庢禒锝堛€冮弮鈺傛埂鐎圭偟骞囬梼鑸殿唽閿涘瞼骞囧韫瑝閸愬秳缍旀稉鍝勭秼閸撳秵鏋冩禒鍓佺波閺嬪嫬鐡ㄩ崷銊ｂ偓?
#### 闂婃娊顣堕幘顓熸杹閺嬭埖鐎穱顔碱槻 (2026-02-25)

- 娣囶喖顦查棅鎶筋暥閹绢厽鏂侀弬顓熸焽缂侇厾鐢婚惃鍕６妫?
- AudioDecodeThread 鐟欙絿鐖滈崥搴ｆ纯閹恒儴鐨熼悽?AudioPlayer::play() 鐏忓棙鏆熼幑顔芥杹閸?SDL 闂冪喎鍨?
- 绾喕绻?SDL 闂婃娊顣堕崶鐐剁殶閼宠姤瀵旂紒顓″箯閸欐牗鏆熼幑?
- 閻╃鍙ч柅鏄忕帆閸氬孩娼靛鎻掕嫙閸忋儱缍嬮崜宥夌叾妫版垶绉风拹鍦殠缁嬪鈧梗AudioPlayer` 娑?`PlayerCore` 娑撳鎽奸敍灞肩瑝閸愬秴顕惔鏃傚缁斿妫弬鍥︽閸氬秲鈧?
#### Frame 缁夎濮╃拠顓濈疅娣囶喖顦?(2026-02-25)

- 娣囶喖顦?VideoFrame 閸?AudioFrame 缁崵宸辩亸鎴犘╅崝銊嚔娑斿顕遍懛瀵告畱瀹曗晜绨濋梻顕€顣?
- 娑?VideoFrame 濞ｈ濮炵粔璇插З閺嬪嫰鈧姴鍤遍弫鏉挎嫲缁夎濮╃挧瀣偓鑹扮箥缁犳顑?
- 娑?AudioFrame 濞ｈ濮炵粔璇插З閺嬪嫰鈧姴鍤遍弫鏉挎嫲缁夎濮╃挧瀣偓鑹扮箥缁犳顑?
- 閺勬儳绱￠崚鐘绘珟閹风柉绀夐弸鍕偓鐘插毐閺佹澘鎷伴幏鐤鐠у鈧壈绻嶇粻妤冾儊閿涘矂妲诲銏＄ガ閹风柉绀?
- 瑜版挸澧犵粵澶夌幆閼宠棄濮忔担宥勭艾閿?
  - `include/core/frame.h`

  - `src/core/frame.cpp`

#### 婢舵俺袙閻礁娅掔€圭偘绶ョ粩鐐扮挨鐠囪褰囨穱顔碱槻 (2026-02-25)

- 娣囶喖顦查幘顓熸杹閺冭泛顦挎稉顏囆掗惍浣告珤缁旂偘绨ょ拠璇插絿閸氬奔绔?AVFormatContext 鐎佃壈鍤ч惃鍕掗惍渚€鏁婄拠?
- 閸︺劍妫張鐔风杽閻滈鑵戦柅姘崇箖闁插秶鐤嗛弮褏澧楃憴锝囩垳閸ｃ劎濮搁幀渚€浼╅崗宥囩彽娴滃绱遍崥搴ｇ敾瀹稿弶鏁兼稉铏圭埠娑撯偓 `Demuxer + PlayerCore + Scheduler` 娑撳鎽?
- 娣囶喗鏁奸弬鍥︽閿?
  - `src/video_player.cpp`

### 瀹歌尙鐓￠梻顕€顣?
- 闂婂疇顫嬫０鎴濇倱濮濄儰濞囬悽銊х暆閸楁洜娈戦弮鍫曟？鐠侊紕鐣婚敍宀勭彯闁喐鍨ㄦ担搴ㄢ偓鐔告尡閺€鐐閸欘垵鍏橀張澶婃倱濮濄儵妫舵０?
- 娴犲懏鏁幐?YUV420P 閺嶇厧绱￠惃鍕潒妫?
- seek 閸旂喕鍏橀棁鈧憰浣界箻娑撯偓濮濄儱鐣崰?


### 闂傤噣顣芥穱顔碱槻鐠佹澘缍?


#### 鐟欏棝顣跺ù浣哄偍瀵洑绗夐崠褰掑帳闂傤噣顣?(2026-02-24)



**闂傤噣顣介幓蹇氬牚**:

- 閹绢厽鏂?mp4 閺傚洣娆㈤弮璁圭礉鐟欏棝顣堕弮鐘崇《濮濓絽鐖堕弰鍓с仛

- 閺冦儱绻旈弰鍓с仛 `stream_index=0, expected=1` 閸欏秴顦查崙铏瑰箛

- 缁嬪绨顏嗗箚 48 濞嗏剝澧犻懗鍊燁嚢閸掔増顒滅涵顔炬畱鐟欏棝顣剁敮?


**闂傤噣顣介崢鐔锋礈**:

- MP4 閺傚洣娆㈤惃鍕ウ妞ゅ搫绨弰顖炵叾妫版垶绁?缁便垹绱?)閸︺劏顫嬫０鎴炵ウ(缁便垹绱?)娑斿澧?
- `av_read_frame()` 鏉╂柨娲栭惃鍕瘶閸欘垵鍏橀弰顖欐崲閹板繑绁﹂惃鍕剁礄闁艾鐖堕弰顖滎儑娑撯偓娑擃亝绁?- 闂婃娊顣跺ù渚婄礆

- 閸樼喍鍞惍渚€浜ｉ崚棰佺瑝閸栧綊鍘ら惃鍕ウ閺冨墎娲块幒銉ㄧ箲閸?false閿涘苯顕遍懛纾嬵潒妫版垵鎶氶弮鐘崇《鐟欙絿鐖?


**鐟欙絽鍠呴弬瑙勵攳**:

- 娣囶喗鏁奸弮褏澧楃憴鍡涱暥鐟欙絿鐖滃顏嗗箚閻ㄥ嫯顕伴崠鍛粹偓鏄忕帆

- 鐏忓棝浜ｉ崚棰佺瑝閸栧綊鍘ゅù浣规鏉╂柨娲?false閿涘本鏁兼稉?continue 鐠哄疇绻冪拠銉ュ瘶

- 瀵邦亞骞嗙拠璇插絿閻╂潙鍩岄幍鎯у煂濮濓絿鈥樺ù浣哄偍瀵洜娈戦崠?


**娣囶喗鏁奸弬鍥︽**:

- 閺冄呭鐟欏棝顣剁憴锝囩垳閸ｃ劌鐤勯悳甯礄鐠囥儲鏋冩禒璺烘倵缂侇厼鍑￠崷銊︾仸閺嬪嫭鏁归弫娑楄厬缁夊娅庨敍?


**閺冦儱绻旂粈杞扮伐**:

```

[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=0, expected=1

[DEBUG] [VIDEO] decodeFrame: packet stream mismatch, skipping

... (闁插秴顦?48 濞?

[DEBUG] [VIDEO] decodeFrame: read packet, stream_index=1, expected=1

[DEBUG] [VIDEO] decodeFrame: success, pts=0

```



#### 闂婃娊顣跺ù浣哄偍瀵洑绗夐崠褰掑帳闂傤噣顣?(2026-02-24)



**闂傤噣顣介幓蹇氬牚**:

- 娑撳氦顫嬫０鎴炵ウ缁便垹绱╅惄绋挎倱閻ㄥ嫰妫舵０姗堢礉闂婃娊顣堕弮鐘崇《濮濓絽鐖剁憴锝囩垳



**闂傤噣顣介崢鐔锋礈**:

- 閸氬本鐗遍惃鍕６妫版﹫绱伴棅鎶筋暥閸栧懎褰查懗鎴掔瑝閺勵垳顑囨稉鈧稉顏囶潶鐠囪褰囬惃鍕ウ



**鐟欙絽鍠呴弬瑙勵攳**:

- 娣囶喗鏁奸弮褏澧楅棅鎶筋暥鐟欙絿鐖滃顏嗗箚閻ㄥ嫯顕伴崠鍛粹偓鏄忕帆

- 鎼存梻鏁ゆ稉搴ゎ潒妫版垼袙閻礁娅掗惄绋挎倱閻ㄥ嫪鎱ㄦ径?


**娣囶喗鏁奸弬鍥︽**:

- 閺冄呭闂婃娊顣剁憴锝囩垳閸ｃ劌鐤勯悳甯礄鐠囥儲鏋冩禒璺烘倵缂侇厼鍑￠崷銊︾仸閺嬪嫭鏁归弫娑楄厬缁夊娅庨敍?


#### YUV 閺佺増宓佸〒鍙夌厠闁挎瑨顕?(2026-02-24)



**闂傤噣顣介幓蹇氬牚**:

- 鐟欙絿鐖滈幋鎰閸氬海鈻兼惔蹇曠彌閸楁娊鈧偓閸戠尨绱濆▽鈩冩箒閻㈠娼伴弰鍓с仛



**闂傤噣顣介崢鐔锋礈**:

- `renderFrame` 閸戣姤鏆熸担璺ㄦ暏闁挎瑨顕ら惃?YUV 閺佺増宓?
- 閸樼喐娼垫导鐘烩偓鎺旀畱閺?`frame->data[0]`閿涘牆褰ч弰?Y 楠炴娊娼伴幐鍥嫛閿?
- 閻掕泛鎮楅柨娆掝嚖閸︽澘浜ｇ拋?Y/U/V 閺勵垵绻涚紒顓炵摠閸屻劎娈?


**鐟欙絽鍠呴弬瑙勵攳**:

1. 娴肩娀鈧帗鏆ｆ稉?AVFrame 閹稿洭鎷￠懓灞肩瑝閺?`data[0]`

2. 濮濓絿鈥樻担璺ㄦ暏 Y/U/V 楠炴娊娼伴惃鍕殶閹诡喖鎷扮悰灞姐亣鐏?


**娣囶喗鏁奸弬鍥︽**:

- `src/display.cpp`

- `src/video_player.cpp`



**娴狅絿鐖滈崣妯绘纯**:

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



### 闂冭埖顔岄崥搴ｇ敾閻╊喗鐖ｉ敍鍫濆坊閸欒尪顔囪ぐ鏇礆

- 閸氬海鐢婚惄顔界垼瀹告彃婀?2026-03-06 娑斿鎮楅惃鍕彿閼哄倷鑵戦柅鎰劄閽€钘夋勾閿涘苯瀵橀幏顒婄窗

  - 缂佺喍绔?`PlayerCore + Scheduler + Filters` 閺嬭埖鐎敍?
  - 婢舵牗瀵曠€涙绠烽妴浣规尡閺€鎯у灙鐞涖劊鈧浇顔曠純顔荤瑢韫囶偅宓庨柨顔藉复閸忋儻绱?
  - D3D11VA / D3D11 閺堚偓鐏忓繐褰查悽銊╂懠鐠侯垽绱?
  - 缁旂姾濡€佃壈鍩呴妴涓?B Repeat閵嗕焦鍩呴崶淇扁偓?


---



## 閻楀牊婀伴崢鍡楀蕉



### v1.0.0 (缁楊兛绔撮梼鑸殿唽)

- 閸掓繂顫愰悧鍫熸拱

- 閸╄桨绨?FFmpeg + SDL2 + Quill 閺嬪嫬缂?
- 閺€顖涘瘮閸╃儤婀伴惃鍕潒妫版垶鎸遍弨鎯у閼?


---



## 娓氭繆绂嗘惔鎾崇暔鐟佸懓顕╅弰?


### Windows



#### FFmpeg 8.0.1

```powershell

# 娴ｈ法鏁?vcpkg

vcpkg install ffmpeg:x64-windows



# 閹存牗澧滈崝銊ょ瑓鏉?
# 鐠佸潡妫?https://www.gyan.dev/ffmpeg/builds/

# 娑撳娴?ffmpeg-8.0.1-full_build.7z

```



#### SDL2 2.30.11

```powershell

# 娴ｈ法鏁?vcpkg

vcpkg install sdl2:x64-windows



# 閹存牗澧滈崝銊ょ瑓鏉?
# 鐠佸潡妫?https://github.com/libsdl-org/SDL/releases

# 娑撳娴?SDL2-devel-2.30.11-VC.zip

```



**濞夈劍鍓?*: 閼汇儲婀崡鏇犲鐎瑰顥?Quill閿涘苯褰茬亸鍡樼爱閻浇袙閸樺鍩?`external/quill`閿涙稐绡冮崣顖欎簰闁俺绻冮崠鍛吀閻炲棗娅掔€瑰顥婇崥搴ゎ啎缂?`QUILL_ROOT`閵?


### Linux



```bash

# 缂傛牞鐦?FFmpeg 8.0.1

./configure --prefix=/usr/local

make -j$(nproc)

sudo make install



# SDL2 闁艾鐖堕柅姘崇箖閸栧懐顓搁悶鍡楁珤鐎瑰顥?
sudo apt install libsdl2-dev

```



### macOS



```bash

# 娴ｈ法鏁?Homebrew

brew install ffmpeg

brew install sdl2

```



**濞夈劍鍓?*: Quill 娑?header-only 鎼存搫绱濋崣顖炩偓姘崇箖 `brew` 鐎瑰顥婇幋鏍纯閹恒儰绗呮潪鑺ョ爱閻礁鎮楅柊宥囩枂 `QUILL_ROOT`閵?


---



## 缂傛牞鐦ч崨鎴掓姢



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



## 閺傚洦銆傞弴瀛樻煀閺冦儱绻?


| 閺冦儲婀?| 閺囧瓨鏌婇崘鍛啇 |

|------|----------|

| 2026-02-17 | 閸掓稑缂撻悧鍫熸拱鐠佹澘缍嶉弬鍥ㄣ€傞敍宀冾唶瑜版洜顑囨稉鈧梼鑸殿唽鐎瑰本鍨氶幆鍛枌 |

| 2026-02-24 | 閺囧瓨鏌婇弮銉ョ箶缁崵绮烘稉杞扮磼娑撴氨楠?Quill 缁狅繝浜鹃敍宀冾唶瑜版洝顫嬫０鎴炵ウ缁便垹绱╅梻顕€顣芥穱顔碱槻 |

| 2026-02-24 | 鐠佹澘缍嶉棅鎶筋暥濞翠胶鍌ㄥ鏇㈡６妫版ê鎷?YUV 濞撳弶鐓嬮梻顕€顣芥穱顔碱槻 |

| 2026-02-24 | 閸掓稑缂?CHANGELOG.md 闂傤噣顣芥穱顔碱槻鐠佹澘缍嶉弬鍥ㄣ€?|

| 2026-02-25 | 鐠佹澘缍嶆径姘卞殠缁嬪鎸遍弨鐐仸閺嬪嫰鍣搁弸鍕嫲闂婃娊顣堕幘顓熸杹閺嬭埖鐎穱顔碱槻 |

| 2026-02-25 | 鐠佹澘缍?Frame 缁夎濮╃拠顓濈疅娣囶喖顦?|

| 2026-02-25 | 鐠佹澘缍嶆径姘承掗惍浣告珤鐎圭偘绶ョ粩鐐扮挨鐠囪褰囨穱顔碱槻 |

| 2026-03-06 | 娣囶喖顦茬亸蹇撶潌缁愭褰涙潻鍥с亣娑撳海鐛ラ崣锝囩級閺€鍙ョ皑娴犺泛鍚嬬€瑰綊妫舵０?|

| 2026-03-07 | 閹恒儱鍙?GitHub Actions 閼奉亜濮╅弽鐓庣础閸ョ偛缍婃稉?CI 閸忕厧顔愰弨褰掆偓?|

| 2026-03-07 | 閹恒儱鍙嗛幘顓熸杹閸掓銆冩稉濠氭懠鐠侯垬鈧浇顔曠純顔藉瘮娑斿懎瀵叉稉搴℃彥閹圭兘鏁＃鏍 |

| 2026-03-08 | 濞撳懐鎮婇崢鍡楀蕉缁旂姾濡稉顓犳畱閺冄嗙熅瀵板嫭寮挎潻甯礉闁灝鍘ょ亸鍡楀嚒缁夊娅庨弬鍥︽鐠囶垰鍟撴稉鍝勭秼閸撳秳绮ㄦ惔鎾剁波閺?|

| 2026-03-18 | 閸氬本顒?D3D11 閸樼喓鏁撳〒鍙夌厠闁剧偓娓剁紒鍫㈠Ц閹緤绱濈拋鏉跨秿 ASS/SSA 閸樼喓鏁撶€涙绠烽柧鐐暪閸欙絼绗岄崗銊ョ湰閺嬪嫬缂撻梼璇差敚濞撳懐鎮?|



---



## 2026-03-06 閺囧瓨鏌?


### Core API / Scheduler / Filter 闁插秵鐎?
- 閺傛澘顤?`core` 鐎涙劖膩閸ф绱癭PlayerCore`閵嗕梗Scheduler`閵嗕梗FrameQueue`閵嗕梗Clock`閵嗕梗Command`閵嗕梗Frame`閵?
- 閺傛澘顤?`filters` 鐎涙劖膩閸ф绱癋ilter 閹恒儱褰涢妴涔ilterRegistry`閵嗕梗FilterPipeline`閵嗕礁鍞寸純顔诲瘨鎼?鐎佃鐦惔?妤楀崬鎷版惔锔芥姢闂€婧库偓?
- 閸掓繃婀″鏇炲弳 `VideoPlayer -> PlayerCore` 闁倿鍘ょ仦鍌︾礉闂呭繐鎮楅崷銊ユ倱閺冦儱鐣幋鎰仸閺嬪嫭鏁归弫娑樿嫙缁夊娅庨弮褑鐭惧鍕瀻閸欏鈧?


### 濞村鐦稉搴㈢€?
- 瑜版挻妞傞弴鎯х穿閸忋儰澶嶉弮鑸电壋韫囧啯绁寸拠鏇熸瀮娴犺绱辨潻娆庣昂濞村鐦弬鍥︽瀹告彃婀?2026-03-07 濞撳懐鎮婇妴?
- `CMakeLists.txt` 閸︺劏顕氶梼鑸殿唽瀵偓婵鎾奸崗?`src/core/*` 娑?`src/filters/*` 娑撹崵绱拠鎴︺€嶉妴?


---



## 2026-03-06 閺嬭埖鐎弨鑸垫殐閺囧瓨鏌?


### 缂佺喍绔撮弽绋跨妇閺嬭埖鐎?
- 閸樺娅庨弮褎鎸遍弨楣冩懠鐠侯垽绱濈€圭偟骞囩紒鐔剁娑?`VideoPlayer + PlayerCore + Scheduler + Filters`閵?
- 閸掔娀娅庨弮?decoder/thread/sync/packet/legacy clock 閻╃鍙ч弬鍥︽閵?
- `CMakeLists.txt` 娴犲懍绻氶悾娆愭煀閺嬭埖鐎紓鏍槯妞ゅ箍鈧?


### 閺傚洦銆?
- 閺傛澘顤?`docs/design/ARCHITECTURE_REFACTOR_2026-03-06.md`閿涘苯瀵橀崥顐﹀櫢閺嬪嫮娲伴弽鍥モ偓浣稿灩闂勩倖膩閸фぜ鈧椒绻氶悾娆愭瀮娴犺埖绔婚崡鏇樷偓?


---



## 2026-03-06 缁愭褰涙担鎾荤崣娣囶喖顦?


### 鐏忓繐鐫嗛懛顏堚偓鍌氱安娑撳海缂夐弨鍓旂€规碍鈧?
- `Display::init()` 閸氼垰濮╅弮鑸靛瘻鐏炲繐绠烽崣顖滄暏閸栧搫鐓欓懛顏堚偓鍌氱安閸掓繂顫愮粣妤€褰涚亸鍝勵嚟閿涘牓妾洪崚璺哄煂 90%閿涘奔绻氶幐浣筋潒妫版垶鐦笟瀣剁礆閵?
- 婢х偛濮?`SDL_WINDOWEVENT_SIZE_CHANGED` 婢跺嫮鎮婇敍灞藉悑鐎归€涚瑝閸氬苯閽╅崣?妞瑰崬濮╂稉瀣畱缁愭褰涚亸鍝勵嚟閸欐ê瀵叉禍瀣╂閵?
- 濞撳弶鐓嬮梼鑸殿唽閹稿绨憴鍡涱暥濮ｆ柧绶ョ拋锛勭暬閻╊喗鐖ｉ崠鍝勭厵閿涘矂浼╅崗宥囩崶閸欙絾瀚嬮幏钘夋倵閻㈠娼伴幏澶夊嚑閵?


### 娣囶喗鏁奸弬鍥︽

- `src/display.cpp`

- `docs/records/DEVELOP_LOG.md`

- `docs/records/CHANGELOG.md`

- `docs/records/VERSION.md`



## 2026-03-07 閺囧瓨鏌?


### 閹绢厽鏂佺粙鍐茬暰閹傜瑢閸╄櫣顢呮禍銈勭鞍婢х偛宸?
- 娣囶喖顦茬粣妤€褰涢張鈧径褍瀵?缂傗晜鏂侀弮鎯邦潒妫版垹鏁鹃棃銏犲讲閼宠棄宕辨担蹇曟畱闂傤噣顣介敍鍫ョ叾妫版垳绗夋稉顓熸焽閿涘鈧?
- 閺勫墽銇氱仦鍌涙煀婢х偛鐔€绾偓閹貉冨煑閺夆槄绱欐潻娑樺閺壜扳偓渚€鐓堕柌蹇旀蒋閿涘鈧?
- 閺€顖涘瘮姒х姵鐖ｉ幏鏍уЗ鏉╂稑瀹抽弶陇绻樼悰?seek閵?
- 閺€顖涘瘮姒х姵鐖ｉ幏鏍уЗ闂婃娊鍣洪弶陇绻樼悰灞界杽閺冨爼鐓堕柌蹇氱殶閼哄倶鈧?
- 鐞涖儱鍘栭棅鎶藉櫤韫囶偅宓庨柨顕嗙窗`+/-` 娑?`閳?閳彵閵?


### 娣囶喗鏁奸弬鍥︽

- include/display.h

- src/display.cpp

- src/core/player_core.cpp

- src/main.cpp

- docs/records/DEVELOP_LOG.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md



## 2026-03-07 閺囧瓨鏌婇敍鍫滅磼娑撴氨楠囧Ο鈥虫健閹恒劏绻橀敍?


### MPC-HC 閺嬭埖鐎Ο鈥虫健閹碘晛鐫?
- 閺傛澘顤冩导浣风瑹缁狙冪唨绾偓鐠佺偓鏌﹀Ο鈥虫健閿涙瓪TaskQueue`閵嗕梗FramePool`閵嗕梗DecoderThread`閵?
- 閺傛澘顤冨〒鍙夌厠閹跺€熻杽鐏炲偊绱癭IVideoRenderer`閵嗕梗SdlVideoRenderer`閵嗕梗D3D11/OpenGL` 閸楃姳缍呯€圭偟骞囬妴涔endererFactory`閵?
- `PlayerCore` 娴犲海娲块幒銉ょ贩鐠?`Display` 鐠嬪啯鏆ｆ稉杞扮贩鐠ф牗瑕嗛弻鎾村▕鐠炩剝甯撮崣锝冣偓?
- 閺傛澘顤冮棅鎶筋暥婢х偛宸卞Ο鈥虫健閿?0 濞堥潧娼庣悰鈥虫珤閵嗕焦璐╅棅鍐叉珤閿涙矖AudioPlayer` 婢х偛濮炵紓鎾冲暱鐟欏倹绁撮幒銉ュ經閵?
- 閺傛澘顤冪憴锝囩垳閸ｃ劎顓搁悶鍡樐侀崸妤嬬窗閼宠棄濮忛幒銏＄ゴ娑撳骸鎮楃粩顖濆殰閸斻劑鈧瀚ㄩ妴?
- 閺傛澘顤冪€涙绠烽敍鍦玆T閿涘鈧焦鎸遍弨鎯у灙鐞涱煉绱橫3U8閿涘鈧浇顔曠純顔衡偓浣告彥閹圭兘鏁妴浣烘瘖閼层們鈧焦褰冩禒韬测偓浣圭壐瀵繑鏁幐浣碘偓浣圭ウ婵帊缍嬬憴锝嗙€界粵澶嬆侀崸妤咁€囬弸韬测偓?
- 閺傛澘顤冨銈夋殔閸╄櫣琚稉搴ㄧ叾鐟欏棝顣跺銈夋殔闁炬拝绱濈悰銉ュ帠闂婃娊鍣?楠炲疇銆€闂婃娊顣跺銈夋殔閵?
- 閸氬本顒為弴瀛樻煀 `.monkeycode/specs/enterprise-quill-logging/tasklist.md` 瀹告彃鐤勯悳浼淬€嶉妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-07 閺囧瓨鏌婇敍鍫熺壐瀵繗鍏橀崝娑氱叐闂冨吀绗屾妯哄瀻妤傛ê鎶氱拠鍕強閿?


### 閼宠棄濮忓Λ鈧弻銉ュ弳閸?
- 閺傛澘顤?`--capabilities` 閸涙垝鎶ら敍姘崇翻閸戦缚绻嶇悰灞炬鐎圭懓娅?鐟欏棝顣剁憴锝囩垳閸?闂婃娊顣剁憴锝囩垳閸ｃ劏鍏橀崝娑崇礉楠炲墎绮伴崙杞板瘜閸旀稒鐗稿蹇氼洬閻╂牜鐓╅梼纰夌礄PASS/PARTIAL/FAIL閿涘鈧?
- 閺傛澘顤?`--evaluate-target` 閸涙垝鎶ら敍姘瘻 `鐎?妤?FPS/婢逛即浜?閻胶宸糮 鐠囧嫪鍙婄€圭偞妞傞幘顓熸杹閸欘垵顢戦幀褌绗岀涵顒冃?D3D11瀵ら缚顔呴妴?


### 缁嬪啿浠撮幀褍顤冨?
- 娣囶喖顦?`src/streaming/dash_manifest_parser.cpp` 閸?MSVC 娑撳娈?raw-string 濮濓絽鍨紓鏍槯闂傤噣顣介敍灞句划婢跺秵鐎鍝勭唨缁捐￥鈧?
- `Demuxer` 娴ｈ法鏁?`av_find_best_stream`閿涘苯鑻熷鏇炲弳 `probesize`/`analyzeduration`/`+genpts` 闁銆嶆晶鐐插繁婢跺秵娼呮刊鎺嶇秼閹恒垺绁撮妴?
- `AudioPlayer` 閺嗘挳婀剁€圭偤妾潏鎾冲毉閸欏倹鏆熼敍娌桺layerCore` 婢跺秶鏁ら柌宥夊櫚閺嶅嘲娅掗獮鑸靛瘻鐠佹儳顦潏鎾冲毉閸欏倹鏆熼崑姘樋闂婃娊浜鹃柌宥夊櫚閺嶆灚鈧?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-07 閺囧瓨鏌婇敍鍦?D11VA 绾剝袙閸ョ偤鈧偓闁炬崘鐭鹃敍?


### 鐟欙絿鐖滈崥搴ｎ伂

- `PlayerCore` 婢х偛濮?D3D11VA 绾剝袙鐏忔繆鐦柅鏄忕帆閿涘湹indows閿涘鈧?
- 绾剝袙閸掓繂顫愰崠鏍с亼鐠愩儲妞傞懛顏勫З閸ョ偤鈧偓鏉烆垵袙閿涘奔绻氱拠浣稿讲閹绢厽鏂侀幀褌绱崗鍫涒偓?


### 鐟欏棝顣舵潏鎾冲毉鐟欏嫭鏆?
- 鐎靛湱鈥栨禒璺烘姎婢х偛濮?`av_hwframe_transfer_data`閿?
- 鐎靛綊娼?`YUV420P` 鐢冾杻閸?`sws_scale` 鏉烆剚宕查敍宀€绮烘稉鈧稉?`YUV420P` 缂?SDL 濞撳弶鐓嬮妴?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp



## 2026-03-07 閺囧瓨鏌婇敍鍫濆礋閺傚洣娆㈤幒銏＄ゴ娑撳孩鐗稿蹇撴礀瑜版帟鍤滈崝銊ュ閿?


### 閺傛澘顤冮崣顖涘⒔鐞涘苯鍙嗛崣?
- 閺傛澘顤?`--probe-file <media_file>` 閸涙垝鎶ら敍姘崇翻閸?`probe.*` 閺堝搫娅掗崣顖濐嚢鐎涙顔岄敍灞藉瘶閸氼偄顔愰崳?鐟欏棝顣?闂婃娊顣堕悩鑸碘偓浣碘偓浣稿瀻鏉堛劎宸奸妴浣告姎閻滃洢鈧礁锛愰柆鎾茬瑢瀵ら缚顔呮穱鈩冧紖閵?


### 閸ョ偛缍婂銉ュ徔闁?
- 閺傛澘顤?`tools/format_regression/run_format_regression.ps1`閿涙碍瀵滈弽閿嬫拱濞撳懎宕熼懛顏勫З閹靛綊鍣洪幍褑顢戦幒銏＄ゴ閵?
- 閺傛澘顤?`tools/format_regression/format_samples.csv`閿涙氨娣幎銈嗙壐瀵繑鐗遍張顑跨瑢妫板嫭婀＄紓鏍垳娣団剝浼呴妴?
- 閺傛澘顤?`docs/workflows/FORMAT_REGRESSION.md`閿涙俺顔囪ぐ鏇″壖閺堫剙寮弫鑸偓浣界翻閸戦缚鐭惧鍕嫲娴ｈ法鏁ら弬鐟扮础閵?
- 閹躲儱鎲℃潏鎾冲毉閿涙瓪docs/reports/FORMAT_REGRESSION_*.md`閵?
- 鏉╂柨娲栭惍浣筋嚔娑斿绱癭0=閸忋劑鍎碢ASS`閿涘畭1=鐎涙ê婀狿ARTIAL`閿涘畭2=鐎涙ê婀狥AIL`閵?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- tools/format_regression/run_format_regression.ps1

- tools/format_regression/format_samples.csv

- docs/workflows/FORMAT_REGRESSION.md

- docs/README.md



## 2026-03-07 閺囧瓨鏌婇敍鍦檌tHub Actions 閼奉亜濮╅崶鐐茬秺閿?


### CI 瀹搞儰缍斿ù?
- 閺傛澘顤?`.github/workflows/format-regression.yml`閿?
  - PR / `main` / `master` 閼奉亜濮╃憴锕€褰傞敍?
  - 閼奉亜濮╂稉瀣祰 `SDL2/FFmpeg` 妫板嫮绱拠鎴滅贩鐠ф牕鎮楅幍褑顢?Windows 閺嬪嫬缂撻敍?
  - 閺嶉攱婀伴悽鐔稿灇 + `run_all_checks.ps1` 閼奉亜濮╅崶鐐茬秺閿?
  - 娑撳﹣绱?`docs/reports/FORMAT_REGRESSION_CI.md` 娴溠呭⒖閵?


### 閺嬪嫬缂撴稉搴ゅ壖閺堫剙鍚嬬€硅鈧?
- `CMakeLists.txt`閿涘湹indows閿涘绱崗鍫熸暜閹?`SDL2::`閵嗕梗FFMPEG::`閵嗕梗unofficial::ffmpeg::` 鐎电厧鍙嗛惄顔界垼閿涘奔绻氶悾?`external/` 閸ョ偤鈧偓鐠侯垰绶為妴?
- `tools/download_test_samples.ps1` 閺€顖涘瘮闁俺绻?PATH 鐟欙絾鐎?`ffmpeg` 閸涙垝鎶ら崥宥忕礉娓氬じ绨?CI 閻╁瓨甯寸拫鍐暏閵?


### 娴犺濮熷〒鍛礋閸氬本顒?
- 閺囧瓨鏌?`.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `0.3`閿涘湧0/P1/P2 閼煎啫娲块崘鑽ょ波閿涘鐖ｇ拋鏉跨暚閹存劧绱?
  - `2.1.1`閿涘牊鐗稿蹇曠叐闂冮潧鐣炬稊澶婂枙缂佹搫绱氶弽鍥唶鐎瑰本鍨氶敍?
  - `2.1.5`閿涘湧ASS/PARTIAL/FAIL 缂佹挻鐏夌悰顭掔礆閺嶅洩顔囩€瑰本鍨氶敍?
  - `2.3.1`閿涘牊鐗稿蹇曠叐闂冪數绮ㄩ弸婊冨讲鏉╄姤鍑介敍澶嬬垼鐠佹澘鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-07 閺囧瓨鏌婇敍鍫熸尡閺€鎯у灙鐞?+ 鐠佸墽鐤?+ 韫囶偅宓庨柨顕€顩婚悧鍫礆



### 娑撶粯绁︾粙瀣厴閸?
- `main` 閹恒儱鍙?`PlaylistManager`閿?
  - 閺€顖涘瘮婢舵碍鏋冩禒璺哄棘閺佺増鎸遍弨鎯у灙鐞涱煉绱?
  - 閺€顖涘瘮 `.m3u8` 鐎电厧鍙嗛敍?
  - 閺€顖涘瘮 `PageUp/PageDown` 娑撳﹣绔存＃?娑撳绔存＃鏍电幢

  - EOF 閼奉亜濮╅崚鍥ㄥ床娑撳绔存い骞库偓?
- `main` 閹恒儱鍙?`SettingsManager`閿?
  - 閸氼垰濮╅崝鐘烘祰 `config/player_settings.ini`閿?
  - 婢惰精瑙﹂崶鐐衡偓鈧妯款吇閸婄》绱?
  - 闁偓閸戣桨绻氱€涙﹢鐓堕柌蹇嬧偓渚€鈧喎瀹虫稉搴″灙鐞涖劎鍌ㄥ鏇樷偓?


### 娴溿倓绨版晶鐐插繁

- 韫囶偅宓庨柨顕€顩婚悧鍫ョ帛鐠併倝鏁担宥呭弿闁劍甯撮崗銉窗

  - `Space` 閹绢厽鏂?閺嗗倸浠犻敍?
  - `Enter/Alt+Enter/F` 閸忋劌鐫嗛敍?
  - `Esc/Q` 闁偓閸戞椽鈧槒绶敍鍫濆弿鐏炲繋绱崗鍫モ偓鈧崗銊ョ潌閿涘绱?
  - `Left/Right` 娑?`Ctrl+Left/Ctrl+Right` 閻╃顕?seek閿?
  - `Up/Down/+/-/M` 闂婃娊鍣烘稉搴ㄦ饯闂婄绱?
  - `[`/`]` 閸欐﹢鈧喍绗?`R` 閹垹顦?1.0x閵?
- 濞撳弶鐓嬫禍瀣╂鐠囬攱鐪伴柧鎹愮熅閹碘晛鐫嶉崚?`PlayerCore` 娑?`VideoPlayer`閿涘本鏁幐浣峰瘜濞翠胶鈻煎☉鍫ｅ瀭閳ユ粈绗傛稉鈧＃?娑撳绔存＃?闁偓閸戣　鈧繃甯堕崚鎯邦嚞濮瑰倶鈧?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-07 閺囧瓨鏌婇敍鍫⑿╅梽?Core 閸楁洖鍘撳ù瀣槸閻╊喗鐖ｉ敍?


### 閺嬪嫬缂撻柊宥囩枂鐠嬪啯鏆?
- 缁夊娅?`BUILD_CORE_TESTS` 闁銆嶉敍宀勪缉閸忓秳绻氶悾娆愭￥閺佸牊绁寸拠鏇炵磻閸忕偨鈧?
- 缁夊娅?`core_frame_queue_tests`閵嗕梗core_clock_tests` 閸?`core_tests` 閼辨艾鎮庨惄顔界垼閵?


### 閺傚洣娆㈠〒鍛倞

- 閸掔娀娅?`tests/core_frame_queue_tests.cpp`閵?
- 閸掔娀娅?`tests/core_clock_tests.cpp`閵?


### 娣囶喗鏁奸弬鍥︽

- CMakeLists.txt

- tests/core_frame_queue_tests.cpp閿涘牆鍨归梽銈忕礆

- tests/core_clock_tests.cpp閿涘牆鍨归梽銈忕礆

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-07 閺囧瓨鏌婇敍鍫濐樆閹稿倸鐡ч獮鏇炲鏉炶棄鍙嗛崣锝忕礆



### 鐎涙绠烽崗銉ュ經閼宠棄濮?
- 娑撶粯绁︾粙瀣煀婢х偛顦婚幐鍌氱摟楠炴洖寮弫甯窗`--subtitle <file.srt>`閵?
- 閺堫亙绱?`--subtitle` 閺冭绱濋懛顏勫З鐏忔繆鐦崝鐘烘祰娑撳骸缍嬮崜宥呯崯娴ｆ挸鎮撻崥?`.srt` 閺傚洣娆㈤妴?
- `VideoPlayer` 閺傛澘顤冩径鏍ㄥ瘯鐎涙绠烽崝鐘烘祰娑撳孩绔婚悶鍡樺复閸欙綇绱濋弨顖涘瘮 SRT 鐟欙絾鐎介獮鎯邦唶瑜版洘娼惄顔芥殶閵?


### 娣囶喗鏁奸弬鍥︽

- include/video_player.h

- src/video_player.cpp

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-07 閺囧瓨鏌婇敍鍫濈摟楠炴洘瑕嗛弻鎾冲綌閸旂姳绗岄弮璺虹碍閸氬本顒為敍?


### 鐎涙绠峰〒鍙夌厠閼宠棄濮?
- 濞撳弶鐓嬮幒銉ュ經閺傛澘顤冪€涙绠烽弬鍥ㄦ拱闁岸浜鹃敍姝欼VideoRenderer::setSubtitleText()`閵?
- SDL 濞撳弶鐓嬮柧鎹愮熅閺傛澘顤冪€涙绠烽崣鐘插鐏炲偊绱濋弨顖涘瘮婢舵俺顢戦弰鍓с仛閵嗕浇绉撮梹鎸庡焻閺傤厹鈧礁绨抽弶澶哥瑢闂冩潙濂栭妴?
- D3D11/OpenGL 濞撳弶鐓嬮崳銊ㄋ夋鎰摟楠炴洘甯撮崣锝嗐€呯€圭偟骞囬敍灞肩箽鐠囦礁顦块崥搴ｎ伂缂傛牞鐦ф稉鈧懛瀛樷偓褋鈧?
- 瑜版挸澧犵€涙绠风€涙膩娑撻缚浜ら柌蹇撶杽閻滃府绱濋棃?ASCII 鐎涙顑佹导姘舵缁狙勬▔缁€鎭掆偓?


### 鐎涙绠烽弮鍫曟？鏉炴潙鎮撳?
- `PlayerCore` 閺傛澘顤冩径鏍ㄥ瘯鐎涙绠锋潪銊╀壕缁狅紕鎮婃稉搴㈡た鐠哄啰鍌ㄥ鏇犵处鐎涙ǜ鈧?
- 濞撳弶鐓嬬敮褑鐭惧鍕瑢缁屾椽妫界捄顖氱窞閸у洦瀵滆ぐ鎾冲閹绢厽鏂侀弮鍫曟？閺囧瓨鏌婄€涙绠烽敍宀冾洬閻╂牗鎸遍弨?閺嗗倸浠?seek閵?
- 娣囶喖顦茬€涙绠烽弴瀛樻煀娑擃厾娈戦柨浣虹煈鎼达箓妫舵０姗堢礉闁灝鍘ら柨浣稿敶鐠嬪啰鏁ゅ〒鍙夌厠閹恒儱褰涢妴?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `1.1.2 鐎涙绠峰〒鍙夌厠閸欑姴濮瀈 閺嶅洩顔囩€瑰本鍨氶敍?
  - `1.1.3 閹绢厽鏂?閺嗗倸浠?seek 閸氬本顒瀈 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫濈摟楠炴洖绱戦崗鍏呯瑢瀵倸鐖舵径鍕倞閿?


### 鐎涙绠峰鈧崗瀹犲厴閸?
- 閺傛澘顤冩潻鎰攽閺冭泛鐡ч獮鏇炵磻閸忔娊鎽肩捄顖ょ礉閺€顖涘瘮閸︺劍鎸遍弨鍙ヨ厬閹?`V` 閸掑洦宕茬€涙绠烽弰鍓с仛瀵偓/閸忕偨鈧?
- 鐎涙绠烽崗鎶芥４閺冨墎鐝涢崡铏缁屽搫褰旈崝鐘茬湴閿涙盯鍣搁弬鏉跨磻閸氼垰鎮楅幐澶婄秼閸撳秵鎸遍弨鐐闂傚瓨浠径宥呯摟楠炴洖鎮撳銉ｂ偓?
- 鐎涙绠峰鈧崗宕囧Ц閹礁婀幘顓熸杹娴兼俺鐦介崘鍛箽閹镐椒绔撮懛杈剧礉鐠恒劌鐛熸担鎾冲瀼閹广垹褰茬紒褎澹欒ぐ鎾冲閺勫墽銇氶崑蹇撱偨閵?


### 瀵倸鐖舵径鍕倞婢х偛宸?
- 婢舵牗瀵曠€涙绠烽崝鐘烘祰鐠侯垰绶炲Λ鈧弻銉︽暭娑?`std::error_code`閿涘矂浼╅崗宥嗘瀮娴犲墎閮寸紒鐔风磽鐢晲绱堕幘顓溾偓?
- 婢х偛濮炵€涙绠风憴锝嗙€藉鍌氱埗閹规洝骞忔稉搴㈡）韫囨妾风痪褍顦╅悶鍡礉绾喕绻氶幘顓熸杹娑撳鎽兼稉宥勮厬閺傤厹鈧?
- 閸旂姾娴囨径杈Е閺冩湹绻氶幐浣哄Ц閹椒绔撮懛杈剧窗濞撳懐鈹栭弮褍鐡ч獮鏇炶嫙缂佈呯敾閹绢厽鏂佹刊鎺嶇秼閵?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `1.1.4 鐎涙绠峰鈧崗鍏呯瑢瀵倸鐖舵径鍕倞` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫濇彥閹圭兘鏁柊宥囩枂閹镐椒绠欓崠鏍电礆



### 閹镐椒绠欓崠鏍厴閸?
- 閺傛澘顤?`hotkey.*` 闁板秶鐤嗘い鐧哥礉鐟曞棛娲婃＃鏍韫囶偅宓庨柨顔煎З娴ｆ嚎鈧?
- 閸氼垰濮╅弮鎯邦嚢閸?`config/player_settings.ini` 楠炶泛绨查悽銊╂暛娴ｅ秵妲х亸鍕┾偓?
- 闁偓閸戠儤妞傜亸鍡楃秼閸撳秹鏁担宥呮礀閸愭瑩鍘ょ純顔芥瀮娴犺绱濇穱婵婄槈闁插秴鎯庨崥搴濈箽閹镐椒绔撮懛娣偓?


### 娴溿倓绨伴柧鎹愮熅鐠嬪啯鏆?
- `Display` 閺€閫涜礋闁俺绻?`HotkeyManager` 婢跺嫮鎮婇柨顔荤秴閺勭姴鐨犻敍灞肩瑝閸愬秴娴愮€规艾鍟撳璁冲瘜闁款喖鈧鈧?
- `Renderer` / `PlayerCore` / `VideoPlayer` 婢х偛濮為悜顓㈡暛缁狅紕鎮婇柅蹇庣炊閹恒儱褰涢妴?
- 娣囨繄鏆€ `Esc` 娑?`Enter` 閻ㄥ嫬鍚嬬€圭顢戞稉鐚寸礉闂勫秳缍嗘妯款吇娴ｈ法鏁ゆ稊鐘冲劵閸ョ偛缍婃搴ㄦ珦閵?


### 瀵倸鐖舵稉搴″悑鐎?
- 鐎靛綊娼▔?`hotkey.*` 闁板秶鐤嗘潻娑滎攽鐎瑰綊鏁婇梽宥囬獓閿涘牅绻氶悾娆撶帛鐠併倕鑻熺拋鏉跨秿閸涘﹨顒熼敍澶堚偓?
- 閺囧瓨鏌?`config/player_settings.ini` 姒涙顓婚弽铚傜伐閿涘矁藟姒绘劖澧嶉張澶婃彥閹圭兘鏁い骞库偓?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `1.3.2 閺€顖涘瘮闁款喕缍呴柊宥囩枂閹镐椒绠欓崠鏈?閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫濇彥閹圭兘鏁崘鑼崐濡偓濞村绗岄幁銏狀槻姒涙顓婚敍?


### 閸愯尙鐛婂Λ鈧ù瀣厴閸?
- `HotkeyManager` 閺傛澘顤冮柨顔荤秴閸愯尙鐛婂Λ鈧ù瀣复閸欙綇绱?
  - `findConflicts()`閿涙俺绻戦崶鐐插暱缁愪礁濮╂担婊冾嚠閿?
  - `hasConflicts()`閿涙艾鎻╅柅鐔峰灲閺傤厽妲搁崥锕€鐡ㄩ崷銊ュ暱缁愪降鈧?
- 閸氼垰濮╅崝鐘烘祰閻戭參鏁柊宥囩枂閺冭绱濋懛顏勫З濡偓濞村鍣告径宥夋暛娴ｅ秴鑻熸潏鎾冲毉閸愯尙鐛婇弮銉ョ箶閵?


### 閹垹顦叉妯款吇閼宠棄濮?
- `HotkeyManager` 閺傛澘顤?`resetToDefaults()`閿涘瞼绮烘稉鈧崶鐐衡偓鈧妯款吇闁款喕缍呴妴?
- 閺傛澘顤冮柊宥囩枂瀵偓閸?`hotkey.restore_defaults`閿?
  - 鐠佸墽鐤嗘稉?`true` 閸氬函绱濇稉瀣╃濞嗏€虫儙閸斻劏鍤滈崝銊︿划婢跺秹绮拋銈夋暛娴ｅ稄绱?
  - 閹垹顦茬€瑰本鍨氶崥搴ゅ殰閸斻劌娲栭崘娆庤礋 `false`閿涘矂浼╅崗宥夊櫢婢跺秷袝閸欐垯鈧?
- 閸欐垹骞囬崘鑼崐閺冩儼鍤滈崝銊︿划婢跺秹绮拋銈夋暛娴ｅ稄绱濋梼鍙夘剾鏉╂劘顢戦張鐔峰З娴ｆ粍顒犳稊澶堚偓?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `1.3.3 閺€顖涘瘮闁款喕缍呴崘鑼崐濡偓濞村绗岄幁銏狀槻姒涙顓籤 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- include/input/hotkey_manager.h

- src/input/hotkey_manager.cpp

- src/main.cpp

- config/player_settings.ini

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫濈摟楠?seek 閸氬本顒炴灞炬暪閿?


### 妤犲本鏁规稉搴″讲閸ョ偛缍婇懗钘夊

- 閺傛澘顤?`--subtitle-sync-check <subtitle.srt>` 閸涙垝鎶ら敍宀€鏁ゆ禍搴ゅ殰閸斻劑鐛欑拠浣哥摟楠炴洘妞傞梻纾嬮叡閸栧綊鍘ら妴?
- 濡偓閺屻儴顩惄鏍﹁⒈缁婧€閺咁垽绱?
  - 妞ゅ搫绨幘顓熸杹閺冨爼妫挎潪杈剧礄ordered閿涘绱?
  - 闂堢偤銆庢惔?seek 鐠哄疇娴嗛敍鍧癳ek閿涘鈧?
- 鏉堟挸鍤?`mismatches` 娑?`PASS/FAIL`閿涘瞼鏁ゆ禍?M1 妤犲本鏁?`1.4.1`閵?


### 娴狅絿鐖滅紒鎾寸€拫鍐╂殻

- 閹绘劕褰囩€涙绠烽弮鍫曟？鏉炴潙灏柊宥呭彆閸忓崬鍤遍弫甯窗

  - `include/subtitle/subtitle_timeline.h`

  - `src/subtitle/subtitle_timeline.cpp`

- `PlayerCore` 婢跺秶鏁ょ紒鐔剁閸栧綊鍘ら崙鑺ユ殶閿涘矂浼╅崗宥堢箥鐞涘矁鐭惧鍕瑢妤犲本鏁圭捄顖氱窞缁犳纭堕崚鍡楀级閵?


### 閺嶈渹绶ユ稉搴㈠Г閸?
- 閺傛澘顤冮弽铚傜伐鐎涙绠烽敍姝歴amples/subtitle/subtitle_seek_sync_sample.srt`

- 閺傛澘顤冮張顒€婀存宀冪槈閹躲儱鎲￠敍姝歞ocs/reports/SUBTITLE_SYNC_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪1.4.1` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫熸尡閺€鎯у灙鐞?5 閺傚洣娆㈡灞炬暪閿?


### 妤犲本鏁归懗钘夊

- 閺傛澘顤?`--playlist-flow-check` 閸涙垝鎶ら悽銊ょ艾 `1.4.2` 閼奉亜濮╂灞炬暪閵?
- 妤犲本鏁归崠鍛儓閿?
  - 閼峰啿鐨?5 閺夆剝鎸遍弨鎯у灙鐞涖劏绶崗銉︾墡妤犲矉绱?
  - 閸?5 閺夆€崇崯娴ｆ挸褰查幍鎾崇磻濡偓閺屻儻绱?
  - EOF 閼奉亜濮╅崚鍥ㄥ床妞ゅ搫绨憰鍡欐磰 `0 -> 1 -> 2 -> 3 -> 4`閵?


### 鏉堟挸鍤稉搴㈠Г閸?
- 閸涙垝鎶ゆ潏鎾冲毉 `playlist-flow-check.*` 鐎涙顔岄敍灞藉瘶閸?`PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md`閵?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪1.4.2` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/PLAYLIST_FLOW_LOCAL_CHECK.md

- samples/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫ｎ啎缂冾噣鍣搁崥顖涗划婢跺秹鐛欓弨璁圭礆



### 妤犲本鏁归懗钘夊

- 閺傛澘顤?`--settings-persistence-check [settings_file]`閿涘瞼鏁ゆ禍?`1.4.3` 閼奉亜濮╂灞炬暪閵?
- 妤犲本鏁瑰ù浣衡柤閿涙艾鍟撻崗銉啎缂?-> 闁插秷娴囩拋鍓х枂 -> 鐎涙顔屾稉鈧懛瀛樷偓褎鐦€靛箍鈧?


### 閺嶏繝鐛欑憰鍡欐磰鐎涙顔?
- `player.volume_percent`

- `player.playback_speed`

- `player.resume_last_playlist`

- `player.last_playlist_index`

- `hotkey.toggle_subtitle`



### 鏉堟挸鍤稉搴㈠Г閸?
- 閸涙垝鎶ゆ潏鎾冲毉 `settings-persistence-check.*` 鐎涙顔岄崪?`PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md`閵?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪1.4.3` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/reports/SETTINGS_PERSISTENCE_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫濐啇閸ｃ劎鐓╅梼浣兯夋鎰剁礆



### 鐟曞棛娲婇懠鍐ㄦ纯

- 鐎瑰本鍨?`2.1.2` 鐎圭懓娅掓灞炬暪鐟曞棛娲婇敍姝歮p4/mkv/mov/avi/webm/flv/ts/m2ts`閵?


### 閸ョ偛缍婇弽閿嬫拱閹碘晛鐫?
- `format_samples.csv` 閺傛澘顤冩稉澶嬫蒋閺嶉攱婀伴敍?
  - `samples/mov/demo__h264_aac__1920x1080__30fps__2ch.mov`

  - `samples/avi/demo__h264_mp3__1280x720__30fps__2ch.avi`

  - `samples/m2ts/demo__h264_ac3__1920x1080__30fps__2ch.m2ts`



### 閼奉亜濮╅悽鐔稿灇閼存碍婀伴幍鈺佺潔

- `download_test_samples.ps1` 閺傛澘顤?`mov/avi/m2ts` 閻╊喖缍嶆稉搴ｆ晸閹存劖绁︾粙瀣ㄢ偓?


### 閺堫剙婀撮崶鐐茬秺缂佹挻鐏?
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=12, PASS=12, PARTIAL=0, FAIL=0, SKIP=0



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪2.1.2` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫ｎ潒妫版垹绱惍浣虹叐闂冧絻藟姒绘劧绱?


### 鐟曞棛娲婇懠鍐ㄦ纯

- 鐎瑰本鍨?`2.1.3` 鐟欏棝顣剁紓鏍垳妤犲本鏁圭憰鍡欐磰閿涙瓪H.264/H.265/VP9/AV1/MPEG-2`閵?


### 閸ョ偛缍婇弽閿嬫拱閹碘晛鐫?
- `format_samples.csv` 閺傛澘顤冮敍?
  - `samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts`



### 閼奉亜濮╅悽鐔稿灇閼存碍婀伴幍鈺佺潔

- `download_test_samples.ps1` 閺傛澘顤?MPEG-2 鐟欏棝顣堕弽閿嬫拱閻㈢喐鍨氬ù浣衡柤閿涘潉mpeg2video + ac3 + ts`閿涘鈧?


### 閺堫剙婀撮崶鐐茬秺缂佹挻鐏?
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=13, PASS=13, PARTIAL=0, FAIL=0, SKIP=0



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪2.1.3` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫ョ叾妫版垹绱惍浣虹叐闂冧絻藟姒绘劧绱?


### 鐟曞棛娲婇懠鍐ㄦ纯

- 鐎瑰本鍨?`2.1.4` 闂婃娊顣剁紓鏍垳妤犲本鏁圭憰鍡欐磰閿涙瓪AAC/MP3/AC3/E-AC3/DTS/FLAC/Opus/Vorbis/PCM`閵?


### 閸ョ偛缍婇弽閿嬫拱閹碘晛鐫?
- 閺傛澘顤?4 閺夛繝鐓舵０鎴犵椽閻焦鐗遍張顒婄窗

  - `samples/mkv/demo__h264_eac3__1920x1080__30fps__2ch.mkv`

  - `samples/mkv/demo__h264_dts__1920x1080__30fps__2ch.mkv`

  - `samples/webm/demo__vp9_vorbis__1920x1080__30fps__2ch.webm`

  - `samples/mov/demo__h264_pcm_s16le__1920x1080__30fps__2ch.mov`



### 閼存碍婀版晶鐐插繁

- `download_test_samples.ps1` 婢х偛濮?E-AC3/DTS/Vorbis/PCM 閺嶉攱婀伴悽鐔稿灇閵?
- DTS (`dca`) 缂傛牜鐖滃ǎ璇插 `-strict -2`閵?
- `run_format_regression.ps1` 婢х偛濮炵紓鏍垳閸氬秶鐡戞禒宄板爱闁板稄绱?
  - `dts` <-> `dca`

  - `hevc` <-> `h265`

  - `pcm` <-> `pcm_*`



### 閺堫剙婀撮崶鐐茬秺缂佹挻鐏?
- `docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md`

  - Total=17, PASS=17, PARTIAL=0, FAIL=0, SKIP=0



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿涙瓪2.1.4` 瀹告彃鐣幋鎰┾偓?


### 娣囶喗鏁奸弬鍥︽

- tools/format_regression/format_samples.csv

- tools/download_test_samples.ps1

- tools/format_regression/run_format_regression.ps1

- docs/reports/FORMAT_REGRESSION_LOCAL_CHECK.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍦杄coderFactory 閸掓繂顫愰崠鏍ㄧウ缁嬪甯撮崗銉礆



### 鐟欙絿鐖滈崚婵嗩潗閸栨牠鎽肩捄顖滅埠娑撯偓

- `DecoderFactory` 閺傛澘顤?`selectBackendOrder(codec_name, prefer_hardware)`閿涘矁绶崙鍝勬倵缁旑垰鈧瑩鈧绨崚妤€鑻熸穱婵堟殌鏉烆垯娆㈢憴锝囩垳閸忔粌绨抽妴?
- `PlayerCore::initDecoders` 閹恒儱鍙嗛崐娆撯偓澶婄碍閸掓绱濋幐澶愩€庢惔蹇撶毦鐠囨洖鎮楃粩顖氬灥婵瀵叉稉?`avcodec_open2`閿涘苯銇戠拹銉ㄥ殰閸斻劌娲栭柅鈧稉瀣╃娑擃亜鈧瑩鈧鈧?
- `tryConfigureD3D11HardwareDecode` 鐠嬪啯鏆ｆ稉铏瑰嚱 D3D11 闁板秶鐤嗛崙鑺ユ殶閿涘苯鎮楃粩顖滅摜閻ｃ儳绮烘稉鈧悽?`DecoderFactory` 閸愬啿鐣鹃妴?


### 闁板秶鐤嗛崗鐓庮啇

- 娣囨繃瀵?`decoder.prefer_hardware_decode` 闁板秶鐤嗘い鍦晸閺佸牞绱?
  - `true`閿涙矮绱崗鍫⑩€栫憴锝呪偓娆撯偓澶涚礉閸愬秴娲栭柅鈧潪顖澬掗敍?
  - `false`閿涙氨娲块幒銉ㄨ泲鏉烆垯娆㈢憴锝囩垳閸婃瑩鈧鈧?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `3.1.1 DecoderFactory 閹恒儱鍙嗛惇鐔风杽閸掓繂顫愰崠鏍ㄧウ缁嬪獖 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- include/decoder/decoder_factory.h

- src/decoder/decoder_factory.cpp

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍦?D11VA 閸楀繐鏅㈡径杈Е鏉烆垵袙閸忔粌绨抽敍?


### 閸ョ偤鈧偓闁炬崘鐭炬晶鐐插繁

- 閸?`PlayerCore::selectVideoPixelFormat` 娑擃叏绱濊ぐ?D3D11VA 閻╊喗鐖ｉ崓蹇曠閺嶇厧绱￠張顏囶潶鐟欙絿鐖滈崳銊﹀絹娓氭稒妞傞敍灞炬▔瀵繘妾风痪褍鍩屾潪顖欐閸氬海顏敍?
  - `video_hw_pixel_fmt_` 闁插秶鐤嗘稉?`AV_PIX_FMT_NONE`閿?
  - `video_decoder_backend_` 缂冾喕璐?`Software`閵?
- 閸︺劏袙閻礁鍨垫慨瀣濞翠胶鈻兼稉顓∷夐崗鍛礂閸熷棝妾风痪褎妫╄箛妤嬬礉娓氬じ绨€规矮缍呴垾婊呪€栫憴锝呭灥婵瀵查幋鎰娴ｅ棗宕楅崯鍡涙▉濞堥潧娲栭柅鈧垾婵堟畱閸︾儤娅欓妴?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `3.1.2 D3D11VA 閸掓繂顫愰崠鏍︾瑢婢惰精瑙﹂崶鐐衡偓鈧潪顖澬抈 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- src/core/player_core.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍦?D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻劑鎽肩捄顖ょ礆



### 濞撳弶鐓嬮崥搴ｎ伂閼宠棄濮忕悰銉╃秷

- `D3D11VideoRenderer` 鐎瑰本鍨氶張鈧亸蹇撳讲閻劌鐤勯悳甯窗

  - `init`閿涙艾鍨卞鐑樿閺屾捇鎽肩捄顖ょ幢

  - `renderFrame`閿涙碍甯撮崗銉ユ姎娑撳﹣绱堕敍?
  - `present`閿涙碍甯撮崗銉ユ啛閻滃府绱?
  - 娴滃娆㈡稉搴濇唉娴滄帟顕Ч鍌炩偓蹇庣炊閵?
- `Display` 閺傛澘顤冨〒鍙夌厠閸氬海顏崣顖濐潎濞村鍏橀崝娑崇窗

  - 閸欘垵顔曠純?renderer 妞瑰崬濮╅崑蹇撱偨閿?
  - 閸欘垱鐓＄拠銏犵秼閸撳秴鐤勯梽?renderer 閸氬海顏崥宥囆為妴?


### D3D11 閸氬海顏弽锟犵崣娑撳骸娲栭柅鈧崡蹇撴倱

- `D3D11VideoRenderer::init` 閸︺劏顕Ч?`direct3d11` 閸氬函绱濇导姘墡妤犲苯鐤勯梽?SDL renderer 閸氬海顏敍?
  - 閸涙垝鑵?`direct3d11/d3d11`閿涙艾鍨垫慨瀣閹存劕濮涢敍?
  - 閺堫亜鎳℃稉顓ㄧ窗閸掓繂顫愰崠鏍с亼鐠愩儱鑻熼悽鍙樼瑐鐏炲倽袝閸?`SoftwareSDL` 閸ョ偤鈧偓闁炬崘鐭鹃敍鍫滅瑢 `3.2.2` 閸楀繐鎮撻敍澶堚偓?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `3.2.1 D3D11 濞撳弶鐓嬮張鈧亸蹇撳讲閻㈩煉绱檌nit/upload/present閿涘ˇ 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- include/display.h

- src/display.cpp

- include/render/d3d11_video_renderer.h

- src/render/d3d11_video_renderer.cpp

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫熻閺屾挸銇戠拹銉ㄥ殰閸斻劑妾风痪褍娲栬ぐ鎺戝弳閸欙綇绱?


### 閺傛澘顤冮崶鐐茬秺閸涙垝鎶?
- 閺傛澘顤?`--renderer-fallback-check <media_file>`閿?
  - 瀵搫鍩楅弸鍕偓?D3D11 濞撳弶鐓嬮崚婵嗩潗閸栨牕銇戠拹銉ユ簚閺咁垽绱?
  - 妤犲矁鐦夐弰顖氭儊閼奉亜濮╅崶鐐衡偓鈧?`SoftwareSDL`閿?
  - 鏉堟挸鍤?`renderer-fallback-check.*` 鐎涙顔岄崪?`PASS/FAIL`閵?


### 閸欘垵顫囧ù瀣厴閸旀稖藟姒?
- 濞撳弶鐓嬮幒銉ュ經閺傛澘顤冮崥搴ｎ伂閸氬秶袨閺屻儴顕楅敍灞炬尡閺€鎯ф珤鐎电懓顦婚弬鏉款杻瑜版挸澧犲〒鍙夌厠閸氬海顏?鐟欙絿鐖滈崥搴ｎ伂閺屻儴顕楅幒銉ュ經閿涘瞼鏁ゆ禍搴℃嚒娴犮倕瀵叉灞炬暪鏉堟挸鍤妴?


### 閺堫剙婀撮幎銉ユ啞

- 閺傛澘顤?`docs/reports/RENDER_FALLBACK_LOCAL_CHECK.md`閿涘矁顔囪ぐ鏇熸拱閸︽澘娲栬ぐ鎺戞嚒娴犮倓绗岀紒鎾寸亯閵?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`閿?
  - `3.3.2 濞撳弶鐓嬫径杈Е闂勫秶楠囨稉宥勮厬閺傤厽鎸遍弨缍?閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍦礽ndows 鏉烆垵袙/绾剝袙娑撹濮忛弽閿嬫拱閸ョ偛缍婇敍?


### 閸ョ偛缍婇懗钘夊鐞涖儵缍?
- 閺傛澘顤?`--windows-backend-session-check <media_file> <hard|soft>`閿涙艾宕熷Ο鈥崇础娴兼俺鐦藉Λ鈧弻銉ょ瑢缂佹挻鐎崠鏍翻閸戞亽鈧?
- 閺傛澘顤冮獮鍓伹旂€?`--windows-backend-check <media_file>`閿涙俺鍤滈崝銊ㄤ粵閸氬牏鈥栫憴?鏉烆垵袙缂佹挻鐏夐獮鎯扮翻閸?`PASS/FAIL`閵?


### 缁嬪啿鐣鹃幀褌鎱ㄦ径?
- `--windows-backend-check` 娴犲骸鎮撴潻娑氣柤閸欏奔绱扮拠婵囨暭娑撹櫣鍩楁潻娑氣柤閹峰鎹ｆ稉銈勯嚋鐎涙劘绻樼粙瀣剁礄hard/soft閿涘鑻熷Ч鍥ㄢ偓浼欑礉鐟欏嫰浼╂导姘崇樈閸ョ偞鏁归崡鈩冾劥閵?
- Windows 鐎涙劘绻樼粙瀣⒔鐞涘本鏁兼稉?`CreateProcess`閿涘矂浼╅崗?shell 闁插秴鐣鹃崥鎴ｎ嚔濞夋洖妯婂鍌氼嚤閼锋潙銇戠拹銉ｂ偓?


### 閺堫剙婀存宀冪槈

- `build/Debug/modern-video-player.exe --windows-backend-check juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `3.3.1 Windows 娑撳钂嬬憴?绾剝袙娑撹濮忛弽閿嬫拱閸欘垱鎸盽 閺嶅洩顔囩€瑰本鍨氶妴?
  - `3.3.3 閸ョ偛缍婇幎銉ユ啞鐎瑰本鏆 閺嶅洩顔囩€瑰本鍨氶妴?


### 閺傛澘顤冮幎銉ユ啞

- `docs/reports/WINDOWS_BACKEND_LOCAL_CHECK.md`



## 2026-03-08 閺囧瓨鏌婇敍鍫㈢彿閼哄倸顕遍懜顏庣窗娑撳﹣绔寸粩?娑撳绔寸粩鐙呯礆



### 娴溿倓绨伴懗钘夊婢х偛宸?
- 閺傛澘顤冪粩鐘哄Ν鐎佃壈鍩呰箛顐ｅ祹闁款噯绱?
  - `HOME`閿涙俺鐑︽潪顑跨瑐娑撯偓缁旂媴绱?
  - `END`閿涙俺鐑︽潪顑跨瑓娑撯偓缁旂姰鈧?
- 缁旂姾濡拠閿嬬湴闁炬崘鐭惧鍙夊ⅵ闁熬绱?
  - `Display -> Renderer -> PlayerCore -> VideoPlayer`閵?


### 婵帊缍嬫穱鈩冧紖閼宠棄濮忔晶鐐插繁

- `Demuxer` 閺傛澘顤冪粩鐘哄Ν閸忓啯鏆熼幑顔啃掗弸鎰剁窗

  - `ChapterInfo`閿涘澃tart/end/title閿涘绱?
  - `MediaInfo::chapters`閵?
- `PlayerCore` 閸︺劍澧﹀鈧刊鎺嶇秼閸氬孩鐎铏圭彿閼哄倹妞傞梻瀵稿仯閿涘苯鑻熼幓鎰返閿?
  - `seekToNextChapter()`閿?
  - `seekToPreviousChapter()`閿?
  - `chapterCount()`閵?


### 妤犲本鏁归懗钘夊鐞涖儵缍?
- 閺傛澘顤?`--chapter-nav-check <media_file>`閿?
  - 閼奉亜濮╅幍褑顢戦幘顓熸杹閵嗕椒绗呮稉鈧粩鐘偓浣风瑐娑撯偓缁旂姴鑻熼弽锟犵崣娴ｅ秶鐤嗛崣妯哄閿?
  - 鏉堟挸鍤?`chapter-nav-check.*` 娑?`PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/CHAPTER_NAV_LOCAL_CHECK.md`閵?


### 閺堫剙婀存宀冪槈

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.1 缁旂姾濡€佃壈鍩呴敍鍫滅瑐娑撯偓缁?娑撳绔寸粩鐙呯礆` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍦?B Repeat閿?


### 娴溿倓绨伴懗钘夊婢х偛宸?
- 閺傛澘顤?A-B Repeat 姒涙顓婚悜顓㈡暛閿?
  - `A`閿涙俺顔曠純?A 閻愮櫢绱?
  - `B`閿涙俺顔曠純?B 閻愮懓鑻熼崥顖滄暏瀵邦亞骞嗛敍?
  - `C`閿涙碍绔婚梽?A-B Repeat閵?
- 鐢喖濮穱鈩冧紖鐞涖儱鍘?`A/B/C` 闁款喕缍呯拠瀛樻閵?


### 閹绢厽鏂侀弽绋跨妇閼宠棄濮忔晶鐐插繁

- `PlayerCore` 閺傛澘顤?A-B Repeat 閻樿埖鈧胶顓搁悶鍡曠瑢閺屻儴顕楅敍?
  - `setABRepeatStart()`閿?
  - `setABRepeatEnd()`閿?
  - `clearABRepeat()`閿?
  - `isABRepeatEnabled()`閿?
  - `abRepeatStart()` / `abRepeatEnd()`閵?
- 閺傛澘顤冮幘顓熸杹瀵邦亞骞嗗Λ鈧ù?`handleABRepeatLoop()`閿?
  - 瑜版挻鎸遍弨鍙ョ秴缂冾喖鍩屾潏?B 閻愯妞傞懛顏勫З seek 閸?A 閻愬箍鈧?


### 妤犲本鏁归懗钘夊鐞涖儵缍?
- 閺傛澘顤?`--ab-repeat-check <media_file>`閿?
  - 閼奉亜濮╅幍褑顢戦垾婊嗩啎缂?A 閻?-> 鐠佸墽鐤?B 閻?-> 鐟欏倸鐧傚顏嗗箚 -> 濞撳懘娅庨垾婵撶幢

  - 鏉堟挸鍤?`ab-repeat-check.*` 娑?`PASS/FAIL`閵?
- 閺傛澘顤冮張顒€婀撮幎銉ユ啞閿涙瓪docs/reports/AB_REPEAT_LOCAL_CHECK.md`閵?


### 閸ョ偛缍婃穱顔碱槻

- 娣囶喖顦?`--settings-persistence-check` 娑撳孩鏌婃妯款吇閻戭參鏁崘鑼崐閿?
  - 濞村鐦柨顔荤秴閻?`b` 鐠嬪啯鏆ｆ稉?`x`閿涘本浠径宥呮礀瑜版帞菙鐎规碍鈧佲偓?


### 閺堫剙婀存宀冪槈

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --chapter-nav-check %TEMP%\\mvp_chapter_sample.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --renderer-fallback-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --windows-backend-check .\\juren-30s.mp4`閿涙瓪PASS`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.2 A-B Repeat` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫熷焻閸ユ拝绱?


### 娴溿倓绨伴懗钘夊婢х偛宸?
- 閺傛澘顤冪粙鍐茬暰閸欘垳鏁ら惃鍕焻閸ユ崘鍏橀崝娑崇窗

  - `S` 鐟欙箑褰傞幋顏勬禈閿?
  - 鏉堟挸鍤弬鍥︽閸愭瑥鍙?`screenshots/` 閻╊喖缍嶉敍?
  - 閸涜棄鎮曢弽鐓庣础娑?`screenshot_YYYYMMDD_HHMMSS_mmm.ppm`閵?


### 閹搭亜娴橀柧鎹愮熅閺€鑸垫殐

- `PlayerCore` 閺傛澘顤冮張鈧潻鎴炶閺屾挸鎶氱紓鎾崇摠閿涘瞼鏁ゆ禍搴℃躬閺嗗倸浠犻幀浣烘纯閹恒儰绻氱€涙ê缍嬮崜宥囨暰闂堫潿鈧?
- `requestScreenshot()` 閸栧搫鍨庢稉銈囶潚鐠侯垰绶為敍?
  - 閹绢厽鏂佹稉顓ㄧ窗瀵倹顒炵粵澶婄窡瑜版挸澧?娑撳绔寸敮褑鎯ら惄姗堢幢

  - 閺嗗倸浠犻幀渚婄窗閻╁瓨甯存禒搴ｇ处鐎涙ê鎶氶拃鐣屾磸閵?
- `main` 娑擃厾娈?`--screenshot-check` 閺€閫涜礋鐟曞棛娲婇弳鍌氫粻閹焦鍩呴崶鎯ф簚閺咁垬鈧?


### 閺堫剙婀存宀冪槈

- `build/Debug/modern-video-player.exe --screenshot-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --ab-repeat-check .\\juren-30s.mp4`閿涙瓪PASS`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.3 閹搭亜娴榒 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫濇姎濮濄儴绻橀敍?


### 娴溿倓绨伴懗钘夊婢х偛宸?
- 閺傛澘顤冮弳鍌氫粻閹礁鎶氬銉ㄧ箻閿?
  - `,` 閸氬酣鈧偓娑撯偓鐢嶇幢

  - `.` 閸撳秷绻樻稉鈧敮褝绱?
  - 濮濄儴绻橀崥搴濈矝娣囨繃瀵旈弳鍌氫粻閻樿埖鈧降鈧?


### 娑撳鎽肩€圭偟骞?
- `PlayerCore` 閺傛澘顤?`stepFrameBackward()` / `stepFrameForward()`閿涘矂鍣伴悽銊⑩偓婊勬畯閸嬫粍鈧?seek + 妫ｆ牕鎶氶崚閿嬫煀閳ユ繃鏁归弫娑樺礋鐢勵劄鏉╂稏鈧?
- 濮濄儴绻橀弮璺侯槻閻劍娓舵潻鎴炶閺屾挸鎶氶弮鍫曟毐娑撳骸鐛熸担?FPS 娴兼壆鐣诲銉╂毐閿涘苯鑻熼崷?seek 閸氬簼瀵岄崝銊﹁閺屾挾娲伴弽鍥ㄦ闂傚鍋ｉ惃鍕浕娑擃亣顫嬫０鎴濇姎閵?
- 闂婃娊顣跺☉鍫ｅ瀭缁捐法鈻奸崷銊︽畯閸嬫粍鈧椒绗夐崘宥囨暏閺?`playback_pts` 閸ョ偛鍟撹ぐ鎾冲娴ｅ秶鐤嗛敍宀勪缉閸忓秴鎶氬銉ㄧ箻閸氬簼缍呯純顔款潶闂婃娊顣堕弮鍫曟寭閸ョ偞濯洪妴?
- `main` 閺傛澘顤?`--frame-step-check <media_file>` 閼奉亝顥呴崨鎴掓姢閵?


### 閺堫剙婀存宀冪槈

- `build/Debug/modern-video-player.exe --frame-step-check .\\juren-30s.mp4`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --settings-persistence-check`閿涙瓪PASS`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `4.4 鐢勵劄鏉╂冻绱欓弳鍌氫粻閹緤绱歚 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫濇▕鐠烘繆鐦庢导鐗堟瀮濡楋絽顕鎰剁礆



### 閺傚洦銆傞崺铏瑰殠閸掗攱鏌?
- `docs/analysis/MPC_HC_GAP_ANALYSIS.md` 閺囧瓨鏌婃稉鐑樺焻閼?`2026-03-08` 閻ㄥ嫬鐤勯悳鎵Ц閹降鈧?
- 鐠囧嫪鍙婃笟婵囧祦娴犲簶鈧粌褰ч惇瀣复閸?妤犮劍鐏﹂垾婵嗗磳缁狙傝礋閳ユ粈鍞惍浣稿弳閸?+ 閺堫剙婀存灞炬暪閹躲儱鎲￠垾婵婁粓閸氬牆鍨介弬顓溾偓?


### 缂佹捁顔戞穱顔筋劀

- 鐏忓棔浜掓稉瀣侀崸妞剧矤閺冄呭閻ㄥ嫧鈧粓顎囬弸?閺堫亝甯撮崗銉⑩偓婵堢眰濮濓絼璐熼垾婊堝劥閸掑棗鐤勯悳鎵斥偓婵撶窗

  - 鐎涙绠风化鑽ょ埠閿?
  - 閹绢厽鏂侀崚妤勩€冮敍?
  - 鐟欙絿鐖滈崳銊ь吀閻炲棴绱?
  - 韫囶偅宓庨柨顔鹃兇缂佺噦绱?
  - 鐠佸墽鐤嗙化鑽ょ埠閵?
- 濡€虫健缂佺喕顓搁弴瀛樻煀娑撶尨绱?
  - `鏉堟儳鍩?MPC-HC 缁涘楠? 0 / 14`

  - `闁劌鍨庣€圭偟骞? 11 / 14`

  - `妤犮劍鐏?閺堫亝甯撮崗? 3 / 14`



### 閸撯晙缍戦柌宥囧仯閺囧瓨鏌?
- `P0` 閼辨氨鍔嶆潪顑胯礋閿?
  - `1080p60 / 4K / 妤傛鐖滈悳鍢?缁嬪啿鐣鹃幀褌绗岄幀褑鍏橀弮銉ョ箶閿?
  - 婢舵岸鐓舵潪?鐎涙绠锋潪?閺冭泛娆㈢拫鍐Ν閿?
  - `OpenGL` 閸氬海顏粵鏍殣閺€鑸垫殐閿?
  - 閸婂秹鈧喖鐓舵０鎴犵摜閻ｃ儳鎴风紒顓炵暚閸犲嫨鈧?
- `P1` 閼辨氨鍔嶉敍姘弿闁插繐鎻╅幑鐑芥暛閵嗕胶鏁鹃獮?缂傗晜鏂?閺冨娴嗛妴浣规姢闂€婊呮暏閹村嘲鍙嗛崣锝冣偓?
- `P2` 閼辨氨鍔嶉敍姘絻娴犺翰鈧焦绁︽刊鎺嶇秼閵嗕胶姣婇懖銈囬兇缂佺喍楠囬崫浣稿閵?


### 娣囶喗鏁奸弬鍥︽

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 2026-03-08 閺囧瓨鏌婇敍鍫㈠閺堫剚鏋冨锝呭坊閸欏弶顔岄拃鑺ョ閻炲棴绱?


### 閸樺棗褰剁粩鐘哄Ν閸樼粯顒犳稊?
- 鐏忓棌鈧粓妯佸▓鍏哥閿涙艾鐔€绾偓閹绢厽鏂侀崳顭掔礄瑜版挸澧犻梼鑸殿唽閿涘鈧繆鐨熼弫缈犺礋閳ユ粓妯佸▓鍏哥閿涙艾鐔€绾偓閹绢厽鏂侀崳顭掔礄閸樺棗褰剁挧椋庡仯閿涘鈧繐绱濋弰搴ｂ€樼拠銉︻唽鐠佹澘缍嶉惃鍕Ц `2026-02-17 ~ 2026-02-25` 閻ㄥ嫭妫張鐔风杽閻滄澘鐔€缁捐￥鈧?
- 娑撴椽妯佸▓鍏哥鐞涖儱鍘栫拠瀛樻閿涙碍妫悧?`decoder / playLoop` 鐠侯垰绶炲鎻掓躬 `2026-03-06` 閺嬭埖鐎弨鑸垫殐閸氬骸鑻熼崗?`PlayerCore + Scheduler + core/*` 娑撳鎽奸妴?


### 閺冄嗙熅瀵板嫯銆冩潻鐗堢濞?
- 鐏忓棙妫張?`video_decoder` / `audio_decoder` 娴犮儱寮?`VideoDecodeThread` / `AudioDecodeThread` / `SyncManager` 閻ㄥ嫭鏋冩禒鍓侀獓鐞涖劏鍫弨鐟板晸娑撻缚鍏橀崝娑氶獓閸樺棗褰剁拋鏉跨秿閵?
- 鐏忓棌鈧粈绗呮稉鈧銉吀閸掓巻鈧繃鏁奸崘娆庤礋閳ユ粓妯佸▓闈涙倵缂侇厾娲伴弽鍥风礄閸樺棗褰剁拋鏉跨秿閿涘鈧繐绱濋柆鍨帳娑撳骸缍嬮崜宥堝嚡娴狅綀绻樻惔锔剧彿閼哄倹璐╁ǎ鍡愨偓?
- 鐏?`USE_NEW_PLAYER_CORE` 閸滃奔澶嶉弮?`tests/core_*` 閻ㄥ嫬宸婚崣鍙夊伎鏉╃増鏁奸崘娆庤礋閳ユ粍鐏﹂弸鍕暪閺?/ 閸氬海鐢诲〒鍛倞閳ユ繂褰涘鍕┾偓?


### 娣囶喗鏁奸弬鍥︽

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





## 2026-03-08 閺囧瓨鏌婇敍鍫熸尡閺€鐐偓褑鍏橀弮銉ョ箶妤犲本鏁归敍?


### 閸欘垵顫囧ù瀣偓褑藟姒?
- 閺傛澘顤?`core::DiagnosticsSnapshot`閿涘瞼绮烘稉鈧€电厧鍤憴锝呯殱鐟佸懎瀵樼拋鈩冩殶閵嗕線鍣哥拠?娑撱垹瀵橀妴浣叫掗惍浣告姎閺佽埇鈧焦瑕嗛弻鎾虫姎閺佽埇鈧線鐓舵０鎴炲絹娴溿倕鎶氶弫鑸偓浣界殶鎼达箑娅掗幒澶婃姎娑撳酣妲﹂崚妤呮毐鎼达负鈧?
- `VideoPlayer` 閺傛澘顤?`getInfo()` / `getDiagnosticsSnapshot()` 闁繋绱堕幒銉ュ經閿涘奔绶甸崨鎴掓姢鐞涘矂鐛欓弨璺烘嫲閸氬海鐢荤拠濠冩焽婢跺秶鏁ら妴?
- `main` 閺傛澘顤?`--performance-log-check <media_file> [sample_ms]`閿涘矁绶崙鐚寸窗

  - renderer / decoder backend閿涘牅缍旀稉鍝勭秼閸?GPU 鐠侯垰绶為弽鍥槕閿涘绱?
  - 鏉╂稓鈻奸獮鍐叉綆 CPU 閸楃姷鏁ゆ稉搴ㄢ偓鏄忕帆閺嶇绺鹃弫甯幢

  - demux / decode / render / scheduler / queue 閸忔娊鏁幐鍥ㄧ垼閵?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --performance-log-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 1200`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/PERFORMANCE_LOG_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.4 鏉堟挸鍤幀褑鍏橀弮銉ョ箶閿涘牊甯€鐢?闂冪喎鍨?CPU/GPU閿涘ˇ 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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





## 2026-03-08 閺囧瓨鏌婇敍?080p60 缁嬪啿鐣鹃幘顓熸杹妤犲本鏁归敍?


### 缁嬪啿鐣鹃幀褔妫粋浣剿夋?
- `main` 閺傛澘顤?`--1080p60-check <media_file> [sample_ms]`閿涘苯顦查悽?`probe + diagnostics` 閸欙絽绶炴宀冪槈閿?
  - `1920x1080 @ 60fps` 閺嶉攱婀伴弰顖氭儊閸栧綊鍘ら惄顔界垼閿?
  - 鏉╃偟鐢婚幘顓熸杹缁愭褰涢崘鍛闂傚瓨妲搁崥锔藉瘮缂侇厽甯规潻娑崇幢

  - 鐠嬪啫瀹抽崳銊︽Ц閸氾箑鍤悳?`late_drop`閿?
  - 鐟欙絽鐨濈憗鍛Ц閸氾箑褰傞悽鐔舵丢閸栧懌鈧?
- 閺傛澘顤?`samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4` 閻ㄥ嫭鐗遍張顒傛晸閹存劘鍓奸張顒€鍙嗛崣锝忕礉閻劋绨張顒€婀寸粙鍐茬暰閹冩礀瑜版帇鈧?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --1080p60-check .\samples\mp4\demo__h264_aac__1920x1080__60fps__2ch.mp4 5000`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/1080P60_STABILITY_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.1 1080p60 闂€鎸庢缁嬪啿鐣綻 閺嶅洩顔囩€瑰本鍨氶妴?
  - `2.3.2 1080p60 缁嬪啿鐣鹃幘顓熸杹鏉堢偓鐖 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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





## 2026-03-08 閺囧瓨鏌婇敍?K 閹绢厽鏂佹稉搴ㄦ缁狙囩崣閺€璁圭礆



### 4K 闂傘劎顩︾悰銉╃秷

- `main` 閺傛澘顤?`--4k-playback-check <media_file> [sample_ms]`閿涘矁浠堥崥鍫ョ崣鐠囦緤绱?
  - `4K` 閺嶉攱婀伴崷銊ㄧ箾缂侇厽鎸遍弨鍓х崶閸欙絽鍞撮弰顖氭儊閹镐胶鐢婚幒銊ㄧ箻閿?
  - 瑜版挸澧犻幘顓熸杹闁炬崘鐭鹃弰顖氭儊閺?`late_drop`閿?
  - hard / soft 娑撱倓閲滅€涙劘绻樼粙瀣╃窗鐠囨繃妲搁崥锕€娼庨崣顖濈箻閸忋儲鎸遍弨鎾呯礉鐠囦焦妲戠涵顒冃掓径杈Е閺冭泛褰查梽宥囬獓閵?
- 妤犲本鏁归柅鏄忕帆婢跺秶鏁ゆ禍鍡楀嚒閺?`runBackendSessionSubprocess()`閿涘矂浼╅崗宥夊櫢婢跺秴鐤勯悳?Windows 閸氬海顏崶鐐衡偓鈧弽锟犵崣閵?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --4k-playback-check .\samples\mkv\demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv 2000`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/4K_PLAYBACK_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.2 4K 閸欘垱鎸遍弨鎾呯礄娴兼ê鍘涚粙鍐茬暰閿涘苯鍟€閹绘劖鈧嗗厴閿涘ˇ 閺嶅洩顔囩€瑰本鍨氶妴?
  - `2.3.3 4K 閹绢厽鏂侀崣顖滄暏楠炶泛褰查梽宥囬獓` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/4K_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md





## 2026-03-08 閺囧瓨鏌婇敍鍫ョ彯閻胶宸奸弽閿嬫拱妤犲本鏁归敍?


### 妤傛鐖滈悳鍥，缁備浇藟姒?
- `main` 閺傛澘顤?`--high-bitrate-check <media_file> [sample_ms]`閿涘苯澧犵純顔款嚢閸欐牗鐗稿蹇曠垳閻滃洤鑻熺憰浣圭湴 `>= 80Mbps`閵?
- 鏉╃偟鐢婚幘顓熸杹缁愭褰涢崥灞炬濡偓閺屻儻绱伴弮鍫曟？閹恒劏绻橀妴涔ate_drop`閵嗕龚emux 娑撱垹瀵樻稉搴＄杽闂?decoder / renderer backend閵?
- `tools/download_test_samples.ps1` 閺傛澘顤?`stress100m__h264_aac__1920x1080__60fps__2ch.mp4` 閻㈢喐鍨氶崗銉ュ經閿涘瞼鏁ゆ禍搴ㄧ彯閻胶宸奸崶鐐茬秺閵?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --high-bitrate-check .\samples\mp4\stress100m__h264_aac__1920x1080__60fps__2ch.mp4 3000`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/HIGH_BITRATE_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `2.2.3 婢堆呯垳閻滃洦鐗遍張顒婄礄>80Mbps閿涘褰查幘顓熸杹` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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





## 2026-03-08 閺囧瓨鏌婇敍鍫ユ毐閺冭埖鎸遍弨鍓旂€规碍鈧囩崣閺€璁圭礆



- `main` 閺傛澘顤?`--long-playback-check <media_file> [sample_ms]`閿涘矁顩﹀Ч鍌炲櫚閺嶉鐛ラ崣锝堝殾鐏?`5000ms`閿涘苯鑻熸潏鎾冲毉 `probe`閵嗕攻ackend閵嗕焦妞傞梻瀛樺腹鏉╂稐绗岄幒澶婃姎/娑撱垹瀵橀梻銊ь洣鐎涙顔岄妴?
- 鏉╃偟鐢婚幘顓熸杹缁愭褰涢崥灞炬濡偓閺屻儻绱癭open_ok`閵嗕浇绻橀崗銉︽尡閺€鎯ф儕閻滎垬鈧胶鐛ラ崣锝囩波閺夌喎鎮楁禒宥咁槱娴滃孩鎸遍弨鐐偓浣碘偓涔dvance_ratio` 鏉堢偓鐖ｉ妴涔cheduler_late_drops=0` 娑?`demux_dropped_packets=0`閵?
- 閺傛澘顤?`docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`閿涘苯鑻熼崥灞绢劄閸欐垵绔烽梻銊ь洣 `6.1 ~ 6.6` 娑撳搫鍑＄€瑰本鍨氶妴?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --long-playback-check .\juren-30s.mp4 10000`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `6.1 閸旂喕鍏橀敍姝?/M2 韫囧懘銆忛崗銊╁劥闁俺绻僠 閺嶅洩顔囩€瑰本鍨氶妴?
  - `6.2 閺嶇厧绱￠敍姘瘜閸旀稒鐗稿蹇曠叐闂冨灚婀佺紒鎾寸亯娑撴柨褰茬憴锝夊櫞` 閺嶅洩顔囩€瑰本鍨氶妴?
  - `6.3 閸掑棜椴搁悳鍥风窗1080p60 缁嬪啿鐣鹃敍?K 閸欘垳鏁ら獮璺哄讲闂勫秶楠嘸 閺嶅洩顔囩€瑰本鍨氶妴?
  - `6.4 娴溿倓绨伴敍姘剁帛鐠併倕鎻╅幑鐑芥暛鐎瑰本鏆ｉ崣顖滄暏` 閺嶅洩顔囩€瑰本鍨氶妴?
  - `6.5 缁嬪啿鐣鹃幀褝绱伴梹鎸庢閹绢厽鏂侀弮?crash` 閺嶅洩顔囩€瑰本鍨氶妴?
  - `6.6 閸欘垵顫囧ù瀣剁窗閸忔娊鏁弮銉ョ箶鐎瑰本鏆 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- src/main.cpp

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- docs/README.md

- docs/reports/LONG_PLAYBACK_LOCAL_CHECK.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 2026-03-08 閺囧瓨鏌婇敍鍫熷絻娴犲墎閮寸紒鐕傜礆



- 閺傛澘顤?`include/plugin/plugin_api.h` 娑撳酣鍣搁崘?`PluginManager`閿涘矁藟姒?`DLL` 閸斻劍鈧礁濮炴潪濮愨偓涔PI` 閻楀牊婀伴弽锟犵崣閵嗕梗load/unload` 閻㈢喎鎳￠崨銊︽埂娑撳骸绱撶敮闀愮箽閹躲們鈧?
- `PluginManager` 閻滄澘褰叉担婊€璐熼幓鎺嶆鐎瑰じ瀵屽▔銊ュ斀婢舵牠鍎寸憴鍡涱暥/闂婃娊顣跺銈夋殔瀹搞儱宸堕敍娌桭ilterRegistry` 閸氬本顒炵悰銉ュ帠濞夈劑鏀㈤幒銉ュ經閿涘本鏁幐浣稿祻鏉炶姤妞傚〒鍛倞閹碘晛鐫嶉悙骞库偓?
- 閺傛澘顤?`sample_logger_plugin` 缁€杞扮伐 `DLL` 娑?`--plugin-check [plugin_dir_or_file]` 妤犲本鏁归崨鎴掓姢閿涘矂鐛欑拠浣瑰絻娴犺泛濮炴潪濮愨偓浣规姢闂€婊勬暈閸愬奔绗岄崡姝屾祰濞撳懐鎮婇梻顓犲箚閵?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `build/Debug/modern-video-player.exe --plugin-check`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/PLUGIN_SYSTEM_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.1 閹绘帊娆㈢化鑽ょ埠` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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



## 2026-03-08 閺囧瓨鏌婇敍鍫熺ウ婵帊缍?HTTP 閸掑棛澧栨稉搴ｇ处閸愯绱?


- 闁插秴鍟?`HttpStreamDownloader`閿涙艾鐔€娴?FFmpeg `avio` 閺€顖涘瘮閻喎鐤?HTTP 閹垫挸绱戦妴浣稿瀻閸ф顕伴崣鏍モ偓浣稿敶闁劎绱﹂崘灞傗偓涓扥F 閻樿埖鈧椒绗岄柨娆掝嚖闁繋绱堕妴?
- `main` 閺傛澘顤?`--streaming-buffer-check <playlist_url> [segment_limit] [target_buffer_bytes]`閿涘苯褰叉稉瀣祰 HLS 婵帊缍嬪〒鍛礋閵嗕浇袙閺嬫劗娴夌€电懓鍨庨悧?URL閿涘苯鑻熸宀冪槈閸掑棛澧栫紓鎾冲暱闂傤厾骞嗛妴?
- 閺傛澘顤冮張顒€婀存径鐟板徔 `samples/streaming/hls_local/*` 娑?`tools/start_streaming_fixture_server.ps1`閿涘瞼鏁ゆ禍搴℃躬閺堫剚婧€閸氼垰濮?HTTP 闂堟瑦鈧焦婀囬崝鈥宠嫙婢跺秶骞囩€圭偤鐛欓妴?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming/hls_local -Port 8765`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8765/sample.m3u8 3 128`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/STREAMING_BUFFER_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.2 濞翠礁鐛熸担鎿勭礄閻喎鐤?HTTP 閸掑棛澧栨稉搴ｇ处閸愯绱歚 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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





## 2026-03-08 閺囧瓨鏌婇敍鍦歀S/DASH 閼奉亪鈧倸绨查惍浣哄芳閿?


- 閹碘晛鐫?`HlsManifestParser`閿涙俺鐦戦崚?`#EXT-X-STREAM-INF` master playlist閵嗕礁顦块惍浣哄芳 variant 娑撳骸鐛熸担鎾存尡閺€鎯у灙鐞涖劊鈧?
- 閹碘晛鐫?`DashManifestParser`閿涙俺袙閺?`Representation` 鐢箑顔旈妴涔aseURL`閵嗕梗Initialization` 娑?`SegmentURL` 閺勫海绮忛妴?
- 閺傛澘顤?`AdaptiveBitrateSelector`閿涘本瀵滄导鎵暬鐢箑顔旈柅澶嬪閺堚偓閸氬牓鈧倻鐖滈悳鍥风礉楠炶泛婀?`main` 娑擃厽褰佹笟?`--adaptive-bitrate-check <manifest_url> <bandwidth_samples_csv> [segment_limit] [target_buffer_bytes]` 閺堫剙婀存灞炬暪閸忋儱褰涢妴?
- 閺傛澘顤?`samples/streaming/abr_local/{hls,dash}` 婢剁懓鍙块敍灞借嫙閹碘晛鐫?`tools/start_streaming_fixture_server.ps1` 閺€顖涘瘮 `mpd/m4s/mp4` 閸愬懎顔愮猾璇茬€烽妴?


### 閺堫剙婀存宀冪槈

- `cmake --build build --config Debug`閿涙岸鈧俺绻冮妴?
- `.\tools\start_streaming_fixture_server.ps1 -RootPath samples/streaming -Port 8766`

- `build/Debug/modern-video-player.exe --streaming-buffer-check http://127.0.0.1:8766/hls_local/sample.m3u8 3 128`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/hls/master.m3u8 900000,3500000,1500000 2 128`閿涙瓪PASS`

- `build/Debug/modern-video-player.exe --adaptive-bitrate-check http://127.0.0.1:8766/abr_local/dash/sample.mpd 900000,3500000,1500000 2 128`閿涙瓪PASS`

- 妤犲本鏁归幎銉ユ啞閿涙瓪docs/reports/ADAPTIVE_BITRATE_LOCAL_CHECK.md`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `7.3 HLS/DASH 閼奉亪鈧倸绨查惍浣哄芳` 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

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

## 2026-03-08 閺囧瓨鏌婇敍鍫濈紦缁斿鍣风粙瀣暥閺嶅洨顒烽敍?


- 鐎瑰本鍨氭禒璇插濞撳懎宕?`0.4`閿涘苯缂撶粩瀣櫡缁嬪顣堕弽鍥╊劮 `v0.2.0-rc1` 娑?`v0.2.0`閵?
- `v0.2.0-rc1` 閻劋绨崘鑽ょ波閸欐垵绔烽梻銊ь洣瀹稿弶鏁归崣锝囨畱閸婃瑩鈧鎻╅悡褝绱盽v0.2.0` 閻劋绨弽鍥唶瑜版挸澧犳稉鑽ゅ殠闁插瞼鈻肩喊鎴濈暚閹存劗鍋ｉ妴?
- 閸氬本顒為崚閿嬫煀瀹割喛绐涚拠鍕強娑撳簼鎹㈤崝鈩冪閸楁洜濮搁幀渚婄礉闁灝鍘ら弬鍥ㄣ€傛禒宥勭箽閻ｆ瑢鈧粈绮庡顔界垼缁涚偓鎼锋担婧锯偓婵堟畱閺冄冨經瀵板嫨鈧?
- 閻㈠彉绨柌宀€鈻肩喊鎴濆嚒閸忓嘲顦崣顖濇嫹濠ь垳娈?`RC` 閺嶅洨顒烽敍灞兼崲閸斺剝绔婚崡鏇氳厬閻?`5.3 濮ｅ繋閲滈柌宀€鈻肩喊鎴犵波閺夌喎澧犺箛鍛淬€忛崣顖涘ⅵ RC 閺嶅洨顒穈 娑旂喎鎮撳銉﹀姬鐡掔偨鈧?


### 閺嶅洨顒风拠瀛樻

- `v0.2.0-rc1`閿涙艾鐔€娴滃骸褰傜敮鍐，缁備線妫撮悳顖氭倵閻ㄥ嫮菙鐎规艾鎻╅悡褍缂撶粩瀣ㄢ偓?
- `v0.2.0`閿涙艾鐔€娴滃骸缍嬮崜?`main` 閻ㄥ嫰鍣风粙瀣暥閺€璺哄經韫囶偆鍙庡铏圭彌閵?


### 閺堫剙婀存宀冪槈

- `git tag --list`閿涙氨鈥樼拋銈嗙垼缁涙儳鍑＄€涙ê婀妴?
- `git show v0.2.0-rc1 --no-patch --stat`

- `git show v0.2.0 --no-patch --stat`



### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `0.4 瀵よ櫣鐝涢柌宀€鈻肩喊鎴炵垼缁涙拝绱檝0.2.0-rc1 / v0.2.0閿涘ˇ 閺嶅洩顔囩€瑰本鍨氶妴?
  - `5.3 濮ｅ繋閲滈柌宀€鈻肩喊鎴犵波閺夌喎澧犺箛鍛淬€忛崣顖涘ⅵ RC 閺嶅洨顒穈 閺嶅洩顔囩€瑰本鍨氶妴?


### 娣囶喗鏁奸弬鍥︽

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/analysis/MPC_HC_GAP_ANALYSIS.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

## 2026-03-08 閺囧瓨鏌婇敍鍫熷⒔鐞涘苯鐣ч崚娆忓經瀵板嫬鎮撳銉礆



- 缂佹挸鎮庨張顒冪枂閹绘劒姘︽稉搴濇崲閸斺剝甯规潻娑€庢惔蹇ョ礉`5.1 WIP 闂勬劕鍩楅敍姘倱閺冩儼绻樼悰灞兼崲閸斺€茬瑝鐡掑懓绻?2 娑撶寯 閸欘垰鍨界€规矮璐熷陇鍐婚敍灞借嫙閸氬本顒為崟楣冣偓澶堚偓?
- `5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 鐏炵偘绨幐澶婃噯閹笛嗩攽閼哄倸顨旂痪锔芥将閿涘苯缍嬮崜宥勭波鎼存挷鑵戠亸姘卞繁閹镐胶鐢婚幀褏娈戦崨銊ф樊鎼达箒鐦夐幑顕嗙礉閺嗗倷绻氶幐浣规弓閸曢箖鈧鈧?


### 閸欙絽绶炵拠瀛樻

- `5.1` 閻ㄥ嫬鍨介弬顓濈贩閹诡喗妲搁張顒冪枂娴犺濮熼幐澶婂礋娑撹崵鍤庢稉鑼额攽閹恒劏绻橀敍姘絺鐢啴妫粋浣碘偓浣瑰絻娴犲墎閮寸紒鐔粹偓浣圭ウ婵帊缍嬬紓鎾冲暱閵嗕竸BR閵嗕線鍣风粙瀣暥閺嶅洨顒锋稉搴＄暓閸掓瑥褰涘鍕贩濞嗏剝鏁归崣锝冣偓?
- `5.2` 闂団偓鐟曚浇娉曢崨銊ｂ偓渚€鍣告径宥嗗⒔鐞涘瞼娈戞潻鍥┾柤鐠囦焦宓侀敍灞肩瑝闁倸鎮庢禒鍛殶瑜版挸澧犳稉鈧▎鈩冩暪閸欙絿绮ㄩ弸婊呮纯閹恒儱鍨界€规艾鐣幋鎰┾偓?


### 娴犺濮熷〒鍛礋閸氬本顒?
- `.monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md`

  - `5.1 WIP 闂勬劕鍩楅敍姘倱閺冩儼绻樼悰灞兼崲閸斺€茬瑝鐡掑懓绻?2 娑撶寯 閺嶅洩顔囩€瑰本鍨氶妴?
  - `5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 娣囨繃瀵斿鍛暚閹存劑鈧?


### 娣囶喗鏁奸弬鍥︽

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md

- docs/README.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md



## 闂傤噣顣?63: 閽€钘夋勾 5.2 閸涖劋绨查弨鑸垫殐閺冦儲澧界悰灞惧閸?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 鐎瑰牆鍨い?`5.2 濮ｅ繐鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑崇礄娣囶喖顦查妴浣告礀瑜版帇鈧焦鏋冨锝忕礆` 閻╊喖澧犻崣顏呮箒閸樼喎鍨幓蹇氬牚閿涘瞼宸辩亸鎴濆讲闁插秴顦查幍褑顢戦惃鍕ウ缁嬪鈧浇绔熼悾灞芥嫲鏉堟挸鍤悧鈺佺暰娑斿鈧?


### 閸掑棙鐎界拋鏉跨秿

1. `docs/plans/MPC_HC_ITERATION_PLAN.md` 閸欘亞绮伴崙琛♀偓婊勭槨閸涖劋绨查崶鍝勭暰閸嬫碍鏁归弫娑欐）閳ユ繄娈戦懞鍌氼殧瀵ら缚顔呴敍灞肩稻濞屸剝婀侀拃钘夌杽閹存劙鈧劖顒為幙宥勭稊閹靛鍞介妴?
2. `docs/workflows/REGRESSION_OPERATION_PLAYBOOK.md` 瀹歌尪顩惄鏍も偓婊冾洤娴ｆ洖浠涢崶鐐茬秺閳ユ繐绱濇担鍡氱箷濞屸剝婀侀崶鐐电摕閳ユ粌鎳嗘禍鏂跨秼婢垛晛鍘戠拋绋夸粵娴犫偓娑斿牄鈧胶顩﹀銏犱粵娴犫偓娑斿牄鈧椒绮堟稊鍫熸閸婃瑥褰叉禒銉ョ繁閸?RC 缂佹捁顔戦垾婵勨偓?
3. 閸ョ姵顒濊ぐ鎾冲閺堚偓閸氬牓鈧倻娈戦崝銊ょ稊閺勵垰鍘涢幎?`5.2` 閽€鑺ュ灇濞翠胶鈻奸弬鍥ㄣ€傞敍灞藉晙缁涘绶熼崥搴ｇ敾鐠恒劌鎳嗛幍褑顢戠拠浣瑰祦閸愬啿鐣鹃弰顖氭儊閸曢箖鈧鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`閿涘本妲戠涵顔兼噯閼哄倸顨旈妴浣告噯娴滄柨鍘戠拋?缁備焦顒涙禍瀣€嶉妴浣瑰⒔鐞涘矂銆庢惔蹇嬧偓浣瑰腹閼芥劕鎳℃禒銈冣偓浣界翻閸戣櫣澧挎稉搴″瑎闁褰涘鍕┾偓?
- 閺囧瓨鏌?`docs/README.md` 閸忋儱褰涢敍宀€鈥樻穱婵婎嚉濞翠胶鈻奸弬鍥ㄣ€傞崣顖滄纯閹恒儲顥呯槐顫偓?
- 閸氬本顒為悧鍫熸拱/閸欐ɑ娲?瀵偓閸欐垼顔囪ぐ鏇礉閺勫海鈥?`5.2` 瀹稿弶婀侀幍褑顢戦幍瀣斀閿涘奔绲炬禒璇插濞撳懎宕熸禒宥勭箽閹镐礁绶熺€瑰本鍨氶妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閺傜増鏋冨锝呭嚒鐟曞棛娲婇垾婊冩噯娑撯偓閼峰啿鎳嗛崶娑樼磻閸欐垯鈧礁鎳嗘禍鏂垮涧閸嬫碍鏁归弫娑掆偓婵堟畱閼哄倸顨旂拠瀛樻閵?
- 閺傜増鏋冨锝呭嚒閺勫海鈥橀垾婊冨帒鐠侀晲绨ㄦい?/ 缁備焦顒涙禍瀣€?/ 鏉堟挸鍤悧?/ RC 閸戝棗顦紒鎾诡啈閳ユ繄娈戦幍褑顢戞潏鍦櫕閵?
- 娴犺濮熷〒鍛礋 `5.2` 閺堫剚顐兼禒宥嗘弓閸曢箖鈧绱濆鍛倵缂侇厼鑸伴幋鎰硶閸涖劍澧界悰宀冪槈閹诡喖鎮楅崘宥呮礀閸愭瑣鈧?


### 娣囶喗鏁奸弬鍥︽

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂傤噣顣?64: 鐞涖儵缍?5.2 閻ｆ瑧妫斿Ο鈩冩緲閿涘潐aily_board / 閸涖劍濮ら敍?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- `5.2` 瀹稿弶婀佸ù浣衡柤閹靛鍞介敍灞肩稻鏉╂宸辩亸鎴濆讲閻╁瓨甯存繅顐㈠晸閻ㄥ嫧鈧粍妫╅惇瀣緲鐠佹澘缍嶉崡?+ 閸涖劍濮ゅΟ鈩冩緲閳ユ繐绱濇稉宥呭焺娴滃骸鎮楃紒顓熺焽濞ｂ偓鐠恒劌鎳嗛幍褑顢戠拠浣瑰祦閵?


### 閸掑棙鐎界拋鏉跨秿

1. 娴犲懏婀?`docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`閿涘矁绻曟稉宥堝喕娴犮儲鏁幘鎴炵槨閸涖劌鐤勯梽鍛⒔鐞涘本妞傞惃鍕秵閹存劖婀伴悾娆戞閵?
2. `daily_board.md` 瑜版挸澧犻崣顏呮箒娴犺濮熺€瑰甯撻敍灞剧梾閺堝璐熸稉銈勯嚋閸涖劋绨叉０鍕殌閸ュ搫鐣炬繅顐㈠晸娴ｅ秶鐤嗛妴?
3. 閸ョ姵顒濋棁鈧憰浣告倱閺冩儼藟姒绘劏鈧粌鎳嗘禍鏂跨秼閺冦儴顔囪ぐ鏇炲幢閳ユ繂鎷伴垾婊勭槨閸涖劍鐪归幀缁樐侀弶搴撯偓婵呰⒈娑擃亣娴囨担鎾扁偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺囧瓨鏌?`.monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md`閿涘奔璐?Day 5 / Day 10 婢х偛濮為弨鑸垫殐閺冦儴顔囪ぐ鏇炲幢閵?
- 閺傛澘顤?`.monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md`閿涘奔缍旀稉?`5.2` 閻ㄥ嫬鎳嗛幎銉ф殌閻ユ洘膩閺夎￥鈧?
- 閸氬本顒?`docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md`閵嗕梗docs/README.md` 娑撳海娴夐崗瀹狀唶瑜版洘鏋冨锝忕礉閺勫海鈥樻潻娆庤⒈娑擃亝膩閺夎法娈戦崗銉ュ經閸滃瞼鏁ら柅鏂烩偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `daily_board.md` 瀹告彃褰查惄瀛樺复婵夘偄鍟撻崨銊ょ安閻ㄥ嫯瀵栭崶鏉戝枙缂佹挶鈧礁娲栬ぐ鎺戞嚒娴犮們鈧攻locker 缂佹捁顔戦妴浣规瀮濡楋絽鎮撳銉ユ嫲闂冭埖顔岀紒鎾诡啈閵?
- `weekly_report_template.md` 瀹告彃褰查悽銊ょ艾婢跺秴鍩楅悽鐔稿灇濮ｅ繐鎳嗛崨銊﹀Г鐎圭偘绶ラ敍灞借嫙濞屽绌?`5.2` 閻ㄥ嫯娉曢崨銊ㄧ槈閹诡喓鈧?
- 娴犺濮熷〒鍛礋 `5.2` 閺堫剚顐兼禒宥嗘弓閸曢箖鈧绱濇禒鍛八夋鎰殌閻ユ洘膩閺夎￥鈧?


### 娣囶喗鏁奸弬鍥︽

- .monkeycode/specs/mpc-hc-alignment-iteration/daily_board.md

- .monkeycode/specs/mpc-hc-alignment-iteration/weekly_report_template.md

- .monkeycode/specs/mpc-hc-alignment-iteration/tasklist.md

- docs/workflows/WEEKLY_CONVERGENCE_PLAYBOOK.md

- docs/README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md





## 闂傤噣顣?65: 濮瑰洦鈧缍嬮崜宥呭閼冲鈧椒濞囬悽銊︽煙瀵繋绗屾宀冪槈閸忋儱褰?


**閺冦儲婀?*: 2026-03-08

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 瑜版挸澧犳禒鎾崇氨瀹歌尙绮￠崗宄邦槵鏉堝啫顦块幘顓熸杹閸ｃ劏鍏橀崝娑楃瑢妤犲本鏁归崨鎴掓姢閿涘奔绲鹃崝鐔诲厴濞撳懎宕熼妴浣峰▏閻劍鏌熷蹇撴嫲妤犲矁鐦夐崗銉ュ經閸掑棙鏆庨崷?`README`閵嗕梗reports`閵嗕梗tasklist` 娑?`main.cpp` 鐢喖濮潏鎾冲毉娑擃叏绱濈紓鍝勭毌娑撯偓娴犵晫绮烘稉鈧幀鏄忣潔閵?


### 閸掑棙鐎界拋鏉跨秿

1. 閻劍鍩涢崣顖滄纯閹恒儰濞囬悽銊ф畱閸旂喕鍏橀敍灞肩瑢瀵偓閸?妤犲本鏁归崥鎴濇嚒娴犮倖璐╅弶鍌氭躬婢舵矮閲滈弬鍥ㄣ€傞崪灞剧爱閻礁搴滈崝鈺勭翻閸戣桨鑵戦敍灞肩瑝閸掆晙绨箛顐︹偓鐔烘倞鐟欙絺鈧粎鈻兼惔蹇曞箛閸︺劌鍩屾惔鏇″厴閸嬫矮绮堟稊鍫氣偓婵勨偓?
2. 閺?`README.md` 閸欘亙绻氶悾娆庣啊鏉堝啯妫張鐔烘畱閸旂喕鍏樻稉搴濆▏閻劎銇氭笟瀣剁礉閺冪姵纭剁€瑰本鏆ｇ憰鍡欐磰瑜版挸澧犳稉鑽ゅ殠閼宠棄濮忛妴?
3. 閸ョ姵顒濋棁鈧憰浣规煀婢х偘绔存禒鑺モ偓鏄忣潔閺傚洦銆傞敍灞惧Ω閳ユ粌濮涢懗濮愨偓浣烘暏濞夋洏鈧線鐛欑拠浣测偓婵嬫肠娑擃厽鏆ｉ悶鍡礉楠炶泛鎮撳銉ュ弳閸欙絿鍌ㄥ鏇樷偓?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md`閿涘瞼閮寸紒鐔告殻閻炲棗缍嬮崜宥呭閼冲鈧椒濞囬悽銊︽煙瀵繈鈧線鍘ょ純顔芥煙瀵繈鈧線鐛欑拠浣告嚒娴犮倓绗岄幎銉ユ啞閺勭姴鐨犻妴?
- 閺囧瓨鏌?`docs/README.md`閿涘奔璐熺拠銉︹偓鏄忣潔閺傚洦銆傛晶鐐插閸忋儱褰涢敍灞借嫙閸︺劍娲块弬鏉垮坊閸欒弓鑵戠拋鎷屽閵?
- 閺囧瓨鏌婇弽?`README.md`閿涘矁藟閸忓懎鍩岃ぐ鎾冲閹槒顫嶉弬鍥ㄣ€傞惃鍕弳閸欙綇绱濋柆鍨帳娴ｈ法鏁ょ拠瀛樻缂佈呯敾閸嬫粎鏆€閸︺劍妫崣锝呯窞閵?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- 閺傜増鏋冨锝呭嚒鐟曞棛娲婇垾婊呮暏閹村嘲褰查惄瀛樺复娴ｈ法鏁ら崝鐔诲厴 / 瀵偓閸欐垿鐛欓弨鎯板厴閸?/ 娴ｈ法鏁ら弬鐟扮础 / 妤犲矁鐦夊ù浣衡柤 / 瑜版挸澧犳潏鍦櫕閳ユ縿鈧?
- 閺傚洦銆傛稉顓炲嚒缂佹瑥鍤?`--capabilities`閵嗕梗--probe-file`閵嗕梗--evaluate-target` 娴犮儱寮锋稉鎾汇€嶆灞炬暪閸涙垝鎶ら惃鍕埠娑撯偓閸忋儱褰涢妴?
- 瑜版挸澧犳禒鎾崇氨濞屸剝婀侀弬鏉款杻娴狅絿鐖滈弨鐟板З閿涘本婀板▎鈥冲涧鐞涖儵缍堥崝鐔诲厴娑撳簼濞囬悽銊嚛閺勫孩鏋冨锝冣偓?


### 娣囶喗鏁奸弬鍥︽

- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md

- docs/README.md

- README.md

- docs/records/VERSION.md

- docs/records/CHANGELOG.md

- docs/records/DEVELOP_LOG.md







## 闂傤噣顣?70: PlayerCore 閻樿埖鈧焦婧€闁插秷顔曠拋锛勵儑娑撯偓闂冭埖顔?


**閺冦儲婀?*: 2026-03-19

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴閸忓牆浠涢幘顓熸杹閸ｃ劌鍞撮弽鍝ュЦ閹焦婧€闁插秷顔曠拋锛勵儑娑撯偓闂冭埖顔岄敍灞肩瑝閸?serial閵嗕椒绗夐崝?copy-back/SoftwareSDL閿涘奔绗夐弨鐟邦樆闁?`PlaybackState` 閹恒儱褰涢妴?
- 閻╊喗鐖ｉ弰顖涘Ω UI 閹绢厽鏂侀幀浣告嫲閸愬懏鐗虫导姘崇樈/濞翠焦鎸夌痪鎸庘偓浣瑰瀵偓閿涘苯鑻熼幎濠冩殠閽€钘夋躬 `PlayerCore` 閸氬嫬鍙嗛崣锝夊櫡閻ㄥ嫮濮搁幀浣规暭閸愭瑦鏁归崣锝呭煂缂佺喍绔?transition 閸忋儱褰涢妴?


### 閸掑棙鐎界拋鏉跨秿

1. 瑜版挸澧?`PlayerCore` 閻?`open / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 闁垝绱伴惄瀛樺复閸?`state_`閿涘瞼濮搁幀浣稿綁閸栨牗鐥呴張澶婂礋娑撯偓閸忋儱褰涢妴?
2. `deferred stop` 娑斿澧犻弰顖滃缁斿绔风亸鏃囶嚔娑斿绱濈€佃壈鍤ф潻鎰攽閹礁鎷?deferred stop 閺冧浇鐭鹃幀浣稿瀻鐟佸倶鈧?
3. `Scheduler` 閻╊喖澧犻崣顏呮箒 `running_ / paused_`閿涘瞼顑囨稉鈧梼鑸殿唽娑撳秴鐤佹潻鍥ㄦ－婵夌偛鍙嗛弴鏉戭樋娑撴艾濮熺拠顓濈疅閿涘苯绨茬拠銉ュ帥閻?`PlayerCore` 缂佺喍绔寸紒瀛樺Б娴兼俺鐦介幀浣碘偓浣界箥鐞涘本鈧礁鎷板ù浣规寜缁炬寧鈧降鈧?


### 鐟欙絽鍠呴弬瑙勵攳

- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `SessionState / RunState / PipelinePhase` 娑?`CoreStateSnapshot`閵?
- 閺傛澘顤?`transitionSessionState / transitionRunState / transitionPipelinePhase / publishPlaybackStateFromInternalState`閿涘苯鑻熼崝鐘插弳闂堢偞纭舵潻浣盒╂穱婵囧Б閸滃瞼濮搁幀浣界讣缁夌粯妫╄箛妞尖偓?
- 鐏?`eof_reached / pending_seek / deferred_stop_pending` 閺€璺烘礀閸掓壆绮烘稉鈧箛顐ゅ弾缁狅紕鎮婇妴?
- 閹?`open / close / play / pause / stop / seek / requestDeferredStop / serviceDeferredStop / onRenderIdle` 閺€閫涜礋閸欘亪鈧俺绻?transition helper 閸欐ɑ娲块悩鑸碘偓渚婄幢鐎电懓顦?`PlaybackState` 娴犲秳绻氶幐浣稿悑鐎瑰箍鈧?
- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€?`docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md`閿涘矁顔囪ぐ鏇狀儑娑撯偓闂冭埖顔屾潏鍦櫕閵嗕讣cheduler 婢跺嫮鎮婄紒鎾诡啈閸滃奔绗呮稉鈧梼鑸殿唽 serial 閸栨牕缂撶拋顔衡偓?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?


### 娣囶喗鏁奸弬鍥︽

- include/core/player_core.h

- src/core/player_core.cpp

- docs/analysis/PLAYERCORE_DAY16_STATE_MACHINE_REDESIGN.md

- docs/records/CHANGELOG.md

- docs/records/VERSION.md

- docs/records/DEVELOP_LOG.md



## 闂傤噣顣?81: PlayerCore seek/flush timeline serial 閸栨牜顑囨禍宀勬▉濞?


**閺冦儲婀?*: 2026-03-20

**閻樿埖鈧?*: 瀹歌尪袙閸?


### 闂傤噣顣介幓蹇氬牚

- 閻劍鍩涚憰浣圭湴缂佈呯敾閸嬫碍鎸遍弨鎯ф珤閸愬懏鐗抽悩鑸碘偓浣规簚闁插秷顔曠拋锛勵儑娴滃矂妯佸▓纰夌礉娴ｅ棙妲戠涵顔荤瑝閸?copy-back閵嗕讣oftwareSDL閵嗕箒I 鐏炲倸鎷版径鏍劥 `PlaybackState`閿涘苯褰ф径鍕倞 seek/flush 閻?timeline serial 閸栨牓鈧?
- 閻╊喗鐖ｉ弰顖濐唨 seek/stop/deferred stop 娑斿鎮楅惃鍕＋ packet閵嗕焦妫憴鍡涱暥鐢佲偓浣规＋闂婃娊顣剁敮褍宓嗘担鎸庢珓閸掑府绱濇稊鐔风箑妞よ娲滄稉?serial 娑撳秴灏柊宥堚偓宀冾潶绾兛娑鍐︹偓?


### 閸掑棙鐎界拋鏉跨秿

1. `ThreadSafeQueue` 閸?`FrameQueue` 瑜版挸澧犻柈钘夊涧閺?stop/clear/flush 鐠囶厺绠熼敍灞剧梾閺?generation/serial閿涙稒妫弮鍫曟？缁炬寧鏆熼幑顔筋劃閸撳秴褰ч懗浠嬫浆閳ユ粏顫﹂崣濠冩濞撳懐鈹栭垾婵娾偓灞肩瑝閺勵垪鈧粏顫﹂弰搴ｂ€橀崚銈呯熬閳ユ縿鈧?
2. seek 娑斿澧犳稉鏄忣洣娓氭繆绂?`scheduler pause + stopDemuxThread + flushPipelines + avcodec_flush_buffers() + audio_player_->stop()`閿涘畮udio consumer 缁捐法鈻奸張顒冮煩娑撳秴鐢弮鍫曟？缁惧灝鍨界€规熬绱漴ender 鐠侯垰绶炴稊鐔哥梾閺?serial 闂冭尙鍤庨妴?
3. 缁楊兛绨╅梼鑸殿唽閺堚偓缁嬪啿螘閻ㄥ嫯鎯ら悙鐧哥礉閺勵垯绻氶幐?queue 鐎圭懓娅掗柅姘辨暏閿涘本鏁兼稉楦款唨濮ｅ繋閲?packet/frame 閼奉亜鐢?serial閿涘苯鑻熼悽?`PlayerCore` 闂嗗棔鑵戦崚鍡涘帳閸滃本绺哄ú?serial閵?


### 鐟欙絽鍠呴弬瑙勵攳

- 閺傛澘顤?`TimelineSerial` 娑?`DemuxPacket`閿涘苯鑻熸稉?`VideoFrame / AudioFrame` 婢х偛濮?`serial` 鐎涙顔岄妴?
- 閸?`PlayerCore` 閸愬懘鍎撮弬鏉款杻 `timeline_serial / pending_seek_serial` 閸欏﹦绮烘稉鈧?helper閿涙瓪allocateNextTimelineSerial / activateTimelineSerial / setPendingSeekSerial / isAcceptedTimelineSerial`閵?
- `open` 閹存劕濮涢弮璺虹紦缁斿顩绘稉?serial閿涙矖seek` 閸忓牆鍨忛崚?pending serial閿涘苯鍟€閸︺劍鍨氶崝鐔锋倵濠碘偓濞蹭紮绱盽stop / requestDeferredStop` 娴兼氨鐝涢崡铏腹鏉?serial閿涘瞼鈥樻穱婵囨＋ worker 閺呮艾鍩岄弮璺哄涧閼虫垝楠囬崙鍝勭熬閺佺増宓侀妴?
- demux 缁捐法鈻奸崥顖氬З閺冭埖宕熼懢?serial閿涘畳ecode/render/audio consumer 閸忋劑鎽肩捄顖氫粵 stale serial 娑撱垹绱旈敍娌漣agnostics 閸滃奔绗撴い瑙勵梾閺屻儱鎳℃禒銈咁嚤閸?serial 鐟欏倹绁寸€涙顔岄妴?


### 閺堫剙婀存灞炬暪缂佹挻鐏?
- `C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe build\modern-video-player.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /m`閿涙岸鈧俺绻冮妴?


### 娣囶喗鏁奸弬鍥︽

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

## 闂傤噣顣?84: PlayerCore 閸擃垯缍旈悽銊╂肠娑擃厼瀵叉稉?runtime failure/recovery policy 閺€璺哄經

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- `PlayerCore` 閻ㄥ嫬顦婚柈銊ュ弳閸欙絽鍑＄紒蹇撶暚閹存劗濮搁幀浣界讣缁夌粯鏁归崣锝忕礉娴ｅ棛鍤庣粙瀣ㄢ偓浣筋啎婢跺洢鈧線妲﹂崚妤€鎷伴弮鍫曟寭閸擃垯缍旈悽銊ょ矝閺侊綀鎯ら崷銊ヮ樋娑擃亜鍙嗛崣锝呭毐閺佷即鍣烽妴?- `SchedulerControlSnapshot` 鏉╂宸?`clock_source`閵嗕工udio-master 缁撅附娼崪?ended policy閿涘cheduler 娴犲秷顩﹂懛顏囶攽閹恒劍鏌囨稉姘鐠囶厺绠熼妴?- decode/resample/output 閻?fatal 閻愬湱宸辩亸鎴犵埠娑撯偓 recovery policy閿涘苯顔愰弰鎾诲櫢閺備即鏆遍崙鍝勫瀻閺侊絿娈戦柨娆掝嚖婢跺嫮鎮婇妴?
### 閸掑棙鐎界拋鏉跨秿
1. 閻樿埖鈧浇绺肩粔濠氭肠娑擃厺绠ｉ崥搴礉娑撳绔寸仦鍌氱箑妞よ崵鎴风紒顓㈡肠娑擃厾娈戦弰?side effects閿涙稑鎯侀崚娆忓弳閸欙絽鍤遍弫棰佺矝閻掓湹绱伴幋鎰礋閳ユ粎濮搁幀浣规簚 + worker 缁狅紕鎮婇崳銊⑩偓婵堟畱濞ｅ嘲鎮庢担鎾扁偓?2. `deferred stop` 閻ㄥ嫭婀扮拹銊︽Ц瀵倹顒?stop completion閿涘矁鈧奔绗夐弰顖涙煀閻ㄥ嫪绗熼崝锛勫Ц閹緤绱遍崶鐘愁劃 request/completion side effects 韫囧懘銆忛崗杈╂暏娑撯偓婵?helper閵?3. scheduler 瀹歌尙绮￠幏銉︽箒 `run_state / pipeline_phase / accepted_timeline_serial`閿涘本婀版潪顔炬埛缂侇厽濡?`clock_source`閵嗕工udio-master 閸?ended policy 缂佹挻鐎崠鏍电礉閼充粙浼╅崗宥呭晙閸旂姵鏌婇惃鍕祩閺侊絽绔风亸鏂剧秴閵?4. runtime failure 閸忓牏绮烘稉鈧幋?`FailureRecoveryPolicy` 閸氬函绱濋崥搴ｇ敾閹碘晛銇?fatal 閻愮顩惄鏍瘱閸ュ瓨妞傛稉宥夋付鐟曚線鍣搁弬鐗堝閹垹顦茬捄顖氱窞閵?
### 婢跺嫮鎮婄紒鎾寸亯
- 閺傛澘顤?`applyStartPlaybackSideEffects / applyResumePlaybackSideEffects / applyPausePlaybackSideEffects / applyStopRequestSideEffects / applyStopCompletionSideEffects / applySeekSideEffects / applySessionReleaseSideEffects`閵?- `requestDeferredStop()` 娑?`serviceDeferredStop()` 瀹告彃顦查悽?stop request/completion side effects閿涘畳eferred stop 娑撳秴鍟€缂佸瓨濮㈤悪顒傜彌閸嬫粍婧€闁槒绶妴?- `SchedulerControlSnapshot` 瀹稿弶鏌婃晶?`clock_source`閵嗕梗audio_output_initialized`閵嗕梗audio_master_sync_active`閵嗕梗ended_policy`閿涘cheduler render wait 闁槒绶鍙夊复閸忋儴绻栨禍娑樼摟濞堢偣鈧?- 閺傛澘顤?`FailureRecoveryPolicy` 娑?`handleRuntimeFailure()`閿涘苯鑻熼幎濠咁潒妫?闂婃娊顣?decode-resample 闁惧彞绗傞惃鍕彠闁?fatal 閻愬湱绮烘稉鈧弨璺哄經閸掓媽顕氶崗銉ュ經閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?- 閺堫亝鏁?UI 鐏炲倸鎷版径鏍劥 `PlaybackState`閿涘本婀版潪顔煎涧閺€璺哄經閸愬懏鐗抽崜顖欑稊閻劌鎷?runtime recovery 缁涙牜鏆愰妴?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- include/core/scheduler.h
- src/core/scheduler.cpp
- docs/analysis/PLAYERCORE_DAY20_SIDE_EFFECTS_AND_RUNTIME_FAILURE_REDESIGN.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?85: PlayerCore 閸撯晙缍戞搴ㄦ珦閺€鑸垫殐閿涙瓔cheduler 缂佸牏澧楃粵鏍殣閵嗕笚ailSession 鐎圭偛瀵叉稉?serial/generation 鐟欏倹绁村鍝勫

**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- `SchedulerControlSnapshot` 闂団偓鐟曚胶鎴风紒顓犵波閺嬪嫬瀵?ended policy / clock policy / audio-master 缁撅附娼妴?- `FailSession` 闂団偓鐟曚椒绮犵紒鐔剁閸忋儱褰涢幒銊ㄧ箻閸掓澘鍙ч柨顔跨箥鐞涘本妞傞柨娆掝嚖閻ㄥ嫬鐤勯梽鍛邦洬閻╂牓鈧?- queue generation 娑?item-level serial 闂団偓鐟曚焦娲块惄纾嬵潎閻ㄥ嫬褰茬憴鍌涚ゴ鏉堝湱鏅妴?
### 閸掑棙鐎界拋鏉跨秿
1. 閼?scheduler 娴犲秳绶风挧鏍х鐏忔梹瀚剧拠顓濈疅閿涘瞼鐡ラ悾銉﹀壈閸ュ彞绱扮紒褏鐢婚崚鍡樻殠閵?2. 閼?`FailSession` 閸欘亜浠犻悾娆忔躬閸忋儱褰涚仦鍌︾礉闁洤鍩屾稉宥呭讲閹垹顦查柨娆掝嚖閺冭埖浠径宥堢熅瀵板嫪绮涙稉宥勭閼锋番鈧?3. generation 鐟欙絽鍠呴惃鍕Ц鐎圭懓娅掗崬銈夊晪閸滃矁绔熼悾灞艰厬閺傤叏绱濇稉宥嗘禌娴?serial 閻ㄥ嫭妞傞梻瀵稿殠閸掋倕鐣鹃妴?
### 婢跺嫮鎮婄紒鎾寸亯
- `SchedulerControlSnapshot` 瀹稿弶鏌婃晶?`clock_policy`閵嗕梗audio_master_policy`閵嗕梗audio_buffered_seconds`閿涘苯鑻熼幍鈺佺潔 ended policy閵?- `Scheduler` 瀹稿弶鏁兼稉铏圭摜閻ｃ儵鈹嶉崝?render wait閿涙矖Scheduler::stop()` 婢х偛濮?self-join 娣囨繃濮㈤妴?- `FailSession` 瀹歌尪顩惄鏍у彠闁款喕绗夐崣顖涗划婢跺秹鏁婄拠顖滃仯楠炴儼鎯ら崚鎵埂鐎圭偠鐭惧鍕┾偓?- `DiagnosticsSnapshot` 閺傛澘顤?stale serial drop 鐠佲剝鏆熸稉?runtime failure 鐠佲剝鏆熼敍灞借嫙閹恒儱鍙嗗Λ鈧弻銉ユ嚒娴犮倛绶崙鎭掆偓?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/scheduler.h
- src/core/scheduler.cpp
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY21_REMAINING_RISKS_CONVERGENCE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md
## 闂傤噣顣?86: serial/failsession 閸ョ偛缍婂Λ鈧弻銉ㄋ夋鎰剁礄鏉╃偟鐢?seek閵嗕焦娈忛崑婊勨偓?seek閵嗕恭lose-reopen閿?
**閺冦儲婀?*: 2026-03-20
**閻樿埖鈧?*: 瀹歌尪袙閸?
### 闂傤噣顣介幓蹇氬牚
- 闂団偓鐟曚浇藟閸忓懍绗撻悽?CLI 閸ョ偛缍婇幒銏ゆ嫛閿涘矂鐛欑拠?serial 鏉堝湱鏅稉?FailSession 缁撅附娼崷銊ュ彠闁款喖婧€閺咁垯绗呴弮鐘叉礀瑜版帇鈧?
### 閸掑棙鐎界拋鏉跨秿
1. 閻滅増婀?performance/software 濡偓閺屻儱浜搁柧鎹愮熅閸嬨儱鎮嶉敍灞肩瑝閻╁瓨甯寸憰鍡欐磰娑撳琚悩鑸碘偓浣界珶閻ｅ被鈧?2. 闂堢偞纭堕悩鑸碘偓浣界讣缁夌粯顒濋崜宥呭涧閸︺劍妫╄箛妤€褰茬憴渚婄礉娑撳秴鍩勬禍搴㈡簚閸?gate閵?3. 閸ョ偛缍婇崨鎴掓姢闂団偓缂佺喍绔?`key=value` 娑?`result=PASS/FAIL`閿涘奔绌舵禍搴ゅ殰閸斻劌瀵插☉鍫ｅ瀭閵?
### 婢跺嫮鎮婄紒鎾寸亯
- `DiagnosticsSnapshot` 閺傛澘顤?`illegal_session_transitions / illegal_run_transitions / illegal_pipeline_transitions`閿涘苯鑻熺€瑰本鍨氱拋鈩冩殶娑撳酣鍣哥純顔藉复閸忋儯鈧?- 閺傛澘顤?CLI 閸涙垝鎶ら敍?  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--close-reopen-serial-check`
- `--performance-log-check` 娑?`--software-video-decode-check` 婢х偛濮為棃鐐寸《鏉╀胶些鐠佲剝鏆熺€涙顔屾潏鎾冲毉閵?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md`閵?
### 閺堫剙婀存灞炬暪缂佹挻鐏?- Debug 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?- 閺傛澘顤?3 娑擃亝顥呴弻銉ユ嚒娴犮倕婀?`juren-30s.mp4` 娑撳﹤娼庢潻鏂挎礀 `PASS`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY22_SERIAL_FAILSESSION_REGRESSION_CHECKS.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?87: serial/failsession 閸ョ偛缍婃晶鐐插娑撯偓闁款喛浠涢崥?gate閿涘牓妾锋担搴㈢础鐠烘垿顥撻梽鈺嬬礆

- `main` 閺傛澘顤冮懕姘値閸涙垝鎶ら敍?  - `--serial-failsession-regression-check <media_file> [seek_count] [paused_seek_count] [sample_ms]`
- 閼辨艾鎮庨崨鎴掓姢閸愬懘鍎存い鍝勭碍閹笛嗩攽閿?  - `--seek-burst-serial-check`
  - `--paused-seek-serial-check`
  - `--close-reopen-serial-check`
- 閺傛澘顤冮張鍝勬珤閸欘垵顕伴懕姘値鏉堟挸鍤敍?  - `serial-failsession-regression-check.seek_burst_ok`
  - `serial-failsession-regression-check.paused_seek_ok`
  - `serial-failsession-regression-check.close_reopen_ok`
  - `serial-failsession-regression-check.pass_count`
  - `serial-failsession-regression-check.total_count`
  - `serial-failsession-regression-check.result`
- 閺堫剙婀存宀冪槈閿?  - Debug 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`閿涙瓪PASS`閿涘潉pass_count=3/3`閿涘鈧?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md`閵?
### 娣囶喗鏁奸弬鍥︽
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY23_SERIAL_FAILSESSION_AGGREGATE_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?88: 瀵搫鍩?FailSession 閸ョ偛缍婇幒銏ゆ嫛娑?codec 闁夸線鍣搁崗銉ョ┛濠у啩鎱ㄦ径?
- 閺傛澘顤?`--forced-failsession-check <media_file> [sample_ms]`閿涘矂鈧俺绻冨ù瀣槸濞夈劌鍙嗙粙鍐茬暰鐟曞棛娲?`FailSession` 鐠侯垰绶為敍灞借嫙鏉堟挸鍤張鍝勬珤閸欘垵顕?`key=value` + `result=PASS/FAIL`閵?- `PlayerCore::decodeVideoFrame` 婢х偛濮?`MVP_FORCE_FAIL_SESSION_ON_VIDEO_DECODE` 濞村鐦▔銊ュ弳瀵偓閸忕绱濋悽銊ょ艾瀵搫鍩楁潻娑樺弳 `FailureRecoveryPolicy::FailSession`閵?- 娣囶喖顦?`FailSession` 娴犲氦袙閻胶鍤庣粙瀣箻閸忋儲妞傞惃?codec 闁夸線鍣搁崗銉ョ┛濠у喛绱?  - `video_codec_mutex_`閵嗕梗audio_codec_mutex_` 鐠嬪啯鏆ｆ稉?`std::recursive_mutex`閿?  - `decodeVideoFrame/decodeAudioFrame` 閻?`lock_guard` 缁鐎烽崥灞绢劄鐠嬪啯鏆ｉ妴?- 閺堫剙婀存宀冪槈閿?  - Debug 閺嬪嫬缂撻柅姘崇箖閿涘畭0 warnings / 0 errors`閵?  - `build\Debug\modern-video-player.exe --forced-failsession-check .\juren-30s.mp4 2200`閿涙瓪PASS`閵?  - `build\Debug\modern-video-player.exe --serial-failsession-regression-check .\juren-30s.mp4`閿涙瓪PASS`閵?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md`閵?
### 娣囶喗鏁奸弬鍥︽
- include/core/player_core.h
- src/core/player_core.cpp
- src/main.cpp
- docs/analysis/PLAYERCORE_DAY24_FORCED_FAILSESSION_AND_CODEC_LOCK_REENTRANCY_FIX.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 闂傤噣顣?89: run_all_checks 閹恒儱鍙?forced-failsession 娑撯偓闁?gate

- `tools/run_all_checks.ps1` 閺傛澘顤冮崣鍌涙殶閿?  - `ForcedFailSessionFile`閿涘牓绮拋銈咁槻閻?`ProbeFile`閿?  - `ForcedFailSessionSampleMs`閿涘牓绮拋?`2200`閿?- 閸ョ偛缍婂ù浣衡柤娴?2 濮濄儲澧挎稉?3 濮濄儻绱?  1. `[1/3]` `--probe-file --json`
  2. `[2/3]` `--forced-failsession-check`
  3. `[3/3]` `run_format_regression.ps1`
- gate 鐟欏嫬鍨晶鐐插繁閿?  - probe 婢惰精瑙﹂惄瀛樺复闁偓閸戠尨绱?  - forced-failsession 婢惰精瑙﹂惄瀛樺复闁偓閸戝搫鑻熺捄瀹犵箖 format regression閵?- 閺堫剙婀存宀冪槈閿?  - `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1 -ExecutablePath "build/Debug/modern-video-player.exe" -ProbeFile "juren-30s.mp4" -ForcedFailSessionSampleMs 2200`
  - `probe/forced-failsession/regression` 闁偓閸戣櫣鐖滈崸鍥﹁礋 `0`閿涘矁鍓奸張顒佲偓濠氣偓鈧崙铏圭垳 `0`閵?- 閺傛澘顤冮崚鍡樼€介弬鍥ㄣ€傞敍姝歞ocs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md`閵?
### 娣囶喗鏁奸弬鍥︽
- tools/run_all_checks.ps1
- docs/analysis/PLAYERCORE_DAY25_RUN_ALL_CHECKS_FORCED_FAILSESSION_GATE.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md



## 闂 90: OpenGL 鍘熺敓 D3D11 浜掓搷浣滃仠姝㈡湡寮傚父閫€鍑轰笌浣庡悶鍚愪慨澶?**鏃ユ湡**: 2026-03-24
**鐘舵€?*: 宸茶В鍐?### 闂鎻忚堪
- `MVP_RENDERER_BACKEND=opengl` 涓嬪凡杩涘叆 `D3D11VA -> OpenGL` 鍘熺敓浜掓搷浣滆矾寰勶紝浣?`--performance-log-check .\juren-30s.mp4 2000` 鍦?stop/close 闃舵寮傚父閫€鍑猴紝鏈緭鍑烘渶缁?`result`銆?- 鍚屾椂鍑虹幇鍘熺敓璺緞鍚炲悙寮傚父鍋忎綆锛歚video_native_output_frames=62` 涔嬪墠鍙兘绋冲畾娓叉煋绾?3 甯э紝琛ㄧ幇涓?stop 鍓嶅崱椤垮拰閫€鍑轰笉绋冲畾銆?### 鍒嗘瀽璁板綍
1. OpenGL 鍘熺敓璺緞浣跨敤 renderer-owned D3D11 device锛孎Fmpeg 纭В绾跨▼涓?OpenGL 娓叉煋绾跨▼鍏变韩鍚屼竴 D3D11 immediate context銆?2. 璇ヨ澶囧垱寤烘椂鏈紑鍚?`ID3D11Multithread::SetMultithreadProtected(TRUE)`锛屽鑷村苟鍙戣闂笉瀹夊叏锛屽嚭鐜?native 璺緞鍗￠】銆乻top 闃舵寮傚父閫€鍑恒€?3. `PlayerCore::applySessionReleaseSideEffects()` 鍘熷厛鍏堥噴鏀?decoder/hw context锛屽啀鍏抽棴 renderer锛涜繖浼氳浠嶆寔鏈?`AV_PIX_FMT_D3D11` 甯у紩鐢ㄧ殑缂撳瓨甯?娓叉煋绾跨▼澶勪簬鍗遍櫓閲婃斁椤哄簭銆?### 澶勭悊缁撴灉
- `src/render/opengl_video_renderer.cpp`
  - 涓?OpenGL 鍘熺敓浜掓搷浣滀娇鐢ㄧ殑 D3D11 璁惧琛ラ綈 `ID3D11Multithread` 澶氱嚎绋嬩繚鎶ゃ€?  - 淇濇寔 `D3D11VA -> D3D11 shader color convert -> WGL_NV_DX_interop -> OpenGL draw` 鍘熺敓璺緞缁х画宸ヤ綔銆?- `src/core/player_core.cpp`
  - 璋冩暣 session release 椤哄簭锛氬厛閲婃斁缂撳瓨 native frame 骞跺叧闂?renderer锛屽啀閲婃斁 decoder/hw context銆?- Release 鏈湴楠岃瘉锛?  - `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 2000`
  - 缁撴灉锛歚renderer_backend=OpenGL`銆乣decoder_backend=D3D11VA`銆乣video_native_output_frames=62`銆乣video_copy_back_frames=0`銆乣render_frames=47`銆乣result=PASS`銆佽繘绋嬮€€鍑虹爜 `0`銆?### 淇敼鏂囦欢
- src/render/opengl_video_renderer.cpp
- src/core/player_core.cpp
- docs/reports/OPENGL_RENDERER_LOCAL_CHECK.md
- docs/guides/PLAYER_FEATURES_USAGE_VALIDATION.md
- docs/records/CHANGELOG.md
- docs/records/VERSION.md
- docs/records/DEVELOP_LOG.md

## 2026-03-24 OpenGL ASS 鏍峰紡閾捐矾澧炲己
- 瀛楀箷妯″瀷鏂板锛歚wrap_style`銆乣spacing`銆乣scale_x_percent`銆乣scale_y_percent`銆乣rotation_degrees`銆乣outline_x/y`銆乣shadow_x/y`銆?- `ASS parser` 鏂板 `WrapStyle`銆乻tyle 瀛楁涓?`\q/\fsp/\fscx/\fscy/\fr/\frz/\xbord/\ybord/\xshad/\yshad` 鏀寔銆?- OpenGL/D3D11 GPU 瀛楀箷娓叉煋宸叉秷璐逛笂杩板瓧娈碉細`DirectWrite character spacing + D2D transform + x/y outline/shadow`銆?- 鏂板 `--subtitle-style-check` 鏍峰紡璇婃柇 CLI銆?- 鏂板鍥炲綊鏍锋湰锛歚samples/subtitles/opengl_ass_style_validation.ass`銆?- 鏈湴楠岃瘉閫氳繃锛歚subtitle-style-check`銆乣subtitle-sync-check`銆乣opengl delay-adjust-check`銆乣opengl-diagnostics` 鍏ㄩ儴 `PASS`銆?
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
### 2026-03-25 鏇存柊锛歄penGL 鍚姩鍗℃鏀舵暃鍒?WASAPI 榛樿绔偣闃诲
- Windows 闊抽杈撳嚭鏂板榛樿 render endpoint preflight锛氶粯璁?`wasapi` 杈撳嚭涓旀病鏈夐粯璁ゆ垨 active render endpoint 鏃讹紝涓嶅啀杩涘叆闃诲鐨?`SDL_OpenAudioDevice(nullptr, ...)`锛岀洿鎺ユ部鐢ㄧ幇鏈?`video-only fallback`銆?- `DiagnosticsSnapshot` 鍜屾挱鏀剧被妫€鏌ュ懡浠ゆ柊澧烇細
  - `audio_device_open_attempted`
  - `audio_init_latency_ms`
  - `audio_init_strategy`
  - `audio_init_detail`
- OpenGL 淇鍚庢湰鏈洪獙璇侊細
  - `$env:MVP_RENDERER_BACKEND='opengl'; .\build\Release\modern-video-player.exe --performance-log-check .\juren-30s.mp4 1200`
  - `audio_device_open_attempted=false`
  - `audio_init_strategy=skip-no-default-render-endpoint`
  - `result=PASS`
- `tools/run_opengl_checks.ps1`锛歚16/16 PASS`
### 2026-03-25 Update: Embedded subtitle multi-track selection UI + CLI
- Added OpenGL subtitle-track controls in bottom bar (previous/next track + `current / total` state).
- Added playback argument `--subtitle-track <stream_index>` to set preferred embedded subtitle stream before media open.
- Added machine-readable diagnostics:
  - `--embedded-subtitle-list <media_file>`
  - `--embedded-subtitle-select-check <media_file> <stream_index>`
- `PlayerCore` subtitle track overlay state now counts selectable supported subtitle tracks (`supported_codec`), not raw stream count.
- Validation: Release build PASS, embedded subtitle list/select checks PASS, OpenGL gate PASS (`16/16`).

### 2026-03-25 Update: Embedded bitmap subtitle path and DirectWrite custom font collection
- Embedded subtitle selection and track-state policy now use `supported_codec` (text + bitmap), not text-only assumptions.
- Added embedded bitmap subtitle decode support for `hdmv_pgs_subtitle` and `dvd_subtitle`, including subtitle bitmap model payload (`SubtitleBitmap`) and item-level bitmap fields.
- Added bitmap subtitle rendering branches in OpenGL and D3D11 subtitle renderers via D2D bitmap composition.
- Added DirectWrite custom subtitle font collection builder from registered private subtitle fonts and applied it in both OpenGL and D3D11 subtitle text paths before system fallback.
- Added and extended diagnostics:
  - `--directwrite-font-collection-check <media_file>`
  - `embedded-subtitle-check.bitmap_codec` / `embedded-subtitle-check.bitmap_item_count`
  - `embedded-subtitle-list.supported_bitmap_track_count`
  - `embedded-subtitle-select-check.bitmap_codec` / `embedded-subtitle-select-check.bitmap_item_count`
- Validation: Release build PASS, embedded subtitle check/list/select PASS, directwrite-font-collection-check PASS, OpenGL gate PASS (`16/16`).

### 2026-03-25 Update: OpenGL HDR output policy + 3D LUT output baseline
- Added OpenGL output policy controls:
  - env `MVP_OPENGL_HDR_OUTPUT_MODE=auto|off|force`
  - CLI `--opengl-hdr-output-mode <auto|off|force>`
- Added OpenGL 3D LUT output controls:
  - env `MVP_OPENGL_3DLUT_FILE=<cube_lut_file>`
  - CLI `--opengl-3dlut <cube_lut_file>`
- Added `.cube` parser + OpenGL 3D LUT upload/sampling in output-stage shading.
- Extended diagnostics output:
  - `renderer_opengl_hdr_bridge_requested/active/mode/decision`
  - `renderer_opengl_output_lut_configured/active/size/path/error`
- Added machine-readable regression command:
  - `--opengl-output-color-check <media_file> <cube_lut_file> [sample_ms]`
- Validation: Release build PASS, `opengl-output-color-check` PASS, OpenGL `performance-log-check` PASS, OpenGL gate PASS (`16/16`).
- Scope note: this update closes HDR policy + LUT plumbing baseline; full DXGI display-level HDR present bridge and ICC/profile-driven LUT generation remain backlog.

### 2026-03-25 Update: OpenGL interaction freeze fix (mouse/keyboard/window events)
- Fixed OpenGL event-thread affinity risk by moving SDL event pumping from render thread to `handleEvents()` main-thread path.
- OpenGL render thread no longer executes `pumpEvents()` and no longer performs window fullscreen transition directly.
- Fullscreen toggle is now consumed and applied in the main-thread event handler after SDL event pumping.
- This targets the user-reported freeze pattern where mouse move/click could lead to UI/hotkey/window-operation deadlock behavior.
- Validation: Release build PASS, OpenGL `performance-log-check` PASS, OpenGL gate PASS (`16/16`).

### 2026-03-25 Update: Cross-platform master execution tasklist
- Added `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md` as the single execution board for cross-platform delivery.
- Consolidated phased tasks with IDs (`CP-xxx`), status (`DONE/NEXT/LATER`), milestone gates (`M1..M5`), and acceptance criteria.
- Added corresponding analysis document `docs/analysis/PLAYERCORE_DAY46_CROSS_PLATFORM_MASTER_TASKLIST_CONSOLIDATION.md`.
- Rebuilt `docs/plans/README.md` to expose the new master tasklist as default plans entry.
- Validation: doc reference check PASS, local `Release` build PASS.








