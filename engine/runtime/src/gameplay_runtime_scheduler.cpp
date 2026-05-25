// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/gameplay_runtime_scheduler.hpp"

#include <algorithm>
#include <limits>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

void add_diagnostic(RuntimeGameplaySchedulerPlan& plan, RuntimeGameplaySchedulerDiagnosticCode code,
                    std::string scheduler_id, std::string system_id, std::string command_id, std::uint64_t target_tick,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeGameplaySchedulerDiagnostic{
        .code = code,
        .scheduler_id = std::move(scheduler_id),
        .system_id = std::move(system_id),
        .command_id = std::move(command_id),
        .target_tick = target_tick,
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] std::uint64_t mix_hash(std::uint64_t hash, std::uint64_t value) noexcept {
    constexpr auto kPrime = std::uint64_t{1099511628211ULL};
    hash ^= value;
    hash *= kPrime;
    return hash;
}

[[nodiscard]] std::uint64_t hash_string(std::string_view value) noexcept {
    auto hash = std::uint64_t{14695981039346656037ULL};
    for (const auto ch : value) {
        hash = mix_hash(hash, static_cast<unsigned char>(ch));
    }
    return hash;
}

void sort_commands(std::vector<RuntimeGameplaySchedulerInputCommandDesc>& commands) {
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

void sort_systems(std::vector<RuntimeGameplaySchedulerSystemDesc>& systems) {
    std::ranges::sort(systems, [](const auto& lhs, const auto& rhs) {
        if (lhs.order != rhs.order) {
            return lhs.order < rhs.order;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.system_id < rhs.system_id;
    });
}

void sort_diagnostics(RuntimeGameplaySchedulerPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.target_tick != rhs.target_tick) {
            return lhs.target_tick < rhs.target_tick;
        }
        if (lhs.system_id != rhs.system_id) {
            return lhs.system_id < rhs.system_id;
        }
        if (lhs.command_id != rhs.command_id) {
            return lhs.command_id < rhs.command_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] std::size_t selected_step_count(const RuntimeGameplaySchedulerRequest& request,
                                              std::size_t available_step_count) {
    if (request.mode == RuntimeGameplaySchedulerMode::paused) {
        return 0U;
    }
    auto max_steps = static_cast<std::size_t>(request.max_steps_per_frame);
    if (request.mode == RuntimeGameplaySchedulerMode::single_step) {
        max_steps = std::min<std::size_t>(max_steps, 1U);
    }
    return std::min(available_step_count, max_steps);
}

void validate_request(RuntimeGameplaySchedulerPlan& plan, const RuntimeGameplaySchedulerRequest& request,
                      std::uint64_t planned_window_end_exclusive, bool tick_window_overflows) {
    if (!is_valid_id(request.scheduler_id)) {
        add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::missing_scheduler_id, request.scheduler_id, {}, {},
                       request.next_tick, "runtime gameplay scheduler id must be non-empty and path-safe", 0U);
    }

    if (request.tick_delta_us == 0U) {
        add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::invalid_tick_delta, request.scheduler_id, {}, {},
                       request.next_tick, "runtime gameplay scheduler tick delta must be non-zero", 0U);
    }

    if (request.max_steps_per_frame == 0U) {
        add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::invalid_max_steps_per_frame, request.scheduler_id,
                       {}, {}, request.next_tick, "runtime gameplay scheduler max steps per frame must be non-zero",
                       0U);
    }

    if (tick_window_overflows) {
        add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::tick_window_overflow, request.scheduler_id, {}, {},
                       request.next_tick,
                       "runtime gameplay scheduler tick window must not overflow uint64 tick coordinates", 0U);
    }

    std::vector<std::string> system_ids;
    system_ids.reserve(request.systems.size());
    for (const auto& system : request.systems) {
        if (!is_valid_id(system.system_id)) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::missing_system_id, request.scheduler_id,
                           system.system_id, {}, request.next_tick,
                           "runtime gameplay system id must be non-empty and path-safe", system.source_index);
        } else if (std::ranges::find(system_ids, system.system_id) != system_ids.end()) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::duplicate_system_id, request.scheduler_id,
                           system.system_id, {}, request.next_tick,
                           "runtime gameplay system ids must be unique in a planning request", system.source_index);
        } else {
            system_ids.push_back(system.system_id);
        }
    }

    std::vector<std::string> command_ids;
    command_ids.reserve(request.input_commands.size());
    for (const auto& command : request.input_commands) {
        if (!is_valid_id(command.command_id)) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::missing_command_id, request.scheduler_id, {},
                           command.command_id, command.target_tick,
                           "runtime gameplay scheduler command id must be non-empty and path-safe",
                           command.source_index);
        } else if (std::ranges::find(command_ids, command.command_id) != command_ids.end()) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::duplicate_command_id, request.scheduler_id, {},
                           command.command_id, command.target_tick,
                           "runtime gameplay scheduler command ids must be unique in a planning request",
                           command.source_index);
        } else {
            command_ids.push_back(command.command_id);
        }

        if (command.target_tick < request.next_tick) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::command_before_next_tick, request.scheduler_id,
                           {}, command.command_id, command.target_tick,
                           "runtime gameplay scheduler command targets a tick before next_tick", command.source_index);
        } else if (command.target_tick >= planned_window_end_exclusive) {
            add_diagnostic(plan, RuntimeGameplaySchedulerDiagnosticCode::command_after_planned_window,
                           request.scheduler_id, {}, command.command_id, command.target_tick,
                           "runtime gameplay scheduler command targets a tick outside the planned step window",
                           command.source_index);
        }
    }
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeGameplaySchedulerRequest& request,
                                                const RuntimeGameplaySchedulerPlan& plan) {
    auto hash = std::uint64_t{14695981039346656037ULL};
    hash = mix_hash(hash, hash_string(request.scheduler_id));
    hash = mix_hash(hash, request.next_tick);
    hash = mix_hash(hash, request.tick_delta_us);
    hash = mix_hash(hash, static_cast<std::uint64_t>(request.mode));
    for (const auto& step : plan.steps) {
        hash = mix_hash(hash, step.tick);
        for (const auto& command : step.command_rows) {
            hash = mix_hash(hash, hash_string(command.command_id));
            hash = mix_hash(hash, command.target_tick);
            hash = mix_hash(hash, command.payload_hash);
            hash = mix_hash(hash, command.source_index);
        }
        for (const auto& system : step.system_rows) {
            hash = mix_hash(hash, hash_string(system.system_id));
            hash = mix_hash(hash, static_cast<std::uint64_t>(system.order));
            hash = mix_hash(hash, system.budget_us);
            hash = mix_hash(hash, system.source_index);
        }
    }
    return hash == 0U ? 1U : hash;
}

} // namespace

