# Cross-platform Vulkan Windows enablement design (2026-03-27)

## 1. Goal
Enable Vulkan backend on Windows while preserving Linux Vulkan baseline and preventing default-behavior regressions on Windows.

## 2. Design Principles
- Keep startup policy conservative on Windows (D3D11 remains default head candidate).
- Make Vulkan availability explicit through capability probe + diagnostics output.
- Fail closed when Vulkan dependency/runtime is unavailable, with deterministic fallback.

## 3. Module Changes
### 3.1 Build system (`CMakeLists.txt`)
- Remove Windows hard-force-off for `ENABLE_VULKAN_RENDERER`.
- Dependency detection split by platform:
  - Linux: keep `pkg-config vulkan` flow.
  - Windows: use `find_package(Vulkan)` and link imported target or include/libs from CMake package.
- Missing dependency behavior:
  - keep warning + force `MVP_ENABLE_VULKAN_RENDERER=OFF`.

### 3.2 Capability publication (`platform_capabilities.cpp`)
- Publish Vulkan support on Windows when compiled and runtime-probe passes.
- Keep priority model:
  - Linux: Vulkan above OpenGL.
  - Windows: Vulkan below D3D11 by default.

### 3.3 Strategy/fallback (`playback_strategy.cpp`)
- Keep Linux-specific fallback normalization unchanged.
- Keep Windows default-order chain unchanged.
- Ensure override path (`MVP_RENDERER_BACKEND=vulkan`) still emits deterministic fallback reason and next candidate.

### 3.4 Diagnostics (`main.cpp`)
- Extend `supported_platform` in `runVulkanDiagnostics()` to Linux+Windows contract.
- Preserve machine-readable fields:
  - `supported_platform`
  - `compiled_in`
  - `runtime_available`
  - `startup_renderer_candidates`
  - `selected_renderer`
  - `fallback_target`
  - `result`

### 3.5 Validation/gate
- Add Windows-local Vulkan check command contract (new or reuse existing checks).
- Keep Linux gate/CI strict Vulkan lane unchanged unless Windows lane Vulkan checks are explicitly introduced.

## 4. Compatibility Contract
- No regression to existing Windows default startup renderer behavior.
- No regression to Linux Vulkan strict lane behavior.
- Existing OpenGL/D3D11 diagnostics and playback checks remain functional.
