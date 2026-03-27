# PLAYERCORE Day76: VK015 Windows Vulkan enablement scope and implementation planner

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Current Vulkan chain is Linux-first and is hard-disabled on Windows at build-switch layer.
- `--vulkan-diagnostics` also treats Linux as the only supported platform.
- Result: Windows cannot compile Vulkan backend and cannot run real Vulkan playback path.

## 2. Current Gap Snapshot
- Build gap:
  - `CMakeLists.txt` forces `ENABLE_VULKAN_RENDERER=OFF` on `WIN32`.
- Runtime capability gap:
  - Vulkan support can only become `compiled_in=true` on Linux build path.
- Diagnostics gap:
  - `runVulkanDiagnostics()` uses `supported_platform = (platform == Linux)`.
- Policy gap:
  - Existing fallback normalization is Linux-specific (`Vulkan -> OpenGL -> SoftwareSDL` when Linux starts with Vulkan).

## 3. Target (Windows + Linux Vulkan)
- Keep Linux Vulkan path as first-class baseline.
- Enable Windows Vulkan as opt-in/available backend without regressing Windows default startup behavior.
- Keep observable fallback behavior and machine-readable diagnostics.

## 4. Non-goals (this round)
- Do not make Vulkan the default renderer on Windows.
- Do not add macOS Vulkan path.
- Do not remove D3D11/OpenGL/Software fallback paths.

## 5. Risks
- Windows Vulkan SDK/loader availability differs by host and CI runner image.
- Swapchain/present-mode support may vary by driver, requiring robust fallback semantics.
- Enabling compile flag without runtime probing can produce misleading diagnostics.

## 6. Proposed Solution Direction
- Relax build switch constraints for Windows.
- Add platform-aware Vulkan dependency detection for Windows and Linux.
- Extend diagnostics support-platform rule from Linux-only to Linux+Windows contract.
- Keep Windows default renderer order stable (D3D11 first), Vulkan available via capability and override path.
- Add/extend validation commands so Windows Vulkan selected/fallback behavior is machine-readable.

## 7. DoD for Windows Vulkan Enablement
- Windows build can compile with Vulkan backend enabled when dependency exists.
- `--vulkan-diagnostics` reports meaningful Windows status (`supported_platform`, `compiled_in`, `runtime_available`).
- `MVP_RENDERER_BACKEND=vulkan` on Windows is observable and either runs Vulkan or deterministic fallback.
- Existing Windows default path remains stable (no forced default switch to Vulkan).
- Documentation chain and records chain are fully synchronized.

## 8. Implementation Outcome (this round)
- CMake now allows Vulkan switch on Windows and attempts dependency resolution via `find_package(Vulkan)`.
- Missing Vulkan dependency on Windows now degrades gracefully with warning and forces `MVP_ENABLE_VULKAN_RENDERER=OFF`.
- `--vulkan-diagnostics` platform contract is extended from Linux-only to Linux+Windows:
  - current host output confirms:
    - `vulkan-diagnostics.platform=Windows`
    - `vulkan-diagnostics.supported_platform=true`
    - `vulkan-diagnostics.compiled_in=false` (due missing Vulkan package on current host)
- Startup fallback observability under `MVP_RENDERER_BACKEND=vulkan` remains intact on Windows (`Vulkan -> D3D11 -> ...`).

## 9. Remaining Work
- Validate Windows host with actual Vulkan SDK/runtime installed to get `compiled_in=true` and verify real Vulkan renderer initialization path.
- Decide whether to add a dedicated Windows Vulkan CI lane or keep Windows Vulkan as local/manual gate.
