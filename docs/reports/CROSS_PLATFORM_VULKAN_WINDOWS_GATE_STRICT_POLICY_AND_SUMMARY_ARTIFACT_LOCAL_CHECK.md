# Cross-platform Vulkan Windows gate strict policy and summary artifact local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-017` follow-up closure for Windows Vulkan gate summary artifact and strict-policy hooks.

## 2. Commands and Results

### Static path scan
```text
rg -n "run_windows_vulkan_checks|windows-vulkan-gate-summary|MVP_REQUIRE_WINDOWS_VULKAN_CHECKS|strict_mode_env_requested" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
```
Result: PASS

### Configure
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
```
Result: PASS
Key warning (expected on current host):
```text
ENABLE_VULKAN_RENDERER requested on Windows but Vulkan SDK/runtime package is missing (find_package(Vulkan)); forcing OFF
```

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Windows Vulkan check + summary output (optional)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary.env"
```
Result: PASS (exit code 0)
Key lines:
```text
windows-vulkan-check.mode=optional
windows-vulkan-check.strict_mode_env_requested=false
windows-vulkan-check.skip_reason=vulkan-not-available
windows-vulkan-check.result=SKIPPED
```
Summary file: `logs/windows-vulkan-gate-summary.env` generated, content matches stdout keys.

### Windows Vulkan check + env strict policy
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='1'; powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-strict.env"; $LASTEXITCODE
```
Result: expected FAIL (exit code 2)
Key lines:
```text
windows-vulkan-check.mode=strict
windows-vulkan-check.strict_mode_env_requested=true
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```
Summary file: `logs/windows-vulkan-gate-summary-strict.env` generated.

## 3. Conclusion
- Windows Vulkan gate now supports persisted summary artifact and env-based strict policy control.
- CI lane is ready to archive Vulkan gate summary/log outputs with existing artifact rules.
- PASS-path strict validation still requires Vulkan-ready Windows host/runner.
