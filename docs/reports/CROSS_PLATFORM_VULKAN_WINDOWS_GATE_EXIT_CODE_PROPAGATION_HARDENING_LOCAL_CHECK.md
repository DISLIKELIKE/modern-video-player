# Cross-platform Vulkan Windows gate exit-code propagation hardening local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-028` workflow hardening that propagates Windows Vulkan gate non-zero exit code after pipeline logging.

## 2. Commands and Results

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Baseline Windows Vulkan gate (optional path)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk028-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.result=SKIPPED
windows-vulkan-check.mode=optional
windows-vulkan-check.vulkan_availability_failure_detail=compiled-in-disabled
```

### Legacy pipeline behavior reproduction (failure swallowed)
```text
powershell -NoProfile -Command '$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS="1"; powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk028-strict-old.env" 2>&1 | Tee-Object -FilePath "logs/windows-vulkan-gate-vk028-strict-old.log"; Write-Host "legacy-last=$LASTEXITCODE"'
```
Result: expected legacy behavior reproduced  
Key lines:
```text
windows-vulkan-check.result=FAIL
legacy-last=2
outer process exit=0
```

### Guarded pipeline behavior reproduction (failure propagated)
```text
powershell -NoProfile -Command '$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS="1"; powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk028-strict-guarded.env" 2>&1 | Tee-Object -FilePath "logs/windows-vulkan-gate-vk028-strict-guarded.log"; $vulkanGateExitCode = $LASTEXITCODE; if ($vulkanGateExitCode -ne 0) { throw (''Windows Vulkan gate failed with exit code {0}'' -f $vulkanGateExitCode) }'
```
Result: expected FAIL (exit non-zero)  
Key lines:
```text
windows-vulkan-check.result=FAIL
Windows Vulkan gate failed with exit code 2
outer process exit=1
```

### Workflow static scan
```text
rg -n "vulkanGateExitCode|Windows Vulkan gate failed with exit code|windows-vulkan-gate-summary.env" .github/workflows/cross-platform-gate.yml
```
Result: PASS

## 3. Conclusion
- Windows gate now preserves Vulkan check fail semantics when output is piped to `Tee-Object`.
- `VK-027` Step Summary path remains available, and fail-fast now blocks subsequent commands when Vulkan gate returns non-zero.
