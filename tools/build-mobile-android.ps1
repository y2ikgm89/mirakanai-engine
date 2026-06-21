#requires -Version 7.0
#requires -PSEdition Core

param(
    [string]$Game = "sample_headless",
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",
    [ValidateSet("arm64-v8a", "x86_64")]
    [string]$AndroidAbi = "arm64-v8a",
    [string]$ValidationLayerJniLibs = ""
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$gameManifest = Join-Path $root "games\$Game\game.agent.json"
if (-not (Test-Path $gameManifest)) {
    Write-Error "Game manifest not found for Android packaging: games/$Game/game.agent.json"
}

function Set-EnvironmentFromAnyScope {
    param([Parameter(Mandatory = $true)][string]$Name)

    if ([string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($Name))) {
        $value = Get-EnvironmentVariableAnyScope $Name
        if (-not [string]::IsNullOrWhiteSpace($value)) {
            [Environment]::SetEnvironmentVariable($Name, $value, "Process")
        }
    }
}

Set-EnvironmentFromAnyScope "ANDROID_HOME"
Set-EnvironmentFromAnyScope "ANDROID_SDK_ROOT"
Set-EnvironmentFromAnyScope "JAVA_HOME"

if ($Configuration -eq "Release") {
    Set-EnvironmentFromAnyScope "MK_ANDROID_KEYSTORE"
    Set-EnvironmentFromAnyScope "MK_ANDROID_KEYSTORE_PASSWORD"
    Set-EnvironmentFromAnyScope "MK_ANDROID_KEY_ALIAS"
    Set-EnvironmentFromAnyScope "MK_ANDROID_KEY_PASSWORD"

    $requiredSigningEnvironment = @(
        "MK_ANDROID_KEYSTORE",
        "MK_ANDROID_KEYSTORE_PASSWORD",
        "MK_ANDROID_KEY_ALIAS",
        "MK_ANDROID_KEY_PASSWORD"
    )
    $missingSigningEnvironment = $requiredSigningEnvironment | Where-Object {
        [string]::IsNullOrWhiteSpace([Environment]::GetEnvironmentVariable($_))
    }
    if ($missingSigningEnvironment.Count -gt 0) {
        Write-Error "Android Release packaging requires signing environment variables: $($missingSigningEnvironment -join ', ')"
    }

    if (-not (Test-Path $env:MK_ANDROID_KEYSTORE)) {
        Write-Error "MK_ANDROID_KEYSTORE does not point to an existing keystore file."
    }
}

$gradleArguments = @(":app:assemble$Configuration", "-Pmirakanai.game=$Game", "-Pmirakanai.androidAbi=$AndroidAbi")
if (-not [string]::IsNullOrWhiteSpace($ValidationLayerJniLibs)) {
    $validationLayerRoot = $ValidationLayerJniLibs
    if (-not [System.IO.Path]::IsPathRooted($validationLayerRoot)) {
        $validationLayerRoot = Join-Path $root $validationLayerRoot
    }
    if (-not (Test-Path -LiteralPath $validationLayerRoot -PathType Container)) {
        Write-Error "ValidationLayerJniLibs does not point to an existing directory: $ValidationLayerJniLibs"
    }
    $validationLayerRoot = (Resolve-Path -LiteralPath $validationLayerRoot).Path
    $requiredValidationLayer = Join-Path $validationLayerRoot (Join-Path $AndroidAbi "libVkLayer_khronos_validation.so")
    if (-not (Test-Path -LiteralPath $requiredValidationLayer -PathType Leaf)) {
        Write-Error "ValidationLayerJniLibs must contain $AndroidAbi/libVkLayer_khronos_validation.so for the Android package ABI."
    }
    $gradleArguments += "-Pmirakanai.validationLayerJniLibs=$validationLayerRoot"
}

& (Join-Path $PSScriptRoot "check-mobile-packaging.ps1") -RequireAndroid

$gradle = Find-CommandOnCombinedPath "gradle"
if (-not $gradle) {
    Write-Error "Gradle is required for Android packaging."
}

$androidRoot = Join-Path $root "platform\android"
Push-Location $androidRoot
try {
    Invoke-CheckedCommand $gradle @gradleArguments
}
finally {
    Pop-Location
}
