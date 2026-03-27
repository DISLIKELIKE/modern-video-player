# Cross-platform Vulkan Windows playback contract validation local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-026` playback contract validation and playback-failure detail observability.

## 2. Commands and Results

### Static path scan
```text
rg -n "playback_contract_valid|playback_missing_required_fields|playback_failure_detail|vulkan-playback-contract-broken|contract-missing-required-fields" tools/run_windows_vulkan_checks.ps1
```
Result: PASS

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline gate (playback not executed on current host)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk026-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.playback_check_executed=false
windows-vulkan-check.playback_contract_valid=n/a
windows-vulkan-check.playback_missing_required_fields=n/a
windows-vulkan-check.playback_failure_detail=not-executed
windows-vulkan-check.result=SKIPPED
```

### Playback contract-broken simulation (mock executable)
```text
mock exe:
logs/mock_windows_vulkan_gate_player.cmd
  --vulkan-diagnostics -> valid PASS fields
  --performance-log-check -> only emits performance-log-check.result=PASS (missing required playback keys)

powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "logs/mock_windows_vulkan_gate_player.cmd" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk026-playback-contract-broken.env"
```
Result: expected FAIL (exit 2)  
Key lines:
```text
windows-vulkan-check.vulkan_availability_probe_passed=true
windows-vulkan-check.playback_check_executed=true
windows-vulkan-check.playback_contract_valid=false
windows-vulkan-check.playback_missing_required_fields=performance-log-check.startup_selected_renderer,performance-log-check.renderer_backend,performance-log-check.startup_renderer_candidates,performance-log-check.startup_renderer_plan_reason
windows-vulkan-check.playback_failure_detail=contract-missing-required-fields
windows-vulkan-check.failure_reason=vulkan-playback-contract-broken
windows-vulkan-check.result=FAIL
```

### Auto policy compatibility matrix
```text
auto + sdk=1 + runtime_probe=0 -> SKIPPED (exit 0)
auto + sdk=1 + runtime_probe=1 -> strict expected FAIL on current host (exit 2)
```
Result: PASS

## 3. Conclusion
- Playback output contract regressions are now surfaced as explicit gate failures.
- Non-playback paths remain behavior-compatible.
- Strict PASS still depends on Vulkan-ready Windows environment.
