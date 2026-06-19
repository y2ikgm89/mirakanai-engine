#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [string]$GameTarget = "sample_desktop_runtime_game",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireVulkanShaders
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

if (-not [System.Runtime.InteropServices.RuntimeInformation]::IsOSPlatform([System.Runtime.InteropServices.OSPlatform]::Linux)) {
    Write-Error "Linux runtime packaging requires a Linux host."
}
if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required."
}

$null = Assert-VcpkgExecutable -Purpose "the Linux runtime package"
Set-MirakanaiVcpkgEnvironment | Out-Null
$tools = Assert-CppBuildTools
if (-not $tools.CPack) {
    Write-Error "CPack is required but was not found. Install official CMake 3.30+."
}

$configureArgs = @(
    "--preset",
    "desktop-runtime-release",
    "-DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=$GameTarget",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=OFF",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_SPIRV=$(if ($RequireVulkanShaders.IsPresent) { 'ON' } else { 'OFF' })"
)
Invoke-CheckedCommand $tools.CMake @configureArgs

$buildDir = Join-Path $root "out/build/desktop-runtime-release"
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

Invoke-CheckedCommand $tools.CMake --build --preset desktop-runtime-release --target MK_desktop_runtime_package_build
Invoke-CheckedCommand $tools.CTest --preset desktop-runtime-release --output-on-failure -R "MK_runtime_host_tests|$([regex]::Escape($GameTarget))(_vulkan_shader_artifacts)?_smoke"

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

Invoke-CheckedCommand $tools.CMake --install $buildDir --config Release --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-linux-runtime.ps1") `
    -InstallPrefix $installPrefix `
    -GameTarget $GameTarget `
    -SmokeArgs $SmokeArgs `
    -RequireVulkanShaders:$RequireVulkanShaders.IsPresent

Invoke-CheckedCommand $tools.CPack --preset desktop-runtime-release
Write-Host "linux-runtime-package: ok ($GameTarget)"
