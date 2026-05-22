// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeGameplayInteractionKind : std::uint8_t {
    trigger,
    damage,
    heal,
    pickup,
    objective_progress,
    objective_complete,
    feedback,
    win,
    loss,
    restart,
};

enum class RuntimeGameplaySessionState : std::uint8_t {
    running,
    won,
    lost,
};

struct RuntimeGameplayEntityState {
    std::string id;
    int health{1};
    int max_health{1};
    bool active{true};

    [[nodiscard]] bool operator==(const RuntimeGameplayEntityState&) const = default;
};

struct RuntimeGameplayPickupState {
    std::string id;
    std::string item_id;
    std::uint32_t quantity{1U};
    bool available{true};

    [[nodiscard]] bool operator==(const RuntimeGameplayPickupState&) const = default;
};

struct RuntimeGameplayObjectiveState {
    std::string id;
    std::uint32_t progress{0U};
    std::uint32_t target{1U};
    bool completed{false};

    [[nodiscard]] bool operator==(const RuntimeGameplayObjectiveState&) const = default;
};

struct RuntimeGameplayInteractionState {
    RuntimeGameplaySessionState session_state{RuntimeGameplaySessionState::running};
    std::vector<RuntimeGameplayEntityState> entities;
    std::vector<RuntimeGameplayPickupState> pickups;
    std::vector<RuntimeGameplayObjectiveState> objectives;

    [[nodiscard]] bool operator==(const RuntimeGameplayInteractionState&) const = default;
};

struct RuntimeGameplayInteractionEvent {
    std::string id;
    RuntimeGameplayInteractionKind kind{RuntimeGameplayInteractionKind::trigger};
    std::string source_entity_id;
    std::string target_entity_id;
    std::string pickup_id;
    std::string objective_id;
    // Optional for non-feedback interactions; non-empty ids produce feedback rows.
    std::string feedback_id;
    int amount{0};

    [[nodiscard]] bool operator==(const RuntimeGameplayInteractionEvent&) const = default;
};

struct RuntimeGameplayInteractionRow {
    std::string event_id;
    RuntimeGameplayInteractionKind kind{RuntimeGameplayInteractionKind::trigger};
    std::string source_entity_id;
    std::string target_entity_id;
    std::string pickup_id;
    std::string objective_id;
    std::string feedback_id;
    int amount{0};
    int previous_health{0};
    int next_health{0};
    std::uint32_t previous_objective_progress{0U};
    std::uint32_t next_objective_progress{0U};
    RuntimeGameplaySessionState previous_session_state{RuntimeGameplaySessionState::running};
    RuntimeGameplaySessionState next_session_state{RuntimeGameplaySessionState::running};
    bool pickup_available_after{false};

    [[nodiscard]] bool operator==(const RuntimeGameplayInteractionRow&) const = default;
};

struct RuntimeGameplayFeedbackRow {
    std::string event_id;
    std::string feedback_id;
    RuntimeGameplayInteractionKind kind{RuntimeGameplayInteractionKind::trigger};
    std::string source_entity_id;
    std::string target_entity_id;
    std::string objective_id;

    [[nodiscard]] bool operator==(const RuntimeGameplayFeedbackRow&) const = default;
};

enum class RuntimeGameplayInteractionDiagnosticCode : std::uint8_t {
    none,
    invalid_event_id,
    duplicate_event_id,
    invalid_entity_state,
    duplicate_entity_id,
    invalid_pickup_state,
    duplicate_pickup_id,
    invalid_objective_state,
    duplicate_objective_id,
    missing_source_entity,
    missing_target_entity,
    missing_pickup,
    unavailable_pickup,
    missing_objective,
    completed_objective,
    invalid_feedback_id,
    invalid_amount,
    terminal_session_state,
};

struct RuntimeGameplayInteractionDiagnostic {
    RuntimeGameplayInteractionDiagnosticCode code{RuntimeGameplayInteractionDiagnosticCode::none};
    std::string event_id;
    RuntimeGameplayInteractionKind kind{RuntimeGameplayInteractionKind::trigger};
    std::string source_entity_id;
    std::string target_entity_id;
    std::string pickup_id;
    std::string objective_id;
    std::string feedback_id;
    int amount{0};
    RuntimeGameplaySessionState previous_session_state{RuntimeGameplaySessionState::running};

    [[nodiscard]] bool operator==(const RuntimeGameplayInteractionDiagnostic&) const = default;
};

struct RuntimeGameplayInteractionPlan {
    bool succeeded{true};
    RuntimeGameplayInteractionState state;
    std::vector<RuntimeGameplayInteractionRow> rows;
    std::vector<RuntimeGameplayFeedbackRow> feedback_rows;
    std::vector<RuntimeGameplayInteractionDiagnostic> diagnostics;

    [[nodiscard]] bool operator==(const RuntimeGameplayInteractionPlan&) const = default;
};

[[nodiscard]] RuntimeGameplayInteractionPlan
plan_runtime_gameplay_interactions(const RuntimeGameplayInteractionState& state,
                                   std::span<const RuntimeGameplayInteractionEvent> events);

} // namespace mirakana::runtime
