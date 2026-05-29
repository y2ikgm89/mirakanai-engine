// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/genre_sandbox_world.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeSandboxCellCoord;
using mirakana::runtime::RuntimeSandboxMutationKind;
using mirakana::runtime::RuntimeSandboxMutationStatus;
using mirakana::runtime::RuntimeSandboxPersistenceStatus;
using mirakana::runtime::RuntimeSandboxWorldStatus;

[[nodiscard]] RuntimeSandboxCellCoord cell(std::int32_t x, std::int32_t y, std::int32_t z) {
    return RuntimeSandboxCellCoord{.x = x, .y = y, .z = z};
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxChunkRow
make_chunk(std::string chunk_id, std::string region_id, bool resident, bool persistent, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxChunkRow{
        .chunk_id = std::move(chunk_id),
        .region_id = std::move(region_id),
        .origin_x = 0,
        .origin_y = 0,
        .origin_z = 0,
        .size_x = 8U,
        .size_y = 8U,
        .size_z = 8U,
        .resident = resident,
        .persistent = persistent,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxExistingCellRow
make_existing_cell(std::string chunk_id, RuntimeSandboxCellCoord coord, std::string block_id, bool destructible,
                   bool protected_cell, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxExistingCellRow{
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .block_id = std::move(block_id),
        .destructible = destructible,
        .protected_cell = protected_cell,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxConstructionCostRow
make_cost(std::string block_id, std::string item_id, std::uint32_t quantity, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxConstructionCostRow{
        .block_id = std::move(block_id),
        .item_id = std::move(item_id),
        .quantity = quantity,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxPlacementIntent
make_placement(std::string intent_id, std::string chunk_id, RuntimeSandboxCellCoord coord, std::string block_id,
               std::vector<mirakana::runtime::RuntimeSandboxConstructionCostRow> provided_costs,
               std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxPlacementIntent{
        .intent_id = std::move(intent_id),
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .block_id = std::move(block_id),
        .provided_costs = std::move(provided_costs),
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxDestructionIntent make_destruction(std::string intent_id,
                                                                                  std::string chunk_id,
                                                                                  RuntimeSandboxCellCoord coord,
                                                                                  std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxDestructionIntent{
        .intent_id = std::move(intent_id),
        .chunk_id = std::move(chunk_id),
        .coord = coord,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxPersistenceRow make_persistence(std::string chunk_id, std::string key,
                                                                               std::uint32_t expected_schema_version,
                                                                               std::uint32_t observed_schema_version,
                                                                               std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSandboxPersistenceRow{
        .chunk_id = std::move(chunk_id),
        .key = std::move(key),
        .expected_schema_version = expected_schema_version,
        .observed_schema_version = observed_schema_version,
        .status = RuntimeSandboxPersistenceStatus::rejected,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSandboxWorldMutationPlan& plan,
                                           mirakana::runtime::RuntimeSandboxDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxWorldMutationRequest make_hash_sensitive_request() {
    return mirakana::runtime::RuntimeSandboxWorldMutationRequest{
        .world_id = "sandbox.hash",
        .world_tick = 4U,
        .chunk_rows = {make_chunk("chunk.0", "region.0", true, true, 1U)},
        .existing_cell_rows = {make_existing_cell("chunk.0", cell(1, 0, 1), "block.soil", true, false, 1U)},
        .placement_intents = {make_placement("place.0", "chunk.0", cell(2, 0, 1), "block.wall",
                                             {make_cost("block.wall", "item.wood", 2U, 1U)}, 1U)},
        .destruction_intents = {make_destruction("destroy.0", "chunk.0", cell(1, 0, 1), 1U)},
        .construction_cost_rows = {make_cost("block.wall", "item.wood", 2U, 1U)},
        .persistence_rows = {make_persistence("chunk.0", "persist.chunk.0", 2U, 1U, 1U)},
        .tile_drop_rows = {},
        .tool_effectiveness_rows = {},
        .spawn_region_rows = {},
        .day_night_event_rows = {},
        .trigger_rows = {},
        .game_content_rule_ids = {},
        .row_budget = 512U,
        .seed = 0U,
    };
}

} // namespace

MK_TEST("runtime sandbox world plans chunk placement destruction costs mutation rows and persistence") {
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(mirakana::runtime::RuntimeSandboxWorldMutationRequest{
            .world_id = "sandbox.package",
            .world_tick = 12U,
            .chunk_rows =
                {
                    make_chunk("chunk.0", "region.spawn", true, true, 1U),
                    make_chunk("chunk.1", "region.spawn", true, true, 2U),
                },
            .existing_cell_rows =
                {
                    make_existing_cell("chunk.0", cell(1, 0, 1), "block.soil", true, false, 1U),
                    make_existing_cell("chunk.0", cell(2, 0, 1), "block.stone", false, true, 2U),
                },
            .placement_intents =
                {
                    make_placement("place.wall", "chunk.0", cell(3, 0, 1), "block.wall",
                                   {make_cost("block.wall", "item.wood", 3U, 1U)}, 1U),
                    make_placement("place.occupied", "chunk.0", cell(1, 0, 1), "block.wall",
                                   {make_cost("block.wall", "item.wood", 3U, 2U)}, 2U),
                    make_placement("place.missing_cost", "chunk.1", cell(1, 0, 1), "block.door", {}, 3U),
                },
            .destruction_intents =
                {
                    make_destruction("destroy.soil", "chunk.0", cell(1, 0, 1), 1U),
                    make_destruction("destroy.protected", "chunk.0", cell(2, 0, 1), 2U),
                },
            .construction_cost_rows =
                {
                    make_cost("block.wall", "item.wood", 3U, 1U),
                    make_cost("block.door", "item.wood", 4U, 2U),
                },
            .persistence_rows =
                {
                    make_persistence("chunk.0", "persist.chunk.0", 2U, 2U, 1U),
                    make_persistence("chunk.1", "persist.chunk.1", 3U, 2U, 2U),
                },
            .tile_drop_rows = {},
            .tool_effectiveness_rows = {},
            .spawn_region_rows = {},
            .day_night_event_rows = {},
            .trigger_rows = {},
            .game_content_rule_ids = {},
            .row_budget = 512U,
            .seed = 0U,
        });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.chunk_rows.size() == 2U);
    MK_REQUIRE(plan.existing_cell_rows.size() == 2U);
    MK_REQUIRE(plan.placement_intent_rows.size() == 3U);
    MK_REQUIRE(plan.destruction_intent_rows.size() == 2U);
    MK_REQUIRE(plan.construction_cost_rows.size() == 2U);
    MK_REQUIRE(plan.mutation_rows.size() == 5U);
    MK_REQUIRE(plan.persistence_rows.size() == 2U);
    MK_REQUIRE(plan.chunk_count == 2U);
    MK_REQUIRE(plan.resident_chunk_count == 2U);
    MK_REQUIRE(plan.placement_intent_count == 3U);
    MK_REQUIRE(plan.accepted_placement_count == 1U);
    MK_REQUIRE(plan.rejected_placement_count == 2U);
    MK_REQUIRE(plan.destruction_intent_count == 2U);
    MK_REQUIRE(plan.accepted_destruction_count == 1U);
    MK_REQUIRE(plan.rejected_destruction_count == 1U);
    MK_REQUIRE(plan.construction_cost_count == 2U);
    MK_REQUIRE(plan.persistence_row_count == 2U);
    MK_REQUIRE(plan.repairable_persistence_row_count == 1U);
    MK_REQUIRE(plan.rejected_unsafe_mutation_count == 3U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_world_mutation);
    MK_REQUIRE(!plan.invoked_persistence_io);
    MK_REQUIRE(!plan.invoked_package_io);
    MK_REQUIRE(plan.mutation_rows[0].kind == RuntimeSandboxMutationKind::placement);
    MK_REQUIRE(plan.mutation_rows[0].status == RuntimeSandboxMutationStatus::accepted);
    MK_REQUIRE(plan.mutation_rows[1].intent_id == "place.occupied");
    MK_REQUIRE(plan.mutation_rows[1].status == RuntimeSandboxMutationStatus::blocked_occupied);
    MK_REQUIRE(plan.mutation_rows[2].intent_id == "place.missing_cost");
    MK_REQUIRE(plan.mutation_rows[2].status == RuntimeSandboxMutationStatus::blocked_missing_cost);
    MK_REQUIRE(plan.mutation_rows[3].kind == RuntimeSandboxMutationKind::destruction);
    MK_REQUIRE(plan.mutation_rows[3].status == RuntimeSandboxMutationStatus::accepted);
    MK_REQUIRE(plan.mutation_rows[4].status == RuntimeSandboxMutationStatus::blocked_protected);
    MK_REQUIRE(plan.persistence_rows[0].status == RuntimeSandboxPersistenceStatus::accepted);
    MK_REQUIRE(plan.persistence_rows[1].status == RuntimeSandboxPersistenceStatus::repairable);
}

MK_TEST("runtime sandbox world emits generic gameplay hook rows without owning game rules") {
    using mirakana::runtime::RuntimeSandboxDayNightPhase;
    using mirakana::runtime::RuntimeSandboxTriggerKind;

    const auto
        plan =
            mirakana::runtime::plan_runtime_sandbox_world_mutation(
                mirakana::runtime::RuntimeSandboxWorldMutationRequest{
                    .world_id = "sandbox.gameplay.hooks",
                    .world_tick = 32U,
                    .chunk_rows = {make_chunk("chunk.0", "region.spawn", true, true, 1U)},
                    .existing_cell_rows = {make_existing_cell("chunk.0", cell(1, 0, 1), "block.soil", true, false, 1U)},
                    .placement_intents = {make_placement("place.wall", "chunk.0", cell(3, 0, 1), "block.wall",
                                                         {make_cost("block.wall", "item.wood", 3U, 1U)}, 1U)},
                    .destruction_intents = {make_destruction("destroy.soil", "chunk.0", cell(1, 0, 1), 1U)},
                    .construction_cost_rows = {make_cost("block.wall", "item.wood", 3U, 1U)},
                    .persistence_rows = {},
                    .tile_drop_rows =
                        {
                            mirakana::runtime::RuntimeSandboxTileDropRow{
                                .block_id = "block.soil",
                                .item_id = "item.soil",
                                .min_quantity = 1U,
                                .max_quantity = 2U,
                                .required_tool_category_id = "tool.digging",
                                .source_index = 1U,
                            },
                        },
                    .tool_effectiveness_rows =
                        {
                            mirakana::runtime::RuntimeSandboxToolEffectivenessRow{
                                .tool_category_id = "tool.digging",
                                .block_tag_id = "tag.soft_ground",
                                .effectiveness_tier = 2U,
                                .source_index = 1U,
                            },
                        },
                    .spawn_region_rows =
                        {
                            mirakana::runtime::RuntimeSandboxSpawnRegionRow{
                                .region_id = "region.spawn",
                                .spawn_group_id = "spawn.passive",
                                .max_active = 4U,
                                .source_index = 1U,
                            },
                        },
                    .day_night_event_rows =
                        {
                            mirakana::runtime::RuntimeSandboxDayNightEventRow{
                                .event_id = "cycle.dawn",
                                .phase = RuntimeSandboxDayNightPhase::dawn,
                                .first_tick = 120U,
                                .repeat_interval_ticks = 240U,
                                .source_index = 1U,
                            },
                        },
                    .trigger_rows =
                        {
                            mirakana::runtime::RuntimeSandboxTriggerRow{
                                .trigger_id = "trigger.camp",
                                .kind = RuntimeSandboxTriggerKind::interaction,
                                .event_id = "cycle.dawn",
                                .chunk_id = "chunk.0",
                                .coord = cell(2, 0, 1),
                                .interaction_id = "interaction.inspect",
                                .source_index = 1U,
                            },
                        },
                    .game_content_rule_ids = {},
                    .row_budget = 64U,
                    .seed = 7U,
                });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.tile_drop_rows.size() == 1U);
    MK_REQUIRE(plan.tool_effectiveness_rows.size() == 1U);
    MK_REQUIRE(plan.spawn_region_rows.size() == 1U);
    MK_REQUIRE(plan.day_night_event_rows.size() == 1U);
    MK_REQUIRE(plan.trigger_rows.size() == 1U);
    MK_REQUIRE(plan.construction_cost_consumption_rows.size() == 1U);
    MK_REQUIRE(plan.construction_cost_consumption_rows[0].intent_id == "place.wall");
    MK_REQUIRE(plan.construction_cost_consumption_rows[0].item_id == "item.wood");
    MK_REQUIRE(plan.construction_cost_consumption_rows[0].required_quantity == 3U);
    MK_REQUIRE(plan.construction_cost_consumption_rows[0].consumed_quantity == 3U);
    MK_REQUIRE(plan.construction_cost_consumption_rows[0].accepted);
    MK_REQUIRE(plan.tile_drop_count == 1U);
    MK_REQUIRE(plan.construction_cost_consumption_count == 1U);
    MK_REQUIRE(plan.tool_effectiveness_count == 1U);
    MK_REQUIRE(plan.spawn_region_count == 1U);
    MK_REQUIRE(plan.day_night_event_count == 1U);
    MK_REQUIRE(plan.trigger_count == 1U);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("runtime sandbox world rejects game specific catalogs formulas and economy hooks") {
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(mirakana::runtime::RuntimeSandboxWorldMutationRequest{
            .world_id = "sandbox.gameplay.reject",
            .world_tick = 33U,
            .chunk_rows = {make_chunk("chunk.0", "region.spawn", true, true, 1U)},
            .existing_cell_rows = {},
            .placement_intents = {},
            .destruction_intents = {},
            .construction_cost_rows = {},
            .persistence_rows = {},
            .tile_drop_rows = {mirakana::runtime::RuntimeSandboxTileDropRow{
                .block_id = "boss.dragon",
                .item_id = "item.trophy",
                .min_quantity = 1U,
                .max_quantity = 1U,
                .required_tool_category_id = "tool.any",
                .source_index = 1U,
            }},
            .tool_effectiveness_rows = {mirakana::runtime::RuntimeSandboxToolEffectivenessRow{
                .tool_category_id = "damage_formula.fire",
                .block_tag_id = "tag.soft_ground",
                .effectiveness_tier = 1U,
                .source_index = 2U,
            }},
            .spawn_region_rows = {mirakana::runtime::RuntimeSandboxSpawnRegionRow{
                .region_id = "region.spawn",
                .spawn_group_id = "npc.merchant",
                .max_active = 1U,
                .source_index = 3U,
            }},
            .day_night_event_rows = {mirakana::runtime::RuntimeSandboxDayNightEventRow{
                .event_id = "economy_balance.market",
                .phase = mirakana::runtime::RuntimeSandboxDayNightPhase::day,
                .first_tick = 1U,
                .repeat_interval_ticks = 10U,
                .source_index = 4U,
            }},
            .trigger_rows = {},
            .game_content_rule_ids = {},
            .row_budget = 64U,
            .seed = 8U,
        });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::unsupported_game_content_rule) ==
               4U);
    MK_REQUIRE(plan.tile_drop_rows.empty());
    MK_REQUIRE(plan.tool_effectiveness_rows.empty());
    MK_REQUIRE(plan.spawn_region_rows.empty());
    MK_REQUIRE(plan.day_night_event_rows.empty());
    MK_REQUIRE(plan.trigger_rows.empty());
    MK_REQUIRE(plan.construction_cost_consumption_rows.empty());
}

MK_TEST("runtime sandbox world rejects malformed rows and game-owned content rules before output rows") {
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(mirakana::runtime::RuntimeSandboxWorldMutationRequest{
            .world_id = "sandbox.renderer",
            .world_tick = 1U,
            .chunk_rows =
                {
                    make_chunk("chunk.0", "region.0", true, true, 1U),
                    make_chunk("chunk.0", "region.0", true, true, 2U),
                    make_chunk("bad/chunk", "region.1", true, true, 3U),
                },
            .existing_cell_rows =
                {
                    make_existing_cell("chunk.0", cell(0, 0, 0), "block.soil", true, false, 1U),
                    make_existing_cell("chunk.0", cell(0, 0, 0), "block.soil", true, false, 2U),
                },
            .placement_intents =
                {
                    make_placement("place.0", "chunk.0", cell(1, 0, 1), "block.wall", {}, 1U),
                    make_placement("place.bad_path", "bad/chunk", cell(1, 0, 1), "block.wall", {}, 2U),
                    make_placement("place.backend", "renderer", cell(1, 0, 1), "block.wall", {}, 3U),
                },
            .destruction_intents = {make_destruction("destroy.backend", "native", cell(0, 0, 0), 1U)},
            .construction_cost_rows = {make_cost("block.wall", "item.wood", 0U, 1U)},
            .persistence_rows = {},
            .tile_drop_rows = {},
            .tool_effectiveness_rows = {},
            .spawn_region_rows = {},
            .day_night_event_rows = {},
            .trigger_rows = {},
            .game_content_rule_ids = {"biome.desert", "balance.block-hardness"},
            .row_budget = 512U,
            .seed = 0U,
        });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::unsupported_backend_reference) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::duplicate_chunk) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::invalid_chunk_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::duplicate_existing_cell) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::invalid_placement_intent) == 2U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::invalid_destruction_intent) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::invalid_construction_cost) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::unsupported_game_content_rule) ==
               2U);
    MK_REQUIRE(plan.chunk_rows.empty());
    MK_REQUIRE(plan.mutation_rows.empty());
    MK_REQUIRE(plan.persistence_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("runtime sandbox world replay hash changes when normalized output fields change") {
    auto request = make_hash_sensitive_request();
    const auto first = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);
    MK_REQUIRE(first.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(first.replay_hash != 0U);

    request.placement_intents[0].coord.x = 3;
    const auto moved_placement = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);
    MK_REQUIRE(moved_placement.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(moved_placement.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.persistence_rows[0].observed_schema_version = 2U;
    const auto changed_persistence_status = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);
    MK_REQUIRE(changed_persistence_status.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(changed_persistence_status.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.destruction_intents[0].coord.x = 2;
    const auto changed_destruction = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);
    MK_REQUIRE(changed_destruction.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(changed_destruction.mutation_rows[1].status == RuntimeSandboxMutationStatus::blocked_missing_cell);
    MK_REQUIRE(changed_destruction.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.placement_intents[0].provided_costs[0].quantity = 5U;
    const auto changed_provided_cost = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);
    MK_REQUIRE(changed_provided_cost.status == RuntimeSandboxWorldStatus::ready);
    MK_REQUIRE(changed_provided_cost.mutation_rows[0].status == RuntimeSandboxMutationStatus::accepted);
    MK_REQUIRE(changed_provided_cost.replay_hash != first.replay_hash);
}

MK_TEST("runtime sandbox world diagnostics are totally ordered by stable public fields") {
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(mirakana::runtime::RuntimeSandboxWorldMutationRequest{
            .world_id = "sandbox.diagnostics",
            .world_tick = 1U,
            .chunk_rows = {make_chunk("chunk.0", "region.0", true, true, 1U)},
            .existing_cell_rows = {make_existing_cell("chunk.0", cell(99, 0, 0), "renderer", true, false, 7U)},
            .placement_intents = {},
            .destruction_intents = {},
            .construction_cost_rows = {},
            .persistence_rows = {},
            .tile_drop_rows = {},
            .tool_effectiveness_rows = {},
            .spawn_region_rows = {},
            .day_night_event_rows = {},
            .trigger_rows = {},
            .game_content_rule_ids = {},
            .row_budget = 512U,
            .seed = 0U,
        });

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::invalid_existing_cell) == 2U);
    MK_REQUIRE(plan.diagnostics[0].message < plan.diagnostics[1].message);
}

MK_TEST("runtime sandbox world generated rows exceeding row budget fail closed") {
    auto request = make_hash_sensitive_request();
    request.row_budget = 3U;

    const auto plan = mirakana::runtime::plan_runtime_sandbox_world_mutation(request);

    MK_REQUIRE(plan.status == RuntimeSandboxWorldStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSandboxDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.chunk_rows.empty());
    MK_REQUIRE(plan.existing_cell_rows.empty());
    MK_REQUIRE(plan.placement_intent_rows.empty());
    MK_REQUIRE(plan.destruction_intent_rows.empty());
    MK_REQUIRE(plan.construction_cost_rows.empty());
    MK_REQUIRE(plan.mutation_rows.empty());
    MK_REQUIRE(plan.persistence_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
