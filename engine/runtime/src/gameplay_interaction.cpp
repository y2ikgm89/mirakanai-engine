// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/gameplay_interaction.hpp"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool valid_id(const std::string_view value) noexcept {
    return !value.empty();
}

[[nodiscard]] RuntimeGameplayEntityState* find_entity(RuntimeGameplayInteractionState& state,
                                                      const std::string_view id) noexcept {
    const auto entity = std::ranges::find_if(state.entities, [id](const RuntimeGameplayEntityState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (entity == state.entities.end()) {
        return nullptr;
    }
    return &(*entity);
}

[[nodiscard]] const RuntimeGameplayEntityState* find_entity(const RuntimeGameplayInteractionState& state,
                                                            const std::string_view id) noexcept {
    const auto entity = std::ranges::find_if(state.entities, [id](const RuntimeGameplayEntityState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (entity == state.entities.end()) {
        return nullptr;
    }
    return &(*entity);
}

[[nodiscard]] RuntimeGameplayPickupState* find_pickup(RuntimeGameplayInteractionState& state,
                                                      const std::string_view id) noexcept {
    const auto pickup = std::ranges::find_if(state.pickups, [id](const RuntimeGameplayPickupState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (pickup == state.pickups.end()) {
        return nullptr;
    }
    return &(*pickup);
}

[[nodiscard]] const RuntimeGameplayPickupState* find_pickup(const RuntimeGameplayInteractionState& state,
                                                            const std::string_view id) noexcept {
    const auto pickup = std::ranges::find_if(state.pickups, [id](const RuntimeGameplayPickupState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (pickup == state.pickups.end()) {
        return nullptr;
    }
    return &(*pickup);
}

[[nodiscard]] RuntimeGameplayObjectiveState* find_objective(RuntimeGameplayInteractionState& state,
                                                            const std::string_view id) noexcept {
    const auto objective = std::ranges::find_if(state.objectives, [id](const RuntimeGameplayObjectiveState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (objective == state.objectives.end()) {
        return nullptr;
    }
    return &(*objective);
}

[[nodiscard]] const RuntimeGameplayObjectiveState* find_objective(const RuntimeGameplayInteractionState& state,
                                                                  const std::string_view id) noexcept {
    const auto objective = std::ranges::find_if(state.objectives, [id](const RuntimeGameplayObjectiveState& candidate) {
        return std::string_view{candidate.id} == id;
    });
    if (objective == state.objectives.end()) {
        return nullptr;
    }
    return &(*objective);
}

void add_diagnostic(std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics,
                    RuntimeGameplayInteractionDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void validate_duplicate_event_ids(const std::span<const RuntimeGameplayInteractionEvent> events,
                                  std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics) {
    for (auto first = events.begin(); first != events.end(); ++first) {
        if (first->id.empty()) {
            continue;
        }
        for (auto second = std::next(first); second != events.end(); ++second) {
            if (second->id == first->id) {
                add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                                .code = RuntimeGameplayInteractionDiagnosticCode::duplicate_event_id,
                                                .event_id = first->id,
                                                .kind = first->kind,
                                                .source_entity_id = first->source_entity_id,
                                                .target_entity_id = first->target_entity_id,
                                                .pickup_id = first->pickup_id,
                                                .objective_id = first->objective_id,
                                                .feedback_id = first->feedback_id,
                                                .amount = first->amount,
                                                .previous_session_state = RuntimeGameplaySessionState::running,
                                            });
                return;
            }
        }
    }
}

void validate_state(const RuntimeGameplayInteractionState& state,
                    std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics) {
    for (auto first = state.entities.begin(); first != state.entities.end(); ++first) {
        if (!valid_id(first->id) || first->max_health <= 0 || first->health < 0 || first->health > first->max_health) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_entity_state,
                                            .source_entity_id = first->id,
                                        });
        }
        for (auto second = std::next(first); second != state.entities.end(); ++second) {
            if (!first->id.empty() && first->id == second->id) {
                add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                                .code = RuntimeGameplayInteractionDiagnosticCode::duplicate_entity_id,
                                                .source_entity_id = first->id,
                                            });
                break;
            }
        }
    }

    for (auto first = state.pickups.begin(); first != state.pickups.end(); ++first) {
        if (!valid_id(first->id) || !valid_id(first->item_id) || first->quantity == 0U) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_pickup_state,
                                            .pickup_id = first->id,
                                        });
        }
        for (auto second = std::next(first); second != state.pickups.end(); ++second) {
            if (!first->id.empty() && first->id == second->id) {
                add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                                .code = RuntimeGameplayInteractionDiagnosticCode::duplicate_pickup_id,
                                                .pickup_id = first->id,
                                            });
                break;
            }
        }
    }

    for (auto first = state.objectives.begin(); first != state.objectives.end(); ++first) {
        if (!valid_id(first->id) || first->target == 0U || first->progress > first->target ||
            (first->completed && first->progress != first->target)) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_objective_state,
                                            .objective_id = first->id,
                                        });
        }
        for (auto second = std::next(first); second != state.objectives.end(); ++second) {
            if (!first->id.empty() && first->id == second->id) {
                add_diagnostic(diagnostics,
                               RuntimeGameplayInteractionDiagnostic{
                                   .code = RuntimeGameplayInteractionDiagnosticCode::duplicate_objective_id,
                                   .objective_id = first->id,
                               });
                break;
            }
        }
    }
}

