# PLAYERCORE Day83: VK021 Windows Vulkan dependency-source observability and find_package hardening

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan dependency resolution already had `VULKAN_SDK` fallback (`VK-020`), but one ambiguity remained:
  - `find_package(Vulkan)` could be `FOUND` while package metadata was incomplete.
- In that case, diagnostics could not clearly tell which dependency path was actually used.

## 2. Gap Snapshot
- CMake decision path was based on `Vulkan_FOUND` without explicit completeness contract.
- `--vulkan-diagnostics` and `windows-vulkan-check.*` lacked build dependency-source observability.

## 3. Solution Direction
- Harden Windows CMake Vulkan resolution:
  - require complete package metadata before accepting `find_package` path.
  - if package metadata is incomplete, switch to `VULKAN_SDK` fallback probe.
  - if fallback is also incomplete, force OFF with warning (safe downgrade unchanged).
- Introduce machine-readable dependency-source signal:
  - compile-time macro: `MVP_VULKAN_DEPENDENCY_SOURCE`
  - diagnostics field: `vulkan-diagnostics.dependency_source`
  - gate summary field: `windows-vulkan-check.diag_dependency_source`

## 4. DoD
- Windows CMake path distinguishes complete/incomplete package metadata and degrades safely.
- Vulkan diagnostics prints dependency source.
- Windows Vulkan gate summary carries dependency source.
- Local configure/build/checks pass without regression.
- Docs/records/index chain fully synchronized.

## 5. Outcome
- CMake now enforces Windows Vulkan package completeness before accepting `find_package`.
- Fallback and disable decisions are explicit and observable.
- `--vulkan-diagnostics` now includes:
  - `vulkan-diagnostics.dependency_source=...`
- `run_windows_vulkan_checks.ps1` summary now includes:
  - `windows-vulkan-check.diag_dependency_source=...`

## 6. Remaining
- Strict PASS proof still requires a Vulkan-ready Windows runner with compile/runtime availability.
