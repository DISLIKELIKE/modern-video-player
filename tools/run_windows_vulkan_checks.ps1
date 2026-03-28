param(
    [string]$ExecutablePath = "build/Release/modern-video-player.exe",
    [string]$ProbeFile = "samples/mp4/demo__h264_aac__1920x1080__60fps__2ch.mp4",
    [int]$SampleMs = 1200,
    [switch]$RequireVulkanAvailable,
    [string]$SummaryOutputPath = ""
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

function Invoke-PlayerCommand {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Executable,
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,
        [hashtable]$EnvOverrides = @{}
    )

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

        $stdoutText = if (Test-Path $stdoutFile) { [string](Get-Content -Path $stdoutFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        $stderrText = if (Test-Path $stderrFile) { [string](Get-Content -Path $stderrFile -Raw -ErrorAction SilentlyContinue) } else { "" }
        return @{
            ExitCode = $process.ExitCode
            Output = ($stdoutText + [Environment]::NewLine + $stderrText).Trim()
        }
    } finally {
        Restore-Environment -Backup $backup -Values $EnvOverrides
        Remove-Item -Path $stdoutFile -ErrorAction SilentlyContinue
        Remove-Item -Path $stderrFile -ErrorAction SilentlyContinue
    }
}

function Parse-KeyValueOutput {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Output
    )

    $result = @{}
    $lines = $Output -split "`r?`n"
    foreach ($line in $lines) {
        if ([string]::IsNullOrWhiteSpace($line)) {
            continue
        }
        $index = $line.IndexOf("=")
        if ($index -le 0) {
            continue
        }
        $key = $line.Substring(0, $index).Trim()
        $value = $line.Substring($index + 1).Trim()
        if ([string]::IsNullOrWhiteSpace($key)) {
            continue
        }
        $result[$key] = $value
    }
    return $result
}

function Value-OrDefault {
    param(
        [hashtable]$Map,
        [string]$Key,
        [string]$DefaultValue = ""
    )

    if ($Map.ContainsKey($Key)) {
        return [string]$Map[$Key]
    }
    return $DefaultValue
}

function Is-TrueValue {
    param([string]$Value)
    return [string]::Equals($Value, "true", [System.StringComparison]::OrdinalIgnoreCase)
}

function Is-TruthyFlagValue {
    param([string]$Value)
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return $false
    }
    $normalized = $Value.Trim().ToLowerInvariant()
    return $normalized -eq "1" -or $normalized -eq "true" -or $normalized -eq "yes" -or $normalized -eq "on"
}

