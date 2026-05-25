// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/genre_rpg_systems.hpp"

#include <algorithm>
#include <array>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

struct RuntimeRpgValidatedIds {
    std::vector<std::string> actor_ids;
    std::vector<std::string> party_ids;
    std::vector<std::string> enemy_ids;
};

struct RuntimeRpgStatLookupRow {
    std::string entity_id;
    std::string stat_id;
    std::int32_t current_value{0};
};

struct RuntimeRpgProgressionLookupRow {
    std::string entity_id;
    std::string track_id;
    std::uint32_t level{1U};
};

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_backend_token(std::string_view token) {
    constexpr auto forbidden = std::array<std::string_view, 10U>{
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "sdl", "sdl3", "imgui", "gpu",
    };
    return std::ranges::find(forbidden, token) != forbidden.end();
}

[[nodiscard]] bool has_backend_reference(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_backend_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_backend_token(token);
}

[[nodiscard]] bool contains_value(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::any_of(values, [value](const auto& candidate) { return candidate == value; });
}

[[nodiscard]] std::string make_key(std::string_view first, std::string_view second) {
    std::string key;
    key.reserve(first.size() + second.size() + 1U);
    key.append(first);
    key.push_back('\n');
    key.append(second);
    return key;
}

[[nodiscard]] std::string make_key(std::string_view first, std::string_view second, std::string_view third) {
    std::string key;
    key.reserve(first.size() + second.size() + third.size() + 2U);
    key.append(first);
    key.push_back('\n');
    key.append(second);
    key.push_back('\n');
    key.append(third);
    return key;
}

void add_diagnostic(RuntimeRpgSystemsPlan& plan, RuntimeRpgDiagnosticCode code, const RuntimeRpgSystemsRequest& request,
                    RuntimeWorldEntityId entity_id, std::string row_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeRpgDiagnostic{
        .code = code,
        .system_id = request.system_id,
        .world_id = request.world_id,
        .entity_id = std::move(entity_id),
        .row_id = std::move(row_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeRpgSystemsPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        return lhs.source_index < rhs.source_index;
    });
}

[[nodiscard]] std::size_t request_row_count(const RuntimeRpgSystemsRequest& request) {
    return request.party_entity_ids.size() + request.enemy_entity_ids.size() + request.stat_rows.size() +
           request.progression_rows.size() + request.skill_rows.size() + request.equipment_rows.size() +
           request.reward_rows.size() + request.save_validation_rows.size();
}

void validate_top_level(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request) {
    if (!is_valid_id(request.system_id)) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::missing_system_id, request, {}, request.system_id,
                       "runtime RPG systems id must be non-empty and path-safe", 0U);
    }
    if (!is_valid_id(request.world_id)) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::missing_world_id, request, {}, request.world_id,
                       "runtime RPG world id must be non-empty and path-safe", 0U);
    }
    if (has_backend_reference(request.system_id) || has_backend_reference(request.world_id)) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::unsupported_backend_reference, request, {}, request.system_id,
                       "runtime RPG systems rows must not encode renderer/platform/backend references", 0U);
    }
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::row_budget_exceeded, request, {}, request.system_id,
                       "runtime RPG systems request exceeds its review row budget", 0U);
    }
}

void validate_actor_list(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                         RuntimeRpgValidatedIds& ids, const std::vector<RuntimeWorldEntityId>& actors,
                         RuntimeRpgCombatSide side) {
    for (std::size_t index = 0U; index < actors.size(); ++index) {
        const auto& actor = actors[index];
        const auto source_index = static_cast<std::uint32_t>(index);
        if (!is_valid_id(actor.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::missing_actor_id, request, actor, actor.value,
                           "runtime RPG actor entity id must be non-empty and path-safe", source_index);
            continue;
        }
        if (contains_value(ids.actor_ids, actor.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_actor_id, request, actor, actor.value,
                           "runtime RPG actor entity ids must be unique across party and enemy lists", source_index);
            continue;
        }
        ids.actor_ids.push_back(actor.value);
        if (side == RuntimeRpgCombatSide::party) {
            ids.party_ids.push_back(actor.value);
        } else {
            ids.enemy_ids.push_back(actor.value);
        }
    }
}