[[nodiscard]] bool requires_source(const RuntimeGameplayInteractionKind kind) noexcept {
    switch (kind) {
    case RuntimeGameplayInteractionKind::trigger:
    case RuntimeGameplayInteractionKind::damage:
    case RuntimeGameplayInteractionKind::heal:
    case RuntimeGameplayInteractionKind::pickup:
    case RuntimeGameplayInteractionKind::feedback:
        return true;
    case RuntimeGameplayInteractionKind::objective_progress:
    case RuntimeGameplayInteractionKind::objective_complete:
    case RuntimeGameplayInteractionKind::win:
    case RuntimeGameplayInteractionKind::loss:
    case RuntimeGameplayInteractionKind::restart:
        return false;
    }
    return false;
}

void validate_source(const RuntimeGameplayInteractionState& state, const RuntimeGameplayInteractionEvent& event,
                     std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics) {
    if (!requires_source(event.kind) && event.source_entity_id.empty()) {
        return;
    }
    if (find_entity(state, event.source_entity_id) != nullptr) {
        return;
    }

    add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                    .code = RuntimeGameplayInteractionDiagnosticCode::missing_source_entity,
                                    .event_id = event.id,
                                    .kind = event.kind,
                                    .source_entity_id = event.source_entity_id,
                                    .target_entity_id = event.target_entity_id,
                                    .pickup_id = event.pickup_id,
                                    .objective_id = event.objective_id,
                                    .feedback_id = event.feedback_id,
                                    .amount = event.amount,
                                    .previous_session_state = state.session_state,
                                });
}

