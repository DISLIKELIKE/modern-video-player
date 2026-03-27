# Cross-platform Vulkan Windows CI step summary observability design (2026-03-27)

## 1. Goal
Expose key Windows Vulkan gate state directly in GitHub Actions Step Summary.

## 2. Design

### 2.1 Workflow summary extraction
- File: `.github/workflows/cross-platform-gate.yml`
- In Windows gate step:
  - read `logs/windows-vulkan-gate-summary.env`
  - parse `key=value` lines into a map
  - publish Markdown table to `GITHUB_STEP_SUMMARY`

### 2.2 Published summary rows
- `result`
- `mode`
- `strict_mode_effective`
- `strict_mode_policy`
- `runner_vulkan_sdk_available`
- `runner_vulkan_runtime_probe_available`
- `runner_vulkan_runtime_probe_detail`
- `diag_contract_valid`
- `playback_contract_valid`
- `failure_reason`
- `vulkan_availability_failure_detail`
- `playback_failure_detail`

### 2.3 Missing-file fallback
- If `logs/windows-vulkan-gate-summary.env` is missing, append a fallback message to Step Summary.

## 3. Compatibility and risk
- Additive observability only; no gate policy/result logic changes.
- Existing artifact upload remains unchanged and still provides full raw data.
