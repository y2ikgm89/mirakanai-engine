#requires -Version 7.0
#requires -PSEdition Core

$ErrorActionPreference = "Stop"

. (Join-Path $PSScriptRoot "common.ps1")

$root = Get-RepoRoot
$runner = Join-Path $PSScriptRoot "run-validation-recipe.ps1"
$recipeCore = Join-Path $PSScriptRoot "validation-recipe-core.ps1"

if (-not (Test-Path -LiteralPath $runner -PathType Leaf)) {
    Write-Error "Missing validation recipe runner: tools/run-validation-recipe.ps1"
}
if (-not (Test-Path -LiteralPath $recipeCore -PathType Leaf)) {
    Write-Error "Missing validation recipe core helper: tools/validation-recipe-core.ps1"
}

. $recipeCore
. (Join-Path $PSScriptRoot "run-validation-recipe-plans.ps1")

function ConvertTo-RunnerResultObject {
    param(
        [Parameter(Mandatory = $true)]
        $Result
    )

    return $Result | ConvertTo-Json -Depth 12 | ConvertFrom-Json
}

function Invoke-RunnerJson {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [int]$ExpectedExitCode = 0
    )

    $mode = "DryRun"
    $recipe = ""
    $gameTarget = ""
    $strictBackend = ""
    $hostGateAcknowledgements = @()
    $timeoutSeconds = 0
    $remainingArguments = @()

    for ($index = 0; $index -lt $Arguments.Count; $index++) {
        $argument = $Arguments[$index]
        switch ($argument) {
            "-Mode" {
                $index++
                $mode = $Arguments[$index]
            }
            "-Recipe" {
                $index++
                $recipe = $Arguments[$index]
            }
            "-GameTarget" {
                $index++
                $gameTarget = $Arguments[$index]
            }
            "-StrictBackend" {
                $index++
                $strictBackend = $Arguments[$index]
            }
            "-HostGateAcknowledgements" {
                $index++
                $hostGateAcknowledgements += $Arguments[$index]
            }
            "-TimeoutSeconds" {
                $index++
                $timeoutSeconds = [int]$Arguments[$index]
            }
            "-RemainingArguments" {
                $index++
                $remainingArguments += $Arguments[$index]
            }
            default {
                $remainingArguments += $argument
            }
        }
    }

    $exitCode = 0
    if ([string]::IsNullOrWhiteSpace($recipe)) {
        $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName "" -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "missing-recipe" -Message "Recipe is required.")
        $exitCode = 2
    } else {
        $plan = Get-ValidationRecipeCommandPlan -RecipeName $recipe -SelectedGameTarget $gameTarget -SelectedStrictBackend $strictBackend
        if ($null -eq $plan) {
            $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName $recipe -Diagnostic (New-RunnerDiagnostic -Severity "error" -Code "unknown-recipe" -Message "Validation recipe '$recipe' is not in the reviewed run-validation-recipe allowlist." -ValidationRecipe $recipe)
            $exitCode = 2
        } else {
            $diagnostic = Test-ValidationRecipeRequest `
                -Plan $plan `
                -Mode $mode `
                -GameTarget $gameTarget `
                -StrictBackend $strictBackend `
                -HostGateAcknowledgements $hostGateAcknowledgements `
                -RemainingArguments $remainingArguments `
                -TimeoutSeconds $timeoutSeconds
            if ($null -ne $diagnostic) {
                $result = New-ValidationRecipeRejectedResult -Mode $mode -RecipeName $recipe -Diagnostic $diagnostic
                $result.hostGates = @($plan.hostGates)
                $result.validationRecipes = @($recipe)
                $exitCode = 2
            } else {
                $result = New-ValidationRecipeDryRunResult -Mode $mode -Plan $plan
            }
        }
    }

    if ($exitCode -ne $ExpectedExitCode) {
        Write-Error "run-validation-recipe.ps1 $($Arguments -join ' ') exited $exitCode, expected $ExpectedExitCode."
    }

    return ConvertTo-RunnerResultObject -Result $result
}

function Assert-RunnerCliSmoke {
    $output = @(& pwsh -NoProfile -ExecutionPolicy Bypass -File $runner -Mode DryRun -Recipe "agent-contract" 2>&1)
    $exitCode = if ($null -eq $global:LASTEXITCODE) { 0 } else { $global:LASTEXITCODE }
    if ($exitCode -ne 0) {
        Write-Error "run-validation-recipe.ps1 CLI smoke exited $exitCode. Output: $($output -join "`n")"
    }

    try {
        $result = (($output | Where-Object { -not [string]::IsNullOrWhiteSpace([string]$_) }) -join "`n") | ConvertFrom-Json
    } catch {
        Write-Error "run-validation-recipe.ps1 CLI smoke did not return JSON. Output: $($output -join "`n")"
    }

    if ($result.status -ne "dry-run" -or $result.recipe -ne "agent-contract") {
        Write-Error "run-validation-recipe.ps1 CLI smoke returned unexpected result."
    }
    Assert-ArgvHasScriptSuffix -Result $result -ScriptSuffix "tools/check-ai-integration.ps1" -Label "CLI smoke argv for agent-contract"
}

function Assert-HasProperty($object, [string]$Property, [string]$Label) {
    if (-not $object.PSObject.Properties.Name.Contains($Property)) {
        Write-Error "$Label missing property: $Property"
    }
}

function Assert-ArrayContains($array, [string]$Expected, [string]$Label) {
    if (@($array) -notcontains $Expected) {
        Write-Error "$Label missing expected value: $Expected"
    }
}

function Assert-ArgvHasScriptSuffix {
    param(
        [Parameter(Mandatory = $true)][object]$Result,
        [Parameter(Mandatory = $true)][string]$ScriptSuffix,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $normalizedSuffix = $ScriptSuffix.Replace('\', '/').TrimEnd('/')
    $hit = $false
    foreach ($a in @($Result.argv)) {
        $s = ([string]$a).Replace('\', '/')
        if ($s.EndsWith($normalizedSuffix, [System.StringComparison]::OrdinalIgnoreCase)) {
            $hit = $true
            break
        }
        if ($s -eq $ScriptSuffix -or $s -eq $normalizedSuffix) {
            $hit = $true
            break
        }
    }
    if (-not $hit) {
        Write-Error "$Label missing argv entry ending with: $ScriptSuffix"
    }
}

function Assert-ArgvContainsText {
    param(
        [Parameter(Mandatory = $true)][object]$Result,
        [Parameter(Mandatory = $true)][string]$Expected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $joined = [string]::Join("`n", @($Result.argv | ForEach-Object { [string]$_ }))
    if (-not $joined.Contains($Expected)) {
        Write-Error "$Label missing expected text: $Expected"
    }
}

function Assert-ArgvDoesNotContainText {
    param(
        [Parameter(Mandatory = $true)][object]$Result,
        [Parameter(Mandatory = $true)][string]$Unexpected,
        [Parameter(Mandatory = $true)][string]$Label
    )

    $joined = [string]::Join("`n", @($Result.argv | ForEach-Object { [string]$_ }))
    if ($joined.Contains($Unexpected)) {
        Write-Error "$Label must not contain text: $Unexpected"
    }
}

function Assert-HighestCommercialSkeletonDryRun {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,

        [Parameter(Mandatory = $true)]
        [string]$HostGate,

        [Parameter(Mandatory = $true)]
        [string]$ReadyCounter
    )

    $result = Assert-DryRunRecipe -Recipe $Recipe -ExpectedArgv @("-Command")
    Assert-ArrayContains $result.hostGates $HostGate "dry-run host gates for $Recipe"
    foreach ($needle in @($Recipe, "validation_recipe_skeleton=1", "ready_claim=0", "$ReadyCounter=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run skeleton row for $Recipe"
    }
    foreach ($needle in @("tools/package-desktop-runtime.ps1", "validate-linux-vulkan-runtime-host.ps1", "validate-android-vulkan-runtime-host.ps1", "validate-apple-metal-platform-host.ps1", "validate-environment-optimization-artifacts.ps1", "validate-environment-weather-physics.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run skeleton row for $Recipe"
    }

    return $result
}

function Assert-EnvironmentPlatformVulkanHostGateDryRun {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,

        [Parameter(Mandatory = $true)]
        [string]$ScriptSuffix,

        [Parameter(Mandatory = $true)]
        [string]$HostGate,

        [Parameter(Mandatory = $true)]
        [string[]]$ExpectedNeedles
    )

    $result = Assert-DryRunRecipe -Recipe $Recipe -ExpectedArgv @("-File", $ScriptSuffix, "-RequireReady")
    Assert-ArrayContains $result.hostGates $HostGate "dry-run host gates for $Recipe"
    foreach ($needle in $ExpectedNeedles) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run platform Vulkan row for $Recipe"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run platform Vulkan row for $Recipe"
    }

    return $result
}

