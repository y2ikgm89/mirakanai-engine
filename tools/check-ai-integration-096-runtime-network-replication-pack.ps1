#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.6 for check-ai-integration.ps1 Runtime Network Replication Pack production contracts.

$replicationHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/production_network_replication.hpp"
$replicationSourceText = Get-AgentSurfaceText "engine/runtime/src/production_network_replication.cpp"
$scriptModdingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/scripting_sandbox.hpp"
$scriptModdingSourceText = Get-AgentSurfaceText "engine/runtime/src/scripting_sandbox.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$replicationTestsText = Get-AgentSurfaceText "tests/unit/runtime_production_network_replication_tests.cpp"
$scriptModdingTestsText = Get-AgentSurfaceText "tests/unit/runtime_scripting_sandbox_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md"
$sandboxNetworkModdingPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md"
$sandboxNetworkThreatModelText = Get-AgentSurfaceText "docs/specs/2026-05-30-sandbox-world-network-security-threat-model.md"
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
        "RuntimeNetworkSandboxMutationCommandRow",
        "RuntimeNetworkSandboxSnapshotDeltaRow",
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
        "RuntimeNetworkReplicationDiagnosticCode::duplicate_sandbox_mutation_command_id",
        "RuntimeNetworkReplicationDiagnosticCode::duplicate_sandbox_mutation_sequence",
        "RuntimeNetworkReplicationDiagnosticCode::non_monotonic_snapshot_tick",
        "RuntimeNetworkReplicationDiagnosticCode::non_monotonic_sandbox_mutation_tick",
        "RuntimeNetworkReplicationDiagnosticCode::invalid_sandbox_mutation_command",
        "RuntimeNetworkReplicationDiagnosticCode::invalid_sandbox_snapshot_delta",
        "RuntimeNetworkReplicationDiagnosticCode::unknown_sandbox_snapshot_command",
        "RuntimeNetworkReplicationDiagnosticCode::snapshot_byte_budget_exceeded",
        "RuntimeNetworkReplicationDiagnosticCode::sandbox_delta_byte_budget_exceeded",
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

foreach ($needle in @(
        "sandbox_mutation_command_rows",
        "sandbox_snapshot_delta_rows",
        "sandbox_mutation_command_count",
        "sandbox_snapshot_delta_count"
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
Assert-ContainsText $replicationTestsText "production network replication reviews sandbox mutation commands and snapshot deltas" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects sandbox mutation replay authority and delta abuse" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication rejects duplicate sandbox mutation command ids" "tests/unit/runtime_production_network_replication_tests.cpp"
Assert-ContainsText $replicationTestsText "production network replication applies one byte budget across snapshots and sandbox deltas" "tests/unit/runtime_production_network_replication_tests.cpp"

foreach ($needle in @(
        "RuntimeScriptModdingAdapterPolicyRow",
        "RuntimeScriptModdingDeniedCapabilityRow",
        "RuntimeScriptModdingPolicyDesc",
        "RuntimeScriptModdingPolicyPlan",
        "plan_runtime_script_modding_policy"
    )) {
    Assert-ContainsText $scriptModdingHeaderText $needle "engine/runtime/include/mirakana/runtime/scripting_sandbox.hpp"
}

foreach ($needle in @(
        "filesystem",
        "network",
        "process",
        "native_plugin",
        "package_mutation",
        "denied_capability_requested",
        "unreviewed_adapter",
        "nondeterministic_adapter",
        "missing_replay_seed"
    )) {
    Assert-ContainsText $scriptModdingSourceText $needle "engine/runtime/src/scripting_sandbox.cpp"
}

foreach ($needle in @(
        "runtime script modding policy plans sorted reviewed deterministic adapter rows",
        "runtime script modding policy rejects unsafe capability requests before adapter rows",
        "runtime script modding policy requires reviewed deterministic adapters"
    )) {
    Assert-ContainsText $scriptModdingTestsText $needle "tests/unit/runtime_scripting_sandbox_tests.cpp"
}

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

foreach ($docSurface in @(
        @{ Text = $sandboxNetworkModdingPlanText; Label = "docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md" },
        @{ Text = $sandboxNetworkThreatModelText; Label = "docs/specs/2026-05-30-sandbox-world-network-security-threat-model.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_script_modding_policy" $docSurface.Label
    Assert-ContainsText $docSurface.Text "broad online" $docSurface.Label
    Assert-ContainsText $docSurface.Text "SDL3" $docSurface.Label
}

foreach ($needle in @(
        "production-network-replication-v1",
        "sandbox-world-network-modding-gate-v1",
        '"unsupportedProductionGaps": []',
        "production-rendering-vfx-profiling-v1",
        "engine/runtime/include/mirakana/runtime/production_network_replication.hpp",
        "RuntimeNetworkReplicationSessionDesc",
        "RuntimeNetworkReplicationPlan",
        "RuntimeNetworkSandboxMutationCommandRow",
        "RuntimeScriptModdingAdapterPolicyRow",
        "plan_runtime_network_replication",
        "plan_runtime_script_modding_policy",
        "network_replication_*",
        "currentRuntimeNetworkReplication",
        "currentSandboxWorldNetworkModding"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
