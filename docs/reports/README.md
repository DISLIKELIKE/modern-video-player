# Reports

This directory stores validation evidence. Reports are evidence, not the active project plan.

## Current Validation Entry Points

| Area | Command or script |
| --- | --- |
| Aggregate Windows checks | `powershell -ExecutionPolicy Bypass -File .\tools\run_all_checks.ps1` |
| Windows CI gate | `powershell -ExecutionPolicy Bypass -File .\tools\run_windows_ci_gate.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4` |
| Linux MVP gate | `bash ./tools/run_linux_mvp_checks.sh ./build/modern-video-player ./juren-30s.mp4 1800` |
| OpenGL gate | `powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath .\build\Release\modern-video-player.exe -ProbeFile .\juren-30s.mp4` |
| Format regression | `powershell -ExecutionPolicy Bypass -File .\tools\format_regression\run_format_regression.ps1` |
| Packaging | `tools/package_windows.ps1`, `tools/package_linux.sh` |

## Useful Current Reports

| Report | Purpose |
| --- | --- |
| [PLAYERCORE_WORKER_THREAD_CONSOLIDATION_LOCAL_CHECK.md](./PLAYERCORE_WORKER_THREAD_CONSOLIDATION_LOCAL_CHECK.md) | Latest PlayerCore worker lifecycle refactor validation. |
| [V1_0_0_RC1_RELEASE_READINESS.md](./V1_0_0_RC1_RELEASE_READINESS.md) | Historical rc1 release-readiness evidence. |
| [FORMAT_REGRESSION_LOCAL_CHECK.md](./FORMAT_REGRESSION_LOCAL_CHECK.md) | Format regression summary. |
| [PERFORMANCE_LOG_LOCAL_CHECK.md](./PERFORMANCE_LOG_LOCAL_CHECK.md) | Performance-log validation. |
| [SUBTITLE_SYNC_LOCAL_CHECK.md](./SUBTITLE_SYNC_LOCAL_CHECK.md) | Subtitle sync validation. |
| [OPENGL_RENDERER_LOCAL_CHECK.md](./OPENGL_RENDERER_LOCAL_CHECK.md) | OpenGL renderer validation. |
| [CROSS_PLATFORM_PHASE9_LOCAL_CHECK.md](./CROSS_PLATFORM_PHASE9_LOCAL_CHECK.md) | Cross-platform observability/CI phase evidence. |

## Historical Reports

The many `CROSS_PLATFORM_VULKAN_*_LOCAL_CHECK.md` and phase reports are retained as implementation evidence. Use them when tracing a specific change, but run fresh gates before making release claims.

## Update Rule

- Add or update a report only after validation was actually run.
- Include command, environment, key output and result.
- Keep current status and TODOs in `docs/plans/CROSS_PLATFORM_MASTER_TASKLIST.md`, not here.