function Assert-EnvironmentPlatformHostGateDryRun {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,

        [Parameter(Mandatory = $true)]
        [string]$ScriptSuffix,

        [Parameter(Mandatory = $true)]
        [string]$HostGate,

        [Parameter(Mandatory = $true)]
        [string[]]$ExpectedNeedles
    )

    $result = Assert-DryRunRecipe -Recipe $Recipe -ExpectedArgv @("-File", $ScriptSuffix, "-RequireReady")
    Assert-ArrayContains $result.hostGates $HostGate "dry-run host gates for $Recipe"
    foreach ($needle in $ExpectedNeedles) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run platform row for $Recipe"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run platform row for $Recipe"
    }

    return $result
}

function Assert-EnvironmentOptimizationArtifactDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-broad-optimization-cross-backend-measurement" -ExpectedArgv @("-File", "tools/validate-environment-optimization-artifacts.ps1", "-RequireReady")
    Assert-ArrayContains $result.hostGates "environment-optimization-artifact-host" "dry-run host gates for environment-broad-optimization-cross-backend-measurement"
    foreach ($needle in @(
            "environment_optimization_measurement_workload_rows=21",
            "environment_optimization_measurement_backend_rows=3",
            "environment_optimization_measurement_before_after_pairs=21",
            "environment_optimization_measurement_profiler_artifacts=21",
            "environment_optimization_measurement_missing_artifacts",
            "environment_optimization_measurement_invalid_hashes",
            "environment_optimization_measurement_path_escapes",
            "environment_optimization_measurement_over_budget=0",
            "environment_broad_optimization_ready=1",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment optimization artifact row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment optimization artifact row"
    }

    return $result
}

function Assert-EnvironmentMetalOptimizationProducerDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-metal-host-optimization-artifact-producer" -ExpectedArgv @("-File", "tools/generate-environment-metal-optimization-artifacts.ps1", "-RequireReady")
    Assert-ArrayContains $result.hostGates "metal-apple" "dry-run host gates for environment-metal-host-optimization-artifact-producer"
    foreach ($needle in @(
            "validation_recipe=environment-metal-host-optimization-artifact-producer",
            "environment_metal_host_optimization_artifact_ready=1",
            "xcrun_xctrace_ready=1",
            "xctrace_template=Metal_System_Trace",
            "environment_metal_host_optimization_artifacts_written=7",
            "environment_metal_host_optimization_profiler_artifacts=7",
            "environment_metal_host_optimization_trace_event_json=7",
            "environment_optimization_measurement_missing_artifacts=0",
            "environment_broad_optimization_ready=1",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment Metal optimization artifact producer"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment Metal optimization artifact producer"
    }

    return $result
}

