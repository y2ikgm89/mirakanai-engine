// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSimulationOrchestrationPlanStatus : std::uint8_t {
    planned = 0,
    no_steps,
    budget_limited,
    invalid_request,
};

enum class RuntimeSimulationOrchestrationDiagnosticCode : std::uint8_t {
    missing_simulation_id = 0,
    invalid_fixed_tick_rate,
    invalid_max_steps_per_frame,
    missing_command_id,
    missing_action_id,
    duplicate_command_id,
    command_before_next_tick,
    command_after_planned_window,
};

struct RuntimeSimulationInputCommandDesc {
    std::string command_id;
    std::string action_id;
    std::uint64_t target_tick{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeSimulationOrchestrationRequest {
    std::string simulation_id;
    std::uint64_t next_tick{0U};
    std::uint32_t fixed_tick_rate_hz{0U};
    std::uint64_t accumulated_time_us{0U};
    std::uint32_t max_steps_per_frame{0U};
    std::vector<RuntimeSimulationInputCommandDesc> input_commands;
};

struct RuntimeSimulationOrchestrationDiagnostic {
    RuntimeSimulationOrchestrationDiagnosticCode code{
        RuntimeSimulationOrchestrationDiagnosticCode::missing_simulation_id};
    std::string simulation_id;
    std::string command_id;
    std::string action_id;
    std::uint64_t target_tick{0U};
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeSimulationStepRow {
    std::string simulation_id;
    std::uint64_t tick{0U};
    std::uint64_t fixed_delta_us{0U};
    std::size_t command_count{0U};
};

struct RuntimeSimulationInputCommandRow {
    std::string simulation_id;
    std::string command_id;
    std::string action_id;
    std::uint64_t target_tick{0U};
    std::uint32_t source_index{0U};
};

struct RuntimeSimulationOrchestrationPlan {
    RuntimeSimulationOrchestrationPlanStatus status{RuntimeSimulationOrchestrationPlanStatus::invalid_request};
    std::vector<RuntimeSimulationOrchestrationDiagnostic> diagnostics;
    std::vector<RuntimeSimulationStepRow> steps;
    std::vector<RuntimeSimulationInputCommandRow> commands;
    std::uint64_t fixed_delta_us{0U};
    std::size_t available_step_count{0U};
    std::size_t planned_step_count{0U};
    std::uint64_t consumed_time_us{0U};
    std::uint64_t remaining_time_us{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans deterministic fixed-timestep simulation rows and ordered input-command playback rows.
/// This helper does not mutate gameplay state, run game rules, pace renderer frames, open network transports,
/// create threads, write packages, or expose native handles.
[[nodiscard]] RuntimeSimulationOrchestrationPlan
plan_runtime_simulation_orchestration(const RuntimeSimulationOrchestrationRequest& request);

} // namespace mirakana::runtime
