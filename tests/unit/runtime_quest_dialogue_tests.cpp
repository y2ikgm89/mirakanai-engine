// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/quest_dialogue.hpp"

#include <span>
#include <string>
#include <vector>

namespace {

[[nodiscard]] mirakana::runtime::RuntimeQuestDialogueDocument valid_quest_dialogue_document() {
    using namespace mirakana::runtime;

    return RuntimeQuestDialogueDocument{
        .flags = {"story.met_elder"},
        .quests =
            std::vector<RuntimeQuestDesc>{
                RuntimeQuestDesc{
                    .id = "quest.intro",
                    .title_localization_key = "quest.intro.title",
                    .prerequisites = {},
                    .objectives =
                        std::vector<RuntimeQuestObjectiveDesc>{
                            RuntimeQuestObjectiveDesc{
                                .id = "talk_elder",
                                .localization_key = "quest.intro.objective.talk_elder",
                                .prerequisites = {},
                                .reward_ids = {"xp_small"},
                            },
                            RuntimeQuestObjectiveDesc{
                                .id = "find_gate",
                                .localization_key = "quest.intro.objective.find_gate",
                                .prerequisites =
                                    std::vector<RuntimeQuestPrerequisite>{
                                        RuntimeQuestPrerequisite{
                                            .kind = RuntimeQuestPrerequisiteKind::objective_completed,
                                            .quest_id = "quest.intro",
                                            .objective_id = "talk_elder",
                                            .flag_id = {},
                                        },
                                    },
                                .reward_ids = {},
                            },
                        },
                    .reward_ids = {"story_unlock"},
                },
            },
        .dialogues =
            std::vector<RuntimeDialogueGraphDesc>{
                RuntimeDialogueGraphDesc{
                    .id = "dialogue.elder",
                    .root_node_id = "start",
                    .nodes =
                        std::vector<RuntimeDialogueNodeDesc>{
                            RuntimeDialogueNodeDesc{
                                .id = "start",
                                .localization_key = "dialogue.elder.start",
                                .choices =
                                    std::vector<RuntimeDialogueChoiceDesc>{
                                        RuntimeDialogueChoiceDesc{
                                            .id = "accept",
                                            .localization_key = "dialogue.elder.accept",
                                            .next_node_id = "accepted",
                                            .prerequisites =
                                                std::vector<RuntimeQuestPrerequisite>{
                                                    RuntimeQuestPrerequisite{
                                                        .kind = RuntimeQuestPrerequisiteKind::flag_set,
                                                        .quest_id = {},
                                                        .objective_id = {},
                                                        .flag_id = "story.met_elder",
                                                    },
                                                },
                                            .action_ids = {"open_quest_intro"},
                                        },
                                    },
                                .action_ids = {},
                            },
                            RuntimeDialogueNodeDesc{
                                .id = "accepted",
                                .localization_key = "dialogue.elder.accepted",
                                .choices = {},
                                .action_ids = {"close_dialogue"},
                            },
                        },
                },
            },
    };
}

[[nodiscard]] mirakana::runtime::RuntimeQuestDialogueValidationContext validation_context() {
    using namespace mirakana::runtime;

    static const std::vector<std::string> localization_keys{
        "quest.intro.title",
        "quest.intro.objective.talk_elder",
        "quest.intro.objective.find_gate",
        "dialogue.elder.start",
        "dialogue.elder.accept",
        "dialogue.elder.accepted",
    };
    static const std::vector<std::string> reward_ids{"xp_small", "story_unlock"};
    static const std::vector<std::string> action_ids{"open_quest_intro", "close_dialogue"};

    return RuntimeQuestDialogueValidationContext{
        .localization_keys = std::span<const std::string>{localization_keys},
        .supported_reward_ids = std::span<const std::string>{reward_ids},
        .supported_action_ids = std::span<const std::string>{action_ids},
    };
}

} // namespace

MK_TEST("runtime quest dialogue document validates deterministic rows") {
    const auto document = valid_quest_dialogue_document();

    const auto first = mirakana::runtime::validate_runtime_quest_dialogue_document(document, validation_context());
    const auto second = mirakana::runtime::validate_runtime_quest_dialogue_document(document, validation_context());

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.rows.size() == 7U);
    MK_REQUIRE(first.rows[0].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::flag);
    MK_REQUIRE(first.rows[0].id == "story.met_elder");
    MK_REQUIRE(first.rows[1].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::quest);
    MK_REQUIRE(first.rows[1].id == "quest.intro");
    MK_REQUIRE(first.rows[2].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::objective);
    MK_REQUIRE(first.rows[2].parent_id == "quest.intro");
    MK_REQUIRE(first.rows[2].id == "talk_elder");
    MK_REQUIRE(first.rows[3].id == "find_gate");
    MK_REQUIRE(first.rows[4].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::dialogue);
    MK_REQUIRE(first.rows[5].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::dialogue_node);
    MK_REQUIRE(first.rows[6].kind == mirakana::runtime::RuntimeQuestDialogueValidationRowKind::dialogue_node);
}

