#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

function Assert-ContainsText {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$Needle,
        [Parameter(Mandatory = $true)][string]$Context
    )

    if (-not $Text.Contains($Needle)) {
        Write-Error "$Context missing: $Needle"
    }
}

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

$validatorRelativePath = "tools/validate-android-gameactivity-host.ps1"
$validatorPath = Join-Path $root $validatorRelativePath
if (-not (Test-Path -LiteralPath $validatorPath -PathType Leaf)) {
    Write-Error "$validatorRelativePath must exist for the reviewed Android GameActivity host gate."
}

$validatorText = Get-Content -LiteralPath $validatorPath -Raw
foreach ($needle in @(
        "RunPackageCommands",
        "RequireReady",
        "ExpectedEvidenceCounters",
        "check-mobile-packaging.ps1",
        "build-mobile-android.ps1",
        "check-android-release-package.ps1",
        "smoke-android-package.ps1",
        "UseLocalValidationKey",
        "StartEmulator",
        "ValidationLayerJniLibs",
        "RequirePackagedValidationLayer",
        "validation_recipe=android-gameactivity-host",
        "host_gate=android-gameactivity",
        "android_gameactivity_run_package_commands=0",
        "android_gameactivity_requires_host_operation=1",
        "android_gameactivity_host_ready=0",
        "android_gameactivity_validation_layer_jni_libs_ready=0",
        "android_gameactivity_production_signing_material=0",
        "android_gameactivity_play_upload_ready=0",
        "android_gameactivity_native_handle_access=0"
    )) {
    Assert-ContainsText $validatorText $needle $validatorRelativePath
}

foreach ($surface in @(
        @{
            Path = "tools/check-android-release-package.ps1"
            Needles = @("AndroidAbi", "ValidationLayerJniLibs", "buildArguments", "libVkLayer_khronos_validation.so")
        },
        @{
            Path = "tools/validate.ps1"
            Needles = @("check-android-gameactivity-host-validator.ps1")
        },
        @{
            Path = "tools/run-validation-recipe-plans.ps1"
            Needles = @("android-gameactivity-host", "validate-android-gameactivity-host.ps1", "ValidationLayerJniLibs", "android_gameactivity_host_ready=1", "android_gameactivity_validation_layer_jni_libs_ready=1", "android_gameactivity_play_upload_ready=0")
        },
        @{
            Path = "engine/agent/manifest.fragments/009-validationRecipes.json"
            Needles = @("android-gameactivity-host", "tools/validate-android-gameactivity-host.ps1", "ValidationLayerJniLibs", "android_gameactivity_host_ready=1", "android_gameactivity_validation_layer_jni_libs_ready=1", "android_gameactivity_play_upload_ready=0")
        },
        @{
            Path = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
            Needles = @("android-gameactivity-host", "production signing", "physical-device matrix")
        },
        @{
            Path = "engine/agent/manifest.fragments/008-packagingTargets.json"
            Needles = @("tools/validate-android-gameactivity-host.ps1", "reviewed fail-closed wrapper")
        },
        @{
            Path = "docs/agent-operational-reference.md"
            Needles = @("tools/validate-android-gameactivity-host.ps1", "-RunPackageCommands -RequireReady")
        },
        @{
            Path = "docs/testing.md"
            Needles = @("Android GameActivity host gate coverage", "android_gameactivity_host_ready=1", "production signing")
        },
        @{
            Path = "docs/current-capabilities.md"
            Needles = @("tools/validate-android-gameactivity-host.ps1", "default invocation is non-mutating")
        },
        @{
            Path = "docs/superpowers/plans/README.md"
            Needles = @("Android GameActivity host gate update", "android_gameactivity_host_ready=0", "android_gameactivity_host_ready=1")
        }
    )) {
    $surfaceText = Get-Content -LiteralPath (Join-Path $root $surface.Path) -Raw
    foreach ($needle in @($surface.Needles)) {
        Assert-ContainsText $surfaceText $needle $surface.Path
    }
}

$defaultLines = @(& $validatorPath)
Assert-LinePresent $defaultLines "validation_recipe=android-gameactivity-host" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "host_gate=android-gameactivity" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_run_package_commands=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_requires_host_operation=1" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_debug_build_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_release_package_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_release_smoke_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_host_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_validation_layer_jni_libs_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_production_signing_material=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_play_upload_ready=0" "Android GameActivity host validator default run"
Assert-LinePresent $defaultLines "android_gameactivity_native_handle_access=0" "Android GameActivity host validator default run"

Write-Information "android-gameactivity-host-validator-check: ok" -InformationAction Continue
