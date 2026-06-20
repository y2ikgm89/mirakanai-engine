#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$InstallPrefix = "",
    [string]$BuildConfig = "Release",
    [string]$GameTarget = "",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireVulkanShaders
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
    Write-Error "Installed Linux runtime validation requires a Linux host."
}

if ([string]::IsNullOrWhiteSpace($InstallPrefix)) {
    $InstallPrefix = Join-Path $root "out/install/linux-runtime-release"
}

$installPrefixPath = [System.IO.Path]::GetFullPath($InstallPrefix)
$installedBin = Join-Path $installPrefixPath "bin"
$installedMetadataPath = Join-Path $installPrefixPath "share/Mirakanai/desktop-runtime-games.json"
$gameMetadata = $null

if (Test-Path -LiteralPath $installedMetadataPath -PathType Leaf) {
    $metadata = Read-DesktopRuntimeGameMetadata -Path $installedMetadataPath
    if ([string]::IsNullOrWhiteSpace($metadata.selectedPackageTarget)) {
        Write-Error "Installed Linux runtime metadata must declare selectedPackageTarget: $installedMetadataPath"
    }
    if ([string]::IsNullOrWhiteSpace($GameTarget)) {
        $GameTarget = $metadata.selectedPackageTarget
    } elseif ($metadata.selectedPackageTarget -ne $GameTarget) {
        Write-Error "Installed Linux runtime metadata selected '$($metadata.selectedPackageTarget)' but validation requested '$GameTarget'."
    }
    $gameMetadata = Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $GameTarget
    Assert-DesktopRuntimeGameManifestContract `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -RequiredPackagingTargets @("desktop-game-runtime", "desktop-runtime-release") `
        -Installed | Out-Null
    Assert-DesktopRuntimeGamePackageFiles `
        -GameMetadata $gameMetadata `
        -Root $installPrefixPath `
        -Installed | Out-Null
}

if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required when installed Linux runtime metadata is unavailable."
}
if ($SmokeArgs.Count -eq 0 -and $null -ne $gameMetadata) {
    $SmokeArgs = @($gameMetadata.packageSmokeArgs)
}
if ($SmokeArgs.Count -eq 0) {
    Write-Error "SmokeArgs are required when installed Linux runtime metadata is unavailable or incomplete."
}

$runtimeExecutable = Join-Path $installedBin $GameTarget
if (-not (Test-Path -LiteralPath $runtimeExecutable -PathType Leaf)) {
    Write-Error "Installed Linux runtime game was not found: $runtimeExecutable"
}

if ($RequireVulkanShaders.IsPresent) {
    $shaderArtifacts = @()
    if ($null -ne $gameMetadata -and $gameMetadata.PSObject.Properties.Name.Contains("vulkanShaderArtifacts")) {
        $shaderArtifacts = @($gameMetadata.vulkanShaderArtifacts | ForEach-Object { [string]$_ })
    }
    if ($shaderArtifacts.Count -eq 0) {
        $shaderArtifacts = @("shaders/runtime_shell.vs.spv", "shaders/runtime_shell.ps.spv")
    }

    foreach ($shaderArtifact in $shaderArtifacts) {
        $normalizedShaderArtifact = ConvertTo-DesktopRuntimeMetadataRelativePath `
            -Path $shaderArtifact `
            -Description "Installed Linux runtime Vulkan shader artifact"
        if (-not $normalizedShaderArtifact.StartsWith("shaders/", [System.StringComparison]::Ordinal)) {
            Write-Error "Installed Linux runtime Vulkan shader artifact must stay under shaders/: $normalizedShaderArtifact"
        }
        $shaderPath = Join-Path $installedBin $normalizedShaderArtifact.Replace("/", [System.IO.Path]::DirectorySeparatorChar)
        if (-not (Test-Path -LiteralPath $shaderPath -PathType Leaf)) {
            Write-Error "Installed Linux runtime Vulkan shader artifact was not found: $shaderPath"
        }
        if ((Get-Item -LiteralPath $shaderPath).Length -le 0) {
            Write-Error "Installed Linux runtime Vulkan shader artifact is empty: $shaderPath"
        }
    }
}

function Invoke-InstalledLinuxRuntimeSmoke {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $installedBin
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Arguments)) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }
    $startInfo.Environment["PATH"] = (($installedBin, $startInfo.Environment["PATH"]) -join [System.IO.Path]::PathSeparator)

    $process = [System.Diagnostics.Process]::Start($startInfo)
    $stdout = $process.StandardOutput.ReadToEnd()
    $stderr = $process.StandardError.ReadToEnd()
    $process.WaitForExit()
    if (-not [string]::IsNullOrWhiteSpace($stdout)) {
        Write-Host $stdout.TrimEnd()
    }
    if (-not [string]::IsNullOrWhiteSpace($stderr)) {
        Write-Host $stderr.TrimEnd()
    }
    if ($process.ExitCode -ne 0) {
        Write-Error "Command failed with exit code $($process.ExitCode): $FilePath $($Arguments -join ' ')"
    }
    return [string]::Join("`n", @($stdout, $stderr))
}

$smokeOutput = Invoke-InstalledLinuxRuntimeSmoke -FilePath $runtimeExecutable -Arguments @($SmokeArgs)
$escapedGameTarget = [regex]::Escape($GameTarget)

foreach ($requiredField in @(
        "linux_package_smoke_ready=1",
        "linux_vulkan_readback_ready=1",
        "linux_vulkan_validation_log_clean=1",
        "VK_LAYER_KHRONOS_validation_ready=1",
        "native_handle_access=0"
    )) {
    if ($smokeOutput -notmatch "(?m)^$escapedGameTarget status=.*\b$([regex]::Escape($requiredField))\b") {
        Write-Error "Installed Linux runtime smoke status line did not prove required field: $requiredField"
    }
}

Write-Host "linux_package_smoke_ready=1"
Write-Host "linux_vulkan_readback_ready=1"
Write-Host "linux_vulkan_validation_log_clean=1"
Write-Host "VK_LAYER_KHRONOS_validation_ready=1"
Write-Host "native_handle_access=0"
Write-Host "installed-linux-runtime-validation: ok ($GameTarget)"
