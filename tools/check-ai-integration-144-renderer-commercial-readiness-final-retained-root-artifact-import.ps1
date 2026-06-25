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
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-readiness-evidence-promotion-v1.md"

foreach ($needle in @(
        "renderer-commercial-readiness-final-retained-root-artifact-import",
        "RendererCommercialReadinessFinalRetainedRootArtifactImport",
        "gh run download",
        "GitHub CLI",
        "GitHub Actions artifacts",
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
        "renderer_commercial_readiness_final_retained_root_artifact_import_missing_assembler_inputs=7",
        "renderer_commercial_readiness_final_retained_root_artifact_import_present_assembler_inputs=7",
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
        "renderer_commercial_readiness=0"
    )) {
    Assert-ContainsText $validationRecipesFragmentText $needle "renderer commercial readiness final retained-root artifact import validation recipe"
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
            "renderer_commercial_readiness=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) final retained-root artifact import"
    }
}
