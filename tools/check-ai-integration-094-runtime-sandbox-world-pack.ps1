#requires -Version 7.0
#requires -PSEdition Core

# Chapter 9.4 for check-ai-integration.ps1 Runtime Sandbox World Pack production contracts.

$sandboxHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp"
$sandboxSourceText = Get-AgentSurfaceText "engine/runtime/src/genre_sandbox_world.cpp"
$sandboxRuntimeHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp"
$sandboxRuntimeSourceText = Get-AgentSurfaceText "engine/runtime/src/sandbox_world_runtime.cpp"
$sandboxPersistenceHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/sandbox_world_persistence.hpp"
$sandboxPersistenceSourceText = Get-AgentSurfaceText "engine/runtime/src/sandbox_world_persistence.cpp"
$sandboxStreamingHeaderText = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/sandbox_world_streaming.hpp"
$sandboxStreamingSourceText = Get-AgentSurfaceText "engine/runtime/src/sandbox_world_streaming.cpp"
$runtimeCMakeText = Get-AgentSurfaceText "engine/runtime/CMakeLists.txt"
$rootCMakeText = Get-AgentSurfaceText "CMakeLists.txt"
$sandboxTestsText = Get-AgentSurfaceText "tests/unit/runtime_genre_sandbox_world_tests.cpp"
$sandboxRuntimeTestsText = Get-AgentSurfaceText "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
$sandboxPersistenceTestsText = Get-AgentSurfaceText "tests/unit/runtime_sandbox_world_persistence_tests.cpp"
$sandboxStreamingTestsText = Get-AgentSurfaceText "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
$sample2dMainText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/main.cpp"
$sample3dMainText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/main.cpp"
$sample2dManifestText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample3dManifestText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$installedValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
$planText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md"
$runtimePlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-29-generic-2d-sandbox-runtime-foundation-v1.md"
$mutationExecutionPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-30-generic-2d-sandbox-mutation-execution-v1.md"
$tileSimulationPlanText = Get-AgentSurfaceText "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md"
$planRegistryText = Get-AgentSurfaceText "docs/superpowers/plans/README.md"
$currentCapabilitiesText = Get-AgentSurfaceText "docs/current-capabilities.md"
$roadmapText = Get-AgentSurfaceText "docs/roadmap.md"
$testingText = Get-AgentSurfaceText "docs/testing.md"
$generatedValidationText = Get-AgentSurfaceText "docs/specs/generated-game-validation-scenarios.md"
$aiGameDevelopmentText = Get-AgentSurfaceText "docs/ai-game-development.md"
$sample2dReadmeText = Get-AgentSurfaceText "games/sample_2d_desktop_runtime_package/README.md"
$sample3dReadmeText = Get-AgentSurfaceText "games/sample_generated_desktop_runtime_3d_package/README.md"
$backlogText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md"
$projectionText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md"
$manifestText = Get-AgentSurfaceText "engine/agent/manifest.json"

foreach ($needle in @(
        "RuntimeSandboxChunkRow",
        "RuntimeSandboxExistingCellRow",
        "RuntimeSandboxPlacementIntent",
        "RuntimeSandboxDestructionIntent",
        "RuntimeSandboxConstructionCostRow",
        "RuntimeSandboxPersistenceRow",
        "RuntimeSandboxTileDropRow",
        "RuntimeSandboxConstructionCostConsumptionRow",
        "RuntimeSandboxToolEffectivenessRow",
        "RuntimeSandboxSpawnRegionRow",
        "RuntimeSandboxDayNightEventRow",
        "RuntimeSandboxTriggerRow",
        "RuntimeSandboxWorldMutationPlan",
        "plan_runtime_sandbox_world_mutation"
    )) {
    Assert-ContainsText $sandboxHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp"
}

foreach ($needle in @(
        "RuntimeSandboxDiagnosticCode::unsupported_backend_reference",
        "RuntimeSandboxDiagnosticCode::unsupported_game_content_rule",
        "RuntimeSandboxDiagnosticCode::row_budget_exceeded",
        "RuntimeSandboxWorldStatus::invalid_request",
        "lhs.message < rhs.message",
        "is_valid_id(intent.chunk_id)",
        "row.provided_costs.size()",
        "has_game_owned_content_reference",
        "construction_cost_consumption_rows",
        "validate_gameplay_hook_rows"
    )) {
    Assert-ContainsText $sandboxSourceText $needle "engine/runtime/src/genre_sandbox_world.cpp"
}

