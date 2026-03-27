param(
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [string]$SummaryOutputPath = "logs/windows-vulkan-gate-playback-contract-canary-summary.env",
    [string]$GateSummaryOutputPath = "logs/windows-vulkan-gate-playback-contract-canary-gate.env"
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

function Contains-Token {
    param(
        [string]$Csv,
        [string]$Token
    )

    if ([string]::IsNullOrWhiteSpace($Csv)) {
        return $false
    }
    $tokens = $Csv -split "," | ForEach-Object { $_.Trim() } | Where-Object { -not [string]::IsNullOrWhiteSpace($_) }
    return $tokens -contains $Token
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

$mockExecutable = Join-Path $logsDir "mock_windows_vulkan_playback_contract_canary.cmd"
@'
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
  rem Intentionally omit required keys to trigger playback-contract-broken path.
  echo performance-log-check.result=PASS
  echo performance-log-check.startup_renderer_candidates=Vulkan ^> D3D11 ^> SoftwareSDL
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
$gatePlaybackContractValid = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_contract_valid" -DefaultValue "missing"
$gatePlaybackFailureDetail = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_failure_detail" -DefaultValue "missing"
$gatePlaybackMissingRequiredFields = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_missing_required_fields" -DefaultValue "missing"

$validationFailureReason = ""
if ($gateExitCode -ne 2) {
    $validationFailureReason = "unexpected-gate-exit-code"
} elseif (-not $gateSummaryFilePresent) {
    $validationFailureReason = "gate-summary-file-missing"
} elseif (-not [string]::Equals($gateResult, "FAIL", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-result"
} elseif (-not [string]::Equals($gateFailureReason, "vulkan-playback-contract-broken", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-failure-reason"
} elseif (-not [string]::Equals($gatePlaybackContractValid, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-contract-valid-flag"
} elseif (-not [string]::Equals($gatePlaybackFailureDetail, "contract-missing-required-fields", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-playback-failure-detail"
} elseif (-not (Contains-Token -Csv $gatePlaybackMissingRequiredFields -Token "performance-log-check.startup_selected_renderer")) {
    $validationFailureReason = "missing-required-field-startup-selected-renderer-not-reported"
} elseif (-not (Contains-Token -Csv $gatePlaybackMissingRequiredFields -Token "performance-log-check.renderer_backend")) {
    $validationFailureReason = "missing-required-field-renderer-backend-not-reported"
} elseif (-not (Contains-Token -Csv $gatePlaybackMissingRequiredFields -Token "performance-log-check.startup_renderer_plan_reason")) {
    $validationFailureReason = "missing-required-field-plan-reason-not-reported"
}

$result = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "PASS" } else { "FAIL" }

$summary = [ordered]@{
    "windows-vulkan-playback-contract-canary.expected_gate_exit_code" = "2"
    "windows-vulkan-playback-contract-canary.actual_gate_exit_code" = [string]$gateExitCode
    "windows-vulkan-playback-contract-canary.gate_summary_file_present" = if ($gateSummaryFilePresent) { "true" } else { "false" }
    "windows-vulkan-playback-contract-canary.gate_result" = $gateResult
    "windows-vulkan-playback-contract-canary.gate_failure_reason" = $gateFailureReason
    "windows-vulkan-playback-contract-canary.gate_playback_contract_valid" = $gatePlaybackContractValid
    "windows-vulkan-playback-contract-canary.gate_playback_failure_detail" = $gatePlaybackFailureDetail
    "windows-vulkan-playback-contract-canary.gate_playback_missing_required_fields" = $gatePlaybackMissingRequiredFields
    "windows-vulkan-playback-contract-canary.validation_failure_reason" = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "none" } else { $validationFailureReason }
    "windows-vulkan-playback-contract-canary.result" = $result
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
