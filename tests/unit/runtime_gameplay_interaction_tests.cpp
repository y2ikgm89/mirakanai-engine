// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/gameplay_interaction.hpp"

#include <span>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeGameplayInteractionState gameplay_interaction_state() {
    using namespace mirakana::runtime;

    return RuntimeGameplayInteractionState{
        .session_state = RuntimeGameplaySessionState::running,
        .entities =
            std::vector<RuntimeGameplayEntityState>{
                RuntimeGameplayEntityState{
                    .id = "player",
                    .health = 8,
                    .max_health = 10,
                    .active = true,
                },
                RuntimeGameplayEntityState{
                    .id = "enemy",
                    .health = 5,
                    .max_health = 5,
                    .active = true,
                },
            },
        .pickups =
            std::vector<RuntimeGameplayPickupState>{
                RuntimeGameplayPickupState{
                    .id = "pickup.gem",
                    .item_id = "gem",
                    .quantity = 1U,
                    .available = true,
                },
            },
        .objectives =
            std::vector<RuntimeGameplayObjectiveState>{
                RuntimeGameplayObjectiveState{
                    .id = "objective.escape",
                    .progress = 1U,
                    .target = 3U,
                    .completed = false,
                },
            },
    };
}

[[nodiscard]] std::vector<mirakana::runtime::RuntimeGameplayInteractionEvent> gameplay_interaction_events() {
    using namespace mirakana::runtime;

    return std::vector<RuntimeGameplayInteractionEvent>{
        RuntimeGameplayInteractionEvent{
            .id = "event.trigger",
            .kind = RuntimeGameplayInteractionKind::trigger,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.trigger",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.damage",
            .kind = RuntimeGameplayInteractionKind::damage,
            .source_entity_id = "player",
            .target_entity_id = "enemy",
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.damage",
            .amount = 3,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.heal",
            .kind = RuntimeGameplayInteractionKind::heal,
            .source_entity_id = "player",
            .target_entity_id = "player",
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.heal",
            .amount = 2,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.pickup",
            .kind = RuntimeGameplayInteractionKind::pickup,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = "pickup.gem",
            .objective_id = {},
            .feedback_id = "feedback.pickup",
            .amount = 1,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.objective",
            .kind = RuntimeGameplayInteractionKind::objective_progress,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = "objective.escape",
            .feedback_id = "feedback.objective",
            .amount = 2,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.feedback",
            .kind = RuntimeGameplayInteractionKind::feedback,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.prompt",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.win",
            .kind = RuntimeGameplayInteractionKind::win,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.win",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.restart",
            .kind = RuntimeGameplayInteractionKind::restart,
            .source_entity_id = {},
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.restart",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.loss",
            .kind = RuntimeGameplayInteractionKind::loss,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.loss",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.restart_after_loss",
            .kind = RuntimeGameplayInteractionKind::restart,
            .source_entity_id = {},
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.restart_after_loss",
            .amount = 0,
        },
    };
}

} // namespace

MK_TEST("runtime gameplay interactions update state and feedback deterministically") {
    using Kind = mirakana::runtime::RuntimeGameplayInteractionKind;
    using Session = mirakana::runtime::RuntimeGameplaySessionState;

    const auto state = gameplay_interaction_state();
    const auto events = gameplay_interaction_events();

    const auto first = mirakana::runtime::plan_runtime_gameplay_interactions(
        state, std::span<const mirakana::runtime::RuntimeGameplayInteractionEvent>{events});
    const auto second = mirakana::runtime::plan_runtime_gameplay_interactions(
        state, std::span<const mirakana::runtime::RuntimeGameplayInteractionEvent>{events});

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.feedback_rows == second.feedback_rows);
    MK_REQUIRE(first.state == second.state);
    MK_REQUIRE(first.rows.size() == 10U);
    MK_REQUIRE(first.feedback_rows.size() == 10U);

    MK_REQUIRE(first.rows[0].kind == Kind::trigger);
    MK_REQUIRE(first.rows[0].feedback_id == "feedback.trigger");
    MK_REQUIRE(first.rows[1].kind == Kind::damage);
    MK_REQUIRE(first.rows[1].previous_health == 5);
    MK_REQUIRE(first.rows[1].next_health == 2);
    MK_REQUIRE(first.rows[2].kind == Kind::heal);
    MK_REQUIRE(first.rows[2].previous_health == 8);
    MK_REQUIRE(first.rows[2].next_health == 10);
    MK_REQUIRE(first.rows[3].kind == Kind::pickup);
    MK_REQUIRE(!first.rows[3].pickup_available_after);
    MK_REQUIRE(first.rows[4].kind == Kind::objective_progress);
    MK_REQUIRE(first.rows[4].previous_objective_progress == 1U);
    MK_REQUIRE(first.rows[4].next_objective_progress == 3U);
    MK_REQUIRE(first.rows[6].kind == Kind::win);
    MK_REQUIRE(first.rows[6].previous_session_state == Session::running);
    MK_REQUIRE(first.rows[6].next_session_state == Session::won);
    MK_REQUIRE(first.rows[7].kind == Kind::restart);
    MK_REQUIRE(first.rows[7].previous_session_state == Session::won);
    MK_REQUIRE(first.rows[7].next_session_state == Session::running);
    MK_REQUIRE(first.rows[8].kind == Kind::loss);
    MK_REQUIRE(first.rows[8].previous_session_state == Session::running);
    MK_REQUIRE(first.rows[8].next_session_state == Session::lost);
    MK_REQUIRE(first.rows[9].kind == Kind::restart);
    MK_REQUIRE(first.rows[9].previous_session_state == Session::lost);
    MK_REQUIRE(first.rows[9].next_session_state == Session::running);

    MK_REQUIRE(first.state.session_state == Session::running);
    MK_REQUIRE(first.state.entities.size() == 2U);
    MK_REQUIRE(first.state.entities[0].id == "player");
    MK_REQUIRE(first.state.entities[0].health == 10);
    MK_REQUIRE(first.state.entities[1].id == "enemy");
    MK_REQUIRE(first.state.entities[1].health == 2);
    MK_REQUIRE(first.state.pickups.size() == 1U);
    MK_REQUIRE(!first.state.pickups[0].available);
    MK_REQUIRE(first.state.objectives.size() == 1U);
    MK_REQUIRE(first.state.objectives[0].completed);
    MK_REQUIRE(first.state.objectives[0].progress == 3U);
}

