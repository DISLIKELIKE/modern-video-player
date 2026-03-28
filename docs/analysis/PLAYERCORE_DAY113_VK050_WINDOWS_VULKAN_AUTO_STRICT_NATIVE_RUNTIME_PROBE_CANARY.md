# PLAYERCORE Day113: VK050 Windows Vulkan auto strict native runtime probe canary

Date: 2026-03-28  
Status: Resolved

## 1. Problem
- After `VK-049`, Windows Vulkan auto strict policy accepts native or SwiftShader runtime probes.
- SwiftShader branch was covered, but native-probe auto strict branch still lacked dedicated canary protection.

## 2. Gap Snapshot
- No independent canary validated:
  - `auto + sdk=1 + native_probe=1 + swiftshader_probe=0`
- Regression risk remained on source-classification and strict-mode promotion for native path.

## 3. Solution
- Added new canary:
  - `tools/run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1`
- Integrated into CI gate runner:
  - `tools/run_windows_ci_gate.ps1`
  - added Step Summary table for native auto strict canary.
- Canary validates:
  - strict promotion under `auto`
  - runtime probe source classification `native`
  - PASS playback contract path

## 4. DoD
- Native-probe auto strict canary passes.
- Existing auto strict SwiftShader and optional-skip canaries remain pass.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan auto strict branch coverage now includes both runtime probe sources:
  - native
  - swiftshader
