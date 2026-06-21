// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/gameplay_execution_loop_2d.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

void add_diagnostic(RuntimeGameplayExecutionLoop2DStepResult& result, RuntimeGameplayExecutionLoop2DDiagnosticCode code,
                    std::string loop_id, std::string detail_id, std::string message, std::uint32_t source_index = 0U) {
    result.diagnostics.push_back(RuntimeGameplayExecutionLoop2DDiagnostic{
        .code = code,
        .loop_id = std::move(loop_id),
        .detail_id = std::move(detail_id),
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

[[nodiscard]] std::uint64_t compute_loop_replay_hash(const RuntimeGameplayExecutionLoop2DDesc& desc,
                                                     const RuntimeGameplayExecutionLoop2DFrameInput& frame,
                                                     const RuntimeGameplayExecutionLoop2DStepResult& result) {
    auto hash = std::uint64_t{14695981039346656037ULL};
    hash = mix_hash(hash, hash_string(desc.loop_id));
    hash = mix_hash(hash, frame.frame_index);
    hash = mix_hash(hash, frame.next_tick);
    hash = mix_hash(hash, desc.fixed_delta_us);
    hash = mix_hash(hash, result.scheduler.replay_hash);
    hash = mix_hash(hash, result.step_count);
    for (const auto& entity : result.world.entity_rows) {
        hash = mix_hash(hash, hash_string(entity.entity_id.value));
        hash = mix_hash(hash, hash_string(entity.region_id.value));
        hash = mix_hash(hash, entity.generation);
    }
    for (const auto& row : result.interactions.rows) {
        hash = mix_hash(hash, hash_string(row.event_id));
        hash = mix_hash(hash, static_cast<std::uint64_t>(row.kind));
        hash = mix_hash(hash, static_cast<std::uint64_t>(row.next_session_state));
        hash = mix_hash(hash, static_cast<std::uint64_t>(row.next_health));
        hash = mix_hash(hash, row.next_objective_progress);
    }
    return hash == 0U ? 1U : hash;
}

void append_counters(RuntimeGameplayExecutionLoop2DStepResult& result) {
    result.counters = {
        RuntimeGameplayExecutionLoop2DCounterRow{
            .counter_id = "2d_gameplay_execution_loop_status",
            .value = result.succeeded() ? 1U : 0U,
        },
        RuntimeGameplayExecutionLoop2DCounterRow{
            .counter_id = "2d_gameplay_execution_loop_steps",
            .value = static_cast<std::uint64_t>(result.step_count),
        },
        RuntimeGameplayExecutionLoop2DCounterRow{
            .counter_id = "2d_gameplay_execution_loop_replay_hash",
            .value = result.replay_hash,
        },
        RuntimeGameplayExecutionLoop2DCounterRow{
            .counter_id = "2d_gameplay_execution_loop_diagnostics",
            .value = static_cast<std::uint64_t>(result.diagnostics.size()),
        },
        RuntimeGameplayExecutionLoop2DCounterRow{
            .counter_id = "2d_gameplay_execution_loop_side_effects",
            .value = static_cast<std::uint64_t>(result.side_effects.size()),
        },
    };
}

void sort_diagnostics(RuntimeGameplayExecutionLoop2DStepResult& result) {
    std::ranges::sort(result.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.detail_id != rhs.detail_id) {
            return lhs.detail_id < rhs.detail_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

void validate_desc(RuntimeGameplayExecutionLoop2DStepResult& result, const RuntimeGameplayExecutionLoop2DDesc& desc,
                   const RuntimeGameplayExecutionLoop2DFrameInput& frame) {
    if (!is_valid_id(desc.loop_id)) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::missing_loop_id, desc.loop_id, {},
                       "runtime 2d gameplay execution loop id must be non-empty and path-safe");
    }
    if (desc.fixed_delta_us == 0U) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::invalid_fixed_delta, desc.loop_id, {},
                       "runtime 2d gameplay execution loop fixed delta must be non-zero");
    }
    if (desc.max_steps_per_frame == 0U) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::invalid_step_budget, desc.loop_id, {},
                       "runtime 2d gameplay execution loop max steps per frame must be non-zero");
    }
    if (desc.max_command_rows_per_frame == 0U) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::invalid_command_budget, desc.loop_id, {},
                       "runtime 2d gameplay execution loop command budget must be non-zero");
    } else if (frame.input_commands.size() > desc.max_command_rows_per_frame) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::invalid_command_budget, desc.loop_id, {},
                       "runtime 2d gameplay execution loop input command count exceeds the frame budget");
    }
    if (frame.observed_delta_us != 0U && frame.observed_delta_us != desc.fixed_delta_us) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::variable_delta_rejected, desc.loop_id, {},
                       "runtime 2d gameplay execution loop rejects variable delta authority");
    }

    std::vector<std::int32_t> orders;
    orders.reserve(desc.systems.size());
    for (const auto& system : desc.systems) {
        if (!system.enabled) {
            continue;
        }
        if (std::ranges::find(orders, system.order) != orders.end()) {
            add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::duplicate_system_order, desc.loop_id,
                           system.system_id, "runtime 2d gameplay execution loop requires unique enabled system order",
                           system.source_index);
            continue;
        }
        orders.push_back(system.order);
    }
}

