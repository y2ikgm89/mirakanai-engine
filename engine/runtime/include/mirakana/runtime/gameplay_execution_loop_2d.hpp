// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/runtime/gameplay_interaction.hpp"
#include "mirakana/runtime/gameplay_runtime_scheduler.hpp"
#include "mirakana/runtime/world_entity_model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeGameplayExecutionLoop2DStatus : std::uint8_t {
    ready = 0,
    no_steps,
    invalid_request,
};

enum class RuntimeGameplayExecutionLoop2DDiagnosticCode : std::uint8_t {
    missing_loop_id = 0,
    invalid_fixed_delta,
    invalid_step_budget,
    invalid_command_budget,
    variable_delta_rejected,
    duplicate_system_order,
    missing_entity_binding,
    scheduler_failed,
    world_entity_model_failed,
    gameplay_interaction_failed,
};

struct RuntimeGameplayExecutionLoop2DDesc {
    std::string loop_id;
    std::uint64_t fixed_delta_us{0U};
    std::uint32_t max_steps_per_frame{0U};
    std::uint32_t max_command_rows_per_frame{0U};
    std::vector<RuntimeGameplaySchedulerSystemDesc> systems;
    RuntimeWorldEntityLifecycleRequest world;
    RuntimeGameplayInteractionState interaction_state;
};

struct RuntimeGameplayExecutionLoop2DFrameInput {
    std::uint64_t frame_index{0U};
    std::uint64_t next_tick{0U};
    std::uint64_t accumulated_time_us{0U};
    std::uint64_t observed_delta_us{0U};
    std::vector<RuntimeGameplaySchedulerInputCommandDesc> input_commands;
    std::vector<RuntimeGameplayInteractionEvent> interaction_events;
};

struct RuntimeGameplayExecutionLoop2DDiagnostic {
    RuntimeGameplayExecutionLoop2DDiagnosticCode code{RuntimeGameplayExecutionLoop2DDiagnosticCode::missing_loop_id};
    std::string loop_id;
    std::string detail_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct RuntimeGameplayExecutionLoop2DCounterRow {
    std::string counter_id;
    std::uint64_t value{0U};
};

struct RuntimeGameplayExecutionLoop2DSideEffectRow {
    std::string side_effect_id;
    std::uint64_t value{0U};
};

struct RuntimeGameplayExecutionLoop2DStepResult {
    RuntimeGameplayExecutionLoop2DStatus status{RuntimeGameplayExecutionLoop2DStatus::invalid_request};
    std::vector<RuntimeGameplayExecutionLoop2DDiagnostic> diagnostics;
    RuntimeGameplaySchedulerPlan scheduler;
    RuntimeWorldEntityLifecyclePlan world;
    RuntimeGameplayInteractionPlan interactions;
    std::vector<RuntimeGameplayExecutionLoop2DCounterRow> counters;
    std::vector<RuntimeGameplayExecutionLoop2DSideEffectRow> side_effects;
    std::uint64_t replay_hash{0U};
    std::size_t step_count{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Composes the first-party 2D gameplay scheduler, world/entity, and interaction row planners for one frame.
/// The loop is value-only: it does not mutate scenes, execute arbitrary callbacks, run IO, own renderer resources,
/// create threads, expose native handles, or infer readiness outside the returned rows and counters.
[[nodiscard]] RuntimeGameplayExecutionLoop2DStepResult
execute_runtime_gameplay_loop_2d_step(const RuntimeGameplayExecutionLoop2DDesc& desc,
                                      const RuntimeGameplayExecutionLoop2DFrameInput& frame);

} // namespace mirakana::runtime
