# Cross-platform Vulkan Windows CI step summary observability local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-027` workflow summary rendering logic for Windows Vulkan gate.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Generate baseline Windows Vulkan summary env
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk027-baseline.env"
```
Result: PASS

### Local preview of workflow-equivalent Step Summary rendering
```text
Parse logs/windows-vulkan-gate-summary-vk027-baseline.env and write:
logs/windows-vulkan-step-summary-preview-vk027.md
```
Result: PASS  
Rendered key lines:
```text
| result | SKIPPED |
| mode | optional |
| strict_mode_effective | false |
| runner_vulkan_runtime_probe_detail | unknown |
| diag_contract_valid | true |
| playback_contract_valid | n/a |
| vulkan_availability_failure_detail | compiled-in-disabled |
| playback_failure_detail | not-executed |
```

### Workflow static scan
```text
rg -n "Windows Vulkan Gate Summary|GITHUB_STEP_SUMMARY|windows-vulkan-gate-summary.env|runner_vulkan_runtime_probe_detail|playback_contract_valid|vulkan_availability_failure_detail" .github/workflows/cross-platform-gate.yml
```
Result: PASS

## 3. Conclusion
- Workflow now publishes Windows Vulkan gate key signals in Step Summary format.
- Existing gate behavior is unchanged; this round is observability-only.
