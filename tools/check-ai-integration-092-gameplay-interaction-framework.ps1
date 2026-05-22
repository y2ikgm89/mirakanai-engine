#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.2 for check-ai-integration.ps1 gameplay interaction framework contracts.

$gameplayInteractionHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp"
$gameplayInteractionSourceText = Get-AgentSurfaceText "engine/runtime/src/gameplay_interaction.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$runtimeTestsText = Get-AgentSurfaceText "tests/unit/runtime_gameplay_interaction_tests.cpp"
$sample2dText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3dText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$generatedGameValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeGameplayInteractionEvent",
        "RuntimeGameplayInteractionState",
        "RuntimeGameplayInteractionDiagnosticCode",
        "RuntimeGameplayInteractionPlan",
        "plan_runtime_gameplay_interactions"
    )) {
    Assert-ContainsText $gameplayInteractionHeaderText $needle "engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp"
}

foreach ($needle in @(
        "RuntimeGameplayInteractionKind::damage",
        "RuntimeGameplayInteractionKind::loss",
        "RuntimeGameplayInteractionKind::objective_progress",
        "RuntimeGameplayInteractionKind::restart",
        "RuntimeGameplayInteractionDiagnosticCode::terminal_session_state",
        "plan_runtime_gameplay_interactions"
    )) {
    Assert-ContainsText $gameplayInteractionSourceText $needle "engine/runtime/src/gameplay_interaction.cpp"
}

Assert-ContainsText $runtimeCMakeText "src/gameplay_interaction.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_gameplay_interaction_tests" "CMakeLists.txt"
Assert-ContainsText $runtimeTestsText "runtime gameplay interactions update state and feedback deterministically" "tests/unit/runtime_gameplay_interaction_tests.cpp"
Assert-ContainsText $runtimeTestsText "runtime gameplay interactions fail closed on invalid requests" "tests/unit/runtime_gameplay_interaction_tests.cpp"

foreach ($sampleSurface in @(
        @{ Text = $sample2dText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3dText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    Assert-ContainsText $sampleSurface.Text "plan_runtime_gameplay_interactions" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_interaction_ready" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_interaction_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_interaction_feedback_rows" $sampleSurface.Label
    Assert-ContainsText $sampleSurface.Text "gameplay_systems_interaction_final_session_state" $sampleSurface.Label
}

foreach ($manifestSurface in @(
        @{ Text = $sample2dManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json" },
        @{ Text = $sample3dManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json" }
    )) {
    Assert-ContainsText $manifestSurface.Text "gameplay_systems_interaction_ready=1" $manifestSurface.Label
    Assert-ContainsText $manifestSurface.Text "gameplay_systems_interaction_rows=10" $manifestSurface.Label
    Assert-ContainsText $manifestSurface.Text "gameplay_systems_interaction_feedback_rows=10" $manifestSurface.Label
    Assert-ContainsText $manifestSurface.Text "gameplay_systems_interaction_final_session_state=running" $manifestSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedGameValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeGameplayInteractionEvent" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_gameplay_interactions" $docSurface.Label
    Assert-ContainsText $docSurface.Text "gameplay_systems_interaction_feedback_rows" $docSurface.Label
}

Assert-ContainsText $planRegistryText "2026-05-23-engine-gameplay-interaction-framework-v1.md" "docs/superpowers/plans/README.md"
Assert-ContainsText $backlogText '| `engine-gameplay-interaction-framework-v1` | `foundational-unblocker` | `implemented-1x-foundation` |' "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
Assert-ContainsText $manifestText "engine-gameplay-interaction-framework-v1" "engine/agent/manifest.json"
Assert-ContainsText $manifestText "engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp" "engine/agent/manifest.json"
Assert-ContainsText $manifestText "RuntimeGameplayInteractionEvent" "engine/agent/manifest.json"
Assert-ContainsText $manifestText "gameplay_systems_interaction_feedback_rows" "engine/agent/manifest.json"
