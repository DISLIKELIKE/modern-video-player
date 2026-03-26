param(
    [string]$ExecutablePath = "build/Release/modern-video-player.exe",
    [string]$OutputCsvPath = "docs/reference/DRIVER_QUIRK_SAMPLE_LIBRARY.csv",
    [string]$HostLabel = $env:COMPUTERNAME,
    [string]$Notes = ""
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

function Invoke-PlayerCommand {
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

        $stdoutText = if (Test-Path $stdoutFile) {
            [string](Get-Content -Path $stdoutFile -Raw -ErrorAction SilentlyContinue)
        } else {
            ""
        }
        $stderrText = if (Test-Path $stderrFile) {
            [string](Get-Content -Path $stderrFile -Raw -ErrorAction SilentlyContinue)
        } else {
            ""
        }
        return @{
            ExitCode = $process.ExitCode
            Output = ($stdoutText + [Environment]::NewLine + $stderrText).Trim()
        }
    }
    finally {
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
    }
}

function Convert-KeyValueOutputToMap {
    param([string]$Text)

    $map = @{}
    foreach ($line in ($Text -split "`r?`n")) {
        if ($line -match '^[A-Za-z0-9_.-]+=') {
            $parts = $line -split '=', 2
            if ($parts.Count -eq 2) {
                $map[$parts[0]] = $parts[1]
            }
        }
    }
    return $map
}

function New-DriverSampleRecord {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Backend,
        [Parameter(Mandatory = $true)]
        [hashtable]$Values,
        [Parameter(Mandatory = $true)]
        [string]$CapturedAtUtc,
        [Parameter(Mandatory = $true)]
        [string]$Platform,
        [Parameter(Mandatory = $true)]
        [string]$SampleHost,
        [Parameter(Mandatory = $true)]
        [string]$NoteText
    )

    $sampleId = ($SampleHost + "-" + $Backend + "-" + $CapturedAtUtc.Replace(":", "").Replace("-", ""))
    $nativePrefix = if ($Backend -eq "OpenGL") {
        "opengl-diagnostics.native_interop"
    } else {
        "d3d11-diagnostics.native_direct"
    }
    $adapterName = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.adapter_name"]
    } else {
        $Values["d3d11-diagnostics.adapter_name"]
    }
    $vendorId = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.vendor_id"]
    } else {
        $Values["d3d11-diagnostics.vendor_id"]
    }
    $deviceId = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.device_id"]
    } else {
        $Values["d3d11-diagnostics.device_id"]
    }
    $driverVersion = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.driver_version"]
    } else {
        $Values["d3d11-diagnostics.driver_version"]
    }
    $featureLevel = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.feature_level"]
    } else {
        $Values["d3d11-diagnostics.feature_level"]
    }
    $softwareAdapter = if ($Backend -eq "OpenGL") {
        $Values["opengl-diagnostics.d3d11.software_adapter"]
    } else {
        $Values["d3d11-diagnostics.software_adapter"]
    }
    $hdrPrefix = if ($Backend -eq "OpenGL") {
        "opengl-diagnostics.hdr_output"
    } else {
        "d3d11-diagnostics.hdr_output"
    }

    [PSCustomObject][ordered]@{
        sample_id = $sampleId
        captured_at_utc = $CapturedAtUtc
        host_label = $SampleHost
        platform = $Platform
        backend = $Backend
        probe_succeeded = if ($Backend -eq "OpenGL") {
            $Values["opengl-diagnostics.probe_succeeded"]
        } else {
            $Values["d3d11-diagnostics.probe_succeeded"]
        }
        adapter_name = $adapterName
        vendor_id = $vendorId
        device_id = $deviceId
        subsystem_id = if ($Backend -eq "D3D11") { $Values["d3d11-diagnostics.subsystem_id"] } else { "" }
        driver_version = $driverVersion
        software_adapter = $softwareAdapter
        feature_level = $featureLevel
        gl_vendor = if ($Backend -eq "OpenGL") { $Values["opengl-diagnostics.gl_vendor"] } else { "" }
        gl_renderer = if ($Backend -eq "OpenGL") { $Values["opengl-diagnostics.gl_renderer"] } else { "" }
        gl_version = if ($Backend -eq "OpenGL") { $Values["opengl-diagnostics.gl_version"] } else { "" }
        native_path_allowed = $Values["$nativePrefix.allowed"]
        startup_disabled = $Values["$nativePrefix.startup_disabled"]
        disable_rule = $Values["$nativePrefix.disable_rule"]
        hard_blocker_rule = if ($Backend -eq "OpenGL") {
            $Values["opengl-diagnostics.native_interop.hard_blocker_rule"]
        } else {
            ""
        }
        quirk_rule = if ($Backend -eq "OpenGL") {
            $Values["opengl-diagnostics.native_interop.quirk_rule_name"]
        } else {
            ""
        }
        hdr_probe_succeeded = $Values["$hdrPrefix.probe_succeeded"]
        hdr_output_found = $Values["$hdrPrefix.output_found"]
        hdr_color_space = $Values["$hdrPrefix.color_space"]
        hdr_advanced_color_active = $Values["$hdrPrefix.advanced_color_active"]
        hdr_active = $Values["$hdrPrefix.hdr_active"]
        notes = $NoteText
    }
}

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$exePath = Resolve-ProjectPath -Root $repoRoot -PathValue $ExecutablePath
if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

