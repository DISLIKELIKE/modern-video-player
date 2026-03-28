[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$ExecutablePath,

    [Parameter(Mandatory = $true)]
    [string]$ProbeFile,

    [Parameter()]
    [int]$SampleMs = 1200,

    [Parameter()]
    [string]$BuildDir = 'build'
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$resolvedExecutablePath = Join-Path (Get-Location) $ExecutablePath
if (-not (Test-Path $resolvedExecutablePath)) {
    throw "Executable not found: $resolvedExecutablePath"
}
$env:Path = (Join-Path (Get-Location) 'external/ffmpeg/bin') + ';' + $env:Path
New-Item -ItemType Directory -Path logs -Force | Out-Null
$stepSummaryPath = $env:GITHUB_STEP_SUMMARY
if ([string]::IsNullOrWhiteSpace($stepSummaryPath)) {
    $stepSummaryPath = Join-Path (Get-Location) 'logs/windows-gate-step-summary.md'
    Write-Warning "GITHUB_STEP_SUMMARY is not set; writing step summary to $stepSummaryPath"
}
& $resolvedExecutablePath --d3d11-diagnostics
& $resolvedExecutablePath --d3d11-hdr-output-check $ProbeFile $SampleMs
$env:MVP_RENDERER_BACKEND = 'd3d11'
& $resolvedExecutablePath --performance-log-check $ProbeFile $SampleMs
Remove-Item Env:MVP_RENDERER_BACKEND -ErrorAction SilentlyContinue
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_checks.ps1 -ExecutablePath $ExecutablePath -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-summary.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate.log'
$vulkanGateExitCode = $LASTEXITCODE
$vulkanSummaryPath = 'logs/windows-vulkan-gate-summary.env'
if (Test-Path $vulkanSummaryPath) {
  $vulkanSummary = @{}
  Get-Content -Path $vulkanSummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanSummary[$key] = $value
      }
    }
  }

  $summaryRows = @(
    @("result", $vulkanSummary["windows-vulkan-check.result"]),
    @("mode", $vulkanSummary["windows-vulkan-check.mode"]),
    @("strict_mode_effective", $vulkanSummary["windows-vulkan-check.strict_mode_effective"]),
    @("strict_mode_policy", $vulkanSummary["windows-vulkan-check.strict_mode_policy"]),
    @("strict_mode_auto_basis", $vulkanSummary["windows-vulkan-check.strict_mode_auto_basis"]),
    @("strict_mode_auto_prerequisites_met", $vulkanSummary["windows-vulkan-check.strict_mode_auto_prerequisites_met"]),
    @("strict_mode_auto_runtime_probe_any_available", $vulkanSummary["windows-vulkan-check.strict_mode_auto_runtime_probe_any_available"]),
    @("strict_mode_auto_runtime_probe_source", $vulkanSummary["windows-vulkan-check.strict_mode_auto_runtime_probe_source"]),
    @("runner_vulkan_sdk_available", $vulkanSummary["windows-vulkan-check.runner_vulkan_sdk_available"]),
    @("runner_vulkan_runtime_probe_available", $vulkanSummary["windows-vulkan-check.runner_vulkan_runtime_probe_available"]),
    @("runner_vulkan_runtime_probe_detail", $vulkanSummary["windows-vulkan-check.runner_vulkan_runtime_probe_detail"]),
    @("runner_vulkan_swiftshader_available", $vulkanSummary["windows-vulkan-check.runner_vulkan_swiftshader_available"]),
    @("runner_vulkan_swiftshader_runtime_probe_available", $vulkanSummary["windows-vulkan-check.runner_vulkan_swiftshader_runtime_probe_available"]),
    @("runner_vulkan_swiftshader_runtime_probe_detail", $vulkanSummary["windows-vulkan-check.runner_vulkan_swiftshader_runtime_probe_detail"]),
    @("vk_icd_filenames", $vulkanSummary["windows-vulkan-check.vk_icd_filenames"]),
    @("diag_contract_valid", $vulkanSummary["windows-vulkan-check.diag_contract_valid"]),
    @("playback_contract_valid", $vulkanSummary["windows-vulkan-check.playback_contract_valid"]),
    @("failure_reason", $vulkanSummary["windows-vulkan-check.failure_reason"]),
    @("vulkan_availability_failure_detail", $vulkanSummary["windows-vulkan-check.vulkan_availability_failure_detail"]),
    @("playback_failure_detail", $vulkanSummary["windows-vulkan-check.playback_failure_detail"])
  )

  "### Windows Vulkan Gate Summary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $summaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Summary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanGateExitCode -ne 0) {
  throw "Windows Vulkan gate failed with exit code $vulkanGateExitCode. See logs/windows-vulkan-gate.log and logs/windows-vulkan-gate-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_contract_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-contract-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-contract-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-contract-canary.log'