MK_TEST("runtime gameplay interactions fail closed on invalid requests") {
    using Code = mirakana::runtime::RuntimeGameplayInteractionDiagnosticCode;
    using Kind = mirakana::runtime::RuntimeGameplayInteractionKind;
    using namespace mirakana::runtime;

    const auto state = gameplay_interaction_state();
    const std::vector<RuntimeGameplayInteractionEvent> events{
        RuntimeGameplayInteractionEvent{
            .id = "event.bad_damage",
            .kind = RuntimeGameplayInteractionKind::damage,
            .source_entity_id = "player",
            .target_entity_id = "missing",
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.damage",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.bad_damage",
            .kind = RuntimeGameplayInteractionKind::restart,
            .source_entity_id = {},
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.restart",
            .amount = 0,
        },
    };

    const auto first =
        plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});
    const auto second =
        plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.state == state);
    MK_REQUIRE(first.rows.empty());
    MK_REQUIRE(first.feedback_rows.empty());
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.diagnostics.size() == 3U);
    MK_REQUIRE(first.diagnostics[0].code == Code::duplicate_event_id);
    MK_REQUIRE(first.diagnostics[0].event_id == "event.bad_damage");
    MK_REQUIRE(first.diagnostics[1].code == Code::missing_target_entity);
    MK_REQUIRE(first.diagnostics[1].kind == Kind::damage);
    MK_REQUIRE(first.diagnostics[1].target_entity_id == "missing");
    MK_REQUIRE(first.diagnostics[2].code == Code::invalid_amount);
    MK_REQUIRE(first.diagnostics[2].event_id == "event.bad_damage");
}

MK_TEST("runtime gameplay interactions reject terminal non-restart events") {
    using Code = mirakana::runtime::RuntimeGameplayInteractionDiagnosticCode;
    using Session = mirakana::runtime::RuntimeGameplaySessionState;
    using namespace mirakana::runtime;

    auto state = gameplay_interaction_state();
    state.session_state = RuntimeGameplaySessionState::lost;
    const std::vector<RuntimeGameplayInteractionEvent> events{
        RuntimeGameplayInteractionEvent{
            .id = "event.trigger_after_loss",
            .kind = RuntimeGameplayInteractionKind::trigger,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.trigger",
            .amount = 0,
        },
    };

    const auto plan =
        plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});

    MK_REQUIRE(!plan.succeeded);
    MK_REQUIRE(plan.state == state);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.feedback_rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code == Code::terminal_session_state);
    MK_REQUIRE(plan.diagnostics[0].previous_session_state == Session::lost);
}

MK_TEST("runtime gameplay interactions fail closed after terminal events in the same plan") {
    using Code = mirakana::runtime::RuntimeGameplayInteractionDiagnosticCode;
    using Kind = mirakana::runtime::RuntimeGameplayInteractionKind;
    using Session = mirakana::runtime::RuntimeGameplaySessionState;
    using namespace mirakana::runtime;

    const auto state = gameplay_interaction_state();
    const std::vector<RuntimeGameplayInteractionEvent> events{
        RuntimeGameplayInteractionEvent{
            .id = "event.win",
            .kind = RuntimeGameplayInteractionKind::win,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.win",
            .amount = 0,
        },
        RuntimeGameplayInteractionEvent{
            .id = "event.feedback_after_win",
            .kind = RuntimeGameplayInteractionKind::feedback,
            .source_entity_id = "player",
            .target_entity_id = {},
            .pickup_id = {},
            .objective_id = {},
            .feedback_id = "feedback.prompt",
            .amount = 0,
        },
    };

    const auto plan =
        plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});

    MK_REQUIRE(!plan.succeeded);
    MK_REQUIRE(plan.state == state);
    MK_REQUIRE(plan.rows.empty());
    MK_REQUIRE(plan.feedback_rows.empty());
    MK_REQUIRE(plan.diagnostics.size() == 1U);
    MK_REQUIRE(plan.diagnostics[0].code == Code::terminal_session_state);
    MK_REQUIRE(plan.diagnostics[0].event_id == "event.feedback_after_win");
    MK_REQUIRE(plan.diagnostics[0].kind == Kind::feedback);
    MK_REQUIRE(plan.diagnostics[0].previous_session_state == Session::won);
}

int main() {
    return mirakana::test::run_all();
}
