# PLAYERCORE Day84: VK022 Windows Vulkan auto strict runtime probe guard

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- Windows Vulkan `auto` strict policy already existed (`VK-019`), but promotion was based on SDK signal only.
- In CI environments where SDK is present but runtime/device is not actually usable, this could raise strict mode too early and create noisy false failures.

## 2. Gap Snapshot
- `run_windows_vulkan_checks.ps1` promoted `auto` to strict with `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1` alone.
- Workflow had SDK provisioning signal, but no explicit runtime probe signal for strict-policy gating.

## 3. Solution Direction
- Tighten `auto` strict prerequisites:
  - strict only when both are true:
    - `MVP_WINDOWS_VULKAN_SDK_AVAILABLE=1`
    - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1`
- Add policy observability fields in gate summary:
  - `windows-vulkan-check.runner_vulkan_runtime_probe_available`
  - `windows-vulkan-check.strict_mode_auto_basis=sdk_and_runtime_probe`
  - `windows-vulkan-check.strict_mode_auto_prerequisites_met`
- Add runtime probe publish path in workflow Windows SDK step:
  - probe `vulkaninfo --summary` from SDK bin/path
  - export `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE=1|0`

## 4. DoD
- `auto` strict mode does not escalate on SDK-only signal.
- Runtime probe signal is exported by workflow and consumed by gate script.
- Machine-readable summary exposes strict auto basis and prerequisites.
- Local matrix validation covers both:
  - `auto + sdk=1 + runtime_probe=0`
  - `auto + sdk=1 + runtime_probe=1`
- Docs/records/index chain fully synchronized.

## 5. Outcome
- `auto` strict behavior is now guarded by SDK + runtime probe dual prerequisites.
- Policy observability is explicit in both workflow logs and gate summary outputs.
- False strict promotion risk is reduced for runners with partial Vulkan readiness.

## 6. Remaining
- Strict PASS still depends on a Vulkan-ready Windows runner where compile/runtime availability are both true.