$vulkanCanaryExitCode = $LASTEXITCODE
$vulkanCanarySummaryPath = 'logs/windows-vulkan-gate-contract-canary-summary.env'
if (Test-Path $vulkanCanarySummaryPath) {
  $vulkanCanarySummary = @{}
  Get-Content -Path $vulkanCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanCanarySummary[$key] = $value
      }
    }
  }

  $canaryRows = @(
    @("result", $vulkanCanarySummary["windows-vulkan-contract-canary.result"]),
    @("expected_gate_exit_code", $vulkanCanarySummary["windows-vulkan-contract-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanCanarySummary["windows-vulkan-contract-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanCanarySummary["windows-vulkan-contract-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanCanarySummary["windows-vulkan-contract-canary.gate_result"]),
    @("gate_failure_reason", $vulkanCanarySummary["windows-vulkan-contract-canary.gate_failure_reason"]),
    @("gate_diag_contract_valid", $vulkanCanarySummary["windows-vulkan-contract-canary.gate_diag_contract_valid"]),
    @("gate_vulkan_availability_failure_detail", $vulkanCanarySummary["windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail"]),
    @("validation_failure_reason", $vulkanCanarySummary["windows-vulkan-contract-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $canaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-contract-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate contract canary failed with exit code $vulkanCanaryExitCode. See logs/windows-vulkan-gate-contract-canary.log and logs/windows-vulkan-gate-contract-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_contract_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-contract-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-contract-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-contract-canary.log'
$vulkanPlaybackCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackCanarySummaryPath = 'logs/windows-vulkan-gate-playback-contract-canary-summary.env'
if (Test-Path $vulkanPlaybackCanarySummaryPath) {
  $vulkanPlaybackCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackCanarySummary[$key] = $value
      }
    }
  }

  $playbackCanaryRows = @(
    @("result", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.gate_playback_failure_detail"]),
    @("validation_failure_reason", $vulkanPlaybackCanarySummary["windows-vulkan-playback-contract-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-contract-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-contract canary failed with exit code $vulkanPlaybackCanaryExitCode. See logs/windows-vulkan-gate-playback-contract-canary.log and logs/windows-vulkan-gate-playback-contract-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_pass_contract_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-pass-contract-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-pass-contract-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-pass-contract-canary.log'
$vulkanPassCanaryExitCode = $LASTEXITCODE
$vulkanPassCanarySummaryPath = 'logs/windows-vulkan-gate-pass-contract-canary-summary.env'
if (Test-Path $vulkanPassCanarySummaryPath) {
  $vulkanPassCanarySummary = @{}
  Get-Content -Path $vulkanPassCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPassCanarySummary[$key] = $value
      }
    }
  }

  $passCanaryRows = @(
    @("result", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.result"]),
    @("expected_gate_exit_code", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_result"]),
    @("gate_mode", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_strict_mode_effective"]),
    @("gate_vulkan_availability_failure_detail", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_contract_valid", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_playback_failure_detail"]),
    @("gate_failure_reason", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.gate_failure_reason"]),
    @("validation_failure_reason", $vulkanPassCanarySummary["windows-vulkan-pass-contract-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate PASS Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $passCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate PASS Contract Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-pass-contract-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPassCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate pass-contract canary failed with exit code $vulkanPassCanaryExitCode. See logs/windows-vulkan-gate-pass-contract-canary.log and logs/windows-vulkan-gate-pass-contract-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_unavailable_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-strict-unavailable-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-strict-unavailable-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-strict-unavailable-canary.log'
$vulkanStrictUnavailableCanaryExitCode = $LASTEXITCODE
$vulkanStrictUnavailableCanarySummaryPath = 'logs/windows-vulkan-gate-strict-unavailable-canary-summary.env'
if (Test-Path $vulkanStrictUnavailableCanarySummaryPath) {
  $vulkanStrictUnavailableCanarySummary = @{}
  Get-Content -Path $vulkanStrictUnavailableCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanStrictUnavailableCanarySummary[$key] = $value
      }
    }
  }

  $strictUnavailableCanaryRows = @(
    @("result", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.result"]),
    @("expected_gate_exit_code", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_result"]),
    @("gate_mode", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_strict_mode_effective"]),
    @("gate_failure_reason", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.gate_playback_check_executed"]),
    @("validation_failure_reason", $vulkanStrictUnavailableCanarySummary["windows-vulkan-strict-unavailable-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Strict Unavailable Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $strictUnavailableCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Strict Unavailable Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-strict-unavailable-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanStrictUnavailableCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate strict-unavailable canary failed with exit code $vulkanStrictUnavailableCanaryExitCode. See logs/windows-vulkan-gate-strict-unavailable-canary.log and logs/windows-vulkan-gate-strict-unavailable-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_runtime_unavailable_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-strict-runtime-unavailable-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-strict-runtime-unavailable-canary.log'
$vulkanStrictRuntimeUnavailableCanaryExitCode = $LASTEXITCODE
$vulkanStrictRuntimeUnavailableCanarySummaryPath = 'logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary.env'
if (Test-Path $vulkanStrictRuntimeUnavailableCanarySummaryPath) {
  $vulkanStrictRuntimeUnavailableCanarySummary = @{}
  Get-Content -Path $vulkanStrictRuntimeUnavailableCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanStrictRuntimeUnavailableCanarySummary[$key] = $value
      }
    }
  }

  $strictRuntimeUnavailableCanaryRows = @(
    @("result", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.result"]),
    @("expected_gate_exit_code", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_result"]),
    @("gate_mode", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_strict_mode_effective"]),
    @("gate_failure_reason", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.gate_playback_check_executed"]),
    @("validation_failure_reason", $vulkanStrictRuntimeUnavailableCanarySummary["windows-vulkan-strict-runtime-unavailable-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Strict Runtime-Unavailable Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $strictRuntimeUnavailableCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Strict Runtime-Unavailable Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-strict-runtime-unavailable-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanStrictRuntimeUnavailableCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate strict-runtime-unavailable canary failed with exit code $vulkanStrictRuntimeUnavailableCanaryExitCode. See logs/windows-vulkan-gate-strict-runtime-unavailable-canary.log and logs/windows-vulkan-gate-strict-runtime-unavailable-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_sdk_missing_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-auto-optional-sdk-missing-canary.log'
$vulkanAutoOptionalSdkMissingCanaryExitCode = $LASTEXITCODE
$vulkanAutoOptionalSdkMissingCanarySummaryPath = 'logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-summary.env'
if (Test-Path $vulkanAutoOptionalSdkMissingCanarySummaryPath) {
  $vulkanAutoOptionalSdkMissingCanarySummary = @{}
  Get-Content -Path $vulkanAutoOptionalSdkMissingCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanAutoOptionalSdkMissingCanarySummary[$key] = $value
      }
    }
  }

  $autoOptionalSdkMissingCanaryRows = @(
    @("result", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.result"]),
    @("expected_gate_exit_code", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_result"]),
    @("gate_mode", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_effective"]),
    @("gate_skip_reason", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_skip_reason"]),
    @("gate_failure_reason", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_playback_check_executed"]),
    @("gate_strict_mode_auto_basis", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_basis"]),
    @("gate_strict_mode_auto_prerequisites_met", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_prerequisites_met"]),
    @("gate_strict_mode_auto_runtime_probe_any_available", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_any_available"]),
    @("gate_strict_mode_auto_runtime_probe_source", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_source"]),
    @("validation_failure_reason", $vulkanAutoOptionalSdkMissingCanarySummary["windows-vulkan-auto-optional-sdk-missing-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Auto Optional SDK-Missing Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $autoOptionalSdkMissingCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Auto Optional SDK-Missing Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-auto-optional-sdk-missing-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanAutoOptionalSdkMissingCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate auto-optional-sdk-missing canary failed with exit code $vulkanAutoOptionalSdkMissingCanaryExitCode. See logs/windows-vulkan-gate-auto-optional-sdk-missing-canary.log and logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_optional_no_probe_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-auto-optional-no-probe-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-auto-optional-no-probe-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-auto-optional-no-probe-canary.log'
$vulkanAutoOptionalNoProbeCanaryExitCode = $LASTEXITCODE
$vulkanAutoOptionalNoProbeCanarySummaryPath = 'logs/windows-vulkan-gate-auto-optional-no-probe-canary-summary.env'
if (Test-Path $vulkanAutoOptionalNoProbeCanarySummaryPath) {
  $vulkanAutoOptionalNoProbeCanarySummary = @{}
  Get-Content -Path $vulkanAutoOptionalNoProbeCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanAutoOptionalNoProbeCanarySummary[$key] = $value
      }
    }
  }

  $autoOptionalNoProbeCanaryRows = @(
    @("result", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.result"]),
    @("expected_gate_exit_code", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_result"]),
    @("gate_mode", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_effective"]),
    @("gate_skip_reason", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_skip_reason"]),
    @("gate_failure_reason", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_playback_check_executed"]),
    @("gate_strict_mode_auto_basis", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_basis"]),
    @("gate_strict_mode_auto_prerequisites_met", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_prerequisites_met"]),
    @("gate_strict_mode_auto_runtime_probe_any_available", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_runtime_probe_any_available"]),
    @("gate_strict_mode_auto_runtime_probe_source", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.gate_strict_mode_auto_runtime_probe_source"]),
    @("validation_failure_reason", $vulkanAutoOptionalNoProbeCanarySummary["windows-vulkan-auto-optional-no-probe-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Auto Optional No-Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $autoOptionalNoProbeCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Auto Optional No-Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-auto-optional-no-probe-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanAutoOptionalNoProbeCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate auto-optional-no-probe canary failed with exit code $vulkanAutoOptionalNoProbeCanaryExitCode. See logs/windows-vulkan-gate-auto-optional-no-probe-canary.log and logs/windows-vulkan-gate-auto-optional-no-probe-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_native_probe_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-native-probe-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-native-probe-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-auto-strict-native-probe-canary.log'
$vulkanAutoStrictNativeProbeCanaryExitCode = $LASTEXITCODE
$vulkanAutoStrictNativeProbeCanarySummaryPath = 'logs/windows-vulkan-gate-auto-strict-native-probe-canary-summary.env'
if (Test-Path $vulkanAutoStrictNativeProbeCanarySummaryPath) {
  $vulkanAutoStrictNativeProbeCanarySummary = @{}
  Get-Content -Path $vulkanAutoStrictNativeProbeCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanAutoStrictNativeProbeCanarySummary[$key] = $value
      }
    }
  }

  $autoStrictNativeProbeCanaryRows = @(
    @("result", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.result"]),
    @("expected_gate_exit_code", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_result"]),
    @("gate_mode", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_effective"]),
    @("gate_strict_mode_auto_basis", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_basis"]),
    @("gate_strict_mode_auto_prerequisites_met", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_prerequisites_met"]),
    @("gate_strict_mode_auto_runtime_probe_any_available", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_runtime_probe_any_available"]),
    @("gate_strict_mode_auto_runtime_probe_source", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_strict_mode_auto_runtime_probe_source"]),
    @("gate_playback_contract_valid", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_playback_failure_detail"]),
    @("gate_failure_reason", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.gate_failure_reason"]),
    @("validation_failure_reason", $vulkanAutoStrictNativeProbeCanarySummary["windows-vulkan-auto-strict-native-probe-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Auto Strict Native Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $autoStrictNativeProbeCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Auto Strict Native Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-auto-strict-native-probe-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanAutoStrictNativeProbeCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate auto-strict-native-probe canary failed with exit code $vulkanAutoStrictNativeProbeCanaryExitCode. See logs/windows-vulkan-gate-auto-strict-native-probe-canary.log and logs/windows-vulkan-gate-auto-strict-native-probe-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_dual_probe_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-dual-probe-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-dual-probe-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-auto-strict-dual-probe-canary.log'
$vulkanAutoStrictDualProbeCanaryExitCode = $LASTEXITCODE
$vulkanAutoStrictDualProbeCanarySummaryPath = 'logs/windows-vulkan-gate-auto-strict-dual-probe-canary-summary.env'
if (Test-Path $vulkanAutoStrictDualProbeCanarySummaryPath) {
  $vulkanAutoStrictDualProbeCanarySummary = @{}
  Get-Content -Path $vulkanAutoStrictDualProbeCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanAutoStrictDualProbeCanarySummary[$key] = $value
      }
    }
  }

  $autoStrictDualProbeCanaryRows = @(
    @("result", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.result"]),
    @("expected_gate_exit_code", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_result"]),
    @("gate_mode", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_effective"]),
    @("gate_strict_mode_auto_basis", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_basis"]),
    @("gate_strict_mode_auto_prerequisites_met", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_prerequisites_met"]),
    @("gate_strict_mode_auto_runtime_probe_any_available", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_runtime_probe_any_available"]),
    @("gate_strict_mode_auto_runtime_probe_source", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_strict_mode_auto_runtime_probe_source"]),
    @("gate_playback_contract_valid", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_playback_failure_detail"]),
    @("gate_failure_reason", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.gate_failure_reason"]),
    @("validation_failure_reason", $vulkanAutoStrictDualProbeCanarySummary["windows-vulkan-auto-strict-dual-probe-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Auto Strict Dual-Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $autoStrictDualProbeCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Auto Strict Dual-Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-auto-strict-dual-probe-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanAutoStrictDualProbeCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate auto-strict-dual-probe canary failed with exit code $vulkanAutoStrictDualProbeCanaryExitCode. See logs/windows-vulkan-gate-auto-strict-dual-probe-canary.log and logs/windows-vulkan-gate-auto-strict-dual-probe-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_auto_strict_swiftshader_probe_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary.log'
$vulkanAutoStrictSwiftShaderProbeCanaryExitCode = $LASTEXITCODE
$vulkanAutoStrictSwiftShaderProbeCanarySummaryPath = 'logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary-summary.env'
if (Test-Path $vulkanAutoStrictSwiftShaderProbeCanarySummaryPath) {
  $vulkanAutoStrictSwiftShaderProbeCanarySummary = @{}
  Get-Content -Path $vulkanAutoStrictSwiftShaderProbeCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanAutoStrictSwiftShaderProbeCanarySummary[$key] = $value
      }
    }
  }

  $autoStrictSwiftShaderProbeCanaryRows = @(
    @("result", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.result"]),
    @("expected_gate_exit_code", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_result"]),
    @("gate_mode", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_effective"]),
    @("gate_strict_mode_auto_basis", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_basis"]),
    @("gate_strict_mode_auto_prerequisites_met", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_prerequisites_met"]),
    @("gate_strict_mode_auto_runtime_probe_any_available", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_runtime_probe_any_available"]),
    @("gate_strict_mode_auto_runtime_probe_source", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_strict_mode_auto_runtime_probe_source"]),
    @("gate_playback_contract_valid", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_playback_failure_detail"]),
    @("gate_failure_reason", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.gate_failure_reason"]),
    @("validation_failure_reason", $vulkanAutoStrictSwiftShaderProbeCanarySummary["windows-vulkan-auto-strict-swiftshader-probe-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Auto Strict SwiftShader Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $autoStrictSwiftShaderProbeCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Auto Strict SwiftShader Probe Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-auto-strict-swiftshader-probe-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanAutoStrictSwiftShaderProbeCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate auto-strict-swiftshader-probe canary failed with exit code $vulkanAutoStrictSwiftShaderProbeCanaryExitCode. See logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary.log and logs/windows-vulkan-gate-auto-strict-swiftshader-probe-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_exit_nonzero_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary.log'
$vulkanStrictDiagExitNonzeroCanaryExitCode = $LASTEXITCODE
$vulkanStrictDiagExitNonzeroCanarySummaryPath = 'logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary.env'
if (Test-Path $vulkanStrictDiagExitNonzeroCanarySummaryPath) {
  $vulkanStrictDiagExitNonzeroCanarySummary = @{}
  Get-Content -Path $vulkanStrictDiagExitNonzeroCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanStrictDiagExitNonzeroCanarySummary[$key] = $value
      }
    }
  }

  $strictDiagExitNonzeroCanaryRows = @(
    @("result", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.result"]),
    @("expected_gate_exit_code", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_result"]),
    @("gate_mode", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_strict_mode_effective"]),
    @("gate_failure_reason", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_playback_check_executed"]),
    @("gate_diag_exit_code", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.gate_diag_exit_code"]),
    @("validation_failure_reason", $vulkanStrictDiagExitNonzeroCanarySummary["windows-vulkan-strict-diag-exit-nonzero-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $strictDiagExitNonzeroCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Strict Diag-Exit-Nonzero Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanStrictDiagExitNonzeroCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate strict-diag-exit-nonzero canary failed with exit code $vulkanStrictDiagExitNonzeroCanaryExitCode. See logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary.log and logs/windows-vulkan-gate-strict-diag-exit-nonzero-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_diag_result_not_pass_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-strict-diag-result-not-pass-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-strict-diag-result-not-pass-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-strict-diag-result-not-pass-canary.log'
$vulkanStrictDiagResultNotPassCanaryExitCode = $LASTEXITCODE
$vulkanStrictDiagResultNotPassCanarySummaryPath = 'logs/windows-vulkan-gate-strict-diag-result-not-pass-canary-summary.env'
if (Test-Path $vulkanStrictDiagResultNotPassCanarySummaryPath) {
  $vulkanStrictDiagResultNotPassCanarySummary = @{}
  Get-Content -Path $vulkanStrictDiagResultNotPassCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanStrictDiagResultNotPassCanarySummary[$key] = $value
      }
    }
  }

  $strictDiagResultNotPassCanaryRows = @(
    @("result", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.result"]),
    @("expected_gate_exit_code", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_result"]),
    @("gate_mode", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_strict_mode_effective"]),
    @("gate_failure_reason", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_playback_check_executed"]),
    @("gate_diag_exit_code", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_exit_code"]),
    @("gate_diag_result", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.gate_diag_result"]),
    @("validation_failure_reason", $vulkanStrictDiagResultNotPassCanarySummary["windows-vulkan-strict-diag-result-not-pass-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $strictDiagResultNotPassCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Strict Diag-Result-Not-Pass Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-strict-diag-result-not-pass-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanStrictDiagResultNotPassCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate strict-diag-result-not-pass canary failed with exit code $vulkanStrictDiagResultNotPassCanaryExitCode. See logs/windows-vulkan-gate-strict-diag-result-not-pass-canary.log and logs/windows-vulkan-gate-strict-diag-result-not-pass-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_strict_compiled_in_disabled_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-strict-compiled-in-disabled-canary.log'
$vulkanStrictCompiledInDisabledCanaryExitCode = $LASTEXITCODE
$vulkanStrictCompiledInDisabledCanarySummaryPath = 'logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary.env'
if (Test-Path $vulkanStrictCompiledInDisabledCanarySummaryPath) {
  $vulkanStrictCompiledInDisabledCanarySummary = @{}
  Get-Content -Path $vulkanStrictCompiledInDisabledCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanStrictCompiledInDisabledCanarySummary[$key] = $value
      }
    }
  }

  $strictCompiledInDisabledCanaryRows = @(
    @("result", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.result"]),
    @("expected_gate_exit_code", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_result"]),
    @("gate_mode", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_strict_mode_effective"]),
    @("gate_failure_reason", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_playback_check_executed"]),
    @("gate_diag_exit_code", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_exit_code"]),
    @("gate_diag_dependency_source", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.gate_diag_dependency_source"]),
    @("validation_failure_reason", $vulkanStrictCompiledInDisabledCanarySummary["windows-vulkan-strict-compiled-in-disabled-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Strict Compiled-In-Disabled Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $strictCompiledInDisabledCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Strict Compiled-In-Disabled Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-strict-compiled-in-disabled-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanStrictCompiledInDisabledCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate strict-compiled-in-disabled canary failed with exit code $vulkanStrictCompiledInDisabledCanaryExitCode. See logs/windows-vulkan-gate-strict-compiled-in-disabled-canary.log and logs/windows-vulkan-gate-strict-compiled-in-disabled-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_optional_skip_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-optional-skip-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-optional-skip-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-optional-skip-canary.log'
$vulkanOptionalSkipCanaryExitCode = $LASTEXITCODE
$vulkanOptionalSkipCanarySummaryPath = 'logs/windows-vulkan-gate-optional-skip-canary-summary.env'
if (Test-Path $vulkanOptionalSkipCanarySummaryPath) {
  $vulkanOptionalSkipCanarySummary = @{}
  Get-Content -Path $vulkanOptionalSkipCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanOptionalSkipCanarySummary[$key] = $value
      }
    }
  }

  $optionalSkipCanaryRows = @(
    @("result", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.result"]),
    @("expected_gate_exit_code", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_result"]),
    @("gate_mode", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_mode"]),
    @("gate_strict_mode_effective", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_strict_mode_effective"]),
    @("gate_skip_reason", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_skip_reason"]),
    @("gate_failure_reason", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_playback_check_executed", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.gate_playback_check_executed"]),
    @("validation_failure_reason", $vulkanOptionalSkipCanarySummary["windows-vulkan-optional-skip-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Optional Skip Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $optionalSkipCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Optional Skip Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-optional-skip-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanOptionalSkipCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate optional-skip canary failed with exit code $vulkanOptionalSkipCanaryExitCode. See logs/windows-vulkan-gate-optional-skip-canary.log and logs/windows-vulkan-gate-optional-skip-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_unsupported_platform_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-unsupported-platform-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-unsupported-platform-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-unsupported-platform-canary.log'
$vulkanUnsupportedPlatformCanaryExitCode = $LASTEXITCODE
$vulkanUnsupportedPlatformCanarySummaryPath = 'logs/windows-vulkan-gate-unsupported-platform-canary-summary.env'
if (Test-Path $vulkanUnsupportedPlatformCanarySummaryPath) {
  $vulkanUnsupportedPlatformCanarySummary = @{}
  Get-Content -Path $vulkanUnsupportedPlatformCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanUnsupportedPlatformCanarySummary[$key] = $value
      }
    }
  }

  $unsupportedPlatformCanaryRows = @(
    @("result", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.result"]),
    @("expected_gate_exit_code", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_result"]),
    @("gate_mode", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_mode"]),
    @("gate_failure_reason", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_failure_reason"]),
    @("gate_vulkan_availability_failure_detail", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_vulkan_availability_failure_detail"]),
    @("gate_skip_reason", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_skip_reason"]),
    @("gate_playback_check_executed", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.gate_playback_check_executed"]),
    @("validation_failure_reason", $vulkanUnsupportedPlatformCanarySummary["windows-vulkan-unsupported-platform-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Unsupported Platform Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $unsupportedPlatformCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Unsupported Platform Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-unsupported-platform-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanUnsupportedPlatformCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate unsupported-platform canary failed with exit code $vulkanUnsupportedPlatformCanaryExitCode. See logs/windows-vulkan-gate-unsupported-platform-canary.log and logs/windows-vulkan-gate-unsupported-platform-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_semantic_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-semantic-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-semantic-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-semantic-canary.log'
$vulkanPlaybackSemanticCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackSemanticCanarySummaryPath = 'logs/windows-vulkan-gate-playback-semantic-canary-summary.env'
if (Test-Path $vulkanPlaybackSemanticCanarySummaryPath) {
  $vulkanPlaybackSemanticCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackSemanticCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackSemanticCanarySummary[$key] = $value
      }
    }
  }

  $playbackSemanticCanaryRows = @(
    @("result", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_playback_failure_detail"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.gate_playback_selected_renderer"]),
    @("validation_failure_reason", $vulkanPlaybackSemanticCanarySummary["windows-vulkan-playback-semantic-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackSemanticCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-semantic-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackSemanticCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-semantic canary failed with exit code $vulkanPlaybackSemanticCanaryExitCode. See logs/windows-vulkan-gate-playback-semantic-canary.log and logs/windows-vulkan-gate-playback-semantic-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_backend_semantic_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-backend-semantic-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-backend-semantic-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-backend-semantic-canary.log'
$vulkanPlaybackBackendSemanticCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackBackendSemanticCanarySummaryPath = 'logs/windows-vulkan-gate-playback-backend-semantic-canary-summary.env'
if (Test-Path $vulkanPlaybackBackendSemanticCanarySummaryPath) {
  $vulkanPlaybackBackendSemanticCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackBackendSemanticCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackBackendSemanticCanarySummary[$key] = $value
      }
    }
  }

  $playbackBackendSemanticCanaryRows = @(
    @("result", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_playback_failure_detail"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_playback_selected_renderer"]),
    @("gate_playback_renderer_backend", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.gate_playback_renderer_backend"]),
    @("validation_failure_reason", $vulkanPlaybackBackendSemanticCanarySummary["windows-vulkan-playback-backend-semantic-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Backend Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackBackendSemanticCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Backend Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-backend-semantic-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackBackendSemanticCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-backend-semantic canary failed with exit code $vulkanPlaybackBackendSemanticCanaryExitCode. See logs/windows-vulkan-gate-playback-backend-semantic-canary.log and logs/windows-vulkan-gate-playback-backend-semantic-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_candidates_semantic_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-candidates-semantic-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-candidates-semantic-canary.log'
$vulkanPlaybackCandidatesSemanticCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackCandidatesSemanticCanarySummaryPath = 'logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary.env'
if (Test-Path $vulkanPlaybackCandidatesSemanticCanarySummaryPath) {
  $vulkanPlaybackCandidatesSemanticCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackCandidatesSemanticCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackCandidatesSemanticCanarySummary[$key] = $value
      }
    }
  }

  $playbackCandidatesSemanticCanaryRows = @(
    @("result", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_playback_failure_detail"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_playback_selected_renderer"]),
    @("gate_playback_renderer_backend", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.gate_playback_renderer_backend"]),
    @("validation_failure_reason", $vulkanPlaybackCandidatesSemanticCanarySummary["windows-vulkan-playback-candidates-semantic-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Candidates Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackCandidatesSemanticCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Candidates Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-candidates-semantic-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackCandidatesSemanticCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-candidates-semantic canary failed with exit code $vulkanPlaybackCandidatesSemanticCanaryExitCode. See logs/windows-vulkan-gate-playback-candidates-semantic-canary.log and logs/windows-vulkan-gate-playback-candidates-semantic-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_plan_reason_semantic_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-plan-reason-semantic-canary.log'
$vulkanPlaybackPlanReasonSemanticCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackPlanReasonSemanticCanarySummaryPath = 'logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary.env'
if (Test-Path $vulkanPlaybackPlanReasonSemanticCanarySummaryPath) {
  $vulkanPlaybackPlanReasonSemanticCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackPlanReasonSemanticCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackPlanReasonSemanticCanarySummary[$key] = $value
      }
    }
  }

  $playbackPlanReasonSemanticCanaryRows = @(
    @("result", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_failure_detail"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_selected_renderer"]),
    @("gate_playback_renderer_backend", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.gate_playback_renderer_backend"]),
    @("validation_failure_reason", $vulkanPlaybackPlanReasonSemanticCanarySummary["windows-vulkan-playback-plan-reason-semantic-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Plan Reason Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackPlanReasonSemanticCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Plan Reason Semantic Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-plan-reason-semantic-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackPlanReasonSemanticCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-plan-reason-semantic canary failed with exit code $vulkanPlaybackPlanReasonSemanticCanaryExitCode. See logs/windows-vulkan-gate-playback-plan-reason-semantic-canary.log and logs/windows-vulkan-gate-playback-plan-reason-semantic-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_result_not_pass_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-result-not-pass-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-result-not-pass-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-result-not-pass-canary.log'
$vulkanPlaybackResultNotPassCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackResultNotPassCanarySummaryPath = 'logs/windows-vulkan-gate-playback-result-not-pass-canary-summary.env'
if (Test-Path $vulkanPlaybackResultNotPassCanarySummaryPath) {
  $vulkanPlaybackResultNotPassCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackResultNotPassCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackResultNotPassCanarySummary[$key] = $value
      }
    }
  }

  $playbackResultNotPassCanaryRows = @(
    @("result", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_playback_failure_detail"]),
    @("gate_playback_result", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_playback_result"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_playback_selected_renderer"]),
    @("gate_playback_renderer_backend", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.gate_playback_renderer_backend"]),
    @("validation_failure_reason", $vulkanPlaybackResultNotPassCanarySummary["windows-vulkan-playback-result-not-pass-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Result-Not-Pass Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackResultNotPassCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Result-Not-Pass Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-result-not-pass-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackResultNotPassCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-result-not-pass canary failed with exit code $vulkanPlaybackResultNotPassCanaryExitCode. See logs/windows-vulkan-gate-playback-result-not-pass-canary.log and logs/windows-vulkan-gate-playback-result-not-pass-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_windows_vulkan_gate_playback_command_exit_nonzero_canary.ps1 -ProbeFile $ProbeFile -SampleMs $SampleMs -SummaryOutputPath 'logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary.env' -GateSummaryOutputPath 'logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-gate.env' 2>&1 | Tee-Object -FilePath 'logs/windows-vulkan-gate-playback-command-exit-nonzero-canary.log'
$vulkanPlaybackCommandExitNonzeroCanaryExitCode = $LASTEXITCODE
$vulkanPlaybackCommandExitNonzeroCanarySummaryPath = 'logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary.env'
if (Test-Path $vulkanPlaybackCommandExitNonzeroCanarySummaryPath) {
  $vulkanPlaybackCommandExitNonzeroCanarySummary = @{}
  Get-Content -Path $vulkanPlaybackCommandExitNonzeroCanarySummaryPath | ForEach-Object {
    if ($_ -match '^\s*([^=]+)=(.*)$') {
      $key = $matches[1].Trim()
      $value = $matches[2].Trim()
      if (-not [string]::IsNullOrWhiteSpace($key)) {
        $vulkanPlaybackCommandExitNonzeroCanarySummary[$key] = $value
      }
    }
  }

  $playbackCommandExitNonzeroCanaryRows = @(
    @("result", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.result"]),
    @("expected_gate_exit_code", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.expected_gate_exit_code"]),
    @("actual_gate_exit_code", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.actual_gate_exit_code"]),
    @("gate_summary_file_present", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_summary_file_present"]),
    @("gate_result", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_result"]),
    @("gate_failure_reason", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_failure_reason"]),
    @("gate_playback_contract_valid", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_contract_valid"]),
    @("gate_playback_failure_detail", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_failure_detail"]),
    @("gate_playback_result", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_result"]),
    @("gate_playback_selected_renderer", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_selected_renderer"]),
    @("gate_playback_renderer_backend", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.gate_playback_renderer_backend"]),
    @("validation_failure_reason", $vulkanPlaybackCommandExitNonzeroCanarySummary["windows-vulkan-playback-command-exit-nonzero-canary.validation_failure_reason"])
  )

  "### Windows Vulkan Gate Playback Command-Exit-Nonzero Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| Key | Value |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "| --- | --- |" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  foreach ($row in $playbackCommandExitNonzeroCanaryRows) {
    $rowValue = if ($null -ne $row[1] -and -not [string]::IsNullOrWhiteSpace([string]$row[1])) { [string]$row[1] } else { "n/a" }
    ("| {0} | {1} |" -f $row[0], $rowValue) | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  }
} else {
  "### Windows Vulkan Gate Playback Command-Exit-Nonzero Canary" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "" | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
  "- windows-vulkan-gate-playback-command-exit-nonzero-canary-summary.env not found." | Out-File -FilePath $stepSummaryPath -Encoding utf8 -Append
}
if ($vulkanPlaybackCommandExitNonzeroCanaryExitCode -ne 0) {
  throw "Windows Vulkan gate playback-command-exit-nonzero canary failed with exit code $vulkanPlaybackCommandExitNonzeroCanaryExitCode. See logs/windows-vulkan-gate-playback-command-exit-nonzero-canary.log and logs/windows-vulkan-gate-playback-command-exit-nonzero-canary-summary.env."
}
powershell -ExecutionPolicy Bypass -File .\tools\run_opengl_checks.ps1 -ExecutablePath $ExecutablePath -ProbeFile $ProbeFile
powershell -ExecutionPolicy Bypass -File .\tools\collect_driver_quirk_sample.ps1 -ExecutablePath $ExecutablePath -OutputCsvPath 'build/driver_quirk_samples.csv' -HostLabel 'github-actions-windows' -Notes 'GitHub Actions Windows runner sample'
powershell -ExecutionPolicy Bypass -File .\tools\package_windows.ps1 -BuildDir $BuildDir -Configuration Release -SkipBuild


