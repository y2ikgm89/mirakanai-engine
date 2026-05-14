#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Assert-Property($object, [string]$property, [string]$label) {
    if (-not $object.PSObject.Properties.Name.Contains($property)) {
        Write-Error "$label missing required property: $property"
    }
}

function Assert-TextContains([string]$relativePath, [string]$pattern, [string]$label) {
    $path = Join-Path $root $relativePath
    if (-not (Test-Path $path)) {
        Write-Error "Missing file for dependency policy check: $relativePath"
    }
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -notmatch $pattern) {
        Write-Error "$label missing expected text pattern: $pattern"
    }
}

function Assert-CacheVariableEquals($preset, [string]$variable, [string]$expected) {
    if (-not $preset.cacheVariables.PSObject.Properties.Name.Contains($variable)) {
        Write-Error "CMake preset '$($preset.name)' missing required cache variable: $variable"
    }

    $actual = [string]$preset.cacheVariables.$variable
    if ($actual -ne $expected) {
        Write-Error "CMake preset '$($preset.name)' must set $variable to '$expected' but got '$actual'"
    }
}

$manifest = Read-Json "vcpkg.json"
$presets = Read-Json "CMakePresets.json"
Assert-Property $manifest '$schema' "vcpkg manifest"
Assert-Property $manifest "builtin-baseline" "vcpkg manifest"
Assert-Property $manifest "features" "vcpkg manifest"

if ($manifest.'$schema' -ne "https://raw.githubusercontent.com/microsoft/vcpkg-tool/main/docs/vcpkg.schema.json") {
    Write-Error "vcpkg manifest must reference the official vcpkg JSON schema"
}

if ($manifest.'builtin-baseline' -notmatch "^[0-9a-f]{40}$") {
    Write-Error "vcpkg builtin-baseline must be a 40-character git commit hash"
}

if ($manifest.dependencies.Count -ne 0) {
    Write-Error "Default build must remain third-party-free; put optional dependencies behind manifest features"
}

$desktopGui = $manifest.features.'desktop-gui'
if (-not $desktopGui) {
    Write-Error "vcpkg manifest must keep optional desktop-gui feature"
}

$desktopRuntime = $manifest.features.'desktop-runtime'
if (-not $desktopRuntime) {
    Write-Error "vcpkg manifest must keep optional desktop-runtime feature"
}

$assetImporters = $manifest.features.'asset-importers'
if (-not $assetImporters) {
    Write-Error "vcpkg manifest must keep optional asset-importers feature"
}

$desktopRuntimeDependencyNames = @()
foreach ($dependency in $desktopRuntime.dependencies) {
    if ($dependency -is [string]) {
        $desktopRuntimeDependencyNames += $dependency
    } else {
        $desktopRuntimeDependencyNames += $dependency.name
    }
}

if ($desktopRuntimeDependencyNames -notcontains "sdl3") {
    Write-Error "desktop-runtime feature must declare dependency: sdl3"
}

if ($desktopRuntimeDependencyNames -contains "imgui") {
    Write-Error "desktop-runtime feature must not depend on Dear ImGui"
}

$dependencyNames = @()
$imguiFeatures = @()
foreach ($dependency in $desktopGui.dependencies) {
    if ($dependency -is [string]) {
        $dependencyNames += $dependency
    } else {
        $dependencyNames += $dependency.name
        if ($dependency.name -eq "imgui") {
            $imguiFeatures = @($dependency.features)
        }
    }
}

foreach ($dependencyName in @("sdl3", "imgui")) {
    if ($dependencyNames -notcontains $dependencyName) {
        Write-Error "desktop-gui feature must declare dependency: $dependencyName"
    }
}

foreach ($feature in @("docking-experimental", "sdl3-binding", "sdl3-renderer-binding")) {
    if ($imguiFeatures -notcontains $feature) {
        Write-Error "desktop-gui imgui dependency must enable feature: $feature"
    }
}

$assetImporterDependencyNames = @()
foreach ($dependency in $assetImporters.dependencies) {
    if ($dependency -is [string]) {
        $assetImporterDependencyNames += $dependency
    } else {
        $assetImporterDependencyNames += $dependency.name
    }
}

foreach ($dependencyName in @("libspng", "fastgltf", "miniaudio")) {
    if ($assetImporterDependencyNames -notcontains $dependencyName) {
        Write-Error "asset-importers feature must declare dependency: $dependencyName"
    }
}

Assert-TextContains "THIRD_PARTY_NOTICES.md" "\| SDL3 \|" "third-party notices"
Assert-TextContains "THIRD_PARTY_NOTICES.md" "\| Dear ImGui \|" "third-party notices"
Assert-TextContains "docs/dependencies.md" "builtin-baseline" "dependency docs"
Assert-TextContains "docs/dependencies.md" "Foundation" "dependency docs"
Assert-TextContains "docs/legal-and-licensing.md" "Foundation" "legal dependency docs"
Assert-TextContains "CMakePresets.json" "desktop-runtime" "CMake presets"
Assert-TextContains "CMakePresets.json" "asset-importers" "CMake presets"
Assert-TextContains "engine/rhi/metal/CMakeLists.txt" 'find_library\(MK_APPLE_FOUNDATION_FRAMEWORK Foundation REQUIRED\)' "Metal Apple SDK linkage"
Assert-TextContains "engine/rhi/metal/CMakeLists.txt" '\$\{MK_APPLE_FOUNDATION_FRAMEWORK\}' "Metal Apple SDK linkage"

$vcpkgPresets = @($presets.configurePresets | Where-Object {
        $_.PSObject.Properties.Name.Contains("cacheVariables") -and
        $_.cacheVariables.PSObject.Properties.Name.Contains("CMAKE_TOOLCHAIN_FILE") -and
        ([string]$_.cacheVariables.CMAKE_TOOLCHAIN_FILE).Contains("external/vcpkg/scripts/buildsystems/vcpkg.cmake")
    })
if ($vcpkgPresets.Count -eq 0) {
    Write-Error "CMake presets must keep explicit vcpkg-backed configure presets for optional dependency lanes"
}
foreach ($preset in $vcpkgPresets) {
    Assert-CacheVariableEquals $preset "VCPKG_MANIFEST_INSTALL" "OFF"
    Assert-CacheVariableEquals $preset "VCPKG_INSTALLED_DIR" '${sourceDir}/vcpkg_installed'
    Assert-CacheVariableEquals $preset "VCPKG_TARGET_TRIPLET" "x64-windows"
    if ($preset.cacheVariables.PSObject.Properties.Name.Contains("VCPKG_MANIFEST_FEATURES")) {
        Write-Error "CMake preset '$($preset.name)' must not declare VCPKG_MANIFEST_FEATURES when VCPKG_MANIFEST_INSTALL is OFF; feature selection belongs in bootstrap-deps."
    }
}

Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=desktop-runtime" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=desktop-gui" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=asset-importers" "bootstrap dependencies"

Assert-TextContains "engine/agent/manifest.json" "buildAssetImporters" "engine manifest commands"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "Mirakanai_HAS_ASSET_IMPORTERS" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(SPNG CONFIG\)" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(fastgltf CONFIG\)" "Mirakanai package config"

Write-Host "dependency-policy-check: ok"
