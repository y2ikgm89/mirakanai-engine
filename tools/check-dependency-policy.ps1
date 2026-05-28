#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Assert-Property($object, [string]$property, [string]$label) {
    if (-not (Test-JsonProperty -Object $object -Property $property)) {
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
    $cacheVariables = Get-JsonPropertyValue -Object $preset -Property "cacheVariables"
    if (-not (Test-JsonProperty -Object $cacheVariables -Property $variable)) {
        Write-Error "CMake preset '$($preset.name)' missing required cache variable: $variable"
    }

    $actual = [string](Get-JsonPropertyValue -Object $cacheVariables -Property $variable)
    if ($actual -ne $expected) {
        Write-Error "CMake preset '$($preset.name)' must set $variable to '$expected' but got '$actual'"
    }
}

$manifest = Read-Json "vcpkg.json"
$presets = Read-CMakePresets
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

$physicsJolt = $manifest.features.'physics-jolt'
if (-not $physicsJolt) {
    Write-Error "vcpkg manifest must keep optional physics-jolt feature"
}

$networkEnet = $manifest.features.'network-enet'
if (-not $networkEnet) {
    Write-Error "vcpkg manifest must keep optional network-enet feature"
}

$desktopRuntimeDependencyNames = @()
foreach ($dependency in $desktopRuntime.dependencies) {
    if ($dependency -is [string]) {
        $desktopRuntimeDependencyNames += $dependency
    } else {
        $desktopRuntimeDependencyNames += $dependency.name
    }
}

foreach ($dependencyName in @("sdl3", "imgui")) {
    if ($desktopRuntimeDependencyNames -contains $dependencyName) {
        Write-Error "desktop-runtime feature uses the first-party Windows native lane and must not declare dependency: $dependencyName"
    }
}

if ($desktopRuntimeDependencyNames.Count -ne 0) {
    Write-Error "desktop-runtime feature uses host SDKs and must not declare vcpkg package dependencies"
}

$dependencyNames = @()
foreach ($dependency in $desktopGui.dependencies) {
    if ($dependency -is [string]) {
        $dependencyNames += $dependency
    } else {
        $dependencyNames += $dependency.name
    }
}

foreach ($dependencyName in @("sdl3", "imgui")) {
    if ($dependencyNames -contains $dependencyName) {
        Write-Error "desktop-gui feature is deferred after SDL3 removal and must not declare dependency: $dependencyName"
    }
}

