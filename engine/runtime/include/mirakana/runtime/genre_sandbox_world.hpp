// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSandboxWorldStatus : std::uint8_t {
    ready = 0,
    no_rows,
    invalid_request,
};

enum class RuntimeSandboxMutationKind : std::uint8_t {
    placement = 0,
    destruction,
};

enum class RuntimeSandboxMutationStatus : std::uint8_t {
    accepted = 0,
    blocked_missing_chunk,
    blocked_out_of_bounds,
    blocked_occupied,
    blocked_missing_cell,
    blocked_protected,
    blocked_missing_cost,
    invalid,
};

enum class RuntimeSandboxPersistenceStatus : std::uint8_t {
    accepted = 0,
    repairable,
    rejected,
};

enum class RuntimeSandboxDiagnosticCode : std::uint8_t {
    missing_world_id = 0,
    unsupported_backend_reference,
    invalid_chunk_id,
    duplicate_chunk,
    invalid_chunk_extent,
    invalid_existing_cell,
    duplicate_existing_cell,
    unknown_existing_cell_chunk,
    invalid_placement_intent,
    duplicate_placement_intent,
    invalid_destruction_intent,
    duplicate_destruction_intent,
    invalid_construction_cost,
    duplicate_construction_cost,
    invalid_persistence_row,
    duplicate_persistence_key,
    unsupported_game_content_rule,
    row_budget_exceeded,
};

struct RuntimeSandboxCellCoord {
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};

    [[nodiscard]] bool operator==(const RuntimeSandboxCellCoord&) const = default;
};

struct RuntimeSandboxChunkRow {
    std::string chunk_id;
    std::string region_id;
    std::int32_t origin_x{0};
    std::int32_t origin_y{0};
    std::int32_t origin_z{0};
    std::uint32_t size_x{0U};
    std::uint32_t size_y{0U};
    std::uint32_t size_z{0U};
    bool resident{false};
    bool persistent{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxChunkRow&) const = default;
};

struct RuntimeSandboxExistingCellRow {
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    bool destructible{false};
    bool protected_cell{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxExistingCellRow&) const = default;
};

struct RuntimeSandboxConstructionCostRow {
    std::string block_id;
    std::string item_id;
    std::uint32_t quantity{0U};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxConstructionCostRow&) const = default;
};

struct RuntimeSandboxPlacementIntent {
    std::string intent_id;
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    std::vector<RuntimeSandboxConstructionCostRow> provided_costs;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxPlacementIntent&) const = default;
};

struct RuntimeSandboxDestructionIntent {
    std::string intent_id;
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxDestructionIntent&) const = default;
};

struct RuntimeSandboxPersistenceRow {
    std::string chunk_id;
    std::string key;
    std::uint32_t expected_schema_version{1U};
    std::uint32_t observed_schema_version{1U};
    RuntimeSandboxPersistenceStatus status{RuntimeSandboxPersistenceStatus::rejected};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxPersistenceRow&) const = default;
};

struct RuntimeSandboxWorldMutationRequest {
    std::string world_id;
    std::uint64_t world_tick{0U};
    std::vector<RuntimeSandboxChunkRow> chunk_rows;
    std::vector<RuntimeSandboxExistingCellRow> existing_cell_rows;
    std::vector<RuntimeSandboxPlacementIntent> placement_intents;
    std::vector<RuntimeSandboxDestructionIntent> destruction_intents;
    std::vector<RuntimeSandboxConstructionCostRow> construction_cost_rows;
    std::vector<RuntimeSandboxPersistenceRow> persistence_rows;
    std::vector<std::string> game_content_rule_ids;
    std::size_t row_budget{512U};
    std::uint64_t seed{0U};
};

struct RuntimeSandboxMutationRow {
    RuntimeSandboxMutationKind kind{RuntimeSandboxMutationKind::placement};
    RuntimeSandboxMutationStatus status{RuntimeSandboxMutationStatus::invalid};
    std::string intent_id;
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string block_id;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxMutationRow&) const = default;
};

struct RuntimeSandboxDiagnostic {
    RuntimeSandboxDiagnosticCode code{RuntimeSandboxDiagnosticCode::missing_world_id};
    std::string world_id;
    std::string row_id;
    std::string chunk_id;
    RuntimeSandboxCellCoord coord;
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSandboxDiagnostic&) const = default;
};

struct RuntimeSandboxWorldMutationPlan {
    RuntimeSandboxWorldStatus status{RuntimeSandboxWorldStatus::invalid_request};
    std::vector<RuntimeSandboxDiagnostic> diagnostics;
    std::vector<RuntimeSandboxChunkRow> chunk_rows;
    std::vector<RuntimeSandboxExistingCellRow> existing_cell_rows;
    std::vector<RuntimeSandboxPlacementIntent> placement_intent_rows;
    std::vector<RuntimeSandboxDestructionIntent> destruction_intent_rows;
    std::vector<RuntimeSandboxConstructionCostRow> construction_cost_rows;
    std::vector<RuntimeSandboxMutationRow> mutation_rows;
    std::vector<RuntimeSandboxPersistenceRow> persistence_rows;
    std::size_t chunk_count{0U};
    std::size_t resident_chunk_count{0U};
    std::size_t placement_intent_count{0U};
    std::size_t accepted_placement_count{0U};
    std::size_t rejected_placement_count{0U};
    std::size_t destruction_intent_count{0U};
    std::size_t accepted_destruction_count{0U};
    std::size_t rejected_destruction_count{0U};
    std::size_t construction_cost_count{0U};
    std::size_t persistence_row_count{0U};
    std::size_t repairable_persistence_row_count{0U};
    std::size_t rejected_unsafe_mutation_count{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_world_mutation{false};
    bool invoked_persistence_io{false};
    bool invoked_package_io{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews sandbox-world chunk rows and mutation intents and emits deterministic package-evidence rows.
/// This value-only planner does not mutate a world, write persistence, load packages, execute streaming, own biome or
/// block-art rules, call renderer/platform/editor APIs, create threads, or expose native handles.
[[nodiscard]] RuntimeSandboxWorldMutationPlan
plan_runtime_sandbox_world_mutation(const RuntimeSandboxWorldMutationRequest& request);

} // namespace mirakana::runtime
