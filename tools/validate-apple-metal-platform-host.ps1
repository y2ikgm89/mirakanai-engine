#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady,
    [ValidateSet("all", "macos", "ios")]
    [string]$Platform = "all",
    [ValidateRange(0, 1024)]
    [int]$Jobs = 0,
    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")
. (Join-Path $PSScriptRoot "apple-host-helpers.ps1")

$root = Get-RepoRoot

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-HostOsCounterValue {
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "macos"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    return "unknown"
}

function Invoke-ToolCapture {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @(),
        [int]$TimeoutSeconds = 20
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $root
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Arguments)) {
        $startInfo.ArgumentList.Add($argument)
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    try {
        $null = $process.Start()
        $completed = $process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $completed) {
            try {
                $process.Kill($true)
            } catch {
                Write-Verbose "failed to kill timed-out process '$FilePath': $_"
            }
            return [pscustomobject]@{
                ExitCode = -1
                Output = ""
                Error = "timeout"
            }
        }
        return [pscustomobject]@{
            ExitCode = $process.ExitCode
            Output = $process.StandardOutput.ReadToEnd()
            Error = $process.StandardError.ReadToEnd()
        }
    } finally {
        $process.Dispose()
    }
}

function Test-TextCounters {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string[]]$Counters
    )

    foreach ($counter in $Counters) {
        if (-not $Text.Contains($counter)) {
            return $false
        }
    }
    return $true
}

function Invoke-MacOSMetalEvidenceIfRequired {
    param([bool]$ShouldRun)

    if (-not $ShouldRun) {
        return [pscustomobject]@{
            CommandQueueReady = $false
            CommandBufferReady = $false
            RenderPipelineReady = $false
            ComputePipelineReady = $false
            TextureUsageRowsReady = $false
            ResourceSynchronizationReady = $false
            ReadbackReady = $false
        }
    }

    $scriptPath = Join-Path $PSScriptRoot "validate-environment-metal-host-aggregate.ps1"
    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-Jobs",
        [string]$Jobs
    ) -TimeoutSeconds 1200
    $text = [string]::Join("`n", @($result.Output, $result.Error))
    $requiredCountersReady = Test-TextCounters -Text $text -Counters @(
        "environment_metal_host_aggregate_command_queue_ready=1",
        "environment_metal_host_aggregate_command_buffer_ready=1",
        "environment_metal_host_aggregate_render_pipeline_ready=1",
        "environment_metal_host_aggregate_compute_pipeline_ready=1",
        "environment_metal_host_aggregate_synchronization_evidence_ready=1",
        "environment_metal_host_aggregate_render_readback_nonzero=1",
        "environment_metal_host_aggregate_compute_readback_nonzero=1"
    )
    $baseReady = ($result.ExitCode -eq 0) -and $requiredCountersReady

    return [pscustomobject]@{
        CommandQueueReady = $baseReady
        CommandBufferReady = $baseReady
        RenderPipelineReady = $baseReady
        ComputePipelineReady = $baseReady
        TextureUsageRowsReady = $baseReady
        ResourceSynchronizationReady = $baseReady
        ReadbackReady = $baseReady
    }
}

function Invoke-IosMetalEvidenceIfRequired {
    param([bool]$ShouldRun)

    if (-not $ShouldRun) {
        return [pscustomobject]@{
            PackageSmokeReady = $false
            FeatureSetChecked = $false
            CommandQueueReady = $false
            PipelineReady = $false
            CommandBufferReady = $false
            ReadbackReady = $false
        }
    }

    $scriptPath = Join-Path $PSScriptRoot "smoke-ios-package.ps1"
    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments @(
        "-NoProfile",
        "-ExecutionPolicy",
        "Bypass",
        "-File",
        $scriptPath,
        "-Game",
        "sample_headless",
        "-Configuration",
        "Debug",
        "-BootTimeoutSeconds",
        "420",
        "-BootAttempts",
        "2"
    ) -TimeoutSeconds 2700
    $text = [string]::Join("`n", @($result.Output, $result.Error))
    $smokeReady = $result.ExitCode -eq 0 -and $text.Contains("ios-smoke: ok")

    return [pscustomobject]@{
        PackageSmokeReady = $smokeReady
        FeatureSetChecked = $smokeReady -and $text.Contains("ios_metal_feature_set_checked=1")
        CommandQueueReady = $smokeReady -and $text.Contains("ios_metal_command_queue_ready=1")
        PipelineReady = $smokeReady -and $text.Contains("ios_metal_pipeline_ready=1")
        CommandBufferReady = $smokeReady -and $text.Contains("ios_metal_command_buffer_ready=1")
        ReadbackReady = $smokeReady -and $text.Contains("ios_metal_readback_ready=1")
    }
}

