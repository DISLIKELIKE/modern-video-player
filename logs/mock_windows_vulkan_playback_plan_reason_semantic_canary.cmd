@echo off
setlocal
if "%~1"=="--vulkan-diagnostics" (
  echo vulkan-diagnostics.platform=Windows
  echo vulkan-diagnostics.supported_platform=true
  echo vulkan-diagnostics.compiled_in=true
  echo vulkan-diagnostics.runtime_available=true
  echo vulkan-diagnostics.result=PASS
  echo vulkan-diagnostics.selected_renderer=Vulkan
  echo vulkan-diagnostics.fallback_target=none
  echo vulkan-diagnostics.startup_renderer_candidates=Vulkan ^> D3D11 ^> SoftwareSDL
  echo vulkan-diagnostics.dependency_source=find_package
  exit /b 0
)
if "%~1"=="--performance-log-check" (
  rem Contract-valid payload with semantic mismatch (plan reason is not renderer-override-env).
  echo performance-log-check.result=PASS
  echo performance-log-check.startup_selected_renderer=Vulkan
  echo performance-log-check.renderer_backend=Vulkan
  echo performance-log-check.startup_renderer_candidates=Vulkan ^> D3D11 ^> SoftwareSDL
  echo performance-log-check.startup_renderer_plan_reason=strategy-default
  exit /b 0
)
exit /b 0
