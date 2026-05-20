// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/quest_dialogue.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool contains_control_character(const std::string_view value) noexcept {
    return std::ranges::any_of(value, [](const char character) {
        const auto byte = static_cast<unsigned char>(character);
        return byte < 0x20U || byte == 0x7FU;
    });
}

[[nodiscard]] bool is_safe_token_character(const char character) noexcept {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') ||
           (character >= '0' && character <= '9') || character == '_' || character == '-' || character == '.' ||
           character == ':' || character == '/';
}

[[nodiscard]] bool is_safe_token(const std::string_view value) noexcept {
    return !value.empty() && !contains_control_character(value) && std::ranges::all_of(value, is_safe_token_character);
}

[[nodiscard]] bool contains_string(const std::span<const std::string> values, const std::string_view value) noexcept {
    return std::ranges::find_if(values, [value](const std::string& candidate) {
               return std::string_view{candidate} == value;
           }) != values.end();
}

[[nodiscard]] bool has_flag(const RuntimeQuestDialogueDocument& document, const std::string_view flag_id) noexcept {
    return std::ranges::find(document.flags, flag_id) != document.flags.end();
}

[[nodiscard]] const RuntimeQuestDesc* find_quest(const RuntimeQuestDialogueDocument& document,
                                                 const std::string_view quest_id) noexcept {
    const auto match = std::ranges::find_if(document.quests,
                                            [quest_id](const RuntimeQuestDesc& quest) { return quest.id == quest_id; });
    return match == document.quests.end() ? nullptr : std::to_address(match);
}

[[nodiscard]] const RuntimeQuestObjectiveDesc* find_objective(const RuntimeQuestDesc& quest,
                                                              const std::string_view objective_id) noexcept {
    const auto match = std::ranges::find_if(
        quest.objectives, [objective_id](const RuntimeQuestObjectiveDesc& row) { return row.id == objective_id; });
    return match == quest.objectives.end() ? nullptr : std::to_address(match);
}

[[nodiscard]] const RuntimeDialogueGraphDesc* find_dialogue(const RuntimeQuestDialogueDocument& document,
                                                            const std::string_view dialogue_id) noexcept {
    const auto match = std::ranges::find_if(
        document.dialogues, [dialogue_id](const RuntimeDialogueGraphDesc& row) { return row.id == dialogue_id; });
    return match == document.dialogues.end() ? nullptr : std::to_address(match);
}

[[nodiscard]] const RuntimeDialogueNodeDesc* find_dialogue_node(const RuntimeDialogueGraphDesc& dialogue,
                                                                const std::string_view node_id) noexcept {
    const auto match = std::ranges::find_if(
        dialogue.nodes, [node_id](const RuntimeDialogueNodeDesc& node) { return node.id == node_id; });
    return match == dialogue.nodes.end() ? nullptr : std::to_address(match);
}

[[nodiscard]] const RuntimeDialogueChoiceDesc* find_dialogue_choice(const RuntimeDialogueNodeDesc& node,
                                                                    const std::string_view choice_id) noexcept {
    const auto match = std::ranges::find_if(
        node.choices, [choice_id](const RuntimeDialogueChoiceDesc& choice) { return choice.id == choice_id; });
    return match == node.choices.end() ? nullptr : std::to_address(match);
}