$hostOs = Get-HostOsCounterValue
$hostMatches = $hostOs -eq "macos"
$xcodeBuild = Find-CommandOnCombinedPath "xcodebuild"
$xcrun = Find-CommandOnCombinedPath "xcrun"
$developerDirectory = Get-AppleDeveloperDirectory
$fullXcodeSelected = Test-FullXcodeDeveloperDirectory $developerDirectory
$macosSdkReady = $hostMatches -and (Test-XcrunSdkAvailable $xcrun "macosx")
$iosSdkReady = $hostMatches -and (Test-XcrunSdkAvailable $xcrun "iphonesimulator")
$iosSimulatorReady = $hostMatches -and (Test-IosSimulatorRuntimeAvailable $xcrun)
$xcodebuildReady = $hostMatches -and -not [string]::IsNullOrWhiteSpace($xcodeBuild) -and
    -not [string]::IsNullOrWhiteSpace($xcrun) -and $fullXcodeSelected
$xcrunMetalReady = $hostMatches -and
    (Test-XcrunToolAvailable $xcrun "macosx" "metal") -and
    (Test-XcrunToolAvailable $xcrun "macosx" "metallib")
$macosFeatureSetTableChecked = $hostMatches -and $macosSdkReady

$runMacosEvidence = $RequireReady.IsPresent -and $hostMatches -and ($Platform -eq "all" -or $Platform -eq "macos") -and
    $xcodebuildReady -and $xcrunMetalReady -and $macosFeatureSetTableChecked
$runIosEvidence = $RequireReady.IsPresent -and $hostMatches -and ($Platform -eq "all" -or $Platform -eq "ios") -and
    $xcodebuildReady -and $iosSdkReady -and $iosSimulatorReady -and $xcrunMetalReady

$macosEvidence = Invoke-MacOSMetalEvidenceIfRequired -ShouldRun:$runMacosEvidence
$iosEvidence = Invoke-IosMetalEvidenceIfRequired -ShouldRun:$runIosEvidence

$macosMetalReady = $xcodebuildReady -and $xcrunMetalReady -and $macosFeatureSetTableChecked -and
    $macosEvidence.CommandQueueReady -and $macosEvidence.CommandBufferReady -and
    $macosEvidence.RenderPipelineReady -and $macosEvidence.ComputePipelineReady -and
    $macosEvidence.TextureUsageRowsReady -and $macosEvidence.ResourceSynchronizationReady -and
    $macosEvidence.ReadbackReady
$iosMetalReady = $xcodebuildReady -and $iosSdkReady -and $iosSimulatorReady -and
    $iosEvidence.FeatureSetChecked -and $iosEvidence.PackageSmokeReady -and
    $iosEvidence.CommandQueueReady -and $iosEvidence.PipelineReady -and
    $iosEvidence.CommandBufferReady -and $iosEvidence.ReadbackReady
$metalAggregateReady = $macosMetalReady -and $iosMetalReady

foreach ($counter in @($ExpectedEvidenceCounters)) {
    if (-not [string]::IsNullOrWhiteSpace($counter)) {
        Write-Output "environment-platform-apple-metal-required-counter: $counter"
    }
}