MK_TEST("runtime quest dialogue document reports invalid ids references and unsupported rows deterministically") {
    using Code = mirakana::runtime::RuntimeQuestDialogueDiagnosticCode;
    using Kind = mirakana::runtime::RuntimeQuestPrerequisiteKind;
    using namespace mirakana::runtime;

    auto document = valid_quest_dialogue_document();
    document.flags.push_back("story.met_elder");
    document.quests.push_back(document.quests.front());
    document.quests.front().title_localization_key = "quest intro unsafe key";
    document.quests.front().objectives.push_back(document.quests.front().objectives.front());
    document.quests.front().objectives[1].prerequisites.push_back(RuntimeQuestPrerequisite{
        .kind = Kind::objective_completed, .quest_id = "quest.missing", .objective_id = "missing", .flag_id = {}});
    document.quests.front().objectives[1].reward_ids.push_back("unknown_reward");
    document.quests.front().objectives[1].localization_key = "quest.intro.objective.missing_key";
    document.dialogues.front().nodes.push_back(document.dialogues.front().nodes.front());
    document.dialogues.front().nodes.front().choices.front().next_node_id = "missing_node";
    document.dialogues.front().nodes.front().choices.front().action_ids.push_back("unknown_action");
    document.dialogues.front().nodes.front().choices.front().prerequisites.push_back(
        RuntimeQuestPrerequisite{.kind = Kind::flag_set, .quest_id = {}, .objective_id = {}, .flag_id = {}});

    const auto first = validate_runtime_quest_dialogue_document(document, validation_context());
    const auto second = validate_runtime_quest_dialogue_document(document, validation_context());

    MK_REQUIRE(!first.succeeded);
    MK_REQUIRE(first.diagnostics == second.diagnostics);
    MK_REQUIRE(first.rows.empty());
    MK_REQUIRE(first.diagnostics.size() == 11U);
    MK_REQUIRE(first.diagnostics[0].code == Code::duplicate_flag_id);
    MK_REQUIRE(first.diagnostics[0].flag_id == "story.met_elder");
    MK_REQUIRE(first.diagnostics[1].code == Code::duplicate_quest_id);
    MK_REQUIRE(first.diagnostics[1].quest_id == "quest.intro");
    MK_REQUIRE(first.diagnostics[2].code == Code::unsafe_localization_key);
    MK_REQUIRE(first.diagnostics[2].localization_key == "quest intro unsafe key");
    MK_REQUIRE(first.diagnostics[3].code == Code::duplicate_objective_id);
    MK_REQUIRE(first.diagnostics[3].objective_id == "talk_elder");
    MK_REQUIRE(first.diagnostics[4].code == Code::missing_localization_key);
    MK_REQUIRE(first.diagnostics[4].localization_key == "quest.intro.objective.missing_key");
    MK_REQUIRE(first.diagnostics[5].code == Code::missing_objective_reference);
    MK_REQUIRE(first.diagnostics[5].referenced_quest_id == "quest.missing");
    MK_REQUIRE(first.diagnostics[6].code == Code::unsupported_reward_id);
    MK_REQUIRE(first.diagnostics[6].reward_id == "unknown_reward");
    MK_REQUIRE(first.diagnostics[7].code == Code::duplicate_dialogue_node_id);
    MK_REQUIRE(first.diagnostics[7].dialogue_node_id == "start");
    MK_REQUIRE(first.diagnostics[8].code == Code::missing_dialogue_node_reference);
    MK_REQUIRE(first.diagnostics[8].referenced_dialogue_node_id == "missing_node");
    MK_REQUIRE(first.diagnostics[9].code == Code::invalid_prerequisite);
    MK_REQUIRE(first.diagnostics[10].code == Code::unsupported_action_id);
    MK_REQUIRE(first.diagnostics[10].action_id == "unknown_action");
}

