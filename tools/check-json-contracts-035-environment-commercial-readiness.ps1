#requires -Version 7.0
#requires -PSEdition Core

# Chapter 35 for check-json-contracts.ps1 static contracts.
$engineForEnvironmentCommercial = Read-Json "engine/agent/manifest.json"
$environmentCommercialSchema = Read-Json "schemas/engine-agent/ai-operable-production-loop.schema.json"
$environmentCommercialLoop = $engineForEnvironmentCommercial.aiOperableProductionLoop

foreach ($requiredSchemaField in @("environmentCommercialClaimMatrix", "environmentPlatformReadinessRows", "environmentOptimizationMeasurementWorkloadRows", "environmentPhysicalWeatherSimulationRows", "environmentArtistWorkflowRows", "environmentCommercialUnsupportedAdjacentClaims")) {
    if (@($environmentCommercialSchema.required) -notcontains $requiredSchemaField) {
        Write-Error "ai-operable-production-loop.schema.json must require $requiredSchemaField"
    }
    if (-not $environmentCommercialSchema.properties.PSObject.Properties.Name.Contains($requiredSchemaField)) {
        Write-Error "ai-operable-production-loop.schema.json must define $requiredSchemaField"
    }
}

$environmentCommercialValidationRecipeNames = @{}
$environmentCommercialValidationRecipesByName = @{}
foreach ($validationRecipe in @($engineForEnvironmentCommercial.validationRecipes)) {
    $recipeName = [string]$validationRecipe.name
    $environmentCommercialValidationRecipeNames[$recipeName] = $true
    $environmentCommercialValidationRecipesByName[$recipeName] = $validationRecipe
}

$expectedHighestCommercialRecipeIds = @(
    "environment-highest-commercial-readiness-closeout",
    "environment-platform-linux-vulkan-package",
    "environment-platform-android-vulkan-package",
    "environment-platform-ios-metal-package",
    "environment-backend-parity-v2-closeout",
    "environment-broad-optimization-cross-backend-measurement",
    "environment-metal-host-optimization-artifact-producer",
    "environment-asset-pipeline-openexr-ktx-basis-full",
    "environment-aaa-preset-asset-library-production",
    "environment-physical-weather-simulation-closeout",
    "environment-artist-workflow-production-closeout"
)
foreach ($recipeId in $expectedHighestCommercialRecipeIds) {
    if (-not $environmentCommercialValidationRecipeNames.ContainsKey($recipeId)) {
        Write-Error "engine manifest validationRecipes missing Environment Highest Commercial Readiness v1 recipe: $recipeId"
    }
}
$environmentHighestCommercialRecipe = $environmentCommercialValidationRecipesByName["environment-highest-commercial-readiness-closeout"]
if ($null -eq $environmentHighestCommercialRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-highest-commercial-readiness-closeout"
} else {
    $environmentHighestCommercialRecipeText = [string]::Join(" ", @([string]$environmentHighestCommercialRecipe.command, [string]$environmentHighestCommercialRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-highest-commercial-readiness.ps1",
            "-RequireReady",
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
        if (-not $environmentHighestCommercialRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-highest-commercial-readiness-closeout' missing highest commercial closeout needle: $needle"
        }
    }
    if ($environmentHighestCommercialRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-highest-commercial-readiness-closeout' must not remain a dry-run skeleton after Task 12"
    }
}

function Assert-ReadyCounterHasSupportingCounters {
    param(
        [Parameter(Mandatory = $true)][string]$Text,
        [Parameter(Mandatory = $true)][string]$ReadyCounter,
        [Parameter(Mandatory = $true)][string[]]$SupportingCounters,
        [Parameter(Mandatory = $true)][string]$Label
    )

    if (-not $Text.Contains($ReadyCounter)) {
        return
    }
    foreach ($supportingCounter in @($SupportingCounters)) {
        if (-not $Text.Contains($supportingCounter)) {
            Write-Error "$Label contains $ReadyCounter without required supporting counter: $supportingCounter"
        }
    }
}

if ($null -ne $environmentHighestCommercialRecipe) {
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_commercial_ready=1" -SupportingCounters @(
        "environment_all_platform_unconditional_ready=1",
        "environment_broad_optimization_ready=1",
        "environment_physical_weather_simulation_ready=1",
        "environment_artist_workflow_production_ready=1",
        "environment_asset_pipeline_openexr_ktx_basis_full_ready=1",
        "environment_native_handle_access=0",
        "environment_commercial_diagnostics=0") -Label "environment-highest-commercial-readiness-closeout recipe"
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_all_platform_unconditional_ready=1" -SupportingCounters @(
        "environment_platform_windows_d3d12_ready=1",
        "environment_platform_windows_vulkan_ready=1",
        "environment_platform_linux_vulkan_ready=1",
        "environment_platform_macos_metal_ready=1",
        "environment_platform_ios_metal_ready=1",
        "environment_platform_android_vulkan_ready=1") -Label "environment-highest-commercial-readiness-closeout recipe"
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_broad_optimization_ready=1" -SupportingCounters @(
        "environment_optimization_measurement_workload_rows=21",
        "environment_optimization_measurement_backend_rows=3",
        "environment_optimization_measurement_before_after_pairs=21",
        "environment_optimization_measurement_profiler_artifacts=21",
        "environment_optimization_measurement_trace_event_json=21",
        "environment_optimization_measurement_missing_artifacts=0",
        "environment_optimization_measurement_over_budget=0") -Label "environment-highest-commercial-readiness-closeout recipe"
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_physical_weather_simulation_ready=1" -SupportingCounters @(
        "environment_weather_simulation_backend_parity_ready=1") -Label "environment-highest-commercial-readiness-closeout recipe"
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_artist_workflow_production_ready=1" -SupportingCounters @(
        "workflow_visible_shell_execution_ready=1",
        "workflow_operator_review_ready=1") -Label "environment-highest-commercial-readiness-closeout recipe"
    Assert-ReadyCounterHasSupportingCounters -Text $environmentHighestCommercialRecipeText -ReadyCounter "environment_asset_pipeline_openexr_ktx_basis_full_ready=1" -SupportingCounters @(
        "runtime_source_parsing=0") -Label "environment-highest-commercial-readiness-closeout recipe"
}
$expectedEnvironmentPlatformHostRecipes = @(
    @{
        Recipe = "environment-platform-linux-vulkan-package"
        Script = "tools/validate-linux-vulkan-runtime-host.ps1"
        Forbidden = "validation_recipe_skeleton=1"
        Needles = @("host=linux", "vulkaninfo_ready=1", "VK_LAYER_KHRONOS_validation_ready=1", "dxc_spirv_codegen_ready=1", "spirv_val_ready=1", "linux_icd_runtime_ready=1", "first_party_linux_runtime_host_ready=1", "linux_package_script_ready=1", "linux_installed_validator_ready=1", "linux_package_smoke_ready=1", "linux_vulkan_readback_ready=1", "linux_vulkan_validation_log_clean=1", "environment_platform_linux_vulkan_ready=1", "environment_platform_requires_linux_vulkan_host_evidence=0")
    },
    @{
        Recipe = "environment-platform-android-vulkan-package"
        Script = "tools/validate-android-vulkan-runtime-host.ps1"
        Forbidden = "validation_recipe_skeleton=1"
        Needles = @("host_has_android_sdk=1", "host_has_android_ndk=1", "adb_device_or_emulator_ready=1", "android_vulkan_profile_ready=1", "android_gpu_debuggable_ready=1", "android_gpu_debug_layer_settings_ready=1", "android_gpu_debug_layer_app_installed=1", "android_gpu_debug_layer_install_requested=1", "android_gpu_debug_layer_install_ready=1", "VK_LAYER_KHRONOS_validation_ready=1", "android_package_smoke_ready=1", "android_vulkan_readback_ready=1", "android_vulkan_validation_layer_enumerated=1", "android_vulkan_validation_log_clean=1", "environment_platform_android_vulkan_ready=1", "environment_platform_requires_android_vulkan_host_evidence=0")
    },
    @{
        Recipe = "environment-platform-ios-metal-package"
        Script = "tools/validate-apple-metal-platform-host.ps1"
        Forbidden = "validation_recipe_skeleton=1"
        Needles = @("host=macos", "xcode_ios_sdk_ready=1", "ios_simulator_or_device_ready=1", "ios_metal_feature_set_checked=1", "ios_package_smoke_ready=1", "ios_metal_command_queue_ready=1", "ios_metal_pipeline_ready=1", "ios_metal_command_buffer_ready=1", "ios_metal_readback_ready=1", "environment_platform_ios_metal_ready=1", "environment_platform_requires_ios_metal_host_evidence=0", "environment_all_platform_unconditional_ready=0")
    }
)
foreach ($recipeContract in $expectedEnvironmentPlatformHostRecipes) {
    $recipeId = [string]$recipeContract.Recipe
    $recipe = $environmentCommercialValidationRecipesByName[$recipeId]
    if ($null -eq $recipe) {
        continue
    }
    $recipeText = [string]::Join(" ", @([string]$recipe.command, [string]$recipe.purpose))
    if (-not $recipeText.Contains([string]$recipeContract.Script)) {
        Write-Error "engine manifest validationRecipes '$recipeId' must route through $($recipeContract.Script)"
    }
    if ($recipeText.Contains([string]$recipeContract.Forbidden)) {
        Write-Error "engine manifest validationRecipes '$recipeId' must not remain a dry-run skeleton after its platform host-validator task"
    }
    foreach ($needle in @($recipeContract.Needles)) {
        if (-not $recipeText.Contains([string]$needle)) {
            Write-Error "engine manifest validationRecipes '$recipeId' missing platform host-validator needle: $needle"
        }
    }
}

$desktopRuntimeGamesCMakeText = Get-Content -Raw -Path (Join-Path $root "games/CMakeLists.txt")
foreach ($needle in @(
        'HOST_BACKEND must be win32 or linux',
        'HOST_BACKEND linux is only supported on Linux',
        'MK_runtime_host_linux',
        'sample_desktop_runtime_game/linux_main.cpp',
        '--require-linux-vulkan-presentation-smoke',
        '--require-linux-vulkan-readback',
        '--require-linux-vulkan-validation-log')) {
    if (-not $desktopRuntimeGamesCMakeText.Contains($needle)) {
        Write-Error "games/CMakeLists.txt missing Linux Vulkan presentation package contract needle: $needle"
    }
}

$linuxSampleRuntimeMainPath = Join-Path $root "games/sample_desktop_runtime_game/linux_main.cpp"
if (-not (Test-Path -LiteralPath $linuxSampleRuntimeMainPath -PathType Leaf)) {
    Write-Error "sample_desktop_runtime_game must provide a Linux-specific runtime package smoke entrypoint: games/sample_desktop_runtime_game/linux_main.cpp"
} else {
    $linuxSampleRuntimeMainText = Get-Content -Raw -Path $linuxSampleRuntimeMainPath
    foreach ($needle in @(
            'probe_linux_desktop_vulkan_presentation',
            'LinuxDesktopVulkanPresentationProbeDesc',
            'linux_desktop_vulkan_presentation_status_name',
            '--require-linux-vulkan-presentation-smoke',
            '--require-linux-vulkan-readback',
            '--require-linux-vulkan-validation-log',
            'linux_package_smoke_ready=',
            'linux_vulkan_readback_ready=',
            'linux_vulkan_validation_log_clean=',
            'environment_platform_linux_vulkan_ready=',
            'environment_platform_windows_vulkan_inferred=0',
            'VK_LAYER_KHRONOS_validation_ready=',
            'native_handle_access=0')) {
        if (-not $linuxSampleRuntimeMainText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/linux_main.cpp missing Linux Vulkan presentation package needle: $needle"
        }
    }
    if ($linuxSampleRuntimeMainText.Contains('environment_platform_windows_vulkan_inferred=1') -or
        $linuxSampleRuntimeMainText.Contains('native_handle_access=1')) {
        Write-Error "games/sample_desktop_runtime_game/linux_main.cpp must not infer Windows Vulkan readiness or expose native handles"
    }
}

$linuxHostContractPath = Join-Path $root "engine/runtime_host/src/linux_desktop_host_contract.cpp"
$linuxHostContractText = Get-Content -Raw -Path $linuxHostContractPath
foreach ($needle in @(
        'probe_linux_desktop_vulkan_presentation',
        'LinuxPresentationXcbWindow',
        'SurfacePlatform::xcb',
        'probe_runtime_surface_support',
        'create_runtime_device',
        'create_runtime_swapchain',
        'acquire_next_runtime_swapchain_image',
        'record_runtime_dynamic_rendering_clear',
        'record_runtime_swapchain_image_readback',
        'present_runtime_swapchain_image',
        'VK_EXT_debug_utils',
        'validation_log_snapshot',
        'validation_log.clean()')) {
    if (-not $linuxHostContractText.Contains($needle)) {
        Write-Error "engine/runtime_host/src/linux_desktop_host_contract.cpp missing Linux Vulkan package probe needle: $needle"
    }
}

$vulkanBackendText = Get-Content -Raw -Path (Join-Path $root "engine/rhi/vulkan/src/vulkan_backend.cpp")
foreach ($needle in @(
        'Vulkan XCB swapchain requires connection and window handles',
        'Vulkan XCB surface commands are unavailable',
        'vkCreateXcbSurfaceKHR failed',
        'SurfacePlatform::xcb',
        'VulkanRuntimeValidationLogSnapshot',
        'vkCreateDebugUtilsMessengerEXT',
        'vkDestroyDebugUtilsMessengerEXT',
        'record_runtime_validation_message')) {
    if (-not $vulkanBackendText.Contains($needle)) {
        Write-Error "engine/rhi/vulkan/src/vulkan_backend.cpp missing Linux XCB swapchain owner needle: $needle"
    }
}

$environmentBackendParityV2Recipe = $environmentCommercialValidationRecipesByName["environment-backend-parity-v2-closeout"]
if ($null -eq $environmentBackendParityV2Recipe) {
    Write-Error "engine manifest validationRecipes missing environment-backend-parity-v2-closeout"
} else {
    $environmentBackendParityV2RecipeText = [string]::Join(" ", @([string]$environmentBackendParityV2Recipe.command, [string]$environmentBackendParityV2Recipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-backend-parity-v2.ps1",
            "-RequireReady",
            "environment_backend_parity_ready=1",
            "15-feature",
            "45 backend-local ready rows",
            "zero D3D12/Vulkan/Metal inference counters",
            "zero native-handle access",
            "zero GPU command execution")) {
        if (-not $environmentBackendParityV2RecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-backend-parity-v2-closeout' missing v2 closeout needle: $needle"
        }
    }
    if ($environmentBackendParityV2RecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-backend-parity-v2-closeout' must not remain a dry-run skeleton after Task 6"
    }
}