[[nodiscard]] RuntimeRpgValidatedIds validate_actor_ids(RuntimeRpgSystemsPlan& plan,
                                                        const RuntimeRpgSystemsRequest& request) {
    RuntimeRpgValidatedIds ids;
    ids.actor_ids.reserve(request.party_entity_ids.size() + request.enemy_entity_ids.size());
    ids.party_ids.reserve(request.party_entity_ids.size());
    ids.enemy_ids.reserve(request.enemy_entity_ids.size());
    validate_actor_list(plan, request, ids, request.party_entity_ids, RuntimeRpgCombatSide::party);
    validate_actor_list(plan, request, ids, request.enemy_entity_ids, RuntimeRpgCombatSide::enemy);
    return ids;
}

[[nodiscard]] bool is_known_actor(const RuntimeRpgValidatedIds& ids, std::string_view entity_id) {
    return contains_value(ids.actor_ids, entity_id);
}

[[nodiscard]] std::vector<RuntimeRpgStatLookupRow> validate_stats(RuntimeRpgSystemsPlan& plan,
                                                                  const RuntimeRpgSystemsRequest& request,
                                                                  const RuntimeRpgValidatedIds& ids) {
    std::vector<std::string> keys;
    std::vector<RuntimeRpgStatLookupRow> stats;
    keys.reserve(request.stat_rows.size());
    stats.reserve(request.stat_rows.size());
    for (const auto& row : request.stat_rows) {
        const auto row_key = make_key(row.entity_id.value, row.stat_id);
        auto row_valid = true;
        if (!is_valid_id(row.stat_id) || row.max_value <= 0 || row.current_value < 0 ||
            row.current_value > row.max_value) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_stat_row, request, row.entity_id, row.stat_id,
                           "runtime RPG stat rows require a path-safe stat id and 0 <= current <= max",
                           row.source_index);
            row_valid = false;
        }
        if (contains_value(keys, row_key)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_stat_row, request, row.entity_id, row.stat_id,
                           "runtime RPG stat rows must be unique per entity and stat id", row.source_index);
            row_valid = false;
        } else {
            keys.push_back(row_key);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_stat_actor, request, row.entity_id, row.stat_id,
                           "runtime RPG stat row references an actor outside the request", row.source_index);
            row_valid = false;
        }
        if (row_valid) {
            stats.push_back(RuntimeRpgStatLookupRow{
                .entity_id = row.entity_id.value,
                .stat_id = row.stat_id,
                .current_value = row.current_value,
            });
        }
    }
    return stats;
}

[[nodiscard]] std::vector<RuntimeRpgProgressionLookupRow> validate_progression(RuntimeRpgSystemsPlan& plan,
                                                                               const RuntimeRpgSystemsRequest& request,
                                                                               const RuntimeRpgValidatedIds& ids) {
    std::vector<std::string> keys;
    std::vector<RuntimeRpgProgressionLookupRow> progression;
    keys.reserve(request.progression_rows.size());
    progression.reserve(request.progression_rows.size());
    for (const auto& row : request.progression_rows) {
        const auto row_key = make_key(row.entity_id.value, row.track_id);
        auto row_valid = true;
        if (!is_valid_id(row.track_id) || row.level == 0U) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_progression_row, request, row.entity_id,
                           row.track_id, "runtime RPG progression rows require a path-safe track id and level > 0",
                           row.source_index);
            row_valid = false;
        }
        if (contains_value(keys, row_key)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_progression_row, request, row.entity_id,
                           row.track_id, "runtime RPG progression rows must be unique per entity and track",
                           row.source_index);
            row_valid = false;
        } else {
            keys.push_back(row_key);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_progression_actor, request, row.entity_id,
                           row.track_id, "runtime RPG progression row references an actor outside the request",
                           row.source_index);
            row_valid = false;
        }
        if (row_valid) {
            progression.push_back(RuntimeRpgProgressionLookupRow{
                .entity_id = row.entity_id.value,
                .track_id = row.track_id,
                .level = row.level,
            });
        }
    }
    return progression;
}

[[nodiscard]] const RuntimeRpgStatLookupRow* find_stat(const std::vector<RuntimeRpgStatLookupRow>& stats,
                                                       std::string_view entity_id, std::string_view stat_id) {
    const auto found = std::ranges::find_if(
        stats, [entity_id, stat_id](const auto& row) { return row.entity_id == entity_id && row.stat_id == stat_id; });
    return found == stats.end() ? nullptr : &(*found);
}

