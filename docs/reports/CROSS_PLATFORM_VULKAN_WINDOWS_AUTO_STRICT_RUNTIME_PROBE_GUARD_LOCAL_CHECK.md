# Cross-platform Vulkan Windows auto strict runtime probe guard local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-022` closure:
  - `auto` strict mode uses dual prerequisites (`sdk + runtime_probe`)
  - workflow/script observability fields are present and machine-readable

## 2. Commands and Results

### Static path scan
```text
rg -n "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE|strict_mode_auto_basis|strict_mode_auto_prerequisites_met|sdk_and_runtime_probe|vulkaninfo" tools/run_windows_vulkan_checks.ps1 .github/workflows/cross-platform-gate.yml
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

### Vulkan diagnostics baseline
```text
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Result: expected FAIL on current host  
Key lines:
```text
vulkan-diagnostics.platform=Windows
vulkan-diagnostics.supported_platform=true
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.dependency_source=disabled
vulkan-diagnostics.result=FAIL
```

### Windows Vulkan gate baseline (policy off)
```text
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk022-baseline.env"
```
Result: PASS  
Key lines:
```text
windows-vulkan-check.strict_mode_policy=off
windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe
windows-vulkan-check.result=SKIPPED
```

### Policy matrix A: auto + sdk=1 + runtime_probe=0
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='0'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk022-auto-sdk1-runtime0.env"
```
Result: PASS (exit 0)  
Key lines:
```text
windows-vulkan-check.strict_mode_policy=auto
windows-vulkan-check.strict_mode_auto_prerequisites_met=false
windows-vulkan-check.mode=optional
windows-vulkan-check.strict_mode_effective=false
windows-vulkan-check.result=SKIPPED
```

### Policy matrix B: auto + sdk=1 + runtime_probe=1
```text
$env:MVP_REQUIRE_WINDOWS_VULKAN_CHECKS='auto'
$env:MVP_WINDOWS_VULKAN_SDK_AVAILABLE='1'
$env:MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE='1'
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath "build/Release/modern-video-player.exe" -ProbeFile "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4" -SampleMs 1200 -SummaryOutputPath "logs/windows-vulkan-gate-summary-vk022-auto-sdk1-runtime1.env"
```
Result: expected FAIL on current host (exit 2)  
Key lines:
```text
windows-vulkan-check.strict_mode_policy=auto
windows-vulkan-check.strict_mode_auto_prerequisites_met=true
windows-vulkan-check.mode=strict
windows-vulkan-check.strict_mode_effective=true
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```

## 3. Conclusion
- `auto` strict policy now correctly requires SDK + runtime probe together.
- Matrix behavior matches design expectations and reduces SDK-only false strict escalation.
- Strict PASS remains dependent on a Vulkan-ready Windows runner/workstation.