if ($dependencyNames.Count -ne 0) {
    Write-Error "desktop-gui feature is deferred after SDL3 removal and must not declare package dependencies"
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

$physicsJoltDependencyNames = @()
$joltPhysicsDefaultFeatures = $null
foreach ($dependency in $physicsJolt.dependencies) {
    if ($dependency -is [string]) {
        $physicsJoltDependencyNames += $dependency
    } else {
        $physicsJoltDependencyNames += $dependency.name
        if ($dependency.name -eq "joltphysics" -and (Test-JsonProperty -Object $dependency -Property "default-features")) {
            $joltPhysicsDefaultFeatures = Get-JsonPropertyValue -Object $dependency -Property "default-features"
        }
    }
}

if ($physicsJoltDependencyNames -notcontains "joltphysics") {
    Write-Error "physics-jolt feature must declare dependency: joltphysics"
}

if ($joltPhysicsDefaultFeatures -ne $false) {
    Write-Error "physics-jolt joltphysics dependency must set default-features to false"
}

$networkEnetDependencyNames = @()
$enetDefaultFeatures = $null
foreach ($dependency in $networkEnet.dependencies) {
    if ($dependency -is [string]) {
        $networkEnetDependencyNames += $dependency
    } else {
        $networkEnetDependencyNames += $dependency.name
        if ($dependency.name -eq "enet" -and (Test-JsonProperty -Object $dependency -Property "default-features")) {
            $enetDefaultFeatures = Get-JsonPropertyValue -Object $dependency -Property "default-features"
        }
    }
}

if ($networkEnetDependencyNames -notcontains "enet") {
    Write-Error "network-enet feature must declare dependency: enet"
}

if ($enetDefaultFeatures -ne $false) {
    Write-Error "network-enet enet dependency must set default-features to false"
}

if ((Get-Content -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -Raw) -match "\| SDL3 \|") {
    Write-Error "third-party notices must not list SDL3 after final SDL3 source and dependency removal"
}
if ((Get-Content -LiteralPath (Join-Path $root "THIRD_PARTY_NOTICES.md") -Raw) -match "\| Dear ImGui \|") {
    Write-Error "third-party notices must not list Dear ImGui while the visible editor shell is deferred and no package dependency declares it"
}
Assert-TextContains "THIRD_PARTY_NOTICES.md" "\| Jolt Physics \|" "third-party notices"
Assert-TextContains "THIRD_PARTY_NOTICES.md" "\| ENet \|" "third-party notices"
Assert-TextContains "docs/dependencies.md" "builtin-baseline" "dependency docs"
Assert-TextContains "docs/dependencies.md" "Foundation" "dependency docs"
Assert-TextContains "docs/dependencies.md" "physics-jolt" "dependency docs"
Assert-TextContains "docs/dependencies.md" "network-enet" "dependency docs"
Assert-TextContains "docs/legal-and-licensing.md" "Foundation" "legal dependency docs"
Assert-TextContains "docs/legal-and-licensing.md" "Jolt Physics" "legal dependency docs"
Assert-TextContains "docs/legal-and-licensing.md" "ENet" "legal dependency docs"
Assert-TextContains "CMakePresets.json" "desktop-runtime" "CMake presets"
Assert-TextContains "CMakePresets.json" "asset-importers" "CMake presets"
Assert-TextContains "CMakePresets.json" "physics-jolt" "CMake presets"
Assert-TextContains "CMakePresets.json" "network-enet" "CMake presets"
Assert-TextContains "tools/validate-physics-jolt.ps1" "physics-jolt" "Jolt validation wrapper"
Assert-TextContains "tools/validate-physics-jolt.ps1" "validate-installed-sdk.ps1" "Jolt validation wrapper"
Assert-TextContains "tools/validate-network-enet.ps1" "network-enet" "ENet validation wrapper"
Assert-TextContains "tools/validate-network-enet.ps1" "validate-installed-sdk.ps1" "ENet validation wrapper"
Assert-TextContains "tools/validate-network-enet.ps1" "mirakana_rhi_d3d12" "ENet validation wrapper install target closure"
Assert-TextContains "tools/validate-network-enet.ps1" "MK_editor_core" "ENet validation wrapper install target closure"
Assert-TextContains "tools/check-native-desktop-contracts.ps1" "IAudioClient" "native desktop public API guard"
Assert-TextContains "tools/check-native-desktop-contracts.ps1" "XINPUT_STATE" "native desktop public API guard"
Assert-TextContains "tools/check-native-desktop-contracts.ps1" "SDL3/" "native desktop public API guard"
Assert-TextContains "tools/check-native-desktop-contracts.ps1" "IUnknown" "native desktop public API guard"
Assert-TextContains "tools/check-native-desktop-contracts.ps1" "editor/include" "native desktop public API guard"
Assert-TextContains "tools/validate.ps1" "check-native-desktop-contracts.ps1" "validation static tasks"
Assert-TextContains "engine/agent/manifest.json" "nativeDesktopContractCheck" "engine manifest commands"
Assert-TextContains "engine/runtime/network/enet/CMakeLists.txt" "winmm" "ENet Windows SDK link closure"
Assert-TextContains "engine/runtime/network/enet/CMakeLists.txt" "ws2_32" "ENet Windows SDK link closure"
Assert-TextContains "engine/audio/wasapi/CMakeLists.txt" "ole32" "WASAPI Windows SDK link closure"
Assert-TextContains "docs/dependencies.md" "WASAPI" "dependency docs"
Assert-TextContains "engine/rhi/metal/CMakeLists.txt" 'find_library\(MK_APPLE_FOUNDATION_FRAMEWORK Foundation REQUIRED\)' "Metal Apple SDK linkage"
Assert-TextContains "engine/rhi/metal/CMakeLists.txt" '\$\{MK_APPLE_FOUNDATION_FRAMEWORK\}' "Metal Apple SDK linkage"

$vcpkgPresets = @($presets.configurePresets | Where-Object {
        (Test-JsonProperty -Object $_ -Property "cacheVariables") -and
        (Test-JsonProperty -Object $_.cacheVariables -Property "CMAKE_TOOLCHAIN_FILE") -and
        ([string](Get-JsonPropertyValue -Object $_.cacheVariables -Property "CMAKE_TOOLCHAIN_FILE")).Contains("external/vcpkg/scripts/buildsystems/vcpkg.cmake")
    })
if ($vcpkgPresets.Count -eq 0) {
    Write-Error "CMake presets must keep explicit vcpkg-backed configure presets for optional dependency lanes"
}
foreach ($preset in $vcpkgPresets) {
    Assert-CacheVariableEquals $preset "VCPKG_MANIFEST_INSTALL" "OFF"
    Assert-CacheVariableEquals $preset "VCPKG_INSTALLED_DIR" '${sourceDir}/vcpkg_installed'
    Assert-CacheVariableEquals $preset "VCPKG_TARGET_TRIPLET" "x64-windows"
    if (Test-JsonProperty -Object $preset.cacheVariables -Property "VCPKG_MANIFEST_FEATURES") {
        Write-Error "CMake preset '$($preset.name)' must not declare VCPKG_MANIFEST_FEATURES when VCPKG_MANIFEST_INSTALL is OFF; feature selection belongs in bootstrap-deps."
    }
}

Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=desktop-runtime" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=desktop-gui" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=asset-importers" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=physics-jolt" "bootstrap dependencies"
Assert-TextContains "tools/bootstrap-deps.ps1" "--x-feature=network-enet" "bootstrap dependencies"

Assert-TextContains "engine/agent/manifest.json" "buildAssetImporters" "engine manifest commands"
Assert-TextContains "engine/agent/manifest.json" "validatePhysicsJolt" "engine manifest commands"
Assert-TextContains "engine/agent/manifest.json" "validateNetworkEnet" "engine manifest commands"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "Mirakanai_HAS_ASSET_IMPORTERS" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(SPNG CONFIG\)" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(fastgltf CONFIG\)" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "Mirakanai_HAS_PHYSICS_JOLT" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(Jolt CONFIG\)" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "Mirakanai_HAS_NETWORK_ENET" "Mirakanai package config"
Assert-TextContains "cmake/MirakanaiConfig.cmake.in" "find_dependency\(unofficial-enet CONFIG\)" "Mirakanai package config"

Write-Host "dependency-policy-check: ok"
