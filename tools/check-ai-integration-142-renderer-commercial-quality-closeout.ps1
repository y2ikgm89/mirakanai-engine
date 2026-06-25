#requires -Version 7.0
#requires -PSEdition Core
# Chapter 142 for Renderer Commercial Quality Closeout v1 active selection and fail-closed guard.

$validatorText = Get-AgentSurfaceText "tools/validate-renderer-commercial-quality-closeout.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$recipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-quality-closeout-v1.md"
$aggregateHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer_commercial_quality_closeout.hpp"
$aggregateSourceText = Get-AgentSurfaceText "engine/renderer/src/renderer_commercial_quality_closeout.cpp"
$aggregateTestsText = Get-AgentSurfaceText "tests/unit/renderer_commercial_quality_closeout_tests.cpp"
$backendParityHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp"
$backendParitySourceText = Get-AgentSurfaceText "engine/renderer/src/backend_renderer_parity_policy.cpp"
$rendererRhiTestsText = Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$metalHeaderText = Get-AgentSurfaceText "engine/rhi/metal/include/mirakana/rhi/metal/metal_backend.hpp"
$metalSourceText = Get-AgentSurfaceText "engine/rhi/metal/src/metal_backend.cpp"
$metalNativeText = Get-AgentSurfaceText "engine/rhi/metal/src/metal_native.mm"
$metalCMakeText = Get-AgentSurfaceText "engine/rhi/metal/CMakeLists.txt"
$metalShaderText = Get-AgentSurfaceText "engine/rhi/metal/shaders/visible_renderer_package_evidence.metal"
$backendScaffoldTestsText = Get-AgentSurfaceText "tests/unit/backend_scaffold_tests.cpp"
$metalAppleValidatorText = Get-AgentSurfaceText "tools/validate-renderer-metal-apple.ps1"
$recipePlansText = Get-AgentSurfaceText "tools/run-validation-recipe-plans.ps1"
$generated3dGameManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"

