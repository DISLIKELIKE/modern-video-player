param(
    [string]$ExecutablePath = "build/Debug/modern-video-player.exe",
    [string]$ProbeFile = "juren-30s.mp4",
    [string]$ForcedFailSessionFile = "",
    [int]$ForcedFailSessionSampleMs = 2200,
    [string]$SamplesFile = "tools/format_regression/format_samples.csv",
    [string]$RegressionOutputFile = "",
    [int]$ProbeTimeoutSec = 120,
    [int]$ForcedFailSessionTimeoutSec = 240,
    [int]$RegressionTimeoutSec = 900,
    [int]$TimeoutExitCode = 124
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

function Resolve-PowerShellExecutable {
    $pwsh = Get-Command -Name "pwsh" -CommandType Application -ErrorAction SilentlyContinue
    if ($null -ne $pwsh -and -not [string]::IsNullOrWhiteSpace($pwsh.Source)) {
        return $pwsh.Source
    }

    $windowsPowerShell = Get-Command -Name "powershell" -CommandType Application -ErrorAction SilentlyContinue
    if ($null -ne $windowsPowerShell -and -not [string]::IsNullOrWhiteSpace($windowsPowerShell.Source)) {
        return $windowsPowerShell.Source
    }

    throw "No PowerShell executable found for child process invocation."
}

function Stop-ProcessTree {
    param([int]$ProcessId)

    if ($ProcessId -le 0) {
        return
    }

    $taskKill = Get-Command -Name "taskkill.exe" -ErrorAction SilentlyContinue
    if ($null -ne $taskKill) {
        & $taskKill.Source /PID $ProcessId /T /F *> $null
        return
    }

    Stop-Process -Id $ProcessId -Force -ErrorAction SilentlyContinue
}

function Invoke-ProcessWithCapture {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,
        [int]$TimeoutSec = 0
    )

    $stdoutFile = [System.IO.Path]::GetTempFileName()
    $stderrFile = [System.IO.Path]::GetTempFileName()
    $exitCodeFile = [System.IO.Path]::GetTempFileName()
    $process = $null

    try {
        $escapedFilePath = $FilePath.Replace("'", "''")
        $escapedExitCodePath = $exitCodeFile.Replace("'", "''")
        $argumentLines = @()
        foreach ($argument in $Arguments) {
            $argumentLines += ("    '" + ([string]$argument).Replace("'", "''") + "'")
        }
        $wrapperScript = @"
`$ErrorActionPreference = 'Stop'
`$ProgressPreference = 'SilentlyContinue'
`$arguments = @(
$($argumentLines -join ",`n")
)
& '$escapedFilePath' @arguments
`$code = `$LASTEXITCODE
if (`$null -eq `$code) {
    `$code = 0
}
Set-Content -Path '$escapedExitCodePath' -Value ([string]`$code) -Encoding ASCII
exit [int]`$code
"@
        $encodedCommand = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($wrapperScript))
        $childPowerShell = Resolve-PowerShellExecutable

        $process = Start-Process -FilePath $childPowerShell `
            -ArgumentList @("-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-EncodedCommand", $encodedCommand) `
            -NoNewWindow `
            -PassThru `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError $stderrFile

        $timedOut = $false
        if ($TimeoutSec -gt 0) {
            $waitTimeoutMs = [Math]::Max(1, $TimeoutSec) * 1000
            if (-not $process.WaitForExit($waitTimeoutMs)) {
                $timedOut = $true
                Stop-ProcessTree -ProcessId $process.Id
                $process.WaitForExit()
            }
        } else {
            $process.WaitForExit()
        }

        if (-not $timedOut) {
            $process.WaitForExit()
        }

        $stdoutText = ""
        $stderrText = ""
        if (Test-Path $stdoutFile) {
            $stdoutRaw = Get-Content -Path $stdoutFile -Raw -ErrorAction SilentlyContinue
            if ($null -ne $stdoutRaw) {
                $stdoutText = [string]$stdoutRaw
            }
        }
        if (Test-Path $stderrFile) {
            $stderrRaw = Get-Content -Path $stderrFile -Raw -ErrorAction SilentlyContinue
            if ($null -ne $stderrRaw) {
                $stderrText = [string]$stderrRaw
            }
        }

        $exitCode = $TimeoutExitCode
        if (-not $timedOut) {
            $exitCode = -1
            if (Test-Path $exitCodeFile) {
                $exitCodeRaw = Get-Content -Path $exitCodeFile -Raw -ErrorAction SilentlyContinue
                $parsedExitCode = 0
                if (-not [string]::IsNullOrWhiteSpace($exitCodeRaw) -and
                    [int]::TryParse($exitCodeRaw.Trim(), [ref]$parsedExitCode)) {
                    $exitCode = $parsedExitCode
                }
            }
        }

        return [PSCustomObject]@{
            ExitCode   = $exitCode
            TimedOut   = $timedOut
            StdoutText = $stdoutText
            StderrText = $stderrText
        }
    } finally {
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
        Remove-Item -Path $exitCodeFile -ErrorAction SilentlyContinue
    }
}

function Invoke-ProcessWithPassthrough {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,
        [int]$TimeoutSec = 0
    )

    $exitCodeFile = [System.IO.Path]::GetTempFileName()
    $process = $null

    try {
        $escapedFilePath = $FilePath.Replace("'", "''")
        $escapedExitCodePath = $exitCodeFile.Replace("'", "''")
        $argumentLines = @()
        foreach ($argument in $Arguments) {
            $argumentLines += ("    '" + ([string]$argument).Replace("'", "''") + "'")
        }
        $wrapperScript = @"
`$ErrorActionPreference = 'Stop'
`$ProgressPreference = 'SilentlyContinue'
`$arguments = @(
$($argumentLines -join ",`n")
)
& '$escapedFilePath' @arguments
`$code = `$LASTEXITCODE
if (`$null -eq `$code) {
    `$code = 0
}
Set-Content -Path '$escapedExitCodePath' -Value ([string]`$code) -Encoding ASCII
exit [int]`$code
"@
        $encodedCommand = [Convert]::ToBase64String([System.Text.Encoding]::Unicode.GetBytes($wrapperScript))
        $childPowerShell = Resolve-PowerShellExecutable

        $process = Start-Process -FilePath $childPowerShell `
            -ArgumentList @("-NoProfile", "-NonInteractive", "-ExecutionPolicy", "Bypass", "-EncodedCommand", $encodedCommand) `
            -NoNewWindow `
            -PassThru

        $timedOut = $false
        if ($TimeoutSec -gt 0) {
            $waitTimeoutMs = [Math]::Max(1, $TimeoutSec) * 1000
            if (-not $process.WaitForExit($waitTimeoutMs)) {
                $timedOut = $true
                Stop-ProcessTree -ProcessId $process.Id
                $process.WaitForExit()
            }
        } else {
            $process.WaitForExit()
        }

        if (-not $timedOut) {
            $process.WaitForExit()
        }

        $exitCode = $TimeoutExitCode
        if (-not $timedOut) {
            $exitCode = -1
            if (Test-Path $exitCodeFile) {
                $exitCodeRaw = Get-Content -Path $exitCodeFile -Raw -ErrorAction SilentlyContinue
                $parsedExitCode = 0
                if (-not [string]::IsNullOrWhiteSpace($exitCodeRaw) -and
                    [int]::TryParse($exitCodeRaw.Trim(), [ref]$parsedExitCode)) {
                    $exitCode = $parsedExitCode
                }
            }
        }

        return [PSCustomObject]@{
            ExitCode = $exitCode
            TimedOut = $timedOut
        }
    } finally {
        Remove-Item -Path $exitCodeFile -ErrorAction SilentlyContinue
    }
}

