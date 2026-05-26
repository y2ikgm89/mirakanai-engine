#requires -Version 7.0
#requires -PSEdition Core

# Chapter 10.2 for check-ai-integration.ps1 Runtime network production security gate.

$headerText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/network_production_security.hpp"
$sourceText = Get-AgentSurfaceText "engine/runtime/src/network_production_security.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$testText = Get-AgentSurfaceText "tests/unit/runtime_network_production_security_tests.cpp"
$enetTestText = Get-AgentSurfaceText "tests/unit/runtime_network_enet_tests.cpp"
$threatModelText = Get-AgentSurfaceText "docs/specs/2026-05-26-networking-production-security-threat-model.md"
$sampleMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sampleManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sampleReadmeText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/README.md"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$networkValidationText = Get-AgentSurfaceText "tools/validate-network-enet.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md"
$registryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$modulesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/004-modules.json"
$readinessFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
$recipesFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/009-validationRecipes.json"
$loopFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
$gameGuidanceFragmentText = Get-AgentSurfaceText "engine/agent/manifest.fragments/014-gameCodeGuidance.json"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeNetworkProductionThreatModelEvidenceRow",
        "RuntimeNetworkProductionValidationEvidenceRow",
        "RuntimeNetworkUnsupportedOnlineClaimRow",
        "RuntimeNetworkProductionSecurityRequest",
        "RuntimeNetworkProductionSecurityPlan",
        "RuntimeNetworkProductionSecurityDiagnosticCode",
        "unsupported_online_service_claim",
        "general_online_ready",
        "plan_runtime_network_production_security_gate"
    )) {
    Assert-ContainsText $headerText $needle "engine/runtime/include/mirakana/runtime/network_production_security.hpp"
}

foreach ($needle in @(
        "missing_loopback_host_evidence",
        "missing_replication_host_evidence",
        "missing_sequence_replay_rejection",
        "missing_input_command_validation",
        "missing_snapshot_validation",
        "missing_rollback_window_diagnostic",
        "unsupported_online_service_claim",
        "side_effect_claim",
        "compute_replay_hash",
        "plan.general_online_ready = false"
    )) {
    Assert-ContainsText $sourceText $needle "engine/runtime/src/network_production_security.cpp"
}

Assert-ContainsText $runtimeCMakeText "src/network_production_security.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_network_production_security_tests" "CMakeLists.txt"

foreach ($needle in @(
        "runtime network production security gate accepts threat modeled loopback and replication evidence",
        "runtime network production security gate separates missing loopback host evidence",
        "runtime network production security gate rejects unsupported online service claims",
        "runtime network production security gate rejects incomplete threat model validation gaps and side effects"
    )) {
    Assert-ContainsText $testText $needle "tests/unit/runtime_network_production_security_tests.cpp"
}

foreach ($needle in @(
        "enet network transport adapter supplies production security loopback host evidence",
        "plan_runtime_network_production_security_gate",
        "general_online_ready"
    )) {
    Assert-ContainsText $enetTestText $needle "tests/unit/runtime_network_enet_tests.cpp"
}

foreach ($needle in @(
        "Attacker Capabilities",
        "Trust Boundaries",
        "Packet Tampering And Replay",
        "Authentication Gap",
        "Denial Of Service",
        "NAT And Matchmaking Exclusions",
        "Save And Rollback Abuse",
        "ENet"
    )) {
    Assert-ContainsText $threatModelText $needle "docs/specs/2026-05-26-networking-production-security-threat-model.md"
}

foreach ($needle in @(
        "validate_network_production_security_package_evidence",
        "network_production_security_status",
        "network_production_security_threat_model_reviewed",
        "network_production_security_loopback_host_evidence",
        "network_production_security_general_online_ready",
        "network_production_security_invoked_external_network_io"
    )) {
    Assert-ContainsText $sampleMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($needle in @(
        "network_production_security_status",
        "network_production_security_threat_model_reviewed",
        "network_production_security_loopback_host_evidence",
        "network_production_security_general_online_ready",
        "network_production_security_invoked_external_network_io"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($needle in @(
        "network-production-security",
        "network_production_security_*",
        "broad-online-services",
        "matchmaking-nat-traversal",
        "network-encryption-authentication"
    )) {
    Assert-ContainsText $sampleManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

Assert-ContainsText $sampleReadmeText "Network production security proof" "games/sample_2d_desktop_runtime_package/README.md"
Assert-ContainsText $networkValidationText "MK_runtime_network_production_security_tests" "tools/validate-network-enet.ps1"
Assert-ContainsText $networkValidationText "runtime_network_(production_security|transport_adapter|enet)_tests" "tools/validate-network-enet.ps1"

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-26-engine-general-production-quality-expansion-v1.md" },
        @{ Text = $registryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $gameGuidanceFragmentText; Label = "engine/agent/manifest.fragments/014-gameCodeGuidance.json" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_network_production_security_gate" $docSurface.Label
    Assert-ContainsText $docSurface.Text "threat" $docSurface.Label
    Assert-ContainsText $docSurface.Text "NAT" $docSurface.Label
    Assert-ContainsText $docSurface.Text "broad online" $docSurface.Label
}

foreach ($needle in @(
        "engine/runtime/include/mirakana/runtime/network_production_security.hpp",
        "Runtime Network Production Security Gate v1",
        "plan_runtime_network_production_security_gate",
        "network-production-security-gate",
        "network-production-security"
    )) {
    Assert-ContainsText $modulesFragmentText $needle "engine/agent/manifest.fragments/004-modules.json"
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}

Assert-ContainsText $readinessFragmentText "network-production-security-gate" "engine/agent/manifest.fragments/006-runtimeBackendReadiness.json"
Assert-ContainsText $recipesFragmentText '"network-production-security"' "engine/agent/manifest.fragments/009-validationRecipes.json"
Assert-ContainsText $loopFragmentText "network-production-security-gate-v1" "engine/agent/manifest.fragments/010-aiOperableProductionLoop.json"
