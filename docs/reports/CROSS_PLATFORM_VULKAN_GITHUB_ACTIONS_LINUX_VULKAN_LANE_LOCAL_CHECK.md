# Cross-platform Vulkan GitHub Actions Linux lane local check

Date: 2026-03-27  
Host: Windows workstation

## 1. Scope
- Validate `VK-012` workflow changes are present and locally regression-safe.

## 2. Commands and Results

### Workflow field scan
```text
rg -n "libvulkan-dev|mesa-vulkan-drivers|ENABLE_VULKAN_RENDERER=ON|logs/linux-mvp-gate-summary.env|Run Linux gate|Install Linux dependencies|Configure Linux build" .github/workflows/cross-platform-gate.yml
```
Result: PASS

### Linux gate script syntax
```text
& "C:\Program Files\Git\bin\bash.exe" -n tools/run_linux_mvp_checks.sh
```
Result: PASS

### Build
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

### Vulkan diagnostics baseline
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
- `VK-012` workflow wiring is complete for Linux lane:
  - Vulkan dependencies explicit
  - Vulkan renderer compile switch explicit
  - Linux gate strict Vulkan check requirement enabled
- Real Linux CI runner execution is still required for final runtime proof.