[[nodiscard]] std::uint32_t level_for(const std::vector<RuntimeRpgProgressionLookupRow>& progression,
                                      std::string_view entity_id) {
    auto level = std::uint32_t{1U};
    for (const auto& row : progression) {
        if (row.entity_id == entity_id) {
            level = std::max(level, row.level);
        }
    }
    return level;
}

void validate_skills(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                     const RuntimeRpgValidatedIds& ids, const std::vector<RuntimeRpgStatLookupRow>& stats) {
    std::vector<std::string> keys;
    keys.reserve(request.skill_rows.size());
    for (const auto& row : request.skill_rows) {
        const auto row_key = make_key(row.entity_id.value, row.skill_id);
        if (!is_valid_id(row.skill_id) || row.required_level == 0U || row.required_stat_value < 0) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_skill_row, request, row.entity_id, row.skill_id,
                           "runtime RPG skill rows require path-safe ids, level > 0, and non-negative requirements",
                           row.source_index);
        }
        if (contains_value(keys, row_key)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_skill_id, request, row.entity_id, row.skill_id,
                           "runtime RPG skill rows must be unique per entity and skill id", row.source_index);
        } else {
            keys.push_back(row_key);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_skill_actor, request, row.entity_id, row.skill_id,
                           "runtime RPG skill row references an actor outside the request", row.source_index);
        }
        if (!row.required_stat_id.empty() && find_stat(stats, row.entity_id.value, row.required_stat_id) == nullptr) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_skill_stat, request, row.entity_id,
                           row.required_stat_id, "runtime RPG skill row requires a stat not present for the same actor",
                           row.source_index);
        }
    }
}

void validate_equipment(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                        const RuntimeRpgValidatedIds& ids, const std::vector<RuntimeRpgStatLookupRow>& stats) {
    std::vector<std::string> keys;
    keys.reserve(request.equipment_rows.size());
    for (const auto& row : request.equipment_rows) {
        const auto row_key = make_key(row.entity_id.value, row.slot_id);
        if (!is_valid_id(row.slot_id) || !is_valid_id(row.item_id) || row.required_level == 0U ||
            row.required_stat_value < 0) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_equipment_row, request, row.entity_id, row.slot_id,
                           "runtime RPG equipment rows require path-safe ids, level > 0, and non-negative requirements",
                           row.source_index);
        }
        if (contains_value(keys, row_key)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_equipment_slot, request, row.entity_id,
                           row.slot_id, "runtime RPG equipment rows must be unique per entity and slot",
                           row.source_index);
        } else {
            keys.push_back(row_key);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_equipment_actor, request, row.entity_id, row.slot_id,
                           "runtime RPG equipment row references an actor outside the request", row.source_index);
        }
        if (!row.required_stat_id.empty() && find_stat(stats, row.entity_id.value, row.required_stat_id) == nullptr) {
            add_diagnostic(
                plan, RuntimeRpgDiagnosticCode::unknown_equipment_stat, request, row.entity_id, row.required_stat_id,
                "runtime RPG equipment row requires a stat not present for the same actor", row.source_index);
        }
    }
}

void validate_combat(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                     const RuntimeRpgValidatedIds& ids, const std::vector<RuntimeRpgStatLookupRow>& stats) {
    if (request.combat_request.max_rounds == 0U) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_combat_rounds, request, {},
                       request.combat_request.encounter_id, "runtime RPG combat loop requests require max_rounds > 0",
                       0U);
    }
    if (!is_valid_id(request.combat_request.encounter_id) || !is_valid_id(request.combat_request.initiative_stat_id)) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_combat_participant, request, {},
                       request.combat_request.encounter_id,
                       "runtime RPG combat loop requests require path-safe encounter and initiative ids", 0U);
        return;
    }

    std::vector<std::string> turn_actor_ids;
    turn_actor_ids.reserve(ids.party_ids.size() + ids.enemy_ids.size());
    for (const auto& entity_id : ids.party_ids) {
        if (contains_value(turn_actor_ids, entity_id)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_combat_turn_actor, request,
                           RuntimeWorldEntityId{.value = entity_id}, request.combat_request.encounter_id,
                           "runtime RPG combat turn actor ids must be unique", 0U);
        } else {
            turn_actor_ids.push_back(entity_id);
        }
        if (find_stat(stats, entity_id, request.combat_request.initiative_stat_id) == nullptr) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_combat_participant, request,
                           RuntimeWorldEntityId{.value = entity_id}, request.combat_request.initiative_stat_id,
                           "runtime RPG combat participant requires the initiative stat", 0U);
        }
    }
    for (const auto& entity_id : ids.enemy_ids) {
        if (contains_value(turn_actor_ids, entity_id)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_combat_turn_actor, request,
                           RuntimeWorldEntityId{.value = entity_id}, request.combat_request.encounter_id,
                           "runtime RPG combat turn actor ids must be unique", 0U);
        } else {
            turn_actor_ids.push_back(entity_id);
        }
        if (find_stat(stats, entity_id, request.combat_request.initiative_stat_id) == nullptr) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_combat_participant, request,
                           RuntimeWorldEntityId{.value = entity_id}, request.combat_request.initiative_stat_id,
                           "runtime RPG combat participant requires the initiative stat", 0U);
        }
    }
}

