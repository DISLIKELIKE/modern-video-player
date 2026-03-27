# Cross-platform Vulkan Windows gate strict policy and summary artifact design (2026-03-27)

## 1. Goal
Complete Windows Vulkan gate operational contract with persisted summary artifact and CI-friendly strict policy toggle.

## 2. Design

### 2.1 Script contract extension
- File: `tools/run_windows_vulkan_checks.ps1`
- New parameter:
  - `-SummaryOutputPath`: writes machine-readable `windows-vulkan-check.*` snapshot to file.
- New policy source:
  - env `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS` (`1|true|yes|on`) enables strict mode.
- Strict mode resolution:
  - CLI switch `-RequireVulkanAvailable` OR env policy enables strict mode.

### 2.2 Machine-readable fields
- Keep all existing `windows-vulkan-check.*` fields.
- Add:
  - `windows-vulkan-check.strict_mode_env_requested`
- Persisted summary file contains same key/value lines as stdout output.

### 2.3 Workflow integration
- File: `.github/workflows/cross-platform-gate.yml`
- Job-level env default:
  - `MVP_REQUIRE_WINDOWS_VULKAN_CHECKS: "0"`
- Windows gate step:
  - ensure `logs/` exists
  - invoke script with `-SummaryOutputPath logs/windows-vulkan-gate-summary.env`
  - tee script output to `logs/windows-vulkan-gate.log`
- Existing artifact upload path already covers `logs/*.log` and `logs/*.env`.

## 3. Compatibility
- Optional mode behavior remains non-blocking for hosts without Vulkan availability.
- Strict mode can be enabled without script changes.
- Existing D3D11/OpenGL checks are unaffected.
