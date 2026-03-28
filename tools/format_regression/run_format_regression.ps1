param(
    [string]$ExecutablePath = "build/Debug/modern-video-player.exe",
    [string]$SamplesFile = "tools/format_regression/format_samples.csv",
    [string]$OutputFile = "",
    [int]$ProbeTimeoutSec = 90,
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

function Invoke-ProbeProcess {
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

function Normalize-Token {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return ""
    }
    return $Text.Trim().ToLowerInvariant()
}

function Normalize-Status {
    param([string]$Text)

    if ([string]::IsNullOrWhiteSpace($Text)) {
        return "UNKNOWN"
    }
    return $Text.Trim().ToUpperInvariant()
}

function Test-EquivalentCodec {
    param(
        [string]$Expected,
        [string]$Actual
    )

    if ([string]::IsNullOrWhiteSpace($Expected) -or [string]::IsNullOrWhiteSpace($Actual)) {
        return $false
    }

    $expectedNorm = Normalize-Token $Expected
    $actualNorm = Normalize-Token $Actual
    if ($expectedNorm -eq $actualNorm) {
        return $true
    }

    if ((@("hevc", "h265") -contains $expectedNorm) -and (@("hevc", "h265") -contains $actualNorm)) {
        return $true
    }

    if ((@("dts", "dca") -contains $expectedNorm) -and (@("dts", "dca") -contains $actualNorm)) {
        return $true
    }

    if (($expectedNorm -eq "pcm" -and $actualNorm.StartsWith("pcm")) -or
        ($actualNorm -eq "pcm" -and $expectedNorm.StartsWith("pcm"))) {
        return $true
    }

    return $false
}

function Escape-MarkdownCell {
    param([string]$Text)

    if ($null -eq $Text) {
        return ""
    }

    $escaped = $Text.Replace("|", "\|")
    $escaped = $escaped.Replace("`r", "")
    $escaped = $escaped.Replace("`n", "<br/>")
    return $escaped
}

function Probe-LinesToMap {
    param([string[]]$OutputLines)

    $probe = @{}
    foreach ($line in $OutputLines) {
        if ($line -match '^probe\.(?<k>[^=]+)=(?<v>.*)$') {
            $probe[$matches["k"]] = $matches["v"]
        }
    }
    return $probe
}

function Convert-ProbeJsonToMap {
    param([object]$ProbeObject)

    $probe = @{}
    if ($null -eq $ProbeObject) {
        return $probe
    }

    if ($null -ne $ProbeObject.open) { $probe["open"] = [string]$ProbeObject.open }
    if ($null -ne $ProbeObject.overall) { $probe["overall"] = [string]$ProbeObject.overall }

    if ($null -ne $ProbeObject.container) {
        if ($null -ne $ProbeObject.container.ext) { $probe["container_ext"] = [string]$ProbeObject.container.ext }
        if ($null -ne $ProbeObject.container.status) { $probe["container_status"] = [string]$ProbeObject.container.status }
    }

    if ($null -ne $ProbeObject.video) {
        if ($null -ne $ProbeObject.video.codec) { $probe["video_codec"] = [string]$ProbeObject.video.codec }
        if ($null -ne $ProbeObject.video.status) { $probe["video_status"] = [string]$ProbeObject.video.status }
    }

    if ($null -ne $ProbeObject.audio) {
        if ($null -ne $ProbeObject.audio.codec) { $probe["audio_codec"] = [string]$ProbeObject.audio.codec }
        if ($null -ne $ProbeObject.audio.status) { $probe["audio_status"] = [string]$ProbeObject.audio.status }
    }

    if ($null -ne $ProbeObject.recommendation -and $null -ne $ProbeObject.recommendation.reason) {
        $probe["reason"] = [string]$ProbeObject.recommendation.reason
    }

    return $probe
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Set-Location $repoRoot

$exePath = Resolve-ProjectPath -Root $repoRoot -PathValue $ExecutablePath
if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

$samplesPath = Resolve-ProjectPath -Root $repoRoot -PathValue $SamplesFile
if (-not (Test-Path $samplesPath)) {
    throw "Samples file not found: $samplesPath"
}

if ([string]::IsNullOrWhiteSpace($OutputFile)) {
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $OutputFile = "docs/reports/FORMAT_REGRESSION_$timestamp.md"
}

$reportPath = Resolve-ProjectPath -Root $repoRoot -PathValue $OutputFile
$reportDir = Split-Path -Path $reportPath -Parent
New-Item -ItemType Directory -Path $reportDir -Force | Out-Null

$samples = Import-Csv -Path $samplesPath
$results = @()

foreach ($sample in $samples) {
    $samplePath = ""
    if ($null -ne $sample.sample_path) {
        $samplePath = $sample.sample_path.Trim()
    }

    if ([string]::IsNullOrWhiteSpace($samplePath)) {
        continue
    }

    $expectedContainer = Normalize-Token $sample.expected_container
    $expectedVideo = Normalize-Token $sample.expected_video_codec
    $expectedAudio = Normalize-Token $sample.expected_audio_codec

    $notes = ""
    if ($null -ne $sample.notes) {
        $notes = $sample.notes.Trim()
    }

    $resolvedSamplePath = Resolve-ProjectPath -Root $repoRoot -PathValue $samplePath
    if (-not (Test-Path $resolvedSamplePath)) {
        $missingNotes = @()
        if (-not [string]::IsNullOrWhiteSpace($notes)) {
            $missingNotes += $notes
        }
        $missingNotes += "sample file not found"

        $results += [PSCustomObject]@{
            SamplePath      = $samplePath
            Exists          = "No"
            Open            = "N/A"
            ContainerStatus = "SKIP"
            VideoStatus     = "SKIP"
            AudioStatus     = "SKIP"
            Overall         = "SKIP"
            ContainerActual = ""
            VideoActual     = ""
            AudioActual     = ""
            Compatibility   = "N/A"
            Notes           = ($missingNotes -join "; ")
        }
        continue
    }

    $processResult = Invoke-ProbeProcess `
        -FilePath $exePath `
        -Arguments @("--probe-file", $resolvedSamplePath, "--json") `
        -TimeoutSec $ProbeTimeoutSec

    $stdoutLines = @()
    $stderrLines = @()
    $processExitCode = $processResult.ExitCode
    if (-not [string]::IsNullOrWhiteSpace($processResult.StdoutText)) {
        $stdoutLines = @($processResult.StdoutText.Replace("`r", "").TrimEnd("`n") -split "`n")
    }
    if (-not [string]::IsNullOrWhiteSpace($processResult.StderrText)) {
        $stderrLines = @($processResult.StderrText.Replace("`r", "").TrimEnd("`n") -split "`n")
    }

    $probe = @{}
    $jsonParsed = $false
    $stdoutText = ($stdoutLines -join "`n").Trim()
    if (-not $processResult.TimedOut) {
        if (-not [string]::IsNullOrWhiteSpace($stdoutText)) {
            try {
                $probeObject = $stdoutText | ConvertFrom-Json -ErrorAction Stop
                $probe = Convert-ProbeJsonToMap -ProbeObject $probeObject
                $jsonParsed = $true
            } catch {
                $probe = Probe-LinesToMap -OutputLines @($stdoutLines + $stderrLines)
            }
        } else {
            $probe = Probe-LinesToMap -OutputLines @($stderrLines)
        }
    }

    $openStatus = if ($probe.ContainsKey("open")) { Normalize-Status $probe["open"] } else { "FAIL" }
    $probeOverall = if ($probe.ContainsKey("overall")) { Normalize-Status $probe["overall"] } else { "FAIL" }
    $containerStatus = if ($probe.ContainsKey("container_status")) { Normalize-Status $probe["container_status"] } else { "FAIL" }
    $videoStatus = if ($probe.ContainsKey("video_status")) { Normalize-Status $probe["video_status"] } else { "FAIL" }
    $audioStatus = if ($probe.ContainsKey("audio_status")) { Normalize-Status $probe["audio_status"] } else { "FAIL" }

    $containerActual = if ($probe.ContainsKey("container_ext")) { Normalize-Token $probe["container_ext"] } else { "" }
    $videoActual = if ($probe.ContainsKey("video_codec")) { Normalize-Token $probe["video_codec"] } else { "" }
    $audioActual = if ($probe.ContainsKey("audio_codec")) { Normalize-Token $probe["audio_codec"] } else { "" }

    $compatibilityNotes = @()
    if (-not [string]::IsNullOrWhiteSpace($expectedContainer) -and $containerActual -ne $expectedContainer) {
        $compatibilityNotes += "container expected=$expectedContainer actual=$containerActual"
    }
    if (-not [string]::IsNullOrWhiteSpace($expectedVideo) -and -not (Test-EquivalentCodec -Expected $expectedVideo -Actual $videoActual)) {
        $compatibilityNotes += "video expected=$expectedVideo actual=$videoActual"
    }
    if (-not [string]::IsNullOrWhiteSpace($expectedAudio) -and -not (Test-EquivalentCodec -Expected $expectedAudio -Actual $audioActual)) {
        $compatibilityNotes += "audio expected=$expectedAudio actual=$audioActual"
    }

    $compatibility = if ($compatibilityNotes.Count -eq 0) { "PASS" } else { "PARTIAL" }
    $overall = "PASS"
    if ($openStatus -eq "FAIL" -or $probeOverall -eq "FAIL" -or $containerStatus -eq "FAIL" -or $videoStatus -eq "FAIL" -or $audioStatus -eq "FAIL") {
        $overall = "FAIL"
    } elseif ($probeOverall -eq "PARTIAL" -or $containerStatus -eq "PARTIAL" -or $videoStatus -eq "PARTIAL" -or $audioStatus -eq "PARTIAL" -or $compatibility -eq "PARTIAL") {
        $overall = "PARTIAL"
    }

    $finalNotes = @()
    if (-not [string]::IsNullOrWhiteSpace($notes)) {
        $finalNotes += $notes
    }
    if ($processResult.TimedOut) {
        $finalNotes += ("probe timed out after " + $ProbeTimeoutSec + "s")
    }
    if (-not $jsonParsed) {
        $finalNotes += "probe json parse fallback"
    }
    if ($probe.ContainsKey("reason")) {
        $finalNotes += ("probe reason: " + $probe["reason"])
    }
    if ($processExitCode -ne 0) {
        $finalNotes += "probe process exit code=$processExitCode"
    }
    if ($stderrLines.Count -gt 0) {
        $finalNotes += ("probe stderr lines=" + $stderrLines.Count)
    }
    if ($compatibilityNotes.Count -gt 0) {
        $finalNotes += ($compatibilityNotes -join "; ")
    }

    $results += [PSCustomObject]@{
        SamplePath      = $samplePath
        Exists          = "Yes"
        Open            = $openStatus
        ContainerStatus = $containerStatus
        VideoStatus     = $videoStatus
        AudioStatus     = $audioStatus
        Overall         = $overall
        ContainerActual = $containerActual
        VideoActual     = $videoActual
        AudioActual     = $audioActual
        Compatibility   = $compatibility
        Notes           = ($finalNotes -join "; ")
    }
}

$total = $results.Count
$passCount = @($results | Where-Object { $_.Overall -eq "PASS" }).Count
$partialCount = @($results | Where-Object { $_.Overall -eq "PARTIAL" }).Count
$failCount = @($results | Where-Object { $_.Overall -eq "FAIL" }).Count
$skipCount = @($results | Where-Object { $_.Overall -eq "SKIP" }).Count

$reportLines = @()
$reportLines += "# Format Regression Report"
$reportLines += ""
$reportLines += "Generated: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')"
$reportLines += "Tool: run_format_regression.ps1"
$reportLines += ("Executable: " + $ExecutablePath)
$reportLines += ("Sample List: " + $SamplesFile)
$reportLines += ""
$reportLines += "## Summary"
$reportLines += ""
$reportLines += "- Total: $total"
$reportLines += "- PASS: $passCount"
$reportLines += "- PARTIAL: $partialCount"
$reportLines += "- FAIL: $failCount"
$reportLines += "- SKIP: $skipCount"
$reportLines += ""
$reportLines += "## Details"
$reportLines += ""
$reportLines += "| Sample | Exists | Open | Container | Video | Audio | Overall | Compatibility | Notes |"
$reportLines += "|---|---|---|---|---|---|---|---|---|"

foreach ($item in $results) {
    $reportLines += "| $(Escape-MarkdownCell $item.SamplePath) | $($item.Exists) | $($item.Open) | $($item.ContainerStatus) ($($item.ContainerActual)) | $($item.VideoStatus) ($($item.VideoActual)) | $($item.AudioStatus) ($($item.AudioActual)) | $($item.Overall) | $($item.Compatibility) | $(Escape-MarkdownCell $item.Notes) |"
}

Set-Content -Path $reportPath -Value $reportLines -Encoding UTF8
Write-Output "Format regression report written to: $reportPath"

if ($failCount -gt 0) {
    exit 2
}
if ($partialCount -gt 0) {
    exit 1
}
exit 0
