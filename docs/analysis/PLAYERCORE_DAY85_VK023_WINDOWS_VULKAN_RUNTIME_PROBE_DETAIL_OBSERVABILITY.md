# PLAYERCORE Day85: VK023 Windows Vulkan runtime probe detail observability

Date: 2026-03-27  
Status: Resolved

## 1. Problem
- `VK-022` added runtime probe availability signal for auto strict guard, but summary output still had only boolean state.
- CI triage still lacked machine-readable detail for why runtime probe was unavailable (missing `vulkaninfo`, SDK missing, non-zero exit code).

## 2. Gap Snapshot
- Workflow stored runtime probe reason only in step logs.
- `run_windows_vulkan_checks.ps1` did not expose runtime probe detail in `windows-vulkan-check.*`.

## 3. Solution Direction
- Export runtime probe detail from workflow:
  - `MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL=<detail>`
- Consume and publish in Windows Vulkan gate summary:
  - `windows-vulkan-check.runner_vulkan_runtime_probe_detail=<detail>`
- Keep strict-policy behavior unchanged (`VK-022` contract preserved).

## 4. DoD
- Workflow publishes both runtime probe availability and detail.
- Windows gate summary contains runtime probe detail field.
- Auto strict behavior remains identical to `VK-022`.
- Local configure/build/gate matrix passes with expected host outcomes.
- Docs/records/index chain synchronized.

## 5. Outcome
- Windows Vulkan gate now provides machine-readable runtime probe diagnosis.
- Remote CI failures can be classified directly from summary artifact without manual log scraping.
- Policy behavior remains stable while observability improves.

## 6. Remaining
- Strict PASS still requires a Vulkan-ready Windows runner/workstation where compile/runtime availability are both true.
