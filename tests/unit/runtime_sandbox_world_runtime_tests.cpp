// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/sandbox_world_runtime.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeSandboxCellCoord;
using mirakana::runtime::RuntimeSandboxWorldRuntimeDiagnosticCode;
using mirakana::runtime::RuntimeSandboxWorldRuntimeStatus;

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
existing_cell(std::string chunk_id, RuntimeSandboxCellCoord coord, std::string block_id, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxExistingCellRow{
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .block_id = std::move(block_id),
        .destructible = true,
        .protected_cell = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldDesc make_world_desc() {
    return mirakana::runtime::RuntimeSandboxWorldDesc{
        .world_id = "sandbox.runtime",
        .world_tick = 42U,
        .chunk_rows =
            {
                chunk("chunk.b", "region.spawn", 4, 0, 2U),
                chunk("chunk.a", "region.spawn", 0, 0, 1U),
            },
        .existing_cell_rows =
            {
                existing_cell("chunk.b", cell(5, 1, 0), "block.stone", 2U),
                existing_cell("chunk.a", cell(1, 1, 0), "block.soil", 1U),
            },
        .row_budget = 64U,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSandboxWorldBuildResult& result,
                                           RuntimeSandboxWorldRuntimeDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime sandbox world builds a deterministic in memory chunk snapshot") {
    const auto result = mirakana::runtime::build_runtime_sandbox_world(make_world_desc());

    MK_REQUIRE(result.status == RuntimeSandboxWorldRuntimeStatus::ready);
    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.diagnostics.empty());
    MK_REQUIRE(result.world.world_id == "sandbox.runtime");
    MK_REQUIRE(result.world.chunk_count == 2U);
    MK_REQUIRE(result.world.cell_count == 2U);
    MK_REQUIRE(result.world.snapshot_hash != 0U);
    MK_REQUIRE(!result.world.invoked_persistence_io);
    MK_REQUIRE(!result.world.invoked_package_io);
    MK_REQUIRE(!result.world.invoked_renderer_upload);
    MK_REQUIRE(result.world.chunks[0].chunk_id == "chunk.a");
    MK_REQUIRE(result.world.chunks[1].chunk_id == "chunk.b");
    MK_REQUIRE(result.world.cells[0].block_id == "block.soil");
    MK_REQUIRE(result.world.cells[1].block_id == "block.stone");
}

MK_TEST("runtime sandbox world samples existing empty and out of bounds cells") {
    const auto result = mirakana::runtime::build_runtime_sandbox_world(make_world_desc());
    MK_REQUIRE(result.succeeded());

    const auto occupied = mirakana::runtime::sample_runtime_sandbox_cell(result.world, cell(1, 1, 0));
    MK_REQUIRE(occupied.status == mirakana::runtime::RuntimeSandboxCellSampleStatus::occupied);
    MK_REQUIRE(occupied.chunk_id == "chunk.a");
    MK_REQUIRE(occupied.block_id == "block.soil");

    const auto empty = mirakana::runtime::sample_runtime_sandbox_cell(result.world, cell(2, 1, 0));
    MK_REQUIRE(empty.status == mirakana::runtime::RuntimeSandboxCellSampleStatus::empty);
    MK_REQUIRE(empty.chunk_id == "chunk.a");
    MK_REQUIRE(empty.block_id.empty());

    const auto missing = mirakana::runtime::sample_runtime_sandbox_cell(result.world, cell(99, 0, 0));
    MK_REQUIRE(missing.status == mirakana::runtime::RuntimeSandboxCellSampleStatus::missing_chunk);
    MK_REQUIRE(missing.chunk_id.empty());
}

MK_TEST("runtime sandbox world rejects invalid chunks cells duplicates and row budgets") {
    auto desc = make_world_desc();
    desc.world_id = "sandbox.gpu";
    desc.chunk_rows.push_back(desc.chunk_rows[0]);
    desc.chunk_rows.push_back(chunk("bad/chunk", "region.spawn", 8, 0, 3U));
    desc.chunk_rows.back().size_x = 0U;
    desc.existing_cell_rows.push_back(existing_cell("chunk.a", cell(1, 1, 0), "block.soil", 3U));
    desc.existing_cell_rows.push_back(existing_cell("missing.chunk", cell(0, 0, 0), "block.soil", 4U));
    desc.row_budget = 3U;

    const auto result = mirakana::runtime::build_runtime_sandbox_world(desc);

    MK_REQUIRE(result.status == RuntimeSandboxWorldRuntimeStatus::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::unsupported_backend_reference) == 1U);
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::duplicate_chunk) == 1U);
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk) == 2U);
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::duplicate_cell) == 1U);
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::unknown_cell_chunk) == 1U);
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(result.world.chunks.empty());
    MK_REQUIRE(result.world.cells.empty());
    MK_REQUIRE(result.world.snapshot_hash == 0U);
}

