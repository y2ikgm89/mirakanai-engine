// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/gameplay_runtime_scheduler.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeGameplaySchedulerSystemDesc
make_system(std::string system_id, std::int32_t order, std::uint32_t budget_us, std::uint32_t source_index,
            bool enabled = true) {
    return mirakana::runtime::RuntimeGameplaySchedulerSystemDesc{
        .system_id = std::move(system_id),
        .order = order,
        .enabled = enabled,
        .budget_us = budget_us,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc make_command(std::string command_id,
                                                                                       std::uint64_t target_tick,
                                                                                       std::uint64_t payload_hash,
                                                                                       std::uint32_t source_index) {
    return mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc{
        .command_id = std::move(command_id),
        .target_tick = target_tick,
        .payload_hash = payload_hash,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeGameplaySchedulerPlan& plan,
                                           mirakana::runtime::RuntimeGameplaySchedulerDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

} // namespace

MK_TEST("runtime gameplay scheduler plans fixed ticks in stable system order") {
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;

    const auto request = mirakana::runtime::RuntimeGameplaySchedulerRequest{
        .scheduler_id = "arena_gameplay",
        .next_tick = 42U,
        .tick_delta_us = 16'666U,
        .accumulated_time_us = 49'998U,
        .max_steps_per_frame = 2U,
        .mode = Mode::run,
        .systems =
            {
                make_system("physics", 20, 400U, 2U),
                make_system("input", 10, 100U, 1U),
                make_system("disabled_debug", 5, 50U, 4U, false),
                make_system("ai", 30, 300U, 3U),
            },
        .input_commands =
            {
                make_command("cmd.interact", 43U, 1002U, 2U),
                make_command("cmd.move", 42U, 1001U, 1U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_gameplay_schedule(request);

    MK_REQUIRE(plan.status == Status::budget_limited);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.available_step_count == 3U);
    MK_REQUIRE(plan.planned_step_count == 2U);
    MK_REQUIRE(plan.consumed_time_us == 33'332U);
    MK_REQUIRE(plan.remaining_time_us == 16'666U);
    MK_REQUIRE(plan.total_system_rows == 6U);
    MK_REQUIRE(plan.total_command_rows == 2U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(plan.steps.size() == 2U);
    MK_REQUIRE(plan.steps[0].tick == 42U);
    MK_REQUIRE(plan.steps[0].command_rows.size() == 1U);
    MK_REQUIRE(plan.steps[0].command_rows[0].command_id == "cmd.move");
    MK_REQUIRE(plan.steps[0].system_rows.size() == 3U);
    MK_REQUIRE(plan.steps[0].system_rows[0].system_id == "input");
    MK_REQUIRE(plan.steps[0].system_rows[1].system_id == "physics");
    MK_REQUIRE(plan.steps[0].system_rows[2].system_id == "ai");
    MK_REQUIRE(plan.steps[0].total_budget_us == 800U);
    MK_REQUIRE(plan.steps[1].tick == 43U);
    MK_REQUIRE(plan.steps[1].command_rows.size() == 1U);
    MK_REQUIRE(plan.steps[1].command_rows[0].command_id == "cmd.interact");
}

MK_TEST("runtime gameplay scheduler supports pause and single-step policies") {
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;

    const auto paused =
        mirakana::runtime::plan_runtime_gameplay_schedule(mirakana::runtime::RuntimeGameplaySchedulerRequest{
            .scheduler_id = "paused_loop",
            .next_tick = 10U,
            .tick_delta_us = 20'000U,
            .accumulated_time_us = 100'000U,
            .max_steps_per_frame = 4U,
            .mode = Mode::paused,
            .systems = {make_system("input", 10, 100U, 1U)},
        });

    MK_REQUIRE(paused.status == Status::paused);
    MK_REQUIRE(paused.succeeded());
    MK_REQUIRE(paused.steps.empty());
    MK_REQUIRE(paused.available_step_count == 5U);
    MK_REQUIRE(paused.planned_step_count == 0U);
    MK_REQUIRE(paused.remaining_time_us == 100'000U);

    const auto single_step =
        mirakana::runtime::plan_runtime_gameplay_schedule(mirakana::runtime::RuntimeGameplaySchedulerRequest{
            .scheduler_id = "step_loop",
            .next_tick = 20U,
            .tick_delta_us = 10'000U,
            .accumulated_time_us = 40'000U,
            .max_steps_per_frame = 3U,
            .mode = Mode::single_step,
            .systems = {make_system("input", 10, 100U, 1U)},
            .input_commands = {make_command("cmd.step", 20U, 777U, 1U)},
        });

    MK_REQUIRE(single_step.status == Status::budget_limited);
    MK_REQUIRE(single_step.steps.size() == 1U);
    MK_REQUIRE(single_step.available_step_count == 4U);
    MK_REQUIRE(single_step.planned_step_count == 1U);
    MK_REQUIRE(single_step.remaining_time_us == 30'000U);
    MK_REQUIRE(single_step.steps[0].command_rows[0].command_id == "cmd.step");
}

MK_TEST("runtime gameplay scheduler rejects replay-unsafe requests") {
    using Code = mirakana::runtime::RuntimeGameplaySchedulerDiagnosticCode;
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;

    const auto request = mirakana::runtime::RuntimeGameplaySchedulerRequest{
        .scheduler_id = "",
        .next_tick = 100U,
        .tick_delta_us = 0U,
        .accumulated_time_us = 50'000U,
        .max_steps_per_frame = 0U,
        .mode = Mode::run,
        .systems =
            {
                make_system("", 10, 100U, 1U),
                make_system("physics", 20, 100U, 2U),
                make_system("physics", 30, 100U, 3U),
            },
        .input_commands =
            {
                make_command("", 100U, 1U, 1U),
                make_command("cmd.duplicate", 100U, 2U, 2U),
                make_command("cmd.duplicate", 100U, 3U, 3U),
                make_command("cmd.past", 99U, 4U, 4U),
                make_command("cmd.future", 101U, 5U, 5U),
            },
    };

    const auto plan = mirakana::runtime::plan_runtime_gameplay_schedule(request);

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.steps.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_scheduler_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_tick_delta) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::invalid_max_steps_per_frame) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_system_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_system_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::missing_command_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::duplicate_command_id) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::command_before_next_tick) == 1U);
    MK_REQUIRE(diagnostic_count(plan, Code::command_after_planned_window) == 1U);
}

MK_TEST("runtime gameplay scheduler rejects overflowing tick windows") {
    using Code = mirakana::runtime::RuntimeGameplaySchedulerDiagnosticCode;
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;

    const auto plan =
        mirakana::runtime::plan_runtime_gameplay_schedule(mirakana::runtime::RuntimeGameplaySchedulerRequest{
            .scheduler_id = "overflow_loop",
            .next_tick = std::numeric_limits<std::uint64_t>::max(),
            .tick_delta_us = 1U,
            .accumulated_time_us = 2U,
            .max_steps_per_frame = 2U,
            .mode = Mode::run,
            .systems = {make_system("input", 10, 100U, 1U)},
        });

    MK_REQUIRE(plan.status == Status::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.steps.empty());
    MK_REQUIRE(diagnostic_count(plan, Code::tick_window_overflow) == 1U);
}

int main() {
    return mirakana::test::run_all();
}
