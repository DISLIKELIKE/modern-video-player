# Cross-platform Vulkan Windows auto strict policy promotion design (2026-03-27)

## 1. Goal
Make Windows Vulkan gate policy self-adaptive so CI automatically enforces strict checks when runner-side Vulkan SDK availability is signaled.

## 2. Design

### 2.1 Policy model
- Policy source: env `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS`
- Supported values:
  - `auto`: strict if `MVP_WINDOWS_VULKAN_SDK_AVAILABLE` is true-ish
  - `1/true/yes/on`: strict
  - `0/false/no/off` or empty: optional
- CLI switch `-RequireVulkanAvailable` remains highest-priority strict override.

### 2.2 Machine-readable fields
- Add to `windows-vulkan-check.*` summary:
  - `strict_mode_policy`
  - `strict_mode_cli_requested`
  - `strict_mode_effective`
- Keep existing `strict_mode_env_requested` field as resolved env-driven strict signal.

### 2.3 Workflow integration
- In `.github/workflows/cross-platform-gate.yml` set job env default:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS: "auto"`
- This enables strict promotion automatically when `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1` is produced by `VK-018` provisioning step.

## 3. Compatibility
- Optional behavior remains available via explicit policy `0`.
- Linux gate behavior is unchanged.
- Existing log/artifact wiring from `VK-017` and SDK provisioning from `VK-018` are preserved.
