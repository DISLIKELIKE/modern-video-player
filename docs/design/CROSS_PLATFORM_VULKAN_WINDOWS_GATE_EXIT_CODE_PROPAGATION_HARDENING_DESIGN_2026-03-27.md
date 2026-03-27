# Cross-platform Vulkan Windows gate exit-code propagation hardening design (2026-03-27)

## 1. Scope
- Harden Windows Vulkan CI gate failure propagation in `.github/workflows/cross-platform-gate.yml`.
- Keep `VK-027` Step Summary observability behavior unchanged.

## 2. Non-goals
- Do not change strict/optional policy rules.
- Do not change `run_windows_vulkan_checks.ps1` result semantics.
- Do not alter Linux lane behavior.

## 3. Root Cause
- Native command executed in PowerShell pipeline does not auto-fail step on non-zero exit.
- Without explicit check, later commands can overwrite effective step result.

## 4. Design
- Add local variable in Windows gate step:
  - `$vulkanGateExitCode = $LASTEXITCODE` immediately after Vulkan gate command.
- Preserve existing Step Summary parse/render block.
- Add fail-fast guard before subsequent checks:
  - `if ($vulkanGateExitCode -ne 0) { throw "..." }`

## 5. Contract
- `run_windows_vulkan_checks.ps1` `exit 0` => step continues.
- `run_windows_vulkan_checks.ps1` non-zero => step fails deterministically.
- Step Summary write still runs before throw path, so failure context remains visible.

## 6. Validation Strategy
- Build passes (`modern-video-player` + `sample_logger_plugin`).
- Baseline gate run remains `SKIPPED` on current host.
- Local PowerShell reproduction validates:
  - legacy pipeline swallows failure at process exit level.
  - guarded pipeline raises exception and returns non-zero.
