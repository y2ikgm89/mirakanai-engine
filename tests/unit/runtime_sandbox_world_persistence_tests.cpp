// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/sandbox_world_persistence.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeSandboxCellCoord;
using mirakana::runtime::RuntimeSandboxWorldAtomicSaveOperationKind;
using mirakana::runtime::RuntimeSandboxWorldPersistenceDiagnosticCode;
using mirakana::runtime::RuntimeSandboxWorldPersistenceStatus;

[[nodiscard]] RuntimeSandboxCellCoord cell(std::int32_t x, std::int32_t y, std::int32_t z) {
    return RuntimeSandboxCellCoord{.x = x, .y = y, .z = z};
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxChunkRow chunk(std::string id, std::string region, std::int32_t origin_x,
                                                              std::int32_t origin_y, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxChunkRow{
        .chunk_id = std::move(id),
        .region_id = std::move(region),
        .origin_x = origin_x,
        .origin_y = origin_y,
        .origin_z = 0,
        .size_x = 4U,
        .size_y = 4U,
        .size_z = 1U,
        .resident = true,
        .persistent = true,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxExistingCellRow
existing_cell(std::string chunk_id, RuntimeSandboxCellCoord coord, std::string tile_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxExistingCellRow{
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .block_id = std::move(tile_id),
        .destructible = true,
        .protected_cell = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorld make_world() {
    const auto result = mirakana::runtime::build_runtime_sandbox_world(mirakana::runtime::RuntimeSandboxWorldDesc{
        .world_id = "sandbox.persist",
        .world_tick = 88U,
        .chunk_rows =
            {
                chunk("chunk.b", "region.spawn", 4, 0, 2U),
                chunk("chunk.a", "region.spawn", 0, 0, 1U),
            },
        .existing_cell_rows =
            {
                existing_cell("chunk.b", cell(5, 1, 0), "tile.stone", 2U),
                existing_cell("chunk.a", cell(1, 1, 0), "tile.soil", 1U),
            },
        .row_budget = 64U,
    });
    MK_REQUIRE(result.succeeded());
    return result.world;
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldPersistenceDocumentDesc make_document_desc() {
    return mirakana::runtime::RuntimeSandboxWorldPersistenceDocumentDesc{
        .world = make_world(),
        .schema_version = 7U,
        .source_package_id = "runtime/packages/main",
        .seed = 12345U,
        .dirty_regions =
            {
                mirakana::runtime::RuntimeSandboxWorldDirtyRegion{
                    .chunk_id = "chunk.b",
                    .intent_id = "place.stone",
                    .min_coord = cell(5, 1, 0),
                    .max_coord_exclusive = cell(6, 2, 1),
                    .layer_mask = 1U,
                    .previous_block_id = {},
                    .new_block_id = "tile.stone",
                    .replay_hash = 44U,
                    .chunk_dirty = true,
                },
                mirakana::runtime::RuntimeSandboxWorldDirtyRegion{
                    .chunk_id = "chunk.a",
                    .intent_id = "place.soil",
                    .min_coord = cell(1, 1, 0),
                    .max_coord_exclusive = cell(2, 2, 1),
                    .layer_mask = 1U,
                    .previous_block_id = {},
                    .new_block_id = "tile.soil",
                    .replay_hash = 43U,
                    .chunk_dirty = true,
                },
            },
        .row_budget = 64U,
        .byte_budget = 4096U,
    };
}

[[nodiscard]] std::size_t
diagnostic_count(const std::vector<mirakana::runtime::RuntimeSandboxWorldPersistenceDiagnostic>& diagnostics,
                 RuntimeSandboxWorldPersistenceDiagnosticCode code) {
    return static_cast<std::size_t>(
        std::ranges::count_if(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; }));
}

} // namespace

MK_TEST("runtime sandbox persistence document emits canonical snapshot rows") {
    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_persistence_document(make_document_desc());

    MK_REQUIRE(plan.status == RuntimeSandboxWorldPersistenceStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.world_id == "sandbox.persist");
    MK_REQUIRE(plan.schema_version == 7U);
    MK_REQUIRE(plan.source_package_id == "runtime/packages/main");
    MK_REQUIRE(plan.seed == 12345U);
    MK_REQUIRE(plan.world_tick == 88U);
    MK_REQUIRE(plan.chunk_rows.size() == 2U);
    MK_REQUIRE(plan.chunk_rows[0].chunk_id == "chunk.a");
    MK_REQUIRE(plan.chunk_rows[0].origin == cell(0, 0, 0));
    MK_REQUIRE(plan.chunk_rows[0].size == cell(4, 4, 1));
    MK_REQUIRE(plan.chunk_rows[0].layer_mask == 1U);
    MK_REQUIRE(plan.chunk_rows[0].changed_cell_count == 1U);
    MK_REQUIRE(plan.chunk_rows[0].source_package_id == "runtime/packages/main");
    MK_REQUIRE(plan.chunk_rows[0].content_hash != 0U);
    MK_REQUIRE(plan.layer_rows.size() == 2U);
    MK_REQUIRE(plan.layer_rows[0].chunk_id == "chunk.a");
    MK_REQUIRE(plan.layer_rows[0].layer_id == 0);
    MK_REQUIRE(plan.layer_rows[0].cell_count == 1U);
    MK_REQUIRE(plan.layer_rows[0].content_hash != 0U);
    MK_REQUIRE(plan.changed_cell_rows.size() == 2U);
    MK_REQUIRE(plan.changed_cell_rows[0].chunk_id == "chunk.a");
    MK_REQUIRE(plan.changed_cell_rows[0].coord == cell(1, 1, 0));
    MK_REQUIRE(plan.changed_cell_rows[0].layer_id == 0);
    MK_REQUIRE(plan.changed_cell_rows[0].tile_id == "tile.soil");
    MK_REQUIRE(plan.changed_cell_rows[0].content_hash != 0U);
    MK_REQUIRE(plan.content_hash != 0U);
    MK_REQUIRE(plan.byte_size_estimate == plan.canonical_text.size());
    MK_REQUIRE(plan.canonical_text.starts_with("format=GameEngine.RuntimeSandboxWorldSnapshot.v1\n"));
    MK_REQUIRE(plan.canonical_text.contains("world.id=sandbox.persist\n"));
    MK_REQUIRE(plan.canonical_text.contains("schema.version=7\n"));
    MK_REQUIRE(plan.canonical_text.contains("source.package.id=runtime/packages/main\n"));
    MK_REQUIRE(plan.canonical_text.contains("seed=12345\n"));
    MK_REQUIRE(plan.canonical_text.contains("world.tick=88\n"));
    MK_REQUIRE(plan.canonical_text.contains("chunk.0.id=chunk.a\n"));
    MK_REQUIRE(plan.canonical_text.contains("cell.0.tile=tile.soil\n"));
    MK_REQUIRE(!plan.invoked_filesystem_io);
    MK_REQUIRE(!plan.invoked_platform_call);
    MK_REQUIRE(!plan.invoked_threading);
}

MK_TEST("runtime sandbox snapshot diff omits unchanged chunks and budgets deterministic dirty chunks") {
    const auto full = mirakana::runtime::plan_runtime_sandbox_world_persistence_document(make_document_desc());
    MK_REQUIRE(full.succeeded());

    auto desc = make_document_desc();
    desc.dirty_regions = {
        mirakana::runtime::RuntimeSandboxWorldDirtyRegion{
            .chunk_id = "chunk.b",
            .intent_id = "place.stone",
            .min_coord = cell(5, 1, 0),
            .max_coord_exclusive = cell(6, 2, 1),
            .layer_mask = 1U,
            .previous_block_id = {},
            .new_block_id = "tile.stone",
            .replay_hash = 44U,
            .chunk_dirty = true,
        },
    };

    const auto diff = mirakana::runtime::plan_runtime_sandbox_world_snapshot_diff(
        mirakana::runtime::RuntimeSandboxWorldSnapshotDiffDesc{
            .document = desc,
            .previous_chunk_rows = full.chunk_rows,
        });

    MK_REQUIRE(diff.status == RuntimeSandboxWorldPersistenceStatus::ready);
    MK_REQUIRE(diff.succeeded());
    MK_REQUIRE(diff.chunk_rows.size() == 1U);
    MK_REQUIRE(diff.chunk_rows[0].chunk_id == "chunk.b");
    MK_REQUIRE(diff.changed_cell_rows.size() == 1U);
    MK_REQUIRE(diff.changed_cell_rows[0].chunk_id == "chunk.b");
    MK_REQUIRE(diff.layer_rows.size() == 1U);
    MK_REQUIRE(diff.content_hash != 0U);

    desc.byte_budget = 8U;
    const auto over_budget = mirakana::runtime::plan_runtime_sandbox_world_snapshot_diff(
        mirakana::runtime::RuntimeSandboxWorldSnapshotDiffDesc{
            .document = desc,
            .previous_chunk_rows = full.chunk_rows,
        });
    MK_REQUIRE(over_budget.status == RuntimeSandboxWorldPersistenceStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(over_budget.diagnostics,
                                RuntimeSandboxWorldPersistenceDiagnosticCode::size_budget_exceeded) == 1U);
    MK_REQUIRE(over_budget.chunk_rows.empty());
    MK_REQUIRE(!over_budget.invoked_filesystem_io);
}

MK_TEST("runtime sandbox migration review reports exact chains and corruption recovery boundaries") {
    const auto migration = mirakana::runtime::review_runtime_sandbox_world_migration(
        mirakana::runtime::RuntimeSandboxWorldMigrationReviewDesc{
            .observed_schema_version = 1U,
            .target_schema_version = 3U,
            .minimum_supported_schema_version = 1U,
            .migration_steps =
                {
                    {.from_schema_version = 2U, .to_schema_version = 3U, .migration_id = "world-v2-v3"},
                    {.from_schema_version = 1U, .to_schema_version = 2U, .migration_id = "world-v1-v2"},
                },
            .row_budget = 16U,
        });

    MK_REQUIRE(migration.status == RuntimeSandboxWorldPersistenceStatus::migration_required);
    MK_REQUIRE(!migration.succeeded());
    MK_REQUIRE(migration.diagnostics.empty());
    MK_REQUIRE(migration.migration_rows.size() == 2U);
    MK_REQUIRE(migration.migration_rows[0].migration_id == "world-v1-v2");
    MK_REQUIRE(migration.migration_rows[1].migration_id == "world-v2-v3");
    MK_REQUIRE(migration.replay_hash != 0U);
    MK_REQUIRE(!migration.invoked_filesystem_io);

    const auto future = mirakana::runtime::review_runtime_sandbox_world_migration(
        mirakana::runtime::RuntimeSandboxWorldMigrationReviewDesc{
            .observed_schema_version = 4U,
            .target_schema_version = 3U,
            .minimum_supported_schema_version = 1U,
        });
    MK_REQUIRE(future.status == RuntimeSandboxWorldPersistenceStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(future.diagnostics,
                                RuntimeSandboxWorldPersistenceDiagnosticCode::unsupported_future_schema) == 1U);
    MK_REQUIRE(!future.invoked_filesystem_io);

    const auto missing = mirakana::runtime::review_runtime_sandbox_world_migration(
        mirakana::runtime::RuntimeSandboxWorldMigrationReviewDesc{
            .observed_schema_version = 1U,
            .target_schema_version = 3U,
            .minimum_supported_schema_version = 1U,
            .migration_steps = {{.from_schema_version = 1U, .to_schema_version = 2U, .migration_id = "world-v1-v2"}},
        });
    MK_REQUIRE(missing.status == RuntimeSandboxWorldPersistenceStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(missing.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::missing_migration) ==
               1U);
    MK_REQUIRE(missing.migration_rows.empty());

    const auto repairable = mirakana::runtime::review_runtime_sandbox_world_migration(
        mirakana::runtime::RuntimeSandboxWorldMigrationReviewDesc{
            .observed_schema_version = 3U,
            .target_schema_version = 3U,
            .minimum_supported_schema_version = 1U,
            .corruption_rows = {{.key = "chunk.a.light", .repairable = true, .message = "missing light cache"}},
        });
    MK_REQUIRE(repairable.status == RuntimeSandboxWorldPersistenceStatus::recovery_required);
    MK_REQUIRE(!repairable.succeeded());
    MK_REQUIRE(diagnostic_count(repairable.diagnostics,
                                RuntimeSandboxWorldPersistenceDiagnosticCode::repairable_corruption) == 1U);
    MK_REQUIRE(repairable.repair_rows.size() == 1U);

    const auto unrecoverable = mirakana::runtime::review_runtime_sandbox_world_migration(
        mirakana::runtime::RuntimeSandboxWorldMigrationReviewDesc{
            .observed_schema_version = 3U,
            .target_schema_version = 3U,
            .minimum_supported_schema_version = 1U,
            .corruption_rows = {{.key = "chunk.a.cells", .repairable = false, .message = "truncated cell rows"}},
        });
    MK_REQUIRE(unrecoverable.status == RuntimeSandboxWorldPersistenceStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(unrecoverable.diagnostics,
                                RuntimeSandboxWorldPersistenceDiagnosticCode::unrecoverable_corruption) == 1U);
    MK_REQUIRE(unrecoverable.repair_rows.empty());
}

MK_TEST("runtime sandbox atomic save plans temp flush replace backup and rollback without filesystem calls") {
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_atomic_save(mirakana::runtime::RuntimeSandboxWorldAtomicSaveDesc{
            .target_path = "saves/worlds/slot_1.sandbox_world",
            .temp_path = "saves/worlds/slot_1.sandbox_world.tmp",
            .backup_path = "saves/worlds/slot_1.sandbox_world.bak",
            .payload_content_hash = 777U,
            .payload_byte_size = 2048U,
            .require_flush = true,
            .row_budget = 8U,
        });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldPersistenceStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.operation_rows.size() == 4U);
    MK_REQUIRE(plan.operation_rows[0].step_index == 0U);
    MK_REQUIRE(plan.operation_rows[0].kind == RuntimeSandboxWorldAtomicSaveOperationKind::write_temp);
    MK_REQUIRE(plan.operation_rows[0].target_path == "saves/worlds/slot_1.sandbox_world.tmp");
    MK_REQUIRE(plan.operation_rows[0].payload_content_hash == 777U);
    MK_REQUIRE(plan.operation_rows[0].payload_byte_size == 2048U);
    MK_REQUIRE(plan.operation_rows[1].kind == RuntimeSandboxWorldAtomicSaveOperationKind::flush_temp);
    MK_REQUIRE(plan.operation_rows[1].flush_required);
    MK_REQUIRE(plan.operation_rows[2].kind == RuntimeSandboxWorldAtomicSaveOperationKind::replace_target_with_temp);
    MK_REQUIRE(plan.operation_rows[2].source_path == "saves/worlds/slot_1.sandbox_world.tmp");
    MK_REQUIRE(plan.operation_rows[2].target_path == "saves/worlds/slot_1.sandbox_world");
    MK_REQUIRE(plan.operation_rows[2].backup_path == "saves/worlds/slot_1.sandbox_world.bak");
    MK_REQUIRE(plan.operation_rows[3].kind ==
               RuntimeSandboxWorldAtomicSaveOperationKind::rollback_from_backup_on_failure);
    MK_REQUIRE(plan.operation_rows[3].rollback_only_on_failure);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_filesystem_io);
    MK_REQUIRE(!plan.invoked_platform_call);
    MK_REQUIRE(!plan.invoked_threading);

    const auto unsafe =
        mirakana::runtime::plan_runtime_sandbox_world_atomic_save(mirakana::runtime::RuntimeSandboxWorldAtomicSaveDesc{
            .target_path = "C:/outside/world.sandbox_world",
            .temp_path = "saves/worlds/../world.tmp",
            .backup_path = "saves\\world.bak",
            .payload_content_hash = 0U,
            .payload_byte_size = 0U,
            .require_flush = true,
            .row_budget = 2U,
        });
    MK_REQUIRE(unsafe.status == RuntimeSandboxWorldPersistenceStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(unsafe.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::invalid_path) == 3U);
    MK_REQUIRE(
        diagnostic_count(unsafe.diagnostics, RuntimeSandboxWorldPersistenceDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(unsafe.operation_rows.empty());
    MK_REQUIRE(!unsafe.invoked_filesystem_io);
}

int main() {
    return mirakana::test::run_all();
}