MK_TEST("runtime quest dialogue state transitions classify accepted blocked ignored completed and invalid rows") {
    using Kind = mirakana::runtime::RuntimeQuestDialogueTransitionKind;
    using Status = mirakana::runtime::RuntimeQuestDialogueTransitionStatus;
    using namespace mirakana::runtime;

    const auto document = valid_quest_dialogue_document();
    RuntimeQuestDialogueState state;

    const auto blocked_objective = advance_runtime_quest_dialogue_state(document, state,
                                                                        RuntimeQuestDialogueTransitionRequest{
                                                                            .kind = Kind::complete_objective,
                                                                            .quest_id = "quest.intro",
                                                                            .objective_id = "find_gate",
                                                                        },
                                                                        validation_context());
    MK_REQUIRE(!blocked_objective.succeeded);
    MK_REQUIRE(blocked_objective.rows.size() == 1U);
    MK_REQUIRE(blocked_objective.rows[0].status == Status::blocked);
    MK_REQUIRE(blocked_objective.state == state);

    const auto completed_objective = advance_runtime_quest_dialogue_state(document, state,
                                                                          RuntimeQuestDialogueTransitionRequest{
                                                                              .kind = Kind::complete_objective,
                                                                              .quest_id = "quest.intro",
                                                                              .objective_id = "talk_elder",
                                                                          },
                                                                          validation_context());
    MK_REQUIRE(completed_objective.succeeded);
    MK_REQUIRE(completed_objective.rows[0].status == Status::completed);
    MK_REQUIRE(completed_objective.reward_ids == std::vector<std::string>{"xp_small"});
    MK_REQUIRE(completed_objective.state.completed_objectives.size() == 1U);
    state = completed_objective.state;

    const auto completed_quest = advance_runtime_quest_dialogue_state(document, state,
                                                                      RuntimeQuestDialogueTransitionRequest{
                                                                          .kind = Kind::complete_objective,
                                                                          .quest_id = "quest.intro",
                                                                          .objective_id = "find_gate",
                                                                      },
                                                                      validation_context());
    MK_REQUIRE(completed_quest.succeeded);
    MK_REQUIRE(completed_quest.rows[0].status == Status::completed);
    MK_REQUIRE(completed_quest.reward_ids == std::vector<std::string>{"story_unlock"});
    MK_REQUIRE(completed_quest.state.completed_objectives.size() == 2U);
    state = completed_quest.state;

    const auto accepted_flag = advance_runtime_quest_dialogue_state(document, state,
                                                                    RuntimeQuestDialogueTransitionRequest{
                                                                        .kind = Kind::set_flag,
                                                                        .flag_id = "story.met_elder",
                                                                    },
                                                                    validation_context());
    MK_REQUIRE(accepted_flag.succeeded);
    MK_REQUIRE(accepted_flag.rows[0].status == Status::accepted);
    MK_REQUIRE(accepted_flag.state.flags_set.size() == 1U);
    state = accepted_flag.state;

    const auto accepted_choice = advance_runtime_quest_dialogue_state(document, state,
                                                                      RuntimeQuestDialogueTransitionRequest{
                                                                          .kind = Kind::choose_dialogue,
                                                                          .dialogue_id = "dialogue.elder",
                                                                          .dialogue_choice_id = "accept",
                                                                      },
                                                                      validation_context());
    MK_REQUIRE(accepted_choice.succeeded);
    MK_REQUIRE(accepted_choice.rows[0].status == Status::accepted);
    MK_REQUIRE(accepted_choice.rows[0].dialogue_node_id == "start");
    MK_REQUIRE(accepted_choice.rows[0].referenced_dialogue_node_id == "accepted");
    const auto expected_dialogue_actions = std::vector<std::string>{"open_quest_intro", "close_dialogue"};
    MK_REQUIRE(accepted_choice.action_ids == expected_dialogue_actions);
    MK_REQUIRE(accepted_choice.state.dialogue_nodes.size() == 1U);
    state = accepted_choice.state;

    const auto ignored_flag = advance_runtime_quest_dialogue_state(document, state,
                                                                   RuntimeQuestDialogueTransitionRequest{
                                                                       .kind = Kind::set_flag,
                                                                       .flag_id = "story.met_elder",
                                                                   },
                                                                   validation_context());
    MK_REQUIRE(ignored_flag.succeeded);
    MK_REQUIRE(ignored_flag.rows[0].status == Status::ignored);
    MK_REQUIRE(ignored_flag.state == state);

    const auto invalid_objective = advance_runtime_quest_dialogue_state(document, state,
                                                                        RuntimeQuestDialogueTransitionRequest{
                                                                            .kind = Kind::complete_objective,
                                                                            .quest_id = "quest.intro",
                                                                            .objective_id = "missing",
                                                                        },
                                                                        validation_context());
    MK_REQUIRE(!invalid_objective.succeeded);
    MK_REQUIRE(invalid_objective.rows[0].status == Status::invalid);
    MK_REQUIRE(invalid_objective.state == state);
}

