# PLAYERCORE Day112: VK049 Windows Vulkan auto strict SwiftShader runtime probe promotion

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- Windows Vulkan gate already had SwiftShader fallback for runtime availability, but `auto` strict promotion still only trusted native runtime probe.
- On CI runners, this could keep gate mode in optional even when SwiftShader runtime probe was usable for deterministic Vulkan playback.

## 2. Gap Snapshot
- `run_windows_vulkan_checks.ps1` auto strict prerequisite:
  - `sdk_available && native_runtime_probe_available`
- SwiftShader probe signals were observable but not part of strict auto promotion.
- No dedicated canary guarded this policy branch.

## 3. Solution
- Expanded auto strict prerequisite to:
  - `sdk_available && (native_runtime_probe_available || swiftshader_runtime_probe_available)`
- Added machine-readable diagnostics:
  - `strict_mode_auto_runtime_probe_any_available`
  - `strict_mode_auto_runtime_probe_source`
  - updated basis to `sdk_and_runtime_probe_or_swiftshader_probe`
- Added new canary:
  - `run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1`
  - validates strict PASS on `auto + sdk=1 + native_probe=0 + swiftshader_probe=1`
- Wired canary into `run_windows_ci_gate.ps1` and Step Summary output.

## 4. DoD
- Auto strict can promote via SwiftShader runtime probe.
- New probe-source observability fields exist in gate summary.
- New canary and existing key canaries pass locally.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan strict-mode evidence chain no longer depends solely on native probe path.
- CI can exercise strict Vulkan playback path when SwiftShader probe is healthy.
