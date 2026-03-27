param(
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [string]$SummaryOutputPath = "logs/windows-vulkan-gate-pass-contract-canary-summary.env",
    [string]$GateSummaryOutputPath = "logs/windows-vulkan-gate-pass-contract-canary-gate.env"
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

$mockExecutable = Join-Path $logsDir "mock_windows_vulkan_pass_contract_canary.cmd"
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
  echo vulkan-diagnostics.startup_renderer_candidates=Vulkan ^> OpenGL ^> SoftwareSDL
  echo vulkan-diagnostics.dependency_source=find_package
  exit /b 0
)
if "%~1"=="--performance-log-check" (
  echo performance-log-check.result=PASS
  echo performance-log-check.startup_selected_renderer=Vulkan
  echo performance-log-check.renderer_backend=Vulkan
  echo performance-log-check.startup_renderer_candidates=Vulkan ^> OpenGL ^> SoftwareSDL
  echo performance-log-check.startup_renderer_plan_reason=renderer-override-env
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
    -RequireVulkanAvailable `
    -SummaryOutputPath $gateSummaryPath
$gateExitCode = $LASTEXITCODE

$gateSummaryFilePresent = Test-Path $gateSummaryPath
$gateSummary = Parse-KeyValueFile -Path $gateSummaryPath
$gateResult = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.result" -DefaultValue "missing"
$gateMode = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.mode" -DefaultValue "missing"
$gateStrictModeEffective = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_effective" -DefaultValue "missing"
$gateFailureReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.failure_reason" -DefaultValue "missing"
$gatePlaybackContractValid = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_contract_valid" -DefaultValue "missing"
$gatePlaybackFailureDetail = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_failure_detail" -DefaultValue "missing"
$gatePlaybackSelectedRenderer = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_selected_renderer" -DefaultValue "missing"
$gatePlaybackRendererBackend = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_renderer_backend" -DefaultValue "missing"

$validationFailureReason = ""
if ($gateExitCode -ne 0) {
    $validationFailureReason = "unexpected-gate-exit-code"
} elseif (-not $gateSummaryFilePresent) {
    $validationFailureReason = "gate-summary-file-missing"
} elseif (-not [string]::Equals($gateResult, "PASS", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-result"
} elseif (-not [string]::Equals($gateMode, "strict", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-mode"
} elseif (-not [string]::Equals($gateStrictModeEffective, "true", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-strict-mode-effective"
} elseif (-not [string]::Equals($gatePlaybackContractValid, "true", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-contract-valid-flag"
} elseif (-not [string]::Equals($gatePlaybackFailureDetail, "none", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-playback-failure-detail"
} elseif (-not [string]::IsNullOrWhiteSpace($gateFailureReason)) {
    $validationFailureReason = "unexpected-failure-reason"
} elseif (-not [string]::Equals($gatePlaybackSelectedRenderer, "Vulkan", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-selected-renderer"
} elseif (-not [string]::Equals($gatePlaybackRendererBackend, "Vulkan", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-renderer-backend"
}

$result = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "PASS" } else { "FAIL" }

$summary = [ordered]@{
    "windows-vulkan-pass-contract-canary.expected_gate_exit_code" = "0"
    "windows-vulkan-pass-contract-canary.actual_gate_exit_code" = [string]$gateExitCode
    "windows-vulkan-pass-contract-canary.gate_summary_file_present" = if ($gateSummaryFilePresent) { "true" } else { "false" }
    "windows-vulkan-pass-contract-canary.gate_result" = $gateResult
    "windows-vulkan-pass-contract-canary.gate_mode" = $gateMode
    "windows-vulkan-pass-contract-canary.gate_strict_mode_effective" = $gateStrictModeEffective
    "windows-vulkan-pass-contract-canary.gate_failure_reason" = $gateFailureReason
    "windows-vulkan-pass-contract-canary.gate_playback_contract_valid" = $gatePlaybackContractValid
    "windows-vulkan-pass-contract-canary.gate_playback_failure_detail" = $gatePlaybackFailureDetail
    "windows-vulkan-pass-contract-canary.gate_playback_selected_renderer" = $gatePlaybackSelectedRenderer
    "windows-vulkan-pass-contract-canary.gate_playback_renderer_backend" = $gatePlaybackRendererBackend
    "windows-vulkan-pass-contract-canary.validation_failure_reason" = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "none" } else { $validationFailureReason }
    "windows-vulkan-pass-contract-canary.result" = $result
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