foreach ($needle in @(
        "bool invoked_world_mutation{false}",
        "bool invoked_persistence_io{false}",
        "bool invoked_package_io{false}"
    )) {
    Assert-ContainsText $sandboxHeaderText $needle "engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp"
}

Assert-ContainsText $runtimeCMakeText "src/genre_sandbox_world.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_genre_sandbox_world_tests" "CMakeLists.txt"
Assert-ContainsText $sandboxTestsText "runtime sandbox world plans chunk placement destruction costs mutation rows and persistence" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "runtime sandbox world rejects malformed rows and game-owned content rules before output rows" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "runtime sandbox world diagnostics are totally ordered by stable public fields" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "runtime sandbox world emits generic gameplay hook rows without owning game rules" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "runtime sandbox world rejects game specific catalogs formulas and economy hooks" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "changed_destruction.replay_hash != first.replay_hash" "tests/unit/runtime_genre_sandbox_world_tests.cpp"
Assert-ContainsText $sandboxTestsText "changed_provided_cost.replay_hash != first.replay_hash" "tests/unit/runtime_genre_sandbox_world_tests.cpp"

foreach ($needle in @(
        "RuntimeSandboxWorldDesc",
        "RuntimeSandboxWorldBuildResult",
        "RuntimeSandboxWorld",
        "RuntimeSandboxWorldSnapshot",
        "RuntimeSandboxCellSample",
        "RuntimeSandboxWorldMutationExecutionStatus",
        "RuntimeSandboxWorldDirtyRegion",
        "RuntimeSandboxWorldMutationExecutionResult",
        "RuntimeSandboxTileMaterialRow",
        "RuntimeSandboxTileSimulationDesc",
        "RuntimeSandboxTileSimulationPlan",
        "RuntimeSandboxTileCollisionSpanRow",
        "RuntimeSandboxTileCellRow",
        "RuntimeSandboxLightPropagationRow",
        "RuntimeSandboxLiquidFlowRow",
        "RuntimeSandboxScheduledTileUpdateRow",
        "build_runtime_sandbox_world",
        "sample_runtime_sandbox_cell",
        "snapshot_runtime_sandbox_world",
        "apply_runtime_sandbox_world_mutations",
        "plan_runtime_sandbox_tile_simulation"
    )) {
    Assert-ContainsText $sandboxRuntimeHeaderText $needle "engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp"
}

foreach ($needle in @(
        "RuntimeSandboxWorldRuntimeDiagnosticCode::unsupported_backend_reference",
        '"platform"',
        "chunks_overlap",
        "compute_snapshot_hash",
        "world_contains_cell",
        "layer_mask_for",
        "compute_dirty_region_hash",
        "compute_light_row_hash",
        "compute_liquid_row_hash",
        "compute_scheduled_row_hash",
        "append_light_rows",
        "append_liquid_flow_row",
        "RuntimeSandboxTileSimulationStatus::ready",
        "RuntimeSandboxTileSimulationDiagnosticCode::unknown_cell_material",
        "plan_runtime_sandbox_tile_simulation",
        "world.chunks.size()",
        "world.cells.size()",
        "world.invoked_persistence_io ? 1U : 0U",
        "RuntimeSandboxCellSampleStatus::missing_chunk",
        "RuntimeSandboxCellSampleStatus::occupied",
        "apply_runtime_sandbox_world_mutations"
    )) {
    Assert-ContainsText $sandboxRuntimeSourceText $needle "engine/runtime/src/sandbox_world_runtime.cpp"
}

Assert-ContainsText $runtimeCMakeText "src/sandbox_world_runtime.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_sandbox_world_runtime_tests" "CMakeLists.txt"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world builds a deterministic in memory chunk snapshot" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world samples existing empty and out of bounds cells" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world rejects invalid chunks cells duplicates and row budgets" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world rejects overlapping chunks before sampling" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "changed_block.world.snapshot_hash != first.world.snapshot_hash" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world snapshot recomputes mutable public world state" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world applies accepted placement and destruction with dirty regions" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox world rejects invalid execution plans without changing the input snapshot" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox tile simulation plans collision light liquid and trigger rows" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "runtime sandbox tile simulation rejects invalid material rows and unknown cells before output" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "RuntimeSandboxTileSimulationDiagnosticCode::row_budget_exceeded" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"
Assert-ContainsText $sandboxRuntimeTestsText "scheduled_update_rows" "tests/unit/runtime_sandbox_world_runtime_tests.cpp"

