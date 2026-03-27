param(
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [string]$SummaryOutputPath = "logs/windows-vulkan-gate-unsupported-platform-canary-summary.env",
    [string]$GateSummaryOutputPath = "logs/windows-vulkan-gate-unsupported-platform-canary-gate.env"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-ProjectPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if ([string]::IsNullOrWhiteSpace($PathValue)) {
        return ""
    }
    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }
    return (Join-Path $Root $PathValue)
}

function Parse-KeyValueFile {
    param([string]$Path)

    $result = @{}
    if (-not (Test-Path $Path)) {
        return $result
    }

    Get-Content -Path $Path | ForEach-Object {
        if ($_ -match '^\s*([^=]+)=(.*)$') {
            $key = $matches[1].Trim()
            $value = $matches[2].Trim()
            if (-not [string]::IsNullOrWhiteSpace($key)) {
                $result[$key] = $value
            }
        }
    }
    return $result
}

function Value-OrDefault {
    param(
        [hashtable]$Map,
        [string]$Key,
        [string]$DefaultValue = ""
    )

    if ($Map.ContainsKey($Key)) {
        return [string]$Map[$Key]
    }
    return $DefaultValue
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

$probePath = Resolve-ProjectPath -Root $repoRoot -PathValue $ProbeFile
if (-not (Test-Path $probePath)) {
    throw "Probe file not found: $probePath"
}

$runnerScript = Join-Path $repoRoot "tools/run_windows_vulkan_checks.ps1"
if (-not (Test-Path $runnerScript)) {
    throw "Gate script not found: $runnerScript"
}

$logsDir = Join-Path $repoRoot "logs"
New-Item -ItemType Directory -Path $logsDir -Force | Out-Null

$mockExecutable = Join-Path $logsDir "mock_windows_vulkan_unsupported_platform_canary.cmd"
@'
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
'@ | Set-Content -Path $mockExecutable -Encoding ascii

$gateSummaryPath = Resolve-ProjectPath -Root $repoRoot -PathValue $GateSummaryOutputPath
$gateSummaryDir = Split-Path -Parent $gateSummaryPath
if (-not [string]::IsNullOrWhiteSpace($gateSummaryDir)) {
    New-Item -ItemType Directory -Path $gateSummaryDir -Force | Out-Null
}

& powershell -ExecutionPolicy Bypass -File $runnerScript `
    -ExecutablePath $mockExecutable `
    -ProbeFile $probePath `
    -SampleMs $SampleMs `
    -SummaryOutputPath $gateSummaryPath
$gateExitCode = $LASTEXITCODE

$gateSummaryFilePresent = Test-Path $gateSummaryPath
$gateSummary = Parse-KeyValueFile -Path $gateSummaryPath
$gateResult = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.result" -DefaultValue "missing"
$gateFailureReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.failure_reason" -DefaultValue "missing"
$gateSkipReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.skip_reason" -DefaultValue "missing"
$gatePlaybackCheckExecuted = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_check_executed" -DefaultValue "missing"
$gateMode = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.mode" -DefaultValue "missing"

$validationFailureReason = ""
if ($gateExitCode -ne 2) {
    $validationFailureReason = "unexpected-gate-exit-code"
} elseif (-not $gateSummaryFilePresent) {
    $validationFailureReason = "gate-summary-file-missing"
} elseif (-not [string]::Equals($gateResult, "FAIL", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-result"
} elseif (-not [string]::Equals($gateFailureReason, "unsupported-platform", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-failure-reason"
} elseif (-not [string]::Equals($gatePlaybackCheckExecuted, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-check-executed-flag"
} elseif (-not [string]::IsNullOrWhiteSpace($gateSkipReason)) {
    $validationFailureReason = "unexpected-skip-reason"
}

$result = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "PASS" } else { "FAIL" }

$summary = [ordered]@{
    "windows-vulkan-unsupported-platform-canary.expected_gate_exit_code" = "2"
    "windows-vulkan-unsupported-platform-canary.actual_gate_exit_code" = [string]$gateExitCode
    "windows-vulkan-unsupported-platform-canary.gate_summary_file_present" = if ($gateSummaryFilePresent) { "true" } else { "false" }
    "windows-vulkan-unsupported-platform-canary.gate_result" = $gateResult
    "windows-vulkan-unsupported-platform-canary.gate_mode" = $gateMode
    "windows-vulkan-unsupported-platform-canary.gate_failure_reason" = $gateFailureReason
    "windows-vulkan-unsupported-platform-canary.gate_skip_reason" = $gateSkipReason
    "windows-vulkan-unsupported-platform-canary.gate_playback_check_executed" = $gatePlaybackCheckExecuted
    "windows-vulkan-unsupported-platform-canary.validation_failure_reason" = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "none" } else { $validationFailureReason }
    "windows-vulkan-unsupported-platform-canary.result" = $result
}

$summaryLines = @()
foreach ($entry in $summary.GetEnumerator()) {
    $line = [string]::Format("{0}={1}", $entry.Key, $entry.Value)
    Write-Output $line
    $summaryLines += $line
}

if (-not [string]::IsNullOrWhiteSpace($SummaryOutputPath)) {
    $summaryPath = Resolve-ProjectPath -Root $repoRoot -PathValue $SummaryOutputPath
    $summaryDir = Split-Path -Parent $summaryPath
    if (-not [string]::IsNullOrWhiteSpace($summaryDir)) {
        New-Item -ItemType Directory -Path $summaryDir -Force | Out-Null
    }
    Set-Content -Path $summaryPath -Value ($summaryLines -join [Environment]::NewLine)
}

if ($result -eq "FAIL") {
    exit 2
}
exit 0
