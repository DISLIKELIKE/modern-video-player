# Cross-platform Vulkan Windows availability failure detail classification design (2026-03-27)

## 1. Goal
Provide machine-readable classification for Windows Vulkan availability-stage failures without changing gate behavior.

## 2. Design

### 2.1 New summary fields
- File: `tools/run_windows_vulkan_checks.ps1`
- Add:
  - `windows-vulkan-check.vulkan_availability_probe_passed`
  - `windows-vulkan-check.vulkan_availability_failure_detail`

### 2.2 Classification rules
- `unsupported-platform` when platform is unsupported.
- `none` when availability probe passes.
- `compiled-in-disabled` when `compiled_in=false` and `diag_dependency_source=disabled`.
- `compiled-in-false` when `compiled_in=false` with non-disabled dependency source.
- `runtime-unavailable` when `runtime_available=false` after compile-ready state.
- `diag-exit-nonzero` when diagnostics exit code indicates failure while compile/runtime flags appear ready.
- `diag-result-not-pass` when diagnostic result field is non-PASS despite other readiness.
- fallback: `unknown`.

### 2.3 Compatibility
- Keep existing fields and semantics:
  - `windows-vulkan-check.result`
  - `windows-vulkan-check.failure_reason`
  - `windows-vulkan-check.skip_reason`
- New fields are additive and backward compatible for existing parsers.

## 3. Risk and mitigation
- Risk: downstream consumers may rely on old minimal field set.
- Mitigation: keep old fields unchanged; append classification fields only.