foreach ($needle in @(
        "RuntimeSandboxWorldPersistenceDocumentDesc",
        "RuntimeSandboxWorldPersistenceDocumentPlan",
        "RuntimeSandboxWorldSnapshotDiffDesc",
        "RuntimeSandboxWorldMigrationReviewPlan",
        "RuntimeSandboxWorldAtomicSavePlan",
        "plan_runtime_sandbox_world_persistence_document",
        "plan_runtime_sandbox_world_snapshot_diff",
        "review_runtime_sandbox_world_migration",
        "plan_runtime_sandbox_world_atomic_save"
    )) {
    Assert-ContainsText $sandboxPersistenceHeaderText $needle "engine/runtime/include/mirakana/runtime/sandbox_world_persistence.hpp"
}

foreach ($needle in @(
        "GameEngine.RuntimeSandboxWorldSnapshot.v1",
        "RuntimeSandboxWorldPersistenceStatus::ready",
        "RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_path",
        "RuntimeSandboxWorldPersistenceDiagnosticCode::unsupported_future_schema",
        "RuntimeSandboxWorldAtomicSaveOperationKind::replace_target_with_temp",
        "RuntimeSandboxWorldAtomicSaveOperationKind::rollback_from_backup_on_failure",
        "append_line(output",
        "is_safe_project_path",
        "plan_runtime_sandbox_world_snapshot_diff",
        "review_runtime_sandbox_world_migration",
        "plan_runtime_sandbox_world_atomic_save"
    )) {
    Assert-ContainsText $sandboxPersistenceSourceText $needle "engine/runtime/src/sandbox_world_persistence.cpp"
}

Assert-ContainsText $runtimeCMakeText "src/sandbox_world_persistence.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_sandbox_world_persistence_tests" "CMakeLists.txt"
Assert-ContainsText $sandboxPersistenceTestsText "runtime sandbox persistence document emits canonical snapshot rows" "tests/unit/runtime_sandbox_world_persistence_tests.cpp"
Assert-ContainsText $sandboxPersistenceTestsText "runtime sandbox snapshot diff omits unchanged chunks and budgets deterministic dirty chunks" "tests/unit/runtime_sandbox_world_persistence_tests.cpp"
Assert-ContainsText $sandboxPersistenceTestsText "runtime sandbox migration review reports exact chains and corruption recovery boundaries" "tests/unit/runtime_sandbox_world_persistence_tests.cpp"
Assert-ContainsText $sandboxPersistenceTestsText "runtime sandbox atomic save plans temp flush replace backup and rollback without filesystem calls" "tests/unit/runtime_sandbox_world_persistence_tests.cpp"

foreach ($needle in @(
        "RuntimeSandboxWorldStreamingSourceRow",
        "RuntimeSandboxWorldAddressableDependencyRow",
        "RuntimeSandboxWorldStreamingPlan",
        "RuntimeSandboxWorldStreamingSafePointDesc",
        "RuntimeSandboxWorldStreamingSafePointResult",
        "unsupported_automatic_lru_eviction",
        "plan_runtime_sandbox_world_streaming",
        "execute_runtime_sandbox_world_streaming_safe_point",
        "bool invoked_automatic_lru_eviction{false}"
    )) {
    Assert-ContainsText $sandboxStreamingHeaderText $needle "engine/runtime/include/mirakana/runtime/sandbox_world_streaming.hpp"
}

foreach ($needle in @(
        "pin_dirty_chunks",
        "plan_runtime_sandbox_world_streaming",
        "plan_runtime_world_region_streaming",
        "plan_runtime_addressable_content_streaming",
        "execute_runtime_world_region_streaming_safe_point",
        "RuntimeSandboxWorldStreamingDiagnosticCode::unsupported_automatic_lru_eviction",
        "RuntimeSandboxWorldStreamingDiagnosticCode::missing_addressable_dependency",
        "allow_automatic_lru_eviction"
    )) {
    Assert-ContainsText $sandboxStreamingSourceText $needle "engine/runtime/src/sandbox_world_streaming.cpp"
}

Assert-ContainsText $runtimeCMakeText "src/sandbox_world_streaming.cpp" "engine/runtime/CMakeLists.txt"
Assert-ContainsText $rootCMakeText "MK_runtime_sandbox_world_streaming_tests" "CMakeLists.txt"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming plans source selected chunks and addressable dependencies" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming pins dirty resident chunks and fails closed on resident budgets" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming rejects missing required addressable dependencies" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming rejects unsupported automatic lru eviction" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming safe point rejects invalid plans without package reads or live mutation" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "runtime sandbox world streaming safe point adopts reviewed package candidates" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"
Assert-ContainsText $sandboxStreamingTestsText "filesystem.read_text_count() == 2" "tests/unit/runtime_sandbox_world_streaming_tests.cpp"

