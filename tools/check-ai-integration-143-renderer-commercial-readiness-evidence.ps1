#requires -Version 7.0
#requires -PSEdition Core
# Chapter 143 for check-ai-integration.ps1 renderer commercial readiness evidence promotion contract.

$schemaText = Get-AgentSurfaceText "schemas/renderer-commercial-readiness-evidence.schema.json"
$readyFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json"
$d3d12ReadyArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/d3d12-quality.json"
$vulkanReadyArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/vulkan-strict-quality.json"
$appleMetalReadyArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/apple-metal-host.json"
$visible3dPackageArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/visible-3d-package.json"
$runtimeUiPackageArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/runtime-ui-package.json"
$environmentPackageArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/environment-package.json"
$generatedGamePackageArtifactText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/generated-game-package.json"
$missingMetalFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json"
$externalEngineRejectedFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-readiness-evidence-promotion-v1.md"
$promotionHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer_commercial_readiness_evidence.hpp"
$promotionSourceText = Get-AgentSurfaceText "engine/renderer/src/renderer_commercial_readiness_evidence.cpp"
$promotionTestsText = Get-AgentSurfaceText "tests/unit/renderer_commercial_readiness_evidence_tests.cpp"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rendererCMakeText = Get-AgentSurfaceText "engine/renderer/CMakeLists.txt"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$readinessValidatorText = Get-AgentSurfaceText "tools/validate-renderer-commercial-readiness-evidence.ps1"
$fixtureGuardCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1"
$validateText = Get-AgentSurfaceText "tools/validate.ps1"
$qualityCloseoutValidatorText = Get-AgentSurfaceText "tools/validate-renderer-commercial-quality-closeout.ps1"
$recipePlansText = Get-AgentSurfaceText "tools/run-validation-recipe-plans.ps1"
$recipeRunnerCheckText = Get-AgentSurfaceText "tools/check-validation-recipe-runner.ps1"
$classifierText = Get-AgentSurfaceText "tools/classify-pr-validation-tier.ps1"
$ciMatrixCheckText = Get-AgentSurfaceText "tools/check-ci-matrix.ps1"

foreach ($surface in @(
        @{ Text = $schemaText; Label = "renderer commercial readiness evidence schema" },
        @{ Text = $readyFixtureText; Label = "renderer commercial readiness ready fixture" },
        @{ Text = $missingMetalFixtureText; Label = "renderer commercial readiness missing Metal fixture" },
        @{ Text = $externalEngineRejectedFixtureText; Label = "renderer commercial readiness external engine rejected fixture" }
    )) {
    foreach ($needle in @(
            "GameEngine.RendererCommercialReadinessEvidence.v1",
            "renderer-commercial-readiness-evidence-promotion-v1",
            "d3d12",
            "vulkan_strict",
            "apple_metal",
            "visible_3d",
            "runtime_ui",
            "environment",
            "generated_game",
            "renderer_quality_matrix",
            "production_vfx_profiling",
            "memory_residency",
            "profiling_capture",
            "official_docs_only",
            "external_engine_zero_material_review",
            "third_party_notices",
            "native_handles_exposed",
            "cross_backend_inference",
            "external_engine_parity",
            "external_engine_shader_used",
            "external_engine_project_import_used",
            "external_engine_api_used",
            "external_engine_compatibility_claim",
            "external_engine_equivalence_claim",
            "external_engine_parity_claim",
            "unity_compatibility",
            "unreal_compatibility",
            "godot_compatibility"
        )) {
        Assert-ContainsText $surface.Text $needle $surface.Label
    }
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $manifestText; Label = "composed manifest" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer commercial readiness evidence plan" }
    )) {
    foreach ($needle in @(
            "Renderer Commercial Readiness Evidence Promotion v1",
            "renderer-commercial-readiness-evidence-promotion-v1",
            "GameEngine.RendererCommercialReadinessEvidence.v1",
            "D3D12",
            "strict Vulkan",
            "Apple-host Metal",
            "renderer quality matrix",
            "production VFX/profiling",
            "Metal memory/profiling",
            "selected 3D/UI/environment/generated-game package evidence",
            "third-party notice",
            "Unity",
            "Unreal Engine",
            "Godot",
            "external engine source",
            "sample code",
            "shader code",
            "UI expression",
            "assets",
            "trademarks",
            "compatibility claims",
            "equivalence claims",
            "parity claims"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer commercial readiness evidence promotion"
    }
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "production loop fragment" },
        @{ Text = $manifestText; Label = "composed manifest" },
        @{ Text = $planText; Label = "renderer commercial readiness evidence plan" }
    )) {
    foreach ($needle in @(
            "rendererCommercialReadinessEvidenceBundleContract",
            "schemas/renderer-commercial-readiness-evidence.schema.json",
            "tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json",
            "tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json",
            "tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json",
            "tools/check-json-contracts-074-renderer-commercial-readiness-evidence.ps1",
            "tools/check-ai-integration-143-renderer-commercial-readiness-evidence.ps1"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) renderer commercial readiness evidence bundle contract"
    }
}

