# PLAYERCORE Day64: VK003 CMake switches and dependency detect

Date: 2026-03-26  
Status: Done

## 1. Problem
- No Vulkan renderer build switch or dependency gating existed.
- Missing dependency handling could either break CI or silently diverge build behavior across hosts.

## 2. Root Cause
- Existing CMake feature matrix covered D3D11/OpenGL/SDL and decoder switches, but not Vulkan renderer.
- No `MVP_HAVE_VULKAN_RENDERER` macro pipeline existed for compile-time guards.

## 3. Solution
- Added CMake switch:
  - `ENABLE_VULKAN_RENDERER` (`ON` on Linux, `OFF` elsewhere).
- Added platform guard:
  - non-Linux (Windows/macOS) force OFF with warning.
- Added dependency detection on Linux:
  - `pkg-config vulkan` (quiet probe).
  - if missing, force OFF with warning (downgrade strategy).
- Added compile/link propagation:
  - source inclusion gate for Vulkan renderer source
  - compile definition `MVP_HAVE_VULKAN_RENDERER`
  - optional include/library append through existing platform extra lists.
- Extended “at least one renderer enabled” rule to include Vulkan.

## 4. Outcome
- Vulkan switch behavior is explicit, observable, and safe on unsupported hosts.
- Existing Windows pipeline remains stable while Linux-first Vulkan path is prepared.