foreach ($needle in @(
        '[switch]$BackendParityReady',
        '[switch]$D3d12Ready',
        '[switch]$VulkanStrictReady',
        '[switch]$AppleMetalReady',
        '[switch]$RendererQualityMatrixReady',
        '[switch]$ProductionVfxProfilingReady',
        '[switch]$MetalMemoryProfilingReady',
        '[switch]$Visible3dPackageReady',
        '[switch]$RuntimeUiPackageReady',
        '[switch]$EnvironmentPackageReady',
        '[switch]$GeneratedGamePackageReady',
        '[switch]$StaticGuardsReady',
        '[switch]$IosMetalPackageEvidenceRequired',
        '[switch]$ExternalEngineApproval',
        '[switch]$BroadBackendParityClaim',
        '[switch]$CommercialRendererReadinessClaim',
        "validation_recipe=renderer-commercial-quality-closeout",
        'renderer_commercial_quality_closeout_status=$status',
        'renderer_commercial_quality_closeout_ready=$(ConvertTo-CounterBit $ready)',
        "renderer_commercial_quality_closeout_value_api_ready=1",
        'renderer_selected_evidence_rows=$($coreEvidenceRows.Count)',
        'renderer_selected_evidence_ready_rows=$readyRows',
        'renderer_selected_evidence_missing_rows=$missingEvidenceRows',
        'renderer_ios_metal_package_evidence_required=$(ConvertTo-CounterBit $IosMetalPackageEvidenceRequired)',
        "renderer_backend_parity_evidence_ready",
        "renderer_d3d12_renderer_quality_ready",
        "renderer_vulkan_strict_renderer_quality_ready",
        "renderer_apple_metal_renderer_quality_ready",
        "renderer_quality_matrix_ready",
        "renderer_production_vfx_profiling_ready",
        "renderer_metal_memory_profiling_ready",
        "renderer_visible_3d_package_ready",
        "renderer_runtime_ui_package_ready",
        "renderer_environment_package_ready",
        "renderer_generated_game_package_ready",
        "renderer_static_guards_ready",
        'renderer_clean_room_source_review_ready=$(ConvertTo-CounterBit $cleanRoomSourceReviewReady)',
        'renderer_third_party_notices_complete=$(ConvertTo-CounterBit $thirdPartyNoticesComplete)',
        'renderer_clean_room_legal_ready=$(ConvertTo-CounterBit ($cleanRoomSourceReviewReady -and $thirdPartyNoticesComplete -and $externalEngineApprovalReady))',
        'renderer_external_engine_approval_ready=$(ConvertTo-CounterBit $externalEngineApprovalReady)',
        'renderer_external_engine_api_used=$(ConvertTo-CounterBit $ExternalEngineApiUsed)',
        'renderer_external_engine_code_used=$(ConvertTo-CounterBit $ExternalEngineCodeUsed)',
        'renderer_external_engine_sample_used=$(ConvertTo-CounterBit $ExternalEngineSampleUsed)',
        'renderer_external_engine_asset_used=$(ConvertTo-CounterBit $ExternalEngineAssetUsed)',
        'renderer_external_engine_trademark_used=$(ConvertTo-CounterBit $ExternalEngineTrademarkUsed)',
        'renderer_external_engine_ui_expression_used=$(ConvertTo-CounterBit $ExternalEngineUiExpressionUsed)',
        'renderer_external_engine_compatibility_claims=$(ConvertTo-CounterBit $ExternalEngineCompatibilityClaims)',
        'renderer_external_engine_equivalence_claims=$(ConvertTo-CounterBit $ExternalEngineEquivalenceClaims)',
        'renderer_native_handle_access=$(ConvertTo-CounterBit $NativeHandleAccess)',
        'renderer_cross_backend_inference=$(ConvertTo-CounterBit $CrossBackendInference)',
        'renderer_external_engine_parity=$(ConvertTo-CounterBit $ExternalEngineParity)',
        'renderer_broad_backend_parity_claim=$(ConvertTo-CounterBit $BroadBackendParityClaim)',
        'renderer_broad_metal_readiness_claim=$(ConvertTo-CounterBit $BroadMetalReadinessClaim)',
        'renderer_broad_renderer_quality_claim=$(ConvertTo-CounterBit $BroadRendererQualityClaim)',
        'renderer_commercial_renderer_readiness_claim=$(ConvertTo-CounterBit $CommercialRendererReadinessClaim)',
        'renderer_third_party_notices_complete=$(ConvertTo-CounterBit $thirdPartyNoticesComplete)',
        'renderer_backend_parity_ready=$(ConvertTo-CounterBit $ready)',
        'renderer_metal_broad_readiness=$(ConvertTo-CounterBit $ready)',
        'renderer_broad_quality_ready=$(ConvertTo-CounterBit $ready)',
        'renderer_commercial_readiness=$(ConvertTo-CounterBit $ready)',
        "renderer_environment_ready=0",
        "require_ready_not_selected",
        "external_engine_material_without_approval",
        "broad_backend_parity_claim_without_exact_evidence",
        "broad_metal_readiness_claim_without_exact_evidence",
        "broad_renderer_quality_claim_without_exact_evidence",
        "commercial_renderer_readiness_claim_without_exact_evidence",
        "Renderer Commercial Quality Closeout v1 is not ready:"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-renderer-commercial-quality-closeout.ps1"
}

foreach ($needle in @(
        "RendererCommercialQualityCloseoutDesc",
        "RendererCommercialQualityCloseoutPlan",
        "RendererCommercialQualityEvidenceRow",
        "plan_renderer_commercial_quality_closeout",
        "metal_memory_profiling",
        "visible_3d_package",
        "runtime_ui_package",
        "environment_package",
        "generated_game_package",
        "claim_control",
        "clean_room",
        "renderer_commercial_readiness",
        "renderer_metal_broad_readiness",
        "renderer_broad_quality_ready",
        "renderer_backend_parity_ready",
        "external_engine_compatibility_claim"
    )) {
    Assert-ContainsText $aggregateHeaderText $needle "renderer commercial quality aggregate public header"
}

foreach ($needle in @(
        "kMetalMemoryProfilingRecipeId",
        "renderer-metal-memory-profiling-host-evidence",
        "renderer-commercial-quality-closeout",
        "cross_backend_inference",
        "external_engine_code_used",
        "third_party_notices_complete",
        "RendererCommercialQualityCloseoutStatus::host_evidence_required"
    )) {
    Assert-ContainsText $aggregateSourceText $needle "renderer commercial quality aggregate source"
}

foreach ($needle in @(
        "missing Metal backend parity",
        "missing Metal quality matrix",
        "dedicated Metal memory profiling evidence",
        "cross backend Metal memory proof transfer",
        "external engine code assets trademarks",
        "approved external material"
    )) {
    Assert-ContainsText $aggregateTestsText $needle "renderer commercial quality aggregate tests"
}

Assert-ContainsText $rootCMakeText "MK_renderer_commercial_quality_closeout_tests" "renderer commercial quality aggregate CMake test target"

Assert-ContainsText $commandsFragmentText "rendererCommercialQualityCloseoutCheck" "manifest commands renderer commercial quality closeout command"
Assert-ContainsText $commandsFragmentText "rendererCommercialQualityCloseoutRequireReady" "manifest commands renderer commercial quality closeout require-ready command"
Assert-ContainsText $recipesFragmentText "renderer-commercial-quality-closeout" "manifest validation recipe renderer commercial quality closeout"
Assert-ContainsText $recipesFragmentText "validate-renderer-commercial-quality-closeout.ps1" "manifest validation recipe renderer commercial quality closeout"
foreach ($needle in @(
        "renderer-commercial-quality-closeout",
        "renderer-commercial-closeout",
        "final-aggregate-fail-closed",
        "iOS Metal evidence is not selected by this recipe"
    )) {
    Assert-ContainsText $recipePlansText $needle "validation recipe runner renderer commercial quality closeout plan"
}

foreach ($needle in @(
        'std::string host_validation_recipe_id{"renderer-metal-memory-profiling-host-evidence"};',
        "BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc"
    )) {
    Assert-ContainsText $backendParityHeaderText $needle "backend renderer parity Metal memory/profiling recipe header"
}

foreach ($needle in @(
        "kAppleMetalMemoryProfilingHostValidationRecipeId",
        "renderer-metal-memory-profiling-host-evidence",
        "metal_host_validation_recipe_feature_compatible",
        "metal_host_validation_recipe_feature_mismatch",
        "BackendRendererParityFeatureKind::memory_residency",
        "BackendRendererParityFeatureKind::profiling_capture"
    )) {
    Assert-ContainsText $backendParitySourceText $needle "backend renderer parity Metal memory/profiling recipe source"
}

foreach ($needle in @(
        "rejects Apple Metal environment recipe on memory profiling proof rows",
        "rejects Apple Metal memory profiling recipe on environment proof rows",
        "keeps Metal parity false when memory profiling recipe family is host gated",
        "keeps Metal parity false when environment recipe family is host gated",
        "renderer-metal-memory-profiling-host-evidence"
    )) {
    Assert-ContainsText $rendererRhiTestsText $needle "backend renderer parity Metal recipe alignment tests"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-host-evidence",
        "check-renderer-metal-memory-profiling-host-evidence.ps1",
        "BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc",
        "make_backend_renderer_parity_apple_metal_memory_profiling_proofs",
        "only for memory_residency and profiling_capture",
        "does not satisfy synchronization"
    )) {
    Assert-ContainsText $recipesFragmentText $needle "manifest validation recipe renderer Metal memory/profiling proof recipe"
}

