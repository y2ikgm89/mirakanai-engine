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

[[nodiscard]] const RuntimeDialogueNodeDesc* find_dialogue_node(const RuntimeDialogueGraphDesc& dialogue,
                                                                const std::string_view node_id) noexcept {
    const auto match = std::ranges::find_if(
        dialogue.nodes, [node_id](const RuntimeDialogueNodeDesc& node) { return node.id == node_id; });
    return match == dialogue.nodes.end() ? nullptr : std::to_address(match);
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

} // namespace mirakana::runtime
