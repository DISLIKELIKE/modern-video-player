# Cross-platform Vulkan Windows diagnostics contract validation design (2026-03-27)

## 1. Goal
Guarantee Windows Vulkan gate can detect and report diagnostics output contract regressions as first-class failures.

## 2. Design

### 2.1 Required-key validation
- File: `tools/run_windows_vulkan_checks.ps1`
- Required diagnostics keys:
  - `vulkan-diagnostics.platform`
  - `vulkan-diagnostics.supported_platform`
  - `vulkan-diagnostics.compiled_in`
  - `vulkan-diagnostics.runtime_available`
  - `vulkan-diagnostics.result`

### 2.2 Contract-broken behavior
- If any required key is missing:
  - mark gate `result=FAIL`
  - set `failure_reason=vulkan-diagnostics-contract-broken`
  - set `vulkan_availability_failure_detail=diag-contract-missing-required-fields`
  - skip availability/playback checks

### 2.3 New observability fields
- `windows-vulkan-check.diag_contract_valid=true|false`
- `windows-vulkan-check.diag_missing_required_fields=<csv|none>`

### 2.4 Compatibility
- For valid diagnostics output, existing strict/optional and availability logic is unchanged.
- New fields are additive for summary consumers.

## 3. Risk and mitigation
- Risk: strictness increase may fail previously "SKIPPED" scenarios when diagnostics contract is broken.
- Mitigation: this is intentional to prevent silent false classification and to protect CI signal quality.
