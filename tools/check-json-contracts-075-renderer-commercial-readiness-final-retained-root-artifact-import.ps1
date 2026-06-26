#requires -Version 7.0
#requires -PSEdition Core
# Chapter 75 for check-json-contracts.ps1 renderer commercial readiness final retained-root artifact import.

$importerText = Get-JsonContractSurfaceText "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
$importerCheckText = Get-JsonContractSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
$commandsFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$validateWorkflowText = Get-JsonContractSurfaceText ".github/workflows/validate.yml"
$ciMatrixCheckText = Get-JsonContractSurfaceText "tools/check-ci-matrix.ps1"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1",
        "ArtifactListJsonRelative",
        "workflow_artifact_list",
        "artifact_handoff_strategy",
        "final_retained_root_artifact",
        "assembler_source_artifacts",
        "assembler_handoff",
        "final_preflight_handoff",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names",
        "missing_assembler_inputs",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready",
        "gh run download",
        "https://cli.github.com/manual/gh_run_download",
        "https://docs.github.com/en/rest/actions/artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-metal-memory-profiling-host-artifacts",
        "renderer-d3d12-commercial-quality-host-evidence",
        "renderer-vulkan-strict-commercial-quality-host-evidence",
        "renderer-apple-metal-commercial-quality-host-evidence",
        "renderer-package-commercial-quality-host-evidence",
        "renderer-quality-vfx-commercial-artifacts",
        "metal-host-optimization-artifacts",
        "linux-vulkan-host-evidence",
        "windows-packages",
        "GameEngine.RendererD3d12CommercialQualityHostEvidence.v1",
        "GameEngine.RendererVulkanStrictCommercialQualityHostEvidence.v1",
        "GameEngine.RendererAppleMetalCommercialQualityHostEvidence.v1",
        "GameEngine.RendererMetalMemoryProfilingHostEvidence.v1",
        "GameEngine.RendererPackageCommercialQualityHostEvidence.v1",
        "GameEngine.RendererQualityVfxCommercialHostEvidence.v1",
        "GameEngine.RendererCleanRoomLegalReviewInput.v1",
        "host-gate-summary.json",
        "renderer_commercial_readiness_final_retained_root_artifact_import_ready",
        "renderer_commercial_readiness=0",
        "renderer_environment_ready=0"
    )) {
    Assert-ContainsText $importerText $needle "renderer commercial readiness final retained-root artifact import script"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import-check: ok",
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=11",
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-clean-room-legal-review-artifacts,renderer-commercial-readiness-final-retained-root",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_root_workflow_artifact_available=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_source_workflow_artifacts=9",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_source_workflow_artifact_names=renderer-clean-room-legal-review-artifacts",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_input_names=d3d12_host_evidence,vulkan_strict_host_evidence,apple_metal_host_evidence,metal_memory_profiling_host_evidence,package_host_evidence,quality_vfx_host_evidence,clean_room_legal_review",
        "renderer_commercial_readiness_final_retained_root_artifact_import_metal_host_gate_reason=mtlresidencyset_unavailable",
        "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready=0",
        "renderer_commercial_readiness_final_retained_root_artifact_import_ready=1",
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $importerCheckText $needle "renderer commercial readiness final retained-root artifact import check"
}

foreach ($needle in @(
        "rendererCommercialReadinessFinalRetainedRootArtifactImport",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
    )) {
    Assert-ContainsText $commandsFragmentText $needle "renderer commercial readiness final retained-root artifact import command"
}

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "renderer-clean-room-legal-review-input",
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "tools/generate-renderer-clean-room-legal-review-input.ps1",
        "GitHub Actions artifacts",
        "renderer-clean-room-legal-review-artifacts",
        "final retained root",
        "seven explicit assembler inputs",
        "assembler_handoff",
        "final_preflight_handoff"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
    Assert-ContainsText $productionLoopFragmentText $needle "renderer commercial readiness final retained-root artifact import production loop"
}

foreach ($surface in @(
        @{ Text = $validateWorkflowText; Label = ".github/workflows/validate.yml" },
        @{ Text = $ciMatrixCheckText; Label = "tools/check-ci-matrix.ps1" }
    )) {
    foreach ($needle in @(
            "renderer-commercial-artifact-intake",
            "Renderer Commercial Artifact Intake",
            "Collect renderer quality/VFX commercial host evidence",
            "tools/collect-renderer-quality-vfx-commercial-host-evidence.ps1",
            "renderer-quality-vfx-commercial-artifacts",
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
