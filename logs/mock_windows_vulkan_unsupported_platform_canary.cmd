@echo off
setlocal
if "%~1"=="--vulkan-diagnostics" (
  echo vulkan-diagnostics.platform=Linux
  echo vulkan-diagnostics.supported_platform=false
  echo vulkan-diagnostics.compiled_in=false
  echo vulkan-diagnostics.runtime_available=false
  echo vulkan-diagnostics.result=FAIL
  echo vulkan-diagnostics.selected_renderer=SoftwareSDL
  echo vulkan-diagnostics.fallback_target=none
  echo vulkan-diagnostics.startup_renderer_candidates=SoftwareSDL
  echo vulkan-diagnostics.dependency_source=find_package
  exit /b 0
)
if "%~1"=="--performance-log-check" (
  rem unsupported-platform branch should not execute playback check
  echo performance-log-check.result=FAIL
  exit /b 0
)
exit /b 0
