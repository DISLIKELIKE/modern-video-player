param(
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [string]$SummaryOutputPath = "logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-summary.env",
    [string]$GateSummaryOutputPath = "logs/windows-vulkan-gate-auto-optional-sdk-missing-canary-gate.env"
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

function Set-EnvVarForCanary {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Value,
        [Parameter(Mandatory = $true)]
        [hashtable]$Backup
    )

    $previousValue = [Environment]::GetEnvironmentVariable($Name)
    $Backup[$Name] = @{
        HadValue = ($null -ne $previousValue)
        Value = $previousValue
    }
    [Environment]::SetEnvironmentVariable($Name, $Value)
}

function Restore-EnvVarsForCanary {
    param([hashtable]$Backup)

    foreach ($entry in $Backup.GetEnumerator()) {
        $name = [string]$entry.Key
        $item = $entry.Value
        if ($item.HadValue) {
            [Environment]::SetEnvironmentVariable($name, [string]$item.Value)
        } else {
            [Environment]::SetEnvironmentVariable($name, $null)
        }
    }
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

$mockExecutable = Join-Path $logsDir "mock_windows_vulkan_auto_optional_sdk_missing_canary.cmd"
@'
@echo off
setlocal
if "%~1"=="--vulkan-diagnostics" (
  echo vulkan-diagnostics.platform=Windows
  echo vulkan-diagnostics.supported_platform=true
  echo vulkan-diagnostics.compiled_in=false
  echo vulkan-diagnostics.runtime_available=false
  echo vulkan-diagnostics.result=FAIL
  echo vulkan-diagnostics.selected_renderer=D3D11
  echo vulkan-diagnostics.fallback_target=none
  echo vulkan-diagnostics.startup_renderer_candidates=Vulkan ^> D3D11 ^> SoftwareSDL
  echo vulkan-diagnostics.dependency_source=find_package
  exit /b 0
)
if "%~1"=="--performance-log-check" (
  rem sdk-missing downgrade path should skip playback check
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

$envBackup = @{}
try {
    Set-EnvVarForCanary -Name "MVP_REQUIRE_WINDOWS_VULKAN_CHECKS" -Value "auto" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_SDK_AVAILABLE" -Value "0" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE" -Value "1" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL" -Value "vulkaninfo-sdk-bin" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_SWIFTSHADER_AVAILABLE" -Value "0" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE" -Value "0" -Backup $envBackup
    Set-EnvVarForCanary -Name "MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_DETAIL" -Value "swiftshader-not-run" -Backup $envBackup

    & powershell -ExecutionPolicy Bypass -File $runnerScript `
        -ExecutablePath $mockExecutable `
        -ProbeFile $probePath `
        -SampleMs $SampleMs `
        -SummaryOutputPath $gateSummaryPath
    $gateExitCode = $LASTEXITCODE
} finally {
    Restore-EnvVarsForCanary -Backup $envBackup
}

$gateSummaryFilePresent = Test-Path $gateSummaryPath
$gateSummary = Parse-KeyValueFile -Path $gateSummaryPath
$gateResult = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.result" -DefaultValue "missing"
$gateMode = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.mode" -DefaultValue "missing"
$gateStrictModeEffective = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_effective" -DefaultValue "missing"
$gateSkipReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.skip_reason" -DefaultValue "missing"
$gateFailureReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.failure_reason" -DefaultValue "missing"
$gateAvailabilityFailureDetail = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.vulkan_availability_failure_detail" -DefaultValue "missing"
$gatePlaybackCheckExecuted = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.playback_check_executed" -DefaultValue "missing"
$gateStrictAutoBasis = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_auto_basis" -DefaultValue "missing"
$gateStrictAutoPrerequisitesMet = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_auto_prerequisites_met" -DefaultValue "missing"
$gateStrictAutoRuntimeProbeAny = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_auto_runtime_probe_any_available" -DefaultValue "missing"
$gateStrictAutoRuntimeProbeSource = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.strict_mode_auto_runtime_probe_source" -DefaultValue "missing"

$validationFailureReason = ""
if ($gateExitCode -ne 0) {
    $validationFailureReason = "unexpected-gate-exit-code"
} elseif (-not $gateSummaryFilePresent) {
    $validationFailureReason = "gate-summary-file-missing"
} elseif (-not [string]::Equals($gateResult, "SKIPPED", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-result"
} elseif (-not [string]::Equals($gateMode, "optional", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-mode"
} elseif (-not [string]::Equals($gateStrictModeEffective, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-strict-mode-effective"
} elseif (-not [string]::Equals($gateSkipReason, "vulkan-not-available", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-skip-reason"
} elseif (-not [string]::IsNullOrWhiteSpace($gateFailureReason)) {
    $validationFailureReason = "unexpected-failure-reason"
} elseif (-not [string]::Equals($gateAvailabilityFailureDetail, "compiled-in-false", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-availability-failure-detail"
} elseif (-not [string]::Equals($gatePlaybackCheckExecuted, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-playback-check-executed-flag"
} elseif (-not [string]::Equals($gateStrictAutoBasis, "sdk_and_runtime_probe_or_swiftshader_probe", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-auto-basis"
} elseif (-not [string]::Equals($gateStrictAutoPrerequisitesMet, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-auto-prerequisites-flag"
} elseif (-not [string]::Equals($gateStrictAutoRuntimeProbeAny, "true", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-auto-runtime-probe-any-flag"
} elseif (-not [string]::Equals($gateStrictAutoRuntimeProbeSource, "native", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-auto-runtime-probe-source"
}

$result = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "PASS" } else { "FAIL" }

$summary = [ordered]@{
    "windows-vulkan-auto-optional-sdk-missing-canary.expected_gate_exit_code" = "0"
    "windows-vulkan-auto-optional-sdk-missing-canary.actual_gate_exit_code" = [string]$gateExitCode
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_summary_file_present" = if ($gateSummaryFilePresent) { "true" } else { "false" }
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_result" = $gateResult
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_mode" = $gateMode
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_effective" = $gateStrictModeEffective
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_skip_reason" = $gateSkipReason
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_failure_reason" = $gateFailureReason
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_vulkan_availability_failure_detail" = $gateAvailabilityFailureDetail
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_playback_check_executed" = $gatePlaybackCheckExecuted
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_basis" = $gateStrictAutoBasis
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_prerequisites_met" = $gateStrictAutoPrerequisitesMet
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_any_available" = $gateStrictAutoRuntimeProbeAny
    "windows-vulkan-auto-optional-sdk-missing-canary.gate_strict_mode_auto_runtime_probe_source" = $gateStrictAutoRuntimeProbeSource
    "windows-vulkan-auto-optional-sdk-missing-canary.validation_failure_reason" = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "none" } else { $validationFailureReason }
    "windows-vulkan-auto-optional-sdk-missing-canary.result" = $result
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