void validate_event(const RuntimeGameplayInteractionState& state, const RuntimeGameplayInteractionEvent& event,
                    std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics) {
    if (!valid_id(event.id)) {
        add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                        .code = RuntimeGameplayInteractionDiagnosticCode::invalid_event_id,
                                        .event_id = event.id,
                                        .kind = event.kind,
                                        .source_entity_id = event.source_entity_id,
                                        .target_entity_id = event.target_entity_id,
                                        .pickup_id = event.pickup_id,
                                        .objective_id = event.objective_id,
                                        .feedback_id = event.feedback_id,
                                        .amount = event.amount,
                                        .previous_session_state = state.session_state,
                                    });
    }

    if (state.session_state != RuntimeGameplaySessionState::running &&
        event.kind != RuntimeGameplayInteractionKind::restart) {
        add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                        .code = RuntimeGameplayInteractionDiagnosticCode::terminal_session_state,
                                        .event_id = event.id,
                                        .kind = event.kind,
                                        .source_entity_id = event.source_entity_id,
                                        .target_entity_id = event.target_entity_id,
                                        .pickup_id = event.pickup_id,
                                        .objective_id = event.objective_id,
                                        .feedback_id = event.feedback_id,
                                        .amount = event.amount,
                                        .previous_session_state = state.session_state,
                                    });
        return;
    }

    validate_source(state, event, diagnostics);

    switch (event.kind) {
    case RuntimeGameplayInteractionKind::damage:
    case RuntimeGameplayInteractionKind::heal:
        if (find_entity(state, event.target_entity_id) == nullptr) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::missing_target_entity,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        if (event.amount <= 0) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_amount,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        break;
    case RuntimeGameplayInteractionKind::pickup:
        if (const auto* pickup = find_pickup(state, event.pickup_id); pickup == nullptr) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::missing_pickup,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        } else if (!pickup->available) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::unavailable_pickup,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        if (event.amount <= 0) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_amount,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        break;
    case RuntimeGameplayInteractionKind::objective_progress:
    case RuntimeGameplayInteractionKind::objective_complete:
        if (const auto* objective = find_objective(state, event.objective_id); objective == nullptr) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::missing_objective,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        } else if (objective->completed && event.kind == RuntimeGameplayInteractionKind::objective_progress) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::completed_objective,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        if (event.kind == RuntimeGameplayInteractionKind::objective_progress && event.amount <= 0) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_amount,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        break;
    case RuntimeGameplayInteractionKind::feedback:
        if (!valid_id(event.feedback_id)) {
            add_diagnostic(diagnostics, RuntimeGameplayInteractionDiagnostic{
                                            .code = RuntimeGameplayInteractionDiagnosticCode::invalid_feedback_id,
                                            .event_id = event.id,
                                            .kind = event.kind,
                                            .source_entity_id = event.source_entity_id,
                                            .target_entity_id = event.target_entity_id,
                                            .pickup_id = event.pickup_id,
                                            .objective_id = event.objective_id,
                                            .feedback_id = event.feedback_id,
                                            .amount = event.amount,
                                            .previous_session_state = state.session_state,
                                        });
        }
        break;
    case RuntimeGameplayInteractionKind::trigger:
    case RuntimeGameplayInteractionKind::win:
    case RuntimeGameplayInteractionKind::loss:
    case RuntimeGameplayInteractionKind::restart:
        break;
    }
}

[[nodiscard]] RuntimeGameplayInteractionRow make_row(const RuntimeGameplayInteractionEvent& event,
                                                     const RuntimeGameplaySessionState previous_session_state) {
    return RuntimeGameplayInteractionRow{
        .event_id = event.id,
        .kind = event.kind,
        .source_entity_id = event.source_entity_id,
        .target_entity_id = event.target_entity_id,
        .pickup_id = event.pickup_id,
        .objective_id = event.objective_id,
        .feedback_id = event.feedback_id,
        .amount = event.amount,
        .previous_health = 0,
        .next_health = 0,
        .previous_objective_progress = 0U,
        .next_objective_progress = 0U,
        .previous_session_state = previous_session_state,
        .next_session_state = previous_session_state,
        .pickup_available_after = false,
    };
}

void add_feedback_row(RuntimeGameplayInteractionPlan& plan, const RuntimeGameplayInteractionEvent& event) {
    if (event.feedback_id.empty()) {
        return;
    }
    plan.feedback_rows.push_back(RuntimeGameplayFeedbackRow{
        .event_id = event.id,
        .feedback_id = event.feedback_id,
        .kind = event.kind,
        .source_entity_id = event.source_entity_id,
        .target_entity_id = event.target_entity_id,
        .objective_id = event.objective_id,
    });
}

