param(
    [string]$BuildDir = "build",
    [string]$Configuration = "Release",
    [string]$Generator = "ZIP",
    [switch]$SkipBuild,
    [string]$CmakePath = "",
    [string]$CpackPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$buildPath = Join-Path $repoRoot $BuildDir
if (-not (Test-Path $buildPath)) {
    throw "Build directory not found: $buildPath"
}

function Resolve-ToolPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ToolName,
        [string]$PreferredPath = ""
    )

    if (-not [string]::IsNullOrWhiteSpace($PreferredPath)) {
        if (-not (Test-Path $PreferredPath)) {
            throw "$ToolName not found at preferred path: $PreferredPath"
        }
        return (Resolve-Path $PreferredPath).Path
    }

    $command = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    $vsCmakeBin = "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"
    $fallbackPath = Join-Path $vsCmakeBin ($ToolName + ".exe")
    if (Test-Path $fallbackPath) {
        return $fallbackPath
    }

    throw "$ToolName was not found in PATH or Visual Studio CMake tools."
}

$cmake = Resolve-ToolPath -ToolName "cmake" -PreferredPath $CmakePath
$cpack = Resolve-ToolPath -ToolName "cpack" -PreferredPath $CpackPath

if (-not $SkipBuild) {
    & $cmake --build $buildPath --config $Configuration --target modern-video-player
    if ($LASTEXITCODE -ne 0) {
        throw "cmake --build failed with exit code $LASTEXITCODE"
    }
}

Push-Location $buildPath
try {
    & $cpack -C $Configuration -G $Generator
    if ($LASTEXITCODE -ne 0) {
        throw "cpack failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

$packageFiles = Get-ChildItem -Path $buildPath -File | Where-Object { $_.Extension -ieq ".zip" }
if (-not $packageFiles) {
    throw "No Windows package artifacts were generated in $buildPath"
}

Write-Output ("windows-package.build_dir=" + $buildPath)
foreach ($packageFile in ($packageFiles | Sort-Object LastWriteTime)) {
    Write-Output ("windows-package.file=" + $packageFile.FullName)
}
