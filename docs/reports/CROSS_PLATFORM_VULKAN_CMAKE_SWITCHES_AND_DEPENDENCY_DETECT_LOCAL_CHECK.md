# Cross-platform Vulkan CMake switches and dependency detect local check

Date: 2026-03-26  
Host: Windows workstation

## 1. Scope
- Validate `VK-003` switch and downgrade behavior.

## 2. Commands and Results

### Configure with explicit Vulkan ON (unsupported host path)
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" -S . -B build-vk-on-win -G "Visual Studio 17 2022" -A x64 -DENABLE_VULKAN_RENDERER=ON
```
Key lines:
```text
CMake Warning: ENABLE_VULKAN_RENDERER requires Linux/Unix non-Apple; forcing OFF
Feature switches: ... VULKAN_RENDERER=OFF ...
```
Result: PASS

### Build baseline after switch updates
```text
& "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" --build build --config Release --target modern-video-player sample_logger_plugin
```
Result: PASS

## 3. Conclusion
- CMake switch and downgrade path behave as designed on unsupported host.
- Build remains stable after Vulkan switch integration.
