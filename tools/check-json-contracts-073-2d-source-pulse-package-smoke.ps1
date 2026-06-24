#requires -Version 7.0
#requires -PSEdition Core

# Chapter 7.3 for check-json-contracts.ps1 2D Source Pulse package smoke manifest contracts.

function Assert-JsonArrayContainsAll {
    param(
        [Parameter(Mandatory = $true)][object[]]$Actual,
        [Parameter(Mandatory = $true)][string[]]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($needle in $Expected) {
        if (@($Actual) -notcontains $needle) {
            Write-Error "$Label must contain $needle"
        }
    }
}

$engine = Read-Json "engine/agent/manifest.json"
$productionLoop = $engine.aiOperableProductionLoop
if ($null -eq $productionLoop.PSObject.Properties["original2dCommercialAuthoringLiveIterationEvidence"]) {
    Write-Error "engine manifest aiOperableProductionLoop must retain original2dCommercialAuthoringLiveIterationEvidence"
}

$productionEvidence = [string]$productionLoop.original2dCommercialAuthoringLiveIterationEvidence
foreach ($needle in @(
        "installed-2d-source-pulse-smoke",
        "--require-2d-source-pulse",
        "2d_source_pulse_status=ready",
        "2d_source_pulse_ready=1",
        "runtime scene validation",
        "operator safe-point review",
        "zero editor-core execution",
        "zero editor-core execution, arbitrary shell execution, package script execution, native handle exposure",
        "external engine schema import",
        "Unity/Unreal/Godot project/schema/code/sample/asset/UI compatibility"
    )) {
    if (-not $productionEvidence.Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop Source Pulse evidence must describe: $needle"
    }
}

$gameGuidance = $engine.gameCodeGuidance
if ($null -eq $gameGuidance.PSObject.Properties["current2dSourcePulsePackageSmoke"]) {
    Write-Error "engine manifest gameCodeGuidance must retain current2dSourcePulsePackageSmoke"
}

$sourcePulseGuidance = [string]$gameGuidance.current2dSourcePulsePackageSmoke
foreach ($needle in @(
        "installed-2d-source-pulse-smoke",
        "--require-2d-source-pulse",
        "2d_source_pulse_event_rows=3",
        "2d_source_pulse_native_backend_rows=1",
        "2d_source_pulse_polling_fallback_rows=1",
        "2d_source_pulse_runtime_replacement_committed_rows=1",
        "2d_source_pulse_external_engine_schema_import=0",
        "2d_source_pulse_external_engine_asset_use=0",
        "2d_source_pulse_external_engine_code_use=0",
        "first-party MIRAIKANAI 2D authoring/live-iteration workflow",
        "does not import Unity, Unreal Engine, or Godot projects",
        "Legal readiness is an engineering evidence gate"
    )) {
    if (-not $sourcePulseGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance Source Pulse package smoke must describe: $needle"
    }
}

$game = Read-Json "games/sample_2d_desktop_runtime_package/game.agent.json"
Assert-JsonArrayContainsAll @($game.aiWorkflow.gameDesignSpec.validationRecipeIds) @(
    "installed-2d-package-playtest-productization-smoke",
    "installed-2d-source-pulse-smoke"
) "sample 2D aiWorkflow.gameDesignSpec.validationRecipeIds"

$sourcePulseValidationRecipe = @($game.validationRecipes | Where-Object { [string]$_.name -eq "installed-2d-source-pulse-smoke" })
if ($sourcePulseValidationRecipe.Count -ne 1) {
    Write-Error "sample 2D game manifest must declare exactly one installed-2d-source-pulse-smoke validation recipe"
}
if (-not ([string]$sourcePulseValidationRecipe[0].command).Contains("--require-2d-source-pulse")) {
    Write-Error "installed-2d-source-pulse-smoke command must require --require-2d-source-pulse"
}

$qualityGate = @($game.aiWorkflow.gameDesignSpec.qualityGates | Where-Object { [string]$_.id -eq "source-pulse-package-smoke" })
if ($qualityGate.Count -ne 1) {
    Write-Error "sample 2D game manifest qualityGates must declare source-pulse-package-smoke"
}
Assert-JsonArrayContainsAll @($qualityGate[0].recipeIds) @("installed-2d-source-pulse-smoke") "source-pulse-package-smoke recipeIds"

$playtestLoop = $game.aiWorkflow.generatedGamePlaytestLoop
$selectedRecipe = @($playtestLoop.selectedRecipes | Where-Object { [string]$_.id -eq "installed-2d-source-pulse-smoke-playtest" })
if ($selectedRecipe.Count -ne 1) {
    Write-Error "sample 2D generatedGamePlaytestLoop must declare exactly one installed-2d-source-pulse-smoke-playtest selected recipe"
}

$selectedRecipe = $selectedRecipe[0]
foreach ($pair in @(
        @{ Name = "validationRecipeId"; Value = "installed-2d-source-pulse-smoke" },
        @{ Name = "reviewedRecipeSurfaceId"; Value = "package-smoke-evidence-review" },
        @{ Name = "evidenceKind"; Value = "package-smoke-counter-import" },
        @{ Name = "runtimeHostLaunchRowId"; Value = "desktop-game-runtime-playtest" },
        @{ Name = "hotReloadSafePointEvidenceId"; Value = "packaged-2d-residency-budget" },
        @{ Name = "hostGatePolicy"; Value = "respect-manifest-host-gates" }
    )) {
    $actualValue = $selectedRecipe.PSObject.Properties[$pair.Name].Value
    if ([string]$actualValue -ne $pair.Value) {
        Write-Error "installed-2d-source-pulse-smoke-playtest $($pair.Name) must be $($pair.Value)"
    }
}

Assert-JsonArrayContainsAll @($selectedRecipe.expectedSignals) @(
    "2d_source_pulse_status=ready",
    "2d_source_pulse_ready=1",
    "2d_source_pulse_event_rows=3",
    "2d_source_pulse_native_backend_rows=1",
    "2d_source_pulse_polling_fallback_rows=1",
    "2d_source_pulse_runtime_replacement_committed_rows=1",
    "2d_source_pulse_runtime_scene_validation_required=1",
    "2d_source_pulse_operator_safe_point_required=1",
    "2d_source_pulse_editor_core_execution=0",
    "2d_source_pulse_arbitrary_shell_execution=0",
    "2d_source_pulse_package_script_execution=0",
    "2d_source_pulse_native_handle_exposure=0",
    "2d_source_pulse_external_engine_schema_import=0",
    "2d_source_pulse_external_engine_asset_use=0",
    "2d_source_pulse_external_engine_code_use=0"
) "installed-2d-source-pulse-smoke-playtest expectedSignals"