void validate_rewards(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                      const RuntimeRpgValidatedIds& ids) {
    std::vector<std::string> reward_ids;
    reward_ids.reserve(request.reward_rows.size());
    for (const auto& row : request.reward_rows) {
        if (!is_valid_id(row.reward_id)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_reward_row, request, row.entity_id, row.reward_id,
                           "runtime RPG reward rows require a path-safe reward id", row.source_index);
        }
        if (contains_value(reward_ids, row.reward_id)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_reward_id, request, row.entity_id, row.reward_id,
                           "runtime RPG reward ids must be unique in a request", row.source_index);
        } else {
            reward_ids.push_back(row.reward_id);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_reward_actor, request, row.entity_id, row.reward_id,
                           "runtime RPG reward row references an actor outside the request", row.source_index);
        }
        if (row.quantity == 0U) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_reward_quantity, request, row.entity_id,
                           row.reward_id, "runtime RPG reward quantity must be non-zero", row.source_index);
        }
    }
}

void validate_saves(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                    const RuntimeRpgValidatedIds& ids) {
    std::vector<std::string> keys;
    keys.reserve(request.save_validation_rows.size());
    for (const auto& row : request.save_validation_rows) {
        const auto row_key = make_key(row.domain, row.key);
        if (!is_valid_id(row.domain) || !is_valid_id(row.key) || row.expected_schema_version == 0U ||
            row.observed_schema_version == 0U) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::invalid_save_row, request, row.entity_id, row.key,
                           "runtime RPG save validation rows require path-safe ids and non-zero schema versions",
                           row.source_index);
        }
        if (contains_value(keys, row_key)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::duplicate_save_key, request, row.entity_id, row.key,
                           "runtime RPG save validation keys must be unique per domain", row.source_index);
        } else {
            keys.push_back(row_key);
        }
        if (!is_known_actor(ids, row.entity_id.value)) {
            add_diagnostic(plan, RuntimeRpgDiagnosticCode::unknown_save_actor, request, row.entity_id, row.key,
                           "runtime RPG save validation row references an actor outside the request", row.source_index);
        }
    }
}

void sort_plan_rows(RuntimeRpgSystemsPlan& plan) {
    std::ranges::sort(plan.stat_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        return lhs.stat_id < rhs.stat_id;
    });
    std::ranges::sort(plan.progression_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        return lhs.track_id < rhs.track_id;
    });
    std::ranges::sort(plan.skill_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        return lhs.skill_id < rhs.skill_id;
    });
    std::ranges::sort(plan.equipment_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.slot_id != rhs.slot_id) {
            return lhs.slot_id < rhs.slot_id;
        }
        return lhs.item_id < rhs.item_id;
    });
    std::ranges::sort(plan.combat_loop_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.round_index != rhs.round_index) {
            return lhs.round_index < rhs.round_index;
        }
        if (lhs.initiative_value != rhs.initiative_value) {
            return lhs.initiative_value > rhs.initiative_value;
        }
        return lhs.entity_id.value < rhs.entity_id.value;
    });
    std::ranges::sort(plan.reward_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.kind != rhs.kind) {
            return static_cast<std::uint8_t>(lhs.kind) < static_cast<std::uint8_t>(rhs.kind);
        }
        return lhs.reward_id < rhs.reward_id;
    });
    std::ranges::sort(plan.save_validation_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.entity_id.value != rhs.entity_id.value) {
            return lhs.entity_id.value < rhs.entity_id.value;
        }
        if (lhs.domain != rhs.domain) {
            return lhs.domain < rhs.domain;
        }
        return lhs.key < rhs.key;
    });
}

