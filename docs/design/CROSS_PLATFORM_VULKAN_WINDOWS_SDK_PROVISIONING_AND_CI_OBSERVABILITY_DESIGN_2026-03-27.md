# Cross-platform Vulkan Windows SDK provisioning and CI observability design (2026-03-27)

## 1. Goal
Improve Windows Vulkan CI readiness by adding optional SDK provisioning and publishing SDK state in machine-readable gate output.

## 2. Design

### 2.1 Workflow provisioning stage
- File: `.github/workflows/cross-platform-gate.yml`
- New Windows-only step:
  - attempt install via `choco install vulkan-sdk`
  - detect SDK root under `C:\VulkanSDK`
  - if found, export:
    - `VULKAN_SDK=<detected path>`
    - add `${VULKAN_SDK}\Bin` to `PATH`
  - publish sdk availability flag:
    - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1|0`
- Behavior:
  - optional/non-blocking (warn on install miss, do not fail gate stage solely on SDK install miss)

### 2.2 Gate observability extension
- File: `tools/run_windows_vulkan_checks.ps1`
- Added summary fields:
  - `windows-vulkan-check.vulkan_sdk_present`
  - `windows-vulkan-check.vulkan_sdk_path`
  - `windows-vulkan-check.runner_vulkan_sdk_available`
- Keeps existing decision path unchanged; fields are observability-only.

## 3. Compatibility
- No behavior regression to optional strict policy introduced by `VK-017`.
- Existing Windows D3D11/OpenGL checks are unchanged.
- Linux lane is unchanged.
