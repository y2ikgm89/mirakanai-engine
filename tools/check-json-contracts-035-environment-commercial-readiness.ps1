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
foreach ($validationRecipe in @($engineForEnvironmentCommercial.validationRecipes)) {
    $environmentCommercialValidationRecipeNames[[string]$validationRecipe.name] = $true
}

$expectedEnvironmentCommercialClaimIds = @(
    "environment_commercial_ready",
    "environment_vulkan_strict_aggregate_ready",
    "environment_metal_host_aggregate_ready",
    "environment_backend_parity_ready",
    "environment_platform_windows_d3d12_ready",
    "environment_platform_windows_vulkan_ready",
    "environment_platform_linux_vulkan_ready",
    "environment_platform_macos_metal_ready",
    "environment_platform_ios_metal_ready",
    "environment_platform_android_vulkan_ready",
    "environment_broad_optimization_ready",
    "environment_asset_pipeline_openexr_ktx_basis_ready",
    "environment_aaa_preset_library_ready",
    "environment_physical_weather_simulation_ready",
    "environment_artist_workflow_ready"
)
$expectedEnvironmentCommercialClaimStates = @{
    environment_commercial_ready = "unsupported"
    environment_vulkan_strict_aggregate_ready = "host-gated"
    environment_metal_host_aggregate_ready = "host-gated"
    environment_backend_parity_ready = "unsupported"
    environment_platform_windows_d3d12_ready = "ready"
    environment_platform_windows_vulkan_ready = "host-gated"
    environment_platform_linux_vulkan_ready = "host-gated"
    environment_platform_macos_metal_ready = "host-gated"
    environment_platform_ios_metal_ready = "host-gated"
    environment_platform_android_vulkan_ready = "host-gated"
    environment_broad_optimization_ready = "unsupported"
    environment_asset_pipeline_openexr_ktx_basis_ready = "unsupported"
    environment_aaa_preset_library_ready = "ready"
    environment_physical_weather_simulation_ready = "unsupported"
    environment_artist_workflow_ready = "unsupported"
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
$backendParityClaim = $environmentCommercialClaimsById["environment_backend_parity_ready"]
if ($null -eq $backendParityClaim -or
    @($backendParityClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-backend-parity" -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("plan_environment_backend_parity") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("normalized feature id") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("profile revision") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("preset pack revision") -or
    -not [string]$backendParityClaim.requiredEvidence.Contains("counter semantics")) {
    Write-Error "engine manifest environment_backend_parity_ready must require the MK_renderer environment parity matrix contract"
}
$broadOptimizationClaim = $environmentCommercialClaimsById["environment_broad_optimization_ready"]
if ($null -eq $broadOptimizationClaim -or
    @($broadOptimizationClaim.validationRecipeIds) -notcontains "desktop-runtime-sample-game-environment-optimization-measurement" -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("plan_environment_optimization_measurement") -or
    -not [string]$broadOptimizationClaim.requiredEvidence.Contains("before/after traces") -or
    -not [string]$broadOptimizationClaim.notes.Contains("environment_broad_optimization_ready=0")) {
    Write-Error "engine manifest environment_broad_optimization_ready must require the MK_renderer optimization measurement contract without ready promotion"
}
$assetPipelineClaim = $environmentCommercialClaimsById["environment_asset_pipeline_openexr_ktx_basis_ready"]
if ($null -eq $assetPipelineClaim -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "asset-importers" -or
    @($assetPipelineClaim.validationRecipeIds) -notcontains "agent-contract" -or
    [string]$assetPipelineClaim.state -ne "unsupported" -or
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
    -not [string]$assetPipelineClaim.notes.Contains("selected D3D12 WARP runtime upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected strict Vulkan runtime upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("selected D3D12 WARP backend-target BC7 compressed upload/readback") -or
    -not [string]$assetPipelineClaim.notes.Contains("Apple-host Metal upload execution") -or
    -not [string]$assetPipelineClaim.notes.Contains("Vulkan BC7 and Metal/ASTC compressed payload execution") -or
    -not [string]$assetPipelineClaim.notes.Contains("broad asset-pipeline readiness remain future work")) {
    Write-Error "engine manifest environment_asset_pipeline_openexr_ktx_basis_ready must record the selected source-to-package cooker, backend-target decision fields, selected D3D12/Vulkan RGBA8 upload/readback evidence, selected D3D12 BC7 compressed upload/readback evidence, and remaining Metal/Vulkan-compressed non-claims"
}
foreach ($broadClaimId in @("environment_commercial_ready", "environment_backend_parity_ready", "environment_broad_optimization_ready", "environment_aaa_preset_library_ready", "environment_physical_weather_simulation_ready", "environment_artist_workflow_ready")) {
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
        state = "host-gated"
        needles = @("Vulkan SDK", "validation layers", "SPIR-V", "D3D12", "environment_platform_windows_vulkan_ready=0")
    },
    @{
        id = "environment_platform_linux_vulkan"
        claimId = "environment_platform_linux_vulkan_ready"
        state = "host-gated"
        needles = @("Linux Vulkan", "Windows Vulkan evidence", "Vulkan SDK", "validation layers", "environment_platform_linux_vulkan_ready=0")
    },
    @{
        id = "environment_platform_macos_metal"
        claimId = "environment_platform_macos_metal_ready"
        state = "host-gated"
        needles = @("Xcode", "Metal", "Windows", "environment_platform_macos_metal_ready=0")
    },
    @{
        id = "environment_platform_ios_metal"
        claimId = "environment_platform_ios_metal_ready"
        state = "host-gated"
        needles = @("Xcode", "simulator", "signing", "macOS desktop Metal", "environment_platform_ios_metal_ready=0")
    },
    @{
        id = "environment_platform_android_vulkan"
        claimId = "environment_platform_android_vulkan_ready"
        state = "host-gated"
        needles = @("Android SDK", "NDK", "Vulkan", "desktop Vulkan", "environment_platform_android_vulkan_ready=0")
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
    if ([string]$environmentCommercialClaimsById["environment_physical_weather_simulation_ready"].state -ne "unsupported") {
        Write-Error "engine manifest environment_physical_weather_simulation_ready must remain unsupported after selected physical-weather evidence rows"
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
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowCommandKind", "EnvironmentArtistWorkflowCommandRequest", "EnvironmentArtistWorkflowCommandCatalog", "EnvironmentArtistWorkflowCommandPlan", "EnvironmentArtistWorkflowCommandReportRow", "make_environment_artist_workflow_command_catalog", "plan_environment_artist_workflow_command", "environment.command.preset.import", "environment.command.source_asset.review", "environment.command.cook.preview", "environment.command.profile_graph.edit", "environment.command.weather_timeline.edit", "environment.command.local_volume.edit", "environment.command.simulation_parameter.edit", "environment.command.quality_budget.edit", "environment.command.package.preview", "environment.command.validation.remediation", "environment.command.publish.package", "dry_run", "revision_checked_apply", "expected_revision", "rejected_stale_revision", "environment.workflow.command_id", "environment.workflow.mode", "environment.workflow.before_revision", "environment.workflow.after_revision", "environment.workflow.revision_checked", "environment.workflow.undo_supported", "environment.workflow.rollback_metadata", "backend work", "package scripts", "native handle access", "complete artist workflow readiness remains unsupported")
    },
    @{
        id = "environment_artist_workflow_asset_browser_foundation"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowAssetKind", "EnvironmentArtistWorkflowAssetBrowserInputRow", "EnvironmentArtistWorkflowAssetBrowserDesc", "EnvironmentArtistWorkflowAssetBrowserRow", "EnvironmentArtistWorkflowAssetBrowserModel", "make_environment_artist_workflow_asset_browser_model", "make_environment_artist_workflow_asset_browser_ui_model", "environment.workflow.asset.preset_library", "environment.workflow.asset.openexr_source", "environment.workflow.asset.ktx2_basis_source", "environment.workflow.asset.cooked_texture", "environment.workflow.asset.environment_profile", "environment.workflow.asset.simulation_preset", "environment.workflow.asset.validation_report", "environment.workflow.asset.package_artifact", "package visibility", "provenance", "budget", "host gate", "validation recipe", "read_only", "review_only", "backend work", "package scripts", "native handle access", "complete artist workflow readiness remains unsupported")
    },
    @{
        id = "environment_artist_workflow_preview_rows_foundation"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowPreviewRowKind", "EnvironmentArtistWorkflowPreviewRowStatus", "EnvironmentArtistWorkflowPreviewDesc", "EnvironmentArtistWorkflowPreviewRow", "EnvironmentArtistWorkflowPreviewModel", "environment_artist_workflow_preview_row_id", "make_environment_artist_workflow_preview_model", "make_environment_artist_workflow_preview_ui_model", "environment.workflow.preview.selected_backend", "environment.workflow.preview.quality_tier", "environment.workflow.preview.missing_host_gate", "environment.workflow.preview.package_budget", "environment.workflow.preview.memory_budget", "environment.workflow.preview.diagnostics", "environment.workflow.preview.unsupported_claim_reason", "selected backend", "quality tier", "missing host gate", "package budget", "memory budget", "diagnostics", "unsupported claim reason", "complete_artist_workflow_ready_claimed=false", "read_only", "review_only", "backend work", "package scripts", "native handle access", "complete artist workflow readiness remains unsupported")
    },
    @{
        id = "environment_artist_workflow_walkthrough_value_model"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowWalkthroughStepKind", "EnvironmentArtistWorkflowWalkthroughStepStatus", "EnvironmentArtistWorkflowWalkthroughStepInputRow", "EnvironmentArtistWorkflowWalkthroughDesc", "EnvironmentArtistWorkflowWalkthroughStepRow", "EnvironmentArtistWorkflowWalkthroughDiagnosticRow", "EnvironmentArtistWorkflowWalkthroughModel", "environment_artist_workflow_walkthrough_step_id", "make_environment_artist_workflow_walkthrough_model", "make_environment_artist_workflow_walkthrough_ui_model", "environment.workflow.walkthrough.import_source_assets", "environment.workflow.walkthrough.cook_assets", "environment.workflow.walkthrough.assemble_preset", "environment.workflow.walkthrough.edit_weather_timeline", "environment.workflow.walkthrough.run_simulation_preview", "environment.workflow.walkthrough.package_sample", "environment.workflow.walkthrough.run_installed_validation", "environment.workflow.walkthrough.inspect_report", "read_only", "review_only", "complete_artist_workflow_ready_claimed=false", "backend execution", "package script execution", "validation recipe execution", "native handle access", "ready promotion", "environment_artist_workflow_ready must remain unsupported", "visible first-party workflow wiring remains future")
    },
    @{
        id = "environment_artist_workflow_visible_execution_review"
        claimId = "environment_artist_workflow_ready"
        state = "ready"
        needles = @("MK_editor_core", "EnvironmentArtistWorkflowExecutionStageStatus", "EnvironmentArtistWorkflowExecutionEvidenceRow", "EnvironmentArtistWorkflowExecutionReviewDesc", "EnvironmentArtistWorkflowExecutionReviewStageRow", "EnvironmentArtistWorkflowExecutionReviewModel", "environment_artist_workflow_execution_stage_status_label", "make_environment_artist_workflow_execution_review_model", "make_environment_artist_workflow_execution_review_ui_model", "environment.workflow.execution.command_catalog", "environment.workflow.execution.asset_browser", "environment.workflow.execution.preview", "environment.workflow.execution.walkthrough", "environment.workflow.execution.external_execution", "environment.workflow.execution.evidence_review", "environment.workflow.execution.operator_review", "environment.workflow.execution.ready_promotion_guard", "externally_supplied_evidence", "operator_review", "visible execution/review", "complete_artist_workflow_ready_claimed=false", "backend execution", "package script execution", "validation recipe execution", "native handle access", "editor-core execution claims", "ready promotion", "environment_artist_workflow_ready must remain unsupported")
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
    if ([string]$artistRow.claimId -ne "environment_artist_workflow_ready") {
        Write-Error "engine manifest environmentArtistWorkflowRows '$artistRowId' must map to environment_artist_workflow_ready"
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
    if ([string]$environmentCommercialClaimsById["environment_artist_workflow_ready"].state -ne "unsupported") {
        Write-Error "engine manifest environment_artist_workflow_ready must remain unsupported after selected artist workflow foundation rows"
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

$environmentCommercialCounterRuleText = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentCommercialClaimCounterRules
foreach ($needle in @("environmentCommercialClaimMatrix", "packageCounter equal to the claim id", "lower_snake_case", "environmentCommercialUnsupportedAdjacentClaims")) {
    if (-not $environmentCommercialCounterRuleText.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentCommercialClaimCounterRules missing: $needle"
    }
}
$environmentBackendParityGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentBackendParityPhase7
foreach ($needle in @("desktop-runtime-sample-game-environment-backend-parity", "EnvironmentBackendParityRequest", "plan_environment_backend_parity", "normalized feature ids", "same profile revision", "same preset pack revision", "counter semantics", "host_evidence_required", "environment_backend_parity_ready=0")) {
    if (-not $environmentBackendParityGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentBackendParityPhase7 missing: $needle"
    }
}
$environmentPlatformReadinessGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPlatformReadinessPhase8
foreach ($needle in @("desktop-runtime-sample-game-environment-platform-readiness", "environmentPlatformReadinessRows", "Windows D3D12", "Windows Vulkan", "Linux Vulkan", "macOS Metal", "iOS Metal", "Android Vulkan", "environment_platform_readiness_status=host_evidence_required", "environment_platform_readiness_ready=0", "environment_all_platform_unconditional_ready=0")) {
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
$environmentPhysicalWeatherGuidance = [string]$engineForEnvironmentCommercial.gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationPhase10
foreach ($needle in @("EnvironmentWeatherSimulationDesc", "EnvironmentWeatherSimulationCellState", "EnvironmentWeatherSimulationCellForcing", "EnvironmentWeatherSimulationPlan", "EnvironmentWeatherSimulationSolverBudgetDesc", "EnvironmentWeatherSimulationSolverBudgetPlan", "EnvironmentWeatherSimulationSolverProfilerArtifactRow", "EnvironmentWeatherSimulationValidationDatasetDesc", "EnvironmentWeatherSimulationValidationDatasetPlan", "EnvironmentWeatherSimulationValidationImageDesc", "EnvironmentWeatherSimulationValidationImagePlan", "EnvironmentWeatherSimulationValidationImageDiagnosticCode", "EnvironmentWeatherSimulationArtistControlDesc", "EnvironmentWeatherSimulationArtistControlPlan", "EnvironmentWeatherSimulationArtistControlDiagnosticCode", "D3d12EnvironmentWeatherSolverDesc", "D3d12EnvironmentWeatherSolverResult", "dispatch_environment_weather_solver", "plan_environment_weather_simulation_validation_dataset", "plan_environment_weather_simulation_validation_images", "plan_environment_weather_simulation_artist_controls", "environment_weather_saturation_vapor_kg_per_m2", "simulate_environment_weather_cpu_reference", "plan_environment_weather_simulation_solver_budget", "production_solver_package_counter_reviewed", "production_solver_package_counter_review_ready", "production_solver_core_reviewed", "production_solver_core_review_ready", "missing_production_solver_core_evidence", "MK_environment_weather_simulation_tests", "MK_d3d12_environment_weather_solver_tests", "NWS vapor-pressure formula", "effective_timestep_s", "water", "deterministic replay", "validation dataset", "validation image hashes", "artist-control hash", "vapor_water_after", "cloud_water_after", "surface_water_after", "water_transfer", "relative humidity percent", "cloud cover percent", "surface wetness percent", "evaporation intensity percent", "precipitation intensity percent", "desktop-runtime-sample-game-environment-weather-simulation-package", "--require-environment-weather-simulation-package", "environment_weather_simulation_package_ready=1", "environment_weather_simulation_steps=1", "environment_weather_simulation_solver_budget_status=host_evidence_required", "environment_weather_simulation_cpu_reference_solver_ready=1", "environment_weather_simulation_gpu_solver_ready=1", "environment_weather_simulation_solver_gpu_budget_us=500000", "environment_weather_simulation_d3d12_gpu_solver_ready=1", "environment_weather_simulation_d3d12_gpu_solver_cells=4", "environment_weather_simulation_d3d12_gpu_solver_dispatches=1", "environment_weather_simulation_d3d12_gpu_solver_backend_parity_ready=0", "environment_weather_simulation_solver_profiler_artifacts=2", "environment_weather_simulation_solver_profiler_tool_rows=2", "environment_weather_simulation_solver_profiler_backend_rows=1", "environment_weather_simulation_solver_profiler_artifact_hash", "environment_weather_simulation_profiler_budget_ready=1", "environment_weather_simulation_production_solver_package_counter_review_ready=1", "environment_weather_simulation_production_solver_package_counter_rows=1", "environment_weather_simulation_production_solver_core_review_ready=1", "environment_weather_simulation_production_solver_core_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_validation_dataset_status=ready", "environment_weather_simulation_validation_dataset_ready=1", "environment_weather_simulation_validation_image_status=ready", "environment_weather_simulation_validation_images_ready=1", "environment_weather_simulation_validation_image_rows=12", "environment_weather_simulation_validation_image_diagnostics=0", "environment_weather_simulation_artist_control_status=ready", "environment_weather_simulation_artist_controls_ready=1", "environment_weather_simulation_artist_control_rows=4", "environment_weather_simulation_artist_control_generated_cells=4", "environment_weather_simulation_artist_control_diagnostics=0", "environment_physical_weather_simulation_ready=0", "Vulkan", "Metal", "backend execution", "native handle access", "raw solver internal access", "broad physical accuracy", "visual quality", "environment_physical_weather_simulation_ready=1 may be inferred")) {
    if (-not $environmentPhysicalWeatherGuidance.Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEnvironmentPhysicalWeatherSimulationPhase10 missing: $needle"
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
