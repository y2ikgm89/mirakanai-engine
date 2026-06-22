#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$SourcePath,
    [string]$OutputJniLibs = "out/host-artifacts/android-validation-layers/jniLibs",
    [ValidateSet("arm64-v8a", "x86_64")]
    [string[]]$AndroidAbi = @("arm64-v8a", "x86_64"),
    [switch]$NoWrite
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function ConvertTo-CounterBit {
    param([bool]$Value)

    if ($Value) {
        return "1"
    }
    return "0"
}

function Resolve-HostPath {
    param([Parameter(Mandatory = $true)][string]$Path)

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return $Path
    }
    return Join-Path $root $Path
}

function ConvertTo-CounterNameToken {
    param([Parameter(Mandatory = $true)][string]$Value)

    return $Value -replace "[^A-Za-z0-9_]", "_"
}

function Get-ValidationLayerSourceRoot {
    param([Parameter(Mandatory = $true)][string]$Path)

    $resolved = Resolve-HostPath $Path
    if (Test-Path -LiteralPath $resolved -PathType Container) {
        return (Resolve-Path -LiteralPath $resolved).Path
    }
    if (-not (Test-Path -LiteralPath $resolved -PathType Leaf)) {
        return ""
    }
    if ([System.IO.Path]::GetExtension($resolved) -ne ".zip") {
        return ""
    }

    $archiveName = [System.IO.Path]::GetFileNameWithoutExtension($resolved)
    $extractRoot = Join-Path $root (Join-Path "out/host-artifacts/android-validation-layers/extracted" $archiveName)
    if (-not (Test-Path -LiteralPath $extractRoot -PathType Container)) {
        $null = New-Item -ItemType Directory -Path $extractRoot -Force
        Expand-Archive -LiteralPath $resolved -DestinationPath $extractRoot -Force
    }
    return (Resolve-Path -LiteralPath $extractRoot).Path
}

function Find-ValidationLayerLibrary {
    param(
        [Parameter(Mandatory = $true)][string]$SourceRoot,
        [Parameter(Mandatory = $true)][string]$Abi
    )

    $matchingLayers = @(Get-ChildItem -LiteralPath $SourceRoot -Recurse -File -Filter "libVkLayer_khronos_validation.so" |
        Where-Object { $_.Directory -and $_.Directory.Name -eq $Abi } |
        Sort-Object FullName)
    if ($matchingLayers.Count -eq 0) {
        return $null
    }
    return $matchingLayers[0]
}

$sourceRoot = Get-ValidationLayerSourceRoot -Path $SourcePath
$sourceReady = -not [string]::IsNullOrWhiteSpace($sourceRoot)
$outputRoot = Resolve-HostPath $OutputJniLibs
$outputRootFull = [System.IO.Path]::GetFullPath($outputRoot)
$outRootFull = [System.IO.Path]::GetFullPath((Join-Path $root "out"))
if ($outputRootFull -ne $outRootFull -and -not $outputRootFull.StartsWith($outRootFull + [System.IO.Path]::DirectorySeparatorChar, [System.StringComparison]::OrdinalIgnoreCase)) {
    Write-Error "OutputJniLibs must be under the ignored out/ tree for host-only validation layer artifacts."
}
$requiredAbi = @($AndroidAbi | Select-Object -Unique)
$foundLayers = @{}

if ($sourceReady) {
    foreach ($abi in $requiredAbi) {
        $layer = Find-ValidationLayerLibrary -SourceRoot $sourceRoot -Abi $abi
        if ($null -ne $layer) {
            $foundLayers[$abi] = $layer.FullName
        }
    }
}

Write-Output "android_validation_layer_helper_source_ready=$(ConvertTo-CounterBit $sourceReady)"
Write-Output "android_validation_layer_helper_required_abi_count=$($requiredAbi.Count)"
Write-Output "android_validation_layer_helper_ready_abi_count=$($foundLayers.Count)"
foreach ($abi in $requiredAbi) {
    $abiToken = ConvertTo-CounterNameToken $abi
    Write-Output "android_validation_layer_helper_${abiToken}_ready=$(ConvertTo-CounterBit $foundLayers.ContainsKey($abi))"
}
Write-Output "android_validation_layer_helper_writes_jni_libs=$(ConvertTo-CounterBit (-not $NoWrite))"
Write-Output "android_validation_layer_helper_repository_mutation=0"

if (-not $sourceReady) {
    Write-Error "SourcePath must be an existing directory or .zip archive containing Android Vulkan validation layer binaries."
}
if ($foundLayers.Count -ne $requiredAbi.Count) {
    $missing = @($requiredAbi | Where-Object { -not $foundLayers.ContainsKey($_) })
    Write-Error "SourcePath is missing libVkLayer_khronos_validation.so for ABI(s): $($missing -join ', ')"
}

if (-not $NoWrite) {
    foreach ($abi in $requiredAbi) {
        $targetDirectory = Join-Path $outputRoot $abi
        $null = New-Item -ItemType Directory -Path $targetDirectory -Force
        Copy-Item -LiteralPath $foundLayers[$abi] -Destination (Join-Path $targetDirectory "libVkLayer_khronos_validation.so") -Force
    }
}

$outputReady = $false
if (-not $NoWrite) {
    $outputReady = $true
    foreach ($abi in $requiredAbi) {
        $expectedLayer = Join-Path $outputRoot (Join-Path $abi "libVkLayer_khronos_validation.so")
        if (-not (Test-Path -LiteralPath $expectedLayer -PathType Leaf)) {
            $outputReady = $false
        }
    }
}
Write-Output "android_validation_layer_helper_output_ready=$(ConvertTo-CounterBit $outputReady)"
Write-Output "android_validation_layer_helper_output_jni_libs=$OutputJniLibs"
Write-Information "android-vulkan-validation-layer-helper: ok" -InformationAction Continue