function Assert-EnvironmentPresetAssetLibraryProductionDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-aaa-preset-asset-library-production" -ExpectedArgv @("-File", "tools/validate-environment-aaa-preset-asset-library.ps1", "-RequireReady")
    foreach ($needle in @(
            "environment_aaa_preset_asset_library_ready=1",
            "environment_aaa_preset_asset_library_sky_atmosphere_presets=24",
            "environment_aaa_preset_asset_library_volumetric_cloud_presets=24",
            "environment_aaa_preset_asset_library_fog_volume_presets=16",
            "environment_aaa_preset_asset_library_rain_presets=12",
            "environment_aaa_preset_asset_library_snow_presets=12",
            "environment_aaa_preset_asset_library_wind_presets=12",
            "environment_aaa_preset_asset_library_material_weathering_presets=24",
            "environment_aaa_preset_asset_library_lighting_ibl_presets=12",
            "environment_aaa_preset_asset_library_weather_timeline_presets=12",
            "environment_aaa_preset_asset_library_biome_environment_presets=8",
            "environment_aaa_preset_asset_library_preview_screenshot_rows=144",
            "environment_aaa_preset_asset_library_sample_scene_consumption_rows=8",
            "environment_preset_asset_license_missing_rows=0",
            "environment_preset_asset_package_budget_overages=0",
            "environment_preset_asset_external_asset_rows=0",
            "environment_aaa_preset_asset_library_missing_objective_rows=0",
            "environment_aaa_preset_asset_library_backend_execution=0",
            "environment_aaa_preset_asset_library_package_script_execution=0",
            "environment_aaa_preset_asset_library_native_handle_access=0",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment preset asset library production row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment preset asset library production row"
    }
    return $result
}

function Assert-EnvironmentAssetPipelineFullDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-asset-pipeline-openexr-ktx-basis-full" -ExpectedArgv @("-File", "tools/validate-environment-asset-pipeline-full.ps1", "-RequireReady")
    foreach ($needle in @(
            "environment_asset_pipeline_openexr_ktx_basis_full_ready=1",
            "environment_asset_pipeline_required_rows=14",
            "environment_asset_pipeline_ready_rows=14",
            "environment_asset_pipeline_openexr_rows=5",
            "environment_asset_pipeline_ktx2_basis_rows=4",
            "environment_asset_pipeline_backend_target_rows=4",
            "environment_asset_pipeline_runtime_rows=1",
            "environment_asset_pipeline_dependency_gated_rows=0",
            "environment_asset_pipeline_package_visible_rows=14",
            "environment_asset_pipeline_host_validated_rows=14",
            "environment_asset_pipeline_source_artifact_rows=14",
            "environment_asset_pipeline_cooked_artifact_rows=14",
            "environment_asset_pipeline_package_counter_rows=14",
            "environment_asset_pipeline_replay_hash_rows=14",
            "environment_asset_pipeline_rejection_diagnostic_rows=1",
            "openexr_scanline_rgba16f_ready=1",
            "openexr_tiled_rgba16f_ready=1",
            "openexr_multipart_ready=1",
            "openexr_metadata_preservation_ready=1",
            "openexr_deep_image_rejected_with_diagnostic=1",
            "ktx2_basis_etc1s_transcode_ready=1",
            "ktx2_basis_uastc_transcode_ready=1",
            "ktx2_mip_level_validation_ready=1",
            "ktx2_color_space_metadata_ready=1",
            "d3d12_bc7_target_ready=1",
            "vulkan_bc7_target_ready=1",
            "metal_astc_target_ready=1",
            "android_vulkan_astc_target_ready=1",
            "runtime_cooked_only_ingest_ready=1",
            "runtime_source_parsing=0",
            "environment_asset_pipeline_runtime_source_parsing=0",
            "environment_asset_pipeline_runtime_optional_codec_execution=0",
            "environment_asset_pipeline_native_handle_access=0",
            "environment_asset_pipeline_gpu_command_executed=0",
            "environment_asset_pipeline_package_command_executed=0",
            "environment_asset_pipeline_cmake_configure_dependency_install=0",
            "environment_asset_pipeline_optional_dependency_feature=asset-importers",
            "environment_asset_pipeline_openexr_dependency_recorded=1",
            "environment_asset_pipeline_ktx_dependency_recorded=1",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment asset pipeline full row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment asset pipeline full row"
    }
    return $result
}

function Assert-EnvironmentPhysicalWeatherSimulationCloseoutDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-physical-weather-simulation-closeout" -ExpectedArgv @("-File", "tools/validate-environment-weather-physics.ps1", "-RequireReady")
    foreach ($needle in @(
            "environment_physical_weather_simulation_ready=1",
            "environment_weather_simulation_cpu_reference_solver_ready=1",
            "environment_weather_simulation_production_solver_ready=1",
            "environment_weather_simulation_backend_parity_ready=1",
            "environment_weather_simulation_physical_weather_ready=1",
            "environment_weather_simulation_d3d12_gpu_solver_ready=1",
            "environment_weather_simulation_vulkan_gpu_solver_ready=1",
            "environment_weather_simulation_metal_gpu_solver_ready=1",
            "environment_weather_simulation_coupled_field_rows=13",
            "environment_weather_simulation_canonical_dataset_rows=12",
            "environment_weather_simulation_dataset_provenance_rows=12",
            "environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12",
            "environment_weather_simulation_canonical_image_rows=12",
            "environment_weather_simulation_backend_solver_rows=3",
            "environment_weather_simulation_host_validated_backend_rows=3",
            "environment_weather_simulation_compute_dispatch_rows=3",
            "environment_weather_simulation_synchronization_rows=3",
            "environment_weather_simulation_readback_rows=3",
            "environment_weather_simulation_mass_conservation_relative_error_max=0.001",
            "environment_weather_simulation_energy_or_stability_error_max=0.002",
            "environment_weather_simulation_negative_density_cells=0",
            "environment_weather_simulation_nan_or_inf_cells=0",
            "environment_weather_simulation_solver_budget_overages=0",
            "environment_weather_simulation_visual_regression_failures=0",
            "environment_weather_simulation_validation_failures=0",
            "environment_weather_simulation_backend_inference=0",
            "environment_weather_simulation_native_handle_access=0",
            "environment_weather_simulation_package_visible_rows=41",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment physical weather simulation closeout row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment physical weather simulation closeout row"
    }
    return $result
}

