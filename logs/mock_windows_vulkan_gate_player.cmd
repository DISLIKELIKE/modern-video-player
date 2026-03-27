@echo off
setlocal
if /I "%~1"=="--vulkan-diagnostics" (
  echo vulkan-diagnostics.platform=Windows
  echo vulkan-diagnostics.supported_platform=true
  echo vulkan-diagnostics.compiled_in=true
  echo vulkan-diagnostics.runtime_available=true
  echo vulkan-diagnostics.result=PASS
  exit /b 0
)
if /I "%~1"=="--performance-log-check" (
  echo performance-log-check.result=PASS
  exit /b 0
)
exit /b 0
