# Cross-platform Vulkan Linux gate checks local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-011` script integration for Vulkan diagnostics gate stage.
- Confirm script keeps expected behavior on non-Linux host.

## 2. Commands and Results

### Script path scan
```text
rg -n "REQUIRE_VULKAN_CHECKS|probe_vulkan_check_availability|gate.has_vk010|vk010_vulkan_diagnostics|vulkan_skip_reason|--vulkan-diagnostics" tools/run_linux_mvp_checks.sh
```
Result: PASS

### Script syntax
```text
& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh
```
Result: PASS

### Non-Linux dispatch guard
```text
& "C:\Program Files\Git\bin\bash.exe" tools/run_linux_mvp_checks.sh
```
Output:
```text
This gate script only supports Linux.
```
Result: Expected FAIL on non-Linux host

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Vulkan diagnostics command baseline
```text
.\build\Release\modern-video-player.exe --vulkan-diagnostics
```
Key lines:
```text
vulkan-diagnostics.supported_platform=false
vulkan-diagnostics.compiled_in=false
vulkan-diagnostics.runtime_available=false
vulkan-diagnostics.result=FAIL
```
Result: Expected FAIL on Windows host

## 3. Conclusion
- `VK-011` integration is complete at script level:
  - Vulkan pre-probe + conditional check path
  - strict/optional gate mode hook (`REQUIRE_VULKAN_CHECKS`)
  - machine-readable report fields and skip reason
- Linux runner execution remains required for runtime `vk010_vulkan_diagnostics` PASS evidence.