void assign_combat_turn_indices(RuntimeRpgSystemsPlan& plan) {
    auto current_round = std::uint32_t{0U};
    auto turn_index = std::uint32_t{0U};
    auto first = true;
    for (auto& row : plan.combat_loop_rows) {
        if (first || row.round_index != current_round) {
            current_round = row.round_index;
            turn_index = 0U;
            first = false;
        }
        row.turn_index = turn_index;
        ++turn_index;
    }
}

[[nodiscard]] RuntimeRpgSkillStatus skill_status(const RuntimeRpgSkillRow& row,
                                                 const std::vector<RuntimeRpgStatLookupRow>& stats,
                                                 const std::vector<RuntimeRpgProgressionLookupRow>& progression) {
    const auto* required_stat =
        row.required_stat_id.empty() ? nullptr : find_stat(stats, row.entity_id.value, row.required_stat_id);
    if (required_stat != nullptr && required_stat->current_value < row.required_stat_value) {
        return RuntimeRpgSkillStatus::blocked_prerequisite;
    }
    if (!row.required_stat_id.empty() && required_stat == nullptr) {
        return RuntimeRpgSkillStatus::invalid;
    }
    if (level_for(progression, row.entity_id.value) < row.required_level) {
        return RuntimeRpgSkillStatus::blocked_progression;
    }
    return RuntimeRpgSkillStatus::unlocked;
}

[[nodiscard]] RuntimeRpgEquipmentStatus
equipment_status(const RuntimeRpgEquipmentRow& row, const std::vector<RuntimeRpgStatLookupRow>& stats,
                 const std::vector<RuntimeRpgProgressionLookupRow>& progression) {
    const auto* required_stat =
        row.required_stat_id.empty() ? nullptr : find_stat(stats, row.entity_id.value, row.required_stat_id);
    if (required_stat != nullptr && required_stat->current_value < row.required_stat_value) {
        return RuntimeRpgEquipmentStatus::blocked_requirement;
    }
    if (!row.required_stat_id.empty() && required_stat == nullptr) {
        return RuntimeRpgEquipmentStatus::invalid;
    }
    if (level_for(progression, row.entity_id.value) < row.required_level) {
        return RuntimeRpgEquipmentStatus::blocked_requirement;
    }
    return RuntimeRpgEquipmentStatus::equipped;
}

[[nodiscard]] RuntimeRpgSaveValidationStatus save_status(const RuntimeRpgSaveValidationRow& row) {
    if (row.observed_schema_version == row.expected_schema_version) {
        return RuntimeRpgSaveValidationStatus::accepted;
    }
    if (row.observed_schema_version < row.expected_schema_version) {
        return RuntimeRpgSaveValidationStatus::repairable;
    }
    return RuntimeRpgSaveValidationStatus::rejected;
}

void copy_value_rows(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                     const std::vector<RuntimeRpgStatLookupRow>& stats,
                     const std::vector<RuntimeRpgProgressionLookupRow>& progression) {
    plan.stat_rows = request.stat_rows;
    plan.progression_rows = request.progression_rows;
    plan.skill_rows = request.skill_rows;
    plan.equipment_rows = request.equipment_rows;
    plan.reward_rows = request.reward_rows;
    plan.save_validation_rows = request.save_validation_rows;

    for (auto& row : plan.skill_rows) {
        row.status = skill_status(row, stats, progression);
        if (row.status == RuntimeRpgSkillStatus::unlocked) {
            ++plan.unlocked_skill_count;
        } else {
            ++plan.blocked_skill_count;
        }
    }
    for (auto& row : plan.equipment_rows) {
        row.status = equipment_status(row, stats, progression);
        if (row.status == RuntimeRpgEquipmentStatus::equipped) {
            ++plan.equipped_item_count;
        } else {
            ++plan.blocked_equipment_count;
        }
    }
    for (auto& row : plan.save_validation_rows) {
        row.status = save_status(row);
        if (row.status == RuntimeRpgSaveValidationStatus::repairable) {
            ++plan.repairable_save_validation_count;
        }
    }
}