foreach ($sampleSurface in @(
        @{ Text = $sample2dMainText; Label = "games/sample_2d_desktop_runtime_package/main.cpp" },
        @{ Text = $sample3dMainText; Label = "games/sample_generated_desktop_runtime_3d_package/main.cpp" }
    )) {
    foreach ($needle in @(
            "mirakana/runtime/genre_sandbox_world.hpp",
            "plan_runtime_sandbox_world_mutation",
            "sandbox_world_status=",
            "sandbox_world_ready=",
            "sandbox_world_mutation_rows=",
            "sandbox_world_replay_hash=",
            "sandbox_world_diagnostics="
        )) {
        Assert-ContainsText $sampleSurface.Text $needle $sampleSurface.Label
    }
}

foreach ($needle in @(
        "sandbox_world_cost_consumption_rows=",
        "sandbox_world_tile_drop_rows=",
        "sandbox_world_tool_effectiveness_rows=",
        "sandbox_world_spawn_region_rows=",
        "sandbox_world_day_night_event_rows=",
        "sandbox_world_trigger_rows="
    )) {
    Assert-ContainsText $sample2dMainText $needle "games/sample_2d_desktop_runtime_package/main.cpp"
}

foreach ($manifestSurface in @(
        @{ Text = $sample2dManifestText; Label = "games/sample_2d_desktop_runtime_package/game.agent.json" },
        @{ Text = $sample3dManifestText; Label = "games/sample_generated_desktop_runtime_3d_package/game.agent.json" }
    )) {
    foreach ($needle in @(
            '"sandbox-world"',
            '"sandboxWorld"',
            "sandbox_world_status=ready",
            "sandbox_world_ready=1",
            "sandbox_world_chunk_rows=2",
            "sandbox_world_placement_accepted_rows=1",
            "sandbox_world_destruction_accepted_rows=1",
            "sandbox_world_persistence_repairable_rows=1",
            "sandbox_world_diagnostics=0"
        )) {
        Assert-ContainsText $manifestSurface.Text $needle $manifestSurface.Label
    }
}

foreach ($needle in @(
        "sandbox_world_cost_consumption_rows=1",
        "sandbox_world_tile_drop_rows=1",
        "sandbox_world_tool_effectiveness_rows=1",
        "sandbox_world_spawn_region_rows=1",
        "sandbox_world_day_night_event_rows=1",
        "sandbox_world_trigger_rows=1"
    )) {
    Assert-ContainsText $sample2dManifestText $needle "games/sample_2d_desktop_runtime_package/game.agent.json"
}

foreach ($needle in @(
        "sandbox_world_status",
        "sandbox_world_ready",
        "sandbox_world_chunk_rows",
        "sandbox_world_resident_chunk_rows",
        "sandbox_world_existing_cell_rows",
        "sandbox_world_placement_accepted_rows",
        "sandbox_world_destruction_accepted_rows",
        "sandbox_world_cost_consumption_rows",
        "sandbox_world_tile_drop_rows",
        "sandbox_world_tool_effectiveness_rows",
        "sandbox_world_spawn_region_rows",
        "sandbox_world_day_night_event_rows",
        "sandbox_world_trigger_rows",
        "sandbox_world_mutation_rows",
        "sandbox_world_persistence_repairable_rows",
        "sandbox_world_rejected_unsafe_mutation_rows",
        "sandbox_world_replay_hash",
        "sandbox_world_diagnostics"
    )) {
    Assert-ContainsText $installedValidationText $needle "tools/validate-installed-desktop-runtime.ps1"
}