foreach ($needle in @(
        "renderer-metal-memory-profiling-host-evidence heap/residency-set/capture evidence",
        "feature-bound",
        "feature-family checked"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "manifest MK_renderer Metal recipe family guidance"
}

foreach ($skillPath in @(
        ".agents/skills/rendering-change/references/full-guidance.md",
        ".agents/skills/gameengine-rendering/references/full-guidance.md",
        ".claude/skills/gameengine-rendering/references/full-guidance.md"
    )) {
    $skillText = Get-AgentSurfaceText $skillPath
    foreach ($needle in @(
            "renderer-metal-memory-profiling-host-evidence",
            "feature-bound",
            "must not be swapped across proof families"
        )) {
        Assert-ContainsText $skillText $needle "$skillPath renderer Metal recipe family guidance"
    }
}

foreach ($needle in @(
        "MetalVisibleRendererPackageEvidenceDesc",
        "MetalVisibleRendererPackageEvidenceRow",
        "MetalVisibleRendererPackageEvidencePlan",
        "MetalNativeVisibleRendererPackageEvidenceDesc",
        "MetalNativeVisibleRendererPackageEvidenceResult",
        "plan_metal_visible_renderer_package_evidence",
        "metal_visible_renderer_package_evidence_status_line",
        "create_native_visible_renderer_package_evidence"
    )) {
    Assert-ContainsText $metalHeaderText $needle "Metal visible renderer package evidence public value API"
}

foreach ($needle in @(
        "renderer_metal_visible_package_evidence_status=",
        "renderer_metal_visible_3d_package_ready",
        "renderer_metal_visible_ui_atlas_package_ready",
        "renderer_metal_visible_environment_package_ready",
        "renderer_metal_visible_generated_game_package_ready",
        "renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $metalSourceText $needle "Metal visible renderer package evidence status line"
}

foreach ($needle in @(
        "mk_visible_renderer_package_vertex",
        "mk_visible_renderer_package_fragment",
        "create_native_visible_renderer_package_evidence",
        "visible_3d_scene_material_lighting_postprocess_ready",
        "ui_atlas_upload_readback_ready",
        "environment_renderer_package_row_consumed",
        "generated_game_package_output_row_ready",
        "Metal visible renderer package native evidence ready"
    )) {
    Assert-ContainsText $metalNativeText $needle "Apple-private Metal visible renderer package native evidence"
}

foreach ($needle in @(
        "visible_renderer_package_evidence.metal",
        "MK_metal_visible_renderer_package_evidence_metallib",
        "MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB"
    )) {
    Assert-ContainsText $metalCMakeText $needle "Metal visible renderer package evidence metallib target"
}

foreach ($needle in @(
        "MK_metal_visible_renderer_package_evidence_metallib",
        "MK_METAL_VISIBLE_RENDERER_PACKAGE_EVIDENCE_METALLIB"
    )) {
    Assert-ContainsText $rootCMakeText $needle "backend scaffold visible package metallib test dependency"
}

foreach ($needle in @(
        "albedo",
        "light_direction",
        "postprocessed"
    )) {
    Assert-ContainsText $metalShaderText $needle "first-party Metal visible renderer package evidence shader"
}

foreach ($needle in @(
        "metal visible renderer package evidence stays host gated off Apple host",
        "metal visible renderer package evidence requires all selected rows",
        "metal visible renderer package evidence rejects native handles and broad claims",
        "metal visible renderer package evidence records deterministic selected package rows",
        "metal visible renderer package native evidence promotes only on Apple host execution"
    )) {
    Assert-ContainsText $backendScaffoldTestsText $needle "Metal visible renderer package evidence backend scaffold tests"
}

foreach ($needle in @(
        "MK_metal_visible_renderer_package_evidence_metallib",
        "renderer_metal_visible_package_evidence_status=ready",
        "renderer_metal_visible_3d_package_ready=1",
        "renderer_metal_visible_ui_atlas_package_ready=1",
        "renderer_metal_visible_environment_package_ready=1",
        "renderer_metal_visible_generated_game_package_ready=1",
        "renderer_metal_visible_package_native_handle_access=0",
        "renderer_metal_visible_package_broad_claims=0",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $metalAppleValidatorText $needle "renderer Metal Apple visible package evidence validator counters"
}

foreach ($needle in @(
        "visible_renderer_package_evidence.metallib",
        "renderer_metal_visible_package_* counters",
        "does not mark backend parity, broad Metal readiness, broad renderer quality, commercial renderer readiness"
    )) {
    Assert-ContainsText $recipePlansText $needle "renderer Metal Apple recipe plan visible package diagnostics"
}

foreach ($needle in @(
        "renderer-metal-visible-package-evidence",
        "renderer_metal_visible_package_evidence_status=ready",
        "renderer-metal-apple-host-evidence",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $generated3dGameManifestText $needle "generated 3D package Metal visible evidence manifest row"
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer commercial quality closeout plan" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-quality-closeout-v1",
            "docs/superpowers/plans/2026-06-25-renderer-commercial-quality-closeout-v1.md",
            "renderer_backend_parity_ready",
            "renderer_metal_broad_readiness",
            "renderer_broad_quality_ready",
            "renderer_commercial_readiness",
            "clean-room",
            "Unity",
            "Unreal Engine",
            "Godot"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer commercial quality closeout selection"
    }
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" }
    )) {
    foreach ($needle in @(
            "renderer_backend_parity_ready=1",
            "renderer_metal_broad_readiness=1",
            "renderer_broad_quality_ready=1",
            "renderer_commercial_readiness=1",
            "Unity-compatible",
            "Unreal-compatible",
            "Godot-compatible",
            "powered by Unreal Engine"
        )) {
        Assert-DoesNotContainText $surface.Text $needle "$($surface.Label) forbidden renderer commercial quality closeout claim"
    }
}
