param(
    [string]$Prerelease = "rc2",
    [string]$WindowsBuildDir = "build-vulkan",
    [string]$LinuxBuildDir = "build-linux-rc2",
    [string]$VulkanSdkPath = "",
    [string]$WslDistro = "Ubuntu-24.04",
    [switch]$InstallLinuxDeps,
    [switch]$SkipWindows,
    [switch]$SkipLinux
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Resolve-RepoRoot {
    return (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
}

function Resolve-VulkanSdkPath {
    param([string]$PreferredPath)

    if (-not [string]::IsNullOrWhiteSpace($PreferredPath)) {
        if (-not (Test-Path $PreferredPath)) {
            throw "Specified Vulkan SDK path does not exist: $PreferredPath"
        }
        return (Resolve-Path $PreferredPath).Path
    }

    if (-not [string]::IsNullOrWhiteSpace($env:VULKAN_SDK) -and (Test-Path $env:VULKAN_SDK)) {
        return (Resolve-Path $env:VULKAN_SDK).Path
    }

    foreach ($candidateRoot in @("D:\VulkanSDK", "C:\VulkanSDK")) {
        if (Test-Path $candidateRoot) {
            $latest = Get-ChildItem -Path $candidateRoot -Directory | Sort-Object Name -Descending | Select-Object -First 1
            if ($latest) {
                return $latest.FullName
            }
        }
    }

    return ""
}

function Invoke-ExternalCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,
        [string]$WorkingDirectory = ""
    )

    if ([string]::IsNullOrWhiteSpace($WorkingDirectory)) {
        & $FilePath @Arguments
    } else {
        Push-Location $WorkingDirectory
        try {
            & $FilePath @Arguments
        }
        finally {
            Pop-Location
        }
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed (exit=$LASTEXITCODE): $FilePath $($Arguments -join ' ')"
    }
}

function Convert-ToWslPath {
    param([Parameter(Mandatory = $true)][string]$WindowsPath)

    $normalized = $WindowsPath -replace "\\", "/"
    if ($normalized -match "^([A-Za-z]):/(.*)$") {
        $drive = $matches[1].ToLowerInvariant()
        $rest = $matches[2]
        return "/mnt/$drive/$rest"
    }
    return $normalized
}

function Get-LatestFile {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [Parameter(Mandatory = $true)][string]$Filter
    )

    if (-not (Test-Path $Path)) {
        return $null
    }

    return Get-ChildItem -Path $Path -File -Filter $Filter |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
}