foreach ($docSurface in @(
        @{ Text = $planText; Label = "docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md" },
        @{ Text = $runtimePlanText; Label = "docs/superpowers/plans/2026-05-29-generic-2d-sandbox-runtime-foundation-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $sample2dReadmeText; Label = "games/sample_2d_desktop_runtime_package/README.md" },
        @{ Text = $sample3dReadmeText; Label = "games/sample_generated_desktop_runtime_3d_package/README.md" }
    )) {
    Assert-ContainsText $docSurface.Text "plan_runtime_sandbox_world_mutation" $docSurface.Label
    Assert-ContainsText $docSurface.Text "sandbox_world_ready=1" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $tileSimulationPlanText; Label = "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $generatedValidationText; Label = "docs/specs/generated-game-validation-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "sandbox_world_tile_drop_rows" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $runtimePlanText; Label = "docs/superpowers/plans/2026-05-29-generic-2d-sandbox-runtime-foundation-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "build_runtime_sandbox_world" $docSurface.Label
    Assert-ContainsText $docSurface.Text "sample_runtime_sandbox_cell" $docSurface.Label
    Assert-ContainsText $docSurface.Text "snapshot_runtime_sandbox_world" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $mutationExecutionPlanText; Label = "docs/superpowers/plans/2026-05-30-generic-2d-sandbox-mutation-execution-v1.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSandboxWorldMutationExecutionStatus" $docSurface.Label
    Assert-ContainsText $docSurface.Text "apply_runtime_sandbox_world_mutations" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeSandboxWorldDirtyRegion" $docSurface.Label
    Assert-ContainsText $docSurface.Text "RuntimeSandboxWorldMutationExecutionResult" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $tileSimulationPlanText; Label = "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $testingText; Label = "docs/testing.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSandboxTileSimulationPlan" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_sandbox_tile_simulation" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $tileSimulationPlanText; Label = "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $testingText; Label = "docs/testing.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSandboxWorldPersistenceDocumentPlan" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_sandbox_world_atomic_save" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $tileSimulationPlanText; Label = "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $aiGameDevelopmentText; Label = "docs/ai-game-development.md" },
        @{ Text = $testingText; Label = "docs/testing.md" },
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "RuntimeSandboxWorldStreamingPlan" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_sandbox_world_streaming" $docSurface.Label
    Assert-ContainsText $docSurface.Text "execute_runtime_sandbox_world_streaming_safe_point" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $runtimePlanText; Label = "docs/superpowers/plans/2026-05-29-generic-2d-sandbox-runtime-foundation-v1.md" },
        @{ Text = $currentCapabilitiesText; Label = "docs/current-capabilities.md" }
    )) {
    Assert-ContainsText $docSurface.Text "MK_runtime_sandbox_world_runtime_tests" $docSurface.Label
}

foreach ($docSurface in @(
        @{ Text = $planRegistryText; Label = "docs/superpowers/plans/README.md" },
        @{ Text = $roadmapText; Label = "docs/roadmap.md" },
        @{ Text = $backlogText; Label = "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md" },
        @{ Text = $projectionText; Label = "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md" }
    )) {
    Assert-ContainsText $docSurface.Text "genre-sandbox-world-pack-v1" $docSurface.Label
    Assert-ContainsText $docSurface.Text "plan_runtime_sandbox_world_mutation" $docSurface.Label
}

foreach ($needle in @(
        "genre-sandbox-world-pack-v1",
        '"unsupportedProductionGaps": []',
        "genre-simulation-management-pack-v1",
        "production-network-replication-v1",
        "engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp",
        "engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp",
        "engine/runtime/include/mirakana/runtime/sandbox_world_persistence.hpp",
        "engine/runtime/include/mirakana/runtime/sandbox_world_streaming.hpp",
        "RuntimeSandboxChunkRow",
        "RuntimeSandboxTileDropRow",
        "RuntimeSandboxConstructionCostConsumptionRow",
        "RuntimeSandboxToolEffectivenessRow",
        "RuntimeSandboxSpawnRegionRow",
        "RuntimeSandboxDayNightEventRow",
        "RuntimeSandboxTriggerRow",
        "RuntimeSandboxWorldDesc",
        "RuntimeSandboxWorldMutationExecutionStatus",
        "RuntimeSandboxWorldDirtyRegion",
        "RuntimeSandboxWorldMutationExecutionResult",
        "RuntimeSandboxTileSimulationPlan",
        "RuntimeSandboxLightPropagationRow",
        "RuntimeSandboxLiquidFlowRow",
        "RuntimeSandboxScheduledTileUpdateRow",
        "RuntimeSandboxWorldPersistenceDocumentPlan",
        "RuntimeSandboxWorldAtomicSavePlan",
        "RuntimeSandboxWorldStreamingPlan",
        "RuntimeSandboxWorldStreamingSafePointResult",
        "plan_runtime_sandbox_world_mutation",
        "build_runtime_sandbox_world",
        "apply_runtime_sandbox_world_mutations",
        "plan_runtime_sandbox_tile_simulation",
        "plan_runtime_sandbox_world_atomic_save",
        "plan_runtime_sandbox_world_streaming",
        "execute_runtime_sandbox_world_streaming_safe_point",
        "sandbox_world_*",
        "currentRuntimeSandboxTileSimulation",
        "currentRuntimeSandboxWorldPersistence",
        "currentRuntimeSandboxWorldStreaming",
        "currentRuntimeSandboxWorld"
    )) {
    Assert-ContainsText $manifestText $needle "engine/agent/manifest.json"
}
