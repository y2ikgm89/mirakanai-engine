#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$GameTarget = "sample_desktop_runtime_game",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireVulkanShaders,
    [string]$DiagnosticLogPath = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Write-PackageDiagnostic {
    param([Parameter(Mandatory = $true)][string]$Message)

    Write-Host $Message
    if (-not [string]::IsNullOrWhiteSpace($DiagnosticLogPath)) {
        $logPath = $DiagnosticLogPath
        if (-not [System.IO.Path]::IsPathRooted($logPath)) {
            $logPath = Join-Path $root $logPath
        }
        $logDirectory = Split-Path -Parent $logPath
        if (-not [string]::IsNullOrWhiteSpace($logDirectory)) {
            New-Item -ItemType Directory -Force -Path $logDirectory | Out-Null
        }
        Add-Content -LiteralPath $logPath -Value $Message -Encoding utf8
    }
}

function Invoke-PackageStage {
    param(
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][scriptblock]$ScriptBlock
    )

    Write-PackageDiagnostic "linux-runtime-package: $Name start"
    try {
        & $ScriptBlock
        Write-PackageDiagnostic "linux-runtime-package: $Name ok"
    } catch {
        Write-PackageDiagnostic "linux-runtime-package: $Name failed: $_"
        throw
    }
}

function Invoke-PackageCommand {
    param(
        [Parameter(Mandatory = $true)][string]$FilePath,
        [string[]]$Arguments = @()
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = (Get-Location).Path
    $startInfo.UseShellExecute = $false
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    foreach ($argument in @($Arguments)) {
        $startInfo.ArgumentList.Add($argument) | Out-Null
    }
    $startInfo.Environment.Clear()
    foreach ($entry in Get-NormalizedProcessEnvironment) {
        $startInfo.Environment[$entry.Key] = $entry.Value
    }

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo
    $null = $process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()
    $process.WaitForExit()
    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()

    if ($process.ExitCode -ne 0) {
        Write-PackageDiagnostic "linux-runtime-package: command failed: $FilePath $($Arguments -join ' ')"
        if (-not [string]::IsNullOrWhiteSpace($stdout)) {
            Write-PackageDiagnostic "linux-runtime-package: stdout:`n$($stdout.TrimEnd())"
        }
        if (-not [string]::IsNullOrWhiteSpace($stderr)) {
            Write-PackageDiagnostic "linux-runtime-package: stderr:`n$($stderr.TrimEnd())"
        }
        Write-Error "Command failed with exit code $($process.ExitCode): $FilePath $($Arguments -join ' ')"
    }
}

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
    Write-Error "Linux runtime packaging requires a Linux host."
}
if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required."
}

$null = Assert-VcpkgExecutable -Purpose "the Linux runtime package"
Set-MirakanaiVcpkgEnvironment | Out-Null
$vcpkgTriplet = Get-VcpkgDefaultTriplet
$presetName = "desktop-runtime-linux-release"
$tools = Assert-CppBuildTools
if (-not $tools.CPack) {
    Write-Error "CPack is required but was not found. Install official CMake 3.30+."
}

$configureArgs = @(
    "--preset",
    $presetName,
    "-DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=$GameTarget",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=OFF",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_SPIRV=$(if ($RequireVulkanShaders.IsPresent) { 'ON' } else { 'OFF' })",
    "-DVCPKG_TARGET_TRIPLET=$vcpkgTriplet"
)
Invoke-PackageStage "configure" {
    Invoke-PackageCommand $tools.CMake $configureArgs
}

$buildDir = Join-Path $root "out/build/desktop-runtime-linux-release"
$metadata = Read-DesktopRuntimeGameMetadata -Path (Join-Path $buildDir "desktop-runtime-games.json")
if ([string]::IsNullOrWhiteSpace($metadata.selectedPackageTarget)) {
    Write-Error "Linux runtime package metadata must declare selectedPackageTarget for package validation."
}
if ($metadata.selectedPackageTarget -ne $GameTarget) {
    Write-Error "Linux runtime package metadata selected '$($metadata.selectedPackageTarget)' but script requested '$GameTarget'."
}
$gameMetadata = Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $GameTarget
Assert-DesktopRuntimeGameManifestContract `
    -GameMetadata $gameMetadata `
    -Root $root `
    -RequiredPackagingTargets @("desktop-game-runtime", "desktop-runtime-release") | Out-Null
Assert-DesktopRuntimeGamePackageFiles `
    -GameMetadata $gameMetadata `
    -Root $root | Out-Null

if ($SmokeArgs.Count -eq 0) {
    $SmokeArgs = @($gameMetadata.packageSmokeArgs)
}
if ($SmokeArgs.Count -eq 0) {
    Write-Error "Linux runtime package target '$GameTarget' does not declare package smoke args."
}

Invoke-PackageStage "build" {
    Invoke-PackageCommand $tools.CMake @("--build", "--preset", $presetName, "--target", "MK_desktop_runtime_package_build")
}
Invoke-PackageStage "ctest" {
    Invoke-PackageCommand $tools.CTest @(
        "--preset",
        $presetName,
        "--output-on-failure",
        "-R",
        "MK_runtime_host_tests|$([regex]::Escape($GameTarget))(_vulkan_shader_artifacts)?_smoke"
    )
}

$installPrefix = Join-Path $root "out/install/linux-runtime-release"
$installRoot = [System.IO.Path]::GetFullPath((Join-Path $root "out/install"))
$installPrefixPath = [System.IO.Path]::GetFullPath($installPrefix)
$trimChars = @([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
$installRootNormalized = $installRoot.TrimEnd($trimChars)
$installPrefixNormalized = $installPrefixPath.TrimEnd($trimChars)
$installRootPrefix = $installRootNormalized + [System.IO.Path]::DirectorySeparatorChar
if (-not $installPrefixNormalized.StartsWith($installRootPrefix, [System.StringComparison]::Ordinal)) {
    Write-Error "Refusing to clean Linux runtime install prefix outside repository install root: $installPrefixPath"
}
if (Test-Path -LiteralPath $installPrefixPath -PathType Container) {
    Remove-Item -LiteralPath $installPrefixPath -Recurse -Force
}

Invoke-PackageStage "install" {
    Invoke-PackageCommand $tools.CMake @("--install", $buildDir, "--config", "Release", "--prefix", $installPrefix)
}
Invoke-PackageStage "validate-installed" {
    & (Join-Path $PSScriptRoot "validate-installed-linux-runtime.ps1") `
        -InstallPrefix $installPrefix `
        -GameTarget $GameTarget `
        -SmokeArgs $SmokeArgs `
        -RequireVulkanShaders:$RequireVulkanShaders.IsPresent
}

Invoke-PackageStage "cpack" {
    Invoke-PackageCommand $tools.CPack @("--preset", $presetName)
}
Write-Host "linux-runtime-package: ok ($GameTarget)"
