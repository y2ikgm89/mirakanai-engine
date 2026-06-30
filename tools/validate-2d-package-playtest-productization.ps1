#requires -Version 7.0
#requires -PSEdition Core

[CmdletBinding()]
param(
    [switch]$RequireReady
)

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
Set-Location $root

$gameManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$recipeId = "installed-2d-package-playtest-productization-smoke"
$playtestRecipeId = "installed-2d-package-playtest-productization-smoke-playtest"
$sourcePulseRecipeId = "installed-2d-source-pulse-smoke"
$sourcePulsePlaytestRecipeId = "installed-2d-source-pulse-smoke-playtest"
$runtimeHostLaunchRowId = "desktop-game-runtime-playtest"
$hotReloadSafePointEvidenceId = "packaged-2d-residency-budget"
$hotReloadExternalEvidenceId = "hot-reload-package-playtest-evidence"
$baseFailureClassifications = @(
    "missing-package-file",
    "invalid-scene-binding",
    "package-load-failure",
    "shader-tool-gap",
    "counter-mismatch",
    "hot-reload-recook-failure",
    "runtime-replacement-failure",
    "host-gated-backend"
)
$packagePlaytestFailureClassifications = @(
    $baseFailureClassifications
    "long-run-budget-exceeded",
    "retained-artifact-missing"
)
$readySmokeExpectations = @{
    "2d_package_playtest_productization_status" = "ready"
    "2d_package_playtest_productization_ready" = "1"
    "2d_package_playtest_productization_recipe_rows" = "1"
    "2d_package_playtest_productization_evidence_rows" = "1"
    "2d_package_playtest_productization_imported_evidence_rows" = "1"
    "2d_package_playtest_productization_package_smoke_counter_rows" = "16"
    "2d_package_playtest_productization_profile_artifact_rows" = "1"
    "2d_package_playtest_productization_remediation_handoff_rows" = "4"
    "2d_package_playtest_productization_failure_classification_rows" = "10"
    "2d_package_playtest_productization_diagnostics" = "0"
    "2d_package_playtest_productization_declared_validation_recipe_rows" = "1"
    "2d_package_playtest_productization_declared_generated_playtest_rows" = "1"
    "2d_package_playtest_productization_declared_runtime_host_launch_rows" = "1"
    "2d_package_playtest_productization_declared_hot_reload_safe_point_rows" = "1"
    "2d_package_playtest_productization_long_run_frames" = "3"
    "2d_package_playtest_productization_long_run_over_budget_frames" = "0"
    "2d_package_playtest_productization_retained_profile_artifact_hashes" = "1"
    "2d_package_playtest_productization_editor_core_execution" = "0"
    "2d_package_playtest_productization_validation_recipe_execution" = "0"
    "2d_package_playtest_productization_arbitrary_shell_execution" = "0"
    "2d_package_playtest_productization_active_session_hot_reload" = "0"
    "2d_package_playtest_productization_native_handle_exposure" = "0"
}
$sourcePulseSmokeExpectations = @{
    "2d_source_pulse_status" = "ready"
    "2d_source_pulse_ready" = "1"
    "2d_source_pulse_event_rows" = "3"
    "2d_source_pulse_native_backend_rows" = "1"
    "2d_source_pulse_polling_fallback_rows" = "1"
    "2d_source_pulse_runtime_replacement_committed_rows" = "1"
    "2d_source_pulse_runtime_scene_validation_required" = "1"
    "2d_source_pulse_operator_safe_point_required" = "1"
    "2d_source_pulse_editor_core_execution" = "0"
    "2d_source_pulse_arbitrary_shell_execution" = "0"
    "2d_source_pulse_package_script_execution" = "0"
    "2d_source_pulse_native_handle_exposure" = "0"
    "2d_source_pulse_external_engine_schema_import" = "0"
    "2d_source_pulse_external_engine_asset_use" = "0"
    "2d_source_pulse_external_engine_code_use" = "0"
}

function Get-ObjectPropertyValue {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string]$Name
    )

    $property = $Object.PSObject.Properties[$Name]
    if ($null -eq $property) {
        return $null
    }
    return $property.Value
}

function New-Set {
    param([Parameter(Mandatory = $true)][object[]]$Values)

    $set = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::Ordinal)
    foreach ($value in @($Values)) {
        $null = $set.Add([string]$value)
    }
    return $set
}

