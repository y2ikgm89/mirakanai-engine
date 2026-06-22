#requires -Version 7.0
#requires -PSEdition Core
# Chapter 131 for check-ai-integration.ps1 MAVG Nanite comparison report gate.

$specText = Get-AgentSurfaceText "docs/specs/2026-06-21-mavg-nanite-comparison-taxonomy-v1.md"
$schemaText = Get-AgentSurfaceText "schemas/mavg-nanite-comparison-report.schema.json"
$validatorText = Get-AgentSurfaceText "tools/validate-mavg-nanite-comparison.ps1"
$commandsFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/002-commands.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$mavgAdvancedPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md"

foreach ($needle in @(
        "MAVG Nanite Comparison Taxonomy v1",
        "GameEngine.MavgNaniteComparisonReport.v1",
        "mavg-nanite-comparison-report-v1",
        "Nanite Virtualized Geometry Overview",
        "Nanite Technical Details",
        "https://dev.epicgames.com/documentation/unreal-engine/nanite-virtualized-geometry-in-unreal-engine",
        "https://dev.epicgames.com/documentation/unreal-engine/nanite-technical-details",
        "virtualized_geometry_system",
        "internal_compressed_mesh_format",
        "fine_grained_streaming",
        "automatic_lod",
        "cluster_visibility_culling",
        "fallback_mesh_behavior",
        "ray_tracing_fallback_behavior",
        "material_support_limits",
        "deformation_support_limits",
        "platform_support_limits",
        "storage_memory_residency",
        "authoring_import_workflow",
        "mavg_nanite_comparison_report_ready=1",
        "mavg_nanite_compatible=0",
        "mavg_nanite_equivalent=0",
        "mavg_nanite_superior=0",
        "mavg_nanite_marketing_claim_allowed=0"
    )) {
    Assert-ContainsText $specText $needle "MAVG Nanite comparison taxonomy spec"
}

foreach ($needle in @(
        "GameEngine.MavgNaniteComparisonReport.v1",
        "mavg-nanite-comparison-report-v1",
        "official_sources",
        "comparison_axes",
        "clean_room_policy",
        "claim_policy",
        '"mavg_nanite_comparison_report_ready": { "const": true }',
        '"mavg_nanite_compatible": { "const": false }',
        '"mavg_nanite_equivalent": { "const": false }',
        '"mavg_nanite_superior": { "const": false }',
        '"mavg_nanite_marketing_claim_allowed": { "const": false }'
    )) {
    Assert-ContainsText $schemaText $needle "mavg-nanite-comparison-report schema"
}

foreach ($needle in @(
        "validation_recipe=mavg-nanite-comparison",
        "mavg_nanite_comparison_report_status=ready",
        'mavg_nanite_comparison_report_ready=$(ConvertTo-CounterBit $ready)',
        "mavg_nanite_comparison_axes=12",
        "mavg_nanite_official_source_rows=2",
        "mavg_nanite_public_docs_only=1",
        "mavg_nanite_first_party_assets_only=1",
        "mavg_nanite_unreal_source_used=0",
        "mavg_nanite_private_format_used=0",
        "mavg_nanite_epic_sample_assets_used=0",
        "mavg_nanite_compatible=0",
        "mavg_nanite_equivalent=0",
        "mavg_nanite_superior=0",
        "mavg_nanite_marketing_claim_allowed=0",
        "MAVG Nanite comparison contract missing required text"
    )) {
    Assert-ContainsText $validatorText $needle "tools/validate-mavg-nanite-comparison.ps1"
}

Assert-ContainsText $commandsFragmentText "mavgNaniteComparisonCheck" "manifest commands MAVG Nanite comparison command"
Assert-ContainsText $manifestText "mavgNaniteComparisonCheck" "composed manifest MAVG Nanite comparison command"

foreach ($surface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $mavgAdvancedPlanText; Label = "MAVG advanced evidence plan" }
    )) {
    foreach ($needle in @(
            "mavg-nanite-comparison-report-v1",
            "GameEngine.MavgNaniteComparisonReport.v1",
            "tools/validate-mavg-nanite-comparison.ps1",
            "mavg_nanite_comparison_report_ready=1",
            "mavg_nanite_public_docs_only=1",
            "mavg_nanite_unreal_source_used=0",
            "mavg_nanite_private_format_used=0",
            "mavg_nanite_epic_sample_assets_used=0",
            "mavg_nanite_compatible=0",
            "mavg_nanite_equivalent=0",
            "mavg_nanite_superior=0",
            "mavg_nanite_marketing_claim_allowed=0"
        )) {
        Assert-ContainsText $surface.Text $needle "$($surface.Label) MAVG Nanite comparison report"
    }
    foreach ($forbiddenNeedle in @(
            "mavg_nanite_compatible=1",
            "mavg_nanite_equivalent=1",
            "mavg_nanite_superior=1",
            "mavg_nanite_marketing_claim_allowed=1"
        )) {
        Assert-DoesNotContainText $surface.Text $forbiddenNeedle "$($surface.Label) forbidden MAVG Nanite claim"
    }
}
