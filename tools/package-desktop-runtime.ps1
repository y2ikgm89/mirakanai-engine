#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$GameTarget = "sample_desktop_runtime_shell",
    [string[]]$SmokeArgs = @(),
    [switch]$RequireD3d12Shaders,
    [switch]$RequireVulkanShaders
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgExe = Join-Path $root "external/vcpkg/vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg is required for the desktop runtime package. Run 'pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1' first."
}

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools
if (-not $tools.CPack) {
    Write-Error "CPack is required but was not found. Install official CMake 3.30+ or Visual Studio Build Tools with C++ CMake tools."
}

if ([string]::IsNullOrWhiteSpace($GameTarget)) {
    Write-Error "GameTarget is required."
}

$targetSmokePattern = [System.Text.RegularExpressions.Regex]::Escape($GameTarget) + "(_shader_artifacts|_vulkan_shader_artifacts)?_smoke"
$ctestPattern = "mirakana_runtime_host_tests|mirakana_runtime_host_sdl3_tests|mirakana_sdl3_platform_tests|mirakana_sdl3_audio_tests|$targetSmokePattern"

$configureArgs = @(
    "--preset",
    "desktop-runtime-release",
    "-DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=$GameTarget",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=$(if ($RequireD3d12Shaders.IsPresent) { 'ON' } else { 'OFF' })",
    "-DMK_REQUIRE_DESKTOP_RUNTIME_SPIRV=$(if ($RequireVulkanShaders.IsPresent) { 'ON' } else { 'OFF' })"
)
Invoke-CheckedCommand $tools.CMake @configureArgs

$buildDir = Join-Path $root "out/build/desktop-runtime-release"
$metadata = Read-DesktopRuntimeGameMetadata -Path (Join-Path $buildDir "desktop-runtime-games.json")
if ([string]::IsNullOrWhiteSpace($metadata.selectedPackageTarget)) {
    Write-Error "Desktop runtime package metadata must declare selectedPackageTarget for package validation."
}
if ($metadata.selectedPackageTarget -ne $GameTarget) {
    Write-Error "Desktop runtime package metadata selected '$($metadata.selectedPackageTarget)' but script requested '$GameTarget'."
}
$gameMetadata = Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $GameTarget
$declaresD3d12ShaderArtifacts = $gameMetadata.PSObject.Properties.Name.Contains("d3d12ShaderArtifacts") -and @($gameMetadata.d3d12ShaderArtifacts).Count -gt 0
if ($RequireD3d12Shaders.IsPresent -and -not $declaresD3d12ShaderArtifacts) {
    Write-Error "-RequireD3d12Shaders can only be used with a desktop runtime package target whose metadata declares D3D12 shader artifacts: $GameTarget"
}
$declaresVulkanShaderArtifacts = $gameMetadata.PSObject.Properties.Name.Contains("vulkanShaderArtifacts") -and @($gameMetadata.vulkanShaderArtifacts).Count -gt 0
if ($RequireVulkanShaders.IsPresent -and -not $declaresVulkanShaderArtifacts) {
    Write-Error "-RequireVulkanShaders can only be used with a desktop runtime package target whose metadata declares Vulkan shader artifacts: $GameTarget"
}
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
    Write-Error "Desktop runtime package target '$GameTarget' does not declare package smoke args."
}
$requireD3d12ShaderArtifacts = $RequireD3d12Shaders.IsPresent -or [bool]$gameMetadata.requiresD3d12Shaders
$requireVulkanShaderArtifacts = $RequireVulkanShaders.IsPresent -or ($gameMetadata.PSObject.Properties.Name.Contains("requiresVulkanShaders") -and [bool]$gameMetadata.requiresVulkanShaders)

Invoke-CheckedCommand $tools.CMake --build --preset desktop-runtime-release
Invoke-CheckedCommand $tools.CTest --preset desktop-runtime-release --output-on-failure -R $ctestPattern

$installPrefix = Join-Path $root "out/install/desktop-runtime-release"
$installRoot = [System.IO.Path]::GetFullPath((Join-Path $root "out/install"))
$installPrefixPath = [System.IO.Path]::GetFullPath($installPrefix)
$trimChars = @([System.IO.Path]::DirectorySeparatorChar, [System.IO.Path]::AltDirectorySeparatorChar)
$installRootNormalized = $installRoot.TrimEnd($trimChars)
$installPrefixNormalized = $installPrefixPath.TrimEnd($trimChars)
$installRootPrefix = $installRootNormalized + [System.IO.Path]::DirectorySeparatorChar
if (-not $installPrefixNormalized.StartsWith($installRootPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
    Write-Error "Refusing to clean desktop runtime install prefix outside repository install root: $installPrefixPath"
}
if (Test-Path -LiteralPath $installPrefixPath -PathType Container) {
    Remove-Item -LiteralPath $installPrefixPath -Recurse -Force
}
$vcpkgInstalled = Join-Path $root "vcpkg_installed/x64-windows"
Invoke-CheckedCommand $tools.CMake --install $buildDir --config Release --prefix $installPrefix
& (Join-Path $PSScriptRoot "validate-installed-sdk.ps1") `
    -InstallPrefix $installPrefix `
    -BuildDir (Join-Path $root "out/build/installed-desktop-runtime-consumer") `
    -BuildConfig Release `
    -AdditionalRuntimePaths @((Join-Path $vcpkgInstalled "bin")) `
    -AdditionalCMakeArgs @("-DCMAKE_PREFIX_PATH=$vcpkgInstalled")
& (Join-Path $PSScriptRoot "validate-installed-desktop-runtime.ps1") `
    -InstallPrefix $installPrefix `
    -GameTarget $GameTarget `
    -SmokeArgs $SmokeArgs `
    -RequireD3d12Shaders:$requireD3d12ShaderArtifacts `
    -RequireVulkanShaders:$requireVulkanShaderArtifacts

Invoke-CheckedCommand $tools.CPack --preset desktop-runtime-release

Write-Host "desktop-runtime-package: ok ($GameTarget)"