void append_combat_rows_for_side(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                                 const std::vector<std::string>& entity_ids, RuntimeRpgCombatSide side,
                                 const std::vector<RuntimeRpgStatLookupRow>& stats) {
    for (std::uint32_t round_index = 0U; round_index < request.combat_request.max_rounds; ++round_index) {
        for (std::size_t entity_index = 0U; entity_index < entity_ids.size(); ++entity_index) {
            const auto& entity_id = entity_ids[entity_index];
            const auto* initiative = find_stat(stats, entity_id, request.combat_request.initiative_stat_id);
            plan.combat_loop_rows.push_back(RuntimeRpgCombatLoopRow{
                .encounter_id = request.combat_request.encounter_id,
                .round_index = round_index,
                .turn_index = 0U,
                .entity_id = RuntimeWorldEntityId{.value = entity_id},
                .side = side,
                .initiative_value = initiative == nullptr ? 0 : initiative->current_value,
                .action_kind = RuntimeRpgCombatActionKind::act,
                .source_index = static_cast<std::uint32_t>(entity_index),
            });
        }
    }
}

void append_combat_rows(RuntimeRpgSystemsPlan& plan, const RuntimeRpgSystemsRequest& request,
                        const RuntimeRpgValidatedIds& ids, const std::vector<RuntimeRpgStatLookupRow>& stats) {
    append_combat_rows_for_side(plan, request, ids.party_ids, RuntimeRpgCombatSide::party, stats);
    append_combat_rows_for_side(plan, request, ids.enemy_ids, RuntimeRpgCombatSide::enemy, stats);
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

[[nodiscard]] std::uint64_t hash_signed(std::int32_t value) noexcept {
    return static_cast<std::uint64_t>(static_cast<std::int64_t>(value));
}

template <typename Enum> [[nodiscard]] std::uint64_t hash_enum(Enum value) noexcept {
    return static_cast<std::uint64_t>(static_cast<std::uint8_t>(value));
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeRpgSystemsRequest& request,
                                                const RuntimeRpgSystemsPlan& plan) {
    auto hash = std::uint64_t{14695981039346656037ULL};
    hash = mix_hash(hash, hash_string(request.system_id));
    hash = mix_hash(hash, hash_string(request.world_id));
    hash = mix_hash(hash, request.world_tick);
    hash = mix_hash(hash, request.seed);
    hash = mix_hash(hash, hash_enum(plan.status));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.party_member_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.enemy_member_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.unlocked_skill_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.blocked_skill_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.equipped_item_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.blocked_equipment_count));
    hash = mix_hash(hash, static_cast<std::uint64_t>(plan.repairable_save_validation_count));
    for (const auto& row : plan.stat_rows) {
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_string(row.stat_id));
        hash = mix_hash(hash, hash_signed(row.current_value));
        hash = mix_hash(hash, hash_signed(row.max_value));
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.progression_rows) {
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_string(row.track_id));
        hash = mix_hash(hash, row.level);
        hash = mix_hash(hash, row.experience);
        hash = mix_hash(hash, row.skill_points);
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.skill_rows) {
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_string(row.skill_id));
        hash = mix_hash(hash, row.required_level);
        hash = mix_hash(hash, hash_string(row.required_stat_id));
        hash = mix_hash(hash, hash_signed(row.required_stat_value));
        hash = mix_hash(hash, hash_enum(row.status));
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.equipment_rows) {
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_string(row.slot_id));
        hash = mix_hash(hash, hash_string(row.item_id));
        hash = mix_hash(hash, row.required_level);
        hash = mix_hash(hash, hash_string(row.required_stat_id));
        hash = mix_hash(hash, hash_signed(row.required_stat_value));
        hash = mix_hash(hash, hash_enum(row.status));
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.combat_loop_rows) {
        hash = mix_hash(hash, hash_string(row.encounter_id));
        hash = mix_hash(hash, row.round_index);
        hash = mix_hash(hash, row.turn_index);
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_enum(row.side));
        hash = mix_hash(hash, hash_signed(row.initiative_value));
        hash = mix_hash(hash, hash_enum(row.action_kind));
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.reward_rows) {
        hash = mix_hash(hash, hash_string(row.reward_id));
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_enum(row.kind));
        hash = mix_hash(hash, hash_string(row.item_id));
        hash = mix_hash(hash, row.quantity);
        hash = mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.save_validation_rows) {
        hash = mix_hash(hash, hash_string(row.entity_id.value));
        hash = mix_hash(hash, hash_string(row.domain));
        hash = mix_hash(hash, hash_string(row.key));
        hash = mix_hash(hash, row.expected_schema_version);
        hash = mix_hash(hash, row.observed_schema_version);
        hash = mix_hash(hash, hash_enum(row.status));
        hash = mix_hash(hash, row.source_index);
    }
    return hash == 0U ? 1U : hash;
}

