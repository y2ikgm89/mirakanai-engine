// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/gameplay_execution_loop_2d.hpp"

#include <cstdint>
#include <string_view>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeGameplaySchedulerSystemDesc
make_system(std::string_view system_id, std::int32_t order, std::uint32_t budget_us, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeGameplaySchedulerSystemDesc{
        .system_id = std::string{system_id},
        .order = order,
        .enabled = true,
        .budget_us = budget_us,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc make_command(std::string_view command_id,
                                                                                       std::uint64_t target_tick,
                                                                                       std::uint64_t payload_hash,
                                                                                       std::uint32_t source_index) {
    return mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc{
        .command_id = std::string{command_id},
        .target_tick = target_tick,
        .payload_hash = payload_hash,
        .source_index = source_index,
    };
}

[[nodiscard]] const mirakana::runtime::RuntimeGameplayExecutionLoop2DCounterRow*
find_counter(const mirakana::runtime::RuntimeGameplayExecutionLoop2DStepResult& result, std::string_view counter_id) {
    for (const auto& counter : result.counters) {
        if (counter.counter_id == counter_id) {
            return &counter;
        }
    }
    return nullptr;
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeGameplayExecutionLoop2DStepResult& result,
                                           mirakana::runtime::RuntimeGameplayExecutionLoop2DDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : result.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime 2d gameplay execution loop composes deterministic scheduler world and interaction rows") {
    const auto
        desc =
            mirakana::runtime::RuntimeGameplayExecutionLoop2DDesc{
                .loop_id = "arena_loop",
                .fixed_delta_us = 16'666U,
                .max_steps_per_frame = 1U,
                .max_command_rows_per_frame = 4U,
                .systems =
                    {
                        make_system("input_actions", 10, 100U, 1U),
                        make_system("world_entities", 20, 250U, 2U),
                        make_system("gameplay_interactions", 30, 300U, 3U),
                    },
                .world =
                    mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
                        .world_id = "arena_world",
                        .regions =
                            {
                                mirakana::runtime::RuntimeWorldRegionRow{
                                    .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "field"},
                                    .source_index = 1U,
                                },
                            },
                        .entities =
                            {
                                mirakana::runtime::RuntimeWorldEntityRow{
                                    .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "player"},
                                    .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "field"},
                                    .archetype_id = "hero",
                                    .active = true,
                                    .generation = 1U,
                                    .source_index = 1U,
                                },
                                mirakana::runtime::RuntimeWorldEntityRow{
                                    .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "enemy"},
                                    .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "field"},
                                    .archetype_id = "slime",
                                    .active = true,
                                    .generation = 1U,
                                    .source_index = 2U,
                                },
                            },
                    },
                .interaction_state =
                    mirakana::runtime::RuntimeGameplayInteractionState{
                        .session_state = mirakana::runtime::RuntimeGameplaySessionState::running,
                        .entities =
                            {
                                mirakana::runtime::RuntimeGameplayEntityState{
                                    .id = "player",
                                    .health = 10,
                                    .max_health = 10,
                                    .active = true,
                                },
                                mirakana::runtime::RuntimeGameplayEntityState{
                                    .id = "enemy",
                                    .health = 3,
                                    .max_health = 3,
                                    .active = true,
                                },
                            },
                    },
            };
    const auto frame = mirakana::runtime::RuntimeGameplayExecutionLoop2DFrameInput{
        .frame_index = 7U,
        .next_tick = 42U,
        .accumulated_time_us = 16'666U,
        .observed_delta_us = 16'666U,
        .input_commands = {make_command("cmd.move.right", 42U, 0x1001U, 1U)},
        .interaction_events =
            {
                mirakana::runtime::RuntimeGameplayInteractionEvent{
                    .id = "hit.enemy",
                    .kind = mirakana::runtime::RuntimeGameplayInteractionKind::damage,
                    .source_entity_id = "player",
                    .target_entity_id = "enemy",
                    .amount = 1,
                },
            },
    };

    const auto first = mirakana::runtime::execute_runtime_gameplay_loop_2d_step(desc, frame);
    const auto second = mirakana::runtime::execute_runtime_gameplay_loop_2d_step(desc, frame);

    MK_REQUIRE(first.status == mirakana::runtime::RuntimeGameplayExecutionLoop2DStatus::ready);
    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.scheduler.steps.size() == 1U);
    MK_REQUIRE(first.scheduler.steps[0].tick == 42U);
    MK_REQUIRE(first.scheduler.steps[0].command_rows.size() == 1U);
    MK_REQUIRE(first.scheduler.steps[0].system_rows.size() == 3U);
    MK_REQUIRE(first.world.entity_rows.size() == 2U);
    MK_REQUIRE(first.world.entity_rows[0].entity_id.value == "enemy");
    MK_REQUIRE(first.world.entity_rows[1].entity_id.value == "player");
    MK_REQUIRE(first.interactions.rows.size() == 1U);
    MK_REQUIRE(first.interactions.state.entities[1].id == "enemy");
    MK_REQUIRE(first.interactions.state.entities[1].health == 2);
    MK_REQUIRE(first.step_count == 1U);
    MK_REQUIRE(first.replay_hash != 0U);
    MK_REQUIRE(first.replay_hash == second.replay_hash);
    MK_REQUIRE(first.side_effects.empty());

    const auto* status = find_counter(first, "2d_gameplay_execution_loop_status");
    const auto* steps = find_counter(first, "2d_gameplay_execution_loop_steps");
    const auto* diagnostics = find_counter(first, "2d_gameplay_execution_loop_diagnostics");
    const auto* side_effects = find_counter(first, "2d_gameplay_execution_loop_side_effects");
    MK_REQUIRE(status != nullptr);
    MK_REQUIRE(status->value == 1U);
    MK_REQUIRE(steps != nullptr);
    MK_REQUIRE(steps->value == 1U);
    MK_REQUIRE(diagnostics != nullptr);
    MK_REQUIRE(diagnostics->value == 0U);
    MK_REQUIRE(side_effects != nullptr);
    MK_REQUIRE(side_effects->value == 0U);
}

