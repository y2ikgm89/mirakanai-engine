#requires -Version 7.0
#requires -PSEdition Core
# Chapter 143 for check-ai-integration.ps1 renderer commercial readiness evidence promotion contract.

$schemaText = Get-AgentSurfaceText "schemas/renderer-commercial-readiness-evidence.schema.json"
$readyFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/ready/evidence.json"
$missingMetalFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/missing_metal/evidence.json"
$externalEngineRejectedFixtureText = Get-AgentSurfaceText "tests/fixtures/renderer/commercial-readiness-evidence/external_engine_rejected/evidence.json"
$productionLoopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-25-renderer-commercial-readiness-evidence-promotion-v1.md"

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
            "third_party_notices",
            "native_handles_exposed",
            "cross_backend_inference",
            "external_engine_parity",
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
