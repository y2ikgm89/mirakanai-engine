#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot

function Assert-TextContains([string]$relativePath, [string]$pattern, [string]$label) {
    $path = Join-Path $root $relativePath
    if (-not (Test-Path $path)) {
        Write-Error "Missing file for C++ standard policy check: $relativePath"
    }
    $content = Get-Content -LiteralPath $path -Raw
    if ($content -notmatch $pattern) {
        Write-Error "$label missing expected text pattern: $pattern"
    }
}

$presets = Read-CMakePresets
$manifest = Read-Json "engine/agent/manifest.json"
$allowedMsvcCxx23Options = @("/std:c++23preview", "/std:c++23")
$expectedMsvcCxx23Option = $null

function Assert-MsvcCxx23Option([string]$value, [string]$label) {
    if ($allowedMsvcCxx23Options -notcontains $value) {
        Write-Error "$label must set MK_MSVC_CXX23_STANDARD_OPTION to /std:c++23preview or /std:c++23"
    }

    if ($script:expectedMsvcCxx23Option -eq $null) {
        $script:expectedMsvcCxx23Option = $value
    } elseif ($script:expectedMsvcCxx23Option -ne $value) {
        Write-Error "$label must use the same MK_MSVC_CXX23_STANDARD_OPTION as other presets"
    }
}

Assert-TextContains "CMakeLists.txt" 'MK_CXX_STANDARD' "root CMake"
Assert-TextContains "CMakeLists.txt" 'cmake_minimum_required\(VERSION 3\.30\)' "root CMake"
Assert-TextContains "CMakeLists.txt" 'cxx_std_\$\{MK_CXX_STANDARD\}' "root CMake"
Assert-TextContains "CMakeLists.txt" 'MK_CXX_STANDARD "23"' "root CMake"
Assert-TextContains "CMakeLists.txt" 'MK_CXX_STANDARD must be 23' "root CMake"
Assert-TextContains "CMakeLists.txt" 'MK_MSVC_CXX23_STANDARD_OPTION' "root CMake"
Assert-TextContains "CMakeLists.txt" '/std:c\+\+23preview' "root CMake"
Assert-TextContains "CMakeLists.txt" '/std:c\+\+23' "root CMake"
Assert-TextContains "CMakeLists.txt" 'MK_ENABLE_CXX_MODULE_SCANNING' "root CMake"
Assert-TextContains "CMakeLists.txt" 'CMAKE_CXX_SCAN_FOR_MODULES ON' "root CMake"
Assert-TextContains "CMakeLists.txt" 'MK_ENABLE_IMPORT_STD' "root CMake"
Assert-TextContains "CMakeLists.txt" 'CXX_MODULE_STD' "root CMake"
Assert-TextContains "CMakeLists.txt" 'CXX_EXTENSIONS OFF' "root CMake"
Assert-TextContains "CMakeLists.txt" '/EHsc' "root CMake"
Assert-TextContains "CMakeLists.txt" 'COMPATIBILITY ExactVersion' "root CMake"
Assert-TextContains "CMakeLists.txt" 'BUNDLE DESTINATION \$\{CMAKE_INSTALL_BINDIR\}' "root CMake"
Assert-TextContains "tools/build-mobile-apple.ps1" '-DMK_ENABLE_CXX_MODULE_SCANNING=OFF' "Apple mobile packaging script"
Assert-TextContains "tools/build-mobile-apple.ps1" '-DMK_ENABLE_IMPORT_STD=OFF' "Apple mobile packaging script"
Assert-TextContains "tools/build-mobile-apple.ps1" '-DBUILD_TESTING=OFF' "Apple mobile packaging script"
Assert-TextContains "tools/build-mobile-apple.ps1" '--target "MirakanaiIOS"' "Apple mobile packaging script"
Assert-TextContains ".clang-format" 'Standard:\s+(Latest|c\+\+23)' "clang-format C++ parser standard"
Assert-TextContains "tools/validate.ps1" 'check-generated-msvc-cxx23-mode.ps1' "validation script"

if ($presets.cmakeMinimumRequired.major -lt 3 -or
    ($presets.cmakeMinimumRequired.major -eq 3 -and $presets.cmakeMinimumRequired.minor -lt 30)) {
    Write-Error "CMakePresets.json cmakeMinimumRequired must be at least 3.30 for CMAKE_CXX_MODULE_STD and module scanning policy"
}