MK_TEST("runtime quest dialogue save state validation reports deterministic rows and diagnostics") {
    using Code = mirakana::runtime::RuntimeQuestDialogueStateDiagnosticCode;
    using Kind = mirakana::runtime::RuntimeQuestDialogueStateRowKind;
    using namespace mirakana::runtime;

    const auto document = valid_quest_dialogue_document();
    const RuntimeQuestDialogueState state{
        .flags_set = {"story.met_elder"},
        .completed_objectives = {RuntimeQuestDialogueObjectiveState{.quest_id = "quest.intro",
                                                                    .objective_id = "talk_elder"}},
        .dialogue_nodes = {RuntimeQuestDialogueNodeState{.dialogue_id = "dialogue.elder", .node_id = "accepted"}},
    };

    const auto first = validate_runtime_quest_dialogue_state(document, state, validation_context());
    const auto second = validate_runtime_quest_dialogue_state(document, state, validation_context());

    MK_REQUIRE(first.succeeded);
    MK_REQUIRE(first.diagnostics.empty());
    MK_REQUIRE(first.rows == second.rows);
    MK_REQUIRE(first.rows.size() == 3U);
    MK_REQUIRE(first.rows[0].kind == Kind::flag);
    MK_REQUIRE(first.rows[0].flag_id == "story.met_elder");
    MK_REQUIRE(first.rows[1].kind == Kind::completed_objective);
    MK_REQUIRE(first.rows[1].quest_id == "quest.intro");
    MK_REQUIRE(first.rows[1].objective_id == "talk_elder");
    MK_REQUIRE(first.rows[2].kind == Kind::dialogue_node);
    MK_REQUIRE(first.rows[2].dialogue_id == "dialogue.elder");
    MK_REQUIRE(first.rows[2].dialogue_node_id == "accepted");

    RuntimeQuestDialogueState invalid_state = state;
    invalid_state.flags_set.push_back("story.met_elder");
    invalid_state.flags_set.push_back("story.missing");
    invalid_state.completed_objectives.push_back(
        RuntimeQuestDialogueObjectiveState{.quest_id = "quest.intro", .objective_id = "talk_elder"});
    invalid_state.completed_objectives.push_back(
        RuntimeQuestDialogueObjectiveState{.quest_id = "quest.intro", .objective_id = "missing"});
    invalid_state.dialogue_nodes.front().node_id = "missing";
    invalid_state.dialogue_nodes.push_back(
        RuntimeQuestDialogueNodeState{.dialogue_id = "dialogue.elder", .node_id = "accepted"});
    invalid_state.dialogue_nodes.push_back(
        RuntimeQuestDialogueNodeState{.dialogue_id = "missing.dialogue", .node_id = "start"});

    const auto invalid_first = validate_runtime_quest_dialogue_state(document, invalid_state, validation_context());
    const auto invalid_second = validate_runtime_quest_dialogue_state(document, invalid_state, validation_context());

    MK_REQUIRE(!invalid_first.succeeded);
    MK_REQUIRE(invalid_first.rows.empty());
    MK_REQUIRE(invalid_first.diagnostics == invalid_second.diagnostics);
    MK_REQUIRE(invalid_first.diagnostics.size() == 7U);
    MK_REQUIRE(invalid_first.diagnostics[0].code == Code::duplicate_flag);
    MK_REQUIRE(invalid_first.diagnostics[0].flag_id == "story.met_elder");
    MK_REQUIRE(invalid_first.diagnostics[1].code == Code::missing_flag);
    MK_REQUIRE(invalid_first.diagnostics[1].flag_id == "story.missing");
    MK_REQUIRE(invalid_first.diagnostics[2].code == Code::duplicate_completed_objective);
    MK_REQUIRE(invalid_first.diagnostics[2].objective_id == "talk_elder");
    MK_REQUIRE(invalid_first.diagnostics[3].code == Code::missing_objective);
    MK_REQUIRE(invalid_first.diagnostics[3].objective_id == "missing");
    MK_REQUIRE(invalid_first.diagnostics[4].code == Code::missing_dialogue_node);
    MK_REQUIRE(invalid_first.diagnostics[4].dialogue_id == "dialogue.elder");
    MK_REQUIRE(invalid_first.diagnostics[4].dialogue_node_id == "missing");
    MK_REQUIRE(invalid_first.diagnostics[5].code == Code::duplicate_dialogue_node);
    MK_REQUIRE(invalid_first.diagnostics[5].dialogue_id == "dialogue.elder");
    MK_REQUIRE(invalid_first.diagnostics[6].code == Code::missing_dialogue);
    MK_REQUIRE(invalid_first.diagnostics[6].dialogue_id == "missing.dialogue");
}

int main() {
    return mirakana::test::run_all();
}
