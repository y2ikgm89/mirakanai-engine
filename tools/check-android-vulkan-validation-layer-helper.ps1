#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-LinePresent {
    param(
        [Parameter(Mandatory = $true)][string[]]$Lines,
        [Parameter(Mandatory = $true)][string]$ExpectedLine,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Lines.Contains($ExpectedLine)) {
        Write-Error "$Context missing expected line: $ExpectedLine"
    }
}

function ConvertTo-LocalPath {
    param([Parameter(Mandatory = $true)][string]$RelativePath)

    return Join-Path $root ($RelativePath -replace "/", [System.IO.Path]::DirectorySeparatorChar)
}

$helperScript = Join-Path $root "tools/prepare-android-vulkan-validation-layers.ps1"
if (-not (Test-Path -LiteralPath $helperScript -PathType Leaf)) {
    Write-Error "tools/prepare-android-vulkan-validation-layers.ps1 must exist for Android Vulkan validation layer host artifact preparation"
}

$fixtureRootRelative = "out/android-vulkan-validation-layer-helper-contract/$PID"
$sourceRootRelative = "$fixtureRootRelative/release/android-binaries-1.4.350.1"
$outputRootRelative = "$fixtureRootRelative/prepared-jniLibs"
foreach ($abi in @("arm64-v8a", "x86_64")) {
    $abiDirectory = ConvertTo-LocalPath "$sourceRootRelative/jniLibs/$abi"
    $null = New-Item -ItemType Directory -Path $abiDirectory -Force
    Set-Content -LiteralPath (Join-Path $abiDirectory "libVkLayer_khronos_validation.so") -Value "synthetic $abi validation layer" -Encoding utf8NoBOM
}

$planLines = @(& $helperScript `
        -SourcePath $sourceRootRelative `
        -OutputJniLibs $outputRootRelative `
        -AndroidAbi "arm64-v8a", "x86_64" `
        -NoWrite)

Assert-LinePresent $planLines "android_validation_layer_helper_source_ready=1" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_required_abi_count=2" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_ready_abi_count=2" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_arm64_v8a_ready=1" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_x86_64_ready=1" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_writes_jni_libs=0" "validation layer helper plan"
Assert-LinePresent $planLines "android_validation_layer_helper_repository_mutation=0" "validation layer helper plan"

$importLines = @(& $helperScript `
        -SourcePath $sourceRootRelative `
        -OutputJniLibs $outputRootRelative `
        -AndroidAbi "arm64-v8a", "x86_64")

Assert-LinePresent $importLines "android_validation_layer_helper_source_ready=1" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_required_abi_count=2" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_ready_abi_count=2" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_arm64_v8a_ready=1" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_x86_64_ready=1" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_writes_jni_libs=1" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_output_ready=1" "validation layer helper import"
Assert-LinePresent $importLines "android_validation_layer_helper_repository_mutation=0" "validation layer helper import"

foreach ($abi in @("arm64-v8a", "x86_64")) {
    $preparedLayer = ConvertTo-LocalPath "$outputRootRelative/$abi/libVkLayer_khronos_validation.so"
    if (-not (Test-Path -LiteralPath $preparedLayer -PathType Leaf)) {
        Write-Error "validation layer helper did not prepare $abi/libVkLayer_khronos_validation.so"
    }
}

Write-Information "android-vulkan-validation-layer-helper-check: ok" -InformationAction Continue