MK_TEST("runtime 2d gameplay execution loop rejects interaction entities missing from world bindings") {
    const auto desc = mirakana::runtime::RuntimeGameplayExecutionLoop2DDesc{
        .loop_id = "arena_loop",
        .fixed_delta_us = 16'666U,
        .max_steps_per_frame = 1U,
        .max_command_rows_per_frame = 4U,
        .systems = {make_system("input_actions", 10, 100U, 1U)},
        .world =
            mirakana::runtime::RuntimeWorldEntityLifecycleRequest{
                .world_id = "arena_world",
                .regions =
                    {
                        mirakana::runtime::RuntimeWorldRegionRow{
                            .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "field"},
                            .source_index = 1U,
                        },
                    },
                .entities =
                    {
                        mirakana::runtime::RuntimeWorldEntityRow{
                            .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "player"},
                            .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "field"},
                            .archetype_id = "hero",
                            .active = true,
                            .generation = 1U,
                            .source_index = 1U,
                        },
                    },
            },
        .interaction_state =
            mirakana::runtime::RuntimeGameplayInteractionState{
                .session_state = mirakana::runtime::RuntimeGameplaySessionState::running,
                .entities =
                    {
                        mirakana::runtime::RuntimeGameplayEntityState{
                            .id = "player",
                            .health = 10,
                            .max_health = 10,
                            .active = true,
                        },
                        mirakana::runtime::RuntimeGameplayEntityState{
                            .id = "ghost.enemy",
                            .health = 2,
                            .max_health = 2,
                            .active = true,
                        },
                    },
            },
    };
    const auto frame = mirakana::runtime::RuntimeGameplayExecutionLoop2DFrameInput{
        .frame_index = 8U,
        .next_tick = 43U,
        .accumulated_time_us = 16'666U,
        .observed_delta_us = 16'666U,
        .input_commands = {make_command("cmd.move.right", 43U, 0x1002U, 1U)},
    };

    const auto result = mirakana::runtime::execute_runtime_gameplay_loop_2d_step(desc, frame);

    MK_REQUIRE(result.status == mirakana::runtime::RuntimeGameplayExecutionLoop2DStatus::invalid_request);
    MK_REQUIRE(!result.succeeded());
    MK_REQUIRE(diagnostic_count(
                   result, mirakana::runtime::RuntimeGameplayExecutionLoop2DDiagnosticCode::missing_entity_binding) ==
               1U);
    const auto* status = find_counter(result, "2d_gameplay_execution_loop_status");
    const auto* diagnostics = find_counter(result, "2d_gameplay_execution_loop_diagnostics");
    MK_REQUIRE(status != nullptr);
    MK_REQUIRE(status->value == 0U);
    MK_REQUIRE(diagnostics != nullptr);
    MK_REQUIRE(diagnostics->value == 1U);
    MK_REQUIRE(result.side_effects.empty());
}

int main() {
    return mirakana::test::run_all();
}
