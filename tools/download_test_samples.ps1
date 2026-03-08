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

function Resolve-ExecutablePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root,
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if ([System.IO.Path]::IsPathRooted($PathValue)) {
        return $PathValue
    }

    $repoCandidate = Join-Path $Root $PathValue
    if (Test-Path $repoCandidate) {
        return $repoCandidate
    }

    $commandInfo = Get-Command -Name $PathValue -CommandType Application -ErrorAction SilentlyContinue
    if ($null -ne $commandInfo -and -not [string]::IsNullOrWhiteSpace($commandInfo.Source)) {
        return $commandInfo.Source
    }

    return $repoCandidate
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

$ffmpeg = Resolve-ExecutablePath -Root $repoRoot -PathValue $FfmpegPath
if (-not (Test-Path $ffmpeg)) {
    throw "ffmpeg not found: $ffmpeg"
}

$baseSourceDir = Resolve-ProjectPath -Root $repoRoot -PathValue "samples/source"
New-Item -ItemType Directory -Force -Path $baseSourceDir | Out-Null

$sampleDirs = @("samples/mp4", "samples/mkv", "samples/webm", "samples/flv", "samples/ts", "samples/mov", "samples/avi", "samples/m2ts")
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
    mp4_h264_aac_1080p60 = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4"
    mp4_h264_aac_hi100m = "samples/mp4/stress100m__h264_aac__1920x1080__60fps__2ch.mp4"
    mkv_hevc_ac3_ma2 = "samples/mkv/demo__hevc_ac3__3840x2160__60fps__6ch__ma2.mkv"
    webm_vp9_opus = "samples/webm/demo__vp9_opus__1920x1080__30fps__2ch.webm"
    flv_h264_aac = "samples/flv/demo__h264_aac__1280x720__30fps__2ch.flv"
    ts_h264_aac = "samples/ts/demo__h264_aac__1920x1080__25fps__2ch.ts"
    ts_mpeg2video_ac3 = "samples/ts/demo__mpeg2video_ac3__1920x1080__30fps__2ch.ts"
    mkv_h264_eac3 = "samples/mkv/demo__h264_eac3__1920x1080__30fps__2ch.mkv"
    mkv_h264_dts = "samples/mkv/demo__h264_dts__1920x1080__30fps__2ch.mkv"
    webm_vp9_vorbis = "samples/webm/demo__vp9_vorbis__1920x1080__30fps__2ch.webm"
    mov_h264_pcm_s16le = "samples/mov/demo__h264_pcm_s16le__1920x1080__30fps__2ch.mov"
    mov_h264_aac = "samples/mov/demo__h264_aac__1920x1080__30fps__2ch.mov"
    avi_h264_mp3 = "samples/avi/demo__h264_mp3__1280x720__30fps__2ch.avi"
    m2ts_h264_ac3 = "samples/m2ts/demo__h264_ac3__1920x1080__30fps__2ch.m2ts"
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
    "-f", "lavfi", "-i", "sine=frequency=1000:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=60",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mp4_h264_aac_1080p60)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=1000:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=60",
    "-c:v", "libx264", "-preset", "veryfast", "-pix_fmt", "yuv420p",
    "-b:v", "100M", "-minrate", "100M", "-maxrate", "100M", "-bufsize", "200M",
    "-x264-params", "nal-hrd=cbr:force-cfr=1",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mp4_h264_aac_hi100m)
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
    "-f", "lavfi", "-i", "sine=frequency=640:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "mpeg2video", "-q:v", "5", "-pix_fmt", "yuv420p",
    "-c:a", "ac3", "-b:a", "192k", "-ac", "2", "-ar", "48000",
    "-f", "mpegts",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.ts_mpeg2video_ac3)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=990:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "eac3", "-b:a", "192k", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mkv_h264_eac3)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=720:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "dca", "-strict", "-2", "-b:a", "512k", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mkv_h264_dts)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=960:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libvpx-vp9", "-b:v", "0", "-crf", "38", "-row-mt", "1", "-cpu-used", "4",
    "-c:a", "libvorbis", "-q:a", "4", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.webm_vp9_vorbis)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=845:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "pcm_s16le", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mov_h264_pcm_s16le)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=910:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "23", "-pix_fmt", "yuv420p",
    "-c:a", "aac", "-ac", "2", "-ar", "48000",
    "-movflags", "+faststart",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.mov_h264_aac)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=730:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1280:720,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "24", "-pix_fmt", "yuv420p",
    "-c:a", "libmp3lame", "-b:a", "192k", "-ac", "2", "-ar", "48000",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.avi_h264_mp3)
)

Invoke-CheckedProcess -FilePath $ffmpeg -Arguments @(
    "-y", "-hide_banner", "-loglevel", "error",
    "-i", $baseFile,
    "-f", "lavfi", "-i", "sine=frequency=860:sample_rate=48000:duration=$d",
    "-t", "$d",
    "-vf", "scale=1920:1080,fps=30",
    "-c:v", "libx264", "-preset", "veryfast", "-crf", "24", "-pix_fmt", "yuv420p",
    "-c:a", "ac3", "-b:a", "256k", "-ac", "2", "-ar", "48000",
    "-f", "mpegts",
    "-shortest", (Resolve-ProjectPath -Root $repoRoot -PathValue $outputFiles.m2ts_h264_ac3)
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
