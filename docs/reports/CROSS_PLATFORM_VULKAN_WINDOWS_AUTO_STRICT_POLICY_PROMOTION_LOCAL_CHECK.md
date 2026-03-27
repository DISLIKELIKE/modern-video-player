# Cross-platform Vulkan Windows auto strict policy promotion local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-019` auto strict policy promotion behavior for Windows Vulkan gate.

## 2. Commands and Results

### Static path scan
```text
rg -n "MVP_REQUIRE_WINDOWS_VULKAN_CHECKS: \"auto\"|strict_mode_policy|strict_mode_effective|strict_mode_cli_requested" .github/workflows/cross-platform-gate.yml tools/run_windows_vulkan_checks.ps1
```
Result: PASS

### Configure
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build -DENABLE_VULKAN_RENDERER=ON
```
Result: PASS

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Policy matrix
1) `auto + sdk=0`
```text
MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto
MVP_WINDOWS_VULKAN_SDK_AVAILABLE=0
powershell ... run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-auto-sdk0.env
```
Result: PASS (exit 0)
Key lines:
```text
windows-vulkan-check.strict_mode_policy=auto
windows-vulkan-check.strict_mode_effective=false
windows-vulkan-check.mode=optional
windows-vulkan-check.result=SKIPPED
```

2) `auto + sdk=1`
```text
MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=auto
MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1
powershell ... run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-auto-sdk1.env
```
Result: expected FAIL (exit 2)
Key lines:
```text
windows-vulkan-check.strict_mode_policy=auto
windows-vulkan-check.strict_mode_effective=true
windows-vulkan-check.mode=strict
windows-vulkan-check.failure_reason=vulkan-not-available-in-strict-mode
windows-vulkan-check.result=FAIL
```

3) `off + sdk=1`
```text
MVP_REQUIRE_WINDOWS_VULKAN_CHECKS=0
MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1
powershell ... run_windows_vulkan_checks.ps1 ... -SummaryOutputPath logs/windows-vulkan-gate-summary-off-sdk1.env
```
Result: PASS (exit 0)
Key lines:
```text
windows-vulkan-check.strict_mode_policy=0
windows-vulkan-check.strict_mode_effective=false
windows-vulkan-check.mode=optional
windows-vulkan-check.result=SKIPPED
```

## 3. Conclusion
- Auto strict promotion is now deterministic and machine-readable.
- Workflow default policy is aligned to auto promotion model.
- Strict PASS-path still requires Vulkan-ready Windows runner.