function Assert-ObjectProperties {
    param(
        [Parameter(Mandatory = $true)]$Object,
        [Parameter(Mandatory = $true)][string[]]$Names,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($name in $Names) {
        if ($null -eq $Object.PSObject.Properties[$name]) {
            Write-Error "$Label missing required property: $name"
        }
    }
}

function Assert-ContainsAll {
    param(
        [Parameter(Mandatory = $true)]$Set,
        [Parameter(Mandatory = $true)][string[]]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    foreach ($item in $Expected) {
        if (-not $Set.Contains($item)) {
            Write-Error "$Label missing required row: $item"
        }
    }
}

function ConvertFrom-StatusLine {
    param([Parameter(Mandatory = $true)][string[]]$Output)

    $statusLine = @($Output | Where-Object { $_ -match "^sample_2d_desktop_runtime_package status=" } | Select-Object -Last 1)
    if ($statusLine.Count -eq 0) {
        Write-Error "2d-package-playtest-productization: package smoke did not emit sample_2d_desktop_runtime_package status line"
    }

    $fields = @{}
    foreach ($match in [regex]::Matches($statusLine[0], '([A-Za-z0-9_]+)=([^ ]+)')) {
        $fields[$match.Groups[1].Value] = $match.Groups[2].Value
    }
    return $fields
}

function Assert-SmokeField {
    param(
        [Parameter(Mandatory = $true)]$Fields,
        [Parameter(Mandatory = $true)][string]$Name,
        [Parameter(Mandatory = $true)][string]$Expected
    )

    if (-not $Fields.ContainsKey($Name)) {
        Write-Error "2d-package-playtest-productization: package smoke missing counter $Name"
    }
    if ([string]$Fields[$Name] -ne $Expected) {
        Write-Error "2d-package-playtest-productization: package smoke counter $Name expected $Expected but was $($Fields[$Name])"
    }
}

$game = Read-Json -RelativePath $gameManifestPath -Root $root
$validationRecipeSet = New-Set (@($game.validationRecipes) | ForEach-Object { [string]$_.name })
if (-not $validationRecipeSet.Contains($recipeId)) {
    Write-Error "$gameManifestPath validationRecipes missing $recipeId"
}
if (-not $validationRecipeSet.Contains($sourcePulseRecipeId)) {
    Write-Error "$gameManifestPath validationRecipes missing $sourcePulseRecipeId"
}

$aiWorkflow = Get-ObjectPropertyValue -Object $game -Name "aiWorkflow"
if ($null -eq $aiWorkflow) {
    Write-Error "$gameManifestPath must declare aiWorkflow"
}

$playtestLoop = Get-ObjectPropertyValue -Object $aiWorkflow -Name "generatedGamePlaytestLoop"
if ($null -eq $playtestLoop) {
    Write-Error "$gameManifestPath aiWorkflow must declare generatedGamePlaytestLoop"
}
Assert-ObjectProperties $playtestLoop @("selectedRecipes", "failureClassifications", "unsupportedClaims") "$gameManifestPath generatedGamePlaytestLoop"

$failureClassificationSet = New-Set (@($playtestLoop.failureClassifications) | ForEach-Object { [string]$_.id })
Assert-ContainsAll $failureClassificationSet $packagePlaytestFailureClassifications "$gameManifestPath generatedGamePlaytestLoop failureClassifications"
Assert-ContainsAll (New-Set @($playtestLoop.unsupportedClaims)) @(
    "arbitrary-shell",
    "raw-manifest-command-evaluation",
    "cooked-package-mutation",
    "engine-internal-edits",
    "native-handles"
) "$gameManifestPath generatedGamePlaytestLoop unsupportedClaims"

$selectedRecipe = @($playtestLoop.selectedRecipes | Where-Object { [string]$_.id -eq $playtestRecipeId })
if ($selectedRecipe.Count -ne 1) {
    Write-Error "$gameManifestPath generatedGamePlaytestLoop selectedRecipes must declare exactly one $playtestRecipeId row"
}
$selectedRecipe = $selectedRecipe[0]
Assert-ObjectProperties $selectedRecipe @(
    "id",
    "validationRecipeId",
    "reviewedRecipeSurfaceId",
    "evidenceKind",
    "expectedSignals",
    "failureClassificationIds",
    "runtimeHostLaunchRowId",
    "hotReloadSafePointEvidenceId",
    "hostGatePolicy"
) "$gameManifestPath generatedGamePlaytestLoop selectedRecipe $playtestRecipeId"
if ([string]$selectedRecipe.validationRecipeId -ne $recipeId) {
    Write-Error "$gameManifestPath $playtestRecipeId validationRecipeId expected $recipeId but was $($selectedRecipe.validationRecipeId)"
}
if ([string]$selectedRecipe.reviewedRecipeSurfaceId -ne "package-smoke-evidence-review") {
    Write-Error "$gameManifestPath $playtestRecipeId reviewedRecipeSurfaceId must be package-smoke-evidence-review"
}
if ([string]$selectedRecipe.evidenceKind -ne "package-smoke-counter-import") {
    Write-Error "$gameManifestPath $playtestRecipeId evidenceKind must be package-smoke-counter-import"
}
if ([string]$selectedRecipe.runtimeHostLaunchRowId -ne $runtimeHostLaunchRowId) {
    Write-Error "$gameManifestPath $playtestRecipeId runtimeHostLaunchRowId expected $runtimeHostLaunchRowId"
}
if ([string]$selectedRecipe.hotReloadSafePointEvidenceId -ne $hotReloadSafePointEvidenceId) {
    Write-Error "$gameManifestPath $playtestRecipeId hotReloadSafePointEvidenceId expected $hotReloadSafePointEvidenceId"
}
Assert-ContainsAll (New-Set @($selectedRecipe.failureClassificationIds)) $packagePlaytestFailureClassifications "$gameManifestPath $playtestRecipeId failureClassificationIds"
Assert-ContainsAll (New-Set @($selectedRecipe.expectedSignals)) @(
    "2d_package_playtest_productization_status=ready",
    "2d_package_playtest_productization_ready=1",
    "2d_package_playtest_productization_editor_core_execution=0",
    "2d_package_playtest_productization_validation_recipe_execution=0",
    "2d_package_playtest_productization_arbitrary_shell_execution=0",
    "2d_package_playtest_productization_active_session_hot_reload=0",
    "2d_package_playtest_productization_native_handle_exposure=0",
    "2d_package_playtest_productization_long_run_frames=3",
    "2d_package_playtest_productization_long_run_over_budget_frames=0",
    "2d_package_playtest_productization_retained_profile_artifact_hashes=1",
    "2d_package_playtest_productization_retained_profile_artifact_hash"
) "$gameManifestPath $playtestRecipeId expectedSignals"

$sourcePulseRecipe = @($playtestLoop.selectedRecipes | Where-Object { [string]$_.id -eq $sourcePulsePlaytestRecipeId })
if ($sourcePulseRecipe.Count -ne 1) {
    Write-Error "$gameManifestPath generatedGamePlaytestLoop selectedRecipes must declare exactly one $sourcePulsePlaytestRecipeId row"
}
$sourcePulseRecipe = $sourcePulseRecipe[0]
Assert-ObjectProperties $sourcePulseRecipe @(
    "id",
    "validationRecipeId",
    "reviewedRecipeSurfaceId",
    "evidenceKind",
    "expectedSignals",
    "failureClassificationIds",
    "runtimeHostLaunchRowId",
    "hotReloadSafePointEvidenceId",
    "hostGatePolicy"
) "$gameManifestPath generatedGamePlaytestLoop selectedRecipe $sourcePulsePlaytestRecipeId"
if ([string]$sourcePulseRecipe.validationRecipeId -ne $sourcePulseRecipeId) {
    Write-Error "$gameManifestPath $sourcePulsePlaytestRecipeId validationRecipeId expected $sourcePulseRecipeId but was $($sourcePulseRecipe.validationRecipeId)"
}
if ([string]$sourcePulseRecipe.reviewedRecipeSurfaceId -ne "package-smoke-evidence-review") {
    Write-Error "$gameManifestPath $sourcePulsePlaytestRecipeId reviewedRecipeSurfaceId must be package-smoke-evidence-review"
}
if ([string]$sourcePulseRecipe.evidenceKind -ne "package-smoke-counter-import") {
    Write-Error "$gameManifestPath $sourcePulsePlaytestRecipeId evidenceKind must be package-smoke-counter-import"
}
if ([string]$sourcePulseRecipe.runtimeHostLaunchRowId -ne $runtimeHostLaunchRowId) {
    Write-Error "$gameManifestPath $sourcePulsePlaytestRecipeId runtimeHostLaunchRowId expected $runtimeHostLaunchRowId"
}
if ([string]$sourcePulseRecipe.hotReloadSafePointEvidenceId -ne $hotReloadSafePointEvidenceId) {
    Write-Error "$gameManifestPath $sourcePulsePlaytestRecipeId hotReloadSafePointEvidenceId expected $hotReloadSafePointEvidenceId"
}
Assert-ContainsAll (New-Set @($sourcePulseRecipe.failureClassificationIds)) $baseFailureClassifications "$gameManifestPath $sourcePulsePlaytestRecipeId failureClassificationIds"
Assert-ContainsAll (New-Set @($sourcePulseRecipe.expectedSignals)) @(
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
) "$gameManifestPath $sourcePulsePlaytestRecipeId expectedSignals"

$streamingTargets = @($game.packageStreamingResidencyTargets | Where-Object { [string]$_.id -eq $hotReloadSafePointEvidenceId })
if ($streamingTargets.Count -ne 1) {
    Write-Error "$gameManifestPath packageStreamingResidencyTargets must declare $hotReloadSafePointEvidenceId"
}
if ([string]$streamingTargets[0].mode -ne "host-gated-safe-point" -or -not [bool]$streamingTargets[0].safePointRequired) {
    Write-Error "$gameManifestPath $hotReloadSafePointEvidenceId must remain host-gated-safe-point with safePointRequired=true"
}

$budget = Get-ObjectPropertyValue -Object $game -Name "performanceBudgets"
if ($null -eq $budget) {
    Write-Error "$gameManifestPath must declare performanceBudgets"
}
$budgetEvidenceSet = New-Set (@($budget.evidenceRows) | ForEach-Object { [string]$_.id })
if (-not $budgetEvidenceSet.Contains($hotReloadExternalEvidenceId)) {
    Write-Error "$gameManifestPath performanceBudgets evidenceRows missing $hotReloadExternalEvidenceId"
}

if ($RequireReady.IsPresent) {
    $smokeArgs = @(
        "--smoke",
        "--max-frames",
        "3",
        "--require-config",
        "runtime/sample_2d_desktop_runtime_package.config",
        "--require-scene-package",
        "runtime/sample_2d_desktop_runtime_package.geindex",
        "--require-win32-runtime-host",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-runtime-ui-renderer-atlas-handoff",
        "--require-sandbox-package-budgets",
        "--require-performance-baseline",
        "--require-long-run-performance-readiness",
        "--require-2d-package-playtest-productization",
        "--require-2d-source-pulse"
    )

    $packageOutput = & (Join-Path $PSScriptRoot "package-desktop-runtime.ps1") `
        -GameTarget "sample_2d_desktop_runtime_package" `
        -RequireD3d12Shaders `
        -SmokeArgs $smokeArgs 2>&1
    $packageExitCode = $LASTEXITCODE
    if ($packageExitCode -ne 0) {
        Write-Error "2d-package-playtest-productization: package smoke failed with exit code $packageExitCode`n$(@($packageOutput) -join "`n")"
    }

    $installedExe = Join-Path $root "out/install/desktop-runtime-release/bin/sample_2d_desktop_runtime_package.exe"
    if (-not (Test-Path -LiteralPath $installedExe -PathType Leaf)) {
        Write-Error "2d-package-playtest-productization: installed sample executable missing after package smoke: $installedExe"
    }
    $smokeOutput = & $installedExe @smokeArgs 2>&1
    $smokeExitCode = $LASTEXITCODE
    if ($smokeExitCode -ne 0) {
        Write-Error "2d-package-playtest-productization: installed package smoke failed with exit code $smokeExitCode`n$(@($smokeOutput) -join "`n")"
    }

    $fields = ConvertFrom-StatusLine -Output @($smokeOutput)
    foreach ($entry in $readySmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
    foreach ($entry in $sourcePulseSmokeExpectations.GetEnumerator()) {
        Assert-SmokeField $fields $entry.Key $entry.Value
    }
    foreach ($positiveCounter in @(
            "2d_package_playtest_productization_long_run_memory_high_water_bytes",
            "2d_package_playtest_productization_retained_profile_artifact_hash"
        )) {
        if (-not $fields.ContainsKey($positiveCounter) -or [decimal]$fields[$positiveCounter] -le 0) {
            Write-Error "2d-package-playtest-productization: package smoke counter $positiveCounter must be positive"
        }
    }
}

Write-Information "2d-package-playtest-productization: ok" -InformationAction Continue
