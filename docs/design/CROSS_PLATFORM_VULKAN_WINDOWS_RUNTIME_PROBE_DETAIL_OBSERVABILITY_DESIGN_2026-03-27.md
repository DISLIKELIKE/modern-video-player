# Cross-platform Vulkan Windows runtime probe detail observability design (2026-03-27)

## 1. Goal
Make Windows Vulkan runtime probe reason machine-readable in CI/gate artifacts.

## 2. Design

### 2.1 Workflow detail export
- File: `.github/workflows/cross-platform-gate.yml`
- Windows Vulkan SDK step already computes `runtimeProbeDetail`; now also exports:
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL=<detail>`
- Existing boolean signal remains:
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1|0`

### 2.2 Gate summary field extension
- File: `tools/run_windows_vulkan_checks.ps1`
- Read env:
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL`
- Publish summary field:
  - `windows-vulkan-check.runner_vulkan_runtime_probe_detail`
- If env is missing/empty, normalize to `unknown`.

### 2.3 Compatibility
- No strict-policy behavior change (`VK-022` rules remain).
- Additive observability only; existing summary consumers remain compatible.

## 3. Risk and mitigation
- Risk: downstream parsers expecting fixed field set.
- Mitigation: new field is additive and does not replace existing keys.