function Assert-EnvironmentArtistWorkflowProductionCloseoutDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-artist-workflow-production-closeout" -ExpectedArgv @("-File", "tools/validate-environment-artist-workflow-production.ps1", "-RequireReady")
    foreach ($needle in @(
            "environment_artist_workflow_production_status=ready",
            "environment_artist_workflow_production_ready=1",
            "workflow_import_openexr_ready=1",
            "workflow_import_ktx2_basis_ready=1",
            "workflow_import_gltf_material_ready=1",
            "workflow_review_usd_materialx_ocio_ready=1",
            "workflow_cook_package_ready=1",
            "workflow_live_preview_d3d12_ready=1",
            "workflow_live_preview_vulkan_ready=1",
            "workflow_live_preview_metal_host_ready=1",
            "workflow_weather_timeline_edit_ready=1",
            "workflow_preset_batch_apply_ready=1",
            "workflow_validation_report_ready=1",
            "workflow_profiler_artifact_review_ready=1",
            "workflow_undo_redo_revision_safety_ready=1",
            "workflow_operator_review_ready=1",
            "environment_artist_workflow_production_requirement_rows=14",
            "environment_artist_workflow_production_ready_rows=14",
            "environment_artist_workflow_editor_core_backend_execution=0",
            "environment_artist_workflow_editor_core_package_script_execution=0",
            "environment_artist_workflow_editor_core_validation_recipe_execution=0",
            "environment_artist_workflow_native_handle_access=0",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment artist workflow production closeout row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "host_gate=environment-artist-workflow-visible-shell-host")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment artist workflow production closeout row"
    }
    return $result
}

function Assert-EnvironmentHighestCommercialReadinessCloseoutDryRun {
    $result = Assert-DryRunRecipe -Recipe "environment-highest-commercial-readiness-closeout" -ExpectedArgv @("-File", "tools/validate-environment-highest-commercial-readiness.ps1", "-RequireReady")
    foreach ($needle in @(
            "environment_highest_commercial_status=ready",
            "environment_highest_commercial_ready=1",
            "environment_commercial_ready=1",
            "environment_commercial_required_rows=16",
            "environment_commercial_ready_rows=16",
            "environment_host_gated_rows=0",
            "environment_dependency_gated_rows=0",
            "environment_blocked_rows=0",
            "environment_unsupported_rows=0",
            "environment_missing_rows=0",
            "environment_native_handle_access=0",
            "environment_commercial_diagnostics=0",
            "environment_strict_vulkan_aggregate_ready=1",
            "environment_metal_aggregate_ready=1",
            "environment_backend_parity_ready=1",
            "environment_platform_windows_d3d12_ready=1",
            "environment_platform_windows_vulkan_ready=1",
            "environment_platform_linux_vulkan_ready=1",
            "environment_platform_macos_metal_ready=1",
            "environment_platform_ios_metal_ready=1",
            "environment_platform_android_vulkan_ready=1",
            "environment_platform_readiness_ready=1",
            "environment_all_platform_unconditional_ready=1",
            "environment_broad_optimization_ready=1",
            "environment_optimization_measurement_workload_rows=21",
            "environment_optimization_measurement_backend_rows=3",
            "environment_optimization_measurement_before_after_pairs=21",
            "environment_optimization_measurement_profiler_artifacts=21",
            "environment_optimization_measurement_trace_event_json=21",
            "environment_optimization_measurement_missing_artifacts=0",
            "environment_optimization_measurement_over_budget=0",
            "environment_asset_pipeline_openexr_ktx_basis_full_ready=1",
            "runtime_source_parsing=0",
            "environment_aaa_preset_asset_library_ready=1",
            "environment_physical_weather_simulation_ready=1",
            "environment_weather_simulation_backend_parity_ready=1",
            "environment_artist_workflow_production_ready=1",
            "workflow_visible_shell_execution_ready=1",
            "workflow_operator_review_ready=1",
            "environment_ready=0",
            "environment_ready_unchanged=1")) {
        Assert-ArgvContainsText -Result $result -Expected $needle -Label "dry-run environment highest commercial readiness closeout row"
    }
    foreach ($needle in @("validation_recipe_skeleton=1", "ready_claim=0", "environment_ready_promotion_blocked_until_all_rows_ready=1")) {
        Assert-ArgvDoesNotContainText -Result $result -Unexpected $needle -Label "dry-run environment highest commercial readiness closeout row"
    }
    return $result
}

