// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/simulation_orchestration.hpp"

#include <algorithm>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

constexpr std::uint64_t kMicrosecondsPerSecond = 1'000'000U;

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

void add_diagnostic(RuntimeSimulationOrchestrationPlan& plan, RuntimeSimulationOrchestrationDiagnosticCode code,
                    std::string simulation_id, std::string command_id, std::string action_id, std::uint64_t target_tick,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeSimulationOrchestrationDiagnostic{
        .code = code,
        .simulation_id = std::move(simulation_id),
        .command_id = std::move(command_id),
        .action_id = std::move(action_id),
        .target_tick = target_tick,
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] std::size_t count_commands_for_tick(const std::vector<RuntimeSimulationInputCommandRow>& commands,
                                                  std::uint64_t tick) {
    return static_cast<std::size_t>(std::ranges::count_if(
        commands, [tick](const RuntimeSimulationInputCommandRow& command) { return command.target_tick == tick; }));
}

void sort_commands(std::vector<RuntimeSimulationInputCommandRow>& commands) {
    std::ranges::sort(commands, [](const auto& lhs, const auto& rhs) {
        if (lhs.target_tick != rhs.target_tick) {
            return lhs.target_tick < rhs.target_tick;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.command_id < rhs.command_id;
    });
}

void sort_diagnostics(RuntimeSimulationOrchestrationPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.target_tick != rhs.target_tick) {
            return lhs.target_tick < rhs.target_tick;
        }
        if (lhs.command_id != rhs.command_id) {
            return lhs.command_id < rhs.command_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void validate_request(RuntimeSimulationOrchestrationPlan& plan, const RuntimeSimulationOrchestrationRequest& request,
                      std::uint64_t planned_window_end_exclusive) {
    if (!is_valid_id(request.simulation_id)) {
        add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::missing_simulation_id, request.simulation_id,
                       {}, {}, request.next_tick, "runtime simulation id must be non-empty and path-safe", 0U);
    }

    if (request.fixed_tick_rate_hz == 0U || request.fixed_tick_rate_hz > kMicrosecondsPerSecond) {
        add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::invalid_fixed_tick_rate,
                       request.simulation_id, {}, {}, request.next_tick,
                       "runtime simulation fixed tick rate must be between 1 and 1000000 Hz", 0U);
    }

    if (request.max_steps_per_frame == 0U) {
        add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::invalid_max_steps_per_frame,
                       request.simulation_id, {}, {}, request.next_tick,
                       "runtime simulation max steps per frame must be non-zero", 0U);
    }

    std::vector<std::string> command_ids;
    command_ids.reserve(request.input_commands.size());
    for (const auto& command : request.input_commands) {
        if (!is_valid_id(command.command_id)) {
            add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::missing_command_id,
                           request.simulation_id, command.command_id, command.action_id, command.target_tick,
                           "runtime simulation input command id must be non-empty and path-safe", command.source_index);
        } else if (std::ranges::find(command_ids, command.command_id) != command_ids.end()) {
            add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::duplicate_command_id,
                           request.simulation_id, command.command_id, command.action_id, command.target_tick,
                           "runtime simulation input command ids must be unique in a planning request",
                           command.source_index);
        } else {
            command_ids.push_back(command.command_id);
        }

        if (!is_valid_id(command.action_id)) {
            add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::missing_action_id, request.simulation_id,
                           command.command_id, command.action_id, command.target_tick,
                           "runtime simulation input command action id must be non-empty and path-safe",
                           command.source_index);
        }

        if (command.target_tick < request.next_tick) {
            add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::command_before_next_tick,
                           request.simulation_id, command.command_id, command.action_id, command.target_tick,
                           "runtime simulation input command targets a tick before next_tick", command.source_index);
        } else if (command.target_tick >= planned_window_end_exclusive) {
            add_diagnostic(plan, RuntimeSimulationOrchestrationDiagnosticCode::command_after_planned_window,
                           request.simulation_id, command.command_id, command.action_id, command.target_tick,
                           "runtime simulation input command targets a tick outside the planned step window",
                           command.source_index);
        }
    }
}

} // namespace

bool RuntimeSimulationOrchestrationPlan::succeeded() const noexcept {
    return status == RuntimeSimulationOrchestrationPlanStatus::planned ||
           status == RuntimeSimulationOrchestrationPlanStatus::no_steps ||
           status == RuntimeSimulationOrchestrationPlanStatus::budget_limited;
}

RuntimeSimulationOrchestrationPlan
plan_runtime_simulation_orchestration(const RuntimeSimulationOrchestrationRequest& request) {
    RuntimeSimulationOrchestrationPlan plan;

    if (request.fixed_tick_rate_hz > 0U && request.fixed_tick_rate_hz <= kMicrosecondsPerSecond) {
        plan.fixed_delta_us = kMicrosecondsPerSecond / request.fixed_tick_rate_hz;
    }
    if (plan.fixed_delta_us > 0U) {
        plan.available_step_count = static_cast<std::size_t>(request.accumulated_time_us / plan.fixed_delta_us);
    }
    if (request.max_steps_per_frame > 0U) {
        plan.planned_step_count = std::min<std::size_t>(plan.available_step_count, request.max_steps_per_frame);
    }

    const auto fallback_window_steps = request.max_steps_per_frame > 0U ? request.max_steps_per_frame : 1U;
    const auto planned_window_end_exclusive =
        request.next_tick +
        static_cast<std::uint64_t>(std::max<std::size_t>(plan.planned_step_count, fallback_window_steps));
    validate_request(plan, request, planned_window_end_exclusive);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeSimulationOrchestrationPlanStatus::invalid_request;
        return plan;
    }

    for (const auto& command : request.input_commands) {
        plan.commands.push_back(RuntimeSimulationInputCommandRow{
            .simulation_id = request.simulation_id,
            .command_id = command.command_id,
            .action_id = command.action_id,
            .target_tick = command.target_tick,
            .source_index = command.source_index,
        });
    }
    sort_commands(plan.commands);

    for (std::size_t index = 0U; index < plan.planned_step_count; ++index) {
        const auto tick = request.next_tick + static_cast<std::uint64_t>(index);
        plan.steps.push_back(RuntimeSimulationStepRow{
            .simulation_id = request.simulation_id,
            .tick = tick,
            .fixed_delta_us = plan.fixed_delta_us,
            .command_count = count_commands_for_tick(plan.commands, tick),
        });
    }

    plan.consumed_time_us = static_cast<std::uint64_t>(plan.planned_step_count) * plan.fixed_delta_us;
    plan.remaining_time_us = request.accumulated_time_us - plan.consumed_time_us;

    if (plan.planned_step_count == 0U) {
        plan.status = RuntimeSimulationOrchestrationPlanStatus::no_steps;
    } else if (plan.available_step_count > plan.planned_step_count) {
        plan.status = RuntimeSimulationOrchestrationPlanStatus::budget_limited;
    } else {
        plan.status = RuntimeSimulationOrchestrationPlanStatus::planned;
    }
    return plan;
}

} // namespace mirakana::runtime
