// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeGameplaySchedulerMode : std::uint8_t {
    run = 0,
    paused,
    single_step,
};

enum class RuntimeGameplaySchedulerStatus : std::uint8_t {
    ready = 0,
    no_steps,
    budget_limited,
    paused,
    invalid_request,
};

enum class RuntimeGameplaySchedulerDiagnosticCode : std::uint8_t {
    missing_scheduler_id = 0,
    invalid_tick_delta,
    invalid_max_steps_per_frame,
    missing_system_id,
    duplicate_system_id,
    missing_command_id,
    duplicate_command_id,
    command_before_next_tick,
    command_after_planned_window,
    tick_window_overflow,
};

struct RuntimeGameplaySchedulerSystemDesc {
    std::string system_id;
    std::int32_t order{0};
    bool enabled{true};
    std::uint32_t budget_us{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeGameplaySchedulerInputCommandDesc {
    std::string command_id;
    std::uint64_t target_tick{0U};
    std::uint64_t payload_hash{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeGameplaySchedulerRequest {
    std::string scheduler_id;
    std::uint64_t next_tick{0U};
    std::uint64_t tick_delta_us{0U};
    std::uint64_t accumulated_time_us{0U};
    std::uint32_t max_steps_per_frame{0U};
    RuntimeGameplaySchedulerMode mode{RuntimeGameplaySchedulerMode::run};
    std::vector<RuntimeGameplaySchedulerSystemDesc> systems;
    std::vector<RuntimeGameplaySchedulerInputCommandDesc> input_commands;
};

struct RuntimeGameplaySchedulerDiagnostic {
    RuntimeGameplaySchedulerDiagnosticCode code{RuntimeGameplaySchedulerDiagnosticCode::missing_scheduler_id};
    std::string scheduler_id;
    std::string system_id;
    std::string command_id;
    std::uint64_t target_tick{0U};
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeGameplaySchedulerCommandRow {
    std::string scheduler_id;
    std::string command_id;
    std::uint64_t target_tick{0U};
    std::uint64_t payload_hash{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeGameplaySchedulerSystemRow {
    std::string scheduler_id;
    std::string system_id;
    std::uint64_t tick{0U};
    std::int32_t order{0};
    std::uint32_t budget_us{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeGameplaySchedulerStepRow {
    std::string scheduler_id;
    std::uint64_t tick{0U};
    std::uint64_t fixed_delta_us{0U};
    std::vector<RuntimeGameplaySchedulerCommandRow> command_rows;
    std::vector<RuntimeGameplaySchedulerSystemRow> system_rows;
    std::uint64_t total_budget_us{0U};
};

struct RuntimeGameplaySchedulerPlan {
    RuntimeGameplaySchedulerStatus status{RuntimeGameplaySchedulerStatus::invalid_request};
    std::vector<RuntimeGameplaySchedulerDiagnostic> diagnostics;
    std::vector<RuntimeGameplaySchedulerStepRow> steps;
    std::size_t available_step_count{0U};
    std::size_t planned_step_count{0U};
    std::size_t total_system_rows{0U};
    std::size_t total_command_rows{0U};
    std::uint64_t consumed_time_us{0U};
    std::uint64_t remaining_time_us{0U};
    std::uint64_t replay_hash{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans authoritative fixed-tick gameplay rows, ordered command playback, and system budgets.
/// The planner is value-only: it does not mutate gameplay state, run systems, create threads, open transports,
/// touch packages, upload renderer resources, or expose native handles.
[[nodiscard]] RuntimeGameplaySchedulerPlan
plan_runtime_gameplay_schedule(const RuntimeGameplaySchedulerRequest& request);

} // namespace mirakana::runtime