$actualCounters = @(
    "validation_recipe=environment-platform-ios-metal-package",
    "host_gate=ios-metal-host",
    "host=$hostOs",
    "host_matches=$(ConvertTo-CounterBit $hostMatches)",
    "xcodebuild_ready=$(ConvertTo-CounterBit $xcodebuildReady)",
    "xcrun_metal_ready=$(ConvertTo-CounterBit $xcrunMetalReady)",
    "metal_feature_set_table_checked=$(ConvertTo-CounterBit $macosFeatureSetTableChecked)",
    "metal_command_queue_ready=$(ConvertTo-CounterBit $macosEvidence.CommandQueueReady)",
    "metal_command_buffer_ready=$(ConvertTo-CounterBit $macosEvidence.CommandBufferReady)",
    "metal_render_pipeline_ready=$(ConvertTo-CounterBit $macosEvidence.RenderPipelineReady)",
    "metal_compute_pipeline_ready=$(ConvertTo-CounterBit $macosEvidence.ComputePipelineReady)",
    "metal_texture_usage_rows_ready=$(ConvertTo-CounterBit $macosEvidence.TextureUsageRowsReady)",
    "metal_resource_synchronization_ready=$(ConvertTo-CounterBit $macosEvidence.ResourceSynchronizationReady)",
    "metal_readback_ready=$(ConvertTo-CounterBit $macosEvidence.ReadbackReady)",
    "environment_platform_macos_metal_ready=$(ConvertTo-CounterBit $macosMetalReady)",
    "environment_platform_requires_macos_metal_host_evidence=$(if ($macosMetalReady) { '0' } else { '1' })",
    "xcode_ios_sdk_ready=$(ConvertTo-CounterBit $iosSdkReady)",
    "ios_simulator_or_device_ready=$(ConvertTo-CounterBit $iosSimulatorReady)",
    "ios_metal_feature_set_checked=$(ConvertTo-CounterBit $iosEvidence.FeatureSetChecked)",
    "ios_package_smoke_ready=$(ConvertTo-CounterBit $iosEvidence.PackageSmokeReady)",
    "ios_metal_command_queue_ready=$(ConvertTo-CounterBit $iosEvidence.CommandQueueReady)",
    "ios_metal_pipeline_ready=$(ConvertTo-CounterBit $iosEvidence.PipelineReady)",
    "ios_metal_command_buffer_ready=$(ConvertTo-CounterBit $iosEvidence.CommandBufferReady)",
    "ios_metal_readback_ready=$(ConvertTo-CounterBit $iosEvidence.ReadbackReady)",
    "environment_platform_ios_metal_ready=$(ConvertTo-CounterBit $iosMetalReady)",
    "environment_platform_requires_ios_metal_host_evidence=$(if ($iosMetalReady) { '0' } else { '1' })",
    "environment_metal_aggregate_status=$(if ($metalAggregateReady) { 'ready' } else { 'host_evidence_required' })",
    "environment_metal_aggregate_ready=$(ConvertTo-CounterBit $metalAggregateReady)",
    "environment_metal_aggregate_macos_metal_ready=$(ConvertTo-CounterBit $macosMetalReady)",
    "environment_metal_aggregate_ios_metal_ready=$(ConvertTo-CounterBit $iosMetalReady)",
    "environment_metal_aggregate_native_handle_access=0",
    "environment_metal_aggregate_diagnostics=0",
    "environment_all_platform_unconditional_ready=0",
    "macos_metal_inferred=0",
    "ios_metal_inferred=0",
    "native_handle_access=0"
)
$actualLine = [string]::Join(" ", $actualCounters)
Write-Output $actualLine

$missingExpectedCounters = @()
foreach ($counter in @($ExpectedEvidenceCounters)) {
    if (-not [string]::IsNullOrWhiteSpace($counter) -and -not $actualLine.Contains($counter)) {
        $missingExpectedCounters += $counter
    }
}
if ($missingExpectedCounters.Count -gt 0) {
    Write-Error "Apple Metal platform evidence is missing expected actual counters: $($missingExpectedCounters -join ', ')"
}

if ($RequireReady) {
    if (($Platform -eq "all" -or $Platform -eq "macos") -and -not $macosMetalReady) {
        Write-Error "macOS Metal platform evidence requires macOS with full Xcode, xcrun metal/metallib, Metal feature table/capability evidence, command queue/buffer, render and compute pipelines, texture usage rows, synchronization, and readback evidence."
    }
    if (($Platform -eq "all" -or $Platform -eq "ios") -and -not $iosMetalReady) {
        Write-Error "iOS Metal platform evidence requires macOS with full Xcode, iOS Simulator SDK/runtime, iOS package smoke, Metal feature-set check, command queue, compute pipeline, command buffer, and readback evidence."
    }
    if ($Platform -eq "all" -and -not $metalAggregateReady) {
        Write-Error "Apple Metal aggregate evidence requires both macOS Metal and iOS Metal platform evidence before environment_metal_aggregate_ready=1."
    }
}
