# Cross-platform Vulkan GitHub Actions Linux lane design (2026-03-27)

## 1. Goal
Deliver `VK-012`: make Linux CI lane build and gate-check Vulkan path as a first-class requirement.

## 2. Scope
- In scope:
  - Linux workflow dependency closure for Vulkan
  - explicit Linux configure switch for Vulkan renderer
  - strict Vulkan check requirement in Linux gate invocation
- Out of scope:
  - rewriting Linux gate script logic (`VK-011` already done)
  - broader workflow matrix expansion beyond existing Linux lane

## 3. CI Contract
- Linux dependency contract:
  - install Vulkan dev/runtime stack needed by current build/gate:
    - `libvulkan-dev`
    - `mesa-vulkan-drivers`
- Linux configure contract:
  - force `-DENABLE_VULKAN_RENDERER=ON` so lane does not silently skip Vulkan compile path.
- Linux gate contract:
  - pass arg `11=1` to `tools/run_linux_mvp_checks.sh`:
    - enables strict Vulkan check requirement (`REQUIRE_VULKAN_CHECKS`).

## 4. Expected Outcome
- Linux lane now fails when Vulkan path is unavailable or checks cannot run.
- Gate summary artifact (`logs/linux-mvp-gate-summary.env`) now includes Vulkan check status fields from `VK-011`.

## 5. Compatibility
- Windows lane remains unchanged.
- Existing Linux non-Vulkan checks remain in same order and keep previous semantics.