bool RuntimeGameplaySchedulerPlan::succeeded() const noexcept {
    return status == RuntimeGameplaySchedulerStatus::ready || status == RuntimeGameplaySchedulerStatus::no_steps ||
           status == RuntimeGameplaySchedulerStatus::budget_limited || status == RuntimeGameplaySchedulerStatus::paused;
}

RuntimeGameplaySchedulerPlan plan_runtime_gameplay_schedule(const RuntimeGameplaySchedulerRequest& request) {
    RuntimeGameplaySchedulerPlan plan;

    if (request.tick_delta_us > 0U) {
        plan.available_step_count = static_cast<std::size_t>(request.accumulated_time_us / request.tick_delta_us);
    }
    if (request.max_steps_per_frame > 0U) {
        plan.planned_step_count = selected_step_count(request, plan.available_step_count);
    }

    const auto fallback_window_steps = request.max_steps_per_frame > 0U ? request.max_steps_per_frame : 1U;
    const auto effective_window_steps = std::max<std::size_t>(plan.planned_step_count, fallback_window_steps);
    const auto remaining_tick_capacity = std::numeric_limits<std::uint64_t>::max() - request.next_tick;
    const auto tick_window_overflows = effective_window_steps > static_cast<std::size_t>(remaining_tick_capacity);
    const auto planned_window_end_exclusive =
        tick_window_overflows ? std::numeric_limits<std::uint64_t>::max()
                              : request.next_tick + static_cast<std::uint64_t>(effective_window_steps);
    validate_request(plan, request, planned_window_end_exclusive, tick_window_overflows);
    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeGameplaySchedulerStatus::invalid_request;
        return plan;
    }

    std::vector<RuntimeGameplaySchedulerSystemDesc> enabled_systems;
    enabled_systems.reserve(request.systems.size());
    for (const auto& system : request.systems) {
        if (system.enabled) {
            enabled_systems.push_back(system);
        }
    }
    sort_systems(enabled_systems);

    auto sorted_commands = request.input_commands;
    sort_commands(sorted_commands);

    for (std::size_t step_index = 0U; step_index < plan.planned_step_count; ++step_index) {
        const auto tick = request.next_tick + static_cast<std::uint64_t>(step_index);
        RuntimeGameplaySchedulerStepRow step{
            .scheduler_id = request.scheduler_id,
            .tick = tick,
            .fixed_delta_us = request.tick_delta_us,
        };

        for (const auto& command : sorted_commands) {
            if (command.target_tick == tick) {
                step.command_rows.push_back(RuntimeGameplaySchedulerCommandRow{
                    .scheduler_id = request.scheduler_id,
                    .command_id = command.command_id,
                    .target_tick = command.target_tick,
                    .payload_hash = command.payload_hash,
                    .source_index = command.source_index,
                });
            }
        }

        for (const auto& system : enabled_systems) {
            step.system_rows.push_back(RuntimeGameplaySchedulerSystemRow{
                .scheduler_id = request.scheduler_id,
                .system_id = system.system_id,
                .tick = tick,
                .order = system.order,
                .budget_us = system.budget_us,
                .source_index = system.source_index,
            });
            step.total_budget_us += system.budget_us;
        }

        plan.total_command_rows += step.command_rows.size();
        plan.total_system_rows += step.system_rows.size();
        plan.steps.push_back(std::move(step));
    }

    plan.consumed_time_us = static_cast<std::uint64_t>(plan.planned_step_count) * request.tick_delta_us;
    plan.remaining_time_us = request.accumulated_time_us - plan.consumed_time_us;

    if (request.mode == RuntimeGameplaySchedulerMode::paused) {
        plan.status = RuntimeGameplaySchedulerStatus::paused;
    } else if (plan.planned_step_count == 0U) {
        plan.status = RuntimeGameplaySchedulerStatus::no_steps;
    } else if (plan.available_step_count > plan.planned_step_count) {
        plan.status = RuntimeGameplaySchedulerStatus::budget_limited;
    } else {
        plan.status = RuntimeGameplaySchedulerStatus::ready;
    }

    plan.replay_hash = compute_replay_hash(request, plan);
    return plan;
}

} // namespace mirakana::runtime
