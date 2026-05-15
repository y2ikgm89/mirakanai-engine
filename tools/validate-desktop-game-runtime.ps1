#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$vcpkgExe = Join-Path $root "external/vcpkg/vcpkg.exe"

if (-not (Test-Path $vcpkgExe)) {
    Write-Error "vcpkg is required for the desktop game runtime validation. Run 'pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1' first."
}

Set-MirakanaiVcpkgEnvironment | Out-Null

$tools = Assert-CppBuildTools

Invoke-CheckedCommand $tools.CMake --preset desktop-runtime

$buildDir = Join-Path $root "out/build/desktop-runtime"
$metadataPath = Join-Path $buildDir "desktop-runtime-games.json"
$metadata = Read-DesktopRuntimeGameMetadata -Path $metadataPath
$registeredGames = @($metadata.registeredGames)
if ($registeredGames.Count -eq 0) {
    Write-Error "Desktop runtime metadata must contain at least one registered game."
}
foreach ($game in $registeredGames) {
    if ([string]::IsNullOrWhiteSpace($game.target)) {
        Write-Error "Desktop runtime metadata contains a registered game without a target name."
    }
    if (@($game.smokeArgs).Count -eq 0) {
        Write-Error "Desktop runtime metadata target '$($game.target)' must declare source-tree smoke args."
    }
    Assert-DesktopRuntimeGameManifestContract `
        -GameMetadata $game `
        -Root $root `
        -RequiredPackagingTargets @("desktop-game-runtime", "desktop-runtime-release") | Out-Null
    Assert-DesktopRuntimeGamePackageFiles `
        -GameMetadata $game `
        -Root $root | Out-Null
}
foreach ($requiredTarget in @(
    "sample_desktop_runtime_shell",
    "sample_desktop_runtime_game",
    "sample_2d_desktop_runtime_package",
    "sample_generated_desktop_runtime_package",
    "sample_generated_desktop_runtime_cooked_scene_package",
    "sample_generated_desktop_runtime_material_shader_package"
)) {
    Get-DesktopRuntimeGameMetadata -Metadata $metadata -GameTarget $requiredTarget | Out-Null
}

$desktopRuntimeTargets = @($registeredGames | ForEach-Object { $_.target })
$buildTargets = @(
    "mirakana_runtime_host_tests",
    "mirakana_runtime_host_sdl3_public_api_compile",
    "mirakana_runtime_host_sdl3_tests",
    "mirakana_sdl3_platform_tests",
    "mirakana_sdl3_audio_tests"
) + $desktopRuntimeTargets
$buildArgs = @("--build", "--preset", "desktop-runtime", "--target") + $buildTargets
Invoke-CheckedCommand $tools.CMake @buildArgs

$testPatterns = @(
    "mirakana_runtime_host_tests",
    "mirakana_runtime_host_sdl3_public_api_compile",
    "mirakana_runtime_host_sdl3_tests",
    "mirakana_sdl3_platform_tests",
    "mirakana_sdl3_audio_tests"
) + @($desktopRuntimeTargets | ForEach-Object {
    [System.Text.RegularExpressions.Regex]::Escape($_) + "(_shader_artifacts|_vulkan_shader_artifacts)?_smoke"
})
$ctestPattern = $testPatterns -join "|"
Invoke-CheckedCommand $tools.CTest --preset desktop-runtime --output-on-failure -R $ctestPattern