void add_diagnostic(std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics,
                    RuntimeQuestDialogueDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

template <typename T, typename Getter>
void validate_duplicate_ids(const std::vector<T>& rows, Getter getter,
                            std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics,
                            const RuntimeQuestDialogueDiagnosticCode code, const std::string& quest_id = {},
                            const std::string& dialogue_id = {}) {
    for (auto first = rows.begin(); first != rows.end(); ++first) {
        const std::string_view first_id{getter(*first)};
        if (first_id.empty()) {
            continue;
        }
        for (auto second = std::next(first); second != rows.end(); ++second) {
            if (getter(*second) != first_id) {
                continue;
            }

            RuntimeQuestDialogueDiagnostic diagnostic{.code = code};
            diagnostic.quest_id = quest_id;
            diagnostic.dialogue_id = dialogue_id;
            switch (code) {
            case RuntimeQuestDialogueDiagnosticCode::duplicate_flag_id:
                diagnostic.flag_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::duplicate_quest_id:
                diagnostic.quest_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::duplicate_objective_id:
                diagnostic.objective_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_id:
                diagnostic.dialogue_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_node_id:
                diagnostic.dialogue_node_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_choice_id:
                diagnostic.dialogue_choice_id = first_id;
                break;
            case RuntimeQuestDialogueDiagnosticCode::none:
            case RuntimeQuestDialogueDiagnosticCode::invalid_flag_id:
            case RuntimeQuestDialogueDiagnosticCode::invalid_quest_id:
            case RuntimeQuestDialogueDiagnosticCode::invalid_objective_id:
            case RuntimeQuestDialogueDiagnosticCode::missing_objective_reference:
            case RuntimeQuestDialogueDiagnosticCode::invalid_prerequisite:
            case RuntimeQuestDialogueDiagnosticCode::unsupported_reward_id:
            case RuntimeQuestDialogueDiagnosticCode::unsafe_localization_key:
            case RuntimeQuestDialogueDiagnosticCode::missing_localization_key:
            case RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_id:
            case RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_node_id:
            case RuntimeQuestDialogueDiagnosticCode::missing_dialogue_node_reference:
            case RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_choice_id:
            case RuntimeQuestDialogueDiagnosticCode::unsupported_action_id:
                break;
            }
            add_diagnostic(diagnostics, std::move(diagnostic));
            return;
        }
    }
}

template <typename T, typename Getter>
[[nodiscard]] bool id_seen_before(const std::vector<T>& rows, const T& current, Getter getter) noexcept {
    for (const auto& row : rows) {
        if (std::addressof(row) == std::addressof(current)) {
            return false;
        }
        if (getter(row) == getter(current)) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool state_has_flag(const RuntimeQuestDialogueState& state, const std::string_view flag_id) noexcept {
    return std::ranges::find(state.flags_set, flag_id) != state.flags_set.end();
}

[[nodiscard]] bool state_has_completed_objective(const RuntimeQuestDialogueState& state,
                                                 const std::string_view quest_id,
                                                 const std::string_view objective_id) noexcept {
    return std::ranges::find_if(state.completed_objectives,
                                [quest_id, objective_id](const RuntimeQuestDialogueObjectiveState& row) {
                                    return row.quest_id == quest_id && row.objective_id == objective_id;
                                }) != state.completed_objectives.end();
}

[[nodiscard]] const RuntimeQuestDialogueNodeState*
find_dialogue_node_state(const RuntimeQuestDialogueState& state, const std::string_view dialogue_id) noexcept {
    const auto match =
        std::ranges::find_if(state.dialogue_nodes, [dialogue_id](const RuntimeQuestDialogueNodeState& row) {
            return row.dialogue_id == dialogue_id;
        });
    return match == state.dialogue_nodes.end() ? nullptr : std::to_address(match);
}

void set_dialogue_node_state(RuntimeQuestDialogueState& state, const std::string& dialogue_id,
                             const std::string& node_id) {
    const auto match =
        std::ranges::find_if(state.dialogue_nodes, [&dialogue_id](const RuntimeQuestDialogueNodeState& row) {
            return row.dialogue_id == dialogue_id;
        });
    if (match == state.dialogue_nodes.end()) {
        state.dialogue_nodes.push_back(RuntimeQuestDialogueNodeState{.dialogue_id = dialogue_id, .node_id = node_id});
        return;
    }
    match->node_id = node_id;
}

[[nodiscard]] bool prerequisites_satisfied(const RuntimeQuestDialogueState& state,
                                           const std::span<const RuntimeQuestPrerequisite> prerequisites) noexcept {
    return std::ranges::all_of(prerequisites, [&state](const RuntimeQuestPrerequisite& prerequisite) {
        switch (prerequisite.kind) {
        case RuntimeQuestPrerequisiteKind::flag_set:
            return state_has_flag(state, prerequisite.flag_id);
        case RuntimeQuestPrerequisiteKind::objective_completed:
            return state_has_completed_objective(state, prerequisite.quest_id, prerequisite.objective_id);
        }
        return false;
    });
}

void add_transition_diagnostic(std::vector<RuntimeQuestDialogueTransitionDiagnostic>& diagnostics,
                               RuntimeQuestDialogueTransitionDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void add_transition_row(std::vector<RuntimeQuestDialogueTransitionRow>& rows, RuntimeQuestDialogueTransitionRow row) {
    rows.push_back(std::move(row));
}

void add_state_diagnostic(std::vector<RuntimeQuestDialogueStateDiagnostic>& diagnostics,
                          RuntimeQuestDialogueStateDiagnostic diagnostic) {
    diagnostics.push_back(std::move(diagnostic));
}

void add_state_row(std::vector<RuntimeQuestDialogueStateRow>& rows, RuntimeQuestDialogueStateRow row) {
    rows.push_back(std::move(row));
}

[[nodiscard]] bool contains_completed_objective(const std::span<const RuntimeQuestDialogueObjectiveState> rows,
                                                const RuntimeQuestDialogueObjectiveState& row) {
    return std::ranges::find(rows, row) != rows.end();
}

[[nodiscard]] bool contains_dialogue_state_for_dialogue(const std::span<const RuntimeQuestDialogueNodeState> rows,
                                                        const std::string_view dialogue_id) {
    return std::ranges::find_if(rows, [dialogue_id](const RuntimeQuestDialogueNodeState& row) {
               return row.dialogue_id == dialogue_id;
           }) != rows.end();
}

[[nodiscard]] bool all_quest_objectives_completed(const RuntimeQuestDesc& quest,
                                                  const RuntimeQuestDialogueState& state) noexcept {
    return std::ranges::all_of(quest.objectives, [&quest, &state](const RuntimeQuestObjectiveDesc& objective) {
        return state_has_completed_objective(state, quest.id, objective.id);
    });
}

void validate_localization_key(std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics, std::string quest_id,
                               std::string objective_id, std::string dialogue_id, std::string dialogue_node_id,
                               std::string dialogue_choice_id, const std::string& localization_key,
                               const RuntimeQuestDialogueValidationContext context) {
    if (!is_safe_token(localization_key)) {
        add_diagnostic(diagnostics, RuntimeQuestDialogueDiagnostic{
                                        .code = RuntimeQuestDialogueDiagnosticCode::unsafe_localization_key,
                                        .quest_id = std::move(quest_id),
                                        .objective_id = std::move(objective_id),
                                        .dialogue_id = std::move(dialogue_id),
                                        .dialogue_node_id = std::move(dialogue_node_id),
                                        .dialogue_choice_id = std::move(dialogue_choice_id),
                                        .localization_key = localization_key});
        return;
    }
    if (!contains_string(context.localization_keys, localization_key)) {
        add_diagnostic(diagnostics, RuntimeQuestDialogueDiagnostic{
                                        .code = RuntimeQuestDialogueDiagnosticCode::missing_localization_key,
                                        .quest_id = std::move(quest_id),
                                        .objective_id = std::move(objective_id),
                                        .dialogue_id = std::move(dialogue_id),
                                        .dialogue_node_id = std::move(dialogue_node_id),
                                        .dialogue_choice_id = std::move(dialogue_choice_id),
                                        .localization_key = localization_key});
    }
}

void validate_prerequisite(std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics,
                           const RuntimeQuestDialogueDocument& document, const RuntimeQuestPrerequisite& prerequisite,
                           std::string quest_id, std::string objective_id, std::string dialogue_id,
                           std::string dialogue_node_id, std::string dialogue_choice_id) {
    switch (prerequisite.kind) {
    case RuntimeQuestPrerequisiteKind::flag_set:
        if (!is_safe_token(prerequisite.flag_id) || !has_flag(document, prerequisite.flag_id)) {
            add_diagnostic(diagnostics, RuntimeQuestDialogueDiagnostic{
                                            .code = RuntimeQuestDialogueDiagnosticCode::invalid_prerequisite,
                                            .quest_id = std::move(quest_id),
                                            .objective_id = std::move(objective_id),
                                            .dialogue_id = std::move(dialogue_id),
                                            .dialogue_node_id = std::move(dialogue_node_id),
                                            .dialogue_choice_id = std::move(dialogue_choice_id),
                                            .flag_id = prerequisite.flag_id});
        }
        return;
    case RuntimeQuestPrerequisiteKind::objective_completed: {
        const auto* const referenced_quest = find_quest(document, prerequisite.quest_id);
        if (!is_safe_token(prerequisite.quest_id) || !is_safe_token(prerequisite.objective_id) ||
            referenced_quest == nullptr || find_objective(*referenced_quest, prerequisite.objective_id) == nullptr) {
            add_diagnostic(diagnostics, RuntimeQuestDialogueDiagnostic{
                                            .code = RuntimeQuestDialogueDiagnosticCode::missing_objective_reference,
                                            .quest_id = std::move(quest_id),
                                            .objective_id = std::move(objective_id),
                                            .dialogue_id = std::move(dialogue_id),
                                            .dialogue_node_id = std::move(dialogue_node_id),
                                            .dialogue_choice_id = std::move(dialogue_choice_id),
                                            .referenced_quest_id = prerequisite.quest_id,
                                            .referenced_objective_id = prerequisite.objective_id});
        }
        return;
    }
    }
}

void validate_reward_ids(std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics, const std::string& quest_id,
                         const std::string& objective_id, const std::span<const std::string> reward_ids,
                         const RuntimeQuestDialogueValidationContext context) {
    for (const auto& reward_id : reward_ids) {
        if (contains_string(context.supported_reward_ids, reward_id)) {
            continue;
        }
        add_diagnostic(diagnostics,
                       RuntimeQuestDialogueDiagnostic{.code = RuntimeQuestDialogueDiagnosticCode::unsupported_reward_id,
                                                      .quest_id = quest_id,
                                                      .objective_id = objective_id,
                                                      .reward_id = reward_id});
    }
}

void validate_action_ids(std::vector<RuntimeQuestDialogueDiagnostic>& diagnostics, const std::string& dialogue_id,
                         const std::string& dialogue_node_id, const std::string& dialogue_choice_id,
                         const std::span<const std::string> action_ids,
                         const RuntimeQuestDialogueValidationContext context) {
    for (const auto& action_id : action_ids) {
        if (contains_string(context.supported_action_ids, action_id)) {
            continue;
        }
        add_diagnostic(diagnostics,
                       RuntimeQuestDialogueDiagnostic{.code = RuntimeQuestDialogueDiagnosticCode::unsupported_action_id,
                                                      .dialogue_id = dialogue_id,
                                                      .dialogue_node_id = dialogue_node_id,
                                                      .dialogue_choice_id = dialogue_choice_id,
                                                      .action_id = action_id});
    }
}

void append_success_rows(RuntimeQuestDialogueValidationResult& result, const RuntimeQuestDialogueDocument& document) {
    for (const auto& flag_id : document.flags) {
        result.rows.push_back(RuntimeQuestDialogueValidationRow{
            .kind = RuntimeQuestDialogueValidationRowKind::flag, .parent_id = {}, .id = flag_id});
    }
    for (const auto& quest : document.quests) {
        result.rows.push_back(RuntimeQuestDialogueValidationRow{
            .kind = RuntimeQuestDialogueValidationRowKind::quest, .parent_id = {}, .id = quest.id});
        for (const auto& objective : quest.objectives) {
            result.rows.push_back(RuntimeQuestDialogueValidationRow{
                .kind = RuntimeQuestDialogueValidationRowKind::objective,
                .parent_id = quest.id,
                .id = objective.id,
            });
        }
    }
    for (const auto& dialogue : document.dialogues) {
        result.rows.push_back(RuntimeQuestDialogueValidationRow{
            .kind = RuntimeQuestDialogueValidationRowKind::dialogue, .parent_id = {}, .id = dialogue.id});
        for (const auto& node : dialogue.nodes) {
            result.rows.push_back(RuntimeQuestDialogueValidationRow{
                .kind = RuntimeQuestDialogueValidationRowKind::dialogue_node,
                .parent_id = dialogue.id,
                .id = node.id,
            });
        }
    }
}

} // namespace

RuntimeQuestDialogueValidationResult
validate_runtime_quest_dialogue_document(const RuntimeQuestDialogueDocument& document,
                                         const RuntimeQuestDialogueValidationContext context) {
    RuntimeQuestDialogueValidationResult result;

    validate_duplicate_ids(
        document.flags, [](const std::string& flag) -> std::string_view { return flag; }, result.diagnostics,
        RuntimeQuestDialogueDiagnosticCode::duplicate_flag_id);
    for (const auto& flag_id : document.flags) {
        if (!is_safe_token(flag_id)) {
            add_diagnostic(result.diagnostics,
                           RuntimeQuestDialogueDiagnostic{.code = RuntimeQuestDialogueDiagnosticCode::invalid_flag_id,
                                                          .flag_id = flag_id});
        }
    }

    validate_duplicate_ids(
        document.quests, [](const RuntimeQuestDesc& quest) -> std::string_view { return quest.id; }, result.diagnostics,
        RuntimeQuestDialogueDiagnosticCode::duplicate_quest_id);
    for (const auto& quest : document.quests) {
        if (id_seen_before(document.quests, quest,
                           [](const RuntimeQuestDesc& row) -> std::string_view { return row.id; })) {
            continue;
        }
        if (!is_safe_token(quest.id)) {
            add_diagnostic(result.diagnostics,
                           RuntimeQuestDialogueDiagnostic{.code = RuntimeQuestDialogueDiagnosticCode::invalid_quest_id,
                                                          .quest_id = quest.id});
        }
        validate_localization_key(result.diagnostics, quest.id, {}, {}, {}, {}, quest.title_localization_key, context);
        validate_duplicate_ids(
            quest.objectives,
            [](const RuntimeQuestObjectiveDesc& objective) -> std::string_view { return objective.id; },
            result.diagnostics, RuntimeQuestDialogueDiagnosticCode::duplicate_objective_id, quest.id);
        for (const auto& prerequisite : quest.prerequisites) {
            validate_prerequisite(result.diagnostics, document, prerequisite, quest.id, {}, {}, {}, {});
        }
        for (const auto& objective : quest.objectives) {
            if (id_seen_before(quest.objectives, objective,
                               [](const RuntimeQuestObjectiveDesc& row) -> std::string_view { return row.id; })) {
                continue;
            }
            if (!is_safe_token(objective.id)) {
                add_diagnostic(result.diagnostics, RuntimeQuestDialogueDiagnostic{
                                                       .code = RuntimeQuestDialogueDiagnosticCode::invalid_objective_id,
                                                       .quest_id = quest.id,
                                                       .objective_id = objective.id});
            }
            validate_localization_key(result.diagnostics, quest.id, objective.id, {}, {}, {},
                                      objective.localization_key, context);
            for (const auto& prerequisite : objective.prerequisites) {
                validate_prerequisite(result.diagnostics, document, prerequisite, quest.id, objective.id, {}, {}, {});
            }
            validate_reward_ids(result.diagnostics, quest.id, objective.id, objective.reward_ids, context);
        }
        validate_reward_ids(result.diagnostics, quest.id, {}, quest.reward_ids, context);
    }

    validate_duplicate_ids(
        document.dialogues, [](const RuntimeDialogueGraphDesc& dialogue) -> std::string_view { return dialogue.id; },
        result.diagnostics, RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_id);
    for (const auto& dialogue : document.dialogues) {
        if (id_seen_before(document.dialogues, dialogue,
                           [](const RuntimeDialogueGraphDesc& row) -> std::string_view { return row.id; })) {
            continue;
        }
        if (!is_safe_token(dialogue.id)) {
            add_diagnostic(result.diagnostics, RuntimeQuestDialogueDiagnostic{
                                                   .code = RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_id,
                                                   .dialogue_id = dialogue.id});
        }
        validate_duplicate_ids(
            dialogue.nodes, [](const RuntimeDialogueNodeDesc& node) -> std::string_view { return node.id; },
            result.diagnostics, RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_node_id, {}, dialogue.id);
        if (find_dialogue_node(dialogue, dialogue.root_node_id) == nullptr) {
            add_diagnostic(result.diagnostics,
                           RuntimeQuestDialogueDiagnostic{
                               .code = RuntimeQuestDialogueDiagnosticCode::missing_dialogue_node_reference,
                               .dialogue_id = dialogue.id,
                               .referenced_dialogue_node_id = dialogue.root_node_id});
        }
        for (const auto& node : dialogue.nodes) {
            if (id_seen_before(dialogue.nodes, node,
                               [](const RuntimeDialogueNodeDesc& row) -> std::string_view { return row.id; })) {
                continue;
            }
            if (!is_safe_token(node.id)) {
                add_diagnostic(
                    result.diagnostics,
                    RuntimeQuestDialogueDiagnostic{.code = RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_node_id,
                                                   .dialogue_id = dialogue.id,
                                                   .dialogue_node_id = node.id});
            }
            validate_localization_key(result.diagnostics, {}, {}, dialogue.id, node.id, {}, node.localization_key,
                                      context);
            validate_duplicate_ids(
                node.choices, [](const RuntimeDialogueChoiceDesc& choice) -> std::string_view { return choice.id; },
                result.diagnostics, RuntimeQuestDialogueDiagnosticCode::duplicate_dialogue_choice_id, {}, dialogue.id);
            validate_action_ids(result.diagnostics, dialogue.id, node.id, {}, node.action_ids, context);
            for (const auto& choice : node.choices) {
                if (id_seen_before(node.choices, choice,
                                   [](const RuntimeDialogueChoiceDesc& row) -> std::string_view { return row.id; })) {
                    continue;
                }
                if (!is_safe_token(choice.id)) {
                    add_diagnostic(result.diagnostics,
                                   RuntimeQuestDialogueDiagnostic{
                                       .code = RuntimeQuestDialogueDiagnosticCode::invalid_dialogue_choice_id,
                                       .dialogue_id = dialogue.id,
                                       .dialogue_node_id = node.id,
                                       .dialogue_choice_id = choice.id});
                }
                validate_localization_key(result.diagnostics, {}, {}, dialogue.id, node.id, choice.id,
                                          choice.localization_key, context);
                if (!choice.next_node_id.empty() && find_dialogue_node(dialogue, choice.next_node_id) == nullptr) {
                    add_diagnostic(result.diagnostics,
                                   RuntimeQuestDialogueDiagnostic{
                                       .code = RuntimeQuestDialogueDiagnosticCode::missing_dialogue_node_reference,
                                       .dialogue_id = dialogue.id,
                                       .dialogue_node_id = node.id,
                                       .dialogue_choice_id = choice.id,
                                       .referenced_dialogue_node_id = choice.next_node_id});
                }
                for (const auto& prerequisite : choice.prerequisites) {
                    validate_prerequisite(result.diagnostics, document, prerequisite, {}, {}, dialogue.id, node.id,
                                          choice.id);
                }
                validate_action_ids(result.diagnostics, dialogue.id, node.id, choice.id, choice.action_ids, context);
            }
        }
    }

    result.succeeded = result.diagnostics.empty();
    if (result.succeeded) {
        append_success_rows(result, document);
    }
    return result;
}

RuntimeQuestDialogueStateValidationResult
validate_runtime_quest_dialogue_state(const RuntimeQuestDialogueDocument& document,
                                      const RuntimeQuestDialogueState& state,
                                      const RuntimeQuestDialogueValidationContext context) {
    RuntimeQuestDialogueStateValidationResult result;

    const auto document_validation = validate_runtime_quest_dialogue_document(document, context);
    if (!document_validation.succeeded) {
        result.succeeded = false;
        add_state_diagnostic(result.diagnostics, RuntimeQuestDialogueStateDiagnostic{
                                                     .code = RuntimeQuestDialogueStateDiagnosticCode::invalid_document,
                                                 });
        return result;
    }

    std::vector<std::string> seen_flags;
    for (const std::string& flag_id : state.flags_set) {
        if (std::ranges::find(seen_flags, flag_id) != seen_flags.end()) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::duplicate_flag,
                                     .flag_id = flag_id,
                                 });
            continue;
        }
        seen_flags.push_back(flag_id);
        if (!is_safe_token(flag_id) || !has_flag(document, flag_id)) {
            add_state_diagnostic(result.diagnostics, RuntimeQuestDialogueStateDiagnostic{
                                                         .code = RuntimeQuestDialogueStateDiagnosticCode::missing_flag,
                                                         .flag_id = flag_id,
                                                     });
            continue;
        }
        add_state_row(result.rows, RuntimeQuestDialogueStateRow{
                                       .kind = RuntimeQuestDialogueStateRowKind::flag,
                                       .flag_id = flag_id,
                                   });
    }

    std::vector<RuntimeQuestDialogueObjectiveState> seen_objectives;
    for (const RuntimeQuestDialogueObjectiveState& objective_state : state.completed_objectives) {
        if (contains_completed_objective(seen_objectives, objective_state)) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::duplicate_completed_objective,
                                     .quest_id = objective_state.quest_id,
                                     .objective_id = objective_state.objective_id,
                                 });
            continue;
        }
        seen_objectives.push_back(objective_state);
        const auto* const quest =
            is_safe_token(objective_state.quest_id) ? find_quest(document, objective_state.quest_id) : nullptr;
        if (quest == nullptr) {
            add_state_diagnostic(result.diagnostics, RuntimeQuestDialogueStateDiagnostic{
                                                         .code = RuntimeQuestDialogueStateDiagnosticCode::missing_quest,
                                                         .quest_id = objective_state.quest_id,
                                                         .objective_id = objective_state.objective_id,
                                                     });
            continue;
        }
        const auto* const objective = is_safe_token(objective_state.objective_id)
                                          ? find_objective(*quest, objective_state.objective_id)
                                          : nullptr;
        if (objective == nullptr) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::missing_objective,
                                     .quest_id = objective_state.quest_id,
                                     .objective_id = objective_state.objective_id,
                                 });
            continue;
        }
        add_state_row(result.rows, RuntimeQuestDialogueStateRow{
                                       .kind = RuntimeQuestDialogueStateRowKind::completed_objective,
                                       .quest_id = objective_state.quest_id,
                                       .objective_id = objective_state.objective_id,
                                   });
    }

    std::vector<RuntimeQuestDialogueNodeState> seen_dialogue_nodes;
    for (const RuntimeQuestDialogueNodeState& node_state : state.dialogue_nodes) {
        if (contains_dialogue_state_for_dialogue(seen_dialogue_nodes, node_state.dialogue_id)) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::duplicate_dialogue_node,
                                     .dialogue_id = node_state.dialogue_id,
                                     .dialogue_node_id = node_state.node_id,
                                 });
            continue;
        }
        seen_dialogue_nodes.push_back(node_state);
        const auto* const dialogue =
            is_safe_token(node_state.dialogue_id) ? find_dialogue(document, node_state.dialogue_id) : nullptr;
        if (dialogue == nullptr) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::missing_dialogue,
                                     .dialogue_id = node_state.dialogue_id,
                                     .dialogue_node_id = node_state.node_id,
                                 });
            continue;
        }
        const auto* const node =
            is_safe_token(node_state.node_id) ? find_dialogue_node(*dialogue, node_state.node_id) : nullptr;
        if (node == nullptr) {
            add_state_diagnostic(result.diagnostics,
                                 RuntimeQuestDialogueStateDiagnostic{
                                     .code = RuntimeQuestDialogueStateDiagnosticCode::missing_dialogue_node,
                                     .dialogue_id = node_state.dialogue_id,
                                     .dialogue_node_id = node_state.node_id,
                                 });
            continue;
        }
        add_state_row(result.rows, RuntimeQuestDialogueStateRow{
                                       .kind = RuntimeQuestDialogueStateRowKind::dialogue_node,
                                       .dialogue_id = node_state.dialogue_id,
                                       .dialogue_node_id = node_state.node_id,
                                   });
    }

    if (!result.diagnostics.empty()) {
        result.succeeded = false;
        result.rows.clear();
    }

    return result;
}