MK_TEST("runtime sandbox world rejects overlapping chunks before sampling") {
    auto desc = make_world_desc();
    desc.chunk_rows = {
        chunk("chunk.a", "region.spawn", 0, 0, 1U),
        chunk("chunk.overlap", "region.spawn", 2, 0, 2U),
    };
    desc.existing_cell_rows = {
        existing_cell("chunk.overlap", cell(3, 1, 0), "block.stone", 1U),
    };

    const auto result = mirakana::runtime::build_runtime_sandbox_world(desc);

    MK_REQUIRE(result.status == RuntimeSandboxWorldRuntimeStatus::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(result, RuntimeSandboxWorldRuntimeDiagnosticCode::invalid_chunk) == 1U);
    MK_REQUIRE(result.world.chunks.empty());
    MK_REQUIRE(result.world.cells.empty());
}

MK_TEST("runtime sandbox world snapshot hash changes only when public world state changes") {
    auto desc = make_world_desc();
    const auto first = mirakana::runtime::build_runtime_sandbox_world(desc);
    const auto first_snapshot = mirakana::runtime::snapshot_runtime_sandbox_world(first.world);
    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first_snapshot.hash == first.world.snapshot_hash);
    MK_REQUIRE(first_snapshot.chunk_count == 2U);
    MK_REQUIRE(first_snapshot.cell_count == 2U);

    desc.existing_cell_rows[0].block_id = "block.granite";
    const auto changed_block = mirakana::runtime::build_runtime_sandbox_world(desc);
    MK_REQUIRE(changed_block.succeeded());
    MK_REQUIRE(changed_block.world.snapshot_hash != first.world.snapshot_hash);

    desc = make_world_desc();
    desc.chunk_rows[0].origin_y = 4;
    desc.existing_cell_rows[0].coord.y = 5;
    const auto changed_chunk = mirakana::runtime::build_runtime_sandbox_world(desc);
    MK_REQUIRE(changed_chunk.succeeded());
    MK_REQUIRE(changed_chunk.world.snapshot_hash != first.world.snapshot_hash);
}

MK_TEST("runtime sandbox world snapshot recomputes mutable public world state") {
    const auto first = mirakana::runtime::build_runtime_sandbox_world(make_world_desc());
    const auto first_snapshot = mirakana::runtime::snapshot_runtime_sandbox_world(first.world);
    MK_REQUIRE(first.succeeded());

    auto mutated = first.world;
    mutated.cells.pop_back();
    const auto removed_cell_snapshot = mirakana::runtime::snapshot_runtime_sandbox_world(mutated);
    MK_REQUIRE(removed_cell_snapshot.cell_count == mutated.cells.size());
    MK_REQUIRE(removed_cell_snapshot.hash != first_snapshot.hash);

    mutated = first.world;
    mutated.cells[0].block_id = "block.granite";
    const auto changed_cell_snapshot = mirakana::runtime::snapshot_runtime_sandbox_world(mutated);
    MK_REQUIRE(changed_cell_snapshot.cell_count == first_snapshot.cell_count);
    MK_REQUIRE(changed_cell_snapshot.hash != first_snapshot.hash);
}

int main() {
    return mirakana::test::run_all();
}
