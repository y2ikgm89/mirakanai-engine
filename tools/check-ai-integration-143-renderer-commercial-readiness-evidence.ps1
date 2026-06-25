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
$validateWorkflowText = Get-AgentSurfaceText ".github/workflows/validate.yml"
$readinessValidatorText = Get-AgentSurfaceText "tools/validate-renderer-commercial-readiness-evidence.ps1"
$collectorText = Get-AgentSurfaceText "tools/collect-renderer-commercial-readiness-evidence.ps1"
$d3d12ArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-d3d12-commercial-quality-artifact.ps1"
$d3d12ArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-d3d12-commercial-quality-artifact.ps1"
$vulkanStrictArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1"
$vulkanStrictArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-vulkan-strict-commercial-quality-artifact.ps1"
$appleMetalArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1"
$appleMetalArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-apple-metal-commercial-quality-artifact.ps1"
$packageArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-package-commercial-quality-artifacts.ps1"
$packageArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-package-commercial-quality-artifacts.ps1"
$qualityVfxArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-quality-vfx-commercial-artifacts.ps1"
$qualityVfxArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-quality-vfx-commercial-artifacts.ps1"
$cleanRoomLegalReviewInputGeneratorText = Get-AgentSurfaceText "tools/generate-renderer-clean-room-legal-review-input.ps1"
$cleanRoomLegalArtifactProducerText = Get-AgentSurfaceText "tools/collect-renderer-clean-room-legal-artifact.ps1"
$cleanRoomLegalArtifactProducerCheckText = Get-AgentSurfaceText "tools/check-renderer-clean-room-legal-artifact.ps1"
$rendererMetalAppleValidatorText = Get-AgentSurfaceText "tools/validate-renderer-metal-apple.ps1"
$collectorCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-evidence-collector.ps1"
$fixtureGuardCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1"
$metalMemoryCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1"
$finalPreflightText = Get-AgentSurfaceText "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1"
$finalPreflightCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1"
$finalAssemblerText = Get-AgentSurfaceText "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1"
$finalAssemblerCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1"
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
            "tools/collect-renderer-commercial-readiness-evidence.ps1",
            "tools/check-renderer-commercial-readiness-evidence-collector.ps1",
            "tools/check-renderer-commercial-readiness-evidence-fixture-guard.ps1",
            "tools/check-renderer-commercial-readiness-evidence-metal-memory.ps1",
            "tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1",
            "tools/check-renderer-commercial-readiness-final-promotion-preflight.ps1",
            "tools/assemble-renderer-commercial-readiness-final-retained-root.ps1",
            "tools/check-renderer-commercial-readiness-final-retained-root-assembler.ps1",
            "tools/check-ci-matrix.ps1",
            "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
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
        'renderer_clean_room_legal_ready',
        'renderer_third_party_notices_complete',
        'require_ready_without_artifact_root',
        'plan_renderer_commercial_readiness_promotion'
    )) {
    Assert-ContainsText $readinessValidatorText $needle "renderer commercial readiness evidence validator"
}

Assert-ContainsText $validateText 'check-renderer-commercial-readiness-evidence-fixture-guard.ps1' `
    "renderer commercial readiness fixture guard validate task"
Assert-ContainsText $validateText 'check-renderer-commercial-readiness-evidence-collector.ps1' `
    "renderer commercial readiness evidence collector validate task"
Assert-ContainsText $validateText 'check-renderer-commercial-readiness-evidence-metal-memory.ps1' `
    "renderer commercial readiness Metal memory validate task"
Assert-ContainsText $validateText 'check-renderer-commercial-readiness-final-promotion-preflight.ps1' `
    "renderer commercial readiness final promotion preflight validate task"
Assert-ContainsText $validateText 'check-renderer-commercial-readiness-final-retained-root-assembler.ps1' `
    "renderer commercial readiness final retained-root assembler validate task"
Assert-ContainsText $validateText 'check-renderer-d3d12-commercial-quality-artifact.ps1' `
    "renderer commercial readiness D3D12 artifact producer validate task"
Assert-ContainsText $validateText 'check-renderer-vulkan-strict-commercial-quality-artifact.ps1' `
    "renderer commercial readiness strict Vulkan artifact producer validate task"
Assert-ContainsText $validateText 'check-renderer-apple-metal-commercial-quality-artifact.ps1' `
    "renderer commercial readiness Apple Metal artifact producer validate task"