function Assert-DryRunRecipe {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Recipe,

        [string[]]$ExpectedArgv = @(),

        [string[]]$HostGateAcknowledgements = @()
    )

    $runnerArguments = @("-Mode", "DryRun", "-Recipe", $Recipe)
    foreach ($acknowledgement in $HostGateAcknowledgements) {
        $runnerArguments += @("-HostGateAcknowledgements", $acknowledgement)
    }

    $result = Invoke-RunnerJson -Arguments $runnerArguments
    foreach ($property in @("recipe", "status", "command", "argv", "hostGates", "diagnostics", "blockedBy")) {
        Assert-HasProperty $result $property "dry-run result for $Recipe"
    }
    if ($result.recipe -ne $Recipe) {
        Write-Error "dry-run result used recipe '$($result.recipe)', expected '$Recipe'"
    }
    if ($result.status -ne "dry-run") {
        Write-Error "dry-run result for $Recipe must use status dry-run"
    }
    foreach ($expected in $ExpectedArgv) {
        if ($expected -like "*.ps1") {
            Assert-ArgvHasScriptSuffix -Result $result -ScriptSuffix $expected -Label "dry-run argv for $Recipe"
        } else {
            Assert-ArrayContains $result.argv $expected "dry-run argv for $Recipe"
        }
    }

    return $result
}

Write-Host "validation-recipe-runner-check: dry-run recipe contracts..."
Assert-RunnerCliSmoke
Assert-DryRunRecipe -Recipe "agent-contract" -ExpectedArgv @("-File", "check-ai-integration.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "default" -ExpectedArgv @("-File", "validate.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "shader-toolchain" -ExpectedArgv @("-File", "check-shader-toolchain.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "renderer-metal-apple-host-evidence" -ExpectedArgv @("-File", "validate-renderer-metal-apple.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "renderer-commercial-quality-closeout" -ExpectedArgv @("-File", "validate-renderer-commercial-quality-closeout.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "renderer-metal-environment-aggregate-apple-host-evidence" -ExpectedArgv @("-File", "validate-environment-metal-host-aggregate.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "environment-weather-metal-solver-host-gate" -ExpectedArgv @("-File", "validate-environment-weather-metal-solver-host-gate.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "network-enet" -ExpectedArgv @("-File", "validate-network-enet.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "desktop-editor" -ExpectedArgv @("-File", "build-editor.ps1") | Out-Null
Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-scene-gpu-package" -ExpectedArgv @("-File", "tools/package-desktop-runtime.ps1", "-GameTarget", "sample_desktop_runtime_game") | Out-Null
$sampleEnvironmentFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-fog-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-fog-package"
}
$sampleVulkanEnvironmentFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-fog-vulkan-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-fog-package"
}
$sampleVulkanPhysicalSkyDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-physical-sky-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-physical-sky-vulkan-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanPhysicalSkyDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-physical-sky-package"
}
$sampleEnvironmentVolumetricFogDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-volumetric-fog-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-volumetric-fog-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentVolumetricFogDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-volumetric-fog-package"
}
$sampleVulkanEnvironmentVolumetricFogRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-volumetric-fog-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentVolumetricFogRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution"
}
$sampleVulkanEnvironmentVolumetricCloudRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-volumetric-cloud-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentVolumetricCloudRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-volumetric-cloud-renderer-execution"
}
$sampleVulkanEnvironmentIblRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-lighting-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentIblRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution"
}
$sampleCloudLayerDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-cloud-layer-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-cloud-layer-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleCloudLayerDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-cloud-layer-package"
}
$sampleCloudLayerRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-cloud-layer-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-cloud-layer-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleCloudLayerRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-cloud-layer-renderer-execution"
}
$sampleEnvironmentPrecipitationDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-precipitation-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-precipitation-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPrecipitationDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-precipitation-package"
}
$sampleEnvironmentPrecipitationRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-precipitation-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-precipitation-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPrecipitationRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-precipitation-renderer-execution"
}
$sampleVulkanEnvironmentPrecipitationRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-precipitation-vulkan-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleVulkanEnvironmentPrecipitationRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution"
}
$sampleEnvironmentSnowDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-snow-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-snow-package-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentSnowDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-snow-package"
}
$sampleEnvironmentSnowRendererExecutionDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-snow-renderer-execution" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-snow-renderer-execution", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentSnowRendererExecutionDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-snow-renderer-execution"
}
$sampleEnvironmentMaterialWeatheringDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-material-weathering" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-material-weathering", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentMaterialWeatheringDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-material-weathering"
}
$sampleEnvironmentAudioPlaybackDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-audio-playback" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-audio-playback", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentAudioPlaybackDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-audio-playback"
}
$sampleEnvironmentProfileDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-profile-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-profile", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentProfileDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-profile-package"
}
$sampleEnvironmentReadyAggregateDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-ready-aggregate" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-ready-aggregate", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentReadyAggregateDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-ready-aggregate"
}
$sampleEnvironmentVulkanStrictAggregateDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-vulkan-strict-aggregate" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-vulkan-strict-aggregate", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentVulkanStrictAggregateDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-vulkan-strict-aggregate"
}
$sampleEnvironmentTextureAssetPipelineVulkanUploadDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-upload" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-vulkan-upload", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentTextureAssetPipelineVulkanUploadDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-upload"
}
$sampleEnvironmentTextureAssetPipelineD3d12CompressedUploadDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-texture-asset-pipeline-d3d12-compressed-upload" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-d3d12-compressed-upload", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentTextureAssetPipelineD3d12CompressedUploadDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-texture-asset-pipeline-d3d12-compressed-upload"
}
$sampleEnvironmentTextureAssetPipelineVulkanCompressedUploadDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-compressed-upload" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-vulkan-compressed-upload", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentTextureAssetPipelineVulkanCompressedUploadDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-compressed-upload"
}
$sampleEnvironmentTextureAssetPipelineMetalCompressedUploadDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-texture-asset-pipeline-metal-compressed-upload" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-SmokeArgs @(", "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-metal-compressed-upload", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentTextureAssetPipelineMetalCompressedUploadDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-texture-asset-pipeline-metal-compressed-upload"
}
$sampleEnvironmentPlatformReadinessDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-platform-readiness" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-platform-readiness", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPlatformReadinessDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-platform-readiness"
}
$sampleEnvironmentPlatformWindowsVulkanDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-renderer", "--require-environment-vulkan-strict-aggregate", "--require-environment-platform-windows-vulkan-evidence", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentPlatformWindowsVulkanDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence"
}
$environmentPlatformLinuxVulkanHostDryRun = Assert-DryRunRecipe -Recipe "environment-platform-linux-vulkan-host-gate" -ExpectedArgv @("-File", "tools/validate-linux-vulkan-runtime-host.ps1", "-RequireReady")
foreach ($needle in @("tools/validate-linux-vulkan-runtime-host.ps1", "-RequireReady")) {
    Assert-ArgvContainsText -Result $environmentPlatformLinuxVulkanHostDryRun -Expected $needle -Label "dry-run argv for environment-platform-linux-vulkan-host-gate"
}
$sampleEnvironmentOptimizationMeasurementDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-optimization-measurement" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-optimization-measurement", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentOptimizationMeasurementDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-optimization-measurement"
}
$sampleEnvironmentWeatherSimulationPackageDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-weather-simulation-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-weather-simulation-package", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentWeatherSimulationPackageDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-weather-simulation-package"
}
$sampleEnvironmentArtistWorkflowPackageDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-artist-workflow-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireD3d12Shaders", "-SmokeArgs @(", "--require-environment-artist-workflow-package", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentArtistWorkflowPackageDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-artist-workflow-package"
}
$sampleEnvironmentWeatherSimulationVulkanSolverDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-environment-weather-simulation-vulkan-solver-package", "runtime/sample_desktop_runtime_game.geindex")) {
    Assert-ArgvContainsText -Result $sampleEnvironmentWeatherSimulationVulkanSolverDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package"
}
$highestCommercialReadinessDryRun = Assert-EnvironmentHighestCommercialReadinessCloseoutDryRun
Assert-EnvironmentPlatformVulkanHostGateDryRun `
    -Recipe "environment-platform-linux-vulkan-package" `
    -ScriptSuffix "tools/validate-linux-vulkan-runtime-host.ps1" `
    -HostGate "linux-vulkan-runtime-host" `
    -ExpectedNeedles @(
        "environment-platform-linux-vulkan-package",
        "host=linux",
        "vulkaninfo_ready=1",
        "VK_LAYER_KHRONOS_validation_ready=1",
        "dxc_spirv_codegen_ready=1",
        "spirv_val_ready=1",
        "linux_icd_runtime_ready=1",
        "first_party_linux_runtime_host_ready=1",
        "linux_package_script_ready=1",
        "linux_installed_validator_ready=1",
        "linux_package_smoke_ready=1",
        "linux_vulkan_readback_ready=1",
        "linux_vulkan_validation_log_clean=1",
        "environment_platform_linux_vulkan_ready=1",
        "environment_platform_requires_linux_vulkan_host_evidence=0") | Out-Null
