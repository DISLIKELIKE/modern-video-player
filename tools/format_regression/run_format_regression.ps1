param(
    [string]$ExecutablePath = "build/Debug/modern-video-player.exe",
    [string]$SamplesFile = "tools/format_regression/format_samples.csv",
    [string]$OutputFile = ""
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

    $stdoutFile = [System.IO.Path]::GetTempFileName()
    $stderrFile = [System.IO.Path]::GetTempFileName()
    $outputLines = @()
    $processExitCode = 0

    try {
        $process = Start-Process -FilePath $exePath `
            -ArgumentList @("--probe-file", $resolvedSamplePath) `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError $stderrFile

        $processExitCode = $process.ExitCode
        if (Test-Path $stdoutFile) {
            $outputLines += @((Get-Content -Path $stdoutFile -ErrorAction SilentlyContinue) | ForEach-Object { [string]$_ })
        }
        if (Test-Path $stderrFile) {
            $outputLines += @((Get-Content -Path $stderrFile -ErrorAction SilentlyContinue) | ForEach-Object { [string]$_ })
        }
    } finally {
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
    }

    $probe = Probe-LinesToMap -OutputLines $outputLines

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
    if (-not [string]::IsNullOrWhiteSpace($expectedVideo) -and $videoActual -ne $expectedVideo) {
        $compatibilityNotes += "video expected=$expectedVideo actual=$videoActual"
    }
    if (-not [string]::IsNullOrWhiteSpace($expectedAudio) -and $audioActual -ne $expectedAudio) {
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
    if ($probe.ContainsKey("reason")) {
        $finalNotes += ("probe reason: " + $probe["reason"])
    }
    if ($processExitCode -ne 0) {
        $finalNotes += "probe process exit code=$processExitCode"
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