Assert-ContainsText $validateText 'check-renderer-package-commercial-quality-artifacts.ps1' `
    "renderer commercial readiness package artifact producer validate task"
Assert-ContainsText $validateText 'check-renderer-quality-vfx-commercial-artifacts.ps1' `
    "renderer commercial readiness quality/VFX artifact producer validate task"
Assert-ContainsText $validateText 'check-renderer-clean-room-legal-artifact.ps1' `
    "renderer commercial readiness clean-room legal artifact producer validate task"
Assert-ContainsText $collectorCheckText 'renderer-commercial-readiness-evidence-collector-check: ok' `
    "renderer commercial readiness evidence collector check"
Assert-ContainsText $fixtureGuardCheckText 'renderer-commercial-readiness-evidence-fixture-guard-check: ok' `
    "renderer commercial readiness fixture guard check"
Assert-ContainsText $metalMemoryCheckText 'renderer-commercial-readiness-evidence-metal-memory-check: ok' `
    "renderer commercial readiness Metal memory check"
Assert-ContainsText $d3d12ArtifactProducerCheckText 'renderer-d3d12-commercial-quality-artifact-check: ok' `
    "renderer commercial readiness D3D12 artifact producer check"
Assert-ContainsText $vulkanStrictArtifactProducerCheckText 'renderer-vulkan-strict-commercial-quality-artifact-check: ok' `
    "renderer commercial readiness strict Vulkan artifact producer check"
Assert-ContainsText $appleMetalArtifactProducerCheckText 'renderer-apple-metal-commercial-quality-artifact-check: ok' `
    "renderer commercial readiness Apple Metal artifact producer check"
Assert-ContainsText $packageArtifactProducerCheckText 'renderer-package-commercial-quality-artifacts-check: ok' `
    "renderer commercial readiness package artifact producer check"
Assert-ContainsText $qualityVfxArtifactProducerCheckText 'renderer-quality-vfx-commercial-artifacts-check: ok' `
    "renderer commercial readiness quality/VFX artifact producer check"
Assert-ContainsText $cleanRoomLegalArtifactProducerCheckText 'renderer-clean-room-legal-artifact-check: ok' `
    "renderer commercial readiness clean-room legal artifact producer check"