foreach ($presetName in @("dev", "desktop-runtime", "desktop-gui", "asset-importers", "release", "desktop-runtime-release", "clang-asan-ubsan", "ci-linux-clang")) {
    $configurePreset = $presets.configurePresets | Where-Object { $_.name -eq $presetName } | Select-Object -First 1
    if (-not $configurePreset) {
        Write-Error "CMakePresets.json missing $presetName configure preset"
    }
    if ($configurePreset.cacheVariables.MK_CXX_STANDARD -ne "23") {
        Write-Error "$presetName configure preset must set MK_CXX_STANDARD to 23"
    }
    Assert-MsvcCxx23Option $configurePreset.cacheVariables.MK_MSVC_CXX23_STANDARD_OPTION "$presetName configure preset"
    if ($configurePreset.cacheVariables.MK_ENABLE_CXX_MODULE_SCANNING -ne "ON") {
        Write-Error "$presetName configure preset must set MK_ENABLE_CXX_MODULE_SCANNING to ON"
    }
    if ($configurePreset.cacheVariables.MK_ENABLE_IMPORT_STD -ne "ON") {
        Write-Error "$presetName configure preset must set MK_ENABLE_IMPORT_STD to ON"
    }
    if ($presetName -eq "clang-asan-ubsan") {
        if ($configurePreset.cacheVariables.MK_ENABLE_SANITIZERS -ne "ON") {
            Write-Error "clang-asan-ubsan configure preset must set MK_ENABLE_SANITIZERS to ON"
        }
    }
}

foreach ($presetName in @("ci-linux-tidy", "coverage", "ci-macos-appleclang")) {
    $configurePreset = $presets.configurePresets | Where-Object { $_.name -eq $presetName } | Select-Object -First 1
    if (-not $configurePreset) {
        Write-Error "CMakePresets.json missing $presetName configure preset"
    }
    if ($configurePreset.cacheVariables.MK_CXX_STANDARD -ne "23") {
        Write-Error "$presetName configure preset must set MK_CXX_STANDARD to 23"
    }
    Assert-MsvcCxx23Option $configurePreset.cacheVariables.MK_MSVC_CXX23_STANDARD_OPTION "$presetName configure preset"
    if ($configurePreset.cacheVariables.MK_ENABLE_CXX_MODULE_SCANNING -ne "OFF") {
        Write-Error "$presetName configure preset must set MK_ENABLE_CXX_MODULE_SCANNING to OFF because its CI host toolchain does not provide supported CMake module scanning."
    }
    if ($configurePreset.cacheVariables.MK_ENABLE_IMPORT_STD -ne "OFF") {
        Write-Error "$presetName configure preset must set MK_ENABLE_IMPORT_STD to OFF when CMake module scanning is disabled."
    }
}

$coverageConfigure = $presets.configurePresets | Where-Object { $_.name -eq "coverage" } | Select-Object -First 1
if ($coverageConfigure.cacheVariables.CMAKE_CXX_FLAGS -ne "--coverage" -or
    $coverageConfigure.cacheVariables.CMAKE_EXE_LINKER_FLAGS -ne "--coverage") {
    Write-Error "coverage configure preset must keep GCC coverage instrumentation flags on compile and link."
}

$cpp23Configure = $presets.configurePresets | Where-Object { $_.name -eq "cpp23-eval" } | Select-Object -First 1
if (-not $cpp23Configure) {
    Write-Error "CMakePresets.json missing cpp23-eval configure preset"
}
if ($cpp23Configure.cacheVariables.MK_CXX_STANDARD -ne "23") {
    Write-Error "cpp23-eval configure preset must set MK_CXX_STANDARD to 23"
}
Assert-MsvcCxx23Option $cpp23Configure.cacheVariables.MK_MSVC_CXX23_STANDARD_OPTION "cpp23-eval configure preset"
if ($cpp23Configure.cacheVariables.MK_ENABLE_CXX_MODULE_SCANNING -ne "ON") {
    Write-Error "cpp23-eval configure preset must set MK_ENABLE_CXX_MODULE_SCANNING to ON"
}
if ($cpp23Configure.cacheVariables.MK_ENABLE_IMPORT_STD -ne "ON") {
    Write-Error "cpp23-eval configure preset must set MK_ENABLE_IMPORT_STD to ON"
}