[[nodiscard]] std::size_t output_row_count(const RuntimeRpgSystemsPlan& plan) {
    return plan.stat_rows.size() + plan.progression_rows.size() + plan.skill_rows.size() + plan.equipment_rows.size() +
           plan.combat_loop_rows.size() + plan.reward_rows.size() + plan.save_validation_rows.size();
}

void clear_output_rows(RuntimeRpgSystemsPlan& plan) {
    plan.stat_rows.clear();
    plan.progression_rows.clear();
    plan.skill_rows.clear();
    plan.equipment_rows.clear();
    plan.combat_loop_rows.clear();
    plan.reward_rows.clear();
    plan.save_validation_rows.clear();
    plan.party_member_count = 0U;
    plan.enemy_member_count = 0U;
    plan.stat_count = 0U;
    plan.progression_count = 0U;
    plan.unlocked_skill_count = 0U;
    plan.blocked_skill_count = 0U;
    plan.equipped_item_count = 0U;
    plan.blocked_equipment_count = 0U;
    plan.combat_turn_count = 0U;
    plan.combat_round_count = 0U;
    plan.reward_count = 0U;
    plan.save_validation_count = 0U;
    plan.repairable_save_validation_count = 0U;
    plan.replay_hash = 0U;
}

void update_counts(RuntimeRpgSystemsPlan& plan, const RuntimeRpgValidatedIds& ids) {
    plan.party_member_count = ids.party_ids.size();
    plan.enemy_member_count = ids.enemy_ids.size();
    plan.stat_count = plan.stat_rows.size();
    plan.progression_count = plan.progression_rows.size();
    plan.combat_turn_count = plan.combat_loop_rows.size();
    plan.reward_count = plan.reward_rows.size();
    plan.save_validation_count = plan.save_validation_rows.size();
    if (!plan.combat_loop_rows.empty()) {
        plan.combat_round_count = plan.combat_loop_rows.back().round_index + 1U;
    }
}

} // namespace

bool RuntimeRpgSystemsPlan::succeeded() const noexcept {
    return status == RuntimeRpgSystemsStatus::ready || status == RuntimeRpgSystemsStatus::no_rows;
}

RuntimeRpgSystemsPlan plan_runtime_rpg_systems(const RuntimeRpgSystemsRequest& request) {
    RuntimeRpgSystemsPlan plan;

    validate_top_level(plan, request);
    const auto ids = validate_actor_ids(plan, request);
    const auto stats = validate_stats(plan, request, ids);
    const auto progression = validate_progression(plan, request, ids);
    validate_skills(plan, request, ids, stats);
    validate_equipment(plan, request, ids, stats);
    validate_combat(plan, request, ids, stats);
    validate_rewards(plan, request, ids);
    validate_saves(plan, request, ids);

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeRpgSystemsStatus::invalid_request;
        return plan;
    }

    copy_value_rows(plan, request, stats, progression);
    append_combat_rows(plan, request, ids, stats);
    sort_plan_rows(plan);
    assign_combat_turn_indices(plan);
    update_counts(plan, ids);

    if (output_row_count(plan) > request.row_budget) {
        add_diagnostic(plan, RuntimeRpgDiagnosticCode::row_budget_exceeded, request, {}, request.system_id,
                       "runtime RPG systems output exceeds its review row budget", 0U);
        sort_diagnostics(plan);
        clear_output_rows(plan);
        plan.status = RuntimeRpgSystemsStatus::invalid_request;
        return plan;
    }

    plan.status = output_row_count(plan) == 0U ? RuntimeRpgSystemsStatus::no_rows : RuntimeRpgSystemsStatus::ready;
    plan.replay_hash = compute_replay_hash(request, plan);
    return plan;
}

} // namespace mirakana::runtime