void validate_entity_bindings(RuntimeGameplayExecutionLoop2DStepResult& result,
                              const RuntimeGameplayExecutionLoop2DDesc& desc) {
    for (const auto& interaction_entity : desc.interaction_state.entities) {
        const auto has_world_entity =
            std::ranges::any_of(result.world.entity_rows, [&interaction_entity](const RuntimeWorldEntityRow& row) {
                return row.entity_id.value == interaction_entity.id;
            });
        if (!has_world_entity) {
            add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::missing_entity_binding, desc.loop_id,
                           interaction_entity.id,
                           "runtime 2d gameplay execution loop interaction entity must exist in world entity rows");
        }
    }
}

} // namespace

bool RuntimeGameplayExecutionLoop2DStepResult::succeeded() const noexcept {
    return status == RuntimeGameplayExecutionLoop2DStatus::ready ||
           status == RuntimeGameplayExecutionLoop2DStatus::no_steps;
}

RuntimeGameplayExecutionLoop2DStepResult
execute_runtime_gameplay_loop_2d_step(const RuntimeGameplayExecutionLoop2DDesc& desc,
                                      const RuntimeGameplayExecutionLoop2DFrameInput& frame) {
    RuntimeGameplayExecutionLoop2DStepResult result;

    validate_desc(result, desc, frame);
    if (!result.diagnostics.empty()) {
        sort_diagnostics(result);
        result.status = RuntimeGameplayExecutionLoop2DStatus::invalid_request;
        append_counters(result);
        return result;
    }

    result.scheduler = plan_runtime_gameplay_schedule(RuntimeGameplaySchedulerRequest{
        .scheduler_id = desc.loop_id,
        .next_tick = frame.next_tick,
        .tick_delta_us = desc.fixed_delta_us,
        .accumulated_time_us = frame.accumulated_time_us,
        .max_steps_per_frame = desc.max_steps_per_frame,
        .mode = RuntimeGameplaySchedulerMode::run,
        .systems = desc.systems,
        .input_commands = frame.input_commands,
    });
    result.world = plan_runtime_world_entity_lifecycle(desc.world);
    result.interactions = plan_runtime_gameplay_interactions(desc.interaction_state, frame.interaction_events);

    if (!result.scheduler.succeeded()) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::scheduler_failed, desc.loop_id, {},
                       "runtime 2d gameplay execution loop scheduler plan failed");
    }
    if (!result.world.succeeded()) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::world_entity_model_failed, desc.loop_id,
                       {}, "runtime 2d gameplay execution loop world entity model plan failed");
    }
    if (!result.interactions.succeeded) {
        add_diagnostic(result, RuntimeGameplayExecutionLoop2DDiagnosticCode::gameplay_interaction_failed, desc.loop_id,
                       {}, "runtime 2d gameplay execution loop gameplay interaction plan failed");
    }
    if (result.world.succeeded()) {
        validate_entity_bindings(result, desc);
    }

    if (!result.diagnostics.empty()) {
        sort_diagnostics(result);
        result.status = RuntimeGameplayExecutionLoop2DStatus::invalid_request;
        append_counters(result);
        return result;
    }

    result.step_count = result.scheduler.steps.size();
    result.status = result.step_count == 0U ? RuntimeGameplayExecutionLoop2DStatus::no_steps
                                            : RuntimeGameplayExecutionLoop2DStatus::ready;
    result.replay_hash = compute_loop_replay_hash(desc, frame, result);
    append_counters(result);
    return result;
}

} // namespace mirakana::runtime
