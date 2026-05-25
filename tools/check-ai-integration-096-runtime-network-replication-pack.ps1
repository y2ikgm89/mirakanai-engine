#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.6 for check-ai-integration.ps1 Runtime Network Replication Pack production contracts.

$replicationHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/production_network_replication.hpp"
$replicationSourceText = Get-AgentSurfaceText "engine/runtime/src/production_network_replication.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$replicationTestsText = Get-AgentSurfaceText "tests/unit/runtime_production_network_replication_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$sample2dReadmeText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/README.md"
$sample3dReadmeText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/README.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$projectionText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeNetworkReplicationSessionDesc",
        "RuntimeReplicatedObjectRow",
        "RuntimeReplicationInputCommandRow",
        "RuntimeReplicationSnapshotRow",
        "RuntimeRollbackPolicyRow",
        "RuntimeNetworkTransportEvidenceRow",
        "RuntimeNetworkReplicationPlan",
        "RuntimeNetworkReplicationDiagnosticCode",
        "plan_runtime_network_replication"
    )) {
    Assert-ContainsText $replicationHeaderText $needle "engine/runtime/include/mirakana/runtime/production_network_replication.hpp"
}

foreach ($needle in @(
        "RuntimeNetworkReplicationDiagnosticCode::invalid_foundation_plan",
        "RuntimeNetworkReplicationDiagnosticCode::unsupported_replication_mode",
        "RuntimeNetworkReplicationDiagnosticCode::unsupported_topology",
        "RuntimeNetworkReplicationDiagnosticCode::channel_authority_mismatch",
        "RuntimeNetworkReplicationDiagnosticCode::duplicate_input_sequence",
        "RuntimeNetworkReplicationDiagnosticCode::non_monotonic_snapshot_tick",
        "RuntimeNetworkReplicationDiagnosticCode::snapshot_byte_budget_exceeded",
        "RuntimeNetworkReplicationDiagnosticCode::rollback_prerequisite_missing",
        "RuntimeNetworkReplicationDiagnosticCode::unsupported_host_claim",
        "RuntimeNetworkReplicationStatus::host_evidence_required",
        "std::numeric_limits<std::uint64_t>::max() - byte_count",
        "clear_output_rows(plan)"
    )) {
    Assert-ContainsText $replicationSourceText $needle "engine/runtime/src/production_network_replication.cpp"
}

foreach ($needle in @(
        "bool invoked_network_io{false}",
        "bool invoked_rollback_execution{false}",
        "bool invoked_world_mutation{false}",
        "requires_transport_host_evidence",
        "has_transport_host_evidence"
    )) {
    Assert-ContainsText $replicationHeaderText $needle "engine/runtime/include/mirakana/runtime/production_network_replication.hpp"
}

Assert-ContainsText $runtimeCMakeText "src/production_network_replication.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_production_network_replication_tests" "CMakeLists.txt"
Assert-ContainsText $replicationTestsText "production network replication plans authoritative rows with host transport evidence" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication separates reviewed planning from missing host transport evidence" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects unsupported lockstep modes" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects unsupported topology unknown channels and authority mismatches" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects nonmonotonic input and snapshot timelines" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects rollback when deterministic prerequisites are absent" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects budgets unsupported host claims and invalid foundation plans" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication counts rejected unsafe rows uniquely" "tests/unit/runtime_production_network_replication_tests.cpp"

foreach ($sampleSurface in @(
        @{ Text = $sample2dMainText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3dMainText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    foreach ($needle in @(
            "mirakana/runtime/production_network_replication.hpp",
            "plan_runtime_network_replication",
            "network_replication_status=",
            "network_replication_reviewed=",
            "network_replication_ready=",
            "network_replication_object_rows=",
            "network_replication_input_rows=",
            "network_replication_snapshot_rows=",
            "network_replication_rollback_rows=",
            "network_replication_replay_hash=",
            "network_replication_transport_host_evidence=",
            "network_replication_invoked_network_io=",
            "network_replication_invoked_rollback_execution=",
            "network_replication_invoked_world_mutation=",
            "network_replication_diagnostics="
        )) {
        Assert-ContainsText $sampleSurface.Text $needle $sampleSurface.Label
    }
}

foreach ($manifestSurface in @(
        @{ Text = $sample2dManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json" },
        @{ Text = $sample3dManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json" }
    )) {
    foreach ($needle in @(
            '"network-replication"',
            "network_replication_status=host_evidence_required",
            "network_replication_reviewed=1",
            "network_replication_ready=0",
            "network_replication_object_rows=2",
            "network_replication_input_rows=2",
            "network_replication_snapshot_rows=2",
            "network_replication_rollback_rows=1",
            "network_replication_transport_host_evidence=0",
            "network_replication_diagnostics=0"
        )) {
        Assert-ContainsText $manifestSurface.Text $needle $manifestSurface.Label
    }
}

foreach ($needle in @(
        "network_replication_status",
        "network_replication_reviewed",
        "network_replication_ready",
        "network_replication_object_rows",
        "network_replication_input_rows",
        "network_replication_snapshot_rows",
        "network_replication_rollback_rows",
        "network_replication_rejected_unsafe_rows",
        "network_replication_replay_hash",
        "network_replication_requires_transport_host_evidence",
        "network_replication_transport_host_evidence",
        "network_replication_invoked_network_io",
        "network_replication_invoked_rollback_execution",
        "network_replication_invoked_world_mutation",
        "network_replication_diagnostics"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $sample2dReadmeText; Label = "games/sample_2d_desktop_runtime_package/README.md" },
        @{ Text = $sample3dReadmeText; Label = "games/sample_generated_desktop_runtime_3d_package/README.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_network_replication" $docSurface.Label
    Assert-ContainsText $docSurface.Text "network_replication_reviewed=1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "network_replication_ready=0" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "production-network-replication-v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_network_replication" $docSurface.Label
}

foreach ($needle in @(
        "production-network-replication-v1",
        '"unsupportedProductionGaps": []',
        "production-rendering-vfx-profiling-v1",
        "engine/runtime/include/mirakana/runtime/production_network_replication.hpp",
        "RuntimeNetworkReplicationSessionDesc",
        "RuntimeNetworkReplicationPlan",
        "plan_runtime_network_replication",
        "network_replication_*",
        "currentRuntimeNetworkReplication"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