Assert-EnvironmentPlatformVulkanHostGateDryRun `
    -Recipe "environment-platform-android-vulkan-package" `
    -ScriptSuffix "tools/validate-android-vulkan-runtime-host.ps1" `
    -HostGate "android-vulkan-runtime-host" `
    -ExpectedNeedles @(
        "environment-platform-android-vulkan-package",
        "host_has_android_sdk=1",
        "host_has_android_ndk=1",
        "adb_device_or_emulator_ready=1",
        "android_vulkan_profile_ready=1",
        "android_gpu_debuggable_ready=1",
        "ValidationLayerJniLibs",
        "artifacts/environment/android/validation-layers/jniLibs",
        "android_validation_layer_jni_libs_ready=1",
        "android_validation_layer_apk_packaged=1",
        "VK_LAYER_KHRONOS_validation_ready=1",
        "android_package_smoke_ready=1",
        "android_vulkan_readback_ready=1",
        "android_vulkan_validation_layer_enumerated=1",
        "android_vulkan_validation_log_clean=1",
        "environment_platform_android_vulkan_ready=1",
        "environment_platform_requires_android_vulkan_host_evidence=0") | Out-Null
Assert-EnvironmentPlatformHostGateDryRun `
    -Recipe "environment-platform-ios-metal-package" `
    -ScriptSuffix "tools/validate-apple-metal-platform-host.ps1" `
    -HostGate "ios-metal-host" `
    -ExpectedNeedles @(
        "environment-platform-ios-metal-package",
        "host=macos",
        "xcode_ios_sdk_ready=1",
        "ios_simulator_or_device_ready=1",
        "ios_metal_feature_set_checked=1",
        "ios_package_smoke_ready=1",
        "ios_metal_command_queue_ready=1",
        "ios_metal_pipeline_ready=1",
        "ios_metal_command_buffer_ready=1",
        "ios_metal_readback_ready=1",
        "environment_platform_ios_metal_ready=1",
        "environment_platform_requires_ios_metal_host_evidence=0") | Out-Null
