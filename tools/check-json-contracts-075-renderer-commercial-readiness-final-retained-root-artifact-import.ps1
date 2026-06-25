#requires -Version 7.0
#requires -PSEdition Core
# Chapter 75 for check-json-contracts.ps1 renderer commercial readiness final retained-root artifact import.

$importerText = Get-JsonContractSurfaceText "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1"
$importerCheckText = Get-JsonContractSurfaceText "tools/check-renderer-commercial-readiness-final-retained-root-artifact-import.ps1"
$commandsFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$validationRecipesFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$productionLoopFragmentText = Get-JsonContractSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "GameEngine.RendererCommercialReadinessFinalRetainedRootArtifactImport.v1",
        "ArtifactListJsonRelative",
        "workflow_artifact_list",
        "assembler_handoff",
        "final_preflight_handoff",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names",
        "renderer_commercial_readiness_final_retained_root_artifact_import_assembler_handoff_ready",
        "renderer_commercial_readiness_final_retained_root_artifact_import_final_preflight_handoff_ready",
        "gh run download",
        "https://cli.github.com/manual/gh_run_download",
        "https://docs.github.com/en/rest/actions/artifacts",
        "renderer-commercial-readiness-final-retained-root",
        "renderer-metal-memory-profiling-host-artifacts",
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
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_workflow_artifacts=5",
        "renderer_commercial_readiness_final_retained_root_artifact_import_required_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_workflow_artifact_list_present=1",
        "renderer_commercial_readiness_final_retained_root_artifact_import_available_workflow_artifacts=4",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_workflow_artifact_names=renderer-commercial-readiness-final-retained-root",
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
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
        "tools/import-renderer-commercial-readiness-final-retained-root-artifacts.ps1",
        "GitHub Actions artifacts",
        "final retained root",
        "seven explicit assembler inputs",
        "assembler_handoff",
        "final_preflight_handoff"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
    Assert-ContainsText $productionLoopFragmentText $needle "renderer commercial readiness final retained-root artifact import production loop"
}