foreach ($presetKind in @("buildPresets", "testPresets")) {
    $preset = $presets.$presetKind | Where-Object { $_.name -eq "cpp23-eval" } | Select-Object -First 1
    if (-not $preset) {
        Write-Error "CMakePresets.json missing cpp23-eval in $presetKind"
    }
}

foreach ($presetName in @("ci-linux-clang", "ci-linux-tidy", "coverage", "ci-macos-appleclang")) {
    foreach ($presetKind in @("buildPresets", "testPresets")) {
        $preset = $presets.$presetKind | Where-Object { $_.name -eq $presetName } | Select-Object -First 1
        if (-not $preset) {
            Write-Error "CMakePresets.json missing $presetName in $presetKind"
        }
    }
}

$cpp23ReleaseConfigure = $presets.configurePresets | Where-Object { $_.name -eq "cpp23-release-eval" } | Select-Object -First 1
if (-not $cpp23ReleaseConfigure) {
    Write-Error "CMakePresets.json missing cpp23-release-eval configure preset"
}
if ($cpp23ReleaseConfigure.cacheVariables.MK_CXX_STANDARD -ne "23") {
    Write-Error "cpp23-release-eval configure preset must set MK_CXX_STANDARD to 23"
}
Assert-MsvcCxx23Option $cpp23ReleaseConfigure.cacheVariables.MK_MSVC_CXX23_STANDARD_OPTION "cpp23-release-eval configure preset"
if ($cpp23ReleaseConfigure.cacheVariables.MK_ENABLE_CXX_MODULE_SCANNING -ne "ON") {
    Write-Error "cpp23-release-eval configure preset must set MK_ENABLE_CXX_MODULE_SCANNING to ON"
}
if ($cpp23ReleaseConfigure.cacheVariables.MK_ENABLE_IMPORT_STD -ne "ON") {
    Write-Error "cpp23-release-eval configure preset must set MK_ENABLE_IMPORT_STD to ON"
}

foreach ($presetKind in @("buildPresets", "testPresets", "packagePresets")) {
    $preset = $presets.$presetKind | Where-Object { $_.name -eq "cpp23-release-eval" } | Select-Object -First 1
    if (-not $preset) {
        Write-Error "CMakePresets.json missing cpp23-release-eval in $presetKind"
    }
}

if (-not $manifest.commands.PSObject.Properties.Name.Contains("evaluateCpp23")) {
    Write-Error "engine manifest must expose commands.evaluateCpp23"
}
if ($manifest.engine.language -ne "C++23") {
    Write-Error "engine manifest must declare C++23"
}
if ($manifest.engine.languagePolicy.requiredStandard -ne "C++23") {
    Write-Error "engine manifest languagePolicy must require C++23"
}
if ($manifest.engine.languagePolicy.msvcMode -ne $expectedMsvcCxx23Option) {
    Write-Error "engine manifest languagePolicy.msvcMode must match MK_MSVC_CXX23_STANDARD_OPTION ($expectedMsvcCxx23Option)"
}
if ($manifest.engine.languagePolicy.msvcModeCacheVariable -ne "MK_MSVC_CXX23_STANDARD_OPTION") {
    Write-Error "engine manifest languagePolicy must document MK_MSVC_CXX23_STANDARD_OPTION"
}

Assert-TextContains "docs/cpp-standard.md" 'C\+\+23' "C++ standard policy doc"
Assert-TextContains "docs/cpp-standard.md" 'Recommendation' "C++ standard policy doc"
Assert-TextContains "docs/cpp-standard.md" 'LanguageStandard' "C++ standard policy doc"
Assert-TextContains "docs/cpp-standard.md" 'stdcpp23preview' "C++ standard policy doc"
Assert-TextContains "docs/cpp-standard.md" 'stdcpp23' "C++ standard policy doc"
Assert-TextContains "docs/cpp-standard.md" 'stdcpplatest' "C++ standard policy doc"

Assert-TextContains "tools/coverage-thresholds.json" 'minLineCoveragePercent' "coverage thresholds policy file"

Write-Host "cpp-standard-policy-check: ok"