function Get-RepoRelativePath {
    param(
        [Parameter(Mandatory = $true)][string]$Root,
        [Parameter(Mandatory = $true)][string]$FullPath
    )

    if ($FullPath.StartsWith($Root, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relative = $FullPath.Substring($Root.Length).TrimStart('\', '/')
        if ([string]::IsNullOrWhiteSpace($relative)) {
            return "."
        }
        return ($relative -replace "\\", "/")
    }
    return $FullPath
}

$repoRoot = Resolve-RepoRoot
$resolvedWindowsBuildDir = Join-Path $repoRoot $WindowsBuildDir
$resolvedLinuxBuildDir = $LinuxBuildDir
$resolvedVulkanSdkPath = Resolve-VulkanSdkPath -PreferredPath $VulkanSdkPath

if (-not $SkipWindows) {
    if ([string]::IsNullOrWhiteSpace($resolvedVulkanSdkPath)) {
        throw "Vulkan SDK was not found. Set -VulkanSdkPath or VULKAN_SDK first."
    }

    $env:VULKAN_SDK = $resolvedVulkanSdkPath
    if ([string]::IsNullOrWhiteSpace($env:CMAKE_PREFIX_PATH)) {
        $env:CMAKE_PREFIX_PATH = $resolvedVulkanSdkPath
    } else {
        $env:CMAKE_PREFIX_PATH = "$resolvedVulkanSdkPath;$($env:CMAKE_PREFIX_PATH)"
    }
    $env:Path = (Join-Path $resolvedVulkanSdkPath "Bin") + ";" + $env:Path

    Invoke-ExternalCommand -FilePath "cmake" -Arguments @(
        "-S", ".",
        "-B", $WindowsBuildDir,
        "-G", "Visual Studio 17 2022",
        "-A", "x64",
        "-DENABLE_VULKAN_RENDERER=ON",
        "-DMVP_VERSION_PRERELEASE=$Prerelease",
        "-DCMAKE_PREFIX_PATH=$resolvedVulkanSdkPath"
    ) -WorkingDirectory $repoRoot

    Invoke-ExternalCommand -FilePath "cmake" -Arguments @(
        "--build", $WindowsBuildDir,
        "--config", "Release",
        "--target", "modern-video-player", "sample_logger_plugin"
    ) -WorkingDirectory $repoRoot

    Invoke-ExternalCommand -FilePath "powershell" -Arguments @(
        "-ExecutionPolicy", "Bypass",
        "-File", ".\tools\package_windows.ps1",
        "-BuildDir", $WindowsBuildDir,
        "-Configuration", "Release",
        "-SkipBuild"
    ) -WorkingDirectory $repoRoot
}

if (-not $SkipLinux) {
    $repoRootWsl = Convert-ToWslPath -WindowsPath $repoRoot
    $linuxSteps = @()

    if ($InstallLinuxDeps) {
        $linuxSteps += @(
            "sudo apt-get update",
            "sudo apt-get install -y build-essential cmake pkg-config libsdl2-dev libavformat-dev libavcodec-dev libavutil-dev libswscale-dev libswresample-dev libass-dev libfontconfig1-dev libfreetype6-dev libgl1-mesa-dev libglu1-mesa-dev libvulkan-dev mesa-vulkan-drivers libva-dev"
        )
    }

    $linuxSteps += @(
        "cd '$repoRootWsl'",
        "chmod +x ./tools/package_linux.sh",
        "./tools/package_linux.sh '$resolvedLinuxBuildDir'"
    )

    $linuxCommand = $linuxSteps -join " && "
    Invoke-ExternalCommand -FilePath "wsl" -Arguments @(
        "-d", $WslDistro,
        "bash", "-lc", $linuxCommand
    ) -WorkingDirectory $repoRoot
}

$windowsZip = $null
$linuxTar = $null
$linuxDeb = $null

if (-not $SkipWindows) {
    $windowsZip = Get-LatestFile -Path $resolvedWindowsBuildDir -Filter "*-$Prerelease-windows-x64.zip"
    if (-not $windowsZip) {
        $windowsZip = Get-LatestFile -Path $resolvedWindowsBuildDir -Filter "*-windows-x64.zip"
    }
}

if (-not $SkipLinux) {
    $linuxTar = Get-LatestFile -Path $repoRoot -Filter "*-$Prerelease-linux-x64.tar.gz"
    if (-not $linuxTar) {
        $linuxTar = Get-LatestFile -Path $repoRoot -Filter "*-linux-x64.tar.gz"
    }
    $linuxDeb = Get-LatestFile -Path $repoRoot -Filter "*_amd64.deb"
}

if (-not $SkipWindows -and -not $windowsZip) {
    throw "Windows package zip not found under $resolvedWindowsBuildDir"
}
if (-not $SkipLinux -and -not $linuxTar) {
    throw "Linux tar.gz package not found under $repoRoot"
}
if (-not $SkipLinux -and -not $linuxDeb) {
    throw "Linux deb package not found under $repoRoot"
}

Write-Output "=== RC Artifacts ==="

if ($windowsZip) {
    $windowsHash = (Get-FileHash -Path $windowsZip.FullName -Algorithm SHA256).Hash
    $windowsRelativePath = Get-RepoRelativePath -Root $repoRoot -FullPath $windowsZip.FullName
    Write-Output ("Windows: {0}" -f $windowsRelativePath)
    Write-Output ("SHA256: {0}" -f $windowsHash)
}

if ($linuxTar) {
    $linuxTarHash = (Get-FileHash -Path $linuxTar.FullName -Algorithm SHA256).Hash
    $linuxTarRelativePath = Get-RepoRelativePath -Root $repoRoot -FullPath $linuxTar.FullName
    Write-Output ("Linux: {0}" -f $linuxTarRelativePath)
    Write-Output ("SHA256: {0}" -f $linuxTarHash)
}

if ($linuxDeb) {
    $linuxDebHash = (Get-FileHash -Path $linuxDeb.FullName -Algorithm SHA256).Hash
    $linuxDebRelativePath = Get-RepoRelativePath -Root $repoRoot -FullPath $linuxDeb.FullName
    Write-Output ("Linux: {0}" -f $linuxDebRelativePath)
    Write-Output ("SHA256: {0}" -f $linuxDebHash)
}