$outputPath = Resolve-ProjectPath -Root $repoRoot -PathValue $OutputCsvPath
$outputDir = Split-Path -Parent $outputPath
if (-not [string]::IsNullOrWhiteSpace($outputDir)) {
    New-Item -ItemType Directory -Force -Path $outputDir | Out-Null
}

$platform = if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Windows)) {
    "Windows"
} elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::Linux)) {
    "Linux"
} elseif ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform(
        [System.Runtime.InteropServices.OSPlatform]::OSX)) {
    "macOS"
} else {
    "Unknown"
}
$capturedAtUtc = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
$records = @()

$openglResult = Invoke-PlayerCommand -Executable $exePath -Arguments @("--opengl-diagnostics")
$openglValues = Convert-KeyValueOutputToMap -Text $openglResult.Output
if ($openglValues["opengl-diagnostics.supported_platform"] -ne "false") {
    if ($openglResult.ExitCode -ne 0) {
        throw "OpenGL diagnostics failed while collecting driver sample.`n$($openglResult.Output)"
    }
    $records += New-DriverSampleRecord `
        -Backend "OpenGL" `
        -Values $openglValues `
        -CapturedAtUtc $capturedAtUtc `
        -Platform $platform `
        -SampleHost $HostLabel `
        -NoteText $Notes
}

$d3d11Result = Invoke-PlayerCommand -Executable $exePath -Arguments @("--d3d11-diagnostics")
$d3d11Values = Convert-KeyValueOutputToMap -Text $d3d11Result.Output
if ($d3d11Values["d3d11-diagnostics.supported_platform"] -ne "false") {
    if ($d3d11Result.ExitCode -ne 0) {
        throw "D3D11 diagnostics failed while collecting driver sample.`n$($d3d11Result.Output)"
    }
    $records += New-DriverSampleRecord `
        -Backend "D3D11" `
        -Values $d3d11Values `
        -CapturedAtUtc $capturedAtUtc `
        -Platform $platform `
        -SampleHost $HostLabel `
        -NoteText $Notes
}

if ($records.Count -eq 0) {
    throw "No supported diagnostics backends were available for sample collection."
}

$existing = @()
if (Test-Path $outputPath) {
    $existing = @(Import-Csv -Path $outputPath)
}

@($existing + $records) | Export-Csv -Path $outputPath -NoTypeInformation -Encoding utf8
Write-Output ("driver-quirk-sample.output=" + $outputPath)
Write-Output ("driver-quirk-sample.record_count=" + $records.Count)
foreach ($record in $records) {
    Write-Output ("driver-quirk-sample.record=" + $record.backend + ":" + $record.adapter_name + ":" + $record.driver_version)
}
