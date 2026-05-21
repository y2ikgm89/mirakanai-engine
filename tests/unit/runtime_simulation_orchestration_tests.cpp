// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/simulation_orchestration.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeSimulationInputCommandDesc
make_command(std::string command_id, std::string action_id, std::uint64_t target_tick, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationInputCommandDesc{
        .command_id = std::move(command_id),
        .action_id = std::move(action_id),
        .target_tick = target_tick,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSimulationOrchestrationPlan& plan,
                                           mirakana::runtime::RuntimeSimulationOrchestrationDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime simulation orchestration plans deterministic fixed step and command rows") {
    using Status = mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus;

    const auto request = mirakana::runtime::RuntimeSimulationOrchestrationRequest{
        .simulation_id = "arena_loop",
        .next_tick = 100U,
        .fixed_tick_rate_hz = 60U,
        .accumulated_time_us = 50'000U,
        .max_steps_per_frame = 4U,
        .input_commands =
            {
                make_command("cmd.move", "move_right", 101U, 4U),
                make_command("cmd.jump", "jump", 100U, 2U),
                make_command("cmd.fire", "fire", 101U, 3U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_simulation_orchestration(request);

    MK_REQUIRE(plan.status == Status::planned);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.fixed_delta_us == 16'666U);
    MK_REQUIRE(plan.planned_step_count == 3U);
    MK_REQUIRE(plan.consumed_time_us == 49'998U);
    MK_REQUIRE(plan.remaining_time_us == 2U);
    MK_REQUIRE(plan.steps.size() == 3U);
    MK_REQUIRE(plan.steps[0].simulation_id == "arena_loop");
    MK_REQUIRE(plan.steps[0].tick == 100U);
    MK_REQUIRE(plan.steps[0].command_count == 1U);
    MK_REQUIRE(plan.steps[1].tick == 101U);
    MK_REQUIRE(plan.steps[1].command_count == 2U);
    MK_REQUIRE(plan.steps[2].tick == 102U);
    MK_REQUIRE(plan.commands.size() == 3U);
    MK_REQUIRE(plan.commands[0].target_tick == 100U);
    MK_REQUIRE(plan.commands[0].command_id == "cmd.jump");
    MK_REQUIRE(plan.commands[1].target_tick == 101U);
    MK_REQUIRE(plan.commands[1].command_id == "cmd.fire");
    MK_REQUIRE(plan.commands[2].target_tick == 101U);
    MK_REQUIRE(plan.commands[2].command_id == "cmd.move");
}

MK_TEST("runtime simulation orchestration reports budget limited fixed step rows") {
    using Status = mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus;

    const auto request = mirakana::runtime::RuntimeSimulationOrchestrationRequest{
        .simulation_id = "slow_frame",
        .next_tick = 7U,
        .fixed_tick_rate_hz = 30U,
        .accumulated_time_us = 200'000U,
        .max_steps_per_frame = 2U,
    };

    const auto plan = mirakana::runtime::plan_runtime_simulation_orchestration(request);

    MK_REQUIRE(plan.status == Status::budget_limited);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.available_step_count == 6U);
    MK_REQUIRE(plan.planned_step_count == 2U);
    MK_REQUIRE(plan.steps.size() == 2U);
    MK_REQUIRE(plan.steps[0].tick == 7U);
    MK_REQUIRE(plan.steps[1].tick == 8U);
    MK_REQUIRE(plan.remaining_time_us == 133'334U);
}

MK_TEST("runtime simulation orchestration rejects invalid replay unsafe command rows") {
    using Code = mirakana::runtime::RuntimeSimulationOrchestrationDiagnosticCode;
    using Status = mirakana::runtime::RuntimeSimulationOrchestrationPlanStatus;

    const auto request = mirakana::runtime::RuntimeSimulationOrchestrationRequest{
        .simulation_id = "",
        .next_tick = 10U,
        .fixed_tick_rate_hz = 0U,
        .accumulated_time_us = 33'333U,
        .max_steps_per_frame = 0U,
        .input_commands =
            {
                make_command("", "move", 10U, 1U),
                make_command("cmd.dupe", "", 10U, 2U),
                make_command("cmd.dupe", "jump", 10U, 3U),
                make_command("cmd.past", "dash", 9U, 4U),
                make_command("cmd.future", "fire", 12U, 5U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_simulation_orchestration(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.steps.empty());
    MK_REQUIRE(plan.commands.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::missing_simulation_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_fixed_tick_rate) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_max_steps_per_frame) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_command_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_action_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_command_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::command_before_next_tick) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::command_after_planned_window) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
