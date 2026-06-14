#requires -Version 7.0
#requires -PSEdition Core

# Chapter 35 for check-json-contracts.ps1 static contracts.
$engineForEnvironmentCommercial = Read-Json "engine/agent/manifest.json"
$environmentCommercialSchema = Read-Json "schemas/engine-agent/ai-operable-production-loop.schema.json"
$environmentCommercialLoop = $engineForEnvironmentCommercial.aiOperableProductionLoop

foreach ($requiredSchemaField in @("environmentCommercialClaimMatrix", "environmentCommercialUnsupportedAdjacentClaims")) {
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
    environment_platform_linux_vulkan_ready = "host-gated"
    environment_platform_macos_metal_ready = "host-gated"
    environment_platform_ios_metal_ready = "host-gated"
    environment_platform_android_vulkan_ready = "host-gated"
    environment_broad_optimization_ready = "unsupported"
    environment_asset_pipeline_openexr_ktx_basis_ready = "unsupported"
    environment_aaa_preset_library_ready = "unsupported"
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