foreach ($needle in @(
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'D3d12ArtifactRelative',
        'VulkanStrictArtifactRelative',
        'AppleMetalArtifactRelative',
        'Visible3dPackageArtifactRelative',
        'RuntimeUiPackageArtifactRelative',
        'EnvironmentPackageArtifactRelative',
        'GeneratedGamePackageArtifactRelative',
        'RendererQualityMatrixArtifactRelative',
        'ProductionVfxProfilingArtifactRelative',
        'MetalMemoryHostEvidenceRelative',
        'CleanRoomLegalArtifactRelative',
        'GameEngine.RendererCleanRoomLegalArtifact.v1',
        'renderer_clean_room_legal_ready',
        'OfficialDocsOnlyReviewReady',
        'LegalReviewReady',
        'ExternalEngineZeroMaterialReviewReady',
        'ThirdPartyNoticesComplete',
        'AllowFixtureArtifactsForSelfTest',
        'fixture_artifact_input_rejected',
        'metal_memory_host_evidence_required',
        'GameEngine.RendererCommercialReadinessEvidence.v1',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'renderer_commercial_readiness_evidence_collector_real_promotion_candidate',
        'renderer_commercial_readiness=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'native_handles_exposed = $false',
        'cross_backend_inference = $false',
        'external_engine_parity = $false'
    )) {
    Assert-ContainsText $collectorText $needle "renderer commercial readiness evidence collector"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Generate',
        'artifacts/renderer/clean-room-legal-review/renderer-commercial-readiness',
        'GameEngine.RendererCleanRoomLegalReviewInput.v1',
        'GameEngine.RendererCleanRoomLegalReviewSourceSummary.v1',
        'renderer-clean-room-legal-review-input',
        'renderer-clean-room-legal-artifact-v1',
        'renderer-clean-room-legal-review-artifacts',
        'https://unity.com/legal/terms-of-service',
        'https://unity.com/legal/branding-trademarks',
        'https://www.unrealengine.com/eula/unreal',
        'https://dev.epicgames.com/docs/dev-portal/unreal-engine/ue-trademark-license',
        'https://godotengine.org/license/',
        'https://docs.godotengine.org/en/stable/about/complying_with_licenses.html',
        'THIRD_PARTY_NOTICES.md',
        'legal_advice = $false',
        'external_engine_material_selected = $false',
        'renderer_clean_room_legal_review_input_written=',
        'renderer_clean_room_legal_review_input_ready=',
        'renderer_clean_room_public_docs_only=1',
        'renderer_external_engine_forbidden_material_detected_rows=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0'
    )) {
    Assert-ContainsText $cleanRoomLegalReviewInputGeneratorText $needle `
        "renderer commercial readiness clean-room legal review input generator"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'D3d12HostEvidenceRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/d3d12-commercial-quality-host-evidence/',
        'GameEngine.RendererD3d12CommercialQualityHostEvidence.v1',
        'GameEngine.RendererCommercialQualityCloseout.v1',
        'renderer-d3d12-commercial-quality-artifact-v1',
        'Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25',
        'd3d12-quality.json',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_d3d12_commercial_quality_artifact_collector_mode=Assemble',
        'renderer_d3d12_commercial_quality_artifact_written=',
        'renderer_d3d12_commercial_quality_fixture_artifact=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'vulkan_inferred',
        'metal_inferred',
        'broad_ui_parity',
        'external_engine_parity',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $d3d12ArtifactProducerText $needle `
        "renderer commercial readiness D3D12 artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-d3d12-commercial-quality-artifact.ps1',
        'GameEngine.RendererD3d12CommercialQualityHostEvidence.v1',
        'renderer-d3d12-commercial-quality-artifact-v1',
        'Microsoft-D3D12-ResourceBarrier-Fence-Timestamp-GpuValidation-2026-06-25',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_d3d12_commercial_quality_artifact_written=1',
        'renderer_d3d12_commercial_quality_fixture_artifact=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=10',
        'renderer_d3d12_renderer_quality_ready',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $d3d12ArtifactProducerCheckText $needle `
        "renderer commercial readiness D3D12 artifact producer check"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'VulkanStrictHostEvidenceRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/vulkan-strict-commercial-quality-host-evidence/',
        'GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1',
        'GameEngine.RendererCommercialQualityCloseout.v1',
        'renderer-vulkan-strict-commercial-quality-artifact-v1',
        'Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25',
        'vulkan-strict-quality.json',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_vulkan_strict_commercial_quality_artifact_collector_mode=Assemble',
        'renderer_vulkan_strict_commercial_quality_artifact_written=',
        'renderer_vulkan_strict_commercial_quality_fixture_artifact=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'd3d12_inferred',
        'metal_inferred',
        'debugging_only_full_pipeline_barrier',
        'external_engine_parity',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $vulkanStrictArtifactProducerText $needle `
        "renderer commercial readiness strict Vulkan artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1',
        'GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1',
        'renderer-vulkan-strict-commercial-quality-artifact-v1',
        'Khronos-Vulkan-Synchronization2-Memory-QueueOwnership-2026-06-25',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_vulkan_strict_commercial_quality_artifact_written=1',
        'renderer_vulkan_strict_commercial_quality_fixture_artifact=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=10',
        'renderer_vulkan_strict_renderer_quality_ready',
        'debugging_only_full_pipeline_barrier',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $vulkanStrictArtifactProducerCheckText $needle `
        "renderer commercial readiness strict Vulkan artifact producer check"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'AppleMetalHostEvidenceRelative',
        'MetalMemoryProfilingHostEvidenceRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/apple-metal-commercial-quality-host-evidence/',
        'artifacts/renderer/metal-memory-profiling-host-evidence/',
        'GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'GameEngine.RendererCommercialQualityCloseout.v1',
        'renderer-apple-metal-commercial-quality-artifact-v1',
        'Apple-Metal-Commercial-Host-Bridge-2026-06-25',
        'renderer-metal-environment-aggregate-apple-host-evidence',
        'Apple-Building-Shader-Library-Precompiling-Source-Files-2026-06-25',
        'Apple-Metal-Shading-Language-Specification-2026-06-25',
        'MTLHeap',
        'MTLResidencySet',
        'MTLCaptureManager',
        'apple-metal-host.json',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_apple_metal_commercial_quality_artifact_collector_mode=Assemble',
        'renderer_apple_metal_commercial_quality_artifact_written=',
        'renderer_apple_metal_commercial_quality_fixture_artifact=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'd3d12_inferred',
        'vulkan_inferred',
        'environment_ready',
        'external_engine_parity',
        'native_handles_exposed',
        'metal_objects_public'
    )) {
    Assert-ContainsText $appleMetalArtifactProducerText $needle `
        "renderer commercial readiness Apple Metal artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1',
        'GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'renderer-apple-metal-commercial-quality-artifact-v1',
        'Apple-Metal-Commercial-Host-Bridge-2026-06-25',
        'renderer-metal-environment-aggregate-apple-host-evidence',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_apple_metal_commercial_quality_artifact_written=1',
        'renderer_apple_metal_commercial_quality_fixture_artifact=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=10',
        'renderer_apple_metal_renderer_quality_ready',
        'MTLHeap',
        'MTLResidencySet',
        'MTLCaptureManager',
        'metal_objects_public',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $appleMetalArtifactProducerCheckText $needle `
        "renderer commercial readiness Apple Metal artifact producer check"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'PackageHostEvidenceRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/package-commercial-quality-host-evidence/',
        'GameEngine.RendererPackageCommercialQualityHostEvidence.v1',
        'renderer-package-commercial-quality-artifacts-v1',
        'GameEngine-Renderer-Package-Commercial-Quality-2026-06-25',
        'visible-3d-package.json',
        'runtime-ui-package.json',
        'environment-package.json',
        'generated-game-package.json',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_package_commercial_quality_artifacts_collector_mode=Assemble',
        'renderer_package_commercial_quality_artifacts_written=',
        'renderer_package_commercial_quality_fixture_artifact=0',
        'renderer_visible_3d_package_ready=1',
        'renderer_runtime_ui_package_ready=1',
        'renderer_environment_package_ready=1',
        'renderer_generated_game_package_ready=1',
        'renderer_package_arbitrary_script_execution=0',
        'renderer_package_script_execution=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'external_engine_parity',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $packageArtifactProducerText $needle `
        "renderer commercial readiness package artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-package-commercial-quality-artifacts.ps1',
        'GameEngine.RendererPackageCommercialQualityHostEvidence.v1',
        'renderer-package-commercial-quality-artifacts-v1',
        'GameEngine-Renderer-Package-Commercial-Quality-2026-06-25',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_package_commercial_quality_artifacts_written=4',
        'renderer_package_commercial_quality_fixture_artifact=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=7',
        'renderer_visible_3d_package_ready',
        'renderer_runtime_ui_package_ready',
        'renderer_environment_package_ready',
        'renderer_generated_game_package_ready',
        'external_engine_parity',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $packageArtifactProducerCheckText $needle `
        "renderer commercial readiness package artifact producer check"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'QualityVfxHostEvidenceRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/quality-vfx-commercial-host-evidence/',
        'GameEngine.RendererQualityVfxCommercialHostEvidence.v1',
        'GameEngine.RendererCommercialQualityCloseout.v1',
        'renderer-quality-vfx-commercial-artifacts-v1',
        'GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25',
        'renderer-quality-matrix.json',
        'production-vfx-profiling.json',
        'renderer_quality_matrix_status',
        'host_evidence_required',
        'rendering_vfx_profiling_reviewed',
        'deterministic_replay_hash_sha256',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_quality_vfx_commercial_artifacts_collector_mode=Assemble',
        'renderer_quality_vfx_commercial_artifacts_written=',
        'renderer_quality_vfx_commercial_fixture_artifact=0',
        'renderer_quality_matrix_ready=1',
        'renderer_production_vfx_profiling_ready=1',
        'renderer_quality_matrix_gpu_command_side_effects=0',
        'renderer_quality_matrix_native_capture_side_effects=0',
        'renderer_quality_matrix_crash_upload_side_effects=0',
        'renderer_production_vfx_native_capture_side_effects=0',
        'renderer_production_vfx_crash_upload_side_effects=0',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'external_engine_parity',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $qualityVfxArtifactProducerText $needle `
        "renderer commercial readiness quality/VFX artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-quality-vfx-commercial-artifacts.ps1',
        'GameEngine.RendererQualityVfxCommercialHostEvidence.v1',
        'renderer-quality-vfx-commercial-artifacts-v1',
        'GameEngine-Renderer-Quality-Vfx-Profiling-2026-06-25',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_quality_vfx_commercial_artifacts_written=2',
        'renderer_quality_vfx_commercial_fixture_artifact=0',
        'renderer_quality_matrix_status=host_evidence_required',
        'rendering_vfx_profiling_reviewed=1',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=9',
        'renderer_quality_matrix_ready',
        'renderer_production_vfx_profiling_ready',
        'renderer_broad_quality_ready',
        'native_handles_exposed'
    )) {
    Assert-ContainsText $qualityVfxArtifactProducerCheckText $needle `
        "renderer commercial readiness quality/VFX artifact producer check"
}

foreach ($needle in @(
        '#requires -Version 7.0',
        '[CmdletBinding(PositionalBinding = $false)]',
        'Mode',
        'Assemble',
        'OutputRootRelative',
        'CleanRoomLegalReviewRelative',
        'artifacts/renderer/commercial-readiness-evidence/',
        'artifacts/renderer/clean-room-legal-review/',
        'GameEngine.RendererCleanRoomLegalReviewInput.v1',
        'GameEngine.RendererCleanRoomLegalArtifact.v1',
        'renderer-clean-room-legal-artifact-v1',
        'renderer-clean-room-legal-artifact',
        'Unity-Legal-Terms-2026-06-25',
        'Unity-Trademark-Guidelines-2026-06-25',
        'Epic-Unreal-Engine-EULA-Trademark-2026-06-25',
        'Epic-Unreal-Engine-Release-Trademark-2026-06-25',
        'Godot-License-2026-06-25',
        'Godot-Trademark-Licensing-2026-06-25',
        'THIRD_PARTY_NOTICES.md',
        'fixture_only = $false',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'external_material_selected',
        'legal_review_id',
        'technical_review_id',
        'external_material_approved',
        'forbidden_material_rows',
        'external_engine_code',
        'external_engine_sample',
        'external_engine_shader',
        'external_engine_asset',
        'external_engine_trademark',
        'external_engine_ui_expression',
        'external_engine_api',
        'compatibility_claim',
        'equivalence_claim',
        'parity_claim',
        'renderer_clean_room_legal_artifact_collector_mode=Assemble',
        'renderer_clean_room_legal_ready=',
        'renderer_clean_room_external_engine_zero_material_ready=',
        'renderer_third_party_notices_complete=',
        'renderer_external_engine_forbidden_material_detected_rows=',
        'renderer_external_engine_forbidden_material_rejected_rows=',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0'
    )) {
    Assert-ContainsText $cleanRoomLegalArtifactProducerText $needle `
        "renderer commercial readiness clean-room legal artifact producer"
}

