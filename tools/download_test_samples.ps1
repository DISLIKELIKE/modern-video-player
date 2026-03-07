param(
    [string]$FfmpegPath = "external/ffmpeg/bin/ffmpeg.exe",
    [int]$DurationSec = 4
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

function Invoke-CheckedProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        throw "Process failed ($FilePath), exit code: $exitCode"
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
Set-Location $repoRoot

$ffmpeg = Resolve-ProjectPath -Root $repoRoot -PathValue $FfmpegPath
if (-not (Test-Path $ffmpeg)) {
    throw "ffmpeg not found: $ffmpeg"
}

$baseSourceDir = Resolve-ProjectPath -Root $repoRoot -PathValue "samples/source"
New-Item -ItemType Directory -Force -Path $baseSourceDir | Out-Null

$sampleDirs = @("samples/mp4", "samples/mkv", "samples/webm", "samples/flv", "samples/ts")
foreach ($dir in $sampleDirs) {
    New-Item -ItemType Directory -Force -Path (Resolve-ProjectPath -Root $repoRoot -PathValue $dir) | Out-Null
}

$baseFile = Resolve-ProjectPath -Root $repoRoot -PathValue "samples/source/base_h264_aac.mp4"
if (-not (Test-Path $baseFile)) {
    $ProgressPreference = "SilentlyContinue"
    Write-Output "Downloading base sample source..."
    Invoke-WebRequest `
        -Uri "https://test-videos.co.uk/vids/bigbuckbunny/mp4/h264/720/Big_Buck_Bunny_720_10s_1MB.mp4" `
        -OutFile $baseFile
}

$d = [Math]::Max(2, $DurationSec)

$outputFiles = @{
    mp4_hevc_aac = "samples/mp4/demo__hevc_aac__1920x1080__30fps__2ch.mp4"
    mkv_hevc_ac3_ma2 = "samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv"
    webm_vp9_opus = "samples/webm/demo__vp9_opus__1920x1080__30fps__2ch.webm"
    flv_h264_aac = "samples/flv/demo__h264_aac__1280x720__30fps__2ch.flv"
    ts_h264_aac = "samples/ts/demo__h264_aac__1920x1080__25fps__2ch.ts"
    mp4_av1_aac = "samples/mp4/demo__av1_aac__1920x1080__30fps__2ch.mp4"
    mkv_av1_flac_ma2 = "samples/mkv/demo__av1_flac__3840x2160__60fps__8ch__ma2.mkv"
    mkv_vp9_opus_ma2 = "samples/mkv/demo__vp9_opus__1920x1080__30fps__2ch__ma2.mkv"
}

Write-Output "Generating sample set..."

$filterHevcAc3Ma2 = "[0:v]scale=3840:2160,fps=60[v];" +
    "[1:a]aformat=sample_rates=48000:channel_layouts=mono,pan=5.1(side)|FL<c0|FR<c0|FC<c0|LFE<c0|SL<c0|SR<c0[a1];" +
    "[2:a]aformat=sample_rates=48000:channel_layouts=mono,pan=5.1(side)|FL<c0|FR<c0|FC<c0|LFE<c0|SL<c0|SR<c0[a2]"

$filterAv1FlacMa2 = "[0:v]scale=3840:2160,fps=60[v];" +
    "[1:a]aformat=sample_rates=48000:channel_layouts=mono,pan=7.1|FL<c0|FR<c0|FC<c0|LFE<c0|BL<c0|BR<c0|SL<c0|SR<c0[a1];" +
    "[2:a]aformat=sample_rates=48000:channel_layouts=mono,pan=7.1|FL<c0|FR<c0|FC<c0|LFE<c0|BL<c0|BR<c0|SL<c0|SR<c0[a2]"

$filterVp9OpusMa2 = "[0:v]scale=1920:1080,fps=30[v];" +
    "[1:a]aformat=sample_rates=48000:channel_layouts=mono,pan=stereo|c0<c0|c1<c0[a1];" +
    "[2:a]aformat=sample_rates=48000:channel_layouts=mono,pan=stereo|c0<c0|c1<c0[a2]"

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=1000:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx265", "-preset", "fast", "-crf", "30", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mp4_hevc_aac)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=700:sample_rate=48000:duration=$d",
    "-f", "lavfi", "-i", "sine=frequency=1200:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-filter_complex", $filterHevcAc3Ma2,
    "-map", "[v]",
    "-map", "[a1]",
    "-map", "[a2]",
    "-c:v", "libx265", "-preset", "fast", "-crf", "32", "-pix_fmt", "yuv420p",
    "-c:a", "ac3", "-b:a", "384k",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mkv_hevc_ac3_ma2)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=950:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", "38", "-row-mt", "1", "-cpu-used", "4",
    "-c:a", "libopus", "-b:a", "128k", "-ac", "2",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.webm_vp9_opus)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=880:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1280:720,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.flv_h264_aac)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=820:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=25",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "24", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-f", "mpegts",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.ts_h264_aac)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=1020:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libsvtav1", "-preset", "10", "-crf", "42", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mp4_av1_aac)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=680:sample_rate=48000:duration=$d",
    "-f", "lavfi", "-i", "sine=frequency=1350:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-filter_complex", $filterAv1FlacMa2,
    "-map", "[v]",
    "-map", "[a1]",
    "-map", "[a2]",
    "-c:v", "libsvtav1", "-preset", "10", "-crf", "45", "-pix_fmt", "yuv420p",
    "-c:a", "flac",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mkv_av1_flac_ma2)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=760:sample_rate=48000:duration=$d",
    "-f", "lavfi", "-i", "sine=frequency=1120:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-filter_complex", $filterVp9OpusMa2,
    "-map", "[v]",
    "-map", "[a1]",
    "-map", "[a2]",
    "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", "38", "-row-mt", "1", "-cpu-used", "4",
    "-c:a", "libopus", "-b:a", "96k",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mkv_vp9_opus_ma2)
)

Write-Output "Sample preparation complete."