function Normalize-PolicyValue {
    param([string]$Value)
    if ([string]::IsNullOrWhiteSpace($Value)) {
        return "off"
    }
    return $Value.Trim().ToLowerInvariant()
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

$diag = Invoke-PlayerCommand -Executable $exePath -Arguments @("--vulkan-diagnostics")
$diagMap = Parse-KeyValueOutput -Output $diag.Output
$diagRequiredKeys = @(
    "vulkan-diagnostics.platform",
    "vulkan-diagnostics.supported_platform",
    "vulkan-diagnostics.compiled_in",
    "vulkan-diagnostics.runtime_available",
    "vulkan-diagnostics.result"
)
$diagMissingRequiredKeys = @()
foreach ($diagRequiredKey in $diagRequiredKeys) {
    if (-not $diagMap.ContainsKey($diagRequiredKey)) {
        $diagMissingRequiredKeys += $diagRequiredKey
    }
}
$diagContractValid = ($diagMissingRequiredKeys.Count -eq 0)
$diagMissingRequiredFields = if ($diagContractValid) { "none" } else { [string]::Join(",", $diagMissingRequiredKeys) }

$vulkanSdkPath = [Environment]::GetEnvironmentVariable("VULKAN_SDK")
$vulkanSdkPresent = -not [string]::IsNullOrWhiteSpace($vulkanSdkPath)
$runnerSdkAvailable = Is-TruthyFlagValue([Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_SDK_AVAILABLE"))
$runnerRuntimeProbeAvailable = Is-TruthyFlagValue([Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_RUNTIME_PROBE_AVAILABLE"))
$runnerRuntimeProbeDetail = [Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_RUNTIME_PROBE_DETAIL")
if ([string]::IsNullOrWhiteSpace($runnerRuntimeProbeDetail)) {
    $runnerRuntimeProbeDetail = "unknown"
}
$swiftShaderAvailable = Is-TruthyFlagValue([Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_SWIFTSHADER_AVAILABLE"))
$swiftShaderIcd = [Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_SWIFTSHADER_ICD")
if ([string]::IsNullOrWhiteSpace($swiftShaderIcd)) {
    $swiftShaderIcd = ""
}
$swiftShaderProbeAvailable = Is-TruthyFlagValue([Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_AVAILABLE"))
$swiftShaderProbeDetail = [Environment]::GetEnvironmentVariable("MVP_WINDOWS_VULKAN_SWIFTSHADER_RUNTIME_PROBE_DETAIL")
if ([string]::IsNullOrWhiteSpace($swiftShaderProbeDetail)) {
    $swiftShaderProbeDetail = "unknown"
}
$vkIcdFilenames = [Environment]::GetEnvironmentVariable("VK_ICD_FILENAMES")
if ([string]::IsNullOrWhiteSpace($vkIcdFilenames)) {
    $vkIcdFilenames = ""
}
$platform = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.platform" -DefaultValue "unknown"
$supportedPlatform = Is-TrueValue (Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.supported_platform" -DefaultValue "false")
$compiledIn = Is-TrueValue (Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.compiled_in" -DefaultValue "false")
$runtimeAvailable = Is-TrueValue (Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.runtime_available" -DefaultValue "false")
$diagResult = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.result" -DefaultValue "FAIL"
$diagSelectedRenderer = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.selected_renderer" -DefaultValue "None"
$diagFallbackTarget = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.fallback_target" -DefaultValue "none"
$diagCandidates = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.startup_renderer_candidates" -DefaultValue ""
$diagDependencySource = Value-OrDefault -Map $diagMap -Key "vulkan-diagnostics.dependency_source" -DefaultValue "unknown"

$strictPolicyRaw = [Environment]::GetEnvironmentVariable("MVP_REQUIRE_WINDOWS_VULKAN_CHECKS")
$strictPolicy = Normalize-PolicyValue($strictPolicyRaw)
$strictFromEnv = $false
$strictAutoPrerequisitesMet = $false
if ($strictPolicy -eq "auto") {
    $strictAutoPrerequisitesMet = $runnerSdkAvailable -and $runnerRuntimeProbeAvailable
    $strictFromEnv = $strictAutoPrerequisitesMet
} elseif ($strictPolicy -eq "1" -or $strictPolicy -eq "true" -or $strictPolicy -eq "yes" -or $strictPolicy -eq "on") {
    $strictFromEnv = $true
}
$strictModeEnabled = $RequireVulkanAvailable.IsPresent -or $strictFromEnv
$strictMode = if ($strictModeEnabled) { "strict" } else { "optional" }
$result = "SKIPPED"
$skipReason = ""
$failureReason = ""
$availabilityProbePassed = $false
$availabilityFailureDetail = "unknown"
$playbackCheckExecuted = $false
$playbackResult = "SKIPPED"
$playbackSelectedRenderer = "None"
$playbackRendererBackend = "None"
$playbackContractValid = "n/a"
$playbackMissingRequiredFields = "n/a"
$playbackFailureDetail = "not-executed"

if (-not $diagContractValid) {
    $result = "FAIL"
    $failureReason = "vulkan-diagnostics-contract-broken"
    $availabilityFailureDetail = "diag-contract-missing-required-fields"
} elseif (-not $supportedPlatform) {
    $result = "FAIL"
    $failureReason = "unsupported-platform"
    $availabilityFailureDetail = "unsupported-platform"
} else {
    $availabilityProbePassed = ($diag.ExitCode -eq 0) -and $compiledIn -and $runtimeAvailable -and ($diagResult -eq "PASS")
    if ($availabilityProbePassed) {
        $availabilityFailureDetail = "none"
        $playbackCheckExecuted = $true
        $playback = Invoke-PlayerCommand `
            -Executable $exePath `
            -Arguments @("--performance-log-check", $probePath, [string]$SampleMs) `
            -EnvOverrides @{ MVP_RENDERER_BACKEND = "vulkan" }
        $playbackMap = Parse-KeyValueOutput -Output $playback.Output
        $playbackRequiredKeys = @(
            "performance-log-check.result",
            "performance-log-check.startup_selected_renderer",
            "performance-log-check.renderer_backend",
            "performance-log-check.startup_renderer_candidates",
            "performance-log-check.startup_renderer_plan_reason"
        )
        $playbackMissingRequiredKeys = @()
        foreach ($playbackRequiredKey in $playbackRequiredKeys) {
            if (-not $playbackMap.ContainsKey($playbackRequiredKey)) {
                $playbackMissingRequiredKeys += $playbackRequiredKey
            }
        }
        $isPlaybackContractValid = ($playbackMissingRequiredKeys.Count -eq 0)
        $playbackContractValid = if ($isPlaybackContractValid) { "true" } else { "false" }
        $playbackMissingRequiredFields = if ($isPlaybackContractValid) { "none" } else { [string]::Join(",", $playbackMissingRequiredKeys) }

        $playbackResult = Value-OrDefault -Map $playbackMap -Key "performance-log-check.result" -DefaultValue "FAIL"
        $playbackSelectedRenderer = Value-OrDefault -Map $playbackMap -Key "performance-log-check.startup_selected_renderer" -DefaultValue "None"
        $playbackRendererBackend = Value-OrDefault -Map $playbackMap -Key "performance-log-check.renderer_backend" -DefaultValue "None"
        $playbackCandidates = Value-OrDefault -Map $playbackMap -Key "performance-log-check.startup_renderer_candidates" -DefaultValue ""
        $playbackPlanReason = Value-OrDefault -Map $playbackMap -Key "performance-log-check.startup_renderer_plan_reason" -DefaultValue ""

        if (-not $isPlaybackContractValid) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-contract-broken"
            $playbackFailureDetail = "contract-missing-required-fields"
        } elseif ($playback.ExitCode -ne 0) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "command-exit-nonzero"
        } elseif (-not [string]::Equals($playbackResult, "PASS", [System.StringComparison]::OrdinalIgnoreCase)) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "result-not-pass"
        } elseif (-not [string]::Equals($playbackSelectedRenderer, "Vulkan", [System.StringComparison]::OrdinalIgnoreCase)) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "selected-renderer-not-vulkan"
        } elseif (-not [string]::Equals($playbackRendererBackend, "Vulkan", [System.StringComparison]::OrdinalIgnoreCase)) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "renderer-backend-not-vulkan"
        } elseif (-not ($playbackCandidates -match "Vulkan")) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "candidates-missing-vulkan"
        } elseif (-not [string]::Equals($playbackPlanReason, "renderer-override-env", [System.StringComparison]::Ordinal)) {
            $result = "FAIL"
            $failureReason = "vulkan-playback-check-failed"
            $playbackFailureDetail = "plan-reason-not-renderer-override-env"
        } else {
            $result = "PASS"
            $playbackFailureDetail = "none"
        }
    } else {
        if (-not $compiledIn) {
            if ($diagDependencySource -eq "disabled") {
                $availabilityFailureDetail = "compiled-in-disabled"
            } else {
                $availabilityFailureDetail = "compiled-in-false"
            }
        } elseif (-not $runtimeAvailable) {
            $availabilityFailureDetail = "runtime-unavailable"
        } elseif ($diag.ExitCode -ne 0) {
            $availabilityFailureDetail = "diag-exit-nonzero"
        } elseif (-not [string]::Equals($diagResult, "PASS", [System.StringComparison]::OrdinalIgnoreCase)) {
            $availabilityFailureDetail = "diag-result-not-pass"
        } else {
            $availabilityFailureDetail = "unknown"
        }

        if ($strictModeEnabled) {
            $result = "FAIL"
            $failureReason = "vulkan-not-available-in-strict-mode"
        } else {
            $result = "SKIPPED"
            $skipReason = "vulkan-not-available"
        }
    }
}

$summary = [ordered]@{
    "windows-vulkan-check.platform" = $platform
    "windows-vulkan-check.vulkan_sdk_present" = if ($vulkanSdkPresent) { "true" } else { "false" }
    "windows-vulkan-check.vulkan_sdk_path" = if ($vulkanSdkPresent) { $vulkanSdkPath } else { "" }
    "windows-vulkan-check.runner_vulkan_sdk_available" = if ($runnerSdkAvailable) { "true" } else { "false" }
    "windows-vulkan-check.runner_vulkan_runtime_probe_available" = if ($runnerRuntimeProbeAvailable) { "true" } else { "false" }
    "windows-vulkan-check.runner_vulkan_runtime_probe_detail" = $runnerRuntimeProbeDetail
    "windows-vulkan-check.runner_vulkan_swiftshader_available" = if ($swiftShaderAvailable) { "true" } else { "false" }
    "windows-vulkan-check.runner_vulkan_swiftshader_icd" = $swiftShaderIcd
    "windows-vulkan-check.runner_vulkan_swiftshader_runtime_probe_available" = if ($swiftShaderProbeAvailable) { "true" } else { "false" }
    "windows-vulkan-check.runner_vulkan_swiftshader_runtime_probe_detail" = $swiftShaderProbeDetail
    "windows-vulkan-check.vk_icd_filenames" = $vkIcdFilenames
    "windows-vulkan-check.strict_mode_policy" = $strictPolicy
    "windows-vulkan-check.strict_mode_cli_requested" = if ($RequireVulkanAvailable.IsPresent) { "true" } else { "false" }
    "windows-vulkan-check.strict_mode_auto_basis" = "sdk_and_runtime_probe"
    "windows-vulkan-check.strict_mode_auto_prerequisites_met" = if ($strictPolicy -eq "auto") { if ($strictAutoPrerequisitesMet) { "true" } else { "false" } } else { "n/a" }
    "windows-vulkan-check.mode" = $strictMode
    "windows-vulkan-check.strict_mode_effective" = if ($strictModeEnabled) { "true" } else { "false" }
    "windows-vulkan-check.strict_mode_env_requested" = if ($strictFromEnv) { "true" } else { "false" }
    "windows-vulkan-check.supported_platform" = if ($supportedPlatform) { "true" } else { "false" }
    "windows-vulkan-check.compiled_in" = if ($compiledIn) { "true" } else { "false" }
    "windows-vulkan-check.runtime_available" = if ($runtimeAvailable) { "true" } else { "false" }
    "windows-vulkan-check.vulkan_availability_probe_passed" = if ($availabilityProbePassed) { "true" } else { "false" }
    "windows-vulkan-check.vulkan_availability_failure_detail" = $availabilityFailureDetail
    "windows-vulkan-check.diag_exit_code" = [string]$diag.ExitCode
    "windows-vulkan-check.diag_contract_valid" = if ($diagContractValid) { "true" } else { "false" }
    "windows-vulkan-check.diag_missing_required_fields" = $diagMissingRequiredFields
    "windows-vulkan-check.diag_result" = $diagResult
    "windows-vulkan-check.diag_selected_renderer" = $diagSelectedRenderer
    "windows-vulkan-check.diag_fallback_target" = $diagFallbackTarget
    "windows-vulkan-check.diag_candidates" = $diagCandidates
    "windows-vulkan-check.diag_dependency_source" = $diagDependencySource
    "windows-vulkan-check.playback_check_executed" = if ($playbackCheckExecuted) { "true" } else { "false" }
    "windows-vulkan-check.playback_contract_valid" = $playbackContractValid
    "windows-vulkan-check.playback_missing_required_fields" = $playbackMissingRequiredFields
    "windows-vulkan-check.playback_failure_detail" = $playbackFailureDetail
    "windows-vulkan-check.playback_result" = $playbackResult
    "windows-vulkan-check.playback_selected_renderer" = $playbackSelectedRenderer
    "windows-vulkan-check.playback_renderer_backend" = $playbackRendererBackend
    "windows-vulkan-check.skip_reason" = $skipReason
    "windows-vulkan-check.failure_reason" = $failureReason
    "windows-vulkan-check.result" = $result
}

$summaryLines = @()
foreach ($entry in $summary.GetEnumerator()) {
    $line = [string]::Format("{0}={1}", $entry.Key, $entry.Value)
    Write-Output $line
    $summaryLines += $line
}

if (-not [string]::IsNullOrWhiteSpace($SummaryOutputPath)) {
    $summaryPath = Resolve-ProjectPath -Root $repoRoot -PathValue $SummaryOutputPath
    $summaryDirectory = Split-Path -Parent $summaryPath
    if (-not [string]::IsNullOrWhiteSpace($summaryDirectory)) {
        New-Item -ItemType Directory -Force -Path $summaryDirectory | Out-Null
    }
    Set-Content -Path $summaryPath -Value ($summaryLines -join [Environment]::NewLine)
}

if ($result -eq "FAIL") {
    exit 2
}
exit 0