foreach ($needle in @(
        'tools/collect-renderer-clean-room-legal-artifact.ps1',
        'GameEngine.RendererCleanRoomLegalReviewInput.v1',
        'renderer-clean-room-legal-artifact-v1',
        'Unity-Legal-Terms-2026-06-25',
        'Unity-Trademark-Guidelines-2026-06-25',
        'Epic-Unreal-Engine-EULA-Trademark-2026-06-25',
        'Epic-Unreal-Engine-Release-Trademark-2026-06-25',
        'Godot-License-2026-06-25',
        'Godot-Trademark-Licensing-2026-06-25',
        'fixture_artifact_input_rejected',
        'unsafe_relative_path',
        'renderer_clean_room_legal_artifact_written=1',
        'renderer_external_engine_forbidden_material_detected_rows=10',
        'renderer_external_engine_forbidden_material_rejected_rows=10',
        'renderer_external_engine_shader_used=1',
        'renderer_external_engine_api_used=1',
        'renderer_external_engine_compatibility_claim=1',
        'renderer_external_engine_equivalence_claim=1',
        'renderer_external_engine_parity_claim=1',
        'renderer_clean_room_legal_ready=1',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0',
        'CleanRoomLegalArtifactRelative',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=11'
    )) {
    Assert-ContainsText $cleanRoomLegalArtifactProducerCheckText $needle `
        "renderer commercial readiness clean-room legal artifact producer check"
}

foreach ($needle in @(
        'renderer_apple_metal_commercial_quality_host_source_status=ready',
        'GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1',
        'renderer-metal-apple-host-evidence',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'renderer-metal-memory-profiling-host-evidence',
        'renderer_apple_metal_commercial_quality_artifact_ready=0',
        'renderer_apple_metal_commercial_quality_native_handles_exposed=0',
        'renderer_apple_metal_commercial_quality_cross_backend_inference=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0'
    )) {
    Assert-ContainsText $rendererMetalAppleValidatorText $needle `
        "renderer Metal Apple host commercial quality source row"
}

