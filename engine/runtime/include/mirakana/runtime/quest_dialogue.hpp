// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeQuestPrerequisiteKind : std::uint8_t {
    flag_set,
    objective_completed,
};

struct RuntimeQuestPrerequisite {
    RuntimeQuestPrerequisiteKind kind{RuntimeQuestPrerequisiteKind::flag_set};
    std::string quest_id;
    std::string objective_id;
    std::string flag_id;

    [[nodiscard]] bool operator==(const RuntimeQuestPrerequisite&) const = default;
};

struct RuntimeQuestObjectiveDesc {
    std::string id;
    std::string localization_key;
    std::vector<RuntimeQuestPrerequisite> prerequisites;
    std::vector<std::string> reward_ids;

    [[nodiscard]] bool operator==(const RuntimeQuestObjectiveDesc&) const = default;
};

struct RuntimeQuestDesc {
    std::string id;
    std::string title_localization_key;
    std::vector<RuntimeQuestPrerequisite> prerequisites;
    std::vector<RuntimeQuestObjectiveDesc> objectives;
    std::vector<std::string> reward_ids;

    [[nodiscard]] bool operator==(const RuntimeQuestDesc&) const = default;
};

struct RuntimeDialogueChoiceDesc {
    std::string id;
    std::string localization_key;
    std::string next_node_id;
    std::vector<RuntimeQuestPrerequisite> prerequisites;
    std::vector<std::string> action_ids;

    [[nodiscard]] bool operator==(const RuntimeDialogueChoiceDesc&) const = default;
};

struct RuntimeDialogueNodeDesc {
    std::string id;
    std::string localization_key;
    std::vector<RuntimeDialogueChoiceDesc> choices;
    std::vector<std::string> action_ids;

    [[nodiscard]] bool operator==(const RuntimeDialogueNodeDesc&) const = default;
};

struct RuntimeDialogueGraphDesc {
    std::string id;
    std::string root_node_id;
    std::vector<RuntimeDialogueNodeDesc> nodes;

    [[nodiscard]] bool operator==(const RuntimeDialogueGraphDesc&) const = default;
};

struct RuntimeQuestDialogueDocument {
    std::vector<std::string> flags;
    std::vector<RuntimeQuestDesc> quests;
    std::vector<RuntimeDialogueGraphDesc> dialogues;

    [[nodiscard]] bool operator==(const RuntimeQuestDialogueDocument&) const = default;
};

struct RuntimeQuestDialogueValidationContext {
    std::span<const std::string> localization_keys;
    std::span<const std::string> supported_reward_ids;
    std::span<const std::string> supported_action_ids;
};

enum class RuntimeQuestDialogueDiagnosticCode : std::uint8_t {
    none,
    invalid_flag_id,
    duplicate_flag_id,
    invalid_quest_id,
    duplicate_quest_id,
    invalid_objective_id,
    duplicate_objective_id,
    missing_objective_reference,
    invalid_prerequisite,
    unsupported_reward_id,
    unsafe_localization_key,
    missing_localization_key,
    invalid_dialogue_id,
    duplicate_dialogue_id,
    invalid_dialogue_node_id,
    duplicate_dialogue_node_id,
    missing_dialogue_node_reference,
    invalid_dialogue_choice_id,
    duplicate_dialogue_choice_id,
    unsupported_action_id,
};

struct RuntimeQuestDialogueDiagnostic {
    RuntimeQuestDialogueDiagnosticCode code{RuntimeQuestDialogueDiagnosticCode::none};
    std::string quest_id;
    std::string objective_id;
    std::string dialogue_id;
    std::string dialogue_node_id;
    std::string dialogue_choice_id;
    std::string flag_id;
    std::string referenced_quest_id;
    std::string referenced_objective_id;
    std::string referenced_dialogue_node_id;
    std::string reward_id;
    std::string action_id;
    std::string localization_key;

    [[nodiscard]] bool operator==(const RuntimeQuestDialogueDiagnostic&) const = default;
};

enum class RuntimeQuestDialogueValidationRowKind : std::uint8_t {
    flag,
    quest,
    objective,
    dialogue,
    dialogue_node,
};

struct RuntimeQuestDialogueValidationRow {
    RuntimeQuestDialogueValidationRowKind kind{RuntimeQuestDialogueValidationRowKind::flag};
    std::string parent_id;
    std::string id;

    [[nodiscard]] bool operator==(const RuntimeQuestDialogueValidationRow&) const = default;
};

struct RuntimeQuestDialogueValidationResult {
    bool succeeded{true};
    std::vector<RuntimeQuestDialogueDiagnostic> diagnostics;
    std::vector<RuntimeQuestDialogueValidationRow> rows;
};

[[nodiscard]] RuntimeQuestDialogueValidationResult
validate_runtime_quest_dialogue_document(const RuntimeQuestDialogueDocument& document,
                                         RuntimeQuestDialogueValidationContext context);

} // namespace mirakana::runtime
