#requires -Version 7.0
#requires -PSEdition Core

param(
    [switch]$RequireReady,
    [string[]]$ExpectedEvidenceCounters = @()
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Get-HostOsCounterValue {
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
        return "linux"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Windows)) {
        return "windows"
    }
    if ([System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::OSX)) {
        return "macos"
    }
    return "unknown"
}

function Join-IfSet {
    param(
        [AllowNull()][string]$Base,
        [Parameter(Mandatory = $true)][string]$Child
    )

    if ([string]::IsNullOrWhiteSpace($Base)) {
        return $null
    }
    return Join-Path $Base $Child
}

function Get-FirstExistingTool {
    param([string[]]$Candidates)

    foreach ($candidate in @($Candidates)) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }
        if (Test-Path -LiteralPath $candidate -PathType Leaf) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }
    return $null
}

function Find-VulkanInfoCommand {
    $envRoots = @(
        (Get-EnvironmentVariableAnyScope "VULKAN_SDK"),
        (Get-EnvironmentVariableAnyScope "VK_SDK_PATH")
    )
    $candidates = @()
    foreach ($envRoot in $envRoots) {
        $candidates += Join-IfSet $envRoot "bin/vulkaninfo"
        $candidates += Join-IfSet $envRoot "Bin/vulkaninfo.exe"
    }

    $fromKnownLocation = Get-FirstExistingTool $candidates
    if ($null -ne $fromKnownLocation) {
        return $fromKnownLocation
    }

    return Find-CommandOnCombinedPath "vulkaninfo"
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

function Get-ShaderToolchainEvidence {
    $scriptPath = Join-Path $PSScriptRoot "check-shader-toolchain.ps1"
    $result = Invoke-ToolCapture -FilePath "pwsh" -Arguments @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $scriptPath) -TimeoutSeconds 60
    $text = [string]::Join("`n", @($result.Output, $result.Error))
    return [pscustomobject]@{
        DxcSpirvCodegenReady = $result.ExitCode -eq 0 -and $text.Contains("shader-toolchain: dxc_spirv_codegen=ready")
        SpirvValReady = $result.ExitCode -eq 0 -and $text.Contains("shader-toolchain: spirv-val=found")
    }
}

$hostOs = Get-HostOsCounterValue
$hostMatches = $hostOs -eq "linux"
$shaderEvidence = Get-ShaderToolchainEvidence
$vulkanInfoCommand = if ($hostMatches) { Find-VulkanInfoCommand } else { $null }
$vulkanInfoReady = $false
$validationLayerReady = $false

if ($hostMatches -and $null -ne $vulkanInfoCommand) {
    $vulkanInfo = Invoke-ToolCapture -FilePath $vulkanInfoCommand -Arguments @("--summary") -TimeoutSeconds 20
    $vulkanInfoText = [string]::Join("`n", @($vulkanInfo.Output, $vulkanInfo.Error))
    $vulkanInfoReady = $vulkanInfo.ExitCode -eq 0 -and
        ($vulkanInfoText.Contains("Vulkan Instance Version") -or $vulkanInfoText.Contains("GPU") -or $vulkanInfoText.Contains("deviceName"))
    $validationLayerReady = $vulkanInfo.ExitCode -eq 0 -and $vulkanInfoText.Contains("VK_LAYER_KHRONOS_validation")
}

$linuxPackageScript = Join-Path $root "tools/package-linux-runtime.ps1"
$linuxInstalledValidator = Join-Path $root "tools/validate-installed-linux-runtime.ps1"
$linuxHostRoot = Join-Path $root "platform/linux"

$linuxIcdRuntimeReady = $hostMatches -and $vulkanInfoReady
$firstPartyLinuxRuntimeHostReady = $hostMatches -and (Test-Path -LiteralPath $linuxHostRoot -PathType Container)
$linuxPackageScriptReady = $hostMatches -and (Test-Path -LiteralPath $linuxPackageScript -PathType Leaf)
$linuxInstalledValidatorReady = $hostMatches -and (Test-Path -LiteralPath $linuxInstalledValidator -PathType Leaf)

$linuxVulkanReady = $hostMatches -and
    $vulkanInfoReady -and
    $validationLayerReady -and
    $shaderEvidence.DxcSpirvCodegenReady -and
    $shaderEvidence.SpirvValReady -and
    $linuxIcdRuntimeReady -and
    $firstPartyLinuxRuntimeHostReady -and
    $linuxPackageScriptReady -and
    $linuxInstalledValidatorReady

foreach ($counter in @($ExpectedEvidenceCounters)) {
    if (-not [string]::IsNullOrWhiteSpace($counter)) {
        Write-Output "environment-platform-linux-vulkan-required-counter: $counter"
    }
}

$actualCounters = @(
    "validation_recipe=environment-platform-linux-vulkan-package",
    "host_gate=linux-vulkan-runtime-host",
    "host=$hostOs",
    "host_matches=$(ConvertTo-CounterBit $hostMatches)",
    "vulkaninfo_ready=$(ConvertTo-CounterBit $vulkanInfoReady)",
    "VK_LAYER_KHRONOS_validation_ready=$(ConvertTo-CounterBit $validationLayerReady)",
    "dxc_spirv_codegen_ready=$(ConvertTo-CounterBit $shaderEvidence.DxcSpirvCodegenReady)",
    "spirv_val_ready=$(ConvertTo-CounterBit $shaderEvidence.SpirvValReady)",
    "linux_icd_runtime_ready=$(ConvertTo-CounterBit $linuxIcdRuntimeReady)",
    "first_party_linux_runtime_host_ready=$(ConvertTo-CounterBit $firstPartyLinuxRuntimeHostReady)",
    "linux_package_script_ready=$(ConvertTo-CounterBit $linuxPackageScriptReady)",
    "linux_installed_validator_ready=$(ConvertTo-CounterBit $linuxInstalledValidatorReady)",
    "environment_platform_linux_vulkan_ready=$(ConvertTo-CounterBit $linuxVulkanReady)",
    "environment_platform_requires_linux_vulkan_host_evidence=$(if ($linuxVulkanReady) { '0' } else { '1' })",
    "environment_all_platform_unconditional_ready=0",
    "windows_vulkan_inferred=0",
    "android_vulkan_inferred=0",
    "native_handle_access=0"
)
Write-Output ([string]::Join(" ", $actualCounters))

if ($RequireReady -and -not $linuxVulkanReady) {
    Write-Error "environment-platform-linux-vulkan-package requires a Linux Vulkan host with vulkaninfo, VK_LAYER_KHRONOS_validation, DXC SPIR-V CodeGen, spirv-val, Linux ICD/runtime, first-party Linux runtime host, Linux package script, and installed validator evidence."
}