foreach ($forbiddenNeedle in @(
        "renderer_backend_parity_ready=1",
        "renderer_metal_broad_readiness=1",
        "renderer_broad_quality_ready=1",
        "renderer_commercial_readiness=1"
    )) {
    Assert-DoesNotContainText $productionLoopFragmentText $forbiddenNeedle "renderer commercial readiness evidence promotion fragment forbidden broad ready claim"
    Assert-DoesNotContainText $manifestText $forbiddenNeedle "renderer commercial readiness evidence promotion manifest forbidden broad ready claim"
}

foreach ($needle in @(
        "RendererCommercialReadinessSourceRow",
        "RendererCommercialReadinessEvidenceRow",
        "RendererCommercialReadinessPromotionDesc",
        "RendererCommercialReadinessPromotionPlan",
        "RendererCommercialReadinessEvidenceDiagnosticCode",
        "RendererCommercialReadinessEvidenceKind",
        "plan_renderer_commercial_readiness_promotion",
        "renderer_backend_parity_ready",
        "renderer_metal_broad_readiness",
        "renderer_broad_quality_ready",
        "renderer_commercial_readiness",
        "request_native_handle_access",
        "request_cross_backend_inference",
        "external_engine_code_used",
        "external_engine_sample_used",
        "external_engine_asset_used",
        "external_engine_trademark_used",
        "external_engine_ui_expression_used",
        "external_engine_api_used",
        "external_engine_compatibility_claim",
        "external_engine_equivalence_claim",
        "external_engine_parity_claim"
    )) {
    Assert-ContainsText $promotionHeaderText $needle "renderer commercial readiness promotion public header"
}

foreach ($needle in @(
        "kEvidenceRecipeId",
        "renderer-commercial-readiness-evidence",
        "kMetalMemoryProfilingRecipeId",
        "renderer-metal-memory-profiling-host-evidence",
        "Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25",
        "Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25",
        "Apple-Metal-Framework-Memory-Capture-2026-06-25",
        "Unity-Legal-Terms-2026-06-25",
        "Epic-Unreal-Engine-EULA-Trademark-2026-06-25",
        "Godot-Trademark-Licensing-2026-06-25",
        "source_ready",
        "is_lower_hex_sha256",
        "row_has_valid_artifact_schema",
        "external_engine_material_used",
        "external_engine_claim",
        "forbidden_material_rejected",
        "native_handle_access",
        "cross_backend_inference",
        "compute_replay_hash",
        "RendererCommercialReadinessPromotionStatus::ready",
        "RendererCommercialReadinessPromotionStatus::evidence_required",
        "RendererCommercialReadinessPromotionStatus::invalid_request"
    )) {
    Assert-ContainsText $promotionSourceText $needle "renderer commercial readiness promotion source"
}

foreach ($needle in @(
        "complete retained evidence rows",
        "Apple-host Metal memory and profiling rows",
        "missing strict Vulkan backend parity",
        "stale source ids",
        "cross-backend Metal inference and native handles",
        "external engine material and compatibility claims",
        "rejected forbidden-material diagnostics non-ready",
        "replay hash changes with accepted artifact details"
    )) {
    Assert-ContainsText $promotionTestsText $needle "renderer commercial readiness promotion tests"
}

foreach ($needle in @(
        "renderer_commercial_readiness_evidence.cpp",
        "MK_renderer_commercial_readiness_evidence_tests",
        "tests/unit/renderer_commercial_readiness_evidence_tests.cpp"
    )) {
    Assert-ContainsText ($rendererCMakeText + "`n" + $rootCMakeText) $needle "renderer commercial readiness promotion CMake"
}