$backendParityV2DryRun = Assert-DryRunRecipe -Recipe "environment-backend-parity-v2-closeout" -ExpectedArgv @("-File", "tools/validate-environment-backend-parity-v2.ps1", "-RequireReady")
foreach ($needle in @("validation_recipe_skeleton=1", "tools/package-desktop-runtime.ps1")) {
    Assert-ArgvDoesNotContainText -Result $backendParityV2DryRun -Unexpected $needle -Label "dry-run backend parity v2 closeout"
}
Assert-EnvironmentOptimizationArtifactDryRun | Out-Null
Assert-EnvironmentMetalOptimizationProducerDryRun | Out-Null
Assert-EnvironmentAssetPipelineFullDryRun | Out-Null
Assert-EnvironmentPresetAssetLibraryProductionDryRun | Out-Null
Assert-EnvironmentPhysicalWeatherSimulationCloseoutDryRun | Out-Null
Assert-EnvironmentArtistWorkflowProductionCloseoutDryRun | Out-Null
Assert-DryRunRecipe -Recipe "desktop-runtime-generated-material-shader-scaffold-package" -ExpectedArgv @("-File", "tools/package-desktop-runtime.ps1", "-GameTarget", "sample_generated_desktop_runtime_material_shader_package") | Out-Null
$materialVulkanDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-scene-shaders", "--require-material-graph-authoring")) {
    Assert-ArgvContainsText -Result $materialVulkanDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict"
}
$sampleVulkanDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-renderer", "--require-native-ui-textured-sprite-atlas")) {
    Assert-ArgvContainsText -Result $sampleVulkanDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package"
}
Assert-ArgvDoesNotContainText -Result $sampleVulkanDryRun -Unexpected "--video-driver" -Label "dry-run argv for desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package"
$sampleVulkanSceneDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-scene-gpu-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-renderer", "--require-scene-gpu-bindings", "--require-renderer-quality-gates")) {
    Assert-ArgvContainsText -Result $sampleVulkanSceneDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-scene-gpu-package"
}
foreach ($needle in @("--video-driver", "--require-native-ui-overlay", "--require-native-ui-textured-sprite-atlas")) {
    Assert-ArgvDoesNotContainText -Result $sampleVulkanSceneDryRun -Unexpected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-scene-gpu-package"
}
$sampleVulkanOverlayDryRun = Assert-DryRunRecipe -Recipe "desktop-runtime-sample-game-vulkan-native-ui-overlay-package" -ExpectedArgv @("-Command")
foreach ($needle in @("tools/package-desktop-runtime.ps1", "-RequireVulkanShaders", "-SmokeArgs @(", "--require-vulkan-renderer", "--require-native-ui-overlay")) {
    Assert-ArgvContainsText -Result $sampleVulkanOverlayDryRun -Expected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-native-ui-overlay-package"
}
foreach ($needle in @("--video-driver", "--require-native-ui-textured-sprite-atlas")) {
    Assert-ArgvDoesNotContainText -Result $sampleVulkanOverlayDryRun -Unexpected $needle -Label "dry-run argv for desktop-runtime-sample-game-vulkan-native-ui-overlay-package"
}
Assert-DryRunRecipe -Recipe "dev-windows-editor-game-module-driver-load-tests" -ExpectedArgv @("-File", "tools/run-editor-game-module-driver-load-tests.ps1") | Out-Null

$unknown = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "not-a-recipe") -ExpectedExitCode 2
if ($unknown.status -ne "rejected" -or @($unknown.diagnostics | Where-Object { $_.code -eq "unknown-recipe" }).Count -ne 1) {
    Write-Error "unknown recipe id must be rejected with diagnostic code unknown-recipe"
}

$unsafeTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-sample-game-scene-gpu-package", "-GameTarget", "..\escape", "-HostGateAcknowledgements", "d3d12-windows-primary") -ExpectedExitCode 2
if ($unsafeTarget.status -ne "rejected" -or @($unsafeTarget.diagnostics | Where-Object { $_.code -eq "unsafe-game-target" }).Count -ne 1) {
    Write-Error "unsafe gameTarget must be rejected with diagnostic code unsafe-game-target"
}

$unsupportedMaterialTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package", "-GameTarget", "sample_desktop_runtime_game", "-HostGateAcknowledgements", "d3d12-windows-primary") -ExpectedExitCode 2
if ($unsupportedMaterialTarget.status -ne "rejected" -or @($unsupportedMaterialTarget.diagnostics | Where-Object { $_.code -eq "unsupported-game-target" }).Count -ne 1) {
    Write-Error "material/shader package recipe must reject non-allowlisted game targets"
}

$unsupportedMaterialVulkanTarget = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict", "-GameTarget", "sample_desktop_runtime_game", "-HostGateAcknowledgements", "vulkan-strict") -ExpectedExitCode 2
if ($unsupportedMaterialVulkanTarget.status -ne "rejected" -or @($unsupportedMaterialVulkanTarget.diagnostics | Where-Object { $_.code -eq "unsupported-game-target" }).Count -ne 1) {
    Write-Error "strict Vulkan material/shader package recipe must reject non-allowlisted game targets"
}

$unsupportedArgs = Invoke-RunnerJson -Arguments @("-Mode", "DryRun", "-Recipe", "agent-contract", "-RemainingArguments", "unexpected-free-form-arg") -ExpectedExitCode 2
if ($unsupportedArgs.status -ne "rejected" -or @($unsupportedArgs.diagnostics | Where-Object { $_.code -eq "unsupported-arguments" }).Count -ne 1) {
    Write-Error "unsupported free-form arguments must be rejected with diagnostic code unsupported-arguments"
}

$missingGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-ui-atlas-metadata-package") -ExpectedExitCode 2
if ($missingGate.status -ne "rejected" -or @($missingGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "missing host-gate acknowledgement must be rejected with diagnostic code missing-host-gate-acknowledgement"
}

$missingVulkanSceneGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-scene-gpu-package") -ExpectedExitCode 2
if ($missingVulkanSceneGate.status -ne "rejected" -or @($missingVulkanSceneGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan scene GPU package recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanOverlayGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-native-ui-overlay-package") -ExpectedExitCode 2
if ($missingVulkanOverlayGate.status -ne "rejected" -or @($missingVulkanOverlayGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan native UI overlay package recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanPrecipitationGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-environment-precipitation-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanPrecipitationGate.status -ne "rejected" -or @($missingVulkanPrecipitationGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan precipitation renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanVolumetricFogGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-volumetric-fog-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanVolumetricFogGate.status -ne "rejected" -or @($missingVulkanVolumetricFogGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan volumetric fog renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanIblGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-vulkan-environment-ibl-renderer-execution") -ExpectedExitCode 2
if ($missingVulkanIblGate.status -ne "rejected" -or @($missingVulkanIblGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan IBL renderer execution recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanEnvironmentAggregateGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-vulkan-strict-aggregate") -ExpectedExitCode 2
if ($missingVulkanEnvironmentAggregateGate.status -ne "rejected" -or @($missingVulkanEnvironmentAggregateGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan environment aggregate recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanTextureAssetPipelineUploadGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-texture-asset-pipeline-vulkan-upload") -ExpectedExitCode 2
if ($missingVulkanTextureAssetPipelineUploadGate.status -ne "rejected" -or @($missingVulkanTextureAssetPipelineUploadGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan environment texture asset-pipeline upload recipe must require vulkan-strict acknowledgement before execute"
}

$missingVulkanWeatherSolverGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package") -ExpectedExitCode 2
if ($missingVulkanWeatherSolverGate.status -ne "rejected" -or @($missingVulkanWeatherSolverGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan environment weather solver recipe must require vulkan-strict acknowledgement before execute"
}

$missingD3d12TextureAssetPipelineCompressedUploadGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-texture-asset-pipeline-d3d12-compressed-upload") -ExpectedExitCode 2
if ($missingD3d12TextureAssetPipelineCompressedUploadGate.status -ne "rejected" -or @($missingD3d12TextureAssetPipelineCompressedUploadGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "D3D12 environment texture asset-pipeline compressed upload recipe must require d3d12-windows-primary acknowledgement before execute"
}

$missingMetalTextureAssetPipelineCompressedUploadGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-texture-asset-pipeline-metal-compressed-upload") -ExpectedExitCode 2
if ($missingMetalTextureAssetPipelineCompressedUploadGate.status -ne "rejected" -or @($missingMetalTextureAssetPipelineCompressedUploadGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "Metal environment texture asset-pipeline compressed upload recipe must require metal-apple acknowledgement before execute"
}

$missingEnvironmentPlatformReadinessGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-platform-readiness") -ExpectedExitCode 2
if ($missingEnvironmentPlatformReadinessGate.status -ne "rejected" -or @($missingEnvironmentPlatformReadinessGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "environment platform readiness recipe must require d3d12-windows-primary acknowledgement before execute"
}

$missingEnvironmentPlatformWindowsVulkanGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence") -ExpectedExitCode 2
if ($missingEnvironmentPlatformWindowsVulkanGate.status -ne "rejected" -or @($missingEnvironmentPlatformWindowsVulkanGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "environment platform Windows Vulkan evidence recipe must require vulkan-strict acknowledgement before execute"
}

$missingEnvironmentPlatformLinuxVulkanHostGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "environment-platform-linux-vulkan-host-gate") -ExpectedExitCode 2
if ($missingEnvironmentPlatformLinuxVulkanHostGate.status -ne "rejected" -or @($missingEnvironmentPlatformLinuxVulkanHostGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "environment platform Linux Vulkan host-gate recipe must require vulkan-strict-linux acknowledgement before execute"
}

$missingEnvironmentOptimizationMeasurementGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-sample-game-environment-optimization-measurement") -ExpectedExitCode 2
if ($missingEnvironmentOptimizationMeasurementGate.status -ne "rejected" -or @($missingEnvironmentOptimizationMeasurementGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "environment optimization measurement recipe must require d3d12-windows-primary acknowledgement before execute"
}

$missingMaterialVulkanGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "desktop-runtime-generated-material-shader-scaffold-package-vulkan-strict") -ExpectedExitCode 2
if ($missingMaterialVulkanGate.status -ne "rejected" -or @($missingMaterialVulkanGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "strict Vulkan material/shader package recipe must require vulkan-strict acknowledgement before execute"
}

$missingEditorDriverGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "dev-windows-editor-game-module-driver-load-tests") -ExpectedExitCode 2
if ($missingEditorDriverGate.status -ne "rejected" -or @($missingEditorDriverGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "dev-windows-editor-game-module-driver-load-tests recipe must require windows-msvc-dev-editor-game-module-driver-ctest acknowledgement before execute"
}

$missingMetalAppleGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "renderer-metal-apple-host-evidence") -ExpectedExitCode 2
if ($missingMetalAppleGate.status -ne "rejected" -or @($missingMetalAppleGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "renderer-metal-apple-host-evidence recipe must require metal-apple acknowledgement before execute"
}

$missingEnvironmentMetalAggregateGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "renderer-metal-environment-aggregate-apple-host-evidence") -ExpectedExitCode 2
if ($missingEnvironmentMetalAggregateGate.status -ne "rejected" -or @($missingEnvironmentMetalAggregateGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "renderer-metal-environment-aggregate-apple-host-evidence recipe must require metal-apple acknowledgement before execute"
}

$missingEnvironmentWeatherMetalSolverGate = Invoke-RunnerJson -Arguments @("-Mode", "Execute", "-Recipe", "environment-weather-metal-solver-host-gate") -ExpectedExitCode 2
if ($missingEnvironmentWeatherMetalSolverGate.status -ne "rejected" -or @($missingEnvironmentWeatherMetalSolverGate.diagnostics | Where-Object { $_.code -eq "missing-host-gate-acknowledgement" }).Count -ne 1) {
    Write-Error "environment-weather-metal-solver-host-gate recipe must require metal-apple acknowledgement before execute"
}

# Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` / `tools/check-ai-integration.ps1` runs once from `tools/validate.ps1` after this script.
# Keeping a second Execute(agent-contract) here would duplicate that multi-minute integration pass.

Write-Host "validation-recipe-runner-check: ok"