RuntimeQuestDialogueTransitionResult advance_runtime_quest_dialogue_state(
    const RuntimeQuestDialogueDocument& document, const RuntimeQuestDialogueState& state,
    const RuntimeQuestDialogueTransitionRequest& request, const RuntimeQuestDialogueValidationContext context) {
    RuntimeQuestDialogueTransitionResult result{.succeeded = true, .state = state};

    const auto validation = validate_runtime_quest_dialogue_document(document, context);
    if (!validation.succeeded) {
        result.succeeded = false;
        add_transition_row(result.rows,
                           RuntimeQuestDialogueTransitionRow{.kind = request.kind,
                                                             .status = RuntimeQuestDialogueTransitionStatus::invalid,
                                                             .quest_id = request.quest_id,
                                                             .objective_id = request.objective_id,
                                                             .dialogue_id = request.dialogue_id,
                                                             .dialogue_choice_id = request.dialogue_choice_id,
                                                             .flag_id = request.flag_id});
        add_transition_diagnostic(result.diagnostics,
                                  RuntimeQuestDialogueTransitionDiagnostic{
                                      .code = RuntimeQuestDialogueTransitionDiagnosticCode::invalid_document,
                                      .quest_id = request.quest_id,
                                      .objective_id = request.objective_id,
                                      .dialogue_id = request.dialogue_id,
                                      .dialogue_choice_id = request.dialogue_choice_id,
                                      .flag_id = request.flag_id});
        return result;
    }

    const auto state_validation = validate_runtime_quest_dialogue_state(document, state, context);
    if (!state_validation.succeeded) {
        result.succeeded = false;
        add_transition_row(result.rows,
                           RuntimeQuestDialogueTransitionRow{.kind = request.kind,
                                                             .status = RuntimeQuestDialogueTransitionStatus::invalid,
                                                             .quest_id = request.quest_id,
                                                             .objective_id = request.objective_id,
                                                             .dialogue_id = request.dialogue_id,
                                                             .dialogue_choice_id = request.dialogue_choice_id,
                                                             .flag_id = request.flag_id});
        add_transition_diagnostic(result.diagnostics,
                                  RuntimeQuestDialogueTransitionDiagnostic{
                                      .code = RuntimeQuestDialogueTransitionDiagnosticCode::invalid_state,
                                      .quest_id = request.quest_id,
                                      .objective_id = request.objective_id,
                                      .dialogue_id = request.dialogue_id,
                                      .dialogue_choice_id = request.dialogue_choice_id,
                                      .flag_id = request.flag_id});
        return result;
    }

    switch (request.kind) {
    case RuntimeQuestDialogueTransitionKind::set_flag: {
        RuntimeQuestDialogueTransitionRow row{
            .kind = request.kind,
            .status = RuntimeQuestDialogueTransitionStatus::accepted,
            .flag_id = request.flag_id,
        };
        if (!is_safe_token(request.flag_id) || !has_flag(document, request.flag_id)) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(
                result.diagnostics,
                RuntimeQuestDialogueTransitionDiagnostic{
                    .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_flag, .flag_id = request.flag_id});
            return result;
        }
        if (state_has_flag(state, request.flag_id)) {
            row.status = RuntimeQuestDialogueTransitionStatus::ignored;
            add_transition_row(result.rows, std::move(row));
            return result;
        }
        result.state.flags_set.push_back(request.flag_id);
        add_transition_row(result.rows, std::move(row));
        return result;
    }
    case RuntimeQuestDialogueTransitionKind::complete_objective: {
        RuntimeQuestDialogueTransitionRow row{
            .kind = request.kind,
            .status = RuntimeQuestDialogueTransitionStatus::completed,
            .quest_id = request.quest_id,
            .objective_id = request.objective_id,
        };
        const auto* const quest = is_safe_token(request.quest_id) ? find_quest(document, request.quest_id) : nullptr;
        if (quest == nullptr) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_quest,
                                          .quest_id = request.quest_id,
                                          .objective_id = request.objective_id});
            return result;
        }
        const auto* const objective =
            is_safe_token(request.objective_id) ? find_objective(*quest, request.objective_id) : nullptr;
        if (objective == nullptr) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_objective,
                                          .quest_id = request.quest_id,
                                          .objective_id = request.objective_id});
            return result;
        }
        if (state_has_completed_objective(state, request.quest_id, request.objective_id)) {
            row.status = RuntimeQuestDialogueTransitionStatus::ignored;
            add_transition_row(result.rows, std::move(row));
            return result;
        }
        if (!prerequisites_satisfied(state, quest->prerequisites) ||
            !prerequisites_satisfied(state, objective->prerequisites)) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::blocked;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::blocked_prerequisite,
                                          .quest_id = request.quest_id,
                                          .objective_id = request.objective_id});
            return result;
        }
        result.state.completed_objectives.push_back(
            RuntimeQuestDialogueObjectiveState{.quest_id = request.quest_id, .objective_id = request.objective_id});
        result.reward_ids = objective->reward_ids;
        if (all_quest_objectives_completed(*quest, result.state)) {
            result.reward_ids.insert(result.reward_ids.end(), quest->reward_ids.begin(), quest->reward_ids.end());
        }
        add_transition_row(result.rows, std::move(row));
        return result;
    }
    case RuntimeQuestDialogueTransitionKind::choose_dialogue: {
        RuntimeQuestDialogueTransitionRow row{
            .kind = request.kind,
            .status = RuntimeQuestDialogueTransitionStatus::accepted,
            .dialogue_id = request.dialogue_id,
            .dialogue_choice_id = request.dialogue_choice_id,
        };
        const auto* const dialogue =
            is_safe_token(request.dialogue_id) ? find_dialogue(document, request.dialogue_id) : nullptr;
        if (dialogue == nullptr) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_dialogue,
                                          .dialogue_id = request.dialogue_id,
                                          .dialogue_choice_id = request.dialogue_choice_id});
            return result;
        }
        const auto* const active_node = find_dialogue_node_state(state, request.dialogue_id);
        const std::string_view node_id =
            active_node == nullptr ? std::string_view{dialogue->root_node_id} : std::string_view{active_node->node_id};
        row.dialogue_node_id = std::string{node_id};
        const auto* const node = find_dialogue_node(*dialogue, node_id);
        if (node == nullptr) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_dialogue_node,
                                          .dialogue_id = request.dialogue_id,
                                          .dialogue_node_id = std::string{node_id},
                                          .dialogue_choice_id = request.dialogue_choice_id});
            return result;
        }
        const auto* const choice = is_safe_token(request.dialogue_choice_id)
                                       ? find_dialogue_choice(*node, request.dialogue_choice_id)
                                       : nullptr;
        if (choice == nullptr) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::invalid;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_dialogue_choice,
                                          .dialogue_id = request.dialogue_id,
                                          .dialogue_node_id = std::string{node_id},
                                          .dialogue_choice_id = request.dialogue_choice_id});
            return result;
        }
        if (!prerequisites_satisfied(state, choice->prerequisites)) {
            result.succeeded = false;
            row.status = RuntimeQuestDialogueTransitionStatus::blocked;
            add_transition_row(result.rows, std::move(row));
            add_transition_diagnostic(result.diagnostics,
                                      RuntimeQuestDialogueTransitionDiagnostic{
                                          .code = RuntimeQuestDialogueTransitionDiagnosticCode::blocked_prerequisite,
                                          .dialogue_id = request.dialogue_id,
                                          .dialogue_node_id = std::string{node_id},
                                          .dialogue_choice_id = request.dialogue_choice_id});
            return result;
        }
        if (!choice->next_node_id.empty()) {
            const auto* const next_node = find_dialogue_node(*dialogue, choice->next_node_id);
            if (next_node == nullptr) {
                result.succeeded = false;
                row.status = RuntimeQuestDialogueTransitionStatus::invalid;
                row.referenced_dialogue_node_id = choice->next_node_id;
                add_transition_row(result.rows, std::move(row));
                add_transition_diagnostic(
                    result.diagnostics, RuntimeQuestDialogueTransitionDiagnostic{
                                            .code = RuntimeQuestDialogueTransitionDiagnosticCode::missing_dialogue_node,
                                            .dialogue_id = request.dialogue_id,
                                            .dialogue_node_id = std::string{node_id},
                                            .dialogue_choice_id = request.dialogue_choice_id});
                return result;
            }
            row.referenced_dialogue_node_id = next_node->id;
            set_dialogue_node_state(result.state, request.dialogue_id, next_node->id);
            result.action_ids = choice->action_ids;
            result.action_ids.insert(result.action_ids.end(), next_node->action_ids.begin(),
                                     next_node->action_ids.end());
        } else {
            result.action_ids = choice->action_ids;
        }
        add_transition_row(result.rows, std::move(row));
        return result;
    }
    }

    result.succeeded = false;
    add_transition_row(result.rows,
                       RuntimeQuestDialogueTransitionRow{.kind = request.kind,
                                                         .status = RuntimeQuestDialogueTransitionStatus::invalid,
                                                         .quest_id = request.quest_id,
                                                         .objective_id = request.objective_id,
                                                         .dialogue_id = request.dialogue_id,
                                                         .dialogue_choice_id = request.dialogue_choice_id,
                                                         .flag_id = request.flag_id});
    add_transition_diagnostic(
        result.diagnostics,
        RuntimeQuestDialogueTransitionDiagnostic{.code = RuntimeQuestDialogueTransitionDiagnosticCode::invalid_request,
                                                 .quest_id = request.quest_id,
                                                 .objective_id = request.objective_id,
                                                 .dialogue_id = request.dialogue_id,
                                                 .dialogue_choice_id = request.dialogue_choice_id,
                                                 .flag_id = request.flag_id});
    return result;
}

} // namespace mirakana::runtime
