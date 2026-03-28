param(
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [string]$SummaryOutputPath = "logs/windows-vulkan-gate-contract-canary-summary.env",
    [string]$GateSummaryOutputPath = "logs/windows-vulkan-gate-contract-canary-gate.env"
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

$cmdExecutable = Join-Path $env:WINDIR "System32\cmd.exe"
if (-not (Test-Path $cmdExecutable)) {
    throw "cmd.exe not found for canary scenario: $cmdExecutable"
}

$gateSummaryPath = Resolve-ProjectPath -Root $repoRoot -PathValue $GateSummaryOutputPath
$gateSummaryDir = Split-Path -Parent $gateSummaryPath
if (-not [string]::IsNullOrWhiteSpace($gateSummaryDir)) {
    New-Item -ItemType Directory -Path $gateSummaryDir -Force | Out-Null
}

# Canary scenario:
# run gate against cmd.exe so diagnostics contract is intentionally broken.
& powershell -ExecutionPolicy Bypass -File $runnerScript `
    -ExecutablePath $cmdExecutable `
    -ProbeFile $probePath `
    -SampleMs $SampleMs `
    -SummaryOutputPath $gateSummaryPath
$gateExitCode = $LASTEXITCODE

$gateSummaryFilePresent = Test-Path $gateSummaryPath
$gateSummary = Parse-KeyValueFile -Path $gateSummaryPath
$gateResult = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.result" -DefaultValue "missing"
$gateFailureReason = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.failure_reason" -DefaultValue "missing"
$gateDiagContractValid = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.diag_contract_valid" -DefaultValue "missing"
$gateAvailabilityFailureDetail = Value-OrDefault -Map $gateSummary -Key "windows-vulkan-check.vulkan_availability_failure_detail" -DefaultValue "missing"

$validationFailureReason = ""
if ($gateExitCode -ne 2) {
    $validationFailureReason = "unexpected-gate-exit-code"
} elseif (-not $gateSummaryFilePresent) {
    $validationFailureReason = "gate-summary-file-missing"
} elseif (-not [string]::Equals($gateResult, "FAIL", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-gate-result"
} elseif (-not [string]::Equals($gateFailureReason, "vulkan-diagnostics-contract-broken", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-failure-reason"
} elseif (-not [string]::Equals($gateDiagContractValid, "false", [System.StringComparison]::OrdinalIgnoreCase)) {
    $validationFailureReason = "unexpected-diag-contract-valid-flag"
} elseif (-not [string]::Equals($gateAvailabilityFailureDetail, "diag-contract-missing-required-fields", [System.StringComparison]::Ordinal)) {
    $validationFailureReason = "unexpected-availability-failure-detail"
}

$result = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "PASS" } else { "FAIL" }

$summary = [ordered]@{
    "windows-vulkan-contract-canary.expected_gate_exit_code" = "2"
    "windows-vulkan-contract-canary.actual_gate_exit_code" = [string]$gateExitCode
    "windows-vulkan-contract-canary.gate_summary_file_present" = if ($gateSummaryFilePresent) { "true" } else { "false" }
    "windows-vulkan-contract-canary.gate_result" = $gateResult
    "windows-vulkan-contract-canary.gate_failure_reason" = $gateFailureReason
    "windows-vulkan-contract-canary.gate_diag_contract_valid" = $gateDiagContractValid
    "windows-vulkan-contract-canary.gate_vulkan_availability_failure_detail" = $gateAvailabilityFailureDetail
    "windows-vulkan-contract-canary.validation_failure_reason" = if ([string]::IsNullOrWhiteSpace($validationFailureReason)) { "none" } else { $validationFailureReason }
    "windows-vulkan-contract-canary.result" = $result
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
