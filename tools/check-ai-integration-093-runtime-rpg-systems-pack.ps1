#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.3 for check-ai-integration.ps1 Runtime RPG Systems Pack production contracts.

$rpgHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp"
$rpgSourceText = Get-AgentSurfaceText "engine/runtime/src/genre_rpg_systems.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$rpgTestsText = Get-AgentSurfaceText "tests/unit/runtime_genre_rpg_systems_tests.cpp"
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
        "RuntimeRpgStatRow",
        "RuntimeRpgProgressionRow",
        "RuntimeRpgSkillRow",
        "RuntimeRpgEquipmentRow",
        "RuntimeRpgCombatLoopRequest",
        "RuntimeRpgRewardRow",
        "RuntimeRpgSaveValidationRow",
        "RuntimeRpgSystemsPlan",
        "plan_runtime_rpg_systems"
    )) {
    Assert-ContainsText $rpgHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp"
}

foreach ($needle in @(
        "RuntimeRpgDiagnosticCode::unsupported_backend_reference",
        "RuntimeRpgDiagnosticCode::row_budget_exceeded",
        "RuntimeRpgSystemsStatus::invalid_request"
    )) {
    Assert-ContainsText $rpgSourceText $needle "engine/runtime/src/genre_rpg_systems.cpp"
}

foreach ($needle in @(
        "bool invoked_combat_execution{false}",
        "bool invoked_reward_application{false}",
        "bool invoked_save_io{false}"
    )) {
    Assert-ContainsText $rpgHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp"
}

Assert-ContainsText $runtimeCMakeText "src/genre_rpg_systems.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_genre_rpg_systems_tests" "CMakeLists.txt"
Assert-ContainsText $rpgTestsText "runtime RPG systems normalizes stats progression skills equipment combat rewards and saves" "tests/unit/runtime_genre_rpg_systems_tests.cpp"
Assert-ContainsText $rpgTestsText "runtime RPG systems rejects unsafe or inconsistent rows before planning" "tests/unit/runtime_genre_rpg_systems_tests.cpp"

foreach ($sampleSurface in @(
        @{ Text = $sample2dMainText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3dMainText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    foreach ($needle in @(
            "mirakana/runtime/genre_rpg_systems.hpp",
            "plan_runtime_rpg_systems",
            "rpg_systems_status=",
            "rpg_systems_ready=",
            "rpg_systems_replay_hash=",
            "rpg_systems_diagnostics="
        )) {
        Assert-ContainsText $sampleSurface.Text $needle $sampleSurface.Label
    }
}

foreach ($manifestSurface in @(
        @{ Text = $sample2dManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json" },
        @{ Text = $sample3dManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json" }
    )) {
    foreach ($needle in @(
            '"rpg-systems"',
            '"rpgSystems"',
            "rpg_systems_status=ready",
            "rpg_systems_ready=1",
            "rpg_systems_stat_rows=8",
            "rpg_systems_combat_turn_rows=6",
            "rpg_systems_save_validation_repairable_rows=1",
            "rpg_systems_diagnostics=0"
        )) {
        Assert-ContainsText $manifestSurface.Text $needle $manifestSurface.Label
    }
}

foreach ($needle in @(
        "rpg_systems_status",
        "rpg_systems_ready",
        "rpg_systems_party_members",
        "rpg_systems_enemy_members",
        "rpg_systems_stat_rows",
        "rpg_systems_progression_rows",
        "rpg_systems_skill_blocked_rows",
        "rpg_systems_equipment_blocked_rows",
        "rpg_systems_combat_turn_rows",
        "rpg_systems_reward_rows",
        "rpg_systems_save_validation_repairable_rows",
        "rpg_systems_replay_hash",
        "rpg_systems_diagnostics"
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
    Assert-ContainsText $docSurface.Text "plan_runtime_rpg_systems" $docSurface.Label
    Assert-ContainsText $docSurface.Text "rpg_systems_ready=1" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "genre-rpg-systems-pack-v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_rpg_systems" $docSurface.Label
}

foreach ($needle in @(
        "genre-rpg-systems-pack-v1",
        "engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp",
        "RuntimeRpgStatRow",
        "plan_runtime_rpg_systems",
        "rpg_systems_*",
        "currentRuntimeRpgSystems"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