foreach ($needle in @(
        'fixture_artifact_input_rejected',
        'renderer_commercial_readiness_evidence_collector_artifact_rows=11',
        'renderer_commercial_readiness_evidence_collector_fixture_artifacts=11',
        'renderer_commercial_readiness_evidence_collector_real_promotion_candidate=0',
        'renderer_commercial_readiness_fixture_artifacts_rejected',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $collectorCheckText $needle "renderer commercial readiness evidence collector check"
}

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
        'artifacts/renderer/commercial-readiness-evidence/metal-memory-full-self-test',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'renderer-metal-memory-profiling-host-evidence-v1',
        'Apple-Metal-MTLHeap-2026-06-24',
        'Apple-Metal-MTLResidencySet-2026-06-24',
        'Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24',
        'Apple-Metal-MTLCaptureManager-2026-06-24',
        'Apple-Metal-ProgrammaticCapture-2026-06-24',
        'capture-summary.txt',
        'renderer_metal_memory_profiling_ready=1',
        'renderer_metal_broad_readiness=0',
        'renderer_commercial_readiness=0',
        'invalid_artifact_fixture_flag',
        'metal_memory_full_host_evidence_required'
    )) {
    Assert-ContainsText $metalMemoryCheckText $needle "renderer commercial readiness Metal memory check"
}

