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
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"

foreach ($needle in @(
        "validation_recipe=renderer-commercial-quality-closeout",
        "renderer_commercial_quality_closeout_status=host_evidence_required",
        "renderer_commercial_quality_closeout_ready=0",
        "renderer_commercial_quality_closeout_value_api_ready=1",
        'renderer_clean_room_source_review_ready=$(ConvertTo-CounterBit $cleanRoomSourceReviewReady)',
        'renderer_external_engine_code_used=$(ConvertTo-CounterBit $externalEngineCodeUsed)',
        'renderer_external_engine_sample_used=$(ConvertTo-CounterBit $externalEngineSampleUsed)',
        'renderer_external_engine_asset_used=$(ConvertTo-CounterBit $externalEngineAssetUsed)',
        'renderer_external_engine_trademark_used=$(ConvertTo-CounterBit $externalEngineTrademarkUsed)',
        'renderer_external_engine_compatibility_claims=$(ConvertTo-CounterBit $externalEngineCompatibilityClaims)',
        'renderer_third_party_notices_complete=$(ConvertTo-CounterBit $thirdPartyNoticesComplete)',
        "renderer_native_handle_access=0",
        "renderer_cross_backend_inference=0",
        "renderer_external_engine_parity=0",
        "renderer_backend_parity_ready=0",
        "renderer_metal_broad_readiness=0",
        "renderer_broad_quality_ready=0",
        "renderer_commercial_readiness=0",
        "renderer_environment_ready=0",
        "validator_integration_and_host_evidence_required"
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
Assert-ContainsText $recipesFragmentText "renderer-commercial-quality-closeout" "manifest validation recipe renderer commercial quality closeout"
Assert-ContainsText $recipesFragmentText "validate-renderer-commercial-quality-closeout.ps1" "manifest validation recipe renderer commercial quality closeout"

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