function Write-CapturedText {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return
    }

    $normalized = $Text.Replace("`r", "").TrimEnd("`n")
    if ([string]::IsNullOrWhiteSpace($normalized)) {
        return
    }

    foreach ($line in ($normalized -split "`n")) {
        Write-Output $line
    }
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

$probeResult = Invoke-ProcessWithCapture `
    -FilePath $exePath `
    -Arguments @("--probe-file", $probePath, "--json") `
    -TimeoutSec $ProbeTimeoutSec

$probeExitCode = $probeResult.ExitCode
$probeJsonRaw = ""
if (-not [string]::IsNullOrWhiteSpace($probeResult.StdoutText)) {
    $probeJsonRaw = $probeResult.StdoutText.Trim()
}
$probeStderrText = ""
if (-not [string]::IsNullOrWhiteSpace($probeResult.StderrText)) {
    $probeStderrText = $probeResult.StderrText.Trim()
}

if ($probeResult.TimedOut) {
    Write-CapturedText -Text $probeResult.StdoutText
    Write-CapturedText -Text $probeResult.StderrText
    Write-Output ("Probe timed out after " + $ProbeTimeoutSec + "s.")
    exit $TimeoutExitCode
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

Write-Output ("Probe exit code: " + $probeExitCode)
if ($probeExitCode -ne 0) {
    Write-Output "Probe step failed; skipping forced FailSession gate and regression."
    exit $probeExitCode
}

Write-Output ("[2/3] Running forced FailSession gate: " + $forcedFailSessionFileToUse +
              " (sample_ms=" + $ForcedFailSessionSampleMs + ")")

$forcedFailSessionResult = Invoke-ProcessWithPassthrough `
    -FilePath $exePath `
    -Arguments @("--forced-failsession-check", $forcedFailSessionPath, [string]$ForcedFailSessionSampleMs) `
    -TimeoutSec $ForcedFailSessionTimeoutSec

if ($forcedFailSessionResult.TimedOut) {
    Write-Output ("Forced FailSession gate timed out after " + $ForcedFailSessionTimeoutSec + "s.")
    exit $TimeoutExitCode
}

$forcedFailSessionExitCode = $forcedFailSessionResult.ExitCode
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
    "-SamplesFile", $SamplesFile,
    "-ProbeTimeoutSec", [string]$ProbeTimeoutSec
)

if (-not [string]::IsNullOrWhiteSpace($RegressionOutputFile)) {
    $regressionArgs += @("-OutputFile", $RegressionOutputFile)
}

$childPowerShell = Resolve-PowerShellExecutable
$regressionResult = Invoke-ProcessWithCapture `
    -FilePath $childPowerShell `
    -Arguments $regressionArgs `
    -TimeoutSec $RegressionTimeoutSec

Write-CapturedText -Text $regressionResult.StdoutText
Write-CapturedText -Text $regressionResult.StderrText

if ($regressionResult.TimedOut) {
    Write-Output ("Format regression timed out after " + $RegressionTimeoutSec + "s.")
    exit $TimeoutExitCode
}

$regressionExitCode = $regressionResult.ExitCode
Write-Output ("Regression exit code: " + $regressionExitCode)
exit $regressionExitCode
