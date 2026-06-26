#requires -Version 7.0
#requires -PSEdition Core
# Chapter 144 for renderer commercial readiness final retained-root artifact import.

$importerText = Get-AgentSurfaceText "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
$importerCheckText = Get-AgentSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
$validateText = Get-AgentSurfaceText "tools/validate.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$recipePlansText = Get-AgentSurfaceText "tools/run-validation-recipe-plans.ps1"
$recipeRunnerCheckText = Get-AgentSurfaceText "tools/check-validation-recipe-runner.ps1"
$validateWorkflowText = Get-AgentSurfaceText ".github/workflows/validate.yml"
$ciMatrixCheckText = Get-AgentSurfaceText "tools/check-ci-matrix.ps1"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-readiness-evidence-promotion-v1.md"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "RendererCommercialReadinessFinalRetainedRootArtifactImport",
        "ArtifactListJsonRelative",
        "workflow_artifact_list",
        "workflow_artifact_list_present",
        "artifact_handoff_strategy",
        "final_retained_root_artifact",
        "assembler_source_artifacts",
        "missing_workflow_artifact_names",
        "missing_assembler_inputs",
        "assembler_handoff",
        "final_preflight_handoff",
        "assembler_handoff_ready",
        "gh run download",
        "GitHub CLI",
        "GitHub Actions artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-metal-memory-profiling-host-artifacts",
        "host-gate-summary.json",
        "mtlresidencyset_unavailable",
        "D3d12CommercialQualityHostEvidence",
        "VulkanStrictCommercialQualityHostEvidence",
        "AppleMetalCommercialQualityHostEvidence",
        "RendererMetalMemoryProfilingHostEvidence",
        "RendererPackageCommercialQualityHostEvidence",
        "RendererQualityVfxCommercialHostEvidence",
        "RendererCleanRoomLegalReviewInput",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $importerText $needle "renderer commercial readiness final retained-root artifact import script"
}

foreach ($needle in @(
        "check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1",
        "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=4",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=renderer-clean-room-legal-review-artifacts",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
        "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $importerCheckText $needle "renderer commercial readiness final retained-root artifact import check"
    Assert-ContainsText $validateText "check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1" `
        "renderer commercial readiness final retained-root artifact import validate task"
}

foreach ($needle in @(
        "rendererCommercialReadinessFinalRetainedRootArtifactImport",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness final retained-root artifact import command"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "GitHub Actions artifacts",
        "seven explicit assembler inputs",
        "assembler_handoff",
        "final_preflight_handoff",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-promotion-preflight",
        "renderer-commercial-readiness-final-retained-root-assembler",
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "renderer-clean-room-legal-review-input",
        "assemble-renderer-commercial-readiness-final-retained-root.ps1",
        "generate-renderer-clean-room-legal-review-input.ps1",
        "import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "artifact-intake-plan-only",
        "clean-room-review-input-plan-only",
        "final-retained-root-plan-only"
    )) {
    Assert-ContainsText $recipePlansText $needle "renderer commercial readiness final retained-root validation recipe runner plan"
}

foreach ($needle in @(
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-promotion-preflight"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-retained-root-assembler"',
        'Assert-DryRunRecipe -Recipe "renderer-clean-room-legal-review-input"',
        'Assert-DryRunRecipe -Recipe "renderer-commercial-readiness-final-retained-root-artifact-import"',
        'Assert-ArgvDoesNotContainText -Result $rendererFinalArtifactImportDryRun -Unexpected "-RunId"'
    )) {
    Assert-ContainsText $recipeRunnerCheckText $needle "renderer commercial readiness final retained-root validation recipe runner check"
}

foreach ($surface in @(
        @{ Text = $productionLoopFragmentText; Label = "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json" },
        @{ Text = $manifestText; Label = "engine/agent/manifest.json" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $planText; Label = "renderer commercial readiness evidence promotion plan" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-readiness-final-retained-root-artifact-import",
            "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
            "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1",
            "GitHub Actions artifacts",
            "artifact_handoff_strategy",
            "missing_assembler_inputs",
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) final retained-root artifact import"
    }
}

foreach ($surface in @(
        @{ Text = $validateWorkflowText; Label = ".github/workflows/validate.yml" },
        @{ Text = $ciMatrixCheckText; Label = "tools/check-ci-matrix.ps1" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-artifact-intake",
            "Renderer Commercial Artifact Intake",
            "gh api -H",
            'actions/runs/${{ github.run_id }}/artifacts?per_page=100',
            "current-run-artifact-intake",
            "renderer-commercial-readiness-current-run-artifact-intake",
            "actions: read",
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) current-run artifact intake"
    }
}
