# Cross-platform Vulkan Windows link/runtime probe design (2026-03-27)

## 1. Goal
Close Windows Vulkan enablement by finishing build-link closure and runtime-truth capability publication without changing default Windows renderer policy.

## 2. Design Decisions

### 2.1 Build-link closure (`CMakeLists.txt`)
- Keep platform-aware Vulkan dependency detection from `Issue 142`.
- Add `${PLATFORM_EXTRA_LIBRARIES}` to Windows `target_link_libraries` for `modern-video-player`.
- Rationale:
  - Windows Vulkan dependency may arrive as imported target (`Vulkan::Vulkan`) or raw library list.
  - Both are carried through `PLATFORM_EXTRA_LIBRARIES` and must reach final link stage.

### 2.2 Runtime probe publication (`platform_capabilities.cpp`)
- Replace constant Vulkan runtime availability with probe-based value.
- Probe flow:
  1. `vkEnumerateInstanceExtensionProperties` for loader accessibility.
  2. `vkCreateInstance` for instance creation viability.
  3. `vkEnumeratePhysicalDevices` for render-capable device presence.
- Publish Vulkan support:
  - `compiled_in=true` under `MVP_HAVE_VULKAN_RENDERER`.
  - `runtime_available=<probe result>`.

## 3. Compatibility
- Keep Windows default priority unchanged:
  - `D3D11` remains ahead of Vulkan.
- Keep Linux Vulkan priority model unchanged.
- Keep fallback chain observability and diagnostics output fields unchanged.

## 4. Risks
- Runtime probe may return false on device/driver-constrained hosts even when compile path is available.
- This is expected and desired because diagnostics should reflect runtime truth instead of static assumption.