foreach ($needle in @(
        'Test-RendererCommercialReadinessMetalMemoryArtifact',
        'Test-RendererCommercialReadinessMetalMemoryHostEvidence',
        'metal_memory_full_host_evidence_required',
        'renderer-metal-memory-profiling-host-evidence-v1',
        'Apple-Metal-MTLHeap-2026-06-24',
        'Apple-Metal-MTLResidencySet-2026-06-24',
        'Apple-Metal-MTLResidencySet-requestResidency-2026-06-24',
        'Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24',
        'Apple-Metal-MTLCaptureManager-2026-06-24',
        'Apple-Metal-ProgrammaticCapture-2026-06-24',
        'capture_artifact_hash_sha256',
        'deterministic_capture_hash_sha256',
        'broad_backend_parity_ready',
        'commercial_renderer_readiness',
        'external_engine_api_parity'
    )) {
    Assert-ContainsText $readinessValidatorText $needle `
        "renderer commercial readiness Metal memory host evidence binding"
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
        'IDXGIAdapter3::QueryVideoMemoryInfo',
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
    Assert-ContainsText $d3d12ArtifactProducerText $needle `
        "renderer commercial readiness D3D12 artifact producer binding"
    Assert-ContainsText $d3d12ArtifactProducerCheckText $needle `
        "renderer commercial readiness D3D12 artifact producer check binding"
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
    Assert-ContainsText $vulkanStrictArtifactProducerText $needle `
        "renderer commercial readiness strict Vulkan artifact producer binding"
    Assert-ContainsText $vulkanStrictArtifactProducerCheckText $needle `
        "renderer commercial readiness strict Vulkan artifact producer check binding"
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
        'rendererCommercialReadinessFinalPromotionPreflight',
        'rendererCommercialReadinessFinalRetainedRootAssembler',
        'rendererCommercialReadinessFinalRetainedRootAssemblerRequireReady',
        'tools/collect-renderer-commercial-readiness-evidence.ps1',
        'tools/validate-renderer-commercial-readiness-evidence.ps1',
        'tools/validate-renderer-commercial-readiness-final-promotion-preflight.ps1',
        'tools/assemble-renderer-commercial-readiness-final-retained-root.ps1',
        'tools/generate-renderer-clean-room-legal-review-input.ps1',
        'renderer-commercial-readiness-evidence'
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness evidence command surface"
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness evidence validation recipe"
}

Assert-ContainsText $commandsFragmentText 'rendererCommercialReadinessEvidenceCollector' `
    "renderer commercial readiness evidence collector command surface"
Assert-ContainsText $validationRecipesFragmentText 'renderer-commercial-readiness-evidence-collector' `
    "renderer commercial readiness evidence collector validation recipe"
Assert-ContainsText $validationRecipesFragmentText 'renderer-commercial-readiness-final-promotion-preflight' `
    "renderer commercial readiness final promotion preflight validation recipe"
Assert-ContainsText $validationRecipesFragmentText 'renderer-commercial-readiness-final-retained-root-assembler' `
    "renderer commercial readiness final retained-root assembler validation recipe"
Assert-ContainsText $commandsFragmentText 'rendererCleanRoomLegalReviewInputGenerator' `
    "renderer commercial readiness clean-room legal review input generator command"
Assert-ContainsText $validationRecipesFragmentText 'renderer-clean-room-legal-review-input' `
    "renderer commercial readiness clean-room legal review input validation recipe"
foreach ($needle in @(
        'Generate renderer clean-room legal review input',
        'Upload renderer clean-room legal review artifacts',
        'renderer-clean-room-legal-review-artifacts',
        'artifacts/renderer/clean-room-legal-review/renderer-commercial-readiness/**',
        'if-no-files-found: error',
        'Upload renderer commercial readiness retained root (Metal host)',
        'renderer-commercial-readiness-final-retained-root',
        'artifacts/renderer/commercial-readiness-evidence/final-retained/**',
        'if-no-files-found: warn',
        'include-hidden-files: false',
        'compression-level: 0'
    )) {
    Assert-ContainsText $validateWorkflowText $needle "renderer commercial readiness final retained-root CI retention"
    Assert-ContainsText $ciMatrixCheckText $needle "renderer commercial readiness final retained-root CI retention check"
}
foreach ($needle in @(
        'renderer_commercial_readiness_final_preflight_required_files=',
        'renderer_commercial_readiness_final_preflight_missing_files',
        'metal_memory_profiling_host_evidence_required',
        'clean_room_legal_artifact_required',
        'fixture_artifact_rejected_',
        'require_ready_without_complete_retained_artifacts',
        'renderer_commercial_readiness='
    )) {
    Assert-ContainsText $finalPreflightText $needle "renderer commercial readiness final preflight validator"
}
foreach ($needle in @(
        'renderer_commercial_readiness_final_preflight_required_files=12',
        'renderer_commercial_readiness_final_preflight_missing_files',
        'metal_memory_profiling_host_evidence_required',
        'clean_room_legal_artifact_required',
        'fixture_artifact_rejected_d3d12_quality',
        'require_ready_without_complete_retained_artifacts',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $finalPreflightCheckText $needle "renderer commercial readiness final preflight check"
}
foreach ($needle in @(
        'renderer-commercial-readiness-final-retained-root-assembler',
        'D3d12HostEvidenceRelative',
        'VulkanStrictHostEvidenceRelative',
        'AppleMetalHostEvidenceRelative',
        'MetalMemoryProfilingHostEvidenceRelative',
        'PackageHostEvidenceRelative',
        'QualityVfxHostEvidenceRelative',
        'CleanRoomLegalReviewRelative',
        '_producer-artifacts',
        'collect-renderer-commercial-readiness-evidence.ps1',
        'validate-renderer-commercial-readiness-final-promotion-preflight.ps1',
        'renderer_commercial_readiness_final_assembler_ready',
        'renderer_backend_parity_ready=0',
        'renderer_metal_broad_readiness=0',
        'renderer_broad_quality_ready=0',
        'renderer_commercial_readiness=0',
        'renderer_environment_ready=0'
    )) {
    Assert-ContainsText $finalAssemblerText $needle "renderer commercial readiness final retained-root assembler"
}
foreach ($needle in @(
        'renderer-commercial-readiness-final-retained-root-assembler-check: ok',
        'renderer_commercial_readiness_final_assembler_status=ready',
        'renderer_commercial_readiness_final_assembler_ready=1',
        'renderer_commercial_readiness_final_preflight_ready=1',
        'renderer_commercial_readiness_final_preflight_missing_files=0',
        'renderer_environment_ready=0'
    )) {
    Assert-ContainsText $finalAssemblerCheckText $needle "renderer commercial readiness final retained-root assembler check"
}
Assert-ContainsText $commandsFragmentText 'rendererD3d12CommercialQualityArtifactCheck' `
    "renderer commercial readiness D3D12 artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererD3d12CommercialQualityArtifactCollector' `
    "renderer commercial readiness D3D12 artifact producer collector command surface"
Assert-ContainsText $commandsFragmentText 'rendererVulkanStrictCommercialQualityArtifactCheck' `
    "renderer commercial readiness strict Vulkan artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererVulkanStrictCommercialQualityArtifactCollector' `
    "renderer commercial readiness strict Vulkan artifact producer collector command surface"
Assert-ContainsText $commandsFragmentText 'rendererAppleMetalCommercialQualityArtifactCheck' `
    "renderer commercial readiness Apple Metal artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererAppleMetalCommercialQualityArtifactCollector' `
    "renderer commercial readiness Apple Metal artifact producer collector command surface"
Assert-ContainsText $commandsFragmentText 'rendererPackageCommercialQualityArtifactsCheck' `
    "renderer commercial readiness package artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererPackageCommercialQualityArtifactsCollector' `
    "renderer commercial readiness package artifact producer collector command surface"
Assert-ContainsText $commandsFragmentText 'rendererQualityVfxCommercialArtifactsCheck' `
    "renderer commercial readiness quality/VFX artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererQualityVfxCommercialArtifactsCollector' `
    "renderer commercial readiness quality/VFX artifact producer collector command surface"
Assert-ContainsText $commandsFragmentText 'rendererCleanRoomLegalArtifactCheck' `
    "renderer commercial readiness clean-room legal artifact producer check command surface"
Assert-ContainsText $commandsFragmentText 'rendererCleanRoomLegalArtifactCollector' `
    "renderer commercial readiness clean-room legal artifact producer collector command surface"

foreach ($needle in @(
        'renderer-d3d12-commercial-quality-artifact',
        'tools/collect-renderer-d3d12-commercial-quality-artifact.ps1',
        'D3d12HostEvidenceRelative',
        'ID3D12Fence',
        'D3D12_RESOURCE_BARRIER',
        'D3D12_QUERY_TYPE_TIMESTAMP',
        'ID3D12Device3::EnqueueMakeResident',
        'IDXGIAdapter3::QueryVideoMemoryInfo',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness D3D12 artifact producer validation recipe"
}

foreach ($needle in @(
        'renderer-vulkan-strict-commercial-quality-artifact',
        'tools/collect-renderer-vulkan-strict-commercial-quality-artifact.ps1',
        'VulkanStrictHostEvidenceRelative',
        'vkCmdPipelineBarrier2',
        'VkDependencyInfo',
        'VK_LAYER_KHRONOS_validation',
        'synchronization validation clean logs',
        'memory binding VUID',
        'spirv-val',
        'debugging_only_full_pipeline_barrier=false',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness strict Vulkan artifact producer validation recipe"
}

foreach ($needle in @(
        'renderer-apple-metal-commercial-quality-artifact',
        'tools/collect-renderer-apple-metal-commercial-quality-artifact.ps1',
        'AppleMetalHostEvidenceRelative',
        'MetalMemoryProfilingHostEvidenceRelative',
        'GameEngine.RendererMetalMemoryProfilingHostEvidence.v1',
        'Metal Shading Language',
        'MTLHeap',
        'MTLResidencySet',
        'MTLCaptureManager',
        'cross_backend_inference=false',
        'native_handles_exposed=false',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness Apple Metal artifact producer validation recipe"
}

foreach ($needle in @(
        'renderer-package-commercial-quality-artifacts',
        'tools/collect-renderer-package-commercial-quality-artifacts.ps1',
        'PackageHostEvidenceRelative',
        'visible-3d-package.json',
        'runtime-ui-package.json',
        'environment-package.json',
        'generated-game-package.json',
        'package script or arbitrary script execution',
        'environment_ready promotion',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness package artifact producer validation recipe"
}

foreach ($needle in @(
        'renderer-quality-vfx-commercial-artifacts',
        'tools/collect-renderer-quality-vfx-commercial-artifacts.ps1',
        'QualityVfxHostEvidenceRelative',
        'renderer-quality-matrix.json',
        'production-vfx-profiling.json',
        'renderer_quality_matrix_status=host_evidence_required',
        'rendering_vfx_profiling_reviewed=true',
        'zero GPU command/native capture/crash-upload side effects',
        'deterministic replay hashes',
        'renderer_commercial_readiness=0'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness quality/VFX artifact producer validation recipe"
}

foreach ($needle in @(
        'renderer-clean-room-legal-artifact',
        'tools/collect-renderer-clean-room-legal-artifact.ps1',
        'CleanRoomLegalReviewRelative',
        'clean-room-legal.json',
        'GameEngine.RendererCleanRoomLegalArtifact.v1',
        'Unity terms/trademark guidance',
        'Unreal Engine EULA/release trademark guidance',
        'Godot license/trademark policy',
        'THIRD_PARTY_NOTICES.md',
        'no copied external engine source',
        'sample code',
        'shader code',
        'UI expression',
        'asset',
        'trademark',
        'project import',
        'API',
        'compatibility',
        'equivalence',
        'parity claim',
        'legal plus technical review ids',
        'renderer_clean_room_legal_ready=1',
        'renderer_clean_room_legal_ready=0',
        'renderer_commercial_readiness'
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle `
        "renderer commercial readiness clean-room legal artifact producer validation recipe"
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