$environmentOptimizationArtifactRecipe = $environmentCommercialValidationRecipesByName["environment-broad-optimization-cross-backend-measurement"]
if ($null -eq $environmentOptimizationArtifactRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-broad-optimization-cross-backend-measurement"
} else {
    $environmentOptimizationArtifactRecipeText = [string]::Join(" ", @([string]$environmentOptimizationArtifactRecipe.command, [string]$environmentOptimizationArtifactRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-optimization-artifacts.ps1",
            "-RequireReady",
            "artifacts/environment/optimization/<task-id>/<backend>/<workload>/",
            "d3d12",
            "vulkan_strict",
            "metal_apple_host",
            "environment_optimization_measurement_workload_rows=21",
            "environment_optimization_measurement_backend_rows=3",
            "environment_optimization_measurement_before_after_pairs=21",
            "environment_optimization_measurement_profiler_artifacts=21",
            "environment_optimization_measurement_trace_event_json=21",
            "environment_optimization_measurement_missing_artifacts=0",
            "environment_optimization_measurement_invalid_hashes=0",
            "environment_optimization_measurement_path_escapes=0",
            "environment_optimization_measurement_over_budget=0",
            "environment_broad_optimization_ready=1",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        if (-not $environmentOptimizationArtifactRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-broad-optimization-cross-backend-measurement' missing artifact closeout needle: $needle"
        }
    }
    if ($environmentOptimizationArtifactRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-broad-optimization-cross-backend-measurement' must not remain a dry-run skeleton after Task 7"
    }
}

$environmentMetalOptimizationProducerRecipe = $environmentCommercialValidationRecipesByName["environment-metal-host-optimization-artifact-producer"]
if ($null -eq $environmentMetalOptimizationProducerRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-metal-host-optimization-artifact-producer"
} else {
    $environmentMetalOptimizationProducerRecipeText = [string]::Join(" ", @([string]$environmentMetalOptimizationProducerRecipe.command, [string]$environmentMetalOptimizationProducerRecipe.purpose))
    foreach ($needle in @(
            "tools/generate-environment-metal-optimization-artifacts.ps1",
            "-RequireReady",
            "host=macos",
            "host_gate=metal-apple",
            "xcrun_xctrace_ready=1",
            "xctrace_template=Metal_System_Trace",
            "environment_metal_host_optimization_artifacts_written=7",
            "environment_metal_host_optimization_profiler_artifacts=7",
            "environment_metal_host_optimization_trace_event_json=7",
            "environment_optimization_measurement_missing_artifacts=0",
            "environment_broad_optimization_ready=1",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        if (-not $environmentMetalOptimizationProducerRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-metal-host-optimization-artifact-producer' missing producer needle: $needle"
        }
    }
    if ($environmentMetalOptimizationProducerRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-metal-host-optimization-artifact-producer' must not be a dry-run skeleton"
    }
}

$environmentAaaPresetAssetLibraryRecipe = $environmentCommercialValidationRecipesByName["environment-aaa-preset-asset-library-production"]
if ($null -eq $environmentAaaPresetAssetLibraryRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-aaa-preset-asset-library-production"
} else {
    $environmentAaaPresetAssetLibraryRecipeText = [string]::Join(" ", @([string]$environmentAaaPresetAssetLibraryRecipe.command, [string]$environmentAaaPresetAssetLibraryRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-aaa-preset-asset-library.ps1",
            "-RequireReady",
            "MK_environment_tests",
            "156 preset assets",
            "environment_aaa_preset_asset_library_ready=1",
            "environment_aaa_preset_asset_library_asset_rows=156",
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
            "environment_preset_asset_license_provenance_rows=156",
            "environment_preset_asset_package_budget_rows=156",
            "environment_preset_asset_license_missing_rows=0",
            "environment_preset_asset_package_budget_overages=0",
            "environment_preset_asset_external_asset_rows=0",
            "environment_aaa_preset_asset_library_missing_objective_rows=0",
            "environment_aaa_preset_asset_library_backend_execution=0",
            "environment_aaa_preset_asset_library_package_script_execution=0",
            "environment_aaa_preset_asset_library_native_handle_access=0",
            "environment_ready=0",
            "environment_commercial_ready=0")) {
        if (-not $environmentAaaPresetAssetLibraryRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-aaa-preset-asset-library-production' missing production asset-library needle: $needle"
        }
    }
    if ($environmentAaaPresetAssetLibraryRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-aaa-preset-asset-library-production' must not remain a dry-run skeleton after Task 8"
    }
}

$expectedEnvironmentCommercialClaimIds = @(
    "environment_highest_commercial_ready",
    "environment_commercial_ready",
    "environment_strict_vulkan_aggregate_ready",
    "environment_metal_aggregate_ready",
    "environment_vulkan_strict_aggregate_ready",
    "environment_metal_host_aggregate_ready",
    "environment_backend_parity_ready",
    "environment_platform_windows_d3d12_ready",
    "environment_platform_windows_vulkan_ready",
    "environment_platform_linux_vulkan_ready",
    "environment_platform_macos_metal_ready",
    "environment_platform_ios_metal_ready",
    "environment_platform_android_vulkan_ready",
    "environment_platform_readiness_ready",
    "environment_all_platform_unconditional_ready",
    "environment_broad_optimization_ready",
    "environment_asset_pipeline_openexr_ktx_basis_ready",
    "environment_asset_pipeline_openexr_ktx_basis_full_ready",
    "environment_aaa_preset_library_ready",
    "environment_aaa_preset_asset_library_ready",
    "environment_physical_weather_simulation_ready",
    "environment_artist_workflow_ready",
    "environment_artist_workflow_production_ready"
)
$expectedEnvironmentCommercialClaimStates = @{
    environment_highest_commercial_ready = "unsupported"
    environment_commercial_ready = "unsupported"
    environment_strict_vulkan_aggregate_ready = "host-gated"
    environment_metal_aggregate_ready = "ready"
    environment_vulkan_strict_aggregate_ready = "ready"
    environment_metal_host_aggregate_ready = "ready"
    environment_backend_parity_ready = "ready"
    environment_platform_windows_d3d12_ready = "ready"
    environment_platform_windows_vulkan_ready = "ready"
    environment_platform_linux_vulkan_ready = "ready"
    environment_platform_macos_metal_ready = "ready"
    environment_platform_ios_metal_ready = "ready"
    environment_platform_android_vulkan_ready = "host-gated"
    environment_platform_readiness_ready = "host-gated"
    environment_all_platform_unconditional_ready = "host-gated"
    environment_broad_optimization_ready = "ready"
    environment_asset_pipeline_openexr_ktx_basis_ready = "ready"
    environment_asset_pipeline_openexr_ktx_basis_full_ready = "ready"
    environment_aaa_preset_library_ready = "ready"
    environment_aaa_preset_asset_library_ready = "ready"
    environment_physical_weather_simulation_ready = "ready"
    environment_artist_workflow_ready = "ready"
    environment_artist_workflow_production_ready = "ready"
}

$environmentCommercialClaimRows = @($environmentCommercialLoop.environmentCommercialClaimMatrix)
if ($environmentCommercialClaimRows.Count -ne $expectedEnvironmentCommercialClaimIds.Count) {
    Write-Error "engine manifest environmentCommercialClaimMatrix must contain exactly $($expectedEnvironmentCommercialClaimIds.Count) target claims"
}
$environmentCommercialClaimsById = @{}
foreach ($claimRow in $environmentCommercialClaimRows) {
    Assert-Properties $claimRow @("id", "state", "validationRecipeIds", "packageCounter", "dependsOn", "requiredEvidence", "forbiddenInference", "notes") "engine manifest environmentCommercialClaimMatrix"
    $claimId = [string]$claimRow.id
    if ($environmentCommercialClaimsById.ContainsKey($claimId)) {
        Write-Error "engine manifest environmentCommercialClaimMatrix duplicate claim id: $claimId"
    }
    $environmentCommercialClaimsById[$claimId] = $claimRow
    if ($expectedEnvironmentCommercialClaimIds -notcontains $claimId) {
        Write-Error "engine manifest environmentCommercialClaimMatrix has unexpected claim id: $claimId"
    }
    if (@("ready", "host-gated", "unsupported") -notcontains [string]$claimRow.state) {
        Write-Error "engine manifest environmentCommercialClaimMatrix '$claimId' has invalid state: $($claimRow.state)"
    }
    if ($expectedEnvironmentCommercialClaimStates.ContainsKey($claimId) -and
        [string]$claimRow.state -ne $expectedEnvironmentCommercialClaimStates[$claimId]) {
        Write-Error "engine manifest environmentCommercialClaimMatrix '$claimId' must remain $($expectedEnvironmentCommercialClaimStates[$claimId]) in Phase 1, not $($claimRow.state)"
    }
    if ([string]$claimRow.packageCounter -ne $claimId -or
        [string]$claimRow.packageCounter -notmatch '^environment_[a-z0-9]+(_[a-z0-9]+)*_ready$') {
        Write-Error "engine manifest environmentCommercialClaimMatrix '$claimId' packageCounter must equal the lower_snake_case claim id"
    }
    foreach ($recipeId in @($claimRow.validationRecipeIds)) {
        if (-not $environmentCommercialValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "engine manifest environmentCommercialClaimMatrix '$claimId' references unknown validation recipe: $recipeId"
        }
    }
}
foreach ($claimId in $expectedEnvironmentCommercialClaimIds) {
    if (-not $environmentCommercialClaimsById.ContainsKey($claimId)) {
        Write-Error "engine manifest environmentCommercialClaimMatrix missing target claim id: $claimId"
    }
}
foreach ($claimRow in $environmentCommercialClaimRows) {
    foreach ($dependencyId in @($claimRow.dependsOn)) {
        if (-not $environmentCommercialClaimsById.ContainsKey([string]$dependencyId)) {
            Write-Error "engine manifest environmentCommercialClaimMatrix '$($claimRow.id)' references unknown dependency: $dependencyId"
        }
    }
}
$metalAggregateClaim = $environmentCommercialClaimsById["environment_metal_host_aggregate_ready"]
if ($null -eq $metalAggregateClaim -or
    @($metalAggregateClaim.validationRecipeIds) -notcontains "renderer-metal-environment-aggregate-apple-host-evidence") {
    Write-Error "engine manifest environment_metal_host_aggregate_ready must reference the dedicated renderer-metal-environment-aggregate-apple-host-evidence validation recipe"
}
$macosMetalPlatformClaim = $environmentCommercialClaimsById["environment_platform_macos_metal_ready"]
if ($null -eq $macosMetalPlatformClaim -or
    [string]$macosMetalPlatformClaim.state -ne "ready" -or
    @($macosMetalPlatformClaim.validationRecipeIds) -notcontains "renderer-metal-environment-aggregate-apple-host-evidence" -or
    -not [string]$macosMetalPlatformClaim.requiredEvidence.Contains("environment_platform_macos_metal_ready=1") -or
    -not [string]$macosMetalPlatformClaim.notes.Contains("environment_platform_macos_metal_evidence_requested=1") -or
    -not [string]$macosMetalPlatformClaim.notes.Contains("environment_platform_readiness_ready=0")) {
    Write-Error "engine manifest environment_platform_macos_metal_ready must be ready only through the Apple-host Metal aggregate recipe without all-platform promotion"
}
$iosMetalPlatformClaim = $environmentCommercialClaimsById["environment_platform_ios_metal_ready"]
if ($null -eq $iosMetalPlatformClaim -or
    [string]$iosMetalPlatformClaim.state -ne "ready" -or
    @($iosMetalPlatformClaim.validationRecipeIds) -notcontains "environment-platform-ios-metal-package" -or
    @($iosMetalPlatformClaim.validationRecipeIds) -notcontains "ios-simulator-smoke" -or
    -not [string]$iosMetalPlatformClaim.requiredEvidence.Contains("environment_platform_ios_metal_ready=1") -or
    -not [string]$iosMetalPlatformClaim.requiredEvidence.Contains("ios_metal_command_buffer_ready=1") -or
    -not [string]$iosMetalPlatformClaim.notes.Contains("validate.yml ios-metal") -or
    -not [string]$iosMetalPlatformClaim.notes.Contains("environment_all_platform_unconditional_ready=0")) {
    Write-Error "engine manifest environment_platform_ios_metal_ready must be ready only through the Apple-host iOS Metal package validator without all-platform promotion"
}
$cleanBreakMetalAggregateClaim = $environmentCommercialClaimsById["environment_metal_aggregate_ready"]
if ($null -eq $cleanBreakMetalAggregateClaim -or
    [string]$cleanBreakMetalAggregateClaim.state -ne "ready" -or
    @($cleanBreakMetalAggregateClaim.dependsOn) -notcontains "environment_metal_host_aggregate_ready" -or
    @($cleanBreakMetalAggregateClaim.dependsOn) -notcontains "environment_platform_macos_metal_ready" -or
    @($cleanBreakMetalAggregateClaim.dependsOn) -notcontains "environment_platform_ios_metal_ready" -or
    -not [string]$cleanBreakMetalAggregateClaim.requiredEvidence.Contains("environment_metal_aggregate_ready=1") -or
    -not [string]$cleanBreakMetalAggregateClaim.requiredEvidence.Contains("validate.yml macos") -or
    -not [string]$cleanBreakMetalAggregateClaim.requiredEvidence.Contains("validate.yml ios-metal") -or
    -not [string]$cleanBreakMetalAggregateClaim.notes.Contains("environment_metal_host_aggregate_ready") -or
    -not [string]$cleanBreakMetalAggregateClaim.notes.Contains("not a compatibility alias")) {
    Write-Error "engine manifest environment_metal_aggregate_ready must be ready only from the combined macOS and iOS Metal hosted evidence rows"
}
$metalAggregateToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-environment-metal-host-aggregate.ps1") -Raw
foreach ($needle in @(
        "environment-platform-macos-metal-evidence:",
        "environment_platform_readiness_status=host_evidence_required",
        "environment_platform_readiness_ready=0",
        "environment_platform_macos_metal_evidence_requested=1",
        "environment_platform_metal_host_aggregate_ready=1",
        "environment_platform_macos_metal_ready=1",
        "environment_platform_requires_macos_metal_host_evidence=0",
        "environment_all_platform_unconditional_ready=0",
        "environment_platform_native_handle_access=0")) {
    if (-not $metalAggregateToolText.Contains($needle)) {
        Write-Error "tools/validate-environment-metal-host-aggregate.ps1 missing macOS Metal platform evidence counter: $needle"
    }
}
$appleMetalPlatformToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-apple-metal-platform-host.ps1") -Raw
foreach ($needle in @(
        '$iosEvidence.CommandBufferReady',
        '$metalAggregateReady = $macosMetalReady -and $iosMetalReady',
        'environment_metal_aggregate_ready=$(ConvertTo-CounterBit $metalAggregateReady)',
        "environment_metal_aggregate_native_handle_access=0",
        "environment_metal_aggregate_diagnostics=0",
        '$missingExpectedCounters',
        'Apple Metal platform evidence is missing expected actual counters')) {
    if (-not $appleMetalPlatformToolText.Contains($needle)) {
        Write-Error "tools/validate-apple-metal-platform-host.ps1 missing strict iOS/aggregate evidence guard: $needle"
    }
}
$backendParityClaim = $environmentCommercialClaimsById["environment_backend_parity_ready"]
if ($null -eq $backendParityClaim -or
    @($backendParityClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-backend-parity" -or
    @($backendParityClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-backend-parity-ready" -or
    @($backendParityClaim.validationRecipeIds) -notcontains "renderer-metal-environment-aggregate-apple-host-evidence" -or
    @($backendParityClaim.validationRecipeIds) -notcontains "environment-backend-parity-v2-closeout" -or
    [string]$backendParityClaim.state -ne "ready" -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("plan_environment_backend_parity") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("EnvironmentBackendParityV2Row") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("evaluate_environment_backend_parity_v2") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("environment_backend_parity_ready=1") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("environment_backend_parity_required_features=15") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("environment_backend_parity_rows=45") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("environment_backend_parity_ready_rows=45") -or
    -not [string]$backendParityClaim.notes.Contains("desktop-runtime-sample-game-environment-backend-parity-ready") -or
    -not [string]$backendParityClaim.notes.Contains("environment-backend-parity-v2-closeout") -or
    -not [string]$backendParityClaim.notes.Contains("environment_backend_parity_metal_host=1") -or
    -not [string]$backendParityClaim.notes.Contains("environment_backend_parity_ready=1") -or
    -not [string]$backendParityClaim.notes.Contains("environment_backend_parity_ready=0")) {
    Write-Error "engine manifest environment_backend_parity_ready must be promoted only by the retained MK_renderer parity matrix plus the clean-break environment backend parity v2 closeout recipe"
}
$windowsVulkanPlatformClaim = $environmentCommercialClaimsById["environment_platform_windows_vulkan_ready"]
if ($null -eq $windowsVulkanPlatformClaim -or
    [string]$windowsVulkanPlatformClaim.state -ne "ready" -or
    @($windowsVulkanPlatformClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence" -or
    -not [string]$windowsVulkanPlatformClaim.requiredEvidence.Contains("environment_platform_windows_vulkan_ready=1") -or
    -not [string]$windowsVulkanPlatformClaim.notes.Contains("environment_platform_windows_vulkan_evidence_requested=1") -or
    -not [string]$windowsVulkanPlatformClaim.notes.Contains("environment_platform_readiness_ready=0")) {
    Write-Error "engine manifest environment_platform_windows_vulkan_ready must be ready only through the Windows strict Vulkan platform evidence bridge without all-platform promotion"
}
$linuxVulkanPlatformClaim = $environmentCommercialClaimsById["environment_platform_linux_vulkan_ready"]
if ($null -eq $linuxVulkanPlatformClaim -or
    [string]$linuxVulkanPlatformClaim.state -ne "ready" -or
    @($linuxVulkanPlatformClaim.validationRecipeIds) -notcontains "environment-platform-linux-vulkan-host-gate" -or
    @($linuxVulkanPlatformClaim.validationRecipeIds) -notcontains "environment-platform-linux-vulkan-package" -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("environment-platform-linux-vulkan-host-gate") -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("environment-platform-linux-vulkan-package") -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("first-party Linux desktop/runtime host") -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("Linux Vulkan ICD/runtime/driver") -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("VK_LAYER_KHRONOS_validation") -or
    -not [string]$linuxVulkanPlatformClaim.requiredEvidence.Contains("spirv-val") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("validate.yml linux-vulkan") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("xvfb-run") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("linux_package_smoke_ready=1") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("linux_vulkan_readback_ready=1") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("linux_vulkan_validation_log_clean=1") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("environment_platform_requires_linux_vulkan_host_evidence=0") -or
    -not [string]$linuxVulkanPlatformClaim.notes.Contains("Win32/x64-windows")) {
    Write-Error "engine manifest environment_platform_linux_vulkan_ready must be promoted only through the dedicated hosted Linux Vulkan package gate without accepting Windows Vulkan or Win32 package evidence"
}
$broadOptimizationClaim = $environmentCommercialClaimsById["environment_broad_optimization_ready"]
if ($null -eq $broadOptimizationClaim -or
    @($broadOptimizationClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-optimization-measurement" -or
    @($broadOptimizationClaim.validationRecipeIds) -notcontains "environment-broad-optimization-cross-backend-measurement" -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("plan_environment_optimization_measurement") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("artifacts/environment/optimization/<task-id>/<backend>/<workload>/") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("21 CPU/GPU/memory/upload/barrier/shader-cache/stutter before/after rows") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("SHA-256 verified profiler artifacts") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("zero escaped paths") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("before/after traces") -or
    -not [string]$broadOptimizationClaim.notes.Contains("tools/validate-environment-optimization-artifacts.ps1") -or
    [string]$broadOptimizationClaim.state -ne "ready" -or
    -not [string]$broadOptimizationClaim.notes.Contains("artifacts/environment/optimization/2026-06-19-metal-host-xctrace-smoke/metal_apple_host/<workload>/") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_missing_artifacts=0") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_workload_rows=21") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_backend_rows=3") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_profiler_artifacts=21") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_trace_event_json=21") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_invalid_hashes=0") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_optimization_measurement_over_budget=0") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_broad_optimization_ready=1")) {
    Write-Error "engine manifest environment_broad_optimization_ready must require the MK_renderer optimization measurement contract plus all 21 retained D3D12, strict Vulkan, and Apple-host Metal artifact rows before ready promotion"
}
$environmentOptimizationArtifactToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-environment-optimization-artifacts.ps1") -Raw
foreach ($needle in @(
        "artifacts/environment/optimization",
        "preset_pack_flythrough",
        "storm_precipitation",
        "dense_volumetric_fog",
        "volumetric_cloud_sunset",
        "snowfield_material_weathering",
        "weather_simulation_stress",
        "asset_library_cold_load",
        "d3d12",
        "vulkan_strict",
        "metal_apple_host",
        "cpu_frame_p95_before_us",
        "gpu_frame_p95_before_us",
        "gpu_timestamp_ticks_per_second",
        "profiler_artifact_path",
        "trace_event_json_path",
        "artifact_hash_sha256",
        "environment_optimization_measurement_missing_artifacts",
        "environment_optimization_measurement_invalid_hashes",
        "environment_optimization_measurement_path_escapes",
        "environment_optimization_measurement_over_budget",
        "environment_broad_optimization_ready")) {
    if (-not $environmentOptimizationArtifactToolText.Contains($needle)) {
        Write-Error "tools/validate-environment-optimization-artifacts.ps1 missing retained artifact validator needle: $needle"
    }
}
$environmentAssetPipelineFullRecipe = $environmentCommercialValidationRecipesByName["environment-asset-pipeline-openexr-ktx-basis-full"]
if ($null -eq $environmentAssetPipelineFullRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-asset-pipeline-openexr-ktx-basis-full"
} else {
    $environmentAssetPipelineFullRecipeText = [string]::Join(" ", @([string]$environmentAssetPipelineFullRecipe.command, [string]$environmentAssetPipelineFullRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-asset-pipeline-full.ps1",
            "-RequireReady",
            "MK_environment_texture_pipeline_v2_tests",
            "asset-importers",
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
        if (-not $environmentAssetPipelineFullRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-asset-pipeline-openexr-ktx-basis-full' missing full asset-pipeline needle: $needle"
        }
    }
    if ($environmentAssetPipelineFullRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-asset-pipeline-openexr-ktx-basis-full' must not remain a dry-run skeleton after Task 9"
    }
}
$environmentAssetPipelineFullToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-environment-asset-pipeline-full.ps1") -Raw
foreach ($needle in @(
        "MK_environment_texture_pipeline_v2_tests",
        "tools/build-asset-importers.ps1",
        "tools/bootstrap-deps.ps1",
        "Missing asset-importers vcpkg packages",
        "direct vcpkg install and CMake configure-time install are not supported",
        "environment_asset_pipeline_openexr_ktx_basis_full_ready=1",
        "environment_asset_pipeline_required_rows=14",
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
        "environment_asset_pipeline_runtime_optional_codec_execution=0",
        "environment_asset_pipeline_native_handle_access=0",
        "environment_asset_pipeline_gpu_command_executed=0",
        "environment_asset_pipeline_package_command_executed=0",
        "environment_asset_pipeline_cmake_configure_dependency_install=0",
        "environment_ready=0",
        "environment_commercial_ready=0")) {
    if (-not $environmentAssetPipelineFullToolText.Contains($needle)) {
        Write-Error "tools/validate-environment-asset-pipeline-full.ps1 missing full asset-pipeline validator needle: $needle"
    }
}
$environmentPhysicalWeatherRecipe = $environmentCommercialValidationRecipesByName["environment-physical-weather-simulation-closeout"]
if ($null -eq $environmentPhysicalWeatherRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-physical-weather-simulation-closeout"
} else {
    $environmentPhysicalWeatherRecipeText = [string]::Join(" ", @([string]$environmentPhysicalWeatherRecipe.command, [string]$environmentPhysicalWeatherRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-weather-physics.ps1",
            "-RequireReady",
            "MK_environment_weather_simulation_tests",
            "MK_d3d12_environment_weather_solver_tests",
            "MK_vulkan_environment_weather_solver_tests",
            "MK_metal_environment_weather_solver_tests",
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
        if (-not $environmentPhysicalWeatherRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-physical-weather-simulation-closeout' missing physical weather closeout needle: $needle"
        }
    }
    if ($environmentPhysicalWeatherRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-physical-weather-simulation-closeout' must not remain a dry-run skeleton after Task 10"
    }
}
$environmentArtistWorkflowProductionRecipe = $environmentCommercialValidationRecipesByName["environment-artist-workflow-production-closeout"]
if ($null -eq $environmentArtistWorkflowProductionRecipe) {
    Write-Error "engine manifest validationRecipes missing environment-artist-workflow-production-closeout"
} else {
    $environmentArtistWorkflowProductionRecipeText = [string]::Join(" ", @([string]$environmentArtistWorkflowProductionRecipe.command, [string]$environmentArtistWorkflowProductionRecipe.purpose))
    foreach ($needle in @(
            "tools/validate-environment-artist-workflow-production.ps1",
            "-RequireReady",
            "MK_editor_environment_tests",
            "sample_desktop_runtime_game",
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
        if (-not $environmentArtistWorkflowProductionRecipeText.Contains($needle)) {
            Write-Error "engine manifest validationRecipes 'environment-artist-workflow-production-closeout' missing production workflow closeout needle: $needle"
        }
    }
    if ($environmentArtistWorkflowProductionRecipeText.Contains("validation_recipe_skeleton=1")) {
        Write-Error "engine manifest validationRecipes 'environment-artist-workflow-production-closeout' must not remain a dry-run skeleton after Task 11"
    }
}
$environmentPhysicalWeatherToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-environment-weather-physics.ps1") -Raw
foreach ($needle in @(
        "MK_environment_weather_simulation_tests",
        "MK_d3d12_environment_weather_solver_tests",
        "MK_vulkan_environment_weather_solver_tests",
        "MK_metal_environment_weather_solver_tests",
        "environment_physical_weather_simulation_ready=1",
        "environment_weather_simulation_backend_parity_ready=1",
        "environment_weather_simulation_coupled_field_rows=13",
        "environment_weather_simulation_canonical_dataset_rows=12",
        "environment_weather_simulation_dataset_provenance_rows=12",
        "environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12",
        "environment_weather_simulation_canonical_image_rows=12",
        "environment_weather_simulation_backend_solver_rows=3",
        "environment_weather_simulation_mass_conservation_relative_error_max=0.001",
        "environment_weather_simulation_energy_or_stability_error_max=0.002",
        "environment_weather_simulation_solver_budget_overages=0",
        "environment_weather_simulation_visual_regression_failures=0",
        "environment_weather_simulation_validation_failures=0",
        "environment_weather_simulation_backend_inference=0",
        "environment_weather_simulation_native_handle_access=0",
        "environment_ready=0",
        "environment_commercial_ready=0")) {
    if (-not $environmentPhysicalWeatherToolText.Contains($needle)) {
        Write-Error "tools/validate-environment-weather-physics.ps1 missing physical weather validator needle: $needle"
    }
}
$environmentArtistWorkflowProductionToolText = Get-Content -LiteralPath (Join-Path $root "tools/validate-environment-artist-workflow-production.ps1") -Raw
foreach ($needle in @(
        "MK_editor_environment_tests",
        "tools/package-desktop-runtime.ps1",
        "--require-environment-artist-workflow-package",
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
    if (-not $environmentArtistWorkflowProductionToolText.Contains($needle)) {
        Write-Error "tools/validate-environment-artist-workflow-production.ps1 missing production workflow validator needle: $needle"
    }
}
$assetPipelineClaim = $environmentCommercialClaimsById["environment_asset_pipeline_openexr_ktx_basis_ready"]
if ($null -eq $assetPipelineClaim -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "asset-importers" -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-asset-pipeline-openexr-ktx-basis-ready" -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-texture-asset-pipeline-metal-compressed-upload" -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "agent-contract" -or
    [string]$assetPipelineClaim.state -ne "ready" -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("EnvironmentTexturePayloadCookRequestV1") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("cook_environment_texture_payload_v1") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("TextureCookBackendDecisionV1") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("payload_transcode_target") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("format_support_evidence_id") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("official_format_support_api") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("RuntimeEnvironmentTextureUploadExecutionResult") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("execute_runtime_environment_texture_payload_upload") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("selected strict Vulkan upload/readback") -or
    -not [string]$assetPipelineClaim.requiredEvidence.Contains("Apple-host Metal upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("EnvironmentTexturePayloadCookResultV1") -or
    -not [string]$assetPipelineClaim.notes.Contains("EnvironmentTexturePayloadCookDiagnostic") -or
    -not [string]$assetPipelineClaim.notes.Contains("has_environment_texture_payload_cook_diagnostic") -or
    -not [string]$assetPipelineClaim.notes.Contains("GameEngine.CookedTextureMetadata.v1") -or
    -not [string]$assetPipelineClaim.notes.Contains("payload_transcode_target") -or
    -not [string]$assetPipelineClaim.notes.Contains("format_support_evidence_id") -or
    -not [string]$assetPipelineClaim.notes.Contains("official_format_support_api") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-texture-asset-pipeline-d3d12-upload") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-texture-asset-pipeline-vulkan-upload") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-texture-asset-pipeline-d3d12-compressed-upload") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-texture-asset-pipeline-vulkan-compressed-upload") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-texture-asset-pipeline-metal-compressed-upload") -or
    -not [string]$assetPipelineClaim.notes.Contains("--require-environment-asset-pipeline-openexr-ktx-basis-ready") -or
    -not [string]$assetPipelineClaim.notes.Contains("environment_asset_pipeline_openexr_ktx_basis_ready=1") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected D3D12 WARP runtime upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected strict Vulkan runtime upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected D3D12 WARP backend-target BC7 compressed upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected strict Vulkan backend-target BC7 compressed upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected Apple-host Metal ASTC compressed package upload/readback counters") -or
    -not [string]$assetPipelineClaim.notes.Contains("MTLBlitCommandEncoder copyFromBuffer") -or
    -not [string]$assetPipelineClaim.notes.Contains("runtime optional codec execution remains unsupported")) {
    Write-Error "engine manifest environment_asset_pipeline_openexr_ktx_basis_ready must record the ready selected source-to-package cooker, backend-target decision fields, selected D3D12/Vulkan RGBA8 upload/readback evidence, selected D3D12/Vulkan BC7 compressed upload/readback evidence, selected Apple-host Metal ASTC compressed package evidence, canonical closeout counter, and remaining non-claims"
}
foreach ($broadClaimId in @("environment_commercial_ready", "environment_backend_parity_ready", "environment_broad_optimization_ready", "environment_aaa_preset_library_ready", "environment_physical_weather_simulation_ready", "environment_artist_workflow_ready", "environment_artist_workflow_production_ready")) {
    $broadClaim = $environmentCommercialClaimsById[$broadClaimId]
    if ($null -ne $broadClaim -and [string]$broadClaim.state -eq "ready") {
        foreach ($dependencyId in @($broadClaim.dependsOn)) {
            if ([string]$environmentCommercialClaimsById[[string]$dependencyId].state -ne "ready") {
                Write-Error "engine manifest environmentCommercialClaimMatrix '$broadClaimId' must not be ready before dependency '$dependencyId' is ready"
            }
        }
    }
}

$expectedEnvironmentPlatformReadinessRows = @(
    @{
        id = "environment_platform_windows_d3d12"
        claimId = "environment_platform_windows_d3d12_ready"
        state = "ready"
        needles = @("Windows SDK", "Direct3D 12", "--require-environment-platform-readiness", "environment_platform_windows_d3d12_ready=1", "Vulkan", "Metal", "Android", "iOS")
    },
    @{
        id = "environment_platform_windows_vulkan"
        claimId = "environment_platform_windows_vulkan_ready"
        state = "ready"
        needles = @("Vulkan SDK", "validation layers", "SPIR-V", "D3D12", "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence", "environment_platform_windows_vulkan_evidence_requested=1", "environment_platform_windows_vulkan_strict_aggregate_ready=1", "environment_platform_windows_vulkan_ready=1", "environment_platform_readiness_ready=0")
    },
    @{
        id = "environment_platform_linux_vulkan"
        claimId = "environment_platform_linux_vulkan_ready"
        state = "ready"
        needles = @("Linux Vulkan", "Windows Vulkan evidence", "Vulkan SDK", "validation layers", "first-party Linux desktop/runtime host", "Win32/x64-windows", "environment-platform-linux-vulkan-host-gate", "tools/validate-linux-vulkan-runtime-host.ps1", "environment-platform-linux-vulkan-package", "linux-vulkan-runtime-host", "vulkan-strict-linux", "validate.yml linux-vulkan", "xvfb-run", "vulkaninfo_ready=1", "VK_LAYER_KHRONOS_validation_ready=1", "dxc_spirv_codegen_ready=1", "spirv_val_ready=1", "linux_icd_runtime_ready=1", "first_party_linux_runtime_host_ready=1", "linux_package_script_ready=1", "linux_installed_validator_ready=1", "linux_package_smoke_ready=1", "linux_vulkan_readback_ready=1", "linux_vulkan_validation_log_clean=1", "VK_EXT_debug_utils", "validation_log_snapshot", "environment_platform_linux_vulkan_ready=1", "environment_platform_requires_linux_vulkan_host_evidence=0", "environment_platform_windows_vulkan_inferred=0")
    },
    @{
        id = "environment_platform_macos_metal"
        claimId = "environment_platform_macos_metal_ready"
        state = "ready"
        needles = @("Xcode", "Metal", "renderer-metal-environment-aggregate-apple-host-evidence", "environment_platform_macos_metal_evidence_requested=1", "environment_platform_macos_metal_ready=1", "environment_platform_readiness_ready=0")
    },
    @{
        id = "environment_platform_ios_metal"
        claimId = "environment_platform_ios_metal_ready"
        state = "ready"
        needles = @("Xcode", "simulator", "signing", "tools/validate-apple-metal-platform-host.ps1", "ios_metal_command_buffer_ready=1", "environment_platform_ios_metal_ready=1", "environment_platform_requires_ios_metal_host_evidence=0", "environment_platform_readiness_ready=0")
    },
    @{
        id = "environment_platform_android_vulkan"
        claimId = "environment_platform_android_vulkan_ready"
        state = "host-gated"
        needles = @("Android SDK", "NDK", "Vulkan", "desktop Vulkan", "tools/validate-android-vulkan-runtime-host.ps1", "environment-platform-android-vulkan-package", "android-vulkan-runtime-host", "host_has_android_sdk=1", "host_has_android_ndk=1", "adb_device_or_emulator_ready=1", "android_vulkan_profile_ready=1", "android_gpu_debuggable_ready=1", "android_gpu_debug_layer_settings_ready=1", "android_gpu_debug_layer_app_installed=1", "android_gpu_debug_layer_install_requested=1", "android_gpu_debug_layer_install_ready=1", "VK_LAYER_KHRONOS_validation_ready=1", "android_package_smoke_ready=1", "android_vulkan_readback_ready=1", "android_vulkan_validation_layer_enumerated=1", "android_vulkan_validation_log_clean=1", "environment_platform_android_vulkan_ready=0", "environment_platform_android_vulkan_ready=1", "environment_platform_requires_android_vulkan_host_evidence=0")
    },
    @{
        id = "environment_platform_unconditional_all_platform"
        claimId = "environment_unconditional_all_platform_parity_ready"
        state = "unsupported"
        needles = @("permanent non-claim", "environment_all_platform_unconditional_ready=0", "all-platform")
    }
)
$environmentPlatformReadinessRows = @($environmentCommercialLoop.environmentPlatformReadinessRows)
if ($environmentPlatformReadinessRows.Count -ne $expectedEnvironmentPlatformReadinessRows.Count) {
    Write-Error "engine manifest environmentPlatformReadinessRows must contain exactly $($expectedEnvironmentPlatformReadinessRows.Count) rows"
}
$environmentPlatformReadinessRowsById = @{}
foreach ($platformRow in $environmentPlatformReadinessRows) {
    Assert-Properties $platformRow @("id", "claimId", "state", "hostOs", "backend", "runtimeTarget", "runtimeSample", "requiredSdks", "requiredCompilers", "requiredValidationTools", "validationRecipeIds", "featureSet", "packageSmoke", "packageCounters", "hostGate", "blockerReason", "forbiddenInference", "notes") "engine manifest environmentPlatformReadinessRows"
    $platformRowId = [string]$platformRow.id
    if ($environmentPlatformReadinessRowsById.ContainsKey($platformRowId)) {
        Write-Error "engine manifest environmentPlatformReadinessRows duplicate row id: $platformRowId"
    }
    $environmentPlatformReadinessRowsById[$platformRowId] = $platformRow
    if (-not $environmentCommercialClaimsById.ContainsKey([string]$platformRow.claimId) -and
        [string]$platformRow.claimId -ne "environment_unconditional_all_platform_parity_ready") {
        Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' references unknown claim id: $($platformRow.claimId)"
    }
    foreach ($arrayProperty in @("requiredSdks", "requiredCompilers", "requiredValidationTools", "validationRecipeIds", "featureSet", "packageCounters")) {
        if (@($platformRow.$arrayProperty).Count -eq 0) {
            Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' must have at least one $arrayProperty row"
        }
    }
    foreach ($recipeId in @($platformRow.validationRecipeIds)) {
        if (-not $environmentCommercialValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' references unknown validation recipe: $recipeId"
        }
    }
    if ([string]$platformRow.state -eq "ready" -and [string]$platformRow.blockerReason -ne "none") {
        Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' ready rows must use blockerReason=none"
    }
    if ([string]$platformRow.state -ne "ready" -and [string]$platformRow.blockerReason -eq "none") {
        Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' non-ready rows must record a blockerReason"
    }
}
foreach ($expectedPlatformRow in $expectedEnvironmentPlatformReadinessRows) {
    $platformRowId = [string]$expectedPlatformRow.id
    if (-not $environmentPlatformReadinessRowsById.ContainsKey($platformRowId)) {
        Write-Error "engine manifest environmentPlatformReadinessRows missing row id: $platformRowId"
        continue
    }
    $platformRow = $environmentPlatformReadinessRowsById[$platformRowId]
    if ([string]$platformRow.claimId -ne [string]$expectedPlatformRow.claimId) {
        Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' must map to $($expectedPlatformRow.claimId), not $($platformRow.claimId)"
    }
    if ([string]$platformRow.state -ne [string]$expectedPlatformRow.state) {
        Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' must remain $($expectedPlatformRow.state), not $($platformRow.state)"
    }
    $platformRowText = [string]::Join(" ", @($platformRow.requiredSdks) + @($platformRow.requiredCompilers) + @($platformRow.requiredValidationTools) + @($platformRow.validationRecipeIds) + @($platformRow.featureSet) + @($platformRow.packageCounters) + @([string]$platformRow.packageSmoke, [string]$platformRow.hostGate, [string]$platformRow.blockerReason, [string]$platformRow.forbiddenInference, [string]$platformRow.notes))
    foreach ($needle in @($expectedPlatformRow.needles)) {
        if (-not $platformRowText.Contains([string]$needle)) {
            Write-Error "engine manifest environmentPlatformReadinessRows '$platformRowId' missing official/host-gated needle: $needle"
        }
    }
}

$expectedEnvironmentOptimizationRows = @(
    @{
        id = "environment_optimization_preset_pack_flythrough_d3d12"
        workload = "preset_pack_flythrough"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "environment_optimization_preset_pack_flythrough_ready=1", "environment_optimization_measurement_before_after_pairs=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_storm_precipitation_d3d12"
        workload = "storm_precipitation"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "storm precipitation", "environment_optimization_storm_precipitation_ready=1", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_dense_volumetric_fog_d3d12"
        workload = "dense_volumetric_fog"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "dense volumetric fog", "environment_optimization_dense_volumetric_fog_ready=1", "environment_optimization_dense_volumetric_fog_cpu_frame_p95_before_us=17600", "environment_optimization_dense_volumetric_fog_gpu_frame_p95_after_us=15000", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_volumetric_cloud_sunset_d3d12"
        workload = "volumetric_cloud_sunset"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "volumetric cloud", "sunset", "environment_optimization_volumetric_cloud_sunset_ready=1", "environment_optimization_volumetric_cloud_sunset_cpu_frame_p95_before_us=18800", "environment_optimization_volumetric_cloud_sunset_gpu_frame_p95_after_us=16200", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_snowfield_material_weathering_d3d12"
        workload = "snowfield_material_weathering"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "snowfield", "material weathering", "environment_optimization_snowfield_material_weathering_ready=1", "environment_optimization_snowfield_material_weathering_cpu_frame_p95_before_us=16900", "environment_optimization_snowfield_material_weathering_gpu_frame_p95_after_us=14600", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_weather_simulation_stress_d3d12"
        workload = "weather_simulation_stress"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "weather simulation", "stress", "environment_optimization_weather_simulation_stress_ready=1", "environment_optimization_weather_simulation_stress_cpu_frame_p95_before_us=19600", "environment_optimization_weather_simulation_stress_gpu_frame_p95_after_us=16900", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    },
    @{
        id = "environment_optimization_asset_library_cold_load_d3d12"
        workload = "asset_library_cold_load"
        state = "ready"
        needles = @("desktop-runtime-sample-game-environment-optimization-measurement", "Windows Performance Recorder", "PIX Timing Capture", "D3D12 timestamp query", "asset-library cold-load", "package_load", "before/after", "environment_optimization_asset_library_cold_load_ready=1", "environment_optimization_asset_library_cold_load_package_load_before_us=98000", "environment_optimization_asset_library_cold_load_package_load_after_us=78000", "environment_optimization_measurement_workload_rows=7", "environment_optimization_measurement_before_after_pairs=7", "environment_optimization_measurement_regression_budget_rows=7", "environment_broad_optimization_ready=0")
    }
)
$environmentOptimizationRows = @($environmentCommercialLoop.environmentOptimizationMeasurementWorkloadRows)
if ($environmentOptimizationRows.Count -ne $expectedEnvironmentOptimizationRows.Count) {
    Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows must contain exactly $($expectedEnvironmentOptimizationRows.Count) rows"
}
$environmentOptimizationRowsById = @{}
foreach ($optimizationRow in $environmentOptimizationRows) {
    Assert-Properties $optimizationRow @("id", "claimId", "state", "backend", "workload", "runtimeTarget", "validationRecipeIds", "requiredMetrics", "requiredTools", "packageSmoke", "packageCounters", "regressionBudget", "blockerReason", "forbiddenInference", "notes") "engine manifest environmentOptimizationMeasurementWorkloadRows"
    $optimizationRowId = [string]$optimizationRow.id
    if ($environmentOptimizationRowsById.ContainsKey($optimizationRowId)) {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows duplicate row id: $optimizationRowId"
    }
    $environmentOptimizationRowsById[$optimizationRowId] = $optimizationRow
    if ([string]$optimizationRow.claimId -ne "environment_broad_optimization_ready") {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' must map to environment_broad_optimization_ready"
    }
    if ([string]$optimizationRow.backend -ne "d3d12") {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' must remain d3d12 for Phase 9 slice 1"
    }
    foreach ($arrayProperty in @("validationRecipeIds", "requiredMetrics", "requiredTools", "packageCounters")) {
        if (@($optimizationRow.$arrayProperty).Count -eq 0) {
            Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' must have at least one $arrayProperty row"
        }
    }
    foreach ($recipeId in @($optimizationRow.validationRecipeIds)) {
        if (-not $environmentCommercialValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' references unknown validation recipe: $recipeId"
        }
    }
    if ([string]$optimizationRow.state -eq "ready" -and [string]$optimizationRow.blockerReason -ne "none") {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' ready rows must use blockerReason=none"
    }
    if ([string]$optimizationRow.state -ne "ready" -and [string]$optimizationRow.blockerReason -eq "none") {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' non-ready rows must record a blockerReason"
    }
}
foreach ($expectedOptimizationRow in $expectedEnvironmentOptimizationRows) {
    $optimizationRowId = [string]$expectedOptimizationRow.id
    if (-not $environmentOptimizationRowsById.ContainsKey($optimizationRowId)) {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows missing row id: $optimizationRowId"
        continue
    }
    $optimizationRow = $environmentOptimizationRowsById[$optimizationRowId]
    if ([string]$optimizationRow.workload -ne [string]$expectedOptimizationRow.workload) {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' must map to workload $($expectedOptimizationRow.workload), not $($optimizationRow.workload)"
    }
    if ([string]$optimizationRow.state -ne [string]$expectedOptimizationRow.state) {
        Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' must remain $($expectedOptimizationRow.state), not $($optimizationRow.state)"
    }
    $optimizationRowText = [string]::Join(" ", @($optimizationRow.validationRecipeIds) + @($optimizationRow.requiredMetrics) + @($optimizationRow.requiredTools) + @($optimizationRow.packageCounters) + @([string]$optimizationRow.packageSmoke, [string]$optimizationRow.regressionBudget, [string]$optimizationRow.blockerReason, [string]$optimizationRow.forbiddenInference, [string]$optimizationRow.notes))
    foreach ($needle in @($expectedOptimizationRow.needles)) {
        if (-not $optimizationRowText.Contains([string]$needle)) {
            Write-Error "engine manifest environmentOptimizationMeasurementWorkloadRows '$optimizationRowId' missing measurement needle: $needle"
        }
    }
}

$expectedEnvironmentPhysicalWeatherRows = @(
    @{
        id = "environment_weather_simulation_cpu_reference_foundation"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("single-step deterministic", "MK_environment", "NWS vapor-pressure formula", "EnvironmentWeatherSimulationDesc", "EnvironmentWeatherSimulationCellState", "EnvironmentWeatherSimulationCellForcing", "EnvironmentWeatherSimulationPlan", "environment_weather_saturation_vapor_kg_per_m2", "simulate_environment_weather_cpu_reference", "MK_environment_weather_simulation_tests", "water conservation error bounds", "timestep clamping", "deterministic replay hash", "environment_physical_weather_simulation_ready stays 0", "GPU acceleration", "backend execution", "native handle access", "artist controls")
    },
    @{
        id = "environment_weather_simulation_package_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("desktop-runtime-sample-game-environment-weather-simulation-package", "--require-environment-weather-simulation-package", "environment_weather_simulation_package_status=ready", "environment_weather_simulation_package_ready=1", "environment_weather_simulation_steps=1", "environment_weather_simulation_cells=4", "environment_weather_simulation_effective_timestep_ms=500", "environment_weather_simulation_water_conservation_error_mg<=1", "environment_weather_simulation_water_conservation_error_bound_mg=1", "environment_weather_simulation_fallback_cpu_reference_used=1", "environment_weather_simulation_invokes_gpu=0", "environment_weather_simulation_invokes_backend=0", "environment_weather_simulation_native_handle_access=0", "environment_physical_weather_simulation_ready=0", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_solver_budget_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverBudgetDesc", "EnvironmentWeatherSimulationSolverBudgetPlan", "EnvironmentWeatherSimulationSolverBudgetStatus::host_evidence_required", "plan_environment_weather_simulation_solver_budget", "environment_weather_simulation_solver_budget_status=host_evidence_required", "environment_weather_simulation_solver_budget_ready=0", "environment_weather_simulation_cpu_reference_solver_ready=1", "environment_weather_simulation_solver_cpu_elapsed_us", "environment_weather_simulation_solver_cpu_budget_us=50000", "environment_weather_simulation_solver_cpu_over_budget=0", "environment_weather_simulation_gpu_solver_ready=1", "environment_weather_simulation_solver_gpu_elapsed_us", "environment_weather_simulation_solver_gpu_budget_us=500000", "environment_weather_simulation_solver_profiler_artifacts=2", "environment_weather_simulation_profiler_budget_ready=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_solver_budget_diagnostics=0", "environment_physical_weather_simulation_ready=0", "retained profiler artifact budget", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_d3d12_gpu_solver_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("D3d12EnvironmentWeatherSolverDesc", "D3d12EnvironmentWeatherSolverResult", "dispatch_environment_weather_solver", "MK_d3d12_environment_weather_solver_tests", "desktop-runtime-sample-game-environment-weather-simulation-package", "environment_weather_simulation_d3d12_gpu_solver_ready=1", "environment_weather_simulation_d3d12_gpu_solver_cells=4", "environment_weather_simulation_d3d12_gpu_solver_dispatches=1", "environment_weather_simulation_d3d12_gpu_solver_barriers", "environment_weather_simulation_d3d12_gpu_solver_native_handle_access=0", "environment_weather_simulation_d3d12_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_d3d12_gpu_solver_failure_stage=0", "environment_weather_simulation_d3d12_gpu_solver_hash", "environment_weather_simulation_gpu_solver_ready=1", "environment_weather_simulation_solver_gpu_budget_us=500000", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend parity", "retained profiler artifacts", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_strict_vulkan_gpu_solver_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("VulkanEnvironmentWeatherSolverDesc", "VulkanEnvironmentWeatherSolverResult", "dispatch_environment_weather_solver", "MK_vulkan_environment_weather_solver_tests", "shaders/environment/weather_simulation.hlsl", "desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package", "--require-environment-weather-simulation-vulkan-solver-package", "DXC -spirv -fspv-target-env=vulkan1.3", "spirv-val", "Vulkan 1.3 synchronization2", "descriptor_set_bindings=3", "environment_weather_simulation_vulkan_gpu_solver_ready=1", "environment_weather_simulation_vulkan_gpu_solver_strict_ready=1", "environment_weather_simulation_vulkan_gpu_solver_cells=4", "environment_weather_simulation_vulkan_gpu_solver_dispatches=1", "environment_weather_simulation_vulkan_gpu_solver_descriptor_set_bindings=3", "environment_weather_simulation_vulkan_gpu_solver_barriers", "environment_weather_simulation_vulkan_gpu_solver_native_handle_access=0", "environment_weather_simulation_vulkan_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_vulkan_gpu_solver_d3d12_inferred=0", "environment_weather_simulation_vulkan_gpu_solver_metal_inferred=0", "environment_weather_simulation_vulkan_gpu_solver_failure_stage=0", "environment_weather_simulation_vulkan_gpu_solver_hash", "environment_weather_simulation_vulkan_gpu_solver_elapsed_us", "environment_weather_simulation_vulkan_gpu_solver_budget_us=500000", "environment_weather_simulation_vulkan_gpu_solver_over_budget=0", "environment_weather_simulation_vulkan_gpu_solver_profiler_budget_ready=1", "environment_physical_weather_simulation_ready=0", "Metal", "D3D12 inference", "backend parity", "production solver readiness", "all-platform readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_metal_gpu_solver_host_gate"
        claimId = "environment_physical_weather_simulation_ready"
        state = "host-gated"
        determinism = "deterministic"
        needles = @("MetalEnvironmentWeatherSolverDesc", "MetalEnvironmentWeatherSolverResult", "metal_environment_weather_solver_cell_row_stride_bytes", "dispatch_environment_weather_solver", "MK_metal_environment_weather_solver_tests", "engine/rhi/metal/shaders/weather_simulation.metal", "environment-weather-metal-solver-host-gate", "environment_weather_simulation_metal_gpu_solver_ready=1", "environment_weather_simulation_metal_gpu_solver_host_validation_recipe_id=environment-weather-metal-solver-host-gate", "environment_weather_simulation_metal_gpu_solver_selected_backend=metal", "environment_weather_simulation_metal_gpu_solver_cells=4", "environment_weather_simulation_metal_gpu_solver_dispatches=1", "environment_weather_simulation_metal_gpu_solver_buffer_bindings=3", "environment_weather_simulation_metal_gpu_solver_command_queue_ready=1", "environment_weather_simulation_metal_gpu_solver_command_buffer_ready=1", "environment_weather_simulation_metal_gpu_solver_metallib_valid=1", "environment_weather_simulation_metal_gpu_solver_compute_pipeline_ready=1", "environment_weather_simulation_metal_gpu_solver_native_handle_access=0", "environment_weather_simulation_metal_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_metal_gpu_solver_d3d12_inferred=0", "environment_weather_simulation_metal_gpu_solver_vulkan_inferred=0", "environment_weather_simulation_metal_gpu_solver_failure_stage=0", "environment_weather_simulation_metal_gpu_solver_hash", "environment_weather_simulation_metal_gpu_solver_elapsed_us", "environment_weather_simulation_metal_gpu_solver_budget_us=500000", "environment_weather_simulation_metal_gpu_solver_over_budget=0", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_physical_weather_ready=0", "environment_physical_weather_simulation_ready=0", "host-gated on macOS/Xcode/Metal", "Metal aggregate readiness", "D3D12", "Vulkan", "backend parity", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_profiler_artifact_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverProfilerArtifactRow", "missing_profiler_artifacts", "invalid_profiler_artifact", "PIX", "WPR", "D3D12 timestamp", "out/performance/sample_desktop_runtime_game", "environment_weather_simulation_solver_profiler_artifacts=2", "environment_weather_simulation_solver_profiler_tool_rows=2", "environment_weather_simulation_solver_profiler_backend_rows=1", "environment_weather_simulation_solver_profiler_artifact_hash", "environment_weather_simulation_profiler_budget_ready=1", "environment_weather_simulation_production_solver_ready=0", "environment_physical_weather_simulation_ready=0", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_production_solver_package_counter_review"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverBudgetDesc", "production_solver_package_counter_reviewed", "production_solver_package_counter_review_ready", "production_solver_package_counter_rows", "missing_production_solver_package_evidence", "environment_weather_simulation_production_solver_package_counter_review_ready=1", "environment_weather_simulation_production_solver_package_counter_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend parity", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_production_solver_core_review"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverBudgetDesc", "validation_dataset_ready", "validation_images_ready", "artist_controls_ready", "production_solver_core_reviewed", "production_solver_core_review_ready", "production_solver_core_rows", "missing_production_solver_core_evidence", "environment_weather_simulation_production_solver_core_review_ready=1", "environment_weather_simulation_production_solver_core_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend parity", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_broad_physical_accuracy_review"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverBudgetDesc", "broad_physical_accuracy_reviewed", "broad_physical_accuracy_review_ready", "broad_physical_accuracy_rows", "missing_broad_physical_accuracy_evidence", "environment_weather_simulation_broad_physical_accuracy_review_ready=1", "environment_weather_simulation_broad_physical_accuracy_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_physical_weather_ready=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend parity", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_visual_quality_review"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationSolverBudgetDesc", "visual_quality_reviewed", "visual_quality_review_ready", "visual_quality_rows", "missing_visual_quality_evidence", "environment_weather_simulation_visual_quality_review_ready=1", "environment_weather_simulation_visual_quality_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_physical_weather_ready=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend parity", "production solver readiness", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_validation_dataset_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationValidationDatasetDesc", "EnvironmentWeatherSimulationValidationCase", "EnvironmentWeatherSimulationValidationDatasetPlan", "EnvironmentWeatherSimulationValidationDatasetDiagnosticCode", "plan_environment_weather_simulation_validation_dataset", "has_environment_weather_simulation_validation_dataset_diagnostic", "environment_weather_simulation_validation_dataset_status=ready", "environment_weather_simulation_validation_dataset_ready=1", "environment_weather_simulation_validation_case_rows=3", "environment_weather_simulation_validation_required_cases=3", "environment_weather_simulation_validation_ready_cases=3", "environment_weather_simulation_validation_supersaturated_condensation_ready=1", "environment_weather_simulation_validation_forced_evaporation_precipitation_ready=1", "environment_weather_simulation_validation_clamped_mixed_grid_ready=1", "environment_weather_simulation_validation_diagnostics=0", "environment_physical_weather_simulation_ready=0", "selected canonical validation dataset", "validation image counters are tracked separately", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_validation_image_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationValidationImageDesc", "EnvironmentWeatherSimulationValidationImageRow", "EnvironmentWeatherSimulationValidationImagePlan", "EnvironmentWeatherSimulationValidationImageDiagnosticCode", "plan_environment_weather_simulation_validation_images", "has_environment_weather_simulation_validation_image_diagnostic", "environment_weather_simulation_validation_image_status=ready", "environment_weather_simulation_validation_images_ready=1", "environment_weather_simulation_validation_image_rows=12", "environment_weather_simulation_validation_required_images=12", "environment_weather_simulation_validation_supersaturated_condensation_images_ready=1", "environment_weather_simulation_validation_forced_evaporation_precipitation_images_ready=1", "environment_weather_simulation_validation_clamped_mixed_grid_images_ready=1", "environment_weather_simulation_validation_image_diagnostics=0", "environment_weather_simulation_validation_image_hash", "environment_physical_weather_simulation_ready=0", "selected canonical validation image counters", "GPU/backend execution", "retained profiler artifacts", "artist controls", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_artist_control_counters"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentWeatherSimulationArtistControlDesc", "EnvironmentWeatherSimulationArtistControlCell", "EnvironmentWeatherSimulationArtistControlPlan", "EnvironmentWeatherSimulationArtistControlDiagnosticCode", "plan_environment_weather_simulation_artist_controls", "has_environment_weather_simulation_artist_control_diagnostic", "environment_weather_simulation_artist_control_status=ready", "environment_weather_simulation_artist_controls_ready=1", "environment_weather_simulation_artist_control_rows=4", "environment_weather_simulation_artist_control_generated_cells=4", "environment_weather_simulation_artist_control_raw_solver_internal_access=0", "environment_weather_simulation_artist_control_native_handle_access=0", "environment_weather_simulation_artist_control_invokes_gpu=0", "environment_weather_simulation_artist_control_invokes_backend=0", "environment_weather_simulation_artist_control_physical_weather_ready=0", "environment_weather_simulation_artist_control_diagnostics=0", "environment_weather_simulation_artist_control_hash", "environment_physical_weather_simulation_ready=0", "selected deterministic artist-facing weather controls", "raw unstable solver internals", "complete physical weather simulation remains unsupported")
    },
    @{
        id = "environment_weather_simulation_physical_closeout"
        claimId = "environment_physical_weather_simulation_ready"
        state = "ready"
        determinism = "deterministic"
        needles = @("EnvironmentPhysicalWeatherCoupledFieldKind", "EnvironmentPhysicalWeatherValidationDatasetRow", "EnvironmentPhysicalWeatherBackendSolverRow", "EnvironmentPhysicalWeatherSimulationCloseoutDesc", "EnvironmentPhysicalWeatherSimulationCloseoutResult", "evaluate_environment_physical_weather_simulation_closeout", "tools/validate-environment-weather-physics.ps1", "MK_environment_weather_simulation_tests", "MK_d3d12_environment_weather_solver_tests", "MK_vulkan_environment_weather_solver_tests", "MK_metal_environment_weather_solver_tests", "environment_physical_weather_simulation_ready=1", "environment_weather_simulation_production_solver_ready=1", "environment_weather_simulation_backend_parity_ready=1", "environment_weather_simulation_physical_weather_ready=1", "environment_weather_simulation_coupled_field_rows=13", "environment_weather_simulation_canonical_dataset_rows=12", "environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12", "environment_weather_simulation_canonical_image_rows=12", "environment_weather_simulation_backend_solver_rows=3", "environment_weather_simulation_solver_budget_overages=0", "environment_weather_simulation_visual_regression_failures=0", "environment_weather_simulation_validation_failures=0", "environment_weather_simulation_backend_inference=0", "environment_weather_simulation_native_handle_access=0", "environment_ready=0", "environment_commercial_ready=0")
    }
)
$environmentPhysicalWeatherRows = @($environmentCommercialLoop.environmentPhysicalWeatherSimulationRows)
if ($environmentPhysicalWeatherRows.Count -ne $expectedEnvironmentPhysicalWeatherRows.Count) {
    Write-Error "engine manifest environmentPhysicalWeatherSimulationRows must contain exactly $($expectedEnvironmentPhysicalWeatherRows.Count) rows"
}
$environmentPhysicalWeatherRowsById = @{}
foreach ($weatherRow in $environmentPhysicalWeatherRows) {
    Assert-Properties $weatherRow @("id", "claimId", "state", "model", "runtimeTarget", "determinism", "validationRecipeIds", "stateVariables", "stabilityConstraints", "validatedEvidence", "packageVisibility", "blockerReason", "forbiddenInference", "notes") "engine manifest environmentPhysicalWeatherSimulationRows"
    $weatherRowId = [string]$weatherRow.id
    if ($environmentPhysicalWeatherRowsById.ContainsKey($weatherRowId)) {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows duplicate row id: $weatherRowId"
    }
    $environmentPhysicalWeatherRowsById[$weatherRowId] = $weatherRow
    if ([string]$weatherRow.claimId -ne "environment_physical_weather_simulation_ready") {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' must map to environment_physical_weather_simulation_ready"
    }
    foreach ($arrayProperty in @("validationRecipeIds", "stateVariables", "stabilityConstraints", "validatedEvidence")) {
        if (@($weatherRow.$arrayProperty).Count -eq 0) {
            Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' must have at least one $arrayProperty row"
        }
    }
    foreach ($recipeId in @($weatherRow.validationRecipeIds)) {
        if (-not $environmentCommercialValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' references unknown validation recipe: $recipeId"
        }
    }
}
foreach ($expectedWeatherRow in $expectedEnvironmentPhysicalWeatherRows) {
    $weatherRowId = [string]$expectedWeatherRow.id
    if (-not $environmentPhysicalWeatherRowsById.ContainsKey($weatherRowId)) {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows missing row id: $weatherRowId"
        continue
    }
    $weatherRow = $environmentPhysicalWeatherRowsById[$weatherRowId]
    if ([string]$weatherRow.claimId -ne [string]$expectedWeatherRow.claimId) {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' must map to $($expectedWeatherRow.claimId), not $($weatherRow.claimId)"
    }
    if ([string]$weatherRow.state -ne [string]$expectedWeatherRow.state) {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' must remain $($expectedWeatherRow.state), not $($weatherRow.state)"
    }
    if ([string]$weatherRow.determinism -ne [string]$expectedWeatherRow.determinism) {
        Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' must remain $($expectedWeatherRow.determinism), not $($weatherRow.determinism)"
    }
    if ([string]$environmentCommercialClaimsById["environment_physical_weather_simulation_ready"].state -ne "ready") {
        Write-Error "engine manifest environment_physical_weather_simulation_ready must be ready after Task 10 physical-weather closeout"
    }
    $weatherRowText = [string]::Join(" ", @([string]$weatherRow.model, [string]$weatherRow.runtimeTarget, [string]$weatherRow.determinism) + @($weatherRow.validationRecipeIds) + @($weatherRow.stateVariables) + @($weatherRow.stabilityConstraints) + @($weatherRow.validatedEvidence) + @([string]$weatherRow.packageVisibility, [string]$weatherRow.blockerReason, [string]$weatherRow.forbiddenInference, [string]$weatherRow.notes))
    foreach ($needle in @($expectedWeatherRow.needles)) {
        if (-not $weatherRowText.Contains([string]$needle)) {
            Write-Error "engine manifest environmentPhysicalWeatherSimulationRows '$weatherRowId' missing CPU reference needle: $needle"
        }
    }
}

$expectedEnvironmentArtistWorkflowRows = @(
    @{
        id = "environment_artist_workflow_command_report_foundation"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowCommandKind", "EnvironmentArtistWorkflowCommandRequest", "EnvironmentArtistWorkflowCommandCatalog", "EnvironmentArtistWorkflowCommandPlan", "EnvironmentArtistWorkflowCommandReportRow", "make_environment_artist_workflow_command_catalog", "plan_environment_artist_workflow_command", "environment.command.preset.import", "environment.command.source_asset.review", "environment.command.cook.preview", "environment.command.profile_graph.edit", "environment.command.weather_timeline.edit", "environment.command.local_volume.edit", "environment.command.simulation_parameter.edit", "environment.command.quality_budget.edit", "environment.command.package.preview", "environment.command.validation.remediation", "environment.command.publish.package", "dry_run", "revision_checked_apply", "expected_revision", "rejected_stale_revision", "environment.workflow.command_id", "environment.workflow.mode", "environment.workflow.before_revision", "environment.workflow.after_revision", "environment.workflow.revision_checked", "environment.workflow.undo_supported", "environment.workflow.rollback_metadata", "backend work", "package scripts", "native handle access", "without visible first-party workflow execution and package-visible closeout evidence")
    },
    @{
        id = "environment_artist_workflow_asset_browser_foundation"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowAssetKind", "EnvironmentArtistWorkflowAssetBrowserInputRow", "EnvironmentArtistWorkflowAssetBrowserDesc", "EnvironmentArtistWorkflowAssetBrowserRow", "EnvironmentArtistWorkflowAssetBrowserModel", "make_environment_artist_workflow_asset_browser_model", "make_environment_artist_workflow_asset_browser_ui_model", "environment.workflow.asset.preset_library", "environment.workflow.asset.openexr_source", "environment.workflow.asset.ktx2_basis_source", "environment.workflow.asset.cooked_texture", "environment.workflow.asset.environment_profile", "environment.workflow.asset.simulation_preset", "environment.workflow.asset.validation_report", "environment.workflow.asset.package_artifact", "package visibility", "provenance", "budget", "host gate", "validation recipe", "read_only", "review_only", "backend work", "package scripts", "native handle access", "without visible first-party workflow execution and package-visible closeout evidence")
    },
    @{
        id = "environment_artist_workflow_preview_rows_foundation"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowPreviewRowKind", "EnvironmentArtistWorkflowPreviewRowStatus", "EnvironmentArtistWorkflowPreviewDesc", "EnvironmentArtistWorkflowPreviewRow", "EnvironmentArtistWorkflowPreviewModel", "environment_artist_workflow_preview_row_id", "make_environment_artist_workflow_preview_model", "make_environment_artist_workflow_preview_ui_model", "environment.workflow.preview.selected_backend", "environment.workflow.preview.quality_tier", "environment.workflow.preview.missing_host_gate", "environment.workflow.preview.package_budget", "environment.workflow.preview.memory_budget", "environment.workflow.preview.diagnostics", "environment.workflow.preview.unsupported_claim_reason", "selected backend", "quality tier", "missing host gate", "package budget", "memory budget", "diagnostics", "unsupported claim reason", "complete_artist_workflow_ready_claimed=false", "read_only", "review_only", "backend work", "package scripts", "native handle access", "without visible first-party workflow execution and package-visible closeout evidence")
    },
    @{
        id = "environment_artist_workflow_walkthrough_value_model"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowWalkthroughStepKind", "EnvironmentArtistWorkflowWalkthroughStepStatus", "EnvironmentArtistWorkflowWalkthroughStepInputRow", "EnvironmentArtistWorkflowWalkthroughDesc", "EnvironmentArtistWorkflowWalkthroughStepRow", "EnvironmentArtistWorkflowWalkthroughDiagnosticRow", "EnvironmentArtistWorkflowWalkthroughModel", "environment_artist_workflow_walkthrough_step_id", "make_environment_artist_workflow_walkthrough_model", "make_environment_artist_workflow_walkthrough_ui_model", "environment.workflow.walkthrough.import_source_assets", "environment.workflow.walkthrough.cook_assets", "environment.workflow.walkthrough.assemble_preset", "environment.workflow.walkthrough.edit_weather_timeline", "environment.workflow.walkthrough.run_simulation_preview", "environment.workflow.walkthrough.package_sample", "environment.workflow.walkthrough.run_installed_validation", "environment.workflow.walkthrough.inspect_report", "read_only", "review_only", "complete_artist_workflow_ready_claimed=false", "backend execution", "package script execution", "validation recipe execution", "native handle access", "ready promotion", "without visible execution/review and package-visible closeout evidence")
    },
    @{
        id = "environment_artist_workflow_visible_execution_review"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowExecutionStageStatus", "EnvironmentArtistWorkflowExecutionEvidenceRow", "EnvironmentArtistWorkflowExecutionReviewDesc", "EnvironmentArtistWorkflowExecutionReviewStageRow", "EnvironmentArtistWorkflowExecutionReviewModel", "environment_artist_workflow_execution_stage_status_label", "make_environment_artist_workflow_execution_review_model", "make_environment_artist_workflow_execution_review_ui_model", "environment.workflow.execution.command_catalog", "environment.workflow.execution.asset_browser", "environment.workflow.execution.preview", "environment.workflow.execution.walkthrough", "environment.workflow.execution.external_execution", "environment.workflow.execution.evidence_review", "environment.workflow.execution.operator_review", "environment.workflow.execution.ready_promotion_guard", "externally_supplied_evidence", "operator_review", "visible execution/review", "complete_artist_workflow_ready_claimed=false", "backend execution", "package script execution", "validation recipe execution", "native handle access", "editor-core execution claims", "ready promotion", "without package-visible closeout evidence")
    },
    @{
        id = "environment_artist_workflow_ready_closeout"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core+sample_desktop_runtime_game", "EnvironmentArtistWorkflowReadyRequirementKind", "EnvironmentArtistWorkflowReadyRequirementStatus", "EnvironmentArtistWorkflowReadyRequirementInputRow", "EnvironmentArtistWorkflowReadyReviewDesc", "EnvironmentArtistWorkflowReadyRequirementRow", "EnvironmentArtistWorkflowReadyReviewModel", "environment_artist_workflow_ready_requirement_id", "environment_artist_workflow_ready_requirement_status_label", "make_environment_artist_workflow_ready_review_model", "make_environment_artist_workflow_ready_review_ui_model", "environment.workflow.ready.visible_editor_shell", "environment.workflow.ready.asset_pipeline", "environment.workflow.ready.selected_preset_library", "environment.workflow.ready.validation_remediation", "environment.workflow.ready.revision_safety", "environment.workflow.ready.production_walkthrough_package", "environment.workflow.ready.editor_core_execution_boundary", "environment.workflow.ready.operator_review", "desktop-runtime-sample-game-environment-artist-workflow-package", "--require-environment-artist-workflow-package", "environment_artist_workflow_package_status=ready", "environment_artist_workflow_package_ready=1", "environment_artist_workflow_ready=1", "environment_artist_workflow_requirement_rows=8", "environment_artist_workflow_ready_rows=8", "environment_artist_workflow_commercial_ready=0", "commercial readiness", "backend parity", "all-platform readiness", "complete physical weather simulation", "broad optimization", "native handle access")
    },
    @{
        id = "environment_artist_workflow_production_closeout"
        claimId = "environment_artist_workflow_production_ready"
        state = "ready"
        needles = @("MK_editor_core+sample_desktop_runtime_game", "EnvironmentArtistWorkflowProductionRequirementKind", "EnvironmentArtistWorkflowProductionRequirementStatus", "EnvironmentArtistWorkflowProductionRequirementInputRow", "EnvironmentArtistWorkflowProductionCloseoutDesc", "EnvironmentArtistWorkflowProductionRequirementRow", "EnvironmentArtistWorkflowProductionCloseoutModel", "environment_artist_workflow_production_requirement_id", "environment_artist_workflow_production_requirement_status_label", "environment_artist_workflow_production_closeout_status_label", "evaluate_environment_artist_workflow_production_closeout", "make_environment_artist_workflow_production_closeout_ui_model", "environment-artist-workflow-production-closeout", "tools/validate-environment-artist-workflow-production.ps1", "workflow_import_openexr_ready=1", "workflow_import_ktx2_basis_ready=1", "workflow_import_gltf_material_ready=1", "workflow_review_usd_materialx_ocio_ready=1", "workflow_cook_package_ready=1", "workflow_live_preview_d3d12_ready=1", "workflow_live_preview_vulkan_ready=1", "workflow_live_preview_metal_host_ready=1", "workflow_weather_timeline_edit_ready=1", "workflow_preset_batch_apply_ready=1", "workflow_validation_report_ready=1", "workflow_profiler_artifact_review_ready=1", "workflow_undo_redo_revision_safety_ready=1", "workflow_operator_review_ready=1", "environment_artist_workflow_production_requirement_rows=14", "environment_artist_workflow_production_ready_rows=14", "environment_artist_workflow_production_ready=1", "environment_artist_workflow_editor_core_backend_execution=0", "environment_artist_workflow_editor_core_package_script_execution=0", "environment_artist_workflow_editor_core_validation_recipe_execution=0", "environment_artist_workflow_native_handle_access=0", "environment_ready=0", "environment_commercial_ready=0", "commercial readiness", "all-platform readiness", "broad optimization")
    }
)
$environmentArtistWorkflowRows = @($environmentCommercialLoop.environmentArtistWorkflowRows)
if ($environmentArtistWorkflowRows.Count -ne $expectedEnvironmentArtistWorkflowRows.Count) {
    Write-Error "engine manifest environmentArtistWorkflowRows must contain exactly $($expectedEnvironmentArtistWorkflowRows.Count) rows"
}
$environmentArtistWorkflowRowsById = @{}
foreach ($artistRow in $environmentArtistWorkflowRows) {
    Assert-Properties $artistRow @("id", "claimId", "state", "owner", "commandIds", "requestModes", "revisionSafety", "reportRows", "validationRecipeIds", "blockerReason", "forbiddenInference", "notes") "engine manifest environmentArtistWorkflowRows"
    $artistRowId = [string]$artistRow.id
    if ($environmentArtistWorkflowRowsById.ContainsKey($artistRowId)) {
        Write-Error "engine manifest environmentArtistWorkflowRows duplicate row id: $artistRowId"
    }
    $environmentArtistWorkflowRowsById[$artistRowId] = $artistRow
    if ([string]$artistRow.claimId -notin @("environment_artist_workflow_ready", "environment_artist_workflow_production_ready")) {
        Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' must map to an environment artist workflow claim"
    }
    foreach ($arrayProperty in @("commandIds", "requestModes", "reportRows", "validationRecipeIds")) {
        if (@($artistRow.$arrayProperty).Count -eq 0) {
            Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' must have at least one $arrayProperty row"
        }
    }
    foreach ($recipeId in @($artistRow.validationRecipeIds)) {
        if (-not $environmentCommercialValidationRecipeNames.ContainsKey([string]$recipeId)) {
            Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' references unknown validation recipe: $recipeId"
        }
    }
}
foreach ($expectedArtistRow in $expectedEnvironmentArtistWorkflowRows) {
    $artistRowId = [string]$expectedArtistRow.id
    if (-not $environmentArtistWorkflowRowsById.ContainsKey($artistRowId)) {
        Write-Error "engine manifest environmentArtistWorkflowRows missing row id: $artistRowId"
        continue
    }
    $artistRow = $environmentArtistWorkflowRowsById[$artistRowId]
    if ([string]$artistRow.claimId -ne [string]$expectedArtistRow.claimId) {
        Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' must map to $($expectedArtistRow.claimId), not $($artistRow.claimId)"
    }
    if ([string]$artistRow.state -ne [string]$expectedArtistRow.state) {
        Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' must remain $($expectedArtistRow.state), not $($artistRow.state)"
    }
    $artistRowText = [string]::Join(" ", @([string]$artistRow.owner, [string]$artistRow.revisionSafety) + @($artistRow.commandIds) + @($artistRow.requestModes) + @($artistRow.reportRows) + @($artistRow.validationRecipeIds) + @([string]$artistRow.blockerReason, [string]$artistRow.forbiddenInference, [string]$artistRow.notes))
    foreach ($needle in @($expectedArtistRow.needles)) {
        if (-not $artistRowText.Contains([string]$needle)) {
            Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' missing workflow needle: $needle"
        }
    }
}

$expectedAdjacentEnvironmentCommercialClaims = @(
    "environment_unconditional_all_platform_parity_ready",
    "environment_broad_renderer_quality_ready",
    "environment_public_native_handles_ready",
    "environment_unlicensed_assets_ready",
    "environment_unreviewed_content_mutation_ready",
    "environment_compatibility_parsers_ready",
    "environment_default_optional_codec_dependencies_ready"
)
$adjacentEnvironmentCommercialClaimsById = @{}
foreach ($adjacentClaim in @($environmentCommercialLoop.environmentCommercialUnsupportedAdjacentClaims)) {
    Assert-Properties $adjacentClaim @("id", "state", "requiredBeforeReadyClaim", "notes") "engine manifest environmentCommercialUnsupportedAdjacentClaims"
    $adjacentId = [string]$adjacentClaim.id
    if ($adjacentEnvironmentCommercialClaimsById.ContainsKey($adjacentId)) {
        Write-Error "engine manifest environmentCommercialUnsupportedAdjacentClaims duplicate id: $adjacentId"
    }
    $adjacentEnvironmentCommercialClaimsById[$adjacentId] = $true
    if ($expectedAdjacentEnvironmentCommercialClaims -notcontains $adjacentId) {
        Write-Error "engine manifest environmentCommercialUnsupportedAdjacentClaims has unexpected id: $adjacentId"
    }
    if ([string]$adjacentClaim.state -ne "unsupported") {
        Write-Error "engine manifest environmentCommercialUnsupportedAdjacentClaims '$adjacentId' must remain unsupported"
    }
    foreach ($requiredClaimId in @($adjacentClaim.requiredBeforeReadyClaim)) {
        if (-not $environmentCommercialClaimsById.ContainsKey([string]$requiredClaimId)) {
            Write-Error "engine manifest environmentCommercialUnsupportedAdjacentClaims '$adjacentId' references unknown target claim: $requiredClaimId"
        }
    }
}
foreach ($adjacentId in $expectedAdjacentEnvironmentCommercialClaims) {
    if (-not $adjacentEnvironmentCommercialClaimsById.ContainsKey($adjacentId)) {
        Write-Error "engine manifest environmentCommercialUnsupportedAdjacentClaims missing id: $adjacentId"
    }
}
if (@($environmentCommercialLoop.unsupportedProductionGaps).Count -ne 0) {
    Write-Error "engine manifest unsupportedProductionGaps must remain [] while environment commercial Phase 1 uses the dedicated claim taxonomy"
}

$commercialReadinessClaim = $environmentCommercialClaimsById["environment_commercial_ready"]
if ($null -eq $commercialReadinessClaim -or
    @($commercialReadinessClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-commercial-readiness" -or
    @($commercialReadinessClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-commercial-vulkan-evidence" -or
    @($commercialReadinessClaim.validationRecipeIds) -notcontains "renderer-metal-environment-aggregate-apple-host-evidence" -or
    [string]$commercialReadinessClaim.state -ne "unsupported") {
    Write-Error "engine manifest environment_commercial_ready must remain unsupported and reference the commercial readiness, commercial Vulkan evidence, and Apple-host Metal aggregate evidence recipes"
}
$commercialReadinessClaimText = ((@($commercialReadinessClaim.validationRecipeIds) + @([string]$commercialReadinessClaim.requiredEvidence, [string]$commercialReadinessClaim.forbiddenInference, [string]$commercialReadinessClaim.notes)) -join " ")
if (@($commercialReadinessClaim.dependsOn) -notcontains "environment_artist_workflow_production_ready" -or
    @($commercialReadinessClaim.dependsOn) -contains "environment_artist_workflow_ready") {
    Write-Error "engine manifest environment_commercial_ready must depend on environment_artist_workflow_production_ready, not the selected environment_artist_workflow_ready row"
}
foreach ($needle in @("Phase 12 slice 1", "--require-environment-commercial-readiness", "environment_commercial_readiness_status=blocked", "environment_commercial_ready=0", "14 required rows", "4 ready rows", "7 host-gated rows", "3 blocked rows", "0 missing rows", "14 package-visible rows", "14 validation-guarded rows", "14 legal-notice-current rows", "optional_dependency_legal_records_current=1", "adjacent_broad_non_claims_declared=1", "native_handle_access=0", "broad_environment_ready_claimed=0", "Phase 12 slice 2", "--require-environment-commercial-vulkan-evidence", "3 ready rows", "5 host-gated rows", "6 blocked rows", "environment_commercial_vulkan_evidence_requested=1", "environment_commercial_strict_vulkan_aggregate_ready=1", "environment_commercial_windows_vulkan_ready=1", "Phase 12 slice 3", "renderer-metal-environment-aggregate-apple-host-evidence", "2 ready rows", "7 blocked rows", "environment_commercial_metal_evidence_requested=1", "environment_commercial_metal_host_aggregate_ready=1", "environment_commercial_macos_metal_ready=1")) {
    if (-not $commercialReadinessClaimText.Contains($needle)) {
        Write-Error "engine manifest environment_commercial_ready Phase 12 blocker gate text missing: $needle"
    }
}

$environmentCommercialCounterRuleText = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentCommercialClaimCounterRules
foreach ($needle in @("environmentCommercialClaimMatrix", "packageCounter equal to the claim id", "lower_snake_case", "environmentCommercialUnsupportedAdjacentClaims")) {
    if (-not $environmentCommercialCounterRuleText.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentCommercialClaimCounterRules missing: $needle"
    }
}
$environmentCommercialAggregateGateGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentCommercialAggregateCloseoutGatePhase12
foreach ($needle in @("EnvironmentCommercialReadinessStatus", "EnvironmentCommercialReadinessRequirementKind", "EnvironmentCommercialReadinessRequirementStatus", "EnvironmentCommercialReadinessRequirementInputRow", "EnvironmentCommercialReadinessDesc", "EnvironmentCommercialReadinessRequirementRow", "EnvironmentCommercialReadinessPlan", "environment_commercial_readiness_status_label", "environment_commercial_readiness_requirement_id", "environment_commercial_readiness_requirement_status_label", "plan_environment_commercial_readiness", "EnvironmentCommercialReadinessV2RowStatus", "EnvironmentCommercialReadinessV2Row", "EnvironmentCommercialReadinessV2Result", "evaluate_environment_commercial_readiness_v2", "MK_environment_commercial_readiness_v2_tests", "16 required rows", "dependency_gated_rows", "environment_highest_commercial_ready=0", "no D3D12-to-Vulkan-or-Metal inference", "no macOS-to-iOS Metal inference", "no all-platform promotion from backend parity while any exact platform row such as Android Vulkan is missing", "duplicate required row ids become diagnostics", "license/solver/visible-shell dependency gates", "diagnostics=false", "native_handle_access=false", "desktop-runtime-sample-game-environment-commercial-readiness", "--require-environment-commercial-readiness", "desktop-runtime-sample-game-environment-commercial-vulkan-evidence", "--require-environment-commercial-vulkan-evidence", "renderer-metal-environment-aggregate-apple-host-evidence", "environment_commercial_readiness_status=blocked", "environment_commercial_ready=0", "environment_commercial_required_rows=14", "environment_commercial_ready_rows=4", "environment_commercial_host_gated_rows=7", "environment_commercial_ready_rows=3", "environment_commercial_ready_rows=2", "environment_commercial_host_gated_rows=5", "environment_commercial_blocked_rows=6", "environment_commercial_blocked_rows=7", "environment_commercial_missing_rows=0", "environment_commercial_package_visible_rows=14", "environment_commercial_validation_guarded_rows=14", "environment_commercial_legal_notice_current_rows=14", "environment_commercial_optional_dependency_legal_records_current=1", "environment_commercial_adjacent_broad_non_claims_declared=1", "environment_commercial_native_handle_access=0", "environment_commercial_broad_environment_ready_claimed=0", "environment_commercial_vulkan_evidence_requested=1", "environment_commercial_strict_vulkan_aggregate_ready=1", "environment_commercial_windows_vulkan_ready=1", "environment_commercial_metal_evidence_requested=1", "environment_commercial_metal_host_aggregate_ready=1", "environment_commercial_macos_metal_ready=1", "validate.yml ios-metal", "environment_platform_ios_metal_ready", "environment_metal_aggregate_ready", "environment-highest-commercial-readiness-closeout", "tools/validate-environment-highest-commercial-readiness.ps1", "environment_highest_commercial_ready=1", "environment_commercial_required_rows=16", "environment_commercial_ready_rows=16", "environment_host_gated_rows=0", "environment_dependency_gated_rows=0", "environment_unsupported_rows=0", "environment_all_platform_unconditional_ready=1", "environment_optimization_measurement_workload_rows=21", "environment_weather_simulation_backend_parity_ready=1", "workflow_visible_shell_execution_ready=1", "runtime_source_parsing=0", "environment_ready_unchanged=1", "strict Vulkan aggregate", "Metal host aggregate", "physical weather simulation", "broad environment_ready")) {
    if (-not $environmentCommercialAggregateGateGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentCommercialAggregateCloseoutGatePhase12 missing: $needle"
    }
}
foreach ($sourceSurface in @(
        @{ Path = "engine/environment/include/mirakana/environment/environment_commercial_readiness.hpp"; Needles = @("EnvironmentCommercialReadinessStatus", "EnvironmentCommercialReadinessRequirementKind", "EnvironmentCommercialReadinessDesc", "EnvironmentCommercialReadinessPlan", "plan_environment_commercial_readiness") },
        @{ Path = "engine/environment/src/environment_commercial_readiness.cpp"; Needles = @("strict_vulkan_aggregate", "metal_host_aggregate", "backend_parity", "physical_weather_simulation", "request_commercial_ready", "environment_commercial_ready =") },
        @{ Path = "tests/unit/environment_tests.cpp"; Needles = @("environment commercial readiness gate reports exact blockers before promotion", "plan.required_row_count == 14U", "plan.ready_row_count == 4U", "plan.host_gated_row_count == 7U", "plan.blocked_row_count == 3U", "environment commercial readiness rejects incomplete package-visible evidence") },
        @{ Path = "engine/environment/include/mirakana/environment/commercial_readiness_v2.hpp"; Needles = @("EnvironmentCommercialReadinessV2RowStatus", "EnvironmentCommercialReadinessV2Row", "EnvironmentCommercialReadinessV2Result", "evaluate_environment_commercial_readiness_v2") },
        @{ Path = "engine/environment/src/commercial_readiness_v2.cpp"; Needles = @("environment_strict_vulkan_aggregate_ready", "environment_metal_aggregate_ready", "environment_platform_linux_vulkan_ready", "environment_platform_ios_metal_ready", "environment_platform_android_vulkan_ready", "environment_all_platform_unconditional_ready", "environment_asset_pipeline_openexr_ktx_basis_full_ready", "required_row_counts", "dependency_gated_rows", "highest_commercial_ready") },
        @{ Path = "tests/unit/environment_commercial_readiness_v2_tests.cpp"; Needles = @("missing_dependency_keeps_environment_highest_commercial_ready_0", "all_dependencies_ready_promotes_environment_highest_commercial_ready_1", "d3d12_ready_does_not_promote_vulkan_or_metal", "macos_metal_ready_does_not_promote_ios_metal", "backend_parity_ready_does_not_promote_all_platform_when_linux_android_ios_missing", "asset_library_counts_without_license_keep_ready_0", "weather_visual_quality_without_solver_validation_keeps_ready_0", "artist_workflow_value_rows_without_visible_shell_keep_ready_0", "native_handle_access_keeps_ready_0", "diagnostics_keep_ready_0", "duplicate_required_row_id_keeps_ready_0", "kRequiredCommercialRows.size()") },
        @{ Path = "CMakeLists.txt"; Needles = @("MK_environment_commercial_readiness_v2_tests", "tests/unit/environment_commercial_readiness_v2_tests.cpp") },
        @{ Path = "engine/environment/CMakeLists.txt"; Needles = @("src/commercial_readiness_v2.cpp") },
        @{ Path = "games/sample_desktop_runtime_game/main.cpp"; Needles = @("--require-environment-commercial-readiness", "--require-environment-commercial-vulkan-evidence", "environment_commercial_readiness_status", "environment_commercial_required_rows", "environment_commercial_broad_environment_ready_claimed", "environment_commercial_vulkan_evidence_requested", "require_environment_commercial_readiness", "require_environment_commercial_vulkan_evidence") },
        @{ Path = "tools/validation-recipe-core.ps1"; Needles = @("Get-SampleDesktopRuntimeGameEnvironmentCommercialReadinessSmokeArgs", "Get-SampleDesktopRuntimeGameEnvironmentCommercialVulkanEvidenceSmokeArgs", "--require-environment-commercial-readiness", "--require-environment-commercial-vulkan-evidence") },
        @{ Path = "tools/run-validation-recipe-plans.ps1"; Needles = @("desktop-runtime-sample-game-environment-commercial-readiness", "desktop-runtime-sample-game-environment-commercial-vulkan-evidence", "commercial-environment-closeout", "Get-SampleDesktopRuntimeGameEnvironmentCommercialReadinessSmokeArgs", "Get-SampleDesktopRuntimeGameEnvironmentCommercialVulkanEvidenceSmokeArgs", "environment-highest-commercial-readiness-closeout", "tools/validate-environment-highest-commercial-readiness.ps1", "environment_highest_commercial_ready=1", "environment_commercial_required_rows=16", "environment_all_platform_unconditional_ready=1", "environment_optimization_measurement_workload_rows=21", "environment-platform-linux-vulkan-package", "environment-platform-android-vulkan-package", "tools/validate-linux-vulkan-runtime-host.ps1", "tools/validate-android-vulkan-runtime-host.ps1", "vulkaninfo_ready=1", "android_vulkan_readback_ready=1", "environment-platform-ios-metal-package", "environment-backend-parity-v2-closeout", "tools/validate-environment-backend-parity-v2.ps1", "45 backend-local ready rows", "environment-broad-optimization-cross-backend-measurement", "environment-metal-host-optimization-artifact-producer", "tools/generate-environment-metal-optimization-artifacts.ps1", "xcrun_xctrace_ready=1", "environment-asset-pipeline-openexr-ktx-basis-full", "environment-aaa-preset-asset-library-production", "environment-physical-weather-simulation-closeout", "tools/validate-environment-weather-physics.ps1", "environment_physical_weather_simulation_ready=1", "environment-artist-workflow-production-closeout", "tools/validate-environment-artist-workflow-production.ps1", "environment_artist_workflow_production_ready=1", "environment_ready_unchanged=1") },
        @{ Path = "tools/validate-environment-highest-commercial-readiness.ps1"; Needles = @("MK_environment_commercial_readiness_v2_tests", "tools/validate-environment-optimization-artifacts.ps1", "environment_highest_commercial_ready", "environment_commercial_required_rows", "environment_strict_vulkan_aggregate_ready", "environment_metal_aggregate_ready", "environment_platform_readiness_ready", "environment_all_platform_unconditional_ready", "environment_broad_optimization_ready", "environment_optimization_measurement_workload_rows", "environment_asset_pipeline_openexr_ktx_basis_full_ready", "environment_aaa_preset_asset_library_ready", "environment_physical_weather_simulation_ready", "environment_weather_simulation_backend_parity_ready", "environment_artist_workflow_production_ready", "workflow_visible_shell_execution_ready", "runtime_source_parsing=0", "environment_ready_unchanged=1") },
        @{ Path = "tools/validate-environment-asset-pipeline-full.ps1"; Needles = @("MK_environment_texture_pipeline_v2_tests", "tools/build-asset-importers.ps1", "environment_asset_pipeline_openexr_ktx_basis_full_ready=1", "environment_asset_pipeline_required_rows=14", "environment_asset_pipeline_openexr_rows=5", "environment_asset_pipeline_ktx2_basis_rows=4", "environment_asset_pipeline_backend_target_rows=4", "environment_asset_pipeline_runtime_rows=1", "environment_asset_pipeline_dependency_gated_rows=0", "environment_asset_pipeline_runtime_source_parsing=0", "environment_asset_pipeline_runtime_optional_codec_execution=0", "environment_asset_pipeline_native_handle_access=0", "environment_asset_pipeline_gpu_command_executed=0", "environment_asset_pipeline_cmake_configure_dependency_install=0", "environment_ready=0", "environment_commercial_ready=0") },
        @{ Path = "engine/tools/include/mirakana/tools/environment_texture_pipeline_v2.hpp"; Needles = @("EnvironmentTexturePipelineV2EvidenceKind", "EnvironmentTexturePipelineV2EvidenceRow", "EnvironmentTexturePipelineV2Desc", "EnvironmentTexturePipelineV2Result", "evaluate_environment_texture_pipeline_v2_full", "runtime_source_parsing", "runtime_optional_codec_execution", "native_handle_access", "gpu_command_executed") },
        @{ Path = "engine/tools/asset/environment_texture_pipeline_v2.cpp"; Needles = @("openexr_scanline_rgba16f", "openexr_tiled_rgba16f", "openexr_multipart", "openexr_metadata_preservation", "openexr_deep_image_rejected", "ktx2_basis_etc1s_transcode", "ktx2_basis_uastc_transcode", "ktx2_mip_level_validation", "ktx2_color_space_metadata", "d3d12_bc7_target", "vulkan_bc7_target", "metal_astc_target", "android_vulkan_astc_target", "runtime_cooked_only_ingest", "full_ready") },
        @{ Path = "tests/unit/environment_texture_pipeline_v2_tests.cpp"; Needles = @("environment texture pipeline v2 promotes only the full official source matrix", "environment texture pipeline v2 rejects selected closeout evidence as incomplete", "environment texture pipeline v2 fails closed on dependency gates and runtime parsing", "result.required_rows == 14U", "result.openexr_rows == 5U", "result.ktx2_basis_rows == 4U", "result.backend_target_rows == 4U") },
        @{ Path = "CMakeLists.txt"; Needles = @("MK_environment_texture_pipeline_v2_tests", "tests/unit/environment_texture_pipeline_v2_tests.cpp") },
        @{ Path = "engine/tools/asset/CMakeLists.txt"; Needles = @("environment_texture_pipeline_v2.cpp") },
        @{ Path = "tools/validate-environment-backend-parity-v2.ps1"; Needles = @("MK_env_backend_parity_v2_tests", "environment_backend_parity_v2_status=ready", "environment_backend_parity_required_features=15", "environment_backend_parity_rows=45", "environment_backend_parity_ready_rows=45", "environment_backend_parity_host_gated_rows=0", "environment_backend_parity_d3d12_inferred=0", "environment_backend_parity_vulkan_inferred=0", "environment_backend_parity_metal_inferred=0", "environment_backend_parity_native_handle_access=0", "environment_backend_parity_invoked_gpu_commands=0", "environment_backend_parity_all_platform_ready=0", "environment_backend_parity_commercial_ready=0", "environment_backend_parity_broad_optimization_ready=0", "environment_backend_parity_broad_environment_ready=0") },
        @{ Path = "tools/validate-environment-aaa-preset-asset-library.ps1"; Needles = @("MK_environment_tests", "environment_aaa_preset_asset_library_ready=1", "environment_aaa_preset_asset_library_asset_rows", "environment_aaa_preset_asset_library_preview_screenshot_rows", "environment_preset_asset_license_missing_rows", "environment_aaa_preset_asset_library_backend_execution=0", "environment_aaa_preset_asset_library_package_script_execution=0", "environment_ready=0", "environment_commercial_ready=0") },
        @{ Path = "tools/validate-environment-weather-physics.ps1"; Needles = @("MK_environment_weather_simulation_tests", "MK_d3d12_environment_weather_solver_tests", "MK_vulkan_environment_weather_solver_tests", "MK_metal_environment_weather_solver_tests", "environment_physical_weather_simulation_ready=1", "environment_weather_simulation_backend_parity_ready=1", "environment_weather_simulation_coupled_field_rows=13", "environment_weather_simulation_canonical_dataset_rows=12", "environment_weather_simulation_canonical_image_rows=12", "environment_weather_simulation_backend_solver_rows=3", "environment_weather_simulation_validation_failures=0", "environment_ready=0", "environment_commercial_ready=0") },
        @{ Path = "tools/validate-environment-artist-workflow-production.ps1"; Needles = @("MK_editor_environment_tests", "tools/package-desktop-runtime.ps1", "--require-environment-artist-workflow-package", "environment_artist_workflow_production_ready=1", "workflow_import_openexr_ready=1", "workflow_live_preview_vulkan_ready=1", "environment_artist_workflow_production_requirement_rows=14", "environment_artist_workflow_editor_core_backend_execution=0", "environment_ready=0", "environment_commercial_ready=0") },
        @{ Path = "editor/core/include/mirakana/editor/environment_artist_workflow_v2.hpp"; Needles = @("EnvironmentArtistWorkflowProductionRequirementKind", "EnvironmentArtistWorkflowProductionRequirementInputRow", "EnvironmentArtistWorkflowProductionCloseoutDesc", "EnvironmentArtistWorkflowProductionCloseoutModel", "evaluate_environment_artist_workflow_production_closeout", "make_environment_artist_workflow_production_closeout_ui_model") },
        @{ Path = "editor/core/src/environment_artist_workflow_v2.cpp"; Needles = @("workflow_import_openexr_ready", "workflow_live_preview_vulkan_ready", "environment_artist_workflow_production_closeout", "environment_artist_workflow_production_ready", "unsupported_claims", "native handle access") },
        @{ Path = "tests/unit/editor_environment_tests.cpp"; Needles = @("editor environment artist workflow production closeout promotes only exact visible package rows", "editor environment artist workflow production closeout fails closed for unsafe or weak evidence", "workflow_import_openexr_ready", "model.required_rows == 14U", "model.ready_rows == 14U") },
        @{ Path = "engine/environment/include/mirakana/environment/weather_simulation.hpp"; Needles = @("EnvironmentPhysicalWeatherCoupledFieldKind", "EnvironmentPhysicalWeatherValidationDatasetRow", "EnvironmentPhysicalWeatherBackendSolverRow", "EnvironmentPhysicalWeatherSimulationCloseoutDesc", "EnvironmentPhysicalWeatherSimulationCloseoutResult", "evaluate_environment_physical_weather_simulation_closeout", "has_environment_physical_weather_closeout_diagnostic") },
        @{ Path = "engine/environment/src/weather_simulation.cpp"; Needles = @("evaluate_environment_physical_weather_simulation_closeout", "required_physical_weather_fields", "required_physical_weather_backends", "backend_parity_ready", "production_solver_ready", "physical_weather_ready", "broad_environment_ready_claim") },
        @{ Path = "tests/unit/environment_weather_simulation_tests.cpp"; Needles = @("environment physical weather closeout promotes only from exact fields datasets images and backend parity", "environment physical weather closeout fails closed for weak provenance inference or thresholds", "result.canonical_dataset_rows", "EnvironmentPhysicalWeatherBackendSolverRow", "backend_parity_mismatch") },
        @{ Path = "CMakePresets.json"; Needles = @('"desktop-runtime-linux-release"', '"VCPKG_TARGET_TRIPLET": "x64-linux"', '"generator": "Ninja"') },
        @{ Path = "tools/common.ps1"; Needles = @("bootstrap-vcpkg.sh", "vcpkg.exe", '"vcpkg"') },
        @{ Path = "tools/bootstrap-deps.ps1"; Needles = @("bootstrap-vcpkg.sh", "Get-VcpkgDefaultTriplet") },
        @{ Path = "tools/package-linux-runtime.ps1"; Needles = @("Get-VcpkgDefaultTriplet", "`$vcpkgTriplet", "-DVCPKG_TARGET_TRIPLET=`$vcpkgTriplet", "desktop-runtime-linux-release", "validate-installed-linux-runtime.ps1") },
        @{ Path = "tools/validate-linux-vulkan-runtime-host.ps1"; Needles = @("Find-VulkanInfoCommand", "--summary", "VK_LAYER_KHRONOS_validation", "dxc_spirv_codegen_ready", "spirv_val_ready", "linux_icd_runtime_ready", "first_party_linux_runtime_host_ready", "linux_package_script_ready", "linux_installed_validator_ready", "linux_package_smoke_ready", "linux_vulkan_readback_ready", "linux_vulkan_validation_log_clean", "environment_platform_linux_vulkan_ready", "windows_vulkan_inferred=0") },
        @{ Path = "tools/validate-android-vulkan-runtime-host.ps1"; Needles = @("StartEmulator", "DeviceSerial", "AvdName", "ConfigureGpuDebugLayers", "GpuDebugLayerApk", "enable_gpu_debug_layers", "gpu_debug_app", "gpu_debug_layer_app", "gpu_debug_layers", "install", "-r", "pm", "path", "VK_LAYER_KHRONOS_validation", "android.hardware.vulkan.version", "android.hardware.vulkan.level", "android_gpu_debuggable_ready", "android_gpu_debug_layer_settings_ready", "android_gpu_debug_layer_app_installed", "android_gpu_debug_layer_install_requested", "android_gpu_debug_layer_install_ready", "android_package_smoke_ready", "android_vulkan_readback_ready", "android_vulkan_validation_layer_enumerated", "android_vulkan_validation_log_clean", "environment_platform_android_vulkan_ready", "desktop_vulkan_inferred=0", "`$smokeArguments = @(", "`$smokeArguments += @(`"-DeviceSerial`", `$Serial)", "Write-Output `$smokeText.TrimEnd()") },
        @{ Path = "tools/validate-installed-desktop-runtime.ps1"; Needles = @("environment_commercial_readiness_status", "environment_commercial_ready", "environment_commercial_required_rows", "environment_commercial_broad_environment_ready_claimed", "environment_commercial_vulkan_evidence_requested", "environment_artist_workflow_production_ready", "workflow_import_openexr_ready", "workflow_live_preview_vulkan_ready") },
        @{ Path = "tools/validate-environment-metal-host-aggregate.ps1"; Needles = @("environment_backend_parity_metal_evidence_requested=1", "environment_backend_parity_metal_evidence_ready=1", "environment_backend_parity_metal_host=1", "environment_backend_parity_ready=0", "environment_backend_parity_cross_host_aggregate_ready=0", "environment_backend_parity_d3d12_inferred=0", "environment_backend_parity_vulkan_inferred=0", "environment_commercial_metal_evidence_requested=1", "environment_commercial_metal_host_aggregate_ready=1", "environment_commercial_macos_metal_ready=1", "environment_commercial_ready_rows=2", "environment_commercial_blocked_rows=7") },
        @{ Path = "tools/generate-environment-metal-optimization-artifacts.ps1"; Needles = @("validation_recipe=environment-metal-host-optimization-artifact-producer", "xcrun", "xctrace", "Metal System Trace", "validate-environment-metal-host-aggregate.ps1", "validate-environment-optimization-artifacts.ps1", "metal_apple_host", "raw_xctrace_ci_artifact", "environment_metal_host_optimization_artifacts_written", "environment_broad_optimization_ready=1", "environment_ready=0", "environment_commercial_ready=0") },
        @{ Path = ".github/workflows/validate.yml"; Needles = @("Environment Metal aggregate host evidence recipe and optimization artifacts", "generate-environment-metal-optimization-artifacts.ps1", "Upload Metal optimization artifacts", "metal-host-optimization-artifacts") },
        @{ Path = "engine/runtime_rhi/include/mirakana/runtime_rhi/environment_platform_evidence_v2.hpp"; Needles = @("EnvironmentPlatformEvidenceV2RowStatus", "EnvironmentPlatformEvidenceV2PlatformId", "EnvironmentPlatformEvidenceV2Row", "EnvironmentPlatformEvidenceV2Result", "evaluate_environment_platform_evidence_v2", "environment-platform-linux-vulkan-package", "environment-platform-android-vulkan-package", "linux_package_script_ready", "android_gpu_debug_layer_app_installed", "android_gpu_debug_layer_install_ready", "android_vulkan_readback_ready", "android_vulkan_validation_layer_enumerated", "android_vulkan_validation_log_clean") },
        @{ Path = "tests/unit/runtime_rhi_environment_platform_evidence_v2_tests.cpp"; Needles = @("windows_vulkan_evidence_does_not_promote_linux_or_android_vulkan", "linux_vulkan_ready_requires_exact_linux_host_gate_and_tool_rows", "android_vulkan_ready_requires_android_device_gpu_debug_layer_settings_and_readback", "android_vulkan_ready_requires_agi_layer_app_install_enumeration_and_clean_validation_log", "native_handle_or_inferred_platform_rows_keep_ready_false") },
        @{ Path = "CMakeLists.txt"; Needles = @("MK_env_platform_v2_tests", "tests/unit/runtime_rhi_environment_platform_evidence_v2_tests.cpp") },
        @{ Path = "engine/renderer/include/mirakana/renderer/environment_backend_parity_v2.hpp"; Needles = @("EnvironmentBackendParityV2Status", "EnvironmentBackendParityV2Feature", "EnvironmentBackendParityV2Row", "EnvironmentBackendParityV2Result", "evaluate_environment_backend_parity_v2", "environment_all_platform_unconditional_ready", "native_handle_access", "inferred_from_other_backend") },
        @{ Path = "engine/renderer/src/environment_backend_parity_v2.cpp"; Needles = @("physical_sky", "height_fog", "volumetric_fog", "volumetric_cloud", "cloud_layer", "rain_precipitation", "snow_precipitation", "material_weathering", "environment_lighting_ibl", "postprocess_depth_input", "texture_payload_rgba8_upload", "texture_payload_bc7_or_astc_upload", "weather_solver_gpu", "debug_profiling_policy", "quality_budget", "desktop-runtime-sample-game-environment-vulkan-strict-aggregate", "renderer-metal-environment-aggregate-apple-host-evidence") },
        @{ Path = "tests/unit/renderer_environment_backend_parity_v2_tests.cpp"; Needles = @("d3d12_vulkan_ready_metal_missing_keeps_parity_0", "macos_metal_ready_ios_metal_missing_keeps_all_platform_0", "ready_rows_with_native_handle_access_keep_parity_0", "ready_rows_with_diagnostics_keep_parity_0", "all_backend_rows_ready_promotes_backend_parity_1", "result.required_rows == 45U", "result.required_features == 15U") },
        @{ Path = "engine/environment/include/mirakana/environment/environment_preset_pack.hpp"; Needles = @("EnvironmentPresetAssetCategory", "EnvironmentPresetAssetLibraryAssetRow", "EnvironmentPresetAssetLibraryProductionDesc", "EnvironmentPresetAssetLibraryProductionResult", "evaluate_environment_preset_asset_library_production") },
        @{ Path = "engine/environment/src/environment_preset_pack.cpp"; Needles = @("evaluate_environment_preset_asset_library_production", "sky_atmosphere_presets", "volumetric_cloud_presets", "preview_screenshot_rows", "sample_scene_consumption_rows", "package_script_execution", "environment_aaa_preset_asset_library_ready") },
        @{ Path = "tests/unit/environment_tests.cpp"; Needles = @("production preset asset library promotes only with objective aaa rows", "selected seven preset library cannot promote production asset library", "production preset asset library fails closed on missing provenance and budget overage", "environment_aaa_preset_asset_library_ready") },
        @{ Path = "CMakeLists.txt"; Needles = @("MK_env_backend_parity_v2_tests", "tests/unit/renderer_environment_backend_parity_v2_tests.cpp") },
        @{ Path = "engine/renderer/CMakeLists.txt"; Needles = @("src/environment_backend_parity_v2.cpp") },
        @{ Path = "games/sample_desktop_runtime_game/game.agent.json"; Needles = @("environment-commercial-readiness-blocker-gate", "environment-commercial-vulkan-evidence-bridge", "desktop-runtime-sample-game-environment-commercial-readiness", "desktop-runtime-sample-game-environment-commercial-vulkan-evidence", "environment_commercial_blocked_rows=6", "environment-highest-commercial-readiness-closeout", "tools/validate-environment-highest-commercial-readiness.ps1", "environment_highest_commercial_ready=1", "environment_commercial_required_rows=16", "environment_all_platform_unconditional_ready=1", "environment_optimization_measurement_workload_rows=21", "environment-platform-linux-vulkan-package", "environment-platform-android-vulkan-package", "tools/validate-linux-vulkan-runtime-host.ps1", "tools/validate-android-vulkan-runtime-host.ps1", "environment-platform-ios-metal-package", "environment-backend-parity-v2-closeout", "environment-broad-optimization-cross-backend-measurement", "environment-asset-pipeline-openexr-ktx-basis-full", "tools/validate-environment-asset-pipeline-full.ps1", "environment_asset_pipeline_openexr_ktx_basis_full_ready=1", "environment_asset_pipeline_required_rows=14", "runtime_source_parsing=0", "environment-aaa-preset-asset-library-production", "tools/validate-environment-aaa-preset-asset-library.ps1", "environment_aaa_preset_asset_library_ready=1", "environment_aaa_preset_asset_library_asset_rows=156", "environment-physical-weather-simulation-closeout", "tools/validate-environment-weather-physics.ps1", "environment_physical_weather_simulation_ready=1", "environment_weather_simulation_backend_parity_ready=1", "environment-artist-workflow-production-closeout", "tools/validate-environment-artist-workflow-production.ps1", "environment_artist_workflow_production_ready=1", "workflow_import_openexr_ready=1", "environment_ready_unchanged=1") }
    )) {
    $sourceSurfaceText = Get-JsonContractSurfaceText $sourceSurface.Path
    foreach ($needle in @($sourceSurface.Needles)) {
        if (-not $sourceSurfaceText.Contains([string]$needle)) {
            Write-Error "$($sourceSurface.Path) missing environment commercial blocker gate needle: $needle"
        }
    }
}
$environmentBackendParityGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentBackendParityPhase7
foreach ($needle in @("desktop-runtime-sample-game-environment-backend-parity", "desktop-runtime-sample-game-environment-backend-parity-ready", "--require-environment-backend-parity-ready", "EnvironmentBackendParityRequest", "plan_environment_backend_parity", "normalized feature ids", "same profile revision", "same preset pack revision", "counter semantics", "host_evidence_required", "renderer-metal-environment-aggregate-apple-host-evidence", "environment_backend_parity_metal_evidence_requested=1", "environment_backend_parity_metal_evidence_ready=1", "environment_backend_parity_metal_host=1", "environment_backend_parity_ready=0", "environment_backend_parity_ready=1", "environment_backend_parity_ready_rows=21", "environment_backend_parity_host_gated_rows=0", "environment_backend_parity_cross_host_aggregate_ready=1", "environment_backend_parity_cross_host_aggregate_ready=0")) {
    if (-not $environmentBackendParityGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentBackendParityPhase7 missing: $needle"
    }
}
$environmentPlatformReadinessGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPlatformReadinessPhase8
foreach ($needle in @("desktop-runtime-sample-game-environment-platform-readiness", "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence", "environment-platform-linux-vulkan-host-gate", "environment-platform-linux-vulkan-package", "tools/validate-linux-vulkan-runtime-host.ps1", "vulkan-strict-linux", "validate.yml linux-vulkan", "xvfb-run", "environmentPlatformReadinessRows", "Windows D3D12", "Windows Vulkan", "Linux Vulkan", "macOS Metal", "iOS Metal", "Android Vulkan", "environment_platform_readiness_status=host_evidence_required", "environment_platform_readiness_ready=0", "environment_platform_windows_vulkan_evidence_requested=1", "environment_platform_windows_vulkan_ready=1", "environment_platform_linux_vulkan_ready=1", "environment_platform_requires_linux_vulkan_host_evidence=0", "environment_platform_android_vulkan_ready=0", "environment_platform_windows_vulkan_inferred=0", "environment_all_platform_unconditional_ready=0")) {
    if (-not $environmentPlatformReadinessGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentPlatformReadinessPhase8 missing: $needle"
    }
}
$environmentOptimizationMeasurementGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentOptimizationMeasurementPhase9
foreach ($needle in @("desktop-runtime-sample-game-environment-optimization-measurement", "environmentOptimizationMeasurementWorkloadRows", "EnvironmentOptimizationMeasurementRequest", "plan_environment_optimization_measurement", "preset_pack_flythrough", "storm_precipitation", "dense_volumetric_fog", "volumetric_cloud_sunset", "snowfield_material_weathering", "weather_simulation_stress", "asset_library_cold_load", "environment_optimization_measurement_status=host_evidence_required", "environment_optimization_measurement_required_workloads=7", "environment_broad_optimization_ready=0")) {
    if (-not $environmentOptimizationMeasurementGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentOptimizationMeasurementPhase9 missing: $needle"
    }
}
$environmentAaaPresetAssetLibraryGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentAaaPresetAssetLibraryPhase8
foreach ($needle in @("EnvironmentPresetAssetLibraryProductionDesc", "evaluate_environment_preset_asset_library_production", "tools/validate-environment-aaa-preset-asset-library.ps1", "MK_environment_tests", "environment_aaa_preset_asset_library_ready=1", "environment_aaa_preset_asset_library_asset_rows=156", "sky_atmosphere=24", "volumetric_cloud=24", "fog_volume=16", "rain=12", "snow=12", "wind=12", "material_weathering=24", "lighting_ibl=12", "weather_timeline=12", "biome_environment=8", "environment_aaa_preset_asset_library_preview_screenshot_rows=144", "environment_aaa_preset_asset_library_sample_scene_consumption_rows=8", "environment_preset_asset_license_missing_rows=0", "environment_preset_asset_package_budget_overages=0", "environment_preset_asset_external_asset_rows=0", "environment_ready=0", "environment_commercial_ready=0", "commercial readiness", "broad environment_ready")) {
    if (-not $environmentAaaPresetAssetLibraryGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentAaaPresetAssetLibraryPhase8 missing: $needle"
    }
}
$environmentAssetPipelineFullGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentAssetPipelineFullPhase9
foreach ($needle in @("EnvironmentTexturePipelineV2EvidenceKind", "EnvironmentTexturePipelineV2EvidenceRow", "EnvironmentTexturePipelineV2Desc", "EnvironmentTexturePipelineV2Result", "evaluate_environment_texture_pipeline_v2_full", "tools/validate-environment-asset-pipeline-full.ps1", "MK_environment_texture_pipeline_v2_tests", "environment_asset_pipeline_openexr_ktx_basis_full_ready=1", "environment_asset_pipeline_required_rows=14", "environment_asset_pipeline_ready_rows=14", "environment_asset_pipeline_source_artifact_rows=14", "environment_asset_pipeline_cooked_artifact_rows=14", "environment_asset_pipeline_package_counter_rows=14", "environment_asset_pipeline_replay_hash_rows=14", "environment_asset_pipeline_rejection_diagnostic_rows=1", "openexr_scanline_rgba16f_ready=1", "openexr_tiled_rgba16f_ready=1", "openexr_multipart_ready=1", "openexr_metadata_preservation_ready=1", "openexr_deep_image_rejected_with_diagnostic=1", "ktx2_basis_etc1s_transcode_ready=1", "ktx2_basis_uastc_transcode_ready=1", "ktx2_mip_level_validation_ready=1", "ktx2_color_space_metadata_ready=1", "d3d12_bc7_target_ready=1", "vulkan_bc7_target_ready=1", "metal_astc_target_ready=1", "android_vulkan_astc_target_ready=1", "runtime_cooked_only_ingest_ready=1", "runtime_source_parsing=0", "environment_asset_pipeline_runtime_optional_codec_execution=0", "environment_asset_pipeline_cmake_configure_dependency_install=0", "environment_ready=0", "environment_commercial_ready=0", "broad environment_ready")) {
    if (-not $environmentAssetPipelineFullGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentAssetPipelineFullPhase9 missing: $needle"
    }
}
$environmentPhysicalWeatherTask10Guidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationTask10
foreach ($needle in @("EnvironmentPhysicalWeatherCoupledFieldKind", "EnvironmentPhysicalWeatherValidationDatasetRow", "EnvironmentPhysicalWeatherBackendSolverRow", "EnvironmentPhysicalWeatherSimulationCloseoutDesc", "EnvironmentPhysicalWeatherSimulationCloseoutResult", "evaluate_environment_physical_weather_simulation_closeout", "has_environment_physical_weather_closeout_diagnostic", "tools/validate-environment-weather-physics.ps1", "MK_environment_weather_simulation_tests", "MK_d3d12_environment_weather_solver_tests", "MK_vulkan_environment_weather_solver_tests", "MK_metal_environment_weather_solver_tests", "environment_physical_weather_simulation_ready=1", "environment_weather_simulation_production_solver_ready=1", "environment_weather_simulation_backend_parity_ready=1", "environment_weather_simulation_coupled_field_rows=13", "environment_weather_simulation_canonical_dataset_rows=12", "environment_weather_simulation_cf_netcdf_or_grib_or_synthetic_rows=12", "environment_weather_simulation_canonical_image_rows=12", "environment_weather_simulation_backend_solver_rows=3", "environment_weather_simulation_solver_budget_overages=0", "environment_weather_simulation_visual_regression_failures=0", "environment_weather_simulation_validation_failures=0", "environment_weather_simulation_backend_inference=0", "environment_weather_simulation_native_handle_access=0", "environment_ready=0", "environment_commercial_ready=0", "commercial readiness", "broad environment_ready")) {
    if (-not $environmentPhysicalWeatherTask10Guidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationTask10 missing: $needle"
    }
}
$environmentPhysicalWeatherGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationPhase10
foreach ($needle in @("EnvironmentWeatherSimulationDesc", "EnvironmentWeatherSimulationCellState", "EnvironmentWeatherSimulationCellForcing", "EnvironmentWeatherSimulationPlan", "EnvironmentWeatherSimulationSolverBudgetDesc", "EnvironmentWeatherSimulationSolverBudgetPlan", "EnvironmentWeatherSimulationSolverProfilerArtifactRow", "EnvironmentWeatherSimulationValidationDatasetDesc", "EnvironmentWeatherSimulationValidationDatasetPlan", "EnvironmentWeatherSimulationValidationImageDesc", "EnvironmentWeatherSimulationValidationImagePlan", "EnvironmentWeatherSimulationValidationImageDiagnosticCode", "EnvironmentWeatherSimulationArtistControlDesc", "EnvironmentWeatherSimulationArtistControlPlan", "EnvironmentWeatherSimulationArtistControlDiagnosticCode", "D3d12EnvironmentWeatherSolverDesc", "D3d12EnvironmentWeatherSolverResult", "VulkanEnvironmentWeatherSolverDesc", "VulkanEnvironmentWeatherSolverResult", "dispatch_environment_weather_solver", "plan_environment_weather_simulation_validation_dataset", "plan_environment_weather_simulation_validation_images", "plan_environment_weather_simulation_artist_controls", "environment_weather_saturation_vapor_kg_per_m2", "simulate_environment_weather_cpu_reference", "plan_environment_weather_simulation_solver_budget", "production_solver_package_counter_reviewed", "production_solver_package_counter_review_ready", "production_solver_core_reviewed", "production_solver_core_review_ready", "missing_production_solver_core_evidence", "broad_physical_accuracy_reviewed", "broad_physical_accuracy_review_ready", "missing_broad_physical_accuracy_evidence", "visual_quality_reviewed", "visual_quality_review_ready", "missing_visual_quality_evidence", "physical_weather_ready", "MK_environment_weather_simulation_tests", "MK_d3d12_environment_weather_solver_tests", "MK_vulkan_environment_weather_solver_tests", "shaders/environment/weather_simulation.hlsl", "DXC -spirv -fspv-target-env=vulkan1.3", "spirv-val", "NWS vapor-pressure formula", "effective_timestep_s", "water", "deterministic replay", "validation dataset", "validation image hashes", "artist-control hash", "vapor_water_after", "cloud_water_after", "surface_water_after", "water_transfer", "relative humidity percent", "cloud cover percent", "surface wetness percent", "evaporation intensity percent", "precipitation intensity percent", "desktop-runtime-sample-game-environment-weather-simulation-package", "--require-environment-weather-simulation-package", "desktop-runtime-sample-game-environment-weather-simulation-vulkan-solver-package", "--require-environment-weather-simulation-vulkan-solver-package", "environment_weather_simulation_package_ready=1", "environment_weather_simulation_steps=1", "environment_weather_simulation_solver_budget_status=host_evidence_required", "environment_weather_simulation_cpu_reference_solver_ready=1", "environment_weather_simulation_gpu_solver_ready=1", "environment_weather_simulation_solver_gpu_budget_us=500000", "environment_weather_simulation_d3d12_gpu_solver_ready=1", "environment_weather_simulation_d3d12_gpu_solver_cells=4", "environment_weather_simulation_d3d12_gpu_solver_dispatches=1", "environment_weather_simulation_d3d12_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_vulkan_gpu_solver_ready=1", "environment_weather_simulation_vulkan_gpu_solver_strict_ready=1", "environment_weather_simulation_vulkan_gpu_solver_cells=4", "environment_weather_simulation_vulkan_gpu_solver_dispatches=1", "environment_weather_simulation_vulkan_gpu_solver_descriptor_set_bindings=3", "environment_weather_simulation_vulkan_gpu_solver_barriers", "environment_weather_simulation_vulkan_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_vulkan_gpu_solver_d3d12_inferred=0", "environment_weather_simulation_vulkan_gpu_solver_metal_inferred=0", "environment_weather_simulation_vulkan_gpu_solver_failure_stage=0", "environment_weather_simulation_vulkan_gpu_solver_hash", "environment_weather_simulation_vulkan_gpu_solver_elapsed_us", "environment_weather_simulation_vulkan_gpu_solver_budget_us=500000", "environment_weather_simulation_vulkan_gpu_solver_over_budget=0", "environment_weather_simulation_vulkan_gpu_solver_profiler_budget_ready=1", "environment_weather_simulation_solver_profiler_artifacts=2", "environment_weather_simulation_solver_profiler_tool_rows=2", "environment_weather_simulation_solver_profiler_backend_rows=1", "environment_weather_simulation_solver_profiler_artifact_hash", "environment_weather_simulation_profiler_budget_ready=1", "environment_weather_simulation_production_solver_package_counter_review_ready=1", "environment_weather_simulation_production_solver_package_counter_rows=1", "environment_weather_simulation_production_solver_core_review_ready=1", "environment_weather_simulation_production_solver_core_rows=1", "environment_weather_simulation_broad_physical_accuracy_review_ready=1", "environment_weather_simulation_broad_physical_accuracy_rows=1", "environment_weather_simulation_visual_quality_review_ready=1", "environment_weather_simulation_visual_quality_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_physical_weather_ready=0", "environment_weather_simulation_validation_dataset_status=ready", "environment_weather_simulation_validation_dataset_ready=1", "environment_weather_simulation_validation_image_status=ready", "environment_weather_simulation_validation_images_ready=1", "environment_weather_simulation_validation_image_rows=12", "environment_weather_simulation_validation_image_diagnostics=0", "environment_weather_simulation_artist_control_status=ready", "environment_weather_simulation_artist_controls_ready=1", "environment_weather_simulation_artist_control_rows=4", "environment_weather_simulation_artist_control_generated_cells=4", "environment_weather_simulation_artist_control_diagnostics=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend execution", "native handle access", "raw solver internal access", "broad physical accuracy", "visual quality", "environment_physical_weather_simulation_ready=1 may be inferred")) {
    if (-not $environmentPhysicalWeatherGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationPhase10 missing: $needle"
    }
}
$environmentPhysicalWeatherMetalGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationMetalHostGatePhase10
foreach ($needle in @("MetalEnvironmentWeatherSolverDesc", "MetalEnvironmentWeatherSolverResult", "metal_environment_weather_solver_cell_row_stride_bytes", "dispatch_environment_weather_solver", "MK_metal_environment_weather_solver_tests", "engine/rhi/metal/shaders/weather_simulation.metal", "environment-weather-metal-solver-host-gate", "environment_weather_simulation_metal_gpu_solver_ready=1", "host_validation_recipe_id=environment-weather-metal-solver-host-gate", "selected_backend=metal", "cells=4", "dispatches=1", "buffer_bindings=3", "command_queue_ready=1", "command_buffer_ready=1", "metallib_valid=1", "compute_pipeline_ready=1", "elapsed_us", "budget_us=500000", "native_handle_access=0", "backend_parity_ready=0", "d3d12_inferred=0", "vulkan_inferred=0", "failure_stage=0", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_physical_weather_ready=0", "environment_physical_weather_simulation_ready=0", "Metal aggregate readiness", "Vulkan/Metal solver parity", "production solver readiness", "complete physical weather simulation", "broad environment_ready")) {
    if (-not $environmentPhysicalWeatherMetalGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationMetalHostGatePhase10 missing: $needle"
    }
}
$environmentArtistWorkflowGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11
foreach ($needle in @("EnvironmentArtistWorkflowCommandKind", "EnvironmentArtistWorkflowCommandRequest", "EnvironmentArtistWorkflowCommandCatalog", "EnvironmentArtistWorkflowCommandPlan", "EnvironmentArtistWorkflowCommandReportRow", "make_environment_artist_workflow_command_catalog", "plan_environment_artist_workflow_command", "environment.command.preset.import", "environment.command.source_asset.review", "environment.command.cook.preview", "environment.command.profile_graph.edit", "environment.command.weather_timeline.edit", "environment.command.local_volume.edit", "environment.command.simulation_parameter.edit", "environment.command.quality_budget.edit", "environment.command.package.preview", "environment.command.validation.remediation", "environment.command.publish.package", "dry-run", "revision-checked", "rejected_stale_revision", "environment.workflow.*", "undo/rollback", "EnvironmentArtistWorkflowAssetKind", "EnvironmentArtistWorkflowAssetBrowserDesc", "make_environment_artist_workflow_asset_browser_model", "environment.workflow.asset.preset_library", "EnvironmentArtistWorkflowPreviewRowKind", "EnvironmentArtistWorkflowPreviewRowStatus", "EnvironmentArtistWorkflowPreviewDesc", "EnvironmentArtistWorkflowPreviewRow", "EnvironmentArtistWorkflowPreviewModel", "make_environment_artist_workflow_preview_model", "make_environment_artist_workflow_preview_ui_model", "environment.workflow.preview.selected_backend", "environment.workflow.preview.quality_tier", "environment.workflow.preview.missing_host_gate", "environment.workflow.preview.package_budget", "environment.workflow.preview.memory_budget", "environment.workflow.preview.diagnostics", "environment.workflow.preview.unsupported_claim_reason", "EnvironmentArtistWorkflowWalkthroughStepKind", "EnvironmentArtistWorkflowWalkthroughStepStatus", "EnvironmentArtistWorkflowWalkthroughStepInputRow", "EnvironmentArtistWorkflowWalkthroughDesc", "EnvironmentArtistWorkflowWalkthroughStepRow", "EnvironmentArtistWorkflowWalkthroughModel", "environment_artist_workflow_walkthrough_step_id", "make_environment_artist_workflow_walkthrough_model", "make_environment_artist_workflow_walkthrough_ui_model", "environment.workflow.walkthrough.import_source_assets", "environment.workflow.walkthrough.cook_assets", "environment.workflow.walkthrough.assemble_preset", "environment.workflow.walkthrough.edit_weather_timeline", "environment.workflow.walkthrough.run_simulation_preview", "environment.workflow.walkthrough.package_sample", "environment.workflow.walkthrough.run_installed_validation", "environment.workflow.walkthrough.inspect_report", "complete_artist_workflow_ready_claimed=false", "backend execution", "package script execution", "validation recipe execution", "native handle access", "environment_artist_workflow_ready=1")) {
    if (-not $environmentArtistWorkflowGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11 missing: $needle"
    }
}
foreach ($needle in @("EnvironmentArtistWorkflowExecutionStageStatus", "EnvironmentArtistWorkflowExecutionEvidenceRow", "EnvironmentArtistWorkflowExecutionReviewDesc", "EnvironmentArtistWorkflowExecutionReviewStageRow", "EnvironmentArtistWorkflowExecutionReviewModel", "environment_artist_workflow_execution_stage_status_label", "make_environment_artist_workflow_execution_review_model", "make_environment_artist_workflow_execution_review_ui_model", "environment.workflow.execution.command_catalog", "environment.workflow.execution.asset_browser", "environment.workflow.execution.preview", "environment.workflow.execution.walkthrough", "environment.workflow.execution.external_execution", "environment.workflow.execution.evidence_review", "environment.workflow.execution.operator_review", "environment.workflow.execution.ready_promotion_guard", "externally_supplied evidence review", "operator review", "visible_first_party_workflow_wired", "external_action_required", "evidence_review_required", "editor-core execution claims", "ready promotion requests")) {
    if (-not $environmentArtistWorkflowGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11 missing slice 5: $needle"
    }
}
foreach ($needle in @("EnvironmentArtistWorkflowReadyRequirementKind", "EnvironmentArtistWorkflowReadyRequirementStatus", "EnvironmentArtistWorkflowReadyRequirementInputRow", "EnvironmentArtistWorkflowReadyReviewDesc", "EnvironmentArtistWorkflowReadyRequirementRow", "EnvironmentArtistWorkflowReadyReviewModel", "environment_artist_workflow_ready_requirement_id", "environment_artist_workflow_ready_requirement_status_label", "make_environment_artist_workflow_ready_review_model", "make_environment_artist_workflow_ready_review_ui_model", "environment.workflow.ready.visible_editor_shell", "environment.workflow.ready.asset_pipeline", "environment.workflow.ready.selected_preset_library", "environment.workflow.ready.validation_remediation", "environment.workflow.ready.revision_safety", "environment.workflow.ready.production_walkthrough_package", "environment.workflow.ready.editor_core_execution_boundary", "environment.workflow.ready.operator_review", "desktop-runtime-sample-game-environment-artist-workflow-package", "--require-environment-artist-workflow-package", "environment_artist_workflow_package_status=ready", "environment_artist_workflow_package_ready=1", "environment_artist_workflow_ready=1", "environment_artist_workflow_requirement_rows=8", "environment_artist_workflow_ready_rows=8", "environment_artist_workflow_editor_core_backend_execution=0", "environment_artist_workflow_editor_core_package_script_execution=0", "environment_artist_workflow_editor_core_validation_recipe_execution=0", "environment_artist_workflow_native_handle_access=0", "environment_artist_workflow_diagnostics=0", "environment_artist_workflow_commercial_ready=0", "environment_commercial_ready", "backend parity", "all-platform readiness", "broad optimization")) {
    if (-not $environmentArtistWorkflowGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11 missing slice 6: $needle"
    }
}
foreach ($needle in @("Phase 11 slice 7", "NativeEditorEnvironmentArtistWorkflowCommandPlanRow", "environment_artist_workflow_command_plans", "environment_artist_workflow_execution_review", "environment_artist_workflow_shell_execution_bridge", "FirstPartyEditorShellSmokeCounters", "environment_artist_workflow_* rows", "five command-plan rows", "eight execution-review rows", "one external execution row", "one operator review row", "dry-run/apply reports", "revision-checked apply", "undo/rollback metadata", "publish confirmation", "zero backend/package-script/validation-recipe/native-handle execution", "complete_artist_workflow_ready_claimed=false")) {
    if (-not $environmentArtistWorkflowGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11 missing slice 7: $needle"
    }
}
foreach ($needle in @("Environment Highest Commercial Readiness v1 Task 11", "EnvironmentArtistWorkflowProductionRequirementKind", "EnvironmentArtistWorkflowProductionRequirementStatus", "EnvironmentArtistWorkflowProductionRequirementInputRow", "EnvironmentArtistWorkflowProductionCloseoutDesc", "EnvironmentArtistWorkflowProductionRequirementRow", "EnvironmentArtistWorkflowProductionCloseoutModel", "environment_artist_workflow_production_requirement_id", "environment_artist_workflow_production_requirement_status_label", "environment_artist_workflow_production_closeout_status_label", "evaluate_environment_artist_workflow_production_closeout", "make_environment_artist_workflow_production_closeout_ui_model", "tools/validate-environment-artist-workflow-production.ps1", "workflow_import_openexr_ready=1", "workflow_import_ktx2_basis_ready=1", "workflow_import_gltf_material_ready=1", "workflow_review_usd_materialx_ocio_ready=1", "workflow_cook_package_ready=1", "workflow_live_preview_d3d12_ready=1", "workflow_live_preview_vulkan_ready=1", "workflow_live_preview_metal_host_ready=1", "workflow_weather_timeline_edit_ready=1", "workflow_preset_batch_apply_ready=1", "workflow_validation_report_ready=1", "workflow_profiler_artifact_review_ready=1", "workflow_undo_redo_revision_safety_ready=1", "workflow_operator_review_ready=1", "environment_artist_workflow_production_requirement_rows=14", "environment_artist_workflow_production_ready_rows=14", "environment_artist_workflow_production_ready=1", "environment_artist_workflow_editor_core_backend_execution=0", "environment_artist_workflow_editor_core_package_script_execution=0", "environment_artist_workflow_editor_core_validation_recipe_execution=0", "environment_artist_workflow_native_handle_access=0", "environment_ready=0", "environment_commercial_ready=0")) {
    if (-not $environmentArtistWorkflowGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentArtistWorkflowPhase11 missing Task 11: $needle"
    }
}
