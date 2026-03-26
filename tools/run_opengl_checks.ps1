param(
    [string]$ExecutablePath = "build/Release/modern-video-player.exe",
    [string]$ProbeFile = "juren-30s.mp4",
    [int]$SampleMs = 1800,
    [string]$TenBitFile = "",
    [int]$TenBitSampleMs = 2200,
    [string]$SubtitleMediaFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [string]$SubtitleFile = "samples/subtitles/opengl_ass_transform_vector_font_validation.ass",
    [string]$TransitionSubtitleFile = "samples/subtitles/opengl_ass_transform_transition_validation.ass",
    [string]$EmbeddedSubtitleBaseMediaFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [string]$EmbeddedAssSubtitleFile = "samples/subtitles/opengl_ass_transform_transition_validation.ass",
    [string]$EmbeddedAssMediaFile = "",
    [string]$EmbeddedTextMediaFile = "",
    [string]$EmbeddedDvdMediaFile = "",
    [string]$EmbeddedPgsSourceFile = "",
    [string]$EmbeddedPgsMediaFile = "",
    [string[]]$SubtitleStyleFiles = @(
        "samples/subtitles/opengl_ass_style_validation.ass",
        "samples/subtitles/opengl_ass_karaoke_clip_validation.ass",
        "samples/subtitles/opengl_ass_animation_validation.ass",
        "samples/subtitles/opengl_ass_transform_vector_font_validation.ass",
        "samples/subtitles/opengl_ass_transform_transition_validation.ass"
    )
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

function Set-TemporaryEnvironment {
    param([hashtable]$Values)

    $backup = @{}
    foreach ($key in $Values.Keys) {
        $envPath = "Env:$key"
        if (Test-Path $envPath) {
            $backup[$key] = (Get-Item $envPath).Value
        } else {
            $backup[$key] = $null
        }
        Set-Item -Path $envPath -Value ([string]$Values[$key])
    }
    return $backup
}

function Restore-Environment {
    param(
        [hashtable]$Backup,
        [hashtable]$Values
    )

    foreach ($key in $Values.Keys) {
        $envPath = "Env:$key"
        if ($Backup.ContainsKey($key) -and $null -ne $Backup[$key]) {
            Set-Item -Path $envPath -Value ([string]$Backup[$key])
        } else {
            Remove-Item -Path $envPath -ErrorAction SilentlyContinue
        }
    }
}

function Test-RequiredPatterns {
    param(
        [string]$Text,
        [string[]]$Patterns
    )

    foreach ($pattern in $Patterns) {
        if ($Text -notmatch $pattern) {
            Write-Output ("Missing pattern: " + $pattern)
            return $false
        }
    }
    return $true
}

function Show-SummaryLines {
    param(
        [string]$Text,
        [string[]]$Patterns
    )

    $lines = $Text -split "`r?`n"
    foreach ($line in $lines) {
        foreach ($pattern in $Patterns) {
            if ($line -match $pattern) {
                Write-Output $line
                break
            }
        }
    }
}

function Invoke-ExternalCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    $stdoutFile = [System.IO.Path]::GetTempFileName()
    $stderrFile = [System.IO.Path]::GetTempFileName()
    try {
        $process = Start-Process -FilePath $Executable `
            -ArgumentList $Arguments `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError $stderrFile

        $stdoutText = if (Test-Path $stdoutFile) { [string](Get-Content -Path $stdoutFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        $stderrText = if (Test-Path $stderrFile) { [string](Get-Content -Path $stderrFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        return @{
            ExitCode = $process.ExitCode
            Output = ($stdoutText + [Environment]::NewLine + $stderrText).Trim()
        }
    } finally {
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
    }
}

function Invoke-PlayerCheck {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,
        [Parameter(Mandatory = $true)]
        [string]$Executable,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,
        [hashtable]$EnvOverrides = @{},
        [string[]]$RequiredPatterns = @(),
        [string[]]$SummaryPatterns = @("result=", "renderer_backend=", "decoder_backend=")
    )

    Write-Output ""
    Write-Output ("== " + $Name + " ==")
    Write-Output ("Command: " + $Executable + " " + ($Arguments -join " "))

    $stdoutFile = [System.IO.Path]::GetTempFileName()
    $stderrFile = [System.IO.Path]::GetTempFileName()
    $backup = Set-TemporaryEnvironment -Values $EnvOverrides
    try {
        $process = Start-Process -FilePath $Executable `
            -ArgumentList $Arguments `
            -NoNewWindow `
            -Wait `
            -PassThru `
            -RedirectStandardOutput $stdoutFile `
            -RedirectStandardError $stderrFile

        $exitCode = $process.ExitCode
        $stdoutText = if (Test-Path $stdoutFile) { [string](Get-Content -Path $stdoutFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        $stderrText = if (Test-Path $stderrFile) { [string](Get-Content -Path $stderrFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        $text = ($stdoutText + [Environment]::NewLine + $stderrText).Trim()
    } finally {
        Restore-Environment -Backup $backup -Values $EnvOverrides
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
    }

    Show-SummaryLines -Text $text -Patterns $SummaryPatterns
    Write-Output ("Exit code: " + $exitCode)

    $patternsOk = Test-RequiredPatterns -Text $text -Patterns $RequiredPatterns
    $ok = ($exitCode -eq 0) -and $patternsOk
    Write-Output ("Status: " + ($(if ($ok) { "PASS" } else { "FAIL" })))
    if (-not $ok) {
        Write-Output "--- Full output start ---"
        Write-Output $text.TrimEnd()
        Write-Output "--- Full output end ---"
    }
    return $ok
}

function Ensure-TenBitSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $ffmpegCommand = Get-Command ffmpeg.exe -ErrorAction SilentlyContinue
    if (-not $ffmpegCommand) {
        throw "ffmpeg.exe not found in PATH, and no ten-bit sample was provided."
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    Write-Host ("Generating temporary 10-bit sample: " + $PathValue)
    $result = Invoke-ExternalCommand -Executable $ffmpegCommand.Source -Arguments @(
        "-y",
        "-f", "lavfi",
        "-i", "testsrc2=size=1280x720:rate=30",
        "-t", "3",
        "-pix_fmt", "yuv420p10le",
        "-c:v", "libx265",
        "-preset", "ultrafast",
        "-x265-params", "profile=main10",
        "-an",
        $PathValue
    )
    if ($result.ExitCode -ne 0 -or -not (Test-Path $PathValue)) {
        throw "Failed to generate 10-bit validation sample: $PathValue`n$result.Output"
    }
    return $PathValue
}

function Ensure-EmbeddedAssSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,
        [Parameter(Mandatory = $true)]
        [string]$BaseMediaPath,
        [Parameter(Mandatory = $true)]
        [string]$SubtitlePath
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $ffmpegCommand = Get-Command ffmpeg.exe -ErrorAction SilentlyContinue
    if (-not $ffmpegCommand) {
        throw "ffmpeg.exe not found in PATH, and no embedded ASS sample was provided."
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    Write-Host ("Generating temporary embedded ASS sample: " + $PathValue)
    $result = Invoke-ExternalCommand -Executable $ffmpegCommand.Source -Arguments @(
        "-y",
        "-i", $BaseMediaPath,
        "-i", $SubtitlePath,
        "-map", "0:v",
        "-map", "0:a?",
        "-map", "1:0",
        "-c:v", "copy",
        "-c:a", "copy",
        "-c:s", "ass",
        $PathValue
    )
    if ($result.ExitCode -ne 0 -or -not (Test-Path $PathValue)) {
        throw "Failed to generate embedded ASS validation sample: $PathValue`n$($result.Output)"
    }
    return $PathValue
}

function Ensure-EmbeddedTextSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,
        [Parameter(Mandatory = $true)]
        [string]$BaseMediaPath
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $ffmpegCommand = Get-Command ffmpeg.exe -ErrorAction SilentlyContinue
    if (-not $ffmpegCommand) {
        throw "ffmpeg.exe not found in PATH, and no embedded text subtitle sample was provided."
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    $subtitlePath = Join-Path $parent "embedded-text-validation.srt"
@"
1
00:00:00,000 --> 00:00:01,500
EMBEDDED TEXT PATH

2
00:00:01,800 --> 00:00:03,000
MOV_TEXT VALIDATION
"@ | Set-Content -Path $subtitlePath -Encoding utf8

    Write-Host ("Generating temporary embedded text subtitle sample: " + $PathValue)
    $result = Invoke-ExternalCommand -Executable $ffmpegCommand.Source -Arguments @(
        "-y",
        "-i", $BaseMediaPath,
        "-i", $subtitlePath,
        "-map", "0:v",
        "-map", "0:a?",
        "-map", "1:0",
        "-c:v", "copy",
        "-c:a", "copy",
        "-c:s", "mov_text",
        $PathValue
    )
    if ($result.ExitCode -ne 0 -or -not (Test-Path $PathValue)) {
        throw "Failed to generate embedded text subtitle validation sample: $PathValue`n$($result.Output)"
    }
    return $PathValue
}

function Ensure-EmbeddedDvdSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,
        [Parameter(Mandatory = $true)]
        [string]$BaseMediaPath
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    $sourceUrl = "https://samples.ffmpeg.org/archive/extension/vob/mpeg+mpeg2video+ac3+dvdsub+Tr_Plan.vob"
    Write-Host ("Downloading temporary DVD subtitle sample: " + $PathValue)
    Invoke-WebRequest -Uri $sourceUrl -OutFile $PathValue
    if (-not (Test-Path $PathValue)) {
        throw "Failed to download DVD subtitle validation sample: $PathValue"
    }
    return $PathValue
}

function Ensure-DownloadedPgsSourceSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    $sourceUrl = "https://samples.ffmpeg.org/sub/PGS/supsample.mkv"
    Write-Host ("Downloading temporary PGS subtitle source sample: " + $PathValue)
    Invoke-WebRequest -Uri $sourceUrl -OutFile $PathValue
    if (-not (Test-Path $PathValue)) {
        throw "Failed to download PGS subtitle source sample: $PathValue"
    }
    return $PathValue
}

function Ensure-EmbeddedPgsSample {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PathValue,
        [Parameter(Mandatory = $true)]
        [string]$BaseMediaPath,
        [Parameter(Mandatory = $true)]
        [string]$PgsSourcePath
    )

    if (Test-Path $PathValue) {
        return $PathValue
    }

    $ffmpegCommand = Get-Command ffmpeg.exe -ErrorAction SilentlyContinue
    if (-not $ffmpegCommand) {
        throw "ffmpeg.exe not found in PATH, and no embedded PGS subtitle sample was provided."
    }

    $parent = Split-Path -Parent $PathValue
    if (-not [string]::IsNullOrWhiteSpace($parent)) {
        New-Item -ItemType Directory -Force -Path $parent | Out-Null
    }

    Write-Host ("Generating temporary embedded PGS subtitle sample: " + $PathValue)
    $result = Invoke-ExternalCommand -Executable $ffmpegCommand.Source -Arguments @(
        "-y",
        "-i", $BaseMediaPath,
        "-i", $PgsSourcePath,
        "-map", "0:v",
        "-map", "0:a?",
        "-map", "1:s:0",
        "-c:v", "copy",
        "-c:a", "copy",
        "-c:s", "copy",
        $PathValue
    )
    if ($result.ExitCode -ne 0 -or -not (Test-Path $PathValue)) {
        throw "Failed to generate embedded PGS subtitle validation sample: $PathValue`n$($result.Output)"
    }
    return $PathValue
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
$cubeLutPath = Resolve-ProjectPath -Root $repoRoot -PathValue "samples/lut/identity_2.cube"
if (-not (Test-Path $cubeLutPath)) {
    throw "Cube LUT file not found: $cubeLutPath"
}

$tenBitPath = $TenBitFile
if ([string]::IsNullOrWhiteSpace($tenBitPath)) {
    $tenBitPath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/opengl-p010-validation.mp4"
}
$tenBitPath = Ensure-TenBitSample -PathValue $tenBitPath

$subtitleMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $SubtitleMediaFile
$subtitlePath = Resolve-ProjectPath -Root $repoRoot -PathValue $SubtitleFile
$transitionSubtitlePath = Resolve-ProjectPath -Root $repoRoot -PathValue $TransitionSubtitleFile
$embeddedSubtitleBaseMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $EmbeddedSubtitleBaseMediaFile
$embeddedAssSubtitlePath = Resolve-ProjectPath -Root $repoRoot -PathValue $EmbeddedAssSubtitleFile
$subtitleStylePaths = @()
foreach ($styleFile in $SubtitleStyleFiles) {
    $stylePath = Resolve-ProjectPath -Root $repoRoot -PathValue $styleFile
    if (Test-Path $stylePath) {
        $subtitleStylePaths += $stylePath
    }
}
$runSubtitleCheck = (Test-Path $subtitleMediaPath) -and (Test-Path $subtitlePath)
$runTransitionSubtitleCheck = (Test-Path $subtitleMediaPath) -and (Test-Path $transitionSubtitlePath)

$runEmbeddedAssCheck = (Test-Path $embeddedSubtitleBaseMediaPath) -and (Test-Path $embeddedAssSubtitlePath)
$embeddedAssMediaPath = ""
if ($runEmbeddedAssCheck) {
    $embeddedAssMediaPath = $EmbeddedAssMediaFile
    if ([string]::IsNullOrWhiteSpace($embeddedAssMediaPath)) {
        $embeddedAssMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/embedded-ass-validation.mkv"
    } else {
        $embeddedAssMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $embeddedAssMediaPath
    }
    $embeddedAssMediaPath = Ensure-EmbeddedAssSample -PathValue $embeddedAssMediaPath -BaseMediaPath $embeddedSubtitleBaseMediaPath -SubtitlePath $embeddedAssSubtitlePath
}

$runEmbeddedTextCheck = (Test-Path $embeddedSubtitleBaseMediaPath)
$embeddedTextMediaPath = ""
if ($runEmbeddedTextCheck) {
    $embeddedTextMediaPath = $EmbeddedTextMediaFile
    if ([string]::IsNullOrWhiteSpace($embeddedTextMediaPath)) {
        $embeddedTextMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/embedded-text-validation.mp4"
    } else {
        $embeddedTextMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $embeddedTextMediaPath
    }
    $embeddedTextMediaPath = Ensure-EmbeddedTextSample -PathValue $embeddedTextMediaPath -BaseMediaPath $embeddedSubtitleBaseMediaPath
}

$runEmbeddedDvdCheck = (Test-Path $embeddedSubtitleBaseMediaPath)
$embeddedDvdMediaPath = ""
if ($runEmbeddedDvdCheck) {
    $embeddedDvdMediaPath = $EmbeddedDvdMediaFile
    if ([string]::IsNullOrWhiteSpace($embeddedDvdMediaPath)) {
        $embeddedDvdMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/embedded-dvd-validation.mkv"
    } else {
        $embeddedDvdMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $embeddedDvdMediaPath
    }
    $embeddedDvdMediaPath = Ensure-EmbeddedDvdSample -PathValue $embeddedDvdMediaPath -BaseMediaPath $embeddedSubtitleBaseMediaPath
}

$runEmbeddedPgsCheck = (Test-Path $embeddedSubtitleBaseMediaPath)
$embeddedPgsSourcePath = ""
$embeddedPgsMediaPath = ""
if ($runEmbeddedPgsCheck) {
    $embeddedPgsSourcePath = $EmbeddedPgsSourceFile
    if ([string]::IsNullOrWhiteSpace($embeddedPgsSourcePath)) {
        $embeddedPgsSourcePath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/embedded-pgs-source.mkv"
    } else {
        $embeddedPgsSourcePath = Resolve-ProjectPath -Root $repoRoot -PathValue $embeddedPgsSourceFile
    }
    $embeddedPgsSourcePath = Ensure-DownloadedPgsSourceSample -PathValue $embeddedPgsSourcePath

    $embeddedPgsMediaPath = $EmbeddedPgsMediaFile
    if ([string]::IsNullOrWhiteSpace($embeddedPgsMediaPath)) {
        $embeddedPgsMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue "build/tmp/embedded-pgs-validation.mkv"
    } else {
        $embeddedPgsMediaPath = Resolve-ProjectPath -Root $repoRoot -PathValue $embeddedPgsMediaPath
    }
    $embeddedPgsMediaPath = Ensure-EmbeddedPgsSample `
        -PathValue $embeddedPgsMediaPath `
        -BaseMediaPath $embeddedSubtitleBaseMediaPath `
        -PgsSourcePath $embeddedPgsSourcePath
}

$checks = @(
    @{
        Name = "OpenGL diagnostics"
        Args = @("--opengl-diagnostics")
        Env = @{}
        Required = @(
            "opengl-diagnostics\.hdr_output\.probe_succeeded=true",
            "opengl-diagnostics\.result=PASS"
        )
        Summary = @("opengl-diagnostics\.", "result=")
    },
    @{
        Name = "DirectWrite subtitle custom font collection"
        Args = @("--directwrite-font-collection-check", $probePath)
        Env = @{}
        Required = @(
            "directwrite-font-collection-check\.factory_ok=true",
            "directwrite-font-collection-check\.result=PASS"
        )
        Summary = @("directwrite-font-collection-check\.", "result=")
    },
    @{
        Name = "OpenGL native playback"
        Args = @("--performance-log-check", $probePath, [string]$SampleMs)
        Env = @{ MVP_RENDERER_BACKEND = "opengl" }
        Required = @(
            "performance-log-check\.renderer_backend=OpenGL",
            "performance-log-check\.result=PASS"
        )
        Summary = @("renderer_backend=", "decoder_backend=", "renderer_opengl_present_mode_", "result=")
    },
    @{
        Name = "OpenGL output color manual cube regression"
        Args = @("--opengl-output-color-check", $probePath, $cubeLutPath, [string]$SampleMs)
        Env = @{}
        Required = @(
            "opengl-output-color-check\.output_lut_source=cube",
            "opengl-output-color-check\.output_lut_active=true",
            "opengl-output-color-check\.result=PASS"
        )
        Summary = @("opengl-output-color-check\.", "result=")
    },
    @{
        Name = "OpenGL output color auto ICC regression"
        Args = @("--opengl-output-color-icc-check", $probePath, [string]$SampleMs)
        Env = @{}
        Required = @(
            "opengl-output-color-icc-check\.output_lut_source=icc-",
            "opengl-output-color-icc-check\.output_icc_profile_available=true",
            "opengl-output-color-icc-check\.result=PASS"
        )
        Summary = @("opengl-output-color-icc-check\.", "result=")
    },
    @{
        Name = "OpenGL copy-back playback"
        Args = @("--performance-log-check", $probePath, [string]$SampleMs)
        Env = @{ MVP_RENDERER_BACKEND = "opengl"; MVP_OPENGL_NATIVE_INTEROP = "disable" }
        Required = @(
            "performance-log-check\.video_native_output_frames=0",
            "performance-log-check\.renderer_opengl_native_interop_active=false",
            "performance-log-check\.result=PASS"
        )
        Summary = @("video_native_output_frames=", "video_copy_back_frames=", "renderer_opengl_native_interop_", "result=")
    },
    @{
        Name = "OpenGL immediate present mode"
        Args = @("--performance-log-check", $probePath, [string]$SampleMs)
        Env = @{ MVP_RENDERER_BACKEND = "opengl"; MVP_OPENGL_PRESENT_MODE = "immediate" }
        Required = @(
            "performance-log-check\.renderer_opengl_present_mode_requested=immediate",
            "performance-log-check\.renderer_opengl_present_mode_active=immediate",
            "performance-log-check\.result=PASS"
        )
        Summary = @("\[diag:opengl-present\]", "renderer_opengl_present_mode_", "result=")
    },
    @{
        Name = "OpenGL interaction freeze stress"
        Args = @("--interaction-freeze-check", $probePath, [string]$SampleMs)
        Env = @{ MVP_RENDERER_BACKEND = "opengl" }
        Required = @(
            "interaction-freeze-check\.renderer_backend=OpenGL",
            "interaction-freeze-check\.injected_events_total=[1-9]",
            "interaction-freeze-check\.result=PASS"
        )
        Summary = @("interaction-freeze-check\.", "result=")
    },
    @{
        Name = "OpenGL 10-bit copy-back playback"
        Args = @("--performance-log-check", $tenBitPath, [string]$TenBitSampleMs)
        Env = @{ MVP_RENDERER_BACKEND = "opengl"; MVP_OPENGL_NATIVE_INTEROP = "disable" }
        Required = @(
            "performance-log-check\.decoder_backend=D3D11VA",
            "performance-log-check\.video_copy_back_frames=[1-9]",
            "performance-log-check\.video_swscale_frames=0",
            "performance-log-check\.result=PASS"
        )
        Summary = @("decoder_backend=", "video_copy_back_frames=", "video_swscale_frames=", "result=")
    }
)

if ($runEmbeddedAssCheck) {
    $checks += @(
        @{
            Name = "Embedded ASS subtitle CLI regression"
            Args = @("--embedded-subtitle-check", $embeddedAssMediaPath)
            Env = @{}
            Required = @(
                "embedded-subtitle-check\.loaded=true",
                "embedded-subtitle-check\.codec_name=ass",
                "embedded-subtitle-check\.item_count=[1-9]",
                "embedded-subtitle-check\.result=PASS"
            )
            Summary = @("embedded-subtitle-check\.", "result=")
        },
        @{
            Name = "OpenGL embedded ASS subtitle playback regression"
            Args = @("--performance-log-check", $embeddedAssMediaPath, [string]$SampleMs)
            Env = @{ MVP_RENDERER_BACKEND = "opengl" }
            Required = @(
                "Embedded subtitle probe: .*loaded=true",
                "Embedded subtitle track attached: path=embedded:",
                "performance-log-check\.renderer_backend=OpenGL",
                "performance-log-check\.result=PASS"
            )
            Summary = @("Embedded subtitle", "renderer_backend=", "decoder_backend=", "result=")
        }
    )
} else {
    Write-Output "Skipping embedded ASS subtitle regression because base media or ASS sample was not found."
}

if ($runEmbeddedTextCheck) {
    $checks += @(
        @{
            Name = "Embedded text subtitle CLI regression"
            Args = @("--embedded-subtitle-check", $embeddedTextMediaPath)
            Env = @{}
            Required = @(
                "embedded-subtitle-check\.loaded=true",
                "embedded-subtitle-check\.codec_name=mov_text",
                "embedded-subtitle-check\.item_count=[1-9]",
                "embedded-subtitle-check\.result=PASS"
            )
            Summary = @("embedded-subtitle-check\.", "result=")
        },
        @{
            Name = "OpenGL embedded text subtitle playback regression"
            Args = @("--performance-log-check", $embeddedTextMediaPath, [string]$SampleMs)
            Env = @{ MVP_RENDERER_BACKEND = "opengl" }
            Required = @(
                "Embedded subtitle probe: .*loaded=true",
                "Embedded subtitle track attached: path=embedded:",
                "performance-log-check\.renderer_backend=OpenGL",
                "performance-log-check\.result=PASS"
            )
            Summary = @("Embedded subtitle", "renderer_backend=", "decoder_backend=", "result=")
        }
    )
} else {
    Write-Output "Skipping embedded text subtitle regression because base media was not found."
}

if ($runEmbeddedDvdCheck) {
    $checks += @(
        @{
            Name = "Embedded DVD bitmap subtitle CLI regression"
            Args = @("--bitmap-subtitle-check", $embeddedDvdMediaPath)
            Env = @{}
            Required = @(
                "bitmap-subtitle-check\.loaded=true",
                "bitmap-subtitle-check\.codec_name=dvd_subtitle",
                "bitmap-subtitle-check\.bitmap_item_count=[1-9]",
                "bitmap-subtitle-check\.bitmap_rect_count=[1-9]",
                "bitmap-subtitle-check\.result=PASS"
            )
            Summary = @("bitmap-subtitle-check\.", "result=")
        },
        @{
            Name = "OpenGL embedded DVD bitmap subtitle playback regression"
            Args = @("--performance-log-check", $embeddedDvdMediaPath, [string]$SampleMs)
            Env = @{ MVP_RENDERER_BACKEND = "opengl" }
            Required = @(
                "Embedded subtitle track load: .*codec=dvd_subtitle.*loaded=true",
                "Embedded subtitle track attached: path=embedded:",
                "performance-log-check\.renderer_backend=OpenGL",
                "performance-log-check\.result=PASS"
            )
            Summary = @("Embedded subtitle", "renderer_backend=", "decoder_backend=", "result=")
        }
    )
} else {
    Write-Output "Skipping embedded DVD subtitle regression because base media was not found."
}

if ($runEmbeddedPgsCheck) {
    $checks += @(
        @{
            Name = "Embedded PGS bitmap subtitle CLI regression"
            Args = @("--bitmap-subtitle-check", $embeddedPgsMediaPath)
            Env = @{}
            Required = @(
                "bitmap-subtitle-check\.loaded=true",
                "bitmap-subtitle-check\.codec_name=hdmv_pgs_subtitle",
                "bitmap-subtitle-check\.bitmap_item_count=[1-9]",
                "bitmap-subtitle-check\.bitmap_rect_count=[1-9]",
                "bitmap-subtitle-check\.result=PASS"
            )
            Summary = @("bitmap-subtitle-check\.", "result=")
        },
        @{
            Name = "OpenGL embedded PGS bitmap subtitle playback regression"
            Args = @("--performance-log-check", $embeddedPgsMediaPath, [string]$SampleMs)
            Env = @{ MVP_RENDERER_BACKEND = "opengl" }
            Required = @(
                "Embedded subtitle track load: .*codec=hdmv_pgs_subtitle.*loaded=true",
                "Embedded subtitle track attached: path=embedded:",
                "performance-log-check\.renderer_backend=OpenGL",
                "performance-log-check\.result=PASS"
            )
            Summary = @("Embedded subtitle", "renderer_backend=", "decoder_backend=", "result=")
        }
    )
} else {
    Write-Output "Skipping embedded PGS subtitle regression because base media was not found."
}

$checks += @{
    Name = "Bitmap subtitle multi-rect stress regression"
    Args = @("--bitmap-subtitle-stress-check")
    Env = @{}
    Required = @(
        "bitmap-subtitle-stress-check\.multi_rect_item_count=[1-9]",
        "bitmap-subtitle-stress-check\.cache_reuse_candidate_count=[1-9]",
        "bitmap-subtitle-stress-check\.result=PASS"
    )
    Summary = @("bitmap-subtitle-stress-check\.", "result=")
}

if ($runSubtitleCheck) {
    $checks += @{
        Name = "OpenGL subtitle delay regression"
        Args = @("--delay-adjust-check", $subtitleMediaPath, $subtitlePath)
        Env = @{ MVP_RENDERER_BACKEND = "opengl" }
        Required = @("delay-adjust-check\.result=PASS")
        Summary = @("delay-adjust-check\.", "result=")
    }
} else {
    Write-Output "Skipping subtitle delay regression because subtitle media/sample was not found."
}

if ($runTransitionSubtitleCheck) {
    $checks += @{
        Name = "OpenGL subtitle transform transition regression"
        Args = @("--delay-adjust-check", $subtitleMediaPath, $transitionSubtitlePath)
        Env = @{ MVP_RENDERER_BACKEND = "opengl" }
        Required = @("delay-adjust-check\.result=PASS")
        Summary = @("delay-adjust-check\.", "result=")
    }
} else {
    Write-Output "Skipping subtitle transform transition regression because subtitle media/sample was not found."
}

foreach ($subtitleStylePath in $subtitleStylePaths) {
    $checks += @{
        Name = ("OpenGL subtitle style regression: " + [System.IO.Path]::GetFileName($subtitleStylePath))
        Args = @("--subtitle-style-check", $subtitleStylePath)
        Env = @{}
        Required = @("subtitle-style-check\.result=PASS")
        Summary = @("subtitle-style-check\.", "result=")
    }
}

$failed = $false
$index = 0
foreach ($check in $checks) {
    $index += 1
    Write-Output ("[" + $index + "/" + $checks.Count + "] " + $check.Name)
    $ok = Invoke-PlayerCheck `
        -Name $check.Name `
        -Executable $exePath `
        -Arguments $check.Args `
        -EnvOverrides $check.Env `
        -RequiredPatterns $check.Required `
        -SummaryPatterns $check.Summary
    if (-not $ok) {
        $failed = $true
        break
    }
}

Write-Output ""
Write-Output ("OpenGL gate result: " + ($(if ($failed) { "FAIL" } else { "PASS" })))
exit $(if ($failed) { 2 } else { 0 })