void apply_event(RuntimeGameplayInteractionPlan& plan, const RuntimeGameplayInteractionEvent& event) {
    auto row = make_row(event, plan.state.session_state);
    switch (event.kind) {
    case RuntimeGameplayInteractionKind::damage: {
        auto* target = find_entity(plan.state, event.target_entity_id);
        row.previous_health = target->health;
        target->health = std::clamp(target->health - event.amount, 0, target->max_health);
        target->active = target->health > 0;
        row.next_health = target->health;
        break;
    }
    case RuntimeGameplayInteractionKind::heal: {
        auto* target = find_entity(plan.state, event.target_entity_id);
        row.previous_health = target->health;
        target->health = std::clamp(target->health + event.amount, 0, target->max_health);
        target->active = target->health > 0;
        row.next_health = target->health;
        break;
    }
    case RuntimeGameplayInteractionKind::pickup: {
        auto* pickup = find_pickup(plan.state, event.pickup_id);
        pickup->available = false;
        row.pickup_available_after = pickup->available;
        break;
    }
    case RuntimeGameplayInteractionKind::objective_progress: {
        auto* objective = find_objective(plan.state, event.objective_id);
        row.previous_objective_progress = objective->progress;
        const auto next_progress =
            std::min(objective->target, objective->progress + static_cast<std::uint32_t>(event.amount));
        objective->progress = next_progress;
        objective->completed = objective->progress == objective->target;
        row.next_objective_progress = objective->progress;
        break;
    }
    case RuntimeGameplayInteractionKind::objective_complete: {
        auto* objective = find_objective(plan.state, event.objective_id);
        row.previous_objective_progress = objective->progress;
        objective->progress = objective->target;
        objective->completed = true;
        row.next_objective_progress = objective->progress;
        break;
    }
    case RuntimeGameplayInteractionKind::win:
        plan.state.session_state = RuntimeGameplaySessionState::won;
        row.next_session_state = plan.state.session_state;
        break;
    case RuntimeGameplayInteractionKind::loss:
        plan.state.session_state = RuntimeGameplaySessionState::lost;
        row.next_session_state = plan.state.session_state;
        break;
    case RuntimeGameplayInteractionKind::restart:
        plan.state.session_state = RuntimeGameplaySessionState::running;
        row.next_session_state = plan.state.session_state;
        break;
    case RuntimeGameplayInteractionKind::trigger:
    case RuntimeGameplayInteractionKind::feedback:
        break;
    }

    plan.rows.push_back(std::move(row));
    add_feedback_row(plan, event);
}

void validate_event_sequence(const RuntimeGameplayInteractionState& state,
                             const std::span<const RuntimeGameplayInteractionEvent> events,
                             std::vector<RuntimeGameplayInteractionDiagnostic>& diagnostics) {
    RuntimeGameplayInteractionPlan projected{
        .succeeded = true,
        .state = state,
        .rows = {},
        .feedback_rows = {},
        .diagnostics = {},
    };

    for (const auto& event : events) {
        const auto diagnostic_count = diagnostics.size();
        validate_event(projected.state, event, diagnostics);
        if (diagnostics.size() == diagnostic_count) {
            apply_event(projected, event);
        }
    }
}

} // namespace

RuntimeGameplayInteractionPlan
plan_runtime_gameplay_interactions(const RuntimeGameplayInteractionState& state,
                                   const std::span<const RuntimeGameplayInteractionEvent> events) {
    RuntimeGameplayInteractionPlan plan{
        .succeeded = true,
        .state = state,
        .rows = {},
        .feedback_rows = {},
        .diagnostics = {},
    };

    validate_state(state, plan.diagnostics);
    const auto state_is_valid = plan.diagnostics.empty();
    validate_duplicate_event_ids(events, plan.diagnostics);
    if (state_is_valid) {
        validate_event_sequence(state, events, plan.diagnostics);
    }

    if (!plan.diagnostics.empty()) {
        plan.succeeded = false;
        plan.state = state;
        plan.rows.clear();
        plan.feedback_rows.clear();
        return plan;
    }

    for (const auto& event : events) {
        apply_event(plan, event);
    }

    return plan;
}

} // namespace mirakana::runtime
