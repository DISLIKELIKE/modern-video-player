param(
    [string]$ExecutablePath = "build/Debug/modern-video-player.exe",
    [string]$ProbeFile = "juren-30s.mp4",
    [string]$ForcedFailSessionFile = "",
    [int]$ForcedFailSessionSampleMs = 2200,
    [string]$SamplesFile = "tools/format_regression/format_samples.csv",
    [string]$RegressionOutputFile = ""
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

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }
    return (Join-Path $Root $PathValue)
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

$exePath = Resolve-ProjectPath -Root $repoRoot -PathValue $ExecutablePath
if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

$probePath = Resolve-ProjectPath -Root $repoRoot -PathValue $ProbeFile
if (-not (Test-Path $probePath)) {
    throw "Probe file not found: $probePath"
}

$forcedFailSessionFileToUse = $ForcedFailSessionFile
if ([string]::IsNullOrWhiteSpace($forcedFailSessionFileToUse)) {
    $forcedFailSessionFileToUse = $ProbeFile
}

$forcedFailSessionPath = Resolve-ProjectPath -Root $repoRoot -PathValue $forcedFailSessionFileToUse
if (-not (Test-Path $forcedFailSessionPath)) {
    throw "Forced FailSession file not found: $forcedFailSessionPath"
}

$samplesPath = Resolve-ProjectPath -Root $repoRoot -PathValue $SamplesFile
if (-not (Test-Path $samplesPath)) {
    throw "Samples file not found: $samplesPath"
}

$regressionScriptPath = Resolve-ProjectPath -Root $repoRoot -PathValue "tools/format_regression/run_format_regression.ps1"
if (-not (Test-Path $regressionScriptPath)) {
    throw "Regression script not found: $regressionScriptPath"
}

Write-Output "[1/3] Probing single file (JSON mode): $ProbeFile"

$probeStdout = [System.IO.Path]::GetTempFileName()
$probeStderr = [System.IO.Path]::GetTempFileName()
$probeExitCode = 0
$probeJsonRaw = ""

try {
    $probeProcess = Start-Process -FilePath $exePath `
        -ArgumentList @("--probe-file", $probePath, "--json") `
        -NoNewWindow `
        -Wait `
        -PassThru `
        -RedirectStandardOutput $probeStdout `
        -RedirectStandardError $probeStderr

    $probeExitCode = $probeProcess.ExitCode
    if (Test-Path $probeStdout) {
        $stdoutRaw = Get-Content -Path $probeStdout -Raw -ErrorAction SilentlyContinue
        if ($null -ne $stdoutRaw) {
            $probeJsonRaw = [string]$stdoutRaw
            $probeJsonRaw = $probeJsonRaw.Trim()
        }
    }
    $probeStderrText = ""
    if (Test-Path $probeStderr) {
        $stderrRaw = Get-Content -Path $probeStderr -Raw -ErrorAction SilentlyContinue
        if ($null -ne $stderrRaw) {
            $probeStderrText = [string]$stderrRaw
            $probeStderrText = $probeStderrText.Trim()
        }
    }

    if (-not [string]::IsNullOrWhiteSpace($probeJsonRaw)) {
        try {
            $probeObj = $probeJsonRaw | ConvertFrom-Json -ErrorAction Stop
            Write-Output ("Probe overall: " + [string]$probeObj.overall)
            if ($null -ne $probeObj.video -and $null -ne $probeObj.audio) {
                Write-Output ("Video: " + [string]$probeObj.video.codec + " / Audio: " + [string]$probeObj.audio.codec)
            }
            if ($null -ne $probeObj.recommendation) {
                Write-Output ("Recommendation: " + [string]$probeObj.recommendation.reason)
            }
        } catch {
            Write-Output "Probe JSON parse failed. Raw output:"
            Write-Output $probeJsonRaw
        }
    } else {
        Write-Output "Probe produced empty stdout."
    }

    if (-not [string]::IsNullOrWhiteSpace($probeStderrText)) {
        Write-Output ("Probe stderr: " + $probeStderrText)
    }
} finally {
    Remove-Item -Path $probeStdout -ErrorAction SilentlyContinue
    Remove-Item -Path $probeStderr -ErrorAction SilentlyContinue
}

Write-Output ("Probe exit code: " + $probeExitCode)
if ($probeExitCode -ne 0) {
    Write-Output "Probe step failed; skipping forced FailSession gate and regression."
    exit $probeExitCode
}

Write-Output ("[2/3] Running forced FailSession gate: " + $forcedFailSessionFileToUse +
              " (sample_ms=" + $ForcedFailSessionSampleMs + ")")

$forcedFailSessionProcess = Start-Process -FilePath $exePath `
    -ArgumentList @("--forced-failsession-check", $forcedFailSessionPath, [string]$ForcedFailSessionSampleMs) `
    -NoNewWindow `
    -Wait `
    -PassThru

$forcedFailSessionExitCode = $forcedFailSessionProcess.ExitCode
Write-Output ("Forced FailSession exit code: " + $forcedFailSessionExitCode)
if ($forcedFailSessionExitCode -ne 0) {
    Write-Output "Forced FailSession gate failed; skipping regression."
    exit $forcedFailSessionExitCode
}

Write-Output "[3/3] Running format regression"

$regressionArgs = @(
    "-ExecutionPolicy", "Bypass",
    "-File", $regressionScriptPath,
    "-ExecutablePath", $ExecutablePath,
    "-SamplesFile", $SamplesFile
)

if (-not [string]::IsNullOrWhiteSpace($RegressionOutputFile)) {
    $regressionArgs += @("-OutputFile", $RegressionOutputFile)
}

$regressionProcess = Start-Process -FilePath "powershell" `
    -ArgumentList $regressionArgs `
    -NoNewWindow `
    -Wait `
    -PassThru

$regressionExitCode = $regressionProcess.ExitCode
Write-Output ("Regression exit code: " + $regressionExitCode)
exit $regressionExitCode