foreach ($needle in @(
        "engine/renderer/include/mirakana/renderer/renderer_commercial_readiness_evidence.hpp",
        "RendererCommercialReadinessSourceRow",
        "RendererCommercialReadinessEvidenceRow",
        "RendererCommercialReadinessPromotionDesc",
        "RendererCommercialReadinessPromotionPlan",
        "plan_renderer_commercial_readiness_promotion",
        "never executes GPU commands",
        "never returns native handles",
        "cross-backend Metal inference",
        "Unity/Unreal Engine/Godot compatibility"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "manifest MK_renderer commercial readiness promotion evidence"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        '[switch]$RequireReady',
        '[string]$ArtifactRootRelative',
        'GameEngine.RendererCommercialReadinessEvidence.v1',
        'renderer-commercial-readiness-evidence-promotion-v1',
        'renderer-commercial-quality-closeout',
        'check-renderer-metal-memory-profiling-host-evidence.ps1',
        'renderer_commercial_readiness_evidence_status',
        'renderer_commercial_readiness_evidence_ready',
        'renderer_backend_parity_ready',
        'renderer_metal_broad_readiness',
        'renderer_broad_quality_ready',
        'renderer_commercial_readiness',
        'renderer_environment_ready=0',
        'fixture_only=1',
        'fixture_artifact_rejected',
        'renderer_commercial_readiness_fixture_artifacts_rejected',
        'Test-RendererCommercialReadinessArtifactFixtureFlag',
        'artifact_hash_mismatch',
        'missing_artifact',
        'external_engine_material_rejected',
        'external_engine_zero_material_review',
        'external_engine_shader_used',
        'external_engine_project_import_used',
        'external_engine_api_used',
        'external_engine_compatibility_claim',
        'external_engine_equivalence_claim',
        'external_engine_parity_claim',
        'renderer_external_engine_zero_material_review_ready',
        'renderer_external_engine_shader_used',
        'renderer_external_engine_project_import_used',
        'renderer_external_engine_api_used',
        'renderer_external_engine_compatibility_claim',
        'renderer_external_engine_equivalence_claim',
        'renderer_external_engine_parity_claim',
        'require_ready_without_artifact_root',
        'plan_renderer_commercial_readiness_promotion'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness evidence validator"
}

Assert-ContainsText $validateText 'check-renderer-commercial-readiness-evidence-fixture-guard.ps1' `
    "renderer commercial readiness fixture guard validate task"
Assert-ContainsText $fixtureGuardCheckText 'renderer-commercial-readiness-evidence-fixture-guard-check: ok' `
    "renderer commercial readiness fixture guard check"

foreach ($needle in @(
        'artifacts/renderer/commercial-readiness-evidence/fixture-guard-self-test',
        'tests/fixtures/renderer/commercial-readiness-evidence/ready',
        'fixture_artifact_rejected',
        'renderer_commercial_readiness_fixture_artifacts_rejected=11',
        'renderer_commercial_readiness=0',
        'Copied fixture artifacts unexpectedly promoted from a retained artifact root.'
    )) {
    Assert-ContainsText $fixtureGuardCheckText $needle "renderer commercial readiness fixture guard check"
}

foreach ($needle in @(
        'command_allocator_list_fence',
        'command_allocator_reuse_fenced',
        'resource_barriers',
        'D3D12_RESOURCE_BARRIER',
        'D3D12_QUERY_TYPE_TIMESTAMP',
        'resolved_query_data',
        'queue_frequency_hz',
        'clock_calibration',
        'debug_layer_or_gpu_based_validation_clean',
        'ID3D12Device3::EnqueueMakeResident',
        'package_visible_readback',
        'deterministic_hash_sha256',
        'native_handles_exposed',
        'renderer_d3d12_command_allocator_fence_ready',
        'renderer_d3d12_resource_barrier_ready',
        'renderer_d3d12_timestamp_ready',
        'renderer_d3d12_debug_validation_ready',
        'renderer_d3d12_residency_ready',
        'renderer_d3d12_package_readback_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness evidence validator D3D12 binding"
    Assert-ContainsText $d3d12ReadyArtifactText $needle "renderer commercial readiness D3D12 ready artifact"
}

foreach ($needle in @(
        'synchronization2',
        'vkCmdPipelineBarrier2',
        'VkDependencyInfo',
        'VK_LAYER_KHRONOS_validation',
        'synchronization_validation',
        'memory_binding',
        'VUID',
        'query_pool_timestamp',
        'spirv_val_ready',
        'debugging_only_full_pipeline_barrier',
        'renderer_vulkan_synchronization2_ready',
        'renderer_vulkan_validation_layer_ready',
        'renderer_vulkan_sync_validation_ready',
        'renderer_vulkan_memory_binding_ready',
        'renderer_vulkan_timestamp_ready',
        'renderer_vulkan_shader_validation_ready',
        'renderer_vulkan_package_readback_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness evidence validator Vulkan binding"
    Assert-ContainsText $vulkanReadyArtifactText $needle "renderer commercial readiness Vulkan ready artifact"
}

foreach ($needle in @(
        'host_toolchain',
        'metal_tool_ready',
        'metallib_tool_ready',
        'msl_shader',
        'device',
        'constant',
        'threadgroup',
        '[[function_constant]]',
        '[[buffer]]',
        '[[texture]]',
        '[[sampler]]',
        '[[vertex]]',
        '[[fragment]]',
        '[[kernel]]',
        'MTLHeap',
        'MTLResidencySet',
        'MTLCaptureManager',
        'capture_scope',
        'visible_package',
        'cross_backend_inference',
        'renderer_apple_metal_xcode_tools_ready',
        'renderer_apple_metal_msl_shader_ready',
        'renderer_apple_metal_heap_ready',
        'renderer_apple_metal_residency_set_ready',
        'renderer_apple_metal_capture_ready',
        'renderer_apple_metal_visible_package_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness evidence validator Apple Metal binding"
    Assert-ContainsText $appleMetalReadyArtifactText $needle "renderer commercial readiness Apple Metal ready artifact"
}

foreach ($needle in @(
        'material_render',
        'lighting_row',
        'shadow_postprocess',
        'package_visible_readback',
        'game_agent_manifest_row',
        'validation_recipe_id',
        'renderer_visible_3d_material_ready',
        'renderer_visible_3d_lighting_ready',
        'renderer_visible_3d_shadow_postprocess_ready',
        'renderer_visible_3d_readback_hash_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness visible 3D package binding"
    Assert-ContainsText $visible3dPackageArtifactText $needle "renderer commercial readiness visible 3D package artifact"
}

foreach ($needle in @(
        'ui_atlas_upload',
        'ui_atlas_readback',
        'renderer_handoff',
        'renderer_runtime_ui_atlas_upload_ready',
        'renderer_runtime_ui_atlas_readback_ready',
        'renderer_runtime_ui_handoff_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness runtime UI package binding"
    Assert-ContainsText $runtimeUiPackageArtifactText $needle "renderer commercial readiness runtime UI package artifact"
}

foreach ($needle in @(
        'environment_renderer_package_consumption',
        'environment_package_row_consumed',
        'environment_ready_promoted',
        'renderer_environment_package_consumption_ready',
        'renderer_environment_ready_promoted'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness environment package binding"
    Assert-ContainsText $environmentPackageArtifactText $needle "renderer commercial readiness environment package artifact"
}

foreach ($needle in @(
        'generated_game_output',
        'generated_game_package_written',
        'generated_game_manifest_id',
        'renderer_generated_game_package_output_ready',
        'renderer_generated_game_manifest_ready'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness generated game package binding"
    Assert-ContainsText $generatedGamePackageArtifactText $needle "renderer commercial readiness generated game package artifact"
}

foreach ($needle in @(
        'arbitrary_script_execution',
        'package_script_execution',
        'renderer_package_arbitrary_script_execution',
        'renderer_package_script_execution'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness package script rejection"
    Assert-ContainsText $visible3dPackageArtifactText $needle "renderer commercial readiness visible 3D package script rejection"
    Assert-ContainsText $runtimeUiPackageArtifactText $needle "renderer commercial readiness runtime UI package script rejection"
    Assert-ContainsText $environmentPackageArtifactText $needle "renderer commercial readiness environment package script rejection"
    Assert-ContainsText $generatedGamePackageArtifactText $needle "renderer commercial readiness generated game package script rejection"
}

foreach ($needle in @(
        'rendererCommercialReadinessEvidenceCheck',
        'rendererCommercialReadinessEvidenceRequireReady',
        'tools/validate-renderer-commercial-readiness-evidence.ps1',
        'renderer-commercial-readiness-evidence'
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness evidence command surface"
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness evidence validation recipe"
}

foreach ($needle in @(
        'ReadinessEvidenceArtifactRootRelative',
        'validate-renderer-commercial-readiness-evidence.ps1',
        'renderer_commercial_readiness_evidence_ready',
        'renderer_commercial_readiness_evidence_status'
    )) {
    Assert-ContainsText $qualityCloseoutValidatorText $needle "renderer commercial quality closeout readiness evidence wrapper"
}

foreach ($needle in @(
        'renderer-commercial-readiness-evidence',
        'validate-renderer-commercial-readiness-evidence.ps1'
    )) {
    Assert-ContainsText $recipePlansText $needle "renderer commercial readiness evidence recipe plan"
    Assert-ContainsText $recipeRunnerCheckText $needle "renderer commercial readiness evidence recipe runner check"
}

foreach ($needle in @(
        'Test-RendererCommercialReadinessEvidencePath',
        'renderer-commercial-readiness-evidence',
        'validate-renderer-commercial-readiness-evidence.ps1',
        'tests/fixtures/renderer/commercial-readiness-evidence'
    )) {
    Assert-ContainsText $classifierText $needle "renderer commercial readiness evidence CI classifier"
    Assert-ContainsText $ciMatrixCheckText $needle "renderer commercial readiness evidence CI matrix check"
}
