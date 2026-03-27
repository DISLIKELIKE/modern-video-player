# PLAYERCORE Day80: VK018 Windows Vulkan SDK provisioning and CI observability

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan gate/CI contract (`VK-016`,`VK-017`) was in place, but CI had no Vulkan SDK provisioning step.
- As a result, Windows lane commonly remained at `compiled_in=false`, with limited root-cause observability for SDK readiness.

## 2. Gap Snapshot
- Workflow had no Windows Vulkan SDK install/probe stage.
- Vulkan gate summary lacked SDK-context fields (`VULKAN_SDK`/runner provisioning state).

## 3. Solution Direction
- Add optional Windows Vulkan SDK provisioning stage in CI:
  - attempt `choco install vulkan-sdk`
  - detect SDK root from `C:\VulkanSDK`
  - export `VULKAN_SDK` and sdk availability marker to workflow env.
- Extend Windows Vulkan gate summary with SDK observability fields:
  - `windows-vulkan-check.vulkan_sdk_present`
  - `windows-vulkan-check.vulkan_sdk_path`
  - `windows-vulkan-check.runner_vulkan_sdk_available`

## 4. DoD
- CI contains Windows Vulkan SDK install/probe step (non-blocking optional path).
- Gate summary includes SDK observability fields.
- Local build/validation remains stable.
- Docs/records/index chain fully synchronized.

## 5. Outcome
- Added optional SDK provisioning stage to Windows lane in workflow.
- Added SDK-related machine-readable fields in `run_windows_vulkan_checks.ps1` summary output.
- Local validation confirms summary file now includes SDK observability fields and keeps deterministic skip behavior on current host.

## 6. Remaining
- Runtime PASS proof still depends on a Vulkan-ready Windows runner (SDK + runtime/device availability).
