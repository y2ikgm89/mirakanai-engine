#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.5 for check-ai-integration.ps1 Runtime Simulation Management Pack production contracts.

$simulationHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp"
$simulationSourceText = Get-AgentSurfaceText "engine/runtime/src/genre_simulation_management.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$simulationTestsText = Get-AgentSurfaceText "tests/unit/runtime_genre_simulation_management_tests.cpp"
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
        "RuntimeSimulationResourceRow",
        "RuntimeSimulationResourceBalanceRow",
        "RuntimeSimulationJobRow",
        "RuntimeSimulationLogisticsLink",
        "RuntimeSimulationLogisticsTransferRow",
        "RuntimeSimulationEconomySummary",
        "RuntimeSimulationPopulationNeedRow",
        "RuntimeSimulationScheduleRow",
        "RuntimeSimulationSaveReviewRow",
        "RuntimeSimulationDashboardRow",
        "RuntimeSimulationManagementPlan",
        "plan_runtime_simulation_management"
    )) {
    Assert-ContainsText $simulationHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp"
}

foreach ($needle in @(
        "RuntimeSimulationDiagnosticCode::unsupported_backend_reference",
        "RuntimeSimulationDiagnosticCode::unsupported_game_balance_rule",
        "RuntimeSimulationDiagnosticCode::row_budget_exceeded",
        "RuntimeSimulationDiagnosticCode::unknown_economy_resource",
        "RuntimeSimulationDiagnosticCode::unknown_population_need_resource",
        "RuntimeSimulationManagementStatus::invalid_request",
        "RuntimeSimulationJobStatus::assigned",
        "RuntimeSimulationLogisticsTransferStatus::scheduled",
        "RuntimeSimulationNeedStatus::deficit",
        "RuntimeSimulationSaveReviewStatus::repairable",
        "output_row_count(plan) > request.row_budget"
    )) {
    Assert-ContainsText $simulationSourceText $needle "engine/runtime/src/genre_simulation_management.cpp"
}

foreach ($needle in @(
        "bool invoked_economy_execution{false}",
        "bool invoked_save_io{false}",
        "bool invoked_runtime_ui{false}",
        "bool invoked_package_io{false}"
    )) {
    Assert-ContainsText $simulationHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp"
}

Assert-ContainsText $runtimeCMakeText "src/genre_simulation_management.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_genre_simulation_management_tests" "CMakeLists.txt"
Assert-ContainsText $simulationTestsText "runtime simulation management plans resources jobs logistics needs saves and dashboard rows" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management rejects unsafe or inconsistent rows before output rows" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management replay hash changes when normalized output fields change" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management projects competing rows without signed overflow" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management rejects missing cross referenced resources" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management diagnostics are ordered by stable public fields" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management diagnostic rows exceeding row budget add budget diagnostic" "tests/unit/runtime_genre_simulation_management_tests.cpp"
Assert-ContainsText $simulationTestsText "runtime simulation management generated rows exceeding row budget fail closed" "tests/unit/runtime_genre_simulation_management_tests.cpp"

foreach ($sampleSurface in @(
        @{ Text = $sample2dMainText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3dMainText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    foreach ($needle in @(
            "mirakana/runtime/genre_simulation_management.hpp",
            "plan_runtime_simulation_management",
            "simulation_management_status=",
            "simulation_management_ready=",
            "simulation_management_resource_balance_rows=",
            "simulation_management_job_assignment_rows=",
            "simulation_management_logistics_transfer_rows=",
            "simulation_management_replay_hash=",
            "simulation_management_diagnostics="
        )) {
        Assert-ContainsText $sampleSurface.Text $needle $sampleSurface.Label
    }
}

foreach ($manifestSurface in @(
        @{ Text = $sample2dManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json" },
        @{ Text = $sample3dManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json" }
    )) {
    foreach ($needle in @(
            '"simulation-management"',
            '"simulationManagement"',
            "simulation_management_status=ready",
            "simulation_management_ready=1",
            "simulation_management_tick_count=240",
            "simulation_management_resource_balance_rows=4",
            "simulation_management_job_assignment_rows=1",
            "simulation_management_dashboard_rows=7",
            "simulation_management_diagnostics=0"
        )) {
        Assert-ContainsText $manifestSurface.Text $needle $manifestSurface.Label
    }
}

foreach ($needle in @(
        "simulation_management_status",
        "simulation_management_ready",
        "simulation_management_tick_count",
        "simulation_management_resource_balance_rows",
        "simulation_management_job_rows",
        "simulation_management_job_assignment_rows",
        "simulation_management_logistics_links",
        "simulation_management_logistics_transfer_rows",
        "simulation_management_logistics_scheduled_transfer_rows",
        "simulation_management_economy_summary_rows",
        "simulation_management_population_need_rows",
        "simulation_management_need_deficit_rows",
        "simulation_management_schedule_rows",
        "simulation_management_save_review_rows",
        "simulation_management_save_review_repairable_rows",
        "simulation_management_dashboard_rows",
        "simulation_management_replay_hash",
        "simulation_management_invoked_economy_execution",
        "simulation_management_invoked_save_io",
        "simulation_management_invoked_runtime_ui",
        "simulation_management_invoked_package_io",
        "simulation_management_diagnostics"
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
    Assert-ContainsText $docSurface.Text "plan_runtime_simulation_management" $docSurface.Label
    Assert-ContainsText $docSurface.Text "simulation_management_ready=1" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "genre-simulation-management-pack-v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_simulation_management" $docSurface.Label
}

foreach ($needle in @(
        "genre-simulation-management-pack-v1",
        '"unsupportedProductionGaps": []',
        "production-network-replication-v1",
        "production-rendering-vfx-profiling-v1",
        "engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp",
        "RuntimeSimulationResourceRow",
        "RuntimeSimulationManagementPlan",
        "plan_runtime_simulation_management",
        "simulation_management_*",
        "currentRuntimeSimulationManagement"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
